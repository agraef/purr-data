// EditorThread.cpp : implementation file
//

#include "stdafx.h"
#include "vst.h"
#include "EditorThread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditorThread

IMPLEMENT_DYNCREATE(CEditorThread, CWinThread)

CEditorThread::CEditorThread()
{
	pop = NULL;
}

CEditorThread::~CEditorThread()
{
}

BOOL CEditorThread::InitInstance()
{
	pop = new CPopupWindow();
	m_pMainWnd = pop;	
	pop->CreateEx( WS_EX_DLGMODALFRAME  , AfxRegisterWndClass(  CS_DBLCLKS)  ," VST window" ,   WS_CAPTION | WS_THICKFRAME   | WS_POPUP | WS_SYSMENU , 10 , 10 , 300 , 300 , NULL , NULL , NULL);		
	pop->SetPlugin( plug );	
	pop->DoInit();
	pop->ShowWindow( SW_SHOW );		
	pop->BringWindowToTop();
	pop->SetFocus();
	return TRUE;
}

int CEditorThread::ExitInstance()
{
	// TODO:  perform any per-thread cleanup here
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CEditorThread, CWinThread)
	//{{AFX_MSG_MAP(CEditorThread)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditorThread message handlers

void CEditorThread::SetPlugin(VSTPlugin *p)
{
	plug = p;	
}
