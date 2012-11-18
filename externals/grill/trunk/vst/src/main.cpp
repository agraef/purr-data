/* 
vst~ - VST plugin object for PD 
based on the work of Jarno Seppänen and Mark Williamson

Copyright (c)2003-2006 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

// needed for CoInitializeEx
#define _WIN32_DCOM

#include "main.h"

#include "editor.h"
#include "vsthost.h"

#include <stdlib.h>
#include <string.h>
#include <string>

#if FLEXT_OS == FLEXT_OS_WIN
#include <direct.h>
#include <io.h>
#include <objbase.h>
#endif


#define VST_VERSION "0.1.0pre27"


class vst
    : public flext_dsp
    , public Responder
{
	FLEXT_HEADER_S(vst,flext_dsp,Setup)

public:
	vst(int argc,const t_atom *argv);
	~vst();

protected:
    virtual bool CbDsp();
    virtual void CbSignal();

    virtual void CbClick() { ms_edit(true); }

    bool ms_plug(int argc,const t_atom *argv);
    bool ms_plug(const AtomList &args) { return ms_plug(args.Count(),args.Atoms()); }
    void mg_plug(AtomList &sym) const { sym(1); SetString(sym[0],plugname.c_str()); }

    void ms_subplug(int argc,const t_atom *argv);
    void ms_subplug(const AtomList &args) { ms_subplug(args.Count(),args.Atoms()); }
    void mg_subplug(AtomList &sym) const { sym(1); SetString(sym[0],subplug.c_str()); }

    void m_pluglist() { if(plug) plug->ListPlugs(sym_pluglist); }

    void mg_editor(bool &ed) { ed = plug && plug->HasEditor(); }

    void ms_edit(bool on) { if(plug) plug->Edit(on); }
    void mg_edit(bool &ed) { ed = plug && plug->IsEdited(); }
    void ms_vis(bool vis) { if(plug) plug->Visible(vis); }
    void mg_vis(bool &vis) { vis = plug && plug->IsVisible(); }

    void mg_winx(int &x) const { x = plug?plug->GetX():0; }
    void mg_winy(int &y) const { y = plug?plug->GetY():0; }
    void mg_winw(int &x) const { x = plug?plug->GetW():0; }
    void mg_winh(int &y) const { y = plug?plug->GetH():0; }
    void ms_winx(int x) { if(plug) plug->SetX(x); }
    void ms_winy(int y) { if(plug) plug->SetY(y); }
    void ms_winw(int x) { if(plug) plug->SetW(x); }
    void ms_winh(int y) { if(plug) plug->SetH(y); }
    void ms_wincaption(bool c) { if(plug) plug->SetCaption(c); }
    void mg_wincaption(bool &c) const { c = plug && plug->GetCaption(); }
    void ms_winhandle(bool c) { if(plug) plug->SetHandle(c); }
    void mg_winhandle(bool &c) const { c = plug && plug->GetHandle(); }
    void ms_wintitle(const AtomList &t);
    void mg_wintitle(AtomList &t) const { if(plug) { t(1); SetString(t[0],plug->GetTitle()); } }
    void m_winfront() const { if(plug) plug->ToFront(); }
    void m_winbelow() const { if(plug) plug->BelowFront(); }

    void mg_chnsin(int &c) const { c = plug?plug->GetNumInputs():0; }
    void mg_chnsout(int &c) const { c = plug?plug->GetNumOutputs():0; }
    void mg_params(int &p) const { p = plug?plug->GetNumParams():0; }
    void mg_programs(int &p) const { p = plug?plug->GetNumPrograms():0; }
    void mg_progcats(int &p) const { p = plug?plug->GetNumCategories():0; }
    void mg_plugname(const t_symbol *&s) const { s = plug?MakeSymbol(plug->GetName()):sym__; }
    void mg_plugvendor(const t_symbol *&s) const { s = plug?MakeSymbol(plug->GetVendorName()):sym__; }
    void mg_plugdll(const t_symbol *&s) const { s = plug?MakeSymbol(plug->GetDllName()):sym__; }
    void mg_plugversion(int &v) const { v = plug?plug->GetVersion():0; }
    void mg_issynth(bool &s) const { s = plug && plug->IsSynth(); }

    void m_print(int ac,const t_atom *av);

    void ms_program(int p) { if(plug && p >= 0) plug->SetCurrentProgram(p); }
    void mg_program(int &p) const { p = plug?plug->GetCurrentProgram():0; }
    void mg_progname(int argc,const t_atom *argv) const;
    
    void m_pname(int pnum);
    void ms_paramnames(int cnt) { paramnames = cnt; if(plug) plug->ScanParams(cnt); }

    void ms_param(int pnum,float val);
    void ms_params(int argc,const t_atom *argv);
    void mg_param(int pnum);
    void mg_params(int argc,const t_atom *argv);
    void m_ptext(int pnum);
    void m_ptexts(int argc,const t_atom *argv);

    void mg_channel(int &chn) const { chn = plug?plug->GetChannel():0; }
    void ms_channel(int chn) { if(plug) plug->SetChannel(chn); }

    void m_note(int note,int vel);
    void m_noteoff(int note) { m_note(note,0); }
    void m_programchange(int ctrl_value) { if(plug) plug->AddProgramChange(ctrl_value); }
    void m_ctrlchange(int control,int ctrl_value) { if(plug) plug->AddControlChange(control,ctrl_value); }
    void m_aftertouch(int ctrl_value) { if(plug) plug->AddAftertouch(ctrl_value); }
    void m_polyaftertouch(int note,int ctrl_value) { if(plug) plug->AddPolyAftertouch(note,ctrl_value); }
    void m_pitchbend(int ctrl_value) { if(plug) plug->AddPitchBend(ctrl_value); }

    void mg_dumpevents(bool &ev) const { ev = plug?plug->GetEvents():false; }
    void ms_dumpevents(bool ev) { if(plug) plug->SetEvents(ev); }

    void mg_playing(bool &p) { p = plug && plug->GetPlaying(); }
    void ms_playing(bool p) { if(plug) plug->SetPlaying(p); }
    void mg_looping(bool &p) { p = plug && plug->GetLooping(); }
    void ms_looping(bool p) { if(plug) plug->SetLooping(p); }
    void mg_feedback(bool &p) { p = plug && plug->GetFeedback(); }
    void ms_feedback(bool p) { if(plug) plug->SetFeedback(p); }
    void mg_samplepos(float &p) { p = plug?(float)plug->GetSamplePos():0; }
    void ms_samplepos(float p) { if(plug) plug->SetSamplePos(p); }
    void mg_ppqpos(float &p) { p = plug?(float)plug->GetPPQPos():0; }
    void ms_ppqpos(float p) { if(plug) plug->SetPPQPos(p); }
    void mg_tempo(float &p) { p = plug?(float)plug->GetTempo():0; }
    void ms_tempo(float p) { if(plug) plug->SetTempo(p); }
    void mg_barstart(float &p) { p = plug?(float)plug->GetBarStart():0; }
    void ms_barstart(float p) { if(plug) plug->SetBarStart(p); }
    void mg_cyclestart(float &p) { p = plug?(float)plug->GetCycleStart():0; }
    void ms_cyclestart(float p) { if(plug) plug->SetCycleStart(p); }
    void mg_cycleend(float &p) { p = plug?(float)plug->GetCycleEnd():0; }
    void ms_cycleend(float p) { if(plug) plug->SetCycleEnd(p); }
    void mg_cyclelength(float &p) { p = plug?(float)(plug->GetCycleEnd()-plug->GetCycleStart()):0; }
    void ms_cyclelength(float p) { if(p) plug->SetCycleEnd(plug->GetCycleStart()+p); }
    void mg_timesignom(int &p) { p = plug?plug->GetTimesigNom():0; }
    void ms_timesignom(int p) { if(plug) plug->SetTimesigNom(p); }
    void mg_timesigden(int &p) { p = plug?plug->GetTimesigDen():0; }
    void ms_timesigden(int p) { if(plug) plug->SetTimesigDen(p); }
    void mg_smpteoffset(int &p) { p = plug?plug->GetSmpteOffset():0; }
    void ms_smpteoffset(int p) { if(plug) plug->SetSmpteOffset(p); }
    void mg_smpterate(int &p) { p = plug?plug->GetSmpteRate():0; }
    void ms_smpterate(int p) { if(plug) plug->SetSmpteRate(p); }

private:
    void display_parameter(int param,bool showparams);

    VSTPlugin *plug;
    std::string plugname,subplug;
    bool visible,bypass,mute;
    int paramnames;

    int blsz;
    bool (VSTPlugin::*vstfun)(t_sample **insigs,t_sample **outsigs,long n);
    bool sigmatch;
    t_sample **vstin,**vstout,**tmpin,**tmpout;

    void InitPlug();
    void ClearPlug();
    bool LoadPlug();
    void InitPlugDSP();
    void InitBuf();
    void ClearBuf();

    static void Setup(t_classid);
	
    virtual void Respond(const t_symbol *sym,int argc = 0,const t_atom *argv = NULL);

    FLEXT_CALLBACK_V(m_print)

    FLEXT_CALLVAR_V(mg_plug,ms_plug)
    FLEXT_CALLVAR_V(mg_subplug,ms_subplug)
    FLEXT_CALLBACK(m_pluglist)

    FLEXT_CALLVAR_B(mg_edit,ms_edit)
    FLEXT_CALLGET_B(mg_editor)
    FLEXT_CALLVAR_B(mg_vis,ms_vis)
    FLEXT_ATTRVAR_B(bypass)
    FLEXT_ATTRVAR_B(mute)

    FLEXT_CALLVAR_I(mg_channel,ms_channel)
    FLEXT_CALLBACK_II(m_note)
    FLEXT_CALLBACK_I(m_noteoff)
    FLEXT_CALLBACK_II(m_ctrlchange)
    FLEXT_CALLBACK_I(m_aftertouch)
    FLEXT_CALLBACK_II(m_polyaftertouch)
    FLEXT_CALLBACK_I(m_pitchbend)

    FLEXT_CALLVAR_B(mg_dumpevents,ms_dumpevents)

    FLEXT_CALLBACK_I(m_programchange)
    FLEXT_CALLVAR_I(mg_program,ms_program)
    FLEXT_CALLBACK_V(mg_progname)

    FLEXT_CALLBACK_I(m_pname)
    FLEXT_ATTRGET_I(paramnames)
    FLEXT_CALLSET_I(ms_paramnames)
    FLEXT_CALLBACK_2(ms_param,int,float)
    FLEXT_CALLBACK_V(ms_params)
    FLEXT_CALLBACK_I(mg_param)
    FLEXT_CALLBACK_V(mg_params)
    FLEXT_CALLBACK_I(m_ptext)
    FLEXT_CALLBACK_V(m_ptexts)

    FLEXT_CALLVAR_I(mg_winx,ms_winx)
    FLEXT_CALLVAR_I(mg_winy,ms_winy)
    FLEXT_CALLVAR_I(mg_winw,ms_winw)
    FLEXT_CALLVAR_I(mg_winh,ms_winh)
    FLEXT_CALLVAR_B(mg_wincaption,ms_wincaption)
    FLEXT_CALLVAR_B(mg_winhandle,ms_winhandle)
    FLEXT_CALLVAR_V(mg_wintitle,ms_wintitle)
    FLEXT_CALLBACK(m_winfront)
    FLEXT_CALLBACK(m_winbelow)

    FLEXT_CALLGET_I(mg_chnsin)
    FLEXT_CALLGET_I(mg_chnsout)
    FLEXT_CALLGET_I(mg_params)
    FLEXT_CALLGET_I(mg_programs)
    FLEXT_CALLGET_I(mg_progcats)
    FLEXT_CALLGET_S(mg_plugname)
    FLEXT_CALLGET_S(mg_plugvendor)
    FLEXT_CALLGET_S(mg_plugdll)
    FLEXT_CALLGET_I(mg_plugversion)
    FLEXT_CALLGET_B(mg_issynth)

    FLEXT_CALLVAR_B(mg_playing,ms_playing)
    FLEXT_CALLVAR_B(mg_looping,ms_looping)
    FLEXT_CALLVAR_B(mg_feedback,ms_feedback)
    FLEXT_CALLVAR_F(mg_samplepos,ms_samplepos)
    FLEXT_CALLVAR_F(mg_ppqpos,ms_ppqpos)
    FLEXT_CALLVAR_F(mg_tempo,ms_tempo)
    FLEXT_CALLVAR_F(mg_barstart,ms_barstart)
    FLEXT_CALLVAR_F(mg_cyclestart,ms_cyclestart)
    FLEXT_CALLVAR_F(mg_cycleend,ms_cycleend)
    FLEXT_CALLVAR_F(mg_cyclelength,ms_cyclelength)
    FLEXT_CALLVAR_I(mg_timesignom,ms_timesignom)
    FLEXT_CALLVAR_I(mg_timesigden,ms_timesigden)
    FLEXT_CALLVAR_I(mg_smpteoffset,ms_smpteoffset)
    FLEXT_CALLVAR_I(mg_smpterate,ms_smpterate)

    static const t_symbol *sym_progname,*sym_pname,*sym_param,*sym_ptext,*sym_pluglist;
};

FLEXT_NEW_DSP_V("vst~",vst);


const t_symbol *vst::sym_progname,*vst::sym_pname,*vst::sym_param,*vst::sym_ptext,*vst::sym_pluglist;

void vst::Setup(t_classid c)
{
    post("");
	post("vst~ %s - VST plugin object, (C)2003-05 Thomas Grill",VST_VERSION);
	post("based on the work of Jarno Seppänen and Mark Williamson");
	post("");
    post("VST PlugIn Technology by Steinberg");
	post("");

	FLEXT_CADDATTR_VAR(c,"plug",mg_plug,ms_plug);
	FLEXT_CADDATTR_VAR(c,"subplug",mg_subplug,ms_subplug);
	FLEXT_CADDMETHOD_(c,0,"getpluglist",m_pluglist);
	FLEXT_CADDATTR_VAR(c,"edit",mg_edit,ms_edit);
	FLEXT_CADDATTR_GET(c,"editor",mg_editor);
	FLEXT_CADDATTR_VAR(c,"vis",mg_vis,ms_vis);
	FLEXT_CADDATTR_VAR1(c,"bypass",bypass);
	FLEXT_CADDATTR_VAR1(c,"mute",mute);
	FLEXT_CADDMETHOD_(c,0,"print",m_print);

	FLEXT_CADDATTR_VAR(c,"channel",mg_channel,ms_channel);
	FLEXT_CADDMETHOD_I(c,0,"noteoff",m_noteoff);
	FLEXT_CADDMETHOD_II(c,0,"note",m_note);
	FLEXT_CADDMETHOD_II(c,0,"patouch",m_polyaftertouch);
	FLEXT_CADDMETHOD_II(c,0,"ctlchg",m_ctrlchange);
	FLEXT_CADDMETHOD_(c,0,"atouch",m_aftertouch);
	FLEXT_CADDMETHOD_(c,0,"pbend",m_pitchbend);

	FLEXT_CADDATTR_VAR(c,"events",mg_dumpevents,ms_dumpevents);

	FLEXT_CADDMETHOD_(c,0,"progchg",m_programchange);
	FLEXT_CADDATTR_VAR(c,"program",mg_program,ms_program);
	FLEXT_CADDMETHOD_(c,0,"getprogname",mg_progname);

	FLEXT_CADDMETHOD_I(c,0,"getpname",m_pname);
	FLEXT_CADDATTR_VAR(c,"pnames",paramnames,ms_paramnames);
	FLEXT_CADDMETHOD_2(c,0,"param",ms_param,int,float);
	FLEXT_CADDMETHOD_(c,0,"param",ms_params);
	FLEXT_CADDMETHOD_(c,0,"getparam",mg_param);
	FLEXT_CADDMETHOD_(c,0,"getparam",mg_params);
	FLEXT_CADDMETHOD_(c,0,"getptext",m_ptext);
	FLEXT_CADDMETHOD_(c,0,"getptext",m_ptexts);

    FLEXT_CADDATTR_VAR(c,"x",mg_winx,ms_winx);
	FLEXT_CADDATTR_VAR(c,"y",mg_winy,ms_winy);
	FLEXT_CADDATTR_VAR(c,"w",mg_winw,ms_winw);
    FLEXT_CADDATTR_VAR(c,"h",mg_winh,ms_winh);
	FLEXT_CADDATTR_VAR(c,"title",mg_wintitle,ms_wintitle);
	FLEXT_CADDATTR_VAR(c,"caption",mg_wincaption,ms_wincaption);
	FLEXT_CADDATTR_VAR(c,"handle",mg_winhandle,ms_winhandle);
	FLEXT_CADDMETHOD_(c,0,"front",m_winfront);
	FLEXT_CADDMETHOD_(c,0,"below",m_winbelow);

    FLEXT_CADDATTR_GET(c,"ins",mg_chnsin);
	FLEXT_CADDATTR_GET(c,"outs",mg_chnsout);
	FLEXT_CADDATTR_GET(c,"params",mg_params);
	FLEXT_CADDATTR_GET(c,"programs",mg_programs);
	FLEXT_CADDATTR_GET(c,"progcats",mg_progcats);
	FLEXT_CADDATTR_GET(c,"name",mg_plugname);
	FLEXT_CADDATTR_GET(c,"vendor",mg_plugvendor);
	FLEXT_CADDATTR_GET(c,"dll",mg_plugdll);
	FLEXT_CADDATTR_GET(c,"version",mg_plugversion);
	FLEXT_CADDATTR_GET(c,"synth",mg_issynth);

	FLEXT_CADDATTR_VAR(c,"playing",mg_playing,ms_playing);
	FLEXT_CADDATTR_VAR(c,"looping",mg_looping,ms_looping);
	FLEXT_CADDATTR_VAR(c,"feedback",mg_feedback,ms_feedback);
	FLEXT_CADDATTR_VAR(c,"samplepos",mg_samplepos,ms_samplepos);
	FLEXT_CADDATTR_VAR(c,"ppqpos",mg_ppqpos,ms_ppqpos);
	FLEXT_CADDATTR_VAR(c,"tempo",mg_tempo,ms_tempo);
	FLEXT_CADDATTR_VAR(c,"barstart",mg_barstart,ms_barstart);
	FLEXT_CADDATTR_VAR(c,"loopstart",mg_cyclestart,ms_cyclestart);
	FLEXT_CADDATTR_VAR(c,"loopend",mg_cycleend,ms_cycleend);
	FLEXT_CADDATTR_VAR(c,"looplength",mg_cyclelength,ms_cyclelength);
	FLEXT_CADDATTR_VAR(c,"timenom",mg_timesignom,ms_timesignom);
	FLEXT_CADDATTR_VAR(c,"timeden",mg_timesigden,ms_timesigden);
	FLEXT_CADDATTR_VAR(c,"smpteoffset",mg_smpteoffset,ms_smpteoffset);
	FLEXT_CADDATTR_VAR(c,"smpterate",mg_smpterate,ms_smpterate);

    sym_progname = MakeSymbol("progname");
    sym_pname = MakeSymbol("pname");
    sym_param = MakeSymbol("param");
    sym_ptext = MakeSymbol("ptext");
    sym_pluglist = MakeSymbol("pluglist");

    VSTPlugin::Setup();
    SetupEditor();
}

static int corefs = 0;

vst::vst(int argc,const t_atom *argv):
    plug(NULL),visible(false),
    blsz(0),
    vstfun(NULL),vstin(NULL),vstout(NULL),tmpin(NULL),tmpout(NULL),
    bypass(false),mute(false),paramnames(0)
{
#if FLEXT_OS == FLEXT_OS_WIN
    // this is necessary for Waveshell
    if(!corefs++) CoInitializeEx(NULL,COINIT_MULTITHREADED+COINIT_SPEED_OVER_MEMORY);
#endif

    int ins = 1,outs = 1;
    if(argc >= 1 && CanbeInt(argv[0])) { ins = GetAInt(argv[0]); argc--,argv++; }
    if(argc >= 1 && CanbeInt(argv[0])) { outs = GetAInt(argv[0]); argc--,argv++; }

    AddInSignal(ins);
    AddOutSignal(outs);     

    if(argc >= 1 && !ms_plug(argc,argv)) InitProblem();
}

vst::~vst()
{
    ClearPlug();
#if FLEXT_OS == FLEXT_OS_WIN
    if(!--corefs) CoUninitialize();
#endif
}

void vst::ClearPlug()
{
    if(plug) {
        ClearBuf(); // needs valid plug
        VSTPlugin::Delete(plug); 
        plug = NULL;
    }
}

void vst::InitPlug()
{
    FLEXT_ASSERT(plug);

	vstfun = plug->IsReplacing()?&VSTPlugin::processReplacing:&VSTPlugin::process;
    sigmatch = plug->GetNumInputs() == CntInSig() && plug->GetNumOutputs() == CntOutSig();
    InitPlugDSP();

    InitBuf();

    plug->ScanParams(paramnames);
}

void vst::InitPlugDSP()
{
    FLEXT_ASSERT(plug);
    // this might be invalid if DSP is switched off, 
    // but the plug will get updated settings with m_dsp later
    plug->DspInit(Samplerate(),Blocksize());
}

void vst::ClearBuf()
{
    if(!plug) return;

    if(vstin) {
        for(int i = 0; i < plug->GetNumInputs(); ++i) FreeAligned(vstin[i]);
        delete[] vstin; vstin = NULL;
        delete[] tmpin; tmpin = NULL;
    }
    if(vstout) {
        for(int i = 0; i < plug->GetNumOutputs(); ++i) FreeAligned(vstout[i]);
        delete[] vstout; vstout = NULL;
        delete[] tmpout; tmpout = NULL;
    }
}

void vst::InitBuf()
{
    FLEXT_ASSERT(!vstin && !tmpin && !vstout && !tmpout);
    const int inputs = plug->GetNumInputs(),outputs = plug->GetNumOutputs();

    int i;

    vstin = new t_sample *[inputs];
    tmpin = new t_sample *[inputs];
    for(i = 0; i < inputs; ++i) vstin[i] = (t_sample *)NewAligned(Blocksize()*sizeof(t_sample));
    
    vstout = new t_sample *[outputs];
    tmpout = new t_sample *[outputs];
    for(i = 0; i < outputs; ++i) vstout[i] = (t_sample *)NewAligned(Blocksize()*sizeof(t_sample));
}

static std::string findFilePath(const std::string &path,const std::string &dllname)
{
#if FLEXT_OS == FLEXT_OS_WIN
	_chdir( path.c_str() );
    WIN32_FIND_DATA data;
    HANDLE fh = FindFirstFile(dllname.c_str(),&data);
    if(fh != INVALID_HANDLE_VALUE) {
        FindClose(fh);
        return path;
    }
#endif
/*
	CFileFind finder;
	if(finder.FindFile( dllname ))
		return path;
	else {
		finder.FindFile();
		while(finder.FindNextFile()) {
			if(finder.IsDirectory()) {
				if(!finder.IsDots()) {
					CString *npath = new CString( finder.GetFilePath()); 
					const C *ret = findFilePath( *npath , dllname );
					if(ret) {
						CString *retstr = new CString(ret);
						return *retstr;
					}
				}
			}
		}
	}
*/
   
    return std::string();
}

