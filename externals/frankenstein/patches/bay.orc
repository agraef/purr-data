; ================================================================
; Header
; ================================================================
sr=44100
kr=882
ksmps=50
nchnls=2

; ================================================================
; Globals
; ================================================================
zakinit 10,10

; Global variable for the reverb unit
ga1 init 0

; ================================================================
; Tables
; ================================================================
; Waveform for the string-pad
iwave ftgen 1, 0, 4096, 10, 1, .5, .33, .25,  .0, .1,  .1, .1

; Senoid required by chorus
isine ftgen 2, 0, 4096, 10, 1


; ================================================================
; Instruments
; ================================================================


instr 1	;String pad
; String-pad borrwoed from the piece "Dorian Gray",
; http://akozar.spymac.net/music/ Modified to fit my needs

;  ihz  = p4
;  idb  = p5/127 * 70    ; rescale MIDI velocity to 70db
;  ipos = p6
;  iamp = ampdb(idb)

; modified by dmorelli
khz	invalue	"hz1"
kpos invalue "pos1"
kamp  invalue "amp1"
kctrl = kamp*70
kctrl =	ampdb(kctrl)
  ; Slow attack and release
;  kctrl   linseg  0, p3/4, iamp, p3/2, 0  
  ; Slight chorus effect
  afund   oscil   kctrl, khz, 1            ; audio oscillator
  acel1   oscil   kctrl, khz - .1, 1       ; audio oscillator - flat
  acel2   oscil   kctrl, khz + .1, 1       ; audio oscillator - sharp
  asig    =   afund + acel1 + acel2

  ; Cut-off high frequencies depending on midi-velocity
  ; (larger velocity implies more brighter sound)
  asig butterlp asig, (p5-60)*40+900
 
  ; Panning
  kppan =       kpos*1.570796325  ; half of PI (radians of 90o angle)
  kpleft        =       cos(kppan)        ; half sign "down"
  kpright       =       sin(kppan)        ; half sign "up"
  asig1 = asig*kpleft;
  asig2 = asig*kpright;
  ; To the chorus effect, through zak channels 1 and 2
  zawm asig1, 1
  zawm asig2, 2
endin


instr 2	;String pad
; String-pad borrwoed from the piece "Dorian Gray",
; http://akozar.spymac.net/music/ Modified to fit my needs

;  ihz  = p4
;  idb  = p5/127 * 70    ; rescale MIDI velocity to 70db
;  ipos = p6
;  iamp = ampdb(idb)

; modified by dmorelli
khz	invalue	"hz2"
kpos invalue "pos2"
kamp  invalue "amp2"
kctrl = kamp*70
kctrl =	ampdb(kctrl)
  ; Slow attack and release
;  kctrl   linseg  0, p3/4, iamp, p3/2, 0  
  ; Slight chorus effect
  afund   oscil   kctrl, khz, 1            ; audio oscillator
  acel1   oscil   kctrl, khz - .1, 1       ; audio oscillator - flat
  acel2   oscil   kctrl, khz + .1, 1       ; audio oscillator - sharp
  asig    =   afund + acel1 + acel2

  ; Cut-off high frequencies depending on midi-velocity
  ; (larger velocity implies more brighter sound)
  asig butterlp asig, (p5-60)*40+900
 
  ; Panning
  kppan =       kpos*1.570796325  ; half of PI (radians of 90o angle)
  kpleft        =       cos(kppan)        ; half sign "down"
  kpright       =       sin(kppan)        ; half sign "up"
  asig1 = asig*kpleft;
  asig2 = asig*kpright;
  ; To the chorus effect, through zak channels 1 and 2
  zawm asig1, 1
  zawm asig2, 2
endin

instr 3	;String pad
; String-pad borrwoed from the piece "Dorian Gray",
; http://akozar.spymac.net/music/ Modified to fit my needs

 ; ihz  = p4
