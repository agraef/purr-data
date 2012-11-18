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

#include "FreeFrame.h"

#define PLUGINDIR "plugins"
#define FREEFRAME_PNAME_LENGTH 16
#define FREEFRAME_ID_LENGTH 4
#define FREEFRAME_PARAM_DETAILS_LENGTH 128

#define FF_CAP_V_BITS_VIDEO	FF_CAP_24BITVIDEO

typedef struct
{
    char	name[FREEFRAME_PNAME_LENGTH+1];
    plugMainType *plugmain;
    unsigned	instance;
    unsigned	numparameters;
} PLUGIN;

typedef struct pdp_freeframe_struct
{
    t_object x_obj;
    t_float x_f;

    t_outlet *x_outlet0;
    t_outlet *x_pname;
    t_outlet *x_nparams;
    t_outlet *x_parname;
    t_outlet *x_partype;
    t_outlet *x_pardefault;
    int x_packet0;
    int x_packet1;
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
    char x_pdetails[FREEFRAME_PARAM_DETAILS_LENGTH]; 
    t_symbol *plugindir;

    PLUGIN *plugins;
    
} t_pdp_freeframe;


void panic(const char *panicstr, ...)
{
    post("pdp_freeframe :: PANIC!! %s\n", panicstr);
    exit(1);
}

static int selector(const struct dirent *dp)
{    
    return (strstr(dp->d_name, ".so") != NULL);
}

static void scan_plugins(t_pdp_freeframe *x, char *plugindir)
{
    x->x_filename_count = scandir(plugindir, &x->x_filename_list, selector, alphasort);
    if (x->x_filename_count < 0)
        x->x_filename_count = 0;
}

void ff_loadplugins(t_pdp_freeframe *x, t_symbol *plugindirsymbol)
{
    char* plugindir = plugindirsymbol->s_name;
    char libname[PATH_MAX];
    plugMainType *plugmain;
    unsigned instance, numparameters;
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
	
        plugin_handle = dlopen(libname, RTLD_NOW);
        dlerror();
        plugmain = (plugMainType *)(unsigned)dlsym(plugin_handle, "plugMain");
        if (plugmain == NULL)
            panic("plugin %s: %s", x->x_filename_list[i]->d_name, dlerror());
	
        PlugInfoStruct *pis = (plugmain(FF_GETINFO, NULL, 0)).PISvalue;

        if ((plugmain(FF_GETPLUGINCAPS, (LPVOID)FF_CAP_V_BITS_VIDEO, 0)).ivalue != FF_TRUE)
            panic("plugin %s: no 24 bit support", pluginname);

        if (pis->APIMajorVersion < 1)
            panic("plugin %s: old api version", pluginname);
	
        if ((plugmain(FF_INITIALISE, NULL, 0)).ivalue == FF_FAIL)
            panic("plugin %s: init failed", pluginname);
	
        VideoInfoStruct vidinfo;
        vidinfo.frameWidth = x->x_width;
        vidinfo.frameHeight = x->x_height;
        vidinfo.orientation = 1;
        vidinfo.bitDepth = FF_CAP_V_BITS_VIDEO;

        instance = plugmain(FF_INSTANTIATE, &vidinfo, 0).ivalue;
        if (instance == FF_FAIL)
            panic("plugin %s: init failed",  pluginname);
	
        numparameters = plugmain(FF_GETNUMPARAMETERS, NULL, 0).ivalue;
        if (numparameters == FF_FAIL)
            panic("plugin %s: numparameters failed",  pluginname);
	
        x->plugins[x->x_plugin_count].plugmain = plugmain;
	
        strncpy(x->plugins[x->x_plugin_count].name, (char *)(pis->pluginName), 16);
        x->plugins[x->x_plugin_count].name[16] = 0;
	
        x->plugins[x->x_plugin_count].instance = instance;
        x->plugins[x->x_plugin_count].numparameters = numparameters;
	
        post("%s [%s] is loaded", x->plugins[x->x_plugin_count].name, pluginname);
        x->x_plugin_count++;
    }
}

void ff_processframe(t_pdp_freeframe *x, int plugin, void *buffer)
{
    x->plugins[plugin].plugmain(FF_PROCESSFRAME, buffer,  x->plugins[plugin].instance);
}

