'use strict';

// Modules

var pwd;

exports.set_pwd = function(pwd_string) {
    pwd = pwd_string;
}

exports.get_pwd = function() {
    return pwd;
}

var fs = require('fs');     // for fs.existsSync
var path = require('path'); // for path.dirname path.extname

// local strings
var lang = require('./pdlang.js');

exports.get_local_string = lang.get_local_string;

var pd_window; 
exports.pd_window;

exports.set_pd_window = function(win) {
    pd_window = win;
    exports.pd_window = win;
}

var nw_create_window;
var nw_close_window;
var nw_app_quit;

exports.set_new_window_fn = function (nw_context_fn) {
    nw_create_window = nw_context_fn;
}

exports.set_close_window_fn = function (nw_context_fn) {
    nw_close_window = nw_context_fn;
}

// Global variables from tcl
var pd_myversion,    // Pd version string
    pd_apilist,      // Available Audio APIs (tcl list)
    pd_midiapilist,  // MIDI APIsa (tcl list)
    pd_nt,           // Something to do with Windows configuration
    fontname,        // Font
    fontweight,      //  config
    pd_fontlist,     //   (Seems to be hard coded in Pd-l2ork)
    pd_whichmidiapi, // MIDI API, set by pd->gui message
    pd_whichapi,     // Audio API, set by pd->gui message
    pd_opendir,      //
    pd_guidir,       //
    pd_tearoff,      //
    put_tearoff,     //
    tcl_version,     //
    canvas_fill,     //
    colors,          //
    global_clipboard, //
    global_selection, //
    k12_mode = 0,         // should be set from argv ('0' is just a stopgap)
    k12_saveas_on_new, //
    autotips,          // tooltips
    magicglass,        // cord inspector
    window_prefs,      //retaining window-specific preferences
    pdtk_canvas_mouseup_name, // not sure what this does
    filetypes,         // valid file extensions for opening/saving (includes Max filetypes)
    untitled_number,   // number to increment for each new patch that is opened
    untitled_directory, // default directory where to create/save new patches
    popup_coords,       // x/y for current popup window (global because there's only one at a time)
    pd_colors = {};                // associative array of canvas color presets

    var pd_filetypes = { ".pd": "Pd Files",
                         ".pat":"Max Patch Files",
                         ".mxt":"Max Text Files",
                         ".mxb":"Max Binary Files",
                         ".help":"Max Help Files"
                       };

    exports.k12_mode = k12_mode;
    exports.pd_filetypes = pd_filetypes;

    popup_coords = [0,0];

    var startup_files = []; // Array of files to be opened at startup (from the command line)

// Keycode vs Charcode: A Primer
// -----------------------------
// * keycode is a unique number assigned to a physical key on the keyboard
// * charcode is the ASCII character (printable or otherwise) that gets output
//     when you depress a particular key
// * keydown and keyup events report keycodes but not charcodes
// * keypress events report charcodes but not keycodes
// * keypress events do _not_ report non-ASCII chars like arrow keys,
//     Alt keypress, Ctrl, (possibly) the keypad Delete key, and others
// * in Pd, we want to send ASCII codes + arrow keys et al to Pd for
//     both keydown and keyup events
// * events (without an auto-repeat) happen in this order:
//       1) keydown
//       2) keypress
//       3) keyup
// Therefore...
// * solution #1: we check for non-ASCII keycodes like arrow keys inside
//     the keydown event
// * solution #2: in the keypress event, we map the charcode to the
//     last keydown keycode we received
// * solution #3: on keyup, we use the keycode to look up the corresponding
//     charcode, and send the charcode on to Pd
var pd_keymap = {}; // to iteratively map keydown/keyup keys
                    // to keypress char codes

function set_keymap(keycode, charcode) {
    pd_keymap[keycode] = charcode;
}

exports.set_keymap = set_keymap;

function get_char_code(keycode) {
    return pd_keymap[keycode];
}

exports.get_char_code = get_char_code;

    // Hard-coded Pd-l2ork font metrics
/*
var font_fixed_metrics = "\
8 5 11 \
9 6 12 \
10 6 13 \
12 7 16 \
14 8 17 \
16 10 19 \
18 11 22 \
24 14 29 \
30 18 37 \
36 22 44";
*/

// Let's try to get some metrics specific to Node-webkit...
    // Hard-coded Pd-l2ork font metrics
var font_fixed_metrics = "\
8 5 11 \
9 6 12 \
10 6 13 \
12 7 16 \
14 8 17 \
16 10 19 \
18 11 22 \
24 14 29 \
30 18 37 \
36 22 44";


// Utility Functions

// originally used to enquote a string to send it to a tcl function
function enquote (x) {
    var foo = x.replace(/,/g, "");
    foo = foo.replace(/;/g, "");
    foo = foo.replace(/"/g, "");
    foo = foo.replace(/ /g, "\\ ");
    foo = foo.trim();
    return foo;
}

// from stackoverflow.com/questions/21698906/how-to-check-if-a-path-is-absolute-or-relative
// this doesn't seem to be needed atm
function path_is_absolute(myPath) {
    var ret = (path.resolve(myPath) === path.normalize(myPath).replace(/(.+)([\/]\\])$/, '$1'));
    return ret;
}

function set_midiapi(val) {
    pd_whichmidiapi = val;
}

function set_audioapi(val) {
    pd_whichapi = val;
}

var last_string = "";
var last_child = {};
var duplicate = 0;

function gui_post(string, color) {
    if (last_string === string) {
        last_child.textContent = "[" + (duplicate + 2) + "] " + last_string;
        duplicate++;
    } else {
        if (color === undefined) { color = "black" };
        var myp = pd_window.document.getElementById('p1');
        var text;
        var span = pd_window.document.createElement("span");
        span.style.color = color;
        var text = pd_window.document.createTextNode(string); 
        span.appendChild(text);
        myp.appendChild(span);
        var printout = pd_window.document.getElementById("console_bottom");
        printout.scrollTop = printout.scrollHeight;

        last_string = string;
        last_child = span;
        duplicate = 0;
    }
}

exports.gui_post = gui_post;

function pd_error_select_by_id(objectid) {
    if (objectid !== null) {
        pdsend("pd findinstance " + objectid);
    }
}
exports.pd_error_select_by_id = pd_error_select_by_id

function gui_post_error(objectid, loglevel, errormsg) {
    var my_p = pd_window.document.getElementById('p1');
    // if we have an object id, make a friendly link...
    var error_title = pd_window.document.createTextNode("error");
    if (objectid.length > 0) {
        var my_a = pd_window.document.createElement('a');
        my_a.href = "javascript:pdgui.pd_error_select_by_id('" + objectid + "')";
        my_a.appendChild(error_title);
        my_p.appendChild(my_a);
    } else {
        my_p.appendChild(error_title);
    }
    var rest = pd_window.document.createTextNode(": " + errormsg + "\n");
    my_p.appendChild(rest);

    // looks like tcl/tk tried to throttle this... maybe we should, too...
    /*
    after cancel .printout.frame.text yview end-2char
    after idle .printout.frame.text yview end-2char
        .printout.frame.text configure -state disabled
    */
}

function menu_save(name) {
//    gui_post(name + " menusave");
    pdsend(name + " menusave");
}

exports.menu_save = menu_save;

function gui_canvas_saveas (name, initfile, initdir) {
    gui_post("working directory is " + pwd);
    //global pd_nt filetypes untitled_directory
    if (!fs.existsSync(initdir)) {
        initdir = pwd;
    }
//    patchwin[name].window.document.getElementById("fileDialog").setAttribute("nwworkingdir", pwd);
    var chooser = patchwin[name].window.document.querySelector('#saveDialog');
    chooser.click();
//    chooser.addEventListener("change", function(evt) {
//        saveas_callback(name, this.value);
//        console.log("tried to open something");
//    }, false);
}

function saveas_callback(cid, file) {
    gui_post("tried a saveas, and the file chosen is " + file);
    var filename = file;
    // It probably isn't possible to arrive at the callback with an
    // empty string.  But I've only tested on Debian so far...
    if (filename === null) {
        return;
    }
    // We don't need to use the codepath below because node-webkit
    // let's us specify the allowed files extensions.  Lo and behold,
    // nw just does the "right thing" whether the user types an extension
    // or not.  This should put us on part with Microsoft Word in the late
    // 90s.
    //var lc = filename.toLowerCase();
    //if (lc.slice(-3) !== '.pd' &&
    //    lc.slice(-4) !== '.pat' &&
    //    lc.slice(-4) !== '.mxt') {
        // remove any other extensions
    //    filename = filename.slice(0,
    //        (filename.length - path.extname(filename).length));
        // add ".pd"
    //    filename = filename + '.pd';
    //}
    // test again after downcasing and maybe adding a ".pd" on the end
    //if (fs.existsSync(filename)) {
    //    var reply = patchwin[cid].window.confirm(filename +
    //        " already exists. Do you want to replace it?");
    //    if (!reply) {
    //        return;
    //    }
    //}
    var directory = path.dirname(filename);
    var basename = path.basename(filename);
    pdsend(cid + " savetofile " + enquote(basename) + " " +
        enquote(directory));

    // haven't implemented these last few commands yet...
    // set untitled_directory $directory
    // add to recentfiles
    //::pd_guiprefs::update_recentfiles "$filename" 1
}

exports.saveas_callback = saveas_callback;

function menu_saveas(name) {
    pdsend(name + " menusaveas");
}

exports.menu_saveas = menu_saveas;

function menu_new () {
    //if { ! [file isdirectory $untitled_directory]} {set untitled_directory $::env(HOME)}
    untitled_directory = pwd;
    pdsend("pd filename " + "Untitled-" + untitled_number + " " + enquote(untitled_directory));
    if (k12_mode == 1) {
        k12_saveas_on_new = 1;
        pdsend("#N canvas");
        pdsend("#X obj -30 -30 preset_hub k12 1 %hidden%");
        pdsend("#X pop 1");
    } else {
        pdsend("#N canvas");
        pdsend("#X pop 1");
    }
    untitled_number++;
}

exports.menu_new = menu_new;

function gui_close_window(cid) {
    nw_close_window(patchwin[cid]);
}

function menu_k12_open_demos () {

}

exports.menu_k12_open_demos = menu_k12_open_demos;


function menu_open (filenames_string) {
    gui_post("menu_open " + filenames_string);
    var file_array = filenames_string.split(";");
    var length = file_array.length;
    gui_post("file_array is " + file_array);
    for (var i = 0; i < length; i++) {
        open_file(file_array[i]);
    }
}

exports.menu_open = menu_open;

function menu_close(name) {
    // not handling the "text editor" yet
    // not handling the "Window" menu yet
    //pdtk_canvas_checkgeometry $name
    pdsend(name + " menuclose 0");
}

exports.menu_close = menu_close;

function canvas_menuclose_callback(cid_for_dialog, cid, force) {
    // Hacky-- this should really be dir/filename here instead of
    // filename/args/dir which is ugly
    var title = patchwin[cid_for_dialog].window.document.title;
    var reply = patchwin[cid_for_dialog].window.confirm("Save changes to " + title + "?");
    if (reply) {
        pdsend(cid_for_dialog + " menusave");
    } else {
        pdsend(cid_for_dialog + " dirty 0");        
        pdsend(cid + " menuclose " + force);
    }
}

function gui_canvas_menuclose(cid_for_dialog, cid, force) {
    // Hack to get around a renderer bug-- not guaranteed to work
    // for long patches
    setTimeout(function() {
            canvas_menuclose_callback(cid_for_dialog, cid, force);
        }, 450);
}

function gui_pd_quit_dialog() {
    var reply = pd_window.window.confirm("Really quit?");
    if (reply === true) {
        pdsend("pd quit");
    }
}

// send a message to Pd
function menu_send() {
    gui_post("message...pdwindow is " + pd_window);
    var message = pd_window.window.prompt("Type a message to send to Pd");
    if (message != undefined && message.length) {
        gui_post("Sending message to Pd: " + message + ";");
        pdsend(message);
    }
}

exports.menu_send = menu_send;

function menu_quit() {
    pdsend("pd verifyquit");
}

exports.menu_quit = menu_quit;

var nw_app_quit;

function app_quit() {
    nw_app_quit();
}

exports.set_app_quitfn = function(quitfn) {
    nw_app_quit = quitfn;
} 

function open_file(file) {
    //from tcl...
    //global pd_opendir pd_guidir pd_nt
    gui_post("open_file: " + file);
    var directory = path.dirname(file);
    var basename = path.basename(file);
    var cyclist;
    if (basename.match(/\.(pat|mxb|help)$/) !=null) {
        gui_post("warning: opening pat|mxb|help not implemented yet");
        //gui_post("converting " + filename);
        if (pd_nt == 0) {
            // on GNU/Linux, cyclist is installed into /usr/bin usually
            cyclist = "/usr/bin/cyclist";
        } else {
            cyclist = pd_guidir + "/bin/cyclist"
        }
        //gui_post(cyclist + filename);
        //convert Max binary to text .pat
        // The following is tcl code which needs to get converted to javascript...
        //set binport [open "| \"$cyclist\" \"$filename\""]
        //set convertedtext [read $binport]
        //if { ! [catch {close $binport} err]} {
        //    if {! [file writable $directory]} {     set directory "/tmp" }
        //    set basename "$basename.pat"
        //    set textpatfile [open "$directory/$basename" w]
        //    puts $textpatfile $convertedtext
        //    close $textpatfile
        //    puts stderr "converted Max binary to text format: $directory/$basename"
        //}
    }
    if (basename.match(/\.(pd|pat|mxt)$/i) != null) {
        pdsend("pd open" + " " + enquote(basename) + " " + enquote(directory));
        pd_opendir = directory;
        //::pd_guiprefs::update_recentfiles "$filename" 1
    }
}

// Doesn't work yet... need to figure out how to send command line args (files) to be opened by
// the unique instance 
function gui_open_files_via_unique(filenames)
{
    gui_post("pdtk_open_files_via_unique " + filenames);
    length = filenames.length;
    if (length != 0) {
        for (var i = 0; i < length; i++) {
            var file = filenames[i];
            //gui_post("open_file " + file);
            open_file(file);
        }
    }
}

function gui_build_filelist(file) {
    startup_files.push(file);
}

// This doesn't work at the moment.  Not sure how to feed the command line filelist to a single
// instance of node-webkit.
function gui_check_unique (unique) {
        // gui_post("pdtk_check_unique " + unique);
    // global appname
    return;
    var final_filenames = new Array;
    var startup_dir = pwd;
    if (unique == 0) {
        var filelist_length = startup_files.length;
        for (var i = 0; i < filelist_length; i++) {
            var file = startup_files[i];
            var dir;
            //gui_post (file [file dirname $file] $startup_dir"
            if (!pathIsAbsolute(file)) {
                file = fs.join(pwd, file);
            }
            final_filenames.push(file);  
        }
        gui_open_files_via_unique(final_filenames);
    }
    // old tcl follows...
	//if path is relative
	//then join pwd and relative path
	//else
	//use the absolute path
	//(no need to check for existence here)

	//            catch {cd [file dirname $file]}
	//            set dir [pwd]

	//file tail should be the filename

	//            set name [file tail $file]
	//            #puts stderr "********DIR:$dir FILE:$name COMBINED:[file join $dir $name]"
	//            lappend final_filenames [file join $dir $name]
	//            cd $startup_dir
	//        }
	//        #puts stderr "send pd-l2ork pdtk_open_files_via_unique $final_filenames"
	//        set outcome [catch {send pd-l2ork pdtk_open_files_via_unique \{$final_filenames\}}]
	//        #puts stderr "outcome = $outcome"
	//        if { $outcome == 0 } {
	//                menu_really_quit
	//                exit
	//        }
	//}
	//        tk appname $appname
	//        #puts stderr "this is unique instance [tk appname]"
}



function gui_startup(version, fontname_from_pd, fontweight_from_pd,
    apilist, midiapilist) {
    console.log("we're starting up...");
    // # tb: user defined typefaces
    // our args:
//    console.log(version);
//    console.log(apilist);
//    console.log(fontname);

    // set some global variables
    pd_myversion = version;
    pd_apilist =  apilist;
    pd_midiapilist = midiapilist;

    fontname = fontname_from_pd;
    fontweight = fontweight_from_pd;
    pd_fontlist = "";
    untitled_number = 1; // global variable to increment for each new patch

    // From tcl, not sure if needed...
       // # on Mac OS X, lower the Pd window to the background so patches open on top
       // if {$pd_nt == 2} { lower . }
       // # on Windows, raise the Pd window so that it has focused when launched
       // if {$pd_nt == 1} { raise . }

//    set fontlist ""
//        if {[info tclversion] >= 8.5} {find_default_font}
//        set_base_font $fontname_from_pd $fontweight_from_pd
//        fit_font_into_metrics

//    # UBUNTU MONO 6 6 8 10 11 14 14 19 22 30
//        # DEJAVU SANS MONO 6 6 8 9 10 12 14 18 22 29

//#    foreach i {6 6 8 10 11 14 14 19 22 30} {
//#        set font [format {{%s} %d %s} $fontname_from_pd $i $fontweight_from_pd]
//#        set pd_fontlist [linsert $pd_fontlist 100000 $font]
//#        set width0 [font measure  $font x]
//#        set height0 [lindex [font metrics $font] 5]
//#        set fontlist [concat $fontlist $i [font measure  $font x] \
//#                          [lindex [font metrics $font] 5]]
//#    }

//    set tclpatch [info patchlevel]
//    if {$tclpatch == "8.3.0" || \
//            $tclpatch == "8.3.1" || \
//            $tclpatch == "8.3.2" || \
//            $tclpatch == "8.3.3" } {
//        set oldtclversion 1
//    } else {
//        set oldtclversion 0
//    }
    pdsend("pd init " + enquote(pwd) + " 0 " + font_fixed_metrics);

//    # add the audio and help menus to the Pd window.  We delayed this
//    # so that we'd know the value of "apilist".
//    menu_addstd .mbar

//    global pd_nt
//    if {$pd_nt == 2} {
//        global pd_macdropped pd_macready
//        set pd_macready 1
//        foreach file $pd_macdropped {
//            pd [concat pd open [pdtk_enquote [file tail $file]] \
//                    [pdtk_enquote  [file dirname $file]] \;]
//            menu_doc_open [file dirname $file] [file tail $file]
//        }
//    }
}





 

