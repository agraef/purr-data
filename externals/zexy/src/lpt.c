/*
 * lpt: read/write the parallel port
 *
 * (c) 1999-2011 IOhannes m zmölnig, forum::für::umläute, institute of electronic music and acoustics (iem)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/*
   (c) 2000:forum::für::umläute:2005

   write to the parallel port
   extended to write to any port (if we do have permissions)

   2005-09-28: write to devices instead of hardware-addresses
     http://people.redhat.com/twaugh/parport/html/ppdev.html
     TODO: don't lock when multiple objects refer to the same device
     TODO: allow readonly/writeonly access
     TODO: test for timeouts,...

   thanks to
    Thomas Musil: adding "control-output" and "input"
*/
#define BASE0  0x3bc
#define BASE1  0x378
#define BASE2  0x278

#define MODE_IOPERM 1
#define MODE_IOPL   0
#define MODE_NONE   -1

#if defined __linux__
# ifndef Z_WANT_LPT
#  define Z_WANT_LPT 1
# endif
#elif defined __WIN32__
// ag: We don't have the requisite dlls, disabled for now.
#define Z_WANT_LPT 0
# if defined __i386__
#  ifndef Z_WANT_LPT
#   define Z_WANT_LPT 1
#  endif
#  define INPOUT_DLL "inpout32.dll"
# elif defined __x86_64__
#  ifndef Z_WANT_LPT
#   define Z_WANT_LPT 1
#  endif
#  define INPOUT_DLL "inpoutx64.dll"
# else
#  warning lpt-support on Windows requires InpOut32, which is only available for i386/amd64
# endif
#else
# warning no lpt-support for this OS
#endif

#ifndef Z_WANT_LPT
# define Z_WANT_LPT 0
#endif

#include "zexy.h"

/* ----------------------- lpt --------------------- */

#if Z_WANT_LPT
# include <stdlib.h>
# include <errno.h>

# if defined __WIN32__
#include "windows.h"
typedef void    (__stdcall *lpOut32)(short, short);
typedef short   (__stdcall *lpInp32)(short);
typedef BOOL    (__stdcall *lpIsInpOutDriverOpen)(void);
typedef BOOL    (__stdcall *lpIsXP64Bit)(void);

/* Some global function pointers (messy but fine for an example) */
lpOut32 gfpOut32;
lpInp32 gfpInp32;
lpIsInpOutDriverOpen gfpIsInpOutDriverOpen;
lpIsXP64Bit gfpIsXP64Bit;
/* the handle to the DLL, so we can close it once all [lpt]-objects have been deleted */
static HINSTANCE hInpOutDll;
/* reference count for the DLL-handle */
static int z_inpout32_refcount = 0;

static int z_getWinErr(char*outstr, size_t len)
{
  wchar_t*errstr;
  size_t i;
  DWORD err = GetLastError();
  outstr[0] = 0;
  if(!err) {
    return 0;
  }
  errstr=calloc(len, sizeof(wchar_t));
  FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errstr, len, NULL);
  for(i=0; i<len; i++) {
    wchar_t wc = errstr[i];
    char c = ((wc>=0) && (wc<128))?((char)(wc)):'?';
    if((10 == c) || (13 == c)) {
      c = ' ';
    }

    outstr[i] = c;
  }
  free(errstr);
  outstr[len-1] = 0;
  return err;
}
static void z_findfile(const char*filename, char*buf, size_t bufsize)
{
  char *bufptr;
  int fd = open_via_path(".", filename, "", buf, &bufptr, bufsize, 0);

  if (fd < 0) {
    snprintf(buf, bufsize-1, "%s", filename);
  } else {
    sys_close(fd);
    snprintf(buf, bufsize-1, "%s/%s", buf, bufptr);
  }
  buf[bufsize-1] = 0;

}

