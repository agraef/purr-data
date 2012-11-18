# create an option for a mode where the Pd window pops up whenever anything is
# printed to the text box in the Pd window

set ::pdwindow::popupmode 0
rename pdtk_post pdtk_post_original
proc pdtk_post {message} {
    if {$::pdwindow::popupmode} {
        wm deiconify .pdwindow
        raise .pdwindow
    }
    pdtk_post_original $message
}

set mymenu .menubar.window
set inserthere [$mymenu index [_ "Parent Window"]]
$mymenu insert $inserthere separator
$mymenu insert $inserthere check -label [_ " Popup mode"] -variable ::pdwindow::popupmode


