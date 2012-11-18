
/* 2801:forum::für::umläute:2002 */

/*
  motormix by CM-labs
*/

#include "MIDIvice.h"
#include <string.h>
#include <ctype.h>

#define LCD_TEXT  0x10
#define LCD_GRAPH 0x11
#define SEG7      0x12

static int x_port = 0;
static t_symbol *ctlin_sym;
static t_symbol *notein_sym;

void outmidi_noteon(int portno, int channel, int pitch, int velo);
void outmidi_controlchange(int portno, int channel, int ctlno, int value);
void sys_putmidibyte(int portno, int byte);

static void outmidi_byte(unsigned char byte)
{ sys_putmidibyte(x_port, byte); }



/* ------------------------- LCDtext ------------------------------- */

static t_class *LCDtext_class;
typedef struct _LCDtext
{
  t_object x_obj;

  int pos;
  int length;
  t_binbuf *bbuf;
} t_LCDtext;


static void LCD_header(unsigned char type)
{
  outmidi_byte(0xF0);
  outmidi_byte(0x00);
  outmidi_byte(0x01);
  outmidi_byte(0x0F);
  outmidi_byte(0x00);
  outmidi_byte(0x11);
  outmidi_byte(0x00);
  outmidi_byte(type);
}
static void LCD_footer(void)
{
  outmidi_byte(0xF7);
}

static void LCDtext_text(t_LCDtext *x)
{
  char *c, *str=0;
  int n, slen;
  int pos = x->pos;
  int len = x->length;
  int rest=0;

  if (!x->bbuf)return;

  binbuf_gettext(x->bbuf, &str, &slen);
  c=str;
  if (len>slen){
    n=slen;
    rest=len-slen;
  } else n=len?len:slen;

  if (n>0x50-pos)n=0x50-pos;

  LCD_header(LCD_TEXT);
  outmidi_byte(pos);
  while(n--)outmidi_byte(*c++);
  while(rest--)outmidi_byte(' ');
  LCD_footer();

  freebytes(str, slen);
}

static void LCDtext_clear(t_LCDtext *x, t_float fmode)
{
  int mode = fmode;
  int offset = (mode==2)?0x28:0x00;
  int n = (mode>0)?0x28:0x50;

  post("offset=%d\tn=%d", offset, n);

  LCD_header(LCD_TEXT);
  outmidi_byte(offset);

  while(n--)outmidi_byte(' ');

  LCD_footer();
}

static void LCDtext_any(t_LCDtext *x, t_symbol *s, int argc, t_atom *argv)
{
  //  t_atom a;
  binbuf_clear(x->bbuf);

  if ((s != &s_list)&&(s != &s_float)&&(s != &s_symbol)){
    t_atom a;
    SETSYMBOL(&a, s);
    binbuf_add(x->bbuf, 1, &a);
  }
  binbuf_add(x->bbuf, argc, argv);

  LCDtext_text(x);
}

static void LCDtext_pos(t_LCDtext *x, t_float pos, t_float len)
{
  if (pos<0 )pos=0;
  if (pos>80)pos=80;
  if (len<0 )len=0;

  x->pos=pos;
  x->length=len;
}


