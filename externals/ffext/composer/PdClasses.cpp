#include "PdClasses.hpp"
#include "Song.hpp"
#include "Track.hpp"
#include "Pattern.hpp"
#include "Editor.hpp"

#include "callwrappers_pd.cpp"
#include "callwrappers_ed.cpp"

#include <iostream>

#include "Common.hpp"

using std::cout;
using std::cerr;
using std::endl;

t_atom result_argv[MAX_RESULT_SIZE];
int result_argc;

t_class *track_proxy_class;
t_class *song_proxy_class;

void track_proxy_setup(void)
{
    track_proxy_class = class_new(
        gensym("track"),
        (t_newmethod)track_proxy_new,
        (t_method)track_proxy_free,
        sizeof(t_track_proxy),
        CLASS_DEFAULT,
        A_SYMBOL, A_SYMBOL, A_NULL
    );
#include "classsetup.cpp" 
    class_addmethod(track_proxy_class, (t_method)track_proxy_data, \
            gensym("data"), A_GIMME, A_NULL);
#if PD_MINOR_VERSION >= 37
    class_setpropertiesfn(track_proxy_class, track_proxy_properties);
    class_setsavefn(track_proxy_class, track_proxy_save);
#endif
    class_sethelpsymbol(track_proxy_class, gensym("track.pd"));
}

t_track_proxy *track_proxy_new(t_symbol *song_name, t_symbol *track_name)
{
    t_track_proxy *x = (t_track_proxy*)pd_new(track_proxy_class);
    x->outlet = outlet_new(&x->x_obj, &s_list);
    x->editor_open = 0;

    // get or create Track object:
    x->track = Track::byName(song_name->s_name, track_name->s_name);

    // set send/recv for communication with editor    
    Song *song = x->track->getSong();
    string base_name = "track_proxy-" + song->getName() + "-" + x->track->getName();
    string recv_name = base_name + "-r";
    string send_name = base_name + "-s";
    x->editor_recv = gensym(recv_name.c_str());
    x->editor_send = gensym(send_name.c_str());
    pd_bind(&x->x_obj.ob_pd, x->editor_recv);

    // bind to TRACK_SELECTOR for loading in-patch data
    pd_bind(&x->x_obj.ob_pd, gensym(TRACK_SELECTOR));

    Editor::init(x);

    return x;
}

void track_proxy_free(t_track_proxy *x)
{
    pd_unbind(&x->x_obj.ob_pd, gensym(TRACK_SELECTOR));
    /* LATER find a way to get TRACK_SELECTOR unbound earlier (at end of load?) */
    /*t_pd* x2;
    while((x2 = pd_findbyclass(gensym(TRACK_SELECTOR), track_proxy_class)))
        pd_unbind(x2, gensym(TRACK_SELECTOR));*/

    pd_unbind(&x->x_obj.ob_pd, x->editor_recv);
}

void track_proxy_save(t_gobj *z, t_binbuf *b)
{
    t_track_proxy *x = (t_track_proxy*)z;
    Track *t = x->track;
    Song *s = t->getSong();

    binbuf_addv(b, "ssiisss;", gensym("#X"), gensym("obj"),
        (t_int)x->x_obj.te_xpix, (t_int)x->x_obj.te_ypix,
        gensym("track"), gensym(s->getName().c_str()),
	gensym(t->getName().c_str()));

    // save paterns
    for(Track::pattern_iterator i = t->pattern_begin(); i != t->pattern_end(); i++)
    {
        Pattern *pattern = i->second;
        binbuf_addv(b, "ss", gensym(TRACK_SELECTOR), gensym("data"));
        t_int r = pattern->getRows();
        t_int c = pattern->getColumns();
        binbuf_addv(b, "sii", gensym(pattern->getName().c_str()), r, c);
        t_atom tmp;
        for(unsigned int j = 0; j < r; j++)
        {
            for(unsigned int k = 0; k < c; k++)
            {
                tmp = pattern->getCell(j, k);
                switch(tmp.a_type)
                {
                case A_SYMBOL:
                    binbuf_addv(b, "s", tmp.a_w.w_symbol);
                    break;
                case A_FLOAT:
                    binbuf_addv(b, "f", tmp.a_w.w_float);
                    break;
                default:
                    binbuf_addv(b, "s", gensym("?"));
                    break;
                }
            }
        }
        binbuf_addv(b, ";");
    }

    // save metadata
    for(Track::meta_iterator i = t->meta_begin(); i != t->meta_end(); i++)
    {
        binbuf_addv(b, "ssssss;", gensym(TRACK_SELECTOR),
                gensym("meta"), gensym("track"), gensym("set"),
                gensym(i->first.c_str()), gensym(i->second.c_str()));
    }

    binbuf_addv(b, "sss;", gensym(TRACK_SELECTOR), gensym("data"), gensym("end"));
}

