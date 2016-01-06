/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution. */
/* g_7_guis.h written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */

#define IEM_GUI_DEFAULTSIZE 15
#define IEM_GUI_MINSIZE 8
#define IEM_SL_DEFAULTSIZE 128
#define IEM_SL_MINSIZE 2
#define IEM_FONT_MINSIZE 4

#define IEM_GUI_DRAW_MODE_UPDATE 0
#define IEM_GUI_DRAW_MODE_MOVE   1
#define IEM_GUI_DRAW_MODE_NEW    2
#define IEM_GUI_DRAW_MODE_SELECT 3
#define IEM_GUI_DRAW_MODE_CONFIG 5

#define IS_A_FLOAT(atom,index) ((atom+index)->a_type == A_FLOAT)
#define IS_A_SYMBOL(atom,index) ((atom+index)->a_type == A_SYMBOL)

#define IEM_GUI_OLD_SND_FLAG 1
#define IEM_GUI_OLD_RCV_FLAG 2

#define IEMGUI_MAX_NUM_LEN 32

#define SCALE_NUM_MINWIDTH 1
#define SCALE_NUM_MINHEIGHT 8
#define SCALE_GOP_MINWIDTH 12
#define SCALE_GOP_MINHEIGHT 12
#define SCALEHANDLE_WIDTH   5
#define SCALEHANDLE_HEIGHT  5
#define LABELHANDLE_WIDTH   5
#define LABELHANDLE_HEIGHT  5

typedef void (*t_iemfunptr)(void *x, t_glist *glist, int mode);

struct _scalehandle;
typedef void (*t_clickhandlefn)(struct _scalehandle *sh, int newstate);
typedef void (*t_motionhandlefn)(struct _scalehandle *sh, t_floatarg f1, t_floatarg f2);

EXTERN t_class *scalehandle_class;

typedef struct _scalehandle
{
    t_pd       h_pd;
    t_object  *h_master;
    t_glist   *h_glist; // this is the canvas to draw on. Note that when objects are edited, "glist" and "canvas" mean the same.
    t_symbol  *h_bindsym;
    int        h_scale; // bool
    char       h_pathname[37]; // max size for ".x%lx.h%lx" = 5+4*sizeof(long)
    char       h_outlinetag[18]; // max size for "h%lx" = 2+2*sizeof(long)
    int        h_dragon; // bool
    int        h_dragx;
    int        h_dragy;
    int        h_offset_x;
    int        h_offset_y;
    int        h_vis; // bool
    t_clickhandlefn h_clickfn;
    t_motionhandlefn h_motionfn;
} t_scalehandle;

typedef struct _iemgui
{
    t_object       x_obj;
    t_glist       *x_glist;
    t_iemfunptr    x_draw;           //29    /* this should be static */
    int            x_h; //unsigned   //80
    int            x_w; //unsigned   //119
    int            x_ldx;            //33
    int            x_ldy;            //33
    int            x_fontsize;//uns. //41
    int            x_fcol;//iemcolor //35  /* foreground */
    int            x_bcol;//iemcolor //41  /* background */
    int            x_lcol;//iemcolor //21  /* label */
    t_symbol      *x_snd;            //18  /* send symbol */
    t_symbol      *x_rcv;            //33  /* receive */
    t_symbol      *x_lab;            //15  /* label */
    t_symbol      *x_snd_unexpanded; //7  /* same 3, with '$' unexpanded */
    t_symbol      *x_rcv_unexpanded; //7
    t_symbol      *x_lab_unexpanded; //6
    int            x_binbufindex;    //4   /* where in binbuf to find these (this should be static) */
    int            x_labelbindex;    //5   /* where in binbuf to find label (this should be static) */
    t_scalehandle *x_handle;         //24
    t_scalehandle *x_lhandle;        //19
    int            x_vis;     //bool //64  /* is the object drawn? */
    int            x_changed; //bool //30  /* has the value changed so that we need to do graphic update */

                                  // grep -w "$1" *.[ch]|wc -l
    t_glist     *x_selected;      // 24 matches
    // from t_iem_fstyle_flags
    unsigned int x_font_style:6;  // 33 matches
    unsigned int x_finemoved:1;   //  7 matches (sliders and [nbx] only)
    unsigned int x_put_in2out:1;  //  9 matches
    unsigned int x_change:1;      // 28 matches  // what's this and why is there also a x_changed ?
    // from t_iem_init_symargs
    unsigned int x_loadinit:1;    // 21 matches
    unsigned int x_locked:1;      //  7 matches ([bng] only)
    unsigned int x_reverse:1;     //  4 matches (sliders only)
    unsigned int dummy:20;
    int legacy_x;                 // legacy x offset
    int legacy_y;                 // legacy y offset
    //int click_x;                  // click getrect x offset
    //int click_y;                  // click getrect y offset
} t_iemgui;

