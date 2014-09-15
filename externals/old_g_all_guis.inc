// version of g_all_guis.[ch] made by Mathieu Bouchard
// so that GUI externals that want the legacy version of
// iemgui internal classes can still work in pd-l2ork
// in aug.2014 and after.

/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution. */
/* g_7_guis.h written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */


#define IEM_GUI_COLNR_WHITE          0
#define IEM_GUI_COLNR_ML_GREY        1
#define IEM_GUI_COLNR_D_GREY         2
#define IEM_GUI_COLNR_L_RED          3
#define IEM_GUI_COLNR_L_ORANGE       4
#define IEM_GUI_COLNR_L_YELLOW       5
#define IEM_GUI_COLNR_L_GREEN        6
#define IEM_GUI_COLNR_L_CYAN         7
#define IEM_GUI_COLNR_L_BLUE         8
#define IEM_GUI_COLNR_L_MAGENTA      9

#define IEM_GUI_COLNR_LL_GREY        10
#define IEM_GUI_COLNR_M_GREY         11
#define IEM_GUI_COLNR_DD_GREY        12
#define IEM_GUI_COLNR_RED            13
#define IEM_GUI_COLNR_ORANGE         14
#define IEM_GUI_COLNR_YELLOW         15
#define IEM_GUI_COLNR_GREEN          16
#define IEM_GUI_COLNR_CYAN           17
#define IEM_GUI_COLNR_BLUE           18
#define IEM_GUI_COLNR_MAGENTA        19

#define IEM_GUI_COLNR_L_GREY         20
#define IEM_GUI_COLNR_MD_GREY        21
#define IEM_GUI_COLNR_BLACK          22
#define IEM_GUI_COLNR_D_RED          23
#define IEM_GUI_COLNR_D_ORANGE       24
#define IEM_GUI_COLNR_D_YELLOW       25
#define IEM_GUI_COLNR_D_GREEN        26
#define IEM_GUI_COLNR_D_CYAN         27
#define IEM_GUI_COLNR_D_BLUE         28
#define IEM_GUI_COLNR_D_MAGENTA      29

#define IEM_GUI_COLOR_SELECTED       255
#define IEM_GUI_COLOR_NORMAL         0

#define IEM_GUI_MAX_COLOR            30

#define IEM_GUI_DEFAULTSIZE 15
#define IEM_GUI_MINSIZE 8
#define IEM_GUI_MAXSIZE 1000
#define IEM_SL_DEFAULTSIZE 128
#define IEM_SL_MINSIZE 2
#define IEM_FONT_MINSIZE 4

#define IEM_BNG_DEFAULTHOLDFLASHTIME 250
#define IEM_BNG_DEFAULTBREAKFLASHTIME 50
#define IEM_BNG_MINHOLDFLASHTIME 50
#define IEM_BNG_MINBREAKFLASHTIME 10

#define IEM_VU_DEFAULTSIZE 3
#define IEM_VU_LARGESMALL  2
#define IEM_VU_MINSIZE     2
#define IEM_VU_MAXSIZE     25
#define IEM_VU_STEPS       40

#define IEM_VU_MINDB    -99.9
#define IEM_VU_MAXDB    12.0
#define IEM_VU_OFFSET   100.0

#define IEM_RADIO_MAX   128

#define IEM_SYM_UNIQUE_SND  256
#define IEM_SYM_UNIQUE_RCV  512
#define IEM_SYM_UNIQUE_LAB  1024
#define IEM_SYM_UNIQUE_ALL  1792
#define IEM_FONT_STYLE_ALL  255

#define IEM_MAX_SYM_LEN      127

#define IEM_GUI_DRAW_MODE_UPDATE 0
#define IEM_GUI_DRAW_MODE_MOVE   1
#define IEM_GUI_DRAW_MODE_NEW    2
#define IEM_GUI_DRAW_MODE_SELECT 3
#define IEM_GUI_DRAW_MODE_ERASE  4
#define IEM_GUI_DRAW_MODE_CONFIG 5
#define IEM_GUI_DRAW_MODE_IO     6


#define IS_A_POINTER(atom,index) ((atom+index)->a_type == A_POINTER)
#define IS_A_FLOAT(atom,index) ((atom+index)->a_type == A_FLOAT)
#define IS_A_SYMBOL(atom,index) ((atom+index)->a_type == A_SYMBOL)
#define IS_A_DOLLAR(atom,index) ((atom+index)->a_type == A_DOLLAR)
#define IS_A_DOLLSYM(atom,index) ((atom+index)->a_type == A_DOLLSYM)

#define IEM_FSTYLE_FLAGS_ALL 0x007fffff
#define IEM_INIT_ARGS_ALL    0x01ffffff

