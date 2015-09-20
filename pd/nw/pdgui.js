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
var path = require('path'); // for path.dirname path.extname path.join

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
    var ret = (path.resolve(myPath) ===
        path.normalize(myPath).replace(/(.+)([\/]\\])$/, '$1'));
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
        my_a.href =
            "javascript:pdgui.pd_error_select_by_id('" + objectid + "')";
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

// convert canvas dimensions to old tcl/tk geometry
// string format. Unfortunately this is exposed (and
// documented) to the user with the "relocate" message
// in both Pd-Extended and Pd-Vanilla.  So we have to
// keep it here for backwards compatibility.
function pd_geo_string(w, h, x, y) {
    return  [w,'x',h,'+',x,'+',y].join("");
}

// In tcl/tk, this function had some checks to apparently
// keep from sending a "relocate" message to Pd, but I'm
// not exactly clear on how it works. If this ends up being
// a cpu hog, check out pdtk_canvas_checkgeometry in the old
// pd.tk
function canvas_check_geometry(cid) {
    var win_w = patchwin[cid].width,
        win_h = patchwin[cid].height,
        win_x = patchwin[cid].x,
        win_y = patchwin[cid].y,
        cnv_width = patchwin[cid].window.innerWidth,
        cnv_height = patchwin[cid].window.innerHeight;
    // We're reusing win_x and win_y below, as it
    // shouldn't make a difference to the bounds
    // algorithm in Pd
    pdsend(cid, "relocate",
           pd_geo_string(win_w, win_h, win_x, win_y),
           pd_geo_string(cnv_width, cnv_height, win_x, win_y)
    );
}

exports.canvas_check_geometry = canvas_check_geometry;

function menu_save(name) {
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
    pdsend(cid, "savetofile", enquote(basename), enquote(directory));

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
    pdsend("pd filename",
           "Untitled-" + untitled_number,
           enquote(untitled_directory));
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
    var reply = patchwin[cid_for_dialog].window
        .confirm("Save changes to " + title + "?");
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
    var directory = path.dirname(file);
    var basename = path.basename(file);
    var cyclist;
    if (basename.match(/\.(pat|mxb|help)$/) !=null) {
        gui_post("warning: opening pat|mxb|help not implemented yet");
        if (pd_nt == 0) {
            // on GNU/Linux, cyclist is installed into /usr/bin usually
            cyclist = "/usr/bin/cyclist";
        } else {
            cyclist = pd_guidir + "/bin/cyclist"
        }
        //convert Max binary to text .pat
        // The following is tcl code which needs to get converted
        // to javascript...
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
        pdsend("pd open", enquote(basename), enquote(directory));
        pd_opendir = directory;
        //::pd_guiprefs::update_recentfiles "$filename" 1
    }
}

// Doesn't work yet... need to figure out how to send command line args
// (files) to be opened by the unique instance 
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

// This doesn't work at the moment.  Not sure how to feed the command line
// filelist to a single instance of node-webkit.
function gui_check_unique (unique) {
    // global appname
    return;
    var final_filenames = new Array;
    var startup_dir = pwd;
    if (unique == 0) {
        var filelist_length = startup_files.length;
        for (var i = 0; i < filelist_length; i++) {
            var file = startup_files[i];
            var dir;
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
    console.log("Starting up...");
    // # tb: user defined typefaces
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
    pdsend("pd init", enquote(pwd), "0", font_fixed_metrics);

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
    var shift = evt.shiftKey ? 1 : 0,
        repeat = evt.repeat ? 1 : 0;
    pdsend(cid, "key", state, char_code, shift, 1, repeat);
}

exports.gui_canvas_sendkey = gui_canvas_sendkey;

function title_callback(cid, title) {
    patchwin[cid].window.document.title = title;
}

function format_window_title(name, dirty_flag, args, dir) {
        return name + " " + (dirty_flag ? "*" : "") + args + " - " + dir;
}

exports.format_window_title = format_window_title;

// This should be used when a file is saved with the name changed
// (and maybe in other situations)
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
    // Not sure why resize and topmost are here-- but we'll pass them on for
    // the time being...
    patchwin[cid] = nw_create_window(cid, 'pd_canvas', width, height,
        xpos, ypos, menu_flag, resize[cid], topmost[cid], my_canvas_color,
        name, dir, dirty_flag, cargs, null);
    // initialize variable to reflect that this window has been opened
    loaded[cid] = 1;
    //#pdtk_standardkeybindings $cid.c
}

/* This gets sent to Pd to trigger each object on the canvas
   to do its "vis" function. The result will be a flood of messages
   back from Pd to the GUI to draw these objects */
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
                    //gui_post("warning: old command: " + old_command_output,
                    //    'blue');
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
function pdsend() {
    var string = Array.prototype.join.call(arguments, ' ');
    client.write(string + ';');
    // for now, let's reprint the outgoing string to the pdwindow
    //gui_post(string + ';', 'red');
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
    var item = patchwin[cid].window.document
        .createElementNS('http://www.w3.org/2000/svg', type);
    if (args !== null) {
        configure_item(item, args);
    }
    return item;
}

// Similar to [canvas itemconfigure], without the need for a reference
// to the canvas
function configure_item(item, attributes) {
    // draw_vis from g_template sends attributes
    // as a ['attr1',val1, 'attr2', val2, etc.] array,
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

// The GUI side probably shouldn't know about "items" on SVG.
function gui_configure_item(cid, tag, attributes) {
    var item = get_item(cid, tag);
    configure_item(item, attributes);
}

function add_gobj_to_svg(svg, gobj) {
    svg.insertBefore(gobj, svg.querySelector('.cord'));
}

// Most of these map either to pd.tk procs, or in some cases
// Tk canvas subcommands
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
    
    // hm... why returning g and not the return value of appendChild?
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
        'shape-rendering': 'crispEdges',
        class: 'border'
    });
    if (isbroken === 1) {
        rect.classList.add('broken_border');
    }
    g.appendChild(rect);
}