void track_proxy_data(t_track_proxy *x, t_symbol *s, int argc, t_atom *argv)
{
    if(argc == 1 && IS_A_SYMBOL(argv,0) && argv[0].a_w.w_symbol == gensym("end"))
    {
        pd_unbind(&x->x_obj.ob_pd, gensym(TRACK_SELECTOR));
        return;
    }
    if(argc < 3 || !IS_A_SYMBOL(argv,0) || !IS_A_FLOAT(argv,1) || !IS_A_FLOAT(argv,2))
    {
        pd_error(x, "unrecognized format for in-patch data");
        return;
    }
    t_symbol *pat = argv[0].a_w.w_symbol;
    t_int rows = (t_int) argv[1].a_w.w_float;
    t_int cols = (t_int) argv[2].a_w.w_float;
    if(argc != (3 + rows * cols))
    {
        pd_error(x, "unrecognized format for in-patch data (malformed data?)");
        return;
    }
    Track *t = x->track;
    t->addPattern(rows, cols, pat->s_name);
    Pattern *p = t->getPattern(pat->s_name);
    if(!p)
    {
        pd_error(x, "fatal error: cannot create patern");
        return;
    }
    int a = 3;
    for(int r = 0; r < rows; r++)
    {
        for(int c = 0; c < cols; c++)
        {
            p->setCell(r, c, argv[a]);
            a++;
        }
    }
}

void track_proxy_properties(t_gobj *z, t_glist *owner)
{
    t_track_proxy *x = (t_track_proxy *) z;
    if(!x->editor_open) Editor::openWindow(x);
    else Editor::closeWindow(x);
}

void track_proxy_send_result(t_track_proxy *x, int outlet, int editor)
{
    if(result_argc <= 0) return;
    if(outlet)
    {
        outlet_list(x->outlet, &s_list, result_argc, &result_argv[0]);
    }
    if(editor)
    {
        Editor::dispatch(x, result_argc, &result_argv[0]);
    }
}

int track_proxy_meta(t_track_proxy *x, t_symbol *sel, int argc, t_atom *argv)
{
    result_argc = 0;
    t_symbol *action = 0, *target = 0, *key = 0;
    int w = -1;

    if(argc < 3 || !IS_A_SYMBOL(argv,0) || !IS_A_SYMBOL(argv,1) || !IS_A_SYMBOL(argv,2))
    {
        pd_error(x, "meta: bad arguments");
        goto usage;
    }
    target = argv[0].a_w.w_symbol;
    action = argv[1].a_w.w_symbol;
    key = argv[2].a_w.w_symbol;

    if(action == gensym("get")) w = 0;
    if(action == gensym("set")) w = 1;
    if(w < 0) goto badargs;

    if(target == gensym("song"))
    {
        pd_error(x, "meta: %s target not implemented yet", target->s_name);
        return -1;
    }
    else if(target == gensym("track"))
    {
        string arg = "";
        string atomStr = "";
        for(int i = 3; i < argc; i++)
        {
            if(arg.length()) arg += " ";
            char buf[MAXPDSTRING];
            atom_string(&argv[i], buf, MAXPDSTRING);
            atomStr = buf;
            if(atomStr.find(" ", 0) != string::npos)
                arg += "{" + atomStr + "}";
            else
                arg += atomStr;
        }

        if(w)
        {
            if(argc < 4) goto badargs;
            x->track->setMeta(key->s_name, arg);
        }
        else
        {
            if(argc < 3) goto badargs;
            string value = "";
            if(argc == 3 && !x->track->hasMeta(key->s_name))
            {
                pd_error(x, "meta: key '%s' does not exist into %s", key->s_name, target->s_name);
                return -5;
            }
            if(x->track->hasMeta(key->s_name))
                value = x->track->getMeta(key->s_name);
            else
                value = arg;
            SETSYMBOL(&result_argv[0], gensym("meta"));
            SETSYMBOL(&result_argv[1], target);
            SETSYMBOL(&result_argv[2], key);
            SETSYMBOL(&result_argv[3], gensym(value.c_str()));
            result_argc = 4;
        }
        return 0;
    }

badargs:
    pd_error(x, "meta: bad arguments");
usage:
    post("usage: meta song|track set <key> <value>");
    post("       meta song|track get <key>");
    return 1;
}

