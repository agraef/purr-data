/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* changes by Thomas Musil IEM KUG Graz Austria 2001 */
/* have to insert gui-objects into editor-list */
/* all changes are labeled with      iemlib      */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#include "g_canvas.h"
#include "s_utf8.h"

#define LMARGIN 2
#define RMARGIN 2
#define TMARGIN 3
/* for some reason, it draws text 1 pixel lower on Mac OS X */
#ifdef __APPLE__
#define BMARGIN 1
#else
#define BMARGIN 1
#endif

#define SEND_FIRST 1
#define SEND_UPDATE 2
#define SEND_CHECK 0

// selection for shift+arrow selecting
// 0 = none;
// 1 = start;
// 2 = end;
static int last_sel = 0;

struct _rtext
{
    char *x_buf;    /*-- raw byte string, assumed UTF-8 encoded (moo) --*/
    int x_bufsize;  /*-- byte length --*/
    int x_selstart; /*-- byte offset --*/
    int x_selend;   /*-- byte offset --*/
    int x_active;      /* 1 if actively editing */
    int x_dragfrom;    /* character onset we're dragging from */
    int x_drawnwidth;  /* screen size in pixels */
    int x_drawnheight;
    t_text *x_text;    /* owner */
    t_glist *x_glist;  /* glist owner belongs to */
    char x_tag[50];    /* tag for gui */
    struct _rtext *x_next; /* next in editor list */
};

t_rtext *rtext_new(t_glist *glist, t_text *who)
{
    t_rtext *x = (t_rtext *)getbytes(sizeof *x);
    x->x_text = who;
    x->x_glist = glist;
    x->x_next = glist->gl_editor->e_rtext;
    x->x_selstart = x->x_selend = x->x_active =
        x->x_drawnwidth = x->x_drawnheight = 0;
    binbuf_gettext(who->te_binbuf, &x->x_buf, &x->x_bufsize);
    glist->gl_editor->e_rtext = x;
    // here we use a more complex tag which will later help us properly
    // select objects inside a gop on its parent that are otherwise not
    // supposed to be there (they don't belong to that canvas). See
    // in pd.tk pdtk_select_all_gop_widgets function and how it affects
    // draw data structures that are displayed via gop (Ico 20140831)
    sprintf(x->x_tag, ".x%zx.t%zx", (t_uint)glist_getcanvas(x->x_glist),
        (t_int)x);
    return (x);
}

static t_rtext *rtext_entered;

void rtext_free(t_rtext *x)
{
    t_editor *e = x->x_glist->gl_editor;
    if (e->e_textedfor == x)
        e->e_textedfor = 0;
    if (e->e_rtext == x)
        e->e_rtext = x->x_next;
    else
    {
        t_rtext *e2;
        for (e2 = e->e_rtext; e2; e2 = e2->x_next)
            if (e2->x_next == x)
        {
            e2->x_next = x->x_next;
            break;
        }
    }
    if (rtext_entered == x) rtext_entered = 0;
    freebytes(x->x_buf, x->x_bufsize);
    freebytes(x, sizeof *x);
}

char *rtext_gettag(t_rtext *x)
{
    return (x->x_tag);
}

void rtext_gettext(t_rtext *x, char **buf, int *bufsize)
{
    *buf = x->x_buf;
    *bufsize = x->x_bufsize;
}

void rtext_settext(t_rtext *x, char *buf, int bufsize)
{
    if (x->x_bufsize) freebytes(x->x_buf, x->x_bufsize);
    x->x_buf = buf;
    x->x_bufsize = bufsize;
}

void rtext_getseltext(t_rtext *x, char **buf, int *bufsize)
{
    *buf = x->x_buf + x->x_selstart;
    *bufsize = x->x_selend - x->x_selstart;
}

t_text *rtext_getowner(t_rtext *x)
{
    return (x->x_text);
}

/* convert t_text te_type symbol for use as a Tk tag */
static t_symbol *rtext_gettype(t_rtext *x)
{
    switch (x->x_text->te_type) 
    {
    case T_TEXT: return gensym("text");
    case T_OBJECT: return gensym("obj");
    case T_MESSAGE: return gensym("msg");
    case T_ATOM: return gensym("atom");
    }
    return (&s_);
}

/* LATER deal with tcl-significant characters */

/* firstone(), lastone()
 *  + returns byte offset of (first|last) occurrence of 'c' in 's[0..n-1]', or
 *    -1 if none was found
 *  + 's' is a raw byte string
 *  + 'c' is a byte value
 *  + 'n' is the length (in bytes) of the prefix of 's' to be searched.
 *  + we could make these functions work on logical characters in utf8 strings,
 *    but we don't really need to...
 */
static int firstone(char *s, int c, int n)
{
    char *s2 = s + n;
    int i = 0;
    while (s != s2)
    {
        //fprintf(stderr,"s=<%s> n=%d s=%d c=%d s2=%d\n", s, n, *s, c, *s2);
        if (*s == c)
        {
            //fprintf(stderr,"DONE\n");
            return (i);
        }
        i++;
        s++;
    }
    //fprintf(stderr,"FAILED\n");
    return (-1);
}

static int lastone(char *s, int c, int n)
{
    char *s2 = s + n;
    while (s2 != s)
    {
        s2--;
        n--;
        if (*s2 == c) return (n);
    }
    return (-1);
}

    /* break the text into lines, and compute byte index of character at
    location (width, height).  Then reset (width, height) to report size of
    resulting line-broken text.  Used for object, message, and comment boxes;
    another version below is for atoms.  Also we report the onsets of
    the beginning and end of the selection, as byte onsets into the reformatted
    text, which we'll use to inform the GUI how to show the selection.

    The input is taken from x->buf and x->bufsize fields of the text object;
    the wrapped text is put in "tempbuf" with byte length outchars_b_p.

    x->x_buf is assumed to contain text in UTF-8 format, in which characters
    may occupy multiple bytes. variables with a "_b" suffix are raw byte
    strings, lengths, or offsets;  those with a "_c" suffix are logical
    character lengths or offsets.
    The UTF8 handling was contributed by Bryan Jurish, who says "moo." */

