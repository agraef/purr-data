# this script makes it so that the cords are hidden when not in edit mode

proc set_cords_by_editmode {mytoplevel} {
    if {$mytoplevel eq ".pdwindow"} {return}
    set tkcanvas [tkcanvas_name $mytoplevel]
 	if { ! [winfo exists $mytoplevel] } {return}
    if {$::editmode($mytoplevel) == 1} {
        $tkcanvas itemconfigure cord -fill black
        $tkcanvas raise cord
    } else {
        $tkcanvas itemconfigure cord -fill white
        $tkcanvas lower cord
    }
}

bind all <<EditMode>> {+set_cords_by_editmode %W}