int track_proxy_editor(t_track_proxy *x, t_symbol *sel, int argc, t_atom *argv)
{
    result_argc = 0;
    t_symbol *arg1 = 0, *arg2 = 0, *arg3 = 0;
    if(argc < 1 || !IS_A_SYMBOL(argv,0))
    {
        pd_error(x, "editor: missing subcommand");
        goto usage;
    }

    arg1 = argv[0].a_w.w_symbol;
    if(arg1 == gensym("show"))
    {
        Editor::openWindow(x);
    }
    else if(arg1 == gensym("hide"))
    {
        Editor::closeWindow(x);
    }
    else if(arg1 == gensym("toggle"))
    {
        if(!x->editor_open) Editor::openWindow(x);
        else Editor::closeWindow(x);
    }
    else
    {
        pd_error(x, "editor: unknown subcommand: %s", arg1->s_name);
        goto usage;
    }
    return 0;

usage:
    post("track: editor: available subcommands:");
    post("    editor show");
    post("    editor hide");
    post("    editor toggle");
    return 1;
}

int track_proxy_getpatterns(t_track_proxy *x)
{
    SETSYMBOL(&result_argv[0], gensym("patternnames"));
    result_argc = 1;
    Track *t = x->track;
    for(Track::pattern_iterator i = t->pattern_begin(); i != t->pattern_end(); i++)
    {
        if(result_argc >= MAX_RESULT_SIZE)
        {
            pd_error(x, "getpatternnames: result too long");
            return -2;
        }
        Pattern *pattern = i->second;
        SETSYMBOL(&result_argv[result_argc], gensym(pattern->getName().c_str()));
        result_argc++;
    }
    return 0;
}

int track_proxy_getpatternsize(t_track_proxy *x, t_symbol *pat)
{
    Pattern *pattern = x->track->getPattern(pat->s_name);
    if(!pattern)
    {
        pd_error(x, "getpatternsize: no such pattern: %s", pat->s_name);
        return -1;
    }
    SETSYMBOL(&result_argv[0], gensym("patternsize"));
    SETSYMBOL(&result_argv[1], pat);
    SETFLOAT(&result_argv[2], pattern->getRows());
    SETFLOAT(&result_argv[3], pattern->getColumns());
    result_argc = 4;
    return 0;
}

int track_proxy_setrow(t_track_proxy *x, t_symbol *sel, int argc, t_atom *argv)
{
    result_argc = 0;
    if(argc < 2 || !IS_A_SYMBOL(argv,0) || !IS_A_FLOAT(argv,1))
    {
        pd_error(x, "setrow: usage: setrow <pattern> <row#> <atom0> <atom1> ...");
        return -1;
    }
    t_symbol *pat = argv[0].a_w.w_symbol;
    Pattern *pattern = x->track->getPattern(pat->s_name);
    t_int r = (t_int) argv[1].a_w.w_float;
    if(!pattern)
    {
        pd_error(x, "setrow: no such pattern: %s", pat->s_name);
        return -2;
    }
    unsigned int argc2 = argc - 2;
    if(argc2 != pattern->getColumns())
    {
        pd_error(x, "setrow: input error: must provide exactly %d elements for a row", pattern->getColumns());
        return -3;
    }
    for(unsigned int i = 0; i < argc2; i++)
    {
        pattern->setCell(r, i, argv[i + 2]);
    }
    return 0;
}

int track_proxy_getrow(t_track_proxy *x, t_symbol *pat, t_floatarg rownum)
{
    t_int r = (t_int) rownum;
    Pattern *pattern = x->track->getPattern(pat->s_name);
    if(!pattern)
    {
        pd_error(x, "getrow: no such pattern: %s", pat->s_name);
        return -2;
    }
    SETSYMBOL(&result_argv[0], gensym("patternrow"));
    SETSYMBOL(&result_argv[1], pat);
    SETFLOAT(&result_argv[2], (t_float) r);
    result_argc = 3;
    for(unsigned int i = 0; i < pattern->getColumns(); i++)
    {
        if(result_argc >= MAX_RESULT_SIZE)
        {
            pd_error(x, "getrow: result too long");
            return -2;
        }
        result_argv[result_argc] = pattern->getCell(r, i);
        result_argc++;
    }
    return 0;
}

