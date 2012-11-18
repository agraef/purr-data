# py/pyext - python script objects for PD and MaxMSP
#
# Copyright (c) 2002-2003 Thomas Grill (xovo@gmx.net)
# For information on usage and redistribution, and for a DISCLAIMER OF ALL
# WARRANTIES, see the file, "license.txt," in this distribution.  
#

"""This is an example script for the py/pyext object's threading functionality.

For threading support pyext exposes several function and variables

- _detach([0/1]): by enabling thread detaching, threads will run in their own threads
- _priority(prio+-): you can raise or lower the priority of the current thread
- _stop({wait time in ms}): stop all running threads (you can additionally specify a wait time in ms)
- _shouldexit: this is a flag which indicates that the running thread should terminate

"""

try:
	import pyext
except:
	print "ERROR: This script must be loaded by the PD/Max pyext external"

from time import sleep

#################################################################

class ex1(pyext._class):
	"""This is a simple class with one method looping over time."""

	# number of inlets and outlets
	_inlets=2
	_outlets=2

	sltime=0.1 # sleep time
	loops=20   # loops to iterate

	# method for bang to any inlet
	def bang_(self,n):
		for i in xrange(self.loops):
			# if _shouldexit is true, the thread ought to stop
			if self._shouldexit: 
				print "BREAK"
				break

			self._outlet(n,i)
			sleep(self.sltime)



