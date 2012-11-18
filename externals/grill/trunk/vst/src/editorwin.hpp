/* 
vst~ - VST plugin object for PD 
based on the work of Jarno Seppänen and Mark Williamson

Copyright (c)2003-2009 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#if FLEXT_OS != FLEXT_OS_WIN
// only Windows code is located in this file
#error Wrong implementation
#endif

#include <map>
#include <windows.h>

class ThrCmp
{
public:
    inline bool operator()(const flext::thrid_t &a,const flext::thrid_t &b) const
    {
        if(sizeof(a) == sizeof(size_t))
            return *(size_t *)&a < *(size_t *)&b;
        else
            return memcmp(&a,&b,sizeof(a)) < 0;
    }
};

typedef std::map<flext::thrid_t,VSTPlugin *,ThrCmp> WndMap;
static WndMap wndmap;
static flext::ThrMutex mapmutex;

#define TIMER_INTERVAL 25
#define WCLNAME "vst~-class"


static LRESULT CALLBACK wndproc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp)
{
    mapmutex.Lock();
    VSTPlugin *plug = wndmap[flext::GetThreadId()];
    mapmutex.Unlock();
    FLEXT_ASSERT(plug != NULL);

    LRESULT res = 0;

    switch(msg) {
        case WM_CREATE: 
            // Initialize the window. 
            plug->StartEditing(hwnd);
            break; 

        case WM_CLOSE:
#ifdef FLEXT_LOGGING
            flext::post("WM_CLOSE");
#endif
            plug->StopEditing();

            DestroyWindow(hwnd);
            break; 
        case WM_DESTROY: 
#ifdef FLEXT_LOGGING
            flext::post("WM_DESTROY");
#endif
            // stop editor thread
            PostQuitMessage(0); 
            break; 
        
        case WM_TIMER: // fall through
        case WM_ENTERIDLE:
            plug->EditorIdle();		
            break; 
#if 0
        case WM_WINDOWPOSCHANGED: {
            // ignore after WM_CLOSE so that x,y positions are preserved
            if(!plug->IsEdited()) break;

            WINDOWPOS *w = (WINDOWPOS *)lp;

	        WINDOWINFO winfo;
	        winfo.cbSize = sizeof(winfo);
	        GetWindowInfo(hwnd,&winfo);
            int cpx = winfo.rcWindow.left-winfo.rcClient.left;
            int cpy = winfo.rcWindow.top-winfo.rcClient.top;
            int csx = winfo.rcWindow.right-winfo.rcClient.right-cpx;
            int csy = winfo.rcWindow.bottom-winfo.rcClient.bottom-cpy;
            // send normalized coordinates to plugin
            plug->SetPos(w->x+cpx,w->y+cpy,false);
            plug->SetSize(w->cx+csx,w->cy+csy,false);
            return 0; 
        }
#else
        case WM_MOVE: {
            // ignore after WM_CLOSE so that x,y positions are preserved
            if(!plug->IsEdited()) break;

            WORD wx = LOWORD(lp),wy = HIWORD(lp);
            short x = reinterpret_cast<short &>(wx),y = reinterpret_cast<short &>(wy);
            // x and y are the coordinates of the client rect (= actual VST interface)

	        WINDOWINFO winfo;
	        winfo.cbSize = sizeof(winfo);
	        GetWindowInfo(hwnd,&winfo);
            int px = winfo.rcWindow.left-winfo.rcClient.left;
            int py = winfo.rcWindow.top-winfo.rcClient.top;
            // send normalized coordinates to plugin
            plug->SetPos(x+px,y+py,false);
            break; 
        }

        case WM_SIZE: {
            if(!plug->IsEdited()) break;

            WORD wx = LOWORD(lp),wy = HIWORD(lp);
            short x = reinterpret_cast<short &>(wx),y = reinterpret_cast<short &>(wy);
            // x and y are the coordinates of the client rect (= actual VST interface)

	        WINDOWINFO winfo;
	        winfo.cbSize = sizeof(winfo);
	        GetWindowInfo(hwnd,&winfo);
            int px = winfo.rcWindow.left-winfo.rcClient.left;
            int py = winfo.rcWindow.top-winfo.rcClient.top;
            int sx = winfo.rcWindow.right-winfo.rcClient.right-px;
            int sy = winfo.rcWindow.bottom-winfo.rcClient.bottom-py;
            // send normalized coordinates to plugin
            plug->SetSize(x+sx,y+sy,false);
            break; 
        }
#endif

#if 0 // NOT needed for Windows
        case WM_PAINT: {
            // Paint the window's client area. 
            RECT rect;
            GetUpdateRect(hwnd,&rect,FALSE);
            ERect erect;
            erect.left = rect.left;
            erect.top = rect.top;
            erect.right = rect.right;
            erect.bottom = rect.bottom;
            plug->Paint(erect);
            break;  
        }
#endif
        case WM_SHOWWINDOW:
            plug->Visible(wp != FALSE,false);
            break;

        default: 
        #ifdef FLEXT_LOGGING
//            flext::post("WND MSG %i, WP=%i, lp=%i",msg,wp,lp);
        #endif

            res = DefWindowProc(hwnd,msg,wp,lp); 
    }
    return res;
}

static void windowsize(HWND wnd,int x,int y,int w,int h)
{
    // pre correction
    WINDOWINFO winfo;
	winfo.cbSize = sizeof(winfo);
	GetWindowInfo(wnd,&winfo);
    int sx1 = (winfo.rcWindow.right-winfo.rcClient.right)-(winfo.rcWindow.left-winfo.rcClient.left);
    int sy1 = (winfo.rcWindow.bottom-winfo.rcClient.bottom)-(winfo.rcWindow.top-winfo.rcClient.top);

    // First reflect new state in flags
    SetWindowPos(wnd,NULL,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);

    // post correction
	GetWindowInfo(wnd,&winfo);
    int sx2 = (winfo.rcWindow.right-winfo.rcClient.right)-(winfo.rcWindow.left-winfo.rcClient.left);
    int sy2 = (winfo.rcWindow.bottom-winfo.rcClient.bottom)-(winfo.rcWindow.top-winfo.rcClient.top);

    // set pos, size and flags
    SetWindowPos(wnd,NULL,x,y,w+sx2-sx1,h+sy2-sy1,SWP_NOZORDER);
}

static void threadfun(flext::thr_params *p)
{
    flext::RelPriority(-2);

    VSTPlugin *plug = (VSTPlugin *)p;
    HINSTANCE hinstance = (HINSTANCE)GetModuleHandle(NULL);
    flext::thrid_t thrid = flext::GetThreadId();

    mapmutex.Lock();
    wndmap[thrid] = plug;
    mapmutex.Unlock();    

    char tmp[256];
    sprintf(tmp,"vst~ - %s",plug->GetName());

    // Get size from plugin
	ERect r;
    plug->GetEditorRect(r);

    HWND wnd = CreateWindowEx(
        plug->GetHandle()?WS_EX_APPWINDOW:WS_EX_TOOLWINDOW,
        WCLNAME,tmp,
        WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX, // no border for the beginning to set proper coordinates
        plug->GetX(),plug->GetY(),r.right-r.left,r.bottom-r.top,
        NULL,NULL,
        hinstance,NULL
    );

    if(!wnd) 
        FLEXT_LOG1("wnd == NULL: %i",GetLastError());
    else {
        // idle timer
        SetTimer(wnd,0,TIMER_INTERVAL,NULL);

        // set caption style
        CaptionEditor(plug,plug->GetCaption());

        if(plug->IsVisible()) {
//            SetForegroundWindow(wnd);
            ShowWindow(wnd,1); 

            // notify plugin
    //	    plug->Dispatch(effEditTop,0,0,0,0);
        }
        else
            ShowWindow(wnd,0);

        try 
        {

        // Message loop
        MSG msg;
        BOOL bRet;
        while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0) { 
            if(bRet == -1) {
                // handle the error and possibly exit
                FLEXT_LOG1("GetMessage error: %i",GetLastError());
            }
            else {
                TranslateMessage(&msg); 
//                double tm1 = flext::GetOSTime();
                DispatchMessage(&msg); 
//                double tm2 = flext::GetOSTime();
//                if(tm2-tm1 > 0.01) FLEXT_LOG1("halt %lf",(tm2-tm1)*1000);
            }
        }

        }

        catch(std::exception &e) {
            flext::post("vst~ - exception caught, exiting: %s",e.what());
        }
        catch(...) {
            flext::post("vst~ - exception caught, exiting");
        }

        if(plug) plug->EditingEnded();
    }

    mapmutex.Lock();
    wndmap.erase(thrid);
    mapmutex.Unlock();
}

void SetupEditor()
{
    HINSTANCE hinstance = (HINSTANCE)GetModuleHandle(NULL);

    // Fill in the window class structure with parameters that describe the main window. 
    WNDCLASS wcx; 
    wcx.style = CS_DBLCLKS; // | CS_HREDRAW | CS_VREDRAW;   // redraw if size changes 
    wcx.lpfnWndProc = wndproc;     // points to window procedure 
    wcx.cbClsExtra = 0;                // no extra class memory 
    wcx.cbWndExtra = 0;                // no extra window memory 
    wcx.hInstance = hinstance;         // handle to instance 
    wcx.hIcon = NULL; //LoadIcon(NULL, IDI_APPLICATION);              // predefined app. icon 
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);                    // predefined arrow 
    wcx.hbrBackground = NULL; //GetStockObject(WHITE_BRUSH);                  // white background brush 
    wcx.lpszMenuName = NULL;    // name of menu resource 
    wcx.lpszClassName = WCLNAME;  // name of window class 

    ATOM at = RegisterClass(&wcx); 
    FLEXT_ASSERT(at);
}

void StartEditor(VSTPlugin *p)
{
#ifdef FLEXT_LOGGING
    flext::post("Start editor 1");
#endif
    flext::LaunchThread(threadfun,reinterpret_cast<flext::thr_params *>(p));
#ifdef FLEXT_LOGGING
    flext::post("Start editor 2");
#endif
}

void StopEditor(VSTPlugin *p) 
{
#ifdef FLEXT_LOGGING
    flext::post("Stop editor 1");
#endif
    PostMessage(p->EditorHandle(),WM_CLOSE,0,0); 
//    flext::StopThread(threadfun,reinterpret_cast<flext::thr_params *>(p));
#ifdef FLEXT_LOGGING
    flext::post("Stop editor 2");
#endif
}

void ShowEditor(VSTPlugin *p,bool show) 
{
    ShowWindow(p->EditorHandle(),show); 
}

void MoveEditor(VSTPlugin *p,int x,int y) 
{
    HWND wnd = p->EditorHandle();
    SetWindowPos(wnd,NULL,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
}

void SizeEditor(VSTPlugin *p,int x,int y) 
{
    HWND wnd = p->EditorHandle();
    SetWindowPos(wnd,NULL,0,0,x,y,SWP_NOMOVE|SWP_NOZORDER);
}

void FrontEditor(VSTPlugin *p) 
{
    SetForegroundWindow(p->EditorHandle());
}

void BelowEditor(VSTPlugin *p) 
{
    HWND fg = GetForegroundWindow();
    if(fg) {
        // obviously the following doesn't really work if the window has been created by a different thread...
        BOOL ok = SetWindowPos(p->EditorHandle(),fg,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
    }
}

void CaptionEditor(VSTPlugin *plug,bool c)
{
    HWND wnd = plug->EditorHandle();
    LONG ns,style = GetWindowLong(wnd,GWL_STYLE);
    if(c) ns = style|WS_BORDER|WS_CAPTION;
    else ns = style&~(WS_BORDER|WS_CAPTION);
    if(ns != style) {
        SetWindowLong(wnd,GWL_STYLE,ns);
        windowsize(wnd,plug->GetX(),plug->GetY(),plug->GetW(),plug->GetH());
    }
}

void HandleEditor(VSTPlugin *plug,bool h)
{
    HWND wnd = plug->EditorHandle();
    bool v = plug->IsVisible();
    if(v) ShowWindow(wnd,FALSE);
    SetWindowLong(wnd,GWL_EXSTYLE,h?WS_EX_APPWINDOW:WS_EX_TOOLWINDOW);
    if(v) ShowWindow(wnd,TRUE);
}

void TitleEditor(VSTPlugin *p,const char *t) 
{
    SetWindowText(p->EditorHandle(),t);
}

/*
bool IsEditorShown(const VSTPlugin *p) 
{
    return IsWindowVisible(p->EditorHandle()) != FALSE;
}
*/