/*
    // Some other menu
    var fooMenu = new nw.Menu();

    // Add to window menu
    windowMenu.append(new nw.MenuItem({
        label: 'Foo',
        submenu: fooMenu
    }));

    // Foo sub-entry
    fooMenu.append(new nw.MenuItem({
        label: 'flub',
        click: function(){
            alert('Foo flub');
        }
    }));

    // Another Foo sub-entry
    fooMenu.append(new nw.MenuItem({
        label: 'bub',
        click: function(){
            alert('Foo bub');
        }
    }));
}


/*
function canvas_create_menus(name) {
    // the "File" menu
	
    // The menus are instantiated here for the patch windows.
    // For the main window, they are created on load, at the 
    // top of this file.
    match_linux_wm [list menu $name.m -relief flat]
    match_linux_wm [list menu $name.m.file  -postcommand [concat pdtk_fixfilemenu $name.m.file] -tearoff $pd_tearoff]
    $name.m add cascade -label File -menu $name.m.file

    $name.m.file add command -label New -command {menu_new} \
        -accelerator [accel_munge "Ctrl+n"]

    $name.m.file add command -label Open -command {menu_open} \
        -accelerator [accel_munge "Ctrl+o"]

	if { $k12_mode == 1 } {
		$name.m.file add command -label {K12 Demos} -command {menu_k12_open_demos}
	}

    match_linux_wm [list $name.m.file add separator]

    $name.m.file add command -label Save -command [concat menu_save $name] \
        -accelerator [accel_munge "Ctrl+s"]

    $name.m.file add command -label "Save as..." \
        -command [concat menu_saveas $name] \
        -accelerator [accel_munge "Ctrl+S"]
	if { $k12_mode == 0 } {
		match_linux_wm [list $name.m.file add separator]

		# arrange menus according to Apple HIG
		if {$pd_nt != 2 } {
			$name.m.file add command -label "Message..." -command {menu_send} \
				-accelerator [accel_munge "Ctrl+m"]
			# these are now part of Preferences... on Mac OS X
		    $name.m.file add command -label Path... \
		        -command {pd pd start-path-dialog \;} 
		    $name.m.file add command -label Startup... \
		        -command {pd pd start-startup-dialog \;} 
		} else { 
			# Cmd-m is minimize window on Mac OS X		
			$name.m.file add command -label "Message..." -command {menu_send}
			match_linux_wm [list $name.m.file add  separator]
			$name.m.file add command -label "Make app from patch..." \
				-command {menu_makeapp 0}
			$name.m.file add command -label "Make app from folder..." \
				-command {menu_makeapp 1}
		}
		match_linux_wm [list $name.m.file add separator]
		$name.m.file add command -label "Print..." -command [concat menu_print $name] \
		    -accelerator [accel_munge "Ctrl+p"]
	}
    # update recent files
    match_linux_wm [list $name.m.file add separator]
    $name.m.file add command -label "No Recent Files" -state disabled
    #match_linux_wm [list $name.m.file add separator]
    #if {[llength $::recentfiles_list] > 0} {
    #    ::pd_menus::update_recentfiles_menu $name.m.file false
    #}

	match_linux_wm [list $name.m.file add separator]
    $name.m.file add command -label Close \
        -command [concat menu_close $name] \
        -accelerator [accel_munge "Ctrl+w"]

	if {$pd_nt != 2} {
		# Mac OS X doesn't put Quit on the File menu
		$name.m.file add command -label Quit -command {menu_quit} \
			-accelerator [accel_munge "Ctrl+q"]
	}

    # the "Edit" menu
    match_linux_wm [list menu $name.m.edit -postcommand [concat menu_fixeditmenu $name] -tearoff $pd_tearoff]
    $name.m add cascade -label Edit -menu $name.m.edit
    
    $name.m.edit add command -label Undo -command [concat menu_undo $name] \
        -accelerator [accel_munge "Ctrl+z"]

    $name.m.edit add command -label Redo -command [concat menu_redo $name] \
        -accelerator [accel_munge "Ctrl+Z"]

    match_linux_wm [list $name.m.edit add separator]

    $name.m.edit add command -label Cut -command [concat menu_cut $name] \
        -accelerator [accel_munge "Ctrl+x"] -state disabled

    $name.m.edit add command -label Copy -command [concat menu_copy $name] \
        -accelerator [accel_munge "Ctrl+c"] -state disabled

    $name.m.edit add command -label Paste \
        -command [concat menu_paste $name] \
        -accelerator [accel_munge "Ctrl+v"]

	if {!$global_clipboard} {
		$name.m.edit entryconfigure "Paste" -state disabled
	} else {
		$name.m.edit entryconfigure "Paste" -state normal
	}

    $name.m.edit add command -label Duplicate \
        -command [concat menu_duplicate $name] \
        -accelerator [accel_munge "Ctrl+d"]

	if {!$global_selection} {
		$name.m.edit entryconfigure "Duplicate" -state disabled
	} else {
		$name.m.edit entryconfigure "Duplicate" -state normal
	}

    $name.m.edit add command -label {Select all} \
        -command [concat menu_selectall $name] \
        -accelerator [accel_munge "Ctrl+a"]

	if { $k12_mode == 0 } {
		$name.m.edit add command -label {Reselect} \
		    -command [concat menu_reselect $name] \
		    -accelerator "Ctrl+Enter" -state disabled
	}

    match_linux_wm [list $name.m.edit add separator]

	$name.m.edit add command -label {Tidy Up} \
		-command [concat menu_tidyup $name] \
	    -accelerator [accel_munge "Ctrl+y"] -state disabled

	if { $k12_mode == 0 } {

		$name.m.edit add command -label {Bring To Front} \
			-command [concat popup_action $name 3] \
		    -accelerator [accel_munge "Ctrl+Up"] -state disabled

		$name.m.edit add command -label {Send To Back} \
			-command [concat popup_action $name 4] \
		    -accelerator [accel_munge "Ctrl+Down"] -state disabled
	}

	match_linux_wm [list $name.m.edit add separator]

	if { $k12_mode == 0 } {

		#if {$pd_nt == 2} { # no key command on Mac OS X, conflicts with standard
		#	$name.m.edit add command -label {Text Editor} \
		#		-command [concat menu_texteditor $name]
		#} else {
		#	$name.m.edit add command -label {Text Editor} \
		#		-accelerator [accel_munge "Ctrl+t"] \
		#		-command [concat menu_texteditor $name]
		#}

		$name.m.edit add command -label Font \
		    -command [concat menu_font $name] 
	}

## jsarlo
    $name.m.edit add checkbutton -label "Cord Inspector" \
        -indicatoron false -selectcolor black \
        -command [concat menu_magicglass $name] \
        -accelerator [accel_munge "Ctrl+r"]

    #if { $editable == 0 } {
    #       $name.m.edit entryconfigure "Cord Inspector" -indicatoron false }
  
    match_linux_wm [list $name.m.edit add separator]
## end jsarlo

	$name.m.edit add command -label "Toggle console" \
	    -accelerator [accel_munge "Ctrl+R"] \
	    -command [concat .controls.switches.console invoke]

	$name.m.edit add command -label "Clear console" \
	    -accelerator [accel_munge "Ctrl+L"] \
	    -command [concat menu_clear_console]

	match_linux_wm [list $name.m.edit add separator]

	if { $k12_mode == 0 } {
    
		# Apple, Microsoft, and others put find functions in the Edit menu.
		$name.m.edit add command -label {Find...} \
		    -accelerator [accel_munge "Ctrl+f"] \
		    -command [concat menu_findobject $name] 
		$name.m.edit add command -label {Find Again} \
		    -accelerator [accel_munge "Ctrl+g"] \
		    -command [concat menu_findagain $name] 
		$name.m.edit add command -label {Find last error} \
		    -command [concat menu_finderror] 

		match_linux_wm [list $name.m.edit add separator]

		############iemlib##################
		# instead of "red = #BC3C60" we take "grey85", so there is no difference,
		# if widget is selected or not.

		$name.m.edit add checkbutton -label "Autotips" \
		    -indicatoron false -selectcolor black \
		    -command [concat menu_tooltips $name] \
		    -accelerator [accel_munge "Ctrl+E"]
	}

    $name.m.edit add checkbutton -label "Edit mode" \
        -indicatoron false -selectcolor black \
        -command [concat menu_editmode $name] \
        -accelerator [accel_munge "Ctrl+e"]
    if {$k12_mode == 0} {
    	match_linux_wm [list $name.m.edit add separator]
        $name.m.edit add command -label {Preferences...} \
        -command {::dialog_prefs::open_prefs_dialog .}
    }

	if { $editable == 1 } {
    	$name.m.edit entryconfigure "Edit mode" -background "#7dd37d" -foreground black
	}

	if { $k12_mode == 0 && $autotips == 1 } {
    	$name.m.edit entryconfigure "Autotips" -background "#7dd37d" -foreground "#dddddd"
	}

	set ::editmode($name) $editable

	#if { $magicglass == 1 } {
    #	$name.m.edit entryconfigure "Cord Inspector" -background "#7dd37d"
	#	menu_magicglass $name
	#}

    #if { $editable == 0 } {
    #    $name.m.edit entryconfigure "Edit mode" -indicatoron false 
    #}

*/



// From what used to be canvas.js

// Global canvas associative arrays (aka javascript objects)
var scroll = {},
    menu = {},
    canvas_color = {},
    topmost = {},
    resize = {},
    xscrollable = {},
    yscrollable = {},
    update_tick = {},
    drag_tick = {},
    undo = {},
    redo = {},
    font = {},
    doscroll = {},
    last_loaded,
    loaded = {},
    popup_menu = {};

    var patchwin = {}; // object filled with cid: [Window object] pairs
    var dialogwin = {}; // object filled with did: [Window object] pairs

exports.get_patchwin = function(name) {
    return patchwin[name];
}

exports.get_dialogwin = function(name) {
    return dialogwin[name];
}

exports.remove_dialogwin = function(name) {
    dialogwin[name] = null;
}


// stopgap...
pd_colors['canvas_color'] = "white";

exports.last_loaded = function () {
    return last_loaded;
}

// close a canvas window

function gui_canvas_cursor(cid, pd_event_type) {
    var patch = get_item(cid, "patchsvg");
    var c;
    // A quick mapping of events to pointers-- these can
    // be revised later
    switch(pd_event_type) {
        case "cursor_runmode_nothing":
            c = 'default';
            break;
        case "cursor_runmode_clickme":
            c = 'pointer';
            break;
        case "cursor_runmode_thicken":
            c = 'inherit';
            break;
        case "cursor_runmode_addpoint":
            c = 'cell';
            break;
        case "cursor_editmode_nothing":
            c = 'pointer';
            break;
        case "cursor_editmode_connect":
            c = '-webkit-grabbing';
            break;
        case "cursor_editmode_disconnect":
            c = 'no-drop';
            break;
        case "cursor_editmode_resize":
            c = 'ew-resize';
            break;
        case "cursor_editmode_resize_bottom_right":
            c = 'se-resize';
            break;
        case "cursor_scroll":
            c = 'all-scroll'; 
            break;
    }
    patch.style.cursor = c;
}

function gui_canvas_sendkey(cid, state, evt, char_code) {
    pdsend(
        cid + " key " +
        state + " " +
        char_code + " " +
        (evt.shiftKey ? 1 : 0) + " " +
        1 + " " +
        (evt.repeat ? 1 : 0)
    );
}

exports.gui_canvas_sendkey = gui_canvas_sendkey;

function title_callback(cid, title) {
    patchwin[cid].window.document.title = title;
}

function format_window_title(name, dirty_flag, args, dir) {
        return name + " " + (dirty_flag ? "*" : "") + args + " - " + dir;
}

exports.format_window_title = format_window_title;

// This should be used when a file is saved with the name changed (and maybe in other situations)
function gui_canvas_set_title(cid, name, args, dir, dirty_flag) {
    var title = format_window_title(name, dirty_flag, args, dir);
    patchwin[cid].title = title;
}

// create a new canvas
// todo: rename parameter "name" to "cid"
function gui_canvas_new(cid, width, height, geometry, editable, name, dir, dirty_flag, cargs) {
    // hack for buggy tcl popups... should go away for node-webkit
    //reset_ctrl_on_popup_window

    // local vars for window-specific behavior
    // visibility of menu and scrollbars, plus canvas background
    scroll[cid] = 1;
    menu[cid] = 1;
    // attempt at getting global presets to play
    // well with local settings.
    var my_canvas_color = "";
    //canvas_color[cid] = orange;
    my_canvas_color = pd_colors['canvas_color'];
    topmost[cid] = 0;
    resize[cid] = 1;
    xscrollable[cid] = 0;
    yscrollable[cid] = 0;
    update_tick[cid] = 0;
    drag_tick[cid] = 0;
    undo[cid] = false;
    redo[cid] = false;
    font[cid] = 10;
    doscroll[cid] = 0;
    // geometry is just the x/y screen offset "+xoff+yoff"
    geometry = geometry.slice(1);   // remove the leading "+"
    geometry = geometry.split("+"); // x/y screen offset (in pixels)
    // Keep patches on the visible screen
    var xpos = Math.min(Number(geometry[0]), window.screen.height - width); 
    var ypos = Math.min(Number(geometry[1]), window.screen.height - height); 
    xpos = Math.max(xpos, 0);
    ypos = Math.max(ypos, 0);
    var menu_flag;
    if (menu[cid] == 1) {
        menu_flag = true;
    } else {
        menu_flag = false;
    }
    last_loaded = cid;
    // Not sure why resize and topmost are here-- but we'll pass them on for the time being...
    patchwin[cid] = nw_create_window(cid, 'pd_canvas', width, height, xpos, ypos, menu_flag,
        resize[cid], topmost[cid], my_canvas_color, name, dir, dirty_flag, cargs, null);
     
    // initialize variable to reflect that this window has been opened
    loaded[cid] = 1;
    //#pdtk_standardkeybindings $cid.c
}

function canvas_map(name) {
    console.log("canvas mapping " + name + "...");
    pdsend(name + " map 1");
}

function gui_canvas_erase_all_gobjs(cid) {
    var top = get_item(cid, 'patchsvg');
    var elem;
    while (elem = top.firstChild) {
        top.removeChild(elem);
    }
}

exports.canvas_map = canvas_map;

