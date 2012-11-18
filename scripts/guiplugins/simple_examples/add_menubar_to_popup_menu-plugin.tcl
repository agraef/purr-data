# this scriptlet removes the menubars from all canvas windows.  You could also
# create your own custom menu and add it to the canvas windows.

# TODO ::pd_menus::update_window_menu expects to find ::patch_menubar not ""

# on Mac OS X, no windows have menubars, so no need to remove for 'aqua'
if {$::windowingsystem ne "aqua"} {
    set ::patch_menubar ""
}
    
set menulist "file edit put find media window help"
.popup add separator
foreach mymenu $menulist {   
    .menubar.$mymenu clone .popup.$mymenu normal
    .popup add cascade -label [_ [string totitle $mymenu]] \
        -menu .popup.$mymenu
}
