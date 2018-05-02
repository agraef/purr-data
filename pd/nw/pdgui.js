"use strict";

var pwd;
var lib_dir;
var help_path, browser_doc, browser_path, browser_init;
var pd_engine_id;

exports.set_pwd = function(pwd_string) {
    pwd = pwd_string;
}

exports.get_pwd = function() {
    return pwd;
}

function defunkify_windows_path(s) {
    var ret = s;
    if (process.platform === "win32") {
        ret = ret.replace(/\\/g, "/");
    }
    return ret;
}

exports.set_pd_engine_id = function (id) {
    pd_engine_id = id;
}

exports.defunkify_windows_path = defunkify_windows_path;

function gui_set_browser_config(doc_flag, path_flag, init_flag, helppath) {
    // post("gui_set_browser_config: " + helppath.join(":"));
    browser_doc = doc_flag;
    browser_path = path_flag;
    browser_init = init_flag;
    help_path = helppath;
    // AG: Start building the keyword index for dialog_search.html. We do this
    // here so that we can be sure that lib_dir and help_path are known already.
    // (This may also be deferred until the browser is launched for the first
    // time, depending on the value of browser_init.)
    if (browser_init == 1) make_index();
}

function gui_set_lib_dir(dir) {
    lib_dir = dir;
}

exports.get_lib_dir = function() {
    return lib_dir;
}

function get_pd_opendir() {
    if (pd_opendir) {
        return pd_opendir;
    } else {
        return pwd;
    }
}

exports.get_pd_opendir = get_pd_opendir;

function set_pd_opendir(dir) {
    pd_opendir = dir;
}

function gui_set_current_dir(dummy, dir_and_filename) {
    set_pd_opendir(path.dirname(dir_and_filename));
}

function gui_set_gui_preset(name) {
    skin.set(name);
}

exports.set_focused_patchwin = function(cid) {
    last_focused = cid;
}

// Keyword index (cf. dialog_search.html)

var fs = require("fs");
var path = require("path");
var dive = require("./dive.js"); // small module to recursively search dirs
var elasticlunr = require("./elasticlunr.js"); // lightweight full-text search engine in JavaScript, cf. https://github.com/weixsong/elasticlunr.js/

var index = elasticlunr();

index.addField("title");
index.addField("keywords");
index.addField("description");
//index.addField("body");
index.addField("path");
index.setRef("id");

