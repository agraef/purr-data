"use strict";
var gui = require("nw.gui"); 
var pdgui = require("./pdgui.js");

// Apply gui preset to this canvas
pdgui.skin.apply(this);

//var name = pdgui.last_loaded();
   
var l = pdgui.get_local_string;

console.log("my working dir is " + pdgui.get_pwd());
document.getElementById("saveDialog")
    .setAttribute("nwworkingdir", pdgui.get_pwd());
document.getElementById("fileDialog")
    .setAttribute("nwworkingdir", pdgui.get_pwd());
document.getElementById("fileDialog").setAttribute("accept",
    Object.keys(pdgui.pd_filetypes).toString());

var last_keydown = "";

// This could probably be in pdgui.js
function add_keymods(key, evt) {
    var shift = evt.shiftKey ? "Shift" : "";
    var ctrl = evt.ctrlKey ? "Ctrl" : "";
    return shift + ctrl + key;
}

function text_to_fudi(text) {
    text = text.trim();
    text = text.replace(/(\$[0-9]+)/g, "\\$1");    // escape dollar signs
    text = text.replace(/(?!\\)(,|;)/g, " \\$1 "); // escape "," and ";"
    text = text.replace(/\{|\}/g, "");             // filter "{" and "}"
    text = text.replace(/\s+/g, " ");              // filter consecutive /s
    return text;
}

// Should probably be in pdgui.js
function encode_for_dialog(s) {
    s = s.replace(/\s/g, "+_");
    s = s.replace(/\$/g, "+d");
    s = s.replace(/;/g, "+s");
    s = s.replace(/,/g, "+c");
    s = s.replace(/\+/g, "++");
    s = "+" + s;
    return s;
}

// These three functions need to be inside canvas_events closure
function canvas_find_whole_word(elem) {
    canvas_events.match_words(elem.checked);
}

function canvas_find_blur() {
    canvas_events.normal();
}

function canvas_find_focus() {
pdgui.post("flub!");
    var state = canvas_events.get_state();
    canvas_events.search();
}

