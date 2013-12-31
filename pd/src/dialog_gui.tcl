package provide dialog_gui 0.1
package require dialog_prefs

namespace eval ::dialog_gui:: {
    namespace export create_gui_dialog
}

####################### gui dialog ##################3

proc ::dialog_gui::apply {mytoplevel} {
    # nothing to do
}

proc ::dialog_gui::cancel {mytoplevel} {
#    pdsend "$mytoplevel cancel"
}

proc ::dialog_gui::ok {mytoplevel} {
    ::dialog_gui::apply $mytoplevel
    ::dialog_gui::cancel $mytoplevel
}

proc ::dialog_gui::setswatch {b swatch} {
    $b configure -image $swatch
}

# this is triggered whenever the ::gui_preset
# variable is written to
proc ::dialog_gui::set_gui_preset {args} {
    set choice $::gui_preset
    switch $choice {
        Vanilla {
            set ::pd_colors(atom_box)        white
            set ::pd_colors(atom_box_border) black
            set ::pd_colors(canvas_color)    white
            set ::pd_colors(canvas_cursor)   black
            set ::pd_colors(text)            black
            set ::pd_colors(box)             white
            set ::pd_colors(box_border)      black
            set ::pd_colors(msg)             white
            set ::pd_colors(msg_border)      black
            set ::pd_colors(iemgui_border)   black
            set ::pd_colors(control_cord)    black
            set ::pd_colors(control_nlet)    white
            set ::pd_colors(iemgui_nlet)     black
            set ::pd_colors(signal_cord)     black
            set ::pd_colors(signal_nlet)     $::pd_colors(signal_cord)
            set ::pd_colors(control_nlet)    white
            set ::pd_colors(xlet_hover)      grey
            set ::pd_colors(link)            blue
            set ::pd_colors(selection)       blue
            set ::pd_colors(selection_rectangle) black
            set ::pd_colors(highlighted_text) black
            set ::pd_colors(highlighted_text_bg) #c3c3c3
            set ::pd_colors(dash_outline)    "#f00"
            set ::pd_colors(dash_fill)       white
            set ::pd_colors(graph_border)    black
            set ::pd_colors(graph)           white
            set ::pd_colors(magic_glass_bg)  black
            set ::pd_colors(magic_glass_bd)  black
            set ::pd_colors(magic_glass_text) "#ffffff"
            set ::pd_colors(magic_glass_flash) "#e87216"
        }
        Inverted {
            set ::pd_colors(atom_box)        black
            set ::pd_colors(atom_box_border) white
            set ::pd_colors(canvas_color)    black
            set ::pd_colors(canvas_cursor)   white
            set ::pd_colors(text)            white
            set ::pd_colors(box)             black
            set ::pd_colors(box_border)      white
            set ::pd_colors(msg)             black
            set ::pd_colors(msg_border)      white
            set ::pd_colors(iemgui_border)   white
            set ::pd_colors(control_cord)    white
            set ::pd_colors(control_nlet)    white
            set ::pd_colors(iemgui_nlet)     white
            set ::pd_colors(signal_cord)     white
            set ::pd_colors(signal_nlet)     $::pd_colors(signal_cord)
            set ::pd_colors(control_nlet)    white
            set ::pd_colors(xlet_hover)      grey
            set ::pd_colors(link)            yellow
            set ::pd_colors(selection)       yellow
            set ::pd_colors(selection_rectangle) white
            set ::pd_colors(highlighted_text) white
            set ::pd_colors(highlighted_text_bg) #3c3c3c
            set ::pd_colors(dash_outline)    "#f00"
            set ::pd_colors(dash_fill)       black
            set ::pd_colors(graph_border)    white
            set ::pd_colors(graph)           gray
            set ::pd_colors(magic_glass_bg)  white
            set ::pd_colors(magic_glass_bd)  white
            set ::pd_colors(magic_glass_text) "#000000"
            set ::pd_colors(magic_glass_flash) "#e87216"
        }
        L2ork    {
            set ::pd_colors(atom_box)        "#eee"
            set ::pd_colors(atom_box_border) "#ccc"
            set ::pd_colors(canvas_color)     white
            set ::pd_colors(canvas_cursor)    black
            set ::pd_colors(text)             black
            set ::pd_colors(box)              "#f6f8f8"
            set ::pd_colors(box_border)       "#ccc"
            set ::pd_colors(msg)              #f8f8f6
            set ::pd_colors(msg_border)       "#ccc"
            set ::pd_colors(iemgui_border)    "#000000"
            set ::pd_colors(iemgui_nlet)      "#000000"
            set ::pd_colors(control_cord)     "#565"
            set ::pd_colors(control_nlet)     white
            set ::pd_colors(signal_cord)      #808095
            set ::pd_colors(signal_nlet)      $::pd_colors(signal_cord)
            set ::pd_colors(xlet_hover)       grey
            set ::pd_colors(link)             #eb5f28
            set ::pd_colors(selection)        #e87216
            set ::pd_colors(selection_rectangle) #e87216
            set ::pd_colors(highlighted_text) black
            set ::pd_colors(highlighted_text_bg) #c3c3c3
            set ::pd_colors(dash_outline)     "#f00"
            set ::pd_colors(dash_fill)        "#ffdddd"
            set ::pd_colors(graph_border)     "#777"
            set ::pd_colors(graph)            white
            set ::pd_colors(magic_glass_bg)    black
            set ::pd_colors(magic_glass_bd)    black
            set ::pd_colors(magic_glass_text)  white
            set ::pd_colors(magic_glass_flash) "#e87216"
        }
        L2ork_Inverted {
            set ::pd_colors(atom_box)       black
            set ::pd_colors(atom_box_border) white
            set ::pd_colors(canvas_color)     black
            set ::pd_colors(canvas_cursor) white
            set ::pd_colors(text)          white
            set ::pd_colors(box)           #090707
            set ::pd_colors(box_border)    #3e3e3e
            set ::pd_colors(msg)       #090707
            set ::pd_colors(msg_border) #3e3e3e
            set ::pd_colors(iemgui_border) white
            set ::pd_colors(iemgui_nlet) white
            set ::pd_colors(control_cord)  white
            set ::pd_colors(control_nlet)  #a294a2
            set ::pd_colors(signal_cord)   #7d7d68
            set ::pd_colors(signal_nlet)   $::pd_colors(signal_cord)
            set ::pd_colors(xlet_hover)    white
            set ::pd_colors(link)          blue
            set ::pd_colors(selection)      #ffff00
            set ::pd_colors(selection_rectangle) white
            set ::pd_colors(highlighted_text) white
            set ::pd_colors(highlighted_text_bg) #3c3c3c
            set ::pd_colors(dash_outline)  "#f00"
            set ::pd_colors(dash_fill)     "#002222"
            set ::pd_colors(graph_border)  "#777"
            set ::pd_colors(graph)         gray
            set ::pd_colors(magic_glass_bg) black
            set ::pd_colors(magic_glass_bd) black
            set ::pd_colors(magic_glass_text) white
            set ::pd_colors(magic_glass_flash) "#e87216"
        }
        Extended    {
            set ::pd_colors(atom_box)       #e0e0e0
            set ::pd_colors(atom_box_border) #c1c1c1
            set ::pd_colors(canvas_color)     white
            set ::pd_colors(canvas_cursor) black
            set ::pd_colors(text)          black
            set ::pd_colors(box)           #f6f8f8
            set ::pd_colors(box_border)    #c1c1c1
            set ::pd_colors(msg)       #f6f8f8
            set ::pd_colors(msg_border) #c1c1c1
            set ::pd_colors(iemgui_border) black
            set ::pd_colors(iemgui_nlet) black
            set ::pd_colors(control_cord)  black
            set ::pd_colors(control_nlet)  white
            set ::pd_colors(signal_cord)   #828297
            set ::pd_colors(signal_nlet)   $::pd_colors(signal_cord)
            set ::pd_colors(control_nlet)         #536253
            set ::pd_colors(xlet_hover)    grey
            set ::pd_colors(link)          blue
            set ::pd_colors(selection)      blue
            set ::pd_colors(selection_rectangle) black
            set ::pd_colors(highlighted_text) black
            set ::pd_colors(highlighted_text_bg) #c3c3c3
            set ::pd_colors(dash_outline)  "#f00"
            set ::pd_colors(dash_fill)     "#f7f7f7"
            set ::pd_colors(graph_border)  "#777"
            set ::pd_colors(graph)         white
            set ::pd_colors(magic_glass_bg) black
            set ::pd_colors(magic_glass_bd) black
            set ::pd_colors(magic_glass_text) white
            set ::pd_colors(magic_glass_flash) "#e87216"
        }
        C64   {
            set ::pd_colors(atom_box)       #3e32a2
            set ::pd_colors(atom_box_border) #7569d7
            set ::pd_colors(canvas_color)     #3e32a2
            set ::pd_colors(canvas_cursor) white
            set ::pd_colors(text)          #a49aea
            set ::pd_colors(box)           #3e32a2
            set ::pd_colors(box_border)    #7569d7
            set ::pd_colors(msg)       #3e32a2
            set ::pd_colors(msg_border) #7569d7
            set ::pd_colors(iemgui_border) #7569d7
            set ::pd_colors(iemgui_nlet) #7569d7
            set ::pd_colors(control_cord)  #7569d7
            set ::pd_colors(control_nlet)  white
            set ::pd_colors(signal_cord)   #7569d7
            set ::pd_colors(signal_nlet)   $::pd_colors(signal_cord)
            set ::pd_colors(control_nlet)         #7c71da
            set ::pd_colors(xlet_hover)    grey
            set ::pd_colors(link)          #e87216
            set ::pd_colors(selection)      #cc9933
            set ::pd_colors(selection_rectangle) #7c71da
            set ::pd_colors(highlighted_text) #3e32a2
            set ::pd_colors(highlighted_text_bg) #a49aea
            set ::pd_colors(dash_outline)  "#f00"
            set ::pd_colors(dash_fill)     "#3e32a2"
            set ::pd_colors(graph_border)  "#777"
            set ::pd_colors(graph)         "#3e32a2"
            set ::pd_colors(magic_glass_bg) black
            set ::pd_colors(magic_glass_bd) black
            set ::pd_colors(magic_glass_text) white
            set ::pd_colors(magic_glass_flash) "#e87216"
        }
        Strongbad {
            set ::pd_colors(atom_box)       black
            set ::pd_colors(atom_box_border) #0b560b
            set ::pd_colors(canvas_color)     black
            set ::pd_colors(canvas_cursor) white
            set ::pd_colors(text)          #4bd046
            set ::pd_colors(box)           black
            set ::pd_colors(box_border)    #0b560b
            set ::pd_colors(msg)       black
            set ::pd_colors(msg_border) #0b560b
            set ::pd_colors(iemgui_border) #0b560b
            set ::pd_colors(iemgui_nlet) #0b560b
            set ::pd_colors(control_cord)  #53b83b
            set ::pd_colors(control_nlet)  #53b83b
            set ::pd_colors(signal_cord)   #53b83b
            set ::pd_colors(signal_nlet)   $::pd_colors(signal_cord)
            set ::pd_colors(xlet_hover)    white
            set ::pd_colors(link)          blue
            set ::pd_colors(selection)      green
            set ::pd_colors(selection_rectangle) #53b83b
            set ::pd_colors(highlighted_text) black
            set ::pd_colors(highlighted_text_bg) #4bd046 
            set ::pd_colors(dash_outline)  "#f00"
            set ::pd_colors(dash_fill)     "#f7f7f7"
            set ::pd_colors(graph_border)  "#777"
            set ::pd_colors(graph)         "#53b83b"
            set ::pd_colors(magic_glass_bg) black
            set ::pd_colors(magic_glass_bd) black
            set ::pd_colors(magic_glass_text) white
            set ::pd_colors(magic_glass_flash) "#e87216"
        }
        Subdued {
            set ::pd_colors(atom_box)       #9fc79f
            set ::pd_colors(atom_box_border) #b1d3b1
            set ::pd_colors(canvas_color)     #c0dcc0
            set ::pd_colors(canvas_cursor) black
            set ::pd_colors(text)          black
            set ::pd_colors(box)           #c0dcc0
            set ::pd_colors(box_border)    #666666
            set ::pd_colors(msg)       #c0dcc0
            set ::pd_colors(msg_border) #666666
            set ::pd_colors(iemgui_border) #666666
            set ::pd_colors(iemgui_nlet) #666666
            set ::pd_colors(control_cord)  #333333
            set ::pd_colors(control_nlet)         #333333
            set ::pd_colors(signal_cord)   #666666
            set ::pd_colors(signal_nlet)   $::pd_colors(signal_cord)
            set ::pd_colors(xlet_hover)    white
            set ::pd_colors(link)          blue
            set ::pd_colors(selection)      blue
            set ::pd_colors(selection_rectangle) #333333
            set ::pd_colors(highlighted_text) black 
            set ::pd_colors(highlighted_text_bg) #c3c3c3
            set ::pd_colors(dash_outline)  "#f00"
            set ::pd_colors(dash_fill)     "#f7f7f7"
            set ::pd_colors(graph_border)  "#777"
            set ::pd_colors(graph)         "#9fc79f"
            set ::pd_colors(magic_glass_bg) black
            set ::pd_colors(magic_glass_bd) black
            set ::pd_colors(magic_glass_text) white
            set ::pd_colors(magic_glass_flash) "#e87216"
        }
    }
}

