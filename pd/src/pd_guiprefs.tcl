#
# Copyright (c) 1997-2009 Miller Puckette.
# Copyright (c) 2011 Yvan Volochine.
#(c) 2008 WordTech Communications LLC. License: standard Tcl license, http://www.tcl.tk/software/tcltk/license.html

package provide pd_guiprefs 0.1


namespace eval ::pd_guiprefs:: {
    namespace export init
    namespace export write_recentfiles
    namespace export update_recentfiles
}

# FIXME should these be globals ?
set ::recentfiles_key ""
set ::recentfiles_domain ""
set ::guipreset_key ""
set ::guipreset_domain ""

#################################################################
# global procedures
#################################################################
# ------------------------------------------------------------------------------
# init preferences
#
proc ::pd_guiprefs::init {} {
    switch -- $::windowingsystem {
        "aqua"  { init_aqua }
        "win32" { init_win }
        "x11"   { init_x11 }
    }
    # assign gui preferences
    # osx special case for arrays
    set arr [expr { $::windowingsystem eq "aqua" }]
    set ::recentfiles_list ""
    catch {set ::recentfiles_list [get_config $::recentfiles_domain \
        $::recentfiles_key $arr]}
    set ::gui_preset ""
    catch {
        set ::gui_preset [get_config $::guipreset_domain $::guipreset_key $arr]
        if { [lindex $::gui_preset 0] == "Custom" } {
            set ::pd_colors(atom_box) [lindex $::gui_preset 1]
            set ::pd_colors(atom_box_border) [lindex $::gui_preset 2]
            set ::pd_colors(canvas_color) [lindex $::gui_preset 3]
            set ::pd_colors(canvas_cursor) [lindex $::gui_preset 4]
            set ::pd_colors(text) [lindex $::gui_preset 5]
            set ::pd_colors(text_in_console) [lindex $::gui_preset 6]
            set ::pd_colors(box) [lindex $::gui_preset 7]
            set ::pd_colors(box_border) [lindex $::gui_preset 8]
            set ::pd_colors(msg) [lindex $::gui_preset 9]
            set ::pd_colors(msg_border) [lindex $::gui_preset 10]
            set ::pd_colors(iemgui_border) [lindex $::gui_preset 11]
            set ::pd_colors(iemgui_nlet) [lindex $::gui_preset 12]
            set ::pd_colors(control_cord) [lindex $::gui_preset 13]
            set ::pd_colors(control_nlet) [lindex $::gui_preset 14]
            set ::pd_colors(signal_cord) [lindex $::gui_preset 15]
            set ::pd_colors(signal_nlet) [lindex $::gui_preset 16]
            set ::pd_colors(xlet_hover) [lindex $::gui_preset 17]
            set ::pd_colors(link) [lindex $::gui_preset 18]
            set ::pd_colors(selection) [lindex $::gui_preset 19]
            set ::pd_colors(selection_rectangle) [lindex $::gui_preset 20]
            set ::pd_colors(highlighted_text) [lindex $::gui_preset 21]
            set ::pd_colors(highlighted_text_bg) [lindex $::gui_preset 22]
            set ::pd_colors(dash_outline) [lindex $::gui_preset 23]
            set ::pd_colors(dash_fill) [lindex $::gui_preset 24]
            set ::pd_colors(graph_border) [lindex $::gui_preset 25]
            set ::pd_colors(graph) [lindex $::gui_preset 26]
            set ::pd_colors(magic_glass_bg) [lindex $::gui_preset 27]
            set ::pd_colors(magic_glass_bd) [lindex $::gui_preset 28]
            set ::pd_colors(magic_glass_text) [lindex $::gui_preset 29]
            set ::pd_colors(magic_glass_flash) [lindex $::gui_preset 30]
            set ::gui_preset [lindex $::gui_preset 0]
        }

    }
}

proc ::pd_guiprefs::init_aqua {} {
    # osx has a "Open Recent" menu with 10 recent files (others have 5 inlined)
    set ::recentfiles_domain org.puredata
    set ::recentfiles_key "NSRecentDocuments"
    set ::total_recentfiles 10
    set ::guipreset_domain org.puredata
    set ::guipreset_key "GuiPreset"
}

proc ::pd_guiprefs::init_win {} {
    # windows uses registry
    set ::recentfiles_domain "HKEY_CURRENT_USER\\Software\\Pd-L2Ork"
    set ::recentfiles_key "RecentDocs"
    set ::guipreset_domain "HKEY_CURRENT_USER\\Software\\Pd-L2Ork"
    set ::guipreset_key "GuiPreset"

}