void ff_freeplugins(t_pdp_freeframe *x)
{
    t_int i;

    for (i=0; i<x->x_plugin_count; i++)
    {
        plugMainType *plugmain = x->plugins[i].plugmain;
    
        plugmain(FF_DEINITIALISE, NULL, 0);
        plugmain(FF_DEINSTANTIATE, NULL, x->plugins[i].instance);
    }
    post("freeing plugin resources : %x", x->plugins);
    freebytes( x->plugins, x->x_filename_count*sizeof(PLUGIN) );
    x->plugins = NULL;
}

static void pdp_freeframe_plugindir(t_pdp_freeframe *x, t_symbol *s)
{
    x->plugindir = s;
    if( s != &s_)
        ff_loadplugins(x, x->plugindir);
}

static void pdp_freeframe_process_rgb(t_pdp_freeframe *x)
{
    t_pdp     *header = pdp_packet_header(x->x_packet0);
    short int *data   = (short int *)pdp_packet_data(x->x_packet0);
    t_pdp     *newheader = pdp_packet_header(x->x_packet1);
    short int *newdata = (short int *)pdp_packet_data(x->x_packet1); 
      
    if ( x->x_plugin_count <= 0 )
    {
        return;
    }

    if ((x->x_width != (t_int)header->info.image.width) || 
        (x->x_height != (t_int)header->info.image.height)) 
    {

    	post("pdp_freeframe :: resizing plugins");
	
    	ff_freeplugins(x);

    	x->x_width = header->info.image.width;
    	x->x_height = header->info.image.height;
    	x->x_size = x->x_width*x->x_height;
    
    	//load the plugins
        ff_loadplugins(x, x->plugindir);
    }
    
    newheader->info.image.encoding = header->info.image.encoding;
    newheader->info.image.width = x->x_width;
    newheader->info.image.height = x->x_height;

    memcpy( newdata, data, x->x_size*3 );

    ff_processframe(x, x->x_plugin, newdata);
   
    return;
}

static void pdp_freeframe_param(t_pdp_freeframe *x, t_floatarg f1, t_floatarg f2)
{
    int i=0;
    plugMainType *plugmain = x->plugins[x->x_plugin].plugmain;
    unsigned instance = x->plugins[x->x_plugin].instance;
    unsigned numparameters = x->plugins[x->x_plugin].numparameters;

    SetParameterStruct sps;
    
    //for (i=0; i<numparameters; i++)
    //{
	//if (plugmain(FF_GETPARAMETERTYPE, (LPVOID)i, 0).ivalue == FF_TYPE_TEXT)
	//    continue;

	//sps.value = plugmain(FF_GETPARAMETER, (LPVOID)i, instance).fvalue;

	//post ("%d param.value %d", i, sps.value);
	
    //if((f2<=1)&&(f2>=0))
    sps.value = f2;
    //sps.value += .01;
	//if (sps.value > 1.0) sps.value = 1.0;
	//else
	//if (sps.value < 0.0) sps.value = 0.0;
	    
	sps.index = f1;
	plugmain(FF_SETPARAMETER, &sps, instance);
    //}
}

