#todo: rename img to imgprefix

# package provide pdtk_drawimage 0.1
# package require tkpng

namespace eval ::pdtk_drawimage:: {
    namespace export pdtk_drawimage_new
    namespace export pdtk_drawimage_vis
}

# Some GUI procs for [drawimage] and [drawsprite]

# Draw an image
proc pdtk_drawimage_new {obj path canvasdir flags} {
    set drawsprite 1
    set image_seq [expr {$flags & $drawsprite}]
    # obj  - .x%lx name for [drawimage] instance
    # path - this is absolute or relative
    #        for [drawsprite] this is the directory of the image sequence
    #        for [drawimage] this is the file path of the image
    # canvasdir - relative paths should be relative to this
    #        (any other possibilities?)
    set i 0
    set matchchar *
    # this will discard $canvasdir for absolute paths, which is nice
    set path [file normalize [file join $canvasdir $path]]
    if {[file isdir $path]} {
        # put a final directory separator for a dir
        set path [string trimright [file join $path { }]]
    } else {
        # if it's a file we don't want a wildcard character
        set matchchar {}
    }
    if {![file exists $path]} {
        pdtk_post "drawimage: warning: path doesn't exist: $path\n"
        pd [concat $obj size 1 1 \;]
        return
    }
    foreach filename [lsort -dictionary [glob -nocomplain -type {f r} \
        -path $path $matchchar]] {
        if {[file extension $filename] eq ".gif" ||
            [file extension $filename] eq ".png"} {
            image create photo ::drawimage_${obj}$i -file "$filename"
            pdtk_post "image is ::drawimage_${obj}$i\n"
            incr i
        }
        if {$i > 1000 || !$image_seq} {break}
    }
    pdtk_post "no of files: $i\n"
    # we bound a symbol to $img in drawimage_new, so we
    # can send back a message with the image dimensions
    # to be used for the selection bbox. This is dumb--
    # pd has no business handling a gui issue like size
    # of a selection rectangle.  That's what Tk is for.
    # But that's a bigger issue to be dealt with later.
    if {$i > 0} {
    pdtk_post "image width is [image width ::drawimage_${obj}0]\n"
    pdtk_post "image height is [image height ::drawimage_${obj}0]\n"
    pdtk_post "obj is $obj\n"
        pd [concat $obj size [image width ::drawimage_${obj}0] \
            [image height ::drawimage_${obj}0] \;]
    } else {
        pdtk_post "drawimage: warning: no images loaded"
    }
}

proc pdtk_drawimage_vis {c x y obj tag seqno l2orktag1 l2orktag2 tag3 drawtag} {
    set img ::drawimage_${obj}
    set len [llength [lsearch -glob -all [image names] ${img}*]]
    if {$len < 1} {return}
    if {$seqno >= $len || $seqno < 0} {set seqno [expr {$seqno % $len}]}
    $c create pimage $x $y -image ${img}$seqno -tags [list $tag $l2orktag1 $l2orktag2 $drawtag] -parent $tag3
}

proc pdtk_drawimage_unvis {c tag} {
    $c delete $tag
}

proc pdtk_drawimage_free {img} {
#    image delete [lsearch -glob -all -inline [image names] ::drawimage_${img}*]

    foreach globalimage [image names] {
        if {[lsearch -glob $globalimage ::drawimage_${img}*] != -1} {
            image delete $globalimage
            pdtk_post "Deleted $globalimage\n"
        }
    }
}
