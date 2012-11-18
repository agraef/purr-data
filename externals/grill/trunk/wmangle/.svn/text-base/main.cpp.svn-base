/* 

wmangle - a window mangler

Copyright (c) 2002-2008 Thomas Grill (xovo@gmx.net)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  

*/

#define FLEXT_ATTRIBUTES 1

#include <flext.h>

#if !defined(FLEXT_VERSION) || (FLEXT_VERSION < 500)
#error You need at least flext version 0.5.0
#endif

#if FLEXT_OS == FLEXT_OS_WIN
#include <windows.h>
#elif FLEXT_OS == FLEXT_OS_MAC
	#if FLEXT_OSAPI == FLEXT_OSAPI_MAC_MACH
		#include <Carbon/Carbon.h>
	#endif
#endif

#include <string.h>

#define VERSION "0.2.1"

class wmangle:
	public flext_base
{
	FLEXT_HEADER_S(wmangle,flext_base,Setup)

public:
	wmangle();
	~wmangle();

protected:
	void m_front();
	void m_close();
	void m_find(const t_symbol *s,bool exact = true);
    void m_findsub(const t_symbol *s) { m_find(s,false); }
	void ms_title(const t_symbol *s);
	void mg_title(const t_symbol *&s);
	void m_dump();
	void ms_poll(int poll);
	void ms_pos(const AtomList &l);
	void mg_pos(AtomList &l);
	void ms_size(const AtomList &l);
	void mg_size(AtomList &l);
	void ms_hidden(bool hd);
	void mg_hidden(bool &hd);
	void ms_closebox(bool is);
	void mg_closebox(bool &is);
	void ms_zoombox(bool is);
	void mg_zoombox(bool &is);
	void ms_resizable(bool is);
	void mg_resizable(bool &is);
	void ms_shadow(bool is);
	void mg_shadow(bool &is);
	
	virtual void m_help();
	
#if FLEXT_OS == FLEXT_OS_MAC
	WindowRef ref;
#elif FLEXT_OS == FLEXT_OS_WIN
    HWND ref;
#else
#error Not implemented
#endif
	int pollfrq;
	
	bool chkwnd();
	void dump();
	
	void errwnd();
	
private:
	Timer tmr;
	
	static void Setup(t_classid c);

#if FLEXT_SYS == FLEXT_SYS_MAX
	t_qelem *qel;
	void tick(void *) { qelem_set(qel); }
	static void qelem(wmangle &th) { if(th.chkwnd()) th.m_dump(); }
#elif FLEXT_SYS == FLEXT_SYS_PD
	void tick(void *) { if(chkwnd()) m_dump(); }
#else
#error Not implemented
#endif
	

	FLEXT_CALLBACK(m_front)
    FLEXT_CALLBACK(m_close)
    FLEXT_CALLBACK(m_help)
	FLEXT_CALLBACK_S(m_find)
	FLEXT_CALLBACK_S(m_findsub)
	FLEXT_CALLBACK(m_dump)
	FLEXT_CALLSET_I(ms_poll)
	FLEXT_ATTRGET_I(pollfrq)
	FLEXT_CALLVAR_B(mg_hidden,ms_hidden)
	FLEXT_CALLVAR_S(mg_title,ms_title)
	FLEXT_CALLVAR_V(mg_pos,ms_pos)
	FLEXT_CALLVAR_V(mg_size,ms_size)
	FLEXT_CALLVAR_B(mg_closebox,ms_closebox)
	FLEXT_CALLVAR_B(mg_zoombox,ms_zoombox)
	FLEXT_CALLVAR_B(mg_resizable,ms_resizable)
	FLEXT_CALLVAR_B(mg_shadow,ms_shadow)
	
	FLEXT_CALLBACK_X(tick);
};

FLEXT_NEW("wmangle",wmangle)


wmangle::wmangle():
	ref(NULL),pollfrq(0)
{ 
	AddInAnything("Bang to get info / Command message");
	FLEXT_ADDTIMER(tmr,tick);
#if FLEXT_SYS == FLEXT_SYS_MAX
	qel = (t_qelem *)qelem_new(this,(t_method)qelem);
#endif

#if 0 //FLEXT_OS == FLEXT_OS_MAC
	// there seems to be a hidden window somewhere in the window list... hide it!
	for(WindowRef tref = FrontWindow(); tref; tref = GetNextWindow(tref)) {
		post("NW %x",tref);
		if((unsigned long)tref == 0x1000000UL) { HideWindow(tref); break; }
	}
#endif
}

