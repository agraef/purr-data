#define FLEXT_ATTRIBUTES 1
#include <flext.h>

#include <pthread.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

#ifdef __linux__
#include <X11/Xlib.h>
#endif

class mousewarp
    : public flext_base
{
    FLEXT_HEADER_S(mousewarp,flext_base,setup)

public:
    
    void mg_pos(AtomList &l)
    {
    #ifdef _WIN32
        POINT pt;
        if(GetCursorPos(&pt)) {
            l(2);
            SetInt(l[0],pt.x);
            SetInt(l[1],pt.y);
        }
    #elif defined(__APPLE__)
        // see  Carbon Event Manager Tasks -> Tracking Mouse Movements
        // we should avoid this because of excessive CPU usage as the documentation says... 
        // does fltk store the mouse position somewhere... otherwise we should do in MAG 
        Point pt;
        GetMouse(&pt);  // can't fail
        l(2);
        SetInt(l[0],pt.h);
        SetInt(l[1],pt.v);
    #elif defined(__linux__)
        Display * display = XOpenDisplay (NULL);
        assert(display);
        Window dummy_window;
        int dummy_int(0),x,y;
        unsigned int mask_return(0);
        XQueryPointer(display, RootWindow (display, DefaultScreen (display)), &dummy_window, &dummy_window,
            x, y, &dummy_int, &dummy_int, &mask_return);
        XCloseDisplay(display);
        l(2);
        SetInt(l[0],x);
        SetInt(l[1],y);
    #else
    #error Not implemented
    #endif
    }
    
    void ms_pos(const AtomList &l)
    {
        if(l.Count() != 2 || !CanbeInt(l[0]) || !CanbeInt(l[1])) return;
    
    #ifdef _WIN32
        SetCursorPos(GetAInt(l[0]),GetAInt(l[1])));
    #elif defined(__APPLE__)
        CGPoint pt;
        pt.x = GetAInt(l[0]),pt.y = GetAInt(l[1]);
        // move the cursor to a global display position
		CGDisplayMoveCursorToPoint(CGMainDisplayID(),pt);
//        CGWarpMouseCursorPosition(pt);
    #elif defined (__linux__)
        Display * display = XOpenDisplay (NULL);
        assert(display);
        XWarpPointer(display, None, RootWindow (display, DefaultScreen (display)), 0, 0, 0, 0, GetAInt(l[0]), GetAInt(l[1]));
        XCloseDisplay(display);
    #else
    #error Not implemented
    #endif
    }

#if 0    
    MAG_EXPORT mag_error mag_MousePosRel(int rx,int ry)
    {
    #ifdef _WIN32
        POINT pt;
        if(GetCursorPos(&pt) && SetCursorPos(pt.x+rx,pt.y+ry))
            return MAG_ERROR_NONE;
        else
            return MAG_ERROR_GENERAL;
    #else
        int x,y;
        mag_error err = mag_MousePosQ(&x,&y);
        if(err == MAG_ERROR_NONE)
            return mag_MousePosAbs(x+rx,y+ry);
        else
            return err;
    #endif
    }
#endif
    
    FLEXT_CALLVAR_V(mg_pos,ms_pos)
    
    static void setup(t_classid c)
    {
        FLEXT_CADDATTR_VAR(c,"pos",mg_pos,ms_pos);
    }
};


FLEXT_NEW("mousewarp",mousewarp)