var canvas_events = (function() {
    var name,
        state,
        scalar_draggables = {}, // elements of a scalar which have the "drag" event enabled
        draggable_elem,         // the current scalar element being dragged
        last_draggable_x,       // last x coord for the element we're dragging
        last_draggable_y,       // last y 
        previous_state = "none", /* last state, excluding explicit 'none' */
        match_words_state = false,
        last_search_term = "",
        svg_view = document.getElementById("patchsvg").viewBox.baseVal,
        textbox = function () {
            return document.getElementById("new_object_textentry");
        },
        events = {
            mousemove: function(evt) {
                //pdgui.post("x: " + evt.pageX + " y: " + evt.pageY +
                //    " modifier: " + (evt.shiftKey + (evt.ctrlKey << 1)));
                pdgui.pdsend(name, "motion",
                    (evt.pageX + svg_view.x),
                    (evt.pageY + svg_view.y),
                    (evt.shiftKey + (evt.ctrlKey << 1))
                );
                evt.stopPropagation();
                evt.preventDefault();
                return false;
            },
            mousedown: function(evt) {
                // tk events (and, therefore, Pd evnets) are one greater
                // than html5...
                var b = evt.button + 1;
                var mod;
                // See if there are any draggable scalar shapes...
                if (Object.keys(scalar_draggables)) {
                    // if so, see if our target is one of them...
                    if (scalar_draggables[evt.target.id]) {
                        // then set some state and turn on the drag events
                        draggable_elem = evt.target;
                        last_draggable_x = evt.pageX;
                        last_draggable_y = evt.pageY;
                        canvas_events.scalar_drag();
                    }
                }
                // For some reason right-click sends a modifier value of "8",
                // and canvas_doclick in g_editor.c depends on that value to
                // do the right thing.  So let's hack...
                if (b === 3) { // right-click
                    mod = 8;
                } else {
                    mod = (evt.shiftKey + (evt.ctrlKey << 1)); 
                }
                pdgui.pdsend(name, "mouse",
                    (evt.pageX + svg_view.x),
                    (evt.pageY + svg_view.y),
                    b, mod
                );
                //evt.stopPropagation();
                //evt.preventDefault();
            },
            mouseup: function(evt) {
                //pdgui.post("mouseup: x: " +
                //    evt.pageX + " y: " + evt.pageY +
                //    " button: " + (evt.button + 1));
                pdgui.pdsend(name, "mouseup",
                    (evt.pageX + svg_view.x),
                    (evt.pageY + svg_view.y),
                    (evt.button + 1)
                );
                evt.stopPropagation();
                evt.preventDefault();
            },
            keydown: function(evt) {
                var key_code = evt.keyCode; 
                var hack = null; // hack for unprintable ascii codes
                switch(key_code) {
                    case 8:
                    case 9:
                    case 10:
                    case 27:
                    //case 32:
                    case 127: hack = key_code; break;
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
                        if (evt.ctrlKey === true) {
                            pdgui.pdsend(name, "selectall");
                            hack = 0; // not sure what to report here...
                        }
                        break;
                    case 88:
                        if (evt.ctrlKey === true) {
                            pdgui.pdsend(name, "cut");
                            hack = 0; // not sure what to report here...
                        }
                        break;
                    case 67:
                        if (evt.ctrlKey === true) {
                            pdgui.pdsend(name, "copy");
                            hack = 0; // not sure what to report here...
                        }
                        break;
                    case 86:
                        if (evt.ctrlKey === true) {
                            pdgui.pdsend(name, "paste");
                            hack = 0; // not sure what to report here...
                        }
                        break;

                    // Need to handle Control key, Alt

                    case 16: hack = "Shift"; break;
                    case 17: hack = "Control"; break;
                    case 18: hack = "Alt"; break;
                }
                if (hack !== null) {
                    pdgui.gui_canvas_sendkey(name, 1, evt, hack);
                    pdgui.set_keymap(key_code, hack);
                }
                //pdgui.post("keydown time: keycode is " + evt.keyCode);
                last_keydown = evt.keyCode;
                //evt.stopPropagation();
                //evt.preventDefault();
            },
            keypress: function(evt) {
                // Hack to handle undo/redo shortcuts
                if (evt.charCode === 26) {
                    if (evt.ctrlKey === true) {
                        if (evt.shiftKey === true) { // ctrl-Shift-z
                            pdgui.pdsend(name, "redo");
                        } else { // ctrl-z
                            pdgui.pdsend(name, "undo");
                        }
                        return;
                    }
                }

                pdgui.gui_canvas_sendkey(name, 1, evt, evt.charCode);
                pdgui.set_keymap(last_keydown, evt.charCode);
                //pdgui.post("keypress time: charcode is " + evt.charCode);
                // Don't do things like scrolling on space, arrow keys, etc.
                //evt.stopPropagation();
                evt.preventDefault();
            },
            keyup: function(evt) {
                var my_char_code = pdgui.get_char_code(evt.keyCode);
                pdgui.gui_canvas_sendkey(name, 0, evt, my_char_code);
                //pdgui.post("keyup time: charcode is: " + my_char_code);
                if (evt.keyCode === 13 && evt.ctrlKey) {
                    pdgui.pdsend(name, "reselect");
                }
                evt.stopPropagation();
                evt.preventDefault();
            },
            text_mousemove: function(evt) {
                evt.stopPropagation();    
                //evt.preventDefault();
                return false;
            },
            text_mousedown: function(evt) {
                if (textbox() !== evt.target) {
                    // Yes: I _really_ want .innerText and NOT .textContent
                    // here.  I want those newlines: although that isn't
                    // standard in Pd-Vanilla, Pd-l2ork uses and preserves
                    // them inside comments
                    utils.create_obj();
                    //var fudi_msg = text_to_fudi(textbox().innerText);
                    //pdgui.pdsend(name, "createobj", fudi_msg);
                    //pdgui.post("formatted content is " + fudi_msg);
                    events.mousedown(evt);
                    canvas_events.normal();
                }
                evt.stopPropagation();    
                //evt.preventDefault();
                return false;
            },
            text_mouseup: function(evt) {
                pdgui.post("mouseup target is " +
                    evt.target + " and textbox is " + textbox());
                //evt.stopPropagation();    
                //evt.preventDefault();
                return false;
            },
            text_keydown: function(evt) {
                evt.stopPropagation();    
                //evt.preventDefault();
                return false;
            },
            text_keyup: function(evt) {
                evt.stopPropagation();    
                //evt.preventDefault();
                // ctrl-Enter to reselect
                if (evt.keyCode === 13 && evt.ctrlKey) {
                    canvas_events.set_obj();
                    pdgui.pdsend(name, "reselect");
                }
                return false;
            },
            text_keypress: function(evt) {
                evt.stopPropagation();    
                //evt.preventDefault();
                return false;
            },
            floating_text_click: function(evt) {
                //pdgui.post("leaving floating mode");
                canvas_events.text();
                evt.stopPropagation();
                evt.preventDefault();
                return false;
            },
            floating_text_keypress: function(evt) {
                //pdgui.post("leaving floating mode");
                canvas_events.text();
                //evt.stopPropagation();
                //evt.preventDefault();
                //return false;
            },
            find_click: function(evt) {
                var t = document.getElementById("canvas_find_text").value;
                if (t !== "") {
                    if (t === last_search_term) {
                        pdgui.pdsend(name, "findagain");
                    } else {
                        pdgui.pdsend(name, "find",
                        encode_for_dialog(t),
                        match_words_state ? "1" : "0");
                    }
                }
                last_search_term = t;
            },
            find_keydown: function(evt) {
                if (evt.keyCode === 13) {
                    events.find_click(evt);
                }
            },
            scalar_draggable_mousemove: function(evt) {
                var new_x = evt.pageX,
                    new_y = evt.pageY;
                var obj = scalar_draggables[draggable_elem.id];
                pdgui.pdsend(obj.cid, "scalar_event", obj.scalar_sym, 
                    obj.drawcommand_sym, obj.event_name, new_x - last_draggable_x,
                    new_y - last_draggable_y);
                last_draggable_x = new_x;
                last_draggable_y = new_y;
            },
            scalar_draggable_mouseup: function(evt) {
                canvas_events.normal();
            }
        },
        utils = {
            create_obj: function() {
                var fudi_msg = text_to_fudi(textbox().innerText);
                pdgui.pdsend(name, "createobj", fudi_msg);
                //pdgui.post("formatted content is " + fudi_msg);
            },
            set_obj: function() {
                var fudi_msg = text_to_fudi(textbox().innerText);
                pdgui.pdsend(name, "setobj", fudi_msg);
                //pdgui.post("formatted content is " + fudi_msg);
            }
        }
    ;

    // Dialog events -- these are set elsewhere now because of a bug
    // with nwworkingdir
    document.querySelector("#saveDialog").addEventListener("change",
        function(evt) {
            pdgui.saveas_callback(name, this.value);
            // reset value so that we can open the same file twice
            this.value = null;
            console.log("tried to open something");
        }, false
    );
    document.querySelector("#fileDialog").addEventListener("change",
        function(evt) {
            var file_array = this.value;
            // reset value so that we can open the same file twice
            this.value = null;
            pdgui.menu_open(file_array);
            console.log("tried to open something");
        }, false
    );
    document.querySelector("#openpanel_dialog").addEventListener("change",
        function(evt) {
            var file_string = this.value;
            // reset value so that we can open the same file twice
            this.value = null;
            pdgui.file_dialog_callback(file_string);
            console.log("tried to openpanel something");
        }, false
    );
    document.querySelector("#savepanel_dialog").addEventListener("change",
        function(evt) {
            var file_string = this.value;
            // reset value so that we can open the same file twice
            this.value = null;
            pdgui.file_dialog_callback(file_string);
            console.log("tried to savepanel something");
        }, false
    );
    document.querySelector("#canvas_find_text").addEventListener("focusin",
        canvas_find_focus, false
    );
    document.querySelector("#canvas_find_text").addEventListener("blur",
        canvas_find_blur, false
    );
    document.querySelector("#canvas_find_button").addEventListener("click",
        events.find_click);
    // closing the Window
    // this isn't actually closing the window yet
    gui.Window.get().on("close", function() {
        pdgui.pdsend(name, "menuclose 0");
    });
    // update viewport size when window size changes
    gui.Window.get().on("maximize", function() {
        pdgui.gui_canvas_getscroll(name);
    });
    gui.Window.get().on("unmaximize", function() {
        pdgui.gui_canvas_getscroll(name);
    });
    gui.Window.get().on("resize", function() {
        pdgui.gui_canvas_getscroll(name);
    });
    // set minimum window size
    gui.Window.get().setMinimumSize(150, 100); 

    return {
        none: function() {
            var name;
            if (state !== "none") {
                previous_state = state;
            }
            state = "none";
            for (var prop in events) {
                if (events.hasOwnProperty(prop)) {
                    name = prop.split("_");
                    name = name[name.length -1];
                    document.removeEventListener(name, events[prop], false);
                }
            }
        },
        normal: function() {
            this.none();

            document.addEventListener("mousemove", events.mousemove, false);
            document.addEventListener("keydown", events.keydown, false);
            document.addEventListener("keypress", events.keypress, false);
            document.addEventListener("keyup", events.keyup, false);
            document.addEventListener("mousedown", events.mousedown, false);
            document.addEventListener("mouseup", events.mouseup, false);
            state = "normal";
            set_edit_menu_modals(true);
        },
        scalar_drag: function() {
            // This scalar_drag is a prototype for moving more of the editing environment 
            // directly to the GUI.  At the moment we're leaving the other "normal" 
            // events live, since behavior like editmode selection still happens from
            // the Pd engine.
            //this.none();
            document.addEventListener("mousemove", events.scalar_draggable_mousemove, false);
            document.addEventListener("mouseup", events.scalar_draggable_mouseup, false);
        },
        text: function() {
            this.none();

            document.addEventListener("mousemove", events.text_mousemove, false);
            document.addEventListener("keydown", events.text_keydown, false);
            document.addEventListener("keypress", events.text_keypress, false);
            document.addEventListener("keyup", events.text_keyup, false);
            document.addEventListener("mousedown", events.text_mousedown, false);
            document.addEventListener("mouseup", events.text_mouseup, false);
            state = "text";
            set_edit_menu_modals(false);
        },
        floating_text: function() {
            this.none();
            this.text();
            document.removeEventListener("mousedown", events.text_mousedown, false);
            document.removeEventListener("mouseup", events.text_mouseup, false);
            document.removeEventListener("keypress", events.text_keypress, false);
            document.removeEventListener("mousemove", events.text_mousemove, false);
            document.addEventListener("click", events.floating_text_click, false);
            document.addEventListener("keypress", events.floating_text_keypress, false);
            document.addEventListener("mousemove", events.mousemove, false);
            state = "floating_text";
            set_edit_menu_modals(false);
        },
        search: function() {
            this.none();
            document.addEventListener("keydown", events.find_keydown, false);
            state = "search";
        },
        register: function(n) {
            name = n;
        },
        get_state: function() {
            return state;
        },
        get_previous_state: function() {
            return previous_state;
        },
        set_obj: function() {
            utils.set_obj();
        },
        create_obj: function() {
            utils.create_obj();
        },
        match_words: function(state) {
            match_words_state = state;
        },
        add_scalar_draggable: function(cid, tag, scalar_sym, drawcommand_sym,
            event_name) {
            scalar_draggables[tag] = {
                cid: cid,
                scalar_sym: scalar_sym,
                drawcommand_sym, drawcommand_sym,
                event_name: event_name
            };
        },
        remove_scalar_draggable: function(id) {
            if (scalar_draggables[id]) {
                scalar_draggables[id] = null;
            }
        }
    }
}());

