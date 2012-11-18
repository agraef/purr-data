#include "stdafx.h"
#include "m_pd.h"
#include "EditorThread.h"
#include "VstHost.h"
#include "PopupWindow.h"
#include "vst\aeffeditor.h"
#include "vst\aeffectx.h"


VstTimeInfo VSTPlugin::_timeInfo;

float VSTPlugin::sample_rate = 44100;


////////////////////
//
/////////////////////
VSTPlugin::VSTPlugin()
{
	queue_size=0;
	_sDllName = NULL;
	h_dll=NULL;
	instantiated=false;		// Constructin' with no instance
	overwrite = false;
	 w = GetForegroundWindow();
	 show_params = false;
	 _midichannel = 0;
	 edited = false;
}

VSTPlugin::~VSTPlugin()
{
	Free();				// Call free
	delete _sDllName;	// if _sDllName = NULL , the operation does nothing -> it's safe.
}
 
int VSTPlugin::Instance( const char *dllname)
{
	h_dll=LoadLibrary(dllname);

	if(h_dll==NULL)	
	{
		return VSTINSTANCE_ERR_NO_VALID_FILE;
	}
	post("Loaded library %s" , dllname);
	PVSTMAIN main = (PVSTMAIN)GetProcAddress(h_dll,"main");
	if(!main)
	{	
		FreeLibrary(h_dll);
		_pEffect=NULL;
		instantiated=false;
		return VSTINSTANCE_ERR_NO_VST_PLUGIN;
	}
	//post("Found main function - about to call it");
	//This calls the "main" function and receives the pointer to the AEffect structure.
	_pEffect = main((audioMasterCallback)&(this->Master));
	
	if(!_pEffect)
	{
		post("VST plugin : unable to create effect");
		FreeLibrary(h_dll);
		_pEffect=NULL;
		instantiated=false;
		return VSTINSTANCE_ERR_REJECTED;
	}
	
	if(  _pEffect->magic!=kEffectMagic)
	{
		post("VST plugin : Instance query rejected by 0x%.8X\n",(int)_pEffect);
		FreeLibrary(h_dll);
		_pEffect=NULL;
		instantiated=false;
		return VSTINSTANCE_ERR_REJECTED;
	}

	//post("VST plugin : Instanced at (Effect*): %.8X\n",(int)_pEffect);

	//init plugin 
	_pEffect->user = this;
	Dispatch( effOpen        ,  0, 0, NULL, 0.0f);
	Dispatch( effSetProgram  ,  0, 0, NULL, 0.0f);
//	Dispatch( effMainsChanged,  0, 1, NULL, 0.0f);

	//************************************set samplerate and stream size here
    // we get it when we init our DSP

//	Dispatch( effSetSampleRate, 0, 0, NULL, (float)Global::pConfig->_pOutputDriver->_samplesPerSec);
//	Dispatch( effSetBlockSize,  0, STREAM_SIZE, NULL, 0.0f);

	
	if (!Dispatch( effGetProductString, 0, 0, &_sProductName, 0.0f))
	{
		CString str1(dllname);
		CString str2 = str1.Mid(str1.ReverseFind('\\')+1);
		int snip = str2.Find('.');
		if ( snip != -1 )
		{
			str1 = str2.Left( snip );
		}
		else
		{
			str1 = str2;
		}
		strcpy(_sProductName,str1);

	}
	
	if (!_pEffect->dispatcher(_pEffect, effGetVendorString, 0, 0, &_sVendorName, 0.0f))
	{
		strcpy(_sVendorName, "Unknown vendor");
	}
	_version = _pEffect->version;
	_isSynth = (_pEffect->flags & effFlagsIsSynth)?true:false;
	overwrite = (_pEffect->flags & effFlagsCanReplacing)?true:false;
	editor = (_pEffect->flags & effFlagsHasEditor)?true:false;

	if ( _sDllName != NULL ) delete _sDllName;
	_sDllName = new char[strlen(dllname)+1];
	sprintf(_sDllName,dllname);
	
	


	//keep plugin name
	instantiated=true;

	return VSTINSTANCE_NO_ERROR;
}

int VSTPlugin::getNumInputs( void )
{
	return _pEffect->numInputs;
}

int VSTPlugin::getNumOutputs( void )
{
	return _pEffect->numOutputs;
}


void VSTPlugin::Create(VSTPlugin *plug)
{
	h_dll=plug->h_dll;
	_pEffect=plug->_pEffect;
	_pEffect->user=this;
	Dispatch( effMainsChanged,  0, 1, NULL, 0.0f);
//	strcpy(_editName,plug->_editName); On current implementation, this replaces the right one. 
	strcpy(_sProductName,plug->_sProductName);
	strcpy(_sVendorName,plug->_sVendorName);
	
	_sDllName = new char[strlen(plug->_sDllName)+1];
	strcpy(_sDllName,plug->_sDllName);

	_isSynth=plug->_isSynth;
	_version=plug->_version;

	plug->instantiated=false;	// We are "stoling" the plugin from the "plug" object so this
								// is just a "trick" so that when destructing the "plug", it
								// doesn't unload the Dll.
	instantiated=true;
}