;  idb  = p5/127 * 70    ; rescale MIDI velocity to 70db
;  ipos = p6
;  iamp = ampdb(idb)

; modified by dmorelli
khz	invalue	"hz3"
kpos invalue "pos3"
kamp  invalue "amp3"
kctrl = kamp*70
kctrl =	ampdb(kctrl)
  ; Slow attack and release
;  kctrl   linseg  0, p3/4, iamp, p3/2, 0  
  ; Slight chorus effect
  afund   oscil   kctrl, khz, 1            ; audio oscillator
  acel1   oscil   kctrl, khz - .1, 1       ; audio oscillator - flat
  acel2   oscil   kctrl, khz + .1, 1       ; audio oscillator - sharp
  asig    =   afund + acel1 + acel2

  ; Cut-off high frequencies depending on midi-velocity
  ; (larger velocity implies more brighter sound)
  asig butterlp asig, (p5-60)*40+900
 
  ; Panning
  kppan =       kpos*1.570796325  ; half of PI (radians of 90o angle)
  kpleft        =       cos(kppan)        ; half sign "down"
  kpright       =       sin(kppan)        ; half sign "up"
  asig1 = asig*kpleft;
  asig2 = asig*kpright;
  ; To the chorus effect, through zak channels 1 and 2
  zawm asig1, 1
  zawm asig2, 2
endin
instr 4	;String pad
; String-pad borrwoed from the piece "Dorian Gray",
; http://akozar.spymac.net/music/ Modified to fit my needs

;  ihz  = p4
;  idb  = p5/127 * 70    ; rescale MIDI velocity to 70db
;  ipos = p6
;  iamp = ampdb(idb)

; modified by dmorelli
khz	invalue	"hz4"
kpos invalue "pos4"
kamp  invalue "amp4"
kctrl = kamp*70
kctrl =	ampdb(kctrl)
  ; Slow attack and release
;  kctrl   linseg  0, p3/4, iamp, p3/2, 0  
  ; Slight chorus effect
  afund   oscil   kctrl, khz, 1            ; audio oscillator
  acel1   oscil   kctrl, khz - .1, 1       ; audio oscillator - flat
  acel2   oscil   kctrl, khz + .1, 1       ; audio oscillator - sharp
  asig    =   afund + acel1 + acel2

  ; Cut-off high frequencies depending on midi-velocity
  ; (larger velocity implies more brighter sound)
  asig butterlp asig, (p5-60)*40+900
 
  ; Panning
  kppan =       kpos*1.570796325  ; half of PI (radians of 90o angle)
  kpleft        =       cos(kppan)        ; half sign "down"
  kpright       =       sin(kppan)        ; half sign "up"
  asig1 = asig*kpleft;
  asig2 = asig*kpright;
  ; To the chorus effect, through zak channels 1 and 2
  zawm asig1, 1
  zawm asig2, 2
endin





; strumento senza invalue

instr 9	;String pad
; String-pad borrwoed from the piece "Dorian Gray",
; http://akozar.spymac.net/music/ Modified to fit my needs

  ihz  = p4
  idb  = p5/127 * 70    ; rescale MIDI velocity to 70db
  ipos = p6
  iamp = ampdb(idb)

; modified by dmorelli

;kpos invalue "pos1"
;kamp  invalue "amp1"
;kctrl = kamp*70
;kctrl =	ampdb(kctrl)
  ; Slow attack and release
  kctrl   linseg  0, p3/4, iamp, p3/2, 0  
  ; Slight chorus effect
  afund   oscil   kctrl, ihz, 1            ; audio oscillator
  acel1   oscil   kctrl, ihz - .1, 1       ; audio oscillator - flat
  acel2   oscil   kctrl, ihz + .1, 1       ; audio oscillator - sharp
  asig    =   afund + acel1 + acel2

  ; Cut-off high frequencies depending on midi-velocity
  ; (larger velocity implies more brighter sound)
  asig butterlp asig, (p5-60)*40+900
 
  ; Panning
  ippan =       ipos*1.570796325  ; half of PI (radians of 90o angle)
  ipleft        =       cos(ippan)        ; half sign "down"
  ipright       =       sin(ippan)        ; half sign "up"
  asig1 = asig*ipleft;
  asig2 = asig*ipright;
  ; To the chorus effect, through zak channels 1 and 2
  zawm asig1, 1
  zawm asig2, 2
