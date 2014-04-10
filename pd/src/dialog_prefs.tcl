# the find dialog panel is a bit unusual in that it is created directly by the
# Tcl 'pd-gui'. Most dialog panels are created by sending a message to 'pd',
# which then sends a message to 'pd-gui' to create the panel.

package provide dialog_prefs 0.1

#package require pd_bindings
#package require dialog_gui

namespace eval ::dialog_prefs:: {
#    namespace export pdtk_
}

# mytoplevel isn't used here, but is kept for compatibility with other dialog ok procs
proc ::dialog_prefs::ok {mytoplevel} {
    return # hash this out later
    variable find_history

    if {$findstring eq ""} {
        if {$::windowingsystem eq "aqua"} {bell}
        return
    }
    if {$find_in_window eq ".pdwindow"} {
        if {$::tcl_version < 8.5} {
            # TODO implement in 8.4 style, without -all
            set matches [.pdwindow.text search -nocase -- $findstring 0.0]
        } else {
            set matches [.pdwindow.text search -all -nocase -- $findstring 0.0]
        }
        .pdwindow.text tag delete sel
        if {[llength $matches] > 0} {
            foreach match $matches {
                .pdwindow.text tag add sel $match "$match wordend"
            }
            .pdwindow.text see [lindex $matches 0]
            lappend find_history $findstring
        }
    } else {
        if {$findstring eq $previous_findstring \
                && $wholeword_button == $previous_wholeword_button} {
            pdsend "$find_in_window findagain"
        } else {
            pdsend [concat $find_in_window find [pdtk_encodedialog $findstring] \
                        $wholeword_button]
            set previous_findstring $findstring
            set previous_wholeword_button $wholeword_button
            lappend find_history $findstring
        }
    }
    if {$::windowingsystem eq "aqua"} {
        # (Mac OS X) hide panel after success, but keep it if unsuccessful by
        # having the couldnotfind proc reopen it
        cancel $mytoplevel
    } else {
        # (GNOME/Windows) find panel should retain focus after a find
        # (yes, a bit of a kludge)
        after 100 "raise .find; focus .find.entry"
    }
}

# mytoplevel isn't used here, but is kept for compatibility with other dialog cancel procs
proc ::dialog_prefs::cancel {mytoplevel} {
    wm withdraw .prefs
}

# the find panel is opened from the menu and key bindings
proc ::dialog_prefs::open_prefs_dialog {mytoplevel} {
    if {[winfo exists .prefs]} {
        wm deiconify .prefs
        raise .prefs
        # obtain last known mouse coords and pop the menu there
        global pointer_x_global pointer_y_global
        wm geometry .prefs "+$pointer_x_global+$pointer_y_global"
    } else {
        create_dialog $mytoplevel
    }
    pd [concat pd audio-properties \;]
    pd [concat pd midi-properties \;]
    ::dialog_gui::create_gui_dialog .prefs.nb.gui
#    need to think about what should get focus with ttk::notebook
#    .prefs.entry selection range 0 end
}

proc ::dialog_prefs::dropdown_set {name value} {
    if {$::windowingsystem eq "aqua"} {
        set my_var [$name cget -textvariable]
        set $my_var $value
    } else {
        $name set "$value"
    }
}

proc ::dialog_prefs::dropdown_by_index {name variable values textvar} {
    ::dialog_prefs::dropdown $name $variable $values $textvar
}

proc ::dialog_prefs::dropdown {name variable values args} {
    set textvar $variable
    if {$args ne ""} {set textvar $args}
    if {$::windowingsystem eq "aqua"} {
        ttk::menubutton $name -menu $name.menu -direction flush \
            -textvariable $textvar -cursor boat
        menu $name.menu -cursor boat
        set i 0
        foreach label $values {
            set value $label
            if {$args ne ""} {set value $i}
            $name.menu add radiobutton -value $value -variable $variable \
                -label $label -command "set $textvar \"$label\""
            incr i
        }
    } else {
        # set combobox width to largest string
        set width 0
        foreach value $values {
            set len [string length $value]
            set width [expr $len > $width ? $len : $width]
        }
        ttk::combobox $name -state readonly -width $width \
            -style Prefs.TCombobox -values $values
        set command "get"
        if {$args ne ""} {
            set command "current"
            # the following cmd prevents a bug in the alsa midi api
            # from throwing an index that's out of range. but once
            # that bug gets fixed, this should be removed so that
            # it doesn't hide any future bugs
            set $variable [expr min([set $variable], [llength $values] - 1)]
        }
        bind $name <<ComboboxSelected>> "set $variable \
            [concat {[} $name $command {]} ]; after 0 {%W selection clear}"
        if {$command eq "get"} {set command "set"}
        $name $command [set $variable]
    }
}

