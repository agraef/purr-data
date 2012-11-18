#ifndef COMPOSER_PDCLASSES_H_INCLUDED
#define COMPOSER_PDCLASSES_H_INCLUDED

#include <m_pd.h>

#define MAX_RESULT_SIZE 128

#define TRACK_SELECTOR "#composer::track"
#define SONG_SELECTOR "#composer::song"

class Track;
class Song;

typedef struct _track_proxy
{
    t_object x_obj;
    t_outlet *outlet;
    Track *track;
    t_int editor_open;
    t_symbol *editor_recv;
    t_symbol *editor_send;
} t_track_proxy;

void track_proxy_setup(void);
t_track_proxy *track_proxy_new(t_symbol *song_name, t_symbol *track_name);
void track_proxy_free(t_track_proxy *x);
void track_proxy_save(t_gobj *z, t_binbuf *b);
void track_proxy_data(t_track_proxy *x, t_symbol *s, int argc, t_atom *argv);
void track_proxy_properties(t_gobj *z, t_glist *owner);
void track_proxy_send_result(t_track_proxy *x, int outlet, int editor);

/*#begin methods*/
int track_proxy_meta(t_track_proxy *x, t_symbol *sel, int argc, t_atom *argv);
int track_proxy_editor(t_track_proxy *x, t_symbol *sel, int argc, t_atom *argv);
int track_proxy_getpatterns(t_track_proxy *x);
int track_proxy_getpatternsize(t_track_proxy *x, t_symbol *pat);
int track_proxy_setrow(t_track_proxy *x, t_symbol *sel, int argc, t_atom *argv);
int track_proxy_getrow(t_track_proxy *x, t_symbol *pat, t_floatarg rownum);
int track_proxy_setcell(t_track_proxy *x, t_symbol *sel, int argc, t_atom *argv);
int track_proxy_getcell(t_track_proxy *x, t_symbol *pat, t_floatarg rownum, t_floatarg colnum);
int track_proxy_addpattern(t_track_proxy *x, t_symbol *name, t_floatarg rows, t_floatarg cols);
int track_proxy_removepattern(t_track_proxy *x, t_symbol *pat);
int track_proxy_resizepattern(t_track_proxy *x, t_symbol *pat, t_floatarg rows, t_floatarg cols);
int track_proxy_renamepattern(t_track_proxy *x, t_symbol *oldName, t_symbol *newName);
int track_proxy_copypattern(t_track_proxy *x, t_symbol *src, t_symbol *dst);
/*#end methods*/

#include "methods_pd.hpp"
#include "methods_ed.hpp"

extern "C" void composer_setup(void);

#endif // COMPOSER_PDCLASSES_H_INCLUDED
