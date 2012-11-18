/* 

flext - C++ layer for Max/MSP and pd (pure data) externals

Copyright (c) 2001-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision$
$LastChangedDate$
$LastChangedBy$
*/

/*! \file flfeatures.h
    \brief Detect version-specific features.
*/
 
#ifndef __FLFEATURES_H
#define __FLFEATURES_H

// check if PD API supports buffer dirty time
#if defined(PD_DEVEL_VERSION) && defined(PD_MAJOR_VERSION) && defined(PD_MINOR_VERSION)
#if PD_MINOR_VERSION >= 36 && PD_MINOR_VERSION <= 38
// array locks have been removed in devel_0_39
	#define _FLEXT_HAVE_PD_GARRAYLOCKS
#endif
#if PD_MINOR_VERSION >= 36
    #define _FLEXT_HAVE_PD_GARRAYUPDATETIME
#endif
#endif

#if defined(MAC_VERSION) || defined(WIN_VERSION) 
    // not for OS9
    #define _FLEXT_HAVE_MAX_INUSEFLAG
#endif

#endif
