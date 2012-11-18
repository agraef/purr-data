#if !defined(AFX_POPUPWINDOW_H__7B1E2281_5085_4F60_8002_5F79B2CAFFE3__INCLUDED_)
#define AFX_POPUPWINDOW_H__7B1E2281_5085_4F60_8002_5F79B2CAFFE3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PopupWindow.h : header file
//

class VSTPlugin;

/////////////////////////////////////////////////////////////////////////////
// CPopupWindow frame

class CPopupWindow : public CFrameWnd
{
	DECLARE_DYNCREATE(CPopupWindow)
// Attributes
public:
	CPopupWindow();    
	virtual ~CPopupWindow();
// Operations
public:
	void DoInit();
	void SetPlugin( VSTPlugin *p);
	VSTPlugin * plug;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPopupWindow)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// Implementation
protected:
	

	// Generated message map functions
	//{{AFX_MSG(CPopupWindow)
	afx_msg void OnEnterIdle(UINT nWhy, CWnd* pWho);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_POPUPWINDOW_H__7B1E2281_5085_4F60_8002_5F79B2CAFFE3__INCLUDED_)