/*    
    ############iemlib##################

	if { $k12_mode == 0 } {

		# the "Put" menu
		match_linux_wm [list menu $name.m.put -tearoff $put_tearoff]
		$name.m add cascade -label Put -menu $name.m.put

		$name.m.put add command -label Object \
		    -command [concat menu_object $name 0] \
		    -accelerator [accel_munge "Ctrl+1"]

		$name.m.put add command -label Message \
		    -command [concat menu_message $name 0] \
		    -accelerator [accel_munge "Ctrl+2"]

		$name.m.put add command -label Number \
		    -command [concat menu_floatatom $name 0] \
		    -accelerator [accel_munge "Ctrl+3"]

		$name.m.put add command -label Symbol \
		    -command [concat menu_symbolatom $name 0] \
		    -accelerator [accel_munge "Ctrl+4"]

		$name.m.put add command -label Comment \
		    -command [concat menu_comment $name 0] \
		    -accelerator [accel_munge "Ctrl+5"]

		match_linux_wm [list $name.m.put add separator]
		
		############iemlib##################

		$name.m.put add command -label Bang \
		    -command [concat menu_bng $name 0] \
		    -accelerator [accel_munge "Shift+Ctrl+b"]
		
		$name.m.put add command -label Toggle \
		    -command [concat menu_toggle $name 0] \
		    -accelerator [accel_munge "Shift+Ctrl+t"]
		
		$name.m.put add command -label Number2 \
		    -command [concat menu_numbox $name 0] \
		    -accelerator [accel_munge "Shift+Ctrl+n"]
		
		$name.m.put add command -label Vslider \
		    -command [concat menu_vslider $name 0] \
		    -accelerator [accel_munge "Shift+Ctrl+v"]
		
		$name.m.put add command -label Hslider \
		    -command [concat menu_hslider $name 0] \
		    -accelerator [accel_munge "Shift+Ctrl+h"]
		
		$name.m.put add command -label Vradio \
		    -command [concat menu_vradio $name 0] \
		    -accelerator [accel_munge "Shift+Ctrl+d"]
		
		$name.m.put add command -label Hradio \
		    -command [concat menu_hradio $name 0] \
		    -accelerator [accel_munge "Shift+Ctrl+i"]
		
		$name.m.put add command -label VU \
		    -command [concat menu_vumeter $name 0] \
		    -accelerator [accel_munge "Shift+Ctrl+u"]
		
		$name.m.put add command -label Canvas \
		    -command [concat menu_mycnv $name 0] \
		    -accelerator [accel_munge "Shift+Ctrl+c"]

		############iemlib##################
		
		match_linux_wm [list $name.m.put add separator]
		
		$name.m.put add command -label Graph \
		    -command [concat menu_graph $name] 

		$name.m.put add command -label Array \
		    -command [concat menu_array $name] 

		# the find menu
		# Apple, Microsoft, and others put find functions in the Edit menu.
		# But in order to move these items to the Edit menu, the Find menu
		# handling needs to be dealt with, including this line in g_canvas.c:
		#         sys_vgui(".mbar.find delete %d\n", i);
		# <hans@at.or.at>
		#match_linux_wm [list menu $name.m.find -tearoff $put_tearoff]
		#$name.m add cascade -label Find -menu $name.m.find
		#
		#$name.m.find add command -label {Find...} \
		#    -accelerator [accel_munge "Ctrl+f"] \
		#    -command [concat menu_findobject $name] 
		#$name.m.find add command -label {Find Again} \
		#    -accelerator [accel_munge "Ctrl+g"] \
		#    -command [concat menu_findagain $name] 
		#$name.m.find add command -label {Find last error} \
		#    -command [concat menu_finderror] 
		
		# the window menu
		match_linux_wm [list menu $name.m.windows -postcommand \
			[concat menu_fixwindowmenu $name] -tearoff $pd_tearoff]

		if {$pd_nt == 2} {
			$name.m.windows add command -label {Minimize} \
				-command "menu_minimize $name" -accelerator [accel_munge "Ctrl+m"]
			$name.m.windows add command -label {Zoom} -command "menu_zoom $name"
		} else {
			$name.m.windows add command -label "Next Window" -command {menu_raisenextwindow} \
				-accelerator "Ctrl+PageDown"
			$name.m.windows add command -label "Previous Window" -command {menu_raisepreviouswindow} \
				-accelerator "Ctrl+PageUp"
		}
		match_linux_wm [list $name.m.windows add separator]
		$name.m.windows add command -label {parent window}\
		    -command [concat menu_windowparent $name] 
		$name.m.windows add command -label {Pd & Console} -command menu_raise_console \
			-accelerator [accel_munge "Ctrl+;"]
		match_linux_wm [list $name.m.windows add separator]

		# the audio menu
		match_linux_wm [list menu $name.m.audio -tearoff $pd_tearoff]

		if {$pd_nt != 2} {
		    $name.m add cascade -label Windows -menu $name.m.windows
		    $name.m add cascade -label Media -menu $name.m.audio
		} else {
		    $name.m add cascade -label Media -menu $name.m.audio
		    $name.m add cascade -label Window -menu $name.m.windows
		    # the MacOS X app menu
		    menu $name.m.apple -tearoff $pd_tearoff
		    $name.m add cascade -label "Apple" -menu $name.m.apple 
		}

		# the "Help" menu

		match_linux_wm [list menu $name.m.help -tearoff $pd_tearoff]
		$name.m add cascade -label Help -menu $name.m.help

		menu_addstd $name.m
	}

    # the popup menu
	match_linux_wm [list menu $name.popup -tearoff false]
	if { $k12_mode == 0 } {
		$name.popup add command -label {Properties} \
		    -command [concat popup_action $name 0]
		$name.popup add command -label {Open} \
		    -command [concat popup_action $name 1]
		$name.popup add command -label {Help} \
		    -command [concat popup_action $name 2]
		match_linux_wm [list $name.popup add separator]
		$name.popup add command -label {To Front} \
		    -command [concat popup_action $name 3]
		$name.popup add command -label {To Back} \
		    -command [concat popup_action $name 4]
	} else {
		$name.popup add command -label {Properties} -state disabled \
		    -command [concat popup_action $name 0] 
		$name.popup add command -label {Open} -state disabled \
		    -command [concat popup_action $name 1]
		$name.popup add command -label {Help} \
		    -command [concat popup_action $name 2]
		match_linux_wm [list $name.popup add separator]
		$name.popup add command -label {To Front} -state disabled \
		    -command [concat popup_action $name 3]
		$name.popup add command -label {To Back} -state disabled \
		    -command [concat popup_action $name 4]
	}

    # fix menu font size on Windows with tk scaling = 1
    if {$pd_nt == 1} {
        $name.m.file configure -font menuFont
        $name.m.edit configure -font menuFont
        $name.m.find configure -font menuFont
        $name.m.put configure -font menuFont
        $name.m.windows configure -font menuFont
        $name.m.audio configure -font menuFont
        $name.m.help configure -font menuFont
        $name.popup configure -font menuFont
    }

    # WM protocol
    wm protocol $name WM_DELETE_WINDOW [concat menu_close $name]

    # bindings.
    # this is idiotic -- how do you just sense what mod keys are down and
    # pass them on? I can't find it anywhere.
    # Here we encode shift as 1, control 2, alt 4, in agreement
    # with definitions in g_canvas.c.  The third button gets "8" but we don't
    # bother with modifiers there.
    # We don't handle multiple clicks yet.

    bind $name.c <Configure> {pdtk_canvas_getscroll %W}
	#bind $name.c <Configure> {after 100 pdtk_canvas_getscroll_configure %W}
    bind $name.c <Button> {pdtk_canvas_click %W %x %y %b 0}
    bind $name.c <Shift-Button> {pdtk_canvas_click %W %x %y %b 1}
    bind $name.c <Control-Shift-Button> {pdtk_canvas_click %W %x %y %b 3}
    # Alt key is called Option on the Mac
    if {$pd_nt == 2} {
        bind $name.c <Option-Button> {pdtk_canvas_click %W %x %y %b 4}
        bind $name.c <Option-Shift-Button> {pdtk_canvas_click %W %x %y %b 5}
        bind $name.c <Option-Control-Button> {pdtk_canvas_click %W %x %y %b 6}
        bind $name.c <Mod1-Button> {pdtk_canvas_click %W %x %y %b 6}
        bind $name.c <Option-Control-Shift-Button> \
            {pdtk_canvas_click %W %x %y %b 7}
    } else {
        bind $name.c <Alt-Button> {pdtk_canvas_click %W %x %y %b 4}
        bind $name.c <Alt-Shift-Button> {pdtk_canvas_click %W %x %y %b 5}
        bind $name.c <Alt-Control-Button> {pdtk_canvas_click %W %x %y %b 6}
        bind $name.c <Alt-Control-Shift-Button> \
            {pdtk_canvas_click %W %x %y %b 7}
    }
    # button 2 is the right button on Mac; on other platforms it's button 3.
    if {$pd_nt == 2} {
        bind $name.c <Button-2> {pdtk_canvas_rightclick %W %x %y %b}
        bind $name.c <Control-Button> {pdtk_canvas_rightclick %W %x %y %b}
    } else {
        bind $name.c <Button-3> {pdtk_canvas_rightclick %W %x %y %b}
        bind $name.c <Control-Button> {pdtk_canvas_click %W %x %y %b 2}
    }
    #on linux, button 2 "pastes" from the X windows clipboard
    if {$pd_nt == 0} {
        bind $name.c <Button-2> {pdtk_canvas_middleclick %W %x %y %b 0;}
    }

    bind $name.c <ButtonRelease> {pdtk_canvas_mouseup %W %x %y %b}
    bind $name.c <Control-Key> {pdtk_canvas_ctrlkey %W %K 0}
    bind $name.c <Control-Shift-Key> {pdtk_canvas_ctrlkey %W %K 1}
    #    bind $name.c <Mod1-Key> {puts stderr [concat mod1 %W %K %A]}
    if {$pd_nt == 2} {
        bind $name.c <Mod1-Key> {pdtk_canvas_ctrlkey %W %K 0}
        bind $name.c <Mod1-Shift-Key> {pdtk_canvas_ctrlkey %W %K 1}
        bind $name.c <Mod1-BackSpace> {pdtk_canvas_sendkey %W 1 %K %A 0 1 %t}
        bind $name.c <Mod1-quoteleft> {menu_raisenextwindow}
    } else {
        bind $name.c <Control-Next>   {menu_raisenextwindow}
        bind $name.c <Control-Prior>  {menu_raisepreviouswindow} ;# needs Tcl/Tk 8.5
	}
    bind $name.c <Key> {pdtk_canvas_sendkey %W 1 %K %A 0 1 %t}
    bind $name.c <Shift-Key> {pdtk_canvas_sendkey %W 1 %K %A 1 1 %t}
    bind $name.c <KeyRelease> {pdtk_canvas_sendkey %W 0 %K %A 0 1 %t}
    bind $name.c <Motion> {pdtk_canvas_motion %W %x %y 0}
    bind $name.c <Shift-Motion> {pdtk_canvas_motion %W %x %y 1}
    bind $name.c <Control-Motion> {pdtk_canvas_motion %W %x %y 2}
    bind $name.c <Control-Shift-Motion> {pdtk_canvas_motion %W %x %y 3}

    # canvas bindings ---------------------------------------------------------
    # just for tooltips right now
	#$name.c bind all <Enter> "puts stderr {%x %y}"
    #$name.c bind inlet <Enter> "pdtk_canvas_enteritem %W %x %y inlet %t"
    #$name.c bind outlet <Enter> "pdtk_canvas_enteritem %W %x %y outlet %t"
    #$name.c bind text <Enter> "pdtk_canvas_enteritem %W %x %y text %t"
    #$name.c bind inlet <Leave> "pdtk_canvas_leaveitem %W inlet 0"
    #$name.c bind outlet <Leave> "pdtk_canvas_leaveitem %W outlet 0"
    #$name.c bind text <Leave> "pdtk_canvas_leaveitem %W text 0"
	
    if {$pd_nt == 2} {
        bind $name.c <Option-Motion> {pdtk_canvas_motion %W %x %y 4}
    } else { 
        bind $name.c <Alt-Motion> {pdtk_canvas_motion %W %x %y 4}
    }
    bind $name.c <Map> {pdtk_canvas_map %W}
    bind $name.c <Unmap> {pdtk_canvas_unmap %W}

    switch $pd_nt { 0 {
        bind $name.c <Button-4>  "pdtk_canvas_scroll $name.c y -1"
        bind $name.c <Button-5>  "pdtk_canvas_scroll $name.c y +1"
        bind $name.c <Shift-Button-4>  "pdtk_canvas_scroll $name.c x -1"
        bind $name.c <Shift-Button-5>  "pdtk_canvas_scroll $name.c x +1"
		#if { $k12_mode == 0 } {
		#    bind $name.c <Control-Button-4>  "pdtk_zoom $name 1"
		#    bind $name.c <Control-Button-5>  "pdtk_zoom $name -1"
		#}
    } default {
        bind $name.c  <MouseWheel> \
            "pdtk_canvas_scroll $name.c y \[expr -abs(%D)/%D\]"
        bind $name.c  <Shift-MouseWheel> \
            "pdtk_canvas_scroll $name.c x \[expr -abs(%D)/%D\]"
    }}

    #dnd bindtarget $name.c text/uri-list <Drop> { pdtk_canvas_makeobjs $name %D %x %y }
    after idle [list dnd bindtarget $name text/uri-list <Drop> { foreach file %D {open_file $file} }]

    #    puts stderr "all done"
    #   after 1 [concat raise $name]
    set pdtk_canvas_mouseup_name ""

	bind $name <FocusIn> "menu_fixeditmenu $name"
	bind $name <FocusOut> "pdtk_canvas_leaveitem $name.c"
	if { $k12_mode == 1 } { pd [concat $name tooltips 1 \;] }
    after idle [concat focus $name.c]

	if { $k12_mode == 1 && $k12_saveas_on_new == 1 } {
		after 1000 [concat pdtk_k12_saveas_on_new $name]
	}

	set ::scroll_on($name) 0
	set ::hit_scrollbar($name) 0
	#set ::scroll_was_cursor($name) 0
	set ::last_scroll_x($name) 0
	set ::last_scroll_y($name) 0
	set ::canvaswidth($name) 0
	set ::canvasheight($name) 0


/*
	if { $k12_mode == 1 } {
		# K-12 menu

		match_linux_wm [list frame $name.k12frame]
		pack $name.k12frame -side left -fill y

		# ---------------------------------- EDIT BUTTON -----------------------------------------
		match_linux_wm [list frame $name.k12frame.edit -relief flat]
		if {$editable==1} {
			match_linux_wm [list button $name.k12frame.edit.b -image i.edit \
				-command [concat menu_editmode $name]]
		} else {
			match_linux_wm [list button $name.k12frame.edit.b -image i.perform \
				-command [concat menu_editmode $name]]
		}
		pack $name.k12frame.edit.b -side left -expand 1 -padx 1 -pady 0
		setTooltip $name.k12frame.edit.b "Toggle between building and playing an instrument"
		pdtk_k12panel_standardkeybindings $name.k12frame.edit.b
		bind $name.k12frame.edit.b <Key> [list pdtk_canvas_sendkey $name.c 1 %K %A 0 1 %t]
    	bind $name.k12frame.edit.b <KeyRelease> [list pdtk_canvas_sendkey $name.c 0 %K %A 0 1 %t]

		# ---------------------------------- DATA VS SOUND BUTTONS ----------------------------------
		match_linux_wm [list frame $name.k12frame.datasound -relief flat]
		match_linux_wm [list button $name.k12frame.datasound.data -text "DATA" -image i.data_on -command [concat pdtk_k12_show_data_icons $name]]
		match_linux_wm [list button $name.k12frame.datasound.sound -text "SOUND" -image i.sound -command [concat pdtk_k12_show_sound_icons $name]]
		pack $name.k12frame.datasound.data $name.k12frame.datasound.sound -side left -pady 1 -padx 1 -expand 0
		setTooltip $name.k12frame.datasound.data "Show DATA objects"
		setTooltip $name.k12frame.datasound.sound "Show SOUND objects"
		pdtk_k12panel_standardkeybindings $name.k12frame.datasound.data
		pdtk_k12panel_standardkeybindings $name.k12frame.datasound.sound

		# ---------------------------------- MESSAGES LABEL -----------------------------------------

		#match_linux_wm [list frame $name.k12frame.msgs -relief flat]
		#match_linux_wm [list label $name.k12frame.msgs.label -relief flat -text "MESSAGES"]
		#pack $name.k12frame.msgs.label -fill x -pady 0 -padx 1

		# ---------------------------------- WII -----------------------------------------
		match_linux_wm [list frame $name.k12frame.wii -relief flat]
		match_linux_wm [list button $name.k12frame.wii.b_wii_connect -image i.wii_connect \
			-command [concat put_K12_objects $name wii_connect]]
		match_linux_wm [list button $name.k12frame.wii.b_wii_buttons -image i.wii_buttons \
			-command [concat put_K12_objects $name wii_buttons]]
		match_linux_wm [list button $name.k12frame.wii.b_wii_hit -image i.wii_hit \
			-command [concat put_K12_objects $name wii_hit]]
		match_linux_wm [list button $name.k12frame.wii.b_wii_accelerometer -image i.wii_accelerometer \
			-command [concat put_K12_objects $name wii_accelerometer]]
		match_linux_wm [list button $name.k12frame.wii.b_wii_speed_xry -image i.wii_speed_xry \
			-command [concat put_K12_objects $name wii_speed_xry]]
		pack $name.k12frame.wii.b_wii_connect $name.k12frame.wii.b_wii_buttons $name.k12frame.wii.b_wii_hit $name.k12frame.wii.b_wii_accelerometer $name.k12frame.wii.b_wii_speed_xry -side left -expand 0 -padx 1 -pady 1
		setTooltip $name.k12frame.wii.b_wii_connect "Wiimote Connect: Use this to connect wiimote to the computer"
		setTooltip $name.k12frame.wii.b_wii_buttons "Wiimote Buttons: Use this to select which Wiimote button should activate objects connected to this object"
		setTooltip $name.k12frame.wii.b_wii_hit "Wiimote Hit: Use this to detect when the wiimote has been shaken like a mallet"
		setTooltip $name.k12frame.wii.b_wii_accelerometer "Wiimote Accelerometer: Use this to monitor Wiimotes acceleration across X, Y, and Z axes"
		setTooltip $name.k12frame.wii.b_wii_speed_xry "Wiimote Speed X, Roll, Y: Use this to detect how quickly is Wiimote moving across individual axes x, roll, and y (requires motion plus)"
		pdtk_k12panel_standardkeybindings $name.k12frame.wii.b_wii_connect
		pdtk_k12panel_standardkeybindings $name.k12frame.wii.b_wii_buttons
		pdtk_k12panel_standardkeybindings $name.k12frame.wii.b_wii_hit
		pdtk_k12panel_standardkeybindings $name.k12frame.wii.b_wii_accelerometer
		pdtk_k12panel_standardkeybindings $name.k12frame.wii.b_wii_speed_xry

		# ---------------------------------- WII2 -----------------------------------------
		match_linux_wm [list frame $name.k12frame.wii2 -relief flat]
		match_linux_wm [list button $name.k12frame.wii2.b_wii_speed -image i.wii_speed \
			-command [concat put_K12_objects $name wii_speed]]
		match_linux_wm [list button $name.k12frame.wii2.b_wii_nunchuk_buttons -image i.wii_nunchuk_buttons \
			-command [concat put_K12_objects $name wii_nunchuk_buttons]]
		match_linux_wm [list button $name.k12frame.wii2.b_wii_nunchuk_hit -image i.wii_nunchuk_hit \
			-command [concat put_K12_objects $name wii_nunchuk_hit]]
		match_linux_wm [list button $name.k12frame.wii2.b_wii_nunchuk_accelerometer -image i.wii_nunchuk_accelerometer \
			-command [concat put_K12_objects $name wii_nunchuk_accelerometer]]
		match_linux_wm [list button $name.k12frame.wii2.b_wii_nunchuk_stick -image i.wii_nunchuk_stick \
			-command [concat put_K12_objects $name wii_nunchuk_stick]]
		pack $name.k12frame.wii2.b_wii_speed $name.k12frame.wii2.b_wii_nunchuk_buttons $name.k12frame.wii2.b_wii_nunchuk_hit $name.k12frame.wii2.b_wii_nunchuk_accelerometer $name.k12frame.wii2.b_wii_nunchuk_stick -side left -expand 0 -padx 1 -pady 1
		setTooltip $name.k12frame.wii2.b_wii_speed "Wiimote Speed: Use this to detect how quickly is Wiimote moving (requires motion plus)"
		setTooltip $name.k12frame.wii2.b_wii_nunchuk_buttons "Wiimote Nunchuk Buttons: Use this to select which Nunchuk button should activate objects connected to this object (requires nunchuk extension)"
		setTooltip $name.k12frame.wii2.b_wii_nunchuk_hit "Wiimote Nunchuk Hit: Use this to detect when the wiimote has been shaken like a mallet (requires nunchuk extension)"
		setTooltip $name.k12frame.wii2.b_wii_nunchuk_accelerometer "Wiimote Nunchuk Accelerometer: Use this to monitor Nunchuk acceleration across X, Y, and Z axes (requires nunchuk extension)"
		setTooltip $name.k12frame.wii2.b_wii_nunchuk_stick "Wiimote Nunchuk stick: Use this to monitor Nunchuk stick motion across X and Y axes (requires nunchuk extension)"
		pdtk_k12panel_standardkeybindings $name.k12frame.wii2.b_wii_speed
		pdtk_k12panel_standardkeybindings $name.k12frame.wii2.b_wii_nunchuk_buttons
		pdtk_k12panel_standardkeybindings $name.k12frame.wii2.b_wii_nunchuk_hit
		pdtk_k12panel_standardkeybindings $name.k12frame.wii2.b_wii_nunchuk_accelerometer
		pdtk_k12panel_standardkeybindings $name.k12frame.wii2.b_wii_nunchuk_stick

		# ---------------------------------- ARDUINO -----------------------------------------
		match_linux_wm [list frame $name.k12frame.arduino -relief flat]
		match_linux_wm [list button $name.k12frame.arduino.b_sarcduino -image i.sarcduino \
			-command [concat put_K12_objects $name sarcduino_connect]]
		match_linux_wm [list button $name.k12frame.arduino.b_sarcduino_digital -image i.sarcduino_digital \
			-command [concat put_K12_objects $name sarcduino_digital]]
		match_linux_wm [list button $name.k12frame.arduino.b_sarcduino_analog -image i.sarcduino_analog \
			-command [concat put_K12_objects $name sarcduino_analog]]
		match_linux_wm [list button $name.k12frame.arduino.b_sarcduino_hit -image i.sarcduino_hit \
			-command [concat put_K12_objects $name sarcduino_hit]]
		match_linux_wm [list button $name.k12frame.arduino.b_sarcduino_piezo -image i.sarcduino_piezo \
			-command [concat put_K12_objects $name sarcduino_piezo]]
		pack $name.k12frame.arduino.b_sarcduino $name.k12frame.arduino.b_sarcduino_digital $name.k12frame.arduino.b_sarcduino_analog $name.k12frame.arduino.b_sarcduino_hit $name.k12frame.arduino.b_sarcduino_piezo -side left -expand 0 -padx 1 -pady 1
		setTooltip $name.k12frame.arduino.b_sarcduino "Arduino Connect: Use this to arduino to the computer"
		setTooltip $name.k12frame.arduino.b_sarcduino_digital "Arduino Digital: Use this to detect on/off states of a digital sensor"
		setTooltip $name.k12frame.arduino.b_sarcduino_analog "Arduino Analog: Use this to monitor analog sensor speed"
		setTooltip $name.k12frame.arduino.b_sarcduino_hit "Arduino Hit: Use this to detect when the arduino analog sensor data has rapidly changed"
		setTooltip $name.k12frame.arduino.b_sarcduino_piezo "Arduino Piezo: Use this to analyze data coming from a piezo microphone sensor"
		pdtk_k12panel_standardkeybindings $name.k12frame.arduino.b_sarcduino
		pdtk_k12panel_standardkeybindings $name.k12frame.arduino.b_sarcduino_digital
		pdtk_k12panel_standardkeybindings $name.k12frame.arduino.b_sarcduino_analog
		pdtk_k12panel_standardkeybindings $name.k12frame.arduino.b_sarcduino_hit
		pdtk_k12panel_standardkeybindings $name.k12frame.arduino.b_sarcduino_piezo

		# ---------------------------------- RPI & MATH ROW 1 -----------------------------------------
		match_linux_wm [list frame $name.k12frame.math_1 -relief flat]
		#match_linux_wm [list button $name.k12frame.math_1.b_sarcduino_net -image i.sarcduino_net \
		#	-command [concat put_K12_objects $name sarcduino_net]]
		match_linux_wm [list button $name.k12frame.math_1.b_rpi_digital -image i.raspberry_digital \
			-command [concat put_K12_objects $name raspberrypi_digital]]
		match_linux_wm [list button $name.k12frame.math_1.b_rpi_analog_out -image i.raspberry_analog_out \
			-command [concat put_K12_objects $name raspberrypi_analog_out]]
		match_linux_wm [list button $name.k12frame.math_1.b_rpi_analog_in -image i.raspberry_analog_in \
			-command [concat put_K12_objects $name raspberrypi_analog_in]]
		match_linux_wm [list button $name.k12frame.math_1.b_math_number -image i.math_number \
			-command [concat put_K12_objects $name math_number]]
		match_linux_wm [list button $name.k12frame.math_1.b_math_netsend -image i.math_netsend \
			-command [concat put_K12_objects $name math_netsend]]
		pack $name.k12frame.math_1.b_rpi_digital $name.k12frame.math_1.b_rpi_analog_out $name.k12frame.math_1.b_rpi_analog_in $name.k12frame.math_1.b_math_number $name.k12frame.math_1.b_math_netsend -side left -expand 0 -padx 1 -pady 1
		#setTooltip $name.k12frame.math_1.b_sarcduino_net "Arduino Net: Use this to retrieve data from arduino devices connected via network"
		setTooltip $name.k12frame.math_1.b_rpi_digital "RaspberryPi Digital: Use this to read from or write to GPIO pins in digital format"
		setTooltip $name.k12frame.math_1.b_rpi_analog_out "RaspberryPi Analog Out: Use this to write to GPIO pins in analog format using PWM"
		setTooltip $name.k12frame.math_1.b_rpi_analog_in "RaspberryPi Analog In: Use this to read from RaspberryPi LOP Shield's analog inputs"
		setTooltip $name.k12frame.math_1.b_math_number "Number: Use this to assign a value to other objects"
		setTooltip $name.k12frame.math_1.b_math_netsend "Netsend: Use this to send data over network to another computer"
		pdtk_k12panel_standardkeybindings $name.k12frame.math_1.b_rpi_digital
		pdtk_k12panel_standardkeybindings $name.k12frame.math_1.b_rpi_analog_out
		pdtk_k12panel_standardkeybindings $name.k12frame.math_1.b_rpi_analog_in
		pdtk_k12panel_standardkeybindings $name.k12frame.math_1.b_math_number
		pdtk_k12panel_standardkeybindings $name.k12frame.math_1.b_math_netsend

		# ---------------------------------- MATH ROW 2 -----------------------------------------
		match_linux_wm [list frame $name.k12frame.math_2 -relief flat]
		match_linux_wm [list button $name.k12frame.math_2.b_math_netreceive -image i.math_netreceive \
			-command [concat put_K12_objects $name math_netreceive]]
		match_linux_wm [list button $name.k12frame.math_2.b_math_tag -image i.math_tag \
			-command [concat put_K12_objects $name math_tag]]
		match_linux_wm [list button $name.k12frame.math_2.b_math_routebytag -image i.math_routebytag \
			-command [concat put_K12_objects $name math_routebytag]]
		match_linux_wm [list button $name.k12frame.math_2.b_math_add -image i.math_add \
			-command [concat put_K12_objects $name math_add]]
		match_linux_wm [list button $name.k12frame.math_2.b_math_subtract -image i.math_subtract \
			-command [concat put_K12_objects $name math_subtract]]
		pack $name.k12frame.math_2.b_math_netreceive $name.k12frame.math_2.b_math_tag $name.k12frame.math_2.b_math_routebytag $name.k12frame.math_2.b_math_add $name.k12frame.math_2.b_math_subtract -side left -expand 0 -padx 1 -pady 1
		setTooltip $name.k12frame.math_2.b_math_netreceive "Netreceive: Use this to receive data from another computer over network"
		setTooltip $name.k12frame.math_2.b_math_tag "Tag: Use this to tag data to be sent over network"
		setTooltip $name.k12frame.math_2.b_math_routebytag "Route By Tag: Use this to filter incoming network data by tag"
		setTooltip $name.k12frame.math_2.b_math_add "Add: Use this to add two values"
		setTooltip $name.k12frame.math_2.b_math_subtract "Subtract: Use this to subtract two values"
		pdtk_k12panel_standardkeybindings $name.k12frame.math_2.b_math_netreceive
		pdtk_k12panel_standardkeybindings $name.k12frame.math_2.b_math_tag
		pdtk_k12panel_standardkeybindings $name.k12frame.math_2.b_math_routebytag
		pdtk_k12panel_standardkeybindings $name.k12frame.math_2.b_math_add
		pdtk_k12panel_standardkeybindings $name.k12frame.math_2.b_math_subtract

		# ---------------------------------- MATH ROW 3 -----------------------------------------
		match_linux_wm [list frame $name.k12frame.math_3 -relief flat]
		match_linux_wm [list button $name.k12frame.math_3.b_math_multiply -image i.math_multiply \
			-command [concat put_K12_objects $name math_multiply]]
		match_linux_wm [list button $name.k12frame.math_3.b_math_divide -image i.math_divide \
			-command [concat put_K12_objects $name math_divide]]
		match_linux_wm [list button $name.k12frame.math_3.b_math_random -image i.math_random \
			-command [concat put_K12_objects $name math_random]]
		match_linux_wm [list button $name.k12frame.math_3.b_math_average -image i.math_average \
			-command [concat put_K12_objects $name math_average]]
		match_linux_wm [list button $name.k12frame.math_3.b_math_scale -image i.math_scale \
			-command [concat put_K12_objects $name math_scale]]
		pack $name.k12frame.math_3.b_math_multiply $name.k12frame.math_3.b_math_divide $name.k12frame.math_3.b_math_random $name.k12frame.math_3.b_math_average $name.k12frame.math_3.b_math_scale -side left -expand 0 -padx 1 -pady 1
		setTooltip $name.k12frame.math_3.b_math_multiply "Multiply: Use this to multiply two values"
		setTooltip $name.k12frame.math_3.b_math_divide "Divide: Use this to divide two values"
		setTooltip $name.k12frame.math_3.b_math_random "Random: Use this to generate random numbers"
		setTooltip $name.k12frame.math_3.b_math_average "Average: Use this to calculate average from a stream of numbers"
		setTooltip $name.k12frame.math_3.b_math_scale "Scale: Use this to scale incoming values to a new range and direction"
		pdtk_k12panel_standardkeybindings $name.k12frame.math_3.b_math_multiply
		pdtk_k12panel_standardkeybindings $name.k12frame.math_3.b_math_divide
		pdtk_k12panel_standardkeybindings $name.k12frame.math_3.b_math_random
		pdtk_k12panel_standardkeybindings $name.k12frame.math_3.b_math_average
		pdtk_k12panel_standardkeybindings $name.k12frame.math_3.b_math_scale

		# ---------------------------------- LOGIC -----------------------------------------
		match_linux_wm [list frame $name.k12frame.logic -relief flat]
		match_linux_wm [list button $name.k12frame.logic.b_logic_compare -image i.logic_compare \
			-command [concat put_K12_objects $name logic_compare]]
		match_linux_wm [list button $name.k12frame.logic.b_logic_mapper -image i.logic_mapper \
			-command [concat put_K12_objects $name logic_mapper]]
		match_linux_wm [list button $name.k12frame.logic.b_logic_metronome -image i.logic_metronome \
			-command [concat put_K12_objects $name logic_metronome]]
		match_linux_wm [list button $name.k12frame.logic.b_logic_counter -image i.logic_counter \
			-command [concat put_K12_objects $name logic_counter]]
		match_linux_wm [list button $name.k12frame.logic.b_logic_sequencer -image i.logic_sequencer \
			-command [concat put_K12_objects $name logic_sequencer]]
		pack $name.k12frame.logic.b_logic_compare $name.k12frame.logic.b_logic_mapper $name.k12frame.logic.b_logic_metronome $name.k12frame.logic.b_logic_counter $name.k12frame.logic.b_logic_sequencer -side left -expand 0 -padx 1 -pady 1
		setTooltip $name.k12frame.logic.b_logic_compare "Compare: Use this to compare two values"
		setTooltip $name.k12frame.logic.b_logic_mapper "Mapper: Use this to map one value to two different but related values"
		setTooltip $name.k12frame.logic.b_logic_metronome "Metronome: Use this to create a steady pulse"
		setTooltip $name.k12frame.logic.b_logic_counter "Counter: Use this to count events"
		setTooltip $name.k12frame.logic.b_logic_sequencer "Sequencer: Use this to map values to MIDI pitches"
		pdtk_k12panel_standardkeybindings $name.k12frame.logic.b_logic_compare
		pdtk_k12panel_standardkeybindings $name.k12frame.logic.b_logic_mapper
		pdtk_k12panel_standardkeybindings $name.k12frame.logic.b_logic_metronome
		pdtk_k12panel_standardkeybindings $name.k12frame.logic.b_logic_counter
		pdtk_k12panel_standardkeybindings $name.k12frame.logic.b_logic_sequencer

		# ---------------------------------- OTHER -----------------------------------------
		match_linux_wm [list frame $name.k12frame.other -relief flat]
		match_linux_wm [list button $name.k12frame.other.b_preset -image i.preset \
			-command [concat put_K12_objects $name preset]] 
		match_linux_wm [list button $name.k12frame.other.b_comment -image i.comment \
			-command [concat menu_comment $name 1]]
		pack $name.k12frame.other.b_preset $name.k12frame.other.b_comment -side left -expand 0 -padx 1 -pady 1
		setTooltip $name.k12frame.other.b_preset "Preset: Use this to store and recall up to four different states of your instrument"
		setTooltip $name.k12frame.other.b_comment "Comment: Use this to post comments inside your patch"
		pdtk_k12panel_standardkeybindings $name.k12frame.other.b_preset
		pdtk_k12panel_standardkeybindings $name.k12frame.other.b_comment

		# ---------------------------------- SOUND LABEL -----------------------------------------
		#match_linux_wm [list frame $name.k12frame.sound -relief flat]
		#match_linux_wm [list label $name.k12frame.sound.label -relief flat -text "SOUND"]
		#pack $name.k12frame.sound.label -fill x -pady 0 -padx 1

		# ---------------------------------- SIGNAL ROW 1 -----------------------------------------
		match_linux_wm [list frame $name.k12frame.signal_1 -relief flat]
		match_linux_wm [list button $name.k12frame.signal_1.b_signal_microphone -image i.signal_microphone \
			-command [concat put_K12_objects $name signal_microphone]]

		match_linux_wm [list button $name.k12frame.signal_1.b_signal_netsend -image i.signal_netsend \
			-command [concat put_K12_objects $name signal_netsend]]
		match_linux_wm [list button $name.k12frame.signal_1.b_signal_netreceive -image i.signal_netreceive \
			-command [concat put_K12_objects $name signal_netreceive]]

		match_linux_wm [list button $name.k12frame.signal_1.b_signal_sampler -image i.signal_sampler \
			-command [concat put_K12_objects $name signal_sampler]]
		match_linux_wm [list button $name.k12frame.signal_1.b_signal_player -image i.signal_player \
			-command [concat put_K12_objects $name signal_player]]
		pack $name.k12frame.signal_1.b_signal_microphone $name.k12frame.signal_1.b_signal_netsend $name.k12frame.signal_1.b_signal_netreceive $name.k12frame.signal_1.b_signal_sampler $name.k12frame.signal_1.b_signal_player -side left -expand 0 -padx 1 -pady 1
		setTooltip $name.k12frame.signal_1.b_signal_microphone "Microphone: Use this to capture and monitor microphone input"
		setTooltip $name.k12frame.signal_1.b_signal_netsend "Netsend Sound: Use this to send your sound over network to another computer"
		setTooltip $name.k12frame.signal_1.b_signal_netreceive "Netreceive Sound: Use this to receive sound from another computer over network"
		setTooltip $name.k12frame.signal_1.b_signal_sampler "Sampler: Use this to record audio from microphone and play it back in various ways"
		setTooltip $name.k12frame.signal_1.b_signal_player "Player: Use this to play WAV files in various ways"
		pdtk_k12panel_standardkeybindings $name.k12frame.signal_1.b_signal_microphone
		pdtk_k12panel_standardkeybindings $name.k12frame.signal_1.b_signal_netsend
		pdtk_k12panel_standardkeybindings $name.k12frame.signal_1.b_signal_netreceive
		pdtk_k12panel_standardkeybindings $name.k12frame.signal_1.b_signal_sampler
		pdtk_k12panel_standardkeybindings $name.k12frame.signal_1.b_signal_player

		# ---------------------------------- SIGNAL ROW 2 -----------------------------------------
		match_linux_wm [list frame $name.k12frame.signal_2 -relief flat]
		match_linux_wm [list button $name.k12frame.signal_2.b_signal_sine -image i.signal_sine \
			-command [concat put_K12_objects $name signal_sine]]
		match_linux_wm [list button $name.k12frame.signal_2.b_signal_saw -image i.signal_saw \
			-command [concat put_K12_objects $name signal_saw]]
		match_linux_wm [list button $name.k12frame.signal_2.b_signal_square -image i.signal_square \
			-command [concat put_K12_objects $name signal_square]]
		match_linux_wm [list button $name.k12frame.signal_2.b_signal_triangle -image i.signal_triangle \
			-command [concat put_K12_objects $name signal_triangle]]
		match_linux_wm [list button $name.k12frame.signal_2.b_signal_envelope -image i.signal_envelope \
			-command [concat put_K12_objects $name signal_envelope]]
		pack $name.k12frame.signal_2.b_signal_sine $name.k12frame.signal_2.b_signal_saw $name.k12frame.signal_2.b_signal_square $name.k12frame.signal_2.b_signal_triangle $name.k12frame.signal_2.b_signal_envelope -side left -expand 0 -padx 1 -pady 1
		setTooltip $name.k12frame.signal_2.b_signal_sine "Sine: Use this to generate sine tone"
		setTooltip $name.k12frame.signal_2.b_signal_saw "Sawtooth: Use this to generate sawtooth tone"
		setTooltip $name.k12frame.signal_2.b_signal_square "Square: Use this to generate square tone"
		setTooltip $name.k12frame.signal_2.b_signal_triangle "Triangle: Use this to generate triangle tone"
		setTooltip $name.k12frame.signal_2.b_signal_envelope "Envelope: Use this to shape sound loudness"
		pdtk_k12panel_standardkeybindings $name.k12frame.signal_2.b_signal_sine
		pdtk_k12panel_standardkeybindings $name.k12frame.signal_2.b_signal_saw
		pdtk_k12panel_standardkeybindings $name.k12frame.signal_2.b_signal_square
		pdtk_k12panel_standardkeybindings $name.k12frame.signal_2.b_signal_triangle
		pdtk_k12panel_standardkeybindings $name.k12frame.signal_2.b_signal_envelope

		# ---------------------------------- SIGNAL ROW 3 -----------------------------------------
		match_linux_wm [list frame $name.k12frame.signal_3 -relief flat]
		match_linux_wm [list button $name.k12frame.signal_3.b_signal_noise -image i.signal_noise \
			-command [concat put_K12_objects $name signal_noise]]
		match_linux_wm [list button $name.k12frame.signal_3.b_signal_pink -image i.signal_pink \
			-command [concat put_K12_objects $name signal_pink]]
		match_linux_wm [list button $name.k12frame.signal_3.b_signal_add -image i.signal_add \
			-command [concat put_K12_objects $name signal_add]]
		match_linux_wm [list button $name.k12frame.signal_3.b_signal_multiply -image i.signal_multiply \
			-command [concat put_K12_objects $name signal_multiply]]
		pack $name.k12frame.signal_3.b_signal_noise $name.k12frame.signal_3.b_signal_pink $name.k12frame.signal_3.b_signal_add $name.k12frame.signal_3.b_signal_multiply -side left -expand 0 -padx 1 -pady 1
		setTooltip $name.k12frame.signal_3.b_signal_noise "Noise: Use this to generate white (harsh) noise"
		setTooltip $name.k12frame.signal_3.b_signal_pink "Pink: Use this to generate pink (softer) noise"
		setTooltip $name.k12frame.signal_3.b_signal_add "Signal Add: Use this to add two sounds (signals)"
		setTooltip $name.k12frame.signal_3.b_signal_multiply "Signal Multiply: Use this to multiply two sounds (signals)"
		pdtk_k12panel_standardkeybindings $name.k12frame.signal_3.b_signal_noise 
		pdtk_k12panel_standardkeybindings $name.k12frame.signal_3.b_signal_pink
		pdtk_k12panel_standardkeybindings $name.k12frame.signal_3.b_signal_add
		pdtk_k12panel_standardkeybindings $name.k12frame.signal_3.b_signal_multiply

		# ---------------------------------- INSTRUMENTS -----------------------------------------
		match_linux_wm [list frame $name.k12frame.instr -relief flat]
		match_linux_wm [list button $name.k12frame.instr.b_instr_short1 -image i.instr_short1 \
			-command [concat put_K12_objects $name instr_short1]]
		match_linux_wm [list button $name.k12frame.instr.b_instr_short2 -image i.instr_short2 \
			-command [concat put_K12_objects $name instr_short2]]
		match_linux_wm [list button $name.k12frame.instr.b_instr_sustained1 -image i.instr_sustained1 \
			-command [concat put_K12_objects $name instr_sustained1]]
		match_linux_wm [list button $name.k12frame.instr.b_instr_sustained2 -image i.instr_sustained2 \
			-command [concat put_K12_objects $name instr_sustained2]]
		pack $name.k12frame.instr.b_instr_short1 $name.k12frame.instr.b_instr_short2 $name.k12frame.instr.b_instr_sustained1 $name.k12frame.instr.b_instr_sustained2 -side left -expand 0 -padx 1 -pady 1
		setTooltip $name.k12frame.instr.b_instr_short1 "Bass Drum: Use this to produce short sounds like a single bass drum hit"
		setTooltip $name.k12frame.instr.b_instr_short2 "Snare Drum: Use this to produce short sounds like a single snare drum hit"
		setTooltip $name.k12frame.instr.b_instr_sustained1 "Air Instrument: Use this to produce long sustained sound like a sound of a woodwind instrument"
		setTooltip $name.k12frame.instr.b_instr_sustained2 "Brass Instrument: Use this to produce long sustained sound like a sound of brass instrument"
		pdtk_k12panel_standardkeybindings $name.k12frame.instr.b_instr_short1
		pdtk_k12panel_standardkeybindings $name.k12frame.instr.b_instr_short2
		pdtk_k12panel_standardkeybindings $name.k12frame.instr.b_instr_sustained1
		pdtk_k12panel_standardkeybindings $name.k12frame.instr.b_instr_sustained2

		# ---------------------------------- F/X -----------------------------------------
		match_linux_wm [list frame $name.k12frame.fx -relief flat]
		match_linux_wm [list button $name.k12frame.fx.b_fx_filter -image i.fx_filter \
			-command [concat put_K12_objects $name fx_filter]]
		match_linux_wm [list button $name.k12frame.fx.b_fx_multitap -image i.fx_multitap \
			-command [concat put_K12_objects $name fx_multitap]]
		match_linux_wm [list button $name.k12frame.fx.b_fx_reverb -image i.fx_reverb \
			-command [concat put_K12_objects $name fx_reverb]]
		pack $name.k12frame.fx.b_fx_filter $name.k12frame.fx.b_fx_multitap $name.k12frame.fx.b_fx_reverb -side left -expand 0 -padx 1 -pady 1
		setTooltip $name.k12frame.fx.b_fx_filter "Filter: Use this to make sound appear muffled or brighter"
		setTooltip $name.k12frame.fx.b_fx_multitap "Echo: Use this to make sound echo"
		setTooltip $name.k12frame.fx.b_fx_reverb "Reverb: Use this to make sound appear as if it is being played in a large space"
		pdtk_k12panel_standardkeybindings $name.k12frame.fx.b_fx_filter
		pdtk_k12panel_standardkeybindings $name.k12frame.fx.b_fx_multitap
		pdtk_k12panel_standardkeybindings $name.k12frame.fx.b_fx_reverb

		# ---------------------------------- OUTPUT/OTHER -----------------------------------------

		match_linux_wm [list frame $name.k12frame.output -relief flat]
		match_linux_wm [list button $name.k12frame.output.b_output -image i.output \
			-command [concat put_K12_objects $name output]] 	
		pack $name.k12frame.output.b_output -side left -expand 0 -padx 1 -pady 1
		setTooltip $name.k12frame.output.b_output "Output: Use this to send audio from computer into speakers"
		pdtk_k12panel_standardkeybindings $name.k12frame.output.b_output

		# ---------------------------------------- NOW PACK THEM ALL -----------------------------------------
		pack $name.k12frame.edit $name.k12frame.datasound $name.k12frame.wii $name.k12frame.wii2 $name.k12frame.arduino $name.k12frame.math_1 $name.k12frame.math_2 $name.k12frame.math_3 $name.k12frame.logic $name.k12frame.other -side top -expand 0 -fill x
	}

	if { $k12_mode == 0 } {
    	wm minsize $name 50 20
	} else {
		wm minsize $name 580 407
	}
*/



