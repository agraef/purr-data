var pdgui = require("./pdgui.js");

// For translations
var l = pdgui.get_local_string;

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
function nw_create_pd_window_menus(gui, w) {
    // Command key for OSX, Control for GNU/Linux and Windows
    var cmd_or_ctrl = process.platform === "darwin" ? "cmd" : "ctrl";
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
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.new.tt")
    }));

    fileMenu.append(new gui.MenuItem({
        label: l("menu.open"),
        key: "o",
        modifiers: cmd_or_ctrl,
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
            chooser.onchange = function() {
                var file_array = this.value;
                // reset value so that we can open the same file twice
                this.value = null;
                pdgui.menu_open(file_array);
                console.log("tried to open something");
            };
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
        modifiers: cmd_or_ctrl
    }));

    fileMenu.append(new gui.MenuItem({
        label: l("menu.saveas"),
        click: function (){},
        enabled: false,
        key: "S",
        tooltip: l("menu.saveas_tt"),
        modifiers: cmd_or_ctrl
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
        modifiers: cmd_or_ctrl,
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
        modifiers: cmd_or_ctrl,
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
        modifiers: cmd_or_ctrl,
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
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.selectall_tt")
    }));

    editMenu.append(new gui.MenuItem({
        type: "separator"
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.zoomin"),
        click: function () {
            gui.Window.get().zoomLevel += 1;
        },
        key: "=",
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.zoomin_tt")
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.zoomout"),
        click: function () {
            gui.Window.get().zoomLevel -= 1;
        },
        key: "-",
        modifiers: cmd_or_ctrl,
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
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.find_tt")
    }));

    editMenu.append(new gui.MenuItem({
        label: l("menu.preferences"),
        click: pdgui.open_prefs,
        key: "p",
        modifiers: cmd_or_ctrl,
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
        click: function() {
            pdgui.raise_next("pd_window");
        },
        //key: "c",
        //modifiers: cmd_or_ctrl,
        tooltip: l("menu.nextwin_tt")
    }));

    winmanMenu.append(new gui.MenuItem({
        label: l("menu.prevwin"),
        click: function() {
            pdgui.raise_prev("pd_window");
        },
        //key: "a",
        //modifiers: cmd_or_ctrl,
        tooltip: l("menu.prevwin_tt")
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
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.audio_on_tt")
    }));

    mediaMenu.append(new gui.MenuItem({
        label: l("menu.audio_off"),
        click: function() {
            pdgui.pdsend("pd dsp 0");
        },
        key: ".",
        modifiers: cmd_or_ctrl,
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
        //modifiers: cmd_or_ctrl,
        tooltip: l("menu.test_tt")
    }));

    mediaMenu.append(new gui.MenuItem({
        label: l("menu.loadmeter"),
        click: function() {
            pdgui.pd_doc_open("doc/7.stuff/tools", "load-meter.pd");
        },
        //key: "a",
        //modifiers: cmd_or_ctrl,
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
        //modifiers: cmd_or_ctrl,
        tooltip: l("menu.about_tt")
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.manual"),
        click: function() {
            pdgui.pd_doc_open("doc/1.manual", "index.htm");
        },
        //key: "a",
        //modifiers: cmd_or_ctrl,
        tooltip: l("menu.manual_tt")
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.browser"),
        click: pdmenu_help_browser,
        //key: "a",
        //modifiers: cmd_or_ctrl,
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
        //modifiers: cmd_or_ctrl,
        tooltip: l("menu.l2ork_list_tt")
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.pd_list"),
        click: function() {
            pdgui.external_doc_open("http://puredata.info/community/lists");
        },
        //key: "a",
        //modifiers: cmd_or_ctrl,
        tooltip: l("menu.pd_list_tt")
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.forums"),
        click: function() {
            pdgui.external_doc_open("http://forum.pdpatchrepo.info/");
        },
        //key: "a",
        //modifiers: cmd_or_ctrl,
        tooltip: l("menu.forums_tt")
    }));

    helpMenu.append(new gui.MenuItem({
        label: l("menu.devtools"),
        click: function() {
            gui.Window.get().showDevTools();
        },
        key: "b", // temporary convenience shortcut-- can change if needed
        modifiers: cmd_or_ctrl,
        tooltip: l("menu.devtools_tt")
    }));

    //helpMenu.append(new gui.MenuItem({
    //    label: l("menu.irc"),
    //    click: function() {
    //        pdgui.external_doc_open("irc://irc.freenode.net/dataflow");
    //    },
    //    //key: "a",
    //    //modifiers: cmd_or_ctrl,
    //    tooltip: l("menu.irc_tt")
    //}));

    // Assign to window
    gui.Window.get(w).menu = windowMenu;
}

exports.nw_create_pd_window_menus = nw_create_pd_window_menus;
