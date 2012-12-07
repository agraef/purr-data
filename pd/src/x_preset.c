/*	preset_node and preset_hub implementation by Ivica Ico Bukvic <ico@vt.edu> (c) 2012
	distributed under GPL v3 license or newer
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m_pd.h"
#include "g_canvas.h"
#include "x_preset.h"
#include "s_stuff.h"

#define PH_DEBUG 0

//		changes in order happen when doing one of the following: cut, 
//		undo cut, delete, undo delete, to front, and to back.
//		this will trigger cascading check for all preset_nodes 
//		is there a way to limit it to ones that are affected?
//		potentially... by taking all the ordinal values above the one moved
//		e.g. if object 1,5 was moved, everything above it
//			 (1,5,1 or 1,6 etc.) needs to be reevaluated BEFORE
//			 reevaluating moved object (if applicable)
//			 this does not affect objects like 2 or 2,7
//			 since it is a different branch altogether
//			 this is however, not currently implemented as said changes
//			 typically occur during editing and as such have little to no
//			 impact on the cpu optimization. live-coding may be affected,
//			 however, and further investigation there may be warranted.

/*	gl_loc int array explanation:

	Each number corresponds to object or parent's location starting with root window.
	This ensures that each object has a unique ID within the preset even when using
	multiple instances of the same abstraction within the same patch.
	This also means that if the object has changed its location within the gl_list
	(e.g. via to_front/to_back), all corresponding IDs have to be updated in the parent.

	EXAMPLE:
	1,4,7 means the preset_node is 8th object in the subpatch or abstraction that is a
	child of 5th object in a parent patch, that is a child of 2nd object of the root
	patch.
*/

//==================== forward declarations ========================//

void preset_hub_add_a_node(t_preset_hub *h, t_preset_node *x);
void preset_hub_recall(t_preset_hub *h, t_float f);
void preset_hub_store(t_preset_hub *h, t_float f);
void preset_hub_add_a_node(t_preset_hub *h, t_preset_node *x);
void preset_hub_delete_a_node(t_preset_hub *h, t_preset_node *x);
void preset_node_seek_hub(t_preset_node *x);
int  preset_hub_compare_loc(int *h_loc, int h_loc_length, int *n_loc, int n_loc_length);
void preset_hub_reset(t_preset_hub *h);
void preset_hub_purge(t_preset_hub *h);
void preset_hub_clear(t_preset_hub *x, t_float f);

static int preset_node_location_changed(t_preset_node *x);
static void preset_node_update_my_glist_location(t_preset_node *x);

//======================== global vars =============================//

/*	following contains a linear list of all hubs and nodes which ensures easy access,
	pairing, deletion, etc. of all nodes and hubs regardless whether they are a part of
	subpatches, root canvases, or abstractions.
*/

t_glob_preset_node_list *gpnl;
t_glob_preset_hub_list *gphl;

int glob_preset_node_list_add(t_preset_node *x)
{
	if(PH_DEBUG) fprintf(stderr,"glob_preset_node_list_add\n");
	t_glob_preset_node_list *n1, *n2;

	if (!gpnl) {
		gpnl = (t_glob_preset_node_list *)t_getbytes(sizeof(*gpnl));
		n2 = gpnl;
	} else {
		n1 = gpnl;
		while(n1->gpnl_next)
			n1 = n1->gpnl_next;
		n2 = (t_glob_preset_node_list *)t_getbytes(sizeof(*n2));
		n1->gpnl_next = n2;
	}
	n2->gpnl_paired = 0;
	n2->gpnl_node = x;
	return(0);
}

int glob_preset_node_list_delete(t_preset_node *x)
{
	if(PH_DEBUG) fprintf(stderr,"glob_preset_node_list_delete\n");
	t_glob_preset_node_list *n1, *n2;
	int found;

	if (!gpnl)
		return(1);

	n1 = gpnl;
	n2 = gpnl;

	// first check if we are the first gpnl
	if (n2->gpnl_node == x) {
		gpnl = n2->gpnl_next;
		freebytes(n2, sizeof(*n2));
	} 
	else {
		while (n2->gpnl_node != x && n2->gpnl_next) {
			n1 = n2;
			n2 = n2->gpnl_next;
		}
		if (n2->gpnl_node == x) {
			n1->gpnl_next = n2->gpnl_next;
			freebytes(n2, sizeof(*n2));
		} else {
			// we should never get here
			if(PH_DEBUG) fprintf(stderr,"error, could not find appropriate gpnl to delete\n");
			return(1);
		}
	}
	
	return(0);
}

int glob_preset_node_list_update_paired(t_preset_node *x, int paired)
{
	if(PH_DEBUG) fprintf(stderr,"glob_preset_node_list_update_paired %d\n", paired);
	t_glob_preset_node_list *n;

	if (!gpnl)
		return(1);

	n = gpnl;

	while (n->gpnl_node != x && n->gpnl_next) {
		n = n->gpnl_next;
	}
	if (n->gpnl_node == x) {
		n->gpnl_paired = 1;
	}
	else {
		// we should never get here
		if(PH_DEBUG) fprintf(stderr,"error, could not find appropriate gpnl to pair\n");
		return(1);
	}
	
	return(0);
}

void glob_preset_node_list_seek_hub(void)
{
	if(PH_DEBUG) fprintf(stderr,"glob_preset_node_list_seek_hub\n");
	if (we_are_undoing)
		return;
	t_glob_preset_node_list *nl;

	if (gpnl) {
		nl = gpnl;
		while(nl) {
			if(PH_DEBUG) fprintf(stderr,"	got node\n");
			if (!nl->gpnl_paired) {
				if(PH_DEBUG) fprintf(stderr,"	seeking\n");
				preset_node_seek_hub(nl->gpnl_node);
			}
			nl = nl->gpnl_next;
		}
	}
	if(PH_DEBUG) fprintf(stderr,"	done\n");
}