static void *LCDtext_new(t_floatarg f)
{
  t_LCDtext *x = (t_LCDtext *)pd_new(LCDtext_class);
  int pos = f;

  if (pos<0)pos=0;
  if (pos>0x4f)pos=0x4f;
  x->pos=pos;
  //  floatinlet_new(&x->x_obj, &x->pos);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym(""));

  x->bbuf = binbuf_new();

  return (x);
}
static void LCDtext_setup(void)
{
  LCDtext_class = class_new(gensym("motormix_LCDtext"), (t_newmethod)LCDtext_new, 
			      0, sizeof(t_LCDtext), 0, A_DEFFLOAT, 0);
  class_addcreator((t_newmethod)LCDtext_new, gensym("mm_LCDtext"), A_DEFFLOAT, 0);

  class_addmethod(LCDtext_class, (t_method)LCDtext_pos, gensym(""), A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(LCDtext_class, (t_method)LCDtext_clear, gensym("clear"), A_DEFFLOAT, 0);
  class_addbang(LCDtext_class, (t_method)LCDtext_text);

  class_addanything(LCDtext_class, LCDtext_any);
  class_sethelpsymbol(LCDtext_class, gensym("MIDIvice/motormix"));
}
/* ------------------------- LCDgraph ------------------------------- */

static t_class *LCDgraph_class;
typedef struct _LCDgraph
{
  t_object x_obj;

  t_float pos;
  int type;
} t_LCDgraph;

static void LCDgraph_valpos(int pos, int val)
{
  if ((pos<1)||(pos>8))return;
  if (val<0)val=0;
  if (val>127)val=127;
  outmidi_byte(pos-1);
  outmidi_byte(val);
}

static void LCDgraph_float(t_LCDgraph *x, t_float f)
{
  if ((x->pos<1)||(x->pos>8))return;

  LCD_header(LCD_GRAPH);
  outmidi_byte(x->type);
  LCDgraph_valpos(x->pos, f);
  LCD_footer();
}

static void LCDgraph_clear(t_LCDgraph *x)
{
  int n = 0x28;
  int pos=0x28;

  LCD_header(LCD_TEXT);
  outmidi_byte(pos);
  while(n--)outmidi_byte(' ');
  LCD_footer();
}


static void LCDgraph_pos(t_LCDgraph *x, t_float pos)
{
  x->pos=pos;
}
static void LCDgraph_typ(t_LCDgraph *x, t_float type)
{
  if (type<0 )type=0;
  if (type>7)type=7;

  x->type=type;
}
static void *LCDgraph_new(t_float ftype)
{
  t_LCDgraph *x = (t_LCDgraph *)pd_new(LCDgraph_class);

  LCDgraph_typ(x, ftype);
  floatinlet_new(&x->x_obj, &x->pos);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("type"));
  x->pos=0;

  return (x);
}
static void LCDgraph_setup(void)
{
  LCDgraph_class = class_new(gensym("motormix_LCDgraph"), (t_newmethod)LCDgraph_new, 
			      0, sizeof(t_LCDgraph), 0, A_DEFFLOAT, 0);
  class_addcreator((t_newmethod)LCDgraph_new, gensym("mm_LCDgraph"), A_DEFFLOAT, 0);

  class_addmethod(LCDgraph_class, (t_method)LCDgraph_typ, gensym("type"), A_DEFFLOAT, 0);
  class_addmethod(LCDgraph_class, (t_method)LCDgraph_pos, gensym(""), A_DEFFLOAT, 0);
  class_addmethod(LCDgraph_class, (t_method)LCDgraph_clear, gensym("clear"), 0);
  class_addfloat (LCDgraph_class, (t_method)LCDgraph_float);

  //  class_addbang(LCDgraph_class, (t_method)LCDgraph_graph);
  class_sethelpsymbol(LCDgraph_class, gensym("MIDIvice/motormix"));
}

/* ------------------------- seg7 ------------------------------- */

static t_class *seg7_class;
typedef struct _seg7
{
  t_object x_obj;

  t_float point1, point2;
  char c1, c2;
} t_seg7;

static void seg7_nibblebyte(char c, int point)
{
  char C = toupper(c);
  char lo=C&0x0F;
  char hi=(C>>4)&0x0F;
  if (point)hi|=0x40;
  //  post("C=%c=%c", c, C);
  //  post("hi=%x\tlo=%x", hi, lo);

  outmidi_byte(hi);
  outmidi_byte(lo);
}

static void seg7_write(char c1, int pt1, char c2, int pt2)
{
  LCD_header(SEG7);

  seg7_nibblebyte(c1, pt1);
  seg7_nibblebyte(c2, pt2);

  LCD_footer();
}

