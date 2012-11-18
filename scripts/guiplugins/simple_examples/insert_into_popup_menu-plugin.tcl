
# for more info: http://tcl.tk/man/tcl8.5/TkCmd/menu.htm

# create the item, then stick it to the canvas using a 'mouseup' message
proc popup_create_put {putitem} {
    pdsend "$::focused_window $putitem"
    pdsend "$::focused_window mouseup $::popup_xpix $::popup_ypix 1"
}

# create our submenu
menu .popup.create
# fix menu font size on Windows with tk scaling = 1
if {$::windowingsystem eq "win32"} {.popup.create configure -font menufont}

# add items to our submenu
# (wrapping the label like [_ ""] means that text will be localized)
.popup.create add command -label [_ "Object"] -command {popup_create_put "obj"}
.popup.create add command -label [_ "Message"] -command {popup_create_put "msg"}
.popup.create add command -label [_ "Number"] -command {popup_create_put "floatatom"}
.popup.create add command -label [_ "Symbol"] -command {popup_create_put "symbolatom"}
.popup.create add command -label [_ "Comment"] -command {popup_create_put "text"}
.popup.create add  separator
.popup.create add command -label [_ "Array"] -command {pdsend "$::focused_window menuarray"}

# insert our submenu as the 0th element on the popup
.popup insert 0 cascade -label [_ "Put"] -menu .popup.create