#define DEFAULTBOXWIDTH 60

/* Older (pre-8.3.4) TCL versions handle text selection differently; this
flag is set from the GUI if this happens.  LATER take this out: early 2006? */

extern int sys_oldtclversion;           
extern int is_dropdown(t_text *x);

static void rtext_formattext(t_rtext *x, int *widthp, int *heightp,
    int *indexp,  char *tempbuf, int *outchars_b_p, int *selstart_b_p,
    int *selend_b_p, int fontwidth, int fontheight)
{
    int widthspec_c = x->x_text->te_width; // width if any specified

    int widthlimit_c = (widthspec_c ? widthspec_c : DEFAULTBOXWIDTH);


    int inindex_b = 0; // index location in the buffer
    int inindex_c = 0; // index location in the u8 chars



        // buffer size in u8 chars
        //fprintf(stderr,"buf = <%s> | last 2 chars = %d %d\n", x->x_buf, x->x_buf[x->x_bufsize-1], x->x_buf[x->x_bufsize]);
        int x_bufsize_c = u8_charnum(x->x_buf, x->x_bufsize);

    int nlines = 0, ncolumns = 0, reportedindex = 0;
    int findx = (*widthp + (fontwidth/2)) / fontwidth;
    int findy = *heightp / fontheight;

    *selstart_b_p = *selend_b_p = 0;



    while (x_bufsize_c - inindex_c > 0)
    {
        int inchars_b  = x->x_bufsize - inindex_b;
        int inchars_c  = x_bufsize_c  - inindex_c;
        int maxindex_c =
            (inchars_c > widthlimit_c ? widthlimit_c : inchars_c);
        int maxindex_b = u8_offset(x->x_buf + inindex_b, maxindex_c,
            x->x_bufsize - inindex_b);
        int eatchar = 1;
        //fprintf(stderr, "firstone <%s> inindex_b=%d maxindex_b=%d\n", x->x_buf + inindex_b, inindex_b, maxindex_b);
        int foundit_b  = firstone(x->x_buf + inindex_b, '\n', maxindex_b);
        int foundit_c;
        //following deals with \v replacement for \n in multiline comments
        int foundit_bv  = firstone(x->x_buf + inindex_b, '\v', maxindex_b);
        if ((foundit_bv < foundit_b && foundit_bv != -1) ||
            (foundit_b == -1 && foundit_bv != -1))
            foundit_b = foundit_bv;
        if (foundit_b < 0) //if we did not find an \n
        { 
            /* too much text to fit in one line? */
            if (inchars_c > widthlimit_c)
            {
                /* is there a space to break the line at?  OK if it's even
                one byte past the end since in this context we know there's
                more text */
                foundit_b =
                    lastone(x->x_buf + inindex_b, ' ', maxindex_b + 1);
                if (foundit_b < 0)
                {
                    foundit_b = maxindex_b;
                    foundit_c = maxindex_c;
                    eatchar = 0;
                }
                else
                    foundit_c = u8_charnum(x->x_buf + inindex_b, foundit_b);
            }
            else
            {
                foundit_b = inchars_b;
                foundit_c = inchars_c;
                eatchar = 0;
            }
        }
        else
            foundit_c = u8_charnum(x->x_buf + inindex_b, foundit_b);

        if (nlines == findy)
        {
            int actualx = (findx < 0 ? 0 :
                (findx > foundit_c ? foundit_c : findx));
            *indexp = inindex_b + u8_offset(x->x_buf + inindex_b, actualx,
                x->x_bufsize - inindex_b);
            reportedindex = 1;
        }
        strncpy(tempbuf + *outchars_b_p, x->x_buf + inindex_b, foundit_b);
        if (x->x_selstart >= inindex_b &&
            x->x_selstart <= inindex_b + foundit_b + eatchar)
                *selstart_b_p = x->x_selstart + *outchars_b_p - inindex_b;
        if (x->x_selend >= inindex_b &&
            x->x_selend <= inindex_b + foundit_b + eatchar)
                *selend_b_p = x->x_selend + *outchars_b_p - inindex_b;
        *outchars_b_p += foundit_b;

        inindex_b += (foundit_b + eatchar);
        inindex_c += (foundit_c + eatchar);
        if (inindex_b < x->x_bufsize)
            tempbuf[(*outchars_b_p)++] = '\n';
        // if we found a row that is longer than previous (total width)
        if (foundit_c > ncolumns)
            ncolumns = foundit_c;
        nlines++;
    }

     if (!reportedindex)
        *indexp = *outchars_b_p;
     if (nlines < 1) nlines = 1;
     if (!widthspec_c)
     {
         while (ncolumns < (x->x_text->te_type == T_TEXT ? 1 : 3))
         {
             tempbuf[(*outchars_b_p)++] = ' ';
             ncolumns++;
         }
     }
     else ncolumns = widthspec_c;
    *widthp = ncolumns * fontwidth;
    *heightp = nlines * fontheight;
//     if (glist_getzoom(x->x_glist) > 1)
     if (0)
     {
         /* zoom margins */
//        *widthp += (LMARGIN + RMARGIN) * glist_getzoom(x->x_glist);
//        *heightp += (TMARGIN + BMARGIN) * glist_getzoom(x->x_glist);
    }
    else
    {
        *widthp += LMARGIN + RMARGIN;
        *heightp += TMARGIN + BMARGIN;
    }

}

    /* same as above, but for atom boxes, which are always on one line. */
