# $Id: bpe.py,v 1.2 2005-12-17 17:52:01 timblech Exp $
#
# Copyright (C) 2005  Tim Blechmann
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.


import pyext

def parse_arguments(*args):
	duration = args[0]
	if duration == 0:
		return args[1:]
	
	length = len(args)
	durations = []
	breakpoints = []
	for i in range(1,length):
		if i % 2 == 0:
			durations.append(args[i])
		else:
			breakpoints.append(args[i])
	duration_factor = duration / reduce(lambda x,y: x + y, durations)
	
	durations = map(lambda x: x*duration_factor, durations)

	ret = []

	for i in range(len(durations)):
		ret.append(breakpoints[i])
		ret.append(durations[i])
	ret[-2] = ret[0]
	return ret
	
class Bpe(pyext._class):
	_inlets = 1
	_outlets = 1

	def list_1(self, *args):
		if args[0] != 0:
			self._outlet(1,parse_arguments(*args))

class VolBpe(pyext._class):
	_inlets = 3
	_outlets = 1

	def __init__(self, dollar1):
		try:
			self.duration = 1000 / dollar1
		except:
			self.duration = 1000
		self._detach = 1
		self._priority(-20)

		self.bpe = []
		self.running = False
		
	def list_1(self, *args):
		arglist = list(args)
		arguments = [self.duration] + arglist
		self.bpe = arglist
		self._outlet(1,parse_arguments(*arguments))
		if not self.running:
			self._outlet(1, None)
			self.running = True
			
	def float_2(self, freq):
		self.duration = 1000 / freq;
		arguments = self.duration + self.bpe
		self._outlet(1,parse_arguments(*arguments))

	
	def float_3(self, switch):
		if switch == 0:
			self._outlet(1, self.bpe[0], 0, self.bpe[0], self.duration)
		else:
			self._outlet(1, self.bpe)
	