function add_doc_to_index(filename, data) {
    var title = path.basename(filename, ".pd"),
        big_line = data.replace("\n", " "),
        keywords,
        desc;
        // We use [\s\S] to match across multiple lines...
        keywords = big_line
            .match(/#X text \-?[0-9]+ \-?[0-9]+ KEYWORDS ([\s\S]*?);/i),
        desc = big_line
            .match(/#X text \-?[0-9]+ \-?[0-9]+ DESCRIPTION ([\s\S]*?);/i);
        keywords = keywords && keywords.length > 1 ? keywords[1].trim() : null;
        desc = desc && desc.length > 1 ? desc[1].trim() : null;
        // Remove the Pd escapes for commas
        desc = desc ? desc.replace(" \\,", ",") : null;
        if (desc) {
            // format Pd's "comma atoms" as normal commas
            desc = desc.replace(" \\,", ",");
        }
    if (title.slice(-5) === "-help") {
        title = title.slice(0, -5);
    }
    index.addDoc({
        "id": filename,
        "title": title,
        "keywords": keywords,
        "description": desc
        //"body": big_line,
    });
}

function read_file(err, filename, stat) {
    if (!err) {
        if (filename.slice(-3) === ".pd") {
            // AG: We MUST read the files synchronously here. This might be a
            // performance issue on some systems, but if we don't do this then
            // we may open a huge number of files simultaneously, causing the
            // process to run out of file handles.
            try {
                var data = fs.readFileSync(filename, { encoding: "utf8", flag: "r" });
                add_doc_to_index(filename, data);
            } catch (read_err) {
                post("err: " + read_err);
            }
        }
    } else {
        // AG: Simply ignore missing/unreadable files and directories.
        // post("err: " + err);
    }
}

var index_done = false;
var index_started = false;

function finish_index() {
    index_done = true;
    post("finished building help index");
}

// AG: pilfered from https://stackoverflow.com/questions/21077670
function expand_tilde(filepath) {
    if (filepath[0] === '~') {
        return path.join(process.env.HOME, filepath.slice(1));
    }
    return filepath;
}

// AG: This is supposed to be executed only once, after lib_dir has been set.
// Note that dive() traverses lib_dir asynchronously, so we report back in
// finish_index() when this is done.
function make_index() {
    var doc_path = browser_doc?path.join(lib_dir, "doc"):lib_dir;
    var i = 0;
    var l = help_path.length;
    function make_index_cont() {
        if (i < l) {
            var doc_path = help_path[i++];
            // AG: These paths might not exist, ignore them in this case. Also
            // note that we need to expand ~ here.
            var full_path = expand_tilde(doc_path);
            fs.lstat(full_path, function(err, stat) {
                if (!err) {
                    post("building help index in " + doc_path);
                    dive(full_path, read_file, make_index_cont);
                } else {
                    make_index_cont();
                }
            });
        } else {
            finish_index();
        }
    }
    index_started = true;
    post("building help index in " + doc_path);
    dive(doc_path, read_file, browser_path?make_index_cont:finish_index);
}

// AG: This is called from dialog_search.html with a callback that expects to
// receive the finished index as its sole argument. We also build the index
// here if needed, using make_index, then simply wait until make_index
// finishes and finally invoke the callback on the resulting index.
function build_index(cb) {
    function build_index_worker() {
        if (index_done == true) {
            cb(index);
        } else {
            setTimeout(build_index_worker, 500);
        }
    }
    if (index_started == false) {
        make_index();
    }
    build_index_worker();
}

exports.build_index = build_index;

// Modules

var cp = require("child_process"); // for starting core Pd from GUI in OSX

var parse_svg_path = require("./parse-svg-path.js");

exports.parse_svg_path = parse_svg_path;

// local strings
var lang = require("./pdlang.js");

exports.get_local_string = lang.get_local_string;

var pd_window;
exports.pd_window;

// Turns out I messed this up. pd_window should really be an
// "nw window", so that you can use it to access all the
// nw window methods and settings.  Instead I set it to the
// DOM window object. This complicates things-- for example,
// in walk_window_list I have to take care when comparing
// patchwin[]-- which are nw windows-- and pd_window.
// I'm not sure of the best way to fix this. Probably we want to
// just deal with DOM windows, but that would mean abstracting
// out the stuff that deals with nw window size and
// positioning.
exports.set_pd_window = function(win) {
    pd_window = win;
    exports.pd_window = win;
}

var font_engine_sanity;

// Here we use an HTML5 canvas hack to measure the width of
// the text to check for a font rendering anomaly. Here's why:
//
// It was reported that Ubuntu 16.04, Arch-- and probably most other Gnu/Linux
// distros going forward-- all end up with text extending past the box border.
// The test_text below is the string used in the bug report.
// OSX, Windows, and older Gnu/Linux stacks (like Ubuntu 14.04) all render
// this text with a width that is within half a pixel of each other (+- 217).
//
// Newer versions of Ubuntu and Arch measured nearly 7 pixels wider.
//
// I don't know what the new Gnu/Linux stack is up to (and I don't have the
// time to spelunk) but it's out of whack with the rest of the desktop
// rendering engines. Worse, there's some kind of quantization going on that
// keeps the new Gnu/Linux stack from hitting anything close to the font
// metrics of Pd Vanilla.
//
// Anyhow, we check for the discrepancy and try our best not to make newer
// versions of Gnu/Linux distros look too shitty...
exports.set_font_engine_sanity = function(win) {
    var canvas = win.document.createElement("canvas"),
        ctx = canvas.getContext("2d"),
        test_text = "struct theremin float x float y";
    canvas.id = "font_sanity_checker_canvas";
    win.document.body.appendChild(canvas);
    ctx.font = "11.65px DejaVu Sans Mono";
    if (Math.floor(ctx.measureText(test_text).width) <= 217) {
        font_engine_sanity = true;
    } else {
        font_engine_sanity = false;
    }
    canvas.parentNode.removeChild(canvas);
}

exports.get_font_engine_sanity = function() {
    return font_engine_sanity;
}

function font_stack_is_maintained_by_troglodytes() {
    return !font_engine_sanity;
}

var nw_create_window;
var nw_close_window;
var nw_app_quit;
var nw_open_html;
var nw_open_textfile;
var nw_open_external_doc;

exports.set_new_window_fn = function (nw_context_fn) {
    nw_create_window = nw_context_fn;
}

exports.set_close_window_fn = function (nw_context_fn) {
    nw_close_window = nw_context_fn;
}

exports.set_open_html_fn = function (nw_context_fn) {
    nw_open_html = nw_context_fn;
}

exports.set_open_textfile_fn = function (nw_context_fn) {
    nw_open_textfile = nw_context_fn;
}

exports.set_open_external_doc_fn = function (nw_context_fn) {
    nw_open_external_doc = nw_context_fn;
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
    k12_mode = 0,         // should be set from argv ("0" is just a stopgap)
    k12_saveas_on_new, //
    autotips,          // tooltips
    magicglass,        // cord inspector
    window_prefs,      //retaining window-specific preferences
    pdtk_canvas_mouseup_name, // not sure what this does
    filetypes,         // valid file extensions for opening/saving (includes Max filetypes)
    untitled_number,   // number to increment for each new patch that is opened
    untitled_directory, // default directory where to create/save new patches
    popup_coords,       // 0: canvas x
                        // 1: canvas y
                        // 2: screen x
                        // 3: screen y
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

// This could probably be in pdgui.js
function add_keymods(key, evt) {
    var shift = evt.shiftKey ? "Shift" : "";
    var ctrl = evt.ctrlKey ? "Ctrl" : "";
    return shift + ctrl + key;
}

function cmd_or_ctrl_key(evt) {
    if (process.platform === "darwin") {
        return evt.metaKey;
    } else {
        return evt.ctrlKey;
    }
}

exports.cmd_or_ctrl_key = cmd_or_ctrl_key;

var last_keydown = "";

exports.keydown = function(cid, evt) {
    var key_code = evt.keyCode,
        hack = null, // hack for unprintable ascii codes
        cmd_or_ctrl
    switch(key_code) {
        case 8: // backspace
        case 9:
        case 10:
        case 27:
        //case 32:
        case 127: hack = key_code; break;
        case 46: hack = 127; break; // some platforms report 46 for Delete
        case 37: hack = add_keymods("Left", evt); break;
        case 38: hack = add_keymods("Up", evt); break;
        case 39: hack = add_keymods("Right", evt); break;
        case 40: hack = add_keymods("Down", evt); break;
        case 33: hack = add_keymods("Prior", evt); break;
        case 34: hack = add_keymods("Next", evt); break;
        case 35: hack = add_keymods("End", evt); break;
        case 36: hack = add_keymods("Home", evt); break;

        // These may be different on Safari...
        case 112: hack = add_keymods("F1", evt); break;
        case 113: hack = add_keymods("F2", evt); break;
        case 114: hack = add_keymods("F3", evt); break;
        case 115: hack = add_keymods("F4", evt); break;
        case 116: hack = add_keymods("F5", evt); break;
        case 117: hack = add_keymods("F6", evt); break;
        case 118: hack = add_keymods("F7", evt); break;
        case 119: hack = add_keymods("F8", evt); break;
        case 120: hack = add_keymods("F9", evt); break;
        case 121: hack = add_keymods("F10", evt); break;
        case 122: hack = add_keymods("F11", evt); break;
        case 123: hack = add_keymods("F12", evt); break;

        // Handle weird behavior for clipboard shortcuts
        // Which don't fire a keypress for some odd reason

        case 65:
            if (cmd_or_ctrl_key(evt)) { // ctrl-a
                // This is handled in the nwjs menu, but we
                // add a way to toggle the window menubar
                // the following command should be uncommented...
                //pdsend(name, "selectall");
                hack = 0; // not sure what to report here...
            }
            break;
        case 88:
            if (cmd_or_ctrl_key(evt)) { // ctrl-x
                // This is handled in the nwjs menubar. If we
                // add a way to toggle the menubar it will be
                // handled with the "cut" DOM listener, so we
                // can probably remove this code...
                //pdsend(name, "cut");
                hack = 0; // not sure what to report here...
            }
            break;
        case 67:
            if (cmd_or_ctrl_key(evt)) { // ctrl-c
                // Handled in nwjs menubar (see above)
                //pdsend(name, "copy");
                hack = 0; // not sure what to report here...
            }
            break;
        case 86:
            if (cmd_or_ctrl_key(evt)) { // ctrl-v
                // We also use "cut" and "copy" DOM event handlers
                // and leave this code in case we need to change
                // tactics for some reason.
                //pdsend(name, "paste");
                hack = 0; // not sure what to report here...
            }
            break;
        case 90:
            if (cmd_or_ctrl_key(evt)) { // ctrl-z undo/redo
                // We have to catch undo and redo here.
                // undo and redo have nw.js menu item shortcuts,
                // and those shortcuts don't behave consistently
                // across platforms:
                // Gnu/Linux: key events for the shortcut do not
                //   propogate down to the DOM
                // OSX: key events for the shortcut _do_ propogate
                //   down to the DOM
                // Windows: not sure...

                // Solution-- let the menu item shortcuts handle
                // undo/redo functionality, and do nothing here...
                //if (evt.shiftKey) {
                //    pdsend(name, "redo");
                //} else {
                //    pdsend(name, "undo");
                //}
            }
            break;

        // Need to handle Control key, Alt

        case 16: hack = "Shift"; break;
        case 17: hack = "Control"; break;
        case 18: hack = "Alt"; break;

        // keycode 55 = 7 key (shifted = '/' on German keyboards)
        case 55:
            if (cmd_or_ctrl_key(evt)) {
                evt.preventDefault();
                pdsend("pd dsp 1");
            }
            break;

    }
    if (hack !== null) {
        canvas_sendkey(cid, 1, evt, hack, evt.repeat);
        set_keymap(key_code, hack);
    }

    //post("keydown time: keycode is " + evt.keyCode);
    last_keydown = evt.keyCode;
    //evt.stopPropagation();
    //evt.preventDefault();
};

exports.keypress = function(cid, evt) {
    // For some reasons <ctrl-e> registers a keypress with
    // charCode of 5. We filter that out here so it doesn't
    // cause trouble when toggling editmode.
    // Also, we're capturing <ctrl-or-cmd-Enter> in the "Edit"
    // menu item "reselect", so we filter it out here as well.
    // (That may change once we find a more flexible way of
    // handling keyboard shortcuts
    if (evt.charCode !== 5 &&
          (!cmd_or_ctrl_key(evt) || evt.charCode !== 10)) {
        canvas_sendkey(cid, 1, evt, evt.charCode,
            evt.repeat);
        set_keymap(last_keydown, evt.charCode,
            evt.repeat);
    }
    //post("keypress time: charcode is " + evt.charCode);
    // Don't do things like scrolling on space, arrow keys, etc.
};

exports.keyup = function(cid, evt) {
    var my_char_code = get_char_code(evt.keyCode);
    // Sometimes we don't have char_code. For example, the
    // nw menu doesn't propogate shortcut events, so we don't get
    // to map a charcode on keydown/keypress. In those cases we'll
    // get null, so we check for that here...
    if (my_char_code) {
        canvas_sendkey(cid, 0, evt, my_char_code, evt.repeat);
    }
    // This can probably be removed
    //if (cmd_or_ctrl_key(evt) &&
    //      (evt.keyCode === 13 || evt.keyCode === 10)) {
    //    pdgui.pdsend(name, "reselect");
    //}
};

    // Hard-coded Pd-l2ork font metrics
/*
var font_fixed_metrics = [
    8, 5, 11,
    9, 6, 12,
    10, 6, 13,
    12, 7, 16,
    14, 8, 17,
    16, 10, 19,
    18, 11, 22,
    24, 14, 29,
    30, 18, 37,
    36, 22, 44 ].join(" ");
*/

// Let's try to get some metrics specific to Node-webkit...
// Hard-coded Pd-l2ork font metrics
var font_fixed_metrics = [
    8, 5, 11,
    9, 6, 12,
    10, 6, 13,
    12, 7, 16,
    14, 8, 17,
    16, 10, 19,
    18, 11, 22,
    24, 14, 29,
    30, 18, 37,
    36, 22, 44 ].join(" ");

// Utility Functions

// This is used to escape spaces and other special delimiters in FUDI
// arguments for dialogs. (The reverse function is sys_decodedialog() in the C
// code.)
function encode_for_dialog(s) {
    s = s.replace(/\+/g, "++");
    s = s.replace(/\s/g, "+_");
    s = s.replace(/\$/g, "+d");
    s = s.replace(/;/g, "+s");
    s = s.replace(/,/g, "+c");
    s = "+" + s;
    return s;
}

exports.encode_for_dialog = encode_for_dialog;

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
// only seems to be used by pddplink_open
function path_is_absolute(myPath) {
    var ret = (path.resolve(myPath) ===
        path.normalize(myPath).replace(/(.+)([\/|\\])$/, "$1"));
    return ret;
}

function set_midiapi(val) {
    pd_whichmidiapi = val;
}

function set_audioapi(val) {
    pd_whichapi = val;
}

var throttle_console_scroll = (function() {
    var scroll_delay;
    return function() {
        if (!scroll_delay) {
            scroll_delay = setTimeout(function() {
                var printout = pd_window.document
                    .getElementById("console_bottom");
                printout.scrollTop = printout.scrollHeight;
                scroll_delay = undefined;
            }, 30);
        }
    }
}());

// Hmm, probably need a closure here...
var current_string = "";
var last_string = "";
var last_child = {};
var last_object_id = "";
var duplicate = 0;

function do_post(object_id, selector, string, type, loglevel) {
    var my_p, my_a, span, text, sel_span, printout, dup_span;
    current_string = current_string + (selector ? selector : "") + string;
    my_p = pd_window.document.getElementById("p1");
    // We can get posts from Pd that are build incrementally, with the final
    // message having a "\n" at the end. So we test for that.
    if (string.slice(-1) === "\n") {
        if (current_string === last_string
            && object_id === last_object_id) {
            duplicate += 1;
            dup_span = last_child.firstElementChild;
            dup_span.textContent = "[" + (duplicate + 1) + "] ";
            current_string = "";
            if (my_p.lastChild !== last_child) {
                my_p.appendChild(last_child);
            }
        } else {
            span = pd_window.document.createElement("span");
            if (type) {
                span.classList.add(type);
            }
            dup_span = pd_window.document.createElement("span");
            sel_span = pd_window.document.createTextNode(
                selector ? selector : "");
            text = pd_window.document.createTextNode(
                (selector && selector !== "") ? ": " + string : current_string);
            if (object_id && object_id.length > 0) {
                my_a = pd_window.document.createElement("a");
                my_a.href = "javascript:pdgui.pd_error_select_by_id('" +
                    object_id + "')";
                my_a.appendChild(sel_span);
                span.appendChild(dup_span); // duplicate tally
                span.appendChild(my_a);
            } else {
                span.appendChild(dup_span);
                span.appendChild(sel_span);
                my_p.appendChild(span);
            }
            span.appendChild(text);
            my_p.appendChild(span);
            last_string = current_string;
            current_string = "";
            last_child = span;
            last_object_id = object_id;
            duplicate = 0;
            // update the scrollbars to the bottom, but throttle it
            // since it is expensive
            throttle_console_scroll();
        }
    }
}

// print message to console-- add a newline for convenience
function post(string, type) {
    do_post(null, null, string + "\n", type, null);
}

exports.post = post;

// print message to console from Pd-- don't add newline
function gui_post(string, type) {
    do_post(null, "", string, type, null);
}

function pd_error_select_by_id(objectid) {
    if (objectid !== null) {
        pdsend("pd findinstance " + objectid);
    }
}

exports.pd_error_select_by_id = pd_error_select_by_id;

function gui_post_error(objectid, loglevel, error_msg) {
    do_post(objectid, "error", error_msg, "error", loglevel);
}

// This is used specifically by [print] so that we can receive the full
// message in a single call. This way we can call do_post with a single
// string message and track the object id with the selector.

function gui_print(object_id, selector, array_of_strings) {
    // Unfortunately the instance finder still uses a "." prefix, so we
    // have to add that here
    do_post("." + object_id, selector, array_of_strings.join(" ") + "\n",
        null, null);
}

function gui_legacy_tcl_command(file, line_number, text) {
    // Print legacy tcl commands on the console. These may still be present in
    // some parts of the code (usually externals) which haven't been converted
    // to the new nw.js gui yet. Usually the presence of such commands
    // indicates a bug that needs to be fixed. This information is most useful
    // for developers, so you may want to comment out the following line if
    // you don't want to see them.
    post("legacy tcl command at " + line_number + " of " + file + ": " + text);
}

function clear_console() {
    var container = pd_window.document.getElementById("p1");
    container.textContent = "";
}

exports.clear_console = clear_console;

// convert canvas dimensions to old tcl/tk geometry
// string format. Unfortunately this is exposed (and
// documented) to the user with the "relocate" message
// in both Pd-Extended and Pd-Vanilla.  So we have to
// keep it here for backwards compatibility.
function pd_geo_string(w, h, x, y) {
    return  [w,"x",h,"+",x,"+",y].join("");
}

// quick hack so that we can paste pd code from clipboard and
// have it affect an empty canvas' geometry
// requires nw.js API
function gui_canvas_change_geometry(cid, w, h, x, y) {
    gui(cid).get_nw_window(function(nw_win) {
        nw_win.width = w;
        nw_win.height = h + 23; // 23 is a kludge to account for menubar
        nw_win.x = x;
        nw_win.y = y;
    });
}

// In tcl/tk, this function had some checks to apparently
// keep from sending a "relocate" message to Pd, but I'm
// not exactly clear on how it works. If this ends up being
// a cpu hog, check out pdtk_canvas_checkgeometry in the old
// pd.tk
function canvas_check_geometry(cid) {
    var win_w = patchwin[cid].width,
        // "23" is a kludge to account for the menubar size.  See comment
        // in nw_create_window of index.js
        win_h = patchwin[cid].height - 23,
        win_x = patchwin[cid].x,
        win_y = patchwin[cid].y,
        cnv_width = patchwin[cid].window.innerWidth,
        cnv_height = patchwin[cid].window.innerHeight - 23;
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

// This is an enormous workaround based off of the comment for this bug:
//   https://github.com/nwjs/nw.js/issues/3372
// Essentially nwworkingdir won't work if you set it through a javascript
// object like a normal human being. Instead, you have to let it get parsed
// by the browser, which means adding a <span> tag as a parent just so we
// can set its innerHTML.
// If this bug is ever resolved then this function can go away, as you should
// be able to just set nwsaveas and nwworkingdir through the setAttribute
// DOM interface.
function build_file_dialog_string(obj) {
    var prop, input = "<input ";
    for (prop in obj) {
        if (obj.hasOwnProperty(prop)) {
            input += prop;
            if (obj[prop]) {
                input += '="' + obj[prop] + '"';
            }
            input += ' ';
        }
    }
    input += "/>";
    return input;
}

exports.build_file_dialog_string = build_file_dialog_string;

function gui_canvas_saveas(name, initfile, initdir, close_flag) {
    var input, chooser,
        span = patchwin[name].window.document.querySelector("#saveDialogSpan");
    if (!fs.existsSync(initdir)) {
        initdir = pwd;
    }
    // If we don't have a ".pd" file extension (e.g., "Untitled-1", add one)
    if (initfile.slice(-3) !== ".pd") {
        initfile += ".pd";
    }
    // This is complicated because of a bug... see build_file_dialog_string

    // NOTE ag: The original code had nwworkingdir set to path.join(initdir,
    // initfile) which doesn't seem right and in fact does *not* work with the
    // latest nw.js on Linux at all (dialog comes up without a path under
    // which to save, "Save" doesn't work until you explicitly select one).

    // Setting nwsaveas to initfile and nwworkingdir to initdir (as you'd
    // expect) works for me on Linux, but it seems that specifying an absolute
    // pathname for nwsaveas is necessary on Windows, and this also works on
    // Linux. Cf. https://github.com/nwjs/nw.js/issues/3372 (which is still
    // open at the time of this writing). -ag
    input = build_file_dialog_string({
        style: "display: none;",
        type: "file",
        id: "saveDialog",
        // using an absolute path here, see comment above
        nwsaveas: path.join(initdir, initfile),
        nwworkingdir: initdir,
        accept: ".pd"
    });
    span.innerHTML = input;
    chooser = patchwin[name].window.document.querySelector("#saveDialog");
    chooser.onchange = function() {
        saveas_callback(name, this.value, close_flag);
        // reset value so that we can open the same file twice
        this.value = null;
        console.log("tried to save something");
    }
    chooser.click();
}

function saveas_callback(cid, file, close_flag) {
    var filename = defunkify_windows_path(file),
        directory = path.dirname(filename),
        basename = path.basename(filename);
    // It probably isn't possible to arrive at the callback with an
    // empty string.  But I've only tested on Debian so far...
    if (filename === null) {
        return;
    }
    pdsend(cid, "savetofile", enquote(basename), enquote(directory),
        close_flag);
    // XXXREVIEW: It seems sensible that we also switch the opendir here. -ag
    set_pd_opendir(directory);
    // update the recent files list
    var norm_path = path.normalize(directory);
    pdsend("pd add-recent-file", enquote(defunkify_windows_path(path.join(norm_path, basename))));
}

exports.saveas_callback = saveas_callback;

function menu_saveas(name) {
    pdsend(name + " menusaveas");
}

exports.menu_saveas = menu_saveas;

function gui_canvas_print(name, initfile, initdir) {
    // AG: This works mostly like gui_canvas_saveas above, except that we
    // create a pdf file and use a different input element and callback.
    var input, chooser,
        span = patchwin[name].window.document.querySelector("#printDialogSpan");
    if (!fs.existsSync(initdir)) {
        initdir = pwd;
    }
    // If we don't have a ".pd" file extension (e.g., "Untitled-1", add one)
    if (initfile.slice(-3) !== ".pd") {
        initfile += ".pd";
    }
    // Adding an "f" now gives .pdf which is what we want.
    initfile += "f";
    input = build_file_dialog_string({
        style: "display: none;",
        type: "file",
        id: "printDialog",
        nwsaveas: path.join(initdir, initfile),
        nwworkingdir: initdir,
        accept: ".pdf"
    });
    span.innerHTML = input;
    chooser = patchwin[name].window.document.querySelector("#printDialog");
    chooser.onchange = function() {
        print_callback(name, this.value);
        // reset value so that we can open the same file twice
        this.value = null;
        console.log("tried to print something");
    }
    chooser.click();
}

function print_callback(cid, file) {
    var filename = defunkify_windows_path(file);
    // It probably isn't possible to arrive at the callback with an
    // empty string.  But I've only tested on Debian so far...
    if (filename === null) {
        return;
    }
    // Let nw.js do the rest (requires nw.js 0.14.6+)
    patchwin[cid].print({ pdf_path: filename, headerFooterEnabled: false });
    post("printed to: " + filename);
}

exports.print_callback = print_callback;

function menu_print(name) {
    pdsend(name + " menuprint");
}

exports.menu_print = menu_print;

function menu_new () {
    // try not to use a global here
    untitled_directory = get_pd_opendir();
    pdsend("pd filename",
           "Untitled-" + untitled_number,
           enquote(defunkify_windows_path(untitled_directory)));
    // I don't think k12_mode works yet. Need to test this.
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

// requires nw.js API
function gui_window_close(cid) {
    // for some edge cases like [vis 1, vis 0(--[send subpatch] we
    // may not have finished creating the window yet. So we check to
    // make sure the canvas cid exists...
    gui(cid).get_nw_window(function(nw_win) {
        nw_close_window(nw_win);
    });
    // remove reference to the window from patchwin object
    set_patchwin(cid, null);
    loading[cid] = null;
}

function menu_k12_open_demos () {

}

exports.menu_k12_open_demos = menu_k12_open_demos;


function menu_open (filenames_string) {
    var file_array = filenames_string.split(";"),
        length = file_array.length,
        i;
    for (i = 0; i < length; i++) {
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
    // filename/args/dir which is ugly. Also, this should use the
    // html5 dialog-- or some CSS equivalent-- instead of the
    // confusing OK/Cancel javascript prompt.
    var nw = patchwin[cid_for_dialog],
        w = nw.window,
        doc = w.document,
        dialog = doc.getElementById("save_before_quit"),
        // hm... we messed up somewhere. It'd be better to set the document
        // title so that we don't have to mess with nw.js-specific properties.
        // Also, it's pretty shoddy to have to split on " * ", and to include
        // the creation arguments in this dialog. We'd be better off storing
        // the actual path and filename somewhere, then just fetching it here.
        title = nw.title.split(" * "),
        dialog_file_slot = doc.getElementById("save_before_quit_filename"),
        yes_button = doc.getElementById("yes_button"),
        no_button = doc.getElementById("no_button"),
        cancel_button = doc.getElementById("cancel_button"),
        filename = title[0],
        dir = title[1];
    if (dir.charAt(0) === "(") {
        dir = dir.slice(dir.indexOf(")")+4); // slice off ") - "
    } else {
        dir = dir.slice(2); // slice off "- "
    }
    dialog_file_slot.textContent = filename;
    dialog_file_slot.title = dir;
    yes_button.onclick = function() {
        w.canvas_events.save_and_close();
    };
    no_button.onclick = function() {
        w.canvas_events.close_without_saving(cid, force);
    };
    cancel_button.onclick = function() {
        w.close_save_dialog();
        w.canvas_events[w.canvas_events.get_previous_state()]();
    }

    // Boy does this seem wrong-- restore() brings the window to the front of
    // the stacking order. But that is really the job of focus(). This works
    // under Ubuntu-- need to test it on OSX...
    nw.restore();
    // Turn off events so that the user doesn't change the canvas state--
    // we actually need to disable the menubar items, too, but we haven't
    // done that yet.
    w.canvas_events.none();
    // go back to original zoom level so that dialog will show up
    nw.zoomLevel = 0;
    // big workaround-- apparently the dialog placement algo and the nw.js
    // zoomLevel state change don't happen deterministically. So we set a
    // timeout to force the dialog to render after the zoomLevel change.

    // Probably the best solution is to give up on using the nw.js zoomLevel
    // method altogether and do canvas zooming completely in the DOM. This will
    // add some math to the canvas_events, so it's probably a good idea to
    // wait until we move most of the GUI functionality out of the C code (or
    // at least until we quit sending incessant "motion" messages to the core).
    w.setTimeout(function() {
        dialog.showModal();
    }, 150);
}

function gui_canvas_menuclose(cid_for_dialog, cid, force) {
    // Hack to get around a renderer bug-- not guaranteed to work
    // for long patches
    setTimeout(function() {
            canvas_menuclose_callback(cid_for_dialog, cid, force);
        }, 450);
}

function gui_quit_dialog() {
    gui_raise_pd_window();
    var reply = pd_window.window.confirm("Really quit?");
    if (reply === true) {
        pdsend("pd quit");
    }
}

// send a message to Pd
function menu_send(name) {
    var message,
        win = name ? patchwin[name] : pd_window;
    message = win.window.prompt("Type a message to send to Pd", name);
    if (message != undefined && message.length) {
        post("Sending message to Pd: " + message + ";");
        pdsend(message);
    }
}

// requires nw.js API (Menuitem)
function canvas_set_editmode(cid, state) {
    gui(cid).get_elem("patchsvg", function(patchsvg, w) {
        w.set_editmode_checkbox(state !== 0 ? true : false);
        if (state !== 0) {
            patchsvg.classList.add("editmode");
        } else {
            patchsvg.classList.remove("editmode");
        }
    });
}

exports.canvas_set_editmode = canvas_set_editmode;

function gui_canvas_set_editmode(cid, state) {
    canvas_set_editmode(cid, state);
}

// requires nw.js API (Menuitem)
function gui_canvas_set_cordinspector(cid, state) {
    patchwin[cid].window.set_cord_inspector_checkbox(state !== 0 ? true : false);
}

function canvas_set_scrollbars(cid, scroll) {
    patchwin[cid].window.document.body.style.
        overflow = scroll ? "visible" : "hidden";
}

exports.canvas_set_scrollbars = canvas_set_scrollbars;

function gui_canvas_set_scrollbars(cid, no_scrollbars) {
    canvas_set_scrollbars(cid, no_scrollbars === 0);
}

exports.menu_send = menu_send;

function gui_set_toplevel_window_list(dummy, attr_array) {
    // We receive an array in the form:
    // ["Name", "address", etc.]
    // where "address" is the cid (x123456etc.)
    // We don't do anything with it at the moment,
    // but they could be added to the "Windows" menu
    // if desired. (Pd Vanilla doesn't do this, but
    // Pd-l2ork (and possibly Pd-extended) did.

    // the "dummy" parameter is just to work around a bug in the gui_vmess API
}

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

function import_file(directory, basename)
{
    if (basename.match(/\.(pat|mxb|help)$/) !=null) {
        post("warning: opening pat|mxb|help not implemented yet");
        if (pd_nt == 0) {
            // on GNU/Linux, cyclist is installed into /usr/bin usually
            cyclist = "/usr/bin/cyclist";
        } else {
            cyclist = pd_guidir + "/bin/cyclist"
        }
        //The following is from tcl and still needs to be ported...

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
}

function process_file(file, do_open) {
    var filename = defunkify_windows_path(file),
        directory = path.dirname(filename),
        basename = path.basename(filename),
        cyclist;
    if (do_open) import_file(directory, basename);
    if (basename.match(/\.(pd|pat|mxt)$/i) != null) {
        if (do_open) {
            pdsend("pd open", enquote(basename),
                   (enquote(directory)));
        }
        set_pd_opendir(directory);
        //::pd_guiprefs::update_recentfiles "$filename" 1
        // update the recent files list
        var norm_path = path.normalize(directory);
        pdsend("pd add-recent-file", enquote(defunkify_windows_path(path.join(norm_path, basename))));
    }
}

function open_file(file) {
    process_file(file, 1);
}

function gui_process_open_arg(file) {
    // AG: This is invoked when the engine opens a patch file via the command
    // line (-open). In this case the file is already loaded, so we just
    // update the opendir and the recent files list.
    process_file(file, 0);
}

function open_html(target) {
    nw_open_html(target);
}

function open_textfile(target) {
    nw_open_textfile(target);
}

// Think about renaming this and pd_doc_open...

// Open a file-- html, text, or Pd.
function doc_open (dir, basename) {
    // normalize to get rid of extra slashes, ".." and "."
    var norm_path = path.normalize(dir);
    if (basename.slice(-4) === ".txt"
        || basename.slice(-2) === ".c") {
        open_textfile(path.join(norm_path, basename));
    } else if (basename.slice(-5) === ".html"
               || basename.slice(-4) === ".htm"
               || basename.slice(-4) === ".pdf") {
        open_html(path.join(norm_path, basename));

    } else {
        pdsend("pd open", enquote(defunkify_windows_path(basename)),
            enquote(defunkify_windows_path(norm_path)));
    }
}

// Need to rethink these names-- it's confusing to have this and
// pd_doc_open available, but we need this one for dialog_search because
// it uses absolute paths
exports.doc_open = doc_open;

// Open a file relative to the main directory where "doc/" and "extra/" live
function pd_doc_open(dir, basename) {
    doc_open(path.join(lib_dir, dir), basename);
}

exports.pd_doc_open = pd_doc_open;

function external_doc_open(url) {
    nw_open_external_doc(url);
}

exports.external_doc_open = external_doc_open;

function gui_set_cwd(dummy, cwd) {
    if (cwd !== ".") {
        pwd = cwd;
        post("working directory is " + cwd);
    }
}

// This doesn't work at the moment.  Not sure how to feed the command line
// filelist to a single instance of node-webkit.
function gui_open_via_unique (secondary_pd_engine_id, unique, file_array) {
    var startup_dir = pwd,
        i,
        file;
    if (unique == 0 && secondary_pd_engine_id !== pd_engine_id) {
        for (i = 0; i < file_array.length; i++) {
            file = file_array[i];
            if (!path.isAbsolute(file)) {
                file = path.join(pwd, file);
            }
            open_file(file);
        }
        quit_secondary_pd_instance(secondary_pd_engine_id);
    }
}

function gui_startup(version, fontname_from_pd, fontweight_from_pd,
    apilist, midiapilist) {
    console.log("Starting up...");
    console.log("gui_startup from GUI...");
    // # tb: user defined typefaces
    // set some global variables
    pd_myversion = version;
    pd_apilist =  apilist;
    pd_midiapilist = midiapilist;

    fontname = fontname_from_pd;
    fontweight = fontweight_from_pd;
    pd_fontlist = "";
    untitled_number = 1; // global variable to increment for each new patch

    // From tcl, not sure if all of it is still needed...

    // # on Mac OS X, lower the Pd window to the background so patches open on top
    // if {$pd_nt == 2} { lower . }
    // # on Windows, raise the Pd window so that it has focused when launched
    // if {$pd_nt == 1} { raise . }

    // set fontlist ""
    // if {[info tclversion] >= 8.5} {find_default_font}
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
    pdsend("pd init", enquote(defunkify_windows_path(pwd)), "0",
        font_fixed_metrics);

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
    last_loaded, // last loaded canvas
    last_focused, // last focused canvas (doesn't include Pd window or dialogs)
    loading = {},
    title_queue= {}, // ugly kluge to work around an ugly race condition
    popup_menu = {};

    var patchwin = {}; // object filled with cid: [Window object] pairs
    var dialogwin = {}; // object filled with did: [Window object] pairs

exports.get_patchwin = function(name) {
    return patchwin[name];
}

var set_patchwin = function(cid, win) {
    patchwin[cid] = win;
    if (win) {
        gui.add(cid, win);
    } else {
        gui.remove(cid, win);
    }
}

exports.set_patchwin = set_patchwin;

exports.get_dialogwin = function(name) {
    return dialogwin[name];
}

exports.set_dialogwin = function(did, win) {
    dialogwin[did] = win;
}

exports.remove_dialogwin = function(name) {
    dialogwin[name] = null;
}

// stopgap...
pd_colors["canvas_color"] = "white";

exports.last_loaded = function () {
    return last_loaded;
}

// close a canvas window

function gui_canvas_cursor(cid, pd_event_type) {
    gui(cid).get_elem("patchsvg", function(patch) {
        // A quick mapping of events to pointers-- these can
        // be revised later
        var c;
        switch(pd_event_type) {
            case "cursor_runmode_nothing":
                c = "default";
                break;
            case "cursor_runmode_clickme":
                // The "pointer" icon seems the natural choice for "clickme"
                // here, but unfortunately it creates ambiguity with the
                // default editmode pointer icon. Not sure what the best
                // solution is, but for now so we use "default" for clickme.
                // That creates another ambiguity, but it's less of an issue
                // since most of the clickable runtime items are fairly obvious
                // anyway.

                //c = "pointer";

                c = "default";
                break;
            case "cursor_runmode_thicken":
                c = "inherit";
                break;
            case "cursor_runmode_addpoint":
                c = "cell";
                break;
            case "cursor_editmode_nothing":
                c = "pointer";
                break;
            case "cursor_editmode_connect":
                c = "-webkit-grabbing";
                break;
            case "cursor_editmode_disconnect":
                c = "no-drop";
                break;
            case "cursor_editmode_resize":
                c = "ew-resize";
                break;
            case "cursor_editmode_resize_bottom_right": c = "se-resize";
                break;
            case "cursor_scroll":
                c = "all-scroll";
                break;
        }
        patch.style.cursor = c;
    });
}

// Note: cid can either be a real canvas id, or the string "pd" for the
// console window
function canvas_sendkey(cid, state, evt, char_code, repeat) {
    var shift = evt.shiftKey ? 1 : 0,
        repeat_number = repeat ? 1 : 0;
    pdsend(cid, "key", state, char_code, shift, 1, repeat_number);
}

exports.canvas_sendkey = canvas_sendkey;

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
    if (patchwin[cid]) {
        patchwin[cid].title = title;
    } else {
        title_queue[cid] = title;
    }
}

function query_title_queue(cid) {
    return title_queue[cid];
}

exports.query_title_queue = query_title_queue;

function free_title_queue(cid) {
    title_queue[cid] = null;
}

exports.free_title_queue = free_title_queue;

function window_is_loading(cid) {
    return loading[cid];
}

exports.window_is_loading = window_is_loading;

function set_window_finished_loading(cid) {
    loading[cid] = null;
}

exports.set_window_finished_loading = set_window_finished_loading;

// wrapper for nw_create_window
function create_window(cid, type, width, height, xpos, ypos, attr_array) {
    nw_create_window(cid, type, width, height, xpos, ypos, attr_array);
    // initialize variable to reflect that this window has been opened
    loading[cid] = true;
    // we call set_patchwin from the callback in pd_canvas
}

// create a new canvas
function gui_canvas_new(cid, width, height, geometry, zoom, editmode, name, dir, dirty_flag, hide_scroll, hide_menu, cargs) {
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
    my_canvas_color = pd_colors["canvas_color"];
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
    var xpos = Math.min(Number(geometry[0]), window.screen.width - width);
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
    create_window(cid, "pd_canvas", width, height,
        xpos, ypos, {
            menu_flag: menu_flag,
            resize: resize[cid],
            topmost: topmost[cid],
            color: my_canvas_color,
            name: name,
            dir: dir,
            dirty: dirty_flag,
            args: cargs,
            zoom: zoom,
            editmode: editmode,
            hide_scroll: hide_scroll
    });
}

/* This gets sent to Pd to trigger each object on the canvas
   to do its "vis" function. The result will be a flood of messages
   back from Pd to the GUI to draw these objects */
function canvas_map(name) {
    console.log("canvas mapping " + name + "...");
    pdsend(name + " map 1");
}

function gui_canvas_erase_all_gobjs(cid) {
    gui(cid).get_elem("patchsvg", function(svg_elem) {
        var elem;
        while (elem = svg_elem.firstChild) {
            svg_elem.removeChild(elem);
        }
    });
}

exports.canvas_map = canvas_map;

// Start Pd

// If the GUI is started first (as in a Mac OSX Bundle) we use this
// function to actually start the core
function spawn_pd(gui_path, port, file_to_open) {
    post("gui_path is " + gui_path);
    var pd_binary,
        platform = process.platform,
        flags = ["-guiport", port];
    if (platform === "darwin") {
        // OSX -- this is currently tailored to work with an app bundle. It
        // hasn't been tested with a system install of pd-l2ork
        pd_binary = path.join(gui_path, "bin", "pd-l2ork");
        if (file_to_open) {
            flags.push("-open", file_to_open);
        }
    } else {
        pd_binary = path.join(gui_path, "..", "bin", "pd-l2ork");
        flags.push("-nrt"); // for some reason realtime causes watchdog to die
    }
    post("binary is " + pd_binary);
    // AG: It isn't nice that we change the cwd halfway through the startup
    // here, but since the GUI launches the engine if we come here (that's how
    // it works on the Mac), we *really* want to launch the engine in the
    // user's home directory and not in some random subdir of the OSX app
    // bundle. Note that to make that work, the pd-l2ork executable needs to
    // be invoked using an absolute path (see above).
    process.chdir(process.env.HOME);
    var child = cp.spawn(pd_binary, flags, {
        stdio: "inherit",
        detached: true
    });
    child.on("error", function(err) {
        pd_window.alert("Couldn't successfully start Pd due to an error:\n\n" +
          err + "\n\nClick Ok to close Pd.");
        process.exit(1);
    });
    child.unref();
    post("Pd started.");
}

// net stuff
var net = require("net");

var HOST = "127.0.0.1";
var PORT;
var connection; // the GUI's socket connection to Pd

exports.set_port = function (port_no) {
    PORT = port_no;
}

var secondary_pd_engines = {};

// This is an alarmingly complicated and brittle approach to opening
// files from a secondary instance of Pd in a currently running instance.
// It works something like this:
// 1. User is running an instance of Purr Data.
// 2. User runs another instance of Purr Data from the command line, specifying
//    files to be opened as command line args. Or, they click on a file which
//    in the desktop or file manager which triggers the same behavior.
// 2. A new Pd process starts-- let's call it a "secondary pd engine".
// 3. The secondary pd engine tries to run a new GUI.
// 4. The secondary GUI forwards an "open" message to the currently running GUI.
// 5. The secondary GUI exits (before spawning any windows).
// 6. The original GUI receives the "open" message, finds the port number
//    for the secondary Pd engine, and opens a socket to it.
// 7. The original GUI receives a message to set the working directory to
//    whatever the secondary Pd engine thinks it should be.
// 8. The original GUI sends a message to the secondary Pd instance, telling
//    it to send a list of files to be opened.
// 9. The original GUI receives a message from the secondary Pd instance
//    with the list of files.
// 10.For each file to be opened, the original GUI sends a message to the
//    _original_ Pd engine to open the file.
// 11.Once these messages have been sent, the original GUI sends a message
//    to the secondary Pd engine to quit.
// 12.The original Pd engine opens the files, and the secondary Pd instance
//    quits.
function connect_as_client_to_secondary_instance(host, port, pd_engine_id) {
    var client = new net.Socket(),
        command_buffer = {
            next_command: ""
    };
    client.setNoDelay(true);
    client.connect(+port, host, function() {
        console.log("CONNECTED TO: " + host + ":" + port);
        secondary_pd_engines[pd_engine_id] = {
            socket: client
        }
        client.write("pd forward_files_from_secondary_instance;");
    });
    client.on("data", function(data) {
        // Terrible thing:
        // We're parsing the data as it comes in-- possibly
        // from multiple ancillary instances of the Pd engine.
        // So to retain some semblance of sanity, we only let the
        // parser evaluate commands that we list in the array below--
        // anything else will be discarded.  This is of course bad
        // because it means simple changes to the code, e.g., changing
        // the name of the function "gui_set_cwd" would cause a bug
        // if you forget to come here and also change that name in the
        // array below.
        // Another terrible thing-- gui_set_cwd sets a single, global
        // var for the working directory. So if the user does something
        // weird like write a script to open files from random directories,
        // there would be a race and it might not work.
        // Yet another terrible thing-- now we're setting the current
        // working directory both in the GUI, and from the secondary instances
        // with "gui_set_cwd" below.
        perfect_parser(data, command_buffer, [
            "gui_set_cwd",
            "gui_open_via_unique"
        ]);
    });
    client.on("close", function () {
        // I guess somebody could script opening patches in an
        // installation, so let's go ahead and delete the key here
        // (The alternative is just setting it to undefined)
        delete secondary_pd_engines[pd_engine_id];
    });
}

function quit_secondary_pd_instance (pd_engine_id) {
    secondary_pd_engines[pd_engine_id].socket.write("pd quit;");
}

// This is called when the running GUI receives an "open" event.
exports.connect_as_client_to_secondary_instance =
    connect_as_client_to_secondary_instance;

function connect_as_client() {
    var client = new net.Socket();
    client.setNoDelay(true);
    // uncomment the next line to use fast_parser (then set its callback below)
    //client.setEncoding("utf8");
    client.connect(PORT, HOST, function() {
        console.log("CONNECTED TO: " + HOST + ":" + PORT);
    });
    connection = client;
    init_socket_events();
}

exports.connect_as_client = connect_as_client;

function connect_as_server(gui_path, file_path) {
    var server = net.createServer(function(c) {
            post("incoming connection to GUI");
            connection = c;
            init_socket_events();
        }),
        port = PORT,
        ntries = 0,
        listener_callback = function() {
            post("GUI listening on port " + port + " on host " + HOST);
            spawn_pd(gui_path, port, file_path);
        };
    server.listen(port, HOST, listener_callback);
    // try to reconnect if necessary
    server.on("error", function (e) {
        if (e.code === "EADDRINUSE" && ntries++ < 20) {
            post("Address in use, retrying...");
            port++;
            setTimeout(function () {
                server.close();
                server.listen(port, HOST); // (already have the callback above)
            }, 30); // Not sure we really need a delay here
        } else {
            pd_window.alert("Error: couldn't bind to a port. Either port nos " +
                  PORT + " through " + port + " are taken or you don't have " +
                  "networking turned on. (See Pd's html doc for details.)");
            server.close();
            process.exit(1);
        }
    });
}

exports.connect_as_server = connect_as_server;

// Add a 'data' event handler for the client socket
// data parameter is what the server sent to this socket

// We're not receiving FUDI (i.e., Pd) messages. Right now we're just using
// the unit separator (ASCII 31) to signal the end of a message. This is
// easier than checking for unescaped semicolons, since it only requires a
// check for a single byte. Of course this makes it more brittle, so it can
// be changed later if needed.

function perfect_parser(data, cbuf, sel_array) {
        var i, len, selector, args;
        len = data.length;
        for (i = 0; i < len; i++) {
            // check for end of command:
            if (data[i] === 31) { // unit separator
                // decode next_command
                try {
                    // This should work for all utf-8 content
                    cbuf.next_command =
                        decodeURIComponent(cbuf.next_command);
                }
                catch(err) {
                    // This should work for ISO-8859-1
                    cbuf.next_command = unescape(cbuf.next_command);
                }
                // Turn newlines into backslash + "n" so
                // eval will do the right thing with them
                cbuf.next_command = cbuf.next_command.replace(/\n/g, "\\n");
                cbuf.next_command = cbuf.next_command.replace(/\r/g, "\\r");
                selector = cbuf.next_command.slice(0, cbuf.next_command.indexOf(" "));
                args = cbuf.next_command.slice(selector.length + 1);
                cbuf.next_command = "";
                // Now evaluate it
                //post("Evaling: " + selector + "(" + args + ");");
                // For communicating with a secondary instance, we filter
                // incoming messages. A better approach would be to make
                // sure that the Pd engine only sends the gui_set_cwd message
                // before "gui_startup".  Then we could just check the
                // Pd engine id in "gui_startup" and branch there, instead of
                // fudging with the parser here.
                if (!sel_array || sel_array.indexOf(selector) !== -1) {
                    eval(selector + "(" + args + ");");
                }
            } else {
                cbuf.next_command += "%" +
                    ("0" // leading zero (for rare case of single digit)
                     + data[i].toString(16)) // to hex
                       .slice(-2); // remove extra leading zero
            }
        }
    };

function init_socket_events () {
    // A not-quite-FUDI command: selector arg1,arg2,etc. These are
    // formatted on the C side to be easy to parse here in javascript
    var command_buffer = {
        next_command: ""
    };
    connection.on("data", function(data) {
        perfect_parser(data, command_buffer);
    });
    connection.on("error", function(e) {
        console.log("Socket error: " + e.code);
        nw_app_quit();
    });

    // Add a "close" event handler for the socket
    connection.on("close", function() {
        //console.log("Connection closed");
        //connection.destroy();
        nw_app_quit(); // set a timeout here if you need to debug
    });
}

exports.init_socket_events = init_socket_events;

// Send commands to Pd
function pdsend() {
    // Using arguments in this way disables V8 optimization for
    // some reason.  But it doesn't look like it makes that much
    // of a difference
    var string = Array.prototype.join.call(arguments, " ");
    connection.write(string + ";");
    // reprint the outgoing string to the pdwindow
    //post(string + ";", "red");
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
    return patchwin[cid].window.document.getElementById(object + "gobj");
}

// Convenience function to get a drawn item of gobj
function get_item(cid, item_id) {
    return patchwin[cid].window.document.getElementById(item_id);
}

// Similar to [canvas create] in tk
function create_item(cid, type, args) {
    var item = patchwin[cid].window.document
        .createElementNS("http://www.w3.org/2000/svg", type);
    if (args !== null) {
        configure_item(item, args);
    }
    return item;
}

// Similar to [canvas itemconfigure], without the need for a reference
// to the canvas
function configure_item(item, attributes) {
    // draw_vis from g_template sends attributes
    // as a ["attr1",val1, "attr2", val2, etc.] array,
    // so we check for that here
    var value, i, attr;
    if (Array.isArray(attributes)) {
        // we should check to make sure length is even here...
        for (i = 0; i < attributes.length; i+=2) {
            value = attributes[i+1];
            item.setAttributeNS(null, attributes[i],
                Array.isArray(value) ? value.join(" "): value);
        }
    } else {
        for (attr in attributes) {
            if (attributes.hasOwnProperty(attr)) {
                item.setAttributeNS(null, attr, attributes[attr]);
            }
        }
    }
}

// The GUI side probably shouldn't know about "items" on SVG.
function gui_configure_item(cid, tag, attributes) {
    gui(cid).get_elem(tag, attributes);
}

function add_gobj_to_svg(svg, gobj) {
    svg.insertBefore(gobj, svg.querySelector(".cord"));
}

// New interface to incrementally move away from the tcl-like functions
// we're currently using. Basically like a trimmed down jquery, except
// we're dealing with multiple toplevel windows so we need an window id
// to get the correct Pd canvas context. Also, some of Pd's t_text tags
// still have "." in them which unfortunately means we must wrap
// getElementById instead of the more expressive querySelector[All] where
// the "." is interpreted as a CSS class selector.

// Methods:
// get_gobj(id, callbackOrObject) returns a reference to this little canvas
//                                interface
// get_elem(id, callbackOrObject) returns a reference to this little canvas
//                                interface
// get_nw_window(callback)        returns a reference to the nw.js Window
//                                wrapper. We keep this separate from the
//                                others in order to annotate those parts
//                                of the code which rely on the nw API (and
//                                abstract them out later if we need to)

// objects are used to set SVG attributes (in the SVG namespace)
// function callbacks have the following args: (DOMElement, window)

// Note about checking for existence:
// Why? Because an iemgui inside a gop canvas will send drawing updates,
// __even__ __if__ that iemgui is outside the bounds of the gop and thus
// not displayed. This would be best fixed in the C code, but I'm not
// exactly sure where or how yet.
// Same problem on Pd Vanilla, except that tk canvas commands on
// non-existent tags don't throw an error.

var gui = (function() {
    var c = {}; // object to hold references to all our canvas closures
    var last_thing; // last thing we got
    var null_fn, null_canvas;
    var create_canvas = function(cid, w) {
        var get = function(parent, sel, arg, suffix) {
            sel = sel + (suffix ? "gobj" : "");
            var elem = parent ?
                parent.querySelector(sel) :
                w.window.document.getElementById(sel);
            last_thing = parent ? last_thing : elem;
            if (elem) {
                if (arg && typeof arg === "object") {
                    configure_item(elem, arg);
                } else if (typeof arg === "function") {
                    arg(elem, w.window, c[cid]);
                }
            }
            return c[cid];
        }
        return {
            append: !w ? null_fn: function(cb) {
                var frag = w.window.document.createDocumentFragment();
                frag = cb(frag, w.window);
                last_thing.appendChild(frag);
                return c[cid];
            },
            get_gobj: !w ? null_fn : function(sel, arg) {
                return get(null, sel, arg, "gobj");
            },
            get_elem: !w ? null_fn : function(sel, arg) {
                return get(null, sel, arg);
            },
            get_nw_window: !w ? null_fn : function(cb) {
                cb(w);
                return c[cid];
            },
            q: !w ? null_fn : function(sel, arg) {
                return last_thing ? get(last_thing, sel, arg) : c[cid];
            },
            debug: function() { return last_thing; }
        }
    };
    // The tcl/tk interface ignores calls to configure non-existent items on
    // canvases. Additionally, its interface was synchronous so that the window
    // would always be guaranteed to exist. So we create a null canvas to keep
    // from erroring out when things don't exist.
    null_fn = function() {
        return null_canvas;
    }
    null_canvas = create_canvas(null);
    var canvas_container = function(cid) {
        last_thing = c[cid] ? c[cid] : null_canvas;
        return last_thing;
    }
    canvas_container.add = function(cid, nw_win) {
        c[cid] = create_canvas(cid, nw_win);
    }
    canvas_container.remove = function(cid, nw_win) {
        c[cid] = null;
    }
    return canvas_container;
}());

// For debugging
exports.gui = gui;

// Most of the following functions map either to pd.tk procs, or in some cases
// tk canvas subcommands

// The "gobj" is a container for all the shapes/paths used to display
// a graphical object on the canvas. This comes in handy-- for example, we
// can displace an object just by translating its "gobj".

// Object, message, and xlet boxes should be crisp (i.e., no anti-aliasing),
// and the "shape-rendering" attribute of "crispEdges" acheives this. However,
// that will also create asymmetric line-widths when scaling-- for example,
// the left edge of a rect may be 3 pixels while the right edge is 4. I'm not
// sure whether this is a bug or just the quirky behavior of value "crispEdges".
// As a workaround, we explicitly add "0.5" to the gobj's translation
// coordinates below. This aligns the shapes-- lines, polygons, and rects
// with a 1px stroke-- to the pixel grid, making them crisp.

// Also-- note that we have a separate function for creating a scalar.
// This is because the user may be drawing lines or paths as
// part of a scalar, in which case we want to leave it up to them to align
// their drawing to the pixel grid. For example, imagine a user pasting a
// path command from the web. If that path already employs the "0.5" offset
// to align to the pixel-grid, a gobj offset would cancel it out. That
// would mean the Pd user always has to do the _opposite_ of what they read
// in SVG tutorials in order to get crisp lines, which is bad.
// In the future, it might make sense to combine the scalar and object
// creation, in which case a flag to toggle the offset would be appropriate.

function gui_gobj_new(cid, tag, type, xpos, ypos, is_toplevel) {
    var g;
    xpos += 0.5,
    ypos += 0.5,
    gui(cid).get_elem("patchsvg", function(svg_elem) {
        var transform_string = "matrix(1,0,0,1," + xpos + "," + ypos + ")";
        g = create_item(cid, "g", {
            id: tag + "gobj",
            transform: transform_string,
            class: type + (is_toplevel !== 0 ? "" : " gop")
        });
        add_gobj_to_svg(svg_elem, g);
    });
    return g;
}

function gui_text_draw_border(cid, tag, bgcolor, isbroken, width, height) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        // isbroken means either
        //     a) the object couldn't create or
        //     b) the box is empty
        var rect = create_item(cid, "rect", {
            width: width,
            height: height,
            //"shape-rendering": "crispEdges",
            class: "border"
        });
        if (isbroken === 1) {
            rect.classList.add("broken_border");
        }
        frag.appendChild(rect);
        return frag;
    });
}

function gui_gobj_draw_io(cid, parenttag, tag, x1, y1, x2, y2, basex, basey,
    type, i, is_signal, is_iemgui) {
    gui(cid).get_gobj(parenttag)
    .append(function(frag) {
        var xlet_class, xlet_id, rect;
        if (is_iemgui) {
            xlet_class = "xlet_iemgui";
            // We have an inconsistency here.  We're setting the tag using
            // string concatenation below, but the "tag" for iemguis arrives
            // to us pre-concatenated.  We need to remove that formatting in c,
            // and in general try to simplify tag creation on the c side as
            // much as possible.
            xlet_id = tag;
        } else if (is_signal) {
            xlet_class = "xlet_signal";
            xlet_id = tag + type + i;
        } else {
            xlet_class = "xlet_control";
            xlet_id = tag + type + i;
        }
        rect = create_item(cid, "rect", {
            width: x2 - x1,
            height: y2 - y1,
            x: x1 - basex,
            y: y1 - basey,
            id: xlet_id,
            class: xlet_class,
            //"shape-rendering": "crispEdges"
        });
        frag.appendChild(rect);
        return frag;
    });
}

function gui_gobj_redraw_io(cid, parenttag, tag, x, y, type, i, basex, basey) {
    // We have to check for null. Here's why...
    // if you create a gatom:
    //   canvas_atom -> glist_add -> text_vis -> glist_retext ->
    //     rtext_retext -> rtext_senditup ->
    //       text_drawborder (firsttime=0) -> glist_drawiofor (firsttime=0)
    // This means that a new gatom tries to redraw its inlets before
    // it has created them.
    gui(cid).get_elem(tag + type + i, {
        x: x - basex,
        y: y - basey
    });
}

function gui_gobj_erase_io(cid, tag) {
    gui(cid).get_elem(tag, function(e) {
        e.parentNode.removeChild(e);
    });
}

function gui_gobj_configure_io(cid, tag, is_iemgui, is_signal, width) {
    gui(cid).get_elem(tag, {
        "stroke-width": width
    })
    .get_elem(tag, function(e) {
        var type;
        if (is_iemgui) {
            type = "xlet_iemgui";
        } else if (is_signal) {
            type = "xlet_signal";
        } else {
            "xlet_control";
        }
        e.classList.add(type);
        e.classList.remove("xlet_selected");
    });
}

function gui_gobj_highlight_io(cid, tag) {
    gui(cid).get_elem(tag, function(e) {
        e.classList.add("xlet_selected");
    });
}

function message_border_points(width, height) {
    return [0,0,
            width+4, 0,
            width, 4,
            width, height-4,
            width+4, height,
            0, height,
            0, 0]
        .join(" ");
}

function gui_message_draw_border(cid, tag, width, height) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var polygon = create_item(cid, "polygon", {
            points: message_border_points(width, height),
            fill: "none",
            stroke: "black",
            class: "border"
            //id: tag + "border"
        });
        frag.appendChild(polygon);
        return frag;
    });
}

function gui_message_flash(cid, tag, state) {
    gui(cid).get_gobj(tag, function(e) {
        if (state !== 0) {
            e.classList.add("flashed");
        } else {
            e.classList.remove("flashed");
        }
    });
}

function gui_message_redraw_border(cid, tag, width, height) {
    gui(cid).get_gobj(tag)
    .q(".border", {
        points: message_border_points(width, height)
    });
}

function atom_border_points(width, height, is_dropdown) {
    // For atom, angle the top-right corner.
    // For dropdown, angle both top-right and bottom-right corners
    var bottom_right_x = is_dropdown ? width - 4 : width;
    return  [0, 0,
            width - 4, 0,
            width, 4,
            width, height - 4,
            bottom_right_x, height,
            0, height,
            0, 0]
        .join(" ");
}

function atom_arrow_points(width, height) {
    var m = height < 20 ? 1 : height / 12;
    return [width - (9 * m), height * 0.5 - Math.floor(1 * m),
        width - (3 * m), height * 0.5 - Math.floor(1 * m),
        width - (6 * m), height * 0.5 + Math.floor(4 * m),
    ].join(" ");
}

function gui_atom_draw_border(cid, tag, type, width, height) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var polygon = create_item(cid, "polygon", {
            points: atom_border_points(width, height, type !== 0),
            fill: "none",
            stroke: "gray",
            "stroke-width": 1,
            class: "border"
            //id: tag + "border"
        });
           
        frag.appendChild(polygon);
        if (type !== 0) { // dropdown
            // 1 = output index
            // 2 = output value
            // Let's make the two visually distinct so that the user can still
            // reason about the patch functionality merely by reading the
            // diagram
            var m = height < 20 ? 1 : height / 12;
            var arrow = create_item(cid, "polygon", {
                points: atom_arrow_points(width, height),
                "class": type === 1 ? "arrow index_arrow" : "arrow value_arrow"
            });
            frag.appendChild(arrow);
        }
        return frag;
    });
}

