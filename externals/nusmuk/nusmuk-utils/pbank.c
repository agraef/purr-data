/* pbank - zack settel 1994  */
// ported to PD - Zack Settel 2004 using the ISPW sources
// note that the generated message receivers are of the format:  "name-n" and not "n-name"(the way it was on the ISPW)
/* features class-independent data structure see pbank.h */


// Source code for this object derived from the original sources in the IRCAM Jimmies release (1994), with the authorization of IRCAM. This object is part of the nSLAM release, developed by La SAT.  nSLAM is also available on the  IRCAM Forum site (http://forum.ircam.fr), as an incentive to PD users to join and contribute to Forum IRCAM"


//  multi row interpolation method provided  by cyrille.henry@la-kitchen.fr

/*
                                                                        
This library is free software; you can redistribute it and/or           
modify it under the terms of the GNU Lesser General Public              
License as published by the Free Software Foundation; either            
version 2 of the License, or (at your option) any later version.        
                                                                        
This library is distributed in the hope that it will be useful,         
but WITHOUT ANY WARRANTY; without even the implied warranty of          
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       
Lesser General Public License for more details.                         
                                                                        
You should have received a copy of the GNU Lesser General Public        
License along with this library; if not, write to the                   
Free Software Foundation, Inc., 59 Temple Place - Suite 330,            
Boston, MA  02111-1307, USA.                                            
                                                                        
Based on PureData by Miller Puckette and others.                        
*/

// Zack Maintenance:  march 06
// fixed recallf method: index bounded now.
// fixed pbank_saveto:  fixed bug when full path is specified.


#include "m_pd.h"
#include <stdio.h>
#include <string.h>


#include "pbank.h"
#include "common.h"

#define VERSION "pbank/pd v1.11  z.s. 2006"
#define  NIL  0L
#define MAX_COLUMNS 2048
#define DEFAULT_COLUMNS 10
#define DEFAULT_ROWS 32
#define MAX_ROWS 256 - EDBUFF
#define PBANK_ID_STRING "theFucKinGPbanKIdeNtifacATioNStrinGGGG"

#define EDBUFF 1	/* edbuffer at rows + 1 */
#define DIRTFLAG 1	/* dirty flag kept at rows + 2 */

#define GETROW(X,R)(((X)->p_data[(R)]))
#define GET_EDBUFF(X)(((X)->p_data[((X)->p_rows)]))
#define CR 13
#define TAB 9



typedef struct pbank	 
{
	t_object p_ob;	 
	t_atom **p_data; 	    /* param matrix : data[PATCH_COUNT] X *data+PARAMCOUNT */
	t_symbol **p_receives;  /* used for back door messaging */
	t_symbol *p_name;	 /* pbank data/file name <optional> */ 
	t_atom **p_dirty; /* points to extra cell in matrix used for dirty flag */
	t_symbol *p_vol;	 /* default volume */ 
	int p_curpatch;
	int p_columns;
	int p_rows;	
    t_canvas *x_canvas;
	
} t_pbank;

t_atom pbank_outlist[3];  /* used by list method for output */

void *pbank_new(t_symbol *s, int argc, t_atom *argv);
void pbank_bang(t_pbank *x);
void pbank_dispose(t_pbank *x);
void pbank_list(t_pbank *x, t_symbol *s, int argc, t_atom *argv);
void pbank_recall(t_pbank  *x,t_float row);
void pbank_recallf(t_pbank  *x,t_float row);
void pbank_dump(t_pbank  *x, t_symbol *s, int argc, t_atom *argv);
void pbank_store(t_pbank  *x,t_float row);
void pbank_set(t_pbank *x, t_symbol *s, int argc, t_atom *argv);
void pbank_put(t_pbank *x, t_symbol *s, int argc, t_atom *argv);
void pbank_interp(t_pbank *x, t_symbol *s, int argc, t_atom *argv);
void pbank_write(t_pbank *x, t_symbol *name);	
void pbank_tobinbuf(t_pbank *x,void *b);
void pbank_read(t_pbank *x, t_symbol *fname);
void pbank_saveto(t_pbank *x,char *fn);
void pbank_db(t_pbank  *x,t_float c,t_float r);
void pbank_setup(void);
int pbank_fromtext(t_pbank *x,void *b);

t_atom **pbank_getmem(t_symbol *name,int columns,int rows);  

t_symbol *z_list, *z_float, *z_symbol, *z_receive;   //  *z_int, 

#define EMPTYSTRING ""

/* class variables */

t_shared *pbank_banks = NIL;	/* linked list of shared data-matrixs */
t_symbol *pbank_ID;

t_atom *pbank_clipboard; /* NOT IMPLEMENTED points to edbuff of "copied" pbank */
/* clipboard needed ???? */

/* ************************************************************ */


void pbank_db(t_pbank  *x, t_float c_float, t_float r_float)
{
    t_atom *a;
    long c = (long) c_float;
    long r = (long) r_float;
    
    *x->p_dirty = (c) ? (t_atom *)1:NIL;


    post("p_dirty at %lx",x->p_dirty);
    post("plus  %ld addr = %lx",r,x->p_data+r);

    a = GETROW(x,r);
}



void pbank_bang(t_pbank  *x)
{

     t_shared *piss = NIL;

      post(":DIRTY FLAS = %ld at %lx",*x->p_dirty,x->p_dirty);
    return;
 
    piss = pbank_banks;		/* maintain list of instance memories */
    while(piss)
    {
	post("->%s at %lx",piss->s_sym->s_name,piss);
	post("			next->%lx",piss->s_next);
	piss = piss->s_next;
    }
}
		/* write item to matrix at given column,row address */
		/* message:  column row thing1......thingN  */
