# this is a plugin for Pd 0.43 that adds the ability to associate Pd patches
# with filetypes like .wav, .ogg, etc.

package provide file_associations 0.1

namespace eval ::file_associations:: {
    namespace export pdtk_post
}

proc ::file_associations::load_file_associations {} {
    set ::associationsdir "[pwd]/../associations"
    # TODO this should search the whole path for 'associations' in order to
    # support user-installed associations, not only included ones
    puts stderr "find_file_associations $::associationsdir"
    if { ! [file isdirectory $::associationsdir]} { return }
    foreach filename [glob -directory $::associationsdir -nocomplain -types {f} -- *.pd] {
        set extension [file rootname [file tail $filename]]
        puts "\tAssociating $filename to $extension"
        lset ::filetypes 0 1 [concat [lindex $::filetypes 0 1] ".$extension"]
        set ::filetypes [lappend ::filetypes \
                             [list "[string toupper $extension] files" ".$extension"]]
    }
}

proc ::file_associations::menu_open {} {
    pdtk_post "using menu_open from file_associations.tcl"
    if { ! [file isdirectory $::pd_menucommands::menu_open_dir]} {
        set ::pd_menucommands::menu_open_dir $::env(HOME)
    }
    set files [tk_getOpenFile -defaultextension .pd \
                       -multiple true \
                       -filetypes $::filetypes \
                       -initialdir $::pd_menucommands::menu_open_dir]
    if {$files ne ""} {
        foreach filename $files {
            set extension [lindex [split $filename .] end]
            if {$extension == "pd"} {
                puts "open_file $filename"
                open_file $filename
            } else {
                set assocpatchfile [open [file join $::associationsdir "$extension.pd"]]
                set patchcontents [regsub -all -- "\\\$FILENAME" [read $assocpatchfile] $filename]
                close $assocpatchfile
                pdsend "pd filename [file tail $filename].pd \
                            [enquote_path $::pd_menucommands::menu_open_dir]"
                pdsend $patchcontents
                pdsend "#X pop 1"
            }
        }
        set ::pd_menucommands::menu_open_dir [file dirname $filename]
    }
}

#::file_associations::load_file_associations
# TODO insert code to override built-in menu_open proc
