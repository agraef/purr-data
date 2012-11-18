/* plugin~, a Pd tilde object for hosting LADSPA/VST plug-ins
   Copyright (C) 2001 Jarno Seppänen
   Copyright (C) 2000 Richard W.E. Furse
   $Id: vstutils.h,v 1.1 2004-01-08 14:55:24 ksvalast Exp $

   This file is part of plugin~.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */

#ifndef __VSTUTILS_H__
#define __VSTUTILS_H__

#include <vstlib.h>

#ifdef WIN32 /* currently for Windows only */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* VST SDK header */
#include "vst/AEffect.h"

void*		vstutils_load_vst_plugin_dll (const char* plugin_dll_filename);
void		vstutils_unload_vst_plugin_dll (void* plugin_dll);
AEffect*	vstutils_init_vst_plugin (void* plugin_dll,
					  const char* plugin_dll_filename,
					  audioMasterCallback am);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WIN32 */

#endif /* __VSTUTILS_H__ */
/* EOF */
