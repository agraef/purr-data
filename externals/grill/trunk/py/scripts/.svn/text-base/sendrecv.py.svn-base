# py/pyext - python script objects for PD and MaxMSP
#
# Copyright (c) 2002-2005 Thomas Grill (gr@grrrr.org)
# For information on usage and redistribution, and for a DISCLAIMER OF ALL
# WARRANTIES, see the file, "license.txt," in this distribution.  
#

"""This is an example script for the py/pyext object's send/receive functionality.

You can:
- bind


There are several classes exposing py/pyext features:
- ex1: A class receiving messages and sending them out again
- ex2: A class receiving messages and putting them out to an outlet
- ex3: Do some PD scripting

"""

try:
	import pyext
except:
	print "ERROR: This script must be loaded by the PD/Max pyext external"


from time import sleep

#################################################################

def recv_gl(arg):
	"""This is a global receive function, it has no access to class members."""
	print "GLOBAL",arg

class ex1(pyext._class):
	"""Example of a class which receives and sends messages

	It has two creation arguments: a receiver and a sender name.
	There are no inlets and outlets.
	Python functions (one global function, one class method) are bound to PD's or Max/MSP's receive symbols.
	The class method sends the received messages out again.
	"""


	# no inlets and outlets
	_inlets=1
	_outlets=0

	recvname=""   
	sendname=""

	def recv(self,*arg):
		"""This is a class-local receive function, which has access to class members."""

		# print some stuff
		print "CLASS",self.recvname,arg

		# send data to specified send address
		self._send(self.sendname,arg)


	def __init__(self,*args):
		"""Class constructor"""

		# store sender/receiver names
		if len(args) >= 1: self.recvname = args[0]
		if len(args) >= 2: self.sendname = args[1]

                self.bind_1()

        def bind_1(self):
		# bind functions to receiver names
		# both are called upon message 
		self._bind(self.recvname,self.recv)
		self._bind(self.recvname,recv_gl)
            
        def unbind_1(self):
		self._unbind(self.recvname,self.recv)
		self._unbind(self.recvname,recv_gl)

	def __del__(self):
		"""Class destructor"""

		# unbinding is automatically done at destruction
		pass


#################################################################

class ex2(pyext._class):
	"""Example of a class which receives a message and forwards it to an outlet

	It has one creation argument: the receiver name.
	"""


	# define inlets and outlets
	_inlets=0
	_outlets=1

	recvname=""   

	def recv(self,*arg):
		"""This is a class-local receive function"""

		# send received data to outlet
		self._outlet(1,arg)


	def __init__(self,rname):
		"""Class constructor"""

		# store receiver names
		self.recvname = rname

		# bind function to receiver name
		self._bind(self.recvname,self.recv)


#################################################################

from math import pi
from cmath import exp
from random import random,randint

class ex3(pyext._class):
	"""Example of a class which does some object manipulation by scripting"""


	# define inlets and outlets
	_inlets=1
	_outlets=0

	def __init__(self):
		"""Class constructor"""

		# called scripting method should run on its own thread
		if self._isthreaded:
		    print "Threading is on"
		    self._detach(1)  

	def bang_1(self):
		"""Do some scripting - PD only!"""

		num = 12				# number of objects
		ori = complex(150,180)  # origin
		rad = 100				# radius
		l = range(num)			# initialize list

		# make flower
		self._tocanvas("obj",ori.real,ori.imag,"bng",20,250,50,0,"empty","yeah","empty",0,-6,64,8,-24198,-1,-1)
		for i in xrange(num):
			l[i] = ori+rad*exp(complex(0,i*2*pi/num))
			self._tocanvas("obj",l[i].real,l[i].imag,"bng",15,250,50,0,"empty","yeah"+str(i),"empty",0,-6,64,8,0,-1,-1)
			self._tocanvas("connect",6,0,7+i,0)

		# blink
		for i in range(10):
			self._send("yeah","bang")
			sleep(1./(i+1))

		# move objects around
		for i in xrange(200):
			ix	= randint(0,num-1)
			l[ix] = ori+rad*complex(2*random()-1,2*random()-1)
			self._send("yeah"+str(ix),"pos",l[ix].real,l[ix].imag)
			sleep(0.02)

		# now delete
		# this is not well-done... from time to time an object remains
		self._tocanvas("editmode",1)
		for i in xrange(num):
			self._tocanvas("mouse",l[i].real,l[i].imag,0,0)
			self._tocanvas("cut")

		self._tocanvas("mouse",ori.real+1,ori.imag+1,0,0)
		self._tocanvas("cut")

		self._tocanvas("editmode",0)

