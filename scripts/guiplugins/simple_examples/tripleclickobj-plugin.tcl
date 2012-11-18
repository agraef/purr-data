# This is a sketch to demonstrate a Tcl "plugin" for Pd: it binds to
# triple-clicks to trigger the creation of a new object.
proc process_tripleclick {window} {
	set mytoplevel [winfo toplevel $window] 
    if {[winfo class $mytoplevel] == "PatchWindow" && $::editmode($mytoplevel)} {
		::pd_connect::pdsend "$mytoplevel obj"
	}
}
bind all <Triple-ButtonRelease-1> {process_tripleclick %W}