static void pdp_freeframe_plugin(t_pdp_freeframe *x, t_floatarg f)
{
    unsigned pi;
    char *parname;
    int partype;
    float pardefault;
    t_atom plist[2];
    t_atom tlist[2];
    t_atom vlist[2];

    if ( x->x_plugin_count <= 0 )
    {
        post( "freeframe : no plugins loaded, check your plugin directory setup" );
        return;
    }
    if((f<x->x_plugin_count)&&(f>-1)) 
    {
        x->x_plugin = f;
    }
    else
    {
        post( "freeframe : plugin out of range : %d", (t_int)f );
        return;
    }
    post ("pdp_freeframe :: %s selected, %d parameters", x->plugins[x->x_plugin].name, x->plugins[x->x_plugin].numparameters);
    outlet_symbol(x->x_pname, gensym( x->plugins[x->x_plugin].name ) );
    outlet_float(x->x_nparams, (float)x->plugins[x->x_plugin].numparameters);
    for ( pi=0; pi<x->plugins[x->x_plugin].numparameters; pi++ )
    {
        parname = (x->plugins[x->x_plugin].plugmain(FF_GETPARAMETERNAME, (LPVOID)pi, 0)).svalue;
        SETFLOAT(&plist[0], pi);
        SETSYMBOL(&plist[1], gensym(parname) );
        outlet_list( x->x_parname, &s_list, 2, &plist[0] );
    }  
    for ( pi=0; pi<x->plugins[x->x_plugin].numparameters; pi++ )
    {
        partype = (x->plugins[x->x_plugin].plugmain(FF_GETPARAMETERTYPE, (LPVOID)pi, 0)).ivalue;
        SETFLOAT(&tlist[0], pi);
        SETFLOAT(&tlist[1], partype );
        outlet_list( x->x_partype, &s_list, 2, &tlist[0] );
        if ( ( partype != FF_TYPE_TEXT ) &&  ( partype > 0 ) ) 
        {
            pardefault = (x->plugins[x->x_plugin].plugmain(FF_GETPARAMETERDEFAULT, (LPVOID)pi, 0)).fvalue;
            SETFLOAT(&vlist[0], pi);
            SETFLOAT(&vlist[1], pardefault );
            outlet_list( x->x_pardefault, &s_list, 2, &vlist[0] );
        }
    }  
}

static void pdp_freeframe_sendpacket(t_pdp_freeframe *x)
{
    /* release the packet */
    pdp_packet_mark_unused(x->x_packet0);
    x->x_packet0 = -1;

    /* unregister and propagate if valid dest packet */
    pdp_packet_pass_if_valid(x->x_outlet0, &x->x_packet1);
}

static void pdp_freeframe_process(t_pdp_freeframe *x)
{
    int encoding;
    t_pdp *header = 0;
    char *parname;
    unsigned pi;
    int partype;
    float pardefault;
    t_atom plist[2];
    t_atom tlist[2];
    t_atom vlist[2];

    /* check if image data packets are compatible */
    if ( (header = pdp_packet_header(x->x_packet0))
         && (PDP_BITMAP == header->type)){
    
        /* pdp_freeframe_process inputs and write into active inlet */
        switch(pdp_packet_header(x->x_packet0)->info.image.encoding){

        case PDP_BITMAP_RGB:
            x->x_packet1 = pdp_packet_clone_rw(x->x_packet0);
            pdp_queue_add(x, pdp_freeframe_process_rgb, pdp_freeframe_sendpacket, &x->x_queue_id);
            break;

        default:
            /* don't know the type, so dont pdp_freeframe_process */
            break;
	    
        }
    }

    // hack to display infos of first loaded plugin
    if ( (x->x_plugin_count > 0) && (!x->x_infosok) )
    {
        x->x_infosok = 1;
        outlet_symbol(x->x_pname, gensym( x->plugins[x->x_plugin].name ) );
        outlet_float(x->x_nparams, (float)x->plugins[x->x_plugin].numparameters);
        for ( pi=0; pi<x->plugins[x->x_plugin].numparameters; pi++ )
        {
            parname = (x->plugins[x->x_plugin].plugmain(FF_GETPARAMETERNAME, (LPVOID)pi, 0)).svalue;
            SETFLOAT(&plist[0], pi);
            SETSYMBOL(&plist[1], gensym(parname) );
            outlet_list( x->x_parname, &s_list, 2, &plist[0] );
        }  
        for ( pi=0; pi<x->plugins[x->x_plugin].numparameters; pi++ )
        {
            partype = (x->plugins[x->x_plugin].plugmain(FF_GETPARAMETERTYPE, (LPVOID)pi, 0)).ivalue;
            SETFLOAT(&tlist[0], pi);
            SETFLOAT(&tlist[1], partype );
            outlet_list( x->x_partype, &s_list, 2, &tlist[0] );
            if ( ( partype != FF_TYPE_TEXT ) &&  ( partype > 0 ) ) 
            {
                pardefault = (x->plugins[x->x_plugin].plugmain(FF_GETPARAMETERDEFAULT, (LPVOID)pi, 0)).fvalue;
                SETFLOAT(&vlist[0], pi);
                SETFLOAT(&vlist[1], pardefault );
                outlet_list( x->x_pardefault, &s_list, 2, &vlist[0] );
            }
        }  
    }
}

