/******************************************************
 *
 * amixer - ALSA sequencer connection manager
 * Copyright (C) 1999-2000 Jaroslav Kysela
 *
 *
 *
 * ported from alsa-1.0.9a to pd by:
 * copyleft (c) 2005 IOhannes m zmölnig
 *
 *   forum::für::umläute
 *
 *   institute of electronic music and acoustics (iem)
 *
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/
#include "m_pd.h"

/* amixer :: allows control of the mixer (controls) for the ALSA soundcard driver */

#ifdef HAVE_ALSA
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <getopt.h>
# include <stdarg.h>
# include <ctype.h>
# include <math.h>
# include <errno.h>
# include <assert.h>
# include <alsa/asoundlib.h>
# include <sys/poll.h>

#define LEVEL_BASIC		(1<<0)
#define LEVEL_INACTIVE		(1<<1)
#define LEVEL_ID		(1<<2)

#endif /* ALSA */


static t_class *amixer_class;

typedef struct _amixer
{
  t_object x_obj;
  t_outlet*x_error;

  char card[64];
} t_amixer;


#ifdef HAVE_ALSA

static const char *control_iface(snd_ctl_elem_id_t *id)
{
  return snd_ctl_elem_iface_name(snd_ctl_elem_id_get_interface(id));
}

static const char *control_type(snd_ctl_elem_info_t *info)
{
  return snd_ctl_elem_type_name(snd_ctl_elem_info_get_type(info));
}

static const char *control_access(snd_ctl_elem_info_t *info)
{
  static char result[10];
  char *res = result;

  *res++ = snd_ctl_elem_info_is_readable(info) ? 'r' : '-';
  *res++ = snd_ctl_elem_info_is_writable(info) ? 'w' : '-';
  *res++ = snd_ctl_elem_info_is_inactive(info) ? 'i' : '-';
  *res++ = snd_ctl_elem_info_is_volatile(info) ? 'v' : '-';
  *res++ = snd_ctl_elem_info_is_locked(info) ? 'l' : '-';
  *res++ = '\0';
  return result;
}


static int check_range(int val, int min, int max)
{
  if (val < min)
    return min;
  if (val > max)
    return max;
  return val;
}

#if 0
static int convert_range(int val, int omin, int omax, int nmin, int nmax)
{
  int orange = omax - omin, nrange = nmax - nmin;
	
  if (orange == 0)
    return 0;
  return rint((((double)nrange * ((double)val - (double)omin)) + ((double)orange / 2.0)) / ((double)orange + (double)nmin));
}
#endif

#if 0
static int convert_db_range(int val, int omin, int omax, int nmin, int nmax)
{
  int orange = omax - omin, nrange = nmax - nmin;
	
  if (orange == 0)
    return 0;
  return rint((((double)nrange * ((double)val - (double)omin)) + ((double)orange / 2.0)) / (double)orange + (double)nmin);
}
#endif

/* Fuction to convert from volume to percentage. val = volume */

static int convert_prange(int val, int min, int max)
{
  int range = max - min;
  int tmp;

  if (range == 0)
    return 0;
  val -= min;
  tmp = rint((double)val/(double)range * 100);
  return tmp;
}

/* Function to convert from percentage to volume. val = percentage */

static int convert_prange1(int val, int min, int max)
{
  int range = max - min;
  int tmp;

  if (range == 0)
    return 0;

  tmp = rint((double)range * ((double)val*.01)) + min;
  return tmp;
}

static const char *get_percent(int val, int min, int max)
{
  static char str[32];
  int p;
	
  p = convert_prange(val, min, max);
  sprintf(str, "%i [%i%%]", val, p);
  return str;
}

#if 0
static const char *get_percent1(int val, int min, int max, int min_dB, int max_dB)
{
  static char str[32];
  int p, db;

  p = convert_prange(val, min, max);
  db = convert_db_range(val, min, max, min_dB, max_dB);
  sprintf(str, "%i [%i%%] [%i.%02idB]", val, p, db / 100, abs(db % 100));
  return str;
}
#endif

