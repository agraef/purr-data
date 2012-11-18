# this script searches a folder for Pd patches, then adds them to the bottom
# of the Media menu for quick opening

# you probably want to set this to something that makes sense for you:
set mypatch_dir ~/Documents/Pd

.menubar.media add separator

foreach filename [glob -directory $mypatch_dir -nocomplain -types {f} -- *.pd] {
    .menubar.media add command -label [file tail $filename] -command "open_file $filename"
}