void pbank_set(t_pbank *x, t_symbol *s, int ac, t_atom *argv)
{ 
    t_atom *shit;
    int column,row;

    if (ac < 3)
    {
    	post("ac=%d",ac);
	error("pbank_set: takes at least 3 arguments: column row item(s)");
	return;
    }
    if ((argv)->a_type != A_FLOAT || (argv+1)->a_type != A_FLOAT)    
    {
	error("pbank_set: first arguments must of type float (columnNo. or rowNo.)");
	return;
    }

    if ((column = (int)(argv)->a_w.w_float) < 0) column = 0;
    else if (column >= x->p_columns) column = x->p_columns-1;
    
    if ((row = (int)(argv+1)->a_w.w_float) < 0) row = 0;
    else if (row >= x->p_rows) row = x->p_rows-1;

    shit = GETROW(x,row);

    ac -=2; /* get data */
    argv +=2;
    while (ac-- && (column < x->p_columns))
    {
	if ((argv)->a_type != A_FLOAT && (argv)->a_type != A_SYMBOL && (argv)->a_type != A_FLOAT)  
	{
	    error("pbank_set:  argument no %d must be afloat or symbol",ac+1);
	    return;
	}    
	*(shit+column++) = *argv++;
    }
    *x->p_dirty = (t_atom *)1;	/* set dirty flag */

}

	/* same as set but writes to the edirt buffer:  'put' column thing1......thingN  */
void pbank_put(t_pbank *x, t_symbol *s, int ac, t_atom *argv)
{ 
    t_atom *shit;
    int column;

    if (ac < 2)
    {
    	post("ac=%d",ac);
	error("pbank_put: takes at least 2 arguments: column item(s)");
	return;
    }
    if ((argv)->a_type != A_FLOAT)    
    {
	error("pbank_put: first argument must a number (columnNo.)");
	return;
    }
    if ((column = (int)(argv)->a_w.w_float) < 0) column = 0;
    else if (column >= x->p_columns) column = x->p_columns-1;
    
    shit  = GET_EDBUFF(x);

    ac --; 	/* get data */
    argv ++;
    while (ac-- && (column < x->p_columns))
    {
	if ((argv)->a_type != A_FLOAT && (argv)->a_type != A_SYMBOL && (argv)->a_type != A_FLOAT)  
	{
	    error("pbank_put:  argument no %d must be a float or symbol",ac);
	    return;
	}    
	*(shit+column++) = *argv++;
    }
}



void pbank_recall(t_pbank  *x, t_float row_float)	/* dumps (outputs) and copies row to edbuffer */
{

    int z,ac;
    t_atom *av,*ebuff;
    long row = (long) row_float;
    float indexf = (float) row_float;

    if (row  < 0) row = 0L;
    else if (row > x->p_rows-1) row = (long)x->p_rows-1;

    if (indexf  < 0) indexf = 0L;
    else if (indexf > x->p_rows-1) indexf = (float)x->p_rows-1;


    if (indexf - row)
    {
        pbank_recallf(x, (t_float) indexf);
        return;
    }

    av = GETROW(x,row);
    ebuff = GET_EDBUFF(x);

    if (x->p_receives)	/* back-door sending??? */
    {
	t_symbol *type;
	ebuff += x->p_columns-1;
	av += x->p_columns-1;
	
	for (z=x->p_columns-1;z>-1;z--,ebuff--,av--)
	{
	    *ebuff = *av;
	    if (x->p_receives[z]->s_thing)
	    {
		ac=1;
		if (ebuff->a_type == A_FLOAT) type = z_float;
		// else if (ebuff->a_type == A_LONG) type = z_int;
		else if (ebuff->a_type == A_SYMBOL) type = z_symbol;
		else 
		{
		    error("pbank_list: 3rd element must be a float or sym.");
		    goto exit;
		}
		typedmess(x->p_receives[z]->s_thing, type,ac, ebuff);
	    }
	}
    }
    else		/* otherwise use outlet */
	for (z=0;z<x->p_columns;z++,av++,ebuff++)
	{
	    *ebuff = *av;
	    pbank_outlist->a_w.w_float = (float)z;
	    (pbank_outlist+1)->a_type = ebuff->a_type;
	    if (ebuff->a_type == A_SYMBOL) 
	    {
	    	(pbank_outlist+1)->a_w.w_symbol = z_symbol;
		(pbank_outlist+2)->a_type = A_SYMBOL;
		(pbank_outlist+2)->a_w = ebuff->a_w;
	    	outlet_list(x->p_ob.ob_outlet,0L,3, pbank_outlist);
	    }
	    else 
	    {
		(pbank_outlist+1)->a_w = ebuff->a_w;
		outlet_list(x->p_ob.ob_outlet,0L,2, pbank_outlist);
	    }
	}
exit:
   return;
}


