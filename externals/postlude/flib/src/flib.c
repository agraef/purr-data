/* flib - PD library for feature extraction 
Copyright (C) 2005  Jamie Bullock

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "flib.h"

typedef struct flib 
{
	t_object t_ob;
} t_flib;

t_class *flib_class;

void *flib_new(void)
{
    t_flib *x = (t_flib *)pd_new(flib_class);
    return (void *)x;
}


void flib_setup(void) 
{
	int i;

	char *ext[] = {"sc~\t\tSpectral Centroid", "ss~\t\tSpectral Smoothness", "irreg~\t\tSpectral Irregularity (methods 1 and 2)", "mspec~\t\tMagnitude Spectrum", "peak~\t\tAmplitude and Frequency of Spectral Peaks", "pspec~\t\tPhase Spectrum", "sfm~\t\tSpectral Flatness Measure", "trist~\t\tTristimulus (x,y,z)", "++~\t\tSum of the samples in each block", "bmax~\t\tThe maximum value and location(s) each block", "melf~\t\tGenerate a mel spaced filter for fft", "clean~\t\tRemoves NaN, inf and -inf from a signal vector", "wdv~\t\tCalculate a wavelet dispersion vector (requires creb)", "hca~\t\tHarmonic component analysis", "cc~\t\tCross correlation"};
	sc_tilde_setup();
	ss_tilde_setup();
	pp_tilde_setup();
	bmax_tilde_setup();
	irreg_tilde_setup();
	mspec_tilde_setup();
	peak_tilde_setup();
	pspec_tilde_setup();
	sfm_tilde_setup();
	trist_tilde_setup();
	melf_tilde_setup();
	clean_tilde_setup();
	wdv_tilde_setup();
	hca_tilde_setup();
	cc_tilde_setup();
   
    post("\n\tflib "VERSION" Feature Extraction Library\n\tby Jamie Bullock\n");

	for(i = 0; i < 15; i++)
		post("\t%s",ext[i]);
		
  flib_class = class_new(gensym("flib"), flib_new, 0, sizeof(t_flib), 0, 0);
}