// this should be called whenever glist has been changed (tofront/back, cut, delete, undo/redo cut/delete)
void glob_preset_node_list_check_loc_and_update(void)
{
	if(PH_DEBUG) fprintf(stderr,"glob_preset_node_list_check_loc_and_update\n");
	t_glob_preset_hub_list *hl;
	t_preset_hub_data *hd;
	int i = 0;

	if (gphl) {
		hl = gphl;
		while(hl) {
			if(PH_DEBUG) fprintf(stderr,"	searching\n");
			hd = hl->gphl_hub->ph_data;
			while (hd) {
				if(PH_DEBUG) fprintf(stderr,"	checking data\n");
				if (hd->phd_node) {
					if(PH_DEBUG) fprintf(stderr,"	node is active\n");
					preset_node_update_my_glist_location(hd->phd_node);
					if (preset_node_location_changed(hd->phd_node)) {
						if(PH_DEBUG) fprintf(stderr,"	location changed...adjusting length %d to %d\n", hd->phd_pn_gl_loc_length, hd->phd_node->pn_gl_loc_length);
						hd->phd_pn_gl_loc_length = hd->phd_node->pn_gl_loc_length;
						hd->phd_pn_gl_loc = (int*)realloc(hd->phd_pn_gl_loc, hd->phd_pn_gl_loc_length*sizeof(int));
						for (i=0; i < hd->phd_pn_gl_loc_length; i++) {
							if(PH_DEBUG) fprintf(stderr,"	loc old:%d new:%d\n", hd->phd_pn_gl_loc[i], hd->phd_node->pn_gl_loc[i]);
							hd->phd_pn_gl_loc[i] = hd->phd_node->pn_gl_loc[i];
						}
					}
				}
				hd = hd->phd_next;
			}
			hl = hl->gphl_next;
		}
	}
}

int glob_preset_hub_list_add(t_preset_hub *x)
{
	if(PH_DEBUG) fprintf(stderr,"glob_preset_hub_list_add\n");
	t_glob_preset_hub_list *h1, *h2;

	if (!gphl) {
		gphl = (t_glob_preset_hub_list *)t_getbytes(sizeof(*gphl));
		h2 = gphl;
	} else {
		h1 = gphl;
		while(h1->gphl_next)
			h1 = h1->gphl_next;
		h2 = (t_glob_preset_hub_list *)t_getbytes(sizeof(*h2));
		h1->gphl_next = h2;
	}
	h2->gphl_hub = x;
	return(0);
}

int glob_preset_hub_list_delete(t_preset_hub *x)
{
	if(PH_DEBUG) fprintf(stderr,"glob_preset_hub_list_delete\n");
	t_glob_preset_hub_list *h1, *h2;
	int found;

	if (!gphl)
		return(1);

	h1 = gphl;
	h2 = gphl;

	// first check if we are the first gphl
	if (h2->gphl_hub == x) {
		gphl = h2->gphl_next;
		freebytes(h2, sizeof(*h2));
	} 
	else {
		while (h2->gphl_hub != x && h2->gphl_next) {
			h1 = h2;
			h2 = h2->gphl_next;
		}
		if (h2->gphl_hub == x) {
			h1->gphl_next = h2->gphl_next;
			freebytes(h2, sizeof(*h2));
		}
		else {
			// we should never get here
			if(PH_DEBUG) fprintf(stderr,"error, could not find appropriate gphl to delete\n");
			return(1);
		}
	}	
	
	return(0);
}

//====================== end global vars ===========================//

//======================== preset_node =============================//

// we declare this class as part of the g_canvas.h (to expose it to the rest of the code)

static void preset_node_update_my_glist_location(t_preset_node *x)
{
	if(PH_DEBUG) fprintf(stderr,"node_update_glist_location\n");
	// location is calculated in respect to the hub (if any), otherwise it is 0
	t_preset_hub *h;
	t_canvas *c = x->pn_canvas;
	int found = 0;

	// do all this only if we are already paired with a hub
	if (x->pn_hub) {

		if (x->pn_old_gl_loc && x->pn_old_gl_loc != x->pn_gl_loc) free(x->pn_old_gl_loc);
		x->pn_old_gl_loc = x->pn_gl_loc;
		x->pn_old_gl_loc_length = x->pn_gl_loc_length;
	
		x->pn_gl_loc = NULL;
		x->pn_gl_loc_length = 0;

		// let's try to find our hub (if any)
		int depth = 0;
		while (!found && c) {
			if (c->gl_phub) {
				if(PH_DEBUG) fprintf(stderr,"	gl_phub != NULL\n");
				h = c->gl_phub;
				while (h) {
					if(PH_DEBUG) fprintf(stderr,"	analyzing hub\n");
					if (!strcmp(h->ph_name->s_name, x->pn_hub_name->s_name)) {
						if(PH_DEBUG) fprintf(stderr,"	found a match\n");
						found = 1;
						break;
					}
					h = h->ph_next;
				}
			}
			c = c->gl_owner;
			if (c)
				depth++;
		}

		if (found) {

			if(PH_DEBUG) fprintf(stderr,"	depth = %d\n", depth);
	
			// allocate depth array
			x->pn_gl_loc = (int*)calloc(depth+1, sizeof(x->pn_gl_loc));

			// now let's figure out where each of the child gobj's location is
			// starting from the back
			int i = 0; 					// iteration within each gl_list
			int j = 0; 					// number of gl_list to traverse
			c = x->pn_canvas;
			t_gobj *g = c->gl_list;
			t_gobj *target = (t_gobj *)x;

			for (j = depth; j >= 0; j--) {

				while (g && g != target) // have to do sanity check for g in the event this is run at patch creation time
				{
					g = g->g_next;
					i++;
					//if(PH_DEBUG) fprintf(stderr,"	searching... %d\n", i);
				}
				// even if the g fails sanity check due to creation time, it will still land on the last created element whose
				// pointer at this point is still null since this means this is being called at the end of the preset_node_new call
				if(PH_DEBUG) fprintf(stderr,"	location = %d %lx %lx\n", i, (t_int)g, (t_int)target);
				x->pn_gl_loc[j] = i;

				// now readjust the target, c, g, and i variables for the next level up
				if (c->gl_owner) {
					target = (t_gobj *)c;
					c = c->gl_owner;
					g = c->gl_list;
					i = 0;
				}
			}
			x->pn_gl_loc_length = depth+1;

			if(PH_DEBUG) fprintf(stderr,"	final structure:\n");
			for (j = 0; j < x->pn_gl_loc_length; j++) {
				if(PH_DEBUG) fprintf(stderr,"	%d: %d\n", j, x->pn_gl_loc[j]);
			}
		}
		else {
			if(PH_DEBUG) fprintf(stderr,"	preset_node: no matching hub %s found\n", x->pn_hub_name->s_name);
		}

		// finally if this is the first time we are creating the object, old_location should be the same as the current location
		if (x->pn_old_gl_loc_length == 0) {
			x->pn_old_gl_loc = x->pn_gl_loc;
			x->pn_old_gl_loc_length = x->pn_gl_loc_length;
		}
	}
}