proc ::dialog_gui::create_gui_dialog {mytoplevel} {
    if [winfo exists $mytoplevel.colors] then return
    set fr [ttk::frame $mytoplevel.colors]
    set p [ttk::frame $fr.presets]
    ttk::label $p.presetlabel -text "Color Preset"
#    ttk::combobox $fr.presets -state readonly -values {Inverted L2ork Foo}
# todo: set presets in _one_ place
    ::dialog_prefs::dropdown $p.presets ::gui_preset {Vanilla Inverted L2ork L2ork_Inverted Extended C64 Strongbad Subdued}
    pack $fr -side top
    grid $p -column 0 -columnspan 3 -row 0 -sticky w -pady 21
    pack $p.presetlabel -side left -padx 7
    pack $p.presets -side left
    set clen [expr {[llength [array names ::pd_colors]] / 2}]
    set i 0
    foreach name [lsort [array names ::pd_colors]] {
        # hack to exclude widths
        if {[regexp {.*width} $name]} {continue}
        set label [string map {_ " "} $name]
        set label [string toupper $label 0 0]
        ::dialog_prefs::swatchbutton $fr.$name ::pd_colors($name)
        ::dialog_prefs::set_swatchbutton $fr.$name \
            ::pd_colors($name)
        ttk::label $fr.${name}label -text "$label"
        grid $fr.${name} -column [expr $i/$clen * 2] -row [expr $i%$clen+1] -sticky e
        grid $fr.${name}label -column [expr $i/$clen * 2 + 1] -row [expr $i%$clen+1] -sticky w -padx 7 -pady 3
        incr i
    }
}