function gui_atom_redraw_border(cid, tag, type, width, height) {
    gui(cid).get_gobj(tag)
    .q("polygon",  {
        points: atom_border_points(width, height, type !== 0) 
    });
    if (type !== 0) {
        gui(cid).get_gobj(tag)
        .q(".arrow", {
            points: atom_arrow_points(width, height)
        });
    }
}

// draw a patch cord
function gui_canvas_line(cid,tag,type,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10) {
    gui(cid).get_elem("patchsvg")
    .append(function(frag) {
        var svg = get_item(cid, "patchsvg"),
        // xoff is for making sure straight lines are crisp.  An SVG stroke
        // straddles the coordinate, with 1/2 the width on each side.
        // Control cords are 1 px wide, which requires a 0.5 x-offset to align
        // the stroke to the pixel grid.
        // Signal cords are 2 px wide = 1px on each side-- no need for x-offset.
            xoff = type === 'signal' ? 0 : 0.5,
            d_array = ["M", p1 + xoff, p2 + xoff,
                       "Q", p3 + xoff, p4 + xoff, p5 + xoff, p6 + xoff,
                       "Q", p7 + xoff, p8 + xoff, p9 + xoff, p10 + xoff],
            path;
        path = create_item(cid, "path", {
            d: d_array.join(" "),
            fill: "none",
            //"shape-rendering": "optimizeSpeed",
            id: tag,
            "class": "cord " + type
        });
        frag.appendChild(path);
        return frag;
    });
}