void pbank_recallf(t_pbank  *x, t_float row_float)	/* interpol, dumps (outputs) and copies row to edbuffer */
{
    int u, v, z, ac;
    t_atom *av,*avplus1, *ebuff;


	// no need to range check, since that's done by caller method

    	// float indexf = (float) row_float;
  	float interpfac;
			
     	u=(int)row_float;

 	interpfac = (float) (row_float - u);
 
 	av = GETROW(x,u);
	av += x->p_columns-1;

 	avplus1 = GETROW(x,u+1);
	avplus1 += x->p_columns-1;



	ebuff = GET_EDBUFF(x);

	ebuff += x->p_columns-1;
	

	// for (v=x->p_columns-1;v>-1;v--,ebuff--)
	// {
	//	 SETFLOAT(ebuff, 0);			//clear edit buffer	
	//}

	ebuff = GET_EDBUFF(x);
	ebuff += x->p_columns-1;

	for (v=x->p_columns-1;v>-1;v--,ebuff--,av--,avplus1--)
	{
	  if (ebuff->a_type == A_FLOAT && av->a_type == A_FLOAT && avplus1->a_type == A_FLOAT)
	     SETFLOAT(ebuff, (t_float)(av->a_w.w_float * (1 - interpfac) + avplus1->a_w.w_float * interpfac)); 
	  else if (ebuff->a_type == A_SYMBOL)
	  {
	      if (interpfac <.5) SETSYMBOL(ebuff, av->a_w.w_symbol);
	      else SETSYMBOL(ebuff, avplus1->a_w.w_symbol);			
	  }	
	  else error("pbank: bug found in recallf method");
	}

#ifdef why_is_this_here_sheefa

	if ((int)indexf!=indexf)
	{
		u++;
			
		av = GETROW(x,u);
		av += x->p_columns-1;

		ebuff = GET_EDBUFF(x);
		ebuff += x->p_columns-1;

		for (v=x->p_columns-1;v>-1;v--,ebuff--,av--)
		{
			if (ebuff->a_type == A_FLOAT && av->a_type == A_FLOAT)
				SETFLOAT(ebuff, ebuff->a_w.w_float + av->a_w.w_float * (indexf - u +1)); 			else if (ebuff->a_type == A_SYMBOL)
				SETSYMBOL(ebuff, av->a_w.w_symbol);
			else 	error("pbank: bug found in recallf method");	
		}
	}

#endif

	if (x->p_receives)	/* back-door sending??? */
	    {
		t_symbol *type;

		ebuff = GET_EDBUFF(x);
		ebuff += x->p_columns-1;

		for (z=x->p_columns-1;z>-1;z--,ebuff--,av--)
			{

		    if (x->p_receives[z]->s_thing)
			    {
				ac=1;
				if (ebuff->a_type == A_FLOAT) type = z_float;
				// else if (ebuff->a_type == A_LONG) type = z_int;
				else 
				if (ebuff->a_type == A_SYMBOL) type = z_symbol;
				else 
					{
					error("pbank_list: 3rd element must be a float or sym.");
					goto exit;
					}
				typedmess(x->p_receives[z]->s_thing, type,ac, ebuff);
			    }
			}
	    }
	else		/* otherwise use outlet */
		{
		ebuff = GET_EDBUFF(x);

		for (z=0;z<x->p_columns;z++,ebuff++)
			{
		    pbank_outlist->a_w.w_float = (float)z;
		    (pbank_outlist+1)->a_type = ebuff->a_type;
		    if (ebuff->a_type == A_SYMBOL) 
			    {
		    	(pbank_outlist+1)->a_w.w_symbol = z_symbol;
				(pbank_outlist+2)->a_type = A_SYMBOL;
				(pbank_outlist+2)->a_w = ebuff->a_w;
				outlet_list(x->p_ob.ob_outlet,0L,3, pbank_outlist);
				}
			else 
				{
				(pbank_outlist+1)->a_w = ebuff->a_w;
				outlet_list(x->p_ob.ob_outlet,0L,2, pbank_outlist);
				}
			}
		}
exit:
   return;

}


void pbank_interp(t_pbank *x, t_symbol *s, int argc, t_atom *argv)
/* interpol, dumps (outputs) and copies row to edbuffer */
{

    int u, v, z, ac;
    t_atom *av,*ebuff;

	ebuff = GET_EDBUFF(x);
	ebuff += x->p_columns-1;
	
	for (v=x->p_columns-1;v>-1;v--,ebuff--)
	{
		SETFLOAT(ebuff, 0);			//clear edit buffer	
	}
	for (u=argc-1;u>-1;u--)
	{
		av = GETROW(x,u);
		av += x->p_columns-1;

		ebuff = GET_EDBUFF(x);
		ebuff += x->p_columns-1;

		for (v=x->p_columns-1;v>-1;v--,ebuff--,av--)
		{
			if (ebuff->a_type == A_FLOAT && av->a_type == A_FLOAT)
				SETFLOAT(ebuff, ebuff->a_w.w_float + av->a_w.w_float * atom_getfloatarg(u, argc, argv) ); //add value of each columns and row
			else 
				SETSYMBOL(ebuff, gensym("null"));
		}
	}

	if (x->p_receives)	/* back-door sending??? */
    {	
		t_symbol *type;

		ebuff = GET_EDBUFF(x);
		ebuff += x->p_columns-1;

		for (z=x->p_columns-1;z>-1;z--,ebuff--,av--)
		{

		    if (x->p_receives[z]->s_thing)
		    {
				ac=1;
				if (ebuff->a_type == A_FLOAT) type = z_float;
				// else if (ebuff->a_type == A_LONG) type = z_int;
				else 
				if (ebuff->a_type == A_SYMBOL) type = z_symbol;
				else 
				{
					error("pbank_list: 3rd element must be a float or sym.");
					goto exit;
				}
				typedmess(x->p_receives[z]->s_thing, type,ac, ebuff);
		    }
		}
    }
    else		/* otherwise use outlet */
	{
		ebuff = GET_EDBUFF(x);

		for (z=0;z<x->p_columns;z++,ebuff++)
		
		{
		    pbank_outlist->a_w.w_float = (float)z;
		    (pbank_outlist+1)->a_type = ebuff->a_type;
		    if (ebuff->a_type == A_SYMBOL) 
			    {
		    	(pbank_outlist+1)->a_w.w_symbol = z_symbol;
				(pbank_outlist+2)->a_type = A_SYMBOL;
				(pbank_outlist+2)->a_w = ebuff->a_w;
				outlet_list(x->p_ob.ob_outlet,0L,3, pbank_outlist);
				}
			else 
				{
				(pbank_outlist+1)->a_w = ebuff->a_w;
				outlet_list(x->p_ob.ob_outlet,0L,2, pbank_outlist);
				}
		
		}
	}

exit:
   return;
}

