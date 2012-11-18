#!/usr/bin/env python
# ======================================================================
# file:		OSC.py
# author:	stefan kersten <steve@k-hornz.de>
# contents:	OSC client module for python
# license:	public domain
# ======================================================================
# $Id: OSC.py,v 1.1 2004-01-20 16:54:44 ksvalast Exp $
# ======================================================================
# copyright (c) 2000 stefan kersten
# ======================================================================
# this module provides simple OSC client functionality
# usage examples down at the end of the file
# ======================================================================

__revision__ = "$Revision: 1.1 $"

# ======================================================================
# imports

import cStringIO, exceptions, math, socket, struct, time, types

# ======================================================================
# constants

SECONDS_UTC_TO_UNIX_EPOCH = 2208988800.0
FLOAT_TO_INT_SCALE = pow(2.0, 32.0)

# ======================================================================
# types

class Value:
    """Abstract OSC value."""
    def __init__(self, value):
	self.value = value

    def binary_value(self):
        pass

    def type_tag(self):
        pass

class Int(Value):
    """32 bit integer value."""
    def __init__(self, value):
        Value.__init__(self, long(value))
        
    def binary_value(self):
        return struct.pack('!l', self.value)

    def type_tag(self):
        return 'i'

class Float(Value):
    """32 bit floating point value."""
    def __init__(self, value):
        Value.__init__(self, float(value))
        
    def binary_value(self):
        return struct.pack('!f', self.value)

    def type_tag(self):
        return 'f'

class String(Value):
    """Null-terminated string padded to multiples of 4 byte."""
    def __init__(self, value):
        Value.__init__(self, str(value))
        
    def binary_value(self):
        v = self.value
	l = len(v)
        return struct.pack('%ds%dx' % (l, self.pad_amount(l)), v)

    def type_tag(self):
        return 's'
    
    def pad_amount(self, len):
        return 4 - (len % 4)

class Time(Value):
    """64 bit timetag in NTP format."""
    def __init__(self, value):
        Value.__init__(self, float(value))
        
    def __add__(self, time):
	return Time(float(self.value + time.value))

    def binary_value(self):
	t = self.value
	# FIXME: how to convert without overflows?
	s = long(t)
	f = long(math.fmod(t, 1.0)*FLOAT_TO_INT_SCALE)
	return struct.pack('!LL', s, f)

# ======================================================================
# utilities

time_module = time
def time():
    """Return current time as float in OSC format."""
    return SECONDS_UTC_TO_UNIX_EPOCH + time_module.time()

# ======================================================================
# classes

class Packet:
    """Abstract base class for all OSC-related containers.

    Has methods for retrieving the proper binary representation
    and its size.
    """
    def __init__(self, packets):
        stream = cStringIO.StringIO()
        self._write_contents(packets, stream)
        self._data = stream.getvalue()

    def get_packet(self):
        """Return the binary representation of the receiver's contents.

        This data is in the proper OSC format and can be sent over a
        socket.
        """
        return self._data

    def get_size(self):
        """Return the size of the receiver's binary data."""
        return len(self._data)
    
    def _write_contents(self, packets, stream):
        """Write packets on stream.

        Private.

        Override in subclasses for specific behavior.
        """
        pass

    def __repr__(self):
        return '<' + \
               str(self.__class__.__name__) + \
               ' instance, size=' + \
               str(self.get_size()) + \
               '>'

    def sendto(self, host, port):
        """Send the receiver's data through a UDP socket."""
        s = socket.socket(socket.SOCK_DGRAM, socket.AF_INET)
        packet = self.get_packet()
        s.sendto(packet, (host, port))
	s.close()

    def sendlocal(self, port):
        """Send the receiver's data through a UDP socket locally."""
        self.sendto('localhost', port)

def _value(x):
    """Convert x(int, float or string) to an OSC object."""
    t = type(x)
    if t == types.FloatType:
        return Float(x)
    if t == types.IntType or t == types.LongType:
        return Int(x)
    # return string representation as default
    return String(str(x))

class Message(Packet):
    """Single OSC message with arguments.

    Message(address, *args) -> Message
    
    address	-- OSC address string
    *args 	-- message argument list
    """
    def __init__(self, address, args=[]):
	Packet.__init__(self, [String(address)] + map(lambda x: _value(x), args))

    def _write_contents(self, args, stream):
        t_stream = cStringIO.StringIO()	# tag stream
        v_stream = cStringIO.StringIO()	# value stream
	# open signature string
	t_stream.write(',')
	# collect tags and arguments
        for v in args[1:]:
            t_stream.write(v.type_tag())
            v_stream.write(v.binary_value())
        # write address
        stream.write(args[0].binary_value())
        # write signature
        stream.write(String(t_stream.getvalue()).binary_value())
        # write arguments
        stream.write(v_stream.getvalue())

class Bundle(Packet):
    """OSC container type with timing information.

    Bundle(time, packets) -> Bundle

    time	-- floating point timetag in OSC units
    packets	-- array of Packet(s)
    """
    def __init__(self, time, packets):
	Packet.__init__(self, [Time(time)] + packets)

    def _write_contents(self, args, stream):
        # write '#bundle' preamble
        stream.write(String('#bundle').binary_value())
        # write timetag
        stream.write(args[0].binary_value())
        # write packets, prefixed with a byte count
        for packet in args[1:]:
            data = packet.get_packet()
            size = len(data)
            stream.write(Int(size).binary_value())
            stream.write(data)

def test(port):
    """Some example messages and bundles, sent to port."""
    Message("/filter/cutoff", [145.1232]).sendlocal(port)
    Message("/http", ["www dot k-hornz dot de", 12, 3.41, "bulb"]).sendlocal(port)
    #print Int(len(Message("/msg").get_packet())).binary_value()
    Bundle(0.1, [Message("/fubar")]).sendlocal(port)
    Bundle(time(), [Message("/msg", [1.0, "+", 1, 61, "0"]), Message("/bang!")]).sendlocal(port)

if __name__ == "__main__":
    """Run dumpOSC on port 10000."""
    test(10000)

# EOF
# ======================================================================