// \todo this should be in the background, because it can take some time
// ideally vst would get a response from VSTPlugin when readily loaded and
// vst would dump out a respective signal to the patcher
bool vst::LoadPlug()
{
    if(plug) ClearPlug();

    VSTPlugin *p = VSTPlugin::New(this);

	// try loading the dll from the raw filename 
    bool ok = p->Instance(plugname.c_str(),subplug.c_str());
	if(ok) 
        FLEXT_LOG("raw filename loaded fine");
    else { 
#if FLEXT_SYS == FLEXT_SYS_PD
        // try finding it on the PD path
	    char *name,dir[1024];
	    int fd = open_via_path("",plugname.c_str(),".dll",dir,&name,sizeof(dir)-1,0);
	    if(fd > 0) close(fd);
	    else name = NULL;

        if(name) {
    		FLEXT_LOG("found VST dll on the PD path");
	        // if dir is current working directory... name points to dir
	        if(dir == name) strcpy(dir,".");
        
            std::string dllname(dir);
            dllname += "\\";
            dllname += name;

	        ok = p->Instance(dllname.c_str());
        }
#endif
    }

    if(!ok) { 
        // try finding it on the VST path
		char *vst_path = getenv("VST_PATH");

		std::string dllname(plugname);
		if(dllname.find(".dll") == -1) dllname += ".dll";			

		if(vst_path) {
    		FLEXT_LOG("found VST_PATH env variable");
            char* tok_path = new char[strlen( vst_path)+1];
            strcpy( tok_path , vst_path);
			char *tok = strtok( tok_path , ";" );
			while( tok != NULL ) {
				std::string abpath( tok );
				if( abpath[abpath.length()-1] != '\\' ) abpath += "\\";

        		FLEXT_LOG1("trying VST_PATH %s",(const char *)abpath.c_str());

				std::string realpath = findFilePath( abpath , dllname );				
				//post( "findFilePath( %s , %s ) = %s\n" , abpath , dllname , realpath );
				if ( realpath.length() ) {
					realpath += plugname;
            		FLEXT_LOG1("trying %s",(const char *)realpath.c_str());

                    ok = p->Instance(realpath.c_str());
					if(ok) {
                		FLEXT_LOG("plugin loaded via VST_PATH");
						break;
					}
				}

				tok = strtok( NULL , ";" );
//                if(!tok) post("%s - couldn't find plugin",thisName());
			}

            delete[] tok_path;
		}
	}

    if(!ok) {
		post("%s - unable to load plugin '%s'",thisName(),plugname.c_str());
        VSTPlugin::Delete(p);
    }
    else {
        plug = p;
        InitPlug();
    }

    return ok;
}

