"use strict";

var gui = require("nw.gui");
var pdgui = require("./pdgui.js");
var pd_menus = require("./pd_menus.js");

// Apply gui preset to this canvas
pdgui.skin.apply(window);

//var name = pdgui.last_loaded();

var l = pdgui.get_local_string;

function nw_window_focus_callback(name) {
    pdgui.set_focused_patchwin(name);
    // on OSX, update the menu on focus
    if (process.platform === "darwin") {
        nw_create_patch_window_menus(gui, window, canvas_events.get_id());
    }
}

function nw_window_blur_callback(name) {
    // Fake a mouseup event to keep from getting a dangling selection box
    if (canvas_events.get_state() === "normal") {
        pdgui.pdsend(name, "mouseup_fake");
    }
}

var canvas_events = (function() {
    var name,
        state,
        scalar_draggables = {}, // scalar child with the "drag" event enabled
        draggable_elem,         // last scalar we dragged
        draggable_label,        // kluge for handling [cnv] label/size anchors
        last_draggable_x,       // last x coord for the element we're dragging
        last_draggable_y,       // last y
        previous_state = "none", /* last state, excluding explicit 'none' */
        match_words_state = false,
        last_dropdown_menu_x,
        last_dropdown_menu_y,
        last_search_term = "",
        svg_view = document.getElementById("patchsvg").viewBox.baseVal,
        last_results = [], // last completion results (autocomplete)
        last_completed = -1, // last Tab completion (autocomplete)
        last_offset = -1, // offset of last Tab completion (autocomplete)
        last_yanked = "", // last yanked completion (to confirm deletion)
        textbox = function () {
            return document.getElementById("new_object_textentry");
        },
        caret_end = function () {
            /* ag: Move the caret to the end of the texbox while editing. This
               is needed for the autcompletion. We essentially fake pressing
               the End key here; maybe there's an easier way to do this, but
               the following seems to work alright, so... We first grab the
               textbox content and determine its length, which is where we
               want the caret to be. */
            var t = textbox();
            var x = t.innerText;
            var p = x.length;
            //console.log("move "+p+": "+x);
            /* The DOM doesn't make this easy. We first have to define a
               range, set its start to the desired caret position, and
               collapse it to a single position (i.e., end = start). Next we
               grab the current selection, remove all currently selected
               ranges, and set our new range. Quite a hullaballoo for such a
               simple task. */
            var r = document.createRange();
            var s = window.getSelection();
            r.setStart(t.childNodes[0], p);
            r.collapse(true);
            s.removeAllRanges();
            s.addRange(r);
            // Defer this to the event loop to prevent losing the
            // keyboard focus.
            setTimeout(function () { t.focus() }, 0);
        },
        current_events = {}, // keep track of our current listeners
        edit_events = function(elem, events, action, init) {
            // convenience routine for adding an object full of
            // event: callback pairs
            var e;
            for (e in events) {
                if (events.hasOwnProperty(e)) {
                    elem[action + "EventListener"](e, events[e], false);
                    if (action === "remove") {
                        // remove the key
                        delete current_events[e];
                    }
                    else if (!init) {
                        // We shouldn't ever be overwriting an existing
                        // event. So send a warning if we already exist
                        if (current_events[e]) {
                            pdgui.post("Consistency check failed for " +
                                "event " + e + " of element " + elem);
                        }
                        current_events[e] = events[e];
                    }
                }
            }
        },
        init_events = function(elem, events) {
            edit_events(elem, events, "add", true);
        },
        add_events = function(elem, events) {
            // add 
            edit_events(elem, events, "add", false);
        },
        remove_events = function(elem, events) {
            edit_events(elem, events, "remove", false);
        },
        find_scalar_draggable = function (elem) {
            var ret = elem;
            while (ret) {
                if (scalar_draggables[ret.id]) {
                    return ret;
                }
                ret = (ret.parentNode && ret.parentNode.tagName === "g") ?
                    ret.parentNode : null;
            }
            return ret; // returning null if no parent group has a listener
        },
        target_is_scrollbar = function(evt) {
            // Don't send the event to Pd if we click on the scrollbars.
            // This is a bit fragile because we're suppressing on
            // HTMLHtmlElement which is too broad...
            if (evt.target.constructor.name === "HTMLHtmlElement") {
                return 1;
            } else {
                return 0;
            }
        },
        target_is_popup = function(evt) {
            // ag: Check whether the event actually goes to the confirmation
            // popup, in order to prevent special processing in touch events.
            // Like target_is_scrollbar above, this just looks for certain
            // constructor names, which may be fragile.
            if (evt.target.constructor.name == "HTMLButtonElement" ||
                evt.target.constructor.name == "HTMLSpanElement" ||
                evt.target.constructor.name == "HTMLDialogElement") {
                return 1;
            } else {
                return 0;
            }
        },
        target_is_canvas = function(evt) {
            // ag: Here we're looking for canvas touches, so that we can keep
            // scrolling enabled in this case. This check is a bit more
            // specific as we're also testing the parent class, so this is
            // hopefully a bit more robust in case of DOM changes.
            if (evt.target.constructor.name == "SVGSVGElement" &&
                evt.target.parentNode.classList.contains("patch_body")) {
                return 1;
            } else {
                return 0;
            }
        },
        target_is_canvasobj = function(evt) {
            function is_canvas_obj(target) {
                return target.classList.contains("obj") &&
                    target.classList.contains("canvasobj");
            };
            // ag: A bit of (maybe over-)defensive programming here: depending
            // on where exactly the user clicked, the actual object may be the
            // parent or the grandparent of the clicked target.
            if (evt.target.classList.contains("border") ||
                evt.target.classList.contains("box_text"))
                return is_canvas_obj(evt.target.parentNode);
            else if (evt.target.parentNode.classList.contains("box_text"))
                return is_canvas_obj(evt.target.parentNode.parentNode);
            else
                return is_canvas_obj(evt.target);
        },
        target_is_ac_dropdown = function(evt) {
            let ac = ac_dropdown();
            if (!ac) {
                return false;
            }
            let rect = ac.getBoundingClientRect();
            let left = rect.left;
            let right = rect.right;
            let top = rect.top;
            let bottom = rect.bottom;

            if (evt.clientX < left || evt.clientX > right ||
                evt.clientY < top || evt.clientY > bottom) {
                return false;
            }
            return true;
        },
        text_to_normalized_svg_path = function(text) {
            text = text.slice(4).trim()  // draw
                       .slice(4).trim()  // path
                       .slice(1).trim()  // d
                       .slice(1).trim(); // =
            if (text.slice(0, 1) === '"') {
                text = text.slice(1);
            }
            if (text.slice(-1) === '"') {
                text = text.slice(0, -1);
            }
            text = pdgui.parse_svg_path(text);
            return "draw path " + text.reduce(function (prev, curr) {
                return prev.concat(curr)
            }).join(" ");
        },
        text_to_fudi = function(text, obj_class, escapes) {
            if (obj_class !== "comment") {
                // trim whitespace at the beginning and end (unless escaped)
                text = text.replace(/^\s+|(?<!\\)\s+$/g, "");
            }

            // special case for draw path d="arbitrary path string" ...
            if (obj_class === "obj" &&
                text.search(/^draw\s+path\s+d\s*=\s*"/) !== -1) {
                text = text_to_normalized_svg_path(text);
            }

            if (escapes) {
                // regex doesn't really work here, do some straight string
                // processing instead
                var buf = "", quote = false, len = text.length;
                for (var i = 0; i < len; i++) {
                    var c = text[i];
                    if (quote) {
                        buf += "\\"+c;
                        quote = false;
                    } else if (c == "," || c == ";") {
                        // '\;' and '\,' need spaces around them
                        buf += " \\"+c+" ";
                    } else if (c == "$" && /^([0-9]|@)/.test(text.slice(i+1))) {
                        buf += "\\"+c;
                    } else if (c == "\\") {
                        quote = 1;
                        // double escape
                        buf += "\\\\";
                    } else {
                        buf += c;
                    }
                }
                text = buf;
            }

            if (obj_class !== "comment") {
                // filter consecutive ascii32
                text = text.replace(/\u0020+/g, " ");
            }
            return text;
        },
        string_to_array_of_chunks = function(msg) {
        // Convert a string (FUDI message) to an array of strings small enough
        // to fit in Pd's socketreceiver buffer of 4096 bytes
            var chunk_max = 1024,
                max_pd_string = 1000,
                left,
                in_array = [],
                out_array = [];
            if (msg.length <= chunk_max) {
                out_array.push([msg]);
            } else {
                // ag: make sure to exclude \v below since we need these as
                // newline substitutes which survive binbuf treatment
                // split on newlines or (unescaped) spaces
                in_array = msg.split(/(?<!\\) |[\t\n]/);
                while (in_array.length) {
                    left = in_array.slice(); // make a copy of in_array
                    if (left.toString().length > chunk_max) {
                        while (1) {
                            if (left.length < 2) {
                                pdgui.post("Warning: string truncated:");
                                pdgui.post(left.toString().
                                    match(/............................../g).join("\n")
                                );
                                break;
                            }
                            left = left.splice(0, left.length >> 1);
                            if (left.toString().length <= chunk_max) {
                                break;
                            }
                        }
                    }
                    // might need a check here for max_pd_string to warn
                    // user if a string is going to get truncated. (That's
                    // what max_pd_string is for above.)
                    out_array.push(left);
                    in_array = in_array.splice(left.length);
                }
            }
            return out_array;
        },
        might_be_a_pd_file = function(stuff_from_clipboard) {
            // Super-simplistic guess at whether the string from the clipboard
            // starts with Pd code. This is just meant as a convenience so that
            // stuff in the copy buffer that obviously isn't Pd code doesn't get
            // in the way when editing.
            var text = stuff_from_clipboard.trim(),
                one = text.charAt(0),
                two = text.charAt(1);
            return (one === "#" && (two === "N" || two === "X"));
        },
        grow_svg_for_element = function(elem) {
            // See if an element overflows the svg bbox, and
            // enlarge the svg to accommodate it.
            // Note: window.scrollX and window.scrollY might not work
            // with Android Chrome 43 and IE.
            var svg = document.getElementById("patchsvg"),
                elem_bbox = elem.getBoundingClientRect(),
                svg_viewbox = svg.getAttribute("viewBox").split(" "),
                w = Math.max(elem_bbox.left + elem_bbox.width + window.scrollX,
                    svg_viewbox[2]),
                h = Math.max(elem_bbox.top + elem_bbox.height + window.scrollY,
                    svg_viewbox[3]);
            svg.setAttribute("viewBox",
                [Math.min(elem_bbox.left, svg_viewbox[0]),
                 Math.min(elem_bbox.top, svg_viewbox[1]),
                 w,
                 h
                ].join(" "));
            svg.setAttribute("width", w);
            svg.setAttribute("height", h);
        },
        dropdown_index_to_pd = function(elem) {
            var highlighted = elem.querySelector(".highlighted");
            if (highlighted) {
                pdgui.pdsend(elem.getAttribute("data-callback"),
                    highlighted.getAttribute("data-index"));
            }
        },
        dropdown_clear_highlight = function() {
            var container = document.querySelector("#dropdown_list"),
                li_array = container.querySelectorAll("li");
            Array.prototype.forEach.call(li_array, function(e) {
                e.classList.remove("highlighted");
            });
        },
        dropdown_highlight_elem = function(elem, scroll) {
            var container = document.querySelector("#dropdown_list");
            if (!elem.classList.contains("highlighted")) {
                dropdown_clear_highlight();
                elem.classList.add("highlighted");
                // Make sure the highlighted element is in view
                if (scroll) {
                    if (elem.offsetTop < container.scrollTop
                        || elem.offsetTop + elem.offsetHeight >
                            container.scrollTop + container.offsetHeight) {
                            if (scroll === "up") {
                                // we can't usse elem.scrollIntoView() here
                                // because it may also change the scrollbar on
                                // the document, which in turn could change the
                                // pageX/pageY, for the mousemove event, which
                                // in turn would make it impossible for us to
                                // filter out unnecessary mousemove calls
                                //    elem.scrollIntoView();
                            container.scrollTop = elem.offsetTop;
                        } else if (scroll === "down") {
                            container.scrollTop = elem.offsetTop + elem.offsetHeight
                                - container.clientHeight;
                        }
                    }
                }
            }
        },
        ac_dropdown = function() {
            return document.getElementById("autocomplete_dropdown")
        },
        // AG: Little helper function to do all the necessary steps to
        // re-create the autocompletion dropdown after changes. We factored
        // this out since it will be needed to deal with changes to both the
        // edited object text and the autocompletion index.
        ac_repopulate = function() {
            // GB: Finding the class from obj: find obj through tag of
            // textbox, get obj class and remove from the class the word
            // "selected". This is necessary because in textbox obj and
            // comment both have class 'obj' and it's import here to
            // differentiate them.
            let obj_class = document
                .getElementById(textbox().getAttribute("tag")+"gobj")
                .getAttribute("class").toString()
                .split(" ").slice(0,1).toString();
            if (obj_class === "obj") { // autocomplete only works for objects
                pdgui.create_autocomplete_dd(document, ac_dropdown(), textbox());
                if (ac_dropdown().getAttribute("searched_text") !== textbox().innerText) {
                    last_results = pdgui.repopulate_autocomplete_dd(document, ac_dropdown, obj_class, textbox().innerText);
                    last_offset = 0;
                }
            }
        },
        events = {
            mousemove: function(evt) {
                //pdgui.post("x: " + evt.pageX + " y: " + evt.pageY +
                //    " modifier: " + (evt.shiftKey + (pdgui.cmd_or_ctrl_key(evt) << 1)));
                if (evt.type === "touchmove") {
                    if (target_is_popup(evt)) {
                        // ag: Presumably this is the confirmation popup, we
                        // don't want to do any special processing there at
                        // all, so bail out immediately and let the default
                        // handlers take over.
                        return;
                    } else if (canvas_menu.edit.editmode.checked &&
                               !pdgui.cmd_or_ctrl_key(evt)) {
                        // ag: Disable touch scrolling while we're doing a
                        // selection-drag, for the same reasons we're
                        // suppressing it while touching an object (see
                        // mousedown, below). Note that to scroll in edit mode
                        // you can just press the Ctrl/Cmd key while dragging,
                        // which will also suppress the selection rectangle to
                        // appear.
                        document.body.style.overflow = 'hidden';
                    }
                }
                // ag: It seems possible to get fractional values here, which
                // pdsend doesn't seem to like, so we truncate the touch
                // coordinates to integers.
                let [pointer_x, pointer_y] = evt.type === "touchmove"
                    ? [Math.trunc(evt.touches[0].pageX),
                       Math.trunc(evt.touches[0].pageY)]
                    : [evt.pageX, evt.pageY];
                pdgui.pdsend(name, "motion",
                    (pointer_x + svg_view.x),
                    (pointer_y + svg_view.y),
                    (evt.shiftKey + (pdgui.cmd_or_ctrl_key(evt) << 1))
                );
                // Commented lines below due to interference with touch scroll behavior
                // evt.stopPropagation();
                // evt.preventDefault();
                return false;
            },
            mousedown: function(evt) {
                // ico@vt.edu capture middle click for a different type of scroll
                // currently disabled due to problem with scrollBy and zoom
                /*if (evt.which == 2)
                {
                    evt.stopPropagation();
                    evt.preventDefault();
                }*/
                if (evt.type === "touchstart") {
                    if (target_is_popup(evt)) {
                        return;
                    } else if (!target_is_canvas(evt)) {
                        // ag: Disable touch scrolling while we're operating a
                        // GUI element, since trying to operate a slider while
                        // the canvas keeps moving beneath you is difficult.
                        // Actually, we're not overly specific here, we just
                        // test whether we're touching anything but the canvas
                        // itself. So, to initiate a scroll using touch,
                        // you'll have to touch a blank spot on the canvas.
                        document.body.style.overflow = 'hidden';
                    }
                }
                let [pointer_x, pointer_y] = evt.type === "touchstart"
                    ? [Math.trunc(evt.touches[0].pageX),
                       Math.trunc(evt.touches[0].pageY)]
                    : [evt.pageX, evt.pageY];
                var target_id, resize_type;
                if (target_is_scrollbar(evt)) {
                    return;
                } else if (evt.target.parentNode &&
                    evt.target.parentNode.classList
                        .contains("clickable_resize_handle")) {
                    draggable_label =
                        evt.target.parentNode.classList.contains("move_handle");

                    // get id ("x123456etcgobj" without the "x" or "gobj")
                    target_id = (draggable_label ? "_l" : "_s") +
                        evt.target.parentNode.parentNode.id.slice(0,-4).slice(1);
                    last_draggable_x = pointer_x + svg_view.x;
                    last_draggable_y = pointer_y + svg_view.y;

                    // Nasty-- we have to forward magic values from g_canvas.h
                    // defines in order to get the correct constrain behavior.
                    if (evt.target.classList.contains("constrain_top_right")) {
                        resize_type = 7; // CURSOR_EDITMODE_RESIZE_X
                    } else if (evt.target.classList
                        .contains("constrain_bottom_right")) {
                        resize_type = 10; // CURSOR_EDITMODE_RESIZE_Y
                    } else if (draggable_label) {
                        resize_type = 11; // CURSOR_EDITMODE_MOVE
                    } else {
                        resize_type = 8; // CURSOR_EDITMODE_RESIZE
                    }

                    // Even nastier-- we now must turn off the cursor styles
                    // so that moving the pointer outside the hotspot doesn't
                    // cause the cursor to change. This happens for the
                    // drag handle to move the gop red rectangle. Unlike
                    // the label handles, it doesn't get immediately
                    // destroyed upon receiving its callback below.
                    pdgui.toggle_drag_handle_cursors(evt.target.parentNode,
                        !!draggable_label, false);

                    pdgui.pdsend(target_id, "_click", resize_type,
                        (pointer_x + svg_view.x),
                        (pointer_y + svg_view.y));
                    canvas_events.iemgui_label_drag();
                    return;
                }
                // tk events (and, therefore, Pd events) are one greater
                // than html5...
                var b = evt.button + 1 || 1;
                var mod, match_elem;
                // See if there are any draggable scalar shapes...
                if (Object.keys(scalar_draggables).length) {
                    // if so, see if our target is one of them...
                    match_elem = find_scalar_draggable(evt.target);
                    if (match_elem) {
                        // then set some state and turn on the drag events
                        draggable_elem = match_elem;
                        last_draggable_x = pointer_x;
                        last_draggable_y = pointer_y;
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
                    mod = (evt.shiftKey + (pdgui.cmd_or_ctrl_key(evt) << 1));
                }
                pdgui.pdsend(name, "mouse",
                    (pointer_x + svg_view.x),
                    (pointer_y + svg_view.y),
                    b, mod
                );
                // If Alt is pressed on a box_text, fake a keyup to prevent
                // dangling temp runmode in case the click opens a subpatch.
                if (evt.altKey && target_is_canvasobj(evt)) {
                    pdgui.canvas_sendkey(name, 0, evt, "Alt", 0);
                }
                //evt.stopPropagation();
                //evt.preventDefault();
            },
            mouseup: function(evt) {
                //pdgui.post("mouseup: x: " +
                //    evt.pageX + " y: " + evt.pageY +
                //    " button: " + (evt.button + 1));
                if (evt.type === "touchend") {
                    if (target_is_popup(evt)) {
                        return;
                    }
                    // re-enable scrolling
                    document.body.style.overflow = '';
                }
                let [pointer_x, pointer_y] = evt.type === "touchend"
                    ? [Math.trunc(evt.changedTouches[0].pageX),
                       Math.trunc(evt.changedTouches[0].pageY)]
                    : [evt.pageX, evt.pageY];
                pdgui.pdsend(name, "mouseup",
                    (pointer_x + svg_view.x),
                    (pointer_y + svg_view.y),
                    (evt.button + 1) || 1
                );
                evt.stopPropagation();
                evt.preventDefault();
            },
            keydown: function(evt) {
                pdgui.keydown(name, evt);
                // prevent the default behavior of scrolling using arrow keys
                if (document.querySelector("#patchsvg")) {
                    if ([37, 38, 39, 40].indexOf(evt.keyCode) > -1) {
                        evt.preventDefault();
                    }
                }
            },
            keypress: function(evt) {
                pdgui.keypress(name, evt);
                // Don't do things like scrolling on space, arrow keys, etc.
                evt.preventDefault();
            },
            keyup: function(evt) {
                pdgui.keyup(name, evt);
                evt.stopPropagation();
                evt.preventDefault();
            },
            text_mousemove: function(evt) {
                if (evt.target.parentNode === document.getElementById("autocomplete_dropdown")) {
                    let sel = document.getElementById("autocomplete_dropdown").getAttribute("selected_item");
                    let new_sel = evt.target.getAttribute("idx");
                    pdgui.update_autocomplete_selected(document.getElementById("autocomplete_dropdown"), sel, new_sel)
                }
                evt.stopPropagation();
                //evt.preventDefault();
                return false;
            },
            text_mousedown: function(evt) {
                if (evt.target.parentNode === ac_dropdown() || (evt.target.tagName === 'SPAN' && evt.target.closest('p') && evt.target.closest('p').parentNode === ac_dropdown())) {
                    pdgui.select_result_autocomplete_dd(document, textbox(), ac_dropdown());
                    last_yanked = "";
                    // ag: Don't do the usual object instantiation thing if
                    // we've clicked on the autocompletion dropdown. This
                    // means that the user can just go on editing, entering
                    // object arguments, etc.
                    evt.stopPropagation();
                    //evt.preventDefault();
                    caret_end();
                    return false;
                }
                if(target_is_ac_dropdown(evt)) {
                    evt.stopPropagation();
                    return false;
                }
                if (textbox() !== evt.target && !target_is_scrollbar(evt)) {
                    utils.create_obj();
                    // send a mousedown and mouseup event to Pd to instantiate
                    // the object
                    events.mousedown(evt);
                    //events.mouseup(evt);
                    canvas_events.normal();
                }
                evt.stopPropagation();
                //evt.preventDefault();
                return false;
            },
            text_mouseup: function(evt) {
                //pdgui.post("mouseup target is " +
                //    evt.target + " and textbox is " + textbox());
                //evt.stopPropagation();
                //evt.preventDefault();
                return false;
            },
            text_keydown: function(evt) {
                evt.stopPropagation();
                setTimeout(function() {
                    pdgui.gui_message_update_textarea_border(textbox(), 0);
                }, 0);
                //evt.preventDefault();
                return false;
            },
            text_keyup: function(evt) {
                evt.stopPropagation();

                // GB: Autocomplete feature
                switch (evt.keyCode) {
                    case 40: // arrowdown
                        pdgui.update_autocomplete_dd_arrowdown(ac_dropdown())
                        last_yanked = "";
                        break;
                    case 38: // arrowup
                        pdgui.update_autocomplete_dd_arrowup(ac_dropdown())
                        last_yanked = "";
                        break;
                    case 13: // enter
                        // if there is no item selected on autocomplete dropdown, enter make the obj box bigger
                        if(ac_dropdown() === null || ac_dropdown().getAttribute("selected_item") === "-1") {
                            grow_svg_for_element(textbox());
                        } else { // else, if there is a selected item on autocompletion tool, the selected item is written on the box
                            pdgui.select_result_autocomplete_dd(document, textbox(), ac_dropdown());
                            caret_end();
                            // No need to instantiate the object here,
                            // presumably the user wants to go on editing.
                        }
                        last_yanked = "";
                        break;
                    case 9: // tab
                        [last_completed, last_offset] = pdgui.select_result_autocomplete_dd(document, textbox(), ac_dropdown(), last_completed, last_offset, last_results, evt.shiftKey?-1:1);
                        last_yanked = "";
                        caret_end();
                        break;
                    case 27: // esc
                        pdgui.delete_autocomplete_dd(document, ac_dropdown());
                        last_completed = last_offset = -1;
                        last_results = [];
                        if (last_yanked != "") {
                            pdgui.post("Operation aborted.")
                        }
                        last_yanked = "";
                        break;
		    case 89:
                        if (evt.ctrlKey) { // ctrl-y
                            // AG: Note that this key is usually bound to the
                            // Tidy Up operation in the Edit menu, but this
                            // presumably won't interfere with our use here,
                            // which is to "yank" the current completion
                            // (remove it from the completion index).
                            last_completed = last_offset = -1;
                            last_results = [];
                            if (textbox().innerText === "") {
                                pdgui.delete_autocomplete_dd(document, ac_dropdown());
                                last_yanked = "";
                            } else if (textbox().innerText === last_yanked) {
                                // confirmed, really yank now
                                if (pdgui.remove_completion(textbox().innerText, console.log)) {
                                    pdgui.post("Removed completion: "+textbox().innerText);
                                    ac_repopulate();
                                    last_results = [];
                                    last_completed = last_offset = -1;
                                }
                                last_yanked = "";
                            } else if (pdgui.check_completion(textbox().innerText)) {
                                // for some safety, ask the user to confirm with another ctrl+y
                                pdgui.post("Really remove completion "+textbox().innerText+"?");
                                pdgui.post("Press ctrl+y again to confirm (Esc to abort).");
                                last_yanked = textbox().innerText;
                            } else {
                                pdgui.post("No completion "+textbox().innerText);
                            }
                        }
                        break;
                    default:
                        // ag: Only update the state if a "valid key" is
                        // pressed, not some modifier or other special key.
                        function is_valid_key(e)
                        {
                            // See https://stackoverflow.com/questions/51296562
                            // This is a quick hack which works because the
                            // names of special keys are all multiple chars
                            // long and only contain letters and numbers.
                            return e.key.length == 1 // ASCII
                                // non-ASCII:
                                || (e.key.length > 1 && /[^a-zA-Z0-9]/.test(e.key))
                                // Spacebar:
                                || e.keyCode == 32
                                // We also include Backspace and Del here
                                // since they are used in editing. Note that
                                // Return (key code 13) is handled above.
                                || e.keyCode == 8 || e.keyCode == 46;
                        }
                        if (is_valid_key(evt)) {
                            last_results = [];
                            last_completed = last_offset = -1;
                            last_yanked = "";
                            if (textbox().innerText === "") {
                                pdgui.delete_autocomplete_dd(document, ac_dropdown());
                            } else {
                                ac_repopulate();
                            }
                        }
                }

                //evt.preventDefault();
                return false;
            },
            text_keypress: function(evt) {
                evt.stopPropagation();
                //evt.preventDefault();
                return false;
            },
            text_paste: function(evt) {
                evt.preventDefault();
                var text = evt.clipboardData.getData("text");
                // ag: We need to insert each line individually here, with
                // line breaks in between. Otherwise the behavior of
                // "insertText" is to insert a *paragraph* break between
                // lines, so that we get two newlines for each newline in the
                // input. (Who thought that this was a good design? There's
                // probably a reason why this WebKit API is deprecated, but
                // since there's no replacement for it, we have to use it.)
                // Also note that we skip the line break at the end of the
                // paste buffer, since it will be stripped off on the C side
                // during object creation anyway.
                var lines = text.split("\n");
                for (var i = 0; i < lines.length; i++) {
                    document.execCommand("insertText", false, lines[i]);
                    if (i < lines.length-1)
                        document.execCommand("insertLineBreak", false);
                }
                grow_svg_for_element(textbox());
            },
            floating_text_click: function(evt) {
                if (target_is_scrollbar(evt)) {
                    return;
                }
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
                        pdgui.encode_for_dialog(t),
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
                let [new_x, new_y] = evt.type === "touchmove"
                    ? [Math.trunc(evt.touches[0].pageX),
                       Math.trunc(evt.touches[0].pageY)]
                    : [evt.pageX, evt.pageY],
                    dx = new_x - last_draggable_x,
                    dy = new_y - last_draggable_y,
                    // For the sake of convenience we're sending transformed
                    // dx and dy back to the user. If it turns out this is
                    // expensive we can either optimize this or just leave it
                    // up to the user to handle on their own. (But I should
                    // mention that it was non-trivial for me to do the math
                    // of inverting and multiplying the matrices from within
                    // a Pd patch. And I'm the author of this API. Make
                    // of that what you will...)
                    minv = draggable_elem.getCTM().inverse(),
                    tx = minv.a * dx + minv.c * dy,
                    ty = minv.b * dx + minv.d * dy;
                var obj = scalar_draggables[draggable_elem.id];
                pdgui.pdsend(obj.cid, "scalar_event", obj.scalar_sym,
                    obj.drawcommand_sym, obj.array_sym, obj.index,
                    obj.event_name, dx, dy, tx, ty);
                last_draggable_x = new_x;
                last_draggable_y = new_y;
            },
            scalar_draggable_mouseup: function(evt) {
                canvas_events.normal();
            },
            iemgui_label_mousemove: function(evt) {
                // This is very convoluted.
                // 1. In the generic mousedown handler we detect a click for a
                //    label handle, red gop rect handle, or [cnv] resize anchor.
                //    That sets this callback for dragging the handle.
                // 2. The mousedown also sends a message to Pd to tell it that
                //    a handle has been clicked. The message is forwarded
                //    to the relevant handle (a t_scalehandle in Pd).
                // 3. Pd erases *all* handles, then redraws the one for this
                //    object. That *eventually* leaves just the handle(s)
                //    for the current object. This is either a single handle
                //    (for most iemguis and the canvas red rect), or possibly
                //    two handles for [cnv]-- one for resizing and one for its
                //    label.
                // 4. This function responds to mouse motion. It looks up
                //    the current handle by tag (using the draggable_lable
                //    kludge to choose between the cnv_resize_handle and
                //    everything else), sends a message to Pd to alter the
                //    object/label accordingly, then displaces the little
                //    handle itself. We unfortunately can't merely keep a
                //    reference to the handle element because in #3 Pd will
                //    have erased it.
                // Pro: I don't have to dig into the C code to get this to
                //      work.
                // Con: It's inherently racy. #3 and #4 happen at the same
                //      time, so it's possible to apply dx/dy to the wrong
                //      handle (which will eventually get erased by Pd anyway).
                //      Anyhow, this is all very bad, but it works so it's
                //      at least not the worst of all possible worlds.
                let [new_x, new_y] = evt.type === "touchmove"
                    ? [Math.trunc(evt.touches[0].pageX),
                       Math.trunc(evt.touches[0].pageY)]
                    : [evt.pageX, evt.pageY];
                var dx = (new_x + svg_view.x) - last_draggable_x,
                    dy = (new_y + svg_view.y) - last_draggable_y,
                    handle_elem = document.querySelector(
                        draggable_label ?
                            ".move_handle" :
                            ".cnv_resize_handle"
                        ),
                    target_id = (draggable_label ? "_l" : "_s") +
                        handle_elem.parentNode.id.slice(0,-4).slice(1),
                    is_canvas_gop_rect = document.
                        getElementsByClassName("gop_drag_handle").length ?
                        true : false;

                last_draggable_x = new_x + svg_view.x;
                last_draggable_y = new_y + svg_view.y;

                pdgui.pdsend(target_id, "_motion",
                    (new_x + svg_view.x),
                    (new_y + svg_view.y));
            },
            iemgui_label_mouseup: function(evt) {
                //pdgui.post("lifting the mousebutton on an iemgui label");
                // Set last state (none doesn't count as a state)
                //pdgui.post("previous state is "
                //    + canvas_events.get_previous_state());
                var label_handle = document.querySelector(".move_handle");
                var cnv_resize_handle =
                    document.querySelector(".cnv_resize_handle");
                // Restore our cursor bindings for any drag handles that
                // happen to exist
                if (label_handle) {
                    pdgui.toggle_drag_handle_cursors(label_handle,
                        true, true);
                }
                if (cnv_resize_handle) {
                    pdgui.toggle_drag_handle_cursors(cnv_resize_handle,
                        false, true);
                }
                canvas_events[canvas_events.get_previous_state()]();
            },
            dropdown_menu_keydown: function(evt) {
                var select_elem = document.querySelector("#dropdown_list"),
                    li;
                switch(evt.keyCode) {
                    case 13:
                    case 32:
                        dropdown_index_to_pd(select_elem);
                        select_elem.style.setProperty("display", "none");
                        canvas_events.normal();
                        break;
                    case 27: // escape
                        select_elem.style.setProperty("display", "none");
                        canvas_events.normal();
                        break;
                    case 38: // up
                        li = select_elem.querySelector(".highlighted");
                        li = li.previousElementSibling ||
                             li.parentElement.lastElementChild;
                        dropdown_highlight_elem(li, "up");
                        evt.preventDefault();
                        break;
                    case 40: // down
                        li = select_elem.querySelector(".highlighted");
                        li = li.nextElementSibling ||
                             li.parentElement.firstElementChild;
                        dropdown_highlight_elem(li, "down");
                        evt.preventDefault();
                        break;
                    default:
                }
            },
            dropdown_menu_keypress: function(evt) {
                var li_nodes = document.querySelectorAll("#dropdown_list li"),
                    string_array = [],
                    highlighted,
                    highlighted_index,
                    match,
                    offset;
                highlighted = document
                    .querySelector("#dropdown_list .highlighted");
                if (highlighted) {
                    highlighted_index =
                        +document.querySelector("#dropdown_list .highlighted")
                            .getAttribute("data-index");
                    offset = highlighted_index + 1;
                } else {
                    highlighted_index = 1;
                    offset = 2;
                }
                Array.prototype.forEach.call(li_nodes, function(e, i, a) {
                    var s = a[(i + offset) % a.length];
                    string_array.push(s.textContent.trim());
                });
                match = string_array.indexOf(
                    string_array.find(function(e) {
                        return e.charAt(0).toLowerCase() ===
                            String.fromCharCode(evt.charCode).toLowerCase();
                }));
                if (match !== undefined) {
                    match = (match + offset) % li_nodes.length;
                    if (match !== highlighted_index) {
                        dropdown_highlight_elem(li_nodes[match],
                            match < highlighted_index ? "up" : "down");
                    }
                }
            },
            dropdown_menu_mousedown: function(evt) {
                let [pointer_x, pointer_y] = evt.type === "touchstart"
                ? [Math.trunc(evt.touches[0].pageX),
                   Math.trunc(evt.touches[0].pageY)]
                : [evt.pageX, evt.pageY];

                var select_elem = document.querySelector("#dropdown_list"),
                    in_dropdown = evt.target;
                while (in_dropdown) {
                    if (in_dropdown.id === "dropdown_list") {
                        break;
                    }
                    in_dropdown = in_dropdown.parentNode;
                }
                // Allow scrollbar click and drag without closing the menu
                if (in_dropdown &&
                        pointer_x - select_elem.offsetLeft >
                        select_elem.clientWidth) {
                        return;
                }

                // Special case for OSX, where the scrollbar doesn't take
                // up any extra space
                if (nw.process.platform === "darwin"
                    && (evt.target.id === "dropdown_list")) {
                    return;
                }
                if (evt.target.parentNode
                    && evt.target.parentNode.parentNode
                    && evt.target.parentNode.parentNode.id === "dropdown_list") {
                    dropdown_highlight_elem(evt.target);
                }
                // This selects whatever item is highlighted even
                // if we click outside the menu. Might be better to
                // cancel in that case.
                if(evt.type === "mousedown") {  // For some reason calling dropdown_index_to_pd() on touchstart sends the selected data twice. @spidercatnat
                    dropdown_index_to_pd(select_elem);
                }
                select_elem.style.setProperty("display", "none");
                canvas_events.normal();
            },
            dropdown_menu_mouseup: function(evt) {
                var i, select_elem;
                // This can be triggered if the user keeps the mouse down
                // to highlight an element and releases the mouse button to
                // choose that element
                if (evt.target.parentNode
                    && evt.target.parentNode.parentNode
                    && evt.target.parentNode.parentNode.id === "dropdown_list") {
                    select_elem = document.querySelector("#dropdown_list");
                    dropdown_index_to_pd(select_elem);
                    select_elem.style.setProperty("display", "none");
                    canvas_events.normal();
                }
            },
            dropdown_menu_wheel: function(evt) {
                // Here we generate bogus mouse coords so that
                // we can break through the filter below if we're
                // using the mouse wheel to scroll in the list.
                last_dropdown_menu_x = Number.MIN_VALUE;
                last_dropdown_menu_y = Number.MIN_VALUE;
            },
            dropdown_menu_mousemove: function(evt) {
                let [pointer_x, pointer_y] = evt.type === "touchmove"
                ? [Math.trunc(evt.touches[0].pageX),
                   Math.trunc(evt.touches[0].pageY)]
                : [evt.pageX, evt.pageY];
                // For whatever reason, Chromium decides to trigger the
                // mousemove/mouseenter/mouseover events if the element
                // underneath it changes (or for mousemove, if the element
                // moves at all). Unfortunately that means we have to track
                // the mouse position the entire time to filter out the
                // true mousemove events from the ones Chromium generates
                // when a scroll event changes the element under the mouse.
                if (pointer_x !== last_dropdown_menu_x
                    || pointer_y !== last_dropdown_menu_y) {
                    if (evt.target.parentNode
                        && evt.target.parentNode.parentNode
                        && evt.target.parentNode.parentNode.id === "dropdown_list") {
                        dropdown_highlight_elem(evt.target);
                    } else {
                        dropdown_clear_highlight();
                    }
                }
                last_dropdown_menu_x = pointer_x;
                last_dropdown_menu_y = pointer_y;
            }
        },
        changes_in_completion_index = 0,
        utils = {
            create_obj: function() {
                // Yes: I _really_ want .innerText and NOT .textContent
                // here.  I want those newlines: although that isn't
                // standard in Pd-Vanilla, Pd-l2ork uses and preserves
                // them inside comments
                var msg = textbox().innerText;
                // GB: find obj class and remove word "selected"
                let obj = document
                    .getElementById(textbox().getAttribute("tag")+"gobj");
                let obj_class = obj.getAttribute("class")
                    .toString().split(" ").slice(0,1).toString();
                if (obj_class == "comment") {
                    // ag: This might be a bug in the browser engine, but the
                    // <p> element tries to be clever about newlines and adds
                    // extra newlines where it thinks they are appropriate
                    // (they never are). Therefore we replace 2 or more
                    // consecutive empty lines by a single one. Alas, this
                    // means that you can only ever have a single empty line
                    // in sequence. To work around this, enter a space on an
                    // empty line and it will be preserved.
                    msg = msg.replace(/\n\n\n+/g, "\n\n");
                    // ag: Visual comment formatting: We need to replace \n
                    // with \v to protect the newlines from the binbuf
                    // routines on the C side -- for them, \n is just
                    // whitespace which they eat for breakfast. However, we
                    // also need to preserve comma and semicolon atoms which
                    // get special treatment from the binfuf parser.
                    msg = msg.replace(/\n/g, "\v");
                    msg = msg.replace(/,[ \t]*\v/g, ", \v");
                    msg = msg.replace(/;[ \t]*\v/g, "; ");
                }
                var fudi_msg = text_to_fudi(msg, obj_class, true),
                    fudi_array = string_to_array_of_chunks(fudi_msg);
                for (var i = 0; i < fudi_array.length; i++) {
                    var chunk = fudi_array[i].join(" ");
                    pdgui.pdsend(name, "obj_addtobuf", chunk);
                }
                pdgui.pdsend(name, "obj_buftotext");

                // GB: index created object
                if (pdgui.autocomplete_enabled()) {
                    var obj_text = text_to_fudi(msg, obj_class, false);
                    pdgui.index_obj_completion(obj_class, obj_text);
                    // GB: save every 50 changes, so that we don't loose too
                    // much data in case purr-data unexpectedly quits or crashes
                    if (changes_in_completion_index===0 ||
                        ++changes_in_completion_index > 50) {
                        pdgui.write_completion_index();
                        changes_in_completion_index = 1;
                    }
                }
            }
        }
    ;

    return {
        none: function() {
            var evt_name, prop;
            if (state !== "none") {
                previous_state = state;
            }
            state = "none";
            // remove whatever we currently have bound in current_events object
            remove_events(document, current_events);
        },
        normal: function() {
            canvas_events.none();
            add_events(document, {
                "mousemove": events.mousemove,
                "touchmove": events.mousemove,
                "keydown": events.keydown,
                "keypress": events.keypress,
                "keyup": events.keyup,
                "mousedown": events.mousedown,
                "touchstart": events.mousedown,
                "mouseup": events.mouseup,
                "touchend": events.mouseup
            });
            state = "normal";
            set_edit_menu_modals(true);
        },
        scalar_drag: function() {
            // This scalar_drag is a prototype for moving more of the editing
            // environment directly to the GUI.  At the moment we're leaving
            // the other "normal" events live, since behavior like editmode
            // selection still happens from the Pd engine.
            //this.none();
            add_events(document, {
                "mousemove": events.scalar_draggable_mousemove,
                "touchmove": events.scalar_draggable_mousemove,
                "mouseup": events.scalar_draggable_mouseup,
                "touchend": events.scalar_draggable_mouseup
            });
        },
        iemgui_label_drag: function() {
            // This is a workaround for dragging iemgui labels. Resizing iemguis
            // currently happens in Pd (canvas_doclick and canvas_motion). (Look
            // for MA_RESIZE.)
            // The exception is my_canvas, which is weird because the visible
            // rectangle extends past the bbox that it reports to Pd.
            // Unfortunately that means a lot of work to treat it separately.
            canvas_events.none();
            add_events(document, {
                "mousemove": events.iemgui_label_mousemove,
                "touchmove": events.iemgui_label_mousemove,
                "mouseup": events.iemgui_label_mouseup,
                "touchend": events.iemgui_label_mouseup
            });
        },
        hscroll_drag: function(e) {
            e.preventDefault(); // prevent drag-and-drop- events
            canvas_events.none();
            document.getElementById("hscroll").style
                .cssText += "background-color: rgba(0, 0, 0, 0.5) !important";
            document.getElementById("patchsvg").style.cursor =
                "-webkit-grabbing";
            add_events(document, {
                "mouseup": function(evt) {
                    document.getElementById("hscroll").style
                        .setProperty("background-color", "rgba(0, 0, 0, 0.267)");
                    document.getElementById("patchsvg").style.cursor = "default";
                    canvas_events[canvas_events.get_previous_state()]();
                },
                "mousemove": function(evt) {
                    if (evt.movementX != 0) {
                        //console.log("move: " + e.movementX);
                        var hscroll = document.getElementById("hscroll");
                        var svg_elem = document.getElementById("patchsvg");
                        var min_width = document.body.clientWidth + 3;
                        var width = svg_elem.getAttribute('width');
                        var xScrollSize;
                        xScrollSize = hscroll.offsetWidth;
                        var xTranslate = evt.movementX *
                            ((width - min_width)/(min_width - xScrollSize)) *
                            (evt.movementX > 0 ? 1 : 0.75);
                        if (xTranslate > 0 && xTranslate < 1) {
                            xTranslate = 1;
                        }
                        if (xTranslate < 0 && xTranslate > -1) {
                            xTranslate = -1;
                        }
                        //console.log(xTranslate);
                        window.scrollBy(xTranslate, 0);
                    }
                }
            });
        },
        vscroll_drag: function(e) {
            e.preventDefault(); // prevent drag-and-drop events
            canvas_events.none();
            document.getElementById("vscroll").style
                .cssText += "background-color: rgba(0, 0, 0, 0.5) !important";
            document.getElementById("patchsvg").style.cursor =
                "-webkit-grabbing";
            add_events(document, {
                "mouseup": function(evt) {
                    document.getElementById("vscroll").style
                        .setProperty("background-color", "rgba(0, 0, 0, 0.267)");
                    document.getElementById("patchsvg").style.cursor = "default";
                    canvas_events[canvas_events.get_previous_state()]();
                },
                "mousemove": function(evt) {
                    if (evt.movementY != 0) {
                        //console.log("move: " + e.movementY);
                        var vscroll = document.getElementById("vscroll");
                        var svg_elem = document.getElementById("patchsvg");
                        var min_height = document.body.clientHeight + 3;
                        var height = svg_elem.getAttribute('height');
                        var yScrollSize;
                        yScrollSize = vscroll.offsetHeight;
                        var yTranslate = evt.movementY *
                            ((height - min_height)/(min_height - yScrollSize)) *
                            (evt.movementY > 0 ? 2 : 1.5);
                        if (yTranslate > 0 && yTranslate < 1) {
                            yTranslate = 1;
                        }
                        if (yTranslate < 0 && yTranslate > -1) {
                            yTranslate = -1;
                        }
                        //console.log(yTranslate);
                        window.scrollBy(0, yTranslate);
                    }
                }
            });
        },
        text: function() {
            canvas_events.none();
            add_events(document, {
                "mousemove": events.text_mousemove,
                "keydown": events.text_keydown,
                "keypress": events.text_keypress,
                "keyup": events.text_keyup,
                "paste": events.text_paste,
                "mousedown": events.text_mousedown,
                "mouseup": events.text_mouseup
            });
            state = "text";
            set_edit_menu_modals(false);
        },
        floating_text: function() {
            canvas_events.none();
            canvas_events.text();
            remove_events(document, {
                "mousedown": events.text_mousedown,
                "mouseup": events.text_mouseup,
                "keypress": events.text_keypress,
                "mousemove": events.text_mousemove
            });
            add_events(document, {
                "click": events.floating_text_click,
                "keypress": events.floating_text_keypress,
                "mousemove": events.mousemove
            });
            state = "floating_text";
            set_edit_menu_modals(false);
        },
        dropdown_menu: function() {
            canvas_events.none();
            add_events(document, {
                "mousedown": events.dropdown_menu_mousedown,
                "touchstart": events.dropdown_menu_mousedown,
                "mouseup": events.dropdown_menu_mouseup,
                "touchend": events.dropdown_menu_mouseup,
                "mousemove": events.dropdown_menu_mousemove,
                "touchmove": events.dropdown_menu_mousemove,
                "keydown": events.dropdown_menu_keydown,
                "keypress": events.dropdown_menu_keypress
            });
            document.querySelector("#dropdown_list")
                .addEventListener("wheel", events.dropdown_menu_wheel, false);
        },
        search: function() {
            canvas_events.none();
            add_events(document, {
                "keydown": events.find_keydown
            });
            state = "search";
        },
        update_scrollbars: function() {
            pdgui.gui_update_scrollbars(name);
        },
        register: function(n) {
            name = n;
        },
        get_id: function() {
            return name;
        },
        get_state: function() {
            return state;
        },
        get_previous_state: function() {
            return previous_state;
        },
        create_obj: function() {
            utils.create_obj();
        },
        match_words: function(state) {
            match_words_state = state;
        },
        find_reset: function() {
            last_search_term = "";
        },
        add_scalar_draggable: function(cid, tag, scalar_sym, drawcommand_sym,
            event_name, array_sym, index) {
            scalar_draggables[tag] = {
                cid: cid,
                scalar_sym: scalar_sym,
                drawcommand_sym: drawcommand_sym,
                event_name: event_name,
                array_sym: array_sym,
                index: index
            };
        },
        remove_scalar_draggable: function(id) {
            if (scalar_draggables[id]) {
                scalar_draggables[id] = null;
            }
        },
        save_and_close: function() {
            pdgui.pdsend(name, "menusave", 1);
        },
        close_without_saving: function(cid, force) {
            pdgui.pdsend(name, "dirty 0");
            pdgui.pdsend(cid, "menuclose", force);
        },
        close_save_dialog: function() {
            document.getElementById("save_before_quit").close();
        },
        paste_from_pd_file: function(name, clipboard_data) {
            var line, lines, i, pd_message;
            // This lets the user copy some Pd source file from another
            // application and paste the code directly into a canvas window
            // (empty or otherwise). It does a quick check to make sure the OS
            // clipboard is holding text that could conceivably be Pd source
            // code. But that's not 100% foolproof and the engine might crash
            // and burn, so be careful! :)

            // We only want to check the external buffer and/or send a "paste"
            // event to Pd if the canvas is in a "normal" state.
            if (canvas_events.get_state() !== "normal") {
                return;
            }
            if (!might_be_a_pd_file(clipboard_data)) {
                pdgui.post("paste error: clipboard doesn't appear to contain valid Pd code");
                return;
            }

            // clear the buffer
            pdgui.pdsend(name, "copyfromexternalbuffer");
            pd_message = "";
            lines = clipboard_data.split("\n");
            for (i = 0; i < lines.length; i++) {
                line = lines[i];
                // process pd_message if it ends with a semicolon that
                // isn't preceded by a backslash
                if (line.slice(-1) === ";" &&
                    (line.length < 2 || line.slice(-2, -1) !== "\\")) {
                    if (pd_message === "") {
                        pd_message = line;
                    } else {
                        pd_message = pd_message + " " + line;
                    }
                    pdgui.pdsend(name, "copyfromexternalbuffer", pd_message);
                    pd_message = "";
                } else {
                    pd_message = pd_message + " " + line;
                    pd_message = pd_message.replace(/\n/g, "");
                }
            }
            // This signals to the engine that we're done filling the external
            // buffer, that it can do necessary checks and call some internal
            // cleanup code.

            pdgui.pdsend(name, "copyfromexternalbuffer");
            // Send a canvas "paste" message to Pd
            pdgui.pdsend(name, "paste");
            // Finally, make sure to reset the buffer so that next time around
            // we start from a clean slate. (Otherwise, the engine will just
            // add stuff to the existing buffer contents.)
            pdgui.pdsend(name, "reset_copyfromexternalbuffer");
        },
        init: function() {
            document.getElementById("saveDialog")
                .setAttribute("nwworkingdir", pdgui.get_pwd());
            document.getElementById("fileDialog")
                .setAttribute("nwworkingdir", pdgui.get_pwd());
            document.getElementById("fileDialog").setAttribute("accept",
                Object.keys(pdgui.pd_filetypes).toString());
            // Dialog events -- these are set elsewhere now because of a bug
            // with nwworkingdir
            document.querySelector("#saveDialog").addEventListener("change",
                function(evt) {
                    pdgui.saveas_callback(name, evt.target.value, 0);
                    // reset value so that we can open the same file twice
                    evt.target.value = null;
                    //console.log("tried to save something");
                }, false
            );

            // add listener for the scrollbars
            document.getElementById("hscroll").
                addEventListener("mousedown", canvas_events.hscroll_drag, false);
            document.getElementById("vscroll").
                addEventListener("mousedown", canvas_events.vscroll_drag, false);

            init_events(document, {
                "contextmenu": function(evt) {
                    // Whoa-- huge workaround! Right now we're getting
                    // the popup menu the way Pd Vanilla does it:
                    // 1) send a mouse(down) message to Pd
                    // 2) Pd checks whether it wants to send us a popup
                    // 3) Pd checks what popup menu items are available for
                    //    obj/canvas
                    // 4) Pd sends GUI back a message with this info
                    // 5) GUI finally displays the popup
                    // 6) GUI keeps a _global_ _variable_ to remember the popup
                    //    coords
                    // 7) User clicks an option in the popup
                    // 8) GUI sends a message back to Pd with the popup index
                    //    and coords
                    // 9) Pd walks the linked list of objects to look up the
                    //    object
                    // 10) Pd asks the object if it reacts to popups, and if it
                    //     reacts to the selected item in the popup
                    // 11) Pd sends a message to the relevant object for the
                    //     item in question
                    // nw.js has a nice little "contextmenu" event handler, but
                    // it's too difficult to use when passing between GUI and
                    // Pd (twice). In the future we should do all popup menu
                    // event handling in the GUI, and only pass a message to Pd
                    // when the user has clicked an item.
                    // For now, however, we just turn off its default behavior
                    // and control it with a bunch of complicated callbacks.

                    //console.log("got a context menu evt...");
                    evt.stopPropagation()
                    evt.preventDefault();
                },
                "scroll": function() {
                    // map onscroll event
                    pdgui.gui_update_scrollbars(name);
                },
                "cut": function(evt) {
                    // Cut event
                    // This event doesn't currently get called because the
                    // nw menubar receives the event and doesn't propagate
                    // to the DOM. But if we add the ability to toggle menubar
                    // display, we might need to rely on this listener.
                    pdgui.pdsend(name, "cut");
                },
                "copy": function(evt) {
                    // Copy event
                    // On OSX, this event gets triggered when we're editing
                    // inside an object/message box. So we only forward the
                    // copy message to Pd if we're in a "normal" canvas state
                    if (canvas_events.get_state() === "normal") {
                        pdgui.pdsend(name, "copy");
                    }
                },
                "paste": function(evt) {
                    // Listen to paste event
                    // XXXTODO: Not sure whether this is even needed any more,
                    // as the paste-from-clipboard functionality has been moved
                    // to its own menu option. So this code may possibly be
                    // removed in the future. -ag
                    if (canvas_events.get_state() !== "normal") {
                        return;
                    }
                    // Send a canvas "paste" message to Pd
                    pdgui.pdsend(name, "paste");
                },
                "wheel": function(evt) {
                    // MouseWheel event for zooming
                    var d = { deltaX: 0, deltaY: 0, deltaZ: 0 };
                    Object.keys(d).forEach(function(key) {
                        if (evt[key] < 0) {
                            d[key] = -1;
                        } else if (evt[key] > 0) {
                            d[key] = 1;
                        } else {
                            d[key] = 0;
                        }
                    });
                    if (pdgui.cmd_or_ctrl_key(evt)) {
                        // scroll up for zoom-in, down for zoom-out
                        pdgui.nw_window_zoom(name, -d.deltaY);
                    }
                    // Send a message on to Pd for the [mousewheel] legacy
                    // object (in the future we can refcount to prevent
                    // forwarding these messages when there's no extant
                    // receiver)
                    pdgui.pdsend(name, "legacy_mousewheel",
                        d.deltaX, d.deltaY, d.deltaZ);
                }
            });

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
            document.querySelector("#openpanel_dialog")
                .addEventListener("change", function(evt) {
                    var file_string = evt.target.value;
                    // reset value so that we can open the same file twice
                    evt.target.value = null;
                    pdgui.file_dialog_callback(file_string);
                    //console.log("tried to openpanel something");
                }, false
            );
            document.querySelector("#savepanel_dialog")
                .addEventListener("change", function(evt) {
                    var file_string = evt.target.value;
                    // reset value so that we can open the same file twice
                    evt.target.value = null;
                    pdgui.file_dialog_callback(file_string);
                    //console.log("tried to savepanel something");
                }, false
            );
            document.querySelector("#canvas_find_text")
                .addEventListener("focusin", canvas_events.search, false
            );

            // disable drag and drop for the time being
            init_events(window, {
                "dragover": function (evt) {
                    evt.preventDefault();
                },
                "drop": function (evt) {
                    evt.preventDefault();
                }
            });

            // Add placeholder text... this all needs to be collected into an
            // add_events function similiar to the one in index.js
            document.querySelector("#canvas_find_text").placeholder =
                l("canvas.find.placeholder");
            document.querySelector("#canvas_find_text").addEventListener("blur",
                canvas_events.normal, false
            );
            document.querySelector("#canvas_find_button")
                .addEventListener("click", events.find_click
            );
            // We need to separate these into nw_window events and html5 DOM
            // events closing the Window this isn't actually closing the window
            // yet
            gui.Window.get().on("close", function(arg) {
                pdgui.canvas_check_geometry(name);
                pdgui.pdsend(name, "menuclose 0");
                if (arg === "quit") {
                    pdgui.menu_quit();
                }
            });
            // update viewport size when window size changes
            gui.Window.get().on("maximize", function() {
                pdgui.gui_canvas_get_scroll(name);
            });
            gui.Window.get().on("restore", function() {
                pdgui.gui_canvas_get_scroll(name);
            });
            gui.Window.get().on("resize", function() {
                pdgui.gui_canvas_get_scroll(name);
            });
            gui.Window.get().on("focus", function() {
                nw_window_focus_callback(name);
            });
            gui.Window.get().on("blur", function() {
                nw_window_blur_callback(name);
            });
            gui.Window.get().on("move", function(x, y) {
                var w = gui.Window.get();
                pdgui.pdsend(name, "setbounds", x, y,
                    x + w.width, y + w.height);
            });

            // set minimum window size
            gui.Window.get().setMinimumSize(150, 100);
        }
    };
}());

// Stop-gap translator. We copy/pasted this in each dialog, too. It
// should be moved to pdgui.js
function translate_form() {
    var i
    var elements = document.querySelectorAll("[data-i18n]");
    for (i = 0; i < elements.length; i++) {
        var data = elements[i].dataset.i18n;
        if (data.slice(0,7) === "[title]") {
            elements[i].title = l(data.slice(7));
        } else {
            elements[i].textContent = l(data);
        }
    }
}

// This gets called from the nw_create_window function in index.html
// It provides us with our canvas id from the C side.  Once we have it
// we can create the menu and register event callbacks
function register_window_id(cid, attr_array) {
    var kludge_title;
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
    // Initialize global DOM state/events
    canvas_events.init(document);
    translate_form();
    // Trigger a "focus" event so that OSX updates the menu for this window
    nw_window_focus_callback(cid);
    canvas_events.normal();
    // Initialize the zoom level to the value retrieved from the patch, if any.
    nw.Window.get().zoomLevel = attr_array.zoom;
    pdgui.canvas_map(cid); // side-effect: triggers gui_canvas_get_scroll
    pdgui.canvas_query_editmode(cid);
    // For now, there is no way for the cord inspector to be turned on by
    // default. But if this changes we need to set its menu item checkbox
    // accordingly here
    // set_cord_inspector_checkbox();

    // Set scroll bars
    pdgui.canvas_set_scrollbars(cid, !attr_array.hide_scroll);
    // One final kludge-- because window creation is asynchronous, we may
    // have gotten a dirty flag before the window was created. In that case
    // we check the title_queue to see if our title now contains an asterisk
    // (which is the visual cue for "dirty")

    // Enable/disable the warning for multiple dirty instances
    pdgui.gui_canvas_warning(cid, attr_array.warid);

    // Two possibilities for handling this better:
    // have a representation of canvas attys in pdgui.js (editmode, dirty, etc.)
    // or
    // send those attys from Pd after mapping the canvas
    kludge_title = pdgui.query_title_queue(cid);
    if (kludge_title) {
        nw.Window.get().title = kludge_title;
    }
    pdgui.free_title_queue(cid);
    document.body.addEventListener("load", update_menu_items(cid), false);
}

function create_popup_menu(name) {
    // The right-click popup menu
    var popup_menu = new gui.Menu();
    pdgui.add_popup(name, popup_menu);

    popup_menu.append(new gui.MenuItem({
        label: l("canvas.menu.props"),
        click: function() {
            pdgui.popup_action(name, 0);
        }
    }));
    popup_menu.append(new gui.MenuItem({
        label: l("canvas.menu.open"),
        click: function() {
            pdgui.popup_action(name, 1);
        }
    }));
    popup_menu.append(new gui.MenuItem({
        label: l("canvas.menu.saveas"),
        click: function() {
            pdgui.popup_action(name, 5);
        }
    }));
    popup_menu.append(new gui.MenuItem({
        label: l("canvas.menu.help"),
        click: function() {
            pdgui.popup_action(name, 2);
        }
    }));
    popup_menu.append(new gui.MenuItem({
        type: "separator",
    }));
    popup_menu.append(new gui.MenuItem({
        label: l("canvas.menu.front"),
        click: function() {
            pdgui.popup_action(name, 3);
        }
    }));
    popup_menu.append(new gui.MenuItem({
        label: l("canvas.menu.back"),
        click: function() {
            pdgui.popup_action(name, 4);
        }
    }));
}

function nw_undo_menu(undo_text, redo_text) {
    // Disabling undo/redo menu buttons on OSX will
    // turn off any DOM events associated with that
    // key command. So we can't disable them here--
    // otherwise it would turn off text-editing
    // undo/redo.
    if (process.platform !== "darwin" && undo_text === "no") {
        canvas_menu.edit.undo.enabled = false;
    } else {
        canvas_menu.edit.undo.enabled = true;
        canvas_menu.edit.undo.label = l("menu.undo") + " " + undo_text;
    }
    if (process.platform !== "darwin" && redo_text === "no") {
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
        canvas_events.create_obj();
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
    // OSX needs to keep these enabled, otherwise the events won't trigger
    if (process.platform === "darwin") {
        state = true;
    }
    canvas_menu.edit.undo.enabled = state;
    canvas_menu.edit.redo.enabled = state;
    canvas_menu.edit.cut.enabled = state;
    canvas_menu.edit.copy.enabled = state;
    canvas_menu.edit.paste.enabled = state;
    canvas_menu.edit.selectall.enabled = state;
    canvas_menu.edit.font.enabled = state;
}

function get_editmode_checkbox() {
    return canvas_menu.edit.editmode.checked;
}

function set_editmode_checkbox(state) {
    canvas_menu.edit.editmode.checked = state;
}

function set_cord_inspector_checkbox(state) {
    canvas_menu.edit.cordinspector.checked = state;
}

// stop-gap
function menu_generic () {
    alert("Please implement this");
}

function minit(menu_item, options) {
    var key;
    for (key in options) {
        if (options.hasOwnProperty(key)) {
            // For click callbacks, we want to check if canvas state is
            // "none", in which case we don't call them. This is just a
            // hack, though-- it'd be a better UX to disable all menu items
            // when we're in the "none" state.
            menu_item[key] = (key !== "click") ?
                options[key] :
                function() {
                    if (canvas_events.get_state() !== "none") {
                        options[key]();
                    }
            };
        }
    }
}

// used, so that we can reference menu later
var m  = null;

function nw_create_patch_window_menus(gui, w, name) {
    // if we're on GNU/Linux or Windows, create the menus:
    m = canvas_menu = pd_menus.create_menu(gui);

    // File sub-entries
    // We explicitly enable these menu items because on OSX
    // the console menu disables them. (Same for Edit and Put menu)
    minit(m.file.new_file, { click: pdgui.menu_new });
    minit(m.file.open, {
        click: function() {
            var input, chooser,
                span = w.document.querySelector("#fileDialogSpan");
            // Complicated workaround-- see comment in build_file_dialog_string
            input = pdgui.build_file_dialog_string({
                style: "display: none;",
                type: "file",
                id: "fileDialog",
                nwworkingdir: pdgui.funkify_windows_path(pdgui.get_pd_opendir()),
                multiple: null,
                // These are copied from pd_filetypes in pdgui.js
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
                //console.log("tried to open something");
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
        }
    });
    minit(m.file.saveas, {
        enabled: true,
        click: function (){
            pdgui.canvas_check_geometry(name);
            pdgui.menu_saveas(name);
        }
    });
    minit(m.file.print, {
        enabled: true,
        click: function (){
            pdgui.canvas_check_geometry(name);
            pdgui.menu_print(name);
        }
    });
    minit(m.file.message, {
        click: function() { pdgui.menu_send(name); }
    });
    minit(m.file.close, {
        enabled: true,
        click: function() {
            pdgui.canvas_check_geometry(name);
            pdgui.menu_close(name);
        }
    });
    minit(m.file.quit, {
        click: pdgui.menu_quit
    });

    // Edit menu
    minit(m.edit.undo, {
        enabled: true,
        click: function () {
            if (canvas_events.get_state() === "text") {
                document.execCommand("undo", false, null);
            } else {
                pdgui.pdsend(name, "undo");
            }
        }
    });
    minit(m.edit.redo, {
        enabled: true,
        click: function () {
            if (canvas_events.get_state() === "text") {
                document.execCommand("redo", false, null);
            } else {
                pdgui.pdsend(name, "redo");
            }
        }
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
    minit(m.edit.paste_clipboard, {
        enabled: true,
        click: function () {
            var clipboard = nw.Clipboard.get();
            var text = clipboard.get('text');
            //pdgui.post("** paste from clipboard: "+text);
            canvas_events.paste_from_pd_file(name, text);
        }
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
            } else if (process.platform === "darwin") {
                // big kluge for OSX to select all inside a
                // contenteditable element (needed because
                // the stupid MacBuiltin is buggy-- see pd_menus.js)
                document.execCommand("selectAll", false, null);
            }
        }
    });
    minit(m.edit.clear_console, {
        enabled: true,
        click: pdgui.clear_console
    });
    minit(m.edit.reselect, {
        enabled: true,
        click: function() {
            // This is a bit complicated... menu item shortcuts receive
            // key events before the DOM, so we have to make sure to
            // filter out <ctrl-or-cmd-Enter> in the DOM eventhandler
            // in the "normal" keypress callback.
            // We also have to make sure to send the text ahead to Pd
            // to make sure it has it in the before before attempting
            // to "reselect".
            // As we move the editing functionality from the c code to
            // the GUI, this will get less complex.
            if (canvas_events.get_state() === "floating_text" ||
                canvas_events.get_state() === "text") {
                canvas_events.text(); // anchor the object
                canvas_events.create_obj();
            }
            pdgui.pdsend(name, "reselect");
        }
    });
    minit(m.edit.encapsulate, {
        enabled: true,
        click: function() { pdgui.pdsend(name, "encapsulate"); }
    });
    minit(m.edit.tidyup, {
        enabled: true,
        click: function() { pdgui.pdsend(name, "tidy"); }
    });
    minit(m.edit.font, {
        enabled: true,
        /*click: function () { pdgui.pdsend(name, "menufont"); } */
    });
    minit(m.font.s8, {
        enabled: true,
        click: function () {
            m.font.s8.checked = true;
            m.font.s10.checked = false;
            m.font.s12.checked = false;
            m.font.s16.checked = false;
            m.font.s24.checked = false;
            m.font.s36.checked = false;
            pdgui.gui_menu_font_change_size(name, 8);
        }
    });
    minit(m.font.s10, {
        enabled: true,
        click: function () {
            m.font.s8.checked = false;
            m.font.s10.checked = true;
            m.font.s12.checked = false;
            m.font.s16.checked = false;
            m.font.s24.checked = false;
            m.font.s36.checked = false;
            pdgui.gui_menu_font_change_size(name, 10);
        }
    });
    minit(m.font.s12, {
        enabled: true,
        click: function () {
            m.font.s8.checked = false;
            m.font.s10.checked = false;
            m.font.s12.checked = true;
            m.font.s16.checked = false;
            m.font.s24.checked = false;
            m.font.s36.checked = false;
            pdgui.gui_menu_font_change_size(name, 12);
        }
    });
    minit(m.font.s16, {
        enabled: true,
        click: function () {
            m.font.s8.checked = false;
            m.font.s10.checked = false;
            m.font.s12.checked = false;
            m.font.s16.checked = true;
            m.font.s24.checked = false;
            m.font.s36.checked = false;
            pdgui.gui_menu_font_change_size(name, 16);
        }
    });
    minit(m.font.s24, {
        enabled: true,
        click: function () {
            m.font.s8.checked = false;
            m.font.s10.checked = false;
            m.font.s12.checked = false;
            m.font.s16.checked = false;
            m.font.s24.checked = false;
            m.font.s36.checked = false;
            pdgui.gui_menu_font_change_size(name, 24);
        }
    });
    minit(m.font.s36, {
        enabled: true,
        click: function () {
            m.font.s8.checked = false;
            m.font.s10.checked = false;
            m.font.s12.checked = false;
            m.font.s16.checked = false;
            m.font.s24.checked = false;
            m.font.s36.checked = true;
            pdgui.gui_menu_font_change_size(name, 36);
        }
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
                // this resets the last search term so that the next search
                // starts from the beginning again
                canvas_events.find_reset();
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
            pdgui.nw_window_zoom(name, +1);
        }
    });
    minit(m.view.zoomout, {
        enabled: true,
        click: function () {
            pdgui.nw_window_zoom(name, -1);
        }
    });
    minit(m.view.optimalzoom, {
        enabled: true,
        click: function () {
            pdgui.gui_canvas_optimal_zoom(name, 1, 1);
            pdgui.gui_canvas_get_scroll(name);
        }
    });
    minit(m.view.horizzoom, {
        enabled: true,
        click: function () {
            pdgui.gui_canvas_optimal_zoom(name, 1, 0);
            pdgui.gui_canvas_get_scroll(name);
        }
    });
    minit(m.view.vertzoom, {
        enabled: true,
        click: function () {
            pdgui.gui_canvas_optimal_zoom(name, 0, 1);
            pdgui.gui_canvas_get_scroll(name);
        }
    });
    minit(m.view.zoomreset, {
        enabled: true,
        click: function () {
            gui.Window.get().zoomLevel = 0;
            pdgui.pdsend(name, "zoom", 0);
            pdgui.gui_canvas_get_scroll(name);
        }
    });
    minit(m.view.fullscreen, {
        click: function() {
            var win = gui.Window.get();
            win.toggleFullscreen();
            pdgui.gui_canvas_get_scroll(name);
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
    minit(m.put.listbox, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "listbox 0");
        }
    });
    minit(m.put.dropdown, {
        enabled: true,
        click: function() {
            update_live_box();
            pdgui.pdsend(name, "dirty 1");
            pdgui.pdsend(name, "dropdown 0");
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
    minit(m.win.abstractions, {
        enabled: true,
        click: function () {
            pdgui.pdsend(name, "getabstractions");
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
            pdgui.pd_doc_open("doc/7.stuff/tools", "testtone.pd", 1);
        }
    });
    minit(m.media.loadmeter, {
        click: function() {
            pdgui.pd_doc_open("doc/7.stuff/tools", "load-meter.pd", 1);
        }
    });

    // Help menu
    minit(m.help.about, {
        click: function() {
            pdgui.pd_doc_open("doc/about", "about.pd", 1);
        }
    });
    minit(m.help.manual, {
        click: function() {
            pdgui.pd_doc_open("doc/1.manual", "index.htm");
        }
    });
    minit(m.help.tutorial, {
        click: function() {
            pdgui.pd_doc_open("doc/about", "Purr-Data-Intro.pdf");
        }
    });
    minit(m.help.browser, {
        click: pdgui.open_search
    });
    minit(m.help.intro, {
        click: function() {
            pdgui.pd_doc_open("doc/5.reference", "help-intro.pd", 1);
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
        click: function() {
            pdgui.external_doc_open("http://puredata.info/community/IRC");
        }
    });
    minit(m.help.devtools, {
        click: function () {
            gui.Window.get().showDevTools();
        }
    });
}

function init_menu_font_size(size) {
    //pdgui.post("init_menu_font_size " + size);
    m.font.s8.checked = false;
    m.font.s10.checked = false;
    m.font.s12.checked = false;
    m.font.s16.checked = false;
    m.font.s24.checked = false;
    m.font.s36.checked = false;

    switch(size)
    {
        case 8:
            m.font.s8.checked = true;
            break;
        case 10:
            m.font.s10.checked = true;
            break;
        case 12:
            m.font.s12.checked = true;
            break;
        case 16:
            m.font.s16.checked = true;
            break;
        case 24:
            m.font.s24.checked = true;
            break;
        case 36:
            m.font.s36.checked = true;
            break;
    }
}

// ico@vt.edu 2020-08-24: this is called when the window is finally
// loaded and then asks libpd to tell us what is the font state
// LATER: we can use this to also update the undo state appropriately
function update_menu_items(cid) {
    pdgui.pdsend(cid, "updatemenu");
}
