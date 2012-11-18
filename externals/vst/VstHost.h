#ifndef _VSTPLUGIN_HOST
#define _VSTPLUGIN_HOST

#include "Vst\AEffectx.h"
#include <afxcoll.h>
#define MAX_EVENTS		64
#define MAX_INOUTS		8

#define VSTINSTANCE_ERR_NO_VALID_FILE -1
#define VSTINSTANCE_ERR_NO_VST_PLUGIN -2
#define VSTINSTANCE_ERR_REJECTED -3
#define VSTINSTANCE_NO_ERROR 0

#define MIDI_NOTEON 144
#define MIDI_NOTEOFF 128
#define MIDI_POLYAFTERTOUCH 160
#define MIDI_CONTROLCHANGE 176
#define MIDI_PROGRAMCHANGE 192
#define MIDI_AFTERTOUCH 208
#define MIDI_PITCHBEND 224

typedef AEffect* (*PVSTMAIN)(audioMasterCallback audioMaster);
typedef HWND (*POPWIN)(void);
typedef HWND (*GETWIN)(void);

class VSTPlugin 
{
public:
	void StopEditing();
	int GetNumCategories();
	bool GetProgramName( int cat, int p , char* buf);
	void AddControlChange( int control , int value );
	void AddProgramChange( int value );
	void AddPitchBend( int value );
	void AddAftertouch( int value );
	bool editor;
	bool ShowParams();
	void SetShowParameters( bool s);
	void OnEditorCLose();
	void SetEditWindow( HWND h );
	CEditorThread* b;
	RECT GetEditorRect();
	void EditorIdle();
	void edit(void);
	bool replace(  );
	VSTPlugin();
	~VSTPlugin();

	void Free();
	int Instance( const char *dllname);
	void Create(VSTPlugin *plug);
	void Init( float samplerate , float blocksize );

	virtual int GetNumParams(void) { return _pEffect->numParams; }
	virtual void GetParamName(int numparam,char* name)
	{
		if ( numparam < _pEffect->numParams ) Dispatch(effGetParamName,numparam,0,name,0.0f);
		else strcpy(name,"Out of Range");

	}
	virtual void GetParamValue(int numparam,char* parval)
	{
		if ( numparam < _pEffect->numParams ) DescribeValue(numparam,parval);
		else strcpy(parval,"Out of Range");
	}
	virtual float GetParamValue(int numparam)
	{
		if ( numparam < _pEffect->numParams ) return (_pEffect->getParameter(_pEffect, numparam));
		else return -1.0;
	}

	int getNumInputs( void );
	int getNumOutputs( void );

	virtual char* GetName(void) { return _sProductName; }
	unsigned long  GetVersion() { return _version; }
	char* GetVendorName(void) { return _sVendorName; }
	char* GetDllName(void) { return _sDllName; }

	long NumParameters(void) { return _pEffect->numParams; }
	float GetParameter(long parameter) { return _pEffect->getParameter(_pEffect, parameter); }
	bool DescribeValue(int parameter,char* psTxt);
	bool SetParameter(int parameter, float value);
	bool SetParameter(int parameter, int value);
	void SetCurrentProgram(int prg);
	int GetCurrentProgram();
	int NumPrograms() { return _pEffect->numPrograms; }
	bool IsSynth() { return _isSynth; }

	bool AddMIDI(unsigned char data0,unsigned char data1=0,unsigned char data2=0);
	void SendMidi();


	void processReplacing( float **inputs, float **outputs, long sampleframes );
	void process( float **inputs, float **outputs, long sampleframes );

	AEffect *_pEffect;
	long Dispatch(long opCode, long index, long value, void *ptr, float opt)
	{
		return _pEffect->dispatcher(_pEffect, opCode, index, value, ptr, opt);
	}

	static long Master(AEffect *effect, long opcode, long index, long value, void *ptr, float opt);

	bool AddNoteOn( unsigned char note,unsigned char speed,unsigned char midichannel=0);
	bool AddNoteOff( unsigned char note,unsigned char midichannel=0);
	

	char _midichannel;
	bool instantiated;
	int _instance;		// Remove when Changing the FileFormat.

	HWND w;

protected:

	HMODULE h_dll;
	HMODULE h_winddll;

	char _sProductName[64];
	char _sVendorName[64];
	char *_sDllName;	// Contains dll name
	ULONG _version;
	bool _isSynth;

	float * inputs[MAX_INOUTS];
	float * outputs[MAX_INOUTS];
	float junk[256];

	static VstTimeInfo _timeInfo;
	VstMidiEvent midievent[MAX_EVENTS];
	VstEvents events;
	int	queue_size;
	bool overwrite;



private:
	bool edited;
	bool show_params;
	static float sample_rate;
};


#endif // _VSTPLUGIN_HOST