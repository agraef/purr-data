"use strict";
var gui = require("nw.gui");
console.log("foo is foo");
console.log("gurgle is " + gui.App.argv);
var pdgui = require("./pdgui.js");
var port_no = gui.App.argv[0]; // fed to us by the Pd process
var gui_dir = gui.App.argv[3];
var pwd = process.env.PWD;
pdgui.set_port(port_no);
pdgui.set_pwd(pwd);
pdgui.set_gui_dir(gui_dir);
pdgui.set_pd_window(this);
pdgui.set_app_quitfn(app_quit);
pdgui.set_open_html_fn(open_html);
pdgui.set_open_textfile_fn(open_textfile);
pdgui.set_open_external_doc_fn(open_external_doc);

// gui preset
pdgui.skin.apply(this);

// For translations
var l = pdgui.get_local_string;

function app_quit () {
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

var chooser = document.querySelector("#fileDialog");
chooser.addEventListener("change", function(evt) {
    var file_array = this.value;
    // reset value so that we can open the same file twice
    this.value = null;
    pdgui.menu_open(file_array);
    console.log("tried to open something");
}, false);

document.getElementById("dsp_control").addEventListener("click",
    function(evt) {
        var dsp_state = this.checked ? 1 : 0;
        pdgui.pdsend("pd dsp", dsp_state);
    }
);

var find_bar = document.getElementById("console_find_text");
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

find_bar.defaultValue = "Search in Console";

console_find_set_default(find_bar);


// Invoke some functions to create main menus, etc.
gui.Window.get().on("close", function() {
    pdgui.menu_quit();
});
console.log(gui.App.argv); 

document.getElementById("fileDialog").setAttribute("nwworkingdir", pwd);
document.getElementById("fileDialog").setAttribute("accept",
    Object.keys(pdgui.pd_filetypes).toString());

nw_create_pd_window_menus();

gui.Window.get().setMinimumSize(350,250);

pdgui.connect();
pdgui.init_socket_events();

// nw context callbacks (mostly just creating/destroying windows)
pdgui.set_new_window_fn(nw_create_window);
pdgui.set_close_window_fn(nw_close_window);

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

function pdmenu_copy () {
    alert("Please implement pdmenu_copy"); 
}

function pdmenu_selectall () {
    alert("Please implement pdmenu_selectall"); 
}

function pdmenu_preferences () {
    alert("Please implement pdmenu_preferences"); 
}

function pdmenu_next_win () {
    alert("Please implement pdmenu_preferences"); 
}

function pdmenu_previous_win () {
    alert("Please implement pdmenu_preferences"); 
}

function pdmenu_parent_win () {
    alert("Please implement pdmenu_preferences"); 
}

function pdmenu_console_win () {
    alert("Please implement pdmenu_preferences"); 
}

function pdmenu_audio_on () {
    alert("Please implement pdmenu_preferences"); 
}

function pdmenu_audio_off () {
    alert("Please implement pdmenu_preferences"); 
}

function pdmenu_test_audio () {
    alert("Please implement pdmenu_preferences"); 
}

function pdmenu_load_meter () {
    alert("Please implement pdmenu_preferences"); 
}

function pdmenu_about_pd () {
    alert("Please implement pdmenu_preferences"); 
}

function pdmenu_manual () {
    alert("Please implement pdmenu_preferences"); 
}

function pdmenu_help_browser () {
    alert("Please implement pdmenu_preferences"); 
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

// Menus for the main Pd window
function nw_create_pd_window_menus () {
    // Window menu
    var windowMenu = new gui.Menu({
        type: "menubar"
    });

    // File menu
    var fileMenu = new gui.Menu();

    // Add to window menu
    windowMenu.append(new gui.MenuItem({
        label: l("menu.file"),
        submenu: fileMenu
    }));

    // File sub-entries
    fileMenu.append(new gui.MenuItem({
        label: l("menu.new"),
        click: pdgui.menu_new,
        key: "n",
        modifiers: "ctrl",
        tooltip: l("menu.new.tt")
    }));

    fileMenu.append(new gui.MenuItem({
        label: l("menu.open"),
        key: "o",
        modifiers: "ctrl",
        tooltip: l("menu.open.tt"),
        click: function (){
            var input, chooser,
                span = document.querySelector("#fileDialogSpan");
            // Complicated workaround-- see comment in build_file_dialog_string
            input = pdgui.build_file_dialog_string({
                style: "display: none;",
                type: "file",
                id: "fileDialog",
                nwworkingdir: pwd,
                multiple: null,
                // These are copied from pd_filetypes in pdgui.js
                accept: ".pd,.pat,.mxt,.mxb,.help"
            });
            span.innerHTML = input;
            var chooser = document.querySelector("#fileDialog");
            chooser.click();
        }
    }));

    if (pdgui.k12_mode == 1) {
        fileMenu.append(new gui.MenuItem({
        label: l("menu.k12.demos"),
        tooltip: l("menu.k12.demos_tt"),
        click: pdgui.menu_k12_open_demos
        }));
    }

    fileMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    // Note: this must be different for the main Pd window
    fileMenu.append(new gui.MenuItem({
        label: l("menu.save"),
            click: function () {},
            enabled: false,
        key: "s",
        tooltip: l("menu.save.tt"),
        modifiers: "ctrl"
    }));

    fileMenu.append(new gui.MenuItem({
        label: l("menu.saveas"),
        click: function (){},
        enabled: false,
        key: "S",
        tooltip: l("menu.saveas_tt"),
        modifiers: "ctrl"
    }));

    if (pdgui.k12_mode == 0) {
        fileMenu.append(new gui.MenuItem({
            type: "separator"
        }));
    }

    fileMenu.append(new gui.MenuItem({
        label: l("menu.message"),
        click: pdgui.menu_send,
        key: "m",
        modifiers: "ctrl",
        tooltip: l("menu.message_tt")
    }));

    if (pdgui.k12_mode == 0) {
        fileMenu.append(new gui.MenuItem({
            type: "separator"
        }));
    }

    // recent files go here

    // anther separator goes here if there are any recent files

    // Note: there's no good reason to have this here
    fileMenu.append(new gui.MenuItem({
        label: l("menu.close"),
        click: function () {},
        enabled: false,
    }));

    // Quit Pd
    fileMenu.append(new gui.MenuItem({
        label: l("menu.quit"),
        click: pdgui.menu_quit,
        key: "q",
        modifiers: "ctrl",
        tooltip: l("menu.quit_tt")
    }));


    // Edit menu
    var editMenu = new gui.Menu();

    // Add to window menu
    windowMenu.append(new gui.MenuItem({
    label: l("menu.edit"),
    submenu: editMenu
    }));

    // Edit sub-entries
    editMenu.append(new gui.MenuItem({
        label: l("menu.copy"),
        click: function() {
            document.execCommand("copy");
        },
        key: "c",
        modifiers: "ctrl",
        tooltip: l("menu.copy_tt")
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.selectall"),
        click: function () {
            var container_id = "p1", range;
            // This should work across browsers
            if (document.selection) {
                range = document.body.createTextRange();
                range.moveToElementText(document.getElementById(container_id));
                range.select();
            } else if (window.getSelection) {
                range = document.createRange();
                range.selectNode(document.getElementById(container_id));
                // we need to empty the current selection to avoid a strange
                // error when trying to select all right after Pd starts:
                // "The given range and the current selection belong to two
                //  different document fragments."
                // (I guess nw.js somehow starts up with the selection being 
                // somewhere outside the window...)
                window.getSelection().empty();
                window.getSelection().addRange(range);
            }
        },
        key: "a",
        modifiers: "ctrl",
        tooltip: l("menu.selectall_tt")
    }));

    editMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.zoomin"),
        click: function () {
            gui.Window.get().zoomLevel += 1;
            pdgui.post("zoom level is " + gui.Window.get().zoomLevel);
        },
        key: "=",
        modifiers: "ctrl",
        tooltip: l("menu.zoomin_tt")
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.zoomout"),
        click: function () {
            gui.Window.get().zoomLevel -= 1;
            pdgui.post("zoom level is " + gui.Window.get().zoomLevel);
        },
        key: "-",
        modifiers: "ctrl",
        tooltip: l("menu.zoomout_tt")
    }));

    editMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.find"),
        click: function () {
            var find_bar = document.getElementById("console_find"),
                find_bar_text = document.getElementById("console_find_text"),
                text_container = document.getElementById("console_bottom"),
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
        },
        key: "f",
        modifiers: "ctrl",
        tooltip: l("menu.find_tt")
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.preferences"),
        click: pdgui.open_prefs,
        key: "p",
        modifiers: "ctrl",
        tooltip: l("menu.preferences_tt")
    }));


    // Windows menu... call it "winman" (i.e., window management)
    // to avoid confusion
    var winmanMenu = new gui.Menu();

    // Add to windows menu
    windowMenu.append(new gui.MenuItem({
    label: l("menu.windows"),
    submenu: winmanMenu
    }));

    // Winman sub-entries
    winmanMenu.append(new gui.MenuItem({
        label: l("menu.nextwin"),
        click: pdmenu_next_win,
        //key: "c",
        //modifiers: "ctrl",
        tooltip: l("menu.nextwin_tt")
    }));

    winmanMenu.append(new gui.MenuItem({
        label: l("menu.prevwin"),
        click: pdmenu_previous_win,
        //key: "a",
        //modifiers: "ctrl",
        tooltip: l("menu.prevwin_tt")
    }));

    winmanMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    winmanMenu.append(new gui.MenuItem({
        label: l("menu.parentwin"),
        click: pdmenu_parent_win,
        //key: "a",
        //modifiers: "ctrl",
        tooltip: l("menu.parentwin_tt")
    }));

    winmanMenu.append(new gui.MenuItem({
        label: l("menu.pdwin"),
        click: pdmenu_console_win,
        //key: "a",
        //modifiers: "ctrl",
        tooltip: l("menu.pdwin_tt")
    }));

    // Media menu
    var mediaMenu = new gui.Menu();

    // Add to window menu
    windowMenu.append(new gui.MenuItem({
    label: l("menu.media"),
    submenu: mediaMenu
    }));

    // Media sub-entries
    mediaMenu.append(new gui.MenuItem({
        label: l("menu.audio_on"),
        click: function() {
            pdgui.pdsend("pd dsp 1");
        },
        key: "/",
        modifiers: "ctrl",
        tooltip: l("menu.audio_on_tt")
    }));

    mediaMenu.append(new gui.MenuItem({
        label: l("menu.audio_off"),
        click: function() {
            pdgui.pdsend("pd dsp 0");
        },
        key: ".",
        modifiers: "ctrl",
        tooltip: l("menu.audio_off_tt")
    }));

    mediaMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    mediaMenu.append(new gui.MenuItem({
        label: l("menu.test"),
        click: function() {
            pdgui.pd_doc_open("doc/7.stuff/tools", "testtone.pd");
        },
        //key: "a",
        //modifiers: "ctrl",
        tooltip: l("menu.test_tt")
    }));

    mediaMenu.append(new gui.MenuItem({
        label: l("menu.loadmeter"),
        click: function() {
            pdgui.pd_doc_open("doc/7.stuff/tools", "load-meter.pd");
        },
        //key: "a",
        //modifiers: "ctrl",
        tooltip: l("menu.loadmeter_tt")
    }));

    // Help menu
    var helpMenu = new gui.Menu();

    // Add to window menu
    windowMenu.append(new gui.MenuItem({
    label: l("menu.help"),
    submenu: helpMenu
    }));

    // Help sub-entries
    helpMenu.append(new gui.MenuItem({
        label: l("menu.about"),
        click: function() {
            pdgui.pd_doc_open("doc/1.manual", "1.introduction.txt");
        },
        //key: "c",
        //modifiers: "ctrl",
        tooltip: l("menu.about_tt")
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.manual"),
        click: function() {
            pdgui.pd_doc_open("doc/1.manual", "index.htm");
        },
        //key: "a",
        //modifiers: "ctrl",
        tooltip: l("menu.manual_tt")
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.browser"),
        click: pdmenu_help_browser,
        //key: "a",
        //modifiers: "ctrl",
        tooltip: l("menu.browser_tt")
    }));

    helpMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.l2ork_list"),
        click: function() {
            pdgui.external_doc_open("http://disis.music.vt.edu/listinfo/l2ork-dev");
        },
        //key: "a",
        //modifiers: "ctrl",
        tooltip: l("menu.l2ork_list_tt")
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.pd_list"),
        click: function() {
            pdgui.external_doc_open("http://puredata.info/community/lists");
        },
        //key: "a",
        //modifiers: "ctrl",
        tooltip: l("menu.pd_list_tt")
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.forums"),
        click: function() {
            pdgui.external_doc_open("http://forum.pdpatchrepo.info/");
        },
        //key: "a",
        //modifiers: "ctrl",
        tooltip: l("menu.forums_tt")
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.devtools"),
        click: function() {
            gui.Window.get().showDevTools();
        },
        key: "b", // temporary convenience shortcut-- can change if needed
        modifiers: "ctrl",
        tooltip: l("menu.devtools_tt")
    }));

    //helpMenu.append(new gui.MenuItem({
    //    label: l("menu.irc"),
    //    click: function() {
    //        pdgui.external_doc_open("irc://irc.freenode.net/dataflow");
    //    },
    //    //key: "a",
    //    //modifiers: "ctrl",
    //    tooltip: l("menu.irc_tt")
    //}));

    // Assign to window
    gui.Window.get().menu = windowMenu;
}

function nw_close_window(window) {
    window.close(true);
}

// Way too many arguments here-- rethink this interface
function nw_create_window(cid, type, width, height, xpos, ypos, menu_flag,
    resize, topmost, cnv_color, canvas_string, dir, dirty_flag, cargs,
    attr_array) {
        // todo: make a separate way to format the title for OSX
    var my_title =  pdgui.format_window_title(canvas_string, dirty_flag,
        cargs, dir);
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

function canvasNew(args) {
    console.log(args);
}
