/*
disis.munger~ 1.3.3
a realtime multichannel granulator

a flext (cross-platform PD & Max/MSP) port of
the munger~ object from the PeRColate library (0.9 beta6)
http://www.music.columbia.edu/PeRColate/

Original PeRColate library by:

Dan Trueman http://www.music.princeton.edu/~dan/
R. Luke DuBois's http://www.lukedubois.com/

Flext port and additions by:
Ivica Ico Bukvic http://ico.bukvic.net
Ji-Sun Kim hideaway@vt.edu
http://disis.music.vt.edu

Released under GPL license
(whichever is the latest version--as of this release, version 2)
For more info on the GPL license please visit:
http://www.gnu.org/copyleft/gpl.html

For latest changes please see changelog
*/

//flext new/delete overload
//#define FLEXT_USE_CMEM

#include <ADSR.h>
#include <flext.h>
#include <math.h>
#include <time.h>
#include <stdio.h>

//Unnecessary to include these two header file for this version
//#include <stdlib.h>
//#include <flstk.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 400)
#error You need at least flext version 0.4.0 with STK support
#endif

//version
#define MUNGER_MAJOR 1
#define MUNGER_MINOR 3
#define MUNGER_REV 3

//MSVC doesn't know RANDOM(), while GCC's (at least on Linux) has rand() limit much higher
#ifndef __GNUC__
#define RANDOM() (rand())
#else
#define RANDOM() (random()%32768)
#endif

#define ONE_OVER_HALFRAND 0.00006103516							// constant = 1. / 16384.0
#define ONE_OVER_MAXRAND 0.000030517578							// 1 / 32768
#define MINSPEED .001											//minimum speed through buffer on playback
#define ENVSIZE 32
#define ONE_OVER_ENVSIZE .0078125
#define MINSIZE 64												// twice ENVSIZE. minimum grainsize in samples
#define RAND01 (((long)RANDOM() * ONE_OVER_MAXRAND)				//RANDOM() numbers 0-1
#define RAND11 (((long)RANDOM() - 16384.) * ONE_OVER_HALFRAND)	//RANDOM() numbers -1 to 1
#define WINLENGTH 1024
#define PITCHTABLESIZE 1000										//max number of transpositions for the "scale" message
#define RECORDRAMP 1000
#define RECORDRAMP_INV 0.001

//these are arbitrary--can we trust users not to do something silly and remove these?
#define MAXCHANNELS 64
#define MAXVOICES 1000

//useful define
#ifndef TWOPI
#define TWOPI 6.28318530717958647692
#endif

