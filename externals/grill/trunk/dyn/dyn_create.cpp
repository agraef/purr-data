/* 
dyn - dynamical object management

Copyright (c)2003-2004 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "dyn_proto.h"
#include <list>

static const t_symbol *k_obj = gensym("obj");
static const t_symbol *k_msg = gensym("msg");
static const t_symbol *k_text = gensym("text");

static const t_symbol *sym_vis = gensym("vis");
static const t_symbol *sym_loadbang = gensym("loadbang");
static const t_symbol *sym_pd = gensym("pd");
static const t_symbol *sym_dsp = gensym("dsp");

static const t_symbol *sym_dyn = gensym("dyn");
static const t_symbol *sym_dot = gensym(".");


static t_gobj *getlast(t_glist *gl)
{
    t_gobj *go = gl->gl_list;
    if(go)
        while(go->g_next) go = go->g_next;
    return go;
}


void *NewPDObject(int type,t_glist *glist,const t_symbol *hdr,int _argc_,const t_atom *_argv_)
{
//    sys_lock();

    const t_symbol *kind;
    switch(type) {
        case DYN_TYPE_PATCHER:
            hdr = sym_pd;
            // fall through
        case DYN_TYPE_OBJECT:
            kind = k_obj;
            break;
        case DYN_TYPE_MESSAGE:
            kind = k_msg;
            ASSERT(hdr == NULL);
            break;
        case DYN_TYPE_TEXT:
            kind = k_text;
            ASSERT(hdr == NULL);
            break;
    }

    void *newest = NULL;
    t_gobj *last = NULL;

    if(type == DYN_TYPE_PATCHER && !glist) {
        /* 
            For a dyn root canvas we can not simply put a [pd] into canvas_getcurrent 
            because the [pd] would be visible in this canvas then.

            On the other hand, we can also not simply create a new canvas with 
            canvas_getcurrent active because it would not be on the list of root dsp canvases
            then.

            Hence, we have to pop all current canvases to be at the root, create our canvas
            to be a real root canvas and then push back all the canvases.
        */

        /* 
            remember current directory -
            abstractions residing in the directory of the current canvas
            (which normally is the one hosting dyn) will be found
        */
        t_symbol *dir;
        if(canvas_getcurrent()) dir = canvas_getcurrentdir();
        else dir = const_cast<t_symbol *>(sym_dot);

        // pop current canvases
        std::list<t_glist *> glstack;
        for(;;) {
            t_glist *gl = canvas_getcurrent();
            if(!gl) break;
            glstack.push_front(gl);
            canvas_unsetcurrent(gl);
        }

        // set canvas environment
        // this must be done manually if there is no owner
        glob_setfilename(NULL,const_cast<t_symbol *>(sym_dyn),dir);

        t_atom arg[6];
	    SETFLOAT(arg+0,0);	// xpos
	    SETFLOAT(arg+1,0);	// ypos
	    SETFLOAT(arg+2,1000);	// xwidth 
	    SETFLOAT(arg+3,1000);	// xwidth 
	    SETSYMBOL(arg+4,const_cast<t_symbol *>(sym_dyncanvas));	// canvas name
	    SETFLOAT(arg+5,0);	// invisible

	    t_glist *canvas = canvas_new(NULL,NULL,6,arg);
        /* or, alternatively - but this needs some message processing
        pd_typedmess(&pd_canvasmaker,gensym("canvas"),6,arg);
	    t_glist *canvas = canvas_getcurrent();
        */

        // must do that....
	    canvas_unsetcurrent(canvas);

        // push back all the canvases
        for(std::list<t_glist *>::iterator it = glstack.begin(); it != glstack.end(); ++it)
            canvas_setcurrent(*it);

        // clear environment
        glob_setfilename(NULL,&s_,&s_);

        newest = canvas;
    }
    else {
        ASSERT(glist);

        int argc = _argc_+(hdr?3:2);
        t_atom *argv = new t_atom[argc];

	    // position x/y = 0/0
        t_atom *a = argv;
	    SETFLOAT(a,0); a++;
        SETFLOAT(a,0); a++;
        if(hdr) { SETSYMBOL(a,const_cast<t_symbol *>(hdr)); a++; }
        memcpy(a,_argv_,_argc_*sizeof(t_atom));

        last = getlast(glist);

	    // set selected canvas as current
        pd_typedmess((t_pd *)glist,(t_symbol *)kind,argc,argv);
//        canvas_obj(glist,(t_symbol *)kind,argc,argv);
        newest = getlast(glist);

        delete[] argv;
    }

    if(kind == k_obj && glist) {
        // check for created objects and abstractions

        t_object *o = (t_object *)pd_newest();

        if(!o) {
            // PD creates a text object when the intended object could not be created
            t_gobj *trash = getlast(glist);

            // Test for newly created object....
            if(trash && last != trash) {
                // Delete it!
                glist_delete(glist,trash);
            }
            newest = NULL;
        }
        else
            newest = &o->te_g;
    }

	// look for latest created object
	if(newest) {
//	    if(glist) canvas_setcurrent(glist); 

		// send loadbang (if it is an abstraction)
		if(pd_class(&((t_gobj *)newest)->g_pd) == canvas_class) {
			// hide the sub-canvas
			pd_vmess((t_pd *)newest,const_cast<t_symbol *>(sym_vis),"i",0);

            // loadbang the abstraction
			pd_vmess((t_pd *)newest,const_cast<t_symbol *>(sym_loadbang),"");
        }

		// restart dsp - that's necessary because ToCanvas is called manually
		canvas_update_dsp();

    	// pop the current canvas 
//	    if(glist) canvas_unsetcurrent(glist); 
    }

//    sys_unlock();

    return newest;
}

dyn_patcher *root = NULL;

dyn_ident *NewObject(int type,dyn_callback cb,dyn_ident *owner,const t_symbol *hdr,int argc,const t_atom *argv)
{
    int err = DYN_ERROR_NONE;
    dyn_ident *ret = NULL;

    dyn_patcher *p;
    if(owner == DYN_ID_NONE) {
        if(!root) {
            void *newobj = NewPDObject(DYN_TYPE_PATCHER,NULL,NULL);
            dyn_ident *id = new dyn_ident(DYN_TYPE_PATCHER,NULL);
            root = new dyn_patcher(id,NULL,(t_glist *)newobj);
        }
        p = root;
    }
    else
        p = owner->Patcher();

    if(p) {
        void *newobj = NewPDObject(type,p->glist(),hdr,argc,argv);
        if(newobj) {
            ret = new dyn_ident(type,cb);

            switch(type) {
                case DYN_TYPE_PATCHER:
                    ret->Set(new dyn_patcher(ret,p,(t_glist *)newobj));
                    break;
                case DYN_TYPE_OBJECT:
                    ret->Set(new dyn_object(ret,p,(t_gobj *)newobj));
                    break;
                case DYN_TYPE_MESSAGE:
                    ret->Set(new dyn_message(ret,p,(t_gobj *)newobj));
                    break;
                case DYN_TYPE_TEXT:
                    ret->Set(new dyn_text(ret,p,(t_gobj *)newobj));
                    break;
            }
        }
        else
            err = DYN_ERROR_NOTCREATED;
    }
    else
        err = DYN_ERROR_NOSUB;

    if(err != DYN_ERROR_NONE) {
        if(ret) delete ret;
        throw err;
    }
    else
        return ret;
}

void DelObject(dyn_ident *obj)
{
    ASSERT(obj);
    if(obj->data) Destroy(obj->data); // delete database object
    delete obj; // delete ID
}