typedef struct _bng
{
    t_iemgui x_gui;
    int      x_flashed; // bool
    int      x_flashtime_break; // >= 0
    int      x_flashtime_hold; // >= 0
    t_clock  *x_clock_hld;
    t_clock  *x_clock_brk;
    t_clock  *x_clock_lck;
} t_bng;

typedef struct _slider
{
    t_iemgui x_gui;
    int      x_pos;
    int      x_val;
    int      x_center; // is this necessary ?
    int      x_thick; // bool
    int      x_lin0_log1; // bool
    int      x_steady; // bool
    double   x_min;
    double   x_max;
    double   x_k;
    double   x_last;
    int      x_is_last_float; // bool
    int      x_orient; // bool: 0=horizontal ([hsl]), 1=vertical ([vsl])
} t_slider;

typedef struct _radio
{
    t_iemgui x_gui;
    int      x_on; // unsigned < x_number
    int      x_on_old; /* for use by [hdl] [vdl] */
    int      x_change; // bool
    int      x_number; // unsigned > 0
    int      x_drawn; // unsigned < x_number
    t_atom   x_at[2]; // tmp buffer for outlet_list
    int      x_orient; // bool: 0=horiz, 1=vert
} t_radio;

typedef struct _toggle
{
    t_iemgui x_gui;
    t_float  x_on;
    t_float  x_nonzero;
} t_toggle;

typedef struct _my_canvas
{
    t_iemgui x_gui;
    t_atom   x_at[2]; // tmp buffer for outlet_list
    int      x_vis_w; // unsigned
    int      x_vis_h; // unsigned
} t_my_canvas;

typedef struct _vu
{
    t_iemgui x_gui;
    int      x_led_size; // unsigned
    int      x_peak; // 1..40 if visible ; 0 if invisible
    int      x_rms;  // 1..40 if visible ; 0 if invisible
    t_float  x_fp; // value for outlet 1
    t_float  x_fr; // value for outlet 0
    int      x_scale; // bool
    t_outlet *x_out_rms; // outlet 0
    t_outlet *x_out_peak; // outlet 1
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
    int      x_lin0_log1; // bool
    char     x_buf[IEMGUI_MAX_NUM_LEN];
    int      x_numwidth; // unsigned (width in pixels)
    int      x_scalewidth;  /* temporary value for scalehandle */
    int      x_scaleheight; /* temporary value for scalehandle */
    int      x_tmpfontsize; /* temporary value for scalehandle */
    int      x_log_height;
    int      x_hide_frame;  /* 0 default, 1 just arrow, 2, just frame, 3 both */
} t_my_numbox;

extern int sys_noloadbang;
extern int iemgui_color_hex[];

EXTERN int iemgui_clip_size(int size);
EXTERN int iemgui_clip_font(int size);
EXTERN void iemgui_verify_snd_ne_rcv(t_iemgui *iemgui);
EXTERN t_symbol *iemgui_getfloatsym(t_atom *a);
EXTERN t_symbol *iemgui_getfloatsymarg(int i, int argc, t_atom *argv);
EXTERN void iemgui_new_getnames(t_iemgui *iemgui, int indx, t_atom *argv);
EXTERN void iemgui_all_colfromload(t_iemgui *iemgui, int *bflcol);
EXTERN void iemgui_send(t_iemgui *x, t_symbol *s);
EXTERN void iemgui_receive(t_iemgui *x, t_symbol *s);
EXTERN void iemgui_label(t_iemgui *x, t_symbol *s);
EXTERN void iemgui_label_pos(t_iemgui *x, t_symbol *s, int ac, t_atom *av);
EXTERN void iemgui_label_font(t_iemgui *x, t_symbol *s, int ac, t_atom *av);
EXTERN void iemgui_label_getrect(t_iemgui x_gui, t_glist *x, int *xp1, int *yp1, int *xp2, int *yp2);
EXTERN void iemgui_shouldvis(t_iemgui *x, int mode);
EXTERN void iemgui_size(t_iemgui *x);
EXTERN void iemgui_delta(t_iemgui *x, t_symbol *s, int ac, t_atom *av);
EXTERN void iemgui_pos(t_iemgui *x, t_symbol *s, int ac, t_atom *av);
EXTERN void iemgui_color(t_iemgui *x, t_symbol *s, int ac, t_atom *av);
EXTERN void iemgui_displace(t_gobj *z, t_glist *glist, int dx, int dy);
EXTERN void iemgui_displace_withtag(t_gobj *z, t_glist *glist, int dx, int dy);
EXTERN void iemgui_select(t_gobj *z, t_glist *glist, int selected);
EXTERN void iemgui_delete(t_gobj *z, t_glist *glist);
EXTERN void iemgui_vis(t_gobj *z, t_glist *glist, int vis);
EXTERN void iemgui_save(t_iemgui *x, t_symbol **srl, int *bflcol);
EXTERN void iemgui_properties(t_iemgui *x, t_symbol **srl);
EXTERN int iemgui_dialog(t_iemgui *x, int argc, t_atom *argv);