// this is called during the canvas creation (e.g. at load time) as part of pre-loadbang event,
// during object creation (in case it is manually created), as well as when hubs try to query
// nodes that have not been paired yet. 
void preset_node_seek_hub(t_preset_node *x)
{
	if(PH_DEBUG) fprintf(stderr,"preset_node_seek_hub %lx\n", (t_int)x->pn_hub);
	if (we_are_undoing)
		return;
	t_canvas *y = x->pn_canvas;
	t_preset_hub *h;

	if (!x->pn_hub) {
		if(PH_DEBUG) fprintf(stderr,"	have to seek\n");
		while (!x->pn_hub && y) {
			h = y->gl_phub;
			while (h) {
				if (h->ph_name->s_name == x->pn_hub_name->s_name) {
					x->pn_hub = h;
					if(PH_DEBUG) fprintf(stderr,"	node found hub\n");
					// update our location in respect to the newfound hub
					preset_node_update_my_glist_location(x);
					// add a node on the hub's list of nodes and copy location to its struct
					preset_hub_add_a_node(x->pn_hub, x);
					// reflect that this node is paired in the global list of all existing nodes
					glob_preset_node_list_update_paired(x, 1);		
					break;
				}
				h = h->ph_next;			
			}
			y = y->gl_owner;
		}
	}
}

static int preset_node_location_changed(t_preset_node *x)
{
	int i;
	if (x->pn_old_gl_loc_length != x->pn_gl_loc_length) {
		return(1);
	}
	for (i = 0; i < x->pn_gl_loc_length; i++) {
		if (x->pn_gl_loc[i] != x->pn_old_gl_loc[i])
			return(1);
	}
	return(0);
}

static void preset_node_anything(t_preset_node *x, t_symbol *s, int argc, t_atom *argv)
{
	if (PH_DEBUG) fprintf(stderr,"preset_node_anything %lx\n", (t_int)x);
	int i;
	alist_list(&x->pn_val, 0, argc, argv);
	if(PH_DEBUG) {
		if (x->pn_val.l_vec->l_a.a_type == A_SYMBOL)
			fprintf(stderr,"	%lx data is %s\n", (t_int)x, x->pn_val.l_vec->l_a.a_w.w_symbol->s_name);
		else if (x->pn_val.l_vec->l_a.a_type == A_FLOAT)
			fprintf(stderr,"	%lx data is %f\n", (t_int)x, x->pn_val.l_vec->l_a.a_w.w_float);
	}
	// check for pointers and warn user presetting them has not been tested
    for (i = 0; i < x->pn_val.l_n; i++)
    {
        if (x->pn_val.l_vec[i].l_a.a_type == A_POINTER)
        {
			pd_error(x, "preset_node preset received a pointer as part of a list--this has not been tested, use at your own risk and please report any successes/failures. thank you!");
			break;
        }
    }
	
}

	//=============== following functions are for interaction with the hub/pd =================//

int preset_node_check_location(t_preset_node *x)
{
	int result;
	if (x->pn_hub) {
		preset_node_update_my_glist_location(x);
		return(preset_node_location_changed(x));
	}
	return(0);
}

void preset_node_request_hub_recall(t_preset_node *x, t_float f)
{
	if (x->pn_hub)
		preset_hub_recall(x->pn_hub, f);
}

void preset_node_request_hub_store(t_preset_node *x, t_float f)
{
	if (x->pn_hub)
		preset_hub_store(x->pn_hub, f);
}

void preset_node_set_and_output_value(t_preset_node *x, t_alist val)
{
	if(PH_DEBUG) fprintf(stderr,"preset_node_set_and_output_value %lx\n", (t_int)x);
	t_atom *outv;
	if (val.l_n > 0) {
		alist_clear(&x->pn_val);
		alist_clone(&val, &x->pn_val);
		XL_ATOMS_ALLOCA(outv, x->pn_val.l_n);
		alist_toatoms(&x->pn_val, outv);
		outlet_list(x->pn_outlet, &s_list, x->pn_val.l_n, outv);
		if(PH_DEBUG) {
			if (outv->a_type == A_SYMBOL)
				fprintf(stderr,"	%lx outputs %s\n", (t_int)x, outv->a_w.w_symbol->s_name);
			else if (outv->a_type == A_FLOAT)
				fprintf(stderr,"	%lx outputs %f\n", (t_int)x, outv->a_w.w_float);
		}
		XL_ATOMS_FREEA(outv, x->pn_val.l_n);
	}
}

void preset_node_clear(t_preset_node *x, t_float f)
{
	t_atom ap[2];
	t_preset_hub_data *hd2;
	t_node_preset *np1, *np2;
	int changed = 0;

	if(PH_DEBUG) fprintf(stderr,"preset_node_clear %d\n", (int)f);

	if (x->pn_hub) {
		hd2 = x->pn_hub->ph_data;

		// only remove this object's preset
		if (hd2) {
			if(PH_DEBUG) fprintf(stderr,"	got ph_data\n");
			while (hd2 && hd2->phd_node != x) {
				hd2 = hd2->phd_next;
			}
			if (hd2) {
				np1 = hd2->phd_npreset;
				// if it is first one
				if (np1->np_preset == (int)f) {
					hd2->phd_npreset = np1->np_next;
					if (np1->np_val.l_n)
						alist_clear(&np1->np_val);
					freebytes(np1, sizeof(*np1));
					changed = 1;
					if(PH_DEBUG) fprintf(stderr,"	found preset to delete (first)\n");
				} else {
					while (np1) {
						np2 = np1->np_next;
						if (np2 && np2->np_preset == (int)f) {
							np1->np_next = np2->np_next;
							if (np2->np_val.l_n)
								alist_clear(&np2->np_val);
							freebytes(np2, sizeof(*np2));
							changed = 1;
							if(PH_DEBUG) fprintf(stderr,"	found preset to delete\n");
							break;
						}
						np1 = np1->np_next;
					}
				}
			}
		}
	}
	if (changed) canvas_dirty(x->pn_hub->ph_canvas, 1);

	SETFLOAT(ap+0, f);
	SETFLOAT(ap+1, (t_float)changed);
	outlet_anything(x->pn_outlet, gensym("node_clear"), 2, ap);
}


void preset_node_clearall(t_preset_node *x, t_float f) {
	if (x->pn_hub)
		preset_hub_clear(x->pn_hub, f);
}

void preset_node_reset(t_preset_node *x) {
	if (x->pn_hub)
		preset_hub_reset(x->pn_hub);
}

void preset_node_purge(t_preset_node *x) {
	if (x->pn_hub)
		preset_hub_purge(x->pn_hub);
}

	//==================== end functions are for interaction with the hub =====================//

