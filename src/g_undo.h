
#ifndef __g_undo_h_
#define __g_undo_h_

/*
Infinite undo by Ivica Ico Bukvic <ico@vt.edu> Dec. 2011

This is the home of infinite undo queue. Each root canvas has one of these.
Only canvas that is root will instantiate the pointer to t_undo_action struct.
All sub-canvases (including abstractions) will be linked to the root window.
Once the root window is destroyed, so is its undo queue. Each canvas (t_glist)
defined in g_canvas.h now also has information on t_undo_action queue and a
pointer to the last item in the doubly-linked list (queue).

First initialized undo is never filled (it is of a type 0). First (new) action
creates another t_undo_action and saves its contents there and updates "last"
pointer inside the t_canvas (t_glist).

t_undo_action requires canvas information in case we've deleted a canvas and
then undoed its deletion which will in effect change its pointer (memory
location). Then we need to call check_canvas_pointers to update all of the old
(stale) undo actions to corresepond with the new memory location.

What about abstractions? Once they are recreated  (e.g. after delete, followed
by an undo) all undo actions (except for its deletion in the parent window should
be purged since abstraction's state will now default to its original (saved) state.

Types of undo data:
0 - init data (start of the queue)
1 - connect & disconnect 
2 - cut, clear & typing into objects
3 - motion, inclding "tidy up" and stretching
4 - paste & duplicate
5 - apply
6 - arrange (to front/back)
7 - canvas apply
8 - create
*/

struct _undo_action
{
	t_canvas *x;				/* canvas undo is associated with */
	int type;					/* defines what kind of data container it is */
	void *data;					/* each action will have a different data container */
	char *name;					/* name of current action */
	struct _undo_action *prev;	/* previous undo action */
	struct _undo_action *next;	/* next undo action */
} t_undo_action;

#ifndef t_undo_action
#define t_undo_action struct _undo_action
#endif

EXTERN t_undo_action *canvas_undo_init(t_canvas *x);
EXTERN t_undo_action *canvas_undo_add(t_canvas *x, int type, const char *name);
EXTERN void canvas_undo_undo(t_canvas *x);
EXTERN void canvas_undo_redo(t_canvas *x);
EXTERN void canvas_undo_rebranch(t_undo_action *u);
EXTERN void canvas_undo_check_canvas_pointers(t_canvas *x);
EXTERN void canvas_undo_purge_abstraction_actions(t_canvas *x);
EXTERN void canvas_undo_free(t_canvas *x);

/* --------- 8. create ----------- */

typedef struct _undo_create      
{
    int u_index;    			/* index of the created object object */
    t_binbuf *u_objectbuf;      /* the object cleared or typed into */
    t_binbuf *u_reconnectbuf;   /* connections into and out of object */
} t_undo_create;

extern void canvas_undo_create(t_canvas *x, void *z, int action);
extern void *canvas_undo_set_create(t_canvas *x);

/* ------------------------------- */

#endif /* __g_undo_h_ */