void wmangle::Setup(t_classid c)
{
	post("wmangle " VERSION ", (c)2002-2008 Thomas Grill");

	FLEXT_CADDMETHOD_(c,0,"front",m_front);
	FLEXT_CADDMETHOD_(c,0,"find",m_find);
	FLEXT_CADDMETHOD_(c,0,"findsub",m_findsub);
	FLEXT_CADDMETHOD_(c,0,"close",m_close);
	FLEXT_CADDMETHOD_(c,0,"help",m_help);
	FLEXT_CADDBANG(c,0,m_dump);
	FLEXT_CADDMETHOD_(c,0,"dump",m_dump);
	FLEXT_CADDATTR_VAR(c,"poll",pollfrq,ms_poll);
	FLEXT_CADDATTR_VAR(c,"hidden",mg_hidden,ms_hidden);
	FLEXT_CADDATTR_VAR(c,"title",mg_title,ms_title);
	FLEXT_CADDATTR_VAR(c,"pos",mg_pos,ms_pos);
	FLEXT_CADDATTR_VAR(c,"size",mg_size,ms_size);
	FLEXT_CADDATTR_VAR(c,"closebox",mg_closebox,ms_closebox);
	FLEXT_CADDATTR_VAR(c,"zoombox",mg_zoombox,ms_zoombox);	
	FLEXT_CADDATTR_VAR(c,"resizable",mg_resizable,ms_resizable);	
	FLEXT_CADDATTR_VAR(c,"shadow",mg_shadow,ms_shadow);	
}

wmangle::~wmangle()
{
#if FLEXT_SYS == FLEXT_SYS_MAX
	qelem_free(qel);
#endif
}


void wmangle::m_help()
{
	post("%s version " VERSION " (using flext " FLEXT_VERSTR "), (C)2002-2008 Thomas Grill",thisName());
}

void wmangle::errwnd() { post("%s - no active window!",thisName()); }

void wmangle::ms_hidden(bool hd) 
{ 
	if(chkwnd()) { 
#if FLEXT_OS == FLEXT_OS_MAC
		ShowHide(ref,!hd);
#elif FLEXT_OS == FLEXT_OS_WIN
        SetWindowPos(ref,NULL,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|(hd?SWP_HIDEWINDOW:SWP_SHOWWINDOW));
#else
#error Not implemented
#endif
	}
	else 
	    errwnd();
}

void wmangle::mg_hidden(bool &hd) 
{ 
	if(chkwnd()) { 
#if FLEXT_OS == FLEXT_OS_MAC || FLEXT_OS == FLEXT_OS_WIN
		hd = !IsWindowVisible(ref);
#else
#error Not implemented
#endif
	}
	else 
	    errwnd();
}