static void *preset_node_new(t_symbol *s, int argc, t_atom *argv)
{
	if(PH_DEBUG) fprintf(stderr,"===preset_node_new===\n");
	t_glist *glist=(t_glist *)canvas_getcurrent();
	t_canvas *canvas = (t_canvas *)glist_getcanvas(glist);

	// first check if the mandatory argument is sane, otherwise bail out
    if (!(argc > 0 && argv[0].a_type == A_SYMBOL)) {
		pd_error(canvas, "preset_node requires alphanumeric name as its argument");
		return(NULL);
	}

    t_preset_node *x = (t_preset_node *)pd_new(preset_node_class);

	// read basic creation arguments
	x->pn_hub_name = (t_symbol *)atom_getsymbol(&argv[0]);

	x->pn_canvas = canvas;
	t_canvas *y = x->pn_canvas;
	//t_preset_hub *h;
	alist_init(&x->pn_val);

	x->pn_hub = NULL;
	x->pn_gl_loc_length = 0;
	x->pn_gl_loc = NULL;
	x->pn_old_gl_loc_length = 0;  
	x->pn_old_gl_loc = NULL;
 	x->pn_outlet = outlet_new(&x->pn_obj, 0);

	glob_preset_node_list_add(x);

	// now we seek our potential hub (if it exists), this is also called as part of 
	// pre-loadbang if the patch is being loaded (this one is for manually created objects)
	// do this only if we are not undoing, otherwise, we'll have the undo do it for us
	// once it has repositioned objects to their original locations
	// (the undo check is done inside the preset_node_seek_hub)
	preset_node_seek_hub(x);

    return(x);
}

static void preset_node_free(t_preset_node* x)
{
	// deactivate a node on the hub's list of nodes
	if (x->pn_hub)
		preset_hub_delete_a_node(x->pn_hub, x);
	glob_preset_node_list_delete(x);

	alist_clear(&x->pn_val);

	// the two arrays can point to same locations so here we prevent double free
	// (this is only possible initially when the object is first instantiated)
	if (x->pn_gl_loc != x->pn_old_gl_loc) {
		free(x->pn_gl_loc);
		free(x->pn_old_gl_loc);
	} else {
		free(x->pn_gl_loc);
	}	
}

void preset_node_setup(void)
{
	preset_node_class = class_new(gensym("preset_node"), 
	    (t_newmethod)preset_node_new, (t_method)preset_node_free, 
	    sizeof(t_preset_node), 0, A_GIMME, 0);

	// have to call this after everything has loaded, otherwise, the hub may not yet exist
	// we do this using pre-loadbang call that is issued before other loadbangs, otherwise
	// out-of-order loadbangs can issue preset recall before the preset has been properly
	// configured
	class_addmethod(preset_node_class, (t_method)preset_node_seek_hub,
        gensym("pre-loadbang"), 0);

    class_addmethod(preset_node_class, (t_method)preset_node_request_hub_recall,
        gensym("recall"), A_DEFFLOAT, 0);
    class_addmethod(preset_node_class, (t_method)preset_node_request_hub_store,
        gensym("store"), A_DEFFLOAT, 0);

    class_addmethod(preset_node_class, (t_method)preset_node_clear,
        gensym("clear"), A_DEFFLOAT, 0);
    class_addmethod(preset_node_class, (t_method)preset_node_clearall,
        gensym("clearall"), A_DEFFLOAT, 0);
    class_addmethod(preset_node_class, (t_method)preset_node_reset,
        gensym("reset"), A_NULL, 0);
    class_addmethod(preset_node_class, (t_method)preset_node_purge,
        gensym("purge"), A_NULL, 0);

	// we use anything to cover virtually all presetable types of data
	class_addanything(preset_node_class, preset_node_anything);
}

//====================== end preset_node ===========================//

//======================== preset_hub ==============================//

// we declare this class as part of the g_canvas.h (to expose it to the rest of the code)

typedef enum
{
	H_NONE,
    H_NODE,
	H_LOCATION,
    H_PRESET,
    H_PRESET_DATA
}  t_hub_parser;

/*	syntax for saving a preset hub (all in a single line, here it is
	separated for legibility sakes):
	#X obj X Y preset_hub NAME %hidden%
	%node% LOCATION_ARRAY_LENGTH LOCATION_ARRAY_(INT) 1 2 3 etc.
	%preset% 1 data
	%preset% 2 4
	etc
	%node% 2 0
	%preset% 1 5.561
	%preset% 3 7.00001 anything including lists
	%preset% 5 some_text
	;

	NB: %hidden% is used to hide optional arguments following that argument
		it can be used by any other object as well
*/

void preset_hub_save(t_gobj *z, t_binbuf *b)
{
	if(PH_DEBUG) fprintf(stderr,"preset_hub_save\n");
	t_atom *outv;
	int i;
	t_preset_hub_data *phd;
	t_node_preset *np;

	t_preset_hub *x = (t_preset_hub *)z;

	binbuf_addv(b, "ssiiss", gensym("#X"), gensym("obj"), (int)x->ph_obj.te_xpix,
				(int)x->ph_obj.te_ypix, gensym("preset_hub"), x->ph_name);

	if (x->ph_invis)
		binbuf_addv(b, "i", (int)x->ph_invis);

	binbuf_addv(b, "s", gensym("%hidden%"));

	phd = x->ph_data;
	while (phd) {
		if(PH_DEBUG) fprintf(stderr,"	saving phd\n");
		// designate a node and state whether it is active or disabled
		// (disabled nodes are ones that have presets saved but have been deleted since--
		// we keep these in the case of undo actions during the session that may go beyond
		// saving something into a file)
		binbuf_addv(b, "si", gensym("%node%"), phd->phd_pn_gl_loc_length);

		// gather info about the length of the node's location and store it
		for (i = 0; i < phd->phd_pn_gl_loc_length; i++) {
			binbuf_addv(b,"i", (int)phd->phd_pn_gl_loc[i]);
		}

		// save preset data
		np = phd->phd_npreset;
		while (np) {
			if (np->np_val.l_n > 0) {
				binbuf_addv(b, "si", gensym("%preset%"), (int)np->np_preset);
				for (i = 0; i < np->np_val.l_n; i++) {
					if (np->np_val.l_vec[i].l_a.a_type == A_FLOAT)
						binbuf_addv(b, "f", np->np_val.l_vec[i].l_a.a_w.w_float);
					else if (np->np_val.l_vec[i].l_a.a_type == A_SYMBOL)
						binbuf_addv(b, "s", np->np_val.l_vec[i].l_a.a_w.w_symbol);	
				}
			}
			np = np->np_next;
		}

		phd = phd->phd_next;
	}
	if(PH_DEBUG) fprintf(stderr,"	done\n");
	binbuf_addv(b, ";");
}

