#include "m_pd.h"
#include <stdio.h>
#include "string.h"
#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#include "g_magicglass.h"
#include "g_canvas.h"

#define MG_CLOCK_CLEAR_DELAY 500.5
#define MG_CLOCK_FLASH_DELAY 50
#define MG_SAMPLE_COUNT 2205

EXTERN int glist_getfont(t_glist *x);

t_class *magicGlass_class;

void magicGlass_clearText(t_magicGlass *x);

void magicGlass_bind(t_magicGlass *x, t_object *obj, int outno)
{
    //fprintf(stderr,"magicglass_bind %lx\n", (t_int)x);
    if (x->x_connectedObj != obj)
    {
        if (x->x_connectedObj)
        {
            obj_disconnect(x->x_connectedObj,
                           x->x_connectedOutno,
                           &x->x_obj,
                           0);
        }
        x->x_connectedObj = obj;
        x->x_connectedOutno = outno;
        x->x_maxSize = 1;
        magicGlass_clearText(x);
        obj_connect(obj, outno, &x->x_obj, 0);
    }
}

void magicGlass_unbind(t_magicGlass *x)
{
    //fprintf(stderr,"magicglass_unbind %lx\n", (t_int)x);
    if (x->x_connectedObj)
    {
        obj_disconnect(x->x_connectedObj,
                       x->x_connectedOutno,
                       &x->x_obj,
                       0);
    }
    x->x_dspOn = 0;
    x->x_maxSample = -999999;
    x->x_sampleCount = 0;
    x->x_connectedObj = NULL;
    x->x_connectedOutno = 0;
    x->x_maxSize = 1;
}

void magicGlass_updateText(t_magicGlass *x, int moved)
{
    //fprintf(stderr,"magicglass_updateText\n");
    int bgSize;
    x->x_display_font = sys_hostfontsize(glist_getfont(x->x_c));

    if (x->x_visible)
    {
        if (!moved)
        {
            //char *color;
            if (x->x_issignal || strcmp(x->x_old_string, x->x_string))
            {
                //color = "$pd_colors(magic_glass_text)";
            }
            else
            {
                //color = "$pd_colors(magic_glass_flash)";
                gui_vmess("gui_cord_inspector_flash", "xi", x->x_c, 1);
                clock_delay(x->x_flashClock, MG_CLOCK_FLASH_DELAY);
            }
            //sys_vgui(".x%x.c itemconfigure magicGlassText -text {%s} "
            //         "-fill %s -font {{%s} -%d %s}\n",
            //        x->x_c,
            //        x->x_string,
            //        color,
            //        sys_font, x->x_display_font, sys_fontweight);
        }
        else
        {
            //sys_vgui(".x%x.c itemconfigure magicGlassText -text {%s}\n",
            //        x->x_c,
            //        x->x_string);
        }

        if (strlen(x->x_string) > 0)
        {
            if (strlen(x->x_string) > x->x_maxSize)
                x->x_maxSize = strlen(x->x_string);
        }
        bgSize = x->x_x +
            (sys_fontwidth(x->x_display_font) * x->x_maxSize) + 26;
        gui_vmess("gui_cord_inspector_update", "xsiiiiii",
            x->x_c, x->x_string,
            x->x_x, x->x_y, bgSize, 
            x->x_y - (int)(sys_fontheight(x->x_display_font)/2) - 3,
            x->x_y + (int)(sys_fontheight(x->x_display_font)/2) + 3,
            moved);
    }
}

void magicGlass_drawNew(t_magicGlass *x)
{
    gui_vmess("gui_gobj_new", "xssiii",
        x->x_c, "cord_inspector", "cord_inspector", 0, 0, 0);
    gui_vmess("gui_cord_inspector_new", "xi",
        x->x_c, x->x_display_font);
    magicGlass_updateText(x, 0);
    clock_delay(x->x_flashClock, MG_CLOCK_FLASH_DELAY);
}

void magicGlass_undraw(t_magicGlass *x)
{
    //fprintf(stderr,"magicglass_undraw\n");
    gui_vmess("gui_cord_inspector_erase", "x",
        x->x_c);
}

/* Note: this is a misnomer. This actually changes
   the text back to the original color _after_ the
   flash happened. (Unflash?) */
void magicGlass_flashText(t_magicGlass *x)
{
    //fprintf(stderr,"magicglass_flashText\n");
    gui_vmess("gui_cord_inspector_flash", "xi",
        x->x_c, 0);
}

