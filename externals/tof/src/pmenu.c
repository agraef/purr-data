/* pmenu widget for PD                                              *
 * Based on:
 * pmenu by Ben Bogart         
 * and button from GGEE by Guenter Geiger                                *

 * This program is distributed under the terms of the GNU General Public *
 * License                                                               *

 * pmenu is free software; you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.                                   *

 * pmenu is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          */

/* CHANGES
 
 0.3 Added the possibility of adding anything (instead of only symbols)
 0.3 Split the output message into two outlets
 
 */


/* Append " x " to the following line to show debugging messages */
#define DEBUG(x) 



#include <m_pd.h>
#include <g_canvas.h>
#include <stdio.h>
#include <string.h>


static char pmenu_buffer[MAXPDSTRING];




typedef struct _pmenu
{
     t_object x_obj;

     t_glist * x_glist;

     t_symbol*  callback;
	
     int current_selection;
     	 
     t_symbol* bg_color;
     t_symbol* fg_color;
     t_symbol* hi_color;
     t_symbol* co_color;
    
     t_atom* options;
     int     options_memory;
	 int     options_count;
   
      int indicator;
      int focusing;
	
	t_outlet* outlet1;
	t_outlet* outlet2;
   
} t_pmenu;

#include "pmenu_w.h"

static void pmenu_output(t_pmenu* x)
{
	/*
  t_atom atoms[2];
  SETFLOAT(atoms,x->current_selection);
  //SETSYMBOL(atoms+1,x->x_options[x->current_selection]);
  atoms[1] = *(x->options+x->current_selection);
  outlet_list(x->x_obj.ob_outlet, &s_list, 2, atoms);
	*/
	outlet_float(x->outlet1, x->current_selection);
	// outlet_anything(t_outlet *x, t_symbol *s, int argc, t_atom *argv)
	if ( (x->options+x->current_selection)->a_type == A_SYMBOL ) {
		outlet_symbol(x->outlet2, atom_getsymbol(x->options+x->current_selection));
	} else {
		outlet_float(x->outlet2, atom_getfloat(x->options+x->current_selection));

	}
	
	
				 
  //if ( x->send != x->s_empty && x->send->s_thing) pd_forwardmess(x->send->s_thing, 2,atoms);

}


static void pmenu_callback(t_pmenu* x, t_floatarg f)
{
  //DEBUG(post("output start");)
  x->current_selection= (int)f;
  //pmenu_w_text(x,x->x_options[x->current_selection]);
  pmenu_output(x);
  
}
/*
static void pmenu_save(t_gobj *z, t_binbuf *b)
{
        DEBUG(post("save start");)
    
	
    t_pmenu *x = (t_pmenu *)z;
    
    t_atom* creation = binbuf_getvec(x->x_obj.te_binbuf);
       
    
    t_symbol* send = x->s_empty;
    t_symbol* receive = x->s_empty;
    char buf[80];
    if ( x->send_set ) {
        atom_string(creation + 3, buf, 80);
        send = gensym(buf);
	}
	
    if ( x->receive_set ) {
		atom_string(creation + 4, buf, 80);
		receive = gensym(buf);
	}
    
	DEBUG(post("send: %s receive: %s",send->s_name,receive->s_name);)
	
    binbuf_addv(b, "ssiisiississss", gensym("#X"), gensym("obj"),
                x->x_obj.te_xpix, x->x_obj.te_ypix ,  
                atom_getsymbol(creation),
                x->x_width, x->x_height, send,receive,x->saveitems,
                x->bg_color, x->fg_color,x->hi_color,x->co_color);
                
	// Loop for menu items
	int i;
	if ( x->saveitems) {
		for(i=0 ; i<x->x_num_options ; i++)
		{
			DEBUG(post("saving option: %s",x->x_options[i]->s_name);)
			binbuf_addv(b, "s", x->x_options[i]);
		}
	}	
    binbuf_addv(b, ";");
    
    DEBUG(post("save end");)
}
*/

static void pmenu_clear(t_pmenu* x) {
	x->options_count = 0;
	x->current_selection = -1;
	pmenu_w_clear(x);
}

/*
static void pmenu_size(t_pmenu* x,t_symbol *s, int argc, t_atom *argv) {
	
	if (argc>2) argc =2;
	switch (argc) {
		case 2: if ( (argv+1)->a_type == A_FLOAT) x->x_height = atom_getfloat(argv+1);
		case 1: if ( argv->a_type == A_FLOAT) x->x_width = atom_getfloat(argv);
		break;
	}
	
	if ( x->x_width < 10) x->x_width = 10;
	if ( x->x_height < 10) x->x_height = 10;
	
	if ( pmenu_w_is_visible(x) ) {
		pmenu_w_resize(x);
	}
}
*/


