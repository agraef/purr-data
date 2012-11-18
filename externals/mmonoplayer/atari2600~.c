/*****************************************************************************/
/*                                                                           */
/* TIA Chip Sound Simulator for PD                                           */
/* Purpose: To emulate the sound generation hardware of the Atari TIA chip.  */
/* Author(s):  Kyle Buza, Ron Fries                                          */
/*                                                                           */
/*****************************************************************************/

#include <stdlib.h>
#include "m_pd.h"

static t_class *atari_2600_class;  

typedef signed char Int8;
typedef signed int Int16;

#ifdef WIN32
#define int8  char
#define int16 short
#define int32 int
#else
#define int8  char
#define int16 int
#define int32 long
#endif

#define uint8  unsigned int8 
#define uint16 unsigned int16
#define uint32 unsigned int32

/* definitions for AUDCx (15, 16) */
#define SET_TO_1     0x00      /* 0000 */
#define POLY4        0x01      /* 0001 */
#define DIV31_POLY4  0x02      /* 0010 */
#define POLY5_POLY4  0x03      /* 0011 */
#define PURE         0x04      /* 0100 */
#define PURE2        0x05      /* 0101 */
#define DIV31_PURE   0x06      /* 0110 */
#define POLY5_2      0x07      /* 0111 */
#define POLY9        0x08      /* 1000 */
#define POLY5        0x09      /* 1001 */
#define DIV31_POLY5  0x0a      /* 1010 */
#define POLY5_POLY5  0x0b      /* 1011 */
#define DIV3_PURE    0x0c      /* 1100 */
#define DIV3_PURE2   0x0d      /* 1101 */
#define DIV93_PURE   0x0e      /* 1110 */
#define DIV3_POLY5   0x0f      /* 1111 */

#define DIV3_MASK    0x0c                 

#define AUDC0        0x15
#define AUDC1        0x16
#define AUDF0        0x17
#define AUDF1        0x18
#define AUDV0        0x19
#define AUDV1        0x1a

/* the size (in entries) of the 4 polynomial tables */
#define POLY4_SIZE  0x000f
#define POLY5_SIZE  0x001f
#define POLY9_SIZE  0x01ff

/* channel definitions */
#define CHAN1       0
#define CHAN2       1

#define FALSE       0
#define TRUE        1

/* Initialze the bit patterns for the polynomials. */

/* The 4bit and 5bit patterns are the identical ones used in the tia chip. */
/* Though the patterns could be packed with 8 bits per byte, using only a */
/* single bit per byte keeps the math simple, which is important for */
/* efficient processing. */

static uint8 Bit4[POLY4_SIZE] =
      { 1,1,0,1,1,1,0,0,0,0,1,0,1,0,0 };

static uint8 Bit5[POLY5_SIZE] =
      { 0,0,1,0,1,1,0,0,1,1,1,1,1,0,0,0,1,1,0,1,1,1,0,1,0,1,0,0,0,0,1 };

/* I've treated the 'Div by 31' counter as another polynomial because of */
/* the way it operates.  It does not have a 50% duty cycle, but instead */
/* has a 13:18 ratio (of course, 13+18 = 31).  This could also be */
/* implemented by using counters. */

static uint8 Div31[POLY5_SIZE] =
      { 0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0 };

typedef struct _atari_2600
{ 
  t_object x_obj;
  t_float x15;
  t_float x16;
  t_float x17;
  t_float x18;
  t_float x19;
  t_float x1a;
  long Bit9[POLY9_SIZE];
  long P4[2];
  long P5[2];
  long P9[2];
  long Div_n_cnt[2];
  long Div_n_max[2];
  long AUDC[2];
  long AUDF[2];
  long AUDV[2];
  long Outvol[2];
  long volume;
} t_atari_2600;  

void *atari_2600_new(void);  
t_int *atari_2600_perform(t_int *w); 
void atari_2600_dsp(t_atari_2600 *x, t_signal **sp, short *count);  