static void rtext_formatatom(t_rtext *x, int *widthp, int *heightp,
    int *indexp,  char *tempbuf, int *outchars_b_p, int *selstart_b_p,
    int *selend_b_p, int fontwidth, int fontheight)
{
    int findx = *widthp / fontwidth;  /* character index; want byte index */
    *indexp = 0;
        /* special case: for number boxes, try to pare the number down
        to the specified width of the box. */
    if (x->x_text->te_width > 0 && binbuf_getnatom(x->x_text->te_binbuf) == 1 &&
        binbuf_getvec(x->x_text->te_binbuf)->a_type == A_FLOAT &&
        x->x_bufsize > x->x_text->te_width)
    {
            /* try to reduce size by dropping decimal digits */
        int wantreduce = x->x_bufsize - x->x_text->te_width;
        char *decimal = 0, *nextchar, *ebuf = x->x_buf + x->x_bufsize,
            *s1, *s2;
        int ndecimals;
        strncpy(tempbuf, x->x_buf, x->x_bufsize);
        tempbuf[x->x_bufsize] = 0;
        ebuf = tempbuf + x->x_bufsize;
        for (decimal = tempbuf; decimal < ebuf; decimal++)
            if (*decimal == '.')
                break;
        if (decimal >= ebuf)
            goto giveup;
        for (nextchar = decimal + 1; nextchar < ebuf; nextchar++)
            if (*nextchar < '0' || *nextchar > '9')
                break;
        if (nextchar - decimal - 1 < wantreduce)
           goto giveup;
        for (s1 = nextchar - wantreduce, s2 = s1 + wantreduce;
            s2 < ebuf; s1++, s2++)
                *s1 = *s2;
        *outchars_b_p = x->x_text->te_width;
        goto done;
    giveup:
            /* give up and bash last char to '>' */
        tempbuf[x->x_text->te_width-1] = '>';
        tempbuf[x->x_text->te_width] = 0;
        *outchars_b_p = x->x_text->te_width;
    done: ;
        *indexp = findx;
        *widthp = x->x_text->te_width * fontwidth;
    }
    else
    {
        int outchars_c = 0, prev_b = 0;
        int widthlimit_c = (x->x_text->te_width > 0 ? x->x_text->te_width :
                1000);   /* nice big fat limit since we can't wrap */
        uint32_t thischar;
        *outchars_b_p = 0;
        for (outchars_c = 0;
            *outchars_b_p < x->x_bufsize && outchars_c < widthlimit_c;
                outchars_c++)
        {

            prev_b = *outchars_b_p;
            thischar = u8_nextchar(x->x_buf, outchars_b_p);
            if (findx > outchars_c)
                *indexp = *outchars_b_p;
            if (thischar == '\n' || !thischar)
            {
                *(outchars_b_p) = prev_b;
                break;
            }
            memcpy(tempbuf + prev_b, x->x_buf + prev_b, *outchars_b_p - prev_b);
                /* if box is full and there's more, bash last char to '>' */
            if (outchars_c == widthlimit_c-1 && x->x_bufsize > *(outchars_b_p)
                 && (x->x_buf[*(outchars_b_p)] != ' ' ||
                    x->x_bufsize > *(outchars_b_p)+1))
            {
                tempbuf[prev_b] = '>';
            }
        }
        if (x->x_text->te_width > 0)
            *widthp = x->x_text->te_width * fontwidth;
        else *widthp = (outchars_c > 3 ? outchars_c : 3) * fontwidth;
        tempbuf[*outchars_b_p] = 0;
    }
    if (*indexp > *outchars_b_p)
        *indexp = *outchars_b_p;
    if (*indexp < 0)
        *indexp = 0;
    *selstart_b_p = x->x_selstart;
    *selend_b_p = x->x_selend;
// These need to be checked... can't remember if we're using these
// defines in Purr GUI
    *widthp += (LMARGIN + RMARGIN - 2);
    *heightp = fontheight + (TMARGIN + BMARGIN - 1);
}

    /* the following routine computes line breaks and carries out
    some action which could be:
        SEND_FIRST - draw the box  for the first time
        SEND_UPDATE - redraw the updated box
        otherwise - don't draw, just calculate.
    Called with *widthp and *heightp as coordinates of
    a test point, the routine reports the index of the character found
    there in *indexp.  *widthp and *heightp are set to the width and height
    of the entire text in pixels.
    */

    /* LATER get this and sys_vgui to work together properly,
        breaking up messages as needed.  As of now, there's
        a limit of 1950 characters, imposed by sys_vgui(). */
#define UPBUFSIZE 4000

void text_getfont(t_text *x, t_glist *thisglist,
    int *fheightp, int *fwidthp, int *guifsize);

