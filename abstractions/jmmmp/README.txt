Abstractions from João Pais

(c) 2005-9 João Pais - jmmmpais@googlemail.com
Released under the BSD license (more information in each abstraction).


This package has several base utilities that make coding a bit easier. It is composed of the following abstractions:

array-edit - edit properties of arrays and populate them following several formulas
but - Monochrome bang button
clock - Chronometer with display in seconds
dacm~ - Mono dac~ for lazy people
datei-o - Sends the message "open ../../"
datei-r - Sends the message "read ../../"
datei-w - Sends the message "write ../../"
dsp01 - DSP switch
f+ - Counter with variable increment
gui-edit - edit standard GUI objects fast
lbang - loadbang which can be triggered more often
liner~ - practical implementation of [line~]
liner+~ - practical implementation of signal envelopping
mat~ - Level meter with amplitude control
maat~ - Level meter with amplitude control, stereo
met~ - Level meter with amplitude control (with VU, too CPU expensive for me)
metrum - Metro with GUI
m-i - Automatic conversion of MIDI controller
mk - shows the controller number and MIDI value
oscD - Counts received OSC messages
oscS - Interface for sendOSC
pd-colors - Pd color palettes (Data Structures + Tcl/Tk)
rec-name - Automatic naming for a record/playback engine
sguigot - spigot GUI implementation
sliders - GUI for incoming midi data
snaps~ - snapshot~ GUI implementation
spectrogram~ - Spectrogram with 512 bins resolution
stoppuhr - Chronometer with two layers
tastin - Gate for keyboard input
uhr - Shows the time


It is recomended to use these abstractions with Pd-extended, since I don't keep track of which externals are used. Some abstractions use other ones of this package, so it is also better to have always the whole package in one place.

2009.02.20


Non-working or discarded abstractions:

aufnahme~ - Multichannel audio recorder (1 to 8 channels)
bcf2000 - Store and recall presets for Behringer BCF2000
-dsp - replaced by dsp01
datei-l - replaced with datei-o