proc ::pd_guiprefs::init_x11 {} {
    # linux uses ~/.config/pure-data dir
    set ::recentfiles_domain "~/.pd-l2ork"
    set ::recentfiles_key "recent_files"
    set ::guipreset_domain "~/.pd-l2ork"
    set ::guipreset_key "gui_theme"
    prepare_configdir
}

# ------------------------------------------------------------------------------
# write recent files
#
proc ::pd_guiprefs::write_recentfiles {} {
    write_config $::recentfiles_list $::recentfiles_domain $::recentfiles_key true
}

proc ::pd_guiprefs::write_guipreset {} {
    set output $::gui_preset
    if { $::gui_preset == "Custom" } {
        lappend output $::pd_colors(atom_box)
        lappend output $::pd_colors(atom_box_border)
        lappend output $::pd_colors(canvas_color)
        lappend output $::pd_colors(canvas_cursor)
        lappend output $::pd_colors(text)
        lappend output $::pd_colors(text_in_console)
        lappend output $::pd_colors(box)
        lappend output $::pd_colors(box_border)
        lappend output $::pd_colors(msg)
        lappend output $::pd_colors(msg_border)
        lappend output $::pd_colors(iemgui_border)
        lappend output $::pd_colors(iemgui_nlet)
        lappend output $::pd_colors(control_cord)
        lappend output $::pd_colors(control_nlet)
        lappend output $::pd_colors(signal_cord)
        lappend output $::pd_colors(signal_nlet)
        lappend output $::pd_colors(xlet_hover)
        lappend output $::pd_colors(link)
        lappend output $::pd_colors(selection)
        lappend output $::pd_colors(selection_rectangle)
        lappend output $::pd_colors(highlighted_text)
        lappend output $::pd_colors(highlighted_text_bg)
        lappend output $::pd_colors(dash_outline)
        lappend output $::pd_colors(dash_fill)
        lappend output $::pd_colors(graph_border)
        lappend output $::pd_colors(graph)
        lappend output $::pd_colors(magic_glass_bg)
        lappend output $::pd_colors(magic_glass_bd)
        lappend output $::pd_colors(magic_glass_text)
        lappend output $::pd_colors(magic_glass_flash)
    }
    write_config $output $::guipreset_domain $::guipreset_key true
}

# ------------------------------------------------------------------------------
# this is called when opening a document (wheredoesthisshouldgo.tcl)
#
proc ::pd_guiprefs::update_recentfiles {afile save} {
    # remove duplicates first
    set index [lsearch -exact $::recentfiles_list $afile]
    set ::recentfiles_list [lreplace $::recentfiles_list $index $index]
    #puts stderr "afile=$afile save=$save"
    # insert new one in the beginning and crop the list
    set ::recentfiles_list [linsert $::recentfiles_list 0 $afile]
    set ::recentfiles_list [lrange $::recentfiles_list 0 [expr $::total_recentfiles - 1]]
    #::pd_menus::update_recentfiles_menu .mbar.file $save
    ::pd_guiprefs::write_recentfiles
}

proc ::pd_guiprefs::update_guipreset {preset} {
    set ::gui_preset $preset
    ::pd_guiprefs::write_guipreset
}

#################################################################
# main read/write procedures
#################################################################

# ------------------------------------------------------------------------------
# get configs from a file or the registry
#
proc ::pd_guiprefs::get_config {adomain {akey} {arr}} {
    switch -- $::windowingsystem {
        "aqua"  { set conf [get_config_aqua $adomain $akey $arr] }
        "win32" { set conf [get_config_win $adomain $akey $arr] }
        "x11"   { set conf [get_config_x11 $adomain $akey $arr] }
    }
    return $conf
}

# ------------------------------------------------------------------------------
# write configs to a file or to the registry
# $arr is true if the data needs to be written in an array
#
proc ::pd_guiprefs::write_config {data {adomain} {akey} {arr false}} {
    switch -- $::windowingsystem {
        "aqua"  { write_config_aqua $data $adomain $akey $arr }
        "win32" { write_config_win $data $adomain $akey $arr }
        "x11"   { write_config_x11 $data $adomain $akey }
    }
}

#################################################################
# os specific procedures
#################################################################

