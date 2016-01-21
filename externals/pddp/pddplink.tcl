
proc pddplink_open {filename dir} {
    if {[string first "://" $filename] > -1} {
        menu_openfile $filename
    } elseif {[file pathtype $filename] eq "absolute"} {
        if {[file extension $filename] eq ".pd"} {
            set dir [file dirname $filename]
            set name [file tail $filename]
            menu_doc_open $dir $name            
        } else {
            menu_openfile $filename
        }
    } elseif {[file exists [file join $dir $filename]]} {
        set fullpath [file normalize [file join $dir $filename]]
        set dir [file dirname $fullpath]
        set filename [file tail $fullpath]
        menu_doc_open $dir $filename
    } else {
        bell ;# beep on error to provide instant feedback
        pdtk_post "\[pddplink\] ERROR file not found: $filename\n"
    }
}
