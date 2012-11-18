"""This is an example script for the py/pyext object's threading functionality.
For threading support pyext exposes several function and variables

- _detach([0/1]): by enabling thread detaching, threads will run in their own threads
- _priority(prio+-): you can raise or lower the priority of the current thread
- _stop({wait time in ms}): stop all running threads (you can additionally specify a wait time in ms)
- _shouldexit: this is a flag which indicates that the running thread should terminate
"""

import os, freesoundpy
try:
	import pyext
except:
	print "ERROR: This script must be loaded by the PD/Max pyext external"	

savepath = os.getcwd()
def convert2mb(nbbytes):
		MB = 9.53674316 * 10e-8
		return nbbytes * MB	

class fspd(pyext._class):
	"""Connect to FreeSound and fetch samplings"""

	# number of inlets and outlets
	_inlets=1
	_outlets=2


	def __init__(self,*args):
		self.fs = freesoundpy.Freesound()
		#arguments of [pyext freesoundpd fspd username password]
		if len(args) == 2: 
			self.username = args[0]
			self.password = args[1]
			try:
				self.fs.login(self.username, self.password)
				print '\nConnected'
				#self._outlet(1,'Connected')
			except:
				print '\nUsername and password error'
				#self._outlet(1,'UsernamePasswordError')

	def connect_1(self,*a):
		if len(a) == 2:
			try:
				self.fs.login(a[0], a[1])
				print '\nConnected'
				#self._outlet(1,'Connected')
			except:
				print '\nUsername and password error'
				#self._outlet(1,'UsernamePasswordError')
		
	def search_1(self,a):
		print "\nSearching:",a
		sfresult = self.fs.search(a)
		print len(sfresult.getElementsByTagName("sample")), "results"
		
		for sample in sfresult.childNodes:
			if self._shouldexit:
				#print "STOP"
				break
			if sample.nodeType == sample.ELEMENT_NODE:
				info = self.fs.get_sample_info(sample.attributes["id"].value)
				sfid = info.getElementsByTagName('sample')[0].attributes["id"].value
				sfpreview = info.getElementsByTagName('preview')[0].firstChild.data
				sfname = info.getElementsByTagName('originalFilename')[0].firstChild.data[:30]
				print "\nID".ljust(8),"Username".ljust(21),"Filename".ljust(30),"Date".ljust(13),"Down".ljust(7),"SR".ljust(8),"BIT".ljust(6),"CH".ljust(6),"Length".ljust(12),"Size".ljust(9)
				print sfid.strip().ljust(7) \
				,info.getElementsByTagName('name')[0].firstChild.data[:20].strip().ljust(21) \
				,sfname.strip().ljust(30) \
				,info.getElementsByTagName('date')[0].firstChild.data[:-9].strip().ljust(13) \
				,info.getElementsByTagName('downloads')[0].firstChild.data.strip().ljust(7) \
				,info.getElementsByTagName('samplerate')[0].firstChild.data.strip().ljust(8) \
				,info.getElementsByTagName('bitdepth')[0].firstChild.data.strip().ljust(6) \
				,info.getElementsByTagName('channels')[0].firstChild.data.strip().ljust(6) \
				,"%.2fs".ljust(11," ") % float(info.getElementsByTagName('duration')[0].firstChild.data[:5]) \
				,"%.2f".ljust(9) % convert2mb(float(info.getElementsByTagName('filesize')[0].firstChild.data))

	def preview_1(self,a):
			try:
				info = self.fs.get_sample_info(a)
				sfurl = info.getElementsByTagName('preview')[0].firstChild.data
				self._outlet(2, str(sfurl))
			except:
				print("cannot find this sampling\n")
				
	def set_1(self,a):
			global savepath
			savepath = a
								
	def get_1(self,a):
			print '\nDownloading...'
			try:
				info = self.fs.get_sample_info(a)
				sfname = info.getElementsByTagName('originalFilename')[0].firstChild.data
				
				dwfile = savepath + '/' + sfname
				self.fs.download(a, dwfile)
				print 'Done:', dwfile
			except:
				print("Cannot download\n")				