void magicGlass_clearText(t_magicGlass *x)
{
    //fprintf(stderr,"magicglass_clearText\n");
    strcpy(x->x_old_string, x->x_string);
    x->x_string[0] = 0;
    magicGlass_updateText(x, 0);
}

void magicGlass_bang(t_magicGlass *x) 
{
    x->x_issignal = 0;
    strcpy(x->x_old_string, x->x_string);
    strcpy(x->x_string, "bang");
    magicGlass_updateText(x, 0);
    clock_delay(x->x_clearClock, MG_CLOCK_CLEAR_DELAY);
}

void magicGlass_float(t_magicGlass *x, t_float f) 
{
    x->x_issignal = 0;
    strcpy(x->x_old_string, x->x_string);
    sprintf(x->x_string, "%g", f);
    magicGlass_updateText(x, 0);
    clock_delay(x->x_clearClock, MG_CLOCK_CLEAR_DELAY);
}

void magicGlass_symbol(t_magicGlass *x, t_symbol *sym)
{
    x->x_issignal = 0;
    strcpy(x->x_old_string, x->x_string);
    sprintf(x->x_string, "symbol %s", sym->s_name);
    magicGlass_updateText(x, 0);
    clock_delay(x->x_clearClock, MG_CLOCK_CLEAR_DELAY);
}

void magicGlass_anything(t_magicGlass *x, t_symbol *sym, int argc, t_atom *argv)
{
    char aString[4096];
    char valueString[4096];
    int i;

    x->x_issignal = 0;

    strcpy(x->x_old_string, x->x_string);
    strcpy(aString, sym->s_name);
    valueString[0] = 0;
    for (i = 0; i < argc; i++)
    {
        if (argv[i].a_type == A_SYMBOL)
        {
            sprintf(valueString, " %s", argv[i].a_w.w_symbol->s_name);
            strcat(aString, valueString);
        }
        else if (argv[i].a_type == A_FLOAT)
        {
            sprintf(valueString, " %g", argv[i].a_w.w_float);
            strcat(aString, valueString);
        }
    }
    strcpy(x->x_string, aString);
    magicGlass_updateText(x, 0);
    clock_delay(x->x_clearClock, MG_CLOCK_CLEAR_DELAY);
}

void magicGlass_list(t_magicGlass *x, t_symbol *sym, int argc, t_atom *argv)
{
    char aString[4096];
    char valueString[4096];
    int i;

    x->x_issignal = 0;

    aString[0] = 0;
    valueString[0] = 0;

    strcpy(x->x_old_string, x->x_string);
    strcpy(aString, sym->s_name);
    for (i = 0; i < argc; i++)
    {
        if (argv[i].a_type == A_SYMBOL)
        {
            sprintf(valueString, " %s", argv[i].a_w.w_symbol->s_name);
            strcat(aString, valueString);
        }
        else if (argv[i].a_type == A_FLOAT)
        {
            sprintf(valueString, " %g", argv[i].a_w.w_float);
            strcat(aString, valueString);
        }
    }
    strcpy(x->x_string, aString);
    magicGlass_updateText(x, 0);
    clock_delay(x->x_clearClock, MG_CLOCK_CLEAR_DELAY);
}

void magicGlass_setCanvas(t_magicGlass *x, t_glist *c)
{
    x->x_c = c;
}

void magicGlass_show(t_magicGlass *x)
{
    //fprintf(stderr,"magicglass_show\n");
    if (!x->x_visible)
    {
        x->x_sampleCount = 0;
        x->x_minSample =  999999;
        x->x_maxSample = -999999;
        x->x_string[0] = 0;
        x->x_visible = 1;
        magicGlass_drawNew(x);
    }
}

void magicGlass_hide(t_magicGlass *x)
{
    //fprintf(stderr,"magicglass_hide\n");
    if (x->x_visible)
    {
        magicGlass_undraw(x);
        x->x_sampleCount = 0;
        x->x_minSample =  999999;
        x->x_maxSample = -999999;
        x->x_string[0] = 0;
        x->x_visible = 0;
    }
}

void magicGlass_moveText(t_magicGlass *x, int pX, int pY)
{
    //fprintf(stderr,"magicglass_moveText\n");
    x->x_x = pX;
    x->x_y = pY;
    magicGlass_updateText(x, 1);
}

int magicGlass_bound(t_magicGlass *x)
{
    //fprintf(stderr,"magicglass_bound\n");
    if (x->x_connectedObj != NULL)
        return 1;
    else
        return 0;
}

