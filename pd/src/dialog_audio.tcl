package provide dialog_audio 0.1

namespace eval ::dialog_audio:: {
#    namespace export pdtk_audio_dialog
}

# TODO this panel really needs some reworking, it works but the code is very
# unreadable.  The panel could look a lot better too, like using menubuttons
# instead of regular buttons with tk_popup for pulldown menus.
# * make sure combobox is setting the device number
# * get focus order to do right
# * add "Close" button to prefs dialog

####################### audio dialog ##################3

proc ::dialog_audio::apply {mytoplevel} {
    global audio_indev1 audio_indev2 audio_indev3 audio_indev4 
    global audio_inchan1 audio_inchan2 audio_inchan3 audio_inchan4
    global audio_inenable1 audio_inenable2 audio_inenable3 audio_inenable4
    global audio_outdev1 audio_outdev2 audio_outdev3 audio_outdev4 
    global audio_outchan1 audio_outchan2 audio_outchan3 audio_outchan4
    global audio_outenable1 audio_outenable2 audio_outenable3 audio_outenable4
    global audio_sr audio_advance audio_callback audio_blocksize

    # Hackety hack! Rather than make this audio dialog code sane,
    # which would be a larger project, I'm just making the user interface
    # look more friendly. The global "enable" variables were used
    # for checkbuttons; I simplified the interface by removing them and
    # adding a "None" option to the device list.  This means I have
    # to parse the dev names for the string "None" and set the "enable"
    # variables accordingly. I also assume "None" is the last value in the
    # list.

    foreach type {in out} {
        foreach i {1 2 3 4} {
            if {[set audio_${type}dev${i}] == \
                [llength [set ::audio_${type}devlist]] || 
                [set audio_${type}chan${i}] <= 0} {
                set audio_${type}dev${i} 0
                set audio_${type}enable${i} 0
                set audio_${type}chan${i} 0
            } else {
                set audio_${type}enable${i} 1
            }
        }
    }

    pd [concat pd audio-dialog \
        $audio_indev1 \
        $audio_indev2 \
        $audio_indev3 \
        $audio_indev4 \
        [expr $audio_inchan1 * ( $audio_inenable1 ? 1 : -1 ) ]\
        [expr $audio_inchan2 * ( $audio_inenable2 ? 1 : -1 ) ]\
        [expr $audio_inchan3 * ( $audio_inenable3 ? 1 : -1 ) ]\
        [expr $audio_inchan4 * ( $audio_inenable4 ? 1 : -1 ) ]\
        $audio_outdev1 \
        $audio_outdev2 \
        $audio_outdev3 \
        $audio_outdev4 \
        [expr $audio_outchan1 * ( $audio_outenable1 ? 1 : -1 ) ]\
        [expr $audio_outchan2 * ( $audio_outenable2 ? 1 : -1 ) ]\
        [expr $audio_outchan3 * ( $audio_outenable3 ? 1 : -1 ) ]\
        [expr $audio_outchan4 * ( $audio_outenable4 ? 1 : -1 ) ]\
        $audio_sr \
        $audio_advance \
        $audio_callback \
        $audio_blocksize \
        \;]

    # Pd always makes devices contiguous-- for example, if you only set
    # device 1 and device 3 it will change device 3 to device 2.
    # So we look for non-contiguous devices and request an update
    # on connect so that the user doesn't see incorrect information
    # in the GUI. This rebuilds part of the dialog window which causes
    # a slight flicker-- otherwise I'd just do this everytime:
    # pdsend "pd audio-properties $::audio_longform
    foreach type {in out} {
        set empty_dev 0
        set aliased_dev 0
        foreach i {1 2 3 4} {
            set enabled [set audio_${type}enable$i]
            if {$empty_dev && $enabled} {
                set aliased_dev 1
                break
            } elseif {!$enabled} {incr empty_dev}
        }
        if {$aliased_dev} {
            pd [concat pd audio-properties $::audio_longform \;]
            break
        }
    }
    pd [concat pd save-preferences \;]
}

proc ::dialog_audio::cancel {mytoplevel} {
#    pdsend "$mytoplevel cancel"
}

proc ::dialog_audio::ok {mytoplevel} {
    ::dialog_audio::apply $mytoplevel
    ::dialog_audio::cancel $mytoplevel
}

proc ::dialog_audio::setapi {var - op} {
    if {$op ne "write"} {return}
    set name [set $var]
    set index [lsearch -exact -index 0 $::pd_apilist $name]
    set ::pd_whichapi [lindex $::pd_apilist $index 1]
    pd [concat pd audio-setapi $::pd_whichapi \;]
}

proc ::dialog_audio::setlongform {widget} {
    set state [set ::$widget]
    if {$state == 0} {
        # back to single devs
        set extra_devs 0
        foreach type {in out} {
            foreach i {2 3 4} {
                if { [set ::audio_${type}chan$i] > 0 &&
                     [set ::audio_${type}enable$i] > 0 } {
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
                       set ::audio_${type}chan$i 0
                       set ::audio_${type}enable$i 0
                   }
               }
            ::dialog_audio::apply [winfo parent $widget]
            }
        }
    }
    pd [concat pd audio-properties $state \;]
}