#define IEM_GUI_OLD_SND_FLAG 1
#define IEM_GUI_OLD_RCV_FLAG 2

#define IEM_GUI_COLOR_EDITED 16711680
#define IEMGUI_MAX_NUM_LEN 32

typedef struct _iem_fstyle_flags
{
    unsigned int x_font_style:6;
    unsigned int x_rcv_able:1;
    unsigned int x_snd_able:1;
    unsigned int x_lab_is_unique:1;
    unsigned int x_rcv_is_unique:1;
    unsigned int x_snd_is_unique:1;
    unsigned int x_lab_arg_tail_len:6;
    unsigned int x_lab_is_arg_num:6;
    unsigned int x_shiftdown:1;
    unsigned int x_selected:2;
    unsigned int x_finemoved:1;
    unsigned int x_put_in2out:1;
    unsigned int x_change:1;
    unsigned int x_thick:1;
    unsigned int x_lin0_log1:1;
    unsigned int x_steady:1;
    unsigned int dummy:1;
} t_iem_fstyle_flags;

typedef struct _iem_init_symargs
{
    unsigned int x_loadinit:1;
    unsigned int x_rcv_arg_tail_len:6;
    unsigned int x_snd_arg_tail_len:6;
    unsigned int x_rcv_is_arg_num:6;
    unsigned int x_snd_is_arg_num:6;
    unsigned int x_scale:1;
    unsigned int x_flashed:1;
    unsigned int x_locked:1;
    unsigned int x_reverse:1; /* bugfix */
    unsigned int dummy:3;
} t_iem_init_symargs;

typedef void (*t_iemfunptr)(void *x, t_glist *glist, int mode);

typedef struct _iemgui
{
    t_object           x_obj;
    t_glist            *x_glist;
    t_iemfunptr        x_draw;
    int                x_h;
    int                x_w;
    int                x_ldx;
    int                x_ldy;
    char               x_font[MAXPDSTRING]; /* font names can be long! */
    t_iem_fstyle_flags x_fsf;
    int                x_fontsize;
    t_iem_init_symargs x_isa;
    int                x_fcol;
    int                x_bcol;
    int                x_lcol;
    t_symbol           *x_snd;              /* send symbol */
    t_symbol           *x_rcv;              /* receive */
    t_symbol           *x_lab;              /* label */
    t_symbol           *x_snd_unexpanded;   /* same 3, with '$' unexpanded */
    t_symbol           *x_rcv_unexpanded;
    t_symbol           *x_lab_unexpanded;
    int                x_binbufindex;       /* where in binbuf to find these */
    int                x_labelbindex;       /* where in binbuf to find label */
} t_iemgui;

typedef struct _iemguidummy
{
    t_iemgui x_gui;
    int      x_dum1;
    int      x_dum2;
    int      x_dum3;
} t_iemguidummy;

typedef struct _bng
{
    t_iemgui x_gui;
    int      x_flashed;
    int      x_flashtime_break;
    int      x_flashtime_hold;
    t_clock  *x_clock_hld;
    t_clock  *x_clock_brk;
    t_clock  *x_clock_lck;
} t_bng;

typedef struct _hslider
{
    t_iemgui x_gui;
    int      x_pos;
    int      x_val;
    int      x_center;
    int      x_thick;
    int      x_lin0_log1;
    int      x_steady;
    double   x_min;
    double   x_max;
    double   x_k;
} t_hslider;

typedef struct _hdial
{
    t_iemgui x_gui;
    int      x_on;
    int      x_on_old;  /* LATER delete this; it's used for old version */
    int      x_change;
    int      x_number;
    int      x_drawn;
    t_atom   x_at[2];
} t_hdial;

typedef struct _toggle
{
    t_iemgui x_gui;
    t_float    x_on;
    t_float    x_nonzero;
} t_toggle;

typedef struct _my_canvas
{
    t_iemgui x_gui;
    t_atom   x_at[3];
    int      x_vis_w;
    int      x_vis_h;
} t_my_canvas;

typedef struct _vslider
{
    t_iemgui x_gui;
    int      x_pos;
    int      x_val;
    int      x_lin0_log1;
    int      x_steady;
    double   x_min;
    double   x_max;
    double   x_k;
} t_vslider;

typedef struct _vu
{
    t_iemgui x_gui;
    int      x_led_size;
    int      x_peak;
    int      x_rms;
    t_float    x_fp;
    t_float    x_fr;
    int      x_scale;
    void     *x_out_rms;
    void     *x_out_peak;
    unsigned int x_updaterms:1;
    unsigned int x_updatepeak:1;
} t_vu;

