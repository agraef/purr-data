/* --------------------------------------------------------------------------*/
/*                                                                           */
/* object for getting file listings using wildcard patterns,                 */
/* based on the interface of [textfile]                                      */
/* Written by Hans-Christoph Steiner <hans@at.or.at>                         */
/*                                                                           */
/* Copyright (c) 2010 Hans-Christoph Steiner                                 */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU General Public License               */
/* as published by the Free Software Foundation; either version 3            */
/* of the License, or (at your option) any later version.                    */
/*                                                                           */
/* See file LICENSE for further informations on licensing terms.             */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software Foundation,   */
/* Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.           */
/*                                                                           */
/* --------------------------------------------------------------------------*/

#include <m_pd.h>

#ifdef _WIN32
#define _WIN32_WINNT 0x0400
#include <windows.h>
#else
#include <stdlib.h>
#include <glob.h>
#endif

#include <stdio.h>
#include <string.h>

#define DEBUG(x)
//#define DEBUG(x) x 

// TODO check out what happens with [getfilesnames $1] and empty args
// TODO using an A_GIMME to support floats, etc as file names

/*------------------------------------------------------------------------------
 *  CLASS DEF
 */
static t_class *getfilenames_class;

typedef struct _getfilenames {
	t_object            x_obj;
    t_canvas*           x_canvas;    
	t_symbol*           x_pattern;
	t_symbol*           active_pattern;
#ifdef _WIN32
	HANDLE              hFind;
#else
	glob_t              glob_buffer;
    unsigned int        current_glob_position;
#endif
	t_outlet            *data_outlet;
	t_outlet            *info_outlet;
	t_outlet            *endoflist_outlet;
} t_getfilenames;

/*------------------------------------------------------------------------------
 *  GLOBAL VARIABLES
 */

/* pre-generated symbols */
t_symbol *ps_info, *ps_matches, *ps_pattern, *ps_position, *ps_rewind;

/*------------------------------------------------------------------------------
 * IMPLEMENTATION                    
 */

// TODO: make FindFirstFile display when its just a dir

static void normalize_path(t_getfilenames* x, char *normalized, const char *original)
{
    char buf[FILENAME_MAX];
    t_symbol *cwd = canvas_getdir(x->x_canvas);
#ifdef _WIN32
    char patternBuffer[FILENAME_MAX];
    char envVarBuffer[FILENAME_MAX];
    if( (original[0] == '~') && (original[1] == '/'))
    {
        strcpy(patternBuffer,"%USERPROFILE%");
        strncat(patternBuffer, original + 1, FILENAME_MAX - 1);
    }
    else
    {
        patternBuffer = original;
    }
	ExpandEnvironmentStrings(patternBuffer, envVarBuffer, FILENAME_MAX - 2);
    sys_unbashfilename(envVarBuffer, buf);
#else
    // TODO translate env vars to a full path using /bin/sh and exec
    strncpy(buf, original, FILENAME_MAX);
#endif
    if(sys_isabsolutepath(buf)) {
        strncpy(normalized, buf, FILENAME_MAX);
        return;
    }
    strncpy(normalized, cwd->s_name, FILENAME_MAX);
    if(normalized[(strlen(normalized)-1)] != '/') {
        strncat(normalized, "/", 1);
    }
    if(buf[0] == '.') {
        if(buf[1] == '/') {
            strncat(normalized, buf + 2, 
                    FILENAME_MAX - strlen(normalized));
        } else if(buf[1] == '.' && buf[2] == '/') {
            strncat(normalized, buf, 
                    FILENAME_MAX - strlen(normalized));
        }
    } else if(buf[0] != '/') {
        strncat(normalized, buf, 
                FILENAME_MAX - strlen(normalized));
    } else {
        strncpy(normalized, buf, FILENAME_MAX);
    }
}

/* functions for implementing the querying methods */

static void getfilenames_matches(t_getfilenames *x)
{
    t_atom output_atom;
    SETFLOAT(&output_atom, (t_float)x->glob_buffer.gl_pathc);
    outlet_anything(x->info_outlet, ps_matches, 1, &output_atom);
}