void preset_hub_bang(t_preset_hub *x)
{
	t_atom ap[1];
	SETFLOAT(ap+0, (t_float)x->ph_preset);
    outlet_anything(x->ph_outlet, gensym("current"), 1, ap);
}

void preset_hub_recall(t_preset_hub *x, t_float f)
{
	if(PH_DEBUG) fprintf(stderr,"hub_recall\n");
	t_atom ap[2];
	t_preset_hub_data *hd;
	t_node_preset *np;
	t_float valid = 0;

	if (f>=0) {
		// check if preset exists and apply it
		if (x->ph_data) {
			hd = x->ph_data;
			while (hd) {
				if(PH_DEBUG) fprintf(stderr,"   searching\n");
				// now check if the object is active (node pointer is not NULL)
				if (hd->phd_node) {
					if(PH_DEBUG) fprintf(stderr,"	object active\n");
					if (hd->phd_npreset) {
						if(PH_DEBUG) fprintf(stderr,"	object has presets\n");
						np = hd->phd_npreset;
						while (np) {
							if(PH_DEBUG) fprintf(stderr,"	searching presets\n");
							if (np->np_preset == (int)f) {
								valid = 1;
								if(PH_DEBUG) fprintf(stderr,"	valid %d\n", (hd->phd_node ? 1:0));
								if (np->np_val.l_n > 0)
									preset_node_set_and_output_value(hd->phd_node, np->np_val);
								break;
							}
							np = np->np_next;
						}
					}
				}
				hd = hd->phd_next;
			}
		}
		if (valid)
			x->ph_preset = f;
		if(PH_DEBUG) fprintf(stderr,"	done\n");

		SETFLOAT(ap+0, f);
		SETFLOAT(ap+1, (t_float)valid);
		outlet_anything(x->ph_outlet, gensym("recall"), 2, ap);
	}
}

void preset_hub_store(t_preset_hub *h, t_float f)
{
	if(PH_DEBUG) fprintf(stderr,"preset_hub_store\n");
	t_atom ap[1];
	t_preset_hub_data *hd1;
	t_node_preset *np1, *np2;
	int overwrite;
	t_atom val;
	int changed = 0;

	np1 = NULL;
	np2 = NULL;

	int dspstate = canvas_suspend_dsp();

	if (f>=0) {
		//check if there are any existing nodes
		if (h->ph_data) {
			hd1 = h->ph_data;
			while (hd1) {
				if(PH_DEBUG) fprintf(stderr,"	analyzing phd\n");
				if (hd1->phd_node) {
					if(PH_DEBUG) fprintf(stderr,"	node is active\n");
					// only if the node is active (not NULL/disabled)

					// reset np1 and np2 from the previous run
					// (if we have more phd_nodes, stale data will append
					// new presets to the last phd_node eventually resulting in crash)
					np1 = NULL;
					np2 = NULL;

					overwrite = 0;
					if (hd1->phd_npreset) {
						// if this node has already pre-existing presets
						// first check if there is already one we need to overwrite
						np2 = hd1->phd_npreset;
						while (np2) {
							if (np2->np_preset == (int)f) {
								if(PH_DEBUG) fprintf(stderr,"	overwriting\n");
								overwrite = 1;
								break;
							}
							np1 = np2;
							np2 = np2->np_next;
						}
					}

					if (!overwrite && hd1->phd_node->pn_val.l_n > 0) {
						// we need to create a new preset (this is also true if hd1->phd_npreset is NULL)
						changed = 1;
						if(PH_DEBUG) fprintf(stderr,"	creating new preset\n");
						np2 = (t_node_preset *)t_getbytes(sizeof(*np2));
						if (np1)
							np1->np_next = np2;
						else hd1->phd_npreset = np2;
						np2->np_preset = (int)f;
						np2->np_next = NULL;
					}

					if (hd1->phd_node->pn_val.l_n > 0) {
						changed = 1;
						if(PH_DEBUG) {
							fprintf(stderr,"	node data len = %d, old hub data len = %d\n", hd1->phd_node->pn_val.l_n, np2->np_val.l_n);
							if (hd1->phd_node->pn_val.l_vec->l_a.a_type == A_SYMBOL)
								fprintf(stderr,"	%lx outputs %s\n", (t_int)hd1->phd_node, hd1->phd_node->pn_val.l_vec->l_a.a_w.w_symbol->s_name);
							else if (hd1->phd_node->pn_val.l_vec->l_a.a_type == A_FLOAT)
								fprintf(stderr,"	%lx outputs %f\n", (t_int)hd1->phd_node, hd1->phd_node->pn_val.l_vec->l_a.a_w.w_float);
						}
						alist_clear(&np2->np_val);
						alist_clone(&hd1->phd_node->pn_val, &np2->np_val);
						if(PH_DEBUG) {
							fprintf(stderr,"	node data len = %d, NEW hub data len = %d\n", hd1->phd_node->pn_val.l_n, np2->np_val.l_n);
							if (hd1->phd_node->pn_val.l_vec->l_a.a_type == A_SYMBOL)
								fprintf(stderr,"	%lx outputs %s\n", (t_int)hd1->phd_node, np2->np_val.l_vec->l_a.a_w.w_symbol->s_name);
							else if (hd1->phd_node->pn_val.l_vec->l_a.a_type == A_FLOAT)
								fprintf(stderr,"	%lx outputs %f\n", (t_int)hd1->phd_node, np2->np_val.l_vec->l_a.a_w.w_float);
						}
						// finally if this is the first preset, hd1->phd_npreset will be NULL so,
						// let's have it point to the newly created n2
						if (!hd1->phd_npreset)
							hd1->phd_npreset = np2;
					}
				}
				hd1 = hd1->phd_next;
			}
		}
		canvas_resume_dsp(dspstate);

		if (changed) canvas_dirty(h->ph_canvas, 1);

		SETFLOAT(ap+0, f);
		outlet_anything(h->ph_outlet, gensym("store"), 1, ap);
	}
}

int preset_hub_compare_loc(int *h_loc, int h_loc_length, int *n_loc, int n_loc_length)
{
	int i;
	if (h_loc_length != n_loc_length)
		return(1);
	for (i = 0; i < h_loc_length; i++) {
		if (h_loc[i] != n_loc[i])
			return(1);
	}
	return(0);
}

