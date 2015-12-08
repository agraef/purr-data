"use strict";

var gui = require("nw.gui"); 
var pdgui = require("./pdgui.js");
var pd_menus = require("./pd_menus.js");

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

function nw_window_focus_callback() {
    pdgui.post("window was focused");
    // on OSX, update the menu on focus
    if (process.platform === "darwin") {
        nw_create_patch_window_menus(gui, window, name);
    }
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

function cmd_or_ctrl_key(evt) {
    if (process.platform === "darwin") {
        return evt.metaKey;
    } else {
        return evt.ctrlKey;
    }
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
        keydown_autorepeat = false,
        svg_view = document.getElementById("patchsvg").viewBox.baseVal,
        textbox = function () {
            return document.getElementById("new_object_textentry");
        },
        events = {
            mousemove: function(evt) {
                //pdgui.post("x: " + evt.pageX + " y: " + evt.pageY +
                //    " modifier: " + (evt.shiftKey + (cmd_or_ctrl_key(evt) << 1)));
                pdgui.pdsend(name, "motion",
                    (evt.pageX + svg_view.x),
                    (evt.pageY + svg_view.y),
                    (evt.shiftKey + (cmd_or_ctrl_key(evt) << 1))
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
                if (b === 3 || (process.platform === "darwin" && evt.ctrlKey)) {
                    // right-click
                    mod = 8;
                } else {
                    mod = (evt.shiftKey + (cmd_or_ctrl_key(evt) << 1)); 
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
                var key_code = evt.keyCode,
                    hack = null; // hack for unprintable ascii codes
                keydown_autorepeat = evt.repeat;
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
                        if (cmd_or_ctrl_key(evt)) {
                            pdgui.pdsend(name, "selectall");
                            hack = 0; // not sure what to report here...
                        }
                        break;
                    case 88:
                        if (cmd_or_ctrl_key(evt)) {
                            pdgui.pdsend(name, "cut");
                            hack = 0; // not sure what to report here...
                        }
                        break;
                    case 67:
                        if (cmd_or_ctrl_key(evt)) {
                            pdgui.pdsend(name, "copy");
                            hack = 0; // not sure what to report here...
                        }
                        break;
                    case 86:
                        if (cmd_or_ctrl_key(evt)) {
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
                    pdgui.canvas_sendkey(name, 1, evt, hack, keydown_autorepeat);
                    pdgui.set_keymap(key_code, hack);
                }

                //pdgui.post("keydown time: keycode is " + evt.keyCode);
                last_keydown = evt.keyCode;
                //evt.stopPropagation();
                //evt.preventDefault();
            },
            keypress: function(evt) {
                // Hack to handle undo/redo shortcuts. Other menu shortcuts are
                // in pd_menus. It'd be best to have a JSON file called
                // pd_shortcuts.js so we can keep them all in a central
                // location, but that's a bigger project.
                if (evt.charCode === 26) {
                    if (cmd_or_ctrl_key(evt)) {
                        if (evt.shiftKey === true) { // ctrl-Shift-z
                            pdgui.pdsend(name, "redo");
                        } else { // ctrl-z
                            pdgui.pdsend(name, "undo");
                        }
                        return;
                    }
                }
                pdgui.canvas_sendkey(name, 1, evt, evt.charCode,
                    keydown_autorepeat);
                pdgui.set_keymap(last_keydown, evt.charCode, keydown_autorepeat);
                //pdgui.post("keypress time: charcode is " + evt.charCode);
                // Don't do things like scrolling on space, arrow keys, etc.
                //evt.stopPropagation();
                evt.preventDefault();
            },
            keyup: function(evt) {
                var my_char_code = pdgui.get_char_code(evt.keyCode);
                pdgui.canvas_sendkey(name, 0, evt, my_char_code, evt.repeat);
                //pdgui.post("keyup time: charcode is: " + my_char_code);
                if (evt.keyCode === 13 && cmd_or_ctrl_key(evt)) {
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
                // ctrl-Enter to instantiate object
                if (evt.keyCode === 13 && cmd_or_ctrl_key(evt)) {
                    canvas_events.text(); // anchor the object
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
            console.log("tried to save something");
        }, false
    );

    // The following is commented out because we have to set the
    // event listener inside nw_create_pd_window_menus due to a
    // bug with nwworkingdir

    //document.querySelector("#fileDialog").addEventListener("change",
    //    function(evt) {
    //        var file_array = this.value;
    //        // reset value so that we can open the same file twice
    //        this.value = null;
    //        pdgui.menu_open(file_array);
    //        console.log("tried to open something\n\n\n\n\n\n\n\n");
    //    }, false
    //);
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
    // We need to separate these into nw_window events and html5 DOM events
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
    gui.Window.get().on("focus", function() {
        nw_window_focus_callback();
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
                    name = name[name.length - 1];
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
    // (and annoying) scrollbar flash.
    // For OSX, we have a single menu and just track which window has the
    // focus.
    if (process.platform !== "darwin") {
        nw_create_patch_window_menus(gui, window, cid);
    }
    create_popup_menu(cid);
    canvas_events.register(cid);
    // Trigger a "focus" event so that OSX updates the menu for this window
    nw_window_focus_callback();
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

function nw_undo_menu(undo_text, redo_text) {
    if (undo_text === "no") {
        canvas_menu.edit.undo.enabled = false;
    } else {
        canvas_menu.edit.undo.enabled = true;
        canvas_menu.edit.undo.label = l("menu.undo") + " " + undo_text;
    }
    if (redo_text === "no") {
        canvas_menu.edit.redo.enabled = false;
    } else {
        canvas_menu.edit.redo.enabled = true;
        canvas_menu.edit.redo.label = l("menu.redo") + " " + redo_text;
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

var canvas_menu = {};

function set_edit_menu_modals(state) {
    canvas_menu.edit.undo.enabled = state;
    canvas_menu.edit.redo.enabled = state;
    canvas_menu.edit.cut.enabled = state;
    canvas_menu.edit.copy.enabled = state;
    canvas_menu.edit.paste.enabled = state;
}

// stop-gap
function menu_generic () {
    alert("Please implement this");
}

function minit(menu_item, options) {
    var o;
    for (o in options) {
        if (options.hasOwnProperty(o)) {
            menu_item[o] = options[o];
        }
    }
}

function nw_create_patch_window_menus(gui, w, name) {
    // if we're on GNU/Linux or Windows, create the menus:
    var m = canvas_menu = pd_menus.create_menu(gui);

    // File sub-entries
    // We explicitly enable these menu items because on OSX
    // the console menu disables them. (Same for Edit and Put menu)
    minit(m.file.new_file, { click: pdgui.menu_new });
    minit(m.file.open, {
        click: function() {
            var input, chooser,
                span = w.document.querySelector("#fileDialogSpan");
            input = pdgui.build_file_dialog_string({
                style: "display: none;",
                type: "file",
                id: "fileDialog",
                nwworkingdir: "/user/home",
                multiple: null,
                accept: ".pd,.pat,.mxt,.mxb,.help"
            });
            span.innerHTML = input;
            chooser = w.document.querySelector("#fileDialog");
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
    minit(m.file.save, {
        enabled: true,
        click: function () {
            pdgui.canvas_check_geometry(name); // should this go in menu_save?
            pdgui.menu_save(name);
        },
    });
    minit(m.file.saveas, {
        enabled: true,
        click: function (){
            pdgui.canvas_check_geometry(name);
            pdgui.menu_saveas(name);
        },
    });
    minit(m.file.message, {
        click: function() { pdgui.menu_send(name); }
    });
    minit(m.file.close, {
        enabled: true,
        click: function() { pdgui.menu_close(name); }
    });
    minit(m.file.quit, {
        click: pdgui.menu_quit,
    });

    // Edit menu
    minit(m.edit.undo, {
        enabled: true,
        click: function () { pdgui.pdsend(name, "undo"); }
    });
    minit(m.edit.redo, {
        enabled: true,
        click: function () { pdgui.pdsend(name, "redo"); }
    });
    minit(m.edit.cut, {
        enabled: true,
        click: function () { pdgui.pdsend(name, "cut"); }
    });
    minit(m.edit.copy, {
        enabled: true,
        click: function () { pdgui.pdsend(name, "copy"); }
    });
    minit(m.edit.paste, {
        enabled: true,
        click: function () { pdgui.pdsend(name, "paste"); }
    });
    minit(m.edit.duplicate, {
        enabled: true,
        click: function () { pdgui.pdsend(name, "duplicate"); }
    });
    minit(m.edit.selectall, {
        enabled: true,
        click: function (evt) {
            if (canvas_events.get_state() === "normal") {
                pdgui.pdsend(name, "selectall");
            }
        }
    });
    minit(m.edit.reselect, {
        enabled: true,
        click: function () { pdgui.pdsend(name, "reselect"); }
    });
    minit(m.edit.tidyup, {
        enabled: true,
        click: function() { pdgui.pdsend(name, "tidy"); }
    });
    minit(m.edit.tofront, {
        enabled: true,
        click: function() { pdgui.popup_action(name, 3); }
    });
    minit(m.edit.toback, {
        enabled: true,
        click: function() { pdgui.popup_action(name, 4); }
    });
    minit(m.edit.font, {
        enabled: true,
        click: function () { pdgui.pdsend(name, "menufont"); }
    });
    minit(m.edit.cordinspector, {
        enabled: true,
        click: function() { pdgui.pdsend(name, "magicglass 0"); }
    });
    minit(m.edit.find, {
        click: function () {
            var find_bar = w.document.getElementById("canvas_find"),
                find_bar_text = w.document.getElementById("canvas_find_text"),
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
        }
    });
    minit(m.edit.findagain, {
        enabled: true,
        click: function() {
            pdgui.pdsend(name, "findagain");
        }
    });
    minit(m.edit.finderror, {
        enabled: true,
        click: function() {
            pdgui.pdsend("pd finderror");
        }
    });
    minit(m.edit.autotips, {
        enabled: true,
        click: menu_generic
    });
    minit(m.edit.editmode, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "editmode 0");
        }
    });
    minit(m.edit.preferences, {
        click: pdgui.open_prefs
    });

    // View menu
    minit(m.view.zoomin, {
        enabled: true,
        click: function () {
            var z = gui.Window.get().zoomLevel;
            if (z < 8) { z++; }
            gui.Window.get().zoomLevel = z;
        }
    });
    minit(m.view.zoomout, {
        enabled: true,
        click: function () {
            var z = gui.Window.get().zoomLevel;
            if (z > -7) { z--; } 
            gui.Window.get().zoomLevel = z;
        }
    });
    minit(m.view.zoomreset, {
        enabled: true,
        click: function () {
            gui.Window.get().zoomLevel = 0;
        }
    });
    minit(m.view.fullscreen, {
        click: function() {
            var win = gui.Window.get();
            var fullscreen = win.isFullscreen;
            win.isFullscreen = !fullscreen;
            pdgui.post("fullscreen is " + fullscreen);
        }
    });

    // Put menu
    minit(m.put.object, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "obj 0");
        }
    });
    minit(m.put.message, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "msg 0");
        }
    });
    minit(m.put.number, {
        enabled: true,
        click: function() { 
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "floatatom 0");
        }
    });
    minit(m.put.symbol, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "symbolatom 0");
        }
    });
    minit(m.put.comment, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "text 0");
        }
    });
    minit(m.put.bang, {
        enabled: true,
        click: function(e) {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "bng 0");
        }
    });
    minit(m.put.toggle, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "toggle 0");
        }
    });
    minit(m.put.number2, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "numbox 0");
        }
    });
    minit(m.put.vslider, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "vslider 0");
        }
    });
    minit(m.put.hslider, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "hslider 0");
        }
    });
    minit(m.put.vradio, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "vradio 0");
        }
    });
    minit(m.put.hradio, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "hradio 0");
        }
    });
    minit(m.put.vu, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "vumeter 0");
        }
    });
    minit(m.put.cnv, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "mycnv 0");
        }
    });
    //minit(m.put.graph, {
    //    enabled: true,
    //    click: function() {
    //        update_live_box();
    //        pdgui.pdsend(name, "dirty 1");
    //        // leaving out some placement logic... see pd.tk menu_graph
    //        pdgui.pdsend(name, "graph NULL 0 0 0 0 30 30 0 30");
    //    },
    //});
    minit(m.put.array, {
        enabled: true,
        click: function() {
                update_live_box();
                pdgui.pdsend(name, "dirty 1");
                pdgui.pdsend(name, "menuarray");
            }
    });

    // Window
    minit(m.win.nextwin, {
        click: function() {
            pdgui.raise_next(name);
        }
    });
    minit(m.win.prevwin, {
        click: function() {
            pdgui.raise_prev(name);
        }
    });
    minit(m.win.parentwin, {
        enabled: true,
        click: function() {
            pdgui.pdsend(name, "findparent", 0);
        }
    });
    minit(m.win.visible_ancestor, {
        enabled: true,
        click: function() {
            pdgui.pdsend(name, "findparent", 1);
        }
    });
    minit(m.win.pdwin, {
        enabled: true,
        click: function() {
            pdgui.raise_pd_window();
        }
    });

    // Media menu
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

    // Help menu
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
        click: menu_generic
    });
    minit(m.help.devtools, {
        click: function () {
            gui.Window.get().showDevTools();
        }
    });
}