# ------------------------------------------------------------------------------
# osx: read a plist file
#
proc ::pd_guiprefs::get_config_aqua {adomain {akey} {arr false}} {
    if {![catch {exec defaults read $adomain $akey} conf]} {
        if {$arr} {
            set conf [plist_array_to_tcl_list $conf]
        }
    } else {
        # initialize NSRecentDocuments with an empty array
        exec defaults write $adomain $akey -array
        set conf {}
    }
    return $conf
}

# ------------------------------------------------------------------------------
# win: read in the registry
#
proc ::pd_guiprefs::get_config_win {adomain {akey} {arr false}} {
    package require registry
    if {![catch {registry get $adomain $akey} conf]} {
        return [expr {$conf}]
    } else {
        return {}
    }
}

# ------------------------------------------------------------------------------
# linux: read a config file and return its lines split into individual entries
#
proc ::pd_guiprefs::get_config_x11 {adomain {akey} {arr false}} {
    set filename [file join $adomain $akey]
    set conf {}
    if {
        [file exists $filename] == 1
        && [file readable $filename]
    } {
        set fl [open $filename r]
        while {[gets $fl line] >= 0} {
           lappend conf $line
        }
        close $fl
    }
    return $conf
}

# ------------------------------------------------------------------------------
# osx: write configs to plist file
# if $arr is true, we write an array
#
proc ::pd_guiprefs::write_config_aqua {data {adomain} {akey} {arr false}} {
    # FIXME empty and write again so we don't loose the order
    if {[catch {exec defaults write $adomain $akey -array} errorMsg]} {
        ::pdwindow::error "write_config_aqua $akey: $errorMsg"
    }
    if {$arr} {
        foreach filepath $data {
            set escaped [escape_for_plist $filepath]
            exec defaults write $adomain $akey -array-add "$escaped"
        }
    } else {
        set escaped [escape_for_plist $data]
        exec defaults write $adomain $akey '$escaped'
    }
}

# ------------------------------------------------------------------------------
# win: write configs to registry
# if $arr is true, we write an array
#
proc ::pd_guiprefs::write_config_win {data {adomain} {akey} {arr false}} {
    package require registry
    # FIXME: ugly
    if {$arr} {
        if {[catch {registry set $adomain $akey $data multi_sz} errorMsg]} {
            ::pdwindow::error "write_config_win $data $akey: $errorMsg"
        }
    } else {
        if {[catch {registry set $adomain $akey $data sz} errorMsg]} {
            ::pdwindow::error "write_config_win $data $akey: $errorMsg"
        }
    }
}

# ------------------------------------------------------------------------------
# linux: write configs to USER_APP_CONFIG_DIR
#
proc ::pd_guiprefs::write_config_x11 {data {adomain} {akey}} {
    # right now I (yvan) assume that data are just \n separated, i.e. no keys
    set data [join $data "\n"]
    set filename [file join $adomain $akey]
    if {[catch {set fl [open $filename w]} errorMsg]} {
        ::pdwindow::error "write_config_x11 $data $akey: $errorMsg"
    } else {
        puts -nonewline $fl $data
        close $fl
    }
}

#################################################################
# utils
#################################################################

# ------------------------------------------------------------------------------
# linux only! : look for pd config directory and create it if needed
#
proc ::pd_guiprefs::prepare_configdir {} {
    if {[file isdirectory $::recentfiles_domain] != 1} {
        file mkdir $::recentfiles_domain
        #::pdwindow::debug "Created $::recentfiles_domain preferences folder.\n"
    }
}

# ------------------------------------------------------------------------------
# osx: handles arrays in plist files (thanks hc)
#
proc ::pd_guiprefs::plist_array_to_tcl_list {arr} {
    set result {}
    set filelist $arr
    regsub -all -- {("?),\s+("?)} $filelist {\1 \2} filelist
    regsub -all -- {\n} $filelist {} filelist
    regsub -all -- {^\(} $filelist {} filelist
    regsub -all -- {\)$} $filelist {} filelist
    regsub -line -- {^'(.*)'$} $filelist {\1} filelist

    foreach file $filelist {
        set filename [regsub -- {,$} $file {}]
        lappend result $filename
    }
    return $result
}

# the Mac OS X 'defaults' command uses single quotes to quote things,
# so they need to be escaped
proc ::pd_guiprefs::escape_for_plist {str} {
    return [regsub -all -- {'} $str {\\'}]
}
