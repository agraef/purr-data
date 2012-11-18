This library extends the performance of miller puckette's pure data (pd).

iem_matrix is written by Thomas Musil from IEM Graz Austria
 and it is compatible to miller puckette's pd-0.37-3 to pd-0.39-2.
see also LICENCE.txt, GnuGPL.txt.

iem_matrix contains 7 objects: 
"matrix_bundle_stat~",
"matrix_diag_mul_line8~", "matrix_diag_mul_line~", "matrix_diag_mul_stat~",
"matrix_mul_line8~", "matrix_mul_line~" and "matrix_mul_stat~".

matrix_mul_*~ multiplies a message matrix with a signal array.
matrix_diag_mul_*~ multiplies a message diagonal matrix with a signal array.
matrix_bundle_*~ is a kind of additive signal multiplexer.

The extension *_line~ means, each matrix message element will be interpolated
 from the previous to the current vallue by a signal ramp (like line~ does).
The extension *_line8~ means, each matrix message element will be interpolated
 everey 8 samples from the previous to the current vallue by a signal ramp
 (this ramp has little steps, only every 8 sample, the value change).
The extension *_stat~ means, each new matrix message element change the
 signal matrix multiplicator, there is no interpolation (like sig~ does).