function gui_canvas_drawio(cid, parenttag, tag, x1, y1, x2, y2, basex, basey,
    type, i, is_signal, is_iemgui) {
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
        class: 'border'
        //id: tag + 'border'
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
        //id: tag + 'border'
    });
    g.appendChild(polygon);
}

// draw a patch cord
function gui_canvas_line(cid,tag,type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10) {
    var d_array = ['M', p1 + 0.5, p2 + 0.5,
                   'Q', p3 + 0.5, p4 + 0.5, p5 + 0.5, p6 + 0.5,
                   'Q', p7 + 0.5, p8 + 0.5 ,p9 + 0.5, p10 + 0.5];
    var svg = get_item(cid, "patchsvg");
    var path = create_item(cid, 'path', {
        d: d_array.join(" "),
        fill: 'none',
        'shape-rendering': 'optimizeSpeed',
        id: tag,
        'class': 'cord ' + type
    });
    svg.appendChild(path);
}

function gui_canvas_select_line(cid, tag) {
    var line = get_item(cid, tag);
    if (line !== null) {
        line.classList.add('selected_line');
    } else {
        gui_post("gui_canvas_select_line: can't find line");
    }
}

function gui_canvas_deselect_line(cid, tag) {
    var line = get_item(cid, tag);
    if (line !== null) {
        line.classList.remove('selected_line');
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
        gui_post("canvas_delete_line: something is borked because the line doesn't exist");
    }
}

function gui_canvas_updateline(cid,tag,x1,y1,x2,y2,yoff) {
    var halfx = parseInt((x2 - x1)/2);
    var halfy = parseInt((y2 - y1)/2);
    var d_array = ['M',x1,y1,
                  'Q',x1,y1+yoff,x1+halfx,y1+halfy,
                  'Q',x2,y2-yoff,x2,y2];
    var cord = get_item(cid, tag);
    configure_item(cord, { d: d_array.join(" ") });
}

function text_line_height_kludge(fontsize, fontsize_type) {
    var pd_fontsize = fontsize_type === 'gui' ?
        gui_fontsize_to_pd_fontsize(fontsize) :
        fontsize;
    switch (pd_fontsize) {
        case 8: return 11;
        case 10: return 13;
        case 12: return 16;
        case 16: return 19;
        case 24: return 29;
        case 36: return 44;
        default: return gui_fontsize + 2;
    }
}

function text_to_tspans(canvasname, svg_text, text) {
    var lines, i, len, tspan, fontsize;
    lines = text.split('\n'); 
    len = lines.length;
    // Get fontsize (minus the trailing "px")
    fontsize = svg_text.getAttribute('font-size').slice(0, -2);

    for (i = 0; i < len; i++) {
        tspan = create_item(canvasname, 'tspan', {
            dy: i == 0 ? 0 : text_line_height_kludge(+fontsize, 'gui') + 'px',
            x: 0
        });
        // find a way to abstract away the canvas array and the DOM here
        var text_node = patchwin[canvasname].window.document.createTextNode(lines[i]);
        tspan.appendChild(text_node);
        svg_text.appendChild(tspan);
    }
}

