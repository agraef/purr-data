/* jack utility externals for linux pd
 * copyright 2003 Gerard van Dongen gml@xs4all.nl

* This program is free software; you can redistribute it and/or               
* modify it under the terms of the GNU General Public License                 
* as published by the Free Software Foundation; either version 2              
* of the License, or (at your option) any later version.                      
* This program is distributed in the hope that it will be useful,             
* but WITHOUT ANY WARRANTY; without even the implied warranty of              
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               
* GNU General Public License for more details.                                
* You should have received a copy of the GNU General Public License           
* along with this program; if not, write to the Free Software                 
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. 



* objects:  
* jack-connect
* this can query and set the port connections on the jack system
* methods:
* jack-ports
* this can query jack ports with regex's




 */




#include "m_pd.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <jack/jack.h>


static t_class *jackports_class;
static t_class *jackconnect_class;

typedef struct _jackports {
	t_object x_obj;
	t_outlet *input_ports, *output_ports;
	char expression[128];
	char *buffer; //used internally it doesn't have to be reserved every time
	t_atom *a_outlist;

} t_jackports;

typedef struct _jackconnect {
	t_object x_obj;
	t_symbol *input_client,*input_port, *output_client,*output_port;
	char source[128],destination[128]; //ought to be enough for most names
	int connected;
} t_jackconnect;

static jack_client_t * jc;



/********************************************************************************************************

methods for jack-ports

*********************************************************************************************************/

void jackports_input(t_jackports *x, t_symbol *s,int argc, t_atom *argv)
{
	if (jc){
	const char ** ports;

	int l = 0;
	int n = 0;
	int keyflag = 0;
	int expflag =0;
	int portflags = 0;
	t_symbol *s_client;
	t_symbol *s_port;
	char *t;

	if (!strcmp(s->s_name,"bang")) {
		strcpy(x->expression,"");
		expflag = 1;
	} else {

		//parse symbol s and all arguments for keywords:
		//physical,virtual,input and output

		if (!strcmp(s->s_name,"physical")) {
			portflags = portflags | JackPortIsPhysical;
			keyflag = 1;
		}
		if (!strcmp(s->s_name,"virtual")) {
			portflags = portflags & (~JackPortIsPhysical);
			keyflag = 1;
		}		
		if (!strcmp(s->s_name,"input")) {
			portflags = portflags | JackPortIsInput;
			keyflag = 1;
		}

		if (!strcmp(s->s_name,"output")) {
			portflags = portflags | JackPortIsOutput;
			keyflag = 1;
		}
		if (!keyflag) {
			strcpy(x->expression,s->s_name);
			expflag = 1;
		}
		for (n=0;n<argc;n++) {
			keyflag = 0;
			atom_string(argv+n,x->buffer,128);
			if (!strcmp(x->buffer,"physical")) {
				portflags = portflags | JackPortIsPhysical;
				keyflag = 1;
			}
			if (!strcmp(x->buffer,"virtual")) {
				portflags = portflags & (~JackPortIsPhysical);
				keyflag = 1;
			}		
			if (!strcmp(x->buffer,"input")) {
				portflags = portflags | JackPortIsInput;
				keyflag = 1;
			}
			
			if (!strcmp(x->buffer,"output")) {
				portflags = portflags | JackPortIsOutput;
				keyflag = 1;
			}
			if (!keyflag && !expflag) {
				strcpy(x->expression,x->buffer);
				expflag = 1;
			}


		}


	}


	ports = jack_get_ports (jc,x->expression,NULL,portflags|JackPortIsOutput);
	n=0;
	if (ports) {
		while (ports[n]) {
			//seperate port and client
			
			l = strlen(ports[n]);
			t = strchr(ports[n],':');

			if (t) {
				s_port=gensym(strchr(ports[n],':')+1);
			

				snprintf(x->buffer,l-strlen(s_port->s_name),ports[n]);
				s_client = gensym(x->buffer);

				SETSYMBOL(x->a_outlist,s_client);			
				SETSYMBOL(x->a_outlist+1,s_port);

				// output in output-outlet
				outlet_list(x->output_ports,&s_list,2,x->a_outlist);
			}

			n++;
		}
	}
	free(ports);

	ports = jack_get_ports (jc,x->expression,NULL,portflags|JackPortIsInput);
	n=0;
	if (ports) {
		while (ports[n]) {
			l = strlen(ports[n]);
			t = strchr(ports[n],':');

			if (t) {
				s_port=gensym(strchr(ports[n],':')+1);
			

				snprintf(x->buffer,l-strlen(s_port->s_name),ports[n]);
				s_client = gensym(x->buffer);

				SETSYMBOL(x->a_outlist,s_client);			
				SETSYMBOL(x->a_outlist+1,s_port);

				// output in output-outlet
				outlet_list(x->input_ports,&s_list,2,x->a_outlist);
			}
		
			
			n++;
		}
	}
	free(ports);

	strcpy(x->expression,"");//reset regex
	}

}




