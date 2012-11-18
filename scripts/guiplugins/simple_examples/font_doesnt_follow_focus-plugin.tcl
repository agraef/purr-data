# this plugin disables the following of focused window by overriding a proc

rename ::dialog_font::update_font_dialog {}
proc ::dialog_font::update_font_dialog {mytoplevel} {
    variable canvaswindow
    if {[winfo exists .font]} {
        wm title .font [format [_ "%s Font"] [lookup_windowname $canvaswindow]]
    } else {
        set canvaswindow $mytoplevel
    }
}
