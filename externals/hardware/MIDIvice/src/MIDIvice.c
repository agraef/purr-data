#include "MIDIvice.h"
void motormix_setup();

typedef struct MIDIvice 
{
  t_object t_ob;
} t_MIDIvice;

t_class *MIDIvice_class;

static void MIDIvice_help(void)
{
  post("\nMIDIvice  "VERSION);
  post("supported devices:");
  post("\tmotormix\t\tMotorMix(tm) by cm-labs(r)"
       "\n");
}

void *MIDIvice_new(void)
{
  t_MIDIvice *x = (t_MIDIvice *)pd_new(MIDIvice_class);
  return (void *)x;
}


void MIDIvice_setup(void) 
{
  motormix_setup();

  /* ************************************** */
  post("\n\tMIDIvice "VERSION);
  post("\tcopyleft forum::für::umläute @ IEM/KUG 2002-2008");
 
  MIDIvice_class = class_new(gensym("MIDIvice"), MIDIvice_new, 0, sizeof(t_MIDIvice), 0, 0);
  class_addmethod(MIDIvice_class, MIDIvice_help, gensym("help"), 0);
}