void wmangle::ms_closebox(bool cl)
{
	if(chkwnd()) {
#if FLEXT_OS == FLEXT_OS_MAC
		WindowAttributes setattr = (cl?kWindowCloseBoxAttribute:0),clrattr = (!cl?kWindowCloseBoxAttribute:0);
		ChangeWindowAttributes(ref,setattr,clrattr);
#elif FLEXT_OS == FLEXT_OS_WIN
        DWORD stl = GetWindowLong(ref,GWL_STYLE);
        SetWindowLong(ref,GWL_STYLE,cl?stl|WS_SYSMENU:stl&~WS_SYSMENU);
        SetWindowPos(ref,NULL,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
#else
#error
#endif	
	}
	else 
	    errwnd();
}

void wmangle::mg_closebox(bool &cl)
{
	if(chkwnd()) {
#if FLEXT_OS == FLEXT_OS_MAC
		WindowAttributes attr;
		GetWindowAttributes(ref,&attr);
		cl = attr&kWindowCloseBoxAttribute;
#elif FLEXT_OS == FLEXT_OS_WIN
        cl = (GetWindowLong(ref,GWL_STYLE)&WS_SYSMENU) != 0;
#else
#error
#endif
	}
	else 
	    errwnd();
}

void wmangle::ms_zoombox(bool zm)
{
	if(chkwnd()) {
#if FLEXT_OS == FLEXT_OS_MAC
		WindowAttributes setattr = (zm?kWindowCollapseBoxAttribute:0),clrattr = (!zm?kWindowCollapseBoxAttribute:0);
		ChangeWindowAttributes(ref,setattr,clrattr);
#elif FLEXT_OS == FLEXT_OS_WIN
        DWORD stl = GetWindowLong(ref,GWL_STYLE);
        SetWindowLong(ref,GWL_STYLE,zm?stl|WS_MAXIMIZE:stl&~WS_MAXIMIZE);
        SetWindowPos(ref,NULL,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
#else
#error
#endif
	}
	else 
	    errwnd();
}

void wmangle::mg_zoombox(bool &cl)
{
	if(chkwnd()) {
#if FLEXT_OS == FLEXT_OS_MAC
		WindowAttributes attr;
		GetWindowAttributes(ref,&attr);
		cl = attr&kWindowCollapseBoxAttribute;
#elif FLEXT_OS == FLEXT_OS_WIN
        cl = (GetWindowLong(ref,GWL_STYLE)&WS_MAXIMIZE) != 0;
#else
#error
#endif
	}
	else 
	    errwnd();
}

void wmangle::ms_resizable(bool fl)
{
	if(chkwnd()) {
#if FLEXT_OS == FLEXT_OS_MAC
		WindowAttributes setattr = (fl?kWindowResizableAttribute:0),clrattr = (!fl?kWindowResizableAttribute:0);
		ChangeWindowAttributes(ref,setattr,clrattr);
#elif FLEXT_OS == FLEXT_OS_WIN
        DWORD stl = GetWindowLong(ref,GWL_STYLE);
        SetWindowLong(ref,GWL_STYLE,fl?stl|WS_SIZEBOX:stl&~WS_SIZEBOX);
        SetWindowPos(ref,NULL,0,0,0,0,SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE|SWP_FRAMECHANGED);
#else
#error
#endif
	}
	else 
	    errwnd();
}

void wmangle::mg_resizable(bool &fl)
{
	if(chkwnd()) {
#if FLEXT_OS == FLEXT_OS_MAC
		WindowAttributes attr;
		GetWindowAttributes(ref,&attr);
		fl = attr&kWindowResizableAttribute;
#elif FLEXT_OS == FLEXT_OS_WIN
        fl = (GetWindowLong(ref,GWL_STYLE)&WS_SIZEBOX) != 0;
#else
#error
#endif
	}
	else 
	    errwnd();
}

void wmangle::ms_shadow(bool fl)
{
	if(chkwnd()) {
#if FLEXT_OS == FLEXT_OS_MAC
		WindowAttributes setattr = (!fl?kWindowNoShadowAttribute:0),clrattr = (fl?kWindowNoShadowAttribute:0);
		ChangeWindowAttributes(ref,setattr,clrattr);
#elif FLEXT_OS == FLEXT_OS_WIN
#else
#error
#endif
	}
	else 
	    errwnd();
}

void wmangle::mg_shadow(bool &fl)
{
	if(chkwnd()) {
#if FLEXT_OS == FLEXT_OS_MAC
		WindowAttributes attr;
		GetWindowAttributes(ref,&attr);
		fl = !(attr&kWindowNoShadowAttribute);
#elif FLEXT_OS == FLEXT_OS_WIN
        fl = false;
#else
#error
#endif
	}
	else 
	    errwnd();
}

void wmangle::m_front() 
{ 
#if FLEXT_OS == FLEXT_OS_MAC
	ref = FrontWindow();
#elif FLEXT_OS == FLEXT_OS_WIN
    ref = GetForegroundWindow();
//    ref = GetTopWindow(NULL);
#else
#error
#endif
}

void wmangle::m_close()
{
    if(chkwnd()) {
#if FLEXT_OS == FLEXT_OS_MAC
        EventRef event;
        CreateEvent( NULL, kEventClassWindow, kEventWindowClose, GetCurrentEventTime(), kEventAttributeUserEvent, &event );
        SetEventParameter( event, kEventParamDirectObject, typeWindowRef, sizeof(ref), &ref );
        SendEventToWindow( event, ref );
        ReleaseEvent( event );
//        DisposeWindow(ref);
#elif FLEXT_OS == FLEXT_OS_WIN
        DestroyWindow(ref);
#else
#error
#endif
        ref = NULL;
    }
	else 
	    errwnd();
}

#if FLEXT_OS == FLEXT_OS_MAC
static WindowRef traverselist(WindowClass wcl,const char *s,bool exact)
{
    WindowRef tref = GetFrontWindowOfClass(wcl,FALSE);
	for(int i = 0; tref; ++i) {
//		post("R %x",tref);
		char buf[256];
		CFStringRef cfstr;
//		GetWTitle(ref,(UC *)buf);
		CopyWindowTitleAsCFString(tref,&cfstr);
		if(cfstr && CFStringGetCString(cfstr,buf,sizeof buf-1,kCFStringEncodingASCII)) 
		{
//			post("W %s",buf);
            if(exact? 
                strcmp(buf,s) == 0: 
                strstr(buf,s) != NULL
            ) break;
		}
		tref = GetNextWindowOfClass(tref,wcl,FALSE);
	} 
	return tref;
}
#elif FLEXT_OS == FLEXT_OS_WIN
struct enumparms
{
    HWND *ref;
    const char *s;
    bool exact;
};

static BOOL CALLBACK enumwnd(HWND hwnd,LPARAM lParam)
{
    enumparms *parms = (enumparms *)lParam;
    char buf[256];
    GetWindowText(hwnd,buf,sizeof buf);
    if(parms->exact? strcmp(buf,parms->s) == 0 : strstr(buf,parms->s) != NULL) {
        *parms->ref = hwnd;
        return FALSE;
    }
    else {
        // also search for child windows
        EnumChildWindows(hwnd,enumwnd,lParam);
        return TRUE;
    }
}
#endif

void wmangle::m_find(const t_symbol *s,bool exact) 
{ 
#if FLEXT_OS == FLEXT_OS_MAC
	WindowRef ref1 = traverselist(kAllWindowClasses,GetString(s),exact);
//	WindowRef ref2 = traverselist(FrontNonFloatingWindow(),GetString(s),exact);
//	ref = ref1?ref1:ref2;
	ref = ref1;
#elif FLEXT_OS == FLEXT_OS_WIN
	HWND tref;
    enumparms parms;
    parms.ref = &tref;
    parms.s = GetString(s);
    parms.exact = exact;
    EnumWindows(enumwnd,(LPARAM)&parms);
    ref = tref;
#else
#error
#endif
	if(!ref) post("%s - Warning: window not found",thisName());
}

void wmangle::mg_title(const t_symbol *&s) 
{ 
	if(chkwnd()) {
		char buf[256];
#if FLEXT_OS == FLEXT_OS_MAC
		CFStringRef cfstr;
//		GetWTitle(ref,(UC *)buf);
		CopyWindowTitleAsCFString(ref,&cfstr);
		if(cfstr && CFStringGetCString(cfstr,buf,sizeof buf-1,kCFStringEncodingASCII)) 
#elif FLEXT_OS == FLEXT_OS_WIN
        if(GetWindowText(ref,buf,sizeof buf))
#else
#error
#endif
            s = MakeSymbol(buf);
        else {
        	s = MakeSymbol("");
            post("%s - Could not get window title",thisName());
        }
	} 
	else {
		errwnd();
		s = MakeSymbol("");
	}
}

void wmangle::ms_title(const t_symbol *s) 
{ 
	if(chkwnd()) {
#if FLEXT_OS == FLEXT_OS_MAC
//		SetWTitle(ref,(UC *)tmp);
		CFStringRef cfstr = CFStringCreateWithCString(kCFAllocatorDefault,GetString(s),kCFStringEncodingASCII);
		SetWindowTitleWithCFString(ref,cfstr);
#elif FLEXT_OS == FLEXT_OS_WIN
        int ret = SetWindowText(ref,GetString(s));
        FLEXT_ASSERT(ret);
#else
#error
#endif
	}
	else 
	    errwnd();
}

bool wmangle::chkwnd()
{
	if(!ref) return false;

#if FLEXT_OS == FLEXT_OS_MAC
	if(!IsValidWindowPtr(ref)) 
#elif FLEXT_OS == FLEXT_OS_WIN
    WINDOWINFO info;
    if(!GetWindowInfo(ref,&info))
#else
#error
#endif
    {
		post("%s - window not present anymore",thisName());
		ref = NULL;
		return false;
	}
	else
		return true;
}

void wmangle::ms_pos(const AtomList &l)
{
	if(chkwnd()) { 
		if(l.Count() >= 2 && CanbeInt(l[0]) && CanbeInt(l[1])) {	
            bool ontop = l.Count() >= 3 && GetAInt(l[2]);
#if FLEXT_OS == FLEXT_OS_MAC
			MoveWindow(ref,GetAInt(l[0]),GetAInt(l[1]),ontop); 	
#elif FLEXT_OS == FLEXT_OS_WIN
            SetWindowPos(ref,ontop?HWND_TOPMOST:HWND_NOTOPMOST,GetAInt(l[0]),GetAInt(l[1]),0,0,SWP_NOSIZE); 
/*
            HWND parent = GetParent(ref);
            RECT rect;
            GetWindowRect(ref,&rect);
            int x = GetAInt(l[0]),y = GetAInt(l[1]);
            if(parent != NULL) {
                RECT prect;
                GetClientRect(parent,&prect);
                x -= rect.left, y -= rect.top;
            }
            MoveWindow(ref,x,y,rect.right-rect.left,rect.bottom-rect.top,TRUE); 
*/
#else
#error
#endif
		}
		else 
			error("%s syntax: pos x y [keepinfront]",thisName());
	}
	else errwnd();
}

void wmangle::mg_pos(AtomList &pos)
{
	if(chkwnd()) { 
		pos(4);
		
#if FLEXT_OS == FLEXT_OS_MAC
		Rect rect;
		GetWindowBounds(ref,kWindowContentRgn,&rect);
		SetInt(pos[0],rect.left);
		SetInt(pos[1],rect.top);
		
		GetWindowBounds(ref,kWindowStructureRgn,&rect);		
		SetInt(pos[2],rect.left);
		SetInt(pos[3],rect.top);
#elif FLEXT_OS == FLEXT_OS_WIN
        RECT rect;
        GetWindowRect(ref,&rect);
		SetInt(pos[0],rect.left);
		SetInt(pos[1],rect.top);

        GetClientRect(ref,&rect);
		SetInt(pos[2],rect.left);
		SetInt(pos[3],rect.top);
#else
#error
#endif
	}
	else 
	    errwnd();
}

void wmangle::ms_size(const AtomList &l)
{
	if(chkwnd()) { 
		if(l.Count() == 2 && CanbeInt(l[0]) && CanbeInt(l[1])) {	
#if FLEXT_OS == FLEXT_OS_MAC
			SizeWindow(ref,GetAInt(l[0]),GetAInt(l[1]),true); 
#elif FLEXT_OS == FLEXT_OS_WIN
            SetWindowPos(ref,NULL,0,0,GetAInt(l[0]),GetAInt(l[1]),SWP_NOMOVE|SWP_NOZORDER);
#else
#error
#endif
		}
		else 
			error("%s syntax: size x y",thisName());
	}
	else 
	    errwnd();
}

void wmangle::mg_size(AtomList &size)
{
	if(chkwnd()) { 
		size(4);
		
#if FLEXT_OS == FLEXT_OS_MAC
		Rect rect;
		GetWindowBounds(ref,kWindowContentRgn,&rect);
		SetInt(size[0],rect.right-rect.left);
		SetInt(size[1],rect.bottom-rect.top);
		
		GetWindowBounds(ref,kWindowStructureRgn,&rect);		
		SetInt(size[2],rect.right-rect.left);
		SetInt(size[3],rect.bottom-rect.top);
#elif FLEXT_OS == FLEXT_OS_WIN
        RECT rect;
        GetWindowRect(ref,&rect);
		SetInt(size[0],rect.right-rect.left);
		SetInt(size[1],rect.bottom-rect.top);

        GetClientRect(ref,&rect);
		SetInt(size[2],rect.right-rect.left);
		SetInt(size[3],rect.bottom-rect.top);
#else
#error
#endif
	}
	else 
	    errwnd();
}

void wmangle::dump()
{
	DumpAttrib("title");
	DumpAttrib("pos");
	DumpAttrib("size");
	DumpAttrib("hide");
	DumpAttrib("close");
	DumpAttrib("zoom");
	DumpAttrib("resizable");
	DumpAttrib("shadow");
}

void wmangle::m_dump()
{
	if(chkwnd()) 
	    dump();
	else 
	    errwnd();
}

void wmangle::ms_poll(int f)
{
	pollfrq = f;
	if(pollfrq) 
		tmr.Periodic(pollfrq/1000.);
	else
		tmr.Reset();
}