void Tia_sound_init(t_atari_2600 *x, uint16 sample_freq, uint16 playback_freq);
void Update_tia_sound(t_atari_2600 *x, uint16 addr, uint8 val);
void Tia_process (t_atari_2600 *x, t_float *buffer, uint16 n);

void atari_2600_tilde_setup(void)
{
	atari_2600_class = class_new(gensym("atari_2600~"),
			(t_newmethod)atari_2600_new, 0, sizeof(t_atari_2600), CLASS_DEFAULT, A_DEFFLOAT, 0);
	class_addmethod(atari_2600_class, (t_method)atari_2600_dsp, gensym("dsp"), 0);
}

void *atari_2600_new(void)
{  
  t_atari_2600 *x = (t_atari_2600 *)pd_new(atari_2600_class);
  x->x15 = 0;
  x->x16 = 0;
  x->x17 = 0;
  x->x18 = 0;
  x->x19 = 0;
  x->x1a = 0;
  floatinlet_new(&x->x_obj, &x->x15);
  floatinlet_new(&x->x_obj, &x->x17);
  floatinlet_new(&x->x_obj, &x->x19);
  floatinlet_new(&x->x_obj, &x->x16);
  floatinlet_new(&x->x_obj, &x->x18);
  floatinlet_new(&x->x_obj, &x->x1a);
  outlet_new(&x->x_obj, &s_signal);

  Tia_sound_init(x, 32000, 32000);
  return (x);
}   

void atari_2600_dsp(t_atari_2600 *x, t_signal **sp, short *count) 
{   
  dsp_add(atari_2600_perform, 3, sp[0]->s_vec, sp[0]->s_n, x); 
}  

t_int *atari_2600_perform(t_int *w) 
{     
  t_float *outL = (t_float *)(w[1]); 
  t_atari_2600 *x = (t_atari_2600 *)(w[3]);  

  Update_tia_sound(x, 0x15, x->x15);
  Update_tia_sound(x, 0x16, x->x16);
  Update_tia_sound(x, 0x17, x->x17);
  Update_tia_sound(x, 0x18, x->x18);
  Update_tia_sound(x, 0x19, x->x19);
  Update_tia_sound(x, 0x1a, x->x1a);

  Tia_process(x, outL, (int)(w[2]));

  return (w + 4);
}  


/*****************************************************************************/
/*                                                                           */
/* Module:  TIA Chip Sound Simulator                                         */
/* Purpose: To emulate the sound generation hardware of the Atari TIA chip.  */
/* Author:  Ron Fries                                                        */
/*                                                                           */
/* Revision History:                                                         */
/*    10-Sep-96 - V1.0 - Initial Release                                     */
/*    14-Jan-97 - V1.1 - Cleaned up sound output by eliminating counter      */
/*                       reset.                                              */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*                 License Information and Copyright Notice                  */
/*                 ========================================                  */
/*                                                                           */
/* TiaSound is Copyright(c) 1996 by Ron Fries                                */
/*                                                                           */
/* This library is free software; you can redistribute it and/or modify it   */
/* under the terms of version 2 of the GNU Library General Public License    */
/* as published by the Free Software Foundation.                             */
/*                                                                           */
/* This library is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library */
/* General Public License for more details.                                  */
/* To obtain a copy of the GNU Library General Public License, write to the  */
/* Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.   */
/*                                                                           */
/* Any permitted reproduction of these routines, in whole or in part, must   */
/* bear this legend.                                                         */
/*                                                                           */
/*****************************************************************************/

