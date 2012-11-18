/*
 *   Pure Data Packet module.
 *   Copyright (c) by Tom Schouten <pdp@zzz.kotnet.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <dlfcn.h>

#include "pdp.h"
#include "frei0r.h"

#define FREI0R_PNAME_LENGTH 128

typedef struct
{
    char name[FREI0R_PNAME_LENGTH+1];
    unsigned numparameters;
    int plugin_type;
    f0r_instance_t instance;
    f0r_plugin_info_t plugin_info;
    int (*f0r_init)(void);
    void (*f0r_get_plugin_info)(f0r_plugin_info_t* pluginInfo);
    void (*f0r_get_param_info)(f0r_param_info_t* info, int param_index);
    f0r_instance_t (*f0r_construct)(unsigned int width, unsigned int height);
    void (*f0r_destruct)(f0r_instance_t instance);
    void (*f0r_set_param_value)(f0r_instance_t instance, f0r_param_t param, int param_index);
    void (*f0r_get_param_value)(f0r_instance_t instance, f0r_param_t param, int param_index);
    void (*f0r_update)(f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe);
    void (*f0r_update2)(f0r_instance_t instance, double time,
                      const uint32_t* inframe1, const uint32_t* inframe2, const uint32_t* inframe3, uint32_t* outframe);
    int (*f0r_deinit)(void);
} PLUGIN;

typedef struct pdp_frei0r_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_pname;
    t_outlet *x_nparams;
    t_outlet *x_parname;
    t_outlet *x_partype;
    int x_packet0;
    int x_packet1;
    int x_packet2;
    int x_packet3;

    // middle input packets
    int x_packetm0;
    int x_packetm1;

    // right input packets
    int x_packetr0;
    int x_packetr1;

    int x_dropped;
    int x_queue_id;

    int x_width;
    int x_height;
    int x_size;

    int x_plugin_count;
    struct dirent **x_filename_list;
    int x_filename_count;

    int x_plugin; 
    int x_infosok; 

    t_symbol *plugindir;

    PLUGIN *plugins;
    
} t_pdp_frei0r;

void panic(const char *panicstr, ...)
{
    post("pdp_frei0r :: PANIC!! %s\n", panicstr);
    exit(1);
}

static int selector(const struct dirent *dp)
{    
    return (strstr(dp->d_name, ".so") != NULL);
}

static void scan_plugins(t_pdp_frei0r *x, char *plugindir)
{
    x->x_filename_count = scandir(plugindir, &x->x_filename_list, selector, alphasort);
    if (x->x_filename_count < 0)
    {
       x->x_filename_count = 0;
    }
}

void fr_loadplugins(t_pdp_frei0r *x, t_symbol *plugindirsymbol)
{
    char* plugindir = plugindirsymbol->s_name;
    char libname[PATH_MAX];
    int i;
    char *pluginname;
    void *plugin_handle;
    
    scan_plugins(x, plugindir);

    x->x_plugin_count = 0;
    x->plugins = (PLUGIN *)getbytes(x->x_filename_count*sizeof(PLUGIN));
    if (x->plugins == NULL)
    {
	panic("no memory for loading plugins\n");
    }  
    else
    {
        post( "allocated plugins : %x", x->plugins );
    }

    for (i=0; i<x->x_filename_count; i++)
    {
	pluginname = x->x_filename_list[i]->d_name;
	
	snprintf(libname, PATH_MAX, "%s/%s", plugindir, pluginname);
	
        post( "pdp_frei0r : opening : %s", pluginname );

	plugin_handle = dlopen(libname, RTLD_NOW);
	dlerror();

        // get functions pointers
        *(void**) (&x->plugins[x->x_plugin_count].f0r_init) = dlsym(plugin_handle, "f0r_init");
        *(void**) (&x->plugins[x->x_plugin_count].f0r_get_plugin_info) = dlsym(plugin_handle, "f0r_get_plugin_info");
        *(void**) (&x->plugins[x->x_plugin_count].f0r_get_param_info) = dlsym(plugin_handle, "f0r_get_param_info");
        *(void**) (&x->plugins[x->x_plugin_count].f0r_construct) = dlsym(plugin_handle, "f0r_construct");
        *(void**) (&x->plugins[x->x_plugin_count].f0r_destruct) = dlsym(plugin_handle, "f0r_destruct");
        *(void**) (&x->plugins[x->x_plugin_count].f0r_set_param_value) = dlsym(plugin_handle, "f0r_set_param_value");
        *(void**) (&x->plugins[x->x_plugin_count].f0r_get_param_value) = dlsym(plugin_handle, "f0r_get_param_value");
        *(void**) (&x->plugins[x->x_plugin_count].f0r_update) = dlsym(plugin_handle, "f0r_update");
	dlerror();

        // check for special function update2
        *(void**) (&x->plugins[x->x_plugin_count].f0r_update2) = dlsym(plugin_handle, "f0r_update2");
        if (!dlerror())
        {
           // continue;
        }

        // init plugin
        (*x->plugins[x->x_plugin_count].f0r_init)();

        // instantiate
        x->plugins[x->x_plugin_count].instance = (*x->plugins[x->x_plugin_count].f0r_construct)(x->x_width, x->x_height);

        // get plugin infos
        (*x->plugins[x->x_plugin_count].f0r_get_plugin_info)(&x->plugins[x->x_plugin_count].plugin_info); 
        strcpy( x->plugins[x->x_plugin_count].name, x->plugins[x->x_plugin_count].plugin_info.name );
        x->plugins[x->x_plugin_count].numparameters=x->plugins[x->x_plugin_count].plugin_info.num_params;
        x->plugins[x->x_plugin_count].plugin_type=x->plugins[x->x_plugin_count].plugin_info.plugin_type;

        if ( ( x->plugins[x->x_plugin_count].plugin_info.color_model != F0R_COLOR_MODEL_RGBA8888 ) &&
             ( x->plugins[x->x_plugin_count].plugin_info.color_model != F0R_COLOR_MODEL_BGRA8888 ) &&
             ( x->plugins[x->x_plugin_count].plugin_info.color_model != F0R_COLOR_MODEL_PACKED32 ) )
        {
           post( "pdp_frei0r : warning : plugin : %s use unsupported color model (%d) : ignored ...", 
                              pluginname, x->plugins[x->x_plugin_count].plugin_info.color_model );
           continue;
        }
 
        x->x_plugin_count++;
    }
}

void fr_processframe(t_pdp_frei0r *x, int plugin, void *ibuffer, void *imbuffer, void *irbuffer, void *obuffer)
{
   double time = 0.0;

      if ( x->x_plugin_count <= 0 )
      {
           return;
      }

      if ( ( x->plugins[x->x_plugin].plugin_type == F0R_PLUGIN_TYPE_FILTER ) ||
           ( x->plugins[x->x_plugin].plugin_type == F0R_PLUGIN_TYPE_SOURCE ) )
      {
         // process frame
         (*x->plugins[x->x_plugin].f0r_update)(x->plugins[x->x_plugin].instance, time, ibuffer, obuffer);
      }
      if ( x->plugins[x->x_plugin].plugin_type == F0R_PLUGIN_TYPE_MIXER2 )
      {
         if ( ( x->x_packet1 != -1 ) && ( x->x_packet2 != -1 ) && ( x->x_packetm1 != -1 ) )
         {
           // process frame
           (*x->plugins[x->x_plugin].f0r_update2)(x->plugins[x->x_plugin].instance, time, ibuffer, imbuffer, irbuffer, obuffer);
         }
      }
      if ( x->plugins[x->x_plugin].plugin_type == F0R_PLUGIN_TYPE_MIXER3 )
      {
         if ( ( x->x_packet1 != -1 ) && ( x->x_packet2 != -1 ) && ( x->x_packetm1 != -1 ) && ( x->x_packetr1 != -1 ) )
         {
           // process frame
           (*x->plugins[x->x_plugin].f0r_update2)(x->plugins[x->x_plugin].instance, time, ibuffer, imbuffer, irbuffer, obuffer);
         }
      }
}

void fr_freeplugins(t_pdp_frei0r *x)
{
  t_int i;

    for (i=0; i<x->x_plugin_count; i++)
    {
       // destroy plugin
       (*x->plugins[i].f0r_destruct)(x->plugins[i].instance);
    }
    post("freeing plugin resources : %x", x->plugins);
    freebytes( x->plugins, x->x_filename_count*sizeof(PLUGIN) );
    x->plugins = NULL;
}

static void pdp_frei0r_plugindir(t_pdp_frei0r *x, t_symbol *s)
{
    x->plugindir = s;
    if( s != &s_)
        fr_loadplugins(x, x->plugindir);
}

static void pdp_frei0r_process_rgba(t_pdp_frei0r *x)
{
    t_pdp     *newheader = pdp_packet_header(x->x_packet2);
    char *newdata = (char *)pdp_packet_data(x->x_packet2); 
    t_pdp     *lheader = pdp_packet_header(x->x_packet1);
    char *ldata   = (char *)pdp_packet_data(x->x_packet1);
    t_pdp     *mheader = pdp_packet_header(x->x_packetm1);
    char *mdata = (char *)pdp_packet_data(x->x_packetm1); 
    t_pdp     *rheader = pdp_packet_header(x->x_packetr1);
    char *rdata = (char *)pdp_packet_data(x->x_packetr1); 

    if ((x->x_width != (t_int)lheader->info.image.width) || 
        (x->x_height != (t_int)lheader->info.image.height)) 
    {

    	post("pdp_frei0r :: resizing plugins");
	
    	fr_freeplugins(x);

    	x->x_width = lheader->info.image.width;
    	x->x_height = lheader->info.image.height;
    	x->x_size = x->x_width*x->x_height;

    	//load the plugins
    	fr_loadplugins(x, x->plugindir);
    }
    
    newheader->info.image.encoding = lheader->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, ldata, x->x_size*sizeof(uint32_t) );

    fr_processframe(x, x->x_plugin, ldata, mdata, rdata, newdata);
   

}

static void pdp_frei0r_param(t_pdp_frei0r *x, t_symbol *s, int argc, t_atom *argv)
{
  int pnumber;
  f0r_param_info_t param_infos;
 
      if ( x->x_plugin_count <= 0 )
      {
        post( "frei0r : set parameter : no plugins loaded, check your plugin directory setup" );
        return;
      }

      if ( argc < 2 )
      {
         post("pdp_frei0r : set parameter : insufficient parameters (%d)", argc );
         return;
      }

      if ( argv[0].a_type != A_FLOAT )
      {
         post("pdp_frei0r : set parameter : wrong parameter number" );
         return;
      }
      pnumber = (int)argv[0].a_w.w_float;
      // post("pdp_frei0r : setting parameter : %d", pnumber );

      if ( (pnumber<0) || (pnumber>=(int)x->plugins[x->x_plugin].numparameters) )
      {
         post("pdp_frei0r : set parameter : wrong parameter number : %d : max : %d", pnumber, x->plugins[x->x_plugin].numparameters );
         return;
      }

      (*x->plugins[x->x_plugin].f0r_get_param_info)(&param_infos, pnumber);

      // set parameter
      switch (param_infos.type)
      {
        case F0R_PARAM_BOOL:
          {
           f0r_param_bool fvalue;
 
            if ( argc != 2 )
            {
              post("pdp_frei0r : wrong parameter arguments (%d) for boolean", argc );
              return;
            }
            if ( argv[1].a_type != A_FLOAT )
            {
               post("pdp_frei0r : wrong parameter value" );
               return;
            }
            if ( argv[1].a_w.w_float != 0. && argv[1].a_w.w_float != 1. )
            {
               // post("pdp_frei0r : wrong parameter value for boolean" );
               return;
            }
            fvalue=argv[1].a_w.w_float;
            (*x->plugins[x->x_plugin].f0r_set_param_value)(x->plugins[x->x_plugin].instance, &fvalue, pnumber);
          }
          break;
        case F0R_PARAM_DOUBLE:
          {
           f0r_param_double fvalue;

            if ( argc != 2 )
            {
              post("pdp_frei0r : wrong parameter arguments (%d) for double", argc );
              return;
            }
            if ( argv[1].a_type != A_FLOAT )
            {
               post("pdp_frei0r : wrong parameter value" );
               return;
            }
            fvalue=argv[1].a_w.w_float;
            (*x->plugins[x->x_plugin].f0r_set_param_value)(x->plugins[x->x_plugin].instance, &fvalue, pnumber);
          }
          break;
        case F0R_PARAM_COLOR:
          {
           struct f0r_param_color color;

            if ( argc != 4 )
            {
              post("pdp_frei0r : wrong parameter arguments (%d) for color", argc );
              return;
            }
            if ( argv[1].a_type != A_FLOAT ||
                 argv[2].a_type != A_FLOAT ||
                 argv[3].a_type != A_FLOAT )
            {
               post("pdp_frei0r : wrong parameter value" );
               return;
            }
            color.r = argv[1].a_w.w_float;
            color.g = argv[2].a_w.w_float;
            color.b = argv[3].a_w.w_float;
            (*x->plugins[x->x_plugin].f0r_set_param_value)(x->plugins[x->x_plugin].instance, &color, pnumber);
          }
          break;
        case F0R_PARAM_POSITION:
          {
           struct f0r_param_position position;

            if ( argc != 3 )
            {
              post("pdp_frei0r : wrong parameter arguments (%d) for position", argc );
              return;
            }
            if ( argv[1].a_type != A_FLOAT ||
                 argv[2].a_type != A_FLOAT )
            {
               post("pdp_frei0r : wrong parameter value" );
               return;
            }
            position.x = argv[1].a_w.w_float;
            position.y = argv[2].a_w.w_float;
            (*x->plugins[x->x_plugin].f0r_set_param_value)(x->plugins[x->x_plugin].instance, &position, pnumber);
          }
          break;
        default:
          post("pdp_frei0r : unsupported parameter type (%d)", param_infos.type );
          return;
      }
}

static void pdp_frei0r_plugin(t_pdp_frei0r *x, t_floatarg f)
{
  unsigned pi;
  char *parname;
  int partype;
  t_atom plist[2];
  t_atom tlist[2];
  t_atom vlist[2];
  f0r_param_info_t param_infos;

    if ( x->x_plugin_count <= 0 )
    {
      post( "frei0r : no plugins loaded, check your plugin directory setup" );
      return;
    }
    if((f<x->x_plugin_count)&&(f>-1)) 
    {
      x->x_plugin = f;
    }
    else
    {
      post( "frei0r : plugin out of range : %d", (t_int)f );
      return;
    }
    post ("pdp_frei0r :: %s selected, %d parameters", x->plugins[x->x_plugin].name, x->plugins[x->x_plugin].numparameters);
    outlet_symbol(x->x_pname, gensym( x->plugins[x->x_plugin].name ) );
    outlet_float(x->x_nparams, (float)x->plugins[x->x_plugin].numparameters);
    for ( pi=0; pi<x->plugins[x->x_plugin].numparameters; pi++ )
    {
      (*x->plugins[x->x_plugin].f0r_get_param_info)(&param_infos, pi);
      SETFLOAT(&plist[0], pi);
      SETSYMBOL(&plist[1], gensym(param_infos.name) );
      outlet_list( x->x_parname, &s_list, 2, &plist[0] );
      SETFLOAT(&tlist[0], pi);
      SETFLOAT(&tlist[1], param_infos.type );
      outlet_list( x->x_partype, &s_list, 2, &tlist[0] );
    }
}

static void pdp_frei0r_sendpacket(t_pdp_frei0r *x)
{
    /* release the incoming packets */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;
    pdp_packet_mark_unused(x->x_packet1);
    x->x_packet1 = -1;
    pdp_packet_mark_unused(x->x_packetr0);
    x->x_packetr0 = -1;
    pdp_packet_mark_unused(x->x_packetr1);
    x->x_packetr1 = -1;
    pdp_packet_mark_unused(x->x_packetm0);
    x->x_packetm0 = -1;
    pdp_packet_mark_unused(x->x_packetm1);
    x->x_packetm1 = -1;

    x->x_packet3=-1;
    x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet3, x->x_packet2, pdp_gensym("bitmap/rgb/*") );
    pdp_packet_mark_unused(x->x_packet2);
    x->x_packet2 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet3);
}