static char *stripesc(char *buf)
{
#if FLEXT_SYS == FLEXT_SYS_PD
    // strip char escapes (only in newer/devel PD version)
    char *cs = buf,*cd = cs;
    while(*cs) {
        if(*cs != '\\') *(cd++) = *cs;
        ++cs;
    }
    *cd = 0;
#endif
    return buf;
}

bool vst::ms_plug(int argc,const t_atom *argv)
{
    ClearPlug();

    plugname.clear();
	char buf[255];	
	for(int i = 0; i < argc; i++) {
		if(i > 0) plugname += ' ';
		GetAString(argv[i],buf,sizeof buf);
#if FLEXT_OS == FLEXT_OS_WIN
        strlwr(buf);
#endif
		plugname += stripesc(buf);
	}

    if(!plugname.length()) 
        return false;
    else
        return LoadPlug();
}

void vst::ms_subplug(int argc,const t_atom *argv)
{
    subplug.clear();
	char buf[255];	
	for(int i = 0; i < argc; i++) {
		if(i > 0) subplug += ' ';
		GetAString(argv[i],buf,sizeof buf);
		subplug += stripesc(buf);
	}

    LoadPlug();
}

bool vst::CbDsp()
{
    if(plug) {
        FLEXT_ASSERT(vstfun);

        InitPlugDSP();

        if(blsz != Blocksize()) {
            blsz = Blocksize();
            ClearBuf();
            InitBuf();
        }
    }
    return true;
}

