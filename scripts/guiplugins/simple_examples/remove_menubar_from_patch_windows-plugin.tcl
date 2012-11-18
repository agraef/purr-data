# this scriptlet removes the menubars from all canvas windows.  You could also
# create your own custom menu and add it to the canvas windows.

# on Mac OS X, no windows have menubars, so no need to remove for 'aqua'
if {$::windowingsystem ne "aqua"} {
    set ::canvas_menubar ""
}