static long get_integer(char **ptr, long min, long max)
{
  int tmp, tmp1, tmp2;

  if (**ptr == ':')
    (*ptr)++;
  if (**ptr == '\0' || (!isdigit(**ptr) && **ptr != '-'))
    return min;
  tmp = strtol(*ptr, ptr, 10);
  tmp1 = tmp;
  tmp2 = 0;
  if (**ptr == '.') {
    (*ptr)++;
    tmp2 = strtol(*ptr, ptr, 10);
  }
  if (**ptr == '%') {
    tmp1 = convert_prange1(tmp, min, max);
    (*ptr)++;
  }
  tmp1 = check_range(tmp1, min, max);
  if (**ptr == ',')
    (*ptr)++;
  return tmp1;
}

static long get_integer64(char **ptr, long long min, long long max)
{
  long long tmp, tmp1, tmp2;

  if (**ptr == ':')
    (*ptr)++;
  if (**ptr == '\0' || (!isdigit(**ptr) && **ptr != '-'))
    return min;
  tmp = strtol(*ptr, ptr, 10);
  tmp1 = tmp;
  tmp2 = 0;
  if (**ptr == '.') {
    (*ptr)++;
    tmp2 = strtol(*ptr, ptr, 10);
  }
  if (**ptr == '%') {
    tmp1 = convert_prange1(tmp, min, max);
    (*ptr)++;
  }
  tmp1 = check_range(tmp1, min, max);
  if (**ptr == ',')
    (*ptr)++;
  return tmp1;
}

static int get_volume_simple(char **ptr, int min, int max, int orig)
{
  int tmp, tmp1, tmp2;

  if (**ptr == ':')
    (*ptr)++;
  if (**ptr == '\0' || (!isdigit(**ptr) && **ptr != '-'))
    return min;
  tmp = atoi(*ptr);
  if (**ptr == '-')
    (*ptr)++;
  while (isdigit(**ptr))
    (*ptr)++;
  tmp1 = tmp;
  tmp2 = 0;
  if (**ptr == '.') {
    (*ptr)++;
    tmp2 = atoi(*ptr);
    while (isdigit(**ptr))
      (*ptr)++;
  }
  if (**ptr == '%') {
    tmp1 = convert_prange1(tmp, min, max);
    (*ptr)++;
  }
  if (**ptr == '+') {
    tmp1 = orig + tmp1;
    (*ptr)++;
  } else if (**ptr == '-') {
    tmp1 = orig - tmp1;
    (*ptr)++;
  }
  tmp1 = check_range(tmp1, min, max);
  if (**ptr == ',')
    (*ptr)++;
  return tmp1;
}

static int get_bool_simple(char **ptr, char *str, int invert, int orig)
{
  if (**ptr == ':')
    (*ptr)++;
  if (!strncasecmp(*ptr, str, strlen(str))) {
    orig = 1 ^ (invert ? 1 : 0);
    while (**ptr != '\0' && **ptr != ',' && **ptr != ':')
      (*ptr)++;
  }
  if (**ptr == ',' || **ptr == ':')
    (*ptr)++;
  return orig;
}

