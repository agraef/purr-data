import sys, os, time, signal, subprocess
try:
	import pyext
except:
	print "ERROR: This script must be loaded by the PD pyext external"
	sys.exit()

class sub(pyext._class):
	"""A simple script to start and stop process"""

	# number of inlets and outlets
	_inlets=1
	_outlets=1
	
	def __init__(self,*args):
		pass
	
	def start_1(self,a):
		global process
		process = subprocess.Popen(str(a))
		self._outlet(1, 'opening '+str(a))
		
	def stop_1(self,*a):
		os.kill(process.pid, signal.SIGTERM)
		self._outlet(1, 'stopping process '+str(process.pid))