static void seg7_clear(t_seg7 *x, t_float fmode)
{
  int mode = fmode;

  LCD_header(SEG7);
  if (mode!=2)seg7_nibblebyte(' ', 0);
  else seg7_nibblebyte(0, 0);
  if (mode!=1)seg7_nibblebyte(' ', 0);
  else seg7_nibblebyte(0, 0);

  LCD_footer();
}

static void seg7_list(t_seg7 *x, t_symbol *s, int argc, t_atom* argv)
{
  char seg1, seg2;
  int pt1=(x->point1 != 0);
  int pt2=(x->point2 != 0);

  seg1=(argv->a_type==A_SYMBOL)?
    *atom_getsymbol(argv)->s_name:
    (atom_getint(argv)%10+0x30);
  argv++;
  if (argc>1)seg2=(argv->a_type==A_SYMBOL)?*atom_getsymbol(argv)->s_name:(atom_getint(argv)%10+0x30);
  else {
    seg2=seg1;
    seg1=' ';
  }
  //  post("seg1=%c\tseg2=%c", seg1, seg2);
  
  x->c1=seg1;
  x->c2=seg2;

  seg7_write(seg1, pt1, seg2, pt2);
}
static void seg7_symbol(t_seg7 *x, t_symbol *s)
{
  int pt1=(x->point1 != 0);
  int pt2=(x->point2 != 0);
  char c1=*s->s_name;
  char c2=x->c2;

  x->c1=c1;

  //  post("c1=%c\tc2=%c", c1, c2);

  seg7_write(c1, pt1, c2, pt2);
}
static void seg7_symbol2(t_seg7 *x, t_symbol *s)
{
  x->c2=*s->s_name;
}
static void seg7_bang(t_seg7 *x)
{
  int pt1=(x->point1 != 0);
  int pt2=(x->point2 != 0);
  char c1=x->c1;
  char c2=x->c2;

  seg7_write(c1, pt1, c2, pt2);
}

static void *seg7_new(void)
{
  t_seg7 *x = (t_seg7 *)pd_new(seg7_class);
 
  floatinlet_new(&x->x_obj, &x->point1);
  inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("symbol"), gensym(""));
  floatinlet_new(&x->x_obj, &x->point2);

  x->point1=x->point2=0;
  x->c1=x->c2=0;

  return (x);
}
static void seg7_setup(void)
{
  seg7_class = class_new(gensym("motormix_7seg"), (t_newmethod)seg7_new, 
			      0, sizeof(t_seg7), 0, A_GIMME, 0);
  class_addcreator((t_newmethod)seg7_new, gensym("mm_7seg"), A_GIMME, 0);
  class_addbang(seg7_class, seg7_bang);
  class_addlist(seg7_class, seg7_list);
  class_addsymbol(seg7_class, seg7_symbol);
  class_addmethod(seg7_class, (t_method)seg7_clear, gensym("clear"), A_DEFFLOAT, 0);
  class_addmethod(seg7_class, (t_method)seg7_symbol2, gensym(""), A_DEFSYMBOL, 0);
  class_sethelpsymbol(seg7_class, gensym("MIDIvice/motormix"));
}

/* ------------------------- LED ------------------------------- */

static t_class *LED_class;
typedef struct _LED
{
  t_object x_obj;

  t_float LED;
  t_float state;
} t_LED;

static void LED_float(t_LED *x, t_float f)
{
  int MSB, LSB;
  int state = x->state;
  int offset = 0;
  int id = f;

  if (id<0)return;
  LSB=id%8;
  MSB=id/8;

  x->LED=id;


  if (state>0)offset=0x40;
  else if (state<0)offset=0x50;

  //  if (MSB<8)offset++;
#if 0
  post("LED: %x %x %x %x", 0x0C, MSB, 0x2C, LSB+offset);
  //  post("LED: %d %d %d %d\n", 0x0C, MSB, 0x2C, LSB+offset);
#endif
  outmidi_controlchange(x_port>>4, x_port&15, 0x0C, MSB);
  outmidi_controlchange(x_port>>4, x_port&15, 0x2C, LSB+offset);
}

