"use strict";

var pdgui = require("./pdgui.js");
var l = pdgui.get_local_string; // For menu names
var osx_menu = null; // OSX App menu -- a single one per running instance

function create_menu(gui, type) {
    // On OSX we create a menu only once, and then enable/disable menuitems
    // and switch out functions as needed.

    // We specify the label here because nw.js won't create a menu item
    // without one. We also specify the keyboard shortcuts here because
    // nw.js won't create an event listener unless you make the
    // shortcut immediately when creating the menu item. (It also
    // won't let you update the keyboard shortcut binding later.)
    var m = {};

    var osx = process.platform === "darwin",
        cmd_or_ctrl = osx ? "cmd" : "ctrl"; // for keybindings

    // OSX just spawns a single canvas menu and then enables/disables
    // the various menu items as needed.
    var canvas_menu = osx || (type !== "console");

    if (osx_menu) {
        return osx_menu; // don't spawn multiple menus on OSX
    }
    // Window menu
    var windowMenu = new gui.Menu({ type: "menubar" });

    // File menu
    var fileMenu = new gui.Menu();

    // Add to window menu
    windowMenu.append(new gui.MenuItem({
        label: l("menu.file"),
        submenu: fileMenu
    }));

    // File sub-entries
    m.file = {};
    fileMenu.append(m.file.new_file = new gui.MenuItem({
        label: l("menu.new"),
        key: "n",
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.new_tt")
    }));
    fileMenu.append(m.file.open = new gui.MenuItem({
        label: l("menu.open"),
        key: "o",
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.open_tt")
    }));
    if (pdgui.k12_mode == 1) {
        fileMenu.append(m.file.k12 = new gui.MenuItem({
            label: l("menu.k12_demos"),
            tooltip: l("menu.k12_demos_tt")
        }));
    }
    fileMenu.append(new gui.MenuItem({ type: "separator" }));

    if (canvas_menu) {
        fileMenu.append(m.file.save = new gui.MenuItem({
            label: l("menu.save"),
            key: "s",
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.save_tt")
        }));
        fileMenu.append(m.file.saveas = new gui.MenuItem({
            label: l("menu.saveas"),
            key: "s",
            modifiers: cmd_or_ctrl + "+shift",
            tooltip: l("menu.saveas_tt")
        }));
    }
    if (pdgui.k12_mode == 0) {
        fileMenu.append(new gui.MenuItem({ type: "separator" }));
    }
    fileMenu.append(m.file.message = new gui.MenuItem({
        label: l("menu.message"),
        key: "m",
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.message_tt")
    }));
    if (pdgui.k12_mode == 0) {
        fileMenu.append(new gui.MenuItem({ type: "separator" }));
    }
    if (canvas_menu) {
        fileMenu.append(m.file.close = new gui.MenuItem({
            label: l("menu.close"),
            key: "w",
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.close_tt"),
        }));
    }
    fileMenu.append(m.file.quit = new gui.MenuItem({
        label: l("menu.quit"),
        key: "q",
        modifiers: cmd_or_ctrl
    }));

    // Edit menu
    var editMenu = new gui.Menu();

    // Add to window menu
    windowMenu.append(new gui.MenuItem({
        label: l("menu.edit"),
        submenu: editMenu
    }));

    // Edit sub-entries
    m.edit = {};
    if (canvas_menu) {
        editMenu.append(m.edit.undo = new gui.MenuItem({
            label: l("menu.undo"),
            tooltip: l("menu.undo_tt")
        }));
        editMenu.append(m.edit.redo = new gui.MenuItem({
            label: l("menu.redo"),
            tooltip: l("menu.redo_tt")
        }));
        editMenu.append(new gui.MenuItem({ type: "separator" }));
        editMenu.append(m.edit.cut = new gui.MenuItem({
            label: l("menu.cut"),
            tooltip: l("menu.cut_tt")
        }));
    }
    editMenu.append(m.edit.copy = new gui.MenuItem({
        label: l("menu.copy"),
        tooltip: l("menu.copy_tt")
    }));
    if (canvas_menu) {
        editMenu.append(m.edit.paste = new gui.MenuItem({
            label: l("menu.paste"),
            tooltip: l("menu.paste_tt")
        }));
        editMenu.append(m.edit.duplicate = new gui.MenuItem({
            label: l("menu.duplicate"),
            key: "d",
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.duplicate_tt")
        }));
    }
    editMenu.append(m.edit.selectall = new gui.MenuItem({
        label: l("menu.selectall"),
        tooltip: l("menu.selectall_tt")
    }));
    if (canvas_menu) {
        // Unfortunately nw.js doesn't allow
        // key: "Return" or key: "Enter", so we
        // can't bind to ctrl-Enter here. (Even
        // tried fromCharCode...)
        editMenu.append(m.edit.reselect = new gui.MenuItem({
            label: l("menu.reselect"),
            key: String.fromCharCode(10),
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.reselect_tt")
        }));
    }
    editMenu.append(new gui.MenuItem({ type: "separator" }));
    editMenu.append(m.edit.clear_console = new gui.MenuItem({
        label: l("menu.clear_console"),
        tooltip: l("menu.clear_console"),
        key: "l",
        modifiers: "shift+" + cmd_or_ctrl
    }));
        editMenu.append(new gui.MenuItem({ type: "separator" }));
    if (canvas_menu) {
        editMenu.append(m.edit.tidyup = new gui.MenuItem({
            label: l("menu.tidyup"),
            key: "y",
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.tidyup_tt")
        }));
        editMenu.append(m.edit.tofront = new gui.MenuItem({
            label: l("menu.tofront"),
            tooltip: l("menu.tofront_tt")
        }));
        editMenu.append(m.edit.toback = new gui.MenuItem({
            label: l("menu.toback"),
            tooltip: l("menu.toback_tt")
        }));
        editMenu.append(new gui.MenuItem({ type: "separator" }));
        editMenu.append(m.edit.font = new gui.MenuItem({
            label: l("menu.font"),
            tooltip: l("menu.font_tt")
        }));
        editMenu.append(m.edit.cordinspector = new gui.MenuItem({
            label: l("menu.cordinspector"),
            key: "r",
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.cordinspector_tt")
        }));
        editMenu.append(new gui.MenuItem({ type: "separator" }));
    }
    editMenu.append(m.edit.find = new gui.MenuItem({
        label: l("menu.find"),
        key: "f",
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.find_tt")
    }));
    if (canvas_menu) {
        editMenu.append(m.edit.findagain = new gui.MenuItem({
            label: l("menu.findagain"),
            key: "g",
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.findagain")
        }));
        editMenu.append(m.edit.finderror = new gui.MenuItem({
            label: l("menu.finderror"),
            tooltip: l("menu.finderror_tt")
        }));
        editMenu.append(new gui.MenuItem({ type: "separator" }));
        editMenu.append(m.edit.autotips = new gui.MenuItem({
            label: l("menu.autotips"),
            tooltip: l("menu.autotips_tt")
        }));
        editMenu.append(m.edit.editmode = new gui.MenuItem({
            label: l("menu.editmode"),
            key: "e",
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.editmode_tt")
        }));
        editMenu.append(new gui.MenuItem({ type: "separator" }));
    }
    editMenu.append(m.edit.preferences = new gui.MenuItem({
        label: l("menu.preferences"),
        key: "p",
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.preferences_tt")
    }));

    // View menu
    var viewMenu = new gui.Menu();

    // Add to window menu
    windowMenu.append(new gui.MenuItem({
        label: l("menu.view"),
        submenu: viewMenu
    }));

    // View sub-entries
    m.view= {};
    viewMenu.append(m.view.zoomin = new gui.MenuItem({
        label: l("menu.zoomin"),
        key: "=",
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.zoomin_tt")
    }));
    viewMenu.append(m.view.zoomout = new gui.MenuItem({
        label: l("menu.zoomout"),
        key: "-",
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.zoomout_tt")
    }));
    viewMenu.append(new gui.MenuItem({ type: "separator" }));
    viewMenu.append(m.view.zoomreset = new gui.MenuItem({
        label: l("menu.zoomreset"),
        key: "0",
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.zoomreset_tt")
    }));
    viewMenu.append(new gui.MenuItem({ type: "separator" }));
    viewMenu.append(m.view.fullscreen = new gui.MenuItem({
        label: l("menu.fullscreen"),
        key: process.platform === "darwin" ? "f" : "f11",
        modifiers: process.platform === "darwin" ? "cmd+ctrl" : null,
        tooltip: l("menu.fullscreen_tt")
    }));

    if (canvas_menu) {
        // Put menu
        var putMenu = new gui.Menu();

        // Add to window menu
        windowMenu.append(new gui.MenuItem({
        label: l("menu.put"),
        submenu: putMenu
        }));

        // Put menu sub-entries
        m.put = {};
        putMenu.append(m.put.object = new gui.MenuItem({
            label: l("menu.object"),
            key: "1",
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.object_tt")
        }));
        putMenu.append(m.put.message = new gui.MenuItem({
            label: l("menu.msgbox"),
            key: "2",
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.msgbox_tt")
        }));
        putMenu.append(m.put.number = new gui.MenuItem({
            label: l("menu.number"),
            key: "3",
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.number_tt")
        }));
        putMenu.append(m.put.symbol = new gui.MenuItem({
            label: l("menu.symbol"),
            key: "4",
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.symbol_tt")
        }));
        putMenu.append(m.put.comment = new gui.MenuItem({
            label: l("menu.comment"),
            key: "5",
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.comment_tt")
        }));
        putMenu.append(new gui.MenuItem({ type: "separator" }));
        putMenu.append(m.put.bang = new gui.MenuItem({
            label: l("menu.bang"),
            key: "b",
            modifiers: cmd_or_ctrl + "+shift",
            tooltip: l("menu.bang_tt")
        }));
        putMenu.append(m.put.toggle = new gui.MenuItem({
            label: l("menu.toggle"),
            key: "t",
            modifiers: cmd_or_ctrl + "+shift",
            tooltip: l("menu.toggle_tt")
        }));
        putMenu.append(m.put.number2 = new gui.MenuItem({
            label: l("menu.number2"),
            key: "n",
            modifiers: cmd_or_ctrl + "+shift",
            tooltip: l("menu.number2")
        }));
        putMenu.append(m.put.vslider = new gui.MenuItem({
            label: l("menu.vslider"),
            key: "v",
            modifiers: cmd_or_ctrl + "+shift",
            tooltip: l("menu.vslider_tt")
        }));
        putMenu.append(m.put.hslider = new gui.MenuItem({
            label: l("menu.hslider"),
            key: "h",
            modifiers: cmd_or_ctrl + "+shift",
            tooltip: l("menu.hslider_tt")
        }));
        putMenu.append(m.put.vradio = new gui.MenuItem({
            label: l("menu.vradio"),
            key: "d",
            modifiers: cmd_or_ctrl + "+shift",
            tooltip: l("menu.vradio_tt")
        }));
        putMenu.append(m.put.hradio = new gui.MenuItem({
            label: l("menu.hradio"),
            key: "i",
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.hradio_tt")
        }));
        putMenu.append(m.put.vu = new gui.MenuItem({
            label: l("menu.vu"),
            key: "u",
            modifiers: cmd_or_ctrl,
            tooltip: l("menu.vu_tt")
        }));
        putMenu.append(m.put.cnv = new gui.MenuItem({
            label: l("menu.cnv"),
            key: "c",
            modifiers: cmd_or_ctrl + "+shift",
            tooltip: l("menu.cnv_tt")
        }));
        putMenu.append(new gui.MenuItem({ type: "separator" }));
        //putMenu.append(m.put.graph = new gui.MenuItem());
        putMenu.append(m.put.array = new gui.MenuItem({
            label: l("menu.array"),
            tooltip: l("menu.array_tt")
        }));
    }

    // Windows menu... call it "winman" (i.e., window management)
    // to avoid confusion
    var winmanMenu = new gui.Menu();

    // Add to windows menu
    windowMenu.append(new gui.MenuItem({
    label: l("menu.windows"),
    submenu: winmanMenu
    }));

    // Win sub-entries
    m.win = {};
    winmanMenu.append(m.win.nextwin = new gui.MenuItem({
        label: l("menu.nextwin"),
        key: String.fromCharCode(12), // Page down
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.nextwin_tt")
    }));
    winmanMenu.append(m.win.prevwin = new gui.MenuItem({
        label: l("menu.prevwin"),
        key: String.fromCharCode(11), // Page up
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.prevwin_tt")
    }));
    if (canvas_menu) {
        winmanMenu.append(new gui.MenuItem({ type: "separator" }));
        winmanMenu.append(m.win.parentwin = new gui.MenuItem({
            label: l("menu.parentwin"),
            tooltip: l("menu.parentwin_tt")
        }));
        winmanMenu.append(m.win.visible_ancestor = new gui.MenuItem({
            label: l("menu.visible_ancestor"),
            tooltip: l("menu.visible_ancestor_tt")
        }));
        winmanMenu.append(m.win.pdwin = new gui.MenuItem({
            label: l("menu.pdwin"),
            tooltip: l("menu.pdwin_tt"),
            key: "r",
            modifiers: cmd_or_ctrl 
        }));
    }
    // Media menu
    var mediaMenu = new gui.Menu();

    // Add to window menu
    windowMenu.append(new gui.MenuItem({
    label: l("menu.media"),
    submenu: mediaMenu
    }));

    // Media sub-entries
    m.media = {};
    mediaMenu.append(m.media.audio_on = new gui.MenuItem({
        label: l("menu.audio_on"),
        key: "/",
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.audio_on_tt")
    }));
    mediaMenu.append(m.media.audio_off = new gui.MenuItem({
        label: l("menu.audio_off"),
        key: ".",
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.audio_off_tt")
    }));
    mediaMenu.append(new gui.MenuItem({ type: "separator" }));
    mediaMenu.append(m.media.test = new gui.MenuItem({
        label: l("menu.test"),
        tooltip: l("menu.test_tt")
    }));
    mediaMenu.append(m.media.loadmeter = new gui.MenuItem({
        label: l("menu.loadmeter"),
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
    m.help = {};
    helpMenu.append(m.help.about = new gui.MenuItem({
        label: l("menu.about"),
        tooltip: l("menu.about_tt")
    }));
    helpMenu.append(m.help.manual = new gui.MenuItem({
        label: l("menu.manual"),
        tooltip: l("menu.manual"),
    }));
    helpMenu.append(m.help.browser = new gui.MenuItem({
        label: l("menu.browser"),
        tooltip: l("menu.browser_tt")
    }));
    helpMenu.append(new gui.MenuItem({ type: "separator" }));
    helpMenu.append(m.help.l2ork_list = new gui.MenuItem({
        label: l("menu.l2ork_list"),
        tooltip: l("menu.l2ork_list_tt")
    }));
    helpMenu.append(m.help.pd_list = new gui.MenuItem({
        label: l("menu.pd_list"),
        tooltip: l("menu.pd_list_tt")
    }));
    helpMenu.append(m.help.forums = new gui.MenuItem({
        label: l("menu.forums"),
        tooltip: l("menu.forums_tt")
    }));
    helpMenu.append(m.help.irc = new gui.MenuItem({
        label: l("menu.irc"),
        tooltip: l("menu.irc_tt")
    }));
    helpMenu.append(m.help.devtools = new gui.MenuItem({
        label: l("menu.devtools"),
        key:"b",
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.devtools_tt")
    }));

    // Assign to window
    gui.Window.get().menu = windowMenu;

    // If we're on OSX, store the object
    if (process.platform === "darwin") {
        osx_menu = m;
    }
    return m;
}

exports.create_menu = create_menu;
