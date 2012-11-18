/******************************************************
 *
 * dmxin - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   0603:forum::für::umläute:2008
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/


#include "dmx4pd.h"

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

static t_class *dmxin_class;

typedef struct _dmxin
{
  t_object x_obj;
  int      x_device;
  int      x_port;

  dmx_t x_dmxbuffer[512];


  t_outlet*x_outlet1, *x_outlet2;
} t_dmxin;

static void dmx_doread(t_dmxin*x) {
  int dmxin=x->x_device;
  fd_set readset;
  post("dmxin_doread: %d", dmxin);
  if(dmxin<=0)return;

  FD_ZERO(&readset);
  FD_SET(dmxin, &readset);
  FD_SET(0, &readset);

  int n=select(dmxin+1, &readset, NULL,NULL, NULL);
  if(n>0 && FD_ISSET(dmxin, &readset)) {
    dmx_t dmxbuffer[512];
    int i=0;
    lseek (dmxin, 0, SEEK_SET);
    n=read (dmxin, dmxbuffer, sizeof(dmxbuffer));
    for(i=0; i<512; i+=2) {
      int c=dmxbuffer[i];
      if(c!=x->x_dmxbuffer[i]) {
        x->x_dmxbuffer[i]=c;
        post("read %03d @ %03d", c, i);
      }
    }
  }
}
static void dmxin_bang(t_dmxin*x)
{
  dmx_doread(x);
}

static void dmxin_close(t_dmxin*x)
{
  if(x->x_device>=0) {
    close(x->x_device);
  }
  x->x_device=-1;
}


static void dmxin_open(t_dmxin*x, t_symbol*s_devname)
{
  int argc=2;
  const char *args[2] = {"--dmxin", s_devname->s_name};
  const char**argv=args;
  char*devname="";
  int fd;

  if(s_devname && s_devname->s_name)
    devname=s_devname->s_name;

  //  strncpy(args[0], "--dmx", MAXPDSTRING);
  //  strncpy(args[1], devname, MAXPDSTRING);

  fd = open (DMXINdev(&argc, argv), O_RDONLY);

  if(fd!=-1) {
    dmxin_close(x);
    x->x_device=fd;
  }
}


static void *dmxin_new(void)
{
  int i=0;
  t_dmxin *x = (t_dmxin *)pd_new(dmxin_class);

  x->x_device=0;
  x->x_port=0;

  for(i=0; i<sizeof(x->x_dmxbuffer); i++) {
    x->x_dmxbuffer[i]=0;
  }


  x->x_outlet1=outlet_new(&x->x_obj, &s_float);
  x->x_outlet2=outlet_new(&x->x_obj, &s_float);



  dmxin_open(x, gensym(""));
  return (x);
}
static void dmxin_free(t_dmxin*x)
{
  dmxin_close(x);
}

void dmxin_setup(void)
{
  dmxin_class = class_new(gensym("dmxin"), (t_newmethod)dmxin_new, (t_method)dmxin_free,
                          sizeof(t_dmxin),  
                          0,
                          A_NULL);
  
  class_addbang(dmxin_class, dmxin_bang);
  
#ifdef DMX4PD_POSTBANNER
  DMX4PD_POSTBANNER;
#endif
}
