/* 
termination - terminated pd

Copyright (c)2008 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.

$LastChangedRevision: 75 $
$LastChangedDate: 2008-03-19 14:51:11 +0100 (Mi, 19 Mär 2008) $
$LastChangedBy: thomas $
*/

#define TERMINATION_VERSION "0.0.1"

#include <flext.h>
#include <set>

#if FLEXT_OS == FLEXT_OS_WIN
#   include <windows.h>
#endif

class termination
    : public flext_base
{
    FLEXT_HEADER_S(termination,flext_base,setup)

public:
    static void setup(t_classid c)
    {
#if FLEXT_OS == FLEXT_OS_WIN
        installpump();
#ifdef PD_DEVEL_VERSION
        sys_callback(dopump,NULL,0);
#else
#   error Need idle callback functionality!
#endif
#else
#   error Platform not supported
#endif

        post("termination - installed termination hook");
        post("Version " TERMINATION_VERSION " - (c)2008 Thomas Grill");
#ifdef FLEXT_DEBUG
        post("Debug version, compiled on " __DATE__ " " __TIME__);
#endif
        post("");
    }
    
    termination()
    {
        AddOutBang();
        objs.insert(this);
    }
    
    ~termination()
    {
        objs.erase(this);
    }
    
protected:

    static void signal()
    {
        post("SIGNAL");
        Lock();
        for(Objs::const_iterator it = objs.begin(); it != objs.end(); ++it)
            (*it)->ToQueueBang(0);
        Unlock();
    }

#if FLEXT_OS == FLEXT_OS_WIN
    static LRESULT CALLBACK pumpfun(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
    {
    #ifdef FLEXT_DEBUG
        fprintf(stderr,"PUMP %i\n",uMsg);
    #endif

        if(uMsg == WM_QUERYENDSESSION) {
            post("END!!!!!");
            exit(0);
//            signal();
            return TRUE;
        }
        else
            return DefWindowProc(hwnd,uMsg,wParam,lParam);
    }

    static HWND window;

    static t_int dopump(t_int *)
    {
        MSG msg;
        int status;
        if(::PeekMessage(& msg, window, 0, 0,PM_NOREMOVE)) {
            post("PUMP");
            status = ::GetMessage (& msg, 0, 0, 0);
            if(status <= 0) return 0;

            ::TranslateMessage (&msg);
            ::DispatchMessage (&msg);
            return 1;
        }
        else
            return 2; 
    }

    static void installpump()
    {
        HINSTANCE inst = GetModuleHandle(NULL);
        WNDCLASS wc;
        wc.style = 0;
        wc.lpfnWndProc = pumpfun; 
        wc.cbClsExtra = 0; 
        wc.cbWndExtra = 0; 
        wc.hInstance = inst; 
        wc.hIcon = NULL; 
        wc.hCursor = NULL; 
        wc.hbrBackground = NULL; 
        wc.lpszMenuName =  NULL; 
        wc.lpszClassName = "termination hidden window class"; 

        RegisterClass(&wc);
        window = CreateWindow(wc.lpszClassName,"termination hidden window",WS_OVERLAPPEDWINDOW|WS_DISABLE,0,0,10,10,NULL,NULL,inst,NULL);
        FLEXT_ASSERT(window);
        
        post("INSTALLED terminator");
    }
#endif

    typedef std::set<termination *> Objs;
    static Objs objs;
};

#if FLEXT_OS == FLEXT_OS_WIN
HWND termination::window = NULL;
#endif

std::set<termination *> termination::objs;

FLEXT_NEW("termination",termination)
