#ifndef COMPOSER_EDITOR_H_INCLUDED
#define COMPOSER_EDITOR_H_INCLUDED

#include "PdClasses.hpp"
#include <m_pd.h>

class Editor
{
public:
    static void uploadCode();
    static void init(t_track_proxy *x);
    static void dispatch(t_track_proxy *x, int argc, t_atom* argv);
    static void openWindow(t_track_proxy *x);
    static void closeWindow(t_track_proxy *x);
};

#endif // COMPOSER_EDITOR_H_INCLUDED
