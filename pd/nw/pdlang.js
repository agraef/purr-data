"use strict";

var lang;

try {
    // try the locale given by navigator.language
    lang = require("./locales/" + navigator.language + "/translation.json");
} catch (e) {
    // if that fails then fall back to the default locale "en"
    lang = require("./locales/en/translation.json");
}

exports.lang = lang;

function recursive_key_splitter(key, object) {
    var subkeys = key.split(".");
    if (subkeys.length > 1) {
        return recursive_key_splitter(subkeys.slice(1).join("."), object[subkeys[0]]);
    } else {
        return object[subkeys[0]];
    }
}

exports.get_local_string = function (key) {
    return recursive_key_splitter(key, lang);
};
