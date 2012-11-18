# py/pyext - python script objects for PD and MaxMSP
#
# Copyright (c) 2002-2003 Thomas Grill (xovo@gmx.net)
# For information on usage and redistribution, and for a DISCLAIMER OF ALL
# WARRANTIES, see the file, "license.txt," in this distribution.  
#

"""This is an example script for showing a nonsense tcl/tk application."""

try:
	import pyext
except:
	print "ERROR: This script must be loaded by the PD/Max pyext external"

from Tkinter import *
import random


class Application(Frame):                                           
	"""This is the TK application class"""

	# Button pressed
	def say_hi(self):                                                    
		self.extcl._outlet(1,"hi there, everyone!")

	# Mouse motion over canvas
	def evfunc(self, ev):
		x = random.uniform(-3,3)
		y = random.uniform(-3,3)
		self.mcanv.move('group',x,y)	

	# Create interface stuff
	def createWidgets(self):                                             
		self.hi = Button(self)                                         
		self.hi["text"] = "Hi!"                                       
		self.hi["fg"]   = "red"                                       
		self.hi["command"] =  self.say_hi                                                                       
		self.hi.pack({"side": "left"})                                
                                                                    
		self.mcanv = Canvas(self)
		self.mcanv.pack({"side": "left"})
		self.mcanv.bind("<Motion>", self.evfunc)
		self.mcanv.create_rectangle(50,50,200,200)
		r = self.mcanv.create_rectangle(50,50,200,200)
		self.mcanv.addtag_withtag('group',r)

		for i in xrange(500):
			x = random.uniform(50,200)
			y = random.uniform(50,200)
			l = self.mcanv.create_line(x,y,x+1,y)
			self.mcanv.addtag_withtag('group',l)
                                                                    
	# Constructor
	def __init__(self,cl):
		self.extcl = cl
		Frame.__init__(self)
		self.pack()
		self.createWidgets()
		pass


# derive class from pyext._class

class myapp(pyext._class):   
	"""This class demonstrates how a TCL/TK can be openened from within a pyext external"""

	# how many inlets and outlets?
	_inlets = 1
	_outlets = 1

	# Constructor
	def __init__(self):
		# detach bang method
		self._detach(1)

	def bang_1(self):
		self._priority(-3)
		# display the tcl/tk dialog
		app = Application(self)
		app.mainloop()