class disis_munger
	:public flext_dsp
{
//obligatory flext header (class name, base class name)
//featuring setup function to initialize some data
	FLEXT_HEADER_S(disis_munger, flext_dsp, setup)

public:
	disis_munger(int argc, const t_atom *argv);
//memory allocation/free for recordbuf.
	void	munger_alloc();
	void	munger_free();

//window funcs
	void	munger_setramp(short argc, t_atom *argv);
	void	munger_sethanning( short argc, t_atom *argv);
//scale funcs
	void	munger_scale(short argc, t_atom *argv);
	void	munger_tempered(short argc, t_atom *argv);
	void	munger_smooth(short argc, t_atom *argv);

//multichannel func
	void	munger_spat(short argc, t_atom *argv);
	void	munger_discretepan(short argc, t_atom *argv);

//note funcs
	float	newNote(int whichVoice, int newNote);			//creates a new start position for a new note (oneshot grain)
	float	newNoteSize(int whichVoice, int newNote);		//creates a size for a new note

//buffersize change
	void	munger_bufsize(short argc, t_atom *argv);
//buffersize change (ms)
	void	munger_bufsize_ms(short argc, t_atom *argv);

//set maximum number of voices possible
	void	munger_maxvoices(short argc, t_atom *argv);
//set number of voices to actually use
	void	munger_setvoices(short argc, t_atom *argv);
//set min grain size
	void	munger_setminsize(short argc, t_atom *argv);
//turn on/off backwards grains
	void	munger_ambidirectional(short argc, t_atom *argv);
//turn on/off recording
	void	munger_record(short argc, t_atom *argv);
//clear buffer
	void	munger_clear(short argc, t_atom *argv);

//set overall gain and rand gain range
	void	munger_gain(short argc, t_atom *argv);
	void	munger_randgain(short argc, t_atom *argv);

//fix position for start of grain playback
	void	munger_setposition( short argc, t_atom *argv);

//post current parameter values
	void	munger_poststate(short argc, t_atom *argv);
	void	setpower( short argc, t_atom *argv);

//grain funcs
	int		findVoice();
	float	newSetup(int whichVoice);
	float	newSize(int whichOne);
	int		newDirection();
	float	envelope(int whichone, float sample);

//sample buffer funcs
	void	recordSamp(float sample);
	float	getSamp(double where);

	void	munger_float(double f);
	void	setverbose(short argc, t_atom *argv);

//note funcs
	void	munger_note(short argc, t_atom *argv);
	void	munger_oneshot(short argc, t_atom *argv);

//external buffer funcs
	void	munger_setbuffer(short argc, t_atom *argv);
	void	munger_clearbuffer();
	bool	munger_checkbuffer(bool reset = false);
	float	getExternalSamp(double where);

protected:
	virtual bool CbDsp();
	virtual void CbSignal();
	void	m_float1(float v) { grate = v; tempgrate = v; grate_connected += 1;}
	void	m_float2(float v) { grate_var = v; grate_var_connected += 1;}
	void	m_float3(float v) { glen = v; glen_connected  += 1;}
	void	m_float4(float v) { glen_var = v; glen_var_connected += 1;}
	void	m_float5(float v) { gpitch = v; gpitch_connected += 1;}
	void	m_float6(float v) { gpitch_var = v; gpitch_var_connected += 1;}
	void	m_float7(float v) { gpan_spread = v; gpan_spread_connected += 1;}

	float	maxdelay;

//user controlled vars
	float	grate;							//grain rate
	float	tempgrate;						//grain rate assessed using variation
	float	grate_var;						//grain rate variation; percentage of grain rate
	float	glen;							//grain length
	float	glen_var;
	float	gpitch;
	float	gpitch_var;
	float	gpan_spread;					//how much to spread the grains around center

	float	pitchTable[PITCHTABLESIZE];		//table of pitch values to draw from
	float	twelfth;						//1/12
	float	semitone;
	short	smoothPitch;
	int		scale_len;

	float	gain, randgain;
	float	position;						//playback position (0-1) (if ==-1, then RANDOM(), which is default)

	int		buflen;
	float	maxsize, minsize;
	float	twothirdBufsize, onethirdBufsize;
	float	initbuflen;
	long	maxvoices;

	char	*munger_name;
	int		verbose;
	int		graincounter;
	int		countsamples;

//signals connected? or controls...
	short	grate_connected;
	short	grate_var_connected;
	short	glen_connected;
	short	glen_var_connected;
	short	gpitch_connected;
	short	gpitch_var_connected;
	short	gpan_spread_connected;

//window stuff
	short	doHanning;
	float	*winTime, *winRate;
	float	winTable[WINLENGTH];
	float	rampLength;				//for simple linear ramp

//voice parameters
	long	*gvoiceSize;			//sample size
	double	*gvoiceSpeed;			//1 = at pitch
	double	*gvoiceCurrent;			//current sample position
	int		*gvoiceDirection;		//1 = forward, -1 backwards
	int		*gvoiceOn;				//currently playing? boolean
	long	*gvoiceDone;			//how many samples already played from grain
	float	*gvoiceLPan;
	float	*gvoiceRPan;
	float	*gvoiceRamp;
	float	*gvoiceOneOverRamp;
	float	*gvoiceGain;
	int		voices;
	float	gimme;

	//ADSR	gvoiceADSR[MAXVOICES];	//MSVC+flext currently not happy with dynamic allocation
	stk::ADSR	*gvoiceADSR;
	int		*gvoiceADSRon; 			//set this to 1 if ADSR is desired instead of symmetrical ramp envelope
	float	*channelGain;
	float	*channelGainSpread;
	int		discretepan;

	float	**gvoiceSpat;
	float	**notechannelGain;
	float	**notechannelGainSpread;

//sample buffer
	float	*recordBuf;
	int		recordOn;			//boolean
	int		recordRampVal;		//ramp for when toggling record on and off
	int		rec_ramping;		//-1 when ramping down, 1 when ramping up, 0 when not ramping. who's a ramp?
	long	recordCurrent;

//other stuff
	long	time;
	int		power;
	short	ambi;
	int		num_channels;
	int		numvoices;

	float	srate, one_over_srate;
	float	srate_ms, one_over_srate_ms;

//external record buffer vars
	t_symbol *bufname;
	buffer	*l_buf;
	long	l_chan;
	short	externalBuffer;

//note and oneshot stuff
	short	oneshot;
	int		newnote;
	float	*noteTransp, *noteSize, *notePan, *noteGain;
	float	*noteAttack, *noteDecay, *noteSustain, *noteRelease;
	int		*noteDirection;

//sample handling
	float **out;
	float *outsamp;

//destructor
	~disis_munger();

private:
	FLEXT_CALLBACK_V(munger_setvoices);
	FLEXT_CALLBACK_V(munger_maxvoices);
	FLEXT_CALLBACK_V(munger_setramp);
	FLEXT_CALLBACK_V(munger_scale);
	FLEXT_CALLBACK_V(munger_bufsize);
	FLEXT_CALLBACK_V(munger_bufsize_ms);
	FLEXT_CALLBACK_V(munger_setminsize);
	FLEXT_CALLBACK_V(setpower);
	FLEXT_CALLBACK_V(munger_record);
	FLEXT_CALLBACK_V(munger_ambidirectional);
	FLEXT_CALLBACK_V(munger_smooth);
	FLEXT_CALLBACK_V(munger_tempered);
	FLEXT_CALLBACK_V(munger_sethanning);
	FLEXT_CALLBACK_V(munger_randgain);
	FLEXT_CALLBACK_V(munger_setposition);
	FLEXT_CALLBACK_V(munger_gain);
	FLEXT_CALLBACK_V(munger_clear);
	FLEXT_CALLBACK_V(munger_poststate);
	FLEXT_CALLBACK_V(setverbose);
	FLEXT_CALLBACK_V(munger_setbuffer);
	FLEXT_CALLBACK_V(munger_spat);
	FLEXT_CALLBACK_V(munger_discretepan);
	FLEXT_CALLBACK_V(munger_note);
	FLEXT_CALLBACK_V(munger_oneshot);

	FLEXT_CALLBACK_F(m_float1);
	FLEXT_CALLBACK_F(m_float2);
	FLEXT_CALLBACK_F(m_float3);
	FLEXT_CALLBACK_F(m_float4);
	FLEXT_CALLBACK_F(m_float5);
	FLEXT_CALLBACK_F(m_float6);
	FLEXT_CALLBACK_F(m_float7);

	static void setup (t_classid c)
	{
		FLEXT_CADDMETHOD_(c, 0, "verbose",setverbose);
		FLEXT_CADDMETHOD_(c, 0, "ramptime", munger_setramp);
		FLEXT_CADDMETHOD_(c, 0, "scale",munger_scale);
		FLEXT_CADDMETHOD_(c, 0, "delaylength",munger_bufsize);
		FLEXT_CADDMETHOD_(c, 0, "delaylength_ms",munger_bufsize_ms);
		FLEXT_CADDMETHOD_(c, 0, "minsize",munger_setminsize);
		FLEXT_CADDMETHOD_(c, 0, "power",setpower);
		FLEXT_CADDMETHOD_(c, 0, "record",munger_record);
		FLEXT_CADDMETHOD_(c, 0, "ambidirectional",munger_ambidirectional);
		FLEXT_CADDMETHOD_(c, 0, "smooth",munger_smooth);
		FLEXT_CADDMETHOD_(c, 0, "tempered",munger_tempered);
		FLEXT_CADDMETHOD_(c, 0, "hanning",munger_sethanning);
		FLEXT_CADDMETHOD_(c, 0, "rand_gain",munger_randgain);
		FLEXT_CADDMETHOD_(c, 0, "position",munger_setposition);
		FLEXT_CADDMETHOD_(c, 0, "gain",munger_gain);
		FLEXT_CADDMETHOD_(c, 0, "voices",munger_setvoices);
		FLEXT_CADDMETHOD_(c, 0, "maxvoices",munger_maxvoices);
		FLEXT_CADDMETHOD_(c, 0, "clear",munger_clear);
		FLEXT_CADDMETHOD_(c, 0, "state",munger_poststate);
		FLEXT_CADDMETHOD_(c, 0, "buffer",munger_setbuffer);
		FLEXT_CADDMETHOD_(c, 0, "spatialize",munger_spat);
		FLEXT_CADDMETHOD_(c, 0, "note",munger_note);
		FLEXT_CADDMETHOD_(c, 0, "oneshot",munger_oneshot);
		FLEXT_CADDMETHOD_(c, 0, "discretepan", munger_discretepan);

		FLEXT_CADDMETHOD(c, 1, m_float1);
		FLEXT_CADDMETHOD(c, 2, m_float2);
		FLEXT_CADDMETHOD(c, 3, m_float3);
		FLEXT_CADDMETHOD(c, 4, m_float4);
		FLEXT_CADDMETHOD(c, 5, m_float5);
		FLEXT_CADDMETHOD(c, 6, m_float6);
		FLEXT_CADDMETHOD(c, 7, m_float7);
	}
};

FLEXT_NEW_DSP_V("disis_munger~", disis_munger) //with parameter

