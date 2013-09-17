# pkgIndex.tcl.  Generated from pkgIndex.tcl.in by configure.
#

package ifneeded helpbrowser 0.1 [list source [file join $dir helpbrowser.tcl]]
package ifneeded pd_guiprefs 0.1 [list source [file join $dir pd_guiprefs.tcl]]
package ifneeded pd_menus 0.1 [list source [file join $dir pd_menus_SHORT.tcl]]

namespace eval ::tkpath {
    proc load_package {dir} {
	load [file join $dir libtkpath0.3.3.so]
	# Allow optional redirect of library components.
	# Only necessary for testing, but could be used elsewhere.
	if {[info exists ::env(TKPATH_LIBRARY)]} {
	    set dir $::env(TKPATH_LIBRARY)
	}
	source $dir/tkpath.tcl
    };# load_package
}

package ifneeded tkpath 0.3.2 [list ::tkpath::load_package $dir]

package ifneeded tkdnd 2.6 \
  "source \{$dir/tkdnd.tcl\} ; \
   tkdnd::initialise \{$dir\} libtkdnd2.6.so tkdnd"

#*EOF*
