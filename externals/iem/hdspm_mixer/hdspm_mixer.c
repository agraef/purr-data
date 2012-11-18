/* Very simple Mixer for RME DSP-MADI and maybe other hammerfall dsp 
   (C) 2003 IEM, Winfried Ritsch  (ritsch at iem.at)
   (c) 2008 iem, Thomas Musil (musil at iem.at) ... modified to a pd external
   Institute of Electronic Music and Acoustics
   GPL - see Licence.txt

   Use it as library for other programs, or standalone commandline programm

   eg.:compile as program: cc -o hdspmmixer -DMAIN -lm -lc -lasound hdsdpmmixer.c
*/

#include "m_pd.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/ioctl.h>
#include <alsa/asoundlib.h>
#include "hdspm_mixer.h"

static t_class *hdspm_mixer_class;

typedef struct _hdspm_mixer
{
  t_object x_ob;
  char     x_card_name[HDSPMM_MAX_CARDS][HDSPMM_MAX_NAME_LEN];
  int      x_card_id;
} t_hdspm_mixer;

static int hdspm_mixer_find_cards(t_hdspm_mixer *x)
{
  int i;
  char *name;
  int card = HDSPMM_ERROR_NO_CARD;
  int n = 0;
  
  while (snd_card_next(&card) >= 0) // searching mixer for hdspm ...
  {
    if(card < 0)
    {
      break;
    }
    else
    {
      snd_card_get_name(card, &name);
      if(strncmp(name, "HDSPM MADI", 9) == 0)
      {
        snprintf(x->x_card_name[n], 6, "hw:%i", n); // printf("HDSP MADI as card %d (%s) found !\n", n, card_name[n]);
        n++;
	      if(n >= HDSPMM_MAX_CARDS)
	        break;
      } 
    }
  }
  
  if(!n)
    return HDSPMM_ERROR_NO_CARD;
  
  for(i = n; i < HDSPMM_MAX_CARDS; i++)
  {
    x->card_name[i][0] = '\0';
  }
  
  return n;
}

int main(int argc, char *argv[])
{
  int err = 0;
  
  int src;
  int dst;
  int val;
  
  /*
   int i;
   
   for(i=0; i < argc; i++)
   printf(" %s ", argv[i]);
   printf("\n");
   */
  /*
   if((err =find_cards()) < 0)
   return err;
   */
  
  if(argc < 4){
    printf("\nusage %s <devnr> <src> <dst> [value] \n\n"
           " devnr ... ALSA-Device eg. for hw:0 is 0 \n"
           " src   ... inputs 0-63 playback 64-127\n"
           " dst   ... out channel 0-63\n"
           "\noptional if wanting to set a value:\n\n"
           " value ... gain 0=0 (mute), 32768=1 (unitGain), 65535 = max\n\n",
           argv[0]);
    
    if(find_cards() < 0)
	    puts("No Hammerfall DSP MADI card found.");
    puts("");
    printf("Version %s, 2003 - IEM, winfried ritsch\n",HDSPMM_VERSION);
    return -1;
  }
  
  cardid = atoi(argv[1]);
  snprintf(card_name[cardid], 6, "hw:%i", cardid);
  
  src = atoi(argv[2]);
  dst = atoi(argv[3]);
  
  if(argc == 4){
    
    printf(" Get Mixer from %d to %d : %d \n",src,dst,
           get_gain(cardid,src,dst));
    
    return 0;
  }
  
  /* arg is 5 */
  val =  atoi(argv[4]);
  
  if((err = set_gain(cardid,src,dst,val)) < 0 )
    printf("Error: Could not set mixer from %d to %d gain %d:%s \n", 
           src,dst,val,snd_strerror(err));
  else
    printf("Set Mixer from %d to %d : %d \n",src,dst,val);
  
  
  return err;
}

static void hdspm_mixer_set_card_id(t_hdspm_mixer *x, t_floatarg fcard_id) // card_id = devnr ... ALSA-Device eg. for hw:0 is 0
{
  if (fcard_id < 0.0)
    fcard_id = 0.0;
  x->x_card_id = (int)fcard_id;
  if(x->x_card_id < HDSPMM_MAX_CARDS)
    snprintf(x->x_card_name[x->x_card_id], 6, "hw:%i", x->x_card_id);
  else
    post("hdspm_mixer: ALSA-Device-Number greater than 2 (0/1/2)");
}