disis_munger::disis_munger(int argc, const t_atom *argv):maxdelay(3000.), num_channels(2), numvoices(50), munger_name(NULL) //argc: argument #, argv:  argument value
{
	float arg1;
	int i,j,n;
	int arg2, arg3;
	char tmp[32];
	const char *c;

	post("disis_munger~ version %d.%d.%d", (int)MUNGER_MAJOR, (int)MUNGER_MINOR, (int)MUNGER_REV);

	if(argc)
	{
		arg1 = GetAFloat(argv[0]);
		if (arg1 >= 100.) maxdelay = arg1;
		if(argc > 1)
		{
			arg2 = GetAInt(argv[1]);
			if (arg2 < 2) arg2 = 2;
			if (arg2 > MAXCHANNELS) arg2 = MAXCHANNELS;
			num_channels = arg2;
			post ("disis_munger~: number channels = %d", num_channels);
		}

		if (argc > 2)
		{
			arg3 = GetAInt(argv[2]);
			if (arg3)
			{
				if (arg3 > 1000) arg3 = 1000; //let us impose upper limit of 1000
				numvoices = arg3;
				post ("disis_munger~: maximum possible number of voices = %d", numvoices);
			}
		}
		if (argc > 3)
		{
			c = GetAString(argv[3]);
			if (c[0] != '_')
			{
				munger_name = new char[sizeof (argv[3])+1];
				GetAString(argv[3], munger_name, sizeof (argv[3])+1);
			}
		}
	}

	srate = Samplerate(); //current sample rate, flext function
	one_over_srate = 1./srate;
	srate_ms = srate/1000.;
	one_over_srate_ms = 1./srate_ms;
	initbuflen = (float)(maxdelay + 50.)* srate_ms;
	buflen = (int)initbuflen;
	maxsize = buflen / 3;
	twothirdBufsize = maxsize * 2;
	onethirdBufsize = maxsize;
	minsize = MINSIZE;

	munger_alloc();

	verbose = 1;
	post("disis_munger~ %s: maxdelay = %d milliseconds", munger_name, (long)maxdelay);

	AddInSignal("Signal input");						//e.g. gated signal.
	AddInFloat("grain separation (ms)");				//float: grain separation
	AddInFloat("grain separation variation (ms)");		//float: grain rate variation
	AddInFloat("grain size (ms)");						//float: grain size
	AddInFloat("grain size variation (ms)");			//float: grain size variation
	AddInFloat("grain pitch (ints=harm. overtones)");	//float: grain pitch
	AddInFloat("grain pitch variation (0-1)");			//float: grain pitch variation
	AddInFloat("stereo spread (0-1)");					//float: stereo spread

	for( n = 0; n < num_channels; n++)
	{
		sprintf(tmp, "Output signal channel : %d", n);
		AddOutSignal(tmp); //dry signal
	}
	grate_connected = 0;
	grate_var_connected = 0;
	glen_connected = 0;
	glen_var_connected = 0;
	gpitch_connected = 0;
	gpitch_var_connected = 0;
	gpan_spread_connected = 0;

	voices = 10;
	gain = 1.1;
	randgain = 0.;
	twelfth = 1./12;
	semitone  = pow(2., 1./12.);
	smoothPitch = 1;
	scale_len = PITCHTABLESIZE;

	grate = 1.;
	tempgrate = 1.;
	grate_var = 0.;
	glen = 1.;
	glen_var = 0.;
	gpitch = 1.;
	gpitch_var = 0.;
	gpan_spread = 0.;
	time = 0;
	position = -1.;
	gimme = 0.;
	power = 1;
	ambi = 0;
	maxvoices = numvoices;

	oneshot = 0;
	newnote = 0;

	for (i = 0; i < numvoices; i++)
	{
		gvoiceSize[i] = 1000;
		gvoiceSpeed[i] = 1.;
		gvoiceCurrent[i] = 0.;
		gvoiceDirection[i] = 1;
		gvoiceOn[i] = 0;
		gvoiceDone[i] = 0;
		gvoiceRPan[i] = .5;
		gvoiceLPan[i] = .5;
		gvoiceGain[i] = 1.;
		gvoiceADSRon[i] = 0;

		for(j=0;j<num_channels;j++)
		{
			gvoiceSpat[i][j] = 0.;
			notechannelGain[i][j] = 0.;
			notechannelGainSpread[i][j] = 0.;
		}

		//note and oneshot inits
		noteTransp[i] = 0.;
		noteSize[i] = 100.;
		notePan[i] = 0.5;
		noteGain[i] = 1.;
		noteAttack[i] = 20.;
		noteDecay[i] = 50.;
		noteSustain[i] = 0.3;
		noteRelease[i] = 200.;
	}

	for(i=0;i<num_channels;i++)
	{
		channelGain[i] = 0.;
		channelGainSpread[i] = 0.;
	}

	doHanning = 0; // init hanning window

	for(i = 0; i < WINLENGTH; i++)
	{
		winTable[i] = 0.5 + 0.5*cos(TWOPI * i/WINLENGTH + .5*TWOPI);
	}

	for(i=0; i<PITCHTABLESIZE; i++)
	{
		pitchTable[i] = 0.;
	}

	rampLength = 256.;

	//sample buffer
	for( i = 0; i < initbuflen; i++) recordBuf[i] = 0;

	recordOn = 1;			//boolean
	recordCurrent = 0;
	recordRampVal = 0;
	rec_ramping = 0;

	l_buf = NULL;
	externalBuffer = 0;		//use internal buffer by default
	l_chan = 0;				//is there any other choice?
	discretepan = 0;		//off by default

	srand(54); //0.54?
}

disis_munger::~disis_munger()
{
	munger_free();
}
//TODO CLEANUP
float disis_munger::newNote(int whichVoice, int newNote)
{
	float newPosition;
	int i, temp;
	buffer *b = l_buf;

	gvoiceSize[whichVoice] 		= (long)newNoteSize(whichVoice, newNote);
	//gvoiceDirection[whichVoice] 	= newDirection(x);
	gvoiceDirection[whichVoice]	= noteDirection[newNote];

	if(num_channels == 2) {
		//gvoiceLPan[whichVoice] 		= ((float)rand() - 16384.) * ONE_OVER_MAXRAND * gpan_spread + 0.5;
		//gvoiceRPan[whichVoice]		= 1. - gvoiceLPan[whichVoice];
		//make equal power panning....
		//gvoiceLPan[whichVoice] 		= powf(gvoiceLPan[whichVoice], 0.5);
		//gvoiceRPan[whichVoice] 		= powf(gvoiceRPan[whichVoice], 0.5);
		gvoiceRPan[whichVoice] 		= powf(notePan[newNote], 0.5);
		gvoiceLPan[whichVoice] 		= powf((1. - notePan[newNote]), 0.5);
	}
	else {
		if(notePan[newNote] == -1.) {
			for(i=0;i<num_channels;i++) {
				notechannelGain[whichVoice][i] 		= 1.;
				notechannelGainSpread[whichVoice][i] = 0.;
			}
		} else {
			for(i=0;i<num_channels;i++) {
				notechannelGain[whichVoice][i] 		= 0.;	//initialize all to 0.
				notechannelGainSpread[whichVoice][i] = 0.;
			}
			temp = (int)notePan[newNote];
			if(temp>=num_channels) temp=0;
			notechannelGain[whichVoice][temp] = 1.;			//update the one we want
		}
		for(i=0;i<num_channels;i++) {
			gvoiceSpat[whichVoice][i] = notechannelGain[whichVoice][i] + ((long)(RANDOM()) - 16384.) * ONE_OVER_HALFRAND * notechannelGainSpread[whichVoice][i];
		}
	}

	gvoiceOn[whichVoice] 		= 1;
	gvoiceDone[whichVoice]		= 0;
	gvoiceGain[whichVoice]		= noteGain[newNote];

	gvoiceADSRon[whichVoice]		= 1;
	//post("adsr %f %f %f %f", noteAttack[newNote], noteDecay[newNote], noteSustain[newNote], noteRelease[newNote]);
	gvoiceADSR[whichVoice].setAllTimes(noteAttack[newNote]/1000., noteDecay[newNote]/1000., noteSustain[newNote], noteRelease[newNote]/1000.);
	gvoiceADSR[whichVoice].keyOn();

/*** set start point; tricky, cause of moving buffer, variable playback rates, backwards/forwards, etc.... ***/

    if(!externalBuffer) {
		// 1. RANDOM() positioning and moving buffer (default)
		if(position == -1. && recordOn == 1) {
			if(gvoiceDirection[whichVoice] == 1) {//going forward
				if(gvoiceSpeed[whichVoice] > 1.)
					newPosition = recordCurrent - onethirdBufsize - (long)(RANDOM()) * ONE_OVER_MAXRAND * onethirdBufsize;
				else
					newPosition = recordCurrent - (long)(RANDOM()) * ONE_OVER_MAXRAND * onethirdBufsize;//was 2/3rds
			}

			else //going backwards
				newPosition = recordCurrent - (long)(RANDOM()) * ONE_OVER_MAXRAND * onethirdBufsize;
		}

		// 2. fixed positioning and moving buffer
		else if (position >= 0. && recordOn == 1) {
			if(gvoiceDirection[whichVoice] == 1) {//going forward
				if(gvoiceSpeed[whichVoice] > 1.)
					//newPosition = recordCurrent - onethirdBufsize - position * onethirdBufsize;
					//this will follow more closely...
					newPosition = recordCurrent - gvoiceSize[whichVoice]*gvoiceSpeed[whichVoice] - position * onethirdBufsize;

				else
					newPosition = recordCurrent - position * onethirdBufsize;//was 2/3rds
			}

			else //going backwards
				newPosition = recordCurrent - position * onethirdBufsize;
		}

		// 3. RANDOM() positioning and fixed buffer
		else if (position == -1. && recordOn == 0) {
			if(gvoiceDirection[whichVoice] == 1) {//going forward
				newPosition = recordCurrent - onethirdBufsize - (long)(RANDOM()) * ONE_OVER_MAXRAND * onethirdBufsize;
			}
			else //going backwards
				newPosition = recordCurrent - (long)(RANDOM()) * ONE_OVER_MAXRAND * onethirdBufsize;
		}

		// 4. fixed positioning and fixed buffer
		else if (position >= 0. && recordOn == 0) {
			if(gvoiceDirection[whichVoice] == 1) {//going forward
				newPosition = recordCurrent - onethirdBufsize - position * onethirdBufsize;
			}
			else //going backwards
				newPosition = recordCurrent - position * onethirdBufsize;
		}
	}
	else {
		if (position == -1.) {
			newPosition = (long)(RANDOM()) * ONE_OVER_MAXRAND * b->Frames();
		}
		else  if (position >= 0.) newPosition = position * b->Frames();
	}

	return newPosition;

}

