
/* sc4pd
   library initialization

   Copyright (c) 2004 Tim Blechmann.               

   This code is derived from:
	SuperCollider real time audio synthesis system
    Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com


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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

   Based on:
     PureData by Miller Puckette and others.
         http://www.crca.ucsd.edu/~msp/software.html
     FLEXT by Thomas Grill
         http://www.parasitaere-kapazitaeten.net/ext
     SuperCollider by James McCartney
         http://www.audiosynth.com
     
   Coded while listening to: Phosphor
*/

#include "sc4pd.hpp"

#define SC4PD_VERSION "0.01"


void sc4pd_library_setup()
{
    post("\nsc4pd: by tim blechmann");
    post("based on SuperCollider by James McCartney");
    post("version "SC4PD_VERSION);
    post("compiled on "__DATE__);
    post("contains: Dust(~), MantissaMask(~), Hasher(~), Median(~), "
	 "BrownNoise(~),\n"
	 "          ClipNoise(~), GrayNoise(~), Dust2(~), WhiteNoise(~), "
	 "PinkNoise(~), \n          Crackle(~), Rand(~), TRand(~), "
	 "TExpRand(~), IRand(~), TIRand(~),\n          CoinGate, "
	 "LinRand(~), NRand(~), ExpRand(~), LFClipNoise(~),\n"
	 "          LFNoise0(~), LFNoise1(~), LFNoise2(~), Logistic(~), "
	 "Latoocarfian(~),\n"
	 "          LinCong(~), amclip(~), scaleneg(~), excess(~), hypot(~), "
	 "ring1(~),\n"
	 "          ring2(~), ring3(~), ring4(~), difsqr(~), sumsqr(~), "
	 "sqrdif(~),\n"
	 "          sqrsum(~), absdif(~), LFSaw(~), LFPulse(~), Impulse(~),\n"
	 "          Integrator(~), Decay~, Decay2~, Lag~, Lag2~, LinExp(~), "
	 "DelayN~,\n"
	 "          DelayL~, DelayC~, CombN~, CombL~, CombC~, AllpassN~, "
	 "AllpassL~,\n"
	 "          AllpassC~, PitchShift~, Resonz~, OnePole(~), OneZero(~), "
	 "TwoPole~, \n"
	 "          TwoZero~, FOS(~), SOS~, RLPF~, RHPF~, LPF~, HPF~, BPF~, "
	 "BRF~,\n"
	 "          LPZ1(~), HPZ1(~), LPZ2(~), HPZ2(~), BPZ2(~), BRZ2(~), "
	 "LFDNoise0~,\n" 
	 "          LFDNoise1~, LFDNoise2~, sc+~, sc-~, sc*~, sc/~, "
	 "Convolution~\n"
	 );

    //initialize objects
    FLEXT_DSP_SETUP(Dust_ar);
    FLEXT_SETUP(Dust_kr);

    FLEXT_DSP_SETUP(MantissaMask_ar);
    FLEXT_SETUP(MantissaMask_kr);

    FLEXT_DSP_SETUP(Hasher_ar);
    FLEXT_SETUP(Hasher_kr);
    
    FLEXT_DSP_SETUP(Median_ar);
    FLEXT_SETUP(Median_kr);

    FLEXT_DSP_SETUP(BrownNoise_ar);
    FLEXT_SETUP(BrownNoise_kr);

    FLEXT_DSP_SETUP(ClipNoise_ar);
    FLEXT_SETUP(ClipNoise_kr);

    FLEXT_DSP_SETUP(GrayNoise_ar);
    FLEXT_SETUP(GrayNoise_kr);

    FLEXT_DSP_SETUP(WhiteNoise_ar);
    FLEXT_SETUP(WhiteNoise_kr);

    FLEXT_DSP_SETUP(PinkNoise_ar);
    FLEXT_SETUP(PinkNoise_kr);

    FLEXT_DSP_SETUP(Dust2_ar);
    FLEXT_SETUP(Dust2_kr);

    FLEXT_DSP_SETUP(Crackle_ar);
    FLEXT_SETUP(Crackle_kr);

    FLEXT_DSP_SETUP(Rand_ar);
    FLEXT_SETUP(Rand_kr);

    FLEXT_DSP_SETUP(TRand_ar);
    FLEXT_SETUP(TRand_kr);

    FLEXT_DSP_SETUP(TExpRand_ar);
    FLEXT_SETUP(TExpRand_kr);

    FLEXT_DSP_SETUP(IRand_ar);
    FLEXT_SETUP(IRand_kr);

    FLEXT_DSP_SETUP(TIRand_ar);
    FLEXT_SETUP(TIRand_kr);

    FLEXT_SETUP(CoinGate_kr);

    FLEXT_DSP_SETUP(LinRand_ar);
    FLEXT_SETUP(LinRand_kr);

    FLEXT_DSP_SETUP(NRand_ar);
    FLEXT_SETUP(NRand_kr);

    FLEXT_DSP_SETUP(ExpRand_ar);
    FLEXT_SETUP(ExpRand_kr);

    FLEXT_DSP_SETUP(LFClipNoise_ar);
    FLEXT_SETUP(LFClipNoise_kr);

    FLEXT_DSP_SETUP(LFNoise0_ar);
    FLEXT_SETUP(LFNoise0_kr);

    FLEXT_DSP_SETUP(LFNoise1_ar);
    FLEXT_SETUP(LFNoise1_kr);

    FLEXT_DSP_SETUP(LFNoise2_ar);
    FLEXT_SETUP(LFNoise2_kr);

    FLEXT_DSP_SETUP(Logistic_ar);
    FLEXT_SETUP(Logistic_kr);

    FLEXT_DSP_SETUP(Latoocarfian_ar);
    FLEXT_SETUP(Latoocarfian_kr);

    FLEXT_DSP_SETUP(LinCong_ar);
    FLEXT_SETUP(LinCong_kr);

    FLEXT_DSP_SETUP(amclip_ar);
    FLEXT_SETUP(amclip_kr);

    FLEXT_DSP_SETUP(scaleneg_ar);
    FLEXT_SETUP(scaleneg_kr);

    FLEXT_DSP_SETUP(excess_ar);
    FLEXT_SETUP(excess_kr);

    FLEXT_DSP_SETUP(hypot_ar);
    FLEXT_SETUP(hypot_kr);

    FLEXT_DSP_SETUP(ring1_ar);
    FLEXT_SETUP(ring1_kr);

    FLEXT_DSP_SETUP(ring2_ar);
    FLEXT_SETUP(ring2_kr);

    FLEXT_DSP_SETUP(ring3_ar);
    FLEXT_SETUP(ring3_kr);

    FLEXT_DSP_SETUP(ring4_ar);
    FLEXT_SETUP(ring4_kr);

    FLEXT_DSP_SETUP(difsqr_ar);
    FLEXT_SETUP(difsqr_kr);

    FLEXT_DSP_SETUP(sumsqr_ar);
    FLEXT_SETUP(sumsqr_kr);

    FLEXT_DSP_SETUP(sqrsum_ar);
    FLEXT_SETUP(sqrsum_kr);

    FLEXT_DSP_SETUP(sqrdif_ar);
    FLEXT_SETUP(sqrdif_kr);

    FLEXT_DSP_SETUP(absdif_ar);
    FLEXT_SETUP(absdif_kr);

    FLEXT_DSP_SETUP(LFSaw_ar);
    FLEXT_SETUP(LFSaw_kr);

    FLEXT_DSP_SETUP(LFPulse_ar);
    FLEXT_SETUP(LFPulse_kr);

    FLEXT_DSP_SETUP(Impulse_ar);
    FLEXT_SETUP(Impulse_kr);

    FLEXT_DSP_SETUP(Integrator_ar);
    FLEXT_SETUP(Integrator_kr);

    FLEXT_DSP_SETUP(Decay_ar); 

    FLEXT_DSP_SETUP(Decay2_ar); 

    FLEXT_DSP_SETUP(Lag_ar); 

    FLEXT_DSP_SETUP(Lag2_ar); 

    FLEXT_DSP_SETUP(Lag3_ar); 

    FLEXT_DSP_SETUP(LinExp_ar);
    FLEXT_SETUP(LinExp_kr);

    FLEXT_DSP_SETUP(DelayN_ar); 

    FLEXT_DSP_SETUP(DelayL_ar); 

    FLEXT_DSP_SETUP(DelayC_ar); 

    FLEXT_DSP_SETUP(CombN_ar); 

    FLEXT_DSP_SETUP(CombL_ar); 

    FLEXT_DSP_SETUP(CombC_ar); 

    FLEXT_DSP_SETUP(AllpassN_ar); 

    FLEXT_DSP_SETUP(AllpassL_ar); 

    FLEXT_DSP_SETUP(AllpassC_ar); 

    FLEXT_DSP_SETUP(PitchShift_ar); 

    FLEXT_DSP_SETUP(Resonz_ar); 

    FLEXT_DSP_SETUP(OnePole_ar); 
    FLEXT_SETUP(OnePole_kr); 

    FLEXT_DSP_SETUP(OneZero_ar); 
    FLEXT_SETUP(OneZero_kr); 

    FLEXT_DSP_SETUP(TwoPole_ar); 

    FLEXT_DSP_SETUP(TwoZero_ar); 

    FLEXT_DSP_SETUP(FOS_ar); 
    FLEXT_SETUP(FOS_kr); 

    FLEXT_DSP_SETUP(SOS_ar); 

    FLEXT_DSP_SETUP(RLPF_ar); 

    FLEXT_DSP_SETUP(RHPF_ar); 

    FLEXT_DSP_SETUP(LPF_ar); 

    FLEXT_DSP_SETUP(HPF_ar); 

    FLEXT_DSP_SETUP(BPF_ar); 

    FLEXT_DSP_SETUP(BRF_ar); 

    FLEXT_DSP_SETUP(LPZ1_ar); 
    FLEXT_SETUP(LPZ1_kr); 

    FLEXT_DSP_SETUP(HPZ1_ar); 
    FLEXT_SETUP(HPZ1_kr); 

    FLEXT_DSP_SETUP(LPZ2_ar); 
    FLEXT_SETUP(LPZ2_kr); 

    FLEXT_DSP_SETUP(HPZ2_ar); 
    FLEXT_SETUP(HPZ2_kr); 

    FLEXT_DSP_SETUP(BRZ2_ar); 
    FLEXT_SETUP(BRZ2_kr); 

    FLEXT_DSP_SETUP(BPZ2_ar); 
    FLEXT_SETUP(BPZ2_kr); 

    FLEXT_DSP_SETUP(LFDNoise0_ar); 

    FLEXT_DSP_SETUP(LFDNoise1_ar); 

    FLEXT_DSP_SETUP(LFDNoise2_ar); 

    FLEXT_DSP_SETUP(scadd_ar); 

    FLEXT_DSP_SETUP(scsub_ar); 

    FLEXT_DSP_SETUP(scmul_ar); 

    FLEXT_DSP_SETUP(scdiv_ar); 

    FLEXT_DSP_SETUP(Convolution_ar); 

    //init ffts
    init_ffts();
}

FLEXT_LIB_SETUP(sc4pd,sc4pd_library_setup);
