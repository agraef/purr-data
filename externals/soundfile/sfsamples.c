
#include <gmerlin/avdec.h>
#include <gavl/gavl.h>

#include <m_pd.h>

#define DEBUG(x) x
//#define DEBUG(x)

static t_class *sfsamples_class;
static t_canvas *canvas;

typedef struct _sfsamples
{
    t_object x_obj;
    t_symbol* filename;
    bgav_t* decoder;
} t_sfsamples;

static void sfsamples_bang(t_sfsamples *x)
{
    DEBUG(post("sfsamples_bang"));
	const gavl_audio_format_t* open_audio_format;
	gavl_time_t gavl_time;

    if(x->filename == &s_) {
        pd_error(x,"no file set");
        return;
    }
	if(!bgav_open(x->decoder, x->filename->s_name)) {
		pd_error(x, "Could not open file %s", x->filename->s_name);
    }
	// only concerned with the first audio stream
	open_audio_format = bgav_get_audio_format(x->decoder, 0);    

    // we can get audio formats that are unkown
	if (open_audio_format->sample_format == GAVL_SAMPLE_NONE) {
 		pd_error(x, "sorry, this file has unsupported audio."); 
	}

    gavl_time = bgav_get_duration(x->decoder, 0);
    outlet_float(x->x_obj.ob_outlet, (float)(gavl_time_to_samples(open_audio_format->samplerate, gavl_time) / (float)open_audio_format->samplerate));

}

static void sfsamples_symbol(t_sfsamples *x, t_symbol* filename)
{
    DEBUG(post("sfsamples_symbol"));
    x->filename = filename;
    sfsamples_bang(x);
}

static void *sfsamples_new(t_symbol *s, int argc, t_atom *argv)
{
    t_sfsamples *x = (t_sfsamples *)pd_new(sfsamples_class);
    // TODO convert args into a filename, so spaces are valid
    x->filename == &s_;

    outlet_new(&x->x_obj, &s_float);

    x->decoder = bgav_create(); 

    return(x);
}

static void sfsamples_free(t_sfsamples *x)
{
    bgav_close(x->decoder);
}

void sfsamples_setup(void)
{
    sfsamples_class = class_new(gensym("sfsamples"), 
                                (t_newmethod)sfsamples_new, 
                                (t_method)sfsamples_free, 
                                sizeof(t_sfsamples), 
                                0, A_GIMME, 0);

    class_addbang(sfsamples_class, (t_method)sfsamples_bang);
    class_addsymbol(sfsamples_class, (t_method)sfsamples_symbol);
}
