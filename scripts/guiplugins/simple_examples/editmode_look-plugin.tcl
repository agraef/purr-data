# this script makes it so that the cords are hidden when not in edit mode

proc set_cords_by_editmode {mytoplevel} {
    if {$mytoplevel eq ".pdwindow"} {return}
    set tkcanvas [tkcanvas_name $mytoplevel]
 	if { ! [winfo exists $mytoplevel] } {return}
    if {$::editmode($mytoplevel) == 1} {
        $tkcanvas configure -background "#fff"
        $tkcanvas itemconfigure graph -fill black
        $tkcanvas itemconfigure array -fill black
        $tkcanvas itemconfigure array -activefill blue
        $tkcanvas itemconfigure label -fill black
        $tkcanvas itemconfigure obj -fill black
        $tkcanvas itemconfigure msg -activefill black
        $tkcanvas itemconfigure atom -activefill black
        $tkcanvas itemconfigure cord -fill black
        $tkcanvas itemconfigure {inlet || outlet} -outline black
    } else {
        $tkcanvas configure -background white
        $tkcanvas itemconfigure graph -fill magenta
        $tkcanvas itemconfigure array -fill cyan
        $tkcanvas itemconfigure array -activefill blue
        $tkcanvas itemconfigure label -fill "#777777"
        $tkcanvas itemconfigure obj -fill grey
        $tkcanvas itemconfigure msg -activefill blue
        $tkcanvas itemconfigure atom -activefill blue
        $tkcanvas itemconfigure cord -fill grey
        $tkcanvas itemconfigure {inlet || outlet} -outline white
        $tkcanvas lower {inlet || outlet}
    }
}

bind all <<EditMode>> {+set_cords_by_editmode %W}