// net stuff
var net = require('net');

var HOST = '127.0.0.1';
var PORT;
var client;

exports.set_port = function (port_no) {
    PORT = port_no;
}


function connect () {
    client = new net.Socket();
    client.setNoDelay(true);
    client.connect(PORT, HOST, function() {
        console.log('CONNECTED TO: ' + HOST + ':' + PORT);
        // Write a test message to the socket as soon as the client is connected,
        //the server will receive it as message from the client. This can be removed
        // once it's obvious it works...
        client.write('I am Chuck Norris!;');
    });
}

exports.connect = connect;

// Add a 'data' event handler for the client socket
// data parameter is what the server sent to this socket

// Pd can send us different types of data:
// 1) The old style tcl commands with "\n" at end (or "\\\n" for continuation)
// 2) new style commands: selector "stringarg",42,"stringarg3",etc.
// Below we separate the wheat from chaff, eval'ing the new commands and just
// printing the old ones in blue to the console

// To facilitate this, the new style commands are always preceded with an
// alarm bell '\a', and end with a vertical tab '\v'. These should produce
// a decent stop-gap since they are rarely used in Pd patch text.

function init_socket_events () {
    var next_command = ''; // A not-quite-FUDI command: selector arg1,arg2,etc.
                           // These are formatted on the C side to be easy
                           // to parse here in javascript
    var old_command = '';  // Old-style sys_vgui cmds (printed to console)
    var cmdHeader = false;

    client.on('data', function(data) {
        var i, len, selector, args;
        len = data.length;
        for (i = 0; i < len; i++) {
            if (cmdHeader) {
                // check for end of command:
                if (data[i] === 11) { // vertical tab '\v'
                    // decode next_command
                    try {
                        // This should work for all utf-8 content
                        next_command = decodeURIComponent(next_command);
                    }
                    catch(err) {
                        // This should work for ISO-8859-1
                        next_command = unescape(next_command);
                    }
                    // Turn newlines into backslash + 'n' so
                    // eval will do the right thing
                    next_command = next_command.replace(/\n/g, '\\n');
                    selector = next_command.slice(0, next_command.indexOf(" "));
                    args = next_command.slice(selector.length + 1);
                    cmdHeader = false;
                    next_command = '';
                    // Now evaluate it 
                    //gui_post('Evaling: ' + selector + '(' + args + ');');
                    eval(selector + '(' + args + ');');
                } else {
                    next_command += '%' +
                        ('0' // leading zero (for rare case of single digit)
                         + data[i].toString(16)) // to hex
                           .slice(-2); // remove extra leading zero
                }
            } else if (data[i] === 7) { // ASCII alarm bell '\a'
                // if we have an old-style message, print it out
                if (old_command !== '') {
                    var old_command_output;
                    try {
                        old_command_output = decodeURIComponent(old_command);
                    }
                    catch(err) {
                        old_command_output = unescape(old_command);
                    }
                    old_command= '';
                    gui_post("warning: old command: " + old_command_output,
                        'blue');
                }
                cmdHeader = true; 
            } else {
                // this is an old-style sys_vgui
                old_command += '%' + ('0' + data[i].toString(16)).slice(-2);
            }
        }
    });

    // Add a 'close' event handler for the client socket
    client.on('close', function() {
        //console.log('Connection closed');
        //client.destroy();
        nw_app_quit(); // set a timeout here if you need to debug
    });
}