function gui_canvas_select_line(cid, tag) {
    gui(cid).get_elem(tag, function(e) {
        e.classList.add("selected_line");
    });
}

function gui_canvas_deselect_line(cid, tag) {
    gui(cid).get_elem(tag, function(e) {
        e.classList.remove("selected_line");
    });
}

// rename to erase_line (or at least standardize with gobj_erase)
function gui_canvas_delete_line(cid, tag) {
    gui(cid).get_elem(tag, function(e) {
        e.parentNode.removeChild(e);
    });
}

function gui_canvas_update_line(cid, tag, x1, y1, x2, y2, yoff) {
    // We have to check for existence here for the special case of
    // preset_node which hides a wire that feeds back from the downstream
    // object to its inlet. Pd refrains from drawing this hidden wire at all.
    // It should also suppress a call here to update that line, but it
    // currently doesn't. So we check for existence.
    gui(cid).get_elem(tag, function(e) {
        var halfx = parseInt((x2 - x1)/2),
            halfy = parseInt((y2 - y1)/2),
            xoff, // see comment in gui_canvas_line about xoff
            d_array;
        xoff = e.classList.contains("signal") ? 0: 0.5;
        d_array = ["M",x1+xoff,y1+xoff,
                   "Q",x1+xoff,y1+yoff+xoff,x1+halfx+xoff,y1+halfy+xoff,
                   "Q",x2+xoff,y2-yoff+xoff,x2+xoff,y2+xoff];
        configure_item(e, { d: d_array.join(" ") });
    });
}

function text_line_height_kludge(fontsize, fontsize_type) {
    var pd_fontsize = fontsize_type === "gui" ?
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

function text_to_tspans(cid, svg_text, text) {
    var lines, i, len, tspan, fontsize, text_node;
    lines = text.split("\n");
    len = lines.length;
    // Get fontsize (minus the trailing "px")
    fontsize = svg_text.getAttribute("font-size").slice(0, -2);
    for (i = 0; i < len; i++) {
        tspan = create_item(cid, "tspan", {
            dy: i == 0 ? 0 : text_line_height_kludge(+fontsize, "gui") + "px",
            x: 0
        });
        // find a way to abstract away the canvas array and the DOM here
        text_node = patchwin[cid].window.document
                    .createTextNode(lines[i]);
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

function font_map() {
    return {
        // pd_size: gui_size
        8: 8.33,
        12: 11.65,
        16: 16.65,
        24: 23.3,
        36: 36.6
    };
}

// This is a suboptimal font map, necessary because some genius "improved"
// the font stack on Gnu/Linux by delivering font metrics that don't match
// at all with what you get in OSX, Windows, nor even the previous version
// of the Gnu/Linux stack.
function suboptimal_font_map() {
    return {
        // pd_size: gui_size
        8: 8.45,
        12: 11.4,
        16: 16.45,
        24: 23.3,
        36: 36
    }
}

function gobj_fontsize_kludge(fontsize, return_type) {
    // These were tested on an X60 running Trisquel (based
    // on Ubuntu 14.04)
    var ret, prop,
        fmap = font_stack_is_maintained_by_troglodytes() ?
            suboptimal_font_map() : font_map();
    if (return_type === "gui") {
        ret = fmap[fontsize];
        return ret ? ret : fontsize;
    } else {
        for (prop in fmap) {
            if (fmap.hasOwnProperty(prop)) {
                if (fmap[prop] == fontsize) {
                    return +prop;
                }
            }
        }
        return fontsize;
    }
}

function pd_fontsize_to_gui_fontsize(fontsize) {
    return gobj_fontsize_kludge(fontsize, "gui");
}

function gui_fontsize_to_pd_fontsize(fontsize) {
    return gobj_fontsize_kludge(fontsize, "pd");
}

// Another hack, similar to above. We use this to
// make sure that there is enough vertical space
// between lines to fill the box when there is
// multi-line text.
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

function gui_text_new(cid, tag, type, isselected, left_margin, font_height, text, font) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var svg_text = create_item(cid, "text", {
            // Maybe it's just me, but the svg spec's explanation of how
            // text x/y and tspan x/y interact is difficult to understand.
            // So here we just translate by the right amount for the
            // left-margin, guaranteeing all tspan children will line up where
            // they should be.

            // Another anomaly-- we add 0.5 to the translation so that the font
            // hinting works correctly. This effectively cancels out the 0.5
            // pixel alignment done in the gobj, so it might be better to
            // specify the offset in whatever is calling this function.

            // I don't know how svg text grid alignment relates to other svg
            // shapes, and I haven't yet found any documentation for it. All I
            // know is an integer offset results in blurry text, and the 0.5
            // offset doesn't.
            transform: "translate(" + (left_margin - 0.5) + ")",
            y: font_height + gobj_font_y_kludge(font),
            // Turns out we can't do 'hanging' baseline
            // because it's borked when scaled. Bummer, because that's how Pd's
            // text is handled under tk...
            // 'dominant-baseline': 'hanging',
            "shape-rendering": "crispEdges",
            "font-size": pd_fontsize_to_gui_fontsize(font) + "px",
            "font-weight": "normal",
            id: tag + "text",
            "class": "box_text"
        });
        // trim off any extraneous leading/trailing whitespace. Because of
        // the way binbuf_gettext works we almost always have a trailing
        // whitespace.
        text = text.trim();
        // fill svg_text with tspan content by splitting on '\n'
        text_to_tspans(cid, svg_text, text);
        frag.appendChild(svg_text);
        if (isselected) {
            gui_gobj_select(cid, tag);
        }
        return frag;
    });
}

// Because of the overly complex code path inside
// canvas_setgraph, multiple erasures can be triggered in a row.
function gui_gobj_erase(cid, tag) {
    gui(cid).get_gobj(tag, function(e) {
        e.parentNode.removeChild(e);
    });
}

function gui_text_set (cid, tag, text) {
    gui(cid).get_elem(tag + "text", function(e) {
        text = text.trim();
        e.textContent = "";
        text_to_tspans(cid, e, text);
    });
}

function gui_text_redraw_border(cid, tag, width, height) {
    // Hm, need to figure out how to refactor to get rid of
    // configure_item call...
    gui(cid).get_gobj(tag, function(e) {
        var b = e.querySelectorAll(".border"),
        i;
        for (i = 0; i < b.length; b++) {
            configure_item(b[i], {
                width: width,
                height: height
            });
        }
    });
}

function gui_gobj_select(cid, tag) {
    gui(cid).get_gobj(tag, function(e) {
        e.classList.add("selected");
    });
}

function gui_gobj_deselect(cid, tag) {
    gui(cid).get_gobj(tag, function(e) {
        e.classList.remove("selected");
    });
}

// This adds a 0.5 offset to align to pixel grid, so it should
// only be used to move gobjs to a new position.  (Should probably
// be renamed to gobj_move to make this more obvious.)
function elem_move(elem, x, y) {
    var t = elem.transform.baseVal.getItem(0);
    t.matrix.e = x+0.5;
    t.matrix.f = y+0.5;
}

function elem_displace(elem, dx, dy) {
        var t = elem.transform.baseVal.getItem(0);
        t.matrix.e += dx;
        t.matrix.f += dy;
}

function elem_get_coords(elem) {
    var t = elem.transform.baseVal.getItem(0);
    return {
        x: t.matrix.e,
        y: t.matrix.f
    }
}

// used for tidy up
function gui_text_displace(name, tag, dx, dy) {
    elem_displace(get_gobj(name, tag), dx, dy);
}

function textentry_displace(t, dx, dy) {
    var transform = t.style.getPropertyValue("transform")
        .split("(")[1]    // get everything after the "("
        .replace(")", "") // remove trailing ")"
        .split(",");      // split into x and y
    var x = +transform[0].trim().replace("px", ""),
        y = +transform[1].trim().replace("px", "");
    t.style.setProperty("transform",
        "translate(" +
        (x + dx) + "px, " +
        (y + dy) + "px)");
}

function gui_canvas_displace_withtag(cid, dx, dy) {
    gui(cid)
    .get_elem("patchsvg", function(svg_elem, w) {
        var i, ol;
        ol = w.document.getElementsByClassName("selected");
        for (i = 0; i < ol.length; i++) {
            elem_displace(ol[i], dx, dy);
        }
    })
    .get_elem("new_object_textentry", function(textentry) {
        textentry_displace(textentry, dx, dy);
    });
}

function gui_canvas_draw_selection(cid, x1, y1, x2, y2) {
    gui(cid).get_elem("patchsvg", function(svg_elem) {
        var points_array = [x1 + 0.5, y1 + 0.5,
                            x2 + 0.5, y1 + 0.5,
                            x2 + 0.5, y2 + 0.5,
                            x1 + 0.5, y2 + 0.5
        ];
        var rect = create_item(cid, "polygon", {
            points: points_array.join(" "),
            fill: "none",
            //"shape-rendering": "optimizeSpeed",
            "stroke-width": 1,
            id: "selection_rectangle",
            display: "inline"
        });
        svg_elem.appendChild(rect);
    });
}

function gui_canvas_move_selection(cid, x1, y1, x2, y2) {
    var points_array = [x1 + 0.5, y1 + 0.5, x2 + 0.5, y1 + 0.5,
                        x2 + 0.5, y2 + 0.5, x1 + 0.5, y2 + 0.5];
    gui(cid).get_elem("selection_rectangle", {
        points: points_array
    });

}

function gui_canvas_hide_selection(cid) {
    gui(cid).get_elem("selection_rectangle", function(e) {
        e.parentElement.removeChild(e);
    });
}

// iemguis

function gui_bng_new(cid, tag, cx, cy, radius) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var circle = create_item(cid, "circle", {
            cx: cx,
            cy: cy,
            r: radius,
            "shape-rendering": "auto",
            fill: "none",
            stroke: "black",
            "stroke-width": 1,
            id: tag + "button"
        });
        frag.appendChild(circle);
        return frag;
    });
}

function gui_bng_button_color(cid, tag, color) {
    gui(cid).get_elem(tag + "button", {
        fill: color
    });
}

function gui_bng_configure(cid, tag, color, cx, cy, r) {
    gui(cid).get_elem(tag + "button", {
        cx: cx,
        cy: cy,
        r: r,
        fill: color
    });
}

function gui_toggle_new(cid, tag, color, width, state, p1,p2,p3,p4,p5,p6,p7,p8,basex,basey) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var points = [p1 - basex, p2 - basey,
                      p3 - basex, p4 - basey
        ].join(" ");
        var cross1 = create_item(cid, "polyline", {
            points: points,
            stroke: color,
            fill: "none",
            id: tag + "cross1",
            display: state ? "inline" : "none",
            "stroke-width": width
        });
        points = [p5 - basex, p6 - basey,
                  p7 - basex, p8 - basey
        ].join(" ");
        var cross2 = create_item(cid, "polyline", {
            points: points,
            stroke: color,
            fill: "none",
            id: tag + "cross2",
            display: state ? "inline" : "none",
            "stroke-width": width
        });
        frag.appendChild(cross1);
        frag.appendChild(cross2);
        return frag;
    });
}

function gui_toggle_resize_cross(cid,tag,w,p1,p2,p3,p4,p5,p6,p7,p8,basex,basey) {
    var points1 = [p1 - basex, p2 - basey,
                  p3 - basex, p4 - basey
    ].join(" "),
        points2 = [p5 - basex, p6 - basey,
                        p7 - basex, p8 - basey
    ].join(" ");
    gui(cid)
    .get_elem(tag + "cross1", {
        points: points1,
        "stroke-width": w
    })
    .get_elem(tag + "cross2", {
        points: points2,
        "stroke-width": w
    });
}

function gui_toggle_update(cid, tag, state, color) {
    var disp = !!state ? "inline" : "none";
    gui(cid)
    .get_elem(tag + "cross1", {
        display: disp,
        stroke: color
    })
    .get_elem(tag + "cross2", {
        display: disp,
        stroke: color
    })
}

function numbox_data_string(w, h) {
    return ["M", 0, 0,
            "L", w - 4, 0,
                 w, 4,
                 w, h,
                 0, h,
            "z",
            "L", 0, 0,
                 (h / 2)|0, (h / 2)|0, // |0 to force int
                 0, h]
    .join(" ");
}

// Todo: send fewer parameters from c
function gui_numbox_new(cid, tag, color, x, y, w, h, is_toplevel) {
    // numbox doesn't have a standard iemgui border,
    // so we must create its gobj manually
    gui(cid).get_elem("patchsvg", function() {
        var g = gui_gobj_new(cid, tag, "iemgui", x, y, is_toplevel);
        var data = numbox_data_string(w, h);
        var border = create_item(cid, "path", {
            d: data,
            fill: color,
            stroke: "black",
            "stroke-width": 1,
            id: (tag + "border"),
            "class": "border"
        });
        g.appendChild(border);
    });
}

function gui_numbox_coords(cid, tag, w, h) {
    gui(cid).get_elem(tag + "border", {
        d: numbox_data_string(w, h)
    });
}

function gui_numbox_draw_text(cid,tag,text,font_size,color,xpos,ypos,basex,basey) {
    // kludge alert -- I'm not sure why I need to add half to the ypos
    // below. But it works for most font sizes.
    gui(cid).get_gobj(tag)
    .append(function(frag, w) {
        var svg_text = create_item(cid, "text", {
            transform: "translate(" +
                        (xpos - basex) + "," +
                        ((ypos - basey + (ypos - basey) * 0.5)|0) + ")",
            "font-size": font_size,
            fill: color,
            id: tag + "text"
        }),
        text_node = w.document.createTextNode(text);
        svg_text.appendChild(text_node);
        frag.appendChild(svg_text);
        return frag;
    });
}

function gui_numbox_update(cid, tag, fcolor, bgcolor, font_name, font_size, font_weight) {
    gui(cid)
    .get_elem(tag + "border", {
        fill: bgcolor
    })
    .get_elem(tag + "text", {
        fill: fcolor,
        "font-size": font_size
    })
    // label may or may not exist, but that's covered by the API
    .get_elem(tag + "label", function() {
        gui_iemgui_label_font(cid, tag, font_name, font_weight, font_size);
    });
}

function gui_numbox_update_text_position(cid, tag, x, y) {
    gui(cid).get_elem(tag + "text", {
        transform: "translate( " + x + "," + ((y + y*0.5)|0) + ")"
    });
}

function gui_slider_new(cid, tag, color, p1, p2, p3, p4, basex, basey) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var indicator = create_item(cid, "line", {
            x1: p1 - basex,
            y1: p2 - basey,
            x2: p3 - basex,
            y2: p4 - basey,
            stroke: color,
            "stroke-width": 3,
            fill: "none",
            id: tag + "indicator"
        });
        frag.appendChild(indicator);
        return frag;
    });
}

function gui_slider_update(cid, tag, p1, p2, p3, p4, basex, basey) {
    gui(cid).get_elem(tag + "indicator", {
        x1: p1 - basex,
        y1: p2 - basey,
        x2: p3 - basex,
        y2: p4 - basey
    });
}

function gui_slider_indicator_color(cid, tag, color) {
    gui(cid).get_elem(tag + "indicator", {
        stroke: color
    });
}

function gui_radio_new(cid, tag, p1, p2, p3, p4, i, basex, basey) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var cell = create_item(cid, "line", {
            x1: p1 - basex,
            y1: p2 - basey,
            x2: p3 - basex,
            y2: p4 - basey,
            // stroke is just black for now
            stroke: "black",
            "stroke-width": 1,
            fill: "none",
            id: tag + "cell_" + i
        });
        frag.appendChild(cell);
        return frag;
    });
}

function gui_radio_create_buttons(cid,tag,color,p1,p2,p3,p4,basex,basey,i,state) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var b = create_item(cid, "rect", {
            x: p1 - basex,
            y: p2 - basey,
            width: p3 - p1,
            height: p4 - p2,
            stroke: color,
            fill: color,
            id: tag + "button_" + i,
            display: state ? "inline" : "none"
        });
        frag.appendChild(b);
        return frag;
    });
}

function gui_radio_button_coords(cid, tag, x1, y1, xi, yi, i, s, d, orient) {
    gui(cid)
    .get_elem(tag + "button_" + i, {
        x: orient ? s : xi+s,
        y: orient ? yi+s : s,
        width: d-(s*2),
        height: d-(s*2)
    })
    // the line to draw the cell for i=0 doesn't exist. Probably was not worth
    // the effort, but it's easier just to check for that here atm.
    if (i > 0) {
        gui(cid)
        .get_elem(tag + "cell_" + i, {
            x1: orient ? 0 : xi,
            y1: orient ? yi : 0,
            x2: orient ? d : xi,
            y2: orient ? yi : d
        });
    }
}

