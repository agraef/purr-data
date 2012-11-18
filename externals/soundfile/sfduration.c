
#include <string.h>
#include <stdio.h>

#include <gmerlin/avdec.h>
#include <gavl/gavl.h>

#include <m_pd.h>

#define DEBUG(x) x
//#define DEBUG(x)

static t_class *sfduration_class;

typedef struct _sfduration
{
    t_object x_obj;
    t_symbol* cwd;
    t_symbol* filename;
    bgav_t* decoder;
} t_sfduration;

static void sfduration_bang(t_sfduration *x)
{
    DEBUG(post("sfduration_bang"));
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
    t_float time_per_sample = (t_float)(gavl_time_to_samples(
                                            open_audio_format->samplerate, 
                                            gavl_time));
    t_float samplerate = (t_float)open_audio_format->samplerate;
    outlet_float(x->x_obj.ob_outlet, time_per_sample / samplerate * 1000.0);

}

static void sfduration_symbol(t_sfduration *x, t_symbol* filename)
{
    DEBUG(post("sfduration_symbol"));
    if(sys_isabsolutepath(filename->s_name)) {
        x->filename = filename;
    } else {
        int buflen;
        char buf[FILENAME_MAX];
        strncpy(buf, x->cwd->s_name, FILENAME_MAX);
        strcat(buf, "/");
        strncat(buf, filename->s_name, FILENAME_MAX - buflen - 2);
        x->filename = gensym(buf);
    }
  
    sfduration_bang(x);
}

static void *sfduration_new(t_symbol *s, int argc, t_atom *argv)
{
    t_sfduration *x = (t_sfduration *)pd_new(sfduration_class);
    // TODO convert args into a filename, so spaces are valid
    x->filename = &s_;
    x->cwd = canvas_getdir(canvas_getcurrent());
    outlet_new(&x->x_obj, &s_float);

    x->decoder = bgav_create(); 

    return(x);
}

static void sfduration_free(t_sfduration *x)
{
    bgav_close(x->decoder);
}

void sfduration_setup(void)
{
    sfduration_class = class_new(gensym("sfduration"), 
                                (t_newmethod)sfduration_new, 
                                (t_method)sfduration_free, 
                                sizeof(t_sfduration), 
                                0, A_GIMME, 0);

    class_addbang(sfduration_class, (t_method)sfduration_bang);
    class_addsymbol(sfduration_class, (t_method)sfduration_symbol);
}