static void pdp_frei0r_process(t_pdp_frei0r *x)
{
   int encoding;
   t_pdp *header = 0;
   char *parname;
   unsigned pi;
   int partype;
   t_atom plist[2];
   t_atom tlist[2];
   t_atom vlist[2];
   f0r_param_info_t param_infos;

   /* check if image data packets are compatible */
   if ( (header = pdp_packet_header(x->x_packet1))
	&& (PDP_BITMAP == header->type)){
    
	/* pdp_frei0r_process inputs and write into active inlet */
	switch(pdp_packet_header(x->x_packet1)->info.image.encoding){

	case PDP_BITMAP_RGBA:
            x->x_packet2 = pdp_packet_clone_rw(x->x_packet1);
            pdp_queue_add(x, pdp_frei0r_process_rgba, pdp_frei0r_sendpacket, &x->x_queue_id);
	    break;

	default:
	    /* don't know the type, so dont pdp_frei0r_process */
	    break;
	    
	}
    }

    // hack to display infos of first loaded plugin
    if ( ( x->x_plugin_count>0 ) && ( !x->x_infosok ) )
    {
       outlet_symbol(x->x_pname, gensym( x->plugins[x->x_plugin].name ) );
       outlet_float(x->x_nparams, (float)x->plugins[x->x_plugin].numparameters);
       for ( pi=0; pi<x->plugins[x->x_plugin].numparameters; pi++ )
       {
         (*x->plugins[x->x_plugin].f0r_get_param_info)(&param_infos, pi);
         SETFLOAT(&plist[0], pi);
         SETSYMBOL(&plist[1], gensym(param_infos.name) );
         outlet_list( x->x_parname, &s_list, 2, &plist[0] );
         SETFLOAT(&tlist[0], pi);
         SETFLOAT(&tlist[1], param_infos.type );
         outlet_list( x->x_partype, &s_list, 2, &tlist[0] );
       }
       x->x_infosok = 1;
    }
}