static void rtext_senditup(t_rtext *x, int action, int *widthp, int *heightp,
    int *indexp)
{
    char smallbuf[200], *tempbuf;
    int outchars_b = 0, guifontsize, fontwidth, fontheight;
    t_canvas *canvas = glist_getcanvas(x->x_glist);
    char smallescbuf[400], *escbuf = 0;
    size_t escchars = 0;
    int selstart_b, selend_b;   /* beginning and end of selection in bytes */
        /* if we're a GOP (the new, "goprect" style) borrow the font size
        from the inside to preserve the spacing */

    text_getfont(x->x_text, x->x_glist, &fontwidth, &fontheight, &guifontsize);
    if (x->x_bufsize >= 100)
         tempbuf = (char *)t_getbytes(2 * x->x_bufsize + 1);
    else tempbuf = smallbuf;
    tempbuf[0] = 0;

    if (x->x_text->te_type == T_ATOM)
        rtext_formatatom(x, widthp, heightp, indexp,
            tempbuf, &outchars_b, &selstart_b,  &selend_b,
            fontwidth, fontheight);
    else rtext_formattext(x, widthp, heightp, indexp,
            tempbuf, &outchars_b, &selstart_b, &selend_b,
            fontwidth, fontheight);
    tempbuf[outchars_b]=0;

    if (action && x->x_text->te_width && x->x_text->te_type != T_ATOM)
    {
        /* if our width is specified but the "natural" width is the
           same as the specified width, set specified width to zero
           so future text editing will automatically change width.
           Except atoms whose content changes at runtime. */
        int widthwas = x->x_text->te_width, newwidth = 0, newheight = 0,
            newindex = 0;
        x->x_text->te_width = 0;
        rtext_senditup(x, 0, &newwidth, &newheight, &newindex);
            if (newwidth != *widthp)
                x->x_text->te_width = widthwas;
    }

    if (action && !canvas->gl_havewindow)
        action = 0;

//    escbuf = (tempbuf == smallbuf)?smallescbuf:t_getbytes(2 * outchars_b + 1);

// need to change this to whatever we're using in Purr...
//    pdgui_strnescape(escbuf, 2 * outchars_b + 1, tempbuf, outchars_b);
    escbuf = tempbuf;

    if (action == SEND_FIRST)
    {
        //fprintf(stderr,"send_first rtext=%zx t_text=%zx\n", x, x->x_text);
        gui_vmess("gui_text_new", "xssiiisi",
            canvas, x->x_tag, rtext_gettype(x)->s_name,
            glist_isselected(x->x_glist, ((t_gobj*)x->x_text)),
            LMARGIN,
            fontheight,
            escbuf,
            guifontsize);
           
    }
    else if (action == SEND_UPDATE)
    {
        gui_vmess("gui_text_set", "xss", canvas, x->x_tag, escbuf);

        // We add the check for T_MESSAGE below so that the box border
        // gets resized correctly using our interim event handling in
        // pd_canvas.html.  I could remove the conditional, but
        // this part of Pd is convoluted enough that I'm not sure
        // if there'd be any side effects.
        // Additionally we avoid redrawing the border here for the
        // dropdown_class as that has its own special width handling.
        if (glist_isvisible(x->x_glist) && !is_dropdown(x->x_text) &&
            (*widthp != x->x_drawnwidth ||
            *heightp != x->x_drawnheight ||
            x->x_text->te_type == T_MESSAGE)) 
        {
            text_drawborder(x->x_text, x->x_glist, x->x_tag,
                *widthp, *heightp, 0);
        }
        if (x->x_active)
        {
            if (selend_b > selstart_b)
            {
                //sys_vgui(".x%zx.c select from %s %d\n", canvas, 
                //    x->x_tag, u8_charnum(tempbuf, selstart_b));
                //sys_vgui(".x%zx.c select to %s %d\n", canvas, 
                //    x->x_tag, u8_charnum(tempbuf, selend_b)
                //      + (sys_oldtclversion ? 0 : -1));
                //sys_vgui(".x%zx.c focus \"\"\n", canvas);        
            }
            else
            {
                //sys_vgui(".x%zx.c select clear\n", canvas);
                //sys_vgui(".x%zx.c icursor %s %d\n", canvas, x->x_tag,
                //    u8_charnum(tempbuf, selstart_b));
                //sys_vgui(".x%zx.c focus %s\n", canvas, x->x_tag);        
            }
        }
    }
    x->x_drawnwidth = *widthp;
    x->x_drawnheight = *heightp;
        
    if (tempbuf != smallbuf)
        t_freebytes(tempbuf, 2 * x->x_bufsize + 1);
//    if (escbuf != smallescbuf)
//        t_freebytes(escbuf, 2 * outchars_b + 1);
}

    /* remake text buffer from binbuf */
void rtext_retext(t_rtext *x)
{
    int w = 0, h = 0, indx;
    t_text *text = x->x_text;
    t_freebytes(x->x_buf, x->x_bufsize);
    binbuf_gettext(text->te_binbuf, &x->x_buf, &x->x_bufsize);
    rtext_senditup(x, SEND_UPDATE, &w, &h, &indx);
}

/* find the rtext that goes with a text item */
t_rtext *glist_findrtext(t_glist *gl, t_text *who)
{
    t_rtext *x=NULL;
    if (!gl->gl_editor)
        canvas_create_editor(gl);
    if (gl->gl_editor->e_rtext)
        for (x = gl->gl_editor->e_rtext; x && x->x_text != who; x = x->x_next)
            ;
    if (!x) bug("glist_findrtext");
    return (x);
}

/* same as above but without error reporting */
t_rtext *glist_tryfindrtext(t_glist *gl, t_text *who)
{
    t_rtext *x=NULL;
    if (!gl->gl_editor)
        canvas_create_editor(gl);
    if (gl->gl_editor->e_rtext)
        for (x = gl->gl_editor->e_rtext; x && x->x_text != who; x = x->x_next)
            ;
    return (x);
}

int rtext_width(t_rtext *x)
{
    int w = 0, h = 0, indx;
    rtext_senditup(x, SEND_CHECK, &w, &h, &indx);
    return (w);
}

int rtext_height(t_rtext *x)
{
    int w = 0, h = 0, indx;
    rtext_senditup(x, SEND_CHECK, &w, &h, &indx);
    return (h);
}

void rtext_draw(t_rtext *x)
{
    int w = 0, h = 0, indx;
    rtext_senditup(x, SEND_FIRST, &w, &h, &indx);
}

/* Not needed since the rtext gets erased along with the parent gobj group */
void rtext_erase(t_rtext *x)
{
    //if (x && x->x_glist)
    //    sys_vgui(".x%zx.c delete %s\n", glist_getcanvas(x->x_glist), x->x_tag);
}

/* Not needed since the rtext gets displaced along with the parent gobj group */
void rtext_displace(t_rtext *x, int dx, int dy)
{
    //sys_vgui(".x%zx.c move %s %d %d\n", glist_getcanvas(x->x_glist), 
    //    x->x_tag, dx, dy);
}

/* This is no longer used-- we do this with CSS now. But keeping the code
   here until we test a bit more. */
