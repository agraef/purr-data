/* 
vst~ - VST plugin object for PD 
based on the work of Jarno Seppänen and Mark Williamson

Copyright (c)2003-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#ifndef __VSTHOST_H
#define __VSTHOST_H

#include <flext.h>
#include <string>
#include <map>
#include <math.h>

#include "AEffectx.h"
#include "AEffEditor.hpp"


#if FLEXT_OS == FLEXT_OS_WIN
#include <windows.h>
typedef HWND WHandle;
typedef HMODULE MHandle;
#elif FLEXT_OS == FLEXT_OS_MAC
#include <CoreServices/CoreServices.h>
typedef WindowRef WHandle;
typedef CFBundleRef MHandle;
#else
#error Platform not supported!
#endif


#define MIDI_MAX_EVENTS	64

class Responder
{
public:
    virtual void Respond(const t_symbol *sym,int argc = 0,const t_atom *argv = NULL) = 0;
};


class VSTPlugin:
    public flext
{
public:
    static VSTPlugin *New(Responder *resp);
    static void Delete(VSTPlugin *p);

    static void Setup();

	bool Instance(const char *plug,const char *subplug = NULL);
	void DspInit(float samplerate,int blocksize);

private:
	VSTPlugin(Responder *resp);
	~VSTPlugin();

    static ThrCond thrcond;
    static void worker(thr_params *p);
    
	void Free();

    //////////////////////////////////////////////////////////////////////////////

public:
    bool Is() const { return effect != NULL; }

    long GetVersion() const { return effect->version; }

    bool IsSynth() const { return HasFlags(effFlagsIsSynth); }
    bool IsReplacing() const { return HasFlags(effFlagsCanReplacing); }
    bool HasEditor() const { return HasFlags(effFlagsHasEditor); }

	const char *GetName() const { return productname; }
	const char *GetVendorName() const { return vendorname; }
	const char *GetDllName() const { return dllname.c_str(); }

    long UniqueID() const { return effect->uniqueID; }

    int GetNumInputs() const { return effect->numInputs; }
    int GetNumOutputs() const { return effect->numOutputs; }

    void ListPlugs(const t_symbol *sym) const;

private:
	char productname[300];
	char vendorname[300];
    std::string dllname;	// Contains dll name

    //////////////////////////////////////////////////////////////////////////////

public:
    int GetNumParams() const { return effect?effect->numParams:0; }
	void GetParamName(int numparam,char *name) const;
	void GetParamValue(int numparam,char *parval) const;
	float GetParamValue(int numparam) const;

    // scan plugin names (can take a _long_ time!!)
    void ScanParams(int i = -1);
    // get number of scanned parameters
    int ScannedParams() const { return paramnamecnt; }
    // get index of named (scanned) parameter... -1 if not found
    int GetParamIx(const char *p) const;

    bool SetParamFloat(int parameter, float value);
    bool SetParamInt(int parameter, int value) { return SetParamFloat(parameter,value/65535.0f); }

    void SetCurrentProgram(int prg) { Dispatch(effSetProgram,0,prg); }
    int GetCurrentProgram() const { return Dispatch(effGetProgram); }
	int GetNumPrograms() const { return effect->numPrograms; }

    int GetNumCategories() const { return Dispatch(effGetNumProgramCategories); }
	bool GetProgramName(int cat,int p,char* buf) const;

private:
    struct NameCmp:
        std::less<std::string>
    {
        bool operator()(const std::string &a,const std::string &b) const { return a.compare(b) < 0; }
    };

    typedef std::map<std::string,int,NameCmp> NameMap;
    int paramnamecnt;
    NameMap paramnames;

    //////////////////////////////////////////////////////////////////////////////

public:
    void SetPos(int x,int y,bool upd = true);
    void SetSize(int x,int y,bool upd = true);
    void SetX(int x,bool upd = true) { SetPos(x,posy,upd); }
    void SetY(int y,bool upd = true) { SetPos(posx,y,upd); }
    void SetW(int x,bool upd = true) { SetSize(x,sizey,upd); }
    void SetH(int y,bool upd = true) { SetSize(sizex,y,upd); }
    int GetX() const { return posx; }
    int GetY() const { return posy; }
    int GetW() const { return sizex; }
    int GetH() const { return sizey; }
    void SetCaption(bool b);
    bool GetCaption() const { return caption; }
    void SetHandle(bool h);
    bool GetHandle() const { return handle; }
    void SetTitle(const char *t);
    const char *GetTitle() const { return title.c_str(); }

    void ToFront();
    void BelowFront();

	void Edit(bool open);

	void StartEditing(WHandle h);
    void StopEditing();
    bool IsEdited() const { return hwnd != NULL; }
    WHandle EditorHandle() const { return hwnd; }
    void EditingEnded() { hwnd = NULL; thrcond.Signal(); }

    void GetEditorRect(ERect &er) const { ERect *r; Dispatch(effEditGetRect,0,0,&r); er = *r; }
    void EditorIdle() { Dispatch(effEditIdle); }

    void Visible(bool vis,bool upd = true);
    bool IsVisible() const { return visible; }

    void Paint(ERect &r) const { Dispatch(effEditDraw,0,0,&r); }

private:
    bool visible;
    int posx,posy,sizex,sizey; // Window position
    bool caption; // Window border
    bool handle; // Window handle (like taskbar button)
    std::string title; // Window title

    //////////////////////////////////////////////////////////////////////////////

public:
    enum {
        MIDI_NOTEOFF = 0x80,
        MIDI_NOTEON = 0x90,
        MIDI_POLYAFTERTOUCH = 0xa0,
        MIDI_CONTROLCHANGE = 0xb0,
        MIDI_PROGRAMCHANGE = 0xc0,
        MIDI_AFTERTOUCH = 0xd0,
        MIDI_PITCHBEND = 0xe0,
        MIDI_SYSEX = 0xf0,
    };
    
    void SetEvents(bool ev) { dumpevents = ev; }
    bool GetEvents() const { return dumpevents; }

    bool AddMIDI(unsigned char data0,unsigned char data1 = 0,unsigned char data2 = 0);

    static int range(int value,int mn = 0,int mx = 127) { return value < mn?mn:(value > mx?mx:value); }

    void SetChannel(int channel) { midichannel = range(channel,0,0xf); }
    int GetChannel() const { return midichannel; }

	bool AddNoteOn(unsigned char note,unsigned char speed /*,unsigned char midichannel = 0*/)
    {
        return AddMIDI(MIDI_NOTEON+midichannel,note,speed);
    }

    bool AddNoteOff(unsigned char note /*,unsigned char midichannel = 0*/)
    {
        return AddMIDI(MIDI_NOTEOFF+midichannel,note,0);
    }
	
	void AddControlChange(int control,int value)
    {
        AddMIDI(MIDI_CONTROLCHANGE+midichannel,range(control),range(value));
    }

	void AddProgramChange(int value)
    {
        AddMIDI(MIDI_PROGRAMCHANGE+midichannel,range(value),0);
    }

	void AddPitchBend(int value)
    {
	    AddMIDI(MIDI_PITCHBEND+midichannel,(value&127),((value>>7)&127));
    }

	void AddAftertouch(int value)
    {
 	    AddMIDI(MIDI_AFTERTOUCH+midichannel,range(value));
    }

	void AddPolyAftertouch(unsigned char note,int value)
    {
 	    AddMIDI(MIDI_POLYAFTERTOUCH+midichannel,note,range(value));
    }