// To keep the object and message box size consistent
// with Pd-Vanilla, we make small changes to the font
// sizes before rendering. If this impedes readability
// we can revisit the issue. Even Pd-Vanilla's box sizing
// changed at version 0.43, so we can break as well if
// it comes to that.
function gobj_fontsize_kludge(fontsize, return_type) {
    // These were tested on an X60 running Trisquel (based
    // on Ubuntu)
    var fontmap = {
        // pd_size: gui_size
        8: 8.33,
        12: 11.65,
        16: 16.65,
        24: 23.3,
        36: 36.6
    };
    var ret, prop;
    if (return_type === 'gui') {
        ret = fontmap[fontsize];
        return ret ? ret : fontsize;
    } else {
        for (prop in fontmap) {
            if (fontmap.hasOwnProperty(prop)) {
                if (fontmap[prop] == fontsize) {
                    return +prop;
                }
            }
        }
        return fontsize;
    }
}

function pd_fontsize_to_gui_fontsize(fontsize) {
    return gobj_fontsize_kludge(fontsize, 'gui');
}

function gui_fontsize_to_pd_fontsize(fontsize) {
    return gobj_fontsize_kludge(fontsize, 'pd');
}

// Another hack, similar to above
function gobj_font_y_kludge(fontsize) {
    switch (fontsize) {
        case 8: return -0.5;
        case 10: return -1;
        case 12: return -1;
        case 16: return -1.5;
        case 24: return -3;
        case 36: return -6;
        default: return 0;
    }
}

function gui_text_new(canvasname, myname, type, isselected, left_margin, font_height, text, font) {
    var lines, i, len, tspan;
    var g = get_gobj(canvasname, myname);
    var svg_text = create_item(canvasname, 'text', {
        // Maybe it's just me, but the svg spec's explanation of how
        // text x/y and tspan x/y interact is difficult to understand.
        // So here we just translate by the right amount for the left-margin,
        // guaranteeing all tspan children will line up where they should be
        transform: 'translate(' + left_margin + ')',
        y: font_height + gobj_font_y_kludge(font),
        // Turns out we can't do 'hanging' baseline
        // because it's borked when scaled. Bummer...
        // 'dominant-baseline': 'hanging',
        'shape-rendering': 'crispEdges',
        'font-size': pd_fontsize_to_gui_fontsize(font) + 'px',
        'font-weight': 'normal',
        id: myname + 'text'
    });

    // trim off any extraneous leading/trailing whitespace. Because of
    // the way binbuf_gettext works we almost always have a trailing
    // whitespace.
    text = text.trim();
    // fill svg_text with tspan content by splitting on '\n'
    text_to_tspans(canvasname, svg_text, text);

    if (g !== null) {
        g.appendChild(svg_text);
    } else {
        gui_post("gui_text_new: can't find parent group " + myname);
    }

    if (isselected) {
        gui_gobj_select(canvasname, myname);
    }
}

// Because of the overly complex code path inside
// canvas_setgraph, multiple erasures can be triggered in a row.
function gui_gobj_erase(cid, tag) {
    var g = get_gobj(cid, tag);
    if (g !== null) {
        g.parentNode.removeChild(g);
    } else {
        gui_post("gui_gobj_erase: gobj " + tag +
            " didn't exist in the first place!");
    }
}

function gui_text_set (cid, tag, text) {
    var svg_text = get_item(cid, tag + 'text');
    if (svg_text !== null) {
        // trim leading/trailing whitespace
        text = text.trim();
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

function gui_gobj_select(cid, tag) {
    var g = get_gobj(cid, tag);
    if (g !== null) {
        g.classList.add('selected');
    } else {
        console.log("text_select: something wrong with group tag: " + tag);
    }
}

function gui_gobj_deselect(cid, tag) {
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
        //var elem = ol[i].transform.baseVal.getItem(0);
        //var new_tx = dx + elem.matrix.e; 
        //var new_ty = dy + elem.matrix.f; 
        //elem.matrix.e = new_tx;
        //elem.matrix.f = new_ty;
    }
    textentry = patchwin[name].window.document.getElementById('new_object_textentry');
    if (textentry !== null) {
        textentry_displace(textentry, dx, dy); 
    }
    //elem.setAttributeNS(null, 'transform',
    //'translate(' + new_tx + ',' + new_ty + ')');
    //}
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

// Oh hack upon hack... why doesn't the iemgui base_config just take care
// of this?
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
        //id: tag + 'border'
    });
    g.appendChild(rect);
}

