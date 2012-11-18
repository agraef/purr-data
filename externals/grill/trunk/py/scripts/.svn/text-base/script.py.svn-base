# py/pyext - python script objects for PD and MaxMSP
#
# Copyright (c) 2002-2005 Thomas Grill (gr@grrrr.org)
# For information on usage and redistribution, and for a DISCLAIMER OF ALL
# WARRANTIES, see the file, "license.txt," in this distribution.  
#

"""Several functions to show the py script functionality"""

import sys

print "Script initialized"

try:
	print "Script arguments: ",sys.argv
except:
	print 

def numargs(*args):   # variable argument list
	"""Return the number of arguments"""
	return len(args)

def strlen(arg):   
	"""Return the string length"""
	# we must convert to string first (it's a symbol type most likely)
	return len(str(arg))


def strcat(*args):
	"""Concatenate several symbols"""
	return reduce(lambda a,b: a+str(b), args,"")

def addall(*args):   # variable argument list
	"""Add a couple of numbers"""
	return reduce(lambda a,b: a+b, args,0)


def ret1():
	return 1,2,3,4


def ret2():
	return "sd","lk","ki"


def ret3():
	return ["sd","lk","ki"]