static int parse_control_id(const char *str, snd_ctl_elem_id_t *id)
{
  int c, size;
  char *ptr;

  while (*str == ' ' || *str == '\t')
    str++;
  if (!(*str))
    return -EINVAL;
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);	/* default */
  while (*str) {
    if (!strncasecmp(str, "numid=", 6)) {
      str += 6;
      snd_ctl_elem_id_set_numid(id, atoi(str));
      while (isdigit(*str))
        str++;
    } else if (!strncasecmp(str, "iface=", 6)) {
      str += 6;
      if (!strncasecmp(str, "card", 4)) {
        snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_CARD);
        str += 4;
      } else if (!strncasecmp(str, "mixer", 5)) {
        snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
        str += 5;
      } else if (!strncasecmp(str, "pcm", 3)) {
        snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_PCM);
        str += 3;
      } else if (!strncasecmp(str, "rawmidi", 7)) {
        snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_RAWMIDI);
        str += 7;
      } else if (!strncasecmp(str, "timer", 5)) {
        snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_TIMER);
        str += 5;
      } else if (!strncasecmp(str, "sequencer", 9)) {
        snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_SEQUENCER);
        str += 9;
      } else {
        return -EINVAL;
      }
    } else if (!strncasecmp(str, "name=", 5)) {
      char buf[64];
      str += 5;
      ptr = buf;
      size = 0;
      if (*str == '\'' || *str == '\"') {
        c = *str++;
        while (*str && *str != c) {
          if (size < (int)sizeof(buf)) {
            *ptr++ = *str;
            size++;
          }
          str++;
        }
        if (*str == c)
          str++;
      } else {
        while (*str && *str != ',') {
          if (size < (int)sizeof(buf)) {
            *ptr++ = *str;
            size++;
          }
          str++;
        }
        *ptr = '\0';
      }
      snd_ctl_elem_id_set_name(id, buf);
    } else if (!strncasecmp(str, "index=", 6)) {
      str += 6;
      snd_ctl_elem_id_set_index(id, atoi(str));
      while (isdigit(*str))
        str++;
    } else if (!strncasecmp(str, "device=", 7)) {
      str += 7;
      snd_ctl_elem_id_set_device(id, atoi(str));
      while (isdigit(*str))
        str++;
    } else if (!strncasecmp(str, "subdevice=", 10)) {
      str += 10;
      snd_ctl_elem_id_set_subdevice(id, atoi(str));
      while (isdigit(*str))
        str++;
    }
    if (*str == ',') {
      str++;
    } else {
      if (*str)
        return -EINVAL;
    }
  }			
  return 0;
}

static int parse_simple_id(const char *str, snd_mixer_selem_id_t *sid)
{
  int c, size;
  char buf[128];
  char *ptr = buf;

  while (*str == ' ' || *str == '\t')
    str++;
  if (!(*str))
    return -EINVAL;
  size = 1;	/* for '\0' */
  if (*str != '"' && *str != '\'') {
    while (*str && *str != ',') {
      if (size < (int)sizeof(buf)) {
        *ptr++ = *str;
        size++;
      }
      str++;
    }
  } else {
    c = *str++;
    while (*str && *str != c) {
      if (size < (int)sizeof(buf)) {
        *ptr++ = *str;
        size++;
      }
      str++;
    }
    if (*str == c)
      str++;
  }
  if (*str == '\0') {
    snd_mixer_selem_id_set_index(sid, 0);
    *ptr = 0;
    goto _set;
  }
  if (*str != ',')
    return -EINVAL;
  *ptr = 0;	/* terminate the string */
  str++;
  if (!isdigit(*str))
    return -EINVAL;
  snd_mixer_selem_id_set_index(sid, atoi(str));
 _set:
  snd_mixer_selem_id_set_name(sid, buf);
  return 0;
}
static void show_control_id(snd_ctl_elem_id_t *id)
{
  unsigned int index, device, subdevice;
  printf("numid=%u,iface=%s,name='%s'",
         snd_ctl_elem_id_get_numid(id),
         control_iface(id),
         snd_ctl_elem_id_get_name(id));
  index = snd_ctl_elem_id_get_index(id);
  device = snd_ctl_elem_id_get_device(id);
  subdevice = snd_ctl_elem_id_get_subdevice(id);
  if (index)
    printf(",index=%i", index);
  if (device)
    printf(",device=%i", device);
  if (subdevice)
    printf(",subdevice=%i", subdevice);
}

