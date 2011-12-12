#include "m_pd.h"
#include "g_canvas.h"
#include <stdio.h>

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
        sys_vgui("pdtk_undomenu nobody no no\n");
	}
	else {
		if (x->u_last->next) {
			//we need to rebranch first then add the new action
			canvas_undo_rebranch(x->u_last);
		}
		x->u_last->next = a;
		a->prev = x->u_last;
		x->u_last = a;
	} 
	fprintf(stderr,"canvas_undo_init\n");
	return(a);
}

t_undo_action *canvas_undo_add(t_canvas *x, int type, const char *name)
{
	fprintf(stderr,"canvas_undo_add\n");
	t_undo_action *a = canvas_undo_init(x);
	
	//TODO: parse out type and fill accordingly
	if (type == 8) { //create
		t_undo_create *data = (t_undo_create *)canvas_undo_set_create(x);
		a->type = 8;
		a->data = (void *)data;
	}
	a->name = name;
    sys_vgui("pdtk_undomenu .x%lx %s no\n", x, a->name);
	return(a);
}

void canvas_undo_undo(t_canvas *x)
{
	fprintf(stderr,"canvas_undo_undo\n");
	if (x->u_queue && x->u_last != x->u_queue) {
		fprintf(stderr,"do it\n");
		glist_noselect(x);
		if (x->u_last->type == 8) { //create
			canvas_undo_create(x, x->u_last->data, UNDO_UNDO);
		}
		x->u_last = x->u_last->prev;
		char *undo_action = x->u_last->name;
		char *redo_action = x->u_last->next->name;
		if (glist_isvisible(x) && glist_istoplevel(x)) {
			sys_vgui("pdtk_undomenu .x%lx %s %s\n", x, undo_action, redo_action);
			sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
		}
	}
}

void canvas_undo_redo(t_canvas *x)
{
	fprintf(stderr,"canvas_undo_redo\n");
	if (x->u_queue && x->u_last->next) {
		fprintf(stderr,"do it\n");
		x->u_last = x->u_last->next;
		glist_noselect(x);
		if (x->u_last->type == 8) { //create
			canvas_undo_create(x, x->u_last->data, UNDO_REDO);
		}
		char *undo_action = x->u_last->name;
		char *redo_action = (x->u_last->next ? x->u_last->next->name : "no");
		if (glist_isvisible(x) && glist_istoplevel(x)) {
			sys_vgui("pdtk_undomenu .x%lx %s %s\n", x, undo_action, redo_action);
			sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
		}
	}
}

void canvas_undo_rebranch(t_undo_action *u)
{
	fprintf(stderr,"canvas_undo_rebranch");
	if (u->next) {
		t_undo_action *a;
		for(a = u->next; a; a = a->next) {
			fprintf(stderr,".");
			if (a->type == 8) { //create
				canvas_undo_create(a->x, a->data, UNDO_FREE);
			}
			freebytes(a, sizeof(*a));
		}
	}
	fprintf(stderr,"done!\n");
}

void canvas_undo_check_canvas_pointers(t_canvas *x)
{

}

void canvas_undo_purge_abstraction_actions(t_canvas *x)
{

}

void canvas_undo_free(t_canvas *x)
{
	fprintf(stderr,"canvas_undo_free");
	if (x->u_queue) {
		t_undo_action *a;
		for(a = x->u_queue; a; a = a->next) {
			fprintf(stderr,".");
			if (a->type == 8) { //create
				canvas_undo_create(a->x, a->data, UNDO_FREE);
			}
			freebytes(a, sizeof(*a));
		}
	}
	fprintf(stderr,"done!\n");
}

