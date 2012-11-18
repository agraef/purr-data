


typedef struct shared
{
	t_symbol *s_sym; /* shared memory name */
	t_atom **s_data; /* memory pointer */
	int s_rows;	/* memory dimension */
	int s_columns;	/* memory dimension */
	int s_refcount;	/* number of objects pointing to "s_data" */
	struct shared *s_next;
} t_shared;