static int show_control(const char* card, const char *space, snd_hctl_elem_t *elem,
			int level)
{
  int err;
  unsigned int item, idx;
  unsigned int count;
  snd_ctl_elem_type_t type;
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_info_t *info;
  snd_ctl_elem_value_t *control;
  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_info_alloca(&info);
  snd_ctl_elem_value_alloca(&control);
  if ((err = snd_hctl_elem_info(elem, info)) < 0) {
    error("Control %s snd_hctl_elem_info error: %s\n", card, snd_strerror(err));
    return err;
  }
  if (level & LEVEL_ID) {
    snd_hctl_elem_get_id(elem, id);
    show_control_id(id);
    printf("\n");
  }
  count = snd_ctl_elem_info_get_count(info);
  type = snd_ctl_elem_info_get_type(info);
  printf("%s; type=%s,access=%s,values=%i", space, control_type(info), control_access(info), count);
  switch (type) {
  case SND_CTL_ELEM_TYPE_INTEGER:
    printf(",min=%li,max=%li,step=%li\n", 
           snd_ctl_elem_info_get_min(info),
           snd_ctl_elem_info_get_max(info),
           snd_ctl_elem_info_get_step(info));
    break;
  case SND_CTL_ELEM_TYPE_INTEGER64:
    printf(",min=%Li,max=%Li,step=%Li\n", 
           snd_ctl_elem_info_get_min64(info),
           snd_ctl_elem_info_get_max64(info),
           snd_ctl_elem_info_get_step64(info));
    break;
  case SND_CTL_ELEM_TYPE_ENUMERATED:
    {
      unsigned int items = snd_ctl_elem_info_get_items(info);
      printf(",items=%u\n", items);
      for (item = 0; item < items; item++) {
        snd_ctl_elem_info_set_item(info, item);
        if ((err = snd_hctl_elem_info(elem, info)) < 0) {
          error("Control %s element info error: %s\n", card, snd_strerror(err));
          return err;
        }
        printf("%s; Item #%u '%s'\n", space, item, snd_ctl_elem_info_get_item_name(info));
      }
      break;
    }
  default:
    printf("\n");
    break;
  }
  if (level & LEVEL_BASIC) {
    if ((err = snd_hctl_elem_read(elem, control)) < 0) {
      error("Control %s element read error: %s\n", card, snd_strerror(err));
      return err;
    }
    printf("%s: values=", space);
    for (idx = 0; idx < count; idx++) {
      if (idx > 0)
        printf(",");
      switch (type) {
      case SND_CTL_ELEM_TYPE_BOOLEAN:
        printf("%s", snd_ctl_elem_value_get_boolean(control, idx) ? "on" : "off");
        break;
      case SND_CTL_ELEM_TYPE_INTEGER:
        printf("%li", snd_ctl_elem_value_get_integer(control, idx));
        break;
      case SND_CTL_ELEM_TYPE_INTEGER64:
        printf("%Li", snd_ctl_elem_value_get_integer64(control, idx));
        break;
      case SND_CTL_ELEM_TYPE_ENUMERATED:
        printf("%u", snd_ctl_elem_value_get_enumerated(control, idx));
        break;
      case SND_CTL_ELEM_TYPE_BYTES:
        printf("0x%02x", snd_ctl_elem_value_get_byte(control, idx));
        break;
      default:
        printf("?");
        break;
      }
    }
    printf("\n");
  }
  return 0;
}