void vst::CbSignal()
{
    if(!plug || !plug->UniqueID() || mute)
        flext_dsp::CbSignal();
    else if(bypass) {
        const int n = Blocksize();
        t_sample *const *insigs = InSig();
        t_sample *const *outsigs = OutSig();

        // copy as many channels as possible and zero dangling ones

        int i,mx = CntInSig();
        if(mx > CntOutSig()) mx = CntOutSig();
        if(mx == 1) {
            CopySamples(outsigs[0],insigs[0],n);
            i = 1;
        }
        else if(mx == 2) {
            t_sample *o1 = outsigs[0],*o2 = outsigs[1];
            const t_sample *i1 = insigs[0],*i2 = insigs[1];
            for(int s = 0; s < n; ++s) {
                const t_sample f = *(i1++);
                *(o2++) = *(i2++);
                *(o1++) = f;
            }
            i = 2;
        }
        else
            for(i = 0; i < mx; ++i) {
                // must copy via temporary buffer as ordering of output signals can collide with input signals
                CopySamples(tmpin[i],insigs[i],n);
                CopySamples(outsigs[i],tmpin[i],n);
            }

        for(; i < CntOutSig(); ++i)
            ZeroSamples(outsigs[i],n);
    }
    else if(sigmatch) {
        if(!(plug->*vstfun)(const_cast<t_sample **>(InSig()),const_cast<t_sample **>(OutSig()),Blocksize())) {
            for(int i = 0; i < CntOutSig(); ++i)
                ZeroSamples(OutSig()[i],Blocksize());
        }
    }
    else {
        const int inputs = plug->GetNumInputs(),outputs = plug->GetNumOutputs();
        const int cntin = CntInSig(),cntout = CntOutSig();
        const int n = Blocksize();
        t_sample *const *insigs = InSig();
        t_sample *const *outsigs = OutSig();
        t_sample **inv,**outv;

        if(inputs <= cntin) 
            inv = const_cast<t_sample **>(insigs);
        else { // more plug inputs than inlets
            int i;
            for(i = 0; i < cntin; ++i) tmpin[i] = const_cast<t_sample *>(insigs[i]);

            // set dangling inputs to zero
            // according to mode... (e.g. set zero)
            for(; i < inputs; ++i) ZeroSamples(tmpin[i] = vstin[i],n);

            inv = tmpin;
        }

        const bool more = outputs <= cntout;
        if(more) // more outlets than plug outputs 
            outv = const_cast<t_sample **>(outsigs);
        else {
            int i;
            for(i = 0; i < cntout; ++i) tmpout[i] = outsigs[i];
            for(; i < outputs; ++i) tmpout[i] = vstout[i];

            outv = tmpout;
        }

        // call plugin DSP function
        if(!(plug->*vstfun)(inv,outv,n)) {
            for(int i = 0; i < outputs; ++i)
                ZeroSamples(outsigs[i],n);
        }

        if(more) {
            // according to mode set dangling output vectors

            // currently simply clear them....
            for(int i = outputs; i < cntout; ++i)
                ZeroSamples(outsigs[i],n);
        }
    }
}