endin

; ================================================================
; EFFECTS
; ================================================================


; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
; Chorus effect, borrowed from http://www.jlpublishing.com/Csound.htm
; I made some of its parameters accesible trhough score
instr 10	;Chorus
  ; Read input from zak
  a1     zar     1
  a2     zar     2
  idlyml=p4      ;delay in milliseconds
  k1             oscili          idlyml/p5, 1, 2
  ar1l           vdelay3 a1, idlyml/5+k1, 900    ;delayed sound 1
  ar1r           vdelay3 a2, idlyml/5+k1, 900    ;delayed sound 1
  k2             oscili          idlyml/p5, .995, 2
  ar2l           vdelay3 a1, idlyml/5+k2, 700    ;delayed sound 2
  ar2r           vdelay3 a2, idlyml/5+k2, 700    ;delayed sound 2
  k3             oscili          idlyml/p5, 1.05, 2
  ar3l           vdelay3 a1, idlyml/5+k3, 700    ;delayed sound 3
  ar3r           vdelay3 a2, idlyml/5+k3, 700    ;delayed sound 3
  k4             oscili          idlyml/p5, 1, 2
  ar4l           vdelay3 a1, idlyml/5+k4, 900    ;delayed sound 4
  ar4r           vdelay3 a2, idlyml/5+k4, 900    ;delayed sound 4
  aoutl          =               (a1+ar1l+ar2l+ar3l+ar4l)*.5
  aoutr          =               (a2+ar1r+ar2r+ar3r+ar4r)*.5

  ; To the output mixer
  zawm            aoutl, 5
  zawm            aoutr, 6
  ; and also to the reverb unit
  ga1 = ga1 + (aoutl+aoutr)*.5
endin

; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
; Reverb
; 8 delay line FDN reverb, with feedback matrix based upon 
; physical modeling scattering junction of 8 lossless waveguides
; of equal characteristic impedance. Based on Julius O. Smith III, 
; "A New Approach to Digital Reverberation using Closed Waveguide
; Networks," Proceedings of the International Computer Music 
; Conference 1985, p. 47-53 (also available as a seperate
; publication from CCRMA), as well as some more recent papers by
; Smith and others.
;
; Coded by Sean Costello, October 1999
instr 25	;Reverb
  ; Note: ga1 is the global input to the reverb.
  afilt1 init 0
  afilt2 init 0
  afilt3 init 0
  afilt4 init 0
  afilt5 init 0
  afilt6 init 0
  afilt7 init 0
  afilt8 init 0
  idel1 = (2473.000/sr)
  idel2 = (2767.000/sr)
  idel3 = (3217.000/sr)
  idel4 = (3557.000/sr)
  idel5 = (3907.000/sr)
  idel6 = (4127.000/sr)
  idel7 = (2143.000/sr)
  idel8 = (1933.000/sr)
  
  
  igain = p4      ; gain of reverb. Adjust empirically
                  ; for desired reverb time. .6 gives
                  ; a good small "live" room sound, .8
                  ; a small hall, .9 a large hall,
                  ; .99 an enormous stone cavern.
  
  ipitchmod = p5  ; amount of random pitch modulation
                  ; for the delay lines. 1 is the "normal"
                  ; amount, but this may be too high for
                  ; held pitches such as piano tones.
                  ; Adjust to taste.
  
  itone = p6      ; Cutoff frequency of lowpass filters
                  ; in feedback loops of delay lines,
                  ; in Hz. Lower cutoff frequencies results
                  ; in a sound with more high-frequency
                  ; damping.
  
  ; k1-k8 are used to add random pitch modulation to the
  ; delay lines. Helps eliminate metallic overtones
  ; in the reverb sound.
  k1      randi   .001, 3.1, .06
  k2      randi   .0011, 3.5, .9
  k3      randi   .0017, 1.11, .7
  k4      randi   .0006, 3.973, .3
  k5      randi   .001, 2.341, .63
  k6      randi   .0011, 1.897, .7
  k7      randi   .0017, 0.891, .9
  k8      randi   .0006, 3.221, .44
  ; apj is used to calculate "resultant junction pressure" for 
  ; the scattering junction of 8 lossless waveguides
  ; of equal characteristic impedance. If you wish to
  ; add more delay lines, simply add them to the following 
  ; equation, and replace the .25 by 2/N, where N is the 
  ; number of delay lines.
  apj = .25 * (afilt1 + afilt2 + afilt3 + afilt4 + afilt5 + afilt6 + afilt7 + afilt8)
  
  
  adum1   delayr  1
  adel1   deltapi idel1 + k1 * ipitchmod
          delayw  ga1 + apj - afilt1
  
  adum2   delayr  1
  adel2   deltapi idel2 + k2 * ipitchmod
          delayw  ga1 + apj - afilt2
  
  adum3   delayr  1
  adel3   deltapi idel3 + k3 * ipitchmod
          delayw  ga1 + apj - afilt3
  
  adum4   delayr  1
  adel4   deltapi idel4 + k4 * ipitchmod
          delayw  ga1 + apj - afilt4
  
  adum5   delayr  1
  adel5   deltapi idel5 + k5 * ipitchmod
          delayw  ga1 + apj - afilt5
  
  adum6   delayr  1
  adel6   deltapi idel6 + k6 * ipitchmod
          delayw  ga1 + apj - afilt6
  
  adum7   delayr  1
  adel7   deltapi idel7 + k7 * ipitchmod
          delayw  ga1 + apj - afilt7
  
  adum8   delayr  1
  adel8   deltapi idel8 + k8 * ipitchmod
          delayw  ga1 + apj - afilt8
  
  ; 1st order lowpass filters in feedback
  ; loops of delay lines.
  afilt1  tone    adel1 * igain, itone
  afilt2  tone    adel2 * igain, itone
  afilt3  tone    adel3 * igain, itone
  afilt4  tone    adel4 * igain, itone
  afilt5  tone    adel5 * igain, itone
  afilt6  tone    adel6 * igain, itone
  afilt7  tone    adel7 * igain, itone
  afilt8  tone    adel8 * igain, itone
  
  ; The outputs of the delay lines are summed
  ; and sent to the stereo outputs. This could
  ; easily be modified for a 4 or 8-channel 
  ; sound system.
  aout1 = (afilt1 + afilt3 + afilt5 + afilt7)
  aout2 = (afilt2 + afilt4 + afilt6 + afilt8)
  ;outs    aout1, aout2
  ; To the output mixer
  zawm aout1, 5
  zawm aout2, 6
  ga1 = 0
endin


; ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
; Output mixer
; It applies a bass enhancement, compression and fadeout
; to the whole piece,
instr 30	;Mixer
  ; Read input from zak
  a1  zar 5
  a2  zar 6
  ; Bass enhancement
  al1 butterlp a1, 100
  al2 butterlp a2, 100
  a1 = al1*1.5 +a1
  a2 = al2*1.5 +a2 

  ; Global amplitude shape
  ; It applies a gain of p4 to the whole piece, and creates a
  ; fadeout the last p5 seconds
  kenv   linseg  p4, p3-p5, p4, p5, 0 
  a1=a1*kenv
  a2=a2*kenv 
  
  ; Compression
  a1 dam a1, 5000, 0.5, 1, 0.2, 0.1  
  a2 dam a2, 5000, 0.5, 1, 0.2, 0.1  
  
  outs a1, a2
  zacl 0, 10
endin

