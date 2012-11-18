/* 
vst~ - VST plugin object for PD 
based on the work of Jarno Seppänen and Mark Williamson

Copyright (c)2003-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "vsthost.h"
#include "editor.h"
#include <exception>
#include "flcontainers.h"

const t_symbol 
    *VSTPlugin::sym_param,
    *VSTPlugin::sym_event,
    *VSTPlugin::sym_evmidi,
    *VSTPlugin::sym_evaudio,
    *VSTPlugin::sym_evvideo,
    *VSTPlugin::sym_evparam,
    *VSTPlugin::sym_evtrigger,
    *VSTPlugin::sym_evsysex,
    *VSTPlugin::sym_ev_,
    *VSTPlugin::sym_midi[8];


class DelPlugin
    : public LifoCell
{
public:
    DelPlugin(VSTPlugin *p): plug(p) {}
    VSTPlugin *plug;
};

static TypedLifo<DelPlugin> todel;
flext::ThrCond VSTPlugin::thrcond;

void VSTPlugin::Setup()
{
    LaunchThread(worker);

    sym_param = flext::MakeSymbol("param");
    sym_event = flext::MakeSymbol("event");
    sym_evmidi = flext::MakeSymbol("midi");
    sym_evaudio = flext::MakeSymbol("audio");
    sym_evvideo = flext::MakeSymbol("video");
    sym_evparam = flext::MakeSymbol("param");
    sym_evtrigger = flext::MakeSymbol("trigger");
    sym_evsysex = flext::MakeSymbol("sysex");
    sym_ev_ = flext::MakeSymbol("???");

    sym_midi[0] = flext::MakeSymbol("noteoff");
    sym_midi[1] = flext::MakeSymbol("note");
    sym_midi[2] = flext::MakeSymbol("atouch");
    sym_midi[3] = flext::MakeSymbol("ctlchg");
    sym_midi[4] = flext::MakeSymbol("progchg");
    sym_midi[5] = flext::MakeSymbol("atouch");
    sym_midi[6] = flext::MakeSymbol("pbend");
    sym_midi[7] = flext::MakeSymbol("sysex");
}

VSTPlugin::VSTPlugin(Responder *resp)
    : effect(NULL),pluginmain(NULL),audiomaster(NULL)
#if FLEXT_OS == FLEXT_OS_WIN
    , hdll(NULL)
#endif
    , hwnd(NULL)
    , responder(resp)
    , posx(0),posy(0),sizex(0),sizey(0)
    , visible(true),caption(true),handle(false)
    , midichannel(0),eventqusz(0),dumpevents(false)
    , paramnamecnt(0)
    , transchg(true)
    , playing(false),looping(false),feedback(false)
    , samplerate(0)
    , samplepos(0),ppqpos(0)
    , tempo(120)
    , timesignom(4),timesigden(4)
    , barstartpos(0)
    , cyclestartpos(0),cycleendpos(0)
    , smpteoffset(0),smpterate(0)
{}

VSTPlugin::~VSTPlugin()
{
	Free();
}

VSTPlugin *VSTPlugin::New(Responder *resp)
{
    FLEXT_ASSERT(resp);
    return new VSTPlugin(resp);
}

void VSTPlugin::Delete(VSTPlugin *p)
{
    FLEXT_ASSERT(p);

    // tell plugin to close editor!
    StopEditor(p);
    // transfer to deletion thread
    todel.Push(new DelPlugin(p));
    thrcond.Signal();
}

void VSTPlugin::worker(thr_params *)
{
    TypedLifo<DelPlugin> tmp;
    bool again = false;
    for(;;) {
        // wait for signal
        if(again) {
            thrcond.TimedWait(0.01);
            again = false;
        }
        else
            thrcond.Wait();

        DelPlugin *p;
        while((p = todel.Pop()) != NULL) {
            // see if editing has stopped
            if(p && p->plug->hwnd == NULL) {
                // yes, it is now safe to delete the plug
#ifdef FLEXT_LOGGING
                post("DELETE %s",p->plug->dllname.c_str());
#endif
                delete p->plug;
                delete p;
            }
            else {
                tmp.Push(p);
                again = true;
            }
        }

        // put back remaining entries
        while((p = tmp.Pop()) != NULL) todel.Push(p);
    }
}
    
#if FLEXT_OS == FLEXT_OS_MAC
OSStatus FSPathMakeFSSpec(const UInt8 *path,FSSpec *spec,Boolean *isDirectory)  /* can be NULL */
{
    OSStatus  result;
    FSRef    ref;

    /* check parameters */
    require_action(NULL != spec, BadParameter, result = paramErr);

    /* convert the POSIX path to an FSRef */
    result = FSPathMakeRef(path, &ref, isDirectory);
    require_noerr(result, FSPathMakeRef);

    /* and then convert the FSRef to an FSSpec */
    result = FSGetCatalogInfo(&ref, kFSCatInfoNone, NULL, NULL, spec, NULL);
    require_noerr(result, FSGetCatalogInfo);
  
FSGetCatalogInfo:
FSPathMakeRef:
BadParameter:
    return result;
}
#endif

