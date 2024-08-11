"use strict";

var cmd_or_ctrl = (process.platform === "darwin") ? "cmd" : "ctrl";
var cmd_or_ctrl_shift = cmd_or_ctrl + "+shift";
var cmd_or_ctrl_alt = cmd_or_ctrl + "+alt";

exports.menu = {
  "new":   { key: "n", modifiers: cmd_or_ctrl },
  "open":   { key: "o", modifiers: cmd_or_ctrl },
  "save":   { key: "s", modifiers: cmd_or_ctrl },
  "saveas": { key: "s", modifiers: cmd_or_ctrl_shift },
  "print":  { key: "p", modifiers: cmd_or_ctrl_shift },
  "message" : { key: "m", modifiers: cmd_or_ctrl },
  "close":  { key: "w", modifiers: cmd_or_ctrl },
  "quit":   { key: "q", modifiers: cmd_or_ctrl },

  "undo":   { key: "z", modifiers: cmd_or_ctrl },
  "redo":   { key: "z", modifiers: cmd_or_ctrl_shift },
  "selectall":{ key: "a", modifiers: cmd_or_ctrl },
  "cut":    { key: "x", modifiers: cmd_or_ctrl },
  "copy":   { key: "c", modifiers: cmd_or_ctrl },
  "paste":  { key: "v", modifiers: cmd_or_ctrl },
  "paste_clipboard": { key: "v", modifiers: cmd_or_ctrl_alt },
  "duplicate": { key: "d", modifiers: cmd_or_ctrl },
  "undo":   { key: "z", modifiers: cmd_or_ctrl },

  "reselect": { key: String.fromCharCode(10), modifiers: cmd_or_ctrl },
  "clear_console": { key: "l", modifiers: cmd_or_ctrl_shift },
  "encapsulate": { key: "e", modifiers: cmd_or_ctrl_shift },
  "tidyup": { key: "y", modifiers: cmd_or_ctrl },
  "cordinspector":   { key: "r", modifiers: cmd_or_ctrl_shift },
  "find":   { key: "f", modifiers: cmd_or_ctrl },
  "findagain":{ key: "g", modifiers: cmd_or_ctrl },
  "editmode": { key: "e", modifiers: cmd_or_ctrl },
  "preferences": { key: (process.platform === "darwin") ? "," : "p",
    modifiers: cmd_or_ctrl },

  "zoomin": { key: "=", modifiers: cmd_or_ctrl },
  "zoomout": { key: "-", modifiers: cmd_or_ctrl },
  "zoomreset": { key: "0", modifiers: cmd_or_ctrl },
  "zoomoptimal": { key: "9", modifiers: cmd_or_ctrl },
  "zoomhoriz": { key: "9", modifiers: cmd_or_ctrl_alt },
  "zoomvert": { key: "9", modifiers: cmd_or_ctrl_shift },
  "fullscreen": { key: (process.platform === "darwin") ? "f" : "F11",
    modifiers: (process.platform === "darwin") ? "cmd+ctrl" : null },

  "object": { key: "1", modifiers: cmd_or_ctrl },
  "msgbox": { key: "2", modifiers: cmd_or_ctrl },
  "number": { key: "3", modifiers: cmd_or_ctrl },
  "symbol": { key: "4", modifiers: cmd_or_ctrl },
  "comment": { key: "5", modifiers: cmd_or_ctrl },
  "dropdown": { key: "6", modifiers: cmd_or_ctrl_shift },
  "bang": { key: "b", modifiers: cmd_or_ctrl_shift },
  "toggle": { key: "t", modifiers: cmd_or_ctrl_shift },
  "number2": { key: "n", modifiers: cmd_or_ctrl_shift },
  "vslider": { key: "v", modifiers: cmd_or_ctrl_shift },
  "hslider": { key: "h", modifiers: cmd_or_ctrl_shift },
  "vradio": { key: "d", modifiers: cmd_or_ctrl_shift },
  "hradio": { key: "i", modifiers: cmd_or_ctrl_shift },
  "vu":     { key: "u", modifiers: cmd_or_ctrl_shift },
  "cnv": { key: "c", modifiers: cmd_or_ctrl_shift },
  "array": { key: "a", modifiers: cmd_or_ctrl_shift },

  "nextwin": { key: "PageDown", modifiers: cmd_or_ctrl },
  "prevwin": { key: "PageUp", modifiers: cmd_or_ctrl },
  "pdwin": { key: "r", modifiers: cmd_or_ctrl },

  "audio_on": { key: "/", modifiers: cmd_or_ctrl },
  "audio_off": { key: ".", modifiers: cmd_or_ctrl },

  "browser": { key: "b", modifiers: cmd_or_ctrl },
  "audio_off": { key: ".", modifiers: cmd_or_ctrl },
  "audio_off": { key: ".", modifiers: cmd_or_ctrl },
  "audio_off": { key: ".", modifiers: cmd_or_ctrl },
}