//creates a size for a new grain
//actual number of samples PLAYED, regardless of pitch
//might be shorter for higher pitches and long grains, to avoid collisions with recordCurrent
//size given now in milliseconds!
//for oneshot notes, this will also scale the ADSR and make it smaller, if the grainSpeed is high
float disis_munger::newNoteSize(int whichOne, int newNote)
{
	float newsize, temp, temp2, pitchExponent;

	//set grain pitch
	pitchExponent = noteTransp[newNote];
	gvoiceSpeed[whichOne] = gpitch * pow(semitone, pitchExponent);

	if(gvoiceSpeed[whichOne] < MINSPEED) gvoiceSpeed[whichOne] = MINSPEED;
	newsize = srate_ms*(noteSize[newNote]);
	//if(newsize > maxsize) newsize = maxsize;
	if(newsize*gvoiceSpeed[whichOne] > maxsize) {
		temp2 = maxsize/gvoiceSpeed[whichOne]; //newsize
		temp = temp2/newsize;
		noteAttack[newNote] *= temp;
		noteDecay[newNote] *= temp;
		noteRelease[newNote] *= temp;
		newsize = temp2;
	}
	//if(newsize < minsize) newsize = minsize;
	return newsize;

}


void disis_munger::munger_spat(short argc, t_atom *argv)
{
	int i, j;

	if (argc)
	{
		for (i=j=0; i < (argc - 1); i+=2)
		{
			channelGain[j] = GetAFloat(argv[i]);
			channelGainSpread[j] = GetAFloat(argv[i+1]);
			if(verbose > 1) post("disis_munger~ %s: channel gain %d = %f, spread = %f", munger_name, j, channelGain[j], channelGainSpread[j]);
			j++;
		}
	}
}

void disis_munger::munger_note(short argc, t_atom *argv)
{
	if (oneshot)
	{
		int i, temp;

		if(argc < 8)
		{
			post("disis_munger~ %s: need 8 args -- transposition, gain, pan, attkT, decayT, susLevel, relT, direction [-1/1]", munger_name);
			return;
		}

		newnote++;

		if(newnote > voices)
		{
			if(verbose > 0) post("disis_munger~ %s: too many notes amadeus.", munger_name);
			return;
		}

		noteTransp[newnote] = GetAFloat(argv[0]);
		noteGain[newnote] = GetAFloat(argv[1]);
		notePan[newnote] = GetAFloat(argv[2]);
		noteAttack[newnote] = GetAFloat(argv[3]);
		noteDecay[newnote] = GetAFloat(argv[4]);
		noteSustain[newnote] = GetAFloat(argv[5]);
		noteRelease[newnote] = GetAFloat(argv[6]);
		noteDirection[newnote] = GetAInt(argv[7]);

		//Stk ADSR bug?
		if (noteSustain[newnote] <= 0.001) noteSustain[newnote] = 0.001;

		noteSize[newnote] = noteAttack[newnote] + noteDecay[newnote] + noteRelease[newnote];
 	}
}

//turn oneshot mode on/off. in oneshot mode, the internal granular voice allocation method goes away
//	so the munger will be silent, except when it receives "note" messages
void disis_munger::munger_oneshot(short argc, t_atom *argv)
{
	int temp;

	if (argc)
	{
		temp = GetAInt(argv[0]);
		oneshot = temp;
    		if(verbose > 1) post("disis_munger~ %s: setting oneshot: %d", munger_name, temp);
	}
}


//external buffer stuff
bool disis_munger::munger_checkbuffer(bool reset)
{
    if (!l_buf) {
    //if(!l_buf || !l_buf->Ok() || !l_buf->Valid()) {
        post("disis_munger~ %s: error: no valid buffer defined", munger_name);
        // return zero length
        return false;
    }
    if(reset) l_buf->Set();  // try to re-associate buffer with munger_name

    if (!l_buf->Ok())
    {
		post("disis_munger~ %s: buffer mysteriously dissapeared. Reverting to internal buffer...", munger_name);
		munger_clearbuffer();
		return false;
    }
    else if (l_buf->Update())
    {
            // buffer parameters have been updated
            if(l_buf->Valid()) {
                if(verbose > 1) post("disis_munger~ %s: updated buffer reference", munger_name);
                return true;
            }
            else {
					post("disis_munger~ %s: error: buffer has become invalid", munger_name);
					munger_clearbuffer();
					return false;
            }
    }
    else return true;
}

void disis_munger::munger_clearbuffer()
{
	if (l_buf)
	{
		delete l_buf;
		l_buf = NULL;
		bufname = NULL;
		externalBuffer = 0;
		if(verbose > 1) post("disis_munger~ %s: external buffer deleted.", munger_name);
	}
}