function gui_radio_update(cid, tag, fgcolor, prev, next) {
    gui(cid)
    .get_elem(tag + "button_" + prev, {
        display: "none"
    })
    .get_elem(tag + "button_" + next, {
        display: "inline",
        fill: fgcolor,
        stroke: fgcolor
    });
}

function gui_vumeter_draw_text(cid,tag,color,xpos,ypos,text,index,basex,basey, font_size, font_weight) {
    gui(cid).get_gobj(tag)
    .append(function(frag, w) {
        var svg_text = create_item(cid, "text", {
            x: xpos - basex,
            y: ypos - basey,
            "font-family": iemgui_fontfamily(fontname),
            "font-size": font_size,
            "font-weight": font_weight,
            id: tag + "text_" + index
        }),
        text_node = w.document.createTextNode(text);
        svg_text.appendChild(text_node);
        frag.appendChild(svg_text);
        return frag;
    });
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
function gui_vumeter_update_text(cid, tag, text, font, selected, color, i) {
    gui(cid).get_elem(tag + "text_" + i, {
        fill: color
    });
}

function gui_vumeter_text_coords(cid, tag, i, xpos, ypos, basex, basey) {
    gui(cid).get_elem(tag + "text_" + i, {
        x: xpos - basex,
        y: ypos - basey
    });
}

function gui_vumeter_erase_text(cid, tag, i) {
    gui(cid).get_elem(tag + "text_" + i, function(e) {
        e.parentNode.removeChild(e);
    });
}

function gui_vumeter_create_steps(cid,tag,color,p1,p2,p3,p4,width,basex,basey,i) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var l = create_item(cid, "line", {
            x1: p1 - basex,
            y1: p2 - basey,
            x2: p3 - basex,
            y2: p4 - basey,
            stroke: color,
            "stroke-width": width,
            "id": tag + "led_" + i
        });
        frag.appendChild(l);
        return frag;
    });
}

function gui_vumeter_update_steps(cid, tag, i, width) {
    gui(cid).get_elem(tag + "led_" + i, {
        "stroke-width": width
    });
}

function gui_vumeter_update_step_coords(cid,tag,i,x1,y1,x2,y2,basex,basey) {
    gui(cid).get_elem(tag + "led_" + i, {
        x1: x1 - basex,
        y1: y1 - basey,
        x2: x2 - basex,
        y2: y2 - basey
    });
}

function gui_vumeter_draw_rect(cid,tag,color,p1,p2,p3,p4,basex,basey) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var rect = create_item(cid, "rect", {
            x: p1 - basex,
            y: p2 - basey,
            width: p3 - p1,
            height: p4 + 1 - p2,
            stroke: color,
            fill: color,
            id: tag + "rect"
        });
        frag.appendChild(rect);
        return frag;
    });
}

function gui_vumeter_update_rect(cid, tag, color) {
    gui(cid).get_elem(tag + "rect", {
        fill: color,
        stroke: color
    });
}

// Oh hack upon hack... why doesn't the iemgui base_config just take care
// of this?
function gui_vumeter_border_size(cid, tag, width, height) {
    gui(cid).get_gobj(tag)
    .q(".border", {
        width: width,
        height: height
    });
}

function gui_vumeter_update_peak_width(cid, tag, width) {
    gui(cid).get_elem(tag + "rect", {
        "stroke-width": width
    });
}

function gui_vumeter_draw_peak(cid,tag,color,p1,p2,p3,p4,width,basex,basey) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var line = create_item(cid, "line", {
            x1: p1 - basex,
            y1: p2 - basey,
            x2: p3 - basex,
            y2: p4 - basey,
            stroke: color,
            "stroke-width": width,
            id: tag + "peak"
        }); 
        frag.appendChild(line);
        return frag;
    });
}

// probably should change tag from "rect" to "cover"
function gui_vumeter_update_rms(cid, tag, p1, p2, p3, p4, basex, basey) {
    gui(cid).get_elem(tag + "rect", {
        x: p1 - basex,
        y: p2 - basey,
        width: p3 - p1,
        height: p4 - p2 + 1
    });
}

function gui_vumeter_update_peak(cid,tag,color,p1,p2,p3,p4,basex,basey) {
    gui(cid).get_elem(tag + "peak", {
        x1: p1 - basex,
        y1: p2 - basey,
        x2: p3 - basex,
        y2: p4 - basey,
        stroke: color
    });
}

function gui_iemgui_base_color(cid, tag, color) {
    gui(cid).get_gobj(tag)
    .q(".border", {
        fill: color
    });
}

function gui_iemgui_move_and_resize(cid, tag, x1, y1, x2, y2) {
    gui(cid).get_gobj(tag, function(e) {
        elem_move(e, x1, y1);
    })
    .q(".border", {
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
    if (name === "DejaVu Sans Mono" || name == "helvetica") {
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
    gui(cid).get_gobj(tag)
    .append(function(frag, w) {
        var svg_text = create_item(cid, "text", {
            // x and y need to be relative to baseline instead of nw anchor
            x: x,
            y: y,
            //"font-size": font + "px",
            "font-family": iemgui_fontfamily(fontname),
            // for some reason the font looks bold in Pd-Vanilla-- not sure why
            "font-weight": fontweight,
            "font-size": fontsize + "px",
            fill: color,
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
            transform: "translate(0," +
                iemgui_font_height(fontname, fontsize) / 2 + ")",
            id: tag + "label"
        });
        var text_node = w.document.createTextNode(text);
        svg_text.appendChild(text_node);
        frag.appendChild(svg_text);
        return frag;
    });
}

function gui_iemgui_label_set(cid, tag, text) {
    gui(cid).get_elem(tag + "label", function(e) {
        e.textContent = text; 
    });
}

function gui_iemgui_label_coords(cid, tag, x, y) {
    gui(cid).get_elem(tag + "label", {
        x: x,
        y: y
    });
}

function gui_iemgui_label_color(cid, tag, color) {
    gui(cid).get_elem(tag + "label", {
        fill: color
    });
}

function gui_iemgui_label_color(cid, tag, color) {
    gui(cid).get_elem(tag + "label", {
        fill: color
    });
}

function gui_iemgui_label_select(cid, tag, is_selected) {
    gui(cid).get_elem(tag + "label", function(e) {
        if (!!is_selected) {
            e.classList.add("iemgui_label_selected");
        } else {
            e.classList.remove("iemgui_label_selected");
        }
    });
}

function gui_iemgui_label_font(cid, tag, fontname, fontweight, fontsize) {
    gui(cid).get_elem(tag + "label", {
        "font-family": iemgui_fontfamily(fontname),
        "font-weight": fontweight,
        "font-size": fontsize + "px",
        transform: "translate(0," + iemgui_font_height(fontname, fontsize) / 2 + ")"
    });
}

// Show or hide little handle for dragging around iemgui labels
function gui_iemgui_label_show_drag_handle(cid, tag, state, x, y, cnv_resize) {
    if (state !== 0) {
        gui(cid).get_gobj(tag)
        .append(function(frag) {
            var rect;
            // Here we use a "line" shape so that we can control its color
            // using the "border" class (for iemguis) or the "gop_rect" class
            // for the graph-on-parent rectangle anchor. In both cases the
            // styles set a stroke property, and a single thick line is easier
            // to define than a "rect" for that case.
            rect = create_item(cid, "line", {
                x1: x,
                y1: y + 3,
                x2: x,
                y2: y + 10,
                "stroke-width": 7,
                class: (cid === tag) ? "gop_drag_handle move_handle gop_rect" :
                    cnv_resize !== 0 ? "cnv_resize_handle border" :
                    "label_drag_handle move_handle border"
            });
            rect.classList.add("clickable_resize_handle");
            frag.appendChild(rect);
            return frag;
        });
    } else {
        gui(cid).get_gobj(tag, function(e) {
            var rect =
                e.getElementsByClassName((cid === tag) ? "gop_drag_handle" :
                    cnv_resize !== 0 ? "cnv_resize_handle" :
                        "label_drag_handle")[0];
            //rect = get_item(cid, "clickable_resize_handle");
            // Need to check for null here...
            if (rect) {
                rect.parentNode.removeChild(rect);
            } else {
                post("error: couldn't delete the iemgui drag handle!");
            }
        });
    }
}

function gui_mycanvas_new(cid,tag,color,x1,y1,x2_vis,y2_vis,x2,y2) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var rect_vis, rect, g;
        rect_vis = create_item(cid, "rect", {
            width: x2_vis - x1,
            height: y2_vis - y1,
            fill: color,
            stroke: color,
            id: tag + "rect"
            }
        );
        // we use a drag_handle, which is square outline with transparent fill
        // that shows the part of the rectangle that may be dragged in editmode.
        // Clicking the rectangle outside of that square will have no effect.
        // Unlike a 'border' it takes the same color as the visible rectangle
        // when deselected.
        // I'm not sure why it was decided to define this object's bbox separate
        // from the visual rectangle. That causes all kinds of usability
        // problems.
        // For just one example, it means we can't simply use the "resize"
        // cursor like all the other iemguis.
        // Unfortunately its ingrained as a core object in Pd, so we have to
        // support it here.
        rect = create_item(cid, "rect", {
            width: x2 - x1,
            height: y2 - y1,
            fill: "none",
            stroke: color,
            id: tag + "drag_handle",
            "class": "border mycanvas_border"
            }
        );
        frag.appendChild(rect_vis);
        frag.appendChild(rect);
        return frag;
    });
}

function gui_mycanvas_update(cid, tag, color, selected) {
    gui(cid)
    .get_elem(tag + "rect", {
        fill: color,
        stroke: color
    })
    .get_elem(tag + "drag_handle", {
        stroke: color
    });
}

function gui_mycanvas_coords(cid, tag, vis_width, vis_height, select_width, select_height) {
    gui(cid)
    .get_elem(tag + "rect", {
        width: vis_width,
        height: vis_height
    })
    .get_elem(tag + "drag_handle", {
        width: select_width,
        height: select_height
    });
}

function gui_scalar_new(cid, tag, isselected, t1, t2, t3, t4, t5, t6,
    is_toplevel) {
    var g;
    // we should probably use gui_gobj_new here, but we"re doing some initial
    // scaling that normal gobjs don't need...
    gui(cid).get_elem("patchsvg", function(svg_elem) {
        var matrix, transform_string, selection_rect;
        matrix = [t1,t2,t3,t4,t5,t6];
        transform_string = "matrix(" + matrix.join() + ")";
        g = create_item(cid, "g", {
            id: tag + "gobj",
            transform: transform_string,
        });
        if (isselected !== 0) {
            g.classList.add("selected");
        }
        if (is_toplevel === 0) {
            g.classList.add("gop");
        }
        // Let's make a selection rect...
        selection_rect = create_item(cid, "rect", {
            class: "border",
            display: "none",
            fill: "none",
            "pointer-events": "none"
        });
        g.appendChild(selection_rect);
        add_gobj_to_svg(svg_elem, g);
    });
    return g;
}

function gui_scalar_erase(cid, tag) {
    gui(cid).get_gobj(tag, function(e) {
        e.parentNode.removeChild(e);
    });
}

// This is unnecessarily complex-- the select rect is a child of the parent
// scalar group, but in the initial Tkpath API the rect was free-standing.
// This means all the coordinate parameters are in the screen position. But
// we need the coords relative to the scalar's x/y-- hence we subtract the
// scalar's basex/basey from the coords below.

// Additionally, this function is a misnomer-- we're not actually drawing
// the rect here.  It's drawn as part of the scalar_vis function.  We're
// merely changing its coords and size.

// Finally, we have this awful display attribute toggling in css
// for selected borders because somehow calling properties on a graph
// triggers this function.  I have no idea why it does that.
function gui_scalar_draw_select_rect(cid, tag, state, x1, y1, x2, y2, basex, basey) {
    gui(cid).get_gobj(tag)
    .q(".border", {
        x: (x1 - basex) + 0.5,
        y: (y1 - basey) + 0.5,
        width: x2 - x1,
        height: y2 - y1
    });
}

function gui_scalar_draw_group(cid, tag, parent_tag, type, attr_array) {
    gui(cid).get_elem(parent_tag)
    .append(function(frag) {
        if (!attr_array) {
            attr_array = [];
        }
        attr_array.push("id", tag);
        var group = create_item(cid, type, attr_array);
        frag.appendChild(group);
        return frag;
    });
}

function gui_scalar_configure_gobj(cid, tag, isselected, t1, t2, t3, t4, t5, t6) {
    var matrix = [t1,t2,t3,t4,t5,t6],
        transform_string = "matrix(" + matrix.join() + ")";
    gui(cid).get_gobj(tag, {
        transform: transform_string 
    });
}

function gui_draw_vis(cid, type, attr_array, tag_array) {
    gui(cid).get_elem(tag_array[0])
    .append(function(frag) {
        var item;
        attr_array.push("id", tag_array[1]);
        item = create_item(cid, type, attr_array);
        frag.appendChild(item);
        return frag;
    });
}

// This is a stop gap to update the old draw commands like [drawpolygon]
// without having to erase and recreate their DOM elements
function gui_draw_configure_old_command(cid, type, attr_array, tag_array) {
    gui(cid).get_elem(tag_array[1], function(e) {
        configure_item(e, attr_array);
    });
}

function gui_draw_erase_item(cid, tag) {
    gui(cid).get_elem(tag, function(e) {
        e.parentNode.removeChild(e);
    });
}

function gui_draw_coords(cid, tag, shape, points) {
    gui(cid).get_elem(tag, function(elem) {
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
    });
}

// set a drag event for a shape that's part of a scalar.
// this is a convenience method for the user, so that dragging outside
// of the bbox of the shape will still register as part of the event.
// (Attempting to set the event more than once is ignored.)
function gui_draw_drag_event(cid, tag, scalar_sym, drawcommand_sym,
    event_name, array_sym, index, state) {
    gui(cid).get_elem("patchsvg", function(svg_elem, w) {
        if (state === 0) {
            w.canvas_events.remove_scalar_draggable(tag);
        } else {
            w.canvas_events.add_scalar_draggable(cid, tag, scalar_sym,
                drawcommand_sym, event_name, array_sym, index);
        }
    });
}

// Events for scalars-- mouseover, mouseout, etc.
function gui_draw_event(cid, tag, scalar_sym, drawcommand_sym, event_name,
    array_sym, index, state) {
    gui(cid).get_elem(tag, function(e) {
        var event_type = "on" + event_name;
        if (state === 1) {
            e[event_type] = function(e) {
                pdsend(cid, "scalar_event", scalar_sym, drawcommand_sym,
                    array_sym, index, event_name, e.pageX, e.pageY);
            };
        } else {
            e[event_type] = null;
        }
    });
}

// Configure one attr/val pair at a time, received from Pd
function gui_draw_configure(cid, tag, attr, val) {
    gui(cid).get_elem(tag, function(e) {
        var obj = {};
        if (Array.isArray(val)) {
            obj[attr] = val.join(" ");
        } else {
            // strings or numbers
            obj[attr] = val;
        }
        configure_item(e, obj);
    });
}

// Special case for viewBox which, in addition to its inexplicably inconsistent
// camelcasing also has no "none" value in the spec. This requires us to create
// a special case to remove the attribute if the user wants to get back to
// the default behavior.
function gui_draw_viewbox(cid, tag, attr, val) {
    // Value will be an empty array if the user provided no values
    gui(cid).get_elem("patchsvg", function(svg_elem) {
        if (val.length) {
            gui_draw_configure(cid, tag, attr, val)
        } else {
            get_item(cid, tag).removeAttribute("viewBox");
        }
    });
}

// Configure multiple attr/val pairs (this should be merged with gui_draw_configure at some point
function gui_draw_configure_all(cid, tag, attr_array) {
    gui(cid).get_elem(tag, attr_array);
}

// Plots for arrays and data structures
function gui_plot_vis(cid, basex, basey, data_array, attr_array, tag_array) {
    gui(cid).get_elem(tag_array[0])
    .append(function(frag) {
        var p = create_item(cid, "path", {
            d: data_array.join(" "),
            id: tag_array[1],
            //stroke: "red",
            //fill: "black",
            //"stroke-width": "0"
        });
        configure_item(p, attr_array);
        frag.appendChild(p);
        return frag;
    });
}

// This function doubles as a visfn for drawnumber. Furthermore it doubles
// as a way to update attributes for drawnumber/symbol without having to
// recreate the object. The "flag" argument is 1 for creating a new element,
// and -1 to set attributes on the existing object.
function gui_drawnumber_vis(cid, parent_tag, tag, x, y, scale_x, scale_y,
    font, fontsize, fontcolor, text, flag, visibility) {
    if (flag === 1) {
        gui(cid).get_elem(parent_tag)
        .append(function(frag) {
            var svg_text = create_item(cid, "text", {
                // x and y are fudge factors. Text on the tk canvas used an
                // anchor at the top-right corner of the text's bbox.  SVG uses
                // the baseline. There's probably a programmatic way to do this,
                // but for now-- fudge factors based on the DejaVu Sans Mono
                // font. :)

                // For an explanation of why we translate by "x" instead of
                // setting the x attribute, see comment in gui_text_new
                transform: "scale(" + scale_x + "," + scale_y + ") " +
                           "translate(" + x + ")",
                y: y + fontsize,
                // Turns out we can't do 'hanging' baseline because it's borked
                // when scaled. Bummer...
                // "dominant-baseline": "hanging",
                //"shape-rendering": "optimizeSpeed",
                "font-size": fontsize + "px",
                fill: fontcolor,
                visibility: visibility === 1 ? "normal" : "hidden",
                id: tag
            });
            // fill svg_text with tspan content by splitting on "\n"
            text_to_tspans(cid, svg_text, text);
            frag.appendChild(svg_text);
            return frag;
        });
    } else {
        gui(cid).get_elem(tag, function(svg_text) {
            configure_item(svg_text, {
                transform: "scale(" + scale_x + "," + scale_y + ") " +
                           "translate(" + x + ")",
                y: y + fontsize,
                // Turns out we can't do 'hanging' baseline because it's borked
                // when scaled. Bummer...
                // "dominant-baseline": "hanging",
                //"shape-rendering": "optimizeSpeed",
                "font-size": fontsize + "px",
                fill: fontcolor,
                visibility: visibility === 1 ? "normal" : "hidden",
                id: tag
            });
            svg_text.textContent = "";
            text_to_tspans(cid, svg_text, text);
        });
    }
}

// closure to handle class-specific data that
// needs to be in the GUI. There shouldn't be
// many cases for this-- for now it's just used
// to cache image data for image-handling classes:
// ggee/image
// moonlib/image (for backwards compatibility only: its API is inherently leaky)
// tof/imagebang
// draw sprite
// draw image
var pd_cache = (function() {
    var d = {};
    return {
        free: function(key) {
            if (d.hasOwnProperty(key)) {
                d[key] = null;
            }
        },
        set: function(key, data) {
            d[key] = data;
            return data;
        },
        get: function(key) {
            if (d.hasOwnProperty(key)) {
                return d[key];
            } else {
                return undefined;
            }
        },
        debug: function() {
            return d;
        }
    };
}());

exports.pd_cache = pd_cache;

function gui_drawimage_new(obj_tag, file_path, canvasdir, flags) {
    var drawsprite = 1,
        drawimage_data = [], // array for base64 image data
        image_seq,
        count = 0,
        matchchar = "*",
        files,
        ext,
        img_types = [".gif", ".jpeg", ".jpg", ".png", ".svg"],
        img; // dummy image to measure width and height
    image_seq = flags & drawsprite;
    if (file_path !== "") {
        if(!path.isAbsolute(file_path)) {
            file_path = path.join(canvasdir, file_path);
        }
        file_path = path.normalize(file_path);
    }
    if (file_path !== "" && fs.existsSync(file_path)) {
        if (image_seq && fs.lstatSync(file_path).isDirectory()) {
            // [draw sprite]
            files = fs.readdirSync(file_path)
                    .sort(); // Note that js's "sort" method doesn't do the
                             // "right thing" for numbers. For that we'd need
                             // to provide our own sorting function
        } else {
            // [draw image]
            files = [path.basename(file_path)];
            file_path = path.dirname(file_path);
        }
        // todo: warn about image sequence with > 999
        files.forEach(function(file) {
            ext = path.extname(file).toLowerCase();
            if (img_types.indexOf(ext) != -1) {
                // Now add an element to that array with the image data
                drawimage_data.push({
                    type: ext === ".jpeg" ? "jpg" : ext.slice(1),
                    data: fs.readFileSync(path.join(file_path, file),"base64")
                });
                count++;
            }
        });
    }

    if (count === 0) {
        // set a default image
        drawimage_data.push({
            type: "png",
            data: get_default_png_data()
        });
        if (file_path !== "") {
            post("draw image: error: couldn't load image");
        }
        post("draw image: warning: no image loaded. Using default png");
    }
    img = new pd_window.Image(); // create an image in the pd_window context
    img.onload = function() {
        pdsend(obj_tag, "size", this.width, this.height);
    };
    img.src = "data:image/" + drawimage_data[0].type +
        ";base64," + drawimage_data[0].data;
    pd_cache.set(obj_tag, drawimage_data); // add the data to container
}