proc ::dialog_prefs::set_color {array key op} {
    # not sure if this is necessary, but just in case...
    if {$op ne "write"} {return}
    set c [set ${array}($key)]
    set commands {}
    switch $key {
        box {set commands [list "itemconfigure \
            box&&(!msg)&&(!atom) -fill $c"] }
        text { set commands [list "itemconfigure \
                   (text&&(!box&&!iemgui)) -fill $c"]
               lappend commands "itemconfigure \
                   label&&graph -fill $c"
              # lappend commands "itemconfigure \
              #     (text&&(!label) -fill $c"
             if {[winfo exists .search.resultstext]} {
                 .search.resultstext configure -foreground $c
                 .search.navtext configure -foreground $c
             }
             if {[winfo exists .printout.frame.text]} {
                 .printout.frame.text configure -foreground $c
             }
        }
        canvas_color {set commands [list "configure -bg $c"]
            if {[winfo exists .search.resultstext]} {
                .search.resultstext configure -bg $c
                .search.navtext configure -bg $c
            }
            if {[winfo exists .printout.frame.text]} {
                .printout.frame.text configure -bg $c
            }
        }
        canvas_cursor {set commands [list "configure -insertbackground $c"]}
        highlighted_text_bg \
            {set commands [list "configure -selectbackground $c"]}
        highlighted_text {set commands [list "configure -selectforeground $c"]}
        msg_border {set commands [list "itemconfigure \
            msg&&box -stroke $c"]}
        msg {set commands [list "itemconfigure \
            msg&&box -fill $c"]}
        control_nlet {set commands [list "itemconfigure \
            ((inlet||outlet)&&control) -fill $c"]
        }
        iemgui_nlet {set commands [list "itemconfigure \
                (inlet&&iemgui)||(outlet&&iemgui) -stroke $c"]
        }
        signal_nlet {set commands [list "itemconfigure \
            inlet&&signal -fill $c"]
            lappend commands "itemconfigure \
                outlet&&signal -fill $c"
        }
        outlet  {set commands [list "itemconfigure outlet -outline $c"]}
        signal_cord {set commands [list "itemconfigure \
            all_cords&&signal -stroke $c"]
            lappend commands "itemconfigure \
            (outlet&&signal)||(inlet&&signal) -stroke $c"
        }
        control_cord {set commands [list "itemconfigure \
            all_cords&&control -stroke $c"]}
        selection {
            set commands [list "itemconfigure \
                selected&&text -fill $c"]
            lappend commands "itemconfigure \
                selected&&(border&&(!iemgui)) -fill $c"
            lappend commands "itemconfigure \
                selected&&border&&iemgui -outline $c"
        }
        box_border {set commands [list "itemconfigure \
            (box)&&(!iemgui) -stroke $c"]}
        iemgui_border {
            set commands [list "itemconfigure border&&iemgui -stroke $c"]}
        atom_box {set commands [list "itemconfigure \
            atom&&box -fill $c"]}
        atom_box_border {set commands [list "itemconfigure \
            atom&&box -stroke $c"]}
        graph_border {set commands [list "itemconfigure \
            graph&&(!label) -stroke $c"]}
        graph {set commands [list "itemconfigure graph&&(!label) -fill $c"]}
        dash_fill {set commands [list "itemconfigure broken&&box -fill $c"]}
        dash_outline {set commands [list "itemconfigure \
            broken&&box -stroke $c"]}
        magic_glass_bg {set commands [list "itemconfigure \
            magicGlassBg -fill $c"]}
        magic_glass_bd {set commands [list "itemconfigure \
            magicGlassLine -fill $c"]}
        magic_glass_text {set commands [list "itemconfigure \
            magicGlassText -fill $c"]}
        link {
            if {[winfo exists .search.resultstext]} {
                .search.resultstext tag configure link -foreground $c
                .search.navtext tag configure link -foreground $c
                .search.f.advancedlabel configure -foreground $c
            }
            return
        }
        default {}
    }
    if {$commands ne ""} {
        foreach w [winfo children .] {
            foreach child [winfo children $w] {
                if {$child ne "" && [winfo class $child] eq "PathCanvas"} {
                    if {$key eq "canvas_color" &&
                        [info exists [format ::%s(%s) $key $w]]} {
                            continue
                    }
                    foreach command $commands {
                        eval $child $command
                    }
                }
            }
        }
    }
    # hack! how do I avoid hard-coding the window names here?
    set mytoplevel .prefs.nb.gui.colors
    if {[winfo exists $mytoplevel.$key]} {
        ::dialog_prefs::set_swatchbutton $mytoplevel.$key ${array}($key)
    }
    
}