static void LED_symbol(t_LED *x, t_symbol *s)
{
  post("motormix_LED: no method for symbol");
}

static void LED_bang(t_LED *x)
{
  LED_float(x, x->LED);
}

static void *LED_new(void)
{
  t_LED *x = (t_LED *)pd_new(LED_class);

  x->LED=-1;
  floatinlet_new(&x->x_obj, &x->state);

  x->state=0;

  return (x);
}
static void LED_setup(void)
{
  LED_class = class_new(gensym("motormix_LED"), (t_newmethod)LED_new, 
			      0, sizeof(t_LED), 0, 0);
  class_addcreator((t_newmethod)LED_new, gensym("mm_LED"), 0);
 
  class_addsymbol(LED_class, LED_symbol);
  class_addfloat (LED_class, LED_float);
  class_addbang  (LED_class, LED_bang);
  class_sethelpsymbol(LED_class, gensym("MIDIvice/motormix"));
}


/* button */
static t_class *button_class;
typedef struct _button
{
  t_object x_obj;
  int      o_itsme;      // the fader changed
  int      o_imtouched;  // the fader was touched

  t_outlet *o_value; // actual fader-value
  t_outlet *o_touch; // 1=touched; 0=released

  int activefader;
  int fader;
  unsigned char MSB, LSB;
} t_button;

static void button_touch(t_button *x, unsigned char value, unsigned char control)
{
  int pressed=0;
  int MSB=x->MSB;
  int LSB=0;

  x->o_imtouched=0;

  if (x->MSB<8){
    if ((value==0x00) || (value==0x40))return;
  }

  LSB=value;

  if (LSB>=0x40){
    pressed=1;
    LSB-=0x40;
  }

  outlet_float(x->o_touch, pressed);
  outlet_float(x->o_value, MSB*8+LSB);
}

static void button_list(t_button *x, t_symbol *s, int argc, t_atom *argv)
{
  unsigned char ctl = atom_getfloatarg(0, argc, argv);
  unsigned char val = atom_getfloatarg(1, argc, argv);
  //  int channel   = atom_getfloatarg(2, argc, argv);

  if (x->o_imtouched)button_touch(x, val, ctl);

  if (ctl==0x0f){
	x->o_imtouched=1;
	x->MSB=val;
      }
}
static void button_free(t_button *x)
{
  pd_unbind(&x->x_obj.ob_pd, ctlin_sym);
}
static void *button_new()
{
  t_button *x = (t_button *)pd_new(button_class);

  x->o_value=outlet_new(&x->x_obj, &s_float);
  x->o_touch=outlet_new(&x->x_obj, &s_float);

  x->o_imtouched = 0;

  pd_bind(&x->x_obj.ob_pd, ctlin_sym);

  return (x);
}
static void button_setup(void)
{
  button_class = class_new(gensym("motormix_button"), (t_newmethod)button_new, (t_method)button_free,
			   sizeof(t_button), CLASS_NOINLET, A_DEFFLOAT,  0);
  class_addcreator((t_newmethod)button_new, gensym("mm_button"), A_DEFFLOAT, 0);
 
  class_addlist(button_class, button_list);
  class_sethelpsymbol(button_class, gensym("MIDIvice/motormix"));
}

/* ------------------------- rotary ------------------------------- */


static t_class *rotary_class;
typedef struct _rotary
{
  t_object x_obj;

  t_outlet *o_value; // actual rot-value
  t_outlet *o_rot; // rot-number

  int rot;
} t_rotary;