static void amixer_bang(t_amixer *x)
{



}
static int amixer_control(t_amixer *x, int argc, t_atom *argv, int roflag)
{
  int err;
  snd_ctl_t *handle;
  snd_ctl_elem_info_t *info;
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;
  char *ptr;
  unsigned int idx, count;
  long tmp;
  snd_ctl_elem_type_t type;
  snd_ctl_elem_info_alloca(&info);
  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);

  if (argc < 1) {
    error("Specify a full control identifier: [[iface=<iface>,][name='name',][index=<index>,][device=<device>,][subdevice=<subdevice>]]|[numid=<numid>]\n");
    return -EINVAL;
  }
  if(A_FLOAT==argv->a_type){
    snd_ctl_elem_id_set_numid(id, atom_getint(argv));
    if(0)
      {
        error("Wrong control identifier: %d\n", atom_getint(argv));
        return -EINVAL;
      }
  } else {
    if (parse_control_id(atom_getsymbol(argv)->s_name, id)) {
      error("Wrong control identifier: %s\n", atom_getsymbol(argv)->s_name);
      return -EINVAL;
    }
  }
  if ((err = snd_ctl_open(&handle, x->card, 0)) < 0) {
    error("Control %s open error: %s\n", x->card, snd_strerror(err));
    return err;
  }
  snd_ctl_elem_info_set_id(info, id);
  if ((err = snd_ctl_elem_info(handle, info)) < 0) {
    error("Control %s cinfo error: %s\n", x->card, snd_strerror(err));
    return err;
  }

  snd_ctl_elem_info_get_id(info, id);	/* FIXME: Remove it when hctl find works ok !!! */
  type = snd_ctl_elem_info_get_type(info);
  count = snd_ctl_elem_info_get_count(info);
  snd_ctl_elem_value_set_id(control, id);

  if (!roflag) {
    t_float atom_float = atom_getfloat(argv+1);
    int atom_isfloat = (A_FLOAT==(argv+1)->a_type);
    post("now setting");
    ptr = atom_getsymbol(argv+1)->s_name;
    for (idx = 0; idx < count && idx < 128 && ptr && *ptr; idx++) {
      switch (type) {
      case SND_CTL_ELEM_TYPE_BOOLEAN:
        tmp = 0;
        if(atom_isfloat){
          tmp=(atom_float>0)?1:0;
        } else if (!strncasecmp(ptr, "on", 2) || !strncasecmp(ptr, "up", 2)) {
          tmp = 1;
          ptr += 2;
        } else if (!strncasecmp(ptr, "yes", 3)) {
          tmp = 1;
          ptr += 3;
        } else if (!strncasecmp(ptr, "toggle", 6)) {
          tmp = snd_ctl_elem_value_get_boolean(control, idx);
          tmp = tmp > 0 ? 0 : 1;
          ptr += 6;
        } else if (isdigit(*ptr)) {
          tmp = atoi(ptr) > 0 ? 1 : 0;
          while (isdigit(*ptr))
            ptr++;
        } else {
          while (*ptr && *ptr != ',')
            ptr++;
        }
        snd_ctl_elem_value_set_boolean(control, idx, tmp);
        break;
      case SND_CTL_ELEM_TYPE_INTEGER:
        tmp = atom_isfloat?((int)atom_float):get_integer(&ptr,
                                                  snd_ctl_elem_info_get_min(info),
                                                  snd_ctl_elem_info_get_max(info));
        snd_ctl_elem_value_set_integer(control, idx, tmp);
        break;
      case SND_CTL_ELEM_TYPE_INTEGER64:
        tmp = atom_isfloat?((int)atom_float):get_integer64(&ptr,
                                                    snd_ctl_elem_info_get_min64(info),
                                                    snd_ctl_elem_info_get_max64(info));
        snd_ctl_elem_value_set_integer64(control, idx, tmp);
        break;
      case SND_CTL_ELEM_TYPE_ENUMERATED:
        tmp =  atom_isfloat?((int)atom_float):get_integer(&ptr, 0, snd_ctl_elem_info_get_items(info) - 1);
        snd_ctl_elem_value_set_enumerated(control, idx, tmp);
        break;
      case SND_CTL_ELEM_TYPE_BYTES:
        tmp =  atom_isfloat?((int)atom_float):get_integer(&ptr, 0, 255);
        snd_ctl_elem_value_set_byte(control, idx, tmp);
        break;
      default:
        break;
      }
      if (!strchr(atom_getsymbol(argv+1)->s_name, ','))
        ptr = atom_getsymbol(argv+1)->s_name;
      else if (*ptr == ',')
        ptr++;
    }
    if ((err = snd_ctl_elem_write(handle, control)) < 0) {
      error("Control %s element write error: %s\n", x->card, snd_strerror(err));
      return err;
    }
  }
  snd_ctl_close(handle);


  if (1) {
    snd_hctl_t *hctl;
    snd_hctl_elem_t *elem;
    if ((err = snd_hctl_open(&hctl, x->card, 0)) < 0) {
      error("Control %s open error: %s\n", x->card, snd_strerror(err));
      return err;
    }
    if ((err = snd_hctl_load(hctl)) < 0) {
      error("Control %s load error: %s\n", x->card, snd_strerror(err));
      return err;
    }
    elem = snd_hctl_find_elem(hctl, id);
    if (elem)
      show_control(x->card, "  ", elem, LEVEL_BASIC | LEVEL_ID);
    else
      printf("Could not find the specified element\n");
    snd_hctl_close(hctl);
  }
}
static void amixer_cget(t_amixer *x, t_symbol *s, int argc, t_atom *argv)
{
  amixer_control(x, argc, argv, 1);
}
static void amixer_cset(t_amixer *x, t_symbol *s, int argc, t_atom *argv)
{
  amixer_control(x, argc, argv, 0);
}

