/* 
clk - syncable clocking objects

Copyright (c)2006-2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

$LastChangedRevision: 1311 $
$LastChangedDate: 2008-01-04 11:24:49 -0500 (Fri, 04 Jan 2008) $
$LastChangedBy: thomas $
*/

#include "clk.h"

#define CLK_VERSION "0.2.1"

namespace clk {

static void clk_main()
{
	flext::post("----------------------------------------");
	flext::post("clk - syncable clocking objects");
    flext::post("version " CLK_VERSION " (c)2006-2008 Thomas Grill");
#ifdef FLEXT_DEBUG
    flext::post("");
    flext::post("DEBUG BUILD - " __DATE__ " " __TIME__);
#endif
	flext::post("----------------------------------------");

	// call the objects' setup routines
	FLEXT_SETUP(Internal);
	FLEXT_SETUP(Sync);
	FLEXT_SETUP(Tap);

	FLEXT_SETUP(Time);
	FLEXT_SETUP(Metro);
	FLEXT_SETUP(Delay);
}

// setup the library
FLEXT_LIB_SETUP(clk,clk_main)

} // namespace
