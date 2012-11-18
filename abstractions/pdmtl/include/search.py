import sys, os

try:
	import pyext
except:
	print "ERROR: This script must be loaded by the PD pyext external"
	sys.exit()

savepath = os.getcwd()
extension = 'wav'

def Walk( root, recurse=1, pattern='*', return_folders=0 ):
	import fnmatch, os, string
	
	# initialize
	result = []

	# must have at least root folder
	try:
		names = os.listdir(root)
	except os.error:
		return result

	# expand pattern
	pattern = pattern or '*'
	pat_list = string.splitfields( pattern , ';' )
	
	# check each file
	for name in names:
		fullname = os.path.normpath(os.path.join(root, name))

		# grab if it matches our pattern and entry type
		for pat in pat_list:
			if fnmatch.fnmatch(name, pat):
				if os.path.isfile(fullname) or (return_folders and os.path.isdir(fullname)):
					result.append(fullname)
				continue
				
		# recursively scan other folders, appending results
		if recurse:
			if os.path.isdir(fullname) and not os.path.islink(fullname):
				result = result + Walk( fullname, recurse, pattern, return_folders )
			
	return result

class file(pyext._class):
	"""A simple script to fetch wav in folders (recursive)"""

	# number of inlets and outlets
	_inlets=1
	_outlets=2
	
	def __init__(self,*args):
		pass
	
	def set_1(self,a):
			global savepath
			savepath = a
			
	def ext_1(self,a):
			global extension
			extension = a			
		
	def fetch_1(self,*a):
		files = Walk(str(savepath), 1, '*.'+str(extension))
		for file in files:
			self._outlet(1, str(file))
		filescap = Walk(str(savepath), 1, '*.'+str(extension).upper())
		for filecap in filescap:
			self._outlet(1, str(filecap))
		self._outlet(2, 'bang')