typedef struct _my_numbox
{
    t_iemgui x_gui;
    t_clock  *x_clock_reset;
    t_clock  *x_clock_wait;
    double   x_val;
    double   x_min;
    double   x_max;
    double   x_k;
    int      x_lin0_log1;
    char     x_buf[IEMGUI_MAX_NUM_LEN];
    int      x_numwidth;
    int      x_log_height;
} t_my_numbox;

typedef struct _vdial
{
    t_iemgui x_gui;
    int      x_on;
    int      x_on_old;
    int      x_change;
    int      x_number;
    int      x_drawn;
    t_atom   x_at[2];
} t_vdial;

#define t_vradio t_vdial
#define t_hradio t_hdial

extern int sys_noloadbang;
extern int iemgui_color_hex[];

static int iemgui_clip_size(int size);
static int iemgui_clip_font(int size);
static int iemgui_modulo_color(int col);
static t_symbol *iemgui_unique2dollarzero(t_symbol *s, int unique_num, int and_unique_flag);
static t_symbol *iemgui_sym2dollararg(t_symbol *s, int nth_arg, int tail_len);
static t_symbol *iemgui_dollarzero2unique(t_symbol *s, int unique_num);
static t_symbol *iemgui_dollararg2sym(t_symbol *s, int nth_arg, int tail_len, int pargc, t_atom *pargv);
static int iemgui_is_dollarzero(t_symbol *s);
static int iemgui_is_dollararg(t_symbol *s, int *tail_len);
static void iemgui_fetch_unique(t_iemgui *iemgui);
static void iemgui_fetch_parent_args(t_iemgui *iemgui, int *pargc, t_atom **pargv);
static void iemgui_verify_snd_ne_rcv(t_iemgui *iemgui);
static void iemgui_all_unique2dollarzero(t_iemgui *iemgui, t_symbol **srlsym);
static void iemgui_all_sym2dollararg(t_iemgui *iemgui, t_symbol **srlsym);
static void iemgui_all_dollarzero2unique(t_iemgui *iemgui, t_symbol **srlsym);
static t_symbol *iemgui_new_dogetname(t_iemgui *iemgui, int indx, t_atom *argv);
static void iemgui_new_getnames(t_iemgui *iemgui, int indx, t_atom *argv);
static void iemgui_all_dollararg2sym(t_iemgui *iemgui, t_symbol **srlsym);
static void iemgui_first_dollararg2sym(t_iemgui *iemgui, t_symbol **srlsym);
static void iemgui_all_col2save(t_iemgui *iemgui, int *bflcol);
static void iemgui_all_colfromload(t_iemgui *iemgui, int *bflcol);
static int iemgui_compatible_col(int i);
static void iemgui_all_dollar2raute(t_symbol **srlsym);
static void iemgui_all_raute2dollar(t_symbol **srlsym);
static void iemgui_send(void *x, t_iemgui *iemgui, t_symbol *s);
static void iemgui_receive(void *x, t_iemgui *iemgui, t_symbol *s);
static void iemgui_label(void *x, t_iemgui *iemgui, t_symbol *s);
static void iemgui_label_pos(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av);
static void iemgui_label_font(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av);
static void iemgui_size(void *x, t_iemgui *iemgui);
static void iemgui_delta(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av);
static void iemgui_pos(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av);
static void iemgui_color(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av);
static int iemgui_list(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av);
static void iemgui_displace(t_gobj *z, t_glist *glist, int dx, int dy);
static void iemgui_displace_withtag(t_gobj *z, t_glist *glist, int dx, int dy);
static void iemgui_select(t_gobj *z, t_glist *glist, int selected);
static void iemgui_delete(t_gobj *z, t_glist *glist);
static void iemgui_vis(t_gobj *z, t_glist *glist, int vis);
static void iemgui_save(t_iemgui *iemgui, t_symbol **srl, int *bflcol);
static void iemgui_properties(t_iemgui *iemgui, t_symbol **srl);
static int iemgui_dialog(t_iemgui *iemgui, t_symbol **srl, int argc, t_atom *argv);

EXTERN int canvas_getdollarzero(void);
EXTERN void canvas_getargs(int *argcp, t_atom **argvp);

static void iem_inttosymargs(t_iem_init_symargs *symargp, int n);
static int iem_symargstoint(t_iem_init_symargs *symargp);
static void iem_inttofstyle(t_iem_fstyle_flags *fstylep, int n);
static int iem_fstyletoint(t_iem_fstyle_flags *fstylep);

/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* g_7_guis.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */
/* thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "m_pd.h"
#include "g_canvas.h"
#include <math.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

/*  #define GGEE_HSLIDER_COMPATIBLE  */

/*------------------ global varaibles -------------------------*/

