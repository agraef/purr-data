"use strict";
var gui = require("nw.gui");
var pdgui = require("./pdgui.js");
var pd_menus = require("./pd_menus.js");

// we're using pwd in fileDialog. If you start Pd by clicking the app bundle
// in OSX, the PWD environment variable doesn't exist. In that case we use
// HOME instead.
var pwd = process.env.PWD !== undefined ? process.env.PWD : process.env.HOME;

// gui preset
pdgui.skin.apply(this);

// For translations
var l = pdgui.get_local_string;

// Set up the Pd Window
set_vars(this);
add_events();
nw_create_pd_window_menus(gui, window);
gui.Window.get().setMinimumSize(350,250);
// Now we create a connection from the GUI to Pd, in one of two ways:
// 1) If the GUI was started by Pd, then we create a tcp client and
//    connect on the port Pd fed us in our command line arguments.
// 2) If Pd hasn't started yet, then the GUI creates a tcp server and spawns
//    Pd using the "-guiport" flag with the port the GUI is listening on.
// Pd always starts the GUI with a certain set of command line arguments. If
// those arguments aren't present then we assume we need to start Pd.
connect();

function have_args() {
    return !!gui.App.argv.length;
}

function set_vars(global) {
    var port_no, gui_dir;
    // If the GUI was started by Pd, our port number is going to be
    // the first argument. If the GUI is supposed to start Pd, we won't
    // have any arguments and need to set it here.
    if (have_args()) {
        port_no = gui.App.argv[0]; // fed to us by the Pd process
        // looks like this is the same as pwd below
        gui_dir = gui.App.argv[3];
    } else {
        // If we're starting Pd, this is the first port number to try. (We'll
        // increment it if that port happens to be taken.
        port_no = 5400; 
        gui_dir = pwd;
    }
    pdgui.set_port(port_no);
    pdgui.set_pwd(pwd);
    pdgui.set_gui_dir(gui_dir);
    pdgui.set_pd_window(global);
    pdgui.set_app_quitfn(app_quit);
    pdgui.set_open_html_fn(open_html);
    pdgui.set_open_textfile_fn(open_textfile);
    pdgui.set_open_external_doc_fn(open_external_doc);

    // nw context callbacks (mostly just creating/destroying windows)
    pdgui.set_new_window_fn(nw_create_window);
    pdgui.set_close_window_fn(nw_close_window);
}

function app_quit() {
    console.log("quitting Pd...");
    gui.App.quit();
}

function open_html(target) {
    gui.Shell.openItem(target);
}

function open_textfile(target) {
    gui.Shell.openItem(target);
}

function open_external_doc(target) {
    gui.Shell.openExternal(target);
}

function nw_window_focus_callback() {
    if (process.platform === "darwin") {
        nw_create_pd_window_menus(gui, window);
    }
}

function add_events() {
    // Find bar
    var find_bar = document.getElementById("console_find_text");
    find_bar.defaultValue = "Search in Console";
    console_find_set_default(find_bar);
    find_bar.addEventListener("keydown",
        function(e) {
            return console_find_keydown(this, e);
        }, false
    );
    find_bar.addEventListener("keypress",
        function(e) {
            console_find_keypress(this, e);
        }, false
    );
    // DSP toggle
    document.getElementById("dsp_control").addEventListener("click",
        function(evt) {
            var dsp_state = this.checked ? 1 : 0;
            pdgui.pdsend("pd dsp", dsp_state);
        }
    );
    // Browser Window Close
    gui.Window.get().on("close", function() {
        pdgui.menu_quit();
    });
    // Focus callback for OSX
    gui.Window.get().on("focus", function() {
        nw_window_focus_callback();
    });
    // Open dialog
    document.getElementById("fileDialog").setAttribute("nwworkingdir", pwd);
    document.getElementById("fileDialog").setAttribute("accept",
        Object.keys(pdgui.pd_filetypes).toString());
}

function connect() {
    var gui_path;
    if (have_args()) { 
        // Pd started the GUI, so connect to it on port provided in our args
        pdgui.post("Pd has started the GUI");
        pdgui.connect_as_client();
    } else {
        // create a tcp server, then spawn Pd with "-guiport" flag and port
        gui_path = window.location.pathname;
        gui_path = gui_path.substr(0, gui_path.lastIndexOf('/'));
        pdgui.post("GUI is starting Pd...");
        pdgui.connect_as_server(gui_path);
    }
}

function console_find_check_default(e) {
    if (e.value === e.defaultValue) {
        return true;
    } else {
        return false;
    }
}

function console_find_set_default(e) {
    e.value = e.defaultValue;
    e.setSelectionRange(0,0);
    e.style.color = "#888";
}