int magicGlass_isOn(t_magicGlass *x)
{
    //fprintf(stderr,"magicglass_isOn\n");
    if (x->x_viewOn)
        return 1;
    else
        return 0;
}

void magicGlass_setOn(t_magicGlass *x, int i)
{
    //fprintf(stderr,"magicglass_setOn\n");
    if (i)
    {
        x->x_viewOn = 1;
    }
    else
    {
        x->x_viewOn = 0;
    }
}

void magicGlass_setDsp(t_magicGlass *x, int i)
{
    //fprintf(stderr,"magicglass_setDsp\n");
    if (i != x->x_dspOn)
    {
        if (i)
        {
            x->x_dspOn = 1;
            x->x_sampleCount = 0;
            x->x_minSample =  999999;
            x->x_maxSample = -999999;
        }
        else
        {
            x->x_dspOn = 0;
        }
    }
}

t_int *magicGlass_perform(t_int *w)
{
    t_magicGlass *x = (t_magicGlass *)(w[1]);
    if (x->x_dspOn && x->x_connectedObj)
    {
        //fprintf(stderr,"magicglass_perform\n");
        float *in = (float *)(w[2]);
        int N = (int)(w[3]);
        int i;
        for (i = 0; i < N; i++)
        {
            if (in[i] > x->x_maxSample)
                x->x_maxSample = in[i];
            if (in[i] < x->x_minSample)
                x->x_minSample = in[i];
            x->x_sampleCount++;
            if (x->x_sampleCount >= MG_SAMPLE_COUNT)
            {
                char l[64], m[64], h[64];
                sprintf(l, "%s%#g", (x->x_minSample < 0.0f ? "" : " "),
                    x->x_minSample);
                l[6] = '\0';
                sprintf(m, "%s%#g", (in[i] < 0.0f ? "" : " "), in[i]);
                m[6] = '\0';
                sprintf(h, "%s%#g", (x->x_maxSample < 0.0f ? "" : " "),
                    x->x_maxSample);
                h[6] = '\0';
                sprintf(x->x_string, "~ %s %s %s", l, m, h);
                magicGlass_updateText(x, 0);
                //x->x_minSample =  999999;
                //x->x_maxSample = -999999;
                x->x_sampleCount = 0;
            }
        }
    }
    return (w + 4);
}

void magicGlass_dsp(t_magicGlass *x, t_signal **sp)
{
    //fprintf(stderr,"magicglass_dsp\n");
    dsp_add(magicGlass_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
    x->x_issignal = 1;
}

void *magicGlass_new(t_glist *c)
{
    //fprintf(stderr,"magicglass_new\n");
    t_magicGlass *x = (t_magicGlass *)pd_new(magicGlass_class);
    x->x_connectedObj = NULL;
    x->x_connectedOutno = 0;
    x->x_visible = 0;
    x->x_c = c;
    x->x_sigF = 0;
    x->x_dspOn = 0;
    x->x_viewOn = 0;
    x->x_maxSample = -999999;
    x->x_sampleCount = 0;
    x->x_clearClock = clock_new(x, (t_method)magicGlass_clearText);
    x->x_flashClock = clock_new(x, (t_method)magicGlass_flashText);
    x->x_maxSize = 1;
    x->x_issignal = 0;
    x->x_display_font = 9;
    return x;
}

void magicGlass_free(t_magicGlass *x)
{
    //fprintf(stderr,"magicglass_free %lx\n", (t_int)x);
    magicGlass_unbind(x);
    x->x_dspOn = 0;
    clock_free(x->x_clearClock);
    clock_free(x->x_flashClock);
}

void magicGlass_setup(void)
{
    magicGlass_class = class_new(gensym("magicGlass"),
                                 0,
                                 (t_method)magicGlass_free,
                                 sizeof(t_magicGlass),
                                 0,
                                 A_DEFFLOAT,
                                 0);
    CLASS_MAINSIGNALIN(magicGlass_class, t_magicGlass, x_sigF);
    class_addmethod(magicGlass_class,
                      (t_method)magicGlass_dsp,
                gensym("dsp"),
                0);
    class_addbang(magicGlass_class, (t_method)magicGlass_bang);
    class_addfloat(magicGlass_class, (t_method)magicGlass_float);
    class_addsymbol(magicGlass_class, (t_method)magicGlass_symbol);
    class_addanything(magicGlass_class, (t_method)magicGlass_anything);
    class_addlist(magicGlass_class, (t_method)magicGlass_list);
}