int iemgui_color_hex[]=
{
    16579836, 10526880, 4210752, 16572640, 16572608,
    16579784, 14220504, 14220540, 14476540, 16308476,
    14737632, 8158332, 2105376, 16525352, 16559172,
    15263784, 1370132, 2684148, 3952892, 16003312,
    12369084, 6316128, 0, 9177096, 5779456,
    7874580, 2641940, 17488, 5256, 5767248
};

int iemgui_vu_db2i[]=
{
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    9, 9, 9, 9, 9,10,10,10,10,10,
    11,11,11,11,11,12,12,12,12,12,
    13,13,13,13,14,14,14,14,15,15,
    15,15,16,16,16,16,17,17,17,18,
    18,18,19,19,19,20,20,20,21,21,
    22,22,23,23,24,24,25,26,27,28,
    29,30,31,32,33,33,34,34,35,35,
    36,36,37,37,37,38,38,38,39,39,
    39,39,39,39,40,40
};

int iemgui_vu_col[]=
{
    0,17,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,
    15,15,15,15,15,15,15,15,15,15,14,14,13,13,13,13,13,13,13,13,13,13,13,19,19,19
};

char *iemgui_vu_scale_str[]=
{
    "",
    "<-99",
    "",
    "",
    "",
    "-50",
    "",
    "",
    "",
    "-30",
    "",
    "",
    "",
    "-20",
    "",
    "",
    "",
    "-12",
    "",
    "",
    "",
    "-6",
    "",
    "",
    "",
    "-2",
    "",
    "",
    "",
    "-0dB",
    "",
    "",
    "",
    "+2",
    "",
    "",
    "",
    "+6",
    "",
    "",
    "",
    ">+12",
    "",
    "",
    "",
    "",
    "",
};


/*------------------ global functions -------------------------*/


int iemgui_clip_size(int size)
{
    if(size < IEM_GUI_MINSIZE)
        size = IEM_GUI_MINSIZE;
    return(size);
}

int iemgui_clip_font(int size)
{
    if(size < IEM_FONT_MINSIZE)
        size = IEM_FONT_MINSIZE;
    return(size);
}

int iemgui_modulo_color(int col)
{
    while(col >= IEM_GUI_MAX_COLOR)
        col -= IEM_GUI_MAX_COLOR;
    while(col < 0)
        col += IEM_GUI_MAX_COLOR;
    return(col);
}

t_symbol *iemgui_dollar2raute(t_symbol *s)
{
    char buf[MAXPDSTRING+1], *s1, *s2;
    if (strlen(s->s_name) >= MAXPDSTRING)
        return (s);
    for (s1 = s->s_name, s2 = buf; ; s1++, s2++)
    {
        if (*s1 == '$')
            *s2 = '#';
        else if (!(*s2 = *s1))
            break;
    }
    return(gensym(buf));
}

t_symbol *iemgui_raute2dollar(t_symbol *s)
{
    char buf[MAXPDSTRING+1], *s1, *s2;
    if (strlen(s->s_name) >= MAXPDSTRING)
        return (s);
    for (s1 = s->s_name, s2 = buf; ; s1++, s2++)
    {
        if (*s1 == '#')
            *s2 = '$';
        else if (!(*s2 = *s1))
            break;
    }
    return(gensym(buf));
}

void iemgui_verify_snd_ne_rcv(t_iemgui *iemgui)
{
    iemgui->x_fsf.x_put_in2out = 1;
    if(iemgui->x_fsf.x_snd_able && iemgui->x_fsf.x_rcv_able)
    {
        if(!strcmp(iemgui->x_snd->s_name, iemgui->x_rcv->s_name))
            iemgui->x_fsf.x_put_in2out = 0;
    }
}

t_symbol *iemgui_new_dogetname(t_iemgui *iemgui, int indx, t_atom *argv)
{
    if (IS_A_SYMBOL(argv, indx))
        return (atom_getsymbolarg(indx, 100000, argv));
    else if (IS_A_FLOAT(argv, indx))
    {
        char str[80];
        sprintf(str, "%d", (int)atom_getintarg(indx, 100000, argv));
        return (gensym(str));
    }
    else return (gensym("empty"));
}

void iemgui_new_getnames(t_iemgui *iemgui, int indx, t_atom *argv)
{
    if (argv)
    {
        iemgui->x_snd = iemgui_new_dogetname(iemgui, indx, argv);
        iemgui->x_rcv = iemgui_new_dogetname(iemgui, indx+1, argv);
        iemgui->x_lab = iemgui_new_dogetname(iemgui, indx+2, argv);
    }
    else iemgui->x_snd = iemgui->x_rcv = iemgui->x_lab = gensym("empty");
    iemgui->x_snd_unexpanded = iemgui->x_rcv_unexpanded =
        iemgui->x_lab_unexpanded = 0;
    iemgui->x_binbufindex = indx;
    iemgui->x_labelbindex = indx + 3;
}

    /* convert symbols in "$" form to the expanded symbols */
