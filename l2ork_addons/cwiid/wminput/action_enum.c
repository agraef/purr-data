#include <stdlib.h>
#include <linux/input.h>
#include "conf.h"

struct lookup_enum action_enum[] = {
#ifdef KEY_ESC
	{"KEY_ESC",	KEY_ESC},
#endif
#ifdef KEY_1
	{"KEY_1",	KEY_1},
#endif
#ifdef KEY_2
	{"KEY_2",	KEY_2},
#endif
#ifdef KEY_3
	{"KEY_3",	KEY_3},
#endif
#ifdef KEY_4
	{"KEY_4",	KEY_4},
#endif
#ifdef KEY_5
	{"KEY_5",	KEY_5},
#endif
#ifdef KEY_6
	{"KEY_6",	KEY_6},
#endif
#ifdef KEY_7
	{"KEY_7",	KEY_7},
#endif
#ifdef KEY_8
	{"KEY_8",	KEY_8},
#endif
#ifdef KEY_9
	{"KEY_9",	KEY_9},
#endif
#ifdef KEY_0
	{"KEY_0",	KEY_0},
#endif
#ifdef KEY_MINUS
	{"KEY_MINUS",	KEY_MINUS},
#endif
#ifdef KEY_EQUAL
	{"KEY_EQUAL",	KEY_EQUAL},
#endif
#ifdef KEY_BACKSPACE
	{"KEY_BACKSPACE",	KEY_BACKSPACE},
#endif
#ifdef KEY_TAB
	{"KEY_TAB",	KEY_TAB},
#endif
#ifdef KEY_Q
	{"KEY_Q",	KEY_Q},
#endif
#ifdef KEY_W
	{"KEY_W",	KEY_W},
#endif
#ifdef KEY_E
	{"KEY_E",	KEY_E},
#endif
#ifdef KEY_R
	{"KEY_R",	KEY_R},
#endif
#ifdef KEY_T
	{"KEY_T",	KEY_T},
#endif
#ifdef KEY_Y
	{"KEY_Y",	KEY_Y},
#endif
#ifdef KEY_U
	{"KEY_U",	KEY_U},
#endif
#ifdef KEY_I
	{"KEY_I",	KEY_I},
#endif
#ifdef KEY_O
	{"KEY_O",	KEY_O},
#endif
#ifdef KEY_P
	{"KEY_P",	KEY_P},
#endif
#ifdef KEY_LEFTBRACE
	{"KEY_LEFTBRACE",	KEY_LEFTBRACE},
#endif
#ifdef KEY_RIGHTBRACE
	{"KEY_RIGHTBRACE",	KEY_RIGHTBRACE},
#endif
#ifdef KEY_ENTER
	{"KEY_ENTER",	KEY_ENTER},
#endif
#ifdef KEY_LEFTCTRL
	{"KEY_LEFTCTRL",	KEY_LEFTCTRL},
#endif
#ifdef KEY_A
	{"KEY_A",	KEY_A},
#endif
#ifdef KEY_S
	{"KEY_S",	KEY_S},
#endif
#ifdef KEY_D
	{"KEY_D",	KEY_D},
#endif
#ifdef KEY_F
	{"KEY_F",	KEY_F},
#endif
#ifdef KEY_G
	{"KEY_G",	KEY_G},
#endif
#ifdef KEY_H
	{"KEY_H",	KEY_H},
#endif
#ifdef KEY_J
	{"KEY_J",	KEY_J},
#endif
#ifdef KEY_K
	{"KEY_K",	KEY_K},
#endif
#ifdef KEY_L
	{"KEY_L",	KEY_L},
#endif
#ifdef KEY_SEMICOLON
	{"KEY_SEMICOLON",	KEY_SEMICOLON},
#endif
#ifdef KEY_APOSTROPHE
	{"KEY_APOSTROPHE",	KEY_APOSTROPHE},
#endif
#ifdef KEY_GRAVE
	{"KEY_GRAVE",	KEY_GRAVE},
#endif
#ifdef KEY_LEFTSHIFT
	{"KEY_LEFTSHIFT",	KEY_LEFTSHIFT},
#endif
#ifdef KEY_BACKSLASH
	{"KEY_BACKSLASH",	KEY_BACKSLASH},
#endif
#ifdef KEY_Z
	{"KEY_Z",	KEY_Z},
#endif
#ifdef KEY_X
	{"KEY_X",	KEY_X},
#endif
#ifdef KEY_C
	{"KEY_C",	KEY_C},
#endif
#ifdef KEY_V
	{"KEY_V",	KEY_V},
#endif
#ifdef KEY_B
	{"KEY_B",	KEY_B},
#endif
#ifdef KEY_N
	{"KEY_N",	KEY_N},
#endif
#ifdef KEY_M
	{"KEY_M",	KEY_M},
#endif
#ifdef KEY_COMMA
	{"KEY_COMMA",	KEY_COMMA},
#endif
#ifdef KEY_DOT
	{"KEY_DOT",	KEY_DOT},
#endif
#ifdef KEY_SLASH
	{"KEY_SLASH",	KEY_SLASH},
#endif
#ifdef KEY_RIGHTSHIFT
	{"KEY_RIGHTSHIFT",	KEY_RIGHTSHIFT},
#endif
#ifdef KEY_KPASTERISK
	{"KEY_KPASTERISK",	KEY_KPASTERISK},
#endif
#ifdef KEY_LEFTALT
	{"KEY_LEFTALT",	KEY_LEFTALT},
#endif
#ifdef KEY_SPACE
	{"KEY_SPACE",	KEY_SPACE},
#endif
#ifdef KEY_CAPSLOCK
	{"KEY_CAPSLOCK",	KEY_CAPSLOCK},
#endif
#ifdef KEY_F1
	{"KEY_F1",	KEY_F1},
#endif
#ifdef KEY_F2
	{"KEY_F2",	KEY_F2},
#endif
#ifdef KEY_F3
	{"KEY_F3",	KEY_F3},
#endif
#ifdef KEY_F4
	{"KEY_F4",	KEY_F4},
#endif
#ifdef KEY_F5
	{"KEY_F5",	KEY_F5},
#endif
#ifdef KEY_F6
	{"KEY_F6",	KEY_F6},
#endif
#ifdef KEY_F7
	{"KEY_F7",	KEY_F7},
#endif
#ifdef KEY_F8
	{"KEY_F8",	KEY_F8},
#endif
#ifdef KEY_F9
	{"KEY_F9",	KEY_F9},
#endif
#ifdef KEY_F10
	{"KEY_F10",	KEY_F10},
#endif
#ifdef KEY_NUMLOCK
	{"KEY_NUMLOCK",	KEY_NUMLOCK},
#endif
#ifdef KEY_SCROLLLOCK
	{"KEY_SCROLLLOCK",	KEY_SCROLLLOCK},
#endif
#ifdef KEY_KP7
	{"KEY_KP7",	KEY_KP7},
#endif
#ifdef KEY_KP8
	{"KEY_KP8",	KEY_KP8},
#endif
#ifdef KEY_KP9
	{"KEY_KP9",	KEY_KP9},
#endif
#ifdef KEY_KPMINUS
	{"KEY_KPMINUS",	KEY_KPMINUS},
#endif
#ifdef KEY_KP4
	{"KEY_KP4",	KEY_KP4},
#endif
#ifdef KEY_KP5
	{"KEY_KP5",	KEY_KP5},
#endif
#ifdef KEY_KP6
	{"KEY_KP6",	KEY_KP6},
#endif
#ifdef KEY_KPPLUS
	{"KEY_KPPLUS",	KEY_KPPLUS},
#endif
#ifdef KEY_KP1
	{"KEY_KP1",	KEY_KP1},
#endif
#ifdef KEY_KP2
	{"KEY_KP2",	KEY_KP2},
#endif
#ifdef KEY_KP3
	{"KEY_KP3",	KEY_KP3},
#endif
#ifdef KEY_KP0
	{"KEY_KP0",	KEY_KP0},
#endif
#ifdef KEY_KPDOT
	{"KEY_KPDOT",	KEY_KPDOT},
#endif
#ifdef KEY_ZENKAKUHANKAKU
	{"KEY_ZENKAKUHANKAKU",	KEY_ZENKAKUHANKAKU},
#endif
#ifdef KEY_102ND
	{"KEY_102ND",	KEY_102ND},
#endif
#ifdef KEY_F11
	{"KEY_F11",	KEY_F11},
#endif
#ifdef KEY_F12
	{"KEY_F12",	KEY_F12},
#endif
#ifdef KEY_RO
	{"KEY_RO",	KEY_RO},
#endif
#ifdef KEY_KATAKANA
	{"KEY_KATAKANA",	KEY_KATAKANA},
#endif
#ifdef KEY_HIRAGANA
	{"KEY_HIRAGANA",	KEY_HIRAGANA},
#endif
#ifdef KEY_HENKAN
	{"KEY_HENKAN",	KEY_HENKAN},
#endif
#ifdef KEY_KATAKANAHIRAGANA
	{"KEY_KATAKANAHIRAGANA",	KEY_KATAKANAHIRAGANA},
#endif
#ifdef KEY_MUHENKAN
	{"KEY_MUHENKAN",	KEY_MUHENKAN},
#endif
#ifdef KEY_KPJPCOMMA
	{"KEY_KPJPCOMMA",	KEY_KPJPCOMMA},
#endif
#ifdef KEY_KPENTER
	{"KEY_KPENTER",	KEY_KPENTER},
#endif
#ifdef KEY_RIGHTCTRL
	{"KEY_RIGHTCTRL",	KEY_RIGHTCTRL},
#endif
#ifdef KEY_KPSLASH
	{"KEY_KPSLASH",	KEY_KPSLASH},
#endif
#ifdef KEY_SYSRQ
	{"KEY_SYSRQ",	KEY_SYSRQ},
#endif
#ifdef KEY_RIGHTALT
	{"KEY_RIGHTALT",	KEY_RIGHTALT},
#endif
#ifdef KEY_LINEFEED
	{"KEY_LINEFEED",	KEY_LINEFEED},
#endif
#ifdef KEY_HOME
	{"KEY_HOME",	KEY_HOME},
#endif
#ifdef KEY_UP
	{"KEY_UP",	KEY_UP},
#endif
#ifdef KEY_PAGEUP
	{"KEY_PAGEUP",	KEY_PAGEUP},
#endif
#ifdef KEY_LEFT
	{"KEY_LEFT",	KEY_LEFT},
#endif
#ifdef KEY_RIGHT
	{"KEY_RIGHT",	KEY_RIGHT},
#endif
#ifdef KEY_END
	{"KEY_END",	KEY_END},
#endif
#ifdef KEY_DOWN
	{"KEY_DOWN",	KEY_DOWN},
#endif
#ifdef KEY_PAGEDOWN
	{"KEY_PAGEDOWN",	KEY_PAGEDOWN},
#endif
#ifdef KEY_INSERT
	{"KEY_INSERT",	KEY_INSERT},
#endif
#ifdef KEY_DELETE
	{"KEY_DELETE",	KEY_DELETE},
#endif
#ifdef KEY_MACRO
	{"KEY_MACRO",	KEY_MACRO},
#endif
#ifdef KEY_MUTE
	{"KEY_MUTE",	KEY_MUTE},
#endif
#ifdef KEY_VOLUMEDOWN
	{"KEY_VOLUMEDOWN",	KEY_VOLUMEDOWN},
#endif
#ifdef KEY_VOLUMEUP
	{"KEY_VOLUMEUP",	KEY_VOLUMEUP},
#endif
#ifdef KEY_POWER
	{"KEY_POWER",	KEY_POWER},
#endif
#ifdef KEY_KPEQUAL
	{"KEY_KPEQUAL",	KEY_KPEQUAL},
#endif
#ifdef KEY_KPPLUSMINUS
	{"KEY_KPPLUSMINUS",	KEY_KPPLUSMINUS},
#endif
#ifdef KEY_PAUSE
	{"KEY_PAUSE",	KEY_PAUSE},
#endif
#ifdef KEY_KPCOMMA
	{"KEY_KPCOMMA",	KEY_KPCOMMA},
#endif
#ifdef KEY_HANGUEL
	{"KEY_HANGUEL",	KEY_HANGUEL},
#endif
#ifdef KEY_HANJA
	{"KEY_HANJA",	KEY_HANJA},
#endif
#ifdef KEY_YEN
	{"KEY_YEN",	KEY_YEN},
#endif
#ifdef KEY_LEFTMETA
	{"KEY_LEFTMETA",	KEY_LEFTMETA},
#endif
#ifdef KEY_RIGHTMETA
	{"KEY_RIGHTMETA",	KEY_RIGHTMETA},
#endif
#ifdef KEY_COMPOSE
	{"KEY_COMPOSE",	KEY_COMPOSE},
#endif
#ifdef KEY_STOP
	{"KEY_STOP",	KEY_STOP},
#endif
#ifdef KEY_AGAIN
	{"KEY_AGAIN",	KEY_AGAIN},
#endif
#ifdef KEY_PROPS
	{"KEY_PROPS",	KEY_PROPS},
#endif
#ifdef KEY_UNDO
	{"KEY_UNDO",	KEY_UNDO},
#endif
#ifdef KEY_FRONT
	{"KEY_FRONT",	KEY_FRONT},
#endif
#ifdef KEY_COPY
	{"KEY_COPY",	KEY_COPY},
#endif
#ifdef KEY_OPEN
	{"KEY_OPEN",	KEY_OPEN},
#endif
#ifdef KEY_PASTE
	{"KEY_PASTE",	KEY_PASTE},
#endif
#ifdef KEY_FIND
	{"KEY_FIND",	KEY_FIND},
#endif
#ifdef KEY_CUT
	{"KEY_CUT",	KEY_CUT},
#endif
#ifdef KEY_HELP
	{"KEY_HELP",	KEY_HELP},
#endif
#ifdef KEY_MENU
	{"KEY_MENU",	KEY_MENU},
#endif
#ifdef KEY_CALC
	{"KEY_CALC",	KEY_CALC},
#endif
#ifdef KEY_SETUP
	{"KEY_SETUP",	KEY_SETUP},
#endif
#ifdef KEY_SLEEP
	{"KEY_SLEEP",	KEY_SLEEP},
#endif
#ifdef KEY_WAKEUP
	{"KEY_WAKEUP",	KEY_WAKEUP},
#endif
#ifdef KEY_FILE
	{"KEY_FILE",	KEY_FILE},
#endif
#ifdef KEY_SENDFILE
	{"KEY_SENDFILE",	KEY_SENDFILE},
#endif
#ifdef KEY_DELETEFILE
	{"KEY_DELETEFILE",	KEY_DELETEFILE},
#endif
#ifdef KEY_XFER
	{"KEY_XFER",	KEY_XFER},
#endif
#ifdef KEY_PROG1
	{"KEY_PROG1",	KEY_PROG1},
#endif
#ifdef KEY_PROG2
	{"KEY_PROG2",	KEY_PROG2},
#endif
#ifdef KEY_WWW
	{"KEY_WWW",	KEY_WWW},
#endif
#ifdef KEY_MSDOS
	{"KEY_MSDOS",	KEY_MSDOS},
#endif
#ifdef KEY_COFFEE
	{"KEY_COFFEE",	KEY_COFFEE},
#endif
#ifdef KEY_DIRECTION
	{"KEY_DIRECTION",	KEY_DIRECTION},
#endif
#ifdef KEY_CYCLEWINDOWS
	{"KEY_CYCLEWINDOWS",	KEY_CYCLEWINDOWS},
#endif
#ifdef KEY_MAIL
	{"KEY_MAIL",	KEY_MAIL},
#endif
#ifdef KEY_BOOKMARKS
	{"KEY_BOOKMARKS",	KEY_BOOKMARKS},
#endif
#ifdef KEY_COMPUTER
	{"KEY_COMPUTER",	KEY_COMPUTER},
#endif
#ifdef KEY_BACK
	{"KEY_BACK",	KEY_BACK},
#endif
#ifdef KEY_FORWARD
	{"KEY_FORWARD",	KEY_FORWARD},
#endif
#ifdef KEY_CLOSECD
	{"KEY_CLOSECD",	KEY_CLOSECD},
#endif
#ifdef KEY_EJECTCD
	{"KEY_EJECTCD",	KEY_EJECTCD},
#endif
#ifdef KEY_EJECTCLOSECD
	{"KEY_EJECTCLOSECD",	KEY_EJECTCLOSECD},
#endif
#ifdef KEY_NEXTSONG
	{"KEY_NEXTSONG",	KEY_NEXTSONG},
#endif
#ifdef KEY_PLAYPAUSE
	{"KEY_PLAYPAUSE",	KEY_PLAYPAUSE},
#endif
#ifdef KEY_PREVIOUSSONG
	{"KEY_PREVIOUSSONG",	KEY_PREVIOUSSONG},
#endif
#ifdef KEY_STOPCD
	{"KEY_STOPCD",	KEY_STOPCD},
#endif
#ifdef KEY_RECORD
	{"KEY_RECORD",	KEY_RECORD},
#endif
#ifdef KEY_REWIND
	{"KEY_REWIND",	KEY_REWIND},
#endif
#ifdef KEY_PHONE
	{"KEY_PHONE",	KEY_PHONE},
#endif
#ifdef KEY_ISO
	{"KEY_ISO",	KEY_ISO},
#endif
#ifdef KEY_CONFIG
	{"KEY_CONFIG",	KEY_CONFIG},
#endif
#ifdef KEY_HOMEPAGE
	{"KEY_HOMEPAGE",	KEY_HOMEPAGE},
#endif
#ifdef KEY_REFRESH
	{"KEY_REFRESH",	KEY_REFRESH},
#endif
#ifdef KEY_EXIT
	{"KEY_EXIT",	KEY_EXIT},
#endif
#ifdef KEY_MOVE
	{"KEY_MOVE",	KEY_MOVE},
#endif
#ifdef KEY_EDIT
	{"KEY_EDIT",	KEY_EDIT},
#endif
#ifdef KEY_SCROLLUP
	{"KEY_SCROLLUP",	KEY_SCROLLUP},
#endif
#ifdef KEY_SCROLLDOWN
	{"KEY_SCROLLDOWN",	KEY_SCROLLDOWN},
#endif
#ifdef KEY_KPLEFTPAREN
	{"KEY_KPLEFTPAREN",	KEY_KPLEFTPAREN},
#endif
#ifdef KEY_KPRIGHTPAREN
	{"KEY_KPRIGHTPAREN",	KEY_KPRIGHTPAREN},
#endif
#ifdef KEY_NEW
	{"KEY_NEW",	KEY_NEW},
#endif
#ifdef KEY_REDO
	{"KEY_REDO",	KEY_REDO},
#endif
#ifdef KEY_F13
	{"KEY_F13",	KEY_F13},
#endif
#ifdef KEY_F14
	{"KEY_F14",	KEY_F14},
#endif
#ifdef KEY_F15
	{"KEY_F15",	KEY_F15},
#endif
#ifdef KEY_F16
	{"KEY_F16",	KEY_F16},
#endif
#ifdef KEY_F17
	{"KEY_F17",	KEY_F17},
#endif
#ifdef KEY_F18
	{"KEY_F18",	KEY_F18},
#endif
#ifdef KEY_F19
	{"KEY_F19",	KEY_F19},
#endif
#ifdef KEY_F20
	{"KEY_F20",	KEY_F20},
#endif
#ifdef KEY_F21
	{"KEY_F21",	KEY_F21},
#endif
#ifdef KEY_F22
	{"KEY_F22",	KEY_F22},
#endif
#ifdef KEY_F23
	{"KEY_F23",	KEY_F23},
#endif
#ifdef KEY_F24
	{"KEY_F24",	KEY_F24},
#endif
#ifdef KEY_PLAYCD
	{"KEY_PLAYCD",	KEY_PLAYCD},
#endif
#ifdef KEY_PAUSECD
	{"KEY_PAUSECD",	KEY_PAUSECD},
#endif
#ifdef KEY_PROG3
	{"KEY_PROG3",	KEY_PROG3},
#endif
#ifdef KEY_PROG4
	{"KEY_PROG4",	KEY_PROG4},
#endif
#ifdef KEY_SUSPEND
	{"KEY_SUSPEND",	KEY_SUSPEND},
#endif
#ifdef KEY_CLOSE
	{"KEY_CLOSE",	KEY_CLOSE},
#endif
#ifdef KEY_PLAY
	{"KEY_PLAY",	KEY_PLAY},
#endif
#ifdef KEY_FASTFORWARD
	{"KEY_FASTFORWARD",	KEY_FASTFORWARD},
#endif
#ifdef KEY_BASSBOOST
	{"KEY_BASSBOOST",	KEY_BASSBOOST},
#endif
#ifdef KEY_PRINT
	{"KEY_PRINT",	KEY_PRINT},
#endif
#ifdef KEY_HP
	{"KEY_HP",	KEY_HP},
#endif
#ifdef KEY_CAMERA
	{"KEY_CAMERA",	KEY_CAMERA},
#endif
#ifdef KEY_SOUND
	{"KEY_SOUND",	KEY_SOUND},
#endif
#ifdef KEY_QUESTION
	{"KEY_QUESTION",	KEY_QUESTION},
#endif
#ifdef KEY_EMAIL
	{"KEY_EMAIL",	KEY_EMAIL},
#endif
#ifdef KEY_CHAT
	{"KEY_CHAT",	KEY_CHAT},
#endif
#ifdef KEY_SEARCH
	{"KEY_SEARCH",	KEY_SEARCH},
#endif
#ifdef KEY_CONNECT
	{"KEY_CONNECT",	KEY_CONNECT},
#endif
#ifdef KEY_FINANCE
	{"KEY_FINANCE",	KEY_FINANCE},
#endif
#ifdef KEY_SPORT
	{"KEY_SPORT",	KEY_SPORT},
#endif
#ifdef KEY_SHOP
	{"KEY_SHOP",	KEY_SHOP},
#endif
#ifdef KEY_ALTERASE
	{"KEY_ALTERASE",	KEY_ALTERASE},
#endif
#ifdef KEY_CANCEL
	{"KEY_CANCEL",	KEY_CANCEL},
#endif
#ifdef KEY_BRIGHTNESSDOWN
	{"KEY_BRIGHTNESSDOWN",	KEY_BRIGHTNESSDOWN},
#endif
#ifdef KEY_BRIGHTNESSUP
	{"KEY_BRIGHTNESSUP",	KEY_BRIGHTNESSUP},
#endif
#ifdef KEY_MEDIA
	{"KEY_MEDIA",	KEY_MEDIA},
#endif
#ifdef KEY_SWITCHVIDEOMODE
	{"KEY_SWITCHVIDEOMODE",	KEY_SWITCHVIDEOMODE},
#endif
#ifdef KEY_KBDILLUMTOGGLE
	{"KEY_KBDILLUMTOGGLE",	KEY_KBDILLUMTOGGLE},
#endif
#ifdef KEY_KBDILLUMDOWN
	{"KEY_KBDILLUMDOWN",	KEY_KBDILLUMDOWN},
#endif
#ifdef KEY_KBDILLUMUP
	{"KEY_KBDILLUMUP",	KEY_KBDILLUMUP},
#endif
#ifdef KEY_SEND
	{"KEY_SEND",	KEY_SEND},
#endif
#ifdef KEY_REPLY
	{"KEY_REPLY",	KEY_REPLY},
#endif
#ifdef KEY_FORWARDMAIL
	{"KEY_FORWARDMAIL",	KEY_FORWARDMAIL},
#endif
#ifdef KEY_SAVE
	{"KEY_SAVE",	KEY_SAVE},
#endif
#ifdef KEY_DOCUMENTS
	{"KEY_DOCUMENTS",	KEY_DOCUMENTS},
#endif
#ifdef KEY_BATTERY
	{"KEY_BATTERY",	KEY_BATTERY},
#endif
#ifdef KEY_UNKNOWN
	{"KEY_UNKNOWN",	KEY_UNKNOWN},
#endif
#ifdef BTN_MISC
	{"BTN_MISC",	BTN_MISC},
#endif
#ifdef BTN_0
	{"BTN_0",	BTN_0},
#endif
#ifdef BTN_1
	{"BTN_1",	BTN_1},
#endif
#ifdef BTN_2
	{"BTN_2",	BTN_2},
#endif
#ifdef BTN_3
	{"BTN_3",	BTN_3},
#endif
#ifdef BTN_4
	{"BTN_4",	BTN_4},
#endif
#ifdef BTN_5
	{"BTN_5",	BTN_5},
#endif
#ifdef BTN_6
	{"BTN_6",	BTN_6},
#endif
#ifdef BTN_7
	{"BTN_7",	BTN_7},
#endif
#ifdef BTN_8
	{"BTN_8",	BTN_8},
#endif
#ifdef BTN_9
	{"BTN_9",	BTN_9},
#endif
#ifdef BTN_MOUSE
	{"BTN_MOUSE",	BTN_MOUSE},
#endif
#ifdef BTN_LEFT
	{"BTN_LEFT",	BTN_LEFT},
#endif
#ifdef BTN_RIGHT
	{"BTN_RIGHT",	BTN_RIGHT},
#endif
#ifdef BTN_MIDDLE
	{"BTN_MIDDLE",	BTN_MIDDLE},
#endif
#ifdef BTN_SIDE
	{"BTN_SIDE",	BTN_SIDE},
#endif
#ifdef BTN_EXTRA
	{"BTN_EXTRA",	BTN_EXTRA},
#endif
#ifdef BTN_FORWARD
	{"BTN_FORWARD",	BTN_FORWARD},
#endif
#ifdef BTN_BACK
	{"BTN_BACK",	BTN_BACK},
#endif
#ifdef BTN_TASK
	{"BTN_TASK",	BTN_TASK},
#endif
#ifdef BTN_JOYSTICK
	{"BTN_JOYSTICK",	BTN_JOYSTICK},
#endif
#ifdef BTN_TRIGGER
	{"BTN_TRIGGER",	BTN_TRIGGER},
#endif
#ifdef BTN_THUMB
	{"BTN_THUMB",	BTN_THUMB},
#endif
#ifdef BTN_THUMB2
	{"BTN_THUMB2",	BTN_THUMB2},
#endif
#ifdef BTN_TOP
	{"BTN_TOP",	BTN_TOP},
#endif
#ifdef BTN_TOP2
	{"BTN_TOP2",	BTN_TOP2},
#endif
#ifdef BTN_PINKIE
	{"BTN_PINKIE",	BTN_PINKIE},
#endif
#ifdef BTN_BASE
	{"BTN_BASE",	BTN_BASE},
#endif
#ifdef BTN_BASE2
	{"BTN_BASE2",	BTN_BASE2},
#endif
#ifdef BTN_BASE3
	{"BTN_BASE3",	BTN_BASE3},
#endif
#ifdef BTN_BASE4
	{"BTN_BASE4",	BTN_BASE4},
#endif
#ifdef BTN_BASE5
	{"BTN_BASE5",	BTN_BASE5},
#endif
#ifdef BTN_BASE6
	{"BTN_BASE6",	BTN_BASE6},
#endif
#ifdef BTN_DEAD
	{"BTN_DEAD",	BTN_DEAD},
#endif
#ifdef BTN_GAMEPAD
	{"BTN_GAMEPAD",	BTN_GAMEPAD},
#endif
#ifdef BTN_A
	{"BTN_A",	BTN_A},
#endif
#ifdef BTN_B
	{"BTN_B",	BTN_B},
#endif
#ifdef BTN_C
	{"BTN_C",	BTN_C},
#endif
#ifdef BTN_X
	{"BTN_X",	BTN_X},
#endif
#ifdef BTN_Y
	{"BTN_Y",	BTN_Y},
#endif
#ifdef BTN_Z
	{"BTN_Z",	BTN_Z},
#endif
#ifdef BTN_TL
	{"BTN_TL",	BTN_TL},
#endif
#ifdef BTN_TR
	{"BTN_TR",	BTN_TR},
#endif
#ifdef BTN_TL2
	{"BTN_TL2",	BTN_TL2},
#endif
#ifdef BTN_TR2
	{"BTN_TR2",	BTN_TR2},
#endif
#ifdef BTN_SELECT
	{"BTN_SELECT",	BTN_SELECT},
#endif
#ifdef BTN_START
	{"BTN_START",	BTN_START},
#endif
#ifdef BTN_MODE
	{"BTN_MODE",	BTN_MODE},
#endif
#ifdef BTN_THUMBL
	{"BTN_THUMBL",	BTN_THUMBL},
#endif
#ifdef BTN_THUMBR
	{"BTN_THUMBR",	BTN_THUMBR},
#endif
#ifdef BTN_DIGI
	{"BTN_DIGI",	BTN_DIGI},
#endif
#ifdef BTN_TOOL_PEN
	{"BTN_TOOL_PEN",	BTN_TOOL_PEN},
#endif
#ifdef BTN_TOOL_RUBBER
	{"BTN_TOOL_RUBBER",	BTN_TOOL_RUBBER},
#endif
#ifdef BTN_TOOL_BRUSH
	{"BTN_TOOL_BRUSH",	BTN_TOOL_BRUSH},
#endif
#ifdef BTN_TOOL_PENCIL
	{"BTN_TOOL_PENCIL",	BTN_TOOL_PENCIL},
#endif
#ifdef BTN_TOOL_AIRBRUSH
	{"BTN_TOOL_AIRBRUSH",	BTN_TOOL_AIRBRUSH},
#endif
#ifdef BTN_TOOL_FINGER
	{"BTN_TOOL_FINGER",	BTN_TOOL_FINGER},
#endif
#ifdef BTN_TOOL_MOUSE
	{"BTN_TOOL_MOUSE",	BTN_TOOL_MOUSE},
#endif
#ifdef BTN_TOOL_LENS
	{"BTN_TOOL_LENS",	BTN_TOOL_LENS},
#endif
#ifdef BTN_TOUCH
	{"BTN_TOUCH",	BTN_TOUCH},
#endif
#ifdef BTN_STYLUS
	{"BTN_STYLUS",	BTN_STYLUS},
#endif
#ifdef BTN_STYLUS2
	{"BTN_STYLUS2",	BTN_STYLUS2},
#endif
#ifdef BTN_TOOL_DOUBLETAP
	{"BTN_TOOL_DOUBLETAP",	BTN_TOOL_DOUBLETAP},
#endif
#ifdef BTN_TOOL_TRIPLETAP
	{"BTN_TOOL_TRIPLETAP",	BTN_TOOL_TRIPLETAP},
#endif
#ifdef BTN_WHEEL
	{"BTN_WHEEL",	BTN_WHEEL},
#endif
#ifdef BTN_GEAR_DOWN
	{"BTN_GEAR_DOWN",	BTN_GEAR_DOWN},
#endif
#ifdef BTN_GEAR_UP
	{"BTN_GEAR_UP",	BTN_GEAR_UP},
#endif
#ifdef KEY_OK
	{"KEY_OK",	KEY_OK},
#endif
#ifdef KEY_SELECT
	{"KEY_SELECT",	KEY_SELECT},
#endif
#ifdef KEY_GOTO
	{"KEY_GOTO",	KEY_GOTO},
#endif
#ifdef KEY_CLEAR
	{"KEY_CLEAR",	KEY_CLEAR},
#endif
#ifdef KEY_POWER2
	{"KEY_POWER2",	KEY_POWER2},
#endif
#ifdef KEY_OPTION
	{"KEY_OPTION",	KEY_OPTION},
#endif
#ifdef KEY_INFO
	{"KEY_INFO",	KEY_INFO},
#endif
#ifdef KEY_TIME
	{"KEY_TIME",	KEY_TIME},
#endif
#ifdef KEY_VENDOR
	{"KEY_VENDOR",	KEY_VENDOR},
#endif
#ifdef KEY_ARCHIVE
	{"KEY_ARCHIVE",	KEY_ARCHIVE},
#endif
#ifdef KEY_PROGRAM
	{"KEY_PROGRAM",	KEY_PROGRAM},
#endif
#ifdef KEY_CHANNEL
	{"KEY_CHANNEL",	KEY_CHANNEL},
#endif
#ifdef KEY_FAVORITES
	{"KEY_FAVORITES",	KEY_FAVORITES},
#endif
#ifdef KEY_EPG
	{"KEY_EPG",	KEY_EPG},
#endif
#ifdef KEY_PVR
	{"KEY_PVR",	KEY_PVR},
#endif
#ifdef KEY_MHP
	{"KEY_MHP",	KEY_MHP},
#endif
#ifdef KEY_LANGUAGE
	{"KEY_LANGUAGE",	KEY_LANGUAGE},
#endif
#ifdef KEY_TITLE
	{"KEY_TITLE",	KEY_TITLE},
#endif
#ifdef KEY_SUBTITLE
	{"KEY_SUBTITLE",	KEY_SUBTITLE},
#endif
#ifdef KEY_ANGLE
	{"KEY_ANGLE",	KEY_ANGLE},
#endif
#ifdef KEY_ZOOM
	{"KEY_ZOOM",	KEY_ZOOM},
#endif
#ifdef KEY_MODE
	{"KEY_MODE",	KEY_MODE},
#endif
#ifdef KEY_KEYBOARD
	{"KEY_KEYBOARD",	KEY_KEYBOARD},
#endif
#ifdef KEY_SCREEN
	{"KEY_SCREEN",	KEY_SCREEN},
#endif
#ifdef KEY_PC
	{"KEY_PC",	KEY_PC},
#endif
#ifdef KEY_TV
	{"KEY_TV",	KEY_TV},
#endif
#ifdef KEY_TV2
	{"KEY_TV2",	KEY_TV2},
#endif
#ifdef KEY_VCR
	{"KEY_VCR",	KEY_VCR},
#endif
#ifdef KEY_VCR2
	{"KEY_VCR2",	KEY_VCR2},
#endif
#ifdef KEY_SAT
	{"KEY_SAT",	KEY_SAT},
#endif
#ifdef KEY_SAT2
	{"KEY_SAT2",	KEY_SAT2},
#endif
#ifdef KEY_CD
	{"KEY_CD",	KEY_CD},
#endif
#ifdef KEY_TAPE
	{"KEY_TAPE",	KEY_TAPE},
#endif
#ifdef KEY_RADIO
	{"KEY_RADIO",	KEY_RADIO},
#endif
#ifdef KEY_TUNER
	{"KEY_TUNER",	KEY_TUNER},
#endif
#ifdef KEY_PLAYER
	{"KEY_PLAYER",	KEY_PLAYER},
#endif
#ifdef KEY_TEXT
	{"KEY_TEXT",	KEY_TEXT},
#endif
#ifdef KEY_DVD
	{"KEY_DVD",	KEY_DVD},
#endif
#ifdef KEY_AUX
	{"KEY_AUX",	KEY_AUX},
#endif
#ifdef KEY_MP3
	{"KEY_MP3",	KEY_MP3},
#endif
#ifdef KEY_AUDIO
	{"KEY_AUDIO",	KEY_AUDIO},
#endif
#ifdef KEY_VIDEO
	{"KEY_VIDEO",	KEY_VIDEO},
#endif
#ifdef KEY_DIRECTORY
	{"KEY_DIRECTORY",	KEY_DIRECTORY},
#endif
#ifdef KEY_LIST
	{"KEY_LIST",	KEY_LIST},
#endif
#ifdef KEY_MEMO
	{"KEY_MEMO",	KEY_MEMO},
#endif
#ifdef KEY_CALENDAR
	{"KEY_CALENDAR",	KEY_CALENDAR},
#endif
#ifdef KEY_RED
	{"KEY_RED",	KEY_RED},
#endif
#ifdef KEY_GREEN
	{"KEY_GREEN",	KEY_GREEN},
#endif
#ifdef KEY_YELLOW
	{"KEY_YELLOW",	KEY_YELLOW},
#endif
#ifdef KEY_BLUE
	{"KEY_BLUE",	KEY_BLUE},
#endif
#ifdef KEY_CHANNELUP
	{"KEY_CHANNELUP",	KEY_CHANNELUP},
#endif
#ifdef KEY_CHANNELDOWN
	{"KEY_CHANNELDOWN",	KEY_CHANNELDOWN},
#endif
#ifdef KEY_FIRST
	{"KEY_FIRST",	KEY_FIRST},
#endif
#ifdef KEY_LAST
	{"KEY_LAST",	KEY_LAST},
#endif
#ifdef KEY_AB
	{"KEY_AB",	KEY_AB},
#endif
#ifdef KEY_NEXT
	{"KEY_NEXT",	KEY_NEXT},
#endif
#ifdef KEY_RESTART
	{"KEY_RESTART",	KEY_RESTART},
#endif
#ifdef KEY_SLOW
	{"KEY_SLOW",	KEY_SLOW},
#endif
#ifdef KEY_SHUFFLE
	{"KEY_SHUFFLE",	KEY_SHUFFLE},
#endif
#ifdef KEY_BREAK
	{"KEY_BREAK",	KEY_BREAK},
#endif
#ifdef KEY_PREVIOUS
	{"KEY_PREVIOUS",	KEY_PREVIOUS},
#endif
#ifdef KEY_DIGITS
	{"KEY_DIGITS",	KEY_DIGITS},
#endif
#ifdef KEY_TEEN
	{"KEY_TEEN",	KEY_TEEN},
#endif
#ifdef KEY_TWEN
	{"KEY_TWEN",	KEY_TWEN},
#endif
#ifdef KEY_DEL_EOL
	{"KEY_DEL_EOL",	KEY_DEL_EOL},
#endif
#ifdef KEY_DEL_EOS
	{"KEY_DEL_EOS",	KEY_DEL_EOS},
#endif
#ifdef KEY_INS_LINE
	{"KEY_INS_LINE",	KEY_INS_LINE},
#endif
#ifdef KEY_DEL_LINE
	{"KEY_DEL_LINE",	KEY_DEL_LINE},
#endif
#ifdef KEY_FN
	{"KEY_FN",	KEY_FN},
#endif
#ifdef KEY_FN_ESC
	{"KEY_FN_ESC",	KEY_FN_ESC},
#endif
#ifdef KEY_FN_F1
	{"KEY_FN_F1",	KEY_FN_F1},
#endif
#ifdef KEY_FN_F2
	{"KEY_FN_F2",	KEY_FN_F2},
#endif
#ifdef KEY_FN_F3
	{"KEY_FN_F3",	KEY_FN_F3},
#endif
#ifdef KEY_FN_F4
	{"KEY_FN_F4",	KEY_FN_F4},
#endif
#ifdef KEY_FN_F5
	{"KEY_FN_F5",	KEY_FN_F5},
#endif
#ifdef KEY_FN_F6
	{"KEY_FN_F6",	KEY_FN_F6},
#endif
#ifdef KEY_FN_F7
	{"KEY_FN_F7",	KEY_FN_F7},
#endif
#ifdef KEY_FN_F8
	{"KEY_FN_F8",	KEY_FN_F8},
#endif
#ifdef KEY_FN_F9
	{"KEY_FN_F9",	KEY_FN_F9},
#endif
#ifdef KEY_FN_F10
	{"KEY_FN_F10",	KEY_FN_F10},
#endif
#ifdef KEY_FN_F11
	{"KEY_FN_F11",	KEY_FN_F11},
#endif
#ifdef KEY_FN_F12
	{"KEY_FN_F12",	KEY_FN_F12},
#endif
#ifdef KEY_FN_1
	{"KEY_FN_1",	KEY_FN_1},
#endif
#ifdef KEY_FN_2
	{"KEY_FN_2",	KEY_FN_2},
#endif
#ifdef KEY_FN_D
	{"KEY_FN_D",	KEY_FN_D},
#endif
#ifdef KEY_FN_E
	{"KEY_FN_E",	KEY_FN_E},
#endif
#ifdef KEY_FN_F
	{"KEY_FN_F",	KEY_FN_F},
#endif
#ifdef KEY_FN_S
	{"KEY_FN_S",	KEY_FN_S},
#endif
#ifdef KEY_FN_B
	{"KEY_FN_B",	KEY_FN_B},
#endif
#ifdef KEY_BRL_DOT1
	{"KEY_BRL_DOT1",	KEY_BRL_DOT1},
#endif
#ifdef KEY_BRL_DOT2
	{"KEY_BRL_DOT2",	KEY_BRL_DOT2},
#endif
#ifdef KEY_BRL_DOT3
	{"KEY_BRL_DOT3",	KEY_BRL_DOT3},
#endif
#ifdef KEY_BRL_DOT4
	{"KEY_BRL_DOT4",	KEY_BRL_DOT4},
#endif
#ifdef KEY_BRL_DOT5
	{"KEY_BRL_DOT5",	KEY_BRL_DOT5},
#endif
#ifdef KEY_BRL_DOT6
	{"KEY_BRL_DOT6",	KEY_BRL_DOT6},
#endif
#ifdef KEY_BRL_DOT7
	{"KEY_BRL_DOT7",	KEY_BRL_DOT7},
#endif
#ifdef KEY_BRL_DOT8
	{"KEY_BRL_DOT8",	KEY_BRL_DOT8},
#endif
#ifdef REL_X
	{"REL_X",	REL_X},
#endif
#ifdef REL_Y
	{"REL_Y",	REL_Y},
#endif
#ifdef REL_Z
	{"REL_Z",	REL_Z},
#endif
#ifdef REL_RX
	{"REL_RX",	REL_RX},
#endif
#ifdef REL_RY
	{"REL_RY",	REL_RY},
#endif
#ifdef REL_RZ
	{"REL_RZ",	REL_RZ},
#endif
#ifdef REL_HWHEEL
	{"REL_HWHEEL",	REL_HWHEEL},
#endif
#ifdef REL_DIAL
	{"REL_DIAL",	REL_DIAL},
#endif
#ifdef REL_WHEEL
	{"REL_WHEEL",	REL_WHEEL},
#endif
#ifdef REL_MISC
	{"REL_MISC",	REL_MISC},
#endif
#ifdef ABS_X
	{"ABS_X",	ABS_X},
#endif
#ifdef ABS_Y
	{"ABS_Y",	ABS_Y},
#endif
#ifdef ABS_Z
	{"ABS_Z",	ABS_Z},
#endif
#ifdef ABS_RX
	{"ABS_RX",	ABS_RX},
#endif
#ifdef ABS_RY
	{"ABS_RY",	ABS_RY},
#endif
#ifdef ABS_RZ
	{"ABS_RZ",	ABS_RZ},
#endif
#ifdef ABS_THROTTLE
	{"ABS_THROTTLE",	ABS_THROTTLE},
#endif
#ifdef ABS_RUDDER
	{"ABS_RUDDER",	ABS_RUDDER},
#endif
#ifdef ABS_WHEEL
	{"ABS_WHEEL",	ABS_WHEEL},
#endif
#ifdef ABS_GAS
	{"ABS_GAS",	ABS_GAS},
#endif
#ifdef ABS_BRAKE
	{"ABS_BRAKE",	ABS_BRAKE},
#endif
#ifdef ABS_HAT0X
	{"ABS_HAT0X",	ABS_HAT0X},
#endif
#ifdef ABS_HAT0Y
	{"ABS_HAT0Y",	ABS_HAT0Y},
#endif
#ifdef ABS_HAT1X
	{"ABS_HAT1X",	ABS_HAT1X},
#endif
#ifdef ABS_HAT1Y
	{"ABS_HAT1Y",	ABS_HAT1Y},
#endif
#ifdef ABS_HAT2X
	{"ABS_HAT2X",	ABS_HAT2X},
#endif
#ifdef ABS_HAT2Y
	{"ABS_HAT2Y",	ABS_HAT2Y},
#endif
#ifdef ABS_HAT3X
	{"ABS_HAT3X",	ABS_HAT3X},
#endif
#ifdef ABS_HAT3Y
	{"ABS_HAT3Y",	ABS_HAT3Y},
#endif
#ifdef ABS_PRESSURE
	{"ABS_PRESSURE",	ABS_PRESSURE},
#endif
#ifdef ABS_DISTANCE
	{"ABS_DISTANCE",	ABS_DISTANCE},
#endif
#ifdef ABS_TILT_X
	{"ABS_TILT_X",	ABS_TILT_X},
#endif
#ifdef ABS_TILT_Y
	{"ABS_TILT_Y",	ABS_TILT_Y},
#endif
#ifdef ABS_TOOL_WIDTH
	{"ABS_TOOL_WIDTH",	ABS_TOOL_WIDTH},
#endif
#ifdef ABS_VOLUME
	{"ABS_VOLUME",	ABS_VOLUME},
#endif
#ifdef ABS_MISC
	{"ABS_MISC",	ABS_MISC},
#endif
	{NULL, 0}
};