function console_unwrap_tag(console_elem, tag_name) {
    var b = console_elem.getElementsByTagName(tag_name),
        parent_elem;
    while (b.length) {
        parent_elem = b[0].parentNode;
        while(b[0].firstChild) {
            parent_elem.insertBefore(b[0].firstChild, b[0]);
        }
        parent_elem.removeChild(b[0]); 
        parent_elem.normalize();
    }
}

function console_find_text(elem, evt, callback) {
    var console_text = document.getElementById("p1"),
        wrap_tag = "mark",
        wrapper_count;
    // Check the input for default text before the event happens
    if (console_find_check_default(elem)) {
       // if so, erase it
        elem.value = "";
        // put this in css and use class here
        elem.style.color = "#000";
    }
    window.setTimeout(function () {
        console_unwrap_tag(console_text, wrap_tag);

        // Check after the event if the value is empty, and if
        // so set it to default value
        if (elem.value === undefined || elem.value === "") {
            console_find_set_default(elem);
        } else if (!console_find_check_default(elem)) {
            window.findAndReplaceDOMText(console_text, {
                //preset: "prose",
                find: elem.value.toLowerCase(),
                wrap: wrap_tag
            });
            // The searchAndReplace API is so bad you can't even know how
            // many matches there were without traversing the DOM and
            // counting the wrappers!
            wrapper_count = console_text.getElementsByTagName(wrap_tag).length;
            if (wrapper_count < 1) {
                elem.style.setProperty("background", "red");
            } else {
                elem.style.setProperty("background", "white");
            }
        }
        if (callback) {
            callback();
        }
    }, 0);
}

// start at top and highlight the first result after a search
function console_find_callback() {
    var highlight_checkbox = document.getElementById("console_find_highlight");
    console_find_highlight_all(highlight_checkbox);
    console_find_traverse.set_index(0);
    console_find_traverse.next();
}

function console_find_keypress(elem, e) {
    console_find_text(elem, e, console_find_callback);
}

function console_find_highlight_all(elem) {
    var matches,
        highlight_tag = "console_find_highlighted",
        state = elem.checked,
        i, len;
    matches = document.getElementById("p1")
        .getElementsByClassName(highlight_tag);
    // remember-- matches is a _live_ collection, not an array.
    // If you remove the highlight_tag from an element, it is
    // automatically removed from the collection. I cannot yet
    // see a single benefit to this behavior-- here, it means
    // we must decrement i to keep from skipping over every
    // other element... :(
    for (i = matches.length - 1; i >= 0; i--) {
        matches[i].classList.remove(highlight_tag);
    }
    if (state) {
        matches = document.getElementById("p1").getElementsByTagName("mark");
        for (i = 0; i < matches.length; i++) {
            matches[i].classList.add(highlight_tag);
        }
    }
}

var console_find_traverse = (function() {
    var count = 0,
        console_text = document.getElementById("p1"),
        wrap_tag = "mark";
    return {
        next: function() {
            var i, last, next,
                elements = console_text.getElementsByTagName(wrap_tag);
            if (elements.length > 0) {
                i = count % elements.length;
                elements[i].classList.add("console_find_current");
                if (elements.length > 1) {
                    last = i === 0 ? elements.length - 1 : i - 1;
                    next = (i + 1) % elements.length;
                    elements[last].classList.remove("console_find_current");
                    elements[next].classList.remove("console_find_current");
                }
                // adjust the scrollbar to make sure the element is visible,
                // but only if necessary.
                // I don't think this is available on all browsers...
                elements[i].scrollIntoViewIfNeeded();
                count++;
            }
        },
        set_index: function(c) {
            count = c;
        }
    }
}());

function console_find_keydown(elem, evt) {
    if (evt.keyCode === 13) {
        console_find_traverse.next();
        evt.stopPropagation();
        evt.preventDefault();
        return false;
    } else if (evt.keyCode === 27) { // escape

    } else if (evt.keyCode === 8 || // backspace or delete
               evt.keyCode === 46) {
        console_find_text(elem, evt, console_find_callback);
    }
}

function nw_close_window(window) {
    window.close(true);
}

// Way too many arguments here-- rethink this interface
function nw_create_window(cid, type, width, height, xpos, ypos, attr_array) {
        // todo: make a separate way to format the title for OSX
    var my_title;
    if (type === "pd_canvas") {
        my_title = pdgui.format_window_title(
            attr_array.name,
            attr_array.dirty,
            attr_array.args,
            attr_array.dir);
    } else {
        my_title = type;
    }
    var my_file =
        type === "pd_canvas" ? "pd_canvas.html" : "dialog_" + type + ".html";

    var new_win = gui.Window.open(my_file, {
        title: my_title,
        position: "center",
        toolbar: false,
        focus: true,
        width: width,
        // We add 23 as a kludge to account for the menubar at the top of
        // the window.  Ideally we would just get rid of the canvas menu
        // altogether to simplify things. But we'd have to add some kind of
        // widget for the "Put" menu.
        height: height + 23,
        x: xpos,
        y: ypos
    });
    //pdgui.post("attr_array is " + attr_array);
    var eval_string = "register_canvas_id(" +
                      JSON.stringify(cid) + ", " +
                      JSON.stringify(attr_array) + ");";
    //pdgui.post("eval string is " + eval_string);
    //if (attr_array !== null) {
    //    pdgui.post("attr_array is " + attr_array.toString());
    //}
    new_win.on("loaded", function() {
        new_win.eval(null, eval_string);
    });
    return new_win;
}

