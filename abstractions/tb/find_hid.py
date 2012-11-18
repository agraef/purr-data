# Python script to search the udev file system for devices
# Copyright (C) 2005 Tim Blechmann
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
#
# $Id: find_hid.py,v 1.3 2005-12-17 17:52:01 timblech Exp $
#

from os import popen, listdir

def parse_device(event, rules):
	pipe = popen('udevinfo -a -p /sys/class/input/%s' % event)
	
	line = pipe.readline()
	while line:
		line.strip()
		if '==' in line:
			setting, value = line.split('==')
			setting = setting.strip()
			if '{' in setting:
				setting = setting.split('{')[1].split('}')[0]
			value = value.strip().strip('"')

			if setting in rules:
				if rules [setting] == value:
					# we matched one rule, so we remove it from the pending rules
					del rules[setting]
					
		line = pipe.readline()

		if len(rules) == 0:
			pipe.close()
			print event
			return float(event.strip("event"))

	pipe.close()
	return -1

def find (*args):
	name = ""
	for token in args:
		name+=" " + str(token)

	name = name.strip()
	rules = name.split('" "')
	rules = map (lambda x: x.strip('"'), rules)

	ruledict = dict()

	for rule in rules:
		setting, value = rule.split('=',1)
		ruledict[setting] = value
	rules = ruledict

	events = filter(lambda x: "event" in x,  listdir('/sys/class/input/'))

	for event in events:
		ret = parse_device(event, dict(rules))
		if ret != -1:
			return int(ret)
		
	return -1