exports.init_socket_events = init_socket_events;

// Send commands to Pd
function pdsend(string) {
    client.write(string + ';');
    // for now, let's reprint the outgoing string to the pdwindow
    // gui_post(string + ';', 'red');
}

exports.pdsend = pdsend;

// Send a ping message back to Pd
function gui_ping() {
    pdsend("pd ping");
}

// Send a message to Pd to ping the "watchdog", which is a program
// that supervises Pd when run with -rt flag on some OSes
function gui_ping_watchdog() {
    pdsend("pd watchdog");
}

// Schedule watchdog pings for the life of the GUI
function gui_watchdog() {
    setInterval(gui_ping_watchdog, 2000);
}

// Text drawing stuff

// Here's the main API, structured to make an easier (inital) transition
// from tcl/tk to javascript

// Gobj container, so that all drawn items are contained in a <g> which
// handles displacing (and in the future, possibly clicks and other events)
function get_gobj(cid, object) {
    return patchwin[cid].window.document.getElementById(object + 'gobj');
}

// Convenience function to get a drawn item of gobj
function get_item(cid,item_id) {
    return patchwin[cid].window.document.getElementById(item_id);
}

// Similar to [canvas create] in tk
function create_item(cid,type,args) {
    var item = patchwin[cid].window.document.createElementNS('http://www.w3.org/2000/svg', type);
    if (args !== null) {
        configure_item(item, args);
    }
    return item;
}

// Similar to [canvas itemconfigure], without the need for a reference to the canvas
function configure_item(item, attributes) {
    // draw_vis from g_template sends attributes as a ['attr1',val1, 'attr2', val2, etc.] array,
    // so we check for that here
    var value;
    if (Array.isArray(attributes)) {
        // we should check to make sure length is even here...
        for (var i = 0; i < attributes.length; i+=2) {
            value = attributes[i+1];
            item.setAttributeNS(null, attributes[i],
                Array.isArray(value) ? value.join(" "): value); 
        }
    } else {
        for (var attr in attributes) {
            if (attributes.hasOwnProperty(attr)) {
                item.setAttributeNS(null, attr, attributes[attr]);
            }
        }
    }
}

// A bit of a stopgap. The GUI side probably shouldn't know about "items"
// on SVG.
function gui_configure_item(cid, tag, attributes) {
    var item = get_item(cid, tag);
    configure_item(item, attributes);
}

function add_gobj_to_svg(svg, gobj) {
    svg.insertBefore(gobj, svg.querySelector('.cord'));
}

// Most of these map either to pd.tk procs, or in some cases Tk canvas subcommands
function gui_text_create_gobj(cid, tag, type, xpos, ypos, is_toplevel) {
    var svg = get_item(cid, "patchsvg"); // "patchsvg" is id for the svg element
    var transform_string = 'matrix(1,0,0,1,' + xpos + ',' + ypos + ')';
    var g = create_item(cid, 'g', {
            id: tag + 'gobj',
            transform: transform_string,
            class: type + (is_toplevel !== 0 ? '' : ' gop'),
            'shape-rendering': 'crispEdges'
    });
    add_gobj_to_svg(svg, g);
//    var bluh = svg.getBBox();
//    var bbox_rect = svg.getElementById('bbox_rect');
//    bbox_rect.setAttributeNS(null, 'width', bluh.width);
//    bbox_rect.setAttributeNS(null, 'height', bluh.height);
//    bbox_rect.setAttributeNS(null, 'fill', 'none');
//    bbox_rect.setAttributeNS(null, 'stroke', 'black');
    
// hm... why returning g and not the return value of appendChild?
//    console.log("create gobj tag is " + tag + " and ret is " + g);
    return g;
}

function gui_text_drawborder(cid, tag, bgcolor, isbroken, x1, y1, x2, y2) {
    var g = get_gobj(cid, tag);
    // isbroken means either
    //     a) the object couldn't create or
    //     b) the box is empty
    var rect = create_item(cid, 'rect', {
        width: x2 - x1,
        height: y2 - y1,
        stroke: 'black',
        fill: 'none',
        'shape-rendering': 'crispEdges',
        class: 'border'
    });
    if (isbroken === 1) {
        rect.classList.add('broken_border');
    }
    g.appendChild(rect);
}

function gui_canvas_drawio(cid, parenttag, tag, x1, y1, x2, y2, basex, basey, type, i, is_signal, is_iemgui) {
    var xlet_class, xlet_id, g = get_gobj(cid, parenttag);
    if (is_iemgui) {
        xlet_class = 'xlet_iemgui';
        // We have an inconsistency here.  We're setting the tag using
        // string concatenation below, but the "tag" for iemguis arrives
        // to us pre-concatenated.  We need to remove that formatting in c, and
        // in general try to simplify tag creation on the c side as much
        // as possible.
        xlet_id = tag;
    } else if (is_signal) {
        xlet_class = 'xlet_signal';
        xlet_id = tag + type + i;
    } else {
        xlet_class = 'xlet_control';
        xlet_id = tag + type + i;
    }
    var rect = create_item(cid, 'rect', {
        width: x2 - x1,
        height: y2 - y1,
        x: x1 - basex,
        y: y1 - basey,
        id: xlet_id,
        class: xlet_class,
        'shape-rendering': 'crispEdges'
    });
    g.appendChild(rect);
}

function gui_canvas_redraw_io(cid, parenttag, tag, x, y, type, i, basex, basey) {
    var xlet = get_item(cid, tag + type + i); 
    // We have to check for null. Here's why...
    // if you create a gatom:
    //   canvas_atom -> glist_add -> text_vis -> glist_retext ->
    //     rtext_retext -> rtext_senditup ->
    //       text_drawborder (firsttime=0) -> glist_drawiofor (firsttime=0)
    // This means that a new gatom tries to redraw its inlets before
    // it has created them.
    if (xlet !== null) {
        configure_item(xlet, { x: x - basex, y: y - basey });
    }
}

function gui_eraseio(cid, tag) {
    gui_post("the tag for this bout-to-ba-leted XLET is " + tag);
    var xlet = get_item(cid, tag);
    xlet.parentNode.removeChild(xlet);
}

function gui_configure_io(cid, tag, is_iemgui, is_signal, width) {
    var xlet = get_item(cid, tag);
    // We have to check for null here. Empty/broken object boxes
    // can have "phantom" xlets as placeholders for connections
    // to other objects. This may happen due to:
    //   * autopatching
    //   * objects which fail to create when loading a patch
    if (xlet !== null) { 
        configure_item(xlet, {
            'stroke-width': width,
        });
        if (is_iemgui) {
            xlet.classList.add('xlet_iemgui');
        } else if (is_signal) {
            xlet.classList.add('xlet_signal');
        } else {
            xlet.classList.add('xlet_control');
        }
        // remove xlet_selected tag
        xlet.classList.remove('xlet_selected');
    }
}

function gui_highlight_io(cid, tag) {
    var xlet = get_item(cid, tag);
    // must check for null (see gui_configure_io)
    if (xlet !== null) {
        xlet.classList.add('xlet_selected');
    }
}