void preset_hub_add_a_node(t_preset_hub *h, t_preset_node *x)
{
	t_preset_hub_data *hd1, *hd2;
	int found = 0;
	int i;

	hd1 = NULL;
	hd2 = NULL;

	if(PH_DEBUG) fprintf(stderr,"preset_hub_add_a_node\n");
	if (h->ph_data) {
		// first check for disabled nodes and reenable them if they match location
		hd1 = h->ph_data;
		while (hd1) {
			if (!hd1->phd_node) {
				// only if the node is disabled (NULL)
				if (!preset_hub_compare_loc(hd1->phd_pn_gl_loc, hd1->phd_pn_gl_loc_length, x->pn_gl_loc, x->pn_gl_loc_length))
				{
					// if this hub node data's location matches that of the node
					if(PH_DEBUG) fprintf(stderr,"	found disabled -> enabling\n");
					found = 1;
					hd1->phd_node = x;
					break;
				}
			}
			hd1 = hd1->phd_next;
		}
	}

	if (!found) {
		// we have no stored node data (or none that match node's location) so let's create a new one
		if(PH_DEBUG) fprintf(stderr,"	creating a new\n");
		// create a new data struct
		hd2 = (t_preset_hub_data *)t_getbytes(sizeof(*hd2));
		// reconstruct the dynamic location array
		hd2->phd_pn_gl_loc_length = x->pn_gl_loc_length;
		hd2->phd_pn_gl_loc = (int*)calloc(hd2->phd_pn_gl_loc_length, sizeof(hd2->phd_pn_gl_loc));
		for (i=0; i < hd2->phd_pn_gl_loc_length; i++) {
			if(PH_DEBUG) fprintf(stderr,"	loc %d\n", x->pn_gl_loc[i]);
			hd2->phd_pn_gl_loc[i] = x->pn_gl_loc[i];
		}
		// assign node value
		hd2->phd_node = x;
		hd2->phd_next = NULL;

		// adjust pointers
		if (!h->ph_data) {
			h->ph_data = hd2;
		} else {
			hd1 = h->ph_data;
			while (hd1->phd_next) {
				hd1 = hd1->phd_next;	
			}
			hd1->phd_next = hd2;
		}
	}
}

void preset_hub_delete_a_node(t_preset_hub *h, t_preset_node *x)
{
	t_preset_hub_data *hd1;
	hd1 = NULL;

	if(PH_DEBUG) fprintf(stderr,"preset_hub_delete_a_node\n");
	if (h->ph_data) {
		// check for enabled nodes only
		hd1 = h->ph_data;
		while (hd1) {
			if (hd1->phd_node && hd1->phd_node == x) {
				// only if the node is enabled and matches ours
				if(PH_DEBUG) fprintf(stderr,"	found enabled -> disabling\n");
				hd1->phd_node = NULL;
				break;
			}
			hd1 = hd1->phd_next;
		}
	}
}

void preset_hub_reset(t_preset_hub *h)
{
	t_atom ap[1];
	t_glob_preset_node_list *nl;
	t_preset_hub_data *hd1, *hd2;
	t_node_preset *np1, *np2;
	t_preset_hub *h1, *h2;
	int changed = 0;

	if(PH_DEBUG) fprintf(stderr,"preset_hub_reset\n");

	// inform all nodes that the hub is letting go of them
	if (gpnl) {
		if(PH_DEBUG) fprintf(stderr,"	we got gpnl\n");
		nl = gpnl;
		while(nl) {
			if(PH_DEBUG) fprintf(stderr,"	analyzing gpnl entry %d\n", nl->gpnl_paired);
			if (nl->gpnl_paired && nl->gpnl_node->pn_hub == h) {
				nl->gpnl_paired = 0;
				nl->gpnl_node->pn_hub = NULL;
				if(PH_DEBUG) fprintf(stderr,"	removed gpnl reference\n");
			}
			nl = nl->gpnl_next;
		}
	}

	// deallocate all the dynamically-allocated memory
	if (h->ph_data) {
		if(PH_DEBUG) fprintf(stderr,"	got ph_data\n");
		hd1 = h->ph_data;
		while (hd1) {
			if (hd1->phd_npreset) {
				np1 = hd1->phd_npreset;
				while (np1) {
					np2 = np1->np_next;
					if (np1->np_val.l_n)
						alist_clear(&np1->np_val);
					freebytes(np1, sizeof(*np1));
					changed = 1;
					if(PH_DEBUG) fprintf(stderr,"	deleting preset\n");
					np1 = np2;
				}
			}
			hd2 = hd1->phd_next;
			freebytes(hd1, sizeof(*hd1));
			changed = 1;
			if(PH_DEBUG) fprintf(stderr,"	deleting ph_data\n");
			hd1 = hd2;
		}
	}

	h->ph_data = NULL;

	// and finally request pairing with nodes (since we deleted all our references)
	glob_preset_node_list_seek_hub();

	if (changed) canvas_dirty(h->ph_canvas, 1);

	SETFLOAT(ap+0, (t_float)changed);
	outlet_anything(h->ph_outlet, gensym("reset"), 1, ap);
}

void preset_hub_clear(t_preset_hub *h, t_float f)
{
	t_atom ap[1];
	t_preset_hub_data *hd2;
	t_node_preset *np1, *np2;
	int changed = 0;

	hd2 = h->ph_data;

	if(PH_DEBUG) fprintf(stderr,"preset_hub_clear\n");

	// deallocate all the dynamically-allocated memory for disabled nodes
	if (hd2) {
		if(PH_DEBUG) fprintf(stderr,"	got ph_data\n");
		hd2 = h->ph_data;
		while (hd2) {
			// all nodes will get their preset cleared regardless whether they are active or disabled
			if (hd2->phd_npreset) {
				np1 = hd2->phd_npreset;
				// if it is first one
				if (np1->np_preset == (int)f) {
					hd2->phd_npreset = np1->np_next;
					if (np1->np_val.l_n)
						alist_clear(&np1->np_val);
					freebytes(np1, sizeof(*np1));
					changed = 1;
					if(PH_DEBUG) fprintf(stderr,"	found preset to delete (first)\n");
				} else {
					while (np1) {
						np2 = np1->np_next;
						if (np2 && np2->np_preset == (int)f) {
							np1->np_next = np2->np_next;
							if (np1->np_val.l_n)
								alist_clear(&np1->np_val);
							freebytes(np2, sizeof(*np2));
							changed = 1;
							if(PH_DEBUG) fprintf(stderr,"	found preset to delete\n");
							break;
						}
						np1 = np1->np_next;
					}
				}
			}
			hd2 = hd2->phd_next;
		}
	}
	if (changed) canvas_dirty(h->ph_canvas, 1);

	SETFLOAT(ap+0, (t_float)changed);
	outlet_anything(h->ph_outlet, gensym("clear"), 1, ap);
}