proc ::dialog_prefs::set_swatchbutton {name variable} {
    if {[set $variable] eq ""} {return}
    image create photo ::img::swatchbutton::$name
    set c [set $variable]
    set bd #000000
    set stupid_top_and_bottom \
        [list $bd $bd $bd $bd $bd $bd $bd $bd $bd $bd $bd $bd]
    set dumb \
        [list $bd $c $c $c $c $c $c $c $c $c $c $bd]
    ::img::swatchbutton::$name put [list $stupid_top_and_bottom \
        $dumb $dumb $dumb $dumb $dumb $dumb $dumb $dumb \
        $dumb $dumb $stupid_top_and_bottom] -to 0 0 12 12
    $name configure -image ::img::swatchbutton::$name
}

proc ::dialog_prefs::swatchmenu_nav {w dir} {
    set new [expr {[$w index active] + 7 * $dir}]
    if {$new > [$w index end] || $new < 0} then return
    $w activate $new
}

proc ::dialog_prefs::swatchbutton_colorchooser {name variable} {
    set col [tk_chooseColor -parent $name]
    if {$col ne ""} {
        set $variable $col
    }
}

proc ::dialog_prefs::swatchbutton {name variable} {
    ttk::button $name \
        -command "::dialog_prefs::swatchbutton_colorchooser $name $variable"
}

# These are images used to build the menu for choosing
# colors. The images hang around in memory until you exit
# Pd, but they shouldn't take up too much space to matter
# (and this only gets called once).  If there's a simpler
# way to build a _straightforward_ _user-friendly_
# colorchooser that would be nice, but I couldn't figure
# one out.
proc ::dialog_prefs::get_colorswatches {} {
    # stolen from the Firefox colorchooser
    set colors {  \
        #ffffff #cfcccc #c0c0c0 #999999 #666666 #333333 #000000 \
        #ffcccc #ff6666 #ff0000 #cc0000 #990000 #660000 #330000 \
        #ffcc99 #ff9966 #ff9900 #ff6600 #cc6600 #993300 #663300 \
        #ffff99 #ffff66 #ffcc66 #ffcc33 #cc9933 #996633 #663333 \
        #ffffcc #ffff33 #ffff00 #ffcc00 #999900 #666600 #333300 \
        #99ff99 #66ff99 #33ff33 #33cc00 #009900 #006600 #003300 \
        #99ffff #33ffff #66cccc #00cccc #339999 #336666 #003333 \
        #ccffff #66ffff #33ccff #3366ff #3333ff #000099 #000066 \
        #ccccff #9999ff #6666cc #6633ff #6600cc #333399 #330099 \
        #ffccff #ff99ff #cc66cc #cc33cc #993399 #663366 #330033 \
    }
    if {[lsearch [image names] ::img::colorswatches::*] == -1} {
        foreach color $colors {
        image create photo ::img::colorswatches::$color
        ::img::colorswatches::$color put $color -to 0 0 16 16
        }
    }
    return $colors
}

proc ::dialog_prefs::help {notebook} {
    set pane [$notebook select]
    regsub {.*\.(.*)} $pane {\1} topic
    set file all_about_${topic}_settings.pd
    set dir [file join $::sys_libdir doc 5.reference]
    menu_doc_open $dir $file
}

proc ::dialog_prefs::dialog_bindings {mytoplevel dialogname} {
    variable modifier

    bind $mytoplevel <KeyPress-Escape> "dialog_${dialogname}::cancel $mytoplevel"
    bind $mytoplevel <KeyPress-Return> "dialog_${dialogname}::ok $mytoplevel"
    bind $mytoplevel <$::modifier-Key-w> "dialog_${dialogname}::cancel $mytoplevel"
    # these aren't supported in the dialog, so alert the user, then break so
    # that no other key bindings are run
    bind $mytoplevel <$::modifier-Key-s>       {bell; break}
    bind $mytoplevel <$::modifier-Shift-Key-S> {bell; break}
    bind $mytoplevel <$::modifier-Key-p>       {bell; break}

    wm protocol $mytoplevel WM_DELETE_WINDOW "dialog_${dialogname}::cancel $mytoplevel"
}