void *jackports_new(void)
{
	t_jackports * x = (t_jackports *)pd_new(jackports_class);
	x->output_ports = outlet_new(&x->x_obj, &s_list);
	x->input_ports = outlet_new(&x->x_obj, &s_list);
	x->a_outlist = getbytes(3 * sizeof(t_atom));
	x->buffer = getbytes(128);

	return (void*)x;
}



/********************************************************************************************************

methods for jack-connect

*********************************************************************************************************/
void jackconnect_getnames(t_jackconnect *x)
{
	char* to = x->source;
	to = (char*)stpcpy( to,x->output_client->s_name);
	to = (char*)stpcpy(to,":");
	to = (char*)stpcpy(to,x->output_port->s_name);
	to = x->destination;
	to = (char*)stpcpy(to,x->input_client->s_name);
	to = (char*)stpcpy(to,":");
	to = (char*)stpcpy(to,x->input_port->s_name);

}
void jackconnect_connect(t_jackconnect *x)
{

	if (jc) {

		jackconnect_getnames(x);
		post("connecting   %s with %s",x->source,x->destination);
		if (!jack_connect(jc,x->source,x->destination)) {
			x->connected = 1;
			outlet_float(x->x_obj.ob_outlet,x->connected);
		}
	}

}

void jackconnect_disconnect(t_jackconnect *x)
{
	if (jc) {

		jackconnect_getnames(x);
		if (!jack_disconnect(jc,x->source,x->destination)) {
			x->connected = 0;
			outlet_float(x->x_obj.ob_outlet,x->connected);
		}
		post("disconnecting   %s with %s",x->source,x->destination);
	}

}

void jackconnect_toggle(t_jackconnect *x)
{
	if (jc) {

		jackconnect_getnames(x);
		post("toggling connection   %s with %s",x->source,x->destination);
		if (jack_disconnect(jc,x->source,x->destination)) {
			jack_connect(jc,x->source,x->destination);
			x->connected = 1;
		} else {
			x->connected = 0;
		}
		outlet_float(x->x_obj.ob_outlet,x->connected);
	}
}


void jackconnect_query(t_jackconnect *x)
{
	if (jc) {

		const char **ports;
		int n=0;
		jackconnect_getnames(x);
		
		post("querying connection   %s with %s",x->source,x->destination);
		
		ports = jack_port_get_all_connections(jc,(jack_port_t *)jack_port_by_name(jc,x->source));
		x->connected = 0;
		
		if(ports){
			while (ports[n]){
				post("n = %i",n);
				if (!strcmp(ports[n],x->destination)){
					x->connected = 1;
					break;
				}
				n++;
				
			}
			free(ports);
		}
		outlet_float(x->x_obj.ob_outlet,x->connected);
	}



}







void *jackconnect_new(void)
{
	t_jackconnect * x = (t_jackconnect *)pd_new(jackconnect_class);

	outlet_new(&x->x_obj, &s_float);
	symbolinlet_new(&x->x_obj,&x->output_client);
	symbolinlet_new(&x->x_obj,&x->output_port);
	symbolinlet_new(&x->x_obj,&x->input_client);
	symbolinlet_new(&x->x_obj,&x->input_port);

	/* to prevent segfaults put default names in the client/port variables */
	x->input_client = gensym("none");
	x->input_port = gensym("none");
	x->output_client =  gensym("none");
	x->output_port =  gensym("none");
	x->connected = 0;
	jackconnect_getnames(x);

	return (void*)x;
}


/********************************************************************************************************

setup for jack-connect and jack-ports

*********************************************************************************************************/



void jackx_setup(void)
{

	post("//////////////////////////////////////////\n"
	     "/////Jack utility external library///////\n"
	     "////Gerard van Dongen, gml@xs4all.nl////\n"
	     "///testing for jack////////////////////\n"
	     "//////////////////////////////////////");


	jc = jack_client_new("jacky-x");




        /*jack ports setup */

	jackports_class = class_new(gensym("jack-ports"),(t_newmethod)jackports_new,0,sizeof(t_jackports),CLASS_DEFAULT,0);




	class_addanything(jackports_class,jackports_input);

	class_sethelpsymbol(jackports_class,gensym("jack-ports"));

	/*jack-connect setup */



	jackconnect_class = class_new(gensym("jack-connect"),(t_newmethod)jackconnect_new,0,sizeof(t_jackconnect),CLASS_DEFAULT,0);

	class_addmethod(jackconnect_class, (t_method)jackconnect_connect, gensym("connect"),0);
	class_addmethod(jackconnect_class, (t_method)jackconnect_disconnect, gensym("disconnect"),0);
	class_addmethod(jackconnect_class, (t_method)jackconnect_toggle, gensym("toggle"),0);
	class_addmethod(jackconnect_class, (t_method)jackconnect_query, gensym("query"),0);
	class_addbang(jackconnect_class, (t_method)jackconnect_toggle);
	class_sethelpsymbol(jackports_class,gensym("jack-connect"));
}