void disis_munger::munger_setbuffer(short argc, t_atom *argv)
{
	if(argc == 0) {
    		// argument list is empty
    		// clear existing buffer
    		if (l_buf) munger_clearbuffer();
	}
	else if(argc == 1 && IsSymbol(argv[0])) {
        	// one symbol given as argument
        	// clear existing buffer
		if (l_buf) munger_clearbuffer();
        	// save buffer munger_name
        	bufname = (t_symbol*)GetSymbol(argv[0]);
        	// make new reference to system buffer object
        	l_buf = new buffer(bufname);
        	if(!l_buf->Ok()) {
            		if(verbose > 0) post("disis_munger~ %s: error: buffer %s is currently not valid!", munger_name, GetAString(argv[0]));
        	}
		else
		{
			if(verbose > 1) post("disis_munger~ %s: successfully associated with the %s buffer.", munger_name, GetAString(argv[0]));
			externalBuffer = 1;
			l_chan = 0;
		}
    	}
    	else {
        	// invalid argument list, leave buffer as is but issue error message to console
        	if(verbose > 0) post("disis_munger~ %s: error: message argument must be a string.", munger_name);
		if (l_buf) munger_clearbuffer();
    	}
}

void disis_munger::setverbose(short argc, t_atom *argv)
{
	int temp;

	if (argc)
	{
		temp = GetAInt(argv[0]);
		if (temp < 0) temp = 0;
		if (temp > 3) temp = 3;
		verbose = temp;
		post("disis_munger~ %s: setting verbose: %d", munger_name, temp);
		if (verbose < 3)
		{
			graincounter = 0;
			countsamples = 0;
		}
	}
}

//grain funcs
float disis_munger::envelope(int whichone, float sample)
{
	long done = gvoiceDone[whichone];
	long tail = gvoiceSize[whichone] - gvoiceDone[whichone];

	if(done < gvoiceRamp[whichone]) sample *= (done*gvoiceOneOverRamp[whichone]);
	else if(tail < gvoiceRamp[whichone]) sample *= (tail*gvoiceOneOverRamp[whichone]);

	return sample;
}
//tries to find an available voice; return -1 if no voices available
int disis_munger::findVoice()
{
	int i = 0, foundOne = -1;
	while(foundOne < 0 && i < voices ) {
		if (!gvoiceOn[i]) foundOne = i;
		i++;
	}
	return foundOne;
}

//creates a new (RANDOM()) start position for a new grain, returns beginning start sample
//sets up size and direction
//max grain size is BUFLENGTH / 3, to avoid recording into grains while they are playing
float disis_munger::newSetup(int whichVoice)
{
	float newPosition;
	buffer *b = l_buf;
	int i, tmpdiscretepan;

	gvoiceSize[whichVoice] 		= (long)newSize(whichVoice);
	gvoiceDirection[whichVoice] 	= newDirection();
	if( num_channels == 2)
	{
		gvoiceLPan[whichVoice] 		= ((long)(RANDOM()) - 16384.) * ONE_OVER_MAXRAND * gpan_spread + 0.5;
		gvoiceRPan[whichVoice]		= 1. - gvoiceLPan[whichVoice];
		//make equal power panning....
		gvoiceLPan[whichVoice] 		= powf(gvoiceLPan[whichVoice], 0.5);
		gvoiceRPan[whichVoice] 		= powf(gvoiceRPan[whichVoice], 0.5);
	}
	else if (discretepan)
	{
		tmpdiscretepan = (int)((long)(RANDOM()) * ONE_OVER_MAXRAND * ((float)num_channels + 0.99));
		for(i=0;i<num_channels;i++) {
			if (i == tmpdiscretepan) gvoiceSpat[whichVoice][i] = channelGain[i] + ((long)(RANDOM()) - 16384.) * ONE_OVER_HALFRAND * channelGainSpread[i];
			else gvoiceSpat[whichVoice][i] = 0.;
		}
	}
	else
	{
		for(i = 0; i < num_channels; i++)
			gvoiceSpat[whichVoice][i] = channelGain[i] + ((long)(RANDOM()) - 16384.) * ONE_OVER_HALFRAND * channelGainSpread[i];
	}
	gvoiceOn[whichVoice] 		= 1;
	gvoiceDone[whichVoice]		= 0;
	gvoiceGain[whichVoice]		= gain + ((long)(RANDOM()) - 16384.) * ONE_OVER_HALFRAND * randgain;

	gvoiceADSRon[whichVoice]		= 0;

	if(gvoiceSize[whichVoice] < 2.*rampLength) {
		gvoiceRamp[whichVoice] = .5 * gvoiceSize[whichVoice];
		if(gvoiceRamp[whichVoice] <= 0.) gvoiceRamp[whichVoice] = 1.;
		gvoiceOneOverRamp[whichVoice] = 1./gvoiceRamp[whichVoice];
	}
	else {
		gvoiceRamp[whichVoice] = rampLength;
		if(gvoiceRamp[whichVoice] <= 0.) gvoiceRamp[whichVoice] = 1.;
		gvoiceOneOverRamp[whichVoice] = 1./gvoiceRamp[whichVoice];
	}


/*** set start point; tricky, cause of moving buffer, variable playback rates, backwards/forwards, etc.... ***/

    if(!externalBuffer) {
	// 1. RANDOM() positioning and moving buffer (default)
	if(position == -1. && recordOn == 1) {
		if(gvoiceDirection[whichVoice] == 1) {//going forward
			if(gvoiceSpeed[whichVoice] > 1.)
				newPosition = recordCurrent - onethirdBufsize - (long)RANDOM() * ONE_OVER_MAXRAND * onethirdBufsize;
			else
				newPosition = recordCurrent - (long)RANDOM() * ONE_OVER_MAXRAND * onethirdBufsize;//was 2/3rds
		}

		else //going backwards
			newPosition = recordCurrent - (long)RANDOM() * ONE_OVER_MAXRAND * onethirdBufsize;
	}

	// 2. fixed positioning and moving buffer
	else if (position >= 0. && recordOn == 1) {
		if(gvoiceDirection[whichVoice] == 1) {//going forward
			if(gvoiceSpeed[whichVoice] > 1.)
				//newPosition = recordCurrent - onethirdBufsize - position * onethirdBufsize;
				//this will follow more closely...
				newPosition = recordCurrent - gvoiceSize[whichVoice]*gvoiceSpeed[whichVoice] - position * onethirdBufsize;

			else
				newPosition = recordCurrent - position * onethirdBufsize;//was 2/3rds
		}

		else //going backwards
			newPosition = recordCurrent - position * onethirdBufsize;
	}

	// 3. RANDOM() positioning and fixed buffer
	else if (position == -1. && recordOn == 0) {
		if(gvoiceDirection[whichVoice] == 1) {//going forward
			newPosition = recordCurrent - onethirdBufsize - (long)RANDOM() * ONE_OVER_MAXRAND * onethirdBufsize;
		}
		else //going backwards
			newPosition = recordCurrent - (long)RANDOM() * ONE_OVER_MAXRAND * onethirdBufsize;
	}

	// 4. fixed positioning and fixed buffer
	else if (position >= 0. && recordOn == 0) {
		if(gvoiceDirection[whichVoice] == 1) {//going forward
			newPosition = recordCurrent - onethirdBufsize - position * onethirdBufsize;
		}
		else //going backwards
			newPosition = recordCurrent - position * onethirdBufsize;
	}
    }
    else {
	if (position == -1.) {
		newPosition = (float)(RANDOM() * ONE_OVER_MAXRAND * (float)(b->Frames()));
	}
	else if (position >= 0.) newPosition = position * (float)b->Frames();
    }

    return newPosition;

}
//creates a size for a new grain
//actual number of samples PLAYED, regardless of pitch
//might be shorter for higher pitches and long grains, to avoid collisions with recordCurrent
//size given now in milliseconds!
float disis_munger::newSize(int whichOne)
{
	float newsize, temp;
	int pitchChoice, pitchExponent;

	//set grain pitch
	if(smoothPitch == 1) gvoiceSpeed[whichOne] = gpitch + ((long)RANDOM() - 16384.) * ONE_OVER_HALFRAND*gpitch_var;
	else {
		//temp = (long)RANDOM() * ONE_OVER_MAXRAND * gpitch_var * (float)PITCHTABLESIZE;
		temp = (long)RANDOM() * ONE_OVER_MAXRAND * gpitch_var * (float)scale_len;
		pitchChoice = (int) temp;
		if(pitchChoice > PITCHTABLESIZE) pitchChoice = PITCHTABLESIZE;
		if(pitchChoice < 0) pitchChoice = 0;
		pitchExponent = (int)pitchTable[pitchChoice];
		gvoiceSpeed[whichOne] = gpitch * pow(semitone, pitchExponent);
	}

	if(gvoiceSpeed[whichOne] < MINSPEED) gvoiceSpeed[whichOne] = MINSPEED;
	newsize = srate_ms*(glen + ((long)RANDOM() - 16384.) * ONE_OVER_HALFRAND * glen_var);
	if(newsize > maxsize) newsize = maxsize;
	if(newsize*gvoiceSpeed[whichOne] > maxsize)
		newsize = maxsize/gvoiceSpeed[whichOne];
	if(newsize < minsize) newsize = minsize;
	return newsize;

}
int disis_munger::newDirection()
{
//-1 == always backwards
//0  == backwards and forwards (default)
//1  == only forwards
	int dir;
	if(ambi == 0) {
		dir = RANDOM()- 16384;
		if (dir < 0) dir = -1;
		else dir = 1;
	}
	else
		if(ambi == -1) dir = -1;
	else
		dir = 1;

	return dir;
}