static void pmenu_add(t_pmenu* x, t_symbol *s, int argc, t_atom *argv) {
	/*
	 x->av = resizebytes(x->av, x->mem_size * sizeof(*(x->av)), 
	 (10 + x->ac) * sizeof(*(x->av)));
	 x->mem_size = 10 + x->ac;
	 */
	
	// resize the options-array if it is too small
	if((argc + x->options_count) > x->options_memory){
          
          x->options = resizebytes( x->options, x->options_memory*sizeof(*(x->options)), 
				(argc + x->options_count+10)*sizeof(*(x->options)));
          x->options_memory=argc + x->options_count+10;
    }
	
	int i;
	t_symbol* label;
	for  ( i=0;i<argc;i++) {
		// Copy atom
		*(x->options+x->options_count) = *(argv+i);
		
		if ((argv+i)->a_type==A_SYMBOL) {
			
		    label = atom_getsymbol(argv+i);
		   
		} else {
		
				atom_string(argv+i, pmenu_buffer, MAXPDSTRING);
				label = gensym(pmenu_buffer);
			
		}
		DEBUG(post("adding option: %s",label->s_name);)
		pmenu_w_additem( x,label,x->options_count);
		x->options_count = x->options_count + 1;
		
	}
	
    
}



// function to change the colors

static void pmenu_colors(t_pmenu* x, t_symbol* s, int argc, t_atom* argv)
{
    
	DEBUG(post("bgcolour start");)
    
    if ( argc && argv->a_type==A_SYMBOL && atom_getsymbol(argv)==gensym("default")) {
		x->bg_color = gensym("grey90");
		x->fg_color = gensym("black");
		x->hi_color = gensym("grey95");
		x->co_color = gensym("black");
	} else {
    
		if (argc > 4) argc = 4;
		switch (argc) {
			case 4: if ((argv+3)->a_type==A_SYMBOL) x->co_color = atom_getsymbol(argv+3);
			case 3: if ((argv+2)->a_type==A_SYMBOL) x->hi_color = atom_getsymbol(argv+2);
			case 2: if ((argv+1)->a_type==A_SYMBOL) x->fg_color = atom_getsymbol(argv+1);
			case 1: if ((argv)->a_type==A_SYMBOL) x->bg_color = atom_getsymbol(argv);
			break;
		}
    }
    
    pmenu_w_apply_colors(x);
	
}


static int pmenu_set_float(t_pmenu* x, t_floatarg item) {
	
	    
	int i=(int)item;
	if( (i < x->options_count) && (i >= 0)) {
		x->current_selection = i;
		//if(pmenu_w_is_visible(x)) pmenu_w_text(x,x->x_options[x->current_selection]);
		pmenu_w_activate(x);
         return 1;
	} else {
		x->current_selection = -1;
		pmenu_w_activate(x);
        return 0;
   }
	
}

/* Function to select a menu option by inlet */
static void pmenu_float(t_pmenu* x, t_floatarg item)
{
	DEBUG(post("iselect start");)
        
	if ( pmenu_set_float(x,item) ) {
		pmenu_output(x);
	} else {
		
		//pd_error(x,"pmenu: expecting value between 0 and %i",x->x_num_options);
	}

	DEBUG(post("iselect end");)
}

static int pmenu_set_symbol(t_pmenu* x, t_symbol *s) {
	

	int i;
        
	/* Compare inlet symbol to each option */
	for(i=0; i < x->options_count; i++) {
	  if( (x->options+i)->a_type==A_SYMBOL && atom_getsymbol(x->options+i)->s_name == s->s_name) {
		    x->current_selection = i;
			//if(pmenu_w_is_visible(x)) pmenu_w_text(x,s);
            pmenu_w_activate(x);
	        return 1;
       }
	}
	x->current_selection = -1;
	pmenu_w_activate(x);
	return 0;
}


/* Function to choose value via symbol name */
static void pmenu_symbol(t_pmenu* x, t_symbol *s)
{
	if(pmenu_set_symbol(x,s)) {
	   pmenu_output(x);
  } else {
	   post("pmenu: '%s' is not an available option.", s->s_name);
  }
	
}

/* Function to choose value via name/index but without outputting it*/
static void pmenu_set(t_pmenu* x, t_symbol *S, int argc, t_atom*argv)
{
  if(!argc)return;

  if(argv->a_type==A_FLOAT) {
      pmenu_set_float(x,atom_getfloat(argv));
    } else if(argv->a_type==A_SYMBOL) {
		pmenu_set_symbol(x,atom_getsymbol(argv));
     } else {
      pd_error(x, "pmenu: can only 'set' symbols or floats");
    }
}






static t_class *pmenu_class;