// 'dump' dumps edit buff,  'dump n' dumps memory n, 'dump n l' dumps row n as one single list
void pbank_dump(t_pbank  *x, t_symbol *s, int argc, t_atom *argv)	/* dumps (outputs) ediuffer or row */
{

    int z,ac, row;
    t_atom *ebuff;

	if (argc==0)
	{
	   ebuff = GET_EDBUFF(x);
	   if (x->p_receives)	/* back-door sending??? */
	   {
		   t_symbol *type;
		   ebuff += x->p_columns-1;
	
		   for (z=x->p_columns-1;z>-1;z--,ebuff--)
		   {
			   if (x->p_receives[z]->s_thing)
			   {
				   ac=1;
				   if (ebuff->a_type == A_FLOAT) type = z_float;
				   // else if (ebuff->a_type == A_LONG) type = z_int;
				   else if (ebuff->a_type == A_SYMBOL) type = z_symbol;
				   else 
				   {
					   error("pbank_list: 3rd element must be a float or sym.");
					   goto exit;
				   }
				   typedmess(x->p_receives[z]->s_thing, type,ac, ebuff);
			   }
		   }
	   }
	   else		/* otherwise use outlet */
		   for (z=0;z<x->p_columns;z++,ebuff++)
		   {
			   pbank_outlist->a_w.w_float = (float)z;
			   (pbank_outlist+1)->a_type = ebuff->a_type;
			   if (ebuff->a_type == A_SYMBOL) 
			   {
				   (pbank_outlist+1)->a_w.w_symbol = z_symbol;
				   (pbank_outlist+2)->a_type = A_SYMBOL;
				   (pbank_outlist+2)->a_w = ebuff->a_w;
				   outlet_list(x->p_ob.ob_outlet,0L,3, pbank_outlist);
			   } 
			   else 
			   {
				   (pbank_outlist+1)->a_w = ebuff->a_w;
				   outlet_list(x->p_ob.ob_outlet,0L,2, pbank_outlist);
			   }
		   }
	}

	else
	{
		if ((row = (int)(argv)->a_w.w_float) < 0) row = 0;
		else if (row >= x->p_rows) row = x->p_rows-1;
  
		ebuff = GETROW(x, row);

		if (x->p_receives)	/* back-door sending??? */
		{
			t_symbol *type;
			ebuff += x->p_columns-1;
	
			for (z=x->p_columns-1;z>-1;z--,ebuff--)
			{
				if (x->p_receives[z]->s_thing)
				{
					ac=1;
					if (ebuff->a_type == A_FLOAT) type = z_float;
					// else if (ebuff->a_type == A_LONG) type = z_int;
					else if (ebuff->a_type == A_SYMBOL) type = z_symbol;
					else 
					{
						error("pbank_list: 3rd element must be a float or sym.");
						goto exit;
					}
					typedmess(x->p_receives[z]->s_thing, type,ac, ebuff);
				}
			}
		}
		else		/* otherwise use outlet */
		{	
			if(argc>1)  // dump as single list
				outlet_list(x->p_ob.ob_outlet,0L,x->p_columns, ebuff);
			else
			{
					for (z=0;z<x->p_columns;z++,ebuff++)
					{
						pbank_outlist->a_w.w_float = (float)z;
						(pbank_outlist+1)->a_type = ebuff->a_type;
						if (ebuff->a_type == A_SYMBOL) 
						{
							(pbank_outlist+1)->a_w.w_symbol = z_symbol;
							(pbank_outlist+2)->a_type = A_SYMBOL;
							(pbank_outlist+2)->a_w = ebuff->a_w;	
							outlet_list(x->p_ob.ob_outlet,0L,3, pbank_outlist);
						}
						else 
						{
							(pbank_outlist+1)->a_w = ebuff->a_w;
							outlet_list(x->p_ob.ob_outlet,0L,2, pbank_outlist);
						}
					}
			}
		}
	}
exit:
   return;
}


void pbank_store(t_pbank  *x, t_float row_float)	/* copies edbuffer to row */
{   
    t_atom *av,*ebuff;
    int z;
    long row = (long) row_float;

    if (row  < 0) row = 0L;
    else if (row >= x->p_rows) row = (long)x->p_rows-1;

    av = GETROW(x,row);
    ebuff = GET_EDBUFF(x);
    for (z=0;z<x->p_columns;z++,av++,ebuff++)
    {
    	 *av = *ebuff;
    }
    *x->p_dirty = (t_atom *)1;	/* set dirty flag */
}


/* eg 'list COLUMN ROW <optional> VALUE'  - argc=2 is read, arg=3 is write */