void Tia_sound_init (t_atari_2600 *x, uint16 sample_freq, uint16 playback_freq)
{
   uint8 chan;
   int16 n;

   /* fill the 9bit polynomial with random bits */
   for (n=0; n<POLY9_SIZE; n++)
   {
      x->Bit9[n] = rand() & 0x01;       /* fill poly9 with random bits */
   }

   /* initialize the local globals */
   for (chan = CHAN1; chan <= CHAN2; chan++)
   {
      x->Outvol[chan] = 0;
      x->Div_n_cnt[chan] = 0;
      x->Div_n_max[chan] = 0;
      x->AUDC[chan] = 0;
      x->AUDF[chan] = 0;
      x->AUDV[chan] = 0;
      x->P4[chan] = 0;
      x->P5[chan] = 0;
      x->P9[chan] = 0;
   }

   x->volume = 100;
}

void Update_tia_sound(t_atari_2600 *x, uint16 addr, uint8 val)
{
    uint16 new_val = 0;
    uint8 chan;

    /* determine which address was changed */
    switch (addr)
    {
       case AUDC0:
          x->AUDC[0] = val & 0x0f;
          chan = 0;
          break;

       case AUDC1:
          x->AUDC[1] = val & 0x0f;
          chan = 1;
          break;

       case AUDF0:
          x->AUDF[0] = val & 0x1f;
          chan = 0;
          break;

       case AUDF1:
          x->AUDF[1] = val & 0x1f;
          chan = 1;
          break;

       case AUDV0:
          x->AUDV[0] = (val & 0x0f) << 3;
          chan = 0;
          break;

       case AUDV1:
          x->AUDV[1] = (val & 0x0f) << 3;
          chan = 1;
          break;

       default:
          chan = 255;
          break;
    }

    /* if the output value changed */
    if (chan != 255)
    {
       /* an AUDC value of 0 is a special case */
       if (x->AUDC[chan] == SET_TO_1)
       {
          /* indicate the clock is zero so no processing will occur */
          new_val = 0;

          /* and set the output to the selected volume */
          x->Outvol[chan] = x->AUDV[chan];
       }
       else
       {
          /* otherwise calculate the 'divide by N' value */
          new_val = x->AUDF[chan] + 1;

          /* if bits 2 & 3 are set, then multiply the 'div by n' count by 3 */
          if ((x->AUDC[chan] & DIV3_MASK) == DIV3_MASK)
          {
             new_val *= 3;
          }
       }

       /* only reset those channels that have changed */
       if (new_val != x->Div_n_max[chan])
       {
          /* reset the divide by n counters */
          x->Div_n_max[chan] = new_val;

          /* if the channel is now volume only or was volume only */
          if ((x->Div_n_cnt[chan] == 0) || (new_val == 0))
          {
             /* reset the counter (otherwise let it complete the previous) */
             x->Div_n_cnt[chan] = new_val;
          }
       }
    }
	
}