void vst::mg_progname(int argc,const t_atom *argv) const
{
    if(plug) {
        int cat,pnum;
        if(argc == 1 && CanbeInt(argv[0])) {
            cat = -1,pnum = GetAInt(argv[0]);
        }
        else if(argc == 2 && CanbeInt(argv[0]) && CanbeInt(argv[1])) {
            cat = GetAInt(argv[0]),pnum = GetAInt(argv[1]);
        }
        else pnum = -1;

        if(pnum >= 0) {
            char str[256];
            plug->GetProgramName(cat,pnum,str);

            t_atom at[3];
            SetInt(at[0],cat);
            SetInt(at[1],pnum);
            SetString(at[2],str);
	        ToOutAnything(GetOutAttr(),sym_progname,3,at);
        }
        else 
            post("%s - Syntax: %s [category] program",thisName(),GetString(thisTag()));
    }
}

void vst::ms_wintitle(const AtomList &t) 
{ 
    if(plug) {
        char txt[256];
        t.Print(txt,sizeof txt);
        plug->SetTitle(txt); 
    }
}

 /**
 *	display the parameters names and values and some other bits and pieces that 
 *	may be of use
 */

void vst::m_print(int ac,const t_atom *av) 
{
    if(!plug) return;

    int i;
	bool params = false;
	bool header = true;
	bool programs = false;
	bool parameters = true;
	int specific = -1;
    if( ac > 0 ) {
		for( i = 0 ; i < ac ; i++) {
			if(IsString(av[i])) {
				const char *buf = GetString(av[i]);	
				if ( strcmp( buf , "-params" ) == 0 ) {
					params = true;
				}
				else if ( strcmp( buf , "-noheader" ) == 0 ) {
					header = false;
				}
				else if ( strcmp( buf , "-programs" ) == 0 ) {
					programs = true;
					parameters = false;
				}
				else if ( strcmp( buf , "-parameters" ) == 0 ) {				
					parameters = false;
				}
				else if ( strcmp( buf , "-help" ) == 0 ) {
					post("print options:");
					post("-help \t\tprint this");
					post("-programs \tshow the programs");
					post("-parameters \tshow the parameters");
					post("-params \tshow the parameter display values");
					post("-noheader \tdo not display the header");
					return;
				}
			}
			else if(CanbeInt(av[i])) {
				int p = GetAInt(av[i]);
				if (( p > 0 ) && ( p <=  plug->GetNumParams())) {
					specific = p - 1;
				}
			}
		}
	 }

	 if ( header ) {
		post("VST~ plugin: %s ", plug->GetName() );
		post("made by: %s ", plug->GetVendorName() );
		post("parameters %d\naudio: %d in(s)/%d out(s) \nLoaded from library \"%s\".\n",
			plug->GetNumParams(),
			CntInSig(),
			CntOutSig(),
			plug->GetDllName());

		post("Flags");
		if ( plug->HasEditor() ) post("Has editor");
		if ( plug->IsReplacing() ) post("Can do replacing");
	 }

	 if ( parameters ) {
		if ( specific == -1) {
			for (i = 0; i < plug->GetNumParams(); i++) 
				display_parameter( i , params );	
		}
		else
			display_parameter( specific , params);	
	 }

	 if( programs ) {
		for( int j = 0; j < plug->GetNumCategories() ; j++ ) {
			for( i = 0 ; i < plug->GetNumParams() ; i++ ) {
				char buf[64];
				plug->GetProgramName( j , i , buf );
				post("Program %d: %s ", i , buf );
			}
		}
	 }
}