function gui_iemgui_base_color(cid, tag, color) {
    var b = get_gobj(cid, tag).querySelector('.border');
    configure_item(b, { fill: color });
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

function iemgui_font_height(name, size) {
    return size;
    var dejaVuSansMono = {
        6: [3, 4], 7: [4, 5], 8: [5, 7], 9: [5, 7], 10: [6, 8],
        11: [7, 8], 12: [7, 9], 13: [8, 9], 14: [8, 10], 15: [9, 12],
        16: [9, 12], 17: [10, 13], 18: [10, 13], 19: [11, 14], 20: [12, 14],
        21: [12, 16], 22: [13, 16], 23: [13, 17], 24: [14, 18], 25: [14, 18],
        26: [15, 20], 27: [16, 20], 28: [16, 21], 29: [17, 21], 30: [17, 22],
        31: [18, 22], 32: [18, 23], 33: [19, 25], 34: [19, 25], 35: [20, 26],
        36: [21, 26], 37: [21, 27], 38: [22, 27], 39: [22, 29], 40: [23, 30],
        41: [23, 30], 42: [24, 31], 43: [24, 31], 44: [25, 33], 45: [26, 33],
        46: [25, 34], 47: [26, 34], 48: [26, 35], 49: [27, 36], 50: [26, 36],
        51: [28, 37], 52: [29, 38], 53: [29, 39], 54: [30, 39], 55: [30, 41],
        56: [31, 41], 57: [31, 42], 58: [32, 43], 59: [32, 43], 60: [32, 45],
        61: [34, 45], 62: [34, 46], 63: [35, 46], 64: [35, 47], 65: [36, 49],
        66: [36, 49], 67: [36, 50], 68: [37, 50], 69: [38, 51], 70: [38, 51],
        71: [38, 52], 72: [39, 52]
    };
    // We use these heights for both the monotype and iemgui's "Helvetica"
    // which, at least on linux, has the same height
    if (name === 'DejaVu Sans Mono' || name == 'helvetica') {
        return dejaVuSansMono[size][1];
    } else {
        return size;
    }
}

function iemgui_fontfamily(name) {
    var family = "DejaVu Sans Mono";
    if (name === "DejaVu Sans Mono") {
        family = "DejaVu Sans Mono"; // probably should add some fallbacks here 
    }
    else if (name === "helvetica") {
        family = "Helvetica, 'DejaVu Sans'";
    }
    else if (name === "times") {
        family = "'Times New Roman', 'DejaVu Serif', 'FreeSerif', serif";
    }
    return family;
}

function gui_iemgui_label_new(cid, tag, x, y, color, text, fontname, fontweight,
    fontsize) {
    var g = get_gobj(cid, tag);
    var svg_text = create_item(cid, 'text', {
        // x and y need to be relative to baseline instead of nw anchor
        x: x,
        y: y,
        //'font-size': font + 'px',
        'font-family': iemgui_fontfamily(fontname),
        // for some reason the font looks bold in Pd-Vanilla-- not sure why
        'font-weight': fontweight,
        'font-size': fontsize + 'px',
        // Iemgui labels are anchored "w" (left-aligned to non-tclers).
        // For no good reason, they are also centered vertically, unlike
        // object box text. Since svg text uses the baseline as a reference
        // by default, we just take half the pixel font size and use that
        // as an additional offset.
        //
        // There is an alignment-baseline property in svg that
        // is supposed to do this for us. However, when I tried choosing
        // "hanging" to get tcl's equivalent of "n", I ran into a bug
        // where the text gets positioned incorrectly when zooming.
        transform: 'translate(0,' +
            iemgui_font_height(fontname, fontsize) / 2 + ')',
        id: tag + 'label'
    });
    var text_node = patchwin[cid].window.document.createTextNode(text);
    svg_text.appendChild(text_node);
    g.appendChild(svg_text);
    var foo = patchwin[cid].window.document.getElementById(tag + 'label');
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
    if (is_selected) {
        svg_text.classList.add('iemgui_label_selected'); 
    } else {
        svg_text.classList.remove('iemgui_label_selected'); 
    }
}

function gui_iemgui_label_font(cid, tag, fontname, fontweight, fontsize) {
    var svg_text = get_item(cid, tag + 'label');
    configure_item(svg_text, {
        'font-family': iemgui_fontfamily(fontname),
        'font-weight': fontweight,
        'font-size': fontsize + 'px',
        transform: 'translate(0,' + iemgui_font_height(fontname, fontsize) / 2 + ')'
    });
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
        'class': 'border mycanvas_border'
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
}

function gui_mycanvas_coords(cid, tag, vis_width, vis_height, select_width, select_height) {
    var r = get_item(cid, tag + 'rect');
    var h = get_item(cid, tag + 'drag_handle');
    configure_item(r, { width: vis_width, height: vis_height });
    configure_item(h, { width: select_width, height: select_height });
}

// Not needed anymore
function gui_mycanvas_select_color(cid,tag,color) {
    //var item = get_item(cid,tag + 'drag_handle');
    //configure_item(item, {stroke: color});
}
 
function gui_create_scalar(cid, tag, isselected, t1, t2, t3, t4, t5, t6,
    is_toplevel) {
    // we should probably use create_gobj here, but we're doing some initial 
    // scaling that normal gobjs don't need...
    var svg = get_item(cid, "patchsvg"); // id for the svg in the DOM
    // Normally put objects on half-pixels to make them crisp, but if we create
    // a scalar in an object box we already did that. This unfortunately
    // creates a 0.5 pix discrepancy between scalars created in object boxes
    // and ones created with [append].  Think about just using shape-rendering
    // value of 'crispEdges' in the places where it matters...
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
        //id: tag + 'selection_rect',
        class: 'border',
        display: 'none',
        fill: 'none',
        'pointer-events': 'none'
    });
    g.appendChild(selection_rect);
    add_gobj_to_svg(svg, g);
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
    // scalar group, but in the initial Tkpath API the rect was free-standing.
    // This means all the coordinate parameters are in the screen position. But
    // we need the coords relative to the scalar's x/y-- hence we subtract the
    // scalar's basex/basey from the coords below.

    // Additionally, we're not actually drawing the rect here.  It's drawn
    // as part of the scalar_vis function.  We're merely changing its coords
    // and size.

    // Finally, we have this awful display attribute toggling in css
    // for selected borders because somehow calling properties on a graph
    // triggers this function.  I have no idea why it does that.
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
    var g = get_item(cid, tag_array[0]);
    attr_array.push('id', tag_array[1]);
    var item = create_item(cid, type, attr_array);
    g.appendChild(item);
}