EXTERN void iem_inttosymargs(t_iemgui *x, int n);
EXTERN int iem_symargstoint(t_iemgui *x);
EXTERN void iem_inttofstyle(t_iemgui *x, int n);
EXTERN int iem_fstyletoint(t_iemgui *x);

EXTERN void canvas_apply_setundo(t_canvas *x, t_gobj *y);

// scalehandle code, as refactored by Mathieu
EXTERN void scalehandle_draw_select(t_scalehandle *h, int px, int py);
EXTERN void scalehandle_draw_select2(t_iemgui *x);
EXTERN void scalehandle_draw_erase(t_scalehandle *h);
EXTERN void scalehandle_draw_erase2(t_iemgui *x);
EXTERN void scalehandle_draw(t_iemgui *x);
EXTERN t_scalehandle *scalehandle_new(t_object *x, t_glist *glist, int scale, t_clickhandlefn chf, t_motionhandlefn mhf);
EXTERN void scalehandle_free(t_scalehandle *h);
EXTERN void properties_set_field_int(long props, const char *gui_field, int value);
EXTERN void scalehandle_dragon_label(t_scalehandle *h, float f1, float f2);
EXTERN void scalehandle_click_label(t_scalehandle *h);
EXTERN void scalehandle_click_scale(t_scalehandle *h);
EXTERN void scalehandle_unclick_scale(t_scalehandle *h);
EXTERN void scalehandle_drag_scale(t_scalehandle *h);
EXTERN void iemgui__clickhook3(t_scalehandle *sh, int newstate);

EXTERN int mini(int a, int b);
EXTERN int maxi(int a, int b);
EXTERN float minf(float a, float b);
EXTERN float maxf(float a, float b);

// other refactor by Mathieu
EXTERN void iemgui_tag_selected(     t_iemgui *x);
EXTERN void iemgui_label_draw_new(   t_iemgui *x);
EXTERN void iemgui_label_draw_move(  t_iemgui *x);
EXTERN void iemgui_label_draw_config(t_iemgui *x);
EXTERN void iemgui_label_draw_select(t_iemgui *x);
EXTERN void iemgui_io_draw_move(t_iemgui *x);
EXTERN void iemgui_draw_io(t_iemgui *x, int old_sr_flags);
EXTERN void iemgui_base_draw_new(t_iemgui *x);
EXTERN void iemgui_base_draw_move(t_iemgui *x);
EXTERN void iemgui_base_draw_config(t_iemgui *x);
EXTERN void iemgui_draw_new(t_iemgui *x);
EXTERN void iemgui_draw_config(t_iemgui *x);
EXTERN void iemgui_draw_move(t_iemgui *x);
EXTERN void iemgui_draw_erase(t_iemgui *x);
EXTERN void wb_init(t_widgetbehavior *wb, t_getrectfn gr, t_clickfn cl); // rename this to iemgui_wb_init

extern t_symbol *s_empty;
EXTERN const char *selection_color;
EXTERN const char *border_color;

static inline int iemgui_has_snd (t_iemgui *x) {return x->x_snd!=s_empty;}
static inline int iemgui_has_rcv (t_iemgui *x) {return x->x_rcv!=s_empty;}
EXTERN const char *iemgui_font(t_iemgui *x);
EXTERN const char *iemgui_typeface(t_iemgui *x);

EXTERN void iemgui_class_addmethods(t_class *c);
EXTERN void scrollbar_update(t_glist *glist);
EXTERN void iemgui_init(t_iemgui *x, t_floatarg f);

EXTERN void iemgui_out_bang(t_iemgui *x, int o, int chk_putin);
EXTERN void iemgui_out_float(t_iemgui *x, int o, int chk_putin, t_float f);
EXTERN void iemgui_out_list(t_iemgui *x, int o, int chk_putin, t_symbol *s, int argc, t_atom *argv);

EXTERN int sys_legacy; // found in s_main.c, used for legacy iemgui positioning 