static void rotary_list(t_rotary *x, t_symbol *s, int argc, t_atom *argv)
{
  unsigned char ctl = atom_getfloatarg(0, argc, argv);
  unsigned char val = atom_getfloatarg(1, argc, argv);
  //  int channel   = atom_getfloatarg(2, argc, argv);

  if ((ctl>=0x40) && (ctl<0x48)) {
    if (x->rot) {
      if (ctl+1-0x40==x->rot){
	//	int value = (val>=64)?val-64:-val;
	outlet_float(x->o_value, (val>=64)?val-64:-val);
	  }
    } else {
      outlet_float(x->o_rot, (t_float)(ctl-0x40+1));
      outlet_float(x->o_value, (val>=64)?val-64:-val);  
    }
  }
}
static void rotary_free(t_rotary *x)
{
  pd_unbind(&x->x_obj.ob_pd, ctlin_sym);
}
static void *rotary_new(t_floatarg f)
{
  t_rotary *x = (t_rotary *)pd_new(rotary_class);

  if ((f<0) || (f>=9)){
    post("motormix_rotary: rot [%d] specified. only rotariess 1..8(&0) are valid", (int)f);
    f=0;
  }
  x->rot = f;

  x->o_value=outlet_new(&x->x_obj, &s_float);
  if (!x->rot) x->o_rot=outlet_new(&x->x_obj, &s_float);

  pd_bind(&x->x_obj.ob_pd, ctlin_sym);

  return (x);
}
static void rotary_setup(void)
{
  rotary_class = class_new(gensym("motormix_rotary"), (t_newmethod)rotary_new,  (t_method)rotary_free,
			      sizeof(t_rotary), CLASS_NOINLET, A_DEFFLOAT,  0);
  class_addcreator((t_newmethod)rotary_new, gensym("mm_rotary"), A_DEFFLOAT, 0);
 
  class_addlist(rotary_class, rotary_list);
  class_sethelpsymbol(rotary_class, gensym("MIDIvice/motormix"));
}

/* ------------------------- encoder ------------------------------- */

static t_class *encoder_class;
typedef struct _encoder
{
  t_object x_obj;

  t_outlet *o_value; // actual rot-value
  t_outlet *o_push;  // pushed
} t_encoder;

static void encoder_list(t_encoder *x, t_symbol *s, int argc, t_atom *argv)
{
  unsigned char ctl = atom_getfloatarg(0, argc, argv);
  unsigned char val = atom_getfloatarg(1, argc, argv);
  //  int channel   = atom_getfloatarg(2, argc, argv);

  if (ctl==0x48) outlet_float(x->o_value, (val>=64)?val-64:-val);
  else if (ctl==0x49)outlet_float(x->o_push, (t_float)(val==0x01));
}
static void encoder_free(t_encoder *x)
{
  pd_unbind(&x->x_obj.ob_pd, ctlin_sym);
}
static void *encoder_new(void)
{
  t_encoder *x = (t_encoder *)pd_new(encoder_class);

  x->o_value=outlet_new(&x->x_obj, &s_float);
  x->o_push =outlet_new(&x->x_obj, &s_float);

  pd_bind(&x->x_obj.ob_pd, ctlin_sym);

  return (x);
}
static void encoder_setup(void)
{
  encoder_class = class_new(gensym("motormix_encoder"), (t_newmethod)encoder_new,  (t_method)encoder_free,
			    sizeof(t_encoder), CLASS_NOINLET, 0);
   class_addcreator((t_newmethod)encoder_new, gensym("mm_encoder"), 0);

  class_addlist(encoder_class, encoder_list);
  class_sethelpsymbol(encoder_class, gensym("MIDIvice/motormix"));
}


/* ------------------------- faderIn ------------------------------- */

static t_class *faderIn_class;
typedef struct _faderIn
{
  t_object x_obj;
  int      o_itsme;      // the fader changed
  int      o_imtouched;  // the fader was touched

  t_outlet *o_value; // actual fader-value
  t_outlet *o_touch; // 1=touched; 0=released
  t_outlet *o_fader; // fader-number

  int activefader;
  int fader;
  unsigned char MSB, LSB;
} t_faderIn;