void iemgui_all_dollararg2sym(t_iemgui *iemgui, t_symbol **srlsym)
{
        /* save unexpanded ones for later */
    iemgui->x_snd_unexpanded = srlsym[0];
    iemgui->x_rcv_unexpanded = srlsym[1];
    iemgui->x_lab_unexpanded = srlsym[2];
    srlsym[0] = canvas_realizedollar(iemgui->x_glist, srlsym[0]);
    srlsym[1] = canvas_realizedollar(iemgui->x_glist, srlsym[1]);
    srlsym[2] = canvas_realizedollar(iemgui->x_glist, srlsym[2]);
}

    /* initialize a single symbol in unexpanded form.  We reach into the
    binbuf to grab them; if there's nothing there, set it to the
    fallback; if still nothing, set to "empty". */
static void iemgui_init_sym2dollararg(t_iemgui *iemgui, t_symbol **symp,
    int indx, t_symbol *fallback)
{
    if (!*symp)
    {
        t_binbuf *b = iemgui->x_obj.ob_binbuf;
        if (binbuf_getnatom(b) > indx)
        {
            char buf[80];
            atom_string(binbuf_getvec(b) + indx, buf, 80);
            *symp = gensym(buf);
        }
        else if (fallback)
            *symp = fallback;
        else *symp = gensym("empty");
    }
}

    /* get the unexpanded versions of the symbols; initialize them if
    necessary. */
void iemgui_all_sym2dollararg(t_iemgui *iemgui, t_symbol **srlsym)
{
    iemgui_init_sym2dollararg(iemgui, &iemgui->x_snd_unexpanded,
        iemgui->x_binbufindex+1, iemgui->x_snd);
    iemgui_init_sym2dollararg(iemgui, &iemgui->x_rcv_unexpanded,
        iemgui->x_binbufindex+2, iemgui->x_rcv);
    iemgui_init_sym2dollararg(iemgui, &iemgui->x_lab_unexpanded,
        iemgui->x_labelbindex, iemgui->x_lab);
    srlsym[0] = iemgui->x_snd_unexpanded;
    srlsym[1] = iemgui->x_rcv_unexpanded;
    srlsym[2] = iemgui->x_lab_unexpanded;
}

void iemgui_first_dollararg2sym(t_iemgui *iemgui, t_symbol **srlsym)
{
    /* delete this function */
}

void iemgui_all_col2save(t_iemgui *iemgui, int *bflcol)
{
    bflcol[0] = -1 - (((0xfc0000 & iemgui->x_bcol) >> 6)|
                      ((0xfc00 & iemgui->x_bcol) >> 4)|((0xfc & iemgui->x_bcol) >> 2));
    bflcol[1] = -1 - (((0xfc0000 & iemgui->x_fcol) >> 6)|
                      ((0xfc00 & iemgui->x_fcol) >> 4)|((0xfc & iemgui->x_fcol) >> 2));
    bflcol[2] = -1 - (((0xfc0000 & iemgui->x_lcol) >> 6)|
                      ((0xfc00 & iemgui->x_lcol) >> 4)|((0xfc & iemgui->x_lcol) >> 2));
}

void iemgui_all_colfromload(t_iemgui *iemgui, int *bflcol)
{
    if(bflcol[0] < 0)
    {
        bflcol[0] = -1 - bflcol[0];
        iemgui->x_bcol = ((bflcol[0] & 0x3f000) << 6)|((bflcol[0] & 0xfc0) << 4)|
            ((bflcol[0] & 0x3f) << 2);
    }
    else
    {
        bflcol[0] = iemgui_modulo_color(bflcol[0]);
        iemgui->x_bcol = iemgui_color_hex[bflcol[0]];
    }
    if(bflcol[1] < 0)
    {
        bflcol[1] = -1 - bflcol[1];
        iemgui->x_fcol = ((bflcol[1] & 0x3f000) << 6)|((bflcol[1] & 0xfc0) << 4)|
            ((bflcol[1] & 0x3f) << 2);
    }
    else
    {
        bflcol[1] = iemgui_modulo_color(bflcol[1]);
        iemgui->x_fcol = iemgui_color_hex[bflcol[1]];
    }
    if(bflcol[2] < 0)
    {
        bflcol[2] = -1 - bflcol[2];
        iemgui->x_lcol = ((bflcol[2] & 0x3f000) << 6)|((bflcol[2] & 0xfc0) << 4)|
            ((bflcol[2] & 0x3f) << 2);
    }
    else
    {
        bflcol[2] = iemgui_modulo_color(bflcol[2]);
        iemgui->x_lcol = iemgui_color_hex[bflcol[2]];
    }
}