// hdll, pluginmain and audiomaster are set here
// must be NULL beforehand!
bool VSTPlugin::NewPlugin(const char *plugname)
{
    FLEXT_ASSERT(!pluginmain && !audiomaster);

    dllname = plugname;

#if FLEXT_OS == FLEXT_OS_WIN
    hdll = LoadLibraryEx(dllname.c_str(),NULL,0 /*DONT_RESOLVE_DLL_REFERENCES*/);
/*
    char buf[255],*c;
    strcpy(buf,dllname.c_str());
    for(c = buf; *c; ++c) 
        if(*c == '/') 
            *c = '\\';
    char *sl = strrchr(buf,'\\');
    if(sl) *sl = 0;
    SetCurrentDirectory(buf);
    hdll = LoadLibrary(dllname.c_str());
*/
    if(hdll) pluginmain = (PVSTMAIN)GetProcAddress(hdll,"main");
    audiomaster = Master;  

#elif FLEXT_OS == FLEXT_OS_MAC

#if 1
	CFStringRef fileNameString = CFStringCreateWithCString(NULL, fileName, kCFStringEncodingUTF8);
	if(fileNameString == 0) goto bail;
	CFURLRef url = CFURLCreateWithFileSystemPath(NULL, fileNameString, kCFURLPOSIXPathStyle, false);
	CFRelease(fileNameString);
	if(url == 0) goto bail;
	hdll = CFBundleCreate(NULL, url);
	CFRelease (url);
	if(hdll && !CFBundleLoadExecutable(hdll)) goto bail;

    PVSTMAIN mainaddr = PluginEntryProc)CFBundleGetFunctionPointerForName(hdll, CFSTR("VSTPluginMain"));
	if(!mainaddr)
		mainaddr = (PluginEntryProc)CFBundleGetFunctionPointerForName(hdll, CFSTR("main_macho"));
#ifdef __CFM__
    pluginmain = (PVSTMAIN)NewMachOFromCFM(mainaddr);
    audiomaster = NewCFMFromMachO(Master);
#else
    pluginmain = (PVSTMAIN)mainaddr;
    audiomaster = Master;
#endif

#else
    short   resFileID;
    FSSpec  spec;
    OSErr err;

    err = FSPathMakeFSSpec(dllname.c_str(),&spec,NULL);
    resFileID = FSpOpenResFile(&spec, fsRdPerm);
    short cResCB = Count1Resources('aEff');

    for(int i = 0; i < cResCB; i++) {
        Handle             codeH;
        CFragConnectionID  connID;
        Ptr                mainAddr;
        Str255             errName;
        Str255             fragName;
        char               fragNameCStr[256];
        short              resID;
        OSType             resType;

        codeH = Get1IndResource('aEff', short(i+1));
        if(!codeH) continue;

        GetResInfo(codeH, &resID, &resType, fragName);
        DetachResource(codeH);
        HLock(codeH);

        err = GetMemFragment(*codeH,
                             GetHandleSize(codeH),
                             fragName,
                             kPrivateCFragCopy,
                             &connID, (Ptr *) & mainAddr, errName);

        if(!err) {
           #ifdef __CFM__
           pluginmain = (PVSTMAIN)NewMachOFromCFM(mainAddr);
           #else
           pluginmain = (PVSTMAIN)mainAddr;
           #endif
        }
    }
    CloseResFile(resFileID);

    audiomaster = 
#ifdef __CFM__
        NewCFMFromMachO(Master);
#else
        Master;
#endif

#endif

#else
#error Platform not supported
#endif    

bail:
    if(pluginmain && audiomaster)
        return true;
    else {
        FreePlugin();
        return false;
    }
}

void VSTPlugin::FreePlugin()
{
#if FLEXT_OS == FLEXT_OS_WIN
    if(hdll) { FreeLibrary(hdll); hdll = NULL; }
#elif FLEXT_OS == FLEXT_OS_MAC
#ifdef __CFM__
    if(audiomaster) DisposeCFMFromMachO(audiomaster);
    if(pluginmain) DisposeMachOFromCFM(pluginmain);
#endif
    if(hdll) {
	    CFBundleUnloadExecutable(hdll);
	    CFRelease(hdll);
        hdll = NULL;
    }
#else
#error Platform not supported
#endif    

    effect = NULL;
    audiomaster = NULL;
    pluginmain = NULL;
} 

/*
This is static to be able to communicate between the plugin methods 
and the static Audiomaster function
the this (plugin->user) pointer has not been initialized at the point it is needed
static should not be a problem, as we are single-threaded and it is immediately 
queried in a called function
*/
long VSTPlugin::uniqueid = 0;