static void pdp_freeframe_input_0(t_pdp_freeframe *x, t_symbol *s, t_floatarg f)
{
    /* if this is a register_ro message or register_rw message, register with packet factory */

    if (s == gensym("register_rw")) 
        x->x_dropped = pdp_packet_convert_ro_or_drop(&x->x_packet0, (int)f, pdp_gensym("bitmap/rgb/*") );

    if ((s == gensym("process")) && (-1 != x->x_packet0) && (!x->x_dropped))
    {
        /* add the process method and callback to the process queue */
        pdp_freeframe_process(x);
    }
}

static void pdp_freeframe_free(t_pdp_freeframe *x)
{
    int i;

    pdp_queue_finish(x->x_queue_id);
    pdp_packet_mark_unused(x->x_packet0);
    ff_freeplugins(x);
}

t_class *pdp_freeframe_class;


void *pdp_freeframe_new(t_floatarg f)
{
    int i;

    t_pdp_freeframe *x = (t_pdp_freeframe *)pd_new(pdp_freeframe_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("plugin"));

    x->x_outlet0 = outlet_new(&x->x_obj, &s_anything); 
    x->x_pname = outlet_new(&x->x_obj, &s_anything); 
    x->x_nparams = outlet_new(&x->x_obj, &s_anything); 
    x->x_parname = outlet_new(&x->x_obj, &s_anything); 
    x->x_partype = outlet_new(&x->x_obj, &s_anything); 
    x->x_pardefault = outlet_new(&x->x_obj, &s_anything); 

    x->x_packet0 = -1;
    x->x_packet1 = -1;
    x->x_queue_id = -1;

    x->x_width  = 320;
    x->x_height = 240;
    x->x_size   = x->x_width * x->x_height;

    //load the plugins
    x->x_plugin_count = 0;
    x->x_infosok = 0;

    char ff_plugin_dir[FILENAME_MAX];
    char *home = getenv("HOME");
    int home_len;
    if(home != NULL)
    {
        home_len = strlen(home);
        if(home_len >= FILENAME_MAX)
            home_len = FILENAME_MAX - 1;
        memcpy(ff_plugin_dir, home, home_len);
        ff_plugin_dir[home_len] = '\0';
        strncpy(ff_plugin_dir, "/.frf", FILENAME_MAX - home_len);
        pdp_freeframe_plugindir(x, gensym(ff_plugin_dir));
    }
    pdp_freeframe_plugin(x, f);

    return (void *)x;
}


#ifdef __cplusplus
extern "C"
{
#endif


    void pdp_freeframe_setup(void)
    {

        post( "pdp_freeframe :: FreeFrame host for Pure Data Packet version 0.3\n                 by Lluis Gomez i Bigorda (lluis@artefacte.org) \n                 & Yves Degoyon (ydegoyon@free.fr)\n                 using GPL code from http://freeframe.sf.net\n                 and Pete Warden http://petewarden.com" );
        pdp_freeframe_class = class_new(gensym("pdp_freeframe"), (t_newmethod)pdp_freeframe_new,
                                        (t_method)pdp_freeframe_free, sizeof(t_pdp_freeframe), 0, A_DEFFLOAT, A_NULL);

        class_addmethod(pdp_freeframe_class, (t_method)pdp_freeframe_input_0, gensym("pdp"),  A_SYMBOL, A_DEFFLOAT, A_NULL);
        class_addmethod(pdp_freeframe_class, (t_method)pdp_freeframe_plugin, gensym("plugin"),  A_FLOAT, A_NULL);   
        class_addmethod(pdp_freeframe_class, (t_method)pdp_freeframe_param, gensym("param"),  A_FLOAT, A_FLOAT, A_NULL);   
        class_addmethod(pdp_freeframe_class, (t_method)pdp_freeframe_plugindir, gensym("plugindir"),  A_SYMBOL, A_NULL);   
        class_addmethod(pdp_freeframe_class, (t_method)pdp_freeframe_plugindir, gensym("pluginsdir"),  A_SYMBOL, A_NULL);   


    }

#ifdef __cplusplus
}
#endif