int iemgui_compatible_col(int i)
{
    int j;

    if(i >= 0)
    {
        j = iemgui_modulo_color(i);
        return(iemgui_color_hex[(j)]);
    }
    else
        return((-1 -i)&0xffffff);
}

void iemgui_all_dollar2raute(t_symbol **srlsym)
{
    srlsym[0] = iemgui_dollar2raute(srlsym[0]);
    srlsym[1] = iemgui_dollar2raute(srlsym[1]);
    srlsym[2] = iemgui_dollar2raute(srlsym[2]);
}

void iemgui_all_raute2dollar(t_symbol **srlsym)
{
    srlsym[0] = iemgui_raute2dollar(srlsym[0]);
    srlsym[1] = iemgui_raute2dollar(srlsym[1]);
    srlsym[2] = iemgui_raute2dollar(srlsym[2]);
}

void iemgui_send(void *x, t_iemgui *iemgui, t_symbol *s)
{
    t_symbol *snd;
    int pargc, tail_len, nth_arg, sndable=1, oldsndrcvable=0;
    t_atom *pargv;

    if(iemgui->x_fsf.x_rcv_able)
        oldsndrcvable += IEM_GUI_OLD_RCV_FLAG;
    if(iemgui->x_fsf.x_snd_able)
        oldsndrcvable += IEM_GUI_OLD_SND_FLAG;

    if(!strcmp(s->s_name, "empty")) sndable = 0;
    snd = iemgui_raute2dollar(s);
    iemgui->x_snd_unexpanded = snd;
    iemgui->x_snd = snd = canvas_realizedollar(iemgui->x_glist, snd);
    iemgui->x_fsf.x_snd_able = sndable;
    iemgui_verify_snd_ne_rcv(iemgui);
    (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_IO + oldsndrcvable);
}

void iemgui_receive(void *x, t_iemgui *iemgui, t_symbol *s)
{
    t_symbol *rcv;
    int pargc, tail_len, nth_arg, rcvable=1, oldsndrcvable=0;
    t_atom *pargv;

    if(iemgui->x_fsf.x_rcv_able)
        oldsndrcvable += IEM_GUI_OLD_RCV_FLAG;
    if(iemgui->x_fsf.x_snd_able)
        oldsndrcvable += IEM_GUI_OLD_SND_FLAG;

    if(!strcmp(s->s_name, "empty")) rcvable = 0;
    rcv = iemgui_raute2dollar(s);
    iemgui->x_rcv_unexpanded = rcv;
    rcv = canvas_realizedollar(iemgui->x_glist, rcv);
    if(rcvable)
    {
        if(strcmp(rcv->s_name, iemgui->x_rcv->s_name))
        {
            if(iemgui->x_fsf.x_rcv_able)
                pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
            iemgui->x_rcv = rcv;
            pd_bind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
        }
    }
    else if(!rcvable && iemgui->x_fsf.x_rcv_able)
    {
        pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
        iemgui->x_rcv = rcv;
    }
    iemgui->x_fsf.x_rcv_able = rcvable;
    iemgui_verify_snd_ne_rcv(iemgui);
    (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_IO + oldsndrcvable);
}

void iemgui_label(void *x, t_iemgui *iemgui, t_symbol *s)
{
    t_symbol *lab;
    int pargc, tail_len, nth_arg;
    t_atom *pargv;

        /* tb: fix for empty label { */
        if (s == gensym(""))
                s = gensym("empty");
        /* tb } */

    lab = iemgui_raute2dollar(s);
    iemgui->x_lab_unexpanded = lab;
    iemgui->x_lab = lab = canvas_realizedollar(iemgui->x_glist, lab);

    if(glist_isvisible(iemgui->x_glist))
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -text {%s} \n",
                 glist_getcanvas(iemgui->x_glist), x,
                 strcmp(s->s_name, "empty")?iemgui->x_lab->s_name:"");
}

void iemgui_label_pos(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    iemgui->x_ldx = (int)atom_getintarg(0, ac, av);
    iemgui->x_ldy = (int)atom_getintarg(1, ac, av);
    if(glist_isvisible(iemgui->x_glist))
        sys_vgui(".x%lx.c coords %lxLABEL %d %d\n",
                 glist_getcanvas(iemgui->x_glist), x,
                 text_xpix((t_object *)x,iemgui->x_glist)+iemgui->x_ldx,
                 text_ypix((t_object *)x,iemgui->x_glist)+iemgui->x_ldy);
}