std::string VSTPlugin::dllloading;

bool VSTPlugin::InstPlugin(long plugid)
{
    uniqueid = plugid;
    dllloading = dllname;

    FLEXT_ASSERT(pluginmain && audiomaster);

	//This calls the "main" function and receives the pointer to the AEffect structure.
    try { effect = pluginmain(audiomaster); }
    catch(std::exception &e) {
        flext::post("vst~ - caught exception while instantiating plugin: %s",e.what());
    }
    catch(...) {
        flext::post("vst~ - caught exception while instantiating plugin");
    }

    if(!effect) 
        return false;
    else if(effect->magic != kEffectMagic) {
		effect = NULL; 
	    return false;
    }
    return true;
}

bool VSTPlugin::Instance(const char *name,const char *subname)
{
    bool ok = false;
    FLEXT_ASSERT(effect == NULL);
    
    try {

/*
    if(!ok && dllname != name) {
        FreePlugin();
        // freshly load plugin
        ok = NewPlugin(name) && InstPlugin();
    }
*/
    ok = NewPlugin(name) && InstPlugin();

    if(ok && subname && *subname && Dispatch(effGetPlugCategory) == kPlugCategShell) {
        // sub plugin-name given -> scan plugs

        long plugid;
	    char tmp[64];

        // Waves5 continues with the next plug after the last loaded
        // that's not what we want - workaround: swallow all remaining
        while((plugid = Dispatch(effShellGetNextPlugin,0,0,tmp))) {}

        // restart from the beginning
	    while((plugid = Dispatch(effShellGetNextPlugin,0,0,tmp))) { 
		    // subplug needs a name
            FLEXT_LOG1("subplug %s",tmp);
            if(!strcmp(subname,tmp)) 
                // found
                break;
	    }

        // re-init with plugid set
        if(plugid) ok = InstPlugin(plugid);
    }

    if(ok) {
	    //init plugin 
	    effect->user = this;
	    ok = Dispatch(effOpen) == 0;
    }

    if(ok) {
	    ok = Dispatch(effIdentify) == 'NvEf';
    }

    if(ok) {
        *productname = 0;
	    long ret = Dispatch(effGetProductString,0,0,productname);

        if(!*productname) {
		    // no product name given by plugin -> extract it from the filename

		    std::string str1(dllname);
		    std::string::size_type slpos = str1.rfind('\\');
		    if(slpos == std::string::npos) {
			    slpos = str1.rfind('/');
			    if(slpos == std::string::npos)
				    slpos = 0;
			    else
				    ++slpos;
		    }
		    else
			    ++slpos;
		    std::string str2 = str1.substr(slpos);
		    int snip = str2.find('.');
            if( snip != std::string::npos )
			    str1 = str2.substr(0,snip);
		    else
			    str1 = str2;
		    strcpy(productname,str1.c_str());
	    }
    	
        if(*productname) {
            char tmp[512];
            sprintf(tmp,"vst~ - %s",productname);
            title = tmp;
        }
        else
            title = "vst~";

	    *vendorname = 0;
	    Dispatch(effGetVendorString,0,0,vendorname);
    }

    }
    catch(std::exception &e) {
        flext::post("vst~ - caught exception while loading plugin: %s",e.what());
        ok = false;
    }
    catch(...) {
        flext::post("vst~ - Caught exception while loading plugin");
        ok = false;
    }

    if(!ok) Free();
	return ok;
}

void VSTPlugin::Free() 
{
    // This should only also in destruction

    try {
	    if(effect) {
            FLEXT_ASSERT(!IsEdited());

            // shut down plugin
		    Dispatch(effMainsChanged, 0, 0);
		    Dispatch(effClose);
        }
    }
    catch(...) {}

    FreePlugin(); 
}

void VSTPlugin::DspInit(float sr,int blsz)
{
    try {
        // sample rate and block size must _first_ be set
	    Dispatch(effSetSampleRate,0,0,NULL,samplerate = sr);
	    Dispatch(effSetBlockSize, 0,blsz);
        // then signal that mains have changed!
        Dispatch(effMainsChanged,0,1);
    }
    catch(std::exception &e) {
        flext::post("vst~ - caught exception while initializing dsp: %s",e.what());
    }
    catch(...) {
        flext::post("vst~ - caught exception while initializing dsp");
    }
}

void VSTPlugin::ListPlugs(const t_symbol *sym) const
{
    if(responder) {
        if(Is() && Dispatch(effGetPlugCategory) == kPlugCategShell) {
            t_atom at;
            // sub plugin-name given -> scan plugs
	        char tmp[64];
	        // scan shell for subplugins
            while(Dispatch(effShellGetNextPlugin,0,0,tmp)) {
                SetString(at,tmp);
                responder->Respond(sym,1,&at);
            }
        }

        // bang
        responder->Respond(sym);
    }
}
