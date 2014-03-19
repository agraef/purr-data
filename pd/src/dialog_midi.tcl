package provide dialog_midi 0.1

namespace eval ::dialog_midi:: {
#    namespace export pdtk_midi_dialog
#    namespace export pdtk_alsa_midi_dialog
}

# TODO this panel really needs some reworking, it works but the code is
# very unreadable


####################### midi dialog ##################

proc ::dialog_midi::apply {mytoplevel} {
    global midi_indev1 midi_indev2 midi_indev3 midi_indev4 
    global midi_outdev1 midi_outdev2 midi_outdev3 midi_outdev4
    global midi_alsain midi_alsaout
    
    pdsend "pd midi-dialog \
        $midi_indev1 \
        $midi_indev2 \
        $midi_indev3 \
        $midi_indev4 \
        $midi_outdev1 \
        $midi_outdev2 \
        $midi_outdev3 \
        $midi_outdev4 \
        $midi_alsain \
        $midi_alsaout"
}

proc ::dialog_midi::cancel {mytoplevel} {
    pdsend "$mytoplevel cancel"
}

proc ::dialog_midi::ok {mytoplevel} {
    ::dialog_midi::apply $mytoplevel
    ::dialog_midi::cancel $mytoplevel
}

proc ::dialog_midi::setapi {var - op} {
    if {$op ne "write"} {return}
    set name [set $var]
    set index [lsearch -exact -index 0 $::pd_midiapilist $name]
    set ::pd_whichmidiapi [lindex $::pd_midiapilist $index 1]
    pdsend "pd midi-setapi $::pd_whichmidiapi"
}

proc ::dialog_midi::setlongform {widget} {
    set state [set ::$widget]
    if {$state == 0} {
        # back to single devs
        set extra_devs 0
        foreach type {in out} {
            foreach i {2 3 4} {
                if { [set ::midi_${type}chan$i] > 0 &&
                     [set ::midi_${type}enable$i] > 0 } {
                          incr extra_devs
                }
            }
        }
        if {$extra_devs} {
            set devices devices
            if {$extra_devs == 1} {set devices device}
            set continue [tk_messageBox -type yesno -message \
                [_ "This will disconnect $extra_devs $devices. Continue?"] \
                 -default "no" -parent [winfo parent $widget] -icon question]
            if {$continue eq "yes"} {
               foreach type {in out} {
                   foreach i {2 3 4} {
                       set ::midi_${type}chan$i 0
                       set ::midi_${type}enable$i 0
                   }
               }
            ::dialog_midi::apply [winfo parent $widget]
            }
        }
    }
    pdsend "pd midi-properties $state"
}

proc ::dialog_midi::create_api_frame {mytoplevel apifr midi_indevlist \
    midi_outdevlist longform} {
    if {![winfo exists $apifr]} {

        # MIDI API
        ttk::labelframe $mytoplevel.api -text [_ "Midi API"] \
            -style Prefs.TLabelframe
        pack $apifr -side top -padx 1 -pady 1 -fill x
        if {$::pd_midiapilist eq ""} {
            ttk::label $apifr.label -text "System Midi"
            grid $apifr.label -sticky e -column 0 -row 0 -padx 3 -pady 10
        } else {
            set api_names {}
            foreach api $::pd_midiapilist {lappend api_names [lindex $api 0]}
            set api_i [lsearch -exact -index 1 $::pd_midiapilist \
                $::pd_whichmidiapi]
            set ::midi_apiname [lindex $::pd_midiapilist $api_i 0]
            ::dialog_prefs::dropdown $apifr.apilist ::midi_apiname $api_names
            trace add variable ::midi_apiname write ::dialog_midi::setapi
            grid $apifr.apilist -sticky e -column 0 -row 0 -padx 3 -pady 10
        }
        ttk::checkbutton $apifr.longbutton -text "Use multiple devices" \
            -command "::dialog_midi::setlongform $apifr.longbutton"
        grid $apifr.longbutton -sticky w -column 1 -row 0 -padx 3 -pady 10
        grid columnconfigure $apifr {0 1} -weight 1
    }
    # disable longbutton if hardware doesn't support multi devices
    set state normal
    if {![expr [llength $midi_indevlist] > 1 && \
        [llength $midi_outdevlist] > 1]} {
            set state disabled
    }
    $apifr.longbutton configure -state disabled
}