static void faderIn_parse(t_faderIn *x, unsigned char value, unsigned char control)
{
  unsigned int fader = (x->fader)?x->fader:x->activefader;
  x->o_itsme=0;

  if (fader+31==control){
    x->LSB=value;
    if (!x->fader)outlet_float(x->o_fader, x->activefader);
    outlet_float(x->o_value, x->MSB+(1./128)*x->LSB);
  }
}
static void faderIn_touch(t_faderIn *x, unsigned char value, unsigned char control)
{
  x->o_imtouched=0;

  if (control==0x2F){
    switch (value) {
    case 0x00:
      if (!x->fader)outlet_float(x->o_fader, x->activefader);
      outlet_float(x->o_touch, 0.);
      break;
    case 0x40:
      if (!x->fader)outlet_float(x->o_fader, x->activefader);
      outlet_float(x->o_touch, 1.);
    default:
      break;
    }
  }
}

static void faderIn_list(t_faderIn *x, t_symbol *s, int argc, t_atom *argv)
{
  unsigned char ctl = atom_getfloatarg(0, argc, argv);
  unsigned char val = atom_getfloatarg(1, argc, argv);
  //  int channel   = atom_getfloatarg(2, argc, argv);

  if (x->o_itsme)    faderIn_parse(x, val, ctl);
  if (x->o_imtouched)faderIn_touch(x, val, ctl);

  if (ctl==0x0f){
    if (x->fader) {
      if (val+1==x->fader){ // touched me
	x->o_imtouched=1;
      }
    } else {
      if (val<=7){ // touched us
	x->o_imtouched=1;
	x->activefader=val+1;
      }
    }
  } else if (ctl<0x08) {
    if (x->fader) {
      if (ctl+1==x->fader){
	x->o_itsme=1;
	x->MSB=val;
      }
    } else {
      x->o_itsme=1;
      x->activefader=ctl+1;
      x->MSB=val;
    }
  }
}
static void faderIn_free(t_faderIn *x)
{
  pd_unbind(&x->x_obj.ob_pd, ctlin_sym);
}
static void *faderIn_new(t_floatarg f)
{
  t_faderIn *x = (t_faderIn *)pd_new(faderIn_class);

  if ((f<0) || (f>=9)){
    post("motormix_fader: fader [%d] specified. only faders 1..8(&0) are valid", (int)f);
    f=0;
  }
  x->fader = f;

  x->o_value=outlet_new(&x->x_obj, &s_float);
  x->o_touch=outlet_new(&x->x_obj, &s_float);
  if (!x->fader) x->o_fader=outlet_new(&x->x_obj, &s_float);

  x->o_itsme     = 0;
  x->o_imtouched = 0;
  x->activefader = 0;

  pd_bind(&x->x_obj.ob_pd, ctlin_sym);

  return (x);
}
static void faderIn_setup(void)
{
  faderIn_class = class_new(gensym("motormix_faderIn"), (t_newmethod)faderIn_new,  (t_method)faderIn_free,
			    sizeof(t_faderIn), CLASS_NOINLET, A_DEFFLOAT,  0);
  class_addcreator((t_newmethod)faderIn_new, gensym("mm_faderIn"), A_DEFFLOAT, 0);
 
  class_addlist(faderIn_class, faderIn_list);
  class_sethelpsymbol(faderIn_class, gensym("MIDIvice/motormix"));
}

/* ------------------------- faderOut ------------------------------- */

static t_class *faderOut_class;
typedef struct _faderOut
{
  t_object x_obj;

  t_float fader;
} t_faderOut;

static void faderOut_float(t_faderOut *x, t_float f)
{
  int fader=x->fader;
  unsigned char MSB, LSB;
  if (f>=128)f=128;
  else if (f<0)f=0;
  if ((fader<1) || (fader>8))return;    

  MSB=f;
  LSB=(f-MSB)*128;
  outmidi_controlchange(x_port>>4, x_port&15, fader-1, MSB);
  outmidi_controlchange(x_port>>4, x_port&15, fader-1+32, LSB);
}

