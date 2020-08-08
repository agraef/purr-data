"use strict";

var pdgui = require("./pdgui.js");
var l = pdgui.get_local_string; // For menu names
var osx_menu = null; // OSX App menu -- a single one per running instance
var recent_files_submenu = null;
var shortcuts = require("./pd_shortcuts.js");

function create_menu(gui, type) {
    // On OSX we create a menu only once, and then enable/disable menuitems
    // and switch out functions as needed.

    // We specify the label here because nw.js won't create a menu item
    // without one. We also specify the keyboard shortcuts here because
    // nw.js won't create an event listener unless you make the
    // shortcut immediately when creating the menu item. (It also
    // won't let you update the keyboard shortcut binding later.)
    var m = {},
        osx = process.platform === "darwin",
        canvas_menu, // menu for canvas = true,  menu for Pd console = false
        window_menu, // window menu bar, or-- for OSX-- app menu bar
        file_menu, // submenus for window menubar...
        edit_menu,
        view_menu,
        put_menu,
        winman_menu,
        media_menu,
        help_menu;

    // We only maintain a single instance of the recent files submenu which
    // gets updated in pdgui.js via a callback from the engine.
    if (!recent_files_submenu) {
        recent_files_submenu = new gui.Menu();
        // NOTE: Since we can't be sure whether the GUI or the engine runs
        // first, make sure that we populate the submenu on the first run in
        // either case.
        pdgui.populate_recent_files(recent_files_submenu);
    }

    // OSX just spawns a single canvas menu and then enables/disables
    // the various menu items as needed.
    canvas_menu = osx || (type !== "console");

    if (osx_menu) {
        return osx_menu; // don't spawn multiple menus on OSX
    }
    // Window menu
    window_menu = new gui.Menu({ type: "menubar" });

    // On OSX, we need to start with the built-in mac menu in order to
    // get the application menu to show up correctly. Unfortunately, this
    // will also spawn a built-in "Edit" and "Window" menu. Even more
    // unfortunately, we must use the built-in "Edit" menu-- without it
    // there is no way to get <command-v> shortcut to trigger the
    // DOM "paste" event.
    if (osx) {
        window_menu.createMacBuiltin("purr-data");
    }

    // File menu
    file_menu = new gui.Menu();

    // File sub-entries
    m.file = {};
    file_menu.append(m.file.new_file = new gui.MenuItem({
        label: l("menu.new"),
        key: shortcuts.menu.new.key,
        modifiers: shortcuts.menu.new.modifiers,
        tooltip: l("menu.new_tt")
    }));
    file_menu.append(m.file.open = new gui.MenuItem({
        label: l("menu.open"),
        key: shortcuts.menu.open.key,
        modifiers: shortcuts.menu.open.modifiers,
        tooltip: l("menu.open_tt")
    }));
    file_menu.append(m.file.recent_files = new gui.MenuItem({
        label: l("menu.recent_files"),
        submenu: recent_files_submenu,
        tooltip: l("menu.recent_files_tt")
    }));
    if (pdgui.k12_mode == 1) {
        file_menu.append(m.file.k12 = new gui.MenuItem({
            label: l("menu.k12_demos"),
            tooltip: l("menu.k12_demos_tt")
        }));
    }
    file_menu.append(new gui.MenuItem({ type: "separator" }));
    if (canvas_menu) {
        file_menu.append(m.file.save = new gui.MenuItem({
            label: l("menu.save"),
            key: shortcuts.menu.save.key,
            modifiers: shortcuts.menu.save.modifiers,
            tooltip: l("menu.save_tt")
        }));
        file_menu.append(m.file.saveas = new gui.MenuItem({
            label: l("menu.saveas"),
            key: shortcuts.menu.saveas.key,
            modifiers: shortcuts.menu.saveas.modifiers,
            tooltip: l("menu.saveas_tt")
        }));
        file_menu.append(m.file.print = new gui.MenuItem({
            label: l("menu.print"),
            key: shortcuts.menu.print.key,
            modifiers: shortcuts.menu.print.modifiers,
            tooltip: l("menu.print_tt")
        }));
    }
    if (pdgui.k12_mode == 0) {
        file_menu.append(new gui.MenuItem({ type: "separator" }));
    }
    file_menu.append(m.file.message = new gui.MenuItem({
        label: l("menu.message"),
        key: shortcuts.menu.message.key,
        modifiers: shortcuts.menu.message.modifiers,
        tooltip: l("menu.message_tt")
    }));
    if (pdgui.k12_mode == 0) {
        file_menu.append(new gui.MenuItem({ type: "separator" }));
    }
    if (canvas_menu) {
        file_menu.append(m.file.close = new gui.MenuItem({
            label: l("menu.close"),
            key: shortcuts.menu.close.key,
            modifiers: shortcuts.menu.close.modifiers,
            tooltip: l("menu.close_tt")
        }));
    }
    file_menu.append(m.file.quit = new gui.MenuItem({
        label: l("menu.quit"),
        key: shortcuts.menu.quit.key,
        modifiers: shortcuts.menu.quit.modifiers 
    }));

    // Edit menu
    m.edit = {};
    // For OSX, we have to use the built-in "Edit" menu-- I haven't
    // found any other way to get "paste" event to trickle down to
    // the DOM. As a consequence, we must fetch the relevant menu
    // items that were created above with createMacBuiltin and assign
    // them to the relevant variables below
    if (osx) {
        edit_menu = window_menu.items[1].submenu;
        m.edit.undo = window_menu.items[1].submenu.items[0];
        // Remove the default Undo, since we can't seem to bind
        // a click function to it
        window_menu.items[1].submenu.remove(m.edit.undo);

        // Now create a new one and insert it at the top
        edit_menu.insert(m.edit.undo = new gui.MenuItem({
            label: l("menu.undo"),
            tooltip: l("menu.undo_tt"),
            key: shortcuts.menu.undo.key,
            modifiers: shortcuts.menu.undo.modifiers
        }), 0);

        m.edit.redo = window_menu.items[1].submenu.items[1];
        // Remove the default Undo, since we can't seem to bind
        // a click function to it
        window_menu.items[1].submenu.remove(m.edit.redo);

        // Now create a new one and insert it at the top
        edit_menu.insert(m.edit.redo = new gui.MenuItem({
            label: l("menu.redo"),
            tooltip: l("menu.redo_tt"),
            key: shortcuts.menu.redo.key,
            modifiers: shortcuts.menu.redo.modifiers
        }), 1);

        // Note: window_menu.items[1].submenu.items[2] is the separator
        m.edit.cut = window_menu.items[1].submenu.items[3];
        m.edit.copy = window_menu.items[1].submenu.items[4];
        m.edit.paste = window_menu.items[1].submenu.items[5];
        // There's no "Delete" item for GNU/Linux or Windows--
        // not sure yet what to do with it.
        m.edit.delete = window_menu.items[1].submenu.items[6];
        // The MacBuiltin "Select All" doesn't propagate down to the DOM
        //on OSX, so we have to remove it
        m.edit.selectall= window_menu.items[1].submenu.items[7];
        window_menu.items[1].submenu.remove(m.edit.selectall);
        // Now we replace it with a custom "Select All" which will
        // propagate to the DOM...
        edit_menu.append(m.edit.selectall = new gui.MenuItem({
            label: l("menu.selectall"),
            tooltip: l("menu.selectall_tt"),
            key: shortcuts.menu.selectall.key,
            modifiers: shortcuts.menu.selectall.modifiers
        }));
        // Finally, let's remove the "Delete" item since it's not hooked
        // in to anything yet...
        window_menu.items[1].submenu.remove(m.edit.delete);
    } else {
        edit_menu = new gui.Menu();
        // Edit sub-entries
        if (canvas_menu) {
            edit_menu.append(m.edit.undo = new gui.MenuItem({
                label: l("menu.undo"),
                tooltip: l("menu.undo_tt"),
                key: shortcuts.menu.undo.key,
                modifiers: shortcuts.menu.undo.modifiers
            }));
            edit_menu.append(m.edit.redo = new gui.MenuItem({
                label: l("menu.redo"),
                tooltip: l("menu.redo_tt"),
                key: shortcuts.menu.redo.key,
                modifiers: shortcuts.menu.redo.modifiers
            }));
            edit_menu.append(new gui.MenuItem({ type: "separator" }));
            edit_menu.append(m.edit.cut = new gui.MenuItem({
                label: l("menu.cut"),
                key: shortcuts.menu.cut.key,
                modifiers: shortcuts.menu.cut.modifiers,
                tooltip: l("menu.cut_tt")
            }));
        }
        edit_menu.append(m.edit.copy = new gui.MenuItem({
            label: l("menu.copy"),
            key: shortcuts.menu.copy.key,
            modifiers: shortcuts.menu.copy.modifiers,
            tooltip: l("menu.copy_tt")
        }));
        if (canvas_menu) {
            edit_menu.append(m.edit.paste = new gui.MenuItem({
                label: l("menu.paste"),
                key: shortcuts.menu.paste.key,
                modifiers: shortcuts.menu.paste.modifiers,
                tooltip: l("menu.paste_tt")
            }));
        }
    }

    // We need "duplicate" for canvas_menu and for OSX, where it's not
    // part of the builtin Edit menu...

    if (canvas_menu) {
        edit_menu.append(m.edit.paste_clipboard = new gui.MenuItem({
            label: l("menu.paste_clipboard"),
            key: shortcuts.menu.paste_clipboard.key,
            modifiers: shortcuts.menu.paste_clipboard.modifiers,
            tooltip: l("menu.paste_clipboard_tt")
        }));
        edit_menu.append(m.edit.duplicate = new gui.MenuItem({
            label: l("menu.duplicate"),
            key: shortcuts.menu.duplicate.key,
            modifiers: shortcuts.menu.duplicate.modifiers,
            tooltip: l("menu.duplicate_tt")
        }));
    }

    // OSX already has "Select All" in the builtin Edit menu...
    if (!osx) {
        edit_menu.append(m.edit.selectall = new gui.MenuItem({
            label: l("menu.selectall"),
            key: shortcuts.menu.selectall.key,
            modifiers: shortcuts.menu.selectall.modifiers,
            tooltip: l("menu.selectall_tt")
        }));
    }

    if (canvas_menu) {
        // Unfortunately nw.js doesn't allow
        // key: "Return" or key: "Enter", so we
        // can't bind to ctrl-Enter here. (Even
        // tried fromCharCode...)
        edit_menu.append(m.edit.reselect = new gui.MenuItem({
            label: l("menu.reselect"),
            key: shortcuts.menu.reselect.key,
            modifiers: shortcuts.menu.reselect.modifiers,
            tooltip: l("menu.reselect_tt")
        }));
    }
    edit_menu.append(new gui.MenuItem({ type: "separator" }));
    edit_menu.append(m.edit.clear_console = new gui.MenuItem({
        label: l("menu.clear_console"),
        tooltip: l("menu.clear_console"),
        key: shortcuts.menu.clear_console.key,
        modifiers: shortcuts.menu.clear_console.modifiers
    }));
        edit_menu.append(new gui.MenuItem({ type: "separator" }));
    if (canvas_menu) {
        edit_menu.append(m.edit.encapsulate = new gui.MenuItem({
            label: l("menu.encapsulate"),
            key: shortcuts.menu.encapsulate.key,
            modifiers: shortcuts.menu.encapsulate.modifiers,
            tooltip: l("menu.encapsulate_tt")
        }));
        edit_menu.append(m.edit.tidyup = new gui.MenuItem({
            label: l("menu.tidyup"),
            key: shortcuts.menu.tidyup.key,
            modifiers: shortcuts.menu.tidyup.modifiers,
            tooltip: l("menu.tidyup_tt")
        }));
        edit_menu.append(m.edit.font = new gui.MenuItem({
            label: l("menu.font"),
            tooltip: l("menu.font_tt")
        }));
        edit_menu.append(m.edit.cordinspector = new gui.MenuItem({
            type: "checkbox",
            label: l("menu.cordinspector"),
            key: shortcuts.menu.cordinspector.key,
            modifiers: shortcuts.menu.cordinspector.modifiers,
            tooltip: l("menu.cordinspector_tt")
        }));
        edit_menu.append(new gui.MenuItem({ type: "separator" }));
    }
    edit_menu.append(m.edit.find = new gui.MenuItem({
        label: l("menu.find"),
        key: shortcuts.menu.find.key,
        modifiers: shortcuts.menu.find.modifiers,
        tooltip: l("menu.find_tt")
    }));
    if (canvas_menu) {
        edit_menu.append(m.edit.findagain = new gui.MenuItem({
            label: l("menu.findagain"),
            key: shortcuts.menu.findagain.key,
            modifiers: shortcuts.menu.findagain.modifiers,
            tooltip: l("menu.findagain")
        }));
        edit_menu.append(m.edit.finderror = new gui.MenuItem({
            label: l("menu.finderror"),
            tooltip: l("menu.finderror_tt")
        }));
        edit_menu.append(new gui.MenuItem({ type: "separator" }));
        m.edit.autotips = new gui.MenuItem({
            label: l("menu.autotips"),
            tooltip: l("menu.autotips_tt")
        });
        // commented out because it doesn't work yet -ag
        //edit_menu.append(m.edit.autotips);
        edit_menu.append(m.edit.editmode = new gui.MenuItem({
            type: "checkbox",
            label: l("menu.editmode"),
            key: shortcuts.menu.editmode.key,
            modifiers: shortcuts.menu.editmode.modifiers,
            tooltip: l("menu.editmode_tt")
        }));
        edit_menu.append(new gui.MenuItem({ type: "separator" }));
    }
    edit_menu.append(m.edit.preferences = new gui.MenuItem({
        label: l("menu.preferences"),
        key: shortcuts.menu.preferences.key,
        modifiers: shortcuts.menu.preferences.modifiers,
        tooltip: l("menu.preferences_tt")
    }));

    // View menu
    view_menu = new gui.Menu();

    // View sub-entries
    m.view = {};
    view_menu.append(m.view.zoomin = new gui.MenuItem({
        label: l("menu.zoomin"),
        key: shortcuts.menu.zoomin.key,
        modifiers: shortcuts.menu.zoomin.modifiers,
        tooltip: l("menu.zoomin_tt")
    }));
    view_menu.append(m.view.zoomout = new gui.MenuItem({
        label: l("menu.zoomout"),
        key: shortcuts.menu.zoomout.key,
        modifiers: shortcuts.menu.zoomout.modifiers,
        tooltip: l("menu.zoomout_tt")
    }));
    view_menu.append(new gui.MenuItem({ type: "separator" }));
    view_menu.append(m.view.zoomreset = new gui.MenuItem({
        label: l("menu.zoomreset"),
        key: shortcuts.menu.zoomreset.key,
        modifiers: shortcuts.menu.zoomreset.modifiers,
        tooltip: l("menu.zoomreset_tt")
    }));
    if (canvas_menu) {
	view_menu.append(m.view.optimalzoom = new gui.MenuItem({
            label: l("menu.zoomoptimal"),
            key: shortcuts.menu.zoomoptimal.key,
            modifiers: shortcuts.menu.zoomoptimal.modifiers,
            tooltip: l("menu.zoomoptimal_tt")
	}));
	view_menu.append(m.view.horizzoom = new gui.MenuItem({
            label: l("menu.zoomhoriz"),
            key: shortcuts.menu.zoomhoriz.key,
            modifiers: shortcuts.menu.zoomhoriz.modifiers,
            tooltip: l("menu.zoomhoriz_tt")
	}));
	view_menu.append(m.view.vertzoom = new gui.MenuItem({
            label: l("menu.zoomvert"),
            key: shortcuts.menu.zoomvert.key,
            modifiers: shortcuts.menu.zoomvert.modifiers,
            tooltip: l("menu.zoomvert_tt")
	}));
    }
    view_menu.append(new gui.MenuItem({ type: "separator" }));
    view_menu.append(m.view.fullscreen = new gui.MenuItem({
        label: l("menu.fullscreen"),
        key: shortcuts.menu.fullscreen.key,
        modifiers: shortcuts.menu.fullscreen.modifiers,
        tooltip: l("menu.fullscreen_tt")
    }));

    if (canvas_menu) {
        // Put menu
        put_menu = new gui.Menu();

        // Put menu sub-entries
        m.put = {};
        put_menu.append(m.put.object = new gui.MenuItem({
            label: l("menu.object"),
            key: shortcuts.menu.object.key,
            modifiers: shortcuts.menu.object.modifiers,
            tooltip: l("menu.object_tt")
        }));
        put_menu.append(m.put.message = new gui.MenuItem({
            label: l("menu.msgbox"),
            key: shortcuts.menu.msgbox.key,
            modifiers: shortcuts.menu.msgbox.modifiers,
            tooltip: l("menu.msgbox_tt")
        }));
        put_menu.append(m.put.number = new gui.MenuItem({
            label: l("menu.number"),
            key: shortcuts.menu.number.key,
            modifiers: shortcuts.menu.number.modifiers,
            tooltip: l("menu.number_tt")
        }));
        put_menu.append(m.put.symbol = new gui.MenuItem({
            label: l("menu.symbol"),
            key: shortcuts.menu.symbol.key,
            modifiers: shortcuts.menu.symbol.modifiers,
            tooltip: l("menu.symbol_tt")
        }));
        put_menu.append(m.put.comment = new gui.MenuItem({
            label: l("menu.comment"),
            key: shortcuts.menu.comment.key,
            modifiers: shortcuts.menu.comment.modifiers,
            tooltip: l("menu.comment_tt")
        }));
        put_menu.append(new gui.MenuItem({ type: "separator" }));
        put_menu.append(m.put.dropdown = new gui.MenuItem({
            label: l("menu.dropdown"),
            key: shortcuts.menu.dropdown.key,
            modifiers: shortcuts.menu.dropdown.modifiers,
            tooltip: l("menu.dropdown_tt")
        }));
        put_menu.append(new gui.MenuItem({ type: "separator" }));
        put_menu.append(m.put.bang = new gui.MenuItem({
            label: l("menu.bang"),
            key: shortcuts.menu.bang.key,
            modifiers: shortcuts.menu.bang.modifiers,
            tooltip: l("menu.bang_tt")
        }));
        put_menu.append(m.put.toggle = new gui.MenuItem({
            label: l("menu.toggle"),
            key: shortcuts.menu.toggle.key,
            modifiers: shortcuts.menu.toggle.modifiers,
            tooltip: l("menu.toggle_tt")
        }));
        put_menu.append(m.put.number2 = new gui.MenuItem({
            label: l("menu.number2"),
            key: shortcuts.menu.number2.key,
            modifiers: shortcuts.menu.number2.modifiers,
            tooltip: l("menu.number2")
        }));
        put_menu.append(m.put.vslider = new gui.MenuItem({
            label: l("menu.vslider"),
            key: shortcuts.menu.vslider.key,
            modifiers: shortcuts.menu.vslider.modifiers,
            tooltip: l("menu.vslider_tt")
        }));
        put_menu.append(m.put.hslider = new gui.MenuItem({
            label: l("menu.hslider"),
            key: shortcuts.menu.hslider.key,
            modifiers: shortcuts.menu.hslider.modifiers,
            tooltip: l("menu.hslider_tt")
        }));
        put_menu.append(m.put.vradio = new gui.MenuItem({
            label: l("menu.vradio"),
            key: shortcuts.menu.vradio.key,
            modifiers: shortcuts.menu.vradio.modifiers,
            tooltip: l("menu.vradio_tt")
        }));
        put_menu.append(m.put.hradio = new gui.MenuItem({
            label: l("menu.hradio"),
            key: shortcuts.menu.hradio.key,
            modifiers: shortcuts.menu.hradio.modifiers,
            tooltip: l("menu.hradio_tt")
        }));
        put_menu.append(m.put.vu = new gui.MenuItem({
            label: l("menu.vu"),
            key: shortcuts.menu.vu.key,
            modifiers: shortcuts.menu.vu.modifiers,
            tooltip: l("menu.vu_tt")
        }));
        put_menu.append(m.put.cnv = new gui.MenuItem({
            label: l("menu.cnv"),
            key: shortcuts.menu.cnv.key,
            modifiers: shortcuts.menu.cnv.modifiers,
            tooltip: l("menu.cnv_tt")
        }));
        put_menu.append(new gui.MenuItem({ type: "separator" }));
        //putMenu.append(m.put.graph = new gui.MenuItem());
        put_menu.append(m.put.array = new gui.MenuItem({
            label: l("menu.array"),
            tooltip: l("menu.array_tt")
        }));
    }

    // Windows menu... call it "winman" (i.e., window management)
    // to avoid confusion
    if (osx) {
        // on OSX, createMacBuiltin creates a window menu
        winman_menu = window_menu.items[2].submenu;
    } else {
        winman_menu = new gui.Menu();
    }
    // Win sub-entries
    m.win = {};
    winman_menu.append(m.win.nextwin = new gui.MenuItem({
        label: l("menu.nextwin"),
        key: shortcuts.menu.nextwin.key,
        modifiers: shortcuts.menu.nextwin.modifiers,
        tooltip: l("menu.nextwin_tt")
    }));
    winman_menu.append(m.win.prevwin = new gui.MenuItem({
        label: l("menu.prevwin"),
        key: shortcuts.menu.prevwin.key,
        modifiers: shortcuts.menu.prevwin.modifiers,
        tooltip: l("menu.prevwin_tt")
    }));
    if (canvas_menu) {
        winman_menu.append(new gui.MenuItem({ type: "separator" }));
        winman_menu.append(m.win.parentwin = new gui.MenuItem({
            label: l("menu.parentwin"),
            tooltip: l("menu.parentwin_tt")
        }));
        winman_menu.append(m.win.visible_ancestor = new gui.MenuItem({
            label: l("menu.visible_ancestor"),
            tooltip: l("menu.visible_ancestor_tt")
        }));
        winman_menu.append(m.win.pdwin = new gui.MenuItem({
            label: l("menu.pdwin"),
            tooltip: l("menu.pdwin_tt"),
            key: shortcuts.menu.pdwin.key,
            modifiers: shortcuts.menu.pdwin.modifiers
        }));
    }

    // Media menu
    media_menu = new gui.Menu();

    // Media sub-entries
    m.media = {};
    media_menu.append(m.media.audio_on = new gui.MenuItem({
        label: l("menu.audio_on"),
        key: shortcuts.menu.audio_on.key,
        modifiers: shortcuts.menu.audio_on.modifiers,
        tooltip: l("menu.audio_on_tt")
    }));
    media_menu.append(m.media.audio_off = new gui.MenuItem({
        label: l("menu.audio_off"),
        key: shortcuts.menu.audio_off.key,
        modifiers: shortcuts.menu.audio_off.modifiers,
        tooltip: l("menu.audio_off_tt")
    }));
    media_menu.append(new gui.MenuItem({ type: "separator" }));
    media_menu.append(m.media.test = new gui.MenuItem({
        label: l("menu.test"),
        tooltip: l("menu.test_tt")
    }));
    media_menu.append(m.media.loadmeter = new gui.MenuItem({
        label: l("menu.loadmeter"),
        tooltip: l("menu.loadmeter_tt")
    }));

    // Help menu
    help_menu = new gui.Menu();

    // Help sub-entries
    m.help = {};
    help_menu.append(m.help.about = new gui.MenuItem({
        label: l("menu.about"),
        tooltip: l("menu.about_tt")
    }));
    help_menu.append(m.help.manual = new gui.MenuItem({
        label: l("menu.manual"),
        tooltip: l("menu.manual_tt")
    }));
    help_menu.append(m.help.browser = new gui.MenuItem({
        label: l("menu.browser"),
        key: shortcuts.menu.browser.key,
        modifiers: shortcuts.menu.browser.modifiers,
        tooltip: l("menu.browser_tt")
    }));
    help_menu.append(m.help.intro = new gui.MenuItem({
        label: l("menu.intro"),
        tooltip: l("menu.intro_tt")
    }));
    help_menu.append(new gui.MenuItem({ type: "separator" }));
    help_menu.append(m.help.l2ork_list = new gui.MenuItem({
        label: l("menu.l2ork_list"),
        tooltip: l("menu.l2ork_list_tt")
    }));
    help_menu.append(m.help.pd_list = new gui.MenuItem({
        label: l("menu.pd_list"),
        tooltip: l("menu.pd_list_tt")
    }));
    help_menu.append(m.help.forums = new gui.MenuItem({
        label: l("menu.forums"),
        tooltip: l("menu.forums_tt")
    }));
    help_menu.append(m.help.irc = new gui.MenuItem({
        label: l("menu.irc"),
        tooltip: l("menu.irc_tt")
    }));
    help_menu.append(m.help.devtools = new gui.MenuItem({
        label: l("menu.devtools"),
        tooltip: l("menu.devtools_tt")
    }));

    // Add submenus to window menu
    if (osx) {
        window_menu.insert(new gui.MenuItem({
            label: l("menu.file"),
            submenu: file_menu
        }), 1);
        // Edit menu created from mac builtin above
        window_menu.insert(new gui.MenuItem({
            label: l("menu.view"),
            submenu: view_menu
        }), 3);
        window_menu.insert(new gui.MenuItem({
            label: l("menu.put"),
            submenu: put_menu
        }), 4);
        window_menu.insert(new gui.MenuItem({
            label: l("menu.media"),
            submenu: media_menu
        }), 5);
        // "Window" menu created from mac builtin above
        window_menu.append(new gui.MenuItem({
            label: l("menu.help"),
            submenu: help_menu
        }));
    } else {
        window_menu.append(new gui.MenuItem({
            label: l("menu.file"),
            submenu: file_menu
        }));
        window_menu.append(new gui.MenuItem({
            label: l("menu.edit"),
            submenu: edit_menu
        }));
        window_menu.append(new gui.MenuItem({
            label: l("menu.view"),
            submenu: view_menu
        }));
        if (canvas_menu) {
            window_menu.append(new gui.MenuItem({
                label: l("menu.put"),
                submenu: put_menu
            }));
        }
        window_menu.append(new gui.MenuItem({
            label: l("menu.media"),
            submenu: media_menu
        }));
        window_menu.append(new gui.MenuItem({
            label: l("menu.windows"),
            submenu: winman_menu
        }));
        window_menu.append(new gui.MenuItem({
            label: l("menu.help"),
            submenu: help_menu
        }));
    }

    // Assign to window
    gui.Window.get().menu = window_menu;

    // If we're on OSX, store the object
    if (process.platform === "darwin") {
        osx_menu = m;
    }
    return m;
}

exports.create_menu = create_menu;