# start a dialog window to select midi devices.  "longform" asks us to make
# controls for opening several devices; if not, we get an extra button to
# turn longform on and restart the dialog.
proc ::dialog_midi::pdtk_midi_dialog {id indev1 indev2 indev3 indev4 \
        outdev1 outdev2 outdev3 outdev4 longform} {
    global midi_indev1 midi_indev2 midi_indev3 midi_indev4
    global midi_outdev1 midi_outdev2 midi_outdev3 midi_outdev4
    global midi_indevlist midi_outdevlist
    global midi_alsain midi_alsaout
    global midi_longform
    set midi_indev1 $indev1
    set midi_indev2 $indev2
    set midi_indev3 $indev3
    set midi_indev4 $indev4
    set midi_outdev1 $outdev1
    set midi_outdev2 $outdev2
    set midi_outdev3 $outdev3
    set midi_outdev4 $outdev4
    set midi_alsain [llength $midi_indevlist]
    set midi_alsaout [llength $midi_outdevlist]
    set midi_longform $longform
    set mytoplevel .prefs.nb.midi
    set apifr $mytoplevel.api
    # not sure why it's ...midi.midi... should probably
    # fix that...
    ::dialog_midi::create_api_frame $mytoplevel $apifr $midi_indevlist \
        $midi_outdevlist $longform
    destroy $mytoplevel.midi
    ttk::frame $mytoplevel.midi
    pack $mytoplevel.midi -side top -fill x

    # todo: put padding with style settings in dialog_prefs.tcl
    set padx 1

    # Devices 
    set devfr [ttk::labelframe $mytoplevel.midi.devs -text [_ "Devices"] \
        -style Prefs.TLabelframe -padding 5]
    pack $devfr -side top -fill x -padx 3 -pady 10

    set j 2
    # todo: change in out to input output and make translatable strings
    foreach {type name} [list in [_ "Input"] out [_ "Output"]] {
        set domulti [expr $longform && \
            [llength [set "midi_${type}devlist"]] > 1]
        if {$domulti} {
            ttk::label $devfr.$type \
                -text [concat $name [_ "Devices"]]
            ttk::label $devfr.${type}ch -text [_ "Channels"]
            grid $devfr.$type -row $j -column 1 -padx $padx
            grid $devfr.${type}ch -row $j -column 3 -padx $padx
            incr j
        } else {
            if {$type eq "in"} {
            ttk::label $devfr.$type \
                -text [_ "Device Name"]
            ttk::label $devfr.${type}ch -text [_ "Channels"]
            grid $devfr.$type -row $j -column 1 -columnspan 2 -padx $padx
            grid $devfr.${type}ch -row $j -column 3 -padx $padx
            incr j
            }
        }
        # Note: it'd be fairly easy to change the GUI to accommodate
        # more than four devices, but Pd only takes and receives at most
        # four devices, so the entire backend would have to change in order
        # to do that
        for {set i 0} {$i < 4} {incr i} {
            set devno [expr $i + 1]
            set row "$devfr.${type}$devno"
            if {$domulti} {
                set ctext "$devno."
            } else {
                set ctext $name
            }
            ttk::label ${row}x0 -text $ctext -anchor w
            set ::midi_${type}dev${devno}label {}
            ::dialog_prefs::dropdown_by_index ${row}x1 \
                "::midi_${type}dev$devno" \
                [set "midi_${type}devlist"] \
                "::midi_${type}dev${devno}label"
            ttk::entry ${row}x2 -textvariable "midi_${type}chan$devno" -width 4
            grid ${row}x0 -row $j -column 0 -sticky e -padx $padx
            grid ${row}x1 -row $j -column 1 -columnspan 2 -sticky ew -padx $padx
            grid ${row}x2 -row $j -column 3 -padx $padx
            grid columnconfigure $mytoplevel.midi.devs {1 2 3} -weight 2
            grid columnconfigure $mytoplevel.midi.devs 0 -weight 1
            incr j
            if {![expr $longform && \
                [llength [set "midi_${type}devlist"]] > 1]} {
                break
            }
        }
    }
    grid rowconfigure $devfr all -pad 3

    # Connect button
    ttk::frame $mytoplevel.midi.buttonframe
    pack $mytoplevel.midi.buttonframe
    pack $mytoplevel.midi.buttonframe -side bottom
    ttk::button $mytoplevel.midi.buttonframe.apply \
        -text [_ "Apply MIDI Settings"] \
        -command "::dialog_midi::apply $mytoplevel"
    pack $mytoplevel.midi.buttonframe.apply -side left -expand 1 -fill x \
        -padx 15
}