static void pdp_frei0r_input_0(t_pdp_frei0r *x, t_symbol *s, t_floatarg f)
{
    if (s == gensym("register_rw")) 
    {
       x->x_packet0=-1;
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("bitmap/rgb/*") );
       // post("pdp_freiOr : drop 1 : %d", x->x_dropped );
       x->x_packet1=-1;
       x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet1, x->x_packet0, pdp_gensym("bitmap/rgba/*") );
       // post("pdp_freiOr : drop 2 : %d", x->x_dropped );
    }

    if ((s == gensym("process")) && (-1 != x->x_packet1) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_frei0r_process(x);
    }
}

static void pdp_frei0r_input_1(t_pdp_frei0r *x, t_symbol *s, t_floatarg f)
{
  char *ddata;
  char *sdata;

    if ( x->x_packetm0 != -1 )
    {
       return;
    }
    x->x_dropped = pdp_packet_convert_rw_or_drop(&x->x_packetm0, (int)f, pdp_gensym("bitmap/rgb/*") );
    // post("pdp_freiOr : middle drop 1 : %d", x->x_dropped );
    x->x_dropped = pdp_packet_convert_rw_or_drop(&x->x_packetm1, x->x_packetm0, pdp_gensym("bitmap/rgba/*") );
    // post("pdp_freiOr : middle drop 2 : %d", x->x_dropped );
}