static void getfilenames_pattern(t_getfilenames *x)
{
    t_atom output_atom;
    SETSYMBOL(&output_atom, x->x_pattern);
    outlet_anything(x->info_outlet, ps_pattern, 1, &output_atom);
}

static void getfilenames_position(t_getfilenames *x)
{
    t_atom output_atom;
    SETFLOAT(&output_atom, x->current_glob_position);
    outlet_anything(x->info_outlet, ps_position, 1, &output_atom);
}

static void getfilenames_info(t_getfilenames *x)
{
    getfilenames_matches(x);
    getfilenames_pattern(x);
    getfilenames_position(x);
}

/* working functions */

static void getfilenames_rewind(t_getfilenames* x)
{
	DEBUG(post("getfilenames_rewind"););
    char normalized_path[FILENAME_MAX] = "";

    normalize_path(x, normalized_path, x->active_pattern->s_name);
#ifdef _WIN32
	WIN32_FIND_DATA findData;
	HANDLE hFind;
	DWORD errorNumber;
	LPVOID lpErrorMessage;
	char fullPathNameBuffer[FILENAME_MAX] = "";
	char unbashBuffer[FILENAME_MAX] = "";
	char outputBuffer[FILENAME_MAX] = "";
	char *pathBuffer;

// arg, looks perfect, but only in Windows Vista
//	GetFinalPathNameByHandle(hFind,fullPathNameBuffer,FILENAME_MAX,FILE_NAME_NORMALIZED);
    GetFullPathName(normalized_path, FILENAME_MAX, fullPathNameBuffer, NULL);
    sys_unbashfilename(fullPathNameBuffer,unbashBuffer);
	
	hFind = FindFirstFile(fullPathNameBuffer, &findData);
	if (hFind == INVALID_HANDLE_VALUE) 
	{
	   errorNumber = GetLastError();
	   switch (errorNumber)
	   {
       case ERROR_FILE_NOT_FOUND:
       case ERROR_PATH_NOT_FOUND:
           pd_error(x,"[getfilenames] nothing found for \"%s\"",
                    x->active_pattern->s_name);
           break;
       default:
           FormatMessage(
               FORMAT_MESSAGE_ALLOCATE_BUFFER | 
               FORMAT_MESSAGE_FROM_SYSTEM,
               NULL,
               errorNumber,
               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
               (LPTSTR) &lpErrorMessage,
               0, NULL );
           pd_error(x,"[getfilenames] %s", (char *)lpErrorMessage);
	   }
	   return;
	} 
    char* unbashBuffer_position = strrchr(unbashBuffer, '/');
    if(unbashBuffer_position)
    {
        pathBuffer = getbytes(FILENAME_MAX+1);
        strncpy(pathBuffer, unbashBuffer, unbashBuffer_position - unbashBuffer);
    }
	do {
        // skip "." and ".."
        if( strcmp(findData.cFileName, ".") && strcmp(findData.cFileName, "..") ) 
		{
            strncpy(outputBuffer, pathBuffer, FILENAME_MAX);
			strcat(outputBuffer,"/");
			strcat(outputBuffer,findData.cFileName);
			outlet_symbol( x->data_outlet, gensym(outputBuffer) );
		}
	} while (FindNextFile(hFind, &findData) != 0);
	FindClose(hFind);
#else /* systems with glob */
	DEBUG(post("globbing %s",normalized_path););
    x->current_glob_position = 0;
	switch( glob( normalized_path, GLOB_TILDE, NULL, &x->glob_buffer ) )
	{
    case GLOB_NOSPACE: 
        pd_error(x,"[getfilenames] out of memory for \"%s\"",normalized_path); 
        break;
# ifdef GLOB_ABORTED
    case GLOB_ABORTED: 
        pd_error(x,"[getfilenames] aborted \"%s\"",normalized_path); 
        break;
# endif
# ifdef GLOB_NOMATCH
    case GLOB_NOMATCH: 
        verbose(4, "[getfilenames] nothing found for \"%s\"",normalized_path); 
        break;
# endif
	}
#endif /* _WIN32 */
}