//buffer funcs
void disis_munger::recordSamp(float sample)
{
	if(recordCurrent >= buflen) recordCurrent = 0;

	if(recordOn) {
		if(rec_ramping == 0) recordBuf[recordCurrent++] = sample; //add sample
		else { //ramp up or down if turning on/off recording
			recordBuf[recordCurrent++] = sample * RECORDRAMP_INV * (float)recordRampVal;
			recordRampVal += rec_ramping;
			if(recordRampVal <= 0) {
				rec_ramping = 0;
				recordOn = 0;
			}
			if(recordRampVal >= RECORDRAMP) rec_ramping = 0;
		}
	}
}

float disis_munger::getSamp(double where)
{
	double alpha, om_alpha, output;
	long first;

	while(where < 0.) where += buflen;
	while(where >= buflen) where -= buflen;

	first = (long)where;

	alpha = where - first;
	om_alpha = 1. - alpha;

	output = recordBuf[first++] * om_alpha;
	if(first <  buflen) {
		output += recordBuf[first] * alpha;
	}
	else {
		output += recordBuf[0] * alpha;
	}

	return (float)output;
}

float disis_munger::getExternalSamp(double where)
{
	double alpha, om_alpha, output;
	long first;

	buffer *b = l_buf;
	float *tab;
	double frames, sampNum, nc;

	if (!b || !b->Ok()) {
		post("disis_munger~ %s: error: external buffer mysteriously AWOL, reverting to internatl buffer...", munger_name);
		externalBuffer = 0;
		return 0.;
	}

	tab = (float *)b->Data();
	frames = (double)b->Frames();
	nc = (double)b->Channels(); 		//== buffer~ framesize...

	if (where < 0.) where = 0.;
	else if (where >= frames) where = 0.;

	sampNum = where*nc;

	first = (long)sampNum;

	alpha = sampNum - first;
	om_alpha = 1. - alpha;

	output = tab[first] * om_alpha;
	first += (long)nc;
	output += tab[first] * alpha;

	return (float)output;
}

void disis_munger::munger_setramp(short argc, t_atom *argv)
{
	doHanning = 0;

	if (argc)
	{
		rampLength = srate_ms * GetAFloat(argv[0]);
		if(rampLength <= 0.) rampLength = 1.;
		if (verbose > 1) post("disis_munger~ %s: setting ramp to: %f ms", munger_name, (rampLength * one_over_srate_ms));
	}
}

void disis_munger::munger_scale(short argc, t_atom *argv)
{
	int i,j;

	if (verbose > 1) post("disis_munger~ %s: loading scale from input list", munger_name);
	smoothPitch = 0;

	for(i=0;i<PITCHTABLESIZE;i++) pitchTable[i] = 0.;
	if (argc > PITCHTABLESIZE) argc = PITCHTABLESIZE;
	for (i=0; i < argc; i++)
	{
		pitchTable[i] = GetAFloat(argv[i]);
	}

	scale_len = argc;

	i = 0;

	//wrap input list through all of pitchTable
	for (j=argc; j < PITCHTABLESIZE; j++) {
		pitchTable[j] = pitchTable[i++];
		if (i >= argc) i = 0;
	}

}

void disis_munger::munger_bufsize(short argc, t_atom *argv)
{
	float temp;

	if (argc)
	{
		temp = srate * GetAFloat(argv[0]);
		if(temp < 3.*(float)MINSIZE)
		{
			temp = 3.*(float)MINSIZE;
			if (verbose > 0) post("disis_munger~ %s error: delaylength too small!", munger_name);
		}
		//if(temp > (float)BUFLENGTH) temp = (float)BUFLENGTH;
		if(temp > initbuflen)
		{
			temp = initbuflen;
			if (verbose > 0) post("disis_munger~ %s error: delaylength too large!", munger_name);
		}
		//buflen = temp;
		//maxsize = buflen / 3.;
		maxsize = temp / 3.;
    	twothirdBufsize = maxsize * 2.;
    	onethirdBufsize = maxsize;
    	if (verbose > 1) post("disis_munger~ %s: setting delaylength to: %f seconds", munger_name, temp/srate);

	}
}
void disis_munger::munger_bufsize_ms(short argc, t_atom *argv)
{
	float temp;

	if (argc)
	{
		temp = srate_ms * GetAFloat(argv[0]);
		if(temp < 3.*(float)MINSIZE)
		{
			temp = 3.*(float)MINSIZE;
			if (verbose > 0) post("disis_munger~ %s error: delaylength_ms too small!", munger_name);
		}
		//if(temp > (float)BUFLENGTH) temp = (float)BUFLENGTH;
		if(temp > initbuflen)
		{
			temp = initbuflen;
			if (verbose > 0) post("disis_munger~ %s error: delaylength_ms too large!", munger_name);
		}
		//buflen = temp;
		//maxsize = buflen / 3.;
		maxsize = temp / 3.;
    	twothirdBufsize = maxsize * 2.;
    	onethirdBufsize = maxsize;
    	if (verbose > 1) post("disis_munger~ %s: setting delaylength to: %d milliseconds", munger_name, (long)(temp/srate_ms));
	}
}
void disis_munger::munger_setminsize(short argc, t_atom *argv)
{
	float temp;

	if (argc)
	{
		temp = srate_ms * GetAFloat(argv[0]);
		if(temp < (float)MINSIZE)
		{
			temp = (float)MINSIZE;
			if (verbose > 0) post("disis_munger~ %s error: minsize too small!", munger_name);
		}
		if(temp >= initbuflen) {
			temp = (float)MINSIZE;
			if (verbose > 0) post("disis_munger~ %s error: minsize too large!", munger_name);
		}
		minsize = temp;
    		if (verbose > 1) post("disis_munger~ %s: setting min grain size to: %f ms", munger_name, (minsize/srate_ms));

	}
}