static void amixer_card(t_amixer *x, t_symbol *s, int argc, t_atom *argv)
{
  if(1==argc){
    int id=-1;
    if(A_FLOAT==argv->a_type)id=atom_getint(argv);
    else id=snd_card_get_index(atom_getsymbol(argv)->s_name);
    if (id >= 0 && id < 32)
      sprintf(x->card, "hw:%i", id);
    else {
      pd_error(x, "invalid card %d", id);
      return;
    }


  } else error("amixer: can only be integer of symbol");
}

#endif /* ALSA */

static void amixer_free(t_amixer *x){
#ifdef HAVE_ALSA


#endif /* ALSA */
}


static void *amixer_new(void)
{
  t_amixer *x = (t_amixer *)pd_new(amixer_class);
  outlet_new(&x->x_obj, 0);
  x->x_error=outlet_new(&x->x_obj, 0);

#ifndef HAVE_ALSA
  error("amixer: compiled without ALSA-suppor !!");
  error("amixer: no functionality enabled!");
#else
  sprintf(x->card, "default");

#endif /* !ALSA */

  return (x);
}


void amixer_setup(void)
{
  post("amixer: ALSA soundcard control");
  post("          Copyright (C) 1999-2000 Jaroslav Kysela");
  post("          ported to pure-data by IOhannes m zmölnig 2005");
  post("          institute of electronic music and acoustics (iem)");
  post("          published under the GNU General Public License version 2");
#ifdef AMIXER_VERSION
  startpost("          version:"AMIXER_VERSION);
#endif
  post("\tcompiled: "__DATE__"");

  amixer_class = class_new(gensym("amixer"), (t_newmethod)amixer_new, (t_method)amixer_free,
                           sizeof(t_amixer), 0, 0);
#ifdef HAVE_ALSA
  class_addmethod(amixer_class, (t_method)amixer_card,gensym("card"), A_GIMME, 0);
  class_addmethod(amixer_class, (t_method)amixer_cget,gensym("get"), A_GIMME, 0);
  class_addmethod(amixer_class, (t_method)amixer_cset,gensym("set"), A_GIMME, 0);
  class_addmethod(amixer_class, (t_method)amixer_cget,gensym("cget"), A_GIMME, 0);
  class_addmethod(amixer_class, (t_method)amixer_cset,gensym("cset"), A_GIMME, 0);

  //  class_addmethod(amixer_class, (t_method)amixer_listdevices,gensym(""), A_DEFSYM, 0);
  class_addbang(amixer_class, (t_method)amixer_bang);
#endif /* ALSA */
}

