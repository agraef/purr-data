#include "m_pd.h"
#include <stdio.h>
#include "string.h"
#include "m_pd.h"
#include "m_imp.h"
#include "s_stuff.h"
#define MG_CLOCK_CLEAR_DELAY 500.5
#define MG_CLOCK_FLASH_DELAY 50
#define MG_SAMPLE_COUNT 2205

EXTERN int glist_getfont(t_glist *x);

t_class *magicGlass_class;

typedef struct _magicGlass
{
    t_object x_obj;
    t_object *x_connectedObj;
    int x_connectedOutno;
    int x_visible;
    char x_string[4096];
    char x_old_string[4096];
    int x_x;
    int x_y;
    int x_c;
    float x_sigF;
    int x_dspOn;
    int x_viewOn;
    float x_maxSample;
    int x_sampleCount;
    t_clock *x_clearClock;
	t_clock *x_flashClock;
	unsigned int x_maxSize;
	unsigned int x_issignal;
	int x_display_font;
} t_magicGlass;

void magicGlass_clearText(t_magicGlass *x);

void magicGlass_bind(t_magicGlass *x, t_object *obj, int outno)
{
	//fprintf(stderr,"magicglass_bind\n");
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
	//fprintf(stderr,"magicglass_unbind\n");
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
	/* change second argument (10.0) to provide optimal scaling in the following entry */
	float font = (float)(sys_hostfontsize(glist_getfont((t_glist *)(x->x_c))))/10.0;
	if (font <= 1.0) {
		x->x_display_font = 9;
		font = 1.0;
	} else {
		x->x_display_font = sys_hostfontsize(glist_getfont((t_glist *)(x->x_c)));
	}

    if (x->x_visible)
    {
		if (!moved) {
			char *color;
			if (x->x_issignal || strcmp(x->x_old_string, x->x_string)) {
				color = "#ffffff";
			}
			else {
				color = "#e87216";
				clock_delay(x->x_flashClock, MG_CLOCK_FLASH_DELAY);
			}
		    sys_vgui(".x%x.c itemconfigure magicGlassText -text {%s} -fill %s\n",
		            x->x_c,
		            x->x_string,
					color);
		} else {
		    sys_vgui(".x%x.c itemconfigure magicGlassText -text {%s}\n",
		            x->x_c,
		            x->x_string);
		}

        if (strlen(x->x_string) > 0)
        {
			if (strlen(x->x_string) > x->x_maxSize) x->x_maxSize = strlen(x->x_string);
        }
		bgSize = x->x_x + (int)((30.0 * font) + ((font * 7.0) * (float)x->x_maxSize));
        sys_vgui(".x%x.c coords magicGlassText %d %d\n",
                 x->x_c,
                 x->x_x + 20,
                 x->x_y);
        sys_vgui(".x%x.c coords magicGlassLine %d %d %d %d %d %d\n",
                 x->x_c,
                 x->x_x + 3,
                 x->x_y,
                 x->x_x + 13,
                 x->x_y + 5,
                 x->x_x + 13,
                 x->x_y - 5);
        sys_vgui(".x%x.c coords magicGlassBg %d %d %d %d\n",
                 x->x_c,
                 x->x_x + 13,
                 x->x_y - (int)(12.0 * font),
                 bgSize,
                 x->x_y + (int)(12.0 * font));
    }
}

void magicGlass_drawNew(t_magicGlass *x)
{
	//fprintf(stderr,"magicglass_drawNew\n");
    sys_vgui(".x%x.c create rectangle 0 0 0 0 -outline #ffffff -fill #000000 -tags magicGlassBg\n",
             x->x_c);
    sys_vgui(".x%x.c create polygon 0 0 0 0 0 0 -fill #000000 -width 4 -tags magicGlassLine\n",
             x->x_c);
    sys_vgui(".x%x.c create text 0 0 -text {} -anchor w -fill #e87216 -font {{%s} %d %s} -tags magicGlassText\n",
             x->x_c, sys_font, x->x_display_font, sys_fontweight);
    sys_vgui(".x%x.c raise magicGlassBg\n",
             x->x_c);
    sys_vgui(".x%x.c raise magicGlassText\n",
             x->x_c);
    magicGlass_updateText(x, 0);
	clock_delay(x->x_flashClock, MG_CLOCK_FLASH_DELAY);
}

void magicGlass_undraw(t_magicGlass *x)
{
	//fprintf(stderr,"magicglass_undraw\n");
    sys_vgui(".x%x.c delete magicGlassBg\n", x->x_c);
    sys_vgui(".x%x.c delete magicGlassLine\n", x->x_c);
    sys_vgui(".x%x.c delete magicGlassText\n", x->x_c);
}

void magicGlass_flashText(t_magicGlass *x)
{
	//fprintf(stderr,"magicglass_flashText\n");
    sys_vgui(".x%x.c itemconfigure magicGlassText -fill #ffffff\n",
         x->x_c);
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

void magicGlass_setCanvas(t_magicGlass *x, int c)
{
    x->x_c = c;
}

void magicGlass_show(t_magicGlass *x)
{
	//fprintf(stderr,"magicglass_show\n");
    if (!x->x_visible) {
		x->x_sampleCount = 0;
		x->x_maxSample = -999999;
		x->x_string[0] = 0;
        x->x_visible = 1;
        magicGlass_drawNew(x);
    }
}

void magicGlass_hide(t_magicGlass *x)
{
	//fprintf(stderr,"magicglass_hide\n");
    if (x->x_visible) {
        magicGlass_undraw(x);
		x->x_sampleCount = 0;
		x->x_maxSample = -999999;
		x->x_string[0] = 0;
		x->x_visible = 0;
	}
}

void magicGlass_moveText(t_magicGlass *x, int pX, int pY)
{
	//fprintf(stderr,"magicglass_moveText\n");
    int bgSize;
    
    x->x_x = pX;
    x->x_y = pY;
    magicGlass_updateText(x, 1);
}

int magicGlass_bound(t_magicGlass *x)
{
	//fprintf(stderr,"magicglass_bound\n");
    if (x->x_connectedObj)
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
	if (i != x->x_dspOn) {
		if (i)
		{
		    x->x_dspOn = 1;
		    x->x_sampleCount = 0;
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
            x->x_sampleCount++;
            if (x->x_sampleCount >= MG_SAMPLE_COUNT)
            {
                sprintf(x->x_string, "~ %g", x->x_maxSample);
                magicGlass_updateText(x, 0);
                x->x_maxSample = -999999;
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

void *magicGlass_new(int c)
{
	//fprintf(stderr,"magicglass_new\n");
    t_magicGlass *x = (t_magicGlass *)pd_new(magicGlass_class);
    x->x_connectedObj= NULL;
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
	//fprintf(stderr,"magicglass_free\n");
    x->x_dspOn = 0;
    clock_free(x->x_clearClock);
}

void magicGlass_setup(void)
{
    magicGlass_class = class_new(gensym("magicGlass"),
                                 (t_newmethod)magicGlass_new,
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