void pbank_list(t_pbank *x, t_symbol *s, int argc, t_atom *argv)
{

   t_atom *av;
   int ac,column,row;

    if ((argc != 2) && (argc != 3))
    {
	error("pbank_list: less than 2, or more than 3 elements in list");
	goto exit;
    }
    
    
    if (((argv)->a_type != A_FLOAT) || ((argv+1)->a_type != A_FLOAT))
    {
	error("pbank_list: first two elements must be numbers");
	goto exit;
    }
    if ((column = (int)(argv)->a_w.w_float) < 0) column = 0;
    else if (column >= x->p_columns) column = x->p_columns-1;

    if ((row = (int)(argv+1)->a_w.w_float) < -1) row = -1;
    else if (row >= x->p_rows) row = x->p_rows-1;
    
    
    if (row == -1)
	{
		av = GET_EDBUFF(x);
	}
	else
	{
		av = GETROW(x,row);
	}

    av += column;
    ac = 1;

    if (argc == 2) /* read */
    {
	if (x->p_receives)
	{
	    if (x->p_receives[column]->s_thing)
	    {
	    	t_symbol *type;
	    	if (av->a_type == A_FLOAT) type = z_float;
		// else if (av->a_type == A_LONG) type = z_int;
		else if (av->a_type == A_SYMBOL) type = z_symbol;
		else 
		{
		    error("pbank_list: 3rd element must be a float or sym.");
		    goto exit;
		}
		typedmess(x->p_receives[column]->s_thing, type,ac, av);
	    }
	}
	else
	{
	    pbank_outlist->a_w.w_float = (t_float)column;
	    (pbank_outlist+1)->a_type = av->a_type;
	    if (av->a_type == A_SYMBOL) 
	    {
	    	(pbank_outlist+1)->a_w.w_symbol = z_symbol;
		(pbank_outlist+2)->a_type = A_SYMBOL;
		(pbank_outlist+2)->a_w = av->a_w;
	    	outlet_list(x->p_ob.ob_outlet,0L,3, pbank_outlist);
	    }
	    else 
	    {
		(pbank_outlist+1)->a_w = av->a_w;
		outlet_list(x->p_ob.ob_outlet,0L,2, pbank_outlist);
	    }
	}

    }
    else /* write */
    {
    	switch ((argv+2)->a_type)
	{
	
	    case A_FLOAT:
		    SETFLOAT(av,(argv+2)->a_w.w_float);
		    break;
	   /*  case  A_LONG:
		    SETLONG(av,(argv+2)->a_w.w_long);
		    break;
            */
	    case  A_SYMBOL:
		    SETSYMBOL(av,(argv+2)->a_w.w_symbol);
		    break;
	    default: 	
		error("pbank_list: 3rd element must be a float or sym.");
		goto exit;
	}
    *x->p_dirty = (t_atom *)1;	/* set dirty flag */
    }
exit:
   return;
	  
}

int pbank_fromtext(t_pbank *x,void *b)
{
    t_atom *av;
    int z,atype;
    int columnpos = 0,columns = 0;
    int rowpos = 0, rows = 0;
    long items = 0;
    int argc = binbuf_getnatom(b),count = 0;
    	
    t_atom *ap = binbuf_getvec(b);		
    

                                                /* get pbank symbol */
    if (ap->a_type != A_SYMBOL || ap->a_w.w_symbol != gensym("pbank"))
    {
	error("pbank_fromtext: bad file format: item one must be the symbol 'pbank");
	goto error;
    }

    for (z=0;z<2;z++)
    {
        ap++, count++;

    	// binbuf_getatom(b,&p1,&p2,&at);		/* get columns and rows */
	if (ap->a_type != A_FLOAT)
	{
	    error("pbank_fromtext: bad file format: item two and three must be COLUMNcount and ROWcount");
	    goto error;
	}
	if (z==0) columns = (int)ap->a_w.w_float;
	else rows = (int)ap->a_w.w_float;
    }
   
    if (columns < 1 || rows < 1)
    {
	error("pbank_fromtext: bad value(s) for item two and/or three (COLUMNcount and/or ROWcount)");
	goto error;
    }
    if (columns != x->p_columns)
    {
	error("pbank_fromtext: bad file format: wrong no. of columns (%d) in file", columns);
	goto error;
    }
    
   		/* get comma */
    ap++, count++;
    if (ap->a_type != A_COMMA)
    {
	error("pbank_fromtext: bad file format: comma expected after third item");
	goto error;
    }
    
       
 
    av = GETROW(x,rowpos);

   // post("pbank_fromtext: columns %d   rows %d",columns,rows);

    while (count < argc)
    {
 
         	 
        if (rowpos > x->p_rows)	// safety check- remove later
        {
            bug("pbank_fromtext: rowpos=%d x->p_rows=%d  items=%d  argc=%d",rowpos,x->p_rows,count,argc);
            error("pbank_fromtext: rowpos greater than x->p_rows:  aborting");
            goto error;
        }
   
 
        
        ap++, count++;		// get next atom
        atype = ap->a_type;
    
	if (atype == A_NULL)
	{
	    if (count == argc)  goto done;
            else 
            {
                error("pbank_fromtext: unexpected A_NULL type read at position %d: aborting", count);
                goto error;
            }
	}
        
        
	if (columnpos >= x->p_columns) goto skipit;
	

 
	if (atype==A_FLOAT || atype==A_SYMBOL)
	{
	    av[columnpos] = *ap;		/* write atom to matrix */
	    items++;
	}

	else if (atype==A_COMMA)		/* new line */
	{
	    if (columnpos) 
	    post("pbank_fromtext: warning: unexpected comma found in column %d row %d", columnpos,rowpos);
	    goto skipit;
	}
	else  
	{
            // post("ATYPE = %d", atype);
	    post("pbank_fromtext: warning: strange token found in at column %d, row %d of file", columnpos,rowpos);
	    goto ignore;
	}
skipit:			/* ignore any data that lies outside of X's dimensions */
	if (atype!=A_COMMA) 
		columnpos++;
	if (columnpos == columns)
	{
	    columnpos = 0;
	    rowpos++;
	    av = GETROW(x,rowpos);
	}		
ignore:;
    }
done:
    return(items);
error:
    return(0);
}


