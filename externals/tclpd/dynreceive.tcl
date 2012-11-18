package require Tclpd 0.2.1
package require TclpdLib 0.17

pd::class dynreceive {
    constructor {
        set @sym {}
        if {[pd::args] > 0} {
            set @sym [pd::arg 0 symbol]
            pd_bind [tclpd_get_instance_pd $self] [gensym $@sym]
        }
        pd::add_outlet $self
    }

    destructor {
        # don't forget to call pd_unbind, or sending things to a symbol
        # bound to dead object will crash pd!
        if {$@sym != {}} {
            pd_unbind [tclpd_get_instance_pd $self] [gensym $@sym]
        }
    }

    0_set {
        # send [set empty( to clear the receive symbol
        set s [pd::arg 0 symbol]
        if {$@sym != {}} {
            pd_unbind [tclpd_get_instance_pd $self] [gensym $@sym]
        }
        if {$s == {empty}} {
            set @sym {}
        } else {
            set @sym $s
            pd_bind [tclpd_get_instance_pd $self] [gensym $@sym]
        }
    }

    0_bang {
        pd::outlet $self 0 bang
    }

    0_float {
        pd::outlet $self 0 float [pd::arg 0 float]
    }

    0_symbol {
        pd::outlet $self 0 symbol [gensym [pd::arg 0 symbol]]
    }

    0_anything {
        set sel [pd::arg 0 symbol]
        set argz [lrange $args 1 end]
        pd::outlet $self 0 $sel $argz
    }
}
