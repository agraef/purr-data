package require Tclpd 0.2.1
package require TclpdLib 0.17

set ::script_path [file dirname [info script]]

pd::guiproc slider2_draw_new {self c x y config state} {
    # import variables from dicts:
    foreach v {headsz width height fgcolor bgcolor orient} \
        {set $v [dict get $config -$v]}
    set x2 [expr {$x+$width+1}]
    set y2 [expr {$y+$height+1}]
    $c create rectangle $x $y $x2 $y2 \
        -outline $fgcolor -fill $bgcolor -tags [list $self border$self]
    switch $orient {
        horizontal {set y1 $y; set x3 [expr {$x+$headsz}]}
        vertical {set y1 [expr {$y2-$headsz}]; set x3 $x2}
    }
    $c create rectangle $x $y1 $x3 $y2 -outline {} -fill $fgcolor \
        -tags [list $self head$self]
    slider2_update $self $c $x $y $config $state
}

pd::guiproc slider2_update {self c x y config state} {
    # import variables from dicts:
    foreach v {initvalue headsz width height label labelpos lblcolor orient} \
        {set $v [dict get $config -$v]}
    foreach v {min max rev} {set $v [dict get $state _$v]}
    set realvalue [expr {1.0*($initvalue-$min)/($max-$min)}]
    if {$realvalue < 0.0} {set realvalue 0}
    if {$realvalue > 1.0} {set realvalue 1}
    if {$rev} {set realvalue [expr {1.0-$realvalue}]}
    if {$orient == "vertical"} {set realvalue [expr {1.0-$realvalue}]}
    switch $orient {
        horizontal {
            set hr [expr {$width-$headsz}]
            $c coords head$self [expr {$x+$hr*$realvalue}] $y \
                [expr {$x+$hr*$realvalue+$headsz}] [expr {$y+$height+1}]
        }
        vertical {
            set vr [expr {$height-$headsz}]
            $c coords head$self $x [expr {$y+$vr*$realvalue}] \
                [expr {$x+$width+1}] [expr {$y+$vr*$realvalue+$headsz}]
        }
    }
    $c delete label$self
    if {$label != {}} {
        switch $labelpos {
            top
            {set lx [expr {$x+$width/2}]; set ly [expr {$y}]; set a "s"}
            bottom
            {set lx [expr {$x+$width/2}]; set ly [expr {$y+$height+2}]; set a "n"}
            left
            {set lx [expr {$x}]; set ly [expr {$y+$height/2}]; set a "e"}
            right
            {set lx [expr {$x+$width+2}]; set ly [expr {$y+$height/2}]; set a "w"}
        }
        $c create text $lx $ly -anchor $a -text $label -fill $lblcolor \
             -tags [list $self label$self]
    }
}

