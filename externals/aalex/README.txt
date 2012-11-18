ABOUT aalex's

These are objects for Pure Data. 

[x11mouse] generates a X11 mouse event
[x11key]   generates a X11 keyboard event
[xmms]     controls the X Multimedia System
[pcre]     matches a symbol against a Perl compatible regular expression. Still needs symbol as a selector for arguments.


I also include a few of my abstractions : 


=================================================
INSTALL

To use on Linux, add the path to this directory into your ~/.pdrc invisible file. 
Currenly, the only one to work on Mac is [matches]. All the others are for GNU/Linux.


=================================================
COMPILE

To compile on Debian, execute the follwing command in a terminal. 
It could probably work on Mac too, instlaling the packages with Fink. Tell
me if ever you need some of them for Mac, as the makefile will need to be modified. 


sudo apt-get install xlib-dev xmms-dev
cd aalex
make


=================================================
AUTHOR

Alexandre Quessy
alex@sourcelibre.com


=================================================
LICENSE

GNU Public License
)c( Copyleft 2006
Enjoy at your own risks !


=================================================
HISTORY

2006-07-18   Fixed *MD64 compilation flag (thanks to r1). Changed [matches] for [pcre].
2006-07      Initial release of old externals plus x11 objects.
