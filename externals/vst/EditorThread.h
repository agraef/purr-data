#if !defined(AFX_EDITORTHREAD_H__9F3ACE98_7522_400D_9404_DFD67E3D721B__INCLUDED_)
#define AFX_EDITORTHREAD_H__9F3ACE98_7522_400D_9404_DFD67E3D721B__INCLUDED_

#include "PopupWindow.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditorThread.h : header file
//

class VSTPLugin;

/////////////////////////////////////////////////////////////////////////////
// CEditorThread thread

class CEditorThread : public CWinThread
{
	DECLARE_DYNCREATE(CEditorThread)
protected:


// Attributes
public:
		CEditorThread();           // protected constructor used by dynamic creation

// Operations
public:
	VSTPlugin* plug;
	void SetPlugin( VSTPlugin *);
	CPopupWindow *pop;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditorThread)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CEditorThread();

	// Generated message map functions
	//{{AFX_MSG(CEditorThread)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITORTHREAD_H__9F3ACE98_7522_400D_9404_DFD67E3D721B__INCLUDED_)