private:
	void SendMidi();

    //	static VstTimeInfo _timeInfo;
	VstMidiEvent midievent[MIDI_MAX_EVENTS];
	VstEvents events;
	int	eventqusz;

	char midichannel;
    bool dumpevents;

    //////////////////////////////////////////////////////////////////////////////

public:

    void SetPlaying(bool p) { if(playing != p) transchg = true,playing = p; }
    bool GetPlaying() const { return playing; }
    void SetLooping(bool p) { if(looping != p) transchg = true,looping = p; }
    bool GetLooping() const { return looping; }
    void SetFeedback(bool p) { feedback = p; }
    bool GetFeedback() const { return feedback; }

    void SetSamplePos(double p) { if(samplepos != p) transchg = true,samplepos = p; }
    double GetSamplePos() const { return samplepos; }
    void SetTempo(double p) { if(tempo != p) transchg = true,tempo = p; }
    double GetTempo() const { return tempo; }
    void SetPPQPos(double p) { if(ppqpos != p) transchg = true,ppqpos = p; }
    double GetPPQPos() const { return ppqpos; }

    void SetTimesigNom(int p) { if(timesignom != p) transchg = true,timesignom = p; }
    int GetTimesigNom() const { return timesignom; }
    void SetTimesigDen(int p) { if(timesigden != p) transchg = true,timesigden = p; }
    int GetTimesigDen() const { return timesigden; }
    void SetBarStart(double p) { if(barstartpos != p) transchg = true,barstartpos = p; }
    double GetBarStart() const { return barstartpos; }
    void SetCycleStart(double p) { if(cyclestartpos != p) transchg = true,cyclestartpos = p; }
    double GetCycleStart() const { return cyclestartpos; }
    void SetCycleEnd(double p) { if(cycleendpos != p) transchg = true,cycleendpos = p; }
    double GetCycleEnd() const { return cycleendpos; }

    void SetSmpteOffset(int p) { if(smpteoffset != p) transchg = true,smpteoffset = p; }
    int GetSmpteOffset() const { return smpteoffset; }
    void SetSmpteRate(int p) { if(smpterate != p) transchg = true,smpterate = p; }
    int GetSmpteRate() const { return smpterate; }