proc ::dialog_prefs::create_dialog {mytoplevel} {
    toplevel .prefs -class [winfo class .]
    wm title .prefs [_ "Pure Data Preferences"]
#    wm geometry .prefs =475x125+150+150
    wm group .prefs .
    wm resizable .prefs 0 0
    wm transient .prefs

    # obtain last known mouse coords and pop the menu there
    global pointer_x_global pointer_y_global
    if {$pointer_x_global == 0 && $pointer_y_global == 0} {
        set pointer_x_global [expr [winfo rootx .]+30]
        set pointer_y_global [expr [winfo rooty .]+30]
    }
    wm geometry .prefs "+$pointer_x_global+$pointer_y_global"

#    .prefs configure -menu $::dialog_menubar

# todo: check this on the mac and on windows
#    .prefs configure -padx 10 -pady 5
    ::dialog_prefs::dialog_bindings .prefs "prefs"
    bind .prefs <$::modifier-Key-f> break

    # Ttk style setup

    # Common settings
    ttk::style configure Prefs.TLabelframe -borderwidth 0
    # todo: don't hardcode font here
    ttk::style configure Prefs.TLabelframe.Label \
        -font "{DejaVu Sans} 9 bold"

    # for OSX swatchbutton
    if {$::windowingsystem eq "x11"} {
        # custom arrow image for ttk::combobox
        set ::prefs_arrowimg [image create photo -data \
            {R0lGODlhGQAVAMIGACEhIVBQUFpaWl1dXXBwcImJif///////yH+EUNyZWF0ZWQg
            d2l0aCBHSU1QACH5BAEKAAcALAAAAAAZABUAAAMleLrc/jDKSau9OOutC/ggUGRE
            CBCbAArcEQBBqwxybd94ru9QAgA7
        }]
        # ttk::style theme use clam
        ttk::style element create Prefs.downarrow image $::prefs_arrowimg
        ttk::style layout Prefs.TCombobox {
            Combobox.focus -sticky nsew -children {
                Combobox.field -sticky nswe -children {
                    Prefs.downarrow -side right -sticky ns
                    Combobox.padding -expand 1 -sticky nswe -children {
                        Combobox.textarea -sticky nswe
                    }
                }
            }
        }
        ttk::style layout PrefsColors.TMenubutton {
            Menubutton.border -sticky nswe -children {
                Menubutton.focus -sticky nswe -children {
                    Menubutton.padding -expand 1 -sticky we -children {
                        Menubutton.label -side left -sticky {}
                    }
                }
            }
        }
        ttk::style configure Prefs.TCombobox -padding 3
        ttk::style map Prefs.TCombobox \
            -fieldbackground {{readonly pressed} #c1c4c7 \
                              {readonly hover} #fafaf9 \
                              readonly #f5f5f4} \
            -foreground {{readonly focus} black}
        # this shouldn't be global, but I can't get it to work for just
        # Prefsdialog class
        option add *TCombobox*Listbox.selectBackground #4a90d9
        option add *TCombobox*Listbox.selectForeground white
    }

    ttk::notebook .prefs.nb
    .prefs.nb add [ttk::frame .prefs.nb.audio -padding 10] \
        -text "Audio" -sticky nsew
    .prefs.nb add [ttk::frame .prefs.nb.midi -padding 10] \
        -text "MIDI" -sticky nsew
    .prefs.nb add [ttk::frame .prefs.nb.gui -padding 10] \
        -text "GUI" -stick nsew
    pack .prefs.nb -fill both -expand 1

    ttk::frame .prefs.bottomframe -padding 10
    pack .prefs.bottomframe -side bottom -fill both  -expand 1
    if {$::windowingsystem ne "aqua"} {
    ttk::button .prefs.bottomframe.closebutton \
        -text "Close" -command "::dialog_prefs::cancel .prefs"
    pack .prefs.bottomframe.closebutton -side right
    }
    ttk::button .prefs.bottomframe.helpbutton \
        -text "Help" -command "::dialog_prefs::help .prefs.nb"
    pack .prefs.bottomframe.helpbutton -side left
}
