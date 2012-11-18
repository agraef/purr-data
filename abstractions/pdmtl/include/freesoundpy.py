#encoding: latin-1

# Aldrin
# Modular Sequencer
# Copyright (C) 2006 The Aldrin Development Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

"""
Provides an interface to the freesound web XML api.
"""

import urllib, urlparse, urllib2, cookielib
import xml.dom.minidom
import os,sys

class Freesound:
	BASE_URL = "http://www.freesound.org/"
	LOGIN_URL = urlparse.urljoin(BASE_URL, "forum/login.php")
	SEARCH_URL = urlparse.urljoin(BASE_URL, "searchTextXML.php")
	FILEINFO_URL = urlparse.urljoin(BASE_URL, "samplesViewSingleXML.php")
	DOWNLOAD_URL = urlparse.urljoin(BASE_URL, "samplesDownload.php")

	def __init__(self):
		self.cj = cookielib.LWPCookieJar()
		opener = urllib2.build_opener(urllib2.HTTPCookieProcessor(self.cj))
		urllib2.install_opener(opener)

	def call_post(self, baseurl, **kargs):
		url = baseurl
		data = urllib.urlencode(kargs)
		p = urllib2.urlopen(url,data)
		return p.read()
		
	def make_url(self, baseurl, **kargs):
		if kargs:
			baseurl += '?' + urllib.urlencode(kargs)
		print baseurl
		return baseurl

	def call(self, baseurl, **kargs):
		return urllib2.urlopen(self.make_url(baseurl,**kargs)).read()

	def call_stream(self, baseurl, **kargs):
		return urllib2.urlopen(self.make_url(baseurl,**kargs))

	def login(self,username,password):
		self.call_post(self.LOGIN_URL, 
			username=username,
			password=password,
			redirect="../index.php",
			login="login",
			autologin="1")
		assert self.is_logged_in()
			
	def is_logged_in(self):
		return urllib2.urlopen(self.SEARCH_URL).info()['content-type'].startswith('text/xml')
			
	def get(self, url):
		return urllib2.urlopen(url).read()

	def get_stream(self, url):
		return urllib2.urlopen(url)
		
	def retrieve(self, url, filename):
		conn = urllib2.urlopen(url)
		outf = open(filename,'wb')
		#print conn.info()['content-type']
		length = int(conn.info()['content-length'])
		while length:
			blocksize = min(length,8192)
			data = conn.read(blocksize)
			outf.write(data)
			length -= blocksize
		#return filename, conn.info()
		return 'done'
		#return urllib.urlretrieve(*args,**kargs)

	def get_sample_info(self, sampleid):
		data = self.call(
			self.FILEINFO_URL,
			id=sampleid)
		xmldoc = xml.dom.minidom.parseString(data)
		assert xmldoc.documentElement.tagName == "freesound"
		return xmldoc.documentElement

	def search(self,search,start=0,searchDescriptions=1,searchTags=1,searchFilenames=0,searchUsernames=0):
		data = self.call(
			self.SEARCH_URL,
			search=search,
			start=start,
			searchDescriptions=searchDescriptions,
			searchTags=searchTags,
			searchFilenames=searchFilenames,
			searchUsernames=searchUsernames)
		xmldoc = xml.dom.minidom.parseString(data)
		assert xmldoc.documentElement.tagName == "freesound"
		return xmldoc.documentElement
		
	def download(self, sampleid, *args, **kargs):
		url = self.make_url(self.DOWNLOAD_URL,id=sampleid)
		return self.retrieve(url, *args, **kargs)

if __name__ == '__main__':
	fs = Freesound()