void pbank_fromfile(t_pbank *x,char *name, int type, int silentFlag) // filename with or without path
{
    void *b;
    int errorf, itemcount;
    char nilbuf[1];
    
    nilbuf[0] = 0;
    
        
    b = binbuf_new();
   //  errorf = binbuf_read(b, name, vol, type);
   
     if (*name != '/' && *name != '~')  // file name is not an absolute path
           errorf =  binbuf_read(b, name, canvas_getdir(x->x_canvas)->s_name, type);
    else
        errorf =  binbuf_read(b, name, nilbuf, type);


    if (errorf) 
	{
     	if (!silentFlag) post("warning: pbank_fromfile:%s not found",name);   // not error, since filename may be used only as symbol for pbanks sharing memory
    }
	else 
    {
 	itemcount = pbank_fromtext(x,b);
        post("pbank_fromfile: %d items read from file %s",itemcount, name);
	*x->p_dirty = NIL;
    }
    binbuf_free(b);
}


#define TEXT 0
#define BINARY 1

void pbank_read(t_pbank *x, t_symbol *fname)
{
 						   /* if (!open_dialog(name, &vol, &type, types, 1)) */
    pbank_fromfile(x,fname->s_name,TEXT,0);
}


	/* format for file <float type> <symbol *pname> <float p1 p2 p3 p4..p10> A_COMMA.... */
void pbank_tobinbuf(t_pbank *x,void *b)
{
    t_atom at,*av;
    t_symbol *s;
    char shit[256];
    int z,i;

    sprintf(shit,"\n");
    s = gensym("");

    SETSYMBOL(&at,gensym("pbank"));	/* write class name */
    binbuf_add(b,1,&at);
    
    SETFLOAT(&at,(t_float) x->p_columns);	/* write column count */
    binbuf_add(b,1,&at);
    
    SETFLOAT(&at,(t_float) x->p_rows);	/* write row count */
    binbuf_add(b,1,&at);
    
    SETCOMMA(&at);		/* write comma */
    binbuf_add(b,1,&at);

    SETSYMBOL(&at,gensym(shit));	/* write CR */
    binbuf_add(b,1,&at);
  
    
    for (z=0;z<x->p_rows;z++)	/* write params to binbuf */
    {
   	av = GETROW(x,z);
 	for (i=0;i<x->p_columns;i++,av++)	
	{
	    
/*	    if (av->a_type == A_FLOAT) 
	    {
	    	post("A_FLOAT"); postfloat(av->a_w.w_float);
	    }
	    
	   // else if (av->a_type == A_LONG)  post("A_LONG %ld", av->a_w.w_long);
		
	    else if (av->a_type == A_SYMBOL) post("A_SYMBOL %s", av->a_w.w_symbol->s_name); 
*/
	    binbuf_add(b,1,av);  /* write atom */
	}
	SETCOMMA(&at);	/* rows seperated by comma */
	binbuf_add(b,1,&at);
	SETSYMBOL(&at,gensym(shit));	/* write CR */
        binbuf_add(b,1,&at);

    }
}

void pbank_write(t_pbank *x, t_symbol *name)	
{
    char fn[256];

    sprintf(fn,"%s", name->s_name);	
	
	/* def volume shit here  */

    pbank_saveto(x,fn);
}

void pbank_saveto(t_pbank *x,char *fn)	
{
    int errorf = 0;
    char nilbuf[1];
    void *b = binbuf_new();

 
   pbank_tobinbuf(x,b);

    nilbuf[0]= 0;	// nilbuf[1]= 0;     bug fixed:  zk-03/06
    
    if (*fn != '/' && *fn != '~')  // file name is not an absolute path
        errorf = binbuf_write(b, fn, canvas_getdir(x->x_canvas)->s_name, TEXT);	/* save as TEXT  */
   else
        errorf =  binbuf_write(b, fn, nilbuf, TEXT);	/* save as TEXT  */
       
    binbuf_free(b);

    if (errorf) 
        error("pbank_saveto: couldn't write %s", fn);
    else 
    {
        *x->p_dirty = NIL;
        post("pbank_write:wrote file %s",fn);
    }
}

void pbank_dispose(t_pbank *x)
{
    int z;
	
    t_shared *prePiss = NIL;
    t_shared *piss = NIL;
    
    if (*x->p_dirty) 
    {
    	/* need a save dialog here */
	post("unsaved data");
    }

    prePiss = piss = pbank_banks;		/* maintain list of instance memories */
    if (x->p_name)
    {
	while(piss)
	{
 
  	  /*  post("->%s at %lx",piss->s_sym->s_name,piss);
	    post("	next->%lx",piss->s_next); */
    
	    if ((t_symbol *)piss->s_sym == x->p_name) break;
	    
	    prePiss = piss;
	    piss = piss->s_next;
	}
	if (!piss) 
	{
	    error("pbank_dispose: bug found- can't find symbol");
	    goto skip;
	}
       /*  post("found %s", piss->s_sym->s_name); */

	piss->s_refcount--;
	if (piss->s_refcount) 
	{
	    /* post("%s refcount = %d",piss->s_sym->s_name, piss->s_refcount); */
	    goto skip;	/* things still pointing to data- don't dispose */
	}
	
        piss->s_sym->s_thing = (void *) 0L;	/* clear symbol to no longer point to this class */

	if (!piss->s_next)  	/* last element in list */
	{
	    if (piss == pbank_banks)	/* first element in list of only one element*/
	    {
		/* post("같같같 unique element in list"); */
		 freebytes(piss, (int)sizeof(t_shared));
		 pbank_banks = NIL;
	    }
	    else /* last element in list - snip it */
	    {
		if (!prePiss) 
		{
		    error("bug found in pbank_dispose (prePiss)");
		    goto skip;
		}
		/* post("같같같 last element in list"); */
		prePiss->s_next = NIL;
		freebytes(piss, (int)sizeof(t_shared));
	    }
	}
	else if (piss == pbank_banks)	/* first element in list of at least two elements */
	{
	   /*  post("같같같 first element in list"); */
	    pbank_banks =  piss->s_next;
	    freebytes(piss, (int)sizeof(t_shared));
	}
	else 		/* element with adjacent elements */
	{
	    if (!prePiss) 
	    {
		error("bug found in pbank_dispose (prePiss)");
		goto skip;
	    }
	  /*  post("같같같 embedded element in list"); */
	    prePiss->s_next = piss->s_next;
	    freebytes(piss, (int)sizeof(t_shared));
	}
	
    }

    for (z=0;z<x->p_rows+EDBUFF;z++) 
    {
    	freebytes((void *)x->p_data[z], (int)sizeof(t_atom)*x->p_columns);
    } 

    freebytes((void *)x->p_data,(int)sizeof(t_atom *)*(x->p_rows+EDBUFF+DIRTFLAG));

skip:
    if (x->p_receives)
    	freebytes((void *)x->p_receives, (int)sizeof(t_symbol *)*x->p_columns);

}



