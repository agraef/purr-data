# py/pyext - python script objects for PD and MaxMSP
#
# Copyright (c) 2002-2005 Thomas Grill (gr@grrrr.org)
# For information on usage and redistribution, and for a DISCLAIMER OF ALL
# WARRANTIES, see the file, "license.txt," in this distribution.  
#

"""This is an example script for the py/pyext signal support.

For numarray see http://numeric.scipy.org
It will probably once be replaced by Numeric(3)
"""

try:
    import pyext
except:
    print "ERROR: This script must be loaded by the PD/Max py/pyext external"

try:
    import psyco
    psyco.full()
    print "Using JIT compilation"
except:
    # don't care
    pass

import sys,math

try:    
    import numarray
except:
    print "Failed importing numarray module:",sys.exc_value


class gain(pyext._class):
    """Just a simple gain stage"""
    
    gain = 0

    def _signal(self):
        # Multiply input vector by gain and copy to output
		try:
			self._outvec(0)[:] = self._invec(0)*self.gain
		except:
			pass


class gain2(pyext._class):
    """More optimized version"""
    
    gain = 0

    def _dsp(self):
        if not self._arraysupport():
            print "No DSP support"
            return False

        # cache vectors in this scope
        self.invec = self._invec(0)
        self.outvec = self._outvec(0)
        # initialize _signal method here for optimized version
        if self.invec is self.outvec:
            self._signal = self.signal1
        else:
            self._signal = self.signal2
        return True

    def signal1(self):
        # Multiply signal vector in place
        self.outvec *= self.gain
        
    def signal2(self):
        # Multiply input vector by gain and copy to output
        self.outvec[:] = self.invec*self.gain


class pan(pyext._class):
    """Stereo panning"""

    def __init__(self):
        self.float_1(0.5)

    def float_1(self,pos):
        """pos ranges from 0 to 1"""
        x = pos*math.pi/2
        self.fl = math.cos(x)
        self.fr = math.sin(x)
    
    def _dsp(self):
        # if _dsp is present it must return True to enable DSP
        return pyext._arraysupport()
    
    def _signal(self):
        # Multiply input vector by gain and copy to output
        iv = self._invec(0)
        # first process right output channel because left one could be
        # identical to input
        # we could also test with 'self._outvec(1)[:] is iv'
        self._outvec(1)[:] = iv*self.fr
        self._outvec(0)[:] = iv*self.fl