# start a dialog window to select audio devices and settings.  "multi"
# is 0 if only one device is allowed; 1 if one apiece may be specified for
# input and output; and 2 if we can select multiple devices.  "longform"
# (which only makes sense if "multi" is 2) asks us to make controls for
# opening several devices; if not, we get an extra button to turn longform
# on and restart the dialog.

proc ::dialog_audio::pdtk_audio_dialog {id \
        indev1 indev2 indev3 indev4 \
        inchan1 inchan2 inchan3 inchan4 \
        outdev1 outdev2 outdev3 outdev4 \
        outchan1 outchan2 outchan3 outchan4 sr advance multi callback \
        longform blocksize} {
    global audio_indev1 audio_indev2 audio_indev3 audio_indev4 
    global audio_inchan1 audio_inchan2 audio_inchan3 audio_inchan4
    global audio_inenable1 audio_inenable2 audio_inenable3 audio_inenable4
    global audio_outdev1 audio_outdev2 audio_outdev3 audio_outdev4
    global audio_outchan1 audio_outchan2 audio_outchan3 audio_outchan4
    global audio_outenable1 audio_outenable2 audio_outenable3 audio_outenable4
    global audio_sr audio_advance audio_callback audio_blocksize
    global audio_indevlist audio_outdevlist
    global pd_indev pd_outdev
    global audio_longform

    set audio_inchan1 [expr ( $inchan1 > 0 ? $inchan1 : -$inchan1 ) ]
    set audio_inenable1 [expr $inchan1 > 0 ]
    set audio_inchan2 [expr ( $inchan2 > 0 ? $inchan2 : -$inchan2 ) ]
    set audio_inenable2 [expr $inchan2 > 0 ]
    set audio_inchan3 [expr ( $inchan3 > 0 ? $inchan3 : -$inchan3 ) ]
    set audio_inenable3 [expr $inchan3 > 0 ]
    set audio_inchan4 [expr ( $inchan4 > 0 ? $inchan4 : -$inchan4 ) ]
    set audio_inenable4 [expr $inchan4 > 0 ]

    # "None" is added as the last value in the dropdown menu, so it's
    # equivalent to the length of the devlist
    set nonein [llength $audio_indevlist]
    set noneout [llength $audio_outdevlist]

    set audio_indev1 [expr ( $audio_inenable1 ? $indev1 : $nonein )]
    set audio_indev2 [expr ( $audio_inenable2 ? $indev2 : $nonein )]
    set audio_indev3 [expr ( $audio_inenable3 ? $indev3 : $nonein )]
    set audio_indev4 [expr ( $audio_inenable4 ? $indev4 : $nonein )]

    set audio_outchan1 [expr ( $outchan1 > 0 ? $outchan1 : -$outchan1 ) ]
    set audio_outenable1 [expr $outchan1 > 0 ]
    set audio_outchan2 [expr ( $outchan2 > 0 ? $outchan2 : -$outchan2 ) ]
    set audio_outenable2 [expr $outchan2 > 0 ]
    set audio_outchan3 [expr ( $outchan3 > 0 ? $outchan3 : -$outchan3 ) ]
    set audio_outenable3 [expr $outchan3 > 0 ]
    set audio_outchan4 [expr ( $outchan4 > 0 ? $outchan4 : -$outchan4 ) ]
    set audio_outenable4 [expr $outchan4 > 0 ]

    set audio_outdev1 [expr ( $audio_outenable1 ? $outdev1 : $noneout )]
    set audio_outdev2 [expr ( $audio_outenable2 ? $outdev2 : $noneout )]
    set audio_outdev3 [expr ( $audio_outenable3 ? $outdev3 : $noneout )]
    set audio_outdev4 [expr ( $audio_outenable4 ? $outdev4 : $noneout )]

    set audio_sr $sr
    set audio_advance $advance
    set audio_callback $callback
    set audio_blocksize $blocksize
    set audio_longform $longform

    set mytoplevel .prefs.nb.audio
    set apifr $mytoplevel.api
    if {![winfo exists $apifr]} {

        # Audio API
        ttk::labelframe $mytoplevel.api -text [_ "Audio API"] \
            -style Prefs.TLabelframe
        pack $apifr -side top -padx 1 -pady 1 -fill x
        set api_names {}
        set ::audio_apiname {};
        foreach api $::pd_apilist {lappend api_names [lindex $api 0]}
        set api_i [lsearch -exact -index 1 $::pd_apilist $::pd_whichapi]
        set ::audio_apiname [lindex $::pd_apilist $api_i 0]
        ::dialog_prefs::dropdown $apifr.apilist ::audio_apiname $api_names
        trace add variable ::audio_apiname write ::dialog_audio::setapi
        grid $apifr.apilist -sticky e -column 0 -row 0 -padx 3 -pady 10
        ttk::checkbutton $apifr.longbutton -text "Use multiple devices" \
            -command "::dialog_audio::setlongform $apifr.longbutton"
        grid $apifr.longbutton -sticky w -column 1 -row 0 -padx 3 -pady 10
        grid columnconfigure $apifr {0 1} -weight 1
    }
    # disable longbutton if hardware doesn't support multi devices
    set state normal
    if {![expr [llength $audio_indevlist] > 1 && \
        $multi>1 && [llength $audio_outdevlist] > 1]} {
            set state disabled
    }
    $apifr.longbutton configure -state $state

    # frame to encapsulate api-specific settings and devices,
    # as well as the "Connect" button
    set afr $mytoplevel.audio

    destroy $afr
    ttk::frame $afr
    pack $afr -side top -fill x

    # todo: put padding with style settings in dialog_prefs.tcl
    set padx 1

    # sample rate and advance
    set sfr [ttk::labelframe $afr.settings -text [_ "Settings"] \
        -style Prefs.TLabelframe -padding 5 ]
    pack $sfr -side top -fill x -padx 3 -pady 10
    ttk::label $sfr.l1 -text [_ "Sample rate"]
    ttk::label $sfr.l2 -text [_ "Block size"]
    ::dialog_prefs::dropdown $sfr.x2 \
        ::audio_blocksize {64 128 256 512 1024 2048}
    ttk::entry $sfr.x1 -textvariable audio_sr -width 7
    ttk::label $sfr.l3 -text [_ "Delay (ms)"]
    ttk::entry $sfr.x3 -textvariable audio_advance -width 7
    grid $sfr.l1 -row 0 -sticky e -padx $padx
    grid $sfr.x1 -row 0 -column 1 -sticky w -padx $padx
    grid $sfr.l2 -row 0 -column 2 -sticky e -padx $padx
    grid $sfr.x2 -row 0 -column 3 -sticky w -padx $padx
    grid $sfr.l3 -row 1 -column 0 -sticky e -padx $padx
    grid $sfr.x3 -row 1 -column 1 -sticky w -padx $padx
    if {$audio_callback >= 0} {
        ttk::label $sfr.l4 -text [_ "Use callbacks"] -anchor e
        ttk::checkbutton $sfr.x4 -variable audio_callback
        grid $sfr.l4 -column 2 -row 1 -sticky e -padx $padx
        grid $sfr.x4 -column 3 -row 1 -sticky w -padx $padx
    }
    grid columnconfigure $sfr {0 2} -weight 1
    grid columnconfigure $sfr {1 3} -weight 2

    # Devices 
    set devfr [ttk::labelframe $afr.devs -text [_ "Devices"] \
        -style Prefs.TLabelframe]
    pack $devfr -side top -fill x -padx 3 -pady 10
    set j 2
    # todo: change in out to input output and make translatable strings
    foreach {type name} [list in [_ "Input"] out [_ "Output"]] {
        set domulti [expr $longform && $multi > 1 && \
            [llength [set "audio_${type}devlist"]] > 1]
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
        for {set i 0} {$i < 4} \
            {incr i} {
            set devno [expr $i + 1]
            set row "$devfr.${type}$devno"
            if {$domulti} {
                set ctext "$devno."
            } else {
                set ctext $name
            }
            ttk::label ${row}x0 -text $ctext -anchor w
            set ::audio_${type}dev${devno}label {}
            ::dialog_prefs::dropdown_by_index ${row}x1 \
                "::audio_${type}dev$devno" \
                [concat [set audio_${type}devlist] None] \
                "::audio_${type}dev${devno}label"
            if {[set audio_${type}enable$devno] > 0} {
                ::dialog_prefs::dropdown_set ${row}x1 [lindex [set audio_${type}devlist] [set audio_${type}dev$devno]]
            } else {
                ::dialog_prefs::dropdown_set ${row}x1 "None"
                set audio_${type} [llength [set audio_${type}devlist]]
            }
            ttk::entry ${row}x2 -textvariable "audio_${type}chan$devno" -width 4
            grid ${row}x0 -row $j -column 0 -sticky e -padx $padx
            grid ${row}x1 -row $j -column 1 -columnspan 2 -sticky ew -padx $padx
            grid ${row}x2 -row $j -column 3 -padx $padx
            grid columnconfigure $afr.devs {1 2 3} -weight 2
            grid columnconfigure $afr.devs 0 -weight 1
            incr j
            if {![expr $longform && $multi > 1 && \
                [llength [set "audio_${type}devlist"]] > 1]} {
                break
            }
        }
    }
    grid rowconfigure $devfr all -pad 3

    # Connect button
    ttk::frame $afr.buttonframe
    pack $afr.buttonframe
    pack $afr.buttonframe -side bottom
    ttk::button $afr.buttonframe.apply -text [_ "Apply Audio Settings"]\
        -command "::dialog_audio::apply $mytoplevel"
    pack $afr.buttonframe.apply -side left -expand 1 -fill x \
        -padx 15

#    $sfr.x1 select from 0
#    $sfr.x1 select adjust end
    focus $apifr.apilist
}