proc ::dialog_midi::pdtk_alsa_midi_dialog {id indev1 indev2 indev3 indev4 \
        outdev1 outdev2 outdev3 outdev4 longform alsa} {
    global midi_indev1 midi_indev2 midi_indev3 midi_indev4 
    global midi_outdev1 midi_outdev2 midi_outdev3 midi_outdev4
    global midi_indevlist midi_outdevlist
    global midi_alsain midi_alsaout
    set midi_indev1 $indev1
    set midi_indev2 $indev2
    set midi_indev3 $indev3
    set midi_indev4 $indev4
    set midi_outdev1 $outdev1
    set midi_outdev2 $outdev2
    set midi_outdev3 $outdev3
    set midi_outdev4 $outdev4
    set midi_alsain [llength $midi_indevlist]
    set midi_alsaout [llength $midi_outdevlist]
    set mytoplevel .prefs.nb.midi
    set apifr $mytoplevel.api
    # not sure why it's ...midi.midi... should probably
    # fix that...
    ::dialog_midi::create_api_frame $mytoplevel $apifr $midi_indevlist \
        $midi_outdevlist $longform
    destroy $mytoplevel.midi
    ttk::frame $mytoplevel.midi
    pack $mytoplevel.midi -side top -fill x

    # todo: put padding with style settings in dialog_prefs.tcl
    set padx 1

    # Devices 
    set devfr [ttk::labelframe $mytoplevel.midi.devs -text [_ "Devices"] \
        -style Prefs.TLabelframe -padding 5]
    pack $devfr -side top -fill x -padx 3 -pady 10

    if {$alsa == 0} {
        set j 2
        foreach {type name} [list in [_ "Input"] out [_ "Output"]] {
            set domulti [expr $longform && \
                [llength [set "midi_${type}devlist"]] > 1]
            if {$domulti} {
                ttk::label $devfr.$type \
                    -text [concat $name [_ "Devices"]]
                ttk::label $devfr.${type}ch -text [_ "Channels"]
                grid $devfr.$type -row $j -column 1 -padx $padx
                grid $devfr.${type}ch -row $j -column 3 -padx $padx
                incr j
            } else {
                if {$type eq "in"} {
                ttk::label $devfr.$type \
                    -text [_ "Device Name"]
                ttk::label $devfr.${type}ch -text [_ "Channels"]
                grid $devfr.$type -row $j -column 1 -columnspan 2 -padx $padx
                grid $devfr.${type}ch -row $j -column 3 -padx $padx
                incr j
                }
            }
            for {set i 0} {$i < [llength [set "midi_${type}devlist"]]} \
                {incr i} {
                set devno [expr $i + 1]
                set row "$devfr.${type}$devno"
                if {$domulti} {
                    set ctext "$devno."
                } else {
                    set ctext $name
                }
                ttk::label ${row}x0 -text $ctext -anchor w
                set ::midi_${type}dev${devno}label {}
                ::dialog_prefs::dropdown_by_index ${row}x1 \
                    "midi_${type}dev$devno" \
                    [set "midi_${type}devlist"] \
                    "::midi_${type}dev${devno}label"
                ttk::entry ${row}x2 -textvariable "midi_${type}chan$devno" -width 4
                grid ${row}x0 -row $j -column 0 -sticky e -padx $padx
                grid ${row}x1 -row $j -column 1 -columnspan 2 -sticky ew -padx $padx
                grid ${row}x2 -row $j -column 3 -padx $padx
                grid columnconfigure $mytoplevel.midi.devs {1 2 3} -weight 2
                grid columnconfigure $mytoplevel.midi.devs 0 -weight 1
                incr j
                if {![expr $longform && \
                    [llength [set "midi_${type}devlist"]] > 1]} {
                    break
                }
            }
        }
        grid rowconfigure $devfr all -pad 3
    } else {
        ttk::label $devfr.l1 -text [_ "In Ports:"]
        entry $devfr.x1 -textvariable midi_alsain -width 4
        pack $devfr.l1 $devfr.x1 -side left
        ttk::label $devfr.l2 -text [_ "Out Ports:"]
        entry $devfr.x2 -textvariable midi_alsaout -width 4
        pack $devfr.l2 $devfr.x2 -side left
    }

    # Connect button
    ttk::frame $mytoplevel.midi.buttonframe
    pack $mytoplevel.midi.buttonframe
    pack $mytoplevel.midi.buttonframe -side bottom
    ttk::button $mytoplevel.midi.buttonframe.apply -text [_ "Connect"]\
        -command "::dialog_midi::apply $mytoplevel"
    pack $mytoplevel.midi.buttonframe.apply -side left -expand 1 -fill x \
        -padx 15
}
