/* 
vst~ - VST plugin object for PD 
based on the work of Jarno Seppänen and Mark Williamson

Copyright (c)2003-2006 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "Editor.h"
#include "VstHost.h"
#include <flext.h>


#if FLEXT_OS == FLEXT_OS_MAC
// only Mac OSX code is situated in this file

void SetupEditor()
{
}

void StartEditor(VSTPlugin *p)
{
	ERect r;
    p->GetEditorRect(r);

	WindowRef hwnd;
	Rect rect;
	rect.top = r.top;
	rect.left = r.left;
	rect.bottom = r.bottom;
	rect.right = r.right;
	OSStatus err = CreateNewWindow(kFloatingWindowClass,kWindowNoAttributes,&rect,&hwnd);
	
	p->StartEditing(hwnd);
}

void StopEditor(VSTPlugin *p) 
{
	ReleaseWindow(p->EditorHandle());
	p->StopEditing();
}

void ShowEditor(VSTPlugin *p,bool show) 
{
	if(show)
		ShowWindow(p->EditorHandle());
	else
		HideWindow(p->EditorHandle());
}

void MoveEditor(VSTPlugin *p,int x,int y) 
{
	MoveWindow(p->EditorHandle(),x,y,false);
}

void SizeEditor(VSTPlugin *p,int x,int y) 
{
	SizeWindow(p->EditorHandle(),x,y,true);
}

void CaptionEditor(VSTPlugin *plug,bool c)
{
	OSStatus ret = ChangeWindowAttributes(plug->EditorHandle(),kWindowNoTitleBarAttribute|kWindowNoShadowAttribute,kWindowNoAttributes);
}

void TitleEditor(VSTPlugin *p,const char *t) 
{
	CFStringRef str = CFStringCreateWithCString(NULL,t,kCFStringEncodingMacRoman); // or kCFStringEncodingASCII ?
	SetWindowTitleWithCFString(p->EditorHandle(),str);
	CFRelease(str);
}

void HandleEditor(VSTPlugin *p,bool h)
{
}

void FrontEditor(VSTPlugin *p)
{
	BringToFront(p->EditorHandle());
}

void BelowEditor(VSTPlugin *p)
{
	SendBehind(p->EditorHandle(),NULL);
}

#endif // FLEXT_OS_MAC
