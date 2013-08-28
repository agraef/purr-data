                  
                             tkpath README
                             _____________

This package implements a canvas widget which supports all features of the
original canvas but adds a number of additional things. There are a
number of new item types that are modelled after its SVG counterpart,
see http://www.w3.org/TR/SVG11/. In addition, all items are put in a tree
structure with a persistent root item with id 0. All other items are
descendants of this root item. The standard canvas items will always be a
child of the root item. The tkpath items, described below, are by default
a child of the root item, but can be configured to be a child of any group
item using the -parent option.

There can be subtle differences compared to the original canvas. 
One such situation is where an option value has switched from an integer 
to float (double).

 o Syntax: The canvas is created using:

        ::tkp::canvas pathName ?options?

    It creates a command as usual:

    pathName option ?arg arg ...?

 o The canvas tree structure:

      0----
	  1
	  2
	  3
	  4
	  5----
	      6
	      7
	  8----
	      9
	     10
	 11
	 12

 o Additional commands

    pathName ancestors tagOrId
        Returns a list of item id's of the first item matching tagOrId
	starting with the root item with id 0.

    pathName children tagOrId
        Lists all children of the first item matching tagOrId.

    pathName depth tagOrId
	Returns the depth in the tree hierarchy of the first
	item matching tagOrId. The root item has depth 0 and children
	of the root has depth 1 and so on.

    pathName distance tagOrId x y
        Returns the closest distance between the point (x, y) and the first
	item matching tagOrId.

    pathName firstchild tagOrId
        Returns the first child item of the first item matching tagOrId.
        Applies only for groups.

    pathName gradient command ?options?
        See tkp::gradient for the commands. The gradients created with this
	command are local to the canvas instance. Only gradients defined
	this way can be used.

    pathName lastchild tagOrId
        Returns the last child item of the first item matching tagOrId.
        Applies only for groups.

    pathName nextsibling tagOrId
        Returns the next sibling item of the first item matching tagOrId.
	If tagOrId is the last child we return empty.

    pathName parent tagOrId
        Returns the parent item of the first item matching tagOrId. This
	command works for all items, also for the standard ones. It is
	therefore better to use this than 'cget id -parent' which is only
	supported for the new tkpath items.

    pathName prevsibling tagOrId
        Returns the previous sibling item of the first item matching tagOrId.
	If tagOrId is the first child we return empty.
    pathName style cmd ?options?
         See tkp::style for the commands. The styles created with this
	command are local to the canvas instance. Only styles defined
	this way can be used.
        
    pathName types
        List all item types defined in canvas.

 o Additional options

    -tagstyle expr|exact|glob     Not implemented.

 o Commands affected by changes

    lower/raise: 
	movement is constrained to siblings. If reference tagOrId
    	not given it defaults to first/last item of the root items children.
    	Items which are not siblings to the reference tagOrId are silently
    	ignored. Good or bad?

    find above/below: 
	is constrained to siblings. Good or bad?

   scale/move:
	if you apply scale or move on a group item it will apply this to all its 
	descendants, also to child group items in a recursive way.

   tag "all":
	Note that this presently also includes the root item which can result in some
	unexpected behavior. In many case you can operate on the root item (0) instead.
	As an example, if you want to move all items in canvas,	then do:
	  	pathName move 0 x y
	and similar for scale etc.

 o New items

	There are various differences compared to SVG. 
	The display attribute names are adapted to tcl conventions, see below.
	Also, SVG is web oriented and therefore tolerates parameter errors to some
	degree, while tk is a programming tool and typically generates errors
	if parameters are wrong. Some syntax changes have also been made. One such is
	the -matrix option where we have delegated specific transforms to our support
	functions in tkpath.tcl. Where the SVG tag names coincide with the ordinary
	canvas item names we have added a "p" in front of its name instead.

	New items:
		circle
		ellipse
		group
		path
		pimage
		pline
		polyline
		ppolygon
		prect
		ptext

 o The options

    The options can be separated into a few groups depending on the nature
    of an item for which they apply. Not all are implemented.

    Fill (fillOptions):
        -fill color|gradientToken       this is either a usual tk color
                                        or the name of a gradient
        -fillopacity float (0,1)
        -fillrule nonzero|evenodd

    Stroke (strokeOptions):
        -stroke color
        -strokedasharray dashArray
        -strokelinecap 
        -strokelinejoin
        -strokemiterlimit float
        -strokeopacity float (0,1)
        -strokewidth float

    Generic (genericOptions):
        -matrix {{a b} {c d} {tx ty}}
	-parent tagOrId
        -state
        -style styleToken
        -tags tagList

    A matrix is specified by a double list as {{a b} {c d} {tx ty}}.
    There are utility functions to create a matrix using simpler transformations,
    such as rotation, translation etc.

    The styleToken is a style created with 'pathName style create'. 
    It's options take precedence over any other options set directly. 
    This is how SVG works (bad?). Currently all a style's options ever set
    are recorded in a cumulative way using a mask. Even if an option is set
    to its default it takes precedence over an items option.

 o The group item

   A group item is merely a placeholder for other items, similar to how a
   frame widget is a container for other widgets. It is a building block for
   the tree structure. Unlike other items, and unlike frame widgets, it 
   doesn't display anything. It has no coordinates which is an additional
   difference. The root item is a special group item with id 0 and tags
   equal to "root". The root group can be configured like other items, but
   its -tags and -parent options are read only.
   Options set in a group are inherited by its children but they never override
   options explicitly set in children. This also applies to group items configured
   with a -style.

   .c create group ?fillOptions strokeOptions genericOptions?

 o The path item

    The path specification must be a single list and not concateneted with
    the rest of the command:

    right:  .c create path {M 10 10 h 10 v 10 h -10 z} -fill blue
    wrong:  .c create path M 10 10 h 10 v 10 h -10 z -fill blue    ;# Error

    Furthermore, coordinates are pixel coordinates and nothing else.
    SVG: It implements the complete syntax of the path elements d attribute with
    one major difference: all separators must be whitespace, no commas, no
    implicit assumptions; all instructions and numbers must form a tcl list.

    .c create path pathSpec ?fillOptions strokeOptions genericOptions?

    All path specifications are normalized initially to the fundamental atoms
    M, L, A, Q, and C, all upper case. When you use the canvas 'coords' command
    it is the normalized path spec that is returned. Bad?

    Visualize this as a pen which always has a current coordinate after
    the first M. Coordinates are floats:

      M x y   Put the pen on the paper at specified coordinate.
              Must be the first atom but can appear any time later.
              The pen doesn't draw anything when moved to this point.
      L x y   Draw a straight line to the given coordinate.
      H x     Draw a horizontal line to the given x coordinate.
      V y     Draw a vertical line to the given y coordinate.
      A rx ry phi largeArc sweep x y
              Draw an elliptical arc from the current point to (x, y). 
              The points are on an ellipse with x-radius rx and y-radius ry.
              The ellipse is rotated by phi degrees. If the arc is less than 
              180 degrees, largeArc is zero, else it is one. If the arc is to be
              drawn in cw direction, sweep is one, and zero for the ccw
              direction.
              NB: the start and end points may not coincide else the result
	          is undefined. If you want to make a circle just do two
		  180 degree arcs.
      Q x1 y1 x y
              Draw a qadratic Bezier curve from the current point to (x, y)
              using control point (x1, y1).
      T x y   Draw a qadratic Bezier curve from the current point to (x, y)
              The control point will be the reflection of the previous Q atoms
              control point. This makes smooth paths.
      C x1 y1 x2 y2 x y
              Draw a cubic Bezier curve from the current point to (x, y)
              using control points (x1, y1) and (x2, y2).
      S x2 y2 x y
              Draw a cubic Bezier curve from the current point to (x, y), using
              (x2, y2) as the control point for this new endpoint. The first
              control point will be the reflection of the previous C atoms
              ending control point. This makes smooth paths.
      Z       Close path by drawing from the current point to the preceeding M 
              point.

    You may use lower case characters for all atoms which then means that all
    coordinates, where relevant, are interpreted as coordinates relative the
    current point.

 o The prect item

    This is a rectangle item with optionally rounded corners.
    Item specific options:

        -rx  corner x-radius, or if -ry not given it sets the uniform radius.
        -ry  corner y-radius

    .c create prect x1 y1 x2 y2 ?-rx -ry fillOptions strokeOptions genericOptions?

 o The circle item

   A plain circle item. Item specific options:

       -r  its radius; defaults to zero

   .c create circle cx cy ?-r fillOptions strokeOptions genericOptions?

 o The ellipse item

    An ellipse item. Item specific options:

        -rx  its x-radius
        -ry  its y-radius

    .c create ellipse cx cy ?-rx -ry fillOptions strokeOptions genericOptions?

 o The pline item

    Makes a single segment straight line.

    .c create pline x1 y1 x2 y2 ?strokeOptions genericOptions?

 o The polyline item

    Makes a multiple segment line with open ends.

    .c create polyline x1 y1 x2 y2 .... ?strokeOptions genericOptions?

 o The ppolygon item

    Makes a closed polygon.

    .c create ppolygon x1 y1 x2 y2 .... ?fillOptions strokeOptions genericOptions?

 o The pimage item

   This displays an image in the canvas anchored nw. If -width or -height is
   nonzero then the image is scaled to this size prior to any affine transform.

   .c create pimage x y ?-image -width -height genericOptions?

 o The ptext item

   Displays text as expected. Note that the x coordinate marks the baseline
   of the text. Gradient fills unsupported so far. Especially the font 
   handling and settings will likely be developed further. 
   Editing not implemented. The default font family and size is platform dependent.
   
   .c create ptext x y ?-text string -textanchor start|middle|end?
       ?-fontfamily fontname -fontsize float?
       ?fillOptions strokeOptions genericOptions?

 o The Matrix

    Each tkpath item has a -matrix option which defines the local coordinate
    system for that item. It is defined as a double list {{a b} {c d} {tx ty}}
    (better with a flat list {a b c d tx ty} ?) where a simple scaling
    is {{sx 0} {0 sy} {0 0}}, a translation {{1 0} {0 1} {tx ty}}, and a 
    rotation around origin with an angle 'a' is {{cos(a) sin(a)} {-sin(a) cos{a}} {0 0}}.
    The simplest way to interpret this is to design an extra coordinate
    system according to the matrix, and then draw the item in that system.

    Inheritance works differently for the -matrix option than for the other
    options which are just overwritten. Instead any set -matrix option
    starting from the root, via any number of group items, to the actual
    item being displayed, are nested. That is, any defined matrices from
    the root down define a sequence of coordinate transformations.

 o Antialiasing, if available, is controlled by the variable tkp::antialias.
    Switch on with:
    set tkp::antialias 1

 o The command tkp::pixelalign says how the platform graphics library draw
   when we specify integer coordinates. Some libraries position a one pixel
   wide line exactly at the pixel boundaries, and smears it out, if
   antialiasing, over the adjecent pixels. This can look blurred since a
   one pixel wide black line suddenly becomes a two pixel wide grey line.
   It seems that cairo and quartz (MacOSX) do this, while gdi+ on Windows
   doesn't. This command just provides the info for you so you may take
   actions. Either you can manually position lines with odd integer widths
   at the center of pixels (adding 0.5), or set the ::tkp::depixelize equal
   to 1, see below.
   
 o With the boolean variable ::tkp::depixelize equal to 1 we try to adjust
   coordinates for objects with integer line widths so that lines ...

 o Styles are created and configured using:

    tkp::style cmd ?options?

        tkp::style cget token option
            Returns the value of an option.

        tkp::style configure token ?option? ?value option value...?
            Configures the object in the usual tcl way.

        tkp::style create ?fillOptions strokeOptions?
            Creates a style object and returns its token.

        tkp::style delete token
            Deletes the object.

        tkp::style inuse token
	    If any item is configured with the style token 1 is
	    returned, else 0.

        tkp::style names
            Returns all existing tokens.

    The same options as for the item are supported with the exception of -style,
    -state, and -tags.


 o Gradients can be of two types, linear and radial. They are created and 
   configured using:

    tkp::gradient command ?options?

        tkp::gradient cget token option
            Returns the value of an option.

        tkp::gradient configure token ?option? ?value option value...?
            Configures the object in the usual tcl way.

        tkp::gradient create type ?-key value ...?
            Creates a linear gradient object with type any of linear or radial
            and returns its token.

        tkp::gradient delete token
            Deletes the object.

        tkp::gradient inuse token
	    If any item is configured with the gradient token 1 is
	    returned, else 0.

        tkp::gradient names
            Returns all existing tokens.

        tkp::gradient type token
            Returns the type (linear|radial) of the gradient.

    The options for linear gradients are:
        -method pad|repeat|reflect    partial implementation; defaults to pad
        -stops {stopSpec ?stopSpec...?}
            where stopSpec is a list {offset color ?opacity?}.
            All offsets must be ordered and run from 0 to 1.
        -lineartransition {x1 y1 x2 y2}
            specifies the transtion vector relative the items bounding box.
            Depending on -units it gets interpreted differently.
            If -units is 'bbox' coordinates run from 0 to 1 and are relative
            the items bounding box. If -units is 'userspace' then they are
            defined in absolute coordinates but in the space of the items
            coordinate system. It defaults to {0 0 1 0}, left to right.
        -matrix {{a b} {c d} {tx ty}}
            sets a specific transformation for the gradient pattern only.
	    NB: not sure about the order transforms, see -units.
        -units bbox|userspace sets the units of the transition coordinates.
            See above. Defaults to bbox.

    The options for radial gradients are the same as for linear gradients
    except that the -lineartransition is replaced by a -radialtransition:
       -radialtransition {cx cy ?r? ?fx fy?}
           specifies the transition circles relative the items bounding box
           and run from 0 to 1. They default to {0.5 0.5 0.5 0.5 0.5}.
           cx,cy is the center of the end circle and fx,fy the center of the
           start point.


 o In memory drawing surface

    tkp::surface new width height

    creates an in memory drawing surface. Its format is platform dependent.
    It returns a token which is a new command.

    tkp::surface names

    lists the existing surface tokens.

    The surface token commands are:

    $token copy imageName

    copies the surface to an existing image (photo) and returns the name of
    the image so you can do: 
    set image [$token copy [image create photo]]
    See Tk_PhotoPutBlock for how it affects the existing image.

    The boolean variable tkp::premultiplyalpha controls how the copy
    action handles surfaces with the alpha component premultiplied. If 1 the
    copy process correctly handles any format with premultiplied alpha. This
    gets the highest quality for antialiasing and correct results for partial
    transparency. It is also slower. If 0 the alpha values are not remultiplied
    and the result is wrong for transparent regions, and gives poor antialiasing
    effects. But it is faster. The default is 1.

    $token create type coords ?options?

    draws the item of type to the surface. All item types except the group 
    and the corresponding options as described above are supported, 
    except the canvas specific -tags and -state.

    $token destroy

    destroys surface.

    $token erase x y width height

    erases the indicated area to transparent.

    $token height 
    $token width

    returns height and width respectively.

    Note that the surface behaves different from the canvas widget. When you have put
    an item there there is no way to configure it or to remove it. If you have done
    a mistake then you have to erase the complete surface and start all over.
    Better to experiment on the canvas and then reproduce your drawing to a surface
    when you are satisfied with it.

    NB: gdi+ seems unable to produce antialiasing effects here but there seems
        to be no gdi+ specific way of drawing in memory bitmaps but had to call
        CreateDIBSection() which is a Win32 GDI API.


 o Helper function for making transformation matrices:

    tkp::transform cmd ?args?

        tkp::transform rotate angle ?centerX centerY?

        tkp::transform scale factorXY ?factorY?

        tkp::transform skewx angle

        tkp::transform skewy angle

        tkp::transform translate x y


 o Known issues:

   - See the TODO file and comments marked "@@@" in the C sources.
     

 o Further documentation:

    - http://www.w3.org/TR/SVG11/

    - http://cairographics.org

Copyright (c) 2005-2008  Mats Bengtsson

BSD style license.