static int z_inpout32_ctor()
{
  if(!z_inpout32_refcount) {
    char filename[MAXPDSTRING];
    z_findfile(INPOUT_DLL, filename, MAXPDSTRING);

    SetLastError(0);
    hInpOutDll = LoadLibrary ( filename );
    if ( hInpOutDll == NULL )  {
      char errstring[MAXPDSTRING];
      int err = z_getWinErr(errstring, MAXPDSTRING);
      error("unable to open %s for accessing the parallel-port!", INPOUT_DLL);
      error("error[%d]: %s", err, errstring);
      error("make sure you have InpOut32 installed");
      error("--> http://www.highrez.co.uk/downloads/inpout32/");
      return 0;
    }
    gfpOut32 = (lpOut32)GetProcAddress(hInpOutDll, "Out32");
    gfpInp32 = (lpInp32)GetProcAddress(hInpOutDll, "Inp32");
    gfpIsInpOutDriverOpen = (lpIsInpOutDriverOpen)GetProcAddress(hInpOutDll,
                            "IsInpOutDriverOpen");
    gfpIsXP64Bit = (lpIsXP64Bit)GetProcAddress(hInpOutDll, "IsXP64Bit");
  }
  z_inpout32_refcount++;

  if (!gfpIsInpOutDriverOpen()) {
    error("unable to start InpOut32 driver!");
    return 0;
  }
  return z_inpout32_refcount;
}
static void z_inpout32_dtor()
{
  z_inpout32_refcount--;
  if(!z_inpout32_refcount) {
    gfpOut32 = NULL;
    gfpInp32 = NULL;
    gfpIsInpOutDriverOpen = NULL;
    gfpIsXP64Bit = NULL;
    FreeLibrary ( hInpOutDll );
  }
}
static void sys_outb(unsigned char byte, unsigned short int port)
{
  if(gfpOut32) {
    gfpOut32(port, byte);
  }
}
static int sys_inb(unsigned short int port)
{
  if(gfpInp32) {
    return gfpInp32(port);
  }
  return 0;
}

/* on windoze everything is so complicated... */
static int ioperm(unsigned short int UNUSED(port),
                  int UNUSED(a), int UNUSED(b))
{
  return(0);
}

static int iopl(int UNUSED(i))
{
  return(-1);
}

# elif defined (__linux__)
/* thankfully there is linux */
#  include <unistd.h>
#  include <sys/ioctl.h>
#  include <linux/ppdev.h>
#  include <linux/parport.h>
#  include <fcntl.h>
#  include <stdio.h>
#  include <sys/io.h>

static void sys_outb(unsigned char byte, unsigned short int port)
{
  outb(byte, port);
}
static int sys_inb(unsigned short int port)
{
  return inb(port);
}

# else
static void sys_outb(unsigned char UNUSED(byte),
                     unsigned short int UNUSED(port))
{}
static int sys_inb(unsigned short int UNUSED(port))
{
  return 0;
}
# endif /* OS */
#endif /* Z_WANT_LPT */


static int count_iopl = 0;
static t_class *lpt_class=NULL;

typedef struct _lpt {
  t_object x_obj;

  unsigned short int port;
  int device; /* file descriptor of device, in case we are using one ...*/

  int mode; /* MODE_IOPERM, MODE_IOPL */
} t_lpt;

#if Z_WANT_LPT
static void lpt_float(t_lpt *x, t_floatarg f)
{
  unsigned char b = f;
#ifdef __linux__
  if (x->device>0) {
    ioctl (x->device, PPWDATA, &b);
  } else
#endif
    if (x->port) {
      sys_outb(b, x->port+0);
    }
}

static void lpt_control(t_lpt *x, t_floatarg f)
{
  unsigned char b = f;
#ifdef __linux__
  if (x->device>0) {
    ioctl (x->device, PPWCONTROL, &b);
  } else
#endif
    if (x->port) {
      sys_outb(b, x->port+2);
    }
}

static void lpt_bang(t_lpt *x)
{
#ifdef __linux__
  if (x->device>0) {
    unsigned char b=0;
    ioctl (x->device, PPRCONTROL, &b);
    outlet_float(x->x_obj.ob_outlet, (t_float)b);
  } else
#endif
    if (x->port) {
      outlet_float(x->x_obj.ob_outlet, (t_float)sys_inb(x->port+1));
    }
}

static void lpt_free(t_lpt *x)
{
#ifdef __linux__
  if (x->device>0) {
    ioctl (x->device, PPRELEASE);
    sys_close(x->device);
    x->device=0;
  } else
#endif
    if (x->port) {
      if (x->mode==MODE_IOPERM && ioperm(x->port, 8, 0)) {
        error("lpt: couldn't clean up device");
      } else if (x->mode==MODE_IOPL && (!--count_iopl) && iopl(0)) {
        error("lpt: couldn't clean up device");
      }
    }
#ifdef __WIN32__
  z_inpout32_dtor();
#endif
}

#else
static void lpt_float(t_lpt *x, t_floatarg f) { ; }
static void lpt_control(t_lpt *x, t_floatarg f) { ; }
static void lpt_bang(t_lpt *x) { ; }
static void lpt_free(t_lpt *x) { ; }
#endif /*  Z_WANT_LPT */