// This is a stop gap to update the old draw commands like [drawpolygon]
// without having to erase and recreate their DOM elements
function gui_draw_configure_old_command(cid, type, attr_array, tag_array) {
    var elem = get_item(cid, tag_array[1]);
    if (elem !== null) {
    configure_item(elem, attr_array);
    }
}

function gui_draw_erase_item(cid, tag) {
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
        case "polyline":
        case "polygon":
            configure_item(elem, {
                points: points
            });
            break;
        default:
    }
}

var gui_draw_drag_event = (function() {
    var last_mousedown,
        last_mouseup,
        last_mousemove;
    return function gui_draw_drag_event(cid, tag, scalar_sym,
        drawcommand_sym, event_name, state) {
        var last_x,
            last_y,
            item = get_item(cid, tag),
            doc = patchwin[cid].window.document,
            mousemove = function(e) {
                var new_x = e.pageX,
                    new_y = e.pageY;
                pdsend(cid, "scalar_event", scalar_sym, drawcommand_sym,
                    event_name, new_x - last_x, new_y - last_y);
                last_x = new_x;
                last_y = new_y;
            },
            mousedown = function(e) {
                if (e.target === item) {
                    last_x = e.pageX;
                    last_y = e.pageY;
                    doc.addEventListener("mousemove", mousemove, false);
                }
            },
            mouseup = function(e) {
                doc.removeEventListener("mousemove", mousemove, false);
            }
        ;
        // Go ahead and remove any event listeners
        doc.removeEventListener("mousedown", last_mousedown, false);
        doc.removeEventListener("mouseup", last_mouseup, false);
        doc.removeEventListener("mousemove", last_mousemove, false);

        // Set mousedown and mouseup events to create our "drag" event
        if (state === 1) {
            doc.addEventListener("mousedown", mousedown, false);
            doc.addEventListener("mouseup", mouseup, false);
            last_mousemove = mousemove;
            last_mouseup = mouseup;
            last_mousedown = mousedown;
        }
    }
}());

function gui_draw_event(cid, tag, scalar_sym, drawcommand_sym, event_name,
    state) {
    var item = get_item(cid, tag),
        event_type = "on" + event_name; 
    if (state === 1) {
        item[event_type] = function(e) {
            pdsend(cid, "scalar_event", scalar_sym, drawcommand_sym, event_name,
                e.pageX, e.pageY);
        };
    } else {
        item[event_type] = null;
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
        //stroke: 'red',
        //fill: 'black',
        //'stroke-width': '0'
    });
    configure_item(p, attr_array);
    if (g !== null) {
        g.appendChild(p);
    }
}