int set_gain(int idx, int src, int dst, int val)
{
  int err;


  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *ctl;
  snd_ctl_t *handle;

  if(idx >= HDSPMM_MAX_CARDS || idx < 0)
	return HDSPMM_ERROR_WRONG_IDX;

  snd_ctl_elem_value_alloca(&ctl);
  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_id_set_name(id, "Mixer");
  snd_ctl_elem_id_set_numid(id, 0);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_HWDEP);
  snd_ctl_elem_id_set_device(id, 0);
  snd_ctl_elem_id_set_subdevice(id, 0);
  snd_ctl_elem_id_set_index(id, 0);
  snd_ctl_elem_value_set_id(ctl, id);

  if ((err = snd_ctl_open(&handle, card_name[idx], SND_CTL_NONBLOCK)) < 0) {
#ifndef QUIET
    fprintf(stderr, "Alsa error: %s\n", snd_strerror(err));
#endif
    return HDSPMM_ERROR_ALSA_OPEN;
  }

  snd_ctl_elem_value_set_integer(ctl, 0, src);
  snd_ctl_elem_value_set_integer(ctl, 1, dst);
  snd_ctl_elem_value_set_integer(ctl, 2, val);

  if ((err = snd_ctl_elem_write(handle, ctl)) < 0) {
#ifndef QUIET
    fprintf(stderr, "Alsa error: %s\n", snd_strerror(err));
#endif
    return HDSPMM_ERROR_ALSA_WRITE;
  }

  val = snd_ctl_elem_value_get_integer(ctl, 2);

  snd_ctl_close(handle);
  return val;
}

int get_gain(int idx, int src, int dst)
{
  int err;
  int val = HDSPMM_ERROR_NO_CARD;

  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *ctl;
  snd_ctl_t *handle;

  if(idx >= HDSPMM_MAX_CARDS || idx < 0)
	return HDSPMM_ERROR_WRONG_IDX;


  snd_ctl_elem_value_alloca(&ctl);
  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_id_set_name(id, "Mixer");
  snd_ctl_elem_id_set_numid(id, 0);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_HWDEP);
  snd_ctl_elem_id_set_device(id, 0);
  snd_ctl_elem_id_set_subdevice(id, 0);
  snd_ctl_elem_id_set_index(id, 0);
  snd_ctl_elem_value_set_id(ctl, id);

  if ((err = snd_ctl_open(&handle, card_name[cardid], SND_CTL_NONBLOCK)) < 0) {
#ifndef QUIET
    fprintf(stderr, "Alsa error: %s\n", snd_strerror(err));
#endif
    return HDSPMM_ERROR_ALSA_OPEN;
  }

  snd_ctl_elem_value_set_integer(ctl, 0, src);
  snd_ctl_elem_value_set_integer(ctl, 1, dst);

  if ((err = snd_ctl_elem_read(handle, ctl)) < 0) {
#ifndef QUIET
    fprintf(stderr, "Alsa error: %s\n", snd_strerror(err));
#endif
    return HDSPMM_ERROR_ALSA_READ;
  }
  val = snd_ctl_elem_value_get_integer(ctl, 2);

  snd_ctl_close(handle);

  return val;
  
  SETFLOAT(x->x_at+1, delta);
  SETFLOAT(x->x_at+2, phi);
  outlet_list(x->x_out_para, &s_list, 3, x->x_at);
  outlet_anything(x->x_obj.ob_outlet, s, x->x_size2d+1, x->x_at);
}





static void hdspm_mixer_get_matrix_element(t_hdspm_mixer *x, t_symbol *s, int argc, t_atom *argv)
{
  int col;
  int help_col, src_index, n=x->x_n_src;
  
  if((argc >= 2)&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1))
  {
    src_index = (int)atom_getintarg(0, argc, argv) - 1;
  }
}

static void *hdspm_mixer_new(void)
{
  t_hdspm_mixer *x = (t_hdspm_mixer *)pd_new(hdspm_mixer_class);
  
  if(hdspm_mixer_find_cards(x) < 0)
    post("hdspm_mixe: No Hammerfall DSP MADI card found!");
  x->x_card_id = 0;
  outlet_new(&x->x_ob, &s_list);
  return (x);
}

void hdspm_mixer_setup(void)
{
  hdspm_mixer_class = class_new(gensym("hdspm_mixer"), (t_newmethod)hdspm_mixer_new, 0, sizeof(t_hdspm_mixer), 0, 0);
  class_addmethod(hdspm_mixer_class, (t_method)hdspm_mixer_get_matrix_element, gensym("get_matrix_element"), A_GIMME, 0);
  class_addmethod(hdspm_mixer_class, (t_method)hdspm_mixer_set_matrix_element, gensym("set_matrix_element"), A_GIMME, 0);
  class_addmethod(hdspm_mixer_class, (t_method)hdspm_mixer_set_card_id, gensym("set_card_id"), A_FLOAT, 0);
  post("hdspm_mixer (V-%s) loaded!   (c) Winfried Ritsch, Thomas Musil 11.2008", HDSPMM_VERSION);
	post("   ritsch%ciem.at, musil%ciem.at iem KUG Graz Austria", '@', '@');
}
