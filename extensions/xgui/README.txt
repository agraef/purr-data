This is the README file for Xgui_0.10b, an experimental GUI (Graphical User Interface) server for pure data.

The goal of Xgui is not to replace the existing GUI, but to add some graphicals functionality to pd.

Xgui is designed to run on all platforms supported by pure data.
It is working very well on linux, well on win2000 and not so well on win98 (something make it slow).

All this documentation is write in a poor english.

1) Here a try to explain what Xgui is :

  Xgui is a graphical server that wait for some orders from a socket.
  I cames with somes pd objects that allow to make the connection with the server.
  For example the "canvas" object allow to create a new windows.

  GOALS :
    To allow users of pure-data to disgn their own gui objects.
  PARADIGME :
    The gui-object will be entirely describe in pd patchs.
    One graphical object <=> One object in a pd patch.
  HOW :
    By provided some atom objects can that can :
      * creates somes windows
      * put some basics graphicals objects in it.
      * receive events in relations with the graphicals objects.

2) Quick start

  1. Run pdx
     This will load the Xgui server
     Then it will load pure-data
     Then it open the patch pdx_connect.pd
     Then it connect pd with the Xgui server via a loadbang.

  2. load one of the patch in the doc/Xgui directory

  3. Then play with the patchs
    and enjoy the result in the Xgui window.


2) Some more technical info :

  On a technical point view all of this is very simple.

  Xgui is divided into 2 part :
    * The Xgui server (Xgui.sh) is a tcl/tk script that exchange data with pd via socket

    * Some pd abstractions that are uses to establish the conection between pd and the Xgui server.(pdx_connect.pd)

    * some pd abstractions that represent a graphical object :
      - the "Canvas.pd" abstraction that is the object to open a Xgui windows.
      - The "seg.pd" abstraction that is the basic object to draw a line in the canvas.


3) Why Xgui is diferent from the iem/grip way ?

  Because in iem or grip, the basic elements for buildings a GUI are on a very hight level :
    Sliders, and so much more.
    Xgui didn't provide this king of high level features but allow the pd user to disgn their own ones.

4) Why Xgui is close to the data features of pure-data, and why it's different.

  The goals of the experimental data features of pure-data is certainly the same as Xgui :
  Provide a tools to compositor to create their own representation for their own musical language. And more.

  Pure data will be THE PERFECT TOOL for writting music and not only to generate RT sound. And more.

  The data approch is based on the Concept of template : The user will disgn the template of an object.
    The HUGE good point is that the user got the ablility to dynamicly cut and paste any objects.
    pure-data will automaticaly manage the duplication of the data.
    The bad point is that today the code that will manage the object is not inside the template.
    This mean that the way to write patch that use the data feature is not very easy.

Miller, please corect me if i'm wrong.


  The Xgui approch is base on his paradigm : One pd object <=> one graphical object.
    This imply that the pd object will provide properties, methods and events of the graphical side.
    And this make very easy to make complex GUI with very few simples objects.
    But it's not possible to duplicate a graphical object without duplicate the pd object...

  As you can see thoses two way of thinking are very complementary, and i hope to find a solution
    to have the best of both world

5) Releases :

  seg_0.01 : 17 of febuary 2002 : First release: basic idea.
  seg_0.02 : 20 of febuary 2002 : Structured basic idea.
    No functionality have been added, but the project is reorganised into folders.
    Some installations script are provided.
  seg_0.03 : 20 of march 2002 : multi windows
    Now seg offer the abollity to open as many windows as needed, to resize them, and draw into the rigth one.
    There is no compatibility between version 0.03 & 0.02 But it's easy to change the patchs
  seg_0.04 : 26 of march 2002 : bug correction & better doc
  seg_0.05 : 08 of april 2002 : add some basics physics modeling tools & examples
  Xgui_0.06 : 28 of april 2002 : add 3 objects (arc,rect,text) + doc + examples + event Rigth click
    Rename the project cause it doesn't deals with only the seg object now.
  Xgui_0.07 : 30 of April 2002 : Add the canvas color methods and corect some smalls bug
  Xgui_0.08 : 6 of june 2002 : Add the ability to syncronise many xgui windows with some peer2peer facility.
  Xgui_0.09 : 23 of july 2002 : Add the ability to save some preset.
  Xgui_0.10 : 10 of novembre 2002 :  add the ability to share some windows with other computer on the web.

6) Todo list :

  * Add some objects :pictures, menu, etc...
  * Add some methodes : nearest object, nearest point onto the object,etc...
  * add some behaviors :  colision, stick, etc...
  * add some event : keyboard, dblclick, etc..
  * integrate Xgui closer into pd.(concatenate the pd.tk and Xgui.sh for ex. Thats working !!!!)
  * why not rewrite it in java ?
  * Or write a policy to put the Xgui code into html using the tcl/tk plugins ?
  * a real doc.
  * Etc...
  * & even more.

7) Future :

  xgui will be more and more internet oriented in a sharing/colaborative/real-time/ point of view.
  But I still whant it be able to :
  * disgn very complex object like sequencer or mixing table.
  * disgn experimental object for driving experimental synthesis methods
  * be used for creating realtime graphics linked with music.
  * be used for writing experimental music
  * be open to be used for something I've not think about.


8) Contact : http://dh7.free.fr

  Please don't hesitate to contact me if you need some help or some info about Xgui.
  Don't hesitate to reports bugs too...

  Any feedback welcome.

  Damien HENRY : dh7@free.fr ; dh@dh7.net

8) Thanks :

* Miller Puckette
* Olaff Matthes for giving me the code of his remote object.
* Everybody involved in pure-data.
* & + ...

9) COPYRIGHT.

  Except as otherwise noted, all files in the Xgui distribution are

    Copyright (c) 2002 damien Henry and others.

For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "LICENSE.txt," included in the Xgui distribution.
