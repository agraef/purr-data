/* 
* 
* jack_transport Copyright (C) 2005 Tim Blechmann
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
*/

#include "m_pd.h"
#include "jack/jack.h"
#include "stdio.h"

static t_class *jack_transport_class;

typedef struct _jack_transport
{
    t_object x_obj;
	t_outlet * x_outlet;
	jack_client_t* x_jack_client;
	jack_position_t* x_pos;
} jack_transport_t;


/* connect to jack server */
static int jack_transport_connect(jack_transport_t * x)
{
	char port_name[80] = "";

	static int client = 0;
	do 
	{
		sprintf(port_name,"pure_data_jack_transport_%d",client);
		client++;
	} 
	while (((x->x_jack_client = jack_client_new (port_name)) == 0) &&
		   client < 100);
	client = 0;

	if (!x->x_jack_client)
	{
		post("jack_transport: can't connect to jack server");
		return 1;
	}

	post("jack_transport: connecting as %s", port_name);
	
	jack_activate(x->x_jack_client);
	
	return 0;
}


static jack_transport_t * jack_transport_new(void)
{
	int status = 0;
	jack_transport_t *x = (jack_transport_t*) pd_new(jack_transport_class);
	
	x->x_outlet = outlet_new(&x->x_obj, NULL);
	x->x_pos = (jack_position_t*) getbytes(sizeof(jack_position_t));
	
	status = jack_transport_connect(x);

	return x;
}

static void jack_transport_starter(jack_transport_t * x)
{
	jack_transport_start(x->x_jack_client);
	return;
}

static void jack_transport_stoper(jack_transport_t * x)
{
	jack_transport_stop(x->x_jack_client);
	return;
}

static void jack_transport_bang(jack_transport_t * x)
{
	float f;
	if (!x->x_jack_client)
		return;
	
	f = (float)jack_get_current_transport_frame(x->x_jack_client);
	
	outlet_float(x->x_outlet, f);
}

static void jack_transport_float(jack_transport_t * x, float f)
{
	if (!x->x_jack_client)
		return;
	
	jack_transport_locate(x->x_jack_client, (jack_nframes_t)f);
}


void jack_transport_setup(void)
{
	jack_transport_class = class_new(gensym("jack_transport"), 
									 (t_newmethod)jack_transport_new,
									 NULL, sizeof(jack_transport_t),
									 CLASS_DEFAULT, 0);
    class_addmethod(jack_transport_class, (t_method)jack_transport_starter,
					gensym("start"),0,0);
	class_addmethod(jack_transport_class, (t_method)jack_transport_stoper,
					gensym("stop"),0,0);
	class_addbang(jack_transport_class, (t_method)jack_transport_bang);
	class_addfloat(jack_transport_class, (t_method)jack_transport_float);

}