t_class *pbank_class;

void pbank_setup(void)
{
	pbank_class = class_new(gensym("pbank"), (t_newmethod) pbank_new, (t_method) pbank_dispose, sizeof(t_pbank), 0, A_GIMME,0); 
	class_addbang(pbank_class, (t_method) pbank_bang);
	class_addlist(pbank_class, (t_method) pbank_list); 
	class_addmethod(pbank_class, (t_method) pbank_put, gensym("put"),A_GIMME,0); 
	class_addmethod(pbank_class, (t_method) pbank_db, gensym("db"), A_FLOAT,A_FLOAT, 0);
	class_addmethod(pbank_class, (t_method) pbank_recall, gensym("recall"), A_FLOAT,0);
	// class_addmethod(pbank_class, (t_method) pbank_recallf, gensym("recallf"), A_FLOAT,0);
	class_addmethod(pbank_class, (t_method) pbank_dump, gensym("dump"),A_GIMME,0);
	class_addmethod(pbank_class, (t_method) pbank_store, gensym("store"), A_FLOAT,0);
	class_addmethod(pbank_class, (t_method) pbank_write, gensym("write"), A_SYMBOL,0);
	class_addmethod(pbank_class, (t_method) pbank_read, gensym("read"), A_SYMBOL,0);
	class_addmethod(pbank_class, (t_method) pbank_interp, gensym("interp"), A_GIMME,0);
	class_addmethod(pbank_class, (t_method) pbank_set, gensym("set"),A_GIMME,0); 

	
	pbank_outlist->a_type = A_FLOAT;

	z_list = gensym("list");
	z_float = gensym("float");
	// z_int = gensym("int");
	z_symbol = gensym("symbol");
	z_receive = gensym("receive");
	pbank_ID = gensym(PBANK_ID_STRING);
	pbank_ID->s_thing = (void *)pbank_ID;

	post("%s", VERSION);
}


/* columns rows  memory-name receiveName<optional> */
/* note: memory-name is optional when no receiveName argument is specified */  
void *pbank_new(t_symbol *s, int argc, t_atom *argv)
{
    int z=0;
    char *shit;
    char piss[256];
  
    t_pbank *x = (t_pbank *)pd_new(pbank_class);

    x->p_name = NIL;
    x->p_receives = NIL;
    x->x_canvas = canvas_getcurrent();

    // x->p_vol = getdefvolume(); 

    if (!InRange(argc,2,4))
    {
	error("pbank_new: bad arg count - takes: colums rows fname <opt symbol> sendname");
	goto err;
    }
    
    
    
    
    if (((argv)->a_type != A_FLOAT) || ((argv+1)->a_type != A_FLOAT))
    {
	error("pbank_new: first two elements must be numbers");
	goto err;
    }


//    if ((argv)->a_w.w_float < 1 || (argv+1)->a_w.w_float < 1) 
//    {
//	error("pbank_new: first or second argument less than 1, can't continue");
//	goto err;
//    }	 

    if (!InRange((argv)->a_w.w_float, 1,MAX_COLUMNS-1))
    {
    	x->p_columns = DEFAULT_COLUMNS;
	post("pbank_new: warning: columns argument out of range, setting to %d",x->p_columns);
    }
    else x->p_columns = (int)(argv)->a_w.w_float;
       
    if (!InRange((argv+1)->a_w.w_float, 1,MAX_ROWS-1))
    {
    	x->p_rows = DEFAULT_ROWS;
	post("pbank_new: warning: rows argument out of range, setting to %d",x->p_rows);
    }
    else x->p_rows = (int)(argv+1)->a_w.w_float;

    

    if (argc == 2) x->p_name = NIL;
    else
    {
	if ((argv+2)->a_type != A_SYMBOL)
	{
	    error("pbank_new: bad third arg: needs to be a symbol (or empty string %s)",gensym("")->s_name);
	    goto err;
	}
	shit = (argv+2)->a_w.w_symbol->s_name;
					/* is it  a crosshatch  or un-defined ("") ? */
	x->p_name = ((shit[0] == '#') || (shit[0] == '"' && shit[1] == '"')) ? NIL:gensym(shit); 
    }


    if (argc == 4) 	/* argc == 4: establish output pointers to receive objects  */
    {
	if ((argv+3)->a_type != A_SYMBOL)
        {
            if ((argv+3)->a_type == A_FLOAT)
            {
                post("pbank_new: warning: 4th arg not symbol: ignoring (%f)", (argv+3)->a_w.w_float);
                goto nosyms;
            }
            else 
            {
                error("pbank_new: optional fourth arg. needs to be a symbol");
                goto err;
            }
        }


	shit = (argv+3)->a_w.w_symbol->s_name;
	if (shit[0] == '#' || (shit[0] == '"' && shit[1] == '"')) goto nosyms; 	/* crosshatch here */;
	
	x->p_receives = (t_symbol **)getbytes((int)sizeof(t_symbol *)*x->p_columns);
    
	for (z=0;z<x->p_columns;z++) 
	{
	    sprintf(piss,"%s-%d",shit,z);	// generate symbol of form:  "name-columnNo"
	    x->p_receives[z] = gensym(piss);
    
	    if (x->p_receives[z]->s_thing)  
	    {							/* sends or receive obj */
            
           //  post("found us a %s",class_getname(pd_class(x->p_receives[z]->s_thing)));
            
		// if ((void *)pd_class(x->p_receives[z]->s_thing)->c_sym != gensym("through")) 
             /*
                    NO TESTING FOR SHARED SYMBOLS   
             
               if (strcmp(class_getname(pd_class(x->p_receives[z]->s_thing)), "send") && 
                    strcmp(class_getname(pd_class(x->p_receives[z]->s_thing)), "receive") &&
                    strcmp(class_getname(pd_class(x->p_receives[z]->s_thing)), "bindlist"))
		{
 		    error("pbank_new: symbol %s already being used, can't use it",shit);
		    post("z=%d  found a %s",z,class_getname(pd_class(x->p_receives[z]->s_thing)));
		    goto err;
		}
            */
            
	    }
	}
    }

nosyms:
    if (NIL == (x->p_data = pbank_getmem(x->p_name, x->p_columns,x->p_rows)))  goto err; /* can't allocate data matrix */
    x->p_dirty = x->p_data+x->p_rows-1+EDBUFF+DIRTFLAG;	/* points to extra cell in matrix used for dirty flag */
 

    if (x->p_name) 
	{
		pbank_fromfile(x,x->p_name->s_name, TEXT,1); 
	}

    outlet_new((t_object *) x, gensym("list"));

    return(x);    
err:
    if (x->p_receives) freebytes((void *)x->p_receives, (int)sizeof(t_symbol *)*x->p_columns);
    return(NIL);
}