function gui_image_free(obj_tag) {
    var c = pd_cache.get(obj_tag);
    if (c) {
        pd_cache.free(obj_tag); // empty the image(s)
    } else {
        post("image: warning: no image data in cache to free");
    }
}

// We use this to get the correct height and width for the svg
// image. Unfortunately svg images are less flexible than normal
// html images-- you have to provide a size and 100% doesn't work.
// So here we load the image data into a new Image, just to get it
// to calculate the dimensions. We then use those dimensions for
// our svg image x/y, after which point the Image below _should_ 
// get garbage collected.
// We add the "tk_anchor" parameter so that we can match the awful interface
// of moonlib/image. We only check for the value "center"-- otherwise we
// assume "nw" (top-left corner) when tk_anchor is undefined, as this matches
// the svg spec.
function img_size_setter(cid, svg_image_tag, type, data, tk_anchor) {
    var img = new pd_window.window.Image(),
        w, h;
    img.onload = function() {
        w = this.width,
        h = this.height;
        configure_item(get_item(cid, svg_image_tag), {
            width: w,
            height: h,
            x: tk_anchor === "center" ? 0 - w/2 : 0,
            y: tk_anchor === "center" ? 0 - h/2 : 0
        });
    };
    img.src = "data:image/" + type + ";base64," + data;
}

function gui_drawimage_vis(cid, x, y, obj, data, seqno, parent_tag) {
    gui(cid).get_elem(parent_tag) // main <g> within the scalar
    .append(function(frag) {
        var item,
            image_array = pd_cache.get(obj),
            len = image_array.length,
            i,
            image_container,
            xy_container,
            obj_tag = "draw" + obj.slice(1) + "." + data.slice(1);
        if (len < 1) {
            return;
        }
        // Wrap around for out-of-bounds sequence numbers
        if (seqno >= len || seqno < 0) {
            seqno %= len;
        }
        // Since sprites can have lots of images we don't want to
        // set props on each one of them every time the user changes
        // an attribute's value. So we use a "g" to receive all the
        // relevant changes.
        // Unfortunately "g" doesn't have x/y attys. So it can't propagate
        // those values down to the child images. Thus we have to add
        // another "g" as a parent and manually convert x/y changes
        // the user makes to a transform. (And we can't use the inner g's
        // transform because the user can set their own transform there.)
        xy_container = create_item(cid, "g", {
            id: obj_tag + "xy",
            transform: "translate(" + x + "," + y + ")"
        });
        image_container = create_item(cid, "g", {
            id: obj_tag
        });
        for (i = 0; i < len; i++) {
            item = create_item(cid, "image", {
                x: x,
                y: y,
                id: obj_tag + i,
                visibility: seqno === i ? "visible" : "hidden",
                preserveAspectRatio: "xMinYMin meet"
            });
            item.setAttributeNS("http://www.w3.org/1999/xlink", "href",
                "data:image/" + image_array[i].type + ";base64," +
                 image_array[i].data);
            image_container.appendChild(item);
        }
        xy_container.appendChild(image_container);
        frag.appendChild(xy_container);
        // Hack to set correct width and height
        for (i = 0; i < len; i++) {
            img_size_setter(cid, obj_tag+i, pd_cache.get(obj)[i].type,
                pd_cache.get(obj)[i].data);
        }
        return frag;
    });
}

// Hack
function gui_drawimage_xy(cid, obj, data, x, y) {
    var obj_tag = "draw" + obj.slice(1) + "." + data.slice(1);
    gui(cid).get_elem(obj_tag + "xy", {
        transform: "translate(" + x + "," + y + ")"
    });
}

function gui_drawimage_index(cid, obj, data, index) {
    var obj_tag = "draw" + obj.slice(1) + "." + data.slice(1);
    gui(cid).get_elem(obj_tag, function(image_container) {
        var len = image_container.childNodes.length,
            image = image_container.childNodes[((index % len) + len) % len],
            last_image =
                image_container.querySelectorAll('[visibility="visible"]'),
            i;
        for (i = 0; i < last_image.length; i++) {
            configure_item(last_image[i], { visibility: "hidden" });
        }
        configure_item(image, { visibility: "visible" });
    });
}

// Default png image data
function get_default_png_data() {
    return ["iVBORw0KGgoAAAANSUhEUgAAABkAAAAZCAMAAADzN3VRAAAAb1BMVEWBgYHX19",
            "f///8vLy/8/Pzx8PH3+Pf19fXz8/Pu7u7l5eXj4+Pn5+fs7Oza2tr6+vnq6urh",
            "4eHe3t7c3Nza2dr6+fro6Og1NTXr6+xYWFi1tbWjo6OWl5aLjItDQ0PPz8+/v7",
            "+wsLCenZ5zc3NOTk4Rpd0DAAAAqElEQVQoz62L2Q6CMBBFhcFdCsomq+v/f6Mn",
            "bdOSBn3ypNO5Nyez+kG0zN9NWZZK8RRbB/2XmMLSvSZp2mehTMVcLGIYbcWcLW",
            "1/U4PIZCvmOCMSaWzEHGaMIq2NmJNn4ORuMybP6xxYD0SnE4NJDdc0fYv0LCJg",
            "9g4RqV3BrJfB7Bzc+ILZOjC+YDYOjC+YKqsyHlOZAX5Msgwm1iRxgDYBSWjCm+",
            "98AAfDEgD0K69gAAAAAElFTkSuQmCC"
           ].join("");
}

function gui_load_default_image(dummy_cid, key) {
    pd_cache.set(key, {
        type: "png",
        data: get_default_png_data()
    });
}

// Load an image and cache the base64 data
function gui_load_image(cid, key, filepath) {
    var data = fs.readFileSync(filepath,"base64"),
        ext = path.extname(filepath);
    pd_cache.set(key, {
        type: ext === ".jpeg" ? "jpg" : ext.slice(1),
        data: data
    });
}

// Draw an image in an object-- used for ggee/image, moonlib/image and
// tof/imagebang. For the meaning of tk_anchor see img_size_setter. This
// interface assumes there is only one image per gobject. If you try to
// set more you'll get duplicate ids.
function gui_gobj_draw_image(cid, tag, image_key, tk_anchor) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var i = create_item(cid, "image", {
            id: tag,
            preserveAspectRatio: "xMinYMin meet"
        });
        i.setAttributeNS("http://www.w3.org/1999/xlink", "href",
            "data:image/" + pd_cache.get(image_key).type + ";base64," +
             pd_cache.get(image_key).data);
        img_size_setter(cid, tag, pd_cache.get(image_key).type,
            pd_cache.get(image_key).data, tk_anchor);
        frag.appendChild(i);
        return frag;
    });
}

function gui_image_size_callback(cid, key, callback) {
    var img = new pd_window.Image(); // create an image in the pd_window context
    img.onload = function() {
        pdsend(callback, "_imagesize", this.width, this.height);
    };
    img.src = "data:image/" + pd_cache.get(key).type +
        ";base64," + pd_cache.get(key).data;
}

function gui_image_draw_border(cid, tag, x, y, w, h) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var b = create_item(cid, "path", {
            "stroke-width": "1",
            fill: "none",
            d: ["m", x, y, w, 0,
                "m", 0, 0, 0, h,
                "m", 0, 0, -w, 0,
                "m", 0, 0, 0, -h
               ].join(" "),
            visibility: "hidden",
            class: "border"
        });
        frag.appendChild(b);
        return frag;
    });
}

function gui_image_toggle_border(cid, tag, state) {
    gui(cid).get_gobj(tag)
    .q(".border", {
        visibility: state === 0 ? "hidden" : "visible"
    });
}

// Switch the data for an existing svg image
function gui_image_configure(cid, tag, image_key, tk_anchor) {
    gui(cid).get_elem(tag, function(e) {
        if (pd_cache.get(image_key)) {
            e.setAttributeNS("http://www.w3.org/1999/xlink", "href",
                "data:image/" + pd_cache.get(image_key).type + ";base64," +
                 pd_cache.get(image_key).data);
            img_size_setter(cid, tag, pd_cache.get(image_key).type,
                pd_cache.get(image_key).data, tk_anchor);
        } else {
            // need to change this to an actual error
            post("image: error: can't find image");
        }
    });
}

// Move an image
function gui_image_coords(cid, tag, x, y) {
    // ggee/image accepts a message that can trigger this, meaning
    // [loadbang] can end up calling this before the patchwindow exists.
    // So we have to check for existence below
    gui(cid).get_gobj(tag, function(e) {
        elem_move(e, x, y);
    });
}

// Scope~
function gui_scope_draw_bg(cid, tag, fg_color, bg_color, w, h, grid_width, dx, dy) {
    gui(cid)
    .get_gobj(tag)
    .append(function(frag) {
        var bg = create_item(cid, "rect", {
            width: w,
            height: h,
            fill: bg_color,
            class: "bg",
            stroke: "black",
            "stroke-width": grid_width
        }),
        path,
        path_string = "",
        fg_xy_path, // to be used for the foreground lines
        fg_mono_path,
        i, x, y, align_x, align_y;
        // Path strings for the grid lines
        // vertical lines...
        for (i = 0, x = dx; i < 7; i++, x += dx) {
            align_x = (x|0) === x ? x : Math.round(x);
            path_string += ["M", align_x, 0, "V", h].join(" ");
        }
        // horizontal lines...
        for (i = 0, y = dy; i < 3; i++, y += dy) {
            align_y = (y|0) === y ? y : Math.round(y);
            path_string += ["M", 0, align_y, "H", w].join(" ");
        }
        path = create_item(cid, "path", {
            d: path_string,
            fill: "none",
            stroke: "black",
            "stroke-width": grid_width,
        });
        // We go ahead and create a path to be used in the foreground. We'll
        // set the actual path data in the draw/redraw functions. Doing it this
        // way will save us having to create and destroy DOM objects each time
        // we redraw the foreground
        fg_xy_path = create_item(cid, "path", {
            fill: "none",
            stroke: fg_color,
            class: "fgxy"
        });
        fg_mono_path = create_item(cid, "path", {
            fill: "none",
            stroke: fg_color,
            class: "fgmono"
        });
        frag.appendChild(bg);
        frag.appendChild(path);
        frag.appendChild(fg_xy_path);
        frag.appendChild(fg_mono_path);
        return frag;
    });
}

function scope_configure_fg(cid, tag, type, data_array) {
    gui(cid)
        .get_gobj(tag)
        .q(type, { // class ".fgxy" or ".fgmono"
            d: data_array.join(" ")
    });
}

function gui_scope_configure_fg_xy(cid, tag, data_array) {
    scope_configure_fg(cid, tag, ".fgxy", data_array);
}

function gui_scope_configure_fg_mono(cid, tag, data_array) {
    scope_configure_fg(cid, tag, ".fgmono", data_array);
}

function gui_scope_configure_bg_color(cid, tag, color) {
    gui(cid).get_gobj(tag)
        .query(".bg", {
            fill: color
        });
}

function gui_scope_configure_fg_color(cid, tag, color) {
    gui(cid).get_gobj(tag)
        .q(".fgxy", { stroke: color })
        .q(".fgmono", { stroke: color });
}

function gui_scope_clear_fg(cid, tag) {
    scope_configure_fg(cid, tag, ".fgxy", []);
    scope_configure_fg(cid, tag, ".fgmono", []);
}

// unauthorized/grid

function get_grid_data(w, h, x_l, y_l) {
    var d, x, y, offset;
    d = [];
    offset = Math.floor(w / x_l);
    if (offset > 0) {
        for (x = 0; x < w; x += offset) {
            d = d.concat(["M", x, 0, x, h]); // vertical line
        }
    } else {
        post("Warning: too many gridlines");
    }
    offset = Math.floor(h / y_l);
    if (offset > 0) {
        for (y = 0; y < h; y += offset) {
            d = d.concat(["M", 0, y, w, y]); // horizontal line
        }
    } else {
        post("Warning: too many gridlines");
    }
    return d.join(" ");
}

function gui_configure_grid(cid, tag, w, h, bg_color, has_grid, x_l, y_l) {
    var grid_d_string = !!has_grid ? get_grid_data(w, h, x_l, y_l) : "",
        point_size = 5;
    gui(cid).get_gobj(tag)
    .q(".bg", {
        width: w,
        height: h,
        fill: bg_color,
    })
    .q(".border", {
        d: ["M", 0, 0, w, 0,
            "M", 0, h, w, h,
            "M", 0, 0, 0, h,
            "M", w, 0, w, h
           ].join(" "),
        fill: "none",
        stroke: "black",
        "stroke-width": 1
    })
    .q(".out_0", {
        y: h + 1,
        width: 7,
        height: 1,
        fill: "none",
        stroke: "black",
        "stroke-width": 1
    })
    .q(".out_1", {
        x: w - 7,
        y: h + 1,
        width: 7,
        height: 1,
        fill: "none",
        stroke: "black",
        "stroke-width": 1
    })
    .q(".grid", {
        d: grid_d_string,
        stroke: "white",
        "stroke-width": 1
    })
    .q(".point", {
        style: "visibility: none;",
        width: 5,
        height: 5,
        fill: "#ff0000",
        stroke: "black",
        "stroke-width": 1
    });
}

function gui_grid_new(cid, tag, x, y, is_toplevel) {
    gui(cid).get_elem("patchsvg", function(svg_elem) {
        gui_gobj_new(cid, tag, "obj", x, y, is_toplevel);
    });
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var bg = create_item(cid, "rect", {
            class: "bg"
        }),
        border = create_item(cid, "path", {
            class: "border" // now we can inherit the css border styles
        }),
        out_0 = create_item(cid, "rect", {
            class: "out_0",
            style: "display: " + (is_toplevel ? "inline;" : "none;")
        }),
        out_1 = create_item(cid, "rect", {
            class: "out_1",
            style: "display: " + (is_toplevel ? "inline;" : "none;")
        }),
        grid = create_item(cid, "path", {
            class: "grid"
        }),
        point = create_item(cid, "rect", {
            class: "point"
        });
        frag.appendChild(bg);
        frag.appendChild(out_0);
        frag.appendChild(out_1);
        frag.appendChild(grid);
        frag.appendChild(point);
        frag.appendChild(border);
        return frag;
    });
}


function gui_grid_point(cid, tag, x, y) {
    gui(cid).get_gobj(tag)
    .q(".point", {
        x: x,
        y: y,
        style: "visibility: visible;"
    });
}

// mknob from moonlib
function gui_mknob_new(cid, tag, x, y, is_toplevel, show_in, show_out) {
    gui(cid).get_elem("patchsvg", function(svg_elem) {
        gui_gobj_new(cid, tag, "obj", x, y, is_toplevel);
    });
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var border = create_item(cid, "path", {
            class: "border" // now we can inherit the css border styles
        }),
        circle = create_item(cid, "circle", {
            class: "circle"
        }),
        line = create_item(cid, "line", {
            class: "dial"
        });
        frag.appendChild(border);
        frag.appendChild(circle);
        frag.appendChild(line);
        return frag;
    });
}

function gui_configure_mknob(cid, tag, size, bg_color, fg_color) {
    gui(cid).get_gobj(tag)
    .q(".border", {
        d: ["M", 0, 0, size, 0,
            "M", 0, size, size, size,
            "M", 0, 0, 0, size,
            "M", size, 0, size, size
           ].join(" "),
        fill: "none",
    })
    .q(".circle", {
        cx: size / 2,
        cy: size / 2,
        r: size / 2,
        fill: bg_color,
        stroke: "black",
        "stroke-width": 1
    })
    .q(".dial", {
        "stroke-width": 2,
        stroke: fg_color
    });
}

function gui_turn_mknob(cid, tag, x1, y1, x2, y2) {
    gui(cid).get_gobj(tag)
    .q(".dial", {
        x1: x1,
        y1: y1,
        x2: x2,
        y2: y2
    });
}

function add_popup(cid, popup) {
    popup_menu[cid] = popup;
}

// envgen
function gui_envgen_draw_bg(cid, tag, bg_color, w, h, points_array) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var bg, border, pline;
        bg = create_item(cid, "rect", {
            width: w,
            height: h,
            fill: bg_color,
            stroke: "black",
            "stroke-width": "2",
            transform: "translate(0.5, 0.5)"
        });
        // draw an extra path so we can give envgen
        // a border class without affecting the
        // background color of envgen
        border = create_item(cid, "path", {
            "stroke-width": 1,
            d: ["M", 0, 0, w+1, 0,
                "M", w+1, 0, w+1, h+1,
                "M", w+1, h+1, 0, h+1,
                "M", 0, h+1, 0, 0].join(" "),
            "class": "border",
        });
        pline = create_item(cid, "polyline", {
            stroke: "black",
            fill: "none",
            transform: "translate(2, 2)",
            points: points_array.join(" ")
        });
        frag.appendChild(bg);
        frag.appendChild(border);
        frag.appendChild(pline);
        return frag;
    });
}

function gui_envgen_draw_doodle(cid, tag, cx, cy) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var d = create_item(cid, "circle", {
            r: "2",
            cx: cx + 2,
            cy: cy + 2
        });
        frag.appendChild(d);
        return frag;
    });
}

function gui_envgen_erase_doodles(cid, tag) {
    gui(cid).get_gobj(tag, function(e) {
        var elem_array = e.querySelectorAll("circle"),
        i;
        if (elem_array.length > 0) {
            for (i = 0; i < elem_array.length; i++) {
                elem_array[i].parentNode.removeChild(elem_array[i]);
            }
        }
    });
}

function gui_envgen_coords(cid, tag, w, h, points_array) {
    gui(cid).get_gobj(tag)
    .q(".border", {
        d: ["M", 0, 0, w+1, 0,
            "M", w+1, 0, w+1, h+1,
            "M", w+1, h+1, 0, h+1,
            "M", 0, h+1, 0, 0].join(" ")
    })
    .q(".rect", {
        width: w,
        height: h
    })
    .q("polyline", {
        points: points_array.join(" ")
    });
}

function gui_envgen_text(cid, tag, x, y, value, duration) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var svg_text = create_item(cid, "text", {
            transform: "translate(" + x + ")",
            y: y,
            "font-size": "12px"
        });
        text_to_tspans(cid, svg_text, value + "x" + duration);
        frag.appendChild(svg_text);
        return frag;
    });
}

function gui_envgen_erase_text(cid, tag) {
    gui(cid).get_gobj(tag)
    .q("text", function(svg_text) {
        svg_text.parentNode.removeChild(svg_text);
    });
}

function gui_envgen_move_xlet(cid, tag, type, i, x, y, basex, basey) {
    gui(cid).get_elem(tag + type + i, {
        x: x - basex,
        y: y - basey
    });
}

exports.add_popup = add_popup;

