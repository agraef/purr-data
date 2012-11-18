// vst.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "vst.h"
#include "m_pd.h"
#include "EditorThread.h"
#include "VstHost.h"
#include "Popupwindow.h"
#include "vst~.h"
#include "export.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CVstApp

BEGIN_MESSAGE_MAP(CVstApp, CWinApp)
	//{{AFX_MSG_MAP(CVstApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVstApp construction

CVstApp::CVstApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance

}

/////////////////////////////////////////////////////////////////////////////
// The one and only CVstApp object

CVstApp theApp;


#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
extern "C" {
#endif

typedef struct _vstlib
{
     t_object x_obj;
} t_vstLib;

static t_class* vstLib_class;

static void* vstLib_new(t_symbol* s) {
    t_vstLib *x = (t_vstLib *)pd_new( vstLib_class);
    return (x);
}

 void vst_setup(void) 
{
	 AFX_MANAGE_STATE(AfxGetStaticModuleState());
    vstLib_class = class_new(gensym("vstLib"), (t_newmethod)vstLib_new, 0,
    	sizeof(t_vstLib), 0, (t_atomtype)0);

	AfxOleInit( );


	// call setups here

	vst_tilde_setup();

     post("VST host  by mark williamson");
	post("contains source code from the Psycle tracker");

     post("Contact: mark@junklight.com  . website: http://www.junklight.com");
     post("VSTLib: version:  0.1 ");
     post("VSTLib: compiled: "__DATE__);
	 post("");
}

#if defined(_LANGUAGE_C_PLUS_PLUS) || defined(__cplusplus)
}
#endif