static void getfilenames_bang(t_getfilenames *x) 
{
    if(x->x_pattern != x->active_pattern) 
    {
        x->active_pattern = x->x_pattern;
//        post("x->active_pattern %s x->x_pattern %s", x->active_pattern->s_name, x->x_pattern->s_name);
        getfilenames_rewind(x);
        getfilenames_info(x); /* after opening new glob, output meta info */
    }
    if(x->current_glob_position < x->glob_buffer.gl_pathc) 
    {
        getfilenames_position(x);
		outlet_symbol(x->data_outlet, 
                       gensym(x->glob_buffer.gl_pathv[x->current_glob_position]));
        x->current_glob_position++;
    }
    else
        outlet_bang(x->endoflist_outlet);
}

static void getfilenames_float(t_getfilenames *x, t_float f)
{
    unsigned int position = (unsigned int)f;
    if(x->current_glob_position != position)
    {
        x->current_glob_position = position;
        if(x->current_glob_position < x->glob_buffer.gl_pathc) 
        {
            getfilenames_position(x);
            outlet_symbol(x->data_outlet, 
                          gensym(x->glob_buffer.gl_pathv[x->current_glob_position]));
        }
        else
            outlet_bang(x->endoflist_outlet);
    }
}

/* the core guts that make a pd object */

static void *getfilenames_new(t_symbol *s)
{
	DEBUG(post("getfilenames_new"););

	t_getfilenames *x = (t_getfilenames *)pd_new(getfilenames_class);
	t_symbol *currentdir;
	char buffer[MAXPDSTRING];

    x->x_canvas = canvas_getcurrent();

    symbolinlet_new(&x->x_obj, &x->x_pattern);
    x->data_outlet = outlet_new(&x->x_obj, &s_symbol);
    x->info_outlet = outlet_new(&x->x_obj, 0);
    x->endoflist_outlet = outlet_new(&x->x_obj, &s_bang);
	
	/* set to the value from the object argument, if that exists */
	if (s != &s_)
	{
		x->x_pattern = s;
        verbose(4, "setting pattern to: %s", x->x_pattern->s_name);
	}
	else
	{
		currentdir = canvas_getcurrentdir();
		strncpy(buffer,currentdir->s_name,MAXPDSTRING);
		strncat(buffer,"/*",MAXPDSTRING);
		x->x_pattern = gensym(buffer);
        verbose(4, "setting pattern to default: *");
	}
    /* this is activated in getfilenames_bang() */
    x->active_pattern = &s_;

	return (x);
}

static void getfilenames_free(t_getfilenames *x)
{
	globfree( &(x->glob_buffer) );
}

void getfilenames_setup(void) 
{
	DEBUG(post("getfilenames_setup"););
	getfilenames_class = class_new(gensym("getfilenames"), 
								  (t_newmethod)getfilenames_new, 
								  (t_method)getfilenames_free, 
								  sizeof(t_getfilenames), 
								  0,
								  A_DEFSYMBOL, 
								  0);
	/* add inlet datatype methods */
	class_addbang(getfilenames_class, (t_method) getfilenames_bang);
	class_addfloat(getfilenames_class, (t_method) getfilenames_float);
	
	/* add inlet message methods */
	class_addmethod(getfilenames_class, (t_method) getfilenames_rewind,
                    gensym("rewind"), 0);
    /* querying methods */
	class_addmethod(getfilenames_class, (t_method) getfilenames_info,
                    gensym("info"), 0);
	class_addmethod(getfilenames_class, (t_method) getfilenames_matches,
                    gensym("matches"), 0);
	class_addmethod(getfilenames_class, (t_method) getfilenames_pattern,
                    gensym("pattern"), 0);
	class_addmethod(getfilenames_class, (t_method) getfilenames_position,
                    gensym("position"), 0);

    /* pre-generate often used symbols */
    ps_rewind = gensym("rewind");
    ps_info = gensym("info");
    ps_matches = gensym("matches");
    ps_pattern = gensym("pattern");
    ps_position = gensym("position");
}