void iemgui_label_font(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    int f = (int)atom_getintarg(0, ac, av);

    if(f == 1) strcpy(iemgui->x_font, "helvetica");
    else if(f == 2) strcpy(iemgui->x_font, "times");
    else
    {
        f = 0;
        strcpy(iemgui->x_font, sys_font);
    }
    iemgui->x_fsf.x_font_style = f;
    f = (int)atom_getintarg(1, ac, av);
    if(f < 4)
        f = 4;
    iemgui->x_fontsize = f;
    if(glist_isvisible(iemgui->x_glist))
        sys_vgui(".x%lx.c itemconfigure %lxLABEL -font {{%s} %d %s}\n",
                 glist_getcanvas(iemgui->x_glist), x, iemgui->x_font, 
				 iemgui->x_fontsize, sys_fontweight);
}

void iemgui_size(void *x, t_iemgui *iemgui)
{
    if(glist_isvisible(iemgui->x_glist))
    {
        (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_MOVE);
        canvas_fixlinesfor(glist_getcanvas(iemgui->x_glist), (t_text*)x);
    }
}

void iemgui_delta(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    iemgui->x_obj.te_xpix += (int)atom_getintarg(0, ac, av);
    iemgui->x_obj.te_ypix += (int)atom_getintarg(1, ac, av);
    if(glist_isvisible(iemgui->x_glist))
    {
        (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_MOVE);
        canvas_fixlinesfor(glist_getcanvas(iemgui->x_glist), (t_text*)x);
    }
}

void iemgui_pos(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    iemgui->x_obj.te_xpix = (int)atom_getintarg(0, ac, av);
    iemgui->x_obj.te_ypix = (int)atom_getintarg(1, ac, av);
    if(glist_isvisible(iemgui->x_glist))
    {
        (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_MOVE);
        canvas_fixlinesfor(glist_getcanvas(iemgui->x_glist), (t_text*)x);
    }
}

void iemgui_color(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    iemgui->x_bcol = iemgui_compatible_col(atom_getintarg(0, ac, av));
    if(ac > 2)
    {
        iemgui->x_fcol = iemgui_compatible_col(atom_getintarg(1, ac, av));
        iemgui->x_lcol = iemgui_compatible_col(atom_getintarg(2, ac, av));
    }
    else
        iemgui->x_lcol = iemgui_compatible_col(atom_getintarg(1, ac, av));
    if(glist_isvisible(iemgui->x_glist))
        (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_CONFIG);
}

