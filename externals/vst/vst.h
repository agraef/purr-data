// vst.h : main header file for the VST DLL
//

#if !defined(AFX_VST_H__013CDC75_CDE8_40AD_AE29_D952471B07F5__INCLUDED_)
#define AFX_VST_H__013CDC75_CDE8_40AD_AE29_D952471B07F5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CVstApp
// See vst.cpp for the implementation of this class
//

class CVstApp : public CWinApp
{
public:
	CVstApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVstApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CVstApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VST_H__013CDC75_CDE8_40AD_AE29_D952471B07F5__INCLUDED_)

