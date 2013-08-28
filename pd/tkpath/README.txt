
                           README for tkpath
                           _________________

This package implements path drawing modelled after its SVG counterpart,
see http://www.w3.org/TR/SVG11/. See the doc directory for more info.

There are three backends used for drawing. They are all platform specific
except for the Tk drawing which uses only the API found in Tk. This
backend is very limited and has some problems with multiple subpaths.
It is only to be used as a fallback when the cairo backend is missing.

The backends:

    1) CoreGraphics for MacOSX, built using ProjectBuilder
 
    2) GDI+ for WinXP, built by VC++7 (.NET), runs also on older system
       using the gdiplus.dll

    3) cairo (http://cairographics.org), built using the automake system;
       the configure.in and Makefile.in files are a hack, so please help
       yourself (and me). It requires a cairo 1.0 installation since
       incompatible API changes appeared before 1.0 (libcairo.so.2 ?).

There used to be two additional backends, GDI and core Tk drawing, but 
these have been dropped.

I could think of another backend based on X11 that has more features than
the compatibility layer of Tk, since the fallback is only necessary on unix 
systems anyway. Perhaps an OpenGL backend would also be useful, mainly on
unix systems without cairo support.

There are two important Design Principles:

    1) Follow the SVG graphics model. Make it more condensed without
       giving up any features. For instance, tkpath keeps only a -matrix
       option which comprises translate, scale etc. attributes

    2) Keep the actual path drawing code separate and independent of any
       canvas code. 

Open Issues:

There are a number of design choices that I'd like to discuss.

 o How to provide coordinates for prect? As the standard Tk way (x1,y1,x2,y2), 
   using sizes (x,y,width,height), or using options (x,y,-width,-height)?

 o What shall the precedence of the -style option compared to the individual
   options be?

Copyright (c) 2005-2008  Mats Bengtsson

BSD style license.
