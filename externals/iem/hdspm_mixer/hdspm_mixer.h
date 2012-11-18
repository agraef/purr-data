/* Very simple Mixer for RME DSP-MADI and maybe other hammerfall dsp 
   (C) 2003 IEM, Winfried Ritsch  (ritsch at iem.at)
   (c) 2008 iem, Thomas Musil (musil at iem.at) ... modified to a pd external
   Institute of Electronic Music and Acoustics
   GPL - see Licence.txt

   Header
*/


/* globals */
#define HDSPMM_VERSION "0.6"
#define HDSPMM_MAX_CARDS 3
#define HDSPMM_MAX_NAME_LEN 128

#define IS_A_FLOAT(atom,index) ((atom+index)->a_type == A_FLOAT)


/* Protos */

int get_gain(int idx, int src, int dst);
int set_gain(int idx, int src, int dst, int val);
int find_cards();

/* Error Codes */

#define HDSPMM_ERROR_WRONG_IDX    -1
#define HDSPMM_ERROR_ALSA_OPEN    -2
#define HDSPMM_ERROR_ALSA_WRITE    -3
#define HDSPMM_ERROR_ALSA_READ    -4
#define HDSPMM_ERROR_NO_CARD -5