// This function doubles as a visfn for drawnumber. Furthermore it doubles
// as a way to update attributes for drawnumber/symbol without having to
// recreate the object. The "flag" argument is 1 for creating a new element,
// and -1 to set attributes on the existing object.
function gui_drawnumber_vis(cid, parent_tag, tag, x, y, scale_x, scale_y,
    font, fontsize, fontcolor, text, flag, visibility) {
    var lines, i, len, tspan;
    var g = get_item(cid, parent_tag);
    var svg_text;
    if (flag === 1) {
        svg_text = create_item(cid, 'text', {
            // x and y are fudge factors. Text on the tk canvas used an anchor
            // at the top-right corner of the text's bbox.  SVG uses the
            // baseline. There's probably a programmatic way to do this, but
            // for now-- fudge factors based on the DejaVu Sans Mono font. :)

            // For an explanation of why we translate by "x" instead of setting
            // the x attribute, see comment in gui_text_new
            transform: 'scale(' + scale_x + ',' + scale_y + ') ' +
                       'translate(' + x + ')',
            y: y + fontsize,
            // Turns out we can't do 'hanging' baseline because it's borked
            // when scaled. Bummer...
            // 'dominant-baseline': 'hanging',
            'shape-rendering': 'optimizeSpeed',
            'font-size': fontsize + 'px',
            fill: fontcolor,
            visibility: visibility === 1 ? 'normal' : 'hidden',
            id: tag
        });
        // fill svg_text with tspan content by splitting on '\n'
        text_to_tspans(cid, svg_text, text);
        if (g !== null) {
            g.appendChild(svg_text);
        } else {
            gui_post("gui_drawnumber: can't find parent group" + parent_tag);
        }
    } else {
        svg_text = get_item(cid, tag);
        configure_item(svg_text, {
            transform: 'scale(' + scale_x + ',' + scale_y + ') ' +
                       'translate(' + x + ')',
            y: y,
            // Turns out we can't do 'hanging' baseline because it's borked
            // when scaled. Bummer...
            // 'dominant-baseline': 'hanging',
            'shape-rendering': 'optimizeSpeed',
            'font-size': font + 'px',
            fill: fontcolor,
            visibility: visibility === 1 ? 'normal' : 'hidden',
            id: tag
        });
        svg_text.textContent = "";
        text_to_tspans(cid, svg_text, text);
    }
}

var drawimage_data = {}; // for storing base64 image data associated with
                         // each [draw image] command
exports.flub = drawimage_data;

function gui_drawimage_new(obj_tag, file_path, canvasdir, flags) {
    var drawsprite = 1,
        image_seq,
        i,
        matchchar = '*',
        files,
        ext,
        img; // dummy image to measure width and height
    image_seq = flags & drawsprite;
    if (!path.isAbsolute(file_path)) {
        file_path = path.join(canvasdir, file_path);
    }
    file_path = path.normalize(file_path);
    if (fs.existsSync(file_path) && fs.lstatSync(file_path).isDirectory()) {

    }
    files = fs.readdirSync(file_path)
                .sort(); // Note that js's 'sort' method doesn't do the
                         // "right thing" for numbers. For that we'd need
                         // to provide our own sorting function
    drawimage_data[obj_tag] = []; // create empty array for base64 image data
    for (i = 0; i < files.length && i < 1000; i++) {
        ext = path.extname(files[i]);

    // todo: tolower()

        if (ext === '.gif' ||
            ext === '.jpg' ||
            ext === '.png' ||
            ext === '.jpeg' ||
            ext === '.svg') {

            gui_post("we got an image at index " + i + ": " + files[i]);
            // Now add an element to that array with the image data
            drawimage_data[obj_tag].push({
                type: ext === '.jpeg' ? 'jpg' : ext.slice(1),
                data: fs.readFileSync(path.join(file_path, files[i]),'base64')
            });
        }
    }
    gui_post("no of files: " + i);
    if (i > 0) {
        img = new pd_window.Image(); // create an image in the pd_window context
        img.onload = function() {
            pdsend(obj_tag, "size", this.width, this.height);
        };
        img.src = 'data:image/' + drawimage_data[obj_tag][0].type +
            ';base64,' + drawimage_data[obj_tag][0].data;
    } else {
        gui_post("drawimage: warning: no images loaded");
    }
}

function img_size_setter(cid, obj, obj_tag, i) {
    var img = new pd_window.window.Image();
    img.onload = function() {
        var w = this.width,
            h = this.height;
        configure_item(get_item(cid, obj_tag + i), {
            width: w,
            height: h
        });
    };
    img.src = 'data:image/' + drawimage_data[obj][i].type +
        ';base64,' + drawimage_data[obj][i].data;
}

function gui_drawimage_vis(cid, x, y, obj, data, seqno, parent_tag) {
    var item,
        g = get_item(cid, parent_tag), // main <g> within the scalar
        len = drawimage_data[obj].length,
        i,
        image_container,
        obj_tag = 'draw' + obj.slice(1) + '.' + data.slice(1);
    if (len < 1) {
        return;
    }
    // Wrap around for out-of-bounds sequence numbers
    if (seqno >= len || seqno < 0) {
        seqno %= len;
    }
    image_container = create_item(cid, 'g', {
        id: obj_tag
    });
    for (i = 0; i < len; i++) {
        item = create_item(cid, 'image', {
            x: x,
            y: y,
            id: obj_tag + i,
            visibility: seqno === i ? 'visible' : 'hidden',
            preserveAspectRatio: "xMinYMin meet"
        });
        item.setAttributeNS('http://www.w3.org/1999/xlink', 'href', 
            'data:image/' + drawimage_data[obj][i].type + ';base64,' +
             drawimage_data[obj][i].data);
        image_container.appendChild(item);
    }
    g.appendChild(image_container);

    // Hack to set correct width and height
    for (i = 0; i < len; i++) {
        img_size_setter(cid, obj, obj_tag, i);
    }
}

