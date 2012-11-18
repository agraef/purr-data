/* ...this is an external for the memPIO interface...
 * 
 * memPIO is a USB-interface with 3 8bit ports
 * each port can be either input or output
 * (honestly: port3 is divided into 2 subports of 4bits with independent directions,
 *  however, this is not yet implemented)
 *
 *
 * "mode <port0> <port1> <port2> <port3>" : mode can be either 0 (input) or 1 (output)
 * "mode <allports>": like above, but set all ports at once to the same value
 * "bang": manual polling
 * "set <val0> <val1> <val2> <val3>": write values to ports in output-mode
 * "set <val>" write the same value to all ports
 *
 * TODO: currently two [memPIO]s interfere
 *        
 *
 * copyleft:forum::für::umläute:2004
 */

/* This only works on Microsoft Windows */
#ifdef _WIN32

#include <conio.h>

#include "m_pd.h"


// IMPORTANT: Adjust this path to your needs
//#import "c:\windows\system\memx.ocx"
#import "c:\winnt\system32\memx.ocx"

/* do a little help thing */

typedef struct memPIO 
{
  t_object x_obj;
  MEMXLib::_DmeMPIOPtr pio;
  int input[3];
  t_outlet *outlet[3];
} t_memPIO;

t_class *memPIO_class;

static void memPIO_help(void)
{
  post("\n\n...mem-PIO for pd"
       "\n\n(l) forum::für::umläute 2004\n"
       "this software is under the GnuGPL that is provided with these files\n");
  post("usage:");
  post("\t'mode <1|0>': set all ports to writable(1) or readable(0=default)");
  post("\t'mode <1|0> <1|0> <1|0>': set the read/write-mode of the individual ports");
  post("\t'bang': returns the values of all readable ports");
  post("\t'set <val>': set all writable ports to the 8bit-value");
  post("\t'set <val1> <val2> <val3>': set writable ports to the corresponding value");
}

static void memPIO_bang(t_memPIO*x){
  int port=3;

  x->pio->UpdateCache ();

  while(port--){
    if (!x->input[port]){
      long l=x->pio->CachedPort [port+1];
      outlet_float(x->outlet[port], l);
    }
  }
}


static void memPIO_mode(t_memPIO*x, t_symbol *s, int argc, t_atom*argv){
  MEMXLib::IO dir;
  bool out;

  switch(argc){
  default:
    error("memPIO: \"mode\" message needs 1 or 3 arguments, have %d", argc);
    return;
  case 1:
    out=(atom_getfloat(argv)>0.f);
    dir=(out)?MEMXLib::DirOut:MEMXLib::DirIn;
    x->pio->DirPort1 =dir; x->input[0]=out;
    x->pio->DirPort2 =dir; x->input[1]=out;
    x->pio->DirPort3H=dir; x->input[2]=out;
    x->pio->DirPort3L=dir;
    break;
  case 3:
    out=(atom_getfloat(argv+0)>0.f);
    x->pio->DirPort1 =(out)?MEMXLib::DirOut:MEMXLib::DirIn; x->input[0]=out;
    out=(atom_getfloat(argv+1)>0.f);
    x->pio->DirPort2 =(out)?MEMXLib::DirOut:MEMXLib::DirIn; x->input[1]=out;
    out=(atom_getfloat(argv+2)>0.f);
    x->pio->DirPort3H=(out)?MEMXLib::DirOut:MEMXLib::DirIn; x->input[2]=out;
    x->pio->DirPort3L=(out)?MEMXLib::DirOut:MEMXLib::DirIn;
  }
}


static void memPIO_set(t_memPIO*x, t_symbol*s, int argc, t_atom*argv){
  int port=3;

  unsigned char val[4]={0,0,0,0};
	
  switch(argc){
  case 1:
    val[0]=val[1]=val[2]=val[3]=atom_getfloat(argv);
    break;
  case 3:
    val[0]=atom_getfloat(argv+0);
    val[1]=atom_getfloat(argv+1);
    val[2]=atom_getfloat(argv+2);
    break;
  default:
    break;
  }

  while(port--)
    if(x->input[port]){
      x->pio->port[port+1]=val[port];
    }
	
}


static void memPIO_free(t_memPIO*x)
{
  x->pio.Release ();
}

void *memPIO_new(void)
{
  int i;
  bool found=false;
  t_memPIO *x = (t_memPIO *)pd_new(memPIO_class);
  // because we use a ActiveX we need to initialize OLE for this app
  OleInitialize (NULL);
  // try to create the ActiveX-instance
  x->pio.CreateInstance (__uuidof(MEMXLib::meMPIO));
  // on error, there is no meM-ActiveX installed
  if (x->pio == NULL)
    {
      error("meM ActiveX not installed!");
      return 0;
    }

  found=false;
  // search until the highest-possible Card-ID
  for(i = 0; i < x->pio->LastAttached; i++)
    {
      // select a device
      x->pio->CardId = i+1;
      // if selected device is attached, return
      if (x->pio->Attached){
	found=true;
	break;
      }
    }
  i++;
  if(!found){
    memPIO_free(x);
    return 0;
  }

  /* these are all outputs */
  for (int n=0; n<3; n++) {
    x->outlet[n] = outlet_new(&x->x_obj, 0);
    x->input[n]=0;
  }

  return (void *)x;
}

void memPIO_setup(void) 
{
  post("\t the memPIO external");
  post("\t (l) forum::für::umläute");
  post("\t  compiled:  "__DATE__"");
  
  memPIO_class = class_new(gensym("memPIO"), 
			   (t_newmethod)memPIO_new, 
			   (t_method)memPIO_free,
			   sizeof(t_memPIO), 
			   0, A_NULL);
  class_addmethod(memPIO_class, (t_method)memPIO_help, gensym("help"), A_NULL);
  class_addmethod(memPIO_class, (t_method)memPIO_mode, gensym("mode"), A_GIMME, A_NULL);
  class_addmethod(memPIO_class, (t_method)memPIO_set,  gensym("set"),  A_GIMME, A_NULL);

  class_addbang(memPIO_class, (t_method)memPIO_bang);
}

#endif /* _WIN32 */
