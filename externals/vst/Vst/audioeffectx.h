#ifndef __audioeffectx__
#define __audioeffectx__

//----------------------------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// version 2.0 extension
// (c)1999 Steinberg Soft+Hardware GmbH
//----------------------------------------------------------------------------------------------------------------------------

#ifndef __AudioEffect__
#include "AudioEffect.hpp"	// version 1.0 base class AudioEffect
#endif

#ifndef __aeffectx__
#include "aeffectx.h"		// version 2.0 'C' extensions and structures
#endif

//----------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
// AudioEffectX extends AudioEffect with the new features. so you should derive
// your plug from AudioEffectX
//----------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------

class AudioEffectX : public AudioEffect
{
public:
	AudioEffectX (audioMasterCallback audioMaster, long numPrograms, long numParams);
	virtual ~AudioEffectX ();

	virtual long dispatcher (long opCode, long index, long value, void *ptr, float opt);

	// 'host' are methods which go from plug to host, and are usually not overridden
	// 'plug' are methods which you may override to implement the according functionality (to host)

//----------------------------------------------------------------------------------------------------------------------------
// events + time
//----------------------------------------------------------------------------------------------------------------------------

	// host
	virtual void wantEvents (long filter = 1);		// filter is currently ignored, midi channel data only (default)
	virtual VstTimeInfo* getTimeInfo (long filter);
										// returns const VstTimeInfo* (or 0 if not supported)
										// filter should contain a mask indicating which fields are requested
										// (see valid masks in aeffectx.h), as some items may require extensive
										// conversions 
	virtual long tempoAt (long pos);	// returns tempo (in bpm * 10000) at sample frame location <pos>
	bool sendVstEventsToHost (VstEvents* events);	// true:success

	// plug
	virtual long processEvents (VstEvents* events) {return 0;}	// wants no more...else return 1!
																// VstEvents and VstMidiEvents are declared in aeffectx.h

//----------------------------------------------------------------------------------------------------------------------------
// parameters and programs
//----------------------------------------------------------------------------------------------------------------------------

	// host
	virtual long getNumAutomatableParameters ();
	virtual long getParameterQuantization ();		// returns the integer value for +1.0 representation,
													// or 1 if full single float precision is maintained
													// in automation. parameter index in <value> (-1: all, any)
	// plug
	virtual bool canParameterBeAutomated (long index) { return true; }
	virtual bool string2parameter (long index, char* text) {return false;} 	// note: implies setParameter. text==0 is to be
																			// expected to check the capability (returns true).	
	virtual float getChannelParameter (long channel, long index) {return 0;}
	virtual long getNumCategories () {return 1L;}
	virtual bool getProgramNameIndexed (long category, long index, char* text) {return false;}
	virtual bool copyProgram (long destination) {return false;}

//----------------------------------------------------------------------------------------------------------------------------
// connections, configuration
//----------------------------------------------------------------------------------------------------------------------------

	// host
	virtual bool ioChanged ();				// tell host numInputs and/or numOutputs and/or numParameters has changed
	virtual bool needIdle ();				// plug needs idle calls (outside its editor window)
	virtual bool sizeWindow (long width, long height);
	virtual double updateSampleRate ();		// gets and returns sample rate from host (may issue setSampleRate() )
	virtual long updateBlockSize ();		// same for block size
	virtual long getInputLatency ();
	virtual long getOutputLatency ();
	virtual AEffect* getPreviousPlug (long input);	// input can be -1 in which case the first found is returned
	virtual AEffect* getNextPlug (long output);		// output can be -1 in which case the first found is returned

	// plug
	virtual void inputConnected (long index, bool state) {}		// input at <index> has been (dis-)connected,
	virtual void outputConnected (long index, bool state) {}	// same as input; state == true: connected
	virtual bool getInputProperties (long index, VstPinProperties* properties) {return false;}
	virtual bool getOutputProperties (long index, VstPinProperties* properties) {return false;}
	virtual VstPlugCategory getPlugCategory()
		{ if (cEffect.flags & effFlagsIsSynth) return kPlugCategSynth; return kPlugCategUnknown; }

//----------------------------------------------------------------------------------------------------------------------------
// realtime
//----------------------------------------------------------------------------------------------------------------------------

	// host
	virtual long willProcessReplacing ();	// returns 0: not implemented, 1: replacing, 2: accumulating
	virtual long getCurrentProcessLevel ();	// returns: 0: not supported,
											// 1: currently in user thread (gui)
											// 2: currently in audio thread or irq (where process is called)
											// 3: currently in 'sequencer' thread or irq (midi, timer etc)
											// 4: currently offline processing and thus in user thread
											// other: not defined, but probably pre-empting user thread.
	virtual long getAutomationState ();		// returns 0: not supported, 1: off, 2:read, 3:write, 4:read/write
	virtual void wantAsyncOperation (bool state = true);	// notify host that we want to operate asynchronously.
											// process() will return immedeately; host will poll getCurrentPosition
											// to see if data are available in time.
	virtual void hasExternalBuffer (bool state = true);		// external dsp, may have their own output buffe (32 bit float)
											// host then requests this via effGetDestinationBuffer

