volctl~

a fast and smooth volume control external ...

volctl~ is doing more or less the same as


     |line~ 0 10		
|    |              |     |     |
|*~ 0|            = |volctl 0 10|
|		            |

except that it is faster

argument1: initial factor
argument2: interpolation time


volctl~ will only probably only compile against pd>=devel_0_37 with gcc.
i'm not planing to do a port to win/osx or any pd without aligned dsp blocks...

todo:
- check icc's segfault