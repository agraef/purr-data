
/*
		Short example of a supercollider3 synth.
		-Kjetil S. Matheussen, 2004.

		To load this one from inside PD, call:'

		server.evalSynth("fileplayer")

*/


arg out=0,bufnum,rate=100,pan=0;

Out.ar( out,
	Pan2.ar(PlayBuf.ar(1,bufnum,rate/100,0,0,1),pan/100,0.8)
)

