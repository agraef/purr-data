About FFTease 3.0

FFTease is a collection of objects designed to facilitate spectral sound
processing in Max. The collection was designed by Eric Lyon and
Christopher Penrose in 1999, and has been maintained by Eric Lyon since
2003. 


Installation 


The contents of "fftease32-externals" were compiled on Mac OSX 10.9.3
and should work on Intel-based Mac computers. For other Unix-based
computers, just type 'make' to build executables appropriate for your
computer. Then type 'perl collect.pl' to collect all the externals to
the "fftease32-externals" folder. Finally, move "fftease32-externals"
and "fftease32-help" to a Pd-accessible location.


Performance Considerations


The default Pd audio buffer settings for both I/O vector size and signal
vector size will work fine for FFT sizes up to around 4096 or so. For
larger FFT sizes, adjusting the Pd signal vector size and I/O vector
size upward can dramatically improve performance. With larger FFT sizes,
the reported CPU load may fluctuate. This is because a large FFT is
being performed only once for several vectors worth of samples. The
default FFT size is 1024, and the default overlap factor is 8. The
maximum FFT size is 1073741824. Let me know if you find a computer
powerful enough to compute the maximum FFT size in real-time. 


For Coders 


Full source code is included, so that intrepid coders may extend
FFTease, or even code up a 64-bit version. (Note that current 64-bit
versions of Pd compute all DSP with 32-bit resolution, thus a 64-bit
port does not yet seem advisable.) The FFTease code is distributed under
the MIT license to facilitate deployment to any combination of free,
open-source, commercial, or closed-source projects.

Have fun!

Eric Lyon
ericlyon@vt.edu
Blacksburg, Virginia
July, 2014