void rtext_select(t_rtext *x, int state)
{
    //t_glist *glist = x->x_glist;
    //t_canvas *canvas = glist_getcanvas(glist);
    //if (glist_istoplevel(glist))
    //    sys_vgui(".x%zx.c itemconfigure %s -fill %s\n", canvas, 
    //        x->x_tag, (state? "$pd_colors(selection)" : "$pd_colors(text)"));
    //if (x->x_text->te_pd->c_wb && x->x_text->te_pd->c_wb->w_displacefnwtag)
    //{
    //    if (state)
    //        sys_vgui(".x%zx.c addtag selected withtag %s\n",
    //               glist_getcanvas(glist), x->x_tag);
    //    else
    //        sys_vgui(".x%zx.c dtag %s selected\n",
    //               glist_getcanvas(glist), x->x_tag);
    //}
    /* Not sure the following is needed anymore either-- commenting it
       out to test what (if any) side-effects there are */
    //canvas_editing = canvas;
}

void rtext_activate(t_rtext *x, int state)
{
    //fprintf(stderr,"rtext_activate state=%d\n", state);
    int w = 0, h = 0, widthspec, heightspec, indx, isgop,
        selstart = -1, selend = -1;
    char *tmpbuf;
    t_glist *glist = x->x_glist;
    t_canvas *canvas = glist_getcanvas(glist);
    if (state && x->x_active) {
        //fprintf(stderr, "duplicate rtext_activate\n");
        return;
    }
    // the following prevents from selecting all when inside an
    // object that is already being texted for... please *test*
    // "fixes" before committing them
    //if (state == x->x_active) return; // avoid excess calls
    if (state)
    {
        //sys_vgui(".x%zx.c focus %s\n", canvas, x->x_tag);
        glist->gl_editor->e_textedfor = x;
        glist->gl_editor->e_textdirty = 0;
        x->x_selstart = 0;
        x->x_selend = x->x_bufsize;
        x->x_active = 1;
    }
    else
    {
        //sys_vgui("selection clear .x%zx.c\n", canvas);
        //sys_vgui(".x%zx.c focus \"\"\n", canvas);
        if (glist->gl_editor->e_textedfor == x)
            glist->gl_editor->e_textedfor = 0;
        x->x_active = 0;
    }

    /* check if it has a window */
    if(!glist->gl_havewindow) return;

    rtext_senditup(x, SEND_UPDATE, &w, &h, &indx);
    /* hack...
       state = 0 no editing
       state = 1 editing
       state = 2 editing a new object
       State 2 isn't necessary, except that Pd has
       traditionally had this "floating" state for
       new objects where the box text is editable and
       the box position follows the mouse
    */

    /* If we're a gop canvas... */
    if (pd_class((t_pd*)x->x_text) == canvas_class &&
        ((t_canvas *)x->x_text)->gl_isgraph)
    {
        widthspec = ((t_canvas *)x->x_text)->gl_pixwidth;
        heightspec = ((t_canvas *)x->x_text)->gl_pixheight;
        isgop = 1;
    }
    else
    {
        int xmin, xmax, tmp;
        gobj_getrect(&x->x_text->te_g, x->x_glist, &xmin, &tmp, &xmax, &tmp);
            /* width if specified. If not, we send the bounding width as
               a negative number */
        widthspec = (x->x_text->te_width ? x->x_text->te_width : -(xmax-xmin));
            /* signal with negative number that we don't have a heightspec */
        heightspec = -1; // signal that we don't have a heightspec
        isgop = 0;
    }

    if(state & (0b1 << 31)) /* arbitray selection */
    {
        selstart = (state >> 16) & 0x7FFF;
        selend = state & 0xFFFF;
        state = 1; // set to editing state
    }

    /* we need to get scroll to make sure we've got the
       correct bbox for the svg */
    canvas_getscroll(glist_getcanvas(canvas));
    /* ugly hack to get around the fact that x_buf is not
       null terminated. If this becomes a problem we can revisit
       it later */
    tmpbuf = t_getbytes(x->x_bufsize + 1);
    sprintf(tmpbuf, "%.*s", (int)x->x_bufsize, x->x_buf);
    /* in case x_bufsize is 0... */
    tmpbuf[x->x_bufsize] = '\0';
    gui_vmess("gui_textarea", "xssiiiisiiiiiii",
        canvas,
        x->x_tag,
        (pd_class((t_pd *)x->x_text) == message_class ? "msg" : "obj"),
        x->x_text->te_xpix,
        x->x_text->te_ypix,
        widthspec,
        heightspec,
        tmpbuf,
        sys_hostfontsize(glist_getfont(glist)),
        sys_fontwidth(glist_getfont(glist)),
        sys_fontheight(glist_getfont(glist)),
        isgop,
        state,
        selstart,
        selend
        );
    freebytes(tmpbuf, x->x_bufsize + 1);
}

    /* figure out which atom a click falls into if any; -1 if you
    clicked on a space or something */
int rtext_findatomfor(t_rtext *x, int xpos, int ypos)
{
    int w = xpos, h = ypos, indx, natom = 0, i, gotone = 0;
        /* get byte index of character clicked on */
    rtext_senditup(x, SEND_UPDATE, &w, &h, &indx);
        /* search through for whitespace before that index */
    for (i = 0; i <= indx; i++)
    {
        if (x->x_buf[i] == ';' || x->x_buf[i] == ',')
            natom++, gotone = 0;
        else if (x->x_buf[i] == ' ' || x->x_buf[i] == '\n')
            gotone = 0;
        else
        {
            if (!gotone)
                natom++;
            gotone = 1;
        }
    }
    return (natom-1);
}


// outputs 1 if found one of the special chars
// this function is used with traversal through rtext below
// using ctrl+left/right and similar shortcuts
static int rtext_compare_special_chars(const char c)
{
        if (c != '\n' && c != '\v' && c != ' ')
            return 0;
        return 1;
}