void vst::display_parameter(int param,bool showparams)
{
	int j = param;
	/* the Steinberg(tm) way... */
	char name[109];
	char display[164];
	float val;

//	if(j == 0) post ("Control input/output(s):");

    memset (name, 0, sizeof(name));
	memset( display, 0 ,sizeof(display));
	plug->GetParamName( j , name );

	if(*name) {
		if (showparams) {
//			plug->DescribeValue( j , display );
            plug->GetParamValue(j,display);
			val = plug->GetParamValue( j );
			post ("parameter[#%d], \"%s\" value=%f (%s) ", j, name,  val,display);			
		}
		else {
			val = plug->GetParamValue( j );
			post ("parameter[#%d], \"%s\" value=%f ", j, name,  val);			
		}
	}
}

void vst::m_pname(int pnum)
{
    if(!plug || pnum < 0 || pnum >= plug->GetNumParams()) return;

	char name[256]; // how many chars needed?
	plug->GetParamName(pnum,name);

    t_atom at[2];
    SetInt(at[0],pnum);
    SetString(at[1],name);
	ToOutAnything(GetOutAttr(),sym_pname,2,at);
}

// set the value of a parameter
void vst::ms_param(int pnum,float val)     
{
    if(!plug || pnum < 0 || pnum >= plug->GetNumParams()) return;

//	float xval = plug->GetParamValue( pnum );
//    if(xval <= 1.0f) // What's that????
    if(true)
    { 
		plug->SetParamFloat( pnum, val );
//		if(echoparam) display_parameter(pnum , true );
	}	
    else
        FLEXT_ASSERT(false);
}

