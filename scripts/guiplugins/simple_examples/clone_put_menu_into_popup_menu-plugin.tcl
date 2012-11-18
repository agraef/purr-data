

# for more info: http://tcl.tk/man/tcl8.5/TkCmd/menu.htm
.menubar.put clone .popup.create normal
.popup insert 0 cascade -label [_ "Put"] -menu .popup.create
.popup.create configure -tearoff 1