void gatom_key(void *z, t_floatarg f);

void rtext_key(t_rtext *x, int keynum, t_symbol *keysym)
{
    //fprintf(stderr,"rtext_key %d %s\n", keynum, keysym->s_name);
    int w = 0, h = 0, indx, i, newsize, ndel;

        /* CR to atom boxes sends message and resets */
    if (keynum == '\n' && x->x_text->te_type == T_ATOM)
    {
        gatom_key(x->x_text, keynum);
        return;
    }

    if (keynum)
    {
        int n = keynum;
        if (n == '\r' || n == '\v') n = '\n';
        if (n == '\b') /* backspace */
        {
            if (x->x_selstart && (x->x_selstart == x->x_selend))
            {
                u8_dec(x->x_buf, &x->x_selstart);
                if (glist_isvisible(glist_getcanvas(x->x_glist)))
                    canvas_getscroll(glist_getcanvas(x->x_glist));
            }
            
        }
        else if (n == 127) /* delete */
        {
            if (x->x_selend < x->x_bufsize && (x->x_selstart == x->x_selend))
                u8_inc(x->x_buf, &x->x_selend);
            if (glist_isvisible(glist_getcanvas(x->x_glist)))
                canvas_getscroll(glist_getcanvas(x->x_glist));
        }
        
        ndel = x->x_selend - x->x_selstart;
        if (ndel)
        {
            for (i = x->x_selend; i < x->x_bufsize; i++)
                x->x_buf[i- ndel] = x->x_buf[i];
            newsize = x->x_bufsize - ndel;
            x->x_buf = resizebytes(x->x_buf, x->x_bufsize, newsize);
            x->x_bufsize = newsize;
        }

/* at Guenter's suggestion, use 'n>31' to test wither a character might
be printable in whatever 8-bit character set we find ourselves. */

/*-- moo:
  ... but test with "<" rather than "!=" in order to accomodate unicode
  codepoints for n (which we get since Tk is sending the "%A" substitution
  for bind <Key>), effectively reducing the coverage of this clause to 7
  bits.  Case n>127 is covered by the next clause.
*/
        if (n == '\n' || (n > 31 && n < 127))
        {
            newsize = x->x_bufsize+1;
            x->x_buf = resizebytes(x->x_buf, x->x_bufsize, newsize);
            for (i = x->x_bufsize; i > x->x_selstart; i--)
                x->x_buf[i] = x->x_buf[i-1];
            x->x_buf[x->x_selstart] = n;
            x->x_bufsize = newsize;
            x->x_selstart = x->x_selstart + 1;
            if (glist_isvisible(glist_getcanvas(x->x_glist)))
                canvas_getscroll(glist_getcanvas(x->x_glist));
        }
        /*--moo: check for unicode codepoints beyond 7-bit ASCII --*/
        else if (n > 127)
        {
            int ch_nbytes = u8_wc_nbytes(n);
            newsize = x->x_bufsize + ch_nbytes;
            x->x_buf = resizebytes(x->x_buf, x->x_bufsize, newsize);
            //fprintf(stderr,"x->x_bufsize=%d newsize=%d\n", x->x_bufsize, newsize);
            //for (i = newsize-1; i >= x->x_selstart; i--)
            //{
                //fprintf(stderr,"%d-%d <%d>\n", i, i-ch_nbytes, x->x_buf[i-ch_nbytes]);
                //x->x_buf[i] = '\0';
            //}
            x->x_bufsize = newsize;
            /*-- moo: assume canvas_key() has encoded keysym as UTF-8 */
            strncpy(x->x_buf+x->x_selstart, keysym->s_name, ch_nbytes);
            x->x_selstart = x->x_selstart + ch_nbytes;
        }
        x->x_selend = x->x_selstart;
        x->x_glist->gl_editor->e_textdirty = 1;
    }
    else if (!strcmp(keysym->s_name, "Right"))
    {
        if (x->x_selend == x->x_selstart && x->x_selstart < x->x_bufsize)
        {
            u8_inc(x->x_buf, &x->x_selstart);
            x->x_selend = x->x_selstart;
        }
        else
            x->x_selstart = x->x_selend;
        last_sel = 0;        
    }
    else if (!strcmp(keysym->s_name, "Left"))
    {
        if (x->x_selend == x->x_selstart && x->x_selstart > 0)
        {
            u8_dec(x->x_buf, &x->x_selstart);
            x->x_selend = x->x_selstart;
        }
        else
            x->x_selend = x->x_selstart;
        last_sel = 0;
    }
    else if (!strcmp(keysym->s_name, "ShiftRight"))
    {
        if (!last_sel) last_sel = 2;
        if (last_sel == 1 && x->x_selstart < x->x_selend)
        {
            if (x->x_selstart < x->x_bufsize)
                u8_inc(x->x_buf, &x->x_selstart);        
        }
        else
        {
            last_sel = 2;
            if (x->x_selend < x->x_bufsize)
                u8_inc(x->x_buf, &x->x_selend);
        }
    }
    else if (!strcmp(keysym->s_name, "ShiftLeft"))
    {
        if (!last_sel) last_sel = 1;
        if (last_sel == 2 && x->x_selend > x->x_selstart)
        {
            x->x_selend = x->x_selend - 1;
        }
        else
        {
            last_sel = 1;
            if (x->x_selstart > 0)
                u8_dec(x->x_buf, &x->x_selstart);
        }
    }
    else if (!strcmp(keysym->s_name, "Up"))
    {
        if (x->x_selstart != x->x_selend)
        {
            x->x_selend = x->x_selstart;
            last_sel = 0;
        }
        else
        {
            // we do this twice and then move to the right
            // as many spots as we had before, this will
            // allow us to go visually above where we used
            // to be in multiline situations (e.g. comments)
            int right = 0;
            //printf("start: selstart=%d x->x_bufsize=%d\n", x->x_selstart, x->x_bufsize);
            if (x->x_selstart > 0 &&
                   (x->x_selstart == x->x_bufsize ||
                    x->x_buf[x->x_selstart] == '\n' ||
                    x->x_buf[x->x_selstart] == '\v'))
            {
                //printf("found break\n");
                u8_dec(x->x_buf, &x->x_selstart);
                right++;
            }
            while (x->x_selstart > 0 &&
                    (x->x_buf[x->x_selstart-1] != '\n' &&
                        x->x_buf[x->x_selstart-1] != '\v'))
            {
                u8_dec(x->x_buf, &x->x_selstart);
                right++;
            }
            if (x->x_selstart == 0)
                right = 0;
            //printf("first linebreak: right=%d selstart=%d\n", right, x->x_selstart);
            if (x->x_selstart > 0)
                u8_dec(x->x_buf, &x->x_selstart);
            //printf("decrease by 1: selstart=%d\n", x->x_selstart);
            while (x->x_selstart > 0 &&
                (x->x_buf[x->x_selstart-1] != '\n' &&
                    x->x_buf[x->x_selstart-1] != '\v'))
                u8_dec(x->x_buf, &x->x_selstart);
            //printf("second linebreak: selstart=%d\n", x->x_selstart);
            if (x->x_selstart < x->x_bufsize && right > 0)
            {
                u8_inc(x->x_buf, &x->x_selstart);
                right--;
            }
            //printf("increase by 1: selstart=%d\n", x->x_selstart);
            while (right > 0 && 
                (x->x_buf[x->x_selstart] != '\n' &&
                    x->x_buf[x->x_selstart] != '\v'))
            {
                u8_inc(x->x_buf, &x->x_selstart);
                right--;
            }
            //printf("final: selstart=%d\n", x->x_selstart);
            x->x_selend = x->x_selstart;
            last_sel = 0;
        }
    }
    else if (!strcmp(keysym->s_name, "Down"))
    {
        if (x->x_selstart != x->x_selend)
        {
            x->x_selstart = x->x_selend;
            last_sel = 0;
        }
        else
        {
            // we do this twice and then move to the right
            // as many spots as we had before, this will
            // allow us to go visually below where we used
            // to be in multiline situations (e.g. comments)
            int right = 0;
            if (x->x_selstart > 0 &&
                    (x->x_buf[x->x_selstart] != '\n' ||
                        x->x_buf[x->x_selstart] != '\v'))
            {
                while (x->x_selstart > 0 &&
                        (x->x_buf[x->x_selstart-1] != '\n' &&
                            x->x_buf[x->x_selstart-1] != '\v'))
                {
                    x->x_selstart--;
                    right++;
                }
            }
            //printf("start: right=%d selstart=%d selend=%d\n", right, x->x_selstart, x->x_selend);
            if (x->x_selend < x->x_bufsize &&
                    (x->x_buf[x->x_selend] == '\n' ||
                        x->x_buf[x->x_selend] == '\v'))
            {
                //printf("found break\n");
                u8_inc(x->x_buf, &x->x_selend);
                right--;
            }
            else while (x->x_selend < x->x_bufsize &&
                (x->x_buf[x->x_selend] != '\n' && x->x_buf[x->x_selend] != '\v'))
            {
                u8_inc(x->x_buf, &x->x_selend);
            }
            //printf("first linebreak: selend=%d\n", x->x_selend);
            if (x->x_selend+1 < x->x_bufsize)
            {
                u8_inc(x->x_buf, &x->x_selend);
            }
            //printf("increase by 1: selend=%d\n", x->x_selend);
            while (right > 0 && x->x_selend < x->x_bufsize &&
                (x->x_buf[x->x_selend] != '\n' &&
                    x->x_buf[x->x_selend] != '\v'))
            {
                u8_inc(x->x_buf, &x->x_selend);
                right--;
            }
            //printf("final: selend=%d\n", x->x_selend);
            x->x_selstart = x->x_selend;
            last_sel = 0;
        }
    }
    else if (!strcmp(keysym->s_name, "Home"))
    {
        if (x->x_selstart)
            u8_dec(x->x_buf, &x->x_selstart);
        while (x->x_selstart > 0 && x->x_buf[x->x_selstart] != '\n')
            u8_dec(x->x_buf, &x->x_selstart);
        x->x_selend = x->x_selstart;
        last_sel = 0;
    }
    else if (!strcmp(keysym->s_name, "ShiftHome"))
    {
        if (x->x_selstart)
        {
            if (last_sel == 2)
                x->x_selend = x->x_selstart;
            u8_dec(x->x_buf, &x->x_selstart);
            last_sel = 1;
        }
        while (x->x_selstart > 0 && x->x_buf[x->x_selstart] != '\n')
            u8_dec(x->x_buf, &x->x_selstart);
        //x->x_selend = x->x_selstart;
        //last_sel = 1;
    }
    else if (!strcmp(keysym->s_name, "End"))
    {
        while (x->x_selend < x->x_bufsize &&
            x->x_buf[x->x_selend] != '\n')
            u8_inc(x->x_buf, &x->x_selend);
        if (x->x_selend < x->x_bufsize)
            u8_inc(x->x_buf, &x->x_selend);
        x->x_selstart = x->x_selend;
        last_sel = 0;
    }
    else if (!strcmp(keysym->s_name, "ShiftEnd"))
    {
        if (last_sel == 1)
            x->x_selstart = x->x_selend;
        while (x->x_selend < x->x_bufsize &&
            x->x_buf[x->x_selend] != '\n')
            u8_inc(x->x_buf, &x->x_selend);
        if (x->x_selend < x->x_bufsize)
        {
            u8_inc(x->x_buf, &x->x_selend);
        }
        //x->x_selstart = x->x_selend;
        last_sel = 2;
    }
    else if (!strcmp(keysym->s_name, "CtrlLeft"))
    {
        /* first find first non-space char going back */
        while (x->x_selstart > 0 &&
            rtext_compare_special_chars(x->x_buf[x->x_selstart-1]))
        {
            u8_dec(x->x_buf, &x->x_selstart);
        }
        /* now go back until you find another space or
           the beginning of the buffer */
        while (x->x_selstart > 0 &&
          !rtext_compare_special_chars(x->x_buf[x->x_selstart-1]))
        {
            u8_dec(x->x_buf, &x->x_selstart);
        }
        if (x->x_buf[x->x_selstart+1] == ' ' &&
            x->x_buf[x->x_selstart] == ' ')
        {
            u8_inc(x->x_buf, &x->x_selstart);
        }
        x->x_selend = x->x_selstart;
    }
    else if (!strcmp(keysym->s_name, "CtrlRight"))
    {
        /* now go forward until you find another space
           or the end of the buffer */
        if (x->x_selend < x->x_bufsize - 1)
            u8_inc(x->x_buf, &x->x_selend);
        while (x->x_selend < x->x_bufsize &&
          !rtext_compare_special_chars(x->x_buf[x->x_selend]))
            u8_inc(x->x_buf, &x->x_selend);
        x->x_selstart = x->x_selend;
    }
    else if (!strcmp(keysym->s_name, "CtrlShiftLeft"))
    {
        int swap = 0;
        int *target;
        if (!last_sel) last_sel = 1;
        if (last_sel == 2 && x->x_selend > x->x_selstart)
            target = &x->x_selend;
        else
        {
            last_sel = 1;
            target = &x->x_selstart;
        }
        /* first find first non-space char going back */
        while (*target > 0 &&
            rtext_compare_special_chars(x->x_buf[*target-1]))
        {
            u8_dec(x->x_buf, target);
        }
        /* now go back until you find another space or
           the beginning of the buffer */
        while (*target > 0 &&
            !rtext_compare_special_chars(x->x_buf[*target-1]))
        {
            u8_dec(x->x_buf, target);
        }
        if (x->x_buf[*target+1] == ' ' &&
            x->x_buf[x->x_selstart] == ' ')
        {
            u8_inc(x->x_buf, target);
        }
        if (x->x_selstart > x->x_selend)
        {
            swap = x->x_selend;
            x->x_selend = x->x_selstart;
            x->x_selstart = swap;
            last_sel = 1;
        }
    }
    else if (!strcmp(keysym->s_name, "CtrlShiftRight"))
    {
        int swap = 0;
        int *target;
        if (!last_sel) last_sel = 2;
        if (last_sel == 1 && x->x_selstart < x->x_selend)
            target = &x->x_selstart;
        else
        {
            last_sel = 2;
            target = &x->x_selend;
        }
        /* now go forward until you find another space or
           the end of the buffer */
        if (*target < x->x_bufsize - 1)
        {
            u8_inc(x->x_buf, target);
        }
        while (*target < x->x_bufsize &&
            !rtext_compare_special_chars(x->x_buf[*target]))
        {
            u8_inc(x->x_buf, target);
        }
        if (x->x_selstart > x->x_selend)
        {
            swap = x->x_selend;
            x->x_selend = x->x_selstart;
            x->x_selstart = swap;
            last_sel = 2;
        }
    }
    rtext_senditup(x, SEND_UPDATE, &w, &h, &indx);
}