/* NOTE: light error checking --- allocates memory and manages allocation list */
/* returns  NIL (on error)  or pointer to existing or new data matrix */
/* note memory size is columns*(rows+1) + 1 - where the extra row is for the edit buffer */
/* and the extra cell is for the DIRTY flag */
/* class independent allocation - needs class independent dispose method */

t_atom **pbank_getmem(t_symbol *name,int columns,int rows) 
{
    int z,i;
    t_shared *piss = NIL;
    t_atom **matrix,*av;
    char crap[256];

    if (!InRange(columns, 1,MAX_COLUMNS-1)) columns =  DEFAULT_COLUMNS;
    if (!InRange(rows, 1,MAX_ROWS-1)) rows = DEFAULT_ROWS;

    if (name)
    {
	if (name->s_thing)
	{
	    if (name->s_thing == pbank_ID->s_thing) /* memory already exists - increment refcount */
	    {
	    	piss = pbank_banks;		/* maintain list of instance memories */
	    	while(piss)
		{
		    if (piss->s_sym == name)	/* pbank instances sharing memory */
		    {
		    	if (piss->s_columns != columns || piss->s_rows != rows)
			{
			    error("pbank_getmem: pbank %s's dimensions must be (%d x %d)",name->s_name,
			    			 piss->s_columns,piss->s_rows);
			    return(NIL);
			}
			piss->s_refcount++;
			/* post("%s refcount=%d",piss->s_sym->s_name,piss->s_refcount); */
			return(piss->s_data);
		    }
		    piss = piss->s_next;
		}
		error("pbank_getmem: bug: name %s not found",name->s_name);
		return(NIL);
	    }
	    else /* bad symbol - already taken - can't do anything */
	    {
		error("pbank_getmem: symbol %s already being used by another object",name->s_name);
		return(NIL);
	    }
	}
	else			/* allocate entry for given name in shared memory list */
	{

	    name->s_thing = pbank_ID->s_thing; /* associate symbol with pbank class */
	    piss = getbytes(sizeof(t_shared)); /* allocate new entry */
	    piss->s_sym = name;
	    piss->s_refcount = 1;
	    piss->s_next = (pbank_banks) ? pbank_banks:NIL; 
	    pbank_banks = piss;		/* link it in at head of list */
	    /* post("PISS alloc %lx",piss); */
	}
    }

    /* allocate memory for data matrix */
    /* allocate "rows" + 1 (edbuff) + 1 (dirty flag) ROWS  */
    matrix = (t_atom **)getbytes((int)sizeof(t_atom *)*(rows+EDBUFF+DIRTFLAG));
    for (z=0;z<rows+EDBUFF;z++) 
	matrix[z] = (t_atom *)getbytes(sizeof(t_atom)*columns);


    for (z=0;z<rows+EDBUFF;z++) /*  data matrix - all 0's */
    {
 
        sprintf(crap,"untitled_%d", z);        
	av = matrix[z];
        SETSYMBOL(av,gensym(crap));
	for (i=1;i<columns;i++)
	    SETFLOAT(av+i,0L);
    } 

    matrix[rows-1+EDBUFF+DIRTFLAG] = NIL;	 /* set dirty flag cell to 0 */
    if (piss) 
    {
    	piss->s_data = matrix;	/* keep pointer to memory with shared */
	piss->s_columns = columns;
	piss->s_rows = rows;
    }
    return(matrix);	/* no errors - return pointer to data matrix */
}