// Kludge to get popup coords to fit the browser's zoom level. As of v0.16.1
// it appears nw.js fixed the bug that required this kludge. The only versions
// affected then are
// a) Windows, which is pinned to version 0.14.7 to support XP and
// b) OSX 10.8 which requires 0.14.7 to run.
// So we do a version check for "0.14.7" to see whether the kludge is
// needed.
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
    // Get page coords for top of window, in case we're scrolled
    gui(cid).get_nw_window(function(nw_win) {
        var win_left = nw_win.window.document.body.scrollLeft,
            win_top = nw_win.window.document.body.scrollTop,
            zoom_level = nw_win.zoomLevel, // these were used to work
            zfactor,                       // around an old nw.js popup pos
                                           // bug. Now it's only necessary
                                           // on Windows, which uses v.0.14
            svg_view_box = nw_win.window.document.getElementById("patchsvg")
                .getAttribute("viewBox").split(" "); // need top-left svg origin

        // Check nw.js version-- if its lts then we need the zoom_kludge...
        zfactor = process.versions.nw === "0.14.7" ? zoom_kludge(zoom_level) : 1;
        // Set the global popup x/y so they can be retrieved by the relevant
        // document's event handler
        popup_coords[0] = xpos;
        popup_coords[1] = ypos;
        //popup_coords[0] = xpos;
        //popup_coords[1] = ypos;
        popup_menu[cid].items[0].enabled = canprop;
        popup_menu[cid].items[1].enabled = canopen;

        // We'll use "isobject" to enable/disable "To Front" and "To Back"
        //isobject;

        // We need to round win_left and win_top because the popup menu
        // interface expects an int. Otherwise the popup position gets wonky
        // when you zoom and scroll...
        xpos = Math.floor(xpos * zfactor) - Math.floor(win_left * zfactor);
        ypos = Math.floor(ypos * zfactor) - Math.floor(win_top * zfactor);

        // Now subtract the x and y offset for the top left corner of the svg.
        // We need to do this because a Pd canvas can have objects with negative
        // coordinates. Thus the SVG viewbox will have negative values for the
        // top left corner, and those must be subtracted from xpos/ypos to get
        // the proper window coordinates.
        xpos -= Math.floor(svg_view_box[0] * zfactor);
        ypos -= Math.floor(svg_view_box[1] * zfactor);

        popup_coords[2] = xpos + nw_win.x;
        popup_coords[3] = ypos + nw_win.y;

        popup_menu[cid].popup(xpos, ypos);
    });
}

function popup_action(cid, index) {
    pdsend(cid, "done-popup", index, popup_coords[0], popup_coords[1]);
}

exports.popup_action = popup_action;

// Graphs and Arrays

// Doesn't look like we needs this

//function gui_graph_drawborder(cid, tag, x1, y1, x2, y2) {
//    var g = get_gobj(cid, tag);
//    var b = create_item(cid, "rect", {
//        width: x2 - x1,
//        height: y2 - y1,
//        stroke: "black",
//        fill: "none",
//        id: tag
//    });
//    g.appendChild(b);
//}

// This sets a GOP subpatch or graph to be "greyed out" when the user
// has opened it to inspect its contents.  (I.e., it has its own window.)
// We never actually remove this tag-- instead we just assume that the
// GOP will get erased and redrawn when its time to show the contents
// again.
function gui_graph_fill_border(cid, tag) {
    gui(cid).get_gobj(tag, function(e) {
        e.classList.add("has_window");
    });
}

function gui_graph_deleteborder(cid, tag) {
    gui(cid).get_elem(tag, function(e) {
        e.parentNode.removeChild(b);
    });
}

function gui_graph_label(cid, tag, label_number, font_height, array_name,
    font, font_size, font_weight, is_selected) {
    gui(cid).get_elem("patchsvg", function(e) {
        var y = font_height * label_number * -1;
        gui_text_new(cid, tag, "graph_label", 0, 0, y, array_name, font_size);
    });
}

function gui_graph_vtick(cid, tag, x, up_y, down_y, tick_pix, basex, basey) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var up_tick,
            down_tick;
        // Don't think these need an ID...
        up_tick = create_item(cid, "line", {
            stroke: "black",
            x1: x - basex,
            y1: up_y - basey,
            x2: x - basex,
            y2: up_y - tick_pix - basey
        });
        down_tick = create_item(cid, "line", {
            stroke: "black",
            x1: x - basex,
            y1: down_y - basey,
            x2: x - basex,
            y2: down_y + tick_pix - basey
        });
        frag.appendChild(up_tick);
        frag.appendChild(down_tick);
        return frag;
    });
}

function gui_graph_htick(cid, tag, y, r_x, l_x, tick_pix, basex, basey) {
    gui(cid).get_gobj(tag)
    .append(function(frag) {
        var left_tick,
            right_tick;
        // Don't think these need an ID...
        left_tick = create_item(cid, "line", {
            stroke: "black",
            x1: l_x - basex,
            y1: y - basey,
            x2: l_x - tick_pix - basex,
            y2: y - basey,
            id: "tick" + y
        });
        right_tick = create_item(cid, "line", {
            stroke: "black",
            x1: r_x - basex,
            y1: y - basey,
            x2: r_x + tick_pix - basex,
            y2: y - basey
        });
        frag.appendChild(left_tick);
        frag.appendChild(right_tick);
        return frag;
    });
}

function gui_graph_tick_label(cid, tag, x, y, text, font, font_size, font_weight, basex, basey, tk_label_anchor) {
    gui(cid).get_gobj(tag)
    .append(function(frag, w) {
        var svg_text, text_node, text_anchor, alignment_baseline;
        // We use anchor identifiers from the tk toolkit:
        //
        // "n" for north, or aligned at the top of the text
        // "s" for south, or default baseline alignment
        // "e" for east, or text-anchor at the end of the text
        // "w" for west, or default text-anchor for left-to-right languages
        //
        // For x labels the tk_label_anchor will either be "n" for labels at the
        // bottom of the graph, or "s" for labels at the top of the graph
        //
        // For y labels the tk_label_anchor will either be "e" for labels at the
        // right of the graph, or "w" for labels at the right.
        //
        // In each case we want the label to be centered around the tick mark.
        // So we default to value "middle" if we didn't get a value for that
        // axis.
        text_anchor = tk_label_anchor === "e" ? "end" :
            tk_label_anchor === "w" ? "start" : "middle";
        alignment_baseline = tk_label_anchor === "n" ? "hanging" :
            tk_label_anchor === "s" ? "auto" : "middle";
        svg_text = create_item(cid, "text", {
            // need a label "y" relative to baseline
            x: x - basex,
            y: y - basey,
            "text-anchor": text_anchor,
            "alignment-baseline": alignment_baseline,
            "font-size": pd_fontsize_to_gui_fontsize(font_size) + "px",
        });
        text_node = w.document.createTextNode(text);
        svg_text.appendChild(text_node);
        frag.appendChild(svg_text);
        return frag;
    });
}

function gui_canvas_drawredrect(cid, x1, y1, x2, y2) {
    gui(cid).get_elem("patchsvg", function(svg_elem) {
        var g = gui_gobj_new(cid, cid, "gop_rect", x1, y1, 1);
        var r = create_item(cid, "rect", {
            width: x2 - x1,
            height: y2 - y1,
            id: "gop_rect"
        });
        g.appendChild(r);
        svg_elem.appendChild(g);
    });
}

function gui_canvas_deleteredrect(cid) {
    // We need to check for existence here, because the first
    // time setting GOP in properties, there is no red rect yet.
    // But setting properties when the subpatch's window is
    // visible calls glist_redraw, and glist_redraw will try to delete
    // the red rect _before_ it's been drawn in this case.
    // Unfortunately, it's quite difficult to refactor those c
    // functions without knowing the side effects.  But ineffectual
    // gui calls should really be minimized-- otherwise it's simply
    // too difficult to debug what's being passed over the socket.
    gui(cid).get_gobj(cid, function(e) {
        e.parentNode.removeChild(e);
    });
}

function gui_canvas_redrect_coords(cid, x1, y1, x2, y2) {
    gui(cid).get_gobj(cid, function(e) {
        elem_move(e, x1, y1);
    })
    .get_elem("gop_rect", {
        width: x2 - x1,
        height: y2 - y1
    });
}

//  Cord Inspector (a.k.a. Magic Glass)

// For clarity, this probably shouldn't be a gobj.  Also, it might be easier to
// make it a div that lives on top of the patchsvg
function gui_cord_inspector_new(cid, font_size) {
    var g = get_gobj(cid, "cord_inspector"),
        ci_rect = create_item(cid, "rect", { id: "cord_inspector_rect" }),
        ci_poly = create_item(cid, "polygon", { id: "cord_inspector_polygon" }),
        ci_text = create_item(cid, "text", {
            id: "cord_inspector_text",
            "font-size": pd_fontsize_to_gui_fontsize(font_size) + "px",
        }),
        text_node = patchwin[cid].window.document.createTextNode("");
    ci_text.appendChild(text_node);
    g.appendChild(ci_rect);
    g.appendChild(ci_poly);
    g.appendChild(ci_text);
}

function gui_cord_inspector_update(cid, text, basex, basey, bg_size, y1, y2, moved) {
    var gobj = get_gobj(cid, "cord_inspector"),
        rect = get_item(cid, "cord_inspector_rect"),
        poly = get_item(cid, "cord_inspector_polygon"),
        svg_text = get_item(cid, "cord_inspector_text"),
        polypoints_array;
    gobj.setAttributeNS(null, "transform",
            "translate(" + (basex + 10.5) + "," + (basey + 0.5) + ")");
    gobj.setAttributeNS(null, "pointer-events", "none");
    gobj.classList.remove("flash");
    // Lots of fudge factors here, tailored to the current default font size
    configure_item(rect, {
        x: 13,
        y: y1 - basey,
        width: bg_size - basex,
        height: y2 - basey + 10
    });
    polypoints_array = [8,0,13,5,13,-5];
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

function gui_cord_inspector_erase(cid) {
    gui(cid).get_gobj("cord_inspector", function(e) {
        e.parentNode.removeChild(e);
    });
}

function gui_cord_inspector_flash(cid, state) {
    gui(cid).get_elem("cord_inspector_text", function(e) {
        if (state === 1) {
            e.classList.add("flash");
        } else {
            e.classList.remove("flash");
        }
    });
}

// Window functions

function gui_raise_window(cid) {
    // Check if the window exists, for edge cases like
    // [vis 1, vis1(---[send this_canvas]
    gui(cid).get_nw_window(function(nw_win) {
        nw_win.focus();
    });
}

// Unfortunately DOM window.focus doesn't actually focus the window, so we
// have to use the chrome API
function gui_raise_pd_window() {
    chrome.windows.getAll(function (w_array) {
        chrome.windows.update(w_array[0].id, { focused: true });
    });
}

// Using the chrome app API, because nw.js doesn't seem
// to let me get a list of the Windows
function walk_window_list(cid, offset) {
    chrome.windows.getAll(function (w_array) {
        chrome.windows.getLastFocused(function (w) {
            var i, next, match = -1;
            for (i = 0; i < w_array.length; i++) {
                if (w_array[i].id === w.id) {
                    match = i;
                    break;
                }
            }
            if (match !== -1) {
                next = (((match + offset) % w_array.length) // modulo...
                        + w_array.length) % w_array.length; // handle negatives
                chrome.windows.update(w_array[next].id, { focused: true });
            } else {
                post("error: cannot find last focused window.");
            }
        });
    })
}

function raise_next(cid) {
    walk_window_list(cid, 1);
}

exports.raise_next = raise_next;

function raise_prev(cid) {
    walk_window_list(cid, -1);
}

exports.raise_prev = raise_prev;

exports.raise_pd_window= gui_raise_pd_window;

// Openpanel and Savepanel

var file_dialog_target;

function file_dialog(cid, type, target, start_path) {
    file_dialog_target = target;
    var query_string = (type === "open" ?
                        "openpanelSpan" : "savepanelSpan"),
        input_span,
        input_elem,
        input_string,
        dialog_options,
        win;
    // We try opening the dialog in the last focused window. There's an
    // edge case where [loadbang]--[openpanel] will trigger before the
    // window has finished loading. In that case we just trigger the
    // dialog in the main Pd window.
    win = last_focused && patchwin[last_focused] ? patchwin[last_focused] :
        pd_window;
    input_span = win.window.document.querySelector("#" + query_string);
    // We have to use an absolute path here because of a bug in nw.js 0.14.7
    if (!path.isAbsolute(start_path)) {
        start_path = path.join(pwd, start_path);
    }
    // We also have to inject html into the dom because of a bug in nw.js
    // 0.14.7. For some reason we can't just change the value of nwworkingdir--
    // it just doesn't work. So this requires us to have the parent <span>
    // around the <input>. Then when we change the innerHTML of the span the
    // new value for nwworkingdir magically works.
    dialog_options = {
        style: "display: none;",
        type: "file",
        id: type === "open" ? "openpanel_dialog" : "savepanel_dialog",
        // using an absolute path here, see comment above
        nwworkingdir: start_path
    };
    if (type !== "open") {
        dialog_options.nwsaveas = "";
    }
    input_string = build_file_dialog_string(dialog_options);
    input_span.innerHTML = input_string;
    // Now that we've rebuilt the input element, let's get a reference to it...
    input_elem = win.window.document.querySelector("#" +
        (type === "open" ? "openpanel_dialog" : "savepanel_dialog"));
    // And add an event handler for the callback
    input_elem.onchange = function() {
        // reset value so that we can open the same file twice
        file_dialog_callback(this.value);
        this.value = null;
        console.log("openpanel/savepanel called");
    };
    win.window.setTimeout(function() {
        input_elem.click(); },
        300
    );
}

function gui_openpanel(cid, target, path) {
    file_dialog(cid, "open", target, path);
}

function gui_savepanel(cid, target, path) {
    file_dialog(cid, "save", target, path);
}

function file_dialog_callback(file_string) {
    pdsend(file_dialog_target, "callback",
        enquote(defunkify_windows_path(file_string)));
}

exports.file_dialog_callback = file_dialog_callback;

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
    dialogwin[did] = create_window(did, "gatom", 265, 300,
        popup_coords[2], popup_coords[3],
        attr_array_to_object(attr_array));
}

function gui_gatom_activate(cid, tag, state) {
    gui(cid).get_gobj(tag, function(e) {
        if (state !== 0) {
            e.classList.add("activated");
        } else {
            e.classList.remove("activated");
        }
    });
}

function gui_dropdown_dialog(did, attr_array) {
    // Just reuse the "gatom" dialog
    dialogwin[did] = create_window(did, "gatom", 265, 300,
        popup_coords[2], popup_coords[3],
        attr_array_to_object(attr_array));
}

function dropdown_populate(w, label_array, current_index) {
    var ol = w.document.querySelector("#dropdown_list ol");
    // clear it out
    ol.innerHTML = '';
    label_array.forEach(function(text, i) {
        var li = w.document.createElement("li");
        li.textContent = text;
        li.setAttribute("data-index", i);
        if (i === current_index) {
            li.classList.add("highlighted");
        }
        ol.appendChild(li);
    });
}

function gui_dropdown_activate(cid, obj_tag, tag, current_index, font_size, state, label_array) {
    var g, select_elem, svg_view, g_bbox,
        doc_height,    // document height, excluding the scrollbar
        menu_height, // height of the list of elements inside the div
        div_y,       // position of div containing the dropdown menu
        div_max,     // max height of the div
        scroll_y,
        offset_anchor; // top or bottom
    // Annoying: obj_tag is just the "x"-prepended hex value for the object,
    // and tag is the one from rtext_gettag that is used as our gobj id
    gui(cid).get_elem("patchsvg", function(svg_elem, w) {
        g = get_gobj(cid, tag);
        if (state !== 0) {
            svg_view = svg_elem.viewBox.baseVal;
            select_elem = w.document.querySelector("#dropdown_list");
            dropdown_populate(w, label_array, current_index);
            // stick the obj_tag in a data field
            select_elem.setAttribute("data-callback", obj_tag);
            // display the menu so we can measure it

            g_bbox = g.getBoundingClientRect();
            // Measuring the document height is tricky-- the following
            // method is the only reliable one I've found. And even here,
            // if you display the select_elem as inline _before_ measuring
            // the doc height, the result ends up being _smaller_. No idea.
            doc_height = w.document.documentElement.clientHeight;
            // Now let's display the select_elem div so we can measure it
            select_elem.style.setProperty("display", "inline");
            menu_height = select_elem.querySelector("ol")
                .getBoundingClientRect().height;
            scroll_y = w.scrollY;
            // If the area below the object is smaller than 75px, then
            // display the menu above the object.
            // If the entire menu won't fit below the object but _will_
            // fit above it, display it above the object.
            // If the menu needs a scrollbar, display it below the object
            if (doc_height - g_bbox.bottom <= 75
                || (menu_height > doc_height - g_bbox.bottom
                    && menu_height <= g_bbox.top)) {
                // menu on top
                offset_anchor = "bottom";
                div_max = g_bbox.top - 2;
                div_y = doc_height - (g_bbox.top + scroll_y);
            }
            else {
                // menu on bottom (possibly with scrollbar)
                offset_anchor = "top";
                div_max = doc_height - g_bbox.bottom - 2;
                div_y = g_bbox.bottom + scroll_y;
            }
            // set a max-height to force scrollbar if needed
            select_elem.style.setProperty("max-height", div_max + "px");
            select_elem.style.setProperty("left",
                (elem_get_coords(g).x - svg_view.x) + "px");
            // Remove "top" and "bottom" props to keep state clean
            select_elem.style.removeProperty("top");
            select_elem.style.removeProperty("bottom");
            // Now position the div relative to either the "top" or "bottom"
            select_elem.style.setProperty(offset_anchor, div_y + "px");
            select_elem.style.setProperty("font-size",
                pd_fontsize_to_gui_fontsize(font_size) + "px");
            select_elem.style.setProperty("min-width", g.getBBox().width + "px");
            w.canvas_events.dropdown_menu();
        } else {
            post("deactivating dropdown menu");
            // Probably want to send this
            pdsend(cid, "key 0 Control 0 1 0");
        }
    });
}

function gui_iemgui_dialog(did, attr_array) {
    //for (var i = 0; i < attr_array.length; i++) {
    //    attr_array[i] = '"' + attr_array[i] + '"';
    //}
    create_window(did, "iemgui", 265, 450,
        popup_coords[2], popup_coords[3],
        attr_array_to_object(attr_array));
}

function gui_dialog_set_field(did, field_name, value) {
    var elem = dialogwin[did].window.document.getElementsByName(field_name)[0];
    elem.value = value;
    dialogwin[did].window.update_attr(elem);
}

// Used when undoing a font size change when the font dialog is open
function gui_font_dialog_change_size(did, font_size) {
    var button;
    if (dialogwin[did]) {
        button = dialogwin[did].window.document.getElementById(font_size);
        button.click();
    } else {
        post("error: no font dialogwin!");
    }
}

function gui_array_new(did, count) {
    var attr_array = [{
        array_gfxstub: did,
        array_name: "array" + count,
        array_size: 100,
        array_flags: 3,
        array_fill: "black",
        array_outline: "black",
        array_in_existing_graph: 0
    }];
    dialogwin[did] = create_window(did, "canvas", 265, 340, 20, 20,
        attr_array);
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
    dialogwin[did] = create_window(did, "canvas", 300, 100,
        popup_coords[2], popup_coords[3],
        attr_arrays);
}

function gui_data_dialog(did, data_string) {
    dialogwin[did] = create_window(did, "data", 250, 300,
        popup_coords[2], popup_coords[3],
        data_string);
}

function gui_text_dialog_clear(did) {
    if (dialogwin[did]) {
        dialogwin[did].window.textarea_clear();
    }
}

function gui_text_dialog_append(did, line) {
    if (dialogwin[did]) {
        dialogwin[did].window.textarea_append(line);
    }
}

function gui_text_dialog_set_dirty(did, state) {
    if (dialogwin[did]) {
        dialogwin[did].window.set_dirty(state !== 0);
    }
}

function gui_text_dialog(did, width, height, font_size) {
    dialogwin[did] = create_window(did, "text", width, height,
        popup_coords[2], popup_coords[3],
        font_size);
}

function gui_text_dialog_raise(did) {
    if (dialogwin[did]) {
        dialogwin[did].focus();
    }
}

function gui_text_dialog_close_from_pd(did, signoff) {
    if (dialogwin[did]) {
        dialogwin[did].window.close_from_pd(signoff !== 0);
    }
}

function gui_remove_gfxstub(did) {
    if (dialogwin[did] !== undefined && dialogwin[did] !== null) {
        dialogwin[did].close(true);
        dialogwin[did] = null;
    }
}

function gui_font_dialog(cid, gfxstub, font_size) {
    var attrs = { canvas: cid, font_size: font_size };
    dialogwin[gfxstub] = create_window(gfxstub, "font", 265, 200, 0, 0,
        attrs);
}

function gui_external_dialog(did, external_name, attr_array) {
    create_window(did, "external", 265, 450,
        popup_coords[2], popup_coords[3],
        {
            name: external_name,
            attributes: attr_array
        });
}