void VSTPlugin::Free() // Called also in destruction
{
	if(instantiated)
	{
		instantiated=false;
		post("VST plugin : Free query 0x%.8X\n",(int)_pEffect);
		_pEffect->user = NULL;
		Dispatch( effMainsChanged, 0, 0, NULL, 0.0f);
		Dispatch( effClose,        0, 0, NULL, 0.0f);
//		delete _pEffect; // <-  Should check for the necessity of this command.
		_pEffect=NULL;
		FreeLibrary(h_dll);
	}
}

void VSTPlugin::Init( float samplerate , float blocksize )
{
	sample_rate = samplerate;
	Dispatch(effOpen        ,  0, 0, NULL, 0.f);
	Dispatch(effMainsChanged,  0, 1, NULL, 0.f);
	Dispatch(effSetSampleRate, 0, 0, 0, (float) sample_rate );
	Dispatch(effSetBlockSize,  0, blocksize, NULL, 0.f );
}


bool VSTPlugin::DescribeValue(int p,char* psTxt)
{
	int parameter = p;
	if(instantiated)
	{
		if(parameter<_pEffect->numParams)
		{
//			char par_name[64];
			char par_display[64];
			char par_label[64];

//			Dispatch(effGetParamName,parameter,0,par_name,0.0f);
			Dispatch(effGetParamDisplay,parameter,0,par_display,0.0f);
			Dispatch(effGetParamLabel,parameter,0,par_label,0.0f);
//			sprintf(psTxt,"%s:%s%s",par_name,par_display,par_label);
			sprintf(psTxt,"%s%s",par_display,par_label);
			return true;
		}
		else	sprintf(psTxt,"NumParams Exeeded");
	}
	else		sprintf(psTxt,"Not loaded");

	return false;
}

bool VSTPlugin::SetParameter(int parameter, float value)
{
	if(instantiated)
	{
		if (( parameter >= 0 ) && (parameter<=_pEffect->numParams))
		{
			_pEffect->setParameter(_pEffect,parameter,value);
			return true;
		}
	}

	return false;
}

bool VSTPlugin::SetParameter(int parameter, int value)
{
	return SetParameter(parameter,value/65535.0f);
}

int VSTPlugin::GetCurrentProgram()
{
	if(instantiated)
		return Dispatch(effGetProgram,0,0,NULL,0.0f);
	else
		return 0;
}

void VSTPlugin::SetCurrentProgram(int prg)
{
	if(instantiated)
		Dispatch(effSetProgram,0,prg,NULL,0.0f);
}

bool VSTPlugin::AddMIDI(unsigned char data0,unsigned char data1,unsigned char data2)
{
	if (instantiated)
	{
		VstMidiEvent* pevent=&midievent[queue_size];

		pevent->type = kVstMidiType;
		pevent->byteSize = 24;
		pevent->deltaFrames = 0;
		pevent->flags = 0;
		pevent->detune = 0;
		pevent->noteLength = 0;
		pevent->noteOffset = 0;
		pevent->reserved1 = 0;
		pevent->reserved2 = 0;
		pevent->noteOffVelocity = 0;
		pevent->midiData[0] = data0;
		pevent->midiData[1] = data1;
		pevent->midiData[2] = data2;
		pevent->midiData[3] = 0;

		if ( queue_size < MAX_EVENTS ) queue_size++;
		SendMidi();
		return true;
	}
	else return false;
}


void VSTPlugin::SendMidi()
{
	if(/*instantiated &&*/ queue_size>0)
	{
		// Prepare MIDI events and free queue dispatching all events
		events.numEvents = queue_size;
		events.reserved  = 0;
		for(int q=0;q<queue_size;q++) events.events[q] = (VstEvent*)&midievent[q];
		
		Dispatch(effProcessEvents, 0, 0, &events, 0.0f);
		queue_size=0;
	}
}


void VSTPlugin::processReplacing( float **inputs, float **outputs, long sampleframes )
{
	_pEffect->processReplacing( _pEffect , inputs , outputs , sampleframes );

}

void VSTPlugin::process( float **inputs, float **outputs, long sampleframes )
{
	_pEffect->process( _pEffect , inputs , outputs , sampleframes );
}


