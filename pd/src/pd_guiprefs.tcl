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
    catch {set ::gui_preset [get_config $::guipreset_domain \
        $::guipreset_key $arr]}
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
    set ::recentfiles_key "recent.files"
    set ::guipreset_domain "~/.pd-l2ork"
    set ::guipreset_key "gui.theme"
    prepare_configdir
}

# ------------------------------------------------------------------------------
# write recent files
#
proc ::pd_guiprefs::write_recentfiles {} {
    write_config $::recentfiles_list $::recentfiles_domain $::recentfiles_key true
}

proc ::pd_guiprefs::write_guipreset {} {
    write_config $::gui_preset $::guipreset_domain $::guipreset_key true
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