void Tia_process(t_atari_2600 *x, t_float *buffer, uint16 n)
{
    uint8 audc0,audv0,audc1,audv1;
    uint8 div_n_cnt0,div_n_cnt1;
    uint8 p5_0, p5_1,outvol_0,outvol_1;

    audc0 = x->AUDC[0];
    audv0 = x->AUDV[0];
    audc1 = x->AUDC[1];
    audv1 = x->AUDV[1];

    /* make temporary local copy */
    p5_0 = x->P5[0];
    p5_1 = x->P5[1];
    outvol_0 = x->Outvol[0];
    outvol_1 = x->Outvol[1];
    div_n_cnt0 = x->Div_n_cnt[0];
    div_n_cnt1 = x->Div_n_cnt[1];

    /* loop until the buffer is filled */
    while (n)
    {
       /* Process channel 0 */
       if (div_n_cnt0 > 1)
       {
          div_n_cnt0--;
       }
       else if (div_n_cnt0 == 1)
       {
          div_n_cnt0 = x->Div_n_max[0];

          /* the P5 counter has multiple uses, so we inc it here */
          p5_0++;
          if (p5_0 == POLY5_SIZE)
             p5_0 = 0;

          /* check clock modifier for clock tick */
          if  (((audc0 & 0x02) == 0) ||
              (((audc0 & 0x01) == 0) && Div31[p5_0]) ||
              (((audc0 & 0x01) == 1) &&  Bit5[p5_0]))
          {
		  
             if (audc0 & 0x04)       /* pure modified clock selected */
             {
                if (outvol_0)        /* if the output was set */
                   outvol_0 = 0;     /* turn it off */
                else {
                   outvol_0 = audv0; /* else turn it on */
				}
             }
             else if (audc0 & 0x08)    /* check for p5/p9 */
             {
                if (audc0 == POLY9)    /* check for poly9 */
                {
                   /* inc the poly9 counter */
                   x->P9[0]++;
                   if (x->P9[0] == POLY9_SIZE)
                      x->P9[0] = 0;

                   if (x->Bit9[x->P9[0]]) {
                      outvol_0 = audv0;
					}
                   else
                      outvol_0 = 0;
                }
                else                        /* must be poly5 */
                {
                   if (Bit5[p5_0]) {
                      outvol_0 = audv0;
					  }
                   else
                      outvol_0 = 0;
                }
             }
             else  /* poly4 is the only remaining option */
             {
                /* inc the poly4 counter */
                x->P4[0]++;
                if (x->P4[0] == POLY4_SIZE)
                   x->P4[0] = 0;

                if (Bit4[x->P4[0]]) {
                   outvol_0 = audv0;
				   }
                else
                   outvol_0 = 0;
             }
          }
       }

       /* Process channel 1 */
       if (div_n_cnt1 > 1)
       {
          div_n_cnt1--;
       }
       else if (div_n_cnt1 == 1)
       {
          div_n_cnt1 = x->Div_n_max[1];

          /* the P5 counter has multiple uses, so we inc it here */
          p5_1++;
          if (p5_1 == POLY5_SIZE)
             p5_1 = 0;

          /* check clock modifier for clock tick */
          if  (((audc1 & 0x02) == 0) ||
              (((audc1 & 0x01) == 0) && Div31[p5_1]) ||
              (((audc1 & 0x01) == 1) &&  Bit5[p5_1]))
          {
             if (audc1 & 0x04)       /* pure modified clock selected */
             {
                if (outvol_1)        /* if the output was set */
                   outvol_1 = 0;     /* turn it off */
                else
                   outvol_1 = audv1; /* else turn it on */
             }
             else if (audc1 & 0x08)    /* check for p5/p9 */
             {
                if (audc1 == POLY9)    /* check for poly9 */
                {
                   /* inc the poly9 counter */
                   x->P9[1]++;
                   if (x->P9[1] == POLY9_SIZE)
                      x->P9[1] = 0;

                   if (x->Bit9[x->P9[1]])
                      outvol_1 = audv1;
                   else
                      outvol_1 = 0;
                }
                else                        /* must be poly5 */
                {
                   if (Bit5[p5_1])
                      outvol_1 = audv1;
                   else
                      outvol_1 = 0;
                }
             }
             else  /* poly4 is the only remaining option */
             {
                /* inc the poly4 counter */
                x->P4[1]++;
                if (x->P4[1] == POLY4_SIZE)
                   x->P4[1] = 0;

                if (Bit4[x->P4[1]])
                   outvol_1 = audv1;
                else
                   outvol_1 = 0;
             }
          }
       }
       {
		  uint32 val = ((((uint32)outvol_0 + (uint32)outvol_1) * x->volume)/100);
		  float va = (((float)val) - 100.0)/100.0;
		  *(buffer++) = va;
          n--;
       }
    }

    /* save for next round */
    x->P5[0] = p5_0;
    x->P5[1] = p5_1;
    x->Outvol[0] = outvol_0;
    x->Outvol[1] = outvol_1;
    x->Div_n_cnt[0] = div_n_cnt0;
    x->Div_n_cnt[1] = div_n_cnt1;
}