// Host callback dispatcher
long VSTPlugin::Master(AEffect *effect, long opcode, long index, long value, void *ptr, float opt)
{
	//post("VST plugin call to host dispatcher: Eff: 0x%.8X, Opcode = %d, Index = %d, Value = %d, PTR = %.8X, OPT = %.3f\n",(int)effect, opcode,index,value,(int)ptr,opt);
	//st( "audioMasterWantMidi %d " , audioMasterWantMidi);

	// Support opcodes
	switch(opcode)
	{
	case audioMasterAutomate:			
		return 0;		// index, value, returns 0
		
	case audioMasterVersion:			
		return 9;		// vst version, currently 7 (0 for older)
		
	case audioMasterCurrentId:			
		return 'AASH';	// returns the unique id of a plug that's currently loading

	case audioMasterIdle:
		effect->dispatcher(effect, effEditIdle, 0, 0, NULL, 0.0f);
		return 0;		// call application idle routine (this will call effEditIdle for all open editors too) 
		
	case audioMasterPinConnected:	
		return false;	// inquire if an input or output is beeing connected;

	case audioMasterWantMidi:			
		return 0;

	case audioMasterProcessEvents:		
		return 0; 	// Support of vst events to host is not available

	case audioMasterGetTime:
		memset(&_timeInfo, 0, sizeof(_timeInfo));
		_timeInfo.samplePos = 0;
		_timeInfo.sampleRate = sample_rate;
		return (long)&_timeInfo;
		
		
	case audioMasterTempoAt:			
		return 0;

	case audioMasterNeedIdle:	
		effect->dispatcher(effect, effIdle, 0, 0, NULL, 0.0f);
		return 1;

	case audioMasterGetSampleRate:		
		return sample_rate;	

	case audioMasterGetVendorString:	// Just fooling version string
		strcpy((char*)ptr,"Steinberg");
		return 0;
	
	case audioMasterGetVendorVersion:	
		return 5000;	// HOST version 5000

	case audioMasterGetProductString:	// Just fooling product string
		strcpy((char*)ptr,"Cubase 5.0");
		return 0;

	case audioMasterVendorSpecific:		
		{
			return 0;
		}
		

	case audioMasterGetLanguage:		
		return kVstLangEnglish;
	
	case audioMasterUpdateDisplay:
		post("audioMasterUpdateDisplay");
		effect->dispatcher(effect, effEditIdle, 0, 0, NULL, 0.0f);
		return 0;

		
	case 	audioMasterSetTime:						post("VST master dispatcher: Set Time");break;
	case 	audioMasterGetNumAutomatableParameters:	post("VST master dispatcher: GetNumAutPar");break;
	case 	audioMasterGetParameterQuantization:	post("VST master dispatcher: ParamQuant");break;
	case 	audioMasterIOChanged:					post("VST master dispatcher: IOchanged");break;
	case 	audioMasterSizeWindow:					post("VST master dispatcher: Size Window");break;
	case 	audioMasterGetBlockSize:				post("VST master dispatcher: GetBlockSize");break;
	case 	audioMasterGetInputLatency:				post("VST master dispatcher: GetInLatency");break;
	case 	audioMasterGetOutputLatency:			post("VST master dispatcher: GetOutLatency");break;
	case 	audioMasterGetPreviousPlug:				post("VST master dispatcher: PrevPlug");break;
	case 	audioMasterGetNextPlug:					post("VST master dispatcher: NextPlug");break;
	case 	audioMasterWillReplaceOrAccumulate:		post("VST master dispatcher: WillReplace"); break;
	case 	audioMasterGetCurrentProcessLevel:		return 0; break;
	case 	audioMasterGetAutomationState:			post("VST master dispatcher: GetAutState");break;
	case 	audioMasterOfflineStart:				post("VST master dispatcher: Offlinestart");break;
	case 	audioMasterOfflineRead:					post("VST master dispatcher: Offlineread");break;
	case 	audioMasterOfflineWrite:				post("VST master dispatcher: Offlinewrite");break;
	case 	audioMasterOfflineGetCurrentPass:		post("VST master dispatcher: OfflineGetcurrentpass");break;
	case 	audioMasterOfflineGetCurrentMetaPass:	post("VST master dispatcher: GetGetCurrentMetapass");break;
	case 	audioMasterSetOutputSampleRate:			post("VST master dispatcher: Setsamplerate");break;
	case 	audioMasterGetSpeakerArrangement:		post("VST master dispatcher: Getspeaker");break;
	case 	audioMasterSetIcon:						post("VST master dispatcher: seticon");break;
	case 	audioMasterCanDo:						post("VST master dispatcher: Can Do");break;
	case 	audioMasterOpenWindow:					post("VST master dispatcher: OpenWindow");break;
	case 	audioMasterCloseWindow:					post("VST master dispatcher: CloseWindow");break;
	case 	audioMasterGetDirectory:				post("VST master dispatcher: GetDirectory");break;
//	case		audioMasterUpdateDisplay:				post("VST master dispatcher: audioMasterUpdateDisplay");break;

	default: post("VST master dispatcher: undefed: %d , %d",opcode , effKeysRequired )	;break;
	}	

	
	return 0;
}

