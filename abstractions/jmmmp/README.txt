Abstractions from João Pais

Version 0.47

(c) 2005-2016 João Pais - jmmmpais@gmail.com
Released under the BSD license (more information in each abstraction).


This package has several utilities with different functions. It is composed of abstractions only.

ardourjack-gui - Controls ardour transport and jack settings from Pd
array-edit - Edit properties of arrays and populate them following several formulas
bezier - Transfer function GUI with one cubic bézier curve
bezier~ - Transfer function GUI with one cubic bézier curve at audio rate
but - Monochrome bang button
butt - Color-changing Toggle Button
clock - Chronometer with display in seconds
dacc~ - dynamic dac~ outlet up to 32 channels
dacm~ - Mono dac~ for lazy people
datei-o - Sends the message "open ../../"
datei-r - Sends the message "read ../../"
datei-w - Sends the message "write ../../"
ds-color-sel - color selector for data structures
dsp01 - DSP switch
f+ - Counter with variable increment
gui-edit - edit standard GUI objects fast
jp.menu - Dropdown menu programmed with data structures
jp.preset - Dropdown preset saver programmed with data structures
lbang - loadbang which can be triggered more often
liner~ - practical implementation of [line~]
liner+~ - practical implementation of signal envelopping
mat~ - Level meter with amplitude control
mat-~ - Level meter with amplitude control, horizontal
maat~ - Level meter with amplitude control, stereo
matrixctrl - GUI for [iemmatrix/mtx_mul~]
met~ - Level meter with amplitude control (with VU, too CPU expensive for me)
metrum - Metro with GUI
m-i - Automatic conversion of MIDI controller
mk - Visual display of MIDI inputs
oscD - Counts received OSC messages
oscS - Interface for sendOSC
pd-colors - Pd color palettes (Data Structures + Tcl/Tk)
pix2canvas - Convert images into canvas
rec-name - Automatic naming for a record/playback engine
rgb-color - Pick RGB colors for your GUI objects
sguigot - spigot GUI implementation
sliders - GUI for incoming midi data
snaps~ - snapshot~ GUI implementation
spectrogram~ - Spectrogram with 512 bins resolution
stoppuhr - Chronometer with two layers
swatch - Pick a color using the hue-saturation chart
swatch-gui - Pick a color for your GUI using the hue-saturation chart
tastin - Gate for keyboard input
uhr - Shows the time


The jmmmp library is dependent from the following libraries: cyclone, ext13, ggee, iemlib, iemmatrix, jmmmp, zexy 2.2.6.

2016.06.30


Non-working or discarded abstractions:

aufnahme~ - Multichannel audio recorder (1 to 8 channels)
bcf2000 - Store and recall presets for Behringer BCF2000
-dsp - replaced by dsp01
datei-l - replaced with datei-o