static void *faderOut_new(t_floatarg f)
{
  t_faderOut *x = (t_faderOut *)pd_new(faderOut_class);

  if ((f<0) || (f>=9)){
    post("motormix_fader: fader [%d] specified. only faders 1..8(&0) are valid", (int)f);
    f=0;
  }
  x->fader=f;
  if (!x->fader)floatinlet_new(&x->x_obj, &x->fader);

  return (x);
}
static void faderOut_setup(void)
{
  faderOut_class = class_new(gensym("motormix_faderOut"), (t_newmethod)faderOut_new, 
			      0, sizeof(t_faderOut), 0, A_DEFFLOAT,  0);
  class_addcreator((t_newmethod)faderOut_new, gensym("mm_faderOut"), A_DEFFLOAT, 0);
 
  class_addfloat(faderOut_class, faderOut_float);
  class_sethelpsymbol(faderOut_class, gensym("MIDIvice/motormix"));
}

/* ------------------------- motormix ------------------------------- */

static t_class *motormix_class;
typedef struct _motormix
{
  t_object x_obj;
} t_motormix;
static void motormix_return(t_motormix *x, t_symbol *s, int argc, t_atom *argv)
{
  int p = atom_getfloatarg(0, argc, argv);
  int v = atom_getfloatarg(1, argc, argv);

  if ((p==0x00) && (v==0x7F))
    outlet_bang(x->x_obj.ob_outlet);
}

static void motormix_bang(t_motormix *x)
{ outmidi_noteon(x_port>>4, x_port&15,0,0); }

static void motormix_help(t_motormix *x)
{
  post("MIDIvice - motormix\n===================");
  post("support for MotorMix (tm) by cm-labs (r)");
  post("  \tmotormix_faderIn"
       "\n\tmotormix_rotary"
       "\n\tmotormix_encoder"
       "\n\tmotormix_button"
       "\n\tmotormix_faderOut"
       "\n\tmotormix_LED"
       "\n\tmotormix_LCDtext"
       "\n\tmotormix_LCDgraph"
       "\n\tmotormix_7seg");
  post("(l) forum::für::umläute @ IEM, Graz; 2002");
}
static void motormix_reset(t_motormix *x)
{ /* this LOOKs like a reset; i don't know what it really is... */
  LCD_header(SEG7);
  LCD_footer(); 
}
static void motormix_free(t_motormix *x)
{
  pd_unbind(&x->x_obj.ob_pd, notein_sym);
}
static void *motormix_new(t_floatarg f)
{
  t_motormix *x = (t_motormix *)pd_new(motormix_class);
  x_port=f;
  outlet_new(&x->x_obj, &s_float);
  pd_bind(&x->x_obj.ob_pd, notein_sym);
  //  outmidi_noteon(x_port>>4,x_port&15,0,0); // isn't this dangerous ???
  return (x);
}

void motormix_setup(void)
{
  motormix_class = class_new(gensym("motormix"), (t_newmethod)motormix_new, (t_method)motormix_free,
			     sizeof(t_motormix), 0/*0CLASS_NOINLET*/, /*A_DEFFLOAT,*/  0);
  
  class_addcreator((t_newmethod)motormix_new, gensym("MotorMix"), /*A_DEFFLOAT,*/ 0);

  class_addmethod(motormix_class, (t_method)motormix_help, gensym("help"), 0);
  class_addbang(motormix_class, motormix_bang);
  class_addmethod(motormix_class, (t_method)motormix_reset, gensym("reset"), 0);
  class_addlist(motormix_class, motormix_return);
 
  class_sethelpsymbol(motormix_class, gensym("MIDIvice/motormix"));

  ctlin_sym = gensym("#ctlin");
  notein_sym = gensym("#notein");

  button_setup();
  faderIn_setup();
  faderOut_setup();
  rotary_setup();
  encoder_setup();
  LED_setup();

  LCDtext_setup();
  LCDgraph_setup();
  seg7_setup();

  /* we have to send a motormix_ping once before it starts reacting.
   * this seems to be a problem with pd, but i don't have hardware to test it right now
   */
  outmidi_noteon(x_port>>4,x_port&15,0,0); 
}
