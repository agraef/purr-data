/* Copyright (c) 2003-2004 Tim Blechmann.                                       */
/* For information on usage and redistribution, and for a DISCLAIMER OF ALL     */
/* WARRANTIES, see the file, "COPYING"  in this distribution.                   */
/*                                                                              */
/*                                                                              */
/* tbext is the collection of some external i wrote.                            */
/* some are useful, others aren't...                                            */
/*                                                                              */
/*                                                                              */
/* tbext uses the flext C++ layer for Max/MSP and PD externals.                 */
/* get it at http://www.parasitaere-kapazitaeten.de/PD/ext                      */
/* thanks to Thomas Grill                                                       */
/*                                                                              */
/*                                                                              */
/*                                                                              */
/* This program is free software; you can redistribute it and/or                */
/* modify it under the terms of the GNU General Public License                  */
/* as published by the Free Software Foundation; either version 2               */
/* of the License, or (at your option) any later version.                       */
/*                                                                              */
/* See file LICENSE for further informations on licensing terms.                */
/*                                                                              */
/* This program is distributed in the hope that it will be useful,              */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/* GNU General Public License for more details.                                 */
/*                                                                              */
/* You should have received a copy of the GNU General Public License            */
/* along with this program; if not, write to the Free Software                  */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.  */
/*                                                                              */
/* Based on PureData by Miller Puckette and others.                             */
/*                                                                              */
/*                                                                              */
/*                                                                              */
/* coded while listening to: Hamid Drake & Assif Tsahar: Soul Bodies, Vol. 1    */
/*                           I.S.O.: I.S.O                                      */
/*                                                                              */



#include <flext.h>
#define TBEXT_VERSION "0.05"

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error upgrade your flext version!!!!!!
#endif

void ttbext_setup()
{
  post("\nTBEXT: by tim blechmann");
  post("version "TBEXT_VERSION);
  post("compiled on "__DATE__);
  post("contains: tbroute(~), tbfft1~, tbfft2~, bufline~, fftgrrev~");
  post("          fftgrsort~, fftgrshuf~, him~, sym2num\n");

  FLEXT_SETUP(tbroute);
  FLEXT_DSP_SETUP(tbsroute);
  /* obsolete: FLEXT_DSP_SETUP(tbsig); */
  /* obsolete: FLEXT_DSP_SETUP(tbpow); */
  FLEXT_DSP_SETUP(tbfft1);
  FLEXT_DSP_SETUP(tbfft2);
  FLEXT_DSP_SETUP(fftbuf);
  FLEXT_DSP_SETUP(fftgrsort);
  FLEXT_DSP_SETUP(fftgrshuf);
  FLEXT_DSP_SETUP(fftgrrev);
  FLEXT_DSP_SETUP(him);
  FLEXT_SETUP(sym2num);



}

FLEXT_LIB_SETUP(tbext,ttbext_setup)
