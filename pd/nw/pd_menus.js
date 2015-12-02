"use strict";

var pdgui = require("./pdgui.js");
var l = pdgui.get_local_string; // For menu names
var osx_menu = null; // OSX App menu

function create_menu(gui, type) {
    // type is "console" only on Windows and GNU/Linux. On OSX we
    // create a menu only once, and then enable/disable menuitems and
    // switch out functions as needed.
    var m = {};
    if (osx_menu) {
        return osx_menu;
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
    fileMenu.append(m.file.new_file = new gui.MenuItem({label: l("menu.new")}));
    fileMenu.append(m.file.open = new gui.MenuItem({label: l("menu.open")}));
    if (pdgui.k12_mode == 1) {
        fileMenu.append(m.file.k12 =
            new gui.MenuItem({label: l("menu.k12_demos")}));
    }
    fileMenu.append(new gui.MenuItem({ type: "separator" }));

    if (type !== "console") {
        fileMenu.append(m.file.save =
            new gui.MenuItem({ label: l("menu.save")}));
        fileMenu.append(m.file.saveas =
            new gui.MenuItem({label: l("menu.saveas")}));
    }

    if (pdgui.k12_mode == 0) {
        fileMenu.append(new gui.MenuItem({ type: "separator" }));
    }
    fileMenu.append(m.file.message =
        new gui.MenuItem({ label: l("menu.message")}));
    if (pdgui.k12_mode == 0) {
        fileMenu.append(new gui.MenuItem({ type: "separator" }));
    }
    if (type !== "console") {
        fileMenu.append(m.file.close =
            new gui.MenuItem({ label: l("menu.close")}));
    }
    fileMenu.append(m.file.quit = new gui.MenuItem({ label: l("menu.quit") }));

    // Edit menu
    var editMenu = new gui.Menu();

    // Add to window menu
    windowMenu.append(new gui.MenuItem({
    label: l("menu.edit"),
    submenu: editMenu
    }));

    // Edit sub-entries
    m.edit = {};
    if (type !== "console") {
//      editMenu.append(modals.undo = new gui.MenuItem());
        editMenu.append(m.edit.undo =
            new gui.MenuItem({ label: l("menu.undo") }));
//      editMenu.append(modals.redo = new gui.MenuItem());
        editMenu.append(m.edit.redo =
            new gui.MenuItem({ label: l("menu.redo") }));
        editMenu.append(new gui.MenuItem({ type: "separator" }));
//      editMenu.append(modals.cut = new gui.MenuItem());
        editMenu.append(m.edit.cut =
            new gui.MenuItem({ label: l("menu.cut") }));
    }
//    editMenu.append(modals.copy = new gui.MenuItem());
    editMenu.append(m.edit.copy =
        new gui.MenuItem({ label: l("menu.copy") }));
    if (type !== "console") {
//      editMenu.append(modals.paste = new gui.MenuItem());
        editMenu.append(m.edit.paste =
            new gui.MenuItem({ label: l("menu.paste") }));
        editMenu.append(m.edit.duplicate =
            new gui.MenuItem({ label: l("menu.duplicate") }));
    }
    editMenu.append(m.edit.selectall =
        new gui.MenuItem({ label: l("menu.selectall") }));
    if (type !== "console") {
        editMenu.append(m.edit.reselect =
            new gui.MenuItem({ label: l("menu.reselect") }));
    }
    editMenu.append(new gui.MenuItem({ type: "separator" }));
    editMenu.append(m.edit.zoomin =
        new gui.MenuItem({ label: l("menu.zoomin") }));
    editMenu.append(m.edit.zoomout =
        new gui.MenuItem({ label: l("menu.zoomout") }));
    editMenu.append(new gui.MenuItem({ type: "separator" }));
    if (type !== "console") {
        editMenu.append(m.edit.tidyup =
            new gui.MenuItem({ label: l("menu.tidyup") }));
        editMenu.append(m.edit.tofront =
            new gui.MenuItem({ label: l("menu.tofront") }));
        editMenu.append(m.edit.toback =
            new gui.MenuItem({ label: l("menu.toback") }));
        editMenu.append(new gui.MenuItem({ type: "separator" }));
        editMenu.append(m.edit.font =
            new gui.MenuItem({ label: l("menu.font") }));
        editMenu.append(m.edit.cordinspector =
            new gui.MenuItem({ label: l("menu.cordinspector") }));
        editMenu.append(new gui.MenuItem({ type: "separator" }));
    }
    editMenu.append(m.edit.find =
        new gui.MenuItem({ label: l("menu.find") }));
    if (type !== "console") {
        editMenu.append(m.edit.findagain =
            new gui.MenuItem({ label: l("menu.findagain") }));
        editMenu.append(m.edit.finderror =
            new gui.MenuItem({ label: l("menu.finderror") }));
        editMenu.append(new gui.MenuItem({ type: "separator" }));
        editMenu.append(m.edit.autotips =
            new gui.MenuItem({ label: l("menu.autotips") }));
        editMenu.append(m.edit.editmode =
            new gui.MenuItem({ label: l("menu.editmode") }));
        editMenu.append(new gui.MenuItem({ type: "separator" }));
    }
    editMenu.append(m.edit.preferences =
        new gui.MenuItem({ label: l("menu.preferences") }));

    if (type !== "console") {
        // Put menu
        var putMenu = new gui.Menu();

        // Add to window menu
        windowMenu.append(new gui.MenuItem({
        label: l("menu.put"),
        submenu: putMenu
        }));

        // Put menu sub-entries
        m.put = {};
        putMenu.append(m.put.object =
            new gui.MenuItem({ label: l("menu.object") }));
        putMenu.append(m.put.message =
            new gui.MenuItem({ label: l("menu.msgbox") }));
        putMenu.append(m.put.number =
            new gui.MenuItem({ label: l("menu.number") }));
        putMenu.append(m.put.symbol =
            new gui.MenuItem({ label: l("menu.symbol") }));
        putMenu.append(m.put.comment =
            new gui.MenuItem({ label: l("menu.comment") }));
        putMenu.append(new gui.MenuItem({ type: "separator" }));
        putMenu.append(m.put.bang =
            new gui.MenuItem({ label: l("menu.bang") }));
        putMenu.append(m.put.toggle =
            new gui.MenuItem({ label: l("menu.toggle") }));
        putMenu.append(m.put.number2 =
            new gui.MenuItem({ label: l("menu.number2") }));
        putMenu.append(m.put.vslider =
            new gui.MenuItem({ label: l("menu.vslider") }));
        putMenu.append(m.put.hslider =
            new gui.MenuItem({ label: l("menu.hslider") }));
        putMenu.append(m.put.vradio =
            new gui.MenuItem({ label: l("menu.vradio") }));
        putMenu.append(m.put.hradio =
            new gui.MenuItem({ label: l("menu.hradio") }));
        putMenu.append(m.put.vu =
            new gui.MenuItem({ label: l("menu.vu") }));
        putMenu.append(m.put.cnv =
            new gui.MenuItem({ label: l("menu.cnv") }));
        putMenu.append(new gui.MenuItem({ type: "separator" }));
        //putMenu.append(m.put.graph = new gui.MenuItem());
        putMenu.append(m.put.array =
            new gui.MenuItem({ label: l("menu.array") }));
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
    if (type !== "console") {
        winmanMenu.append(m.win.fullscreen =
            new gui.MenuItem({ label: l("menu.fullscreen") }));
    }
    winmanMenu.append(m.win.nextwin =
        new gui.MenuItem({ label: l("menu.nextwin") }));
    winmanMenu.append(m.win.prevwin =
        new gui.MenuItem({ label: l("menu.prevwin") }));
    if (type !== "console") {
        winmanMenu.append(new gui.MenuItem({ type: "separator" }));
        winmanMenu.append(m.win.parentwin =
            new gui.MenuItem({ label: l("menu.parentwin") }));
        winmanMenu.append(m.win.visible_ancestor =
            new gui.MenuItem({ label: l("menu.visible_ancestor") }));
        winmanMenu.append(m.win.pdwin =
            new gui.MenuItem({ label: l("menu.pdwin") }));
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
    mediaMenu.append(m.media.audio_on =
        new gui.MenuItem({ label: l("menu.audio_on") }));
    mediaMenu.append(m.media.audio_off =
        new gui.MenuItem({ label: l("menu.audio_off") }));
    mediaMenu.append(new gui.MenuItem({ type: "separator" }));
    mediaMenu.append(m.media.test =
        new gui.MenuItem({ label: l("menu.test") }));
    mediaMenu.append(m.media.loadmeter =
        new gui.MenuItem({ label: l("menu.loadmeter") }));

    // Help menu
    var helpMenu = new gui.Menu();

    // Add to window menu
    windowMenu.append(new gui.MenuItem({
    label: l("menu.help"),
    submenu: helpMenu
    }));

    // Help sub-entries
    m.help = {};
    helpMenu.append(m.help.about =
        new gui.MenuItem({ label: l("menu.about") }));
    helpMenu.append(m.help.manual =
        new gui.MenuItem({ label: l("menu.manual") }));
    helpMenu.append(m.help.browser =
        new gui.MenuItem({ label: l("menu.browser") }));
    helpMenu.append(new gui.MenuItem({ type: "separator" }));
    helpMenu.append(m.help.l2ork_list =
        new gui.MenuItem({ label: l("menu.l2ork_list") }));
    helpMenu.append(m.help.pd_list =
        new gui.MenuItem({ label: l("menu.pd_list") }));
    helpMenu.append(m.help.forums =
        new gui.MenuItem({ label: l("menu.forums") }));
    helpMenu.append(m.help.irc =
        new gui.MenuItem({ label: l("menu.irc") }));
    helpMenu.append(m.help.devtools =
        new gui.MenuItem({ label: l("menu.devtools") }));

    // Assign to window
    gui.Window.get().menu = windowMenu;

    // If we're on OSX, store the object
    if (process.platform === "darwin") {
        osx_menu = m;
    }
    return m;
}

exports.create_menu = create_menu;