void rtext_mouse(t_rtext *x, int xval, int yval, int flag)
{
    int w = xval, h = yval, indx;
    rtext_senditup(x, SEND_CHECK, &w, &h, &indx);
    if (flag == RTEXT_DOWN)
    {
        x->x_dragfrom = x->x_selstart = x->x_selend = indx;
    }
    else if (flag == RTEXT_DBL)
    {
        int whereseparator, newseparator;
        x->x_dragfrom = -1;
        whereseparator = 0;
        if ((newseparator = lastone(x->x_buf, ' ', indx)) > whereseparator)
            whereseparator = newseparator+1;
        if ((newseparator = lastone(x->x_buf, '\n', indx)) > whereseparator)
            whereseparator = newseparator+1;
        if ((newseparator = lastone(x->x_buf, ';', indx)) > whereseparator)
            whereseparator = newseparator+1;
        if ((newseparator = lastone(x->x_buf, ',', indx)) > whereseparator)
            whereseparator = newseparator+1;
        x->x_selstart = whereseparator;

        whereseparator = x->x_bufsize - indx;
        if ((newseparator =
            firstone(x->x_buf+indx, ' ', x->x_bufsize - indx)) >= 0 &&
                newseparator < whereseparator)
                    whereseparator = newseparator;
        if ((newseparator =
            firstone(x->x_buf+indx, '\n', x->x_bufsize - indx)) >= 0 &&
                newseparator < whereseparator)
                    whereseparator = newseparator;
        if ((newseparator =
            firstone(x->x_buf+indx, ';', x->x_bufsize - indx)) >= 0 &&
                newseparator < whereseparator)
                    whereseparator = newseparator;
        if ((newseparator =
            firstone(x->x_buf+indx, ',', x->x_bufsize - indx)) >= 0 &&
                newseparator < whereseparator)
                    whereseparator = newseparator;
        x->x_selend = indx + whereseparator;
    }
    else if (flag == RTEXT_SHIFT)
    {
        if (indx * 2 > x->x_selstart + x->x_selend)
            x->x_dragfrom = x->x_selstart, x->x_selend = indx;
        else
            x->x_dragfrom = x->x_selend, x->x_selstart = indx;
    }
    else if (flag == RTEXT_DRAG)
    {
        if (x->x_dragfrom < 0)
            return;
        x->x_selstart = (x->x_dragfrom < indx ? x->x_dragfrom : indx);
        x->x_selend = (x->x_dragfrom > indx ? x->x_dragfrom : indx);
    }
    rtext_senditup(x, SEND_UPDATE, &w, &h, &indx);
}