void preset_hub_purge(t_preset_hub *h)
{
	t_atom ap[1];
	t_preset_hub_data *hd1, *hd2;
	t_node_preset *np1, *np2;
	int changed = 0;

	hd1 = NULL;
	hd2 = NULL;

	if(PH_DEBUG) fprintf(stderr,"preset_hub_purge\n");

	// deallocate all the dynamically-allocated memory for disabled nodes
	if (h->ph_data) {
		if(PH_DEBUG) fprintf(stderr,"	got ph_data\n");
		hd2 = h->ph_data;
		while (hd2) {
			if (!hd2->phd_node) {
				if (hd2->phd_npreset) {
					np1 = hd2->phd_npreset;
					while (np1) {
						np2 = np1->np_next;
						if (np1->np_val.l_n)
							alist_clear(&np1->np_val);
						freebytes(np1, sizeof(*np1));
						changed = 1;
						if(PH_DEBUG) fprintf(stderr,"	deleting preset\n");
						np1 = np2;
					}
				}
				if (hd1 && hd1 != hd2) {
					hd1->phd_next = hd2->phd_next;
				}
				hd1 = hd2;
				if (hd2 == h->ph_data)
					h->ph_data = hd2->phd_next;
				hd2 = hd2->phd_next;
				freebytes(hd1, sizeof(*hd1));
				changed = 1;
				if(PH_DEBUG) fprintf(stderr,"	deleting ph_data\n");
			} else {
				hd1 = hd2;
				hd2 = hd2->phd_next;
			}
		}
	}
	if (changed) canvas_dirty(h->ph_canvas, 1);

	SETFLOAT(ap+0, (t_float)changed);
	outlet_anything(h->ph_outlet, gensym("purge"), 1, ap);
}

static void *preset_hub_new(t_symbol *s, int argc, t_atom *argv)
{
	if(PH_DEBUG) fprintf(stderr,"===preset_hub_new===\n");
	t_glob_preset_node_list *nl;
	t_glob_preset_hub_list *hl;
	t_preset_hub_data *hd1, *hd2;
	t_node_preset *np1, *np2;
	t_hub_parser h_cur = H_NONE;

	t_preset_hub *x, *check;
	t_symbol *name;
	int i, pos, loc_pos;
	int j, data_count; //for lists
	t_glist *glist=(t_glist *)canvas_getcurrent();
	t_canvas *canvas = (t_canvas *)glist_getcanvas(glist);

	loc_pos = 0;
	pos = 0; // position within argc

	// first check if the mandatory argument is sane, otherwise bail out
	if (!(argc > 0 && argv[0].a_type == A_SYMBOL)) {
		pd_error(canvas, "preset_hub requires alphanumeric name as its argument");
		return(NULL);
	}

	// now check if there is already another hub on the same canvas with the same name and fail if so
	check = canvas->gl_phub;
	if (check) {
		while (check) {
			if (!strcmp(atom_getsymbol(&argv[0])->s_name, check->ph_name->s_name)) {
				pd_error(canvas, "preset_hub with the name %s already exists on this canvas", check->ph_name->s_name);
				return(NULL);
			}
			check = check->ph_next;
		}
	}

	x = (t_preset_hub *)pd_new(preset_hub_class);
	x->ph_invis = 0;

	// read basic creation arguments
	x->ph_name = (t_symbol *)atom_getsymbol(&argv[0]);
	pos++;
	if (argc > 1 && argv[1].a_type == A_FLOAT) {
		x->ph_invis = (int)atom_getfloat(&argv[1]);
		if (x->ph_invis < 0) x->ph_invis = 0;
		/*level = x->ph_level;
		while (level > 0 && canvas->gl_owner) {
			if (canvas->gl_owner) canvas = canvas->gl_owner;
			level--;
		}*/
		pos++;
	}
	if(PH_DEBUG) fprintf(stderr,"hub name %s invis %d\n", x->ph_name->s_name, (int)x->ph_invis);

	pos++; // one more time to move ahead of the %hidden% tag

	// assign default x and y position
	x->ph_obj.te_xpix = 0;
	x->ph_obj.te_ypix = 0;

	x->ph_preset = -1;

	x->ph_canvas = canvas;

	// add us to the global hub list
	glob_preset_hub_list_add(x);

	// add hub to the canvas pointer
	if (!canvas->gl_phub)
		canvas->gl_phub = x;
	else {
		t_preset_hub *gl_phub = canvas->gl_phub;
		while (gl_phub->ph_next)
			gl_phub = gl_phub->ph_next;
		gl_phub->ph_next = x;
	}
	x->ph_next = NULL;
	x->ph_data = NULL;

	hd1 = NULL;
	hd2 = NULL;
	np1 = NULL;
	np2 = NULL;

	// load the data from the buffer and build the preset db
	if (pos < argc) {
		for (i=pos; i < argc; i++) {
			if(PH_DEBUG) fprintf(stderr,"	position: %d\n", i);
			// SYMBOL ANALYSIS
			if (argv[i].a_type == A_SYMBOL) {
				if(PH_DEBUG) fprintf(stderr,"	data = %s\n", atom_getsymbol(&argv[i])->s_name);
				if (!strcmp(atom_getsymbol(&argv[i])->s_name, "%node%")) {
					// beginning of a new node
					if(PH_DEBUG) fprintf(stderr,"	new node\n");
					hd2 = (t_preset_hub_data *)t_getbytes(sizeof(*hd2));
					hd2->phd_pn_gl_loc_length = 0;
					if (hd1) {
						hd1->phd_next = hd2;
						if(PH_DEBUG) fprintf(stderr,"	not first node\n");
					}
					else {
						x->ph_data = hd2;
						if(PH_DEBUG) fprintf(stderr,"	first node\n");
					}
					hd1 = hd2;
					np1 = NULL; // have to reset it so that new presets are not erroneously appended to previous node
					h_cur = H_NODE;
				}
				else if (!strcmp(atom_getsymbol(&argv[i])->s_name, "%preset%")) {
					// beginning of a new preset
					if(PH_DEBUG) fprintf(stderr,"	new preset\n");
					np2 = (t_node_preset *)t_getbytes(sizeof(*np2));
					if (np1) {
						np1->np_next = np2;
						if(PH_DEBUG) fprintf(stderr,"	not first preset\n");
					}
					else {
						hd2->phd_npreset = np2;
						if(PH_DEBUG) fprintf(stderr,"	first preset\n");
					}
					np1 = np2;
					h_cur = H_PRESET;
				}
			}
			// FLOAT ANALYSIS
			else if (argv[i].a_type == A_FLOAT) {
				if(PH_DEBUG) fprintf(stderr,"	data = %g\n", atom_getfloat(&argv[i]));
				if (h_cur == H_NODE) {
					// node location length
					hd2->phd_pn_gl_loc_length = (int)atom_getfloat(&argv[i]);
					// reconstruct the dynamic location array
					if (!hd2->phd_pn_gl_loc)
						hd2->phd_pn_gl_loc = (int*)calloc(hd2->phd_pn_gl_loc_length, sizeof(hd2->phd_pn_gl_loc));
					hd2->phd_pn_gl_loc[hd2->phd_pn_gl_loc_length-1] = (int)atom_getfloat(&argv[i]);
					if(PH_DEBUG) fprintf(stderr,"	loc length = %d\n", hd2->phd_pn_gl_loc_length);
					loc_pos = 0;
					h_cur = H_LOCATION;
				}
				else if (h_cur == H_LOCATION) {
					// node location data
					hd2->phd_pn_gl_loc[loc_pos] = (int)atom_getfloat(&argv[i]);
					if(PH_DEBUG) fprintf(stderr,"	loc = %d\n", hd2->phd_pn_gl_loc[loc_pos]);
					loc_pos++;
				}
				else if (h_cur == H_PRESET) {
					// preset number
					if(PH_DEBUG) fprintf(stderr,"	preset %g\n", atom_getfloat(&argv[i]));
					np2->np_preset = (int)atom_getfloat(&argv[i]);
					data_count = i+1;
					// figure out how long of variable data list follows the preset descriptor
					while (data_count < argc && strcmp(atom_getsymbol(&argv[data_count])->s_name, "%preset%") && strcmp(atom_getsymbol(&argv[data_count])->s_name, "%node%")) {
						data_count++;
					}
					if(PH_DEBUG) fprintf(stderr,"	found preset? %d found node? %d\n", strcmp(atom_getsymbol(&argv[data_count])->s_name, "%preset%"), strcmp(atom_getsymbol(&argv[data_count])->s_name, "%node%"));
					data_count = data_count - (i+1);
					if(PH_DEBUG) fprintf(stderr,"	data_count = %d staring @ %d out of %d\n", data_count, i+1, argc);
					alist_init(&np2->np_val);
					alist_list(&np2->np_val, 0, data_count, argv+(i+1));
					i = i + data_count;
					h_cur = H_PRESET_DATA;
				}
			}
		}
	}
	x->ph_outlet = outlet_new(&x->ph_obj, 0);

	// let us pair with our nodes (if any)
	// this is done node-side (node has to traverse canvases upwards to make sure
	// multiple abstractions with same hubs/nodes do not trip over each other)
	// do this only if we are not undoing, otherwise, we'll have the undo do it for us
	// once it has repositioned objects to their original locations
	// (the undo check is done inside the glob_preset_node_list_seek_hub)
	glob_preset_node_list_seek_hub();

    return(x);
}