private:

    bool playing,looping,feedback;
    float samplerate;
    bool transchg;

    double samplepos,tempo;
    double ppqpos;

    int timesignom,timesigden;
    double barstartpos;
    double cyclestartpos,cycleendpos;
    int smpteoffset,smpterate;

    //////////////////////////////////////////////////////////////////////////////

public:
	bool processReplacing(float **inputs,float **outputs,long sampleframes )
    {
        FLEXT_ASSERT(effect);
  	    effect->processReplacing(effect,inputs,outputs,sampleframes);
        if(playing) updatepos(sampleframes);
        return true;
    }

	bool process(float **inputs,float **outputs,long sampleframes )
    {
        FLEXT_ASSERT(effect);
   	    effect->process(effect,inputs,outputs,sampleframes);
        return true;
    }

private:

    void updatepos(long frames);

    //////////////////////////////////////////////////////////////////////////////

private:
    Responder *responder;

    bool NewPlugin(const char *plugname);
    void FreePlugin();
    bool InstPlugin(long plugid = 0);

    static long uniqueid;
    static std::string dllloading;

    inline long GetFlags() const { return effect?effect->flags:0; } 
    inline bool HasFlags(long msk) const { return effect && (effect->flags&msk); } 


#if FLEXT_OS == FLEXT_OS_WIN
    // the handle to the shared library
	MHandle hdll;
#endif
    // the handle to the plugin editor window
    WHandle hwnd;
    // the VST plugin instance
	AEffect *effect;

    typedef AEffect *(VSTCALLBACK *PVSTMAIN)(audioMasterCallback audioMaster);
    PVSTMAIN pluginmain;
    audioMasterCallback audiomaster;
    
    long Dispatch(long opCode,long index = 0,long value = 0,void *ptr = NULL,float opt = 0) const
	{
        FLEXT_ASSERT(effect);
        return effect->dispatcher(effect,opCode,index,value,ptr,opt);
	}

	static long VSTCALLBACK Master(AEffect *effect, long opcode, long index, long value, void *ptr, float opt);

    static const t_symbol *sym_param;
    static const t_symbol *sym_event,*sym_evmidi,*sym_evaudio,*sym_evvideo,*sym_evparam,*sym_evtrigger,*sym_evsysex,*sym_ev_;
    static const t_symbol *sym_midi[8];

    void ProcessEvent(const VstEvent &ev);
};

#endif