void iemgui_displace(t_gobj *z, t_glist *glist, int dx, int dy)
{
    t_iemguidummy *x = (t_iemguidummy *)z;

    x->x_gui.x_obj.te_xpix += dx;
    x->x_gui.x_obj.te_ypix += dy;
    (*x->x_gui.x_draw)((void *)z, glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(glist_getcanvas(glist), (t_text *)z);
}

void iemgui_displace_withtag(t_gobj *z, t_glist *glist, int dx, int dy)
{
    t_iemgui *x = (t_iemgui *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    //x->x_gui.x_draw((void *)z, glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(glist_getcanvas(glist), (t_text *)z);
}

void iemgui_select(t_gobj *z, t_glist *glist, int selected)
{
    t_iemguidummy *x = (t_iemguidummy *)z;

    x->x_gui.x_fsf.x_selected = selected;
    (*x->x_gui.x_draw)((void *)z, glist, IEM_GUI_DRAW_MODE_SELECT);
}

void iemgui_delete(t_gobj *z, t_glist *glist)
{
    canvas_deletelinesfor(glist, (t_text *)z);
}

void iemgui_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_iemguidummy *x = (t_iemguidummy *)z;

    if (vis)
        (*x->x_gui.x_draw)((void *)z, glist, IEM_GUI_DRAW_MODE_NEW);
    else
    {
        (*x->x_gui.x_draw)((void *)z, glist, IEM_GUI_DRAW_MODE_ERASE);
        sys_unqueuegui(z);
    }
}

void iemgui_save(t_iemgui *iemgui, t_symbol **srl, int *bflcol)
{
    srl[0] = iemgui->x_snd;
    srl[1] = iemgui->x_rcv;
    srl[2] = iemgui->x_lab;
    iemgui_all_sym2dollararg(iemgui, srl);
    iemgui_all_col2save(iemgui, bflcol);
}

void iemgui_properties(t_iemgui *iemgui, t_symbol **srl)
{
    srl[0] = iemgui->x_snd;
    srl[1] = iemgui->x_rcv;
    srl[2] = iemgui->x_lab;
    iemgui_all_sym2dollararg(iemgui, srl);
    iemgui_all_dollar2raute(srl);
}

int iemgui_dialog(t_iemgui *iemgui, t_symbol **srl, int argc, t_atom *argv)
{
    char str[144];
    int init = (int)atom_getintarg(5, argc, argv);
    int ldx = (int)atom_getintarg(10, argc, argv);
    int ldy = (int)atom_getintarg(11, argc, argv);
    int f = (int)atom_getintarg(12, argc, argv);
    int fs = (int)atom_getintarg(13, argc, argv);
    int bcol = (int)atom_getintarg(14, argc, argv);
    int fcol = (int)atom_getintarg(15, argc, argv);
    int lcol = (int)atom_getintarg(16, argc, argv);
    int sndable=1, rcvable=1, oldsndrcvable=0;

    if(iemgui->x_fsf.x_rcv_able)
        oldsndrcvable += IEM_GUI_OLD_RCV_FLAG;
    if(iemgui->x_fsf.x_snd_able)
        oldsndrcvable += IEM_GUI_OLD_SND_FLAG;
    if(IS_A_SYMBOL(argv,7))
        srl[0] = atom_getsymbolarg(7, argc, argv);
    else if(IS_A_FLOAT(argv,7))
    {
        sprintf(str, "%d", (int)atom_getintarg(7, argc, argv));
        srl[0] = gensym(str);
    }
    if(IS_A_SYMBOL(argv,8))
        srl[1] = atom_getsymbolarg(8, argc, argv);
    else if(IS_A_FLOAT(argv,8))
    {
        sprintf(str, "%d", (int)atom_getintarg(8, argc, argv));
        srl[1] = gensym(str);
    }
    if(IS_A_SYMBOL(argv,9))
        srl[2] = atom_getsymbolarg(9, argc, argv);
    else if(IS_A_FLOAT(argv,9))
    {
        sprintf(str, "%d", (int)atom_getintarg(9, argc, argv));
        srl[2] = gensym(str);
    }
    if(init != 0) init = 1;
    iemgui->x_isa.x_loadinit = init;
    if(!strcmp(srl[0]->s_name, "empty")) sndable = 0;
    if(!strcmp(srl[1]->s_name, "empty")) rcvable = 0;
    iemgui_all_raute2dollar(srl);
    iemgui_all_dollararg2sym(iemgui, srl);
    if(rcvable)
    {
        if(strcmp(srl[1]->s_name, iemgui->x_rcv->s_name))
        {
            if(iemgui->x_fsf.x_rcv_able)
                pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
            iemgui->x_rcv = srl[1];
            pd_bind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
        }
    }
    else if(!rcvable && iemgui->x_fsf.x_rcv_able)
    {
        pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
        iemgui->x_rcv = srl[1];
    }
    iemgui->x_snd = srl[0];
    iemgui->x_fsf.x_snd_able = sndable;
    iemgui->x_fsf.x_rcv_able = rcvable;
    iemgui->x_lcol = lcol & 0xffffff;
    iemgui->x_fcol = fcol & 0xffffff;
    iemgui->x_bcol = bcol & 0xffffff;
    iemgui->x_lab = srl[2];
    iemgui->x_ldx = ldx;
    iemgui->x_ldy = ldy;
    if(f == 1) strcpy(iemgui->x_font, "helvetica");
    else if(f == 2) strcpy(iemgui->x_font, "times");
    else
    {
        f = 0;
        strcpy(iemgui->x_font, sys_font);
    }
    iemgui->x_fsf.x_font_style = f;
    if(fs < 4)
        fs = 4;
    iemgui->x_fontsize = fs;
    iemgui_verify_snd_ne_rcv(iemgui);
    canvas_dirty(iemgui->x_glist, 1);
    return(oldsndrcvable);
}

void iem_inttosymargs(t_iem_init_symargs *symargp, int n)
{
    memset(symargp, 0, sizeof(*symargp));
    symargp->x_loadinit = (n >>  0);
    symargp->x_scale = (n >>  20);
    symargp->x_flashed = 0;
    symargp->x_locked = 0;
    symargp->x_reverse = 0;
    symargp->dummy = 0;
}

int iem_symargstoint(t_iem_init_symargs *symargp)
{
    return (
        (((symargp->x_loadinit & 1) <<  0) |
        ((symargp->x_scale & 1) <<  20)));
}

void iem_inttofstyle(t_iem_fstyle_flags *fstylep, int n)
{
    memset(fstylep, 0, sizeof(*fstylep));
    fstylep->x_font_style = (n >> 0);
    fstylep->x_shiftdown = 0;
    fstylep->x_selected = 0;
    fstylep->x_finemoved = 0;
    fstylep->x_put_in2out = 0;
    fstylep->x_change = 0;
    fstylep->x_thick = 0;
    fstylep->x_lin0_log1 = 0;
    fstylep->x_steady = 0;
    fstylep->dummy = 0;
}

int iem_fstyletoint(t_iem_fstyle_flags *fstylep)
{
    return ((fstylep->x_font_style << 0) & 63);
}
