_ Memento _

ver. 0.5

This is a work-in-progress collection of a unified preset system for Pd
patches. It tries to loosely follow the Memento [1] design pattern from the Gang of
Four book.

I propose that a Memento here is realised as a directory in a data pool inside
a [pool] object. Currently the pool is hardcoded to be named "RRADICAL". This
could change, but maybe it doesn't need to. Inside the Memento, several
numbered subfolders are created to store the actual substates.

The Memento is only changed or read through the [originator] abstraction. 
[originator] has two arguments: 
  
1) the name of a Memento 
2) $0

Methods of [originator]:
 
 * create Memento-name: creates a new Memento, discards the old.
 * set anything: sets all variables to the values currently in the pool. For use
   after loading a file with the [caretaker]
 * substate management
 * right inlet: OSC messages
 * some more. Look inside.
 
It is not allowed to bypass [originator] to change the values inside the
RRADICAL pool, with one exception: 

The [caretaker] abstraction is responsible for saving and loading pool data from
harddisk. [caretaker] takes a filename to save and load as argument. A GUI for
it is provided in [careGUI].

Methods of [caretaker]:

 * file filename: set's filename to save/load.
 * save poolname: saves the contents of the pool named "poolname". For rradical
   patches this currently has to be "RRADICAL"
 * load poolname: loads the contents, same as save, but the other way around.

Communication between [originator] and anything that wants to save anything is
done through the [commun] abstraction. 

[commun] arguments: 

1) the name of a variable ("key")
2) $0

[commun] accepts anything and spits out anything if [originator] tells it too
(with [originator]'s "set" message). 

Both $0 arguments above are mandatory to keep Memento reusable, which is the
first goal all in RRADical patches.

_ Usage _ 

Every abstraction, that wants to take part in saving its settings with Memento
needs to:
1) include an [originator /Memento $0] objects
2) cross-wrap all variables with [commun /id $0] objects. Because of a bug,
   for wrapping symbols a [symbol] objects has to be added after [commun]'s
   outlet.
3) The slash is there to allow routing of OSC-messages for remote control.

A single [caretaker] or [careGUI] in the parent patch then can save and reload
settings in the pool to disk.

Please follow the tutorial to learn more about Memento-usage.

_ ChangeLog _

0.5
* removed all prepend from cyclone again and instead use [prepent] with is just
  [list prepend]-[list trim]
* replace OSCprepend with a Miller Vanilla version.
* added [list] to commun's outlet to get automatic type conversions. 
  commun now will output proper list-messages. This may result in 
  subtle bugs with older patches however!

0.4
* Wrote a tutorial, yeah.
* Now uses Cyclone's set-able prepend for setting state.
* originator got a OSC-inlet.
* requires a "pool" with the mkchdir messages (available in CVS since Dec 17 2003)
* some cleanup

0.3
* OSC remote control. Useage example is in netcontrol.pd
* copy/paste with messages copy and past to originator.

0.2 
* Use numbered subfolders as states
* More examples, careGUI.pd

0.1 
* Initial release

_ Bugs _

* Plenty. 
* [prepend] strips off type identifiers like "symbol", so you currently have to
  take care of types yourself. Only lists and floats work out of the box 
  currently.
* The "API" is is bound to change.
* spelling errors

_ Footnotes _

[1] Here's a description of the Memento pattern by Kamal Patel from
http://www.kamalpatel.net/Articles/MementoPattern.htm

A look at a Design Pattern - Memento:

A Design Pattern can be thought of as an architectural layout to solve a
problem whose actual implementation could be in millions of ways. The approach
used to solve this problem is by a Design Pattern called Memento. The intent of
a Memento is to capture and externalize an object's internal state so
that the object can be restored to this state later.

Every Design pattern has a motivation. The Motivation behind the Memento
Pattern arose from the need to store the objects current state and then be able
 to restore it when requested. Let us look at an example where a Memento Pattern
could be used. Consider that we want to implement Undo features into our
application. We will need to keep a track of changes as the user works by
capturing each state via a Memento object and then when the user requests an
Undo, we will request the Memento to reset to an the last stored state. We
would achieve this by the two general interfaces exposed by the Memento;
GetState() and SetState().

A memento pattern is constructed by three objects:

Memento
Stores internal state of the Originator object.
Protects against access by objects other than the originator. Mementos
effectively have two interfaces (methods).

Originator
Creates a Memento containing a snapshot of its current internal state
Uses the Memento to restore its internal state

CareTaker
Is responsible for the Memento's safekeeping
Never operates on or examines the state of the Memento

Generally a memento is responsible for storing a snapshot of the internal state
of another object. In our case the Memento was responsible for storing the
state of a grid and restoring the state of the grid upon request. Mementos are
usually dynamic in nature but we used the same concept to store persistent
state of the grids into a VFP table. This is because it was faster an easier to
maintain.

Remember that Design Patterns provide us with a guidance to solve a problem for
the most commonly occurring software problems. We use this as a basis for our
design and then enhance or extend the design depending on our requirements.
