#
# this plugin maps Ctrl-Shift-E (Cmd-Shift-E) to switch all open patches to
# Interact Mode (i.e. the opposite of Edit Mode)

proc global_turn_off_editmode {} {
    pdtk_post "$::pd_menus::accelerator-Shift-E: Turning off edit mode for all open windows"
    foreach window [winfo children .] {
        if {[winfo class $window] eq "PatchWindow"} {
            pdsend "$window editmode 0"
        }
    }
}

# the "E" needs to be upper case, otherwise it gets missed with the 'Shift'
bind all <$::modifier-Shift-Key-E> global_turn_off_editmode
