#include "m_pd.h"
#include "g_canvas.h"
#include <stdio.h>

//used for canvas_objtext to differentiate between objects being created by user
//vs. those (re)created by the undo/redo actions
int we_are_undoing = 0;

extern const char *canvas_undo_name;

t_undo_action *canvas_undo_init(t_canvas *x)
{
	t_undo_action *a = (t_undo_action *)getbytes(sizeof(*a));

	a->type = 0;
	a->x = x;
	a->next = NULL;

	if (!x->u_queue) {
		//this is the first init
		x->u_queue = a;
		x->u_last = a;
		a->prev = NULL;
		a->name = "no";
        sys_vgui("pdtk_undomenu .x%lx no no\n", (t_int)a->x);
	}
	else {
		if (x->u_last->next) {
			//we need to rebranch first then add the new action
			canvas_undo_rebranch(x);
		}
		x->u_last->next = a;
		a->prev = x->u_last;
		x->u_last = a;
	} 
	//fprintf(stderr,"canvas_undo_init\n");
	return(a);
}

t_undo_action *canvas_undo_add(t_canvas *x, int type, const char *name, void *data)
{
	fprintf(stderr,"canvas_undo_add %d\n", type);
	t_undo_action *a = canvas_undo_init(x);
	a->type = type;
	a->data = (void *)data;
	a->name = (char *)name;
	canvas_undo_name = name;
    sys_vgui("pdtk_undomenu .x%lx %s no\n", x, a->name);
	return(a);
}

void canvas_undo_undo(t_canvas *x)
{
	if (x->u_queue && x->u_last != x->u_queue) {
		we_are_undoing = 1;
		fprintf(stderr,"canvas_undo_undo %d\n", x->u_last->type);
		glist_noselect(x);
        switch(x->u_last->type)
        {
		    case 1:	canvas_undo_connect(x, x->u_last->data, UNDO_UNDO); break; 		//connect
		    case 2:	canvas_undo_disconnect(x, x->u_last->data, UNDO_UNDO); break; 	//disconnect
		    case 3:	canvas_undo_cut(x, x->u_last->data, UNDO_UNDO); break; 			//cut
		    case 4:	canvas_undo_move(x, x->u_last->data, UNDO_UNDO); break;			//move
		    case 5:	canvas_undo_paste(x, x->u_last->data, UNDO_UNDO); break;		//paste
		    case 9:	canvas_undo_create(x, x->u_last->data, UNDO_UNDO); break;		//create
		    case 10:canvas_undo_recreate(x, x->u_last->data, UNDO_UNDO); break;		//recreate
		    default:
		        error("canvas_undo_undo: unsupported undo command");
        }
		x->u_last = x->u_last->prev;
		char *undo_action = x->u_last->name;
		char *redo_action = x->u_last->next->name;
		we_are_undoing = 0;
		if (glist_isvisible(x) && glist_istoplevel(x)) {
			sys_vgui("pdtk_undomenu .x%lx %s %s\n", x, undo_action, redo_action);
			sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
		}
	}
}

void canvas_undo_redo(t_canvas *x)
{
	if (x->u_queue && x->u_last->next) {
		we_are_undoing = 1;
		x->u_last = x->u_last->next;
		fprintf(stderr,"canvas_undo_redo %d\n", x->u_last->type);
		glist_noselect(x);
        switch(x->u_last->type)
        {
		    case 1:	canvas_undo_connect(x, x->u_last->data, UNDO_REDO); break; 		//connect
		    case 2:	canvas_undo_disconnect(x, x->u_last->data, UNDO_REDO); break; 	//disconnect
		    case 3:	canvas_undo_cut(x, x->u_last->data, UNDO_REDO); break; 			//cut
		    case 4:	canvas_undo_move(x, x->u_last->data, UNDO_REDO); break;			//move
		    case 5:	canvas_undo_paste(x, x->u_last->data, UNDO_REDO); break;		//paste
		    case 9:	canvas_undo_create(x, x->u_last->data, UNDO_REDO); break;		//create
		    case 10:canvas_undo_recreate(x, x->u_last->data, UNDO_REDO); break;		//recreate
		    default:
		        error("canvas_undo_redo: unsupported undo command");
        }
		char *undo_action = x->u_last->name;
		char *redo_action = (x->u_last->next ? x->u_last->next->name : "no");
		we_are_undoing = 0;
		if (glist_isvisible(x) && glist_istoplevel(x)) {
			sys_vgui("pdtk_undomenu .x%lx %s %s\n", x, undo_action, redo_action);
			sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
		}
	}
}

void canvas_undo_rebranch(t_canvas *x)
{
	fprintf(stderr,"canvas_undo_rebranch");
	if (x->u_last->next) {
		t_undo_action *a;
		for(a = x->u_last->next; a; a = a->next) {
			fprintf(stderr,".");
		    switch(a->type)
		    {
				case 1:	canvas_undo_connect(x, a->data, UNDO_FREE); break; 		//connect
				case 2:	canvas_undo_disconnect(x, a->data, UNDO_FREE); break; 	//disconnect
				case 3:	canvas_undo_cut(x, a->data, UNDO_FREE); break; 			//cut
				case 4:	canvas_undo_move(x, a->data, UNDO_FREE); break;			//move
				case 5:	canvas_undo_paste(x, a->data, UNDO_FREE); break;		//paste
				case 9:	canvas_undo_create(x, a->data, UNDO_FREE); break;		//create
				case 10:canvas_undo_recreate(x, a->data, UNDO_FREE); break;		//recreate
				default:
				    error("canvas_undo_rebranch: unsupported undo command");
		    }
			freebytes(a, sizeof(*a));
		}
	}
	fprintf(stderr,"done!\n");
}

void canvas_undo_check_canvas_pointers(t_canvas *x)
{
	//currently unnecessary unless we decide to implement one central undo for all patchers
}

void canvas_undo_purge_abstraction_actions(t_canvas *x)
{
	//currently unnecessary unless we decide to implement one central undo for all patchers
}

void canvas_undo_free(t_canvas *x)
{
	//fprintf(stderr,"canvas_undo_free");
	if (x->u_queue) {
		t_undo_action *a;
		for(a = x->u_queue; a; a = a->next) {
			//fprintf(stderr,".");
		    switch(a->type)
		    {
				case 1:	canvas_undo_connect(x, a->data, UNDO_FREE); break; 		//connect
				case 2:	canvas_undo_disconnect(x, a->data, UNDO_FREE); break; 	//disconnect
				case 3:	canvas_undo_cut(x, a->data, UNDO_FREE); break; 			//cut
				case 4:	canvas_undo_move(x, a->data, UNDO_FREE); break;			//move
				case 5:	canvas_undo_paste(x, a->data, UNDO_FREE); break;		//paste
				case 9:	canvas_undo_create(x, a->data, UNDO_FREE); break;		//create
				case 10:canvas_undo_recreate(x, a->data, UNDO_FREE); break;		//recreate
				default:
				    error("canvas_undo_rebranch: unsupported undo command");
		    }
			freebytes(a, sizeof(*a));
		}
	}
	//fprintf(stderr,"done!\n");
}