void vst::ms_params(int argc,const t_atom *argv)
{
    if(plug) {
        char str[255]; *str = 0;
        if(argc && CanbeFloat(argv[argc-1]))
            PrintList(argc-1,argv,str,sizeof str);

        if(*str) {
            int ix = plug->GetParamIx(str);
            if(ix >= 0)
                ms_param(ix,GetAFloat(argv[argc-1]));
            else
                post("%s %s - Parameter not found",thisName(),GetString(thisTag()),str);
        }
        else
            post("%s - Syntax: %s name value",thisName(),GetString(thisTag()));
    }
}

void vst::mg_param(int pnum)
{
    if(!plug || pnum < 0 || pnum >= plug->GetNumParams()) return;

    t_atom at[2];
    SetInt(at[0],pnum);
    SetFloat(at[1],plug->GetParamValue(pnum));
	ToOutAnything(GetOutAttr(),sym_param,2,at);
}

void vst::mg_params(int argc,const t_atom *argv)
{
    if(plug) {
        char str[255];
        PrintList(argc,argv,str,sizeof str);

        if(*str) {
            int ix = plug->GetParamIx(str);
            if(ix >= 0)
                mg_param(ix);
            else
                post("%s %s - Parameter not found",thisName(),GetString(thisTag()),str);
        }
        else
            post("%s - Syntax: %s name value",thisName(),GetString(thisTag()));
    }
}

