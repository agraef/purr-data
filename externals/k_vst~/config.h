/* plugin~, a Pd tilde object for hosting LADSPA/VST plug-ins
   Copyright (C) 2000 Jarno Seppänen
   $Id: config.h,v 1.1 2004-01-08 14:55:24 ksvalast Exp $

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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* plug-in architecture config */

#if 0
#ifdef __linux__ /* FIXME? */
#define PLUGIN_TILDE_USE_LADSPA 1
#else
#define PLUGIN_TILDE_USE_LADSPA 0
#endif
#ifdef WIN32
#define PLUGIN_TILDE_USE_VST 1
#else
#define PLUGIN_TILDE_USE_VST 0
#endif
#endif

    /* make sure something was selected */
#if (PLUGIN_TILDE_USE_LADSPA == 0) && (PLUGIN_TILDE_USE_VST == 0)
#error Either PLUGIN_TILDE_USE_LADSPA or PLUGIN_TILDE_USE_VST must be positive
#endif

/* print debug information */
#define PLUGIN_TILDE_DEBUG 0

/* print "useful" information */
#define PLUGIN_TILDE_VERBOSE 0

/* force out-of-place processing */
#define PLUGIN_TILDE_FORCE_OUTOFPLACE 0


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CONFIG_H__ */
/* EOF */