function gui_drawimage_index(cid, obj, data, index) {
    var obj_tag = 'draw' + obj.slice(1) + '.' + data.slice(1);
    var i,
        len = drawimage_data[obj].length,
        image_container = get_item(cid, obj_tag),
        last_image,
        image = image_container.childNodes[index],
        last_image = image_container.querySelectorAll('[visibility="visible"]');

    for (i = 0; i < last_image.length; i++) {
        configure_item(last_image[i], { visibility: 'hidden' });
    }
    configure_item(image, { visibility: 'visible' });
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
    // Set the global popup x/y so they can be retrieved by the relevant
    // document's event handler
    var zoom_level = patchwin[cid].zoomLevel;
    var zfactor = zoom_kludge(zoom_level);
    popup_coords[0] = xpos;
    popup_coords[1] = ypos;
    xpos = Math.floor(xpos * zfactor);
    ypos = Math.floor(ypos * zfactor);
    //popup_coords[0] = xpos;
    //popup_coords[1] = ypos;
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
    pdsend(cid, "done-popup", index, popup_coords.join(" "));
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
        id: "tick" + y
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

//  Cord Inspector (a.k.a. Magic Glass)

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
    gobj.classList.remove('flash');
    var rect = get_item(cid, 'cord_inspector_rect');
    var poly = get_item(cid, 'cord_inspector_polygon');
    var svg_text = get_item(cid, 'cord_inspector_text');
    // Lots of fudge factors here, tailored to the current default font size
    configure_item(rect, {
        x: 13,
        y: y1 - basey,
        width: bg_size - basex,
        height: y2 - basey + 10
    });
    var polypoints_array = [8,0,13,5,13,-5];
     configure_item(poly, {
        points: polypoints_array.join()
    });
    configure_item(svg_text, {
        x: 20,
        y: 5,
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

function gui_cord_inspector_flash(cid, state) {
    var ct = get_item(cid, 'cord_inspector_text');
    if (ct !== null) {
        if (state === 1) {
            ct.classList.add('flash');
        } else {
            ct.classList.remove('flash');
        }
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
    pdsend(file_dialog_target, "callback", enquote(file_string));
}

// Used to convert the ["key", "value"...] arrays coming from
// Pd to a javascript object
function attr_array_to_object(attr_array) {
    var i,
        len = attr_array.length,
        obj = {};
    for (i = 0; i < len; i += 2) {
        obj[attr_array[i]] = attr_array[i+1];
    }
    return obj;
}

function gui_gatom_dialog(did, attr_array) {
    dialogwin[did] = nw_create_window(did, 'gatom', 265, 300, 20, 20, 0,
        0, 1, 'white', 'Properties', '', 0, null,
        attr_array_to_object(attr_array)
    );
}

function gui_iemgui_dialog(did, attr_array) {
    //for (var i = 0; i < attr_array.length; i++) {
    //    attr_array[i] = '"' + attr_array[i] + '"';
    //}
    dialogwin[did] = nw_create_window(did, 'iemgui', 265, 450, 20, 20, 0,
        0, 1, 'white', 'Properties', '', 0, null,
        attr_array_to_object(attr_array));
}

function gui_create_array(did, count) {
    var attr_array = [{
        array_gfxstub: did,
        array_name: 'array' + count,
        array_size: 100,
        array_flags: 3,
        array_fill: 'black',
        array_outline: 'black',
        array_in_existing_graph: 0
    }];
    dialogwin[did] = nw_create_window(did, 'canvas', 265, 340, 20, 20, 0,
        0, 1, 'white', 'Properties', '', 0, null, attr_array);    
}

function gui_canvas_dialog(did, attr_arrays) {
    var i, j, inner_array, prop;
    // Convert array of arrays to an array of objects
    for (i = 0; i < attr_arrays.length; i++) {
        attr_arrays[i] = attr_array_to_object(attr_arrays[i]);
        for (prop in attr_arrays[i]) {
            if (attr_arrays[i].hasOwnProperty(prop)) {
                console.log("array: prop is " + prop);
            }
        }
    }
    dialogwin[did] = nw_create_window(did, 'canvas', 250, 100, 20, 20, 0,
        0, 1, 'white', 'Properties', '', 0, null, attr_arrays);
}

function gui_remove_gfxstub(did) {
    if (dialogwin[did] !== undefined && dialogwin[did] !== null) {
        dialogwin[did].window.close(true);
        dialogwin[did] = null;
    }
}

function gui_font_dialog(cid, gfxstub, font_size) {
    var attrs = { canvas: cid, font_size: font_size };
    dialogwin[gfxstub] = nw_create_window(gfxstub, 'font', 265, 265, 20, 20, 0,
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

function gui_textarea(cid, tag, type, x, y, max_char_width, text,
    font_size, state) {
    var range, svg_view;
    var gobj = get_gobj(cid, tag);
    if (state !== 0) {
        // Hide the gobj while we edit.  However, we want the gobj to
        // contribute to the svg's bbox-- that way when the new_object_textentry
        // goes away we still have the same dimensions.  Otherwise the user
        // can get strange jumps in the viewport when instantiating an object
        // at the extremities of the patch.
        // To solve this, we use 'visibility' instead of 'display', since it
        // still uses the hidden item when calculating the bbox.
        // (We can probably solve this problem by throwing in yet another
        // gui_canvas_getscroll, but this seems like the right way to go
        // anyway.)
        configure_item(gobj, { visibility: 'hidden' });
        var p = patchwin[cid].window.document.createElement('p');
        configure_item(p, {
            id: 'new_object_textentry'
        });
        svg_view = patchwin[cid].window.document.getElementById('patchsvg')
            .viewBox.baseVal;
        p.classList.add(type);
        p.contentEditable = 'true';
        p.style.setProperty('left', (x - svg_view.x) + 'px');
        p.style.setProperty('top', (y - svg_view.y) + 'px');
        p.style.setProperty('font-size',
            pd_fontsize_to_gui_fontsize(font_size) + 'px');
        p.style.setProperty('line-height',
            text_line_height_kludge(font_size, 'pd') + 'px');
        p.style.setProperty('transform', 'translate(0px, 0px)');
        p.style.setProperty('max-width',
            max_char_width === 0 ? '60ch' : max_char_width + 'ch');
        p.style.setProperty('min-width',
            max_char_width === 0 ? '3ch' : max_char_width + 'ch');
        // remove leading/trailing whitespace
        text = text.trim();
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
        configure_item(gobj, { visibility: 'normal' });
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

function do_getscroll(cid) {
    var svg = get_item(cid, 'patchsvg');
    // Not sure why I need to check for null here... I'm waiting for the
    // nw window to load before mapping the Pd canvas, so the patchsvg
    // should always exist.  Perhaps I also need to set an event for
    // document.onload as well...
    if (svg === null) { return; }
    var bbox = svg.getBBox();
    var width = bbox.x > 0 ? bbox.x + bbox.width : bbox.width,
        height = bbox.y > 0 ? bbox.y + bbox.height : bbox.height;
    if (width === 0) {
        width = patchwin[cid].window.document.body.clientWidth;
    }
    if (height === 0) {
        height = patchwin[cid].window.document.body.clientHeight;
    }
    // Since we don't do any transformations on the patchsvg,
    // let's try just using ints for the height/width/viewBox
    // to keep things simple.
    width |= 0; // drop everything to the right of the decimal point
    height |= 0;
    configure_item(svg, {
        viewBox: [bbox.x > 0 ? 0 : bbox.x,
                  bbox.y > 0 ? 0 : bbox.y,
                  width,
                  height] 
                  .join(" "),
        width: width,
        height: height
    });
}

var getscroll_var = {};

// We use a setTimeout here for two reasons:
// 1. nw.js has a nasty Renderer bug  when you try to modify the
//    window before the document has finished loading. To get
//    the error get rid of the setTimeout
// 2. This should protect the user from triggering a bunch of
//    re-layouts.  But this only works because I'm not updating
//    the view to follow the mouse-- for example, when
//    the user is dragging an object beyond the bounds of the
//    viewport. The tcl/tk version actually does follow the
//    mouse. In that case this setTimeout could keep the
//    graphics from displaying until the user releases the mouse,
//    which would be a buggy UI
function gui_canvas_getscroll(cid) {
    clearTimeout(getscroll_var);
    getscroll_var = setTimeout(do_getscroll, 250, cid);
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
        svg = patchwin[cid].window.document.getElementById('patchsvg'),
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

// Bindings for dialog menu of iemgui, canvas, etc.
exports.dialog_bindings = function(did) {
    var dwin = dialogwin[did].window;
    dwin.document.onkeydown = function(evt) {
        if (evt.keyCode === 13) { // enter
            dwin.ok();
        } else if (evt.keyCode === 27) { // escape
            dwin.cancel();
        }
    };
}

exports.resize_window = function(did) {
    var w = dialogwin[did].window.document.body.scrollWidth,
        h = dialogwin[did].window.document.body.scrollHeight;
    dialogwin[did].width = w;
    dialogwin[did].height = h;
}