// Global settings

function gui_pd_dsp(state) {
    if (pd_window !== undefined) {
        pd_window.document.getElementById("dsp_control").checked = !!state;
    }
}

function open_prefs() {
    if (!dialogwin["prefs"]) {
        create_window("prefs", "prefs", 370, 470, 0, 0, null);
    }
}

exports.open_prefs = open_prefs;

function open_search() {
    if (!dialogwin["search"]) {
        create_window("search", "search", 300, 400, 20, 20, null);
    }
}

exports.open_search= open_search;

// This is the same for all windows (initialization is in pd_menus.js).
var recent_files_submenu = null;
var recent_files = null;

// We need to jump through some hoops here since JS closures capture variables
// by reference, which causes trouble when closures are created within a
// loop.
function recent_files_callback(i) {
    return function() {
        var fname = recent_files[i];
        //post("clicked recent file: "+fname);
        open_file(fname);
    }
}

function populate_recent_files(submenu) {
    if (submenu) recent_files_submenu = submenu;
    if (recent_files && recent_files_submenu) {
        //post("recent files: " + recent_files.join(" "));
        while (recent_files_submenu.items.length > 0)
            recent_files_submenu.removeAt(0);
        for (var i = 0; i < recent_files.length; i++) {
            var item = new nw.MenuItem({
                label: path.basename(recent_files[i]),
                tooltip: recent_files[i]
            });
            item.click = recent_files_callback(i);
            recent_files_submenu.append(item);
        }
        if (recent_files_submenu.items.length > 0) {
            recent_files_submenu.append(new nw.MenuItem({
                type: "separator"
            }));
            var item = new nw.MenuItem({
                label: lang.get_local_string("menu.clear_recent_files"),
                tooltip: lang.get_local_string("menu.clear_recent_files_tt")
            });
            item.click = function() {
                pdsend("pd clear-recent-files");
            };
            recent_files_submenu.append(item);
        }
    }
}

exports.populate_recent_files = populate_recent_files;

function gui_recent_files(dummy, recent_files_array) {
    recent_files = recent_files_array;
    populate_recent_files(recent_files_submenu);
}

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
    //    post("arg " + i + " is " + arguments[i]);
    //}
    if (dialogwin["prefs"] !== null) {
        dialogwin["prefs"].eval(null,
            "audio_prefs_callback("  +
            JSON.stringify(attrs) + ");"
        );
    }
}

function gui_midi_properties(gfxstub, sys_indevs, sys_outdevs,
    pd_indevs, pd_outdevs, midi_attrs) {
    var attrs = midi_attrs.concat([
        "midi-apis", pd_midiapilist,
        "midi-indev-names", sys_indevs,
        "midi-outdev-names", sys_outdevs,
        "pd-indevs", pd_indevs,
        "pd-outdevs", pd_outdevs,
        ]);
    //post("got back some midi props...");
    //for (var i = 0; i < arguments.length; i++) {
    //    post("arg " + i + " is " + arguments[i]);
    //}
    if (dialogwin["prefs"] !== null) {
        dialogwin["prefs"].eval(null,
            "midi_prefs_callback("  +
            JSON.stringify(attrs) + ");"
        );
    }
}

function gui_gui_properties(dummy, name, save_zoom, browser_doc, browser_path, browser_init) {
    if (dialogwin["prefs"] !== null) {
        dialogwin["prefs"].window.gui_prefs_callback(name, save_zoom, browser_doc, browser_path, browser_init);
    }
}

function gui_path_properties(dummy, use_stdpath, verbose, path_array) {
    if (dialogwin["prefs"] !== null) {
        dialogwin["prefs"].window.path_prefs_callback(use_stdpath, verbose, path_array);
    }
}

function gui_lib_properties(dummy, defeat_rt, flag_string, lib_array) {
    if (dialogwin["prefs"] !== null) {
        dialogwin["prefs"].window.lib_prefs_callback(defeat_rt, flag_string, lib_array);
    }
}

// Let's try a closure for gui skins
var skin = exports.skin = (function () {
    var dir = "css/";
    var preset = "default";
    var id;
    function set_css(win) {
        win.document.getElementById("page_style")
            .setAttribute("href", dir + preset + ".css");
    }
    return {
        get: function () {
            post("getting preset: " + dir + preset + ".css");
            return dir + preset + ".css";
        },
        set: function (name) {
            preset = name;
            for (id in patchwin) {
                if (patchwin.hasOwnProperty(id) && patchwin[id]) {
                    set_css(patchwin[id].window);
                }
            }
            // hack for the console
            pd_window.document.getElementById("page_style")
                .setAttribute("href", dir + preset + ".css");
        },
        apply: function (win) {
            set_css(win);
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
        win.getSelection().removeAllRanges();
        win.getSelection().addRange(range);
    }
}

// CSS: Cleanly separate style from content.
// Me: Ahhhh!
// Arnold: Get down!
// Me: Wat?
// CSS: Impossible...
// Arnold: Style this. *kappakappakappa*
// Me: Hey, you can't just go around killing people!
// Arnold: It's not human. It's a W3C Standard.
// Me: But how did it get here?
// Arnold: It travelled from the past.
// Me: What does it want?
// Arnold: It won't stop until your energy is completely eliminated.
// Me: What now?
// Arnold: Use this to find what you need. Then get the heck out of there!
function get_style_by_selector(w, selector) {
    var sheet_list = w.document.styleSheets,
        rule_list, i, j,
        len = sheet_list.length;
    for (i = 0; i < len; i++) {
        rule_list = sheet_list[i].cssRules;
        for (j = 0; j < rule_list.length; j++) {
            if (rule_list[j].type == w.CSSRule.STYLE_RULE &&
                rule_list[j].selectorText == selector) {
                return rule_list[j].style;
            }
        }
    }
    return null;
}

// for debugging purposes
exports.get_style_by_selector = get_style_by_selector;

// Big, stupid, ugly SVG data url to shove into CSS when
// the user clicks a box in edit mode. One set of points for
// the "head", or main box, and the other for the "tail", or
// message flag at the right.
function generate_msg_box_bg_data(type, stroke) {
   return 'url(\"data:image/svg+xml;utf8,' +
            '<svg ' +
              "xmlns:svg='http://www.w3.org/2000/svg' " +
              "xmlns='http://www.w3.org/2000/svg' " +
              "xmlns:xlink='http://www.w3.org/1999/xlink' " +
              "version='1.0' " +
              "viewBox='0 0 10 10' " +
              "preserveAspectRatio='none'" +
            ">" +
              "<polyline vector-effect='non-scaling-stroke' " +
                "id='bubbles' " +
                "fill='none' " +
                "stroke=' " +
                  stroke + // Here's our stroke color
                "' " +
                "stroke-width='1' " +
                (type === "head" ?
                    "points='10 0 0 0 0 10 10 10' " : // box
                    "points='0 0 10 0 0 2 0 8 10 10 0 10' ") + // flag
              "/>" +
            "</svg>" +
          '")';
}

// Big problem here-- CSS fails miserably at something as simple as the
// message box flag. We use a backgroundImage svg to address this, but
// for security reasons HTML5 doesn't provide access to svg image styles.
// As a workaround we just seek out the relevant CSS rules and shove the
// whole svg data url into them. We do this each time the user
// clicks a box to edit.
// Also, notice that both CSS and SVG _still_ fail miserably at drawing a
// message box flag that expands in the middle while retaining the same angles
// at the edges. As the message spans more and more lines the ugliness becomes
// more and more apparent.
// Anyhow, this enormous workaround makes it possible to just specify the
// edit box color in CSS for the presets.
function shove_svg_background_data_into_css(w) {
    var head_style = get_style_by_selector(w, "#new_object_textentry.msg"),
        tail_style = get_style_by_selector(w, "p.msg::after"),
        stroke = head_style.outlineColor;
    head_style.backgroundImage = generate_msg_box_bg_data("head", stroke);
    tail_style.backgroundImage = generate_msg_box_bg_data("tail", stroke);
}

function gui_textarea(cid, tag, type, x, y, width_spec, height_spec, text,
    font_size, is_gop, state) {
    var range, svg_view, p,
        gobj = get_gobj(cid, tag);
    if (state !== 0) {
        // Make sure we're in editmode
        canvas_set_editmode(cid, 1);
        // Hide the gobj while we edit.  However, we want the gobj to
        // contribute to the svg's bbox-- that way when the new_object_textentry
        // goes away we still have the same dimensions.  Otherwise the user
        // can get strange jumps in the viewport when instantiating an object
        // at the extremities of the patch.
        // To solve this, we use 'visibility' instead of 'display', since it
        // still uses the hidden item when calculating the bbox.
        // (We can probably solve this problem by throwing in yet another
        // gui_canvas_get_scroll, but this seems like the right way to go
        // anyway.)
        configure_item(gobj, { visibility: "hidden" });
        p = patchwin[cid].window.document.createElement("p");
        configure_item(p, {
            id: "new_object_textentry"
        });
        svg_view = patchwin[cid].window.document.getElementById("patchsvg")
            .viewBox.baseVal;
        p.classList.add(type);
        p.contentEditable = "true";
        p.style.setProperty("left", (x - svg_view.x) + "px");
        p.style.setProperty("top", (y - svg_view.y) + "px");
        p.style.setProperty("font-size",
            pd_fontsize_to_gui_fontsize(font_size) + "px");
        p.style.setProperty("line-height",
            text_line_height_kludge(font_size, "pd") + "px");
        p.style.setProperty("transform", "translate(0px, 0px)");
        p.style.setProperty("max-width",
            width_spec !== 0 ? width_spec + "ch" : "60ch");
        p.style.setProperty("min-width",
            width_spec <= 0 ? "3ch" :
                (is_gop == 1 ? width_spec + "px" :
                    width_spec + "ch"));
        if (is_gop == 1) {
            p.style.setProperty("min-height", height_spec + "px");
        }
        // set backgroundimage for message box
        if (type === "msg") {
            shove_svg_background_data_into_css(patchwin[cid].window);
        }
        // remove leading/trailing whitespace
        text = text.trim();
        p.textContent = text;
        // append to doc body
        patchwin[cid].window.document.body.appendChild(p);
        p.focus();
        select_text(cid, p);
        if (state === 1) {
            patchwin[cid].window.canvas_events.text();
        } else {
            patchwin[cid].window.canvas_events.floating_text();
        }
    } else {
        configure_item(gobj, { visibility: "normal" });
        p = patchwin[cid].window.document.getElementById("new_object_textentry");
        if (p !== null) {
            p.parentNode.removeChild(p);
        }
        if (patchwin[cid].window.canvas_events.get_previous_state() ===
               "search") {
            patchwin[cid].window.canvas_events.search();
        } else {
            patchwin[cid].window.canvas_events.normal();
        }
    }
}

function gui_undo_menu(cid, undo_text, redo_text) {
    // we have to check if the window exists, because Pd starts
    // up with two unvis'd patch windows used for garrays. Plus
    // there may be some calls to subpatches after updating a dialog
    // (like turning on GOP) which call this for a canvas that has
    // been destroyed.
    gui(cid).get_nw_window(function(nw_win) {
        if (cid !== "nobody") {
            nw_win.window.nw_undo_menu(undo_text, redo_text);
        }
    });
}

// leverages the get_nw_window method in the callers...
function canvas_params(nw_win)
{
    // calculate the canvas parameters (svg bounding box and window geometry)
    // for do_getscroll and do_optimalzoom
    var bbox, width, height, min_width, min_height, x, y, svg_elem;
    svg_elem = nw_win.window.document.getElementById("patchsvg");
    bbox = svg_elem.getBBox();
    // We try to do Pd-extended style canvas origins. That is, coord (0, 0)
    // should be in the top-left corner unless there are objects with a
    // negative x or y.
    // To implement the Pd-l2ork behavior, the top-left of the canvas should
    // always be the topmost, leftmost object.
    width = bbox.x > 0 ? bbox.x + bbox.width : bbox.width;
    height = bbox.y > 0 ? bbox.y + bbox.height : bbox.height;
    x = bbox.x > 0 ? 0 : bbox.x,
    y = bbox.y > 0 ? 0 : bbox.y;

    // The svg "overflow" attribute on an <svg> seems to be buggy-- for example,
    // you can't trigger a mouseover event for a <rect> that is outside of the
    // explicit bounds of the svg.
    // To deal with this, we want to set the svg width/height to always be
    // at least as large as the browser's viewport. There are a few ways to
    // do this this, like documentElement.clientWidth, but window.innerWidth
    // seems to give the best results.
    // However, there is either a bug or some strange behavior regarding
    // the viewport size: setting both the height and width of an <svg> to
    // the viewport height/width will display the scrollbars. The height or
    // width must be set to 4 less than the viewport size in order to keep
    // the scrollbars from appearing. Here, we just subtract 4 from both
    // of them. This could lead to some problems with event handlers but I
    // haven't had a problem with it yet.
    min_width = nw_win.window.innerWidth - 4;
    min_height = nw_win.window.innerHeight - 4;
    // Since we don't do any transformations on the patchsvg,
    // let's try just using ints for the height/width/viewBox
    // to keep things simple.
    width |= 0; // drop everything to the right of the decimal point
    height |= 0;
    min_width |= 0;
    min_height |= 0;
    x |= 0;
    y |= 0;
    return { x: x, y: y, w: width, h: height,
             mw: min_width, mh: min_height };
}

function do_getscroll(cid) {
    // Since we're throttling these getscroll calls, they can happen after
    // the patch has been closed. We remove the cid from the patchwin
    // object on close, so we can just check to see if our Window object has
    // been set to null, and if so just return.
    // This is an awfully bad pattern. The whole scroll-checking mechanism
    // needs to be rethought, but in the meantime this should prevent any
    // errors wrt the rendering context disappearing.
    gui(cid).get_nw_window(function(nw_win) {
        var svg_elem = nw_win.window.document.getElementById("patchsvg");
        var { x: x, y: y, w: width, h: height,
            mw: min_width, mh: min_height } = canvas_params(nw_win);
        if (width < min_width) {
            width = min_width;
        }
        // If the svg extends beyond the viewport, it might be nice to pad
        // both the height/width and the x/y coords so that there is extra
        // room for making connections and manipulating the objects.  As it
        // stands objects will be flush with the scrollbars and window
        // edges.
        if (height < min_height) {
            height = min_height;
        }
        configure_item(svg_elem, {
            viewBox: [x, y, width, height].join(" "),
            width: width,
            height: height
        });
    });
}

var getscroll_var = {};

// We use a setTimeout here for two reasons:
// 1. nw.js has a nasty Renderer bug  when you try to modify the
//    window before the document has finished loading. To get
//    the error get rid of the setTimeout
// 2. This should protect the user from triggering a bunch of
//    layouts.  But this only works because I'm not updating
//    the view to follow the mouse-- for example, when
//    the user is dragging an object beyond the bounds of the
//    viewport. The tcl/tk version actually does follow the
//    mouse. In that case this setTimeout could keep the
//    graphics from displaying until the user releases the mouse,
//    which would be a buggy UI
function gui_canvas_get_scroll(cid) {
    if (!getscroll_var[cid]) {
        getscroll_var[cid] = setTimeout(function() {
            do_getscroll(cid);
            getscroll_var[cid] = null;
        }, 250);
    }
}

exports.gui_canvas_get_scroll = gui_canvas_get_scroll;

function do_optimalzoom(cid, hflag, vflag) {
    // determine an optimal zoom level that makes the entire patch fit within
    // the window
    gui(cid).get_nw_window(function(nw_win) {
        var { x: x, y: y, w: width, h: height, mw: min_width, mh: min_height } =
            canvas_params(nw_win);
        // Calculate the optimal horizontal and vertical zoom values,
        // using floor to always round down to the nearest integer. Note
        // that these may well be negative, if the viewport is too small
        // for the patch at the current zoom level. XXXREVIEW: We assume a
        // zoom factor of 1.2 here; this works for me on Linux, but I'm
        // not sure how portable it is. -ag
        var zx = 0, zy = 0;
        if (width>0) zx = Math.floor(Math.log(min_width/width)/Math.log(1.2));
        if (height>0) zy = Math.floor(Math.log(min_height/height)/Math.log(1.2));
        // Optimal zoom is the minimum of the horizontal and/or the vertical
        // zoom values, depending on the h and v flags. This gives us the offset
        // to the current zoom level. We then need to clamp the resulting new
        // zoom level to the valid zoom level range of -8..+7.
        var actz = nw_win.zoomLevel, z = 0;
        if (hflag && vflag)
            z = Math.min(zx, zy);
        else if (hflag)
            z = zx;
        else if (vflag)
            z = zy;
        z += actz;
        if (z < -8) z = -8; if (z > 7) z = 7;
        //post("bbox: "+width+"x"+height+"+"+x+"+"+y+" window size: "+min_width+"x"+min_height+" current zoom level: "+actz+" optimal zoom level: "+z);
        if (z != actz) {
            nw_win.zoomLevel = z;
            pdsend(cid, "zoom", z);
        }
    });
}

var optimalzoom_var = {};

// We use a setTimeout here as with do_getscroll above, but we have to
// use a smaller value here, so that we're done before a subsequent
// call to do_getscroll updates the viewport. XXXREVIEW: Hopefully
// 100 msec are enough for do_optimalzoom to finish.
function gui_canvas_optimal_zoom(cid, h, v) {
    clearTimeout(optimalzoom_var[cid]);
    optimalzoom_var[cid] = setTimeout(do_optimalzoom, 150, cid, h, v);
}

exports.gui_canvas_optimal_zoom = gui_canvas_optimal_zoom;

// handling the selection
function gui_lower(cid, tag) {
    gui(cid).get_elem("patchsvg", function(svg_elem) {
        var first_child = svg_elem.firstElementChild,
        selection = null,
        gobj,
        len,
        i;
        if (tag === "selected") {
            selection = svg_elem.getElementsByClassName("selected");
        } else {
            gobj = get_gobj(cid, tag);
            if (gobj !== null) {
                selection = [gobj];
            }
        }
        if (selection !== null) {
            len = selection.length;
            for (i = len - 1; i >= 0; i--) {
                svg_elem.insertBefore(selection[i], first_child);
            }
        }
    });
}

// This only differs from gui_raise by setting first_child to
// the cord element instead of the first element in the svg.  Really,
// all three of these should be combined into a single function (plus
// all the silly logic on the C side moved here
function gui_raise(cid, tag) {
    gui(cid).get_elem("patchsvg", function(svg_elem) {
        var first_child = svg_elem.querySelector(".cord"),
        selection = null,
        gobj,
        len,
        i;
        if (tag === "selected") {
            selection = svg_elem.getElementsByClassName("selected");
        } else {
            gobj = get_gobj(cid, tag);
            if (gobj !== null) {
                selection = [gobj];
            }
        }
        if (selection !== null) {
            len = selection.length;
            for (i = len - 1; i >= 0; i--) {
                svg_elem.insertBefore(selection[i], first_child);
            }
        }
    });
}

function gui_find_lowest_and_arrange(cid, reference_element_tag, objtag) {
    gui(cid).get_gobj(reference_element_tag, function(ref_elem, w) {
        var svg_elem = w.document.getElementById("patchsvg"),
        selection = null,
        gobj,
        len,
        i; 
        if (objtag === "selected") {
            selection = svg_elem.getElementsByClassName("selected");
        } else {
            gobj = get_gobj(cid, objtag);
            if (gobj !== null) {
                selection = [get_gobj(cid, objtag)];
            }
        }
        if (selection !== null) {
            len = selection.length;
            for (i = len - 1; i >= 0; i--) {
                svg_elem.insertBefore(selection[i], ref_elem);
            }
        }
    });
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

// External GUI classes

function gui_pddplink_open(filename, dir) {
    var full_path, revised_dir, revised_filename;
    if (filename.indexOf("://") > -1) {
        external_doc_open(filename);
    } else if (path_is_absolute(filename)) {
        doc_open(path.dirname(filename), path.basename(filename));
    } else if (fs.existsSync(path.join(dir, filename))) {
        full_path = path.normalize(path.join(dir, filename));
        revised_dir = path.dirname(full_path);
        revised_filename = path.basename(full_path);
        doc_open(revised_dir, revised_filename);
    } else {
        // Give feedback to let user know the link didn't work...
        post("pddplink: error: file not found: " + filename);
    }
}