// Pd Window Menu Bar

function pdmenu_help_browser (w) {
    w.alert("Please implement pdmenu_preferences"); 
}

function pdmenu_l2ork_mailinglist () {
    alert("Please implement pdmenu_preferences"); 
}

function pdmenu_pd_mailinglists () {
    alert("Please implement pdmenu_preferences"); 
}

function pdmenu_forums () {
    alert("Please implement pdmenu_preferences"); 
}

function pdmenu_irc () {
    alert("Please implement pdmenu_preferences"); 
}

function minit(menu_item, options) {
    var o;
    for (o in options) {
        if (options.hasOwnProperty(o)) {
            menu_item[o] = options[o];
        }
    }
}

function nw_create_pd_window_menus(gui, w) {
    var m = pd_menus.create_menu(gui, "console");

    // On OSX we have menu items for canvas operations-- we need to disable
    // them when the console gets focus.
    var osx = process.platform === "darwin";

    // File sub-entries
    minit(m.file.new_file, { click: pdgui.menu_new });
    minit(m.file.open, {
        click: function (){
            var input, chooser,
                span = w.document.querySelector("#fileDialogSpan");
            // Complicated workaround-- see comment in build_file_dialog_string
            input = pdgui.build_file_dialog_string({
                style: "display: none;",
                type: "file",
                id: "fileDialog",
                nwworkingdir: pdgui.get_pwd(),
                multiple: null,
                // These are copied from pd_filetypes in pdgui.js
                accept: ".pd,.pat,.mxt,.mxb,.help"
            });
            span.innerHTML = input;
            var chooser = w.document.querySelector("#fileDialog");
            // Hack-- we have to set the event listener here because we
            // changed out the innerHTML above
            chooser.onchange = function() {
                var file_array = this.value;
                // reset value so that we can open the same file twice
                this.value = null;
                pdgui.menu_open(file_array);
                console.log("tried to open something");
            };
            chooser.click();
        }
    });
    if (pdgui.k12_mode == 1) {
        minit(m.file.k12, { click: pdgui.menu_k12_open_demos });
    }
    // Note: this must be different for the main Pd window
    if (osx) {
        minit(m.file.save, { enabled: false });
        minit(m.file.saveas, { enabled: false });
    }
    minit(m.file.message, { click: pdgui.menu_send });
    if (osx) {
        minit(m.file.close, { enabled: false });
    }
    minit(m.file.quit, { click: pdgui.menu_quit });

    // Edit sub-entries
    if (osx) {
        minit(m.edit.undo, { enabled: false });
        minit(m.edit.redo, { enabled: false });
        minit(m.edit.cut, { enabled: false });
    }
    minit(m.edit.copy, { enabled: true,
        click: function() {
            w.document.execCommand("copy");
        }
    });
    if (osx) {
        minit(m.edit.paste, { enabled: false });
        minit(m.edit.duplicate, { enabled: false });
    }
    minit(m.edit.selectall, {
        enabled: true,
        click: function () {
            var container_id = "p1", range;
            // This should work across browsers
            if (w.document.selection) {
                range = w.document.body.createTextRange();
                range.moveToElementText(w.document.getElementById(container_id));
                range.select();
            } else if (w.getSelection) {
                range = w.document.createRange();
                range.selectNode(w.document.getElementById(container_id));
                // we need to empty the current selection to avoid a strange
                // error when trying to select all right after Pd starts:
                // "The given range and the current selection belong to two
                //  different document fragments."
                // (I guess nw.js somehow starts up with the selection being 
                // somewhere outside the window...)
                w.getSelection().empty();
                w.getSelection().addRange(range);
            }
        }
    });
    minit(m.edit.clear_console, {
        enabled: true,
        click: pdgui.clear_console
    });
    if (osx) {
        minit(m.edit.reselect, { enabled: false });
    }
    if (osx) {
        minit(m.edit.tidyup, { enabled: false });
        minit(m.edit.tofront, { enabled: false });
        minit(m.edit.toback, { enabled: false });
        minit(m.edit.font, { enabled: false });
        minit(m.edit.cordinspector, { enabled: false });
    }
    minit(m.edit.find, {
        click: function () {
            var find_bar = w.document.getElementById("console_find"),
                find_bar_text = w.document.getElementById("console_find_text"),
                text_container = w.document.getElementById("console_bottom"),
                state = find_bar.style.getPropertyValue("display");
            if (state === "none") {
                text_container.style.setProperty("bottom", "1em");
                find_bar.style.setProperty("display", "inline");
                find_bar.style.setProperty("height", "1em");
                text_container.scrollTop = text_container.scrollHeight;
                find_bar_text.focus();
                find_bar_text.select();
            } else {
                text_container.style.setProperty("bottom", "0px");
                find_bar.style.setProperty("display", "none");
            }
        }
    });
    if (osx) {
        minit(m.edit.findagain, { enabled: false });
        minit(m.edit.finderror, { enabled: false });
        minit(m.edit.autotips, { enabled: false });
        minit(m.edit.editmode, { enabled: false });
    }
    minit(m.edit.preferences, {
        click: pdgui.open_prefs,
    });

    // View menu
    minit(m.view.zoomin, {
        click: function () {
            var z = gui.Window.get().zoomLevel;
            if (z < 8) { z++; }
            gui.Window.get().zoomLevel = z;
        }
    });
    minit(m.view.zoomout, {
        click: function () {
            var z = gui.Window.get().zoomLevel;
            if (z > -7) { z--; }
            gui.Window.get().zoomLevel = z;
        }
    });
    minit(m.view.zoomreset, {
        click: function () {
            gui.Window.get().zoomLevel = 0;
        }
    });
    minit(m.view.fullscreen, {
        click: function() {
            var win = gui.Window.get(),
                fullscreen = win.isFullscreen;
            win.isFullscreen = !fullscreen;
            pdgui.post("fullscreen is " + !fullscreen);
        }
    });

    // Put menu
    if (osx) {
        minit(m.put.object, { enabled: false });
        minit(m.put.message, { enabled: false });
        minit(m.put.number, { enabled: false });
        minit(m.put.symbol, { enabled: false });
        minit(m.put.comment, { enabled: false });
        minit(m.put.bang, { enabled: false });
        minit(m.put.toggle, { enabled: false });
        minit(m.put.number2, { enabled: false });
        minit(m.put.vslider, { enabled: false });
        minit(m.put.hslider, { enabled: false });
        minit(m.put.vradio, { enabled: false });
        minit(m.put.hradio, { enabled: false });
        minit(m.put.vu, { enabled: false });
        minit(m.put.cnv, { enabled: false });
        //minit(m.put.graph, { enabled: false });
        minit(m.put.array, { enabled: false });
    }

    // Winman sub-entries
    minit(m.win.nextwin, {
        click: function() {
            pdgui.raise_next("pd_window");
        }
    });
    minit(m.win.prevwin, {
        click: function() {
            pdgui.raise_prev("pd_window");
        }
    });
    if (osx) {
        minit(m.win.parentwin, { enabled: false });
        minit(m.win.visible_ancestor, { enabled: false });
        minit(m.win.pdwin, { enabled: false });
    }

    // Media sub-entries
    minit(m.media.audio_on, {
        click: function() {
            pdgui.pdsend("pd dsp 1");
        }
    });
    minit(m.media.audio_off, {
        click: function() {
            pdgui.pdsend("pd dsp 0");
        }
    });
    minit(m.media.test, {
        click: function() {
            pdgui.pd_doc_open("doc/7.stuff/tools", "testtone.pd");
        }
    });
    minit(m.media.loadmeter, {
        click: function() {
            pdgui.pd_doc_open("doc/7.stuff/tools", "load-meter.pd");
        }
    });

    // Help sub-entries
    minit(m.help.about, {
        click: function() {
            pdgui.pd_doc_open("doc/1.manual", "1.introduction.txt");
        }
    });
    minit(m.help.manual, {
        click: function() {
            pdgui.pd_doc_open("doc/1.manual", "index.htm");
        }
    });
    minit(m.help.browser, {
        click: function() {
            alert("please implement a help browser");
        }
    });
    minit(m.help.l2ork_list, {
        click: function() {
            pdgui.external_doc_open("http://disis.music.vt.edu/listinfo/l2ork-dev");
        }
    });
    minit(m.help.pd_list, {
        click: function() {
            pdgui.external_doc_open("http://puredata.info/community/lists");
        }
    });
    minit(m.help.forums, {
        click: function() {
            pdgui.external_doc_open("http://forum.pdpatchrepo.info/");
        }
    });
    minit(m.help.irc, {
        click: function() { alert("Please link to the irc page") }
    });
    minit(m.help.devtools, {
        click: function () {
            gui.Window.get().showDevTools();
        }
    });
}