	// plug
	virtual long reportCurrentPosition () {return 0;}		// for external dsp, see wantAsyncOperation ()
	virtual float* reportDestinationBuffer () {return 0;}	// for external dsp (dma option)

//----------------------------------------------------------------------------------------------------------------------------
// offline
//----------------------------------------------------------------------------------------------------------------------------

	// host
	virtual bool offlineRead (VstOfflineTask* offline, VstOfflineOption option, bool readSource = true);
	virtual bool offlineWrite (VstOfflineTask* offline, VstOfflineOption option);
	virtual bool offlineStart (VstAudioFile* ptr, long numAudioFiles, long numNewAudioFiles);
	virtual long offlineGetCurrentPass ();
	virtual long offlineGetCurrentMetaPass ();

	// plug
	virtual bool offlineNotify (VstAudioFile* ptr, long numAudioFiles, bool start) { return false; }
	virtual bool offlinePrepare (VstOfflineTask* offline, long count) {return false;}
	virtual bool offlineRun (VstOfflineTask* offline, long count) {return false;}

	virtual long offlineGetNumPasses () {return 0;}
	virtual long offlineGetNumMetaPasses () {return 0;}

//----------------------------------------------------------------------------------------------------------------------------
// other
//----------------------------------------------------------------------------------------------------------------------------

	// host
	virtual void setOutputSamplerate (float samplerate);
	virtual bool getSpeakerArrangement (VstSpeakerArrangement* pluginInput, VstSpeakerArrangement* pluginOutput);
	virtual bool getHostVendorString (char* text);		// fills <text> with a string identifying the vendor (max 64 char)
	virtual bool getHostProductString (char* text);		// fills <text> with a string with product name (max 64 char)
	virtual long getHostVendorVersion ();				// returns vendor-specific version
	virtual long hostVendorSpecific (long lArg1, long lArg2, void* ptrArg, float floatArg);		// no definition
	virtual long canHostDo (char* text);				// see 'hostCanDos' in audioeffectx.cpp
														// returns 0: don't know (default), 1: yes, -1: no
	virtual void isSynth (bool state = true);			// will call wantEvents if true
	virtual void noTail (bool state = true);			// true: tells host we produce no output when silence comes in
														// enables host to omit process() when no data are present
														// on any one input.
	virtual long getHostLanguage ();					// returns VstHostLanguage
	virtual void* openWindow (VstWindow*);				// create new window
	virtual bool closeWindow (VstWindow*);				// close a newly created window
	virtual void* getDirectory ();						// get the plug's directory, FSSpec on mac, else char*
	virtual bool updateDisplay();						// something has changed, update 'multi-fx' display
														// returns true if supported

	// plug
	virtual bool processVariableIo (VstVariableIo* varIo) {return false;}
	virtual bool setSpeakerArrangement (VstSpeakerArrangement* pluginInput, VstSpeakerArrangement* pluginOutput) {return false;}
	virtual void setBlockSizeAndSampleRate (long blockSize, float sampleRate)
		{this->blockSize = blockSize; this->sampleRate = sampleRate;}
	virtual bool setBypass(bool onOff) {return false;}			// for 'soft-bypass; process() still called
	virtual bool getEffectName (char* name) {return false;}		// name max 32 char
	virtual bool getErrorText (char* text) {return false;}		// max 256 char
	virtual bool getVendorString (char* text) {return false;}	// fill text with a string identifying the vendor (max 64 char)
	virtual bool getProductString (char* text) {return false;}	// fill text with a string identifying the product name (max 64 char)					// fills <ptr> with a string with product name (max 64 char)
	virtual long getVendorVersion () {return 0;}				// return vendor-specific version
	virtual long vendorSpecific (long lArg, long lArg2, void* ptrArg, float floatArg) {return 0;}
														// no definition, vendor specific handling
	virtual long canDo (char* text) {return 0;}			// see 'plugCanDos' in audioeffectx.cpp. return values:
														// 0: don't know (default), 1: yes, -1: no
	virtual void* getIcon () {return 0;}				// not yet defined
	virtual bool setViewPosition (long x, long y) {return false;}
	virtual long getGetTailSize () {return 0; }
	virtual long fxIdle () {return 0;}
	virtual bool getParameterProperties (long index, VstParameterProperties* p) {return false;}
	virtual bool keysRequired () {return false;}		// version 1 plugs will return true
	virtual long getVstVersion () {return 2;}
};

#endif