static void *lpt_new(t_symbol *s, int argc, t_atom *argv)
{
  t_lpt *x = (t_lpt *)pd_new(lpt_class);
  const char*devname=0;
  long int hexport = 0;

  if(s==gensym("lp")) {
    error("lpt: the use of 'lp' has been deprecated; use 'lpt' instead");
  }


  inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("control"));
  outlet_new(&x->x_obj, gensym("float"));
  x->mode = MODE_NONE;
  x->port = 0;
  x->device = -1;

#if Z_WANT_LPT
  if ((argc==0)||(argv->a_type==A_FLOAT)) {
    /* FLOAT specifies a parallel port */
    switch ((int)((argc)?atom_getfloat(argv):0)) {
    case 0:
      x->port = BASE0;
      devname="lpt0";
      break;
    case 1:
      x->port = BASE1;
      devname="lpt1";
      break;
    case 2:
      x->port = BASE2;
      devname="lpt2";
      break;
    default:
      error("lpt : only lpt0, lpt1 and lpt2 are accessible");
      x->port = 0;
      return (x);
    }
  } else {
    /* SYMBOL might be a file or a hex port-number */
    devname=atom_getsymbol(argv)->s_name;
    x->device=-1;
    hexport=strtol(devname, 0, 16);
    if((hexport >= 0) && (hexport <= 0xFFFF)) {
      x->port = hexport;
    }
    if(0==x->port) {
#ifdef __linux__
      x->device = sys_open(devname, O_RDWR);
      if(x->device<=0) {
        error("lpt: bad device %s", devname);
        return(x);
      } else {
        if (ioctl (x->device, PPCLAIM)) {
          perror ("PPCLAIM");
          sys_close (x->device);
          x->device=-1;
        }
      }
#endif /* __linux__ */
    }
  }

  if ((x->device<0) && (!x->port)) {
    error("lpt : bad port %x", x->port);
    x->port = 0;
    return (x);
  }
  if (x->device<0) {
    /* this is ugly: when using a named device,
     * we are currently assuming that we have read/write-access
     * of course, this is not necessarily true
     */
    /* furthermore, we might also use the object
     * withOUT write permissions
     * (just reading the parport)
     */
    if (x->port && x->port < 0x400) {
      if (ioperm(x->port, 8, 1)) {
        x->mode=MODE_NONE;
      } else {
        x->mode = MODE_IOPERM;
      }
    }
    if(x->mode==MODE_NONE) {
      if (iopl(3)) {
        x->mode=MODE_NONE;
      } else {
        x->mode=MODE_IOPL;
      }
      count_iopl++;
    }

    if(x->mode==MODE_NONE) {
      error("lpt : couldn't get write permissions");
      x->port = 0;
      return (x);
    }
  }
#ifdef __WIN32__
  z_inpout32_ctor();
#else
  if(x->device>0) {
    post("lpt: connected to device %s", devname);
  } else {
    post("lpt: connected to port %x in mode '%s'", x->port,
         (x->mode==MODE_IOPL)?"iopl":"ioperm");
  }
#endif
  if (x->mode==MODE_IOPL) {
    post("lpt-warning: this might seriously damage your pc...");
  }

#else
  error("zexy has been compiled without [lpt]!");
  count_iopl=0;
#endif /* Z_WANT_LPT */

  devname=0;

  return (x);
}

static void lpt_helper(t_lpt*UNUSED(x))
{
  post("\n"HEARTSYMBOL " lpt :: direct access to the parallel port");
  post("<byte>\t: write byte to the parallel-port");
  post("\ncreation:\t\"lpt [<port>]\": connect to parallel port <port> (0..2)");
  post("\t\t\"lpt <portaddr>\": connect to port @ <portaddr> (hex)");
}

ZEXY_SETUP void lpt_setup(void)
{
  lpt_class = zexy_new("lpt",
                       lpt_new, lpt_free, t_lpt, 0, "*");
  //class_addcreator((t_newmethod)lpt_new, gensym("lp"), A_GIMME, 0);

  class_addfloat(lpt_class, (t_method)lpt_float);
  zexy_addmethod(lpt_class, (t_method)lpt_control, "control", "f");
  class_addbang(lpt_class, (t_method)lpt_bang);

  zexy_addmethod(lpt_class, (t_method)lpt_helper, "help", "");
  zexy_register("lpt");
}