// This gets called from the nw_create_window function in index.html
// It provides us with our canvas id from the C side.  Once we have it
// we can create the menu and register event callbacks
function register_canvas_id(cid) {
    name = cid; // hack
    // We create the window menus and popup menu before doing anything else
    // to ensure that we don't try to set the svg size before these are done.
    // Otherwise we might set the svg size to the window viewport, only to have
    // the menu push down the svg viewport and create scrollbars. Those same
    // scrollbars will get erased once canvas_map triggers, causing a quick
    // (and annoying) scrollbar flash
    nw_create_patch_window_menus(cid);
    create_popup_menu(cid);
    canvas_events.register(cid);
    canvas_events.normal();
    pdgui.canvas_map(cid); // side-effect: triggers gui_canvas_getscroll from Pd
}

function create_popup_menu(name) {
    // The right-click popup menu
    var popup_menu = new gui.Menu();
    pdgui.add_popup(name, popup_menu);

    popup_menu.append(new gui.MenuItem({
        label: "Properties",
        click: function() {
            pdgui.popup_action(name, 0);
        }
    }));
    popup_menu.append(new gui.MenuItem({
        label: "Open",
        click: function() {
            pdgui.popup_action(name, 1);
        }
    }));
    popup_menu.append(new gui.MenuItem({
        label: "Help",
        click: function() {
            pdgui.popup_action(name, 2);
        }
    }));
}