pd::guiclass slider2 {
    constructor {
        pd::add_outlet $self float
        sys_gui "source {[file join $::script_path properties.tcl]}\n"
        # set defaults:
        set @config {
            -width 15 -height 130 -headsz 3 -rangebottom 0 -rangetop 127
            -init 0 -initvalue 0 -jumponclick 0 -label "" -labelpos "top"
            -orient "vertical" -sendsymbol "" -receivesymbol ""
            -fgcolor "#000000" -bgcolor "#ffffff" -lblcolor "#000000"
        }
        set @state {_min 0 _max 127 _rev 0}
        # expanded ($n) send/recv symbols:
        set @send {}
        set @recv {}
        ::$self 0 config {*}$args
    }

    destructor {
        if {[dict get $@config -receivesymbol] != {}} {
            pd_unbind [tclpd_get_instance_pd $self] $@recv
        }
    }

    0_loadbang {
        if {[dict get $@config -init]} {$self 0 bang}
    }

    0_config {
        set newconf [list]
        set optlist [pd::strip_selectors $args]
        set optlist [pd::strip_empty $optlist]
        set int_opts {-width -height -cellsize}
        set bool_opts {-init -jumponclick}
        set ui_opts {-fgcolor -bgcolor -lblcolor -orient -width -height}
        set upd_opts {-rangebottom -rangetop -label -labelpos}
        set conn_opts {-sendsymbol -receivesymbol}
        set ui 0
        set upd 0
        foreach {k v} $optlist {
            if {![dict exists $@config $k]} {
                return -code error "unknown option '$k'"
            }
            if {[dict get $@config $k] == $v} {continue}
            if {[lsearch -exact $int_opts $k] != -1} {set v [expr {int($v)}]}
            if {[lsearch -exact $bool_opts $k] != -1} {set v [expr {int($v)!=0}]}
            if {[lsearch -exact $ui_opts $k] != -1} {set ui 1}
            if {[lsearch -exact $upd_opts $k] != -1} {set upd 1}
            dict set newconf $k $v
        }
        # process -{send,receive}symbol
        if {[dict exists $newconf -receivesymbol]} {
            set new_recv [dict get $newconf -receivesymbol]
            set selfpd [tclpd_get_instance_pd $self]
            if {[dict get $@config -receivesymbol] != {}} {
                pd_unbind $selfpd $@recv
            }
            if {$new_recv != {}} {
                set @recv [canvas_realizedollar \
                    [tclpd_get_glist $self] [gensym $new_recv]]
                pd_bind $selfpd $@recv
            } else {set @recv {}}
        }
        if {[dict exists $newconf -sendsymbol]} {
            set new_send [dict get $newconf -sendsymbol]
            if {$new_send != {}} {
                set @send [canvas_realizedollar \
                    [tclpd_get_glist $self] [gensym $new_send]]
            } else {set @send {}}
        }
        # changing orient -> swap sizes
        if {[dict exists $newconf -orient] && ![dict exists $newconf -width]
            && ![dict exists $newconf -height]} {
            dict set newconf -width [dict get $@config -height]
            dict set newconf -height [dict get $@config -width]
        }
        # no errors up to this point. we can safely merge options
        set @config [dict merge $@config $newconf]
        # adjust reverse range
        set a [dict get $@config -rangebottom]
        set b [dict get $@config -rangetop]
        dict set @state _min [expr {$a>$b?$b:$a}]
        dict set @state _max [expr {$a>$b?$a:$b}]
        dict set @state _rev [expr {$a>$b}]
        set orient [dict get $@config -orient]
        switch $orient {
            horizontal {set dim [dict get $@config -width];  set mul  1}
            vertical   {set dim [dict get $@config -height]; set mul -1}
            default {return -code error "invalid value '$orient' for -orient"}
        }
        # recompute pix2units conversion
        set @pix2units [expr {(2.0 * [dict get $@state _rev] - 1.0) *
            ( [dict get $@state _max] - [dict get $@state _min] ) *
            $mul / ( $dim - [dict get $@config -headsz])}]
        # if ui changed, update it
        if {$ui && [info exists @c]} {
            sys_gui [list $@c delete $self]\n
            sys_gui [list slider2_draw_new $self $@c $@x $@y $@config $@state]\n
        } elseif {$upd && [info exists @c]} {
            sys_gui [list slider2_update $self $@c $@x $@y $@config $@state]\n
        }
        if {[dict exists $newconf -width] || [dict exists $newconf -height]} {
            canvas_fixlinesfor \
                [tclpd_get_glist $self] [tclpd_get_instance_text $self]
        }
    }
    
    0_set {
        foreach v {min max} {set $v [dict get $@state _$v]}
        set f [pd::arg 0 float]
        if {$f < $min} {set f $min}
        if {$f > $max} {set f $max}
        dict set @config -initvalue $f
        if {[info exists @c]} {
            # update ui:
            sys_gui [list slider2_update $self $@c $@x $@y $@config $@state]\n
        }
    }

    0_bang {
        foreach v {initvalue} {set $v [dict get $@config -$v]}
        pd::outlet $self 0 float $initvalue
        if {$@send != {}} {
            set s_thing [$@send cget -s_thing]
            if {$s_thing != {NULL}} {pd_float $s_thing $initvalue}
        }
    }

    0_float {
        $self 0 set {*}$args
        $self 0 bang
    }

    object_save {
        return [list #X obj $@x $@y slider2 {*}[pd::add_empty $@config] \;]
    }

    object_properties {
        set c [string map {$ \\$} $@config]
        gfxstub_new [tclpd_get_object_pd $self] [tclpd_get_instance $self] \
            [list propertieswindow %s $c "\[slider2\] properties"]\n
    }

    widgetbehavior_getrect {
        lassign $args x1 y1
        set x2 [expr {1+$x1+[dict get $@config -width]}]
        set y2 [expr {1+$y1+[dict get $@config -height]}]
        return [list $x1 $y1 $x2 $y2]
    }

    widgetbehavior_displace {
        lassign $args dx dy
        if {$dx != 0 || $dy != 0} {
            incr @x $dx; incr @y $dy
            sys_gui [list $@c move $self $dx $dy]\n
        }
        return [list $@x $@y]
    }

    widgetbehavior_select {
        lassign $args sel
        sys_gui [list $@c itemconfigure $self&&!label$self -outline [lindex \
            [list [dict get $@config -fgcolor] {blue}] $sel]]\n
    }

    widgetbehavior_vis {
        lassign $args @c @x @y vis
        if {$vis} {
            sys_gui [list slider2_draw_new $self $@c $@x $@y $@config $@state]\n
        } else {
            sys_gui [list $@c delete $self]\n
        }
    }

    widgetbehavior_click {
        lassign $args x y shift alt dbl doit
        set h [dict get $@config -height]
        set ypix [expr {[lindex $args 1]-$@y-1}]
        if {$ypix < 0 || $ypix >= $h} {return}
        if {$doit} {
            switch [dict get $@config -orient] {
                horizontal {
                    set @motion_start_x $x
                    set @motion_curr_x $x
                }
                vertical {
                    set @motion_start_y $y
                    set @motion_curr_y $y
                }
            }
            set @motion_start_v [dict get $@config -initvalue]
            tclpd_guiclass_grab [tclpd_get_instance $self] \
                [tclpd_get_glist $self] $x $y
        }
    }

    widgetbehavior_motion {
        lassign $args dx dy
        switch [dict get $@config -orient] {
            horizontal {
                set @motion_curr_x [expr {$dx+$@motion_curr_x}]
                set pixdelta [expr {-1*($@motion_curr_x-$@motion_start_x)}]
            }
            vertical {
                set @motion_curr_y [expr {$dy+$@motion_curr_y}]
                set pixdelta [expr {-1*($@motion_curr_y-$@motion_start_y)}]
            }
        }
        set f [expr {$@motion_start_v+$pixdelta*$@pix2units}]
        $self 0 float {*}[pd::add_selectors [list $f]]
    }
}
