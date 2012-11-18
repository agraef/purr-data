# you can preset the histories of various things like the Tcl entry box in the
# Pd window and the messages in the Message dialog. They are lists of messages
# in order of newness, so the newest item is at the end of the list.

set ::pdwindow::tclentry_history {"menu_message_dialog" "console hide" "console show"}
set ::dialog_message::message_history {"pd dsp 1" "pd dsp 0" "my very custom message"}