// stop-gap
function menu_generic () {
    alert("Please implement this");
}

var modals = {}; // Edit menu items that should be disabled when editing
                 // an object box

function set_edit_menu_modals(state) {
    var item;
    for (item in modals) {
        if (modals.hasOwnProperty(item)) {
            modals[item].enabled = state; 
        }
    }
}

function nw_undo_menu(undo_text, redo_text) {
    if (undo_text === "no") {
        modals.undo.enabled = false;
    } else {
        modals.undo.enabled = true;
        modals.undo.label = l("menu.undo") + " " + undo_text;
    }
    if (redo_text === "no") {
        modals.redo.enabled = false;
    } else {
        modals.redo.enabled = true;
        modals.redo.label = l("menu.redo") + " " + redo_text;
    }
}

function have_live_box() {
    var state = canvas_events.get_state();
    if (state === "text" || state === "floating_text") {
        return true;
    } else {
        return false;
    }
}

// If there's a box being edited, send the box's text to Pd
function update_live_box() {
    if (have_live_box()) {
        canvas_events.set_obj();
    }
}

// If there's a box being edited, try to instantiate it in Pd
function instantiate_live_box() {
    if (have_live_box()) {
        canvas_events.create_obj();
    }
}

// Menus for the Patch window
function nw_create_patch_window_menus(name) {

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
        tooltip: l("menu.new_tt")
    }));

    fileMenu.append(new gui.MenuItem({
        label: l("menu.open"),
        key: "o",
        modifiers: "ctrl",
        tooltip: l("menu.open_tt"),
        click: function() {
            var input, chooser,
                span = document.querySelector("#fileDialogSpan");
            input = pdgui.build_file_dialog_string({
                id: "fileDialog",
                nwworkingdir: "/user/home",
                style: "display: none;",
                type: "file",
            });
            span.innerHTML = input;
            chooser = document.querySelector("#fileDialog");
            chooser.click();
        }
    }));

    if (pdgui.k12_mode == 1) {
        fileMenu.append(new gui.MenuItem({
        label: l("menu.k12_demos"),
        tooltip: l("menu.k12_demos_tt"),
        click: pdgui.menu_k12_open_demos
        }));
    }

    fileMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    // Note: this must be different for the main Pd window
    fileMenu.append(new gui.MenuItem({
        label: l("menu.save"),
        click: function () {
            pdgui.canvas_check_geometry(name); // should this go in menu_save?
            pdgui.menu_save(name);
        },
        key: "s",
        modifiers: "ctrl",
        tooltip: l("menu.save_tt")
    }));

    fileMenu.append(new gui.MenuItem({
        label: l("menu.saveas"),
        click: function (){
            pdgui.canvas_check_geometry(name);
            pdgui.menu_saveas(name);
        },
        key: "s",
        modifiers: "ctrl+shift",
        tooltip: l("menu.saveas_tt")
    }));

    if (pdgui.k12_mode == 0) {
        fileMenu.append(new gui.MenuItem({
            type: "separator"
        }));
    }

    fileMenu.append(new gui.MenuItem({
        label: l("menu.message"),
        click: function() {
            pdgui.menu_send(name);
        },
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

    fileMenu.append(new gui.MenuItem({
        label: l("menu.close"),
        tooltip: l("menu.close_tt"),
        click: function() {
            pdgui.menu_close(name);
        },
        key: "w",
        modifiers: "ctrl"
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
    editMenu.append(modals.undo = new gui.MenuItem({
        label: l("menu.undo"),
        click: function () {
            pdgui.pdsend(name, "undo");
        },
        tooltip: l("menu.undo_tt"),
    }));

    editMenu.append(modals.redo = new gui.MenuItem({
        label: l("menu.redo"),
        click: function () {
            pdgui.pdsend(name, "redo");
        },
        tooltip: l("menu.redo_tt"),
    }));

    editMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    editMenu.append(modals.cut = new gui.MenuItem({
        label: l("menu.cut"),
        click: function () {
            pdgui.pdsend(name, "cut");
        },
        tooltip: l("menu.cut_tt"),
    }));

    editMenu.append(modals.copy = new gui.MenuItem({
        label: l("menu.copy"),
        click: function () {
            pdgui.pdsend(name, "copy");
        },
        tooltip: l("menu.copy_tt"),
    }));

    editMenu.append(modals.paste = new gui.MenuItem({
        label: l("menu.paste"),
        click: function () {
            pdgui.pdsend(name, "paste");
        },
        tooltip: l("menu.paste_tt"),
    }));

    editMenu.append(new gui.MenuItem({
        label:  l("menu.duplicate"),
        click: function () {
            pdgui.pdsend(name, "duplicate");
        },
        key: "d",
        modifiers: "ctrl",
        tooltip: l("menu.duplicate_tt")
    }));

    editMenu.append(modals.selectall = new gui.MenuItem({
        label: l("menu.selectall"),
        click: function (evt) {
            if (canvas_events.get_state() === "normal") {
                pdgui.pdsend(name, "selectall");
            }
        },
        tooltip: l("menu.selectall_tt"),
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.reselect"),
        // Unfortunately nw.js doesn't allow
        // key: "Return" or key: "Enter", so we
        // can't bind to ctrl-Enter here. (Even
        // tried fromCharCode...)
        click: function () {
            pdgui.pdsend(name, "reselect");
        },
        key: String.fromCharCode(10),
        modifiers: "ctrl",
        tooltip: l("menu.reselect_tt")
    }));

    editMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.zoomin"),
        click: function () {
            var z = gui.Window.get().zoomLevel;
            if (z < 8) { z++; }
            gui.Window.get().zoomLevel = z;
            pdgui.post("zoom level is " + gui.Window.get().zoomLevel);
        },
        key: "=",
        modifiers: "ctrl",
        tooltip: l("menu.zoomin")
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.zoomout"),
        click: function () {
            var z = gui.Window.get().zoomLevel;
            if (z > -7) { z--; } 
            gui.Window.get().zoomLevel = z;
            pdgui.post("zoom level is " + gui.Window.get().zoomLevel);
        },
        key: "-",
        modifiers: "ctrl",
        tooltip: l("menu.zoomout_tt"),
    }));

    editMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.tidyup"),
        click: function() {
            pdgui.pdsend(name, "tidy");
        },
        key: "y",
        modifiers: "ctrl",
        tooltip: l("menu.tidyup_tt")
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.tofront"),
        click: function() {
            pdgui.popup_action(name, 3);
        },
        tooltip: l("menu.tofront_tt"),
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.toback"),
        click: function() {
            pdgui.popup_action(name, 4);
        },
        tooltip: l("menu.toback_tt"),
    }));

    editMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.font"),
        click: function () {
            pdgui.pdsend(name, "menufont");
        },
        tooltip: l("menu.font_tt"),
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.cordinspector"),
        click: function() {
            pdgui.pdsend(name, "magicglass 0");
        },
        key: "r",
        modifiers: "ctrl",
        tooltip: l("menu.cordinspector_tt"),
    }));

    editMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.find"),
        click: function () {
            var find_bar = document.getElementById("canvas_find"),
                find_bar_text = document.getElementById("canvas_find_text"),
                state = find_bar.style.getPropertyValue("display");
            // if there's a box being edited, try to instantiate it in Pd
            instantiate_live_box();
            if (state === "none") {
                find_bar.style.setProperty("display", "inline");
                find_bar_text.focus();
                find_bar_text.select();
                canvas_events.search();
            } else {
                find_bar.style.setProperty("display", "none");
                // "normal" seems to be the only viable state for the
                // canvas atm.  But if there are other states added later,
                // we might need to fetch the previous state here.
                canvas_events.normal();
            }
        },
        key: "f",
        modifiers: "ctrl",
        tooltip: l("menu.find_tt"),
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.findagain"),
        click: menu_generic,
        key: "g",
        modifiers: "ctrl",
        tooltip: l("menu.findagain")
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.finderror"),
        click: function() {
            pdgui.pdsend("pd finderror");
        },
        tooltip: l("menu.finderror_tt"),
    }));

    editMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.autotips"),
        click: menu_generic,
        tooltip: l("menu.autotips_tt"),
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.editmode"),
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "editmode 0");
        },
        key: "e",
        modifiers: "ctrl",
        tooltip: l("menu.editmode_tt")
    }));

    editMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.preferences"),
        click: pdgui.open_prefs,
        key: "p",
        modifiers: "ctrl",
        tooltip: l("menu.preferences_tt")
    }));

    // Put menu
    var putMenu = new gui.Menu();

    // Add to window menu
    windowMenu.append(new gui.MenuItem({
    label: l("menu.put"),
    submenu: putMenu
    }));

    // Put menu sub-entries
    putMenu.append(new gui.MenuItem({
        label: l("menu.object"),
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "obj 0");
        },
        key: "1",
        modifiers: "ctrl",
        tooltip: l("menu.object_tt"),
    }));

    putMenu.append(new gui.MenuItem({
        label: l("menu.msgbox"),
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "msg 0");
        },
        key: "2",
        modifiers: "ctrl",
        tooltip: l("menu.msgbox_tt"),
    }));

    putMenu.append(new gui.MenuItem({
        label: l("menu.number"),
        click: function() { 
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "floatatom 0");
        },
        key: "3",
        modifiers: "ctrl",
        tooltip: l("menu.number_tt")
    }));

    putMenu.append(new gui.MenuItem({
        label: l("menu.symbol"),
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "symbolatom 0");
        },
        key: "4",
        modifiers: "ctrl",
        tooltip: l("menu.symbol_tt")
    }));

    putMenu.append(new gui.MenuItem({
        label: l("menu.comment"),
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "text 0");
        },
        key: "5",
        modifiers: "ctrl",
        tooltip: l("menu.comment_tt")
    }));

    putMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    putMenu.append(new gui.MenuItem({
        label: l("menu.bang"),
        click: function(e) {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "bng 0");
        },
        key: "b",
        modifiers: "ctrl-shift",
        tooltip: l("menu.bang_tt")
    }));

    putMenu.append(new gui.MenuItem({
        label: l("menu.toggle"),
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "toggle 0");
        },
        key: "t",
        modifiers: "ctrl-shift",
        tooltip: l("menu.toggle_tt")
    }));

    putMenu.append(new gui.MenuItem({
        label: l("menu.number2"),
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "numbox 0");
        },
        key: "n",
        modifiers: "ctrl-shift",
        tooltip: l("menu.number2")
    }));

    putMenu.append(new gui.MenuItem({
        label: l("menu.vslider"),
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "vslider 0");
        },
        key: "v",
        modifiers: "ctrl-shift",
        tooltip: l("menu.vslider_tt"),
    }));

    putMenu.append(new gui.MenuItem({
        label: l("menu.hslider"),
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "hslider 0");
        },
        key: "h",
        modifiers: "ctrl-shift",
        tooltip: l("menu.hslider_tt"),
    }));

    putMenu.append(new gui.MenuItem({
        label: l("menu.vradio"),
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "vradio 0");
        },
        key: "d",
        modifiers: "ctrl-shift",
        tooltip: l("menu.vradio_tt"),
    }));

    putMenu.append(new gui.MenuItem({
        label: l("menu.hradio"),
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "hradio 0");
        },
        key: "i",
        modifiers: "ctrl",
        tooltip: l("menu.hradio_tt"),
    }));

    putMenu.append(new gui.MenuItem({
        label: l("menu.vu"),
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "vumeter 0");
        },
        key: "u",
        modifiers: "ctrl",
        tooltip: l("menu.vu_tt"),
    }));

    putMenu.append(new gui.MenuItem({
        label: l("menu.cnv"),
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "mycnv 0");
        },
        key: "c",
        modifiers: "ctrl-shift",
        tooltip: l("menu.cnv_tt")
    }));

    putMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    //putMenu.append(new gui.MenuItem({
    //    label: l("menu.graph"),
    //    click: function() {
    //        update_live_box();
    //        pdgui.pdsend(name, "dirty 1");
    //        // leaving out some placement logic... see pd.tk menu_graph
    //        pdgui.pdsend(name, "graph NULL 0 0 0 0 30 30 0 30");
    //    },
    //    tooltip: l("menu.graph_tt"),
    //}));

    putMenu.append(new gui.MenuItem({
        label: l("menu.array"),
        click: function() {
                update_live_box();
                pdgui.pdsend(name, "dirty 1");
                pdgui.pdsend(name, "menuarray");
            },
        tooltip: l("menu.array_tt"),
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
        label: l("menu.fullscreen"),
        click: function() {
            var win = gui.Window.get();
            var fullscreen = win.isFullscreen;
            win.isFullscreen = !fullscreen;
            pdgui.post("fullscreen is " + fullscreen);
        },
        key: "f11",
        //modifiers: "ctrl",
        tooltip: l("menu.nextwin_tt"),
    }));

    winmanMenu.append(new gui.MenuItem({
        label: l("menu.nextwin"),
        click: function() {
            pdgui.raise_next(name);
        },
        key: String.fromCharCode(12), // Page down
        modifiers: "ctrl",
        tooltip: l("menu.nextwin_tt"),
    }));

    winmanMenu.append(new gui.MenuItem({
        label: l("menu.prevwin"),
        click: function() {
            pdgui.raise_prev(name);
        },
        key: String.fromCharCode(11), // Page up
        modifiers: "ctrl",
        tooltip: l("menu.prevwin_tt"),
    }));

    winmanMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    winmanMenu.append(new gui.MenuItem({
        label: l("menu.parentwin"),
        click: function() {
            pdgui.pdsend(name, "findparent", 0);
        },
        tooltip: l("menu.parentwin_tt"),
    }));

    winmanMenu.append(new gui.MenuItem({
        label: l("menu.visible_ancestor"),
        click: function() {
            pdgui.pdsend(name, "findparent", 1);
        },
        tooltip: l("menu.visible_ancestor_tt"),
    }));

    winmanMenu.append(new gui.MenuItem({
        label: l("menu.pdwin"),
        click: function() {
            pdgui.raise_pd_window();
        },
        tooltip: l("menu.pdwin_tt"),
        key: "r",
        modifiers: "ctrl"
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
        tooltip: l("menu.audio_on_tt"),
    }));

    mediaMenu.append(new gui.MenuItem({
        label: l("menu.audio_off"),
        click: function() {
            pdgui.pdsend("pd dsp 0");
        },
        key: ".",
        modifiers: "ctrl",
        tooltip: l("menu.audio_off_tt"),
    }));

    mediaMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    mediaMenu.append(new gui.MenuItem({
        label: l("menu.test"),
        click: function() {
            pdgui.pd_doc_open("doc/7.stuff/tools", "testtone.pd");
        },
        tooltip: l("menu.test_tt"),
    }));

    mediaMenu.append(new gui.MenuItem({
        label: l("menu.loadmeter"),
        click: function() {
            pdgui.pd_doc_open("doc/7.stuff/tools", "load-meter.pd");
        },
        tooltip: l("menu.loadmeter_tt"),
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
        click: menu_generic,
        //key: "c",
        //modifiers: "ctrl",
        tooltip: l("menu.about_tt"),
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.manual"),
        click: menu_generic,
        tooltip: l("menu.manual"),
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.browser"),
        click: menu_generic,
        tooltip: l("menu.browser_tt"),
    }));

    helpMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.l2ork_list"),
        click: menu_generic,
        tooltip: l("menu.l2ork_list_tt"),
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.pd_list"),
        click: menu_generic,
        tooltip: l("menu.pd_list_tt"),
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.forums"),
        click: menu_generic,
        tooltip: l("menu.forums_tt"),
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.irc"),
        click: menu_generic,
        tooltip: l("menu.irc_tt"),
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.devtools"),
        key: "b", // temporary convenience shortcut-- can change if needed
        modifiers: "ctrl",
        click: function () {
            gui.Window.get().showDevTools();
        },
        tooltip: l("menu.devtools_tt"),
    }));

    // Assign to window
    gui.Window.get().menu = windowMenu;

}
