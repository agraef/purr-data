package require Tclpd 0.2.1
package require TclpdLib 0.17

# dynroute: dynamically route messages based on first element
# non-matching arguments are sent to last inlet
# constructor: <float>   specify the number of outlets (default: 1)
# send commands to the right inlet
# available commands:
# add <atom> <float>     route selector <atom> to output number <float>
# remove <atom> <float>  remove previously created routing
# clear

pd::class dynroute {
    constructor {
        pd::add_inlet $self list

        set @num_outlets [pd::arg 0 int]
        if {$@num_outlets < 0} {set @num_outlets 2}

        for {set i 0} {$i < $@num_outlets} {incr i} {
            pd::add_outlet $self list
        }

        set @routing {}
    }

    0_list {
        set sel [pd::arg 0 any]
        set out [expr {$@num_outlets-1}]
        catch {set out [dict get $@routing $sel]}
        pd::outlet $self $out list $args
    }

    1_add {
        set sel [pd::arg 0 any]
        set out [pd::arg 1 int]
        if {$out < 0 || $out >= $@num_outlets} {
            pd::post "error: add: outlet number out of range"
            return
        }
        dict set @routing $sel $out
    }

    1_remove {
        set sel [pd::arg 0 any]
        set out [pd::arg 1 int]
        if {$out < 0 || $out >= $@num_outlets} {
            pd::post "error: add: outlet number out of range"
            return
        }
        catch {dict unset @routing $sel $out}
    }

    1_clear {
        set @routing {}
    }
}