function gui_message_drawborder(cid,tag,width,height) {
    var g = get_gobj(cid, tag);
    var p_array = [0,0,
                   width+4, 0,
                   width, 4,
                   width, height-4,
                   width+4, height,
                   0, height,
                   0, 0];
    var polygon = create_item(cid, 'polygon', {
        points: p_array.join(),
        fill: 'none',
        stroke: 'black',
//        'shape-rendering': 'crispEdges',
//        'stroke-width': 1,
        class: 'border'
//        id: tag + 'border'
    });
    g.appendChild(polygon);
}

function gui_message_flash(cid, tag, state) {
    var g = get_gobj(cid, tag);
    if (state !== 0) {
        g.classList.add('flashed');
    } else {
        g.classList.remove('flashed');
    }
//    var b = get_item(cid, tag + 'border');
//    var w;
//    if (state != 0) { w = 4; } else { w = 1; }
//    configure_item(b, { 'stroke-width': w });
}

function gui_message_redraw_border(cid,tag,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14) {
    var g = get_gobj(cid, tag);
    var b = g.querySelector('.border');
    var p_array = [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14];
    configure_item(b, {
        points: p_array.join(" "),
    });
}


function gui_atom_drawborder(cid,tag,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12) {
    var p_array = [p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12];
    var g = get_gobj(cid, tag);
    var polygon = create_item(cid, 'polygon', {
        points: p_array.join(" "),
        fill: 'none',
        stroke: 'gray',
        'stroke-width': 1,
        class: 'border'
//        id: tag + 'border'
    });
    g.appendChild(polygon);
}

// draw a patch cord
function gui_canvas_line(cid,tag,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10) {
    var d_array = ['M', p1 + 0.5, p2 + 0.5,
                   'Q', p3 + 0.5, p4 + 0.5, p5 + 0.5, p6 + 0.5,
                   'Q', p7 + 0.5, p8 + 0.5 ,p9 + 0.5, p10 + 0.5];
    var svg = get_item(cid, "patchsvg");
    var path = create_item(cid, 'path', {
        d: d_array.join(" "),
        fill: 'none',
        stroke: 'gray',
        'stroke-width': 1,
        'shape-rendering': 'optimizeSpeed',
        id: tag,
        'class': 'cord'
    });
    svg.appendChild(path);
}

function gui_canvas_select_line(cid, tag) {
    var line = get_item(cid, tag);
    if (line !== null) {
        configure_item(line, { class: 'selected_line' });
    } else {
        gui_post("gui_canvas_select_line: can't find line");
    }
}

function gui_canvas_deselect_line(cid, tag) {
    var line = get_item(cid, tag);
    if (line !== null) {
        configure_item(line, { class: '' });
    } else {
        gui_post("gui_canvas_select_line: can't find line");
    }
}

// rename to erase_line (or at least standardize with gobj_erase)
function gui_canvas_delete_line(cid, tag) {
    var line = get_item(cid, tag);
    if (line !== null) {
        line.parentNode.removeChild(line);
    } else {
        gui_post("canvas_delete_line: something is fucked up because the line doesn't exist");
    }
}

function gui_canvas_updateline(cid,tag,x1,y1,x2,y2,yoff) {
//    console.log("cord tag in gui_canvas_updateline is " + tag);
    var halfx = parseInt((x2 - x1)/2);
    var halfy = parseInt((y2 - y1)/2);
    var d_array = ['M',x1,y1,
                  'Q',x1,y1+yoff,x1+halfx,y1+halfy,
                  'Q',x2,y2-yoff,x2,y2];
//    gui_post(d_array.toString());
    var cord = get_item(cid, tag);
    configure_item(cord, { d: d_array.join(" ") });
}

function text_to_tspans(canvasname, svg_text, text) {
    var lines, i, len, tspan;
    lines = text.split('\n'); 
    len = lines.length;
    for (i = 0; i < len; i++) {
        tspan = create_item(canvasname, 'tspan', {
            dy: i == 0 ? 0 : 10,
            x: 0
        });
        // find a way to abstract away the canvas array and the DOM here
        var text_node = patchwin[canvasname].window.document.createTextNode(lines[i]);
        tspan.appendChild(text_node);
        svg_text.appendChild(tspan);
    }
}

function gui_text_new(canvasname, myname, type, isselected, x, y, text, font) {
    gui_post("font is " + font);
    var lines, i, len, tspan;
    var g = get_gobj(canvasname, myname);
    var svg_text = create_item(canvasname, 'text', {
        // x and y are fudge factors. Text on the tk canvas used an anchor
        // at the top-right corner of the text's bbox.  SVG uses the baseline.
        // There's probably a programmatic way to do this, but for now-- fudge factors
        // based on the DejaVu Sans Mono font. :)
        transform: 'translate(' + x + ')',
        y: y,
        // Turns out we can't do 'hanging' baseline
        // because it's borked when scaled. Bummer...
        // 'dominant-baseline': 'hanging',
        'shape-rendering': 'optimizeSpeed',
        'font-size': font + 'px',
        id: myname + 'text'
    });

    // fill svg_text with tspan content by splitting on '\n'
    text_to_tspans(canvasname, svg_text, text);

    if (g !== null) {
        g.appendChild(svg_text);
    } else {
        gui_post("gui_text_new: can't find parent group");
    }

    if (isselected) {
        gui_text_select(canvasname, myname);
    }
}

// Because of the overly complex code path inside
// canvas_setgraph, multiple erasures can be triggered in a row.
function gui_gobj_erase(cid, tag) {
    var g = get_gobj(cid, tag);
    if (g !== null) {
        g.parentNode.removeChild(g);
    } else {
        gui_post("gui_gobj_erase: gobj " + tag + " didn't exist in the first place!");
    }
}

function gui_text_set (cid, tag, text) {
    var svg_text = get_item(cid, tag + 'text');
    if (svg_text !== null) {
        svg_text.textContent = '';
        text_to_tspans(cid, svg_text, text);
    } else {
        // In tk, setting an option for a non-existent canvas
        // item is ignored. Because of that, Miller didn't pay
        // attention to parts of the implementation which attempted
        // to set options before creating the item. To get a sense
        // of where this is happening, uncomment the following line:

        //gui_post("gui_text_set: svg_text doesn't exist: tag: " + tag);
    }
}

function gui_text_redraw_border(cid, tag, x1, y1, x2, y2) {
    var i;
    var g = get_gobj(cid, tag);
    var b = g.querySelectorAll('.border');
    for (i = 0; i < b.length; b++) {
        configure_item(b[i], {
            width: x2 - x1,
            height: y2 - y1
        });
    }
}

function gui_text_select(cid, tag) {
    var g = get_gobj(cid, tag);
//    var b = get_item(cid, tag + 'border');
    if (g !== null) {
        g.classList.add('selected');
//        configure_item(g, { class: 'selected' });
    } else {
        console.log("text_select: something wrong with group tag: " + tag);
    }
//    if (b !== null) {
//        configure_item(b, { class: 'selected_border' });
//    }
}

function gui_text_deselect(cid, tag) {
//    gui_post("deselecting text with tag..." + tag);
    var gobj = get_gobj(cid, tag)
    if (gobj !== null) {
        gobj.classList.remove('selected');
    } else {
        console.log("text_deselect: something wrong with tag: " + tag + 'gobj');
    }
}

function gui_text_select_color(cid, tag) {
// nb: this is handled in css now
return;
    var rect = get_item(cid, tag + 'border');
    if (rect !== null) {
        configure_item(rect, {
            stroke: 'blue',
            'stroke-width': 1,
            'stroke-dasharray': 'none'
        });
    } else {
        gui_post("select_color: something wrong with tag: " + tag + 'border');
    }
}

function elem_move(elem, x, y) {
    var t = elem.transform.baseVal.getItem(0);
    t.matrix.e = x;
    t.matrix.f = y;
}

function elem_displace(elem, dx, dy) {
        var t = elem.transform.baseVal.getItem(0);
        t.matrix.e += dx; 
        t.matrix.f += dy; 
}

    // used for tidy up 
function gui_text_displace(name, tag, dx, dy) {
    elem_displace(get_gobj(name, tag), dx, dy);
}

function textentry_displace(t, dx, dy) {
    var transform = t.style.getPropertyValue('transform')
        .split('(')[1]    // get everything after the '('
        .replace(')', '') // remove trailing ')'
        .split(',');      // split into x and y
    var x = +transform[0].trim().replace('px', ''),
        y = +transform[1].trim().replace('px', '');
gui_post("x is " + x + " and y is " + y);
    t.style.setProperty('transform',
        'translate(' +
        (x + dx) + 'px, ' +
        (y + dy) + 'px)');
}

function gui_canvas_displace_withtag(name, dx, dy) {
    var pwin = patchwin[name], i, textentry;
    var ol = pwin.window.document.getElementsByClassName('selected');
    for (i = 0; i < ol.length; i++) {
        elem_displace(ol[i], dx, dy);
//        var elem = ol[i].transform.baseVal.getItem(0);
//        var new_tx = dx + elem.matrix.e; 
//        var new_ty = dy + elem.matrix.f; 
//        elem.matrix.e = new_tx;
//        elem.matrix.f = new_ty;
    }
    textentry = patchwin[name].window.document.getElementById('new_object_textentry');
    if (textentry !== null) {
        textentry_displace(textentry, dx, dy); 
    }

//        elem.setAttributeNS(null, 'transform',
//            'translate(' + new_tx + ',' + new_ty + ')');
//    }
}

function gui_create_selection_rectangle(cid, x1, y1, x2, y2) {
    var svg = get_item(cid, "patchsvg");
    var points_array = [x1 + 0.5, y1 + 0.5,
                        x2 + 0.5, y1 + 0.5,
                        x2 + 0.5, y2 + 0.5,
                        x1 + 0.5, y2 + 0.5
    ];
    var rect = create_item(cid, 'polygon', {
        points: points_array.join(" "),
        fill: 'none',
        stroke: 'black',
        'shape-rendering': 'optimizeSpeed',
        'stroke-width': 1,
        id: 'selection_rectangle',
        display: 'inline' 
    });
    svg.appendChild(rect);
}

function gui_move_selection_rectangle(cid, x1, y1, x2, y2) {
    var points_array = [x1 + 0.5, y1 + 0.5, x2 + 0.5, y1 + 0.5,
                  x2 + 0.5, y2 + 0.5, x1 + 0.5, y2 + 0.5];
    var rect = get_item(cid, 'selection_rectangle');
    configure_item(rect, { points: points_array });
}

function gui_hide_selection_rectangle(cid) {
//    gui_post("hiding selection");
    var rect = get_item(cid, 'selection_rectangle');
    rect.parentElement.removeChild(rect);
}

// iemguis

function gui_create_bng(cid, tag, cx, cy, radius) {
    var g = get_gobj(cid, tag);
    var circle = create_item(cid, 'circle', {
        cx: cx,
        cy: cy,
        r: radius,
        'shape-rendering': 'auto',
        fill: 'none',
        stroke: 'black',
        'stroke-width': 1,
        id: tag + 'button'
    });
    g.appendChild(circle);
}

function gui_bng_flash(cid, tag, color) {
    var button = get_item(cid, tag + 'button');
    configure_item(button, { fill: color });
}

function gui_create_toggle(cid, tag, color, width, state, p1,p2,p3,p4,p5,p6,p7,p8,basex,basey) {
    var g = get_gobj(cid, tag);
    var points_array = [p1 - basex, p2 - basey,
                        p3 - basex, p4 - basey
    ];
    var cross1 = create_item(cid, 'polyline', {
        points: points_array.join(" "),
        stroke: color,
        fill: 'none',
        id: tag + 'cross1',
        display: state ? 'inline' : 'none',
        'stroke-width': width
    });

    points_array = [p5 - basex, p6 - basey,
                    p7 - basex, p8 - basey
    ];
    var cross2 = create_item(cid, 'polyline', {
        points: points_array.join(" "),
        stroke: color,
        fill: 'none',
        id: tag + 'cross2',
        display: state ? 'inline' : 'none',
        'stroke-width': width
    });
    g.appendChild(cross1);
    g.appendChild(cross2);
}

function gui_toggle_resize_cross(cid,tag,w,p1,p2,p3,p4,p5,p6,p7,p8,basex,basey) {
    var g = get_gobj(cid, tag);
    var points_array = [p1 - basex, p2 - basey,
                        p3 - basex, p4 - basey
    ];
    var cross1 = get_item(cid, tag + 'cross1');
    configure_item(cross1, {
        points: points_array.join(" "),
        'stroke-width': w
    });

    points_array = [p5 - basex, p6 - basey,
                    p7 - basex, p8 - basey
    ];
    var cross2 = get_item(cid, tag + 'cross2');
    configure_item(cross2, {
        points: points_array.join(" "),
        'stroke-width': w
    });
}

function gui_toggle_update(cid, tag, state, color) {
    var cross1 = get_item(cid, tag + 'cross1');
    var cross2 = get_item(cid, tag + 'cross2');
    if (!!state) {
        configure_item(cross1, { display: 'inline', stroke: color });
        configure_item(cross2, { display: 'inline', stroke: color });
    } else {
        configure_item(cross1, { display: 'none', stroke: color });
        configure_item(cross2, { display: 'none', stroke: color });
    }
}

// Todo: send fewer parameters from c
function gui_create_numbox(width,cid,tag,bgcolor,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,basex,basey,half, is_toplevel) {
gui_post("basex is " + basex + " and basey is " + basey);
    // numbox doesn't have a standard iemgui border, so we must create its gobj manually
    var g = gui_text_create_gobj(cid, tag, "iemgui", basex, basey, is_toplevel)
    var data_array = ['M', p1 - basex, p2 - basey,
                      'L', p3 - basex, p4 - basey,
                           p5 - basex, p6 - basey,
                           p7 - basex, p8 - basey,
                           p9 - basex, p10 - basey,
                      'z',
                      'L', basex - basex, basey - basey,
                           half, half,
                           p9 - basex, p10 - basey];
    var border = create_item(cid, 'path', {
        d: data_array.join(" "),
        fill: bgcolor,
        stroke: 'black',
        'stroke-width': 1,
        id: (tag + 'border'),
        'class': 'border'
    });
    g.appendChild(border);
}

function gui_numbox_drawtext(cid,tag,text,font_size,color,xpos,ypos,basex,basey) {
    var g = get_gobj(cid, tag);
    var svg_text = create_item(cid, 'text', {
        x: xpos - basex,
        y: ypos - basey + 5,
        'font-size': font_size,
        fill: color,
        id: tag + 'text'
    });

    var text_node = patchwin[cid].window.document.createTextNode(text);
    svg_text.appendChild(text_node);
    g.appendChild(svg_text);
}

function gui_update_numbox(cid, tag, fcolor, bgcolor, font_name, font_size, font_weight) {
gui_post("inside update_numbox fcolor is " + fcolor);
    var b = get_item(cid, tag + 'border');
    var text = get_item(cid, tag + 'text');
    configure_item(b, { fill: bgcolor });
    configure_item(text, { fill: fcolor, 'font-size': font_size });
}

function gui_create_slider(cid,tag,color,p1,p2,p3,p4,basex, basey) {
    var g = get_gobj(cid, tag);
    var indicator = create_item(cid, 'line', {
        x1: p1 - basex,
        y1: p2 - basey,
        x2: p3 - basex,
        y2: p4 - basey,
        stroke: color,
        'stroke-width': 3,
        fill: 'none',
        id: tag + 'indicator'
    });
    g.appendChild(indicator);

}

function gui_slider_update(cid,tag,p1,p2,p3,p4,basex,basey) {
    var indicator = get_item(cid, tag + 'indicator');
    configure_item(indicator, {
        x1: p1 - basex,
        y1: p2 - basey,
        x2: p3 - basex,
        y2: p4 - basey
    });
}

function gui_slider_indicator_color(cid, tag, color) {
    var i = get_item(cid, tag + 'indicator');
    configure_item(i, {
        stroke: color
    });
}

function gui_create_radio(cid,tag,p1,p2,p3,p4,i,basex,basey) {
    var g = get_gobj(cid, tag);
    var cell = create_item(cid, 'line', {
        x1: p1 - basex,
        y1: p2 - basey,
        x2: p3 - basex,
        y2: p4 - basey,
        // stroke is just black for now
        stroke: 'black',
        'stroke-width': 1,
        fill: 'none',
        id: tag + 'cell_' + i
    });
    g.appendChild(cell);
}

function gui_create_radio_buttons(cid,tag,color,p1,p2,p3,p4,basex,basey,i,state) {
    var g = get_gobj(cid, tag);
    var b = create_item(cid, 'rect', {
        x: p1 - basex,
        y: p2 - basey,
        width: p3 - p1,
        height: p4 - p2,
        stroke: color,
        fill: color,
        id: tag + 'button_' + i,
        display: state ? 'inline' : 'none'
    });
    g.appendChild(b);
}

function gui_radio_button_coords(cid, tag, x1, y1, xi, yi, i, s, d, orient) {
    var button = get_item(cid, tag + 'button_' + i);
    var cell = get_item(cid, tag + 'cell_' + i);
    // the line to draw the cell for i=0 doesn't exist. Probably was not worth
    // the effort, but it's easier just to check for that here atm.
    if (i > 0) {
        configure_item(cell, {
            x1: orient ? 0 : xi,
            y1: orient ? yi : 0,
            x2: orient ? d : xi,
            y2: orient ? yi : d
        });
    }
    configure_item(button, {
        x: orient ? s : xi+s,
        y: orient ? yi+s : s,
        width: d-(s*2),
        height: d-(s*2)
    });
}

function gui_radio_update(cid,tag,bgcolor,prev,next) {
    var prev = get_item(cid, tag + 'button_' + prev);
    var next = get_item(cid, tag + 'button_' + next);
    configure_item(prev, { display: 'none', fill: bgcolor, stroke: bgcolor });
    configure_item(next, { display: 'inline', fill: bgcolor, stroke: bgcolor });
}

