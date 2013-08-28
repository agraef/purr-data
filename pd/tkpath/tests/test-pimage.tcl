#!/bin/sh
# start with WISH \
    exec wish "$0" ${1+"$@"}


#
# Testcase for pimage »move«.
# ----------------------------------------------------------------
# Author : Arndt Roger Schneider
# Date   : 09/22/2007
# License: Tcl-License (aka BSD)
#
# Copyright © 2007 Arndt Roger Schneider
# ----------------------------------------------------------------

package require Tk
package require tkpath ; # version 0.2.4

if {[lsearch [namespace children] ::tcltest] == -1} {
    package require tcltest
    namespace import -force ::tcltest::*
}


# Extend auto_path, that local packages can be used.
lappend auto_path [file dirname [info script]]

# Load Local Packages ...


# prepare the ui.
toplevel .t
pack [tkp::canvas .t.c]
image create photo _oval -data {
R0lGODlhIQAZAPcAMf//////////////9///9//39//39//39/f///f///f/
9/f/9/f3//f3//f39/f39/f37/f37/fv9/fv9/fv7/fv7/fv7/fv3vfv3vfv
3u/39+/39+/v9+/v9+/v7+/v7+/v5+/v5+/v3u/v3u/v3u/v1u/v1u/vzu/v
zu/n7+/n7+/n7+/n5+/n5+/nve/nve/eve/eve/Wre/Wrefv7+fv7+fv5+fv
5+fn7+fn7+fn5+fn5+fn5+fWxufWxufWxufWvefWvefWvefOpefOpefGlOfG
lOfGlOe9jOe9jOe9jN7v997v997v797v79bW1tbW1tala9ala9ala87n787n
787n587n587Ozs7Ozs7Ozs7Oxs7Oxs7Gxs7Gxs6tjM6tjM6lY86lY8bn98bn
98bOzsbOzsbOxsbOxsbGzsbGzsbGzsbGxsbGxsbGvcbGvcbGvcale8ale73n
973n973n773n773Gxr3Gxr29vb29vb29vb2le72le72le72MUr2MUr2MUrW1
tbW1tbW1rbW1rbWcjLWcjLWMY7WMY7WMY63e763e763e7621ta21ta1zKa1z
Ka1zKaW9zqW9zqW9zqWttaWttaWlpaWlpaWlpZzO55zO55ycnJycnJyUe5yU
e5yEa5yEa5yEa5TO55TO55S1xpS1xpSUlJSUlJRKAJRKAJQ5AJQ5AIS954S9
54S93oS93oRjSoRjSoRjSoQpAIQpAHO11nO11nOt1nOt1nOt1nMAAHMAAGuE
nGuEnGuEnGOUxmOUxmOUxmMAAGMAAGMAAFpzjFpzjFpzjFo5AFo5AFo5AFKM
vVKMvVKMvUp7pUp7pUpje0pje0oAAEoAADlrlDlrlDlrlDkpADkpADkAKTkA
KTkAKTkAADkAACl7vSl7vSlrrSlrrSlrrSljjCljjCljjCkAACkAACkAAABj
pQBjpQBKlABKlABKlAA5cwA5cwA5WgA5WgAAawAAawAAawAAUgAAUgAAKQAA
KQAAKQAAAAAAAAAAAAAAACwAAAAAIQAZAAAI/wABAPCApaDBgwgTJvQgEACm
TnmwuLkzsSLFixYzUsyTp1MnAFg+AuDQUELDkygBmEzpoVPBPAAolJxpsqbK
mzhJCoyIBaaGmAJNMgTwc+BNCDiHMpQgkaIGmQyjCmRIwYOHJlaJImUIQSpP
mBygDuzK0I2sf2j/ybOkQUNUoQPv9MSS1aoHCQw1/PmXz5cjS1+U/YNXZ6AE
CmQBfPXQlUNXCBw8QPoHzTHJJhC+/LtXx7EHyx6+Rmbswa0HJP+aCrRrV4a3
dXbJhn7J2q6IWu0gYK1tNcq/TbwX83bhChZv1heI5QpOmzcMdIuO22YFjjVW
jhytPvEgAquLbXWwdtTnLr6JCOra7Qonz90KNeNPIo/vLqIHsVt2t892k8fD
dvEeUKAKPLvNt1sYv/mX31dPUKDfdiIUQRkH5ZH3hTfgGGgVR3f0lx97iPxj
Cl0QNhHHP/RgUR59DCp4lYt8oHWMJZh8Qo2IVJA33nYc8acghA960MMn2aQl
zSRYPWFeE+b5J9x2QUKZnoI7cidlHnJh8mN6UroIpXlRXsVTJ1KGWSZ3VCq5
5TFuSOARJhxhASd2HBFCpyV5uIFnndh18kVDIrwkKE9yzeUGFoVGdOhcTZgU
EAA7LA==} -gamma 1.0


# The TestCase itself:
test pimage-1.1 {move 10 10} {
    set id [.t.c create pimage 100 100 -image _oval]
    .t.c move $id 10 10
    .t.c coords $id
} {{110.0 110.0}}

test pimage-1.2 {coords 10 10} {
    set id [.t.c create pimage 100 100 -image _oval]
    .t.c coords $id 110.0 110.0
    .t.c coords $id
} {{110.0 110.0}}

# Local Variables:
# Coding: utf-8-unix
# Mode: Tcl
# End:


 