void disis_munger::munger_discretepan(short argc, t_atom *argv)
{
	int temp;

	if (argc)
	{
		temp = GetAInt(argv[0]);
		if(temp < 0)
		{
			if (verbose > 0) post("disis_munger~ %s error: discretepan can be only 0 or 1!", munger_name);
			temp = 0;
		}
		if(temp > 1)
		{
			if (verbose > 0) post("disis_munger~ %s error: error: discretepan can be only 0 or 1!", munger_name);
			temp = 1;
		}
		discretepan = temp;

	}
}

void disis_munger::munger_setvoices(short argc, t_atom *argv)
{
	int temp;

	if (argc)
	{
		temp = GetAInt(argv[0]);
		if(temp < 0)
		{
			if (verbose > 0) post("disis_munger~ %s error: voices has to be between 0 and maxvoices (currently %d)!", munger_name, maxvoices);
			temp = 0;
		}
		if(temp > maxvoices)
		{
			if (verbose > 0) post("disis_munger~ %s error: voices has to be between 0 and maxvoices (currently %d)!", munger_name, maxvoices);
			temp = maxvoices;
		}
		voices = temp;
    		if (verbose > 1) post("disis_munger~ %s: setting voices to: %d ", munger_name, voices);

	}
}

void disis_munger::munger_maxvoices(short argc, t_atom *argv)
{
	int temp;

	if (argc)
	{
		temp = GetAInt(argv[0]);
		if(temp < 0)
		{
			if (verbose > 0) post("disis_munger~ %s error: maxvoices cannot be less than 0!", munger_name);
			temp = 0;
		}
		if(temp > numvoices) temp = numvoices;
		maxvoices = temp;
    		if (verbose > 1) post("disis_munger~ %s: setting max voices to: %d ", munger_name, maxvoices);
	}
}

void disis_munger::setpower(short argc, t_atom *argv)
{
	int temp;

	if (argc)
	{
		temp = GetAInt(argv[0]);
		power = temp;
    		post("disis_munger~ %s: setting power: %d", munger_name, temp);
	}
}
void disis_munger::munger_record(short argc, t_atom *argv)
{
	int temp;

	if (argc)
	{
		temp = GetAInt(argv[0]);
		//recordOn = temp;
		if (!temp)
		{
			recordRampVal = RECORDRAMP;
			rec_ramping = -1;
		}
		else
		{
			recordOn = 1;
			recordRampVal = 0;
			rec_ramping = 1;
		}
    		if (verbose > 1) post("disis_munger~ %s: setting record: %d", munger_name, temp);
	}
}

void disis_munger::munger_ambidirectional(short argc, t_atom *argv)
{
	int temp;

	if (argc)
	{
		temp = GetAInt(argv[0]);
		ambi = temp;
    		if (verbose > 1) post("disis_munger~ %s: setting ambidirectional: %d", munger_name, temp);
	}
}

void disis_munger::munger_gain(short argc, t_atom *argv)
{
	float temp;

	if (argc)
	{
		temp = GetAFloat(argv[0]);
    		if (verbose > 1) post("disis_munger~ %s: setting gain to: %f ", munger_name, temp);
    		gain = temp;
	}
}

void disis_munger::munger_setposition(short argc, t_atom *argv)
{
	float temp;

	if (argc)
	{
		temp = GetAFloat(argv[0]);
		if (temp > 1.) temp = 1.;
    		if (temp < 0.) temp = -1.;
    		if (verbose > 1) post("disis_munger~ %s: setting position to: %f ", munger_name, temp);
    		position = temp;
	}
}

void disis_munger::munger_randgain(short argc, t_atom *argv)
{
	float temp;

	if (argc)
	{
		temp = GetAFloat(argv[0]);
    		if (verbose > 1) post("disis_munger~ %s: setting rand_gain to: %f ", munger_name, temp);
    		randgain = temp;
	}
}

void disis_munger::munger_sethanning(short argc, t_atom *argv)
{
	if (verbose > 1) post("disis_munger~ %s: hanning window is busted", munger_name);
	doHanning = 1;
}

void disis_munger::munger_tempered(short argc, t_atom *argv)
{
	int i;
	if (verbose > 1) post("disis_munger~ %s: doing tempered scale", munger_name);
	smoothPitch = 0;
	scale_len = 100;
	for(i=0; i<scale_len-1; i += 2) {
		pitchTable[i] = 0.5*i;
		pitchTable[i+1] = -0.5*i;
	}
	scale_len = PITCHTABLESIZE;
}
void disis_munger::munger_smooth(short argc, t_atom *argv)
{
	if (verbose > 1) post("disis_munger~ %s: doing smooth scale", munger_name);
	smoothPitch = 1;
}

void disis_munger::munger_alloc()
{
	int i;

	recordBuf = new float[buflen+1];
	if (!recordBuf)
	{
		error("disis_munger~ %s: out of memory", munger_name);
	}

	winTime = new float[numvoices];
	winRate = new float[numvoices];
	gvoiceSize = new long[numvoices];
	gvoiceSpeed = new double[numvoices];
	gvoiceCurrent = new double[numvoices];
	gvoiceDirection = new int[numvoices];
	gvoiceOn = new int[numvoices];
	gvoiceDone = new long[numvoices];
	gvoiceLPan = new float[numvoices];
	gvoiceRPan = new float[numvoices];
	gvoiceRamp = new float[numvoices];
	gvoiceOneOverRamp = new float[numvoices];
	gvoiceGain = new float[numvoices];
	gvoiceADSR = new stk::ADSR[numvoices];
	gvoiceADSRon = new int[numvoices];
	noteTransp = new float[numvoices];
	noteSize = new float[numvoices];
	notePan = new float[numvoices];
	noteGain = new float[numvoices];
	noteAttack = new float[numvoices];
	noteDecay = new float[numvoices];
	noteSustain = new float[numvoices];
	noteRelease = new float[numvoices];
	noteDirection = new int[numvoices];

	gvoiceSpat = new float *[numvoices];
	notechannelGain = new float *[numvoices];
	notechannelGainSpread = new float *[numvoices];
	for (i=0; i < numvoices; i++)
	{
		gvoiceSpat[i] = new float[num_channels];
		notechannelGain[i] = new float[num_channels];
		notechannelGainSpread[i] = new float[num_channels];
	}

	out = new float *[num_channels];
	outsamp = new float [num_channels];
	channelGain = new float [num_channels];
	channelGainSpread = new float [num_channels];
}

void disis_munger::munger_clear(short argc, t_atom *argv)
{
	long i;
	for(i=0; i<initbuflen; i++) recordBuf[i] = 0.;
}