/*
static void pmenu_receive(t_pmenu* x, t_symbol* s) {
	if ( x->receive != x->s_empty ) pd_unbind(&x->x_obj.ob_pd, x->receive);
	if ( s == x->s_empty || s ==  x->s_) {
		x->receive = x->s_empty;
	} else {
		x->receive = s;
		pd_bind(&x->x_obj.ob_pd, x->receive);
	}
	
}

static void pmenu_send(t_pmenu* x, t_symbol* s) {

	if ( s == x->s_empty || s ==  x->s_ ) {
		x->send = x->s_empty;
	} else {
		x->send = s;
	}
	
	
	
}
*/

static void pmenu_free(t_pmenu*x)
{
	pmenu_w_menu(x,DESTROY);

	
   freebytes(x->options, x->options_memory * sizeof(*(x->options)));
	
   pd_unbind(&x->x_obj.ob_pd, x->callback);
   
   //if ( x->receive != x->s_empty ) pd_unbind(&x->x_obj.ob_pd, x->receive);
}
/*
static void pmenu_saveitems( t_pmenu* x, t_float f) {
	x->saveitems = (f != 0);
}
*/

static void *pmenu_new(t_symbol *s, int argc, t_atom *argv)
{
    DEBUG(post("pmenu new start");)

    t_pmenu *x = (t_pmenu *)pd_new(pmenu_class);
    int i;
	


    x->current_selection = -1;

    x->options_memory=10;
    x->options=getbytes(sizeof(*(x->options)) * x->options_memory);
	//x->av = getbytes(x->mem_size * sizeof(*(x->av)));
 
    x->options_count = 0 ;
  
    x->indicator = 1;
    x->focusing = 1;
  
    
    // These should match the default colors in pmenu_colors
    x->bg_color = gensym("grey90");
    x->fg_color = gensym("black");
    x->hi_color = gensym("grey95");
    x->co_color = gensym("black");
      
	
    if (argc > 6) argc = 6;
    switch(argc){ 
    case 6: if ((argv+5)->a_type==A_SYMBOL) x->co_color = atom_getsymbol(argv+5);	
	case 5: if ((argv+4)->a_type==A_SYMBOL) x->hi_color = atom_getsymbol(argv+4);	
	case 4: if ((argv+3)->a_type==A_SYMBOL) x->fg_color = atom_getsymbol(argv+3);	
	case 3: if ((argv+2)->a_type==A_SYMBOL) x->bg_color = atom_getsymbol(argv+2);
	case 2: if ((argv+1)->a_type==A_FLOAT) x->focusing = atom_getfloat(argv+1);
	case 1: if ((argv)->a_type==A_FLOAT) x->indicator = atom_getfloat(argv);
      break;
    }

      /* Bind the recieve "pmenu%p" to the widget outlet*/
      sprintf(pmenu_buffer,"pmenu%p",x);
      x->callback = gensym(pmenu_buffer);
      pd_bind(&x->x_obj.ob_pd, x->callback);

      /* define proc in tcl/tk where "pmenu%p" is the receive, "callback" is the method, and "$index" is an argument. */
    sys_vgui("proc select%x {index} {\n pd [concat pmenu%p callback $index \\;]\n }\n",x,x); 
    

    x->outlet1 = outlet_new(&x->x_obj, &s_float);
	x->outlet2 = outlet_new(&x->x_obj, &s_list);

   
   pmenu_w_menu(x,CREATE);
   pmenu_w_apply_colors(x);
   //if (argc > 5) pmenu_add(x,&s_list,argc-5,argv+5);


DEBUG(post("pmenu new end");)

    return (x);
}

void pmenu_setup(void) {

    DEBUG(post("setup start");)

      pmenu_class = class_new(gensym("pmenu"), (t_newmethod)pmenu_new, (t_method)pmenu_free,
				sizeof(t_pmenu),0,A_GIMME,0);
				
	class_addbang(pmenu_class,	(t_method)pmenu_w_pop);
				
	class_addmethod(pmenu_class, (t_method)pmenu_callback,
								  gensym("callback"),A_DEFFLOAT,0);
	
	class_addmethod(pmenu_class, (t_method)pmenu_add,
								  gensym("add"), A_GIMME,0);
	
	class_addmethod(pmenu_class, (t_method)pmenu_clear,
								  gensym("clear"), 0);
						  
	class_addmethod(pmenu_class, (t_method)pmenu_colors,
								  gensym("colors"),A_GIMME,0);
	
        class_addmethod(pmenu_class, (t_method)pmenu_set,
                                    gensym("set"),A_GIMME,0);
	
	class_addsymbol(pmenu_class, (t_method)pmenu_symbol);

	class_addfloat(pmenu_class, (t_method)pmenu_float);



	post("pmenu v0.3 by tof");
}


