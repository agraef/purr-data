// PopupWindow.cpp : implementation file
//

#include "stdafx.h"
#include "m_pd.h"
#include "vst.h"
#include "PopupWindow.h"
#include "EditorThread.h"
#include "VstHost.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CVstApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CPopupWindow

IMPLEMENT_DYNCREATE(CPopupWindow, CFrameWnd)

CPopupWindow::CPopupWindow()
{
	plug = NULL;
}

CPopupWindow::~CPopupWindow()
{
	plug->OnEditorCLose();
	plug = NULL;
}


BEGIN_MESSAGE_MAP(CPopupWindow, CFrameWnd)
	//{{AFX_MSG_MAP(CPopupWindow)
	ON_WM_ENTERIDLE()
	ON_WM_TIMER()
	ON_WM_MOVE()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPopupWindow message handlers

void CPopupWindow::OnEnterIdle(UINT nWhy, CWnd* pWho) 
{
	CFrameWnd::OnEnterIdle(nWhy, pWho);
	
	// TODO: Add your message handler code here
	if (plug != NULL )
	{
		plug->EditorIdle();		
	}
}

void CPopupWindow::SetPlugin(VSTPlugin *p)
{
	plug = p;
	plug->Dispatch(effEditOpen , 0 , 0 , m_hWnd , 0.0f  );
	RECT r = plug->GetEditorRect();
	CString str = theApp.GetProfileString( "VSTPos" , plug->GetName() , "10,10");
	int idx = str.Find(",");
	CString x = str.Left( idx );
	CString y = str.Right( idx );
	printf(" index is %d left is %s and right is %s" , idx , x , y);	
	SetWindowPos( &wndTopMost  , atoi( x )  , atoi( y ) , (r.right - r.left) + 10 , r.bottom - r.top + 30 , SWP_SHOWWINDOW   );	
}

BOOL CPopupWindow::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

void CPopupWindow::DoInit()
{
	printf("DoInit\n");
	plug->Dispatch(effEditTop,0,0, 0,0.0f);			
	printf("Dispatched to the top\n");
	SetTimer(0,25,NULL);
}

void CPopupWindow::OnTimer(UINT nIDEvent) 
{
	plug->Dispatch(effEditIdle, 0, 0, NULL, 0.0f);
	CFrameWnd::OnTimer(nIDEvent);
}

void CPopupWindow::OnMove(int x, int y) 
{
	CFrameWnd::OnMove(x, y);
	if ( plug != NULL )
	{
		char buf[100];
		sprintf( buf , "%d,%d" , x , y );
		theApp.WriteProfileString( "VSTPos" , plug->GetName() , buf );
	}
}


void CPopupWindow::OnFinalRelease() 
{
	//
	CFrameWnd::OnFinalRelease();
}

void CPopupWindow::OnClose() 
{
	// TODO: Add your message handler code here and/or call default
	plug->StopEditing();
	CFrameWnd::OnClose();
}
