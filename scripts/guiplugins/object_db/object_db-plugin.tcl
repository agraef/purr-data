
set enabled_libraries {
    {pd-lib} {boids} {bsaylor} {creb} {cxc} {cyclone} {cyclone (hammer)} {cyclone (sickle)}
    {ekext} {fftease} {flatspace} {flib} {freeverb~} {gem} {ggee} {hardware} {hcs} {hid}
    {iem_ambi} {iem_bin_ambi} {iemgui} {iemlib} {jasch_lib} {jmmmp} {keyboardkeys}
    {la-kitchen} {list-abs} {mapping} {markex} {maxlib} {memento} {mjlib} {motex} {mrpeach}
    {nqpoly~} {nqpoly4} {nusmuk} {oscx} {pan} {pddp} {pdjimmies} {pdmtl abstractions}
    {pdogg} {pdp} {Percolate} {pidip} {pixeltango} {pmpd} {rradical} {rtc} {sigpack}
    {smlib} {toxy} {unauthorized} {vasp} {xsample} {zexy} {zexy Abstraction}
}
# comment the following line to enable all libraries
set enabled_libraries {pd-lib}

# top level categories (tags)
set tlc {conversion pdmtl cyclone maxlib zexy vasp storage audio control connectivity imaging math misc}

# second level (in menu) categories
set c2 {
    connectivity {osc midi network}
    conversion {midi audio}
    cyclone {hammer sickle audio math control}
    zexy {audio analysis matrix control}
    maxlib {control time math glue}
    audio {abstraction conversion fftease cyclone math logical analysis filters delay effects tables}
    vasp {declaration arithmetic basics functions generators transcendent minmax utilities filters fft displace}
    storage {abstraction lists matrix tables}
    pdmtl {control convert list edit flow imaging}
    imaging {{gem particles} {gem manipulators} {gem pixes} {gem geos} {gem opengl} {pdp image} {pdp processing} {pdp abstraction} {pdp 3d} pidip manipulators wrapper particles automata processing}
}





#---------------------------------------------------------------------------------------------
# object -> tags mapping
array set object_tags {}
# tag reverse mapping
array set objects_with_tag {}

# load object -> tags mapping from file
set f [open [file join $::current_pathdir object_tags.tcllist]]
set tmp_db [read $f]
close $f
unset f

foreach {library object tags} $tmp_db {
    # skip unwanted libraries
    if {[lsearch -exact $enabled_libraries $library] == -1} {continue}
    foreach tag $tags {lappend object_tags($object) $tag}
}
unset tmp_db

foreach k [array names object_tags] {
    set object_tags($k) [lsort $object_tags($k)]
    foreach tag $object_tags($k) {lappend objects_with_tag($tag) $k}
}

proc object_db_query {q workingset} {
    global object_tags
    set q _[join [lsort $q] _.*_]_
    set result [list]
    foreach k $workingset {
        set v _[join $object_tags($k) __]_
        if {[regexp $q $v]} {lappend result $k}
    }
    set result
}

# TODO: benchmark which is faster between the two
proc object_db_query_re {q workingset} {
    global object_tags
    set q (?b)\\<[join [lsort $q] \\>.*\\<]\\>
    set result [list]
    foreach k $workingset {
        if {[regexp $q $object_tags($k)]} {lappend result $k}
    }
    set result
}

proc complement {e s} {
    set result [list]
    foreach i $e {if {[lsearch -exact $s $i] == -1} {lappend result $i}}
    set result
}

proc merge {args} {
    array set tmp {}
    foreach arg $args {
        foreach k $arg {
            set tmp($k) .
        }
    }
    set x [array names tmp]
    array unset tmp
    set x
}

proc pdtk_canvas_popup_addObjectBranch {t m lst} {
    set n 0
    foreach {k v} $lst {
        if {$k == {.}} {
            incr n
            set cbrk 0
            if {$n > 18} {
                set cbrk 1
                set n 1
            }
            $m add command -label $v -columnbreak $cbrk \
            -command "pdsend \"\$::focused_window obj \$::popup_xcanvas \$::popup_ycanvas $v\""
        } else {
            if {[llength $v] == 0} continue
            set sub ${m}.sub[incr ::s]
            menu $sub
	    # fix menu font size on Windows with tk scaling = 1
	    if {$::windowingsystem eq "win32"} {$sub configure -font menufont}
            $m add cascade -label $k -menu $sub
            pdtk_canvas_popup_addObjectBranch $t $sub $v
        }
    }
}

proc print_r {l {indent 0}} {
    foreach {k v} $l {
        if {$k == "."} {
            for {set j 0} {$j < $indent} {incr j} {puts -nonewline "  "}
            puts $v
        } else {
            for {set j 0} {$j < $indent} {incr j} {puts -nonewline "  "}
            puts "$k {"
            print_r $v [expr {$indent+1}]
            for {set j 0} {$j < $indent} {incr j} {puts -nonewline "  "}
            puts "}"
        }
    }
}

proc @ {l} {
    set result [list]
    set ls [lsort $l]
    foreach i $ls {lappend result . $i}
    set result
}

set l [list]
set all [array names object_tags]
# *partition* by tag into top-level-categories
foreach c $tlc {
    set c_$c [object_db_query $c $all]
    set all [complement $all [set c_$c]]
}
set c_others $all
# *search* by tag in 2nd-level-categories
# add 2-level categories
foreach {tlcn c2l} $c2 {
    set ll [list]
    set accum [list]
    foreach c2i $c2l {
        set lll [object_db_query $c2i [set c_$tlcn]]
        set accum [merge $accum $lll]
        if {[llength $lll] > 0} {lappend ll $c2i [@ $lll]}
    }
    set others [complement [set c_$tlcn] $accum]
    if {[llength $others] > 0} {lappend ll "others" [@ $others]}
    lappend l $tlcn $ll
}
# add 1-level-categories
foreach tlci $tlc {
    set c2_keys [list]
    # can't use dict on 8.4
    foreach {k v} $c2 {lappend c2_keys $k}
    if {[lsearch -exact $c2_keys $tlci] != -1} {continue}
    set c_1_set [object_db_query $tlci [set c_$tlci]]
    lappend l $tlci [@ $c_1_set]
}
# end menu structure builder

#print_r $l

.popup add separator
set s 0
pdtk_canvas_popup_addObjectBranch - .popup $l