int track_proxy_setcell(t_track_proxy *x, t_symbol *sel, int argc, t_atom *argv)
{
    result_argc = 0;
    if(argc != 4 || !IS_A_SYMBOL(argv,0) || !IS_A_FLOAT(argv,1) || !IS_A_FLOAT(argv,2))
    {
        pd_error(x, "setrow: usage: setcell <pattern> <row#> <col#> <atom>");
        return -1;
    }
    t_symbol *pat = argv[0].a_w.w_symbol;
    Pattern *pattern = x->track->getPattern(pat->s_name);
    t_int r = (t_int) argv[1].a_w.w_float;
    t_int c = (t_int) argv[2].a_w.w_float;
    if(!pattern)
    {
        pd_error(x, "setcell: no such pattern: %s", pat->s_name);
        return -2;
    }
    pattern->setCell(r, c, argv[3]);
    return 0;
}

int track_proxy_getcell(t_track_proxy *x, t_symbol *pat, t_floatarg rownum, t_floatarg colnum)
{
    t_int r = (t_int) rownum;
    t_int c = (t_int) colnum;
    Pattern *pattern = x->track->getPattern(pat->s_name);
    if(!pattern)
    {
        pd_error(x, "getcell: no such pattern: %s", pat->s_name);
        return -2;
    }
    SETSYMBOL(&result_argv[0], gensym("patterncell"));
    SETSYMBOL(&result_argv[1], pat);
    SETFLOAT(&result_argv[2], (t_float) r);
    SETFLOAT(&result_argv[3], (t_float) c);
    result_argv[4] = pattern->getCell(r, c);
    result_argc = 5;
    return 0;
}


int track_proxy_addpattern(t_track_proxy *x, t_symbol *name, t_floatarg rows, t_floatarg cols)
{
    result_argc = 0;
    t_int r = (t_int) rows;
    t_int c = (t_int) cols;
    Pattern *pattern = x->track->getPattern(name->s_name);
    if(pattern)
    {
        pd_error(x, "addpattern: pattern already exist: %s", name->s_name);
        return -3;
    }
    x->track->addPattern(r, c, string(name->s_name));
    return 0;
}

int track_proxy_removepattern(t_track_proxy *x, t_symbol *pat)
{
    result_argc = 0;
    Pattern *pattern = x->track->getPattern(pat->s_name);
    if(!pattern)
    {
        pd_error(x, "removepattern: no such pattern: %s", pat->s_name);
        return -2;
    }
    x->track->removePattern(pat->s_name);
    return 0;
}

int track_proxy_resizepattern(t_track_proxy *x, t_symbol *pat, t_floatarg rows, t_floatarg cols)
{
    result_argc = 0;
    t_int r = (t_int) rows;
    t_int c = (t_int) cols;
    Pattern *pattern = x->track->getPattern(pat->s_name);
    if(!pattern)
    {
        pd_error(x, "resizepattern: no such pattern: %s", pat->s_name);
        return -2;
    }
    if(rows < 1 || cols < 1)
    {
        pd_error(x, "resizepattern: rows and columns must be positive");
        return -6;
    }
    pattern->resize(r, c);
    return 0;
}

int track_proxy_renamepattern(t_track_proxy *x, t_symbol *oldName, t_symbol *newName)
{
    result_argc = 0;
    Pattern *pattern = x->track->getPattern(oldName->s_name);
    if(!pattern)
    {
        pd_error(x, "renamepattern: no such pattern: %s", oldName->s_name);
        return -2;
    }
    pattern = x->track->getPattern(newName->s_name);
    if(pattern)
    {
        pd_error(x, "renamepattern: destination pattern already exist: %s", newName->s_name);
        return -3;
    }
    x->track->renamePattern(oldName->s_name, newName->s_name);
    return 0;
}

int track_proxy_copypattern(t_track_proxy *x, t_symbol *src, t_symbol *dst)
{
    result_argc = 0;
    Pattern *pattern = x->track->getPattern(src->s_name);
    if(!pattern)
    {
        pd_error(x, "copypattern: no such pattern: %s", src->s_name);
        return -2;
    }
    pattern = x->track->getPattern(dst->s_name);
    if(pattern)
    {
        pd_error(x, "copypattern: destination pattern already exist: %s", dst->s_name);
        return -3;
    }
    x->track->copyPattern(src->s_name, dst->s_name);
    return 0;
}

void composer_setup()
{
    track_proxy_setup();
}