static void preset_hub_free(t_preset_hub* x)
{
	if(PH_DEBUG) fprintf(stderr,"preset_hub_free\n");
	t_glob_preset_node_list *nl;
	t_preset_hub_data *hd1, *hd2;
	t_node_preset *np1, *np2;
	t_preset_hub *h1, *h2;

	// inform all nodes that the hub is going bye-bye
	if (gpnl) {
		if(PH_DEBUG) fprintf(stderr,"	we got gpnl\n");
		nl = gpnl;
		while(nl) {
			if(PH_DEBUG) fprintf(stderr,"	analyzing gpnl entry %d\n", nl->gpnl_paired);
			if (nl->gpnl_paired && nl->gpnl_node->pn_hub == x) {
				// we only make the hub pointer null and leave location for undo/redo/save purposes
				nl->gpnl_paired = 0;
				nl->gpnl_node->pn_hub = NULL;
				if(PH_DEBUG) fprintf(stderr,"	removed gpnl reference\n");
			}
			nl = nl->gpnl_next;
		}
	}
	glob_preset_hub_list_delete(x);

	// remove the hub from the canvas pointer list
	h2 = x->ph_canvas->gl_phub;
	h1 = h2;
	if (!h2->ph_next) {
		// if there is only one hub on this canvas
		x->ph_canvas->gl_phub = NULL;
	} else {
		while (h2 && h2 != x) {
			h1 = h2;
			h2 = h2->ph_next;
		}
		if (h2 != x) {
			// this should never happen
			pd_error(x, "preset_hub %s destructor was unable to find its canvas pointer", x->ph_name->s_name);
		} else {
			if (h1 == h2) {
				// this means we're the first on the multi-element list
				x->ph_canvas->gl_phub = h1->ph_next;
			} else {
				h1->ph_next = h2->ph_next;
			}
		}
	}

	// deallocate all the dynamically-allocated memory
	if (x->ph_data) {
		hd1 = x->ph_data;
		while (hd1) {
			if (hd1->phd_npreset) {
				np1 = hd1->phd_npreset;
				while (np1) {
					np2 = np1->np_next;
					if (np1->np_val.l_n)
						alist_clear(&np1->np_val);
					freebytes(np1, sizeof(*np1));
					np1 = np2;
				}
			}
			hd2 = hd1->phd_next;
			freebytes(hd1, sizeof(*hd1));
			hd1 = hd2;
		}
	}
}

void preset_hub_setup(void)
{
	preset_hub_class = class_new(gensym("preset_hub"), 
	    (t_newmethod)preset_hub_new, (t_method)preset_hub_free, 
	    sizeof(t_preset_hub), 0, A_GIMME, 0);

	// have to call this after everything has loaded, otherwise, the hub may not yet exist
	// we do this using pre-loadbang call that is issued before other loadbangs, otherwise
	// out-of-order loadbangs can issue preset recall before the preset has been properly
	// configured
	class_addmethod(preset_hub_class, (t_method)glob_preset_node_list_seek_hub,
        gensym("pre-loadbang"), 0);

    class_addmethod(preset_hub_class, (t_method)preset_hub_recall,
        gensym("recall"), A_DEFFLOAT, 0);
    class_addmethod(preset_hub_class, (t_method)preset_hub_store,
        gensym("store"), A_DEFFLOAT, 0);

    class_addmethod(preset_hub_class, (t_method)preset_hub_clear,
        gensym("clear"), A_DEFFLOAT, 0);
    class_addmethod(preset_hub_class, (t_method)preset_hub_reset,
        gensym("reset"), A_NULL, 0);
    class_addmethod(preset_hub_class, (t_method)preset_hub_purge,
        gensym("purge"), A_NULL, 0);

    class_addbang(preset_hub_class, preset_hub_bang);		// we'll use this to output current preset
	class_setsavefn(preset_hub_class, preset_hub_save);
}

//====================== end preset_hub ============================//

void x_preset_setup(void)
{
    preset_node_setup();
	preset_hub_setup();
}

