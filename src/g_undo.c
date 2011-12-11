#include "m_pd.h"
#include "g_canvas.h"
#include <stdio.h>

t_undo_action *canvas_undo_init(t_canvas *x)
{
	t_undo_action *a = (t_undo_action *)getbytes(sizeof(*a));

	if (!x->u_queue) {
		//this is the first init
		a->type = 0;
		a->x = x;
		a->prev = NULL;
		a->next = NULL;
		x->u_queue = a;
		x->u_last = a;
	}
	fprintf(stderr,"canvas_undo_init\n");
	return(a);
}

t_undo_action *canvas_undo_add(t_canvas *x)
{
	t_undo_action *a;
	return(a);
}

void canvas_undo_undo(t_canvas *x)
{

}

void canvas_undo_redo(t_canvas *x)
{

}

void canvas_undo_rebranch(t_undo_action *u)
{

}

void canvas_undo_check_canvas_pointers(t_canvas *x)
{

}

void canvas_undo_purge_abstraction_actions(t_canvas *x)
{

}

void canvas_undo_free(t_canvas *x)
{
	fprintf(stderr,"canvas_undo_free\n");
	if (x->u_queue) {
		t_undo_action *a;
		for(a = x->u_queue; a; a = a->next) {
			fprintf(stderr,"freeing t_undo_action queue member\n");
			freebytes(a, sizeof(*a));
		}
	}
}