function gui_create_vumeter_text(cid,tag,color,xpos,ypos,text,index,basex,basey) {
    var g = get_gobj(cid, tag);
    var svg_text = create_item(cid, 'text', {
        x: xpos - basex,
        y: ypos - basey,
        //  font-size: font);
        id: tag + 'text_' + index
    });

    var text_node = patchwin[cid].window.document.createTextNode(text);
    svg_text.appendChild(text_node);
    g.appendChild(svg_text);
}

// Oh, what a terrible interface this is!
// the c API for vumeter was just spewing all kinds of state changes
// at tcl/tk, depending on it to just ignore non-existent objects.
// On changes in the Properties dialog, it would
// a) remove all the labels
// b) configure a bunch of _non-existent_ labels
// c) recreate all the missing labels
// To get on to other work we just parrot the insanity here,
// and silently ignore calls to update non-existent text.
function gui_update_vumeter_text(cid, tag, text, font, selected, color, i) {
    var svg_text = get_item(cid, tag + 'text_' + i);
    if (!selected) {
        // Hack...
        if (svg_text !== null) {
            configure_item(svg_text, { fill: color });
        }
    }
}

function gui_vumeter_text_coords(cid, tag, i, xpos, ypos, basex, basey) {
    var t = get_item(cid, tag + 'text_' + i);
    configure_item(t, { x: xpos - basex, y: ypos - basey });
}

function gui_erase_vumeter_text(cid, tag, i) {
    var t = get_item(cid, tag + 'text_' + i);
    t.parentNode.removeChild(t);
}

function gui_create_vumeter_steps(cid,tag,color,p1,p2,p3,p4,width,index,basex,basey,i) {
    var g = get_gobj(cid, tag);
    var l = create_item(cid, 'line', {
        x1: p1 - basex,
        y1: p2 - basey,
        x2: p3 - basex,
        y2: p4 - basey,
        stroke: color,
        'stroke-width': width,
        'id': tag + 'led_' + i
    });
    g.appendChild(l);
}

function gui_update_vumeter_steps(cid, tag, i, width) {
    var step = get_item(cid, tag + 'led_' + i);
    configure_item(step, { 'stroke-width': width });
}

function gui_update_vumeter_step_coords(cid,tag,i,x1,y1,x2,y2,basex,basey) {
gui_post("updating step coords...");
    var l = get_item(cid, tag + 'led_' + i);
    configure_item(l, {
        x1: x1 - basex,
        y1: y1 - basey,
        x2: x2 - basex,
        y2: y2 - basey
    });
}

function gui_create_vumeter_rect(cid,tag,color,p1,p2,p3,p4,basex,basey) {
    var g = get_gobj(cid, tag);
    var rect = create_item(cid, 'rect', {
        x: p1 - basex,
        y: p2 - basey,
        width: p3 - p1,
        height: p4 + 1 - p2,
        stroke: color,
        fill: color,
        id: tag + 'rect'
    });
    g.appendChild(rect);
}

function gui_update_vumeter_rect(cid, tag, color) {
    var r = get_item(cid, tag + 'rect');
    configure_item(r, { fill: color, stroke: color });
}

/* Oh hack upon hack... why doesn't the iemgui base_config just take care of this? */
function gui_vumeter_border_coords(cid, tag, width, height) {
    var r = get_item(cid, tag + 'border');
    configure_item(r, { width: width, height: height });
}

function gui_update_vumeter_peak(cid, tag, width) {
    var r = get_item(cid, tag + 'rect');
    configure_item(r, { 'stroke-width': width });
}

function gui_create_vumeter_peak(cid,tag,color,p1,p2,p3,p4,width,basex,basey) {
    var g = get_gobj(cid, tag);
    var line = create_item(cid, 'line', {
        x1: p1 - basex,
        y1: p2 - basey,
        x2: p3 - basex,
        y2: p4 - basey,
        stroke: color,
        'stroke-width': width,
        id: tag + 'peak'
    });
    g.appendChild(line);
}

// probably should change tag from "rect" to "cover"
function gui_vumeter_update_rms(cid,tag,p1,p2,p3,p4,basex,basey) {
    var rect = get_item(cid, tag + 'rect');
    configure_item(rect, {
        x: p1 - basex,
        y: p2 - basey,
        width: p3 - p1,
        height: p4 - p2 + 1
    });
}

function gui_vumeter_update_peak(cid,tag,color,p1,p2,p3,p4,basex,basey) {
    var line = get_item(cid, tag + 'peak');
    configure_item(line, {
        x1: p1 - basex,
        y1: p2 - basey,
        x2: p3 - basex,
        y2: p4 - basey,
        stroke: color
    });
}

// Think about merging with gui_text_drawborder
function gui_iemgui_drawborder(cid, tag, bgcolor, x1, y1, x2, y2) {
    var g = get_gobj(cid, tag);
    var rect = create_item(cid, 'rect', {
        width: x2 - x1,
        height: y2 - y1,
        fill: bgcolor,
        stroke: 'black',
        'shape-rendering': 'optimizeSpeed',
        'stroke-width': 1,
        class: 'border'
//        id: tag + 'border'
    });
    g.appendChild(rect);
}

function gui_iemgui_move_and_resize(cid, tag, x1, y1, x2, y2) {
    var gobj = get_gobj(cid, tag);
    var item = gobj.querySelector('.border');
    elem_move(gobj, x1, y1);
    configure_item(item, {
        width: x2 - x1,
        height: y2 - y1
    });
}

function gui_iemgui_label_new(cid, tag, x, y, color, text, font) {
    var g = get_gobj(cid, tag);
    var svg_text = create_item(cid, 'text', {
        // x and y need to be relative to baseline instead of nw anchor
        x: x,
        y: y,
//        'font-size': font + 'px',
        id: tag + 'label'
    });

    var text_node = patchwin[cid].window.document.createTextNode(text);
    svg_text.appendChild(text_node);
    g.appendChild(svg_text);
    var foo = patchwin[cid].window.document.getElementById(tag + 'label');
//    console.log("foo is " + foo);
//    console.log("label_new tag is " + tag);
}

function gui_iemgui_label_set(cid, tag, text) {
    get_item(cid, tag + 'label').textContent = text;
}

function gui_iemgui_label_coords(cid, tag, x, y) {
    var svg_text = get_item(cid, tag + 'label');
    configure_item(svg_text, {
        x: x,
        y: y
    });
}

function gui_iemgui_label_color(cid, tag, color) {
    var svg_text = get_item(cid, tag + 'label');
    configure_item(svg_text, {
        fill: color
    });
}

function gui_iemgui_label_select(cid, tag, is_selected) {
    var svg_text = get_item(cid, tag + 'label');
//    console.log("tag is " + tag + " and svg_text is " + svg_text);
    if (is_selected) {
        svg_text.classList.add('iemgui_label_selected'); 
    } else {
        svg_text.classList.remove('iemgui_label_selected'); 
    }
}

function gui_iemgui_label_font(cid, tag, font) {
    var svg_text = get_item(cid, tag + 'label');
    // This has to wait until we remove the tcl formatting
    // that Pd uses for font name/size
//    configure_item(svg_text, {
//        font: font
//    });
}

function gui_create_mycanvas(cid,tag,color,x1,y1,x2_vis,y2_vis,x2,y2) {
    var rect_vis = create_item(cid,'rect', {
        width: x2_vis - x1,
        height: y2_vis - y1,
        fill: color,
        stroke: color,
        id: tag + 'rect'
        }
    );

    // we use a drag_handle-- unlike a 'border' it takes
    // the same color as the visible rectangle when deselected
    var rect = create_item(cid,'rect', {
        width: x2 - x1,
        height: y2 - y1,
        fill: 'none',
        stroke: color,
        id: tag + 'drag_handle',
        'class': 'border'
        }
    );
    var g = get_gobj(cid,tag);
    g.appendChild(rect_vis);
    g.appendChild(rect);
}

function gui_update_mycanvas(cid, tag, color, selected) {
    var r = get_item(cid, tag + 'rect');
    var h = get_item(cid, tag + 'drag_handle');
    configure_item(r, { fill: color, stroke: color });
//    if (!selected) {
//        configure_item(h, { stroke: color });
//    }
}

function gui_mycanvas_coords(cid, tag, vis_width, vis_height, select_width, select_height) {
    var r = get_item(cid, tag + 'rect');
    var h = get_item(cid, tag + 'drag_handle');
    configure_item(r, { width: vis_width, height: vis_height });
    configure_item(h, { width: select_width, height: select_height });
}

function gui_mycanvas_select_color(cid,tag,color) {
//    var item = get_item(cid,tag + 'drag_handle');
//    configure_item(item, {stroke: color});
}
 
function gui_create_scalar(cid, tag, isselected, t1, t2, t3, t4, t5, t6,
    is_toplevel) {
    // we should probably use create_gobj here, but we're doing some initial 
    // scaling that normal gobjs don't need...
    var svg = get_item(cid, "patchsvg"); // "patchsvg" is id for the svg in the DOM
    // Normally put objects on half-pixels to make them crisp, but if we create a
    // scalar in an object box we already did that. This unfortunately creates a 0.5
    // pix discrepancy between scalars created in object boxes and ones created with
    // [append].  Think about just using shape-rendering value of 'crispEdges' in the
    // places where it matters...
    t5 += 0.5;
    t6 += 0.5;
    var matrix = [t1,t2,t3,t4,t5,t6];
    var transform_string = 'matrix(' + matrix.join() + ')';
    var g = create_item(cid, 'g', {
            id: tag + 'gobj',
            transform: transform_string,
    });
    if (isselected !== 0) {
        g.classList.add('selected');
    }
    if (is_toplevel === 0) {
        g.classList.add('gop');
    }
    // Let's make a selection rect... but we can't make it
    // a child of the gobj group because the getrect fn gives
    // us a bbox in the canvas coord system
    var selection_rect = create_item(cid, 'rect', {
//        id: tag + 'selection_rect',
        class: 'border',
        display: 'none',
        fill: 'none',
        'pointer-events': 'none'
    });
    g.appendChild(selection_rect);
    add_gobj_to_svg(svg, g);
//    gui_post("made a scalar...");
    return g;
}

function gui_scalar_erase(cid, tag) {
    var g = get_gobj(cid, tag);
    if (g !== null) {
        g.parentNode.removeChild(g);
    }
    // selection rect...
//    var sr = get_item(cid, tag + 'selection_rect');
//    sr.parentNode.removeChild(sr);
}

function gui_scalar_draw_select_rect(cid, tag, state, x1, y1, x2, y2, basex, basey) {
    // This is unnecessarily complex-- the select rect is a child of the parent
    // scalar group, but in the initial Tkpath API the rect was free-standing.  This
    // means all the coordinate parameters are in the screen position. But we need
    // the coords relative to the scalar's x/y-- hence we subtract the scalar's basex/basey
    // from the coords below
    // Additionally, we're not actually drawing the rect here.  It's drawn
    // as part of the scalar_vis function.  We're merely changing its coords
    // and size.
    // Finally, we have this awful display attribute toggling in css
    // for selected borders because somehow calling properties on a graph
    // triggers this function.  I have no idea why it does that.
    //gui_post("drawselectrect: " + x1 + " " + y1 + " " + x2 + " " + y2 + " " + basex + " " + basey);
    var g = get_gobj(cid, tag);
    var b = g.querySelector('.border');
    configure_item(b, {
        x: (x1 - basex),
        y: (y1 - basey),
        width: x2 - x1,
        height: y2 - y1,
    });
}

function gui_create_scalar_group(cid, tag, parent_tag, attr_array) {
    var parent = get_item(cid, parent_tag);
    if (attr_array === undefined) {
        attr_array = [];
    }
    attr_array.push("id", tag);
    var g = create_item(cid, 'g', attr_array);
    parent.appendChild(g);
    return g; 
}

function gui_scalar_configure_gobj(cid, tag, isselected, t1, t2, t3, t4, t5, t6) {
    var gobj = get_gobj(cid, tag);
    var matrix = [t1,t2,t3,t4,t5,t6];
    var transform_string = 'matrix(' + matrix.join() + ')';
    configure_item(gobj, { transform: transform_string });
}

function gui_draw_vis(cid, type, attr_array, tag_array) {
    gui_post("inside gui_draw_vis");
    for(var i = 0; i < arguments.length; i++) {
        gui_post("arg1 is " + arguments[i]);
    } 
//    gui_post("coords is " + coords);
//    gui_post("coords is array: " + Array.isArray(coords));
    gui_post("arguments[2] is " + arguments[2]);
//    gui_post("type of coords is " + typeof coords);
    gui_post("type of arguments[2] is " + typeof arguments[2]);
    gui_post("arguments[2] is array: " + Array.isArray(arguments[2]));
    var g = get_item(cid, tag_array[0]);
    if (g !== null) {
        gui_post("our parent exists.");
    } else {
        gui_post("our parent doe not exists.");
    }
    //var ca = coords;


/*
    switch(type) {
        case 'rect':
            attr_array.push('x', coords[0]);
            attr_array.push('y', coords[1]);
            attr_array.push('width', coords[2]);
            attr_array.push('height',coords[3]);
            break;
        case 'circle':
            type = 'ellipse';
        case 'ellipse':
            attr_array.push('cx',coords[0]);
            attr_array.push('cy',coords[1]);
            attr_array.push('rx',coords[2]);
            attr_array.push('ry',coords[3]);
            break;
        case 'line':
            attr_array.push('x1',coords[0]); 
            attr_array.push('y1',coords[1]);
            attr_array.push('x2',coords[2]);
            attr_array.push('y2',coords[3]);
            break;
        case 'polyline':
        case 'polygon':
            attr_array.push('points',coords.join(" "));
            break;
        case 'path':
            attr_array.push('d',coords.join(" ")); 
            break;
    }
*/
    attr_array.push('id', tag_array[1]);
    gui_post("create is " + tag_array[1]);
    var item = create_item(cid, type, attr_array);
    if (item !== null) {
        gui_post("we got create.");
    } else {
        gui_post("we doe not got creat.");
    }
    g.appendChild(item);
}

function gui_draw_erase_item(cid, tag) {
    gui_post("baleting... tag is " + tag);
    var item = get_item(cid, tag);
    if (item !== null) {
        item.parentNode.removeChild(item);
    } else {
        gui_post("uh oh... gui_draw_erase_item couldn't find the item...");
    }
}

function gui_draw_coords(cid, tag, shape, points) {
    var elem = get_item(cid, tag);
    switch (shape) {
        case "rect":
            configure_item(elem, {
                x: points[0],
                y: points[1],
                width: points[2],
                height: points[3]
            });
            break;
        case "circle":
            configure_item(elem, {
                cx: points[0],
                cy: points[1]
            });
            break;
    }
}

// Configure one attr/val pair at a time, received from Pd
function gui_draw_configure(cid, tag, attr, val) {
    var item = get_item(cid, tag);
    var obj = {};
    if (Array.isArray(val)) {
        obj[attr] = val.join(" ");
    } else {
        // strings or numbers
        obj[attr] = val;
    }
    configure_item(item, obj);
}

// Configure multiple attr/val pairs (this should be merged with gui_draw_configure at some point
function gui_draw_configure_all(cid, tag, attr_array) {
    var item = get_item(cid, tag);
    configure_item(item, attr_array);
}

// Plots for arrays and data structures
function gui_plot_vis(cid, basex, basey, data_array, attr_array, tag_array) {
    var g = get_item(cid, tag_array[0]);
    var p = create_item(cid, 'path', {
        d: data_array.join(" "),
        id: tag_array[1],
//        stroke: 'red',
//        fill: 'black',
//        'stroke-width': '0'
    });
    configure_item(p, attr_array);
    if (g !== null) {
        g.appendChild(p);
    }
}

function add_popup(cid, popup) {
    popup_menu[cid] = popup;
}

exports.add_popup = add_popup;

// Kludge to get popup coords to fit the browser's zoom level
function zoom_kludge(zoom_level) {
    var zfactor;
    switch(zoom_level) {
        case -7: zfactor = 0.279; break;
        case -6: zfactor = 0.335; break;
        case -5: zfactor = 0.402; break;
        case -4: zfactor = 0.483; break;
        case -3: zfactor = 0.58; break;
        case -2: zfactor = 0.695; break;
        case -1: zfactor = 0.834; break;
        case 1: zfactor = 1.2; break;
        case 2: zfactor = 1.44; break;
        case 3: zfactor = 1.73; break;
        case 4: zfactor = 2.073; break;
        case 5: zfactor = 2.485; break;
        case 6: zfactor = 2.98; break;
        case 7: zfactor = 3.6; break;
        case 8: zfactor = 4.32; break;
        default: zfactor = 1;
    }
    return zfactor;
}

function gui_canvas_popup(cid, xpos, ypos, canprop, canopen, isobject) {
    gui_post("canvas_popup called... " + JSON.stringify(arguments));
    // Set the global popup x/y so they can be retrieved by the relevant doc's event handler
    var zoom_level = patchwin[cid].zoomLevel;
    var zfactor = zoom_kludge(zoom_level);
    popup_coords[0] = xpos;
    popup_coords[1] = ypos;
    xpos = Math.floor(xpos * zfactor);
    ypos = Math.floor(ypos * zfactor);
    gui_post("xpos is " + xpos + " and ypos is " + ypos);
//    popup_coords[0] = xpos;
//    popup_coords[1] = ypos;
    popup_menu[cid].items[0].enabled = canprop;
    popup_menu[cid].items[1].enabled = canopen;

    // We'll use "isobject" to enable/disable "To Front" and "To Back"
    //isobject;
    
    // Get page coords for top of window, in case we're scrolled
    var left = patchwin[cid].window.document.body.scrollLeft;
    var top = patchwin[cid].window.document.body.scrollTop;

    popup_menu[cid].popup(xpos - Math.floor(left * zfactor),
        ypos - Math.floor(top * zfactor));
}

