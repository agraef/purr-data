# py/pyext - python script objects for PD and MaxMSP
#
# Copyright (c) 2002-2007 Thomas Grill (gr@grrrr.org)
# For information on usage and redistribution, and for a DISCLAIMER OF ALL
# WARRANTIES, see the file, "license.txt," in this distribution.  
#

"""This is an example script for the py/pyext object's basic functionality.

pyext Usage:
- Import pyext

- Inherit your class from pyext._class

- Specfiy the number of inlets and outlets:
	Use the class members (variables) _inlets and _outlets
	If not given they default to 1
	You can also use class methods with the same names to return the respective number

- Constructors/Destructors
	You can specify an __init__ constructor and/or an __del__ destructor.
	The constructor will be called with the object's arguments

	e.g. if your PD or MaxMSP object looks like
	[pyext script class arg1 arg2 arg3]

	then the __init__(self,*args) function will be called with a tuple argument
	args = (arg1,arg2,arg3) 
	With this syntax, you will have to give at least one argument.
	By defining the constructor as __init__(self,*args) you can also initialize 
	the class without arguments.

- Methods called by pyext
	The general format is 'tag_inlet(self,arg)' resp. 'tag_inlet(self,*args)':
		tag is the PD or MaxMSP message header.. either bang, float, list etc.
		inlet is the inlet (starting from 1) from which messages are received.
		args is a tuple which corresponds to the content of the message. args can be omitted.

	The inlet index can be omitted. The method name then has the format 'tag_(self,inlet,args)'.
	Here, the inlet index is a additional parameter to the method

	You can also set up methods which react on any message. These have the special forms
		_anything_inlet(self,*args)
	or
		_anything_(self,inlet,*args) 

	Please see below for examples.

	Any return values are ignored - use _outlet (see below).

	Generally, you should avoid method_, method_xx forms for your non-pyext class methods.
	Identifiers (variables and functions) with leading underscores are reserved for pyext.

- Send messages to outlets:
	Use the inherited _outlet method.
	You can either use the form
		self._outlet(outlet,arg1,arg2,arg3,arg4) ... where all args are atoms (no sequence types!)
	or
		self._outlet(outlet,arg) ... where arg is a sequence containing only atoms
		
    Do not use _outlet inside __init__, since the outlets have not been created at that time.

- Use pyext functions and methods: 
	See the __doc__ strings of the pyext module and the pyext._class base class.

"""

try:
	import pyext
except:
	print "ERROR: This script must be loaded by the PD/Max pyext external"

#################################################################

class ex1(pyext._class):
	"""Example of a simple class which receives messages and prints to the console"""

	# number of inlets and outlets
	_inlets=3
	_outlets=0


	# methods for first inlet

	def bang_1(self):
		print "Bang into first inlet"

	def int_1(self,f):
		print "Integer",f,"into first inlet"

	def float_1(self,f):
		print "Float",f,"into first inlet"

	def list_1(self,*s):
		print "List",s,"into first inlet"


	# methods for second inlet

	def hey_2(self):
		print "Tag 'hey' into second inlet"

	def ho_2(self):
		print "Tag 'ho' into second inlet"

	def lets_2(self):
		print "Tag 'lets' into second inlet"

	def go_2(self):
		print "Tag 'go' into second inlet"

	def _anything_2(self,*args):
		print "Some other message into second inlet:",args


	# methods for third inlet

	def onearg_3(self,a):
		print "Tag 'onearg' into third inlet:",a

	def twoargs_3(self,*a):
		if len(a) == 2:
			print "Tag 'twoargs' into third inlet:",a[0],a[1]
		else:
			print "Tag 'twoargs': wrong number of arguments"

	def threeargs_3(self,*a):
		if len(a) == 3:
			print "Tag 'threeargs' into third inlet",a[0],a[1],a[2]
		else:
			print "Tag 'threeargs': wrong number of arguments"

	def varargs_3(self,*args):
		# with *args there can be arguments or not

		print "Tag 'varargs' into third inlet",args



#################################################################

class ex2(pyext._class):
	"""Example of a simple class which receives messages and writes to outlets"""

	# number of inlets and outlets
	_inlets=3
	_outlets=2

	# methods for all inlets

	def hello_(self,n):
		print "Tag 'hello' into inlet",n

	def _anything_(self,n,*args):
		print "Message into inlet",n,":",args


	# methods for first inlet

	def float_1(self,f):
		self._outlet(2,f)

	# methods for second inlet

	def float_2(self,f):
		self._outlet(1,f)


#################################################################

# helper function - determine whether argument is a numeric type
def isNumber(value):
	import types
	if type(value) in (types.FloatType, types.IntType, types.LongType):
		return 1
	else:
		return 0


class ex3(pyext._class):
	"""Example of a simple class doing a typical number addition
	
	It uses a constructor and a class member as temporary storage.
	"""

	# number of inlets and outlets
	_inlets=2
	_outlets=1

	# temporary storage
	tmp=0

	# constructor
	def __init__(self,*args):
		if len(args) == 1: 
			if isNumber(args[0]):
				self.tmp = args[0]
			else:
				print "ex3: __init__ has superfluous arguments"

	# methods 

	def float_1(self,f):
		self._outlet(1,self.tmp+f)

	def float_2(self,f):
		self.tmp = f

	# handlers for MaxMSP int type
	def int_1(self,f):  
		self.float_1(f)

	def int_2(self,f):  
		self.float_2(f)
