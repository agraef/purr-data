# Copyright (c) 1997-2009 Miller Puckette.
#(c) 2008 WordTech Communications LLC. License: standard Tcl license, http://www.tcl.tk/software/tcltk/license.html

package provide pd_menus 0.1

#package require pd_menucommands

# TODO figure out Undo/Redo/Cut/Copy/Paste state changes for menus

# since there is one menubar that is used for all windows, the menu -commands
# use {} quotes so that $::focused_window is interpreted when the menu item
# is called, not when the command is mapped to the menu item.  This is the
# opposite of the 'bind' commands in pd_bindings.tcl
    
namespace eval ::pd_menus:: {
    variable accelerator
    #variable menubar ".mbar"

    namespace export create_menubar
    namespace export configure_for_pdwindow
    namespace export configure_for_canvas
    namespace export configure_for_dialog

    # turn off tearoff menus globally
    option add *tearOff 0
} 

# ------------------------------------------------------------------------------
# update the menu entries for opening recent files (write arg should always be true except the first time when pd is opened)
proc ::pd_menus::update_recentfiles_menu {menu {write true}} {
    #variable menubar
    switch -- $::windowingsystem {
        "aqua"  {::pd_menus::update_openrecent_menu_aqua $menu $write}
        "win32" {::pd_menus::update_recentfiles_on_menu $menu $write}
        "x11"   {::pd_menus::update_recentfiles_on_menu $menu $write}
    }
}

proc ::pd_menus::clear_recentfiles_menu {} {
    set ::recentfiles_list {}
    #::pd_menus::update_recentfiles_menu
    # empty recentfiles in preferences (write empty array)
    ::pd_guiprefs::write_recentfiles
}

proc ::pd_menus::update_openrecent_menu_aqua {mymenu {write}} {
    if {! [winfo exists $mymenu]} {menu $mymenu}
    $mymenu delete 0 end

    # now the list is last first so we just add
    foreach filename $::recentfiles_list {
        $mymenu add command -label [file tail $filename] \
            -command "open_file {$filename}"
    }
    # clear button
    $mymenu add  separator
    $mymenu add command -label [_ "Clear Menu"] \
        -command "::pd_menus::clear_recentfiles_menu"
    # write to config file
    if {$write == true} { ::pd_guiprefs::write_recentfiles }
}

# this expects to be run on the File menu, and to insert above the last separator
proc ::pd_menus::update_recentfiles_on_menu {mymenu {write}} {
    set lastitem [$mymenu index end]
    set i 0
    while {[$mymenu entrycget [expr $lastitem-$i] -label] ne ""} {incr i}
    set bottom_separator [expr $lastitem-$i]
    incr i

    while {[$mymenu entrycget [expr $lastitem-$i] -label] ne ""} {incr i}
    set top_separator [expr $lastitem-$i]
    if {$top_separator < [expr $bottom_separator-1]} {
        $mymenu delete [expr $top_separator+1] [expr $bottom_separator-1]
    }
    # insert the list from the end because we insert each element on the top
    set i [llength $::recentfiles_list]
    while {[incr i -1] > 0} {

        set filename [lindex $::recentfiles_list $i]
        $mymenu insert [expr $top_separator+1] command \
            -label [file tail $filename] -command "open_file {$filename}"
    }
    set filename [lindex $::recentfiles_list 0]
    $mymenu insert [expr $top_separator+1] command \
        -label [file tail $filename] -command "open_file {$filename}"

    # write to config file
    if {$write == true} { ::pd_guiprefs::write_recentfiles }
}