void disis_munger::munger_free()
{
	int i;

	if (recordBuf) delete [] recordBuf;
	if (munger_name) delete [] munger_name;
	if (winTime) delete [] winTime;
	if (winRate) delete [] winRate;
	if (gvoiceSize) delete [] gvoiceSize;
	if (gvoiceSpeed) delete [] gvoiceSpeed;
	if (gvoiceCurrent) delete [] gvoiceCurrent;
	if (gvoiceDirection) delete [] gvoiceDirection;
	if (gvoiceOn) delete [] gvoiceOn;
	if (gvoiceDone) delete [] gvoiceDone;
	if (gvoiceLPan) delete [] gvoiceLPan;
	if (gvoiceRPan) delete [] gvoiceRPan;
	if (gvoiceRamp) delete [] gvoiceRamp;
	if (gvoiceOneOverRamp) delete [] gvoiceOneOverRamp;
	if (gvoiceGain) delete [] gvoiceGain;
	if (gvoiceADSRon) delete [] gvoiceADSRon;
	if (noteTransp) delete [] noteTransp;
	if (noteSize) delete [] noteSize;
	if (notePan) delete [] notePan;
	if (noteGain) delete [] noteGain;
	if (noteAttack) delete [] noteAttack;
	if (noteDecay) delete [] noteDecay;
	if (noteSustain) delete [] noteSustain;
	if (noteRelease) delete [] noteRelease;
	if (noteDirection) delete [] noteDirection;
	if (outsamp) delete [] outsamp;
	if (channelGain) delete [] channelGain;
	if (channelGainSpread) delete [] channelGainSpread;
	if (out) delete [] out;

	if (gvoiceADSR) delete [] gvoiceADSR;

	if (l_buf) delete l_buf;

	for (i=0; i < numvoices; i++)
	{
		if (gvoiceSpat[i]) delete gvoiceSpat[i];
		if (notechannelGain[i]) delete notechannelGain[i];
		if (notechannelGainSpread[i]) delete notechannelGainSpread[i];
	}
	if (gvoiceSpat) delete [] gvoiceSpat;
	if (notechannelGain) delete [] notechannelGain;
	if (notechannelGainSpread) delete [] notechannelGainSpread;
}

void disis_munger::CbSignal()
{
	//ins1 holds the signal vector ofthe first inlet, index 0

	float samp;
	int newvoice, i, j, n;

	const float *ins1 = InSig(0);
	n = Blocksize();

	for (i=0;i<num_channels;i++) {
		out[i] = (float *)(OutSig(i));
	}

	//bypass stuff
	//if(thisHdr()->z_disabled) goto out;

	if(gpan_spread > 1.) gpan_spread = 1.;
	if(gpan_spread < 0.) gpan_spread = 0.;

	if(!power) {
		while(n--)
		{
			for(i = 0; i < num_channels; i++) *out[i]++ = 0.;
		}
	}

	else {
		while(n--) {
			if (verbose > 2)
			{
				countsamples++;
				if (countsamples >= ((int)srate - 1))
				{
					post("disis_munger~ %s: outputting %d grains per second", munger_name, graincounter);
					graincounter = 0;
					countsamples = 0;
				}
			}
			for( i = 0; i < num_channels; i++) outsamp[i] = 0.;
			//record a sample
			recordSamp(*ins1++);

			//grab a note if requested; works in oneshot mode or otherwise
			if (oneshot)
			{
				while(newnote > 0)
				{
					newvoice = findVoice();
					if(newvoice >= 0)
					{
						gvoiceCurrent[newvoice] = newNote(newvoice, newnote);
					}
					newnote--;
				}
			}
			//find a voice if it's time (high resolution). ignore if in "oneshot" mode
			else
			{
				if(time++ >= (long)gimme) {
					time = 0;
					newvoice = findVoice();
					if(newvoice >= 0) {
						gvoiceCurrent[newvoice] = newSetup(newvoice);
					}
					tempgrate = grate + ((long)RANDOM() - 16384.) * ONE_OVER_HALFRAND * grate_var;
					gimme = srate_ms * tempgrate; //grate is actually time-distance between grains
				}
			}
			time++;
			//mix 'em, pan 'em
			for(i=0; i< maxvoices; i++) {
				if(gvoiceOn[i]) {
					//get a sample, envelope it
					if(externalBuffer) samp = getExternalSamp(gvoiceCurrent[i]);
					else samp = getSamp(gvoiceCurrent[i]);
					if (!gvoiceADSRon[i]) samp = envelope(i, samp) * gvoiceGain[i];
					else samp = samp * gvoiceADSR[i].tick() * gvoiceGain[i]; //ADSR_ADRtick->computeSample()
					//pan it
					if(num_channels == 2) {
						outsamp[0] += samp * gvoiceLPan[i];
						outsamp[1] += samp * gvoiceRPan[i];
					}
					else { //multichannel subroutine
						for(j=0;j<num_channels;j++) {
							outsamp[j] += samp * gvoiceSpat[i][j];
						}
					}

					//see if grain is done after jumping to next sample point
					gvoiceCurrent[i] += (double)gvoiceDirection[i] * (double)gvoiceSpeed[i];
					if (!gvoiceADSRon[i]) {
						if(gvoiceDone[i] >= gvoiceSize[i] || ++gvoiceDone[i] >= gvoiceSize[i])
						{
							gvoiceOn[i] = 0;
						}
					}
					else {
						if(gvoiceADSR[i].getState() == gvoiceADSR[i].SUSTAIN) gvoiceADSR[i].keyOff();
						if(++gvoiceDone[i] >= gvoiceSize[i] || gvoiceADSR[i].getState() == gvoiceADSR[i].IDLE)
						{
							gvoiceOn[i] = 0;
						}
					}
					if (!gvoiceOn[i]) graincounter++;
				}
			}
			for(i=0;i<num_channels;i++) {
				*out[i]++ = outsamp[i];
			}
		}
	}
}

bool disis_munger::CbDsp()
{
	float old_srate;

	// recheck buffer on DSP update
	if (externalBuffer) munger_checkbuffer(true);

	old_srate = srate;
	srate = Samplerate();
	if (srate != old_srate)
	{
		one_over_srate = 1./srate;
		srate_ms = srate/1000.;
		one_over_srate_ms = 1./srate_ms;
	}
	return true;
}

void disis_munger::munger_poststate(short argc, t_atom *argv)
{
	post (">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
	post ("***CURRENT disis_munger~ %s PARAMETER VALUES***:", munger_name);
	post ("all times in milliseconds");
	post ("version: %d.%d.%d", (int)MUNGER_MAJOR, (int)MUNGER_MINOR, (int)MUNGER_REV);
	post ("grain spacing = %f", grate);
	post ("grain spacing variation = %f", grate_var);
	post ("grain length = %f", glen);
	post ("grain length variation = %f", glen_var);
	post ("grain transposition multiplier = %f", gpitch);
	post ("grain transposition multiplier variation = %f", gpitch_var);
	post ("panning spread = %f", gpan_spread);
	post ("grain gain = %f", gain);
	post ("grain gain variation = %f", randgain);
	post ("playback position (-1 = RANDOM()) = %f", position);
	post ("grain playback direction (0 = both) = %d", ambi);
	post ("buffer length = %f", buflen * one_over_srate_ms);
	post ("max grain size = %f", maxsize * one_over_srate_ms);
	post ("min grain size = %f", minsize * one_over_srate_ms);
	post ("max number of voices = %ld", maxvoices);
	post ("current number of voices = %d", voices);
	post ("grain envelope (ramp) length = %f", rampLength * one_over_srate_ms);
	post ("recording? = %d", recordOn);
	post ("power = %d", power);
	post ("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
}
