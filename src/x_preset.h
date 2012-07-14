
#ifndef __x_preset_h_
#define __x_preset_h_

/*
Global preset system by Ivica Ico Bukvic <ico@vt.edu> July, 2012
*/

// each node in the patch
typedef struct _preset_node
{
    t_object pn_obj;
    t_atom pn_val;			// last known value (null if not initialized)

	t_symbol *pn_hub_name;	// hub name this node is associated with
	t_preset_hub *pn_hub;	// pointer to the hub (null if none, equiv. to active/disabled)

	t_canvas *pn_canvas;
	int *pn_gl_loc;
	int  pn_gl_loc_length;

	// we use this to compare with the old position,
	// so that hub can be notified of node's new position 
	int *pn_old_gl_loc;
	int  pn_old_gl_loc_length;

	t_outlet *pn_outlet;
} t_preset_node;

// stores each of the presets per each preset_node
typedef struct _node_preset
{
	int np_preset;
	t_atom np_val;
	struct _node_preset *np_next;
} t_node_preset;

// stores each of the nodes within a hub
typedef struct _preset_hub_data
{
	t_preset_node *phd_node;		// if the node is erased, this will be NULL (inactive)
	int *phd_pn_gl_loc;				// last known location of the node (according to the hub)
	int  phd_pn_gl_loc_length;		// location array's length (see x_preset.c for explanation)
	struct _preset_hub_data *phd_next;
	t_node_preset *phd_npreset;
} t_preset_hub_data;

// preset hub (nexus for data saving/recalling)
struct _preset_hub
{
	t_object ph_obj;
	t_symbol *ph_name;
	int ph_invis;					// make it invisible (only for the k12 mode)
	int ph_preset;					// last enabled preset (-1 at init time)

	t_canvas *ph_canvas;

	struct _preset_hub *ph_next;	// next hub on the same canvas
	t_preset_hub_data *ph_data;		// pointer to data (single-linked list)

	t_outlet *ph_outlet;
};

typedef struct _glob_preset_hub_list
{
	t_preset_hub *gphl_hub;
	struct _glob_preset_hub_list *gphl_next;
} t_glob_preset_hub_list;

typedef struct _glob_preset_node_list
{
	t_preset_node *gpnl_node;
	int gpnl_paired;				// whether the node is paired with a hub (otherwise don't bother updating its location)
	struct _glob_preset_node_list *gpnl_next;
} t_glob_preset_node_list;

#ifndef t_preset_hub
#define t_preset_hub struct _preset_hub
#endif

EXTERN int we_are_undoing;

#endif /* __x_preset_h_ */