void vst::m_ptext(int pnum)
{
    if(!plug || pnum < 0 || pnum >= plug->GetNumParams()) return;

	char display[256]; // how many chars needed?
	memset(display,0,sizeof(display));
	plug->GetParamValue(pnum,display);

    t_atom at[2];
    SetInt(at[0],pnum);
    SetString(at[1],display);
	ToOutAnything(GetOutAttr(),sym_ptext,2,at);
}

void vst::m_ptexts(int argc,const t_atom *argv)
{
    if(plug) {
        char str[255];
        PrintList(argc,argv,str,sizeof str);

        if(*str) {
            int ix = plug->GetParamIx(str);
            if(ix >= 0)
                m_ptext(ix);
            else
                post("%s %s - Parameter not found",thisName(),GetString(thisTag()),str);
        }
        else
            post("%s - Syntax: %s name value",thisName(),GetString(thisTag()));
    }
}

void vst::m_note(int note,int velocity)
{
    if(!plug) return;

	if(velocity > 0)
		plug->AddNoteOn(note,velocity);
	else
		plug->AddNoteOff(note);
}

void vst::Respond(const t_symbol *sym,int argc,const t_atom *argv)
{
    FLEXT_ASSERT(sym);
    ToOutAnything(GetOutAttr(),sym,argc,argv);
}