static void pdp_frei0r_input_2(t_pdp_frei0r *x, t_symbol *s, t_floatarg f)
{
  char *ddata;
  char *sdata;

    if ( x->x_packetr0 != -1 )
    {
       return;
    }
    x->x_dropped = pdp_packet_convert_rw_or_drop(&x->x_packetr0, (int)f, pdp_gensym("bitmap/rgb/*") );
    // post("pdp_freiOr : right drop 1 : %d", x->x_dropped );
    x->x_dropped = pdp_packet_convert_rw_or_drop(&x->x_packetr1, x->x_packetr0, pdp_gensym("bitmap/rgba/*") );
    // post("pdp_freiOr : right drop 2 : %d", x->x_dropped );
}

static void pdp_frei0r_free(t_pdp_frei0r *x)
{
  int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    fr_freeplugins(x);
}

t_class *pdp_frei0r_class;

void *pdp_frei0r_new(t_floatarg f)
{
    int i;

    t_pdp_frei0r *x = (t_pdp_frei0r *)pd_new(pdp_frei0r_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("pdp"), gensym("pdp1") );
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("pdp"), gensym("pdp2") );
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("plugin"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_pname = outlet_new(&x->x_obj, &s_anything); 
    x->x_nparams = outlet_new(&x->x_obj, &s_anything); 
    x->x_parname = outlet_new(&x->x_obj, &s_anything); 
    x->x_partype = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_packet2 = -1;
    x->x_packet3 = -1;
    x->x_packetm0 = -1;
    x->x_packetm1 = -1;
    x->x_packetr0 = -1;
    x->x_packetr1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    //load the plugins
    x->x_plugin_count = 0;
    x->x_infosok = 0;
    
    char fr_plugin_dir[FILENAME_MAX];
    char *home = getenv("HOME");
    int home_len;
    if(home != NULL)
    {
        home_len = strlen(home);
        if(home_len >= FILENAME_MAX)
            home_len = FILENAME_MAX - 1;
        memcpy(fr_plugin_dir, home, home_len);
        fr_plugin_dir[home_len] = '\0';
        strncpy(fr_plugin_dir, "/.frf", FILENAME_MAX - home_len);
        pdp_frei0r_plugindir(x, gensym(fr_plugin_dir));
    }

    //select plugin
    pdp_frei0r_plugin(x, f);

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


void pdp_frei0r_setup(void)
{

    post( "pdp_frei0r :: frei0r host for Pure Data Packet version 0.1\n                 by Lluis Gomez i Bigorda (lluis@artefacte.org) \n                 & Yves Degoyon (ydegoyon@free.fr)\n                 using frei0r specification 1.1 by Georg Seidel,\n		 Phillip Promesberger and Martin Bayer\n                 made at piksel yearly gathering : http://www.piksel.no" );
    pdp_frei0r_class = class_new(gensym("pdp_frei0r"), (t_newmethod)pdp_frei0r_new,
    	(t_method)pdp_frei0r_free, sizeof(t_pdp_frei0r), 0, A_DEFFLOAT, A_NULL);

    class_addmethod(pdp_frei0r_class, (t_method)pdp_frei0r_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_frei0r_class, (t_method)pdp_frei0r_input_1, gensym("pdp1"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_frei0r_class, (t_method)pdp_frei0r_input_2, gensym("pdp2"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
    class_addmethod(pdp_frei0r_class, (t_method)pdp_frei0r_plugin, gensym("plugin"),  A_FLOAT, A_NULL);   
    class_addmethod(pdp_frei0r_class, (t_method)pdp_frei0r_param, gensym("param"),  A_GIMME, A_NULL);   
    class_addmethod(pdp_frei0r_class, (t_method)pdp_frei0r_plugindir, gensym("plugindir"),  A_SYMBOL, A_NULL);   

}

#ifdef __cplusplus
}
#endif
