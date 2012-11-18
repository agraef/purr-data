

# for more info: http://tcl.tk/man/tcl8.5/TkCmd/menu.htm
.popup add separator
.popup add command -label "My Favorite Object" \
    -command {pdsend "$::focused_window obj $::popup_xcanvas $::popup_ycanvas osc~"}
