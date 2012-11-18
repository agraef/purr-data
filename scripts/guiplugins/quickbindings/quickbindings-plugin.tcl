# this is a rough attempt to bind keys to little snippets of patches.
# Currently, there are only three keys bound: o r s

package require pd_connect ;# for pdsend

namespace eval ::patch_by_key:: {
    variable patches
    array set patches {}
}

proc ::patch_by_key::init {} {
    variable patches
    # map keys to patch snippets
    set patches(o) "#X obj 1 2 osc~;"
    set patches(r) "#X obj 1 2 r test;"
    set patches(s) "#X obj 1 2 s test;"
}

proc ::patch_by_key::key {tkcanvas key x y} {
    pdtk_post "::patch_by_key::key $tkcanvas $key $x $y"
    variable patches
    set mytoplevel [winfo toplevel $tkcanvas]

    # make sure we have a mapping for the key before continuing
    if {[lsearch -exact [array names patches] $key] == -1} {return}
    # only do this for PatchWindows, not dialogs, pdwindow, etc.
    if {[winfo class $mytoplevel] ne "PatchWindow"} {return}
    # only do this if the canvas is in editmode
    if {$::editmode($mytoplevel) != 1} {return}

    # regexp magic to replace #X with the current canvas name, and the
    # (x,y) with the current mouse location
    set x [$tkcanvas canvasx $x]
    set y [$tkcanvas canvasy $y]
    pdsend [regsub -- {#X (\S+) [0-9]+ [0-9]+} $patches($key) "$mytoplevel \\1 $x $y"]
}

::patch_by_key::init
bind all <Key> "+::patch_by_key::key %W %K %x %y"
