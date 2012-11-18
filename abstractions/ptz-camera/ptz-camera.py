import pyext
import sys
  
class int2bytes(pyext._class):

  # number of inlets and outlets
  _inlets=1
  _outlets=4

  print "int2bytes init"

  # Constructor

  # methods 
  def float_1(self,number):
    number = int(number)
    for count in xrange(4):
      byte = (number & 0xF000) >> 12
      number <<=4
      self._outlet(count+1,byte)
      

      