function popup_action(cid, index) {
gui_post("popup coords are: " + popup_coords.join(" "));
    pdsend(cid + " done-popup " + index + " " + popup_coords.join(" "));
}

exports.popup_action = popup_action;


// Graphs and Arrays

// Doesn't look like we needs this

//function gui_graph_drawborder(cid, tag, x1, y1, x2, y2) {
//    var g = get_gobj(cid, tag);
//    var b = create_item(cid, 'rect', {
//        width: x2 - x1,
//        height: y2 - y1,
//        stroke: 'black',
//        fill: 'none',
//        id: tag
//    });
//    g.appendChild(b);
//}

// refactor-- use a class so this can happen in css
function gui_graph_fill_border(cid, tag) {
    var i;
    var g = get_gobj(cid, tag);
    var b = g.querySelectorAll('.border');
    for (i = 0; i < b.length; i++) {
        configure_item(b[i], {
            fill: 'gray'
        });
    }
}

function gui_graph_deleteborder(cid, tag) {
    var b = get_item(cid, tag);
    b.parentNode.removeChild(b);
}

function gui_graph_label(cid, tag, y, array_name, font, font_size,
    font_weight, is_selected) {
//    var graph = get_item( 
}

function gui_graph_vtick(cid, tag, x, up_y, down_y, tick_pix, basex, basey) {
    var g = get_gobj(cid, tag);
    // Don't think these need an ID...
    var up_tick = create_item(cid, 'line', {
        stroke: 'black',
        x1: x - basex,
        y1: up_y - basey,
        x2: x - basex,
        y2: up_y - tick_pix - basey
    });
    var down_tick = create_item(cid, 'line', {
        stroke: 'black',
        x1: x - basex,
        y1: down_y - basey,
        x2: x - basex,
        y2: down_y + tick_pix - basey
    });
    g.appendChild(up_tick);
    g.appendChild(down_tick);
}

function gui_graph_htick(cid, tag, y, r_x, l_x, tick_pix, basex, basey) {
    var g = get_gobj(cid, tag);
    // Don't think these need an ID...
    var left_tick = create_item(cid, 'line', {
        stroke: 'black',
        x1: l_x - basex,
        y1: y - basey,
        x2: l_x - tick_pix - basex,
        y2: y - basey,
        id: "fuckoff" + y
    });
    var right_tick = create_item(cid, 'line', {
        stroke: 'black',
        x1: r_x - basex,
        y1: y - basey,
        x2: r_x + tick_pix - basex,
        y2: y - basey
    });
    g.appendChild(left_tick);
    g.appendChild(right_tick);
}

function gui_graph_tick_label(cid, tag, x, y, text, font, font_size, font_weight, basex, basey) {
    var g = get_gobj(cid, tag);
    var svg_text = create_item(cid, 'text', {
        // need a label "y" relative to baseline
        x: x - basex,
        y: y - basey,
        'font-size': font_size,
    });

    var text_node = patchwin[cid].window.document.createTextNode(text);
    svg_text.appendChild(text_node);
    g.appendChild(svg_text);
}

function gui_canvas_drawredrect(cid, x1, y1, x2, y2) {
    var svgelem = get_item(cid, 'patchsvg');
    var b = create_item(cid, 'rect', {
        x: x1,
        y: y1,
        width: x2 - x1,
        height: y2 - y1,
        stroke: 'red',
        id: 'GOP'
    });
    svgelem.appendChild(b);
}

function gui_canvas_deleteredrect(cid) {
    var r = get_item(cid, 'GOP');
    // We need to check for existence here, because the first
    // time setting GOP in properties, there is no red rect yet.
    // But setting properties when the subpatch's window is
    // visible calls glist_redraw, and glist_redraw will try to delete
    // the red rect _before_ it's been drawn in this case.
    // Unfortunately, it's quite difficult to refactor those c
    // functions without knowing the side effects.  But ineffectual
    // gui calls should really be minimized-- otherwise it's simply
    // too difficult to debug what's being passed over the socket.
    if (r !== null) {
        r.parentNode.removeChild(r);
    }
}

// Magic Glass (aka Cord Inspector)

// For clarity, this probably shouldn't be a gobj.  Also, it might be easier to
// make it a div that lives on top of the patchsvg
function gui_create_cord_inspector(cid) {
    var g = get_gobj(cid, 'cord_inspector');
    var ci_rect = create_item(cid, 'rect', { id: 'cord_inspector_rect' });
    var ci_poly = create_item(cid, 'polygon', { id: 'cord_inspector_polygon' });
    var ci_text = create_item(cid, 'text', { id: 'cord_inspector_text' });
    var text_node = patchwin[cid].window.document.createTextNode('');
    ci_text.appendChild(text_node);
    g.appendChild(ci_rect);
    g.appendChild(ci_poly);
    g.appendChild(ci_text);
}

function gui_cord_inspector_update(cid, text, basex, basey, bg_size, y1, y2, moved) {
    var gobj = get_gobj(cid, 'cord_inspector');
    gobj.setAttributeNS(null, 'transform',
            'translate(' + (basex + 10.5) + ',' + (basey + 0.5) + ')');
    gobj.setAttributeNS(null, 'pointer-events', 'none');
    var rect = get_item(cid, 'cord_inspector_rect');
    var poly = get_item(cid, 'cord_inspector_polygon');
    var svg_text = get_item(cid, 'cord_inspector_text');
    // Lots of fudge factors here, tailored to the current default font size
    configure_item(rect, {
        x: 13,
        y: y1 - basey,
        width: bg_size - basex,
        height: y2 - basey + 10,
        fill: 'none',
        stroke: 'black'
    });
    var polypoints_array = [8,0,13,5,13,-5];
     configure_item(poly, {
        points: polypoints_array.join()
    });
    configure_item(svg_text, {
        x: 20,
        y: 5,
        fill: 'black'
    });
    // set the text
    svg_text.textContent = text;
}

function gui_erase_cord_inspector(cid) {
    var ci = get_gobj(cid, 'cord_inspector');
    if (ci !== null) {
        ci.parentNode.removeChild(ci);
    } else {
        gui_post("oops, trying to erase cord inspector that doesn't exist!");
    }
}

function gui_cord_inspector_flash(cid) {
    var ct = get_item(cid, 'cord_inspector_text');
    if (ct !== null) {
        configure_item(ct, { fill: 'red' });
    } else {
        gui_post("gui_cord_inspector_flash: trying to flash a non-existent cord inspector!");
    }
}


// Window functions

function gui_raise_window(cid) {
    patchwin[cid].focus();
}

// Openpanel and Savepanel

var file_dialog_target;

function file_dialog(cid, type, target, path) {
    file_dialog_target = target;
    var query_string = (type === 'open' ?
        'openpanel_dialog' : 'savepanel_dialog');
    var d = patchwin[cid].window.document.querySelector('#' + query_string);
    gui_post("set path to " + path);
    d.setAttribute("nwworkingdir", path);
    d.click();
}

function gui_openpanel(cid, target, path) {
    file_dialog(cid, "open", target, path);
}

function gui_savepanel(cid, target, path) {
    file_dialog(cid, "save", target, path);
}

exports.file_dialog_callback = function(file_string) {
    pdsend(file_dialog_target + " callback " + enquote(file_string));
}

function gui_gatom_dialog(did, attr_array) {
    gui_post("fuck tits");
    dialogwin[did] = nw_create_window(did, 'gatom', 265, 540, 20, 20, 0,
        0, 1, 'white', 'Properties', '', 0, null, attr_array);
}

function gui_iemgui_dialog(did, attr_array) {
    gui_post("got a gfxstub " + did + "!!!");
   
//    for (var i = 0; i < attr_array.length; i++) {
//        attr_array[i] = '"' + attr_array[i] + '"';
//    }
    dialogwin[did] = nw_create_window(did, 'iemgui', 265, 450, 20, 20, 0,
        0, 1, 'white', 'Properties', '', 0, null, attr_array);

}

function gui_create_array(did, count) {
    gui_post("trying to create an array...");
    var attr_array = [
        "array_gfxstub", did,
        "array_name", 'array' + count,
        "array_size", 100,
        "array_flags", 3,
        "array_fill", 'black',
        "array_outline", 'black',
        "array_in_existing_graph", 0
    ];
    dialogwin[did] = nw_create_window(did, 'canvas', 265, 340, 20, 20, 0,
        0, 1, 'white', 'Properties', '', 0, null, attr_array);    
}

function gui_canvas_dialog(did, attr_arrays) {
    var i, j, inner_array;
    gui_post("got a gfxstub " + did + "!!!");
    gui_post("attr_arrays are " + attr_arrays);
//    for (i = 0; i < attr_arrays.length; i++) {
//        inner_array = attr_arrays[i];
//        if (inner_array !== undefined) {
//            for (j = 0; j < inner_array.length; j++) {
//                inner_array[i] = '"' + inner_array[i] + '"';
//            }
//        }
//    }
//    dialogwin[did] = nw_create_window(did, 'canvas', 265, 340, 20, 20, 0,
//        0, 1, 'white', 'Properties', '', 0, null, attr_arrays);

    dialogwin[did] = nw_create_window(did, 'canvas', 250, 100, 20, 20, 0,
        0, 1, 'white', 'Properties', '', 0, null, attr_arrays);



}

function gui_remove_gfxstub(did) {
gui_post("did is " + did + " and dialogwin[did] is " + dialogwin[did]);
    if (dialogwin[did] !== undefined && dialogwin[did] !== null) {
        dialogwin[did].window.close(true);
        dialogwin[did] = null;
    }
}

function gui_font_dialog(cid, gfxstub, font_size) {
    var attrs = { canvas: cid, font_size: font_size };
    dialogwin[gfxstub] = nw_create_window(gfxstub, 'font', 265, 540, 20, 20, 0,
        0, 1, 'white', 'Properties', '', 0, null, attrs);
}

// Global settings

function gui_pd_dsp(state) {
    if (pd_window !== undefined) {
        pd_window.document.getElementById('dsp_control').checked = !!state;
    }
}

function open_prefs() {
    if (!dialogwin['prefs']) {
        dialogwin['prefs'] = nw_create_window('prefs', 'prefs',
            265, 540, 20, 20, 0,
            0, 1, 'white', 'Properties', '', 0, null, null);
    }
}

exports.open_prefs = open_prefs;

function gui_audio_properties(gfxstub, sys_indevs, sys_outdevs, 
    pd_indevs, pd_inchans, pd_outdevs, pd_outchans, audio_attrs) {
    
    var attrs = audio_attrs.concat([
        "audio-apis", pd_apilist,
        "sys-indevs", sys_indevs,
        "sys-outdevs", sys_outdevs,
        "pd-indevs", pd_indevs,
        "pd-inchans", pd_inchans,
        "pd-outdevs", pd_outdevs,
        "pd-outchans", pd_outchans
        ]);

    gui_post("got back some audio props...");
    //for (var i = 0; i < arguments.length; i++) {
    //    gui_post("arg " + i + " is " + arguments[i]);
    //}

    if (dialogwin['prefs'] !== null) {
        dialogwin['prefs'].eval(null,
            'audio_prefs_callback('  +
            JSON.stringify(attrs) + ');'
        );
    }
}

function gui_midi_properties(gfxstub, sys_indevs, sys_outdevs,
    pd_indevs, pd_outdevs, midi_attrs) {
    
    var attrs = midi_attrs.concat([
        "midi-apis", pd_midiapilist,
        "sys-indevs", sys_indevs,
        "sys-outdevs", sys_outdevs,
        "pd-indevs", pd_indevs,
        "pd-outdevs", pd_outdevs,
        ]);

    //gui_post("got back some midi props...");
    //for (var i = 0; i < arguments.length; i++) {
    //    gui_post("arg " + i + " is " + arguments[i]);
    //}

    if (dialogwin['prefs'] !== null) {
        dialogwin['prefs'].eval(null,
            'midi_prefs_callback('  +
            JSON.stringify(attrs) + ');'
        );
    }
}

// Let's try a closure for gui skins
exports.skin = (function () {
    var dir = 'css/';
    var preset = 'default';
    var w;
    function apply(win) {
        win.window.document.getElementById('page_style')
            .setAttribute('href', dir + preset + '.css');
    }
    return {
        get: function () {
            gui_post("getting preset: " + dir + preset + '.css');
            return dir + preset + '.css';
        },
        set: function (name) {
            preset = name;
            gui_post("trying to set...");
            for (w in patchwin) {
                if (patchwin.hasOwnProperty(w)) {
                    apply(patchwin[w]);
                }
            }
        },
        apply: function (nw_window) {
            apply(nw_window);
        }
    };
}());

function select_text(cid, elem) {
        var range, win = patchwin[cid].window;
        if (win.document.selection) {
            range = win.document.body.createTextRange();
            range.moveToElementText(elem);
            range.select();
        } else if (win.getSelection) {
            range = win.document.createRange();
            range.selectNodeContents(elem);
            win.getSelection().addRange(range);
        }
}

function gui_textarea(cid, tag, x, y, max_char_width, text, font_size, state) {
    //gui_post("x/y is " + x + '/' + y);
    //gui_post("state? " + state);
    gui_post("tag is " + tag);
    var range, svg_view;
    var gobj = get_gobj(cid, tag);
    if (state !== 0) {
        // Hide the gobj while we edit
        configure_item(gobj, { display: 'none' });
        var p = patchwin[cid].window.document.createElement('p');
        configure_item(p, {
            id: 'new_object_textentry'
        });
        svg_view = patchwin[cid].window.document.getElementById('patchsvg')
            .viewBox.baseVal;
        p.contentEditable = 'true';
        p.style.setProperty('left', (x - svg_view.x) + 'px');
        p.style.setProperty('top', (y - svg_view.y) + 'px');
        p.style.setProperty('font-size', font_size + 'px');
        p.style.setProperty('transform', 'translate(0px, 0px)');
        p.style.setProperty('max-width',
            max_char_width === 0 ? '60ch' : max_char_width + 'ch');
        p.style.setProperty('min-width',
            max_char_width === 0 ? '3ch' : max_char_width + 'ch');
        p.textContent = text;
        patchwin[cid].window.document.body.appendChild(p);
        p.focus();
        select_text(cid, p);
        if (state === 1) {
            patchwin[cid].window.canvas_events.text();
        } else {
            patchwin[cid].window.canvas_events.floating_text();
        }
    } else {
        configure_item(gobj, { display: 'inline' });
        var p = patchwin[cid].window.document.getElementById('new_object_textentry');
        if (p !== null) {
            p.parentNode.removeChild(p);
        }
        patchwin[cid].window.canvas_events.normal();
    }
}

function gui_undo_menu(cid, undo_text, redo_text) {
    // we have to check if the window exists, because Pd starts
    // up with two unvis'd patch windows used for garrays
    if (cid !== 'nobody' && patchwin[cid] !== undefined) {
        patchwin[cid].window.nw_undo_menu(undo_text, redo_text);
    }
}

function gui_canvas_getscroll(cid) {
    var svg = get_item(cid, 'patchsvg');
    var bbox = svg.getBBox();
    var width = bbox.x > 0 ? bbox.x + bbox.width : bbox.width,
        height = bbox.y > 0 ? bbox.y + bbox.height : bbox.height;
    if (width === 0) {
        width = patchwin[cid].window.innerWidth;
    }
    if (height === 0) {
        height = patchwin[cid].window.innerHeight;
    }
    configure_item(svg, {
        viewBox: [bbox.x > 0 ? 0 : bbox.x,
                  bbox.y > 0 ? 0 : bbox.y,
                  width,
                  height] 
                  .join(" ")
    });
    svg.width.baseVal.valueAsString = width;
    svg.height.baseVal.valueAsString = height;
//    console.log("x is " + bbox.x);
//    console.log("y is " + bbox.x);
//    console.log("width is " + bbox.width);
//    console.log("height is " + bbox.height);
}

// handling the selection
function gui_lower(cid, tag) {
    var svg = patchwin[cid].window.document.getElementById('patchsvg'),
        first_child = svg.firstElementChild,
        selection = null,
        gobj, len, i;
    if (tag === 'selected') {
        selection = svg.getElementsByClassName('selected');
    } else {
        gobj = get_gobj(cid, tag);
        if (gobj !== null) {
            selection = [gobj];
        }
    }
    if (selection !== null) {
        len = selection.length;
        for (i = len - 1; i >= 0; i--) {
            svg.insertBefore(selection[i], first_child);
        }
    }
}

// This only differs from gui_raise by setting first_child to
// the cord element instead of the first element in the svg.  Really,
// all three of these should be combined into a single function (plus
// all the silly logic on the C side moved here
function gui_raise(cid, tag) {
    var svg = patchwin[cid].window.document.getElementById('patchsvg'),
        first_child = svg.querySelector('.cord'),
        selection = null,
        gobj, len, i;
    if (tag === 'selected') {
        selection = svg.getElementsByClassName('selected');
    } else {
        gobj = get_gobj(cid, tag);
        if (gobj !== null) {
            selection = [gobj];
        }
    }
    if (selection !== null) {
        len = selection.length;
        for (i = len - 1; i >= 0; i--) {
            svg.insertBefore(selection[i], first_child);
        }
    }
}

function gui_find_lowest_and_arrange(cid, reference_element_tag, objtag) {
    var ref_elem = get_gobj(cid, reference_element_tag),
        svg = patchwin[cid].window.document.getElementsByClassName('patchsvg'),
        selection = null,
        gobj,
        len,
        i;
    if (ref_elem !== null) {
        if (objtag === 'selected') {
            selection = 
            svg.getElementsByClassName('selected');
        } else {
            gobj = get_gobj(cid, objtag);
            if (gobj !== null) {
                selection = [get_gobj(cid, objtag)];
            }
        }
        if (selection !== null) {
            len = selection.length;
            for (i = len - 1; i >= 0; i--) {
                svg.insertBefore(selection[i], ref_elem);
            }
        }
    }
}