bool VSTPlugin::AddNoteOn( unsigned char note,unsigned char speed,unsigned char midichannel)
{
	if(instantiated)
	{
		VstMidiEvent* pevent=&midievent[queue_size];

		pevent->type = kVstMidiType;
		pevent->byteSize = 24;
		pevent->deltaFrames = 0;
		pevent->flags = 0;
		pevent->detune = 0;
		pevent->noteLength = 0;
		pevent->noteOffset = 0;
		pevent->reserved1 = 0;
		pevent->reserved2 = 0;
		pevent->noteOffVelocity = 0;
		pevent->midiData[0] = (char)MIDI_NOTEON | midichannel; // Midi On
		pevent->midiData[1] = note;
		pevent->midiData[2] = speed;
		pevent->midiData[3] = 0;

		if ( queue_size < MAX_EVENTS ) queue_size++;
		SendMidi();
		return true;
	}
	else	return false;


}

bool VSTPlugin::AddNoteOff( unsigned char note,unsigned char midichannel)
{
	if (instantiated)
	{
	
			VstMidiEvent* pevent=&midievent[queue_size];

			pevent->type = kVstMidiType;
			pevent->byteSize = 24;
			pevent->deltaFrames = 0;
			pevent->flags = 0;
			pevent->detune = 0;
			pevent->noteLength = 0;
			pevent->noteOffset = 0;
			pevent->reserved1 = 0;
			pevent->reserved2 = 0;
			pevent->noteOffVelocity = 0;
			pevent->midiData[0] = (char)MIDI_NOTEOFF | midichannel; // Midi Off
			pevent->midiData[1] = note;
			pevent->midiData[2] = 0;
			pevent->midiData[3] = 0;

			if ( queue_size < MAX_EVENTS ) queue_size++;

			SendMidi();
			return true;
	
	}
	else	return false;
}


bool VSTPlugin::replace()
{
	return overwrite;
}


void VSTPlugin::edit()
{	
	if(instantiated)
	{ 		
		if ( ( editor ) && (!edited))
		{			
			edited = true;
			//b = (CEditorThread*) AfxBeginThread(RUNTIME_CLASS( CEditorThread ) );
			b =  new CEditorThread();			
			b->SetPlugin( this );			
			b->CreateThread();			
		}
	}
}

void VSTPlugin::EditorIdle()
{
	Dispatch(effEditIdle,0,0, w,0.0f);			
}

RECT VSTPlugin::GetEditorRect()
{
	RECT ret;
	ERect *r;
	Dispatch(effEditGetRect,0,0, &r,0.0f);				
	ret.top = r->top;
	ret.bottom = r->bottom;
	ret.left = r->left;
	ret.right = r->right;
	return ret;
}

void VSTPlugin::SetEditWindow(HWND h)
{
	w = h;	
	Dispatch(effEditOpen,0,0, w,0.0f);							
}

void VSTPlugin::OnEditorCLose()
{
	Dispatch(effEditClose,0,0, w,0.0f);					
}

void VSTPlugin::SetShowParameters(bool s)
{
	show_params = s;
}

bool VSTPlugin::ShowParams()
{
	return show_params;
}

void VSTPlugin::AddAftertouch(int value)
{
	if (value < 0) value = 0; else if (value > 127) value = 127;
 	AddMIDI( (char)MIDI_NOTEOFF | _midichannel , value );
}

void VSTPlugin::AddPitchBend(int value)
{
		AddMIDI( MIDI_PITCHBEND + (_midichannel & 0xf) , ((value>>7) & 127), (value & 127));
}

void VSTPlugin::AddProgramChange(int value)
{
 if (value < 0) value = 0; else if (value > 127) value = 127;
    AddMIDI( MIDI_PROGRAMCHANGE + (_midichannel & 0xf), value, 0);
}

void VSTPlugin::AddControlChange(int control, int value)
{
  if (control < 0) control = 0;  else if (control > 127) control = 127;
    if (value < 0) value = 0;  else if (value > 127) value = 127;
    AddMIDI( MIDI_CONTROLCHANGE + (_midichannel & 0xf), control, value);
}


bool VSTPlugin::GetProgramName( int cat , int p, char *buf)
{
	int parameter = p;
	if(instantiated)
	{
		if(parameter<NumPrograms())
		{
			Dispatch(effGetProgramNameIndexed,parameter,cat,buf,0.0f);
			return true;
		}
	}
	return false;
}

int VSTPlugin::GetNumCategories()
{
	if(instantiated)
		return Dispatch(effGetNumProgramCategories,0,0,NULL,0.0f);
	else
		return 0;
}

void VSTPlugin::StopEditing()
{
	edited = false;
}
