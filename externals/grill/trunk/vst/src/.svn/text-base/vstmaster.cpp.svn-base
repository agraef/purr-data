/* 
vst~ - VST plugin object for PD 
based on the work of Jarno Seppänen and Mark Williamson

Copyright (c)2003-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "vsthost.h"

static const int VST_VERSION = 100;
static const char *vendor = "grrrr.org";
static const char *product = "vst~";


void VSTPlugin::ProcessEvent(const VstEvent &ev)
{
    if(!responder && dumpevents) return;

    if(ev.type == kVstMidiType) {
        const VstMidiEvent &mev = (const VstMidiEvent &)ev;
        t_atom lst[10];
        SetSymbol(lst[0],sym_evmidi);
        int midi = ((unsigned char)mev.midiData[0]>>4)-8;
        FLEXT_ASSERT(midi >= 0 && midi < 8);
        SetSymbol(lst[1],sym_midi[midi]);
        SetInt(lst[2],(unsigned char)mev.midiData[0]&0x0f);
        SetInt(lst[3],(unsigned char)mev.midiData[1]);
        SetInt(lst[4],(unsigned char)mev.midiData[2]);
        // what about running status? (obviously not possible)
        SetInt(lst[5],mev.deltaFrames);
        SetInt(lst[6],mev.noteLength);
        SetInt(lst[7],mev.noteOffset);
        SetInt(lst[8],(int)mev.detune);
        SetInt(lst[9],(int)mev.noteOffVelocity);
        responder->Respond(sym_event,9,lst);
    }
    else {
        const t_symbol *sym;
        if(ev.type == kVstAudioType)
            sym = sym_evaudio;
        else if(ev.type == kVstVideoType)
            sym = sym_evvideo;
        else if(ev.type == kVstParameterType)
            sym = sym_evparam;
        else if(ev.type == kVstTriggerType)
            sym = sym_evtrigger;
        else if(ev.type == kVstSysExType)
            sym = sym_evsysex;
        else
            sym = sym_ev_;

        int data = ev.byteSize-sizeof(ev.deltaFrames)-sizeof(ev.flags);
        const int stsize = 16;
        t_atom stlst[stsize];
        t_atom *lst = data+3 > stsize?new t_atom[data+3]:stlst;

        SetSymbol(lst[0],sym);
        SetInt(lst[1],ev.deltaFrames);
        SetInt(lst[2],ev.flags);
        for(int i = 0; i < data; ++i) SetInt(lst[3],(unsigned char)ev.data[i]);

        responder->Respond(sym_event,data+3,lst);

        if(lst != stlst) delete[] lst;
    }
}

// Host callback dispatcher
long VSTPlugin::Master(AEffect *effect, long opcode, long index, long value, void *ptr, float opt)
{
    if(opcode != audioMasterGetTime)
  	    FLEXT_LOG6("VST -> host: Eff = 0x%.8X, Opcode = %d, Index = %d, Value = %d, PTR = %.8X, OPT = %.3f\n",(int)effect, opcode,index,value,(int)ptr,opt);

    VSTPlugin *th = effect?(VSTPlugin *)effect->user:NULL;

	switch (opcode) {
    case audioMasterAutomate: // 0
        if(th && th->feedback && th->responder) {
            t_atom lst[2];
            SetInt(lst[0],index);
            SetFloat(lst[1],opt);
            th->responder->Respond(sym_param,2,lst);
        }
        return 0;

    case audioMasterVersion: // 1
        // support VST 2.3
        return 2300;

    case audioMasterCurrentId: // 2
        // set to subplugin id (default 0)
        return uniqueid;

	case audioMasterIdle: // 3
		effect->dispatcher(effect, effEditIdle, 0, 0, NULL, 0.0f);
		return 0;

	case audioMasterPinConnected: // 4
		//! \todo set connection state correctly (if possible..)
		// index=pin, value=0..input, else..output
        FLEXT_LOG2("Pin connected pin=%li conn=%li",index,value);
		return 0; // 0 means connected

	case audioMasterWantMidi: // 6
        FLEXT_LOG1("Want MIDI = %li",value);
		return 0; // VST header says: "currently ignored"

    case audioMasterGetTime: { // 7
        if(!th) return 0;

        static VstTimeInfo time;
		memset(&time,0,sizeof(time));

        // flags
        time.flags = kVstTempoValid|kVstBarsValid|kVstCyclePosValid|kVstPpqPosValid|kVstSmpteValid|kVstTimeSigValid;

        if(th->transchg) { time.flags |= kVstTransportChanged; th->transchg = false; }
        if(th->playing) time.flags |= kVstTransportPlaying;
        if(th->looping) time.flags |= kVstTransportCycleActive;
//        if(th->feedback) time.flags |= kVstAutomationWriting;
            
        time.sampleRate = th->samplerate;
		time.samplePos = th->samplepos;
        time.ppqPos = th->ppqpos;

        time.tempo = th->tempo;
        time.barStartPos = th->barstartpos;
        time.cycleStartPos = th->cyclestartpos;
        time.cycleEndPos = th->cycleendpos;

        time.timeSigNumerator = th->timesignom;
        time.timeSigDenominator = th->timesigden;

        // SMPTE data
        time.smpteOffset = th->smpteoffset;
        time.smpteFrameRate = th->smpterate;

//        time.samplesToNextClock = 0;

        if(value&kVstNanosValid) {
            time.nanoSeconds = flext::GetOSTime()*1.e9;
            time.flags |= kVstNanosValid;
        }

		return (long)&time;
    }

    case audioMasterProcessEvents: { // 8
        // VST event data from plugin
        VstEvents *evs = static_cast<VstEvents *>(ptr);
        if(th) {
            for(int i = 0; i < evs->numEvents; ++i) 
                th->ProcessEvent(*evs->events[i]);
            return 1;
        }
        else 
            return 0;
    }

    case audioMasterSetTime: { // 9
        VstTimeInfo *tminfo = static_cast<VstTimeInfo *>(ptr);
        FLEXT_LOG3("TimeInfo pos=%lf rate=%lf filter=%li",tminfo->samplePos,tminfo->sampleRate,value);
        return 0; // not supported
    }

	case audioMasterTempoAt: // 10
		return 0; // not supported

	case audioMasterGetNumAutomatableParameters: // 11
		return 0; // not supported

    case audioMasterSizeWindow: // 15
        return 0;

    case audioMasterGetSampleRate: // 16
        return 0; // not supported
    case audioMasterGetBlockSize: // 17
        return 0; // not supported

    case audioMasterGetCurrentProcessLevel: // 23
        // return thread state
		return flext::IsThreadRegistered()?1:2;

    case audioMasterGetAutomationState: // 24
//        return th?(th->feedback?2:1):0;
        return 0;

	case audioMasterGetVendorString: // 32
		strcpy((char*)ptr,vendor);
        return 0;

	case audioMasterGetProductString: // 33
		strcpy((char *)ptr,product);
		return 0;

	case audioMasterGetVendorVersion: // 34
		return VST_VERSION;

	case audioMasterCanDo: // 37
    	FLEXT_LOG1("\taudioMasterCanDo PTR = %s",ptr);
        if(!strcmp((char *)ptr,"sendVstEvents"))
            return 1;
        else if(!strcmp((char *)ptr,"sendVstMidiEvent"))
            return 1;
        else if(!strcmp((char *)ptr,"sendVstTimeInfo"))
            return 1; // NOT YET
        else if(!strcmp((char *)ptr,"receiveVstEvents")) 
            return 1;
        else if(!strcmp((char *)ptr,"receiveVstMidiEvent"))
            return 1;
        else if(!strcmp((char *)ptr,"receiveVstTimeInfo"))
            return 1; // NOT YET
        else if(!strcmp((char *)ptr,"reportConnectionChanges"))
            return 0; // \TODO PD has hard times supporting that...
        else if(!strcmp((char *)ptr,"acceptIOChanges"))
            return 0; // \TODO what does this means exactly?
        else if(!strcmp((char *)ptr,"supplyIdle"))
            return 1;
        else if(!strcmp((char *)ptr,"sizeWindow"))
            return 1;
        else if(!strcmp((char *)ptr,"supportShell"))
            return 0; // deprecated - new one is shellCategory
        else if(!strcmp((char *)ptr,"offline"))
            return 0; // not supported
        else if(!strcmp((char *)ptr,"asyncProcessing"))
            return 0; // not supported
        else if(!strcmp((char *)ptr,"shellCategory"))
            return 1; // supported!
        else if(!strcmp((char *)ptr,"editFile"))
            return 0; // not supported
        else if(!strcmp((char *)ptr,"openFileSelector"))
            return 0; // not supported
        else if(!strcmp((char *)ptr,"closeFileSelector"))
            return 0; // not supported
        else if(!strcmp((char *)ptr,"startStopProcess"))
            return 0; // not supported
#ifdef FLEXT_DEBUG
        else
    	    post("Unknown audioMasterCanDo PTR = %s",ptr);
#endif

		return 0; // not supported

	case audioMasterGetLanguage: // 38
		return kVstLangEnglish;

	case audioMasterGetDirectory: // 41
        return (long)(th?th->dllname.c_str():dllloading.c_str());

    case audioMasterUpdateDisplay: // 42
        FLEXT_LOG("UPDATE DISPLAY");
        return 0;

    default:
        FLEXT_LOG1("Unknown opcode %li",opcode);
        return 0;
    }
}

void VSTPlugin::updatepos(long frames)
{
    bool inloop = ppqpos < cycleendpos;

    // \todo should the sample position also jump back when cycling?
    // and if, how?
    samplepos += frames;

    // \todo this factor should be cached
    ppqpos += frames*tempo/(samplerate*60);

    if(looping) {
        double cyclelen = cycleendpos-cyclestartpos;
        if(cyclelen > 0) {
            if(inloop && ppqpos >= cycleendpos) 
                ppqpos = cyclestartpos+fmod(ppqpos-cyclestartpos,cyclelen);
        }
        else
            ppqpos = cyclestartpos;
    }
}
