/* --------------------------------------------------------------------------*/
/*                                                                           */
/* interface to native HID (Human Interface Devices) API                     */
/* Written by Hans-Christoph Steiner <hans@at.or.at>                         */
/* Max/MSP port by Olaf Matthes <olaf.matthes@gmx.de>                        */
/*                                                                           */
/* Copyright (c) 2004-2006 Hans-Christoph Steiner                            */
/*                                                                           */
/* This program is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU General Public License               */
/* as published by the Free Software Foundation; either version 2            */
/* of the License, or (at your option) any later version.                    */
/*                                                                           */
/* See file LICENSE for further informations on licensing terms.             */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software Foundation,   */
/* Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.           */
/*                                                                           */
/* --------------------------------------------------------------------------*/

#ifdef _WIN32
/* any Windows specific includes go in here */
#ifdef PD
#include <windows.h>
#endif /* PD */
#else
#include <unistd.h>
#include <ctype.h>
#endif /* _WIN32 */
#include <stdarg.h>
#include <string.h>

#include "hidio.h"

/*------------------------------------------------------------------------------
 * LOCAL DEFINES
 */

#define DEBUG(x)
//#define DEBUG(x) x 

unsigned short global_debug_level = 0; /* high numbers means more messages */

/*------------------------------------------------------------------------------
 *  GLOBAL VARIABLES
 */

/* count the number of instances of this object so that certain free()
 * functions can be called only after the final instance is detroyed.
 */
t_int hidio_instance_count;

/* this is used to test for the first instance to execute */
double last_execute_time[MAX_DEVICES];

static t_class *hidio_class;

/* mostly for status querying */
unsigned short device_count;

/* store element structs to eliminate symbol table lookups, etc. */
t_hid_element *element[MAX_DEVICES][MAX_ELEMENTS];
/* number of active elements per device */
unsigned short element_count[MAX_DEVICES]; 

/* pre-generated symbols */
t_symbol *ps_open, *ps_device, *ps_poll, *ps_total, *ps_range;
t_symbol *ps_absolute, *ps_button, *ps_key, *ps_led, *ps_pid, *ps_relative;
t_symbol *absolute_symbols[ABSOLUTE_ARRAY_MAX];
t_symbol *button_symbols[BUTTON_ARRAY_MAX];
t_symbol *key_symbols[KEY_ARRAY_MAX];
t_symbol *led_symbols[LED_ARRAY_MAX];
t_symbol *pid_symbols[PID_ARRAY_MAX];
t_symbol *relative_symbols[RELATIVE_ARRAY_MAX];

/* TODO consider issuing a pd_error if more than one instance is attached to
 * one given device */

/*------------------------------------------------------------------------------
 * FUNCTION PROTOTYPES
 */

//static void hidio_poll(t_hidio *x, t_float f);
static void hidio_open(t_hidio *x, t_symbol *s, int argc, t_atom *argv);
//static void hidio_close(t_hidio *x);
//static void hidio_float(t_hidio* x, t_floatarg f);


/*------------------------------------------------------------------------------
 * SUPPORT FUNCTIONS
 */

void debug_post(t_int message_debug_level, const char *fmt, ...)
{
    if(message_debug_level <= global_debug_level)
    {
        char buf[MAXPDSTRING];
        va_list ap;
        //t_int arg[8];
        va_start(ap, fmt);
        vsnprintf(buf, MAXPDSTRING-1, fmt, ap);
        post(buf);
        va_end(ap);

    }
}

void debug_error(t_hidio *x, t_int message_debug_level, const char *fmt, ...)
{
    if(message_debug_level <= global_debug_level)
    {
        char buf[MAXPDSTRING];
        va_list ap;
        //t_int arg[8];
        va_start(ap, fmt);
        vsnprintf(buf, MAXPDSTRING-1, fmt, ap);
        pd_error(x, buf);
        va_end(ap);
    }
}


static void output_status(t_hidio *x, t_symbol *selector, t_float output_value)
{
    t_atom *output_atom = (t_atom *)getbytes(sizeof(t_atom));
#ifdef PD
    SETFLOAT(output_atom, output_value);
#else /* Max */
    atom_setlong(output_atom, (long)output_value);
#endif /* PD */
    outlet_anything( x->x_status_outlet, selector, 1, output_atom);
    freebytes(output_atom,sizeof(t_atom));
}

static void output_open_status(t_hidio *x)
{
    output_status(x, ps_open, x->x_device_open);
}

static void output_device_number(t_hidio *x)
{
    output_status(x, ps_device, x->x_device_number);
}

static void output_poll_time(t_hidio *x)
{
    output_status(x, ps_poll, x->x_delay);
}

static void output_device_count(t_hidio *x)
{
    output_status(x, ps_total, device_count);
}

static void output_element_ranges(t_hidio *x)
{
    if( (x->x_device_number > -1) && (x->x_device_open) )
    {
        unsigned int i;
        t_atom output_data[4];
        
        for(i=0;i<element_count[x->x_device_number];++i)
        {
#ifdef PD
            SETSYMBOL(output_data, element[x->x_device_number][i]->type);
            SETSYMBOL(output_data + 1, element[x->x_device_number][i]->name);
            SETFLOAT(output_data + 2, element[x->x_device_number][i]->min);
            SETFLOAT(output_data + 3, element[x->x_device_number][i]->max);
#else
            atom_setsym(output_data, element[x->x_device_number][i]->type);
            atom_setsym(output_data + 1, element[x->x_device_number][i]->name);
            atom_setlong(output_data + 2, element[x->x_device_number][i]->min);
            atom_setlong(output_data + 3, element[x->x_device_number][i]->max);
#endif /* PD */
            outlet_anything(x->x_status_outlet, ps_range, 4, output_data);
        }
    }
}


static unsigned int name_to_usage(char *usage_name)
{ // output usagepage << 16 + usage
    if(strcmp(usage_name,"pointer") == 0)   return 0x00010001;
    if(strcmp(usage_name,"mouse") == 0)     return 0x00010002;
    if(strcmp(usage_name,"joystick") == 0)  return 0x00010004;
    if(strcmp(usage_name,"gamepad") == 0)   return 0x00010005;
    if(strcmp(usage_name,"keyboard") == 0)  return 0x00010006;
    if(strcmp(usage_name,"keypad") == 0)    return 0x00010007;
    if(strcmp(usage_name,"multiaxiscontroller") == 0) return 0x00010008;
    return 0;
}


static t_int convert_symbol_hex_to_int(t_symbol *s)
{
    if(strncmp(s->s_name, "0x", 2) == 0)
        return (t_int) strtoul(s->s_name, NULL, 16);
    else
        return -1;
}


static short get_device_number_from_arguments(int argc, t_atom *argv)
{
#ifdef PD
    short device_number = -1;
    char device_type_string[MAXPDSTRING] = "";
    unsigned short device_type_instance;
#else
    long device_number = -1;
    char *device_type_string;
    long device_type_instance;
#endif /* PD */
    unsigned int usage;
    unsigned short vendor_id;
    unsigned short product_id;
    t_symbol *first_argument;
    t_symbol *second_argument;

    if(argc == 1)
    {
#ifdef PD
        first_argument = atom_getsymbolarg(0,argc,argv);
        if(first_argument == &s_) 
#else
        atom_arg_getsym(&first_argument, 0,argc,argv);
        if(first_argument == _sym_nothing) 
#endif /* PD */
        { // single float arg means device #
#ifdef PD
            device_number = (short) atom_getfloatarg(0,argc,argv);
#else
            atom_arg_getlong(&device_number, 0, argc, argv);
#endif /* PD */
            if(device_number < 0) device_number = -1;
            debug_post(LOG_DEBUG,"[hidio] setting device# to %d",device_number);
        }
        else
        { // single symbol arg means first instance of a device type
#ifdef PD
            atom_string(argv, device_type_string, MAXPDSTRING-1);
#else
            device_type_string = atom_string(argv);
            // LATER do we have to free this string manually???
#endif /* PD */
            usage = name_to_usage(device_type_string);
            device_number = get_device_number_from_usage(0, usage >> 16, 
                                                         usage & 0xffff);
            debug_post(LOG_INFO,"[hidio] using 0x%04x 0x%04x for %s",
                        usage >> 16, usage & 0xffff, device_type_string);
        }
    }
    else if(argc == 2)
    { 
#ifdef PD
        first_argument = atom_getsymbolarg(0,argc,argv);
        second_argument = atom_getsymbolarg(1,argc,argv);
        if( second_argument == &s_ ) 
#else
        atom_arg_getsym(&first_argument, 0,argc,argv);
        atom_arg_getsym(&second_argument, 1,argc,argv);
        if( second_argument == _sym_nothing ) 
#endif /* PD */
        { /* a symbol then a float means match on usage */
#ifdef PD
            atom_string(argv, device_type_string, MAXPDSTRING-1);
            usage = name_to_usage(device_type_string);
            device_type_instance = atom_getfloatarg(1,argc,argv);
#else
            device_type_string = atom_string(argv);
            usage = name_to_usage(device_type_string);
            atom_arg_getlong(&device_type_instance, 1, argc, argv);
#endif /* PD */
            debug_post(LOG_DEBUG,"[hidio] looking for %s at #%d",
                        device_type_string, device_type_instance);
            device_number = get_device_number_from_usage(device_type_instance,
                                                              usage >> 16, 
                                                              usage & 0xffff);
        }
        else
        { /* two symbols means idVendor and idProduct in hex */
            vendor_id = 
                (unsigned short) strtol(first_argument->s_name, NULL, 16);
            product_id = 
                (unsigned short) strtol(second_argument->s_name, NULL, 16);
            device_number = get_device_number_by_id(vendor_id,product_id);
        }
    }
    return device_number;
}


/* output_message[3] is pre-generated by hidio_build_element_list() and
 * stored in t_hid_element, then just the value is updated.  This saves a bit
 * of CPU time since this is run for every event that is output. */
void hidio_output_event(t_hidio *x, t_hid_element *output_element)
{
/*        debug_post(LOG_DEBUG,"hidio_output_event: instance %d/%d last: %llu", 
                   x->x_instance+1, hidio_instance_count,
                   last_execute_time[x->x_device_number]);*/
#ifdef PD
        SETFLOAT(output_element->output_message + 2, output_element->value);
#else /* Max */
        atom_setlong(output_element->output_message + 2, (long)output_element->value);
#endif /* PD */
        outlet_anything(x->x_data_outlet, output_element->type, 3, 
                        output_element->output_message);
}

void hidio_write_event(t_hidio *x, t_symbol *s, int argc, t_atom *argv)
{
    debug_post(LOG_DEBUG,"hidio_write_event");
    t_symbol *first_argument;
    t_symbol *second_argument;
    t_int first_argument_int;
    t_int second_argument_int;
    
/* TODO add symbol for value to allow for hex values.  This would be useful
 * for sending values greater than Pd's t_float can handle */
    if(argc != 4) 
    {
        pd_error(x, "[hidio] write message must have exactly 4 atoms");
        return;
    }

    first_argument = atom_getsymbolarg(0,argc,argv);
    if(first_argument == &s_) 
    { // first float arg means all float message
        debug_post(LOG_DEBUG,"first_argument == &s_");
        hidio_write_event_ints(x, atom_getintarg(0,argc,argv), atom_getintarg(1,argc,argv),
                               atom_getintarg(2,argc,argv), atom_getintarg(3,argc,argv));
    }
    else
    {
        // if the symbol is a hex number, convert it to a t_int
        first_argument_int = convert_symbol_hex_to_int(first_argument);
        second_argument = atom_getsymbolarg(1,argc,argv);
        if((second_argument == &s_) && (first_argument_int > -1))
        { // symbol page and float usage
            debug_post(LOG_DEBUG,"second_argument == &s_");
            hidio_write_event_symbol_int(x, first_argument, 
                                         atom_getintarg(1,argc,argv),
                                         atom_getintarg(2,argc,argv), 
                                         atom_getintarg(3,argc,argv));
        }
        else
        { // symbol page and usage 
            // if the symbol is a hex number, convert it to a t_int
            if( (first_argument->s_name[0] == '0') && (first_argument->s_name[1] == 'x') )
                first_argument_int = (t_int) strtoul(first_argument->s_name, 0, 16);
            hidio_write_event_symbols(x, first_argument, second_argument,
                                      atom_getintarg(2,argc,argv), atom_getintarg(3,argc,argv));
        }
    }
}

/* stop polling the device */
static void hidio_stop_poll(t_hidio* x) 
{
  debug_post(LOG_DEBUG,"hidio_stop_poll");
  
  if (x->x_started) 
  { 
      clock_unset(x->x_clock);
      debug_post(LOG_INFO,"[hidio] polling stopped");
      x->x_started = 0;
  }
}

/*------------------------------------------------------------------------------
 * METHODS FOR [hidio]'s MESSAGES                    
 */

/* TODO: poll time should be set based on how fast the OS is actually polling
 * the device, whether that is IOUSBEndpointDescriptor.bInterval, or something
 * else.
 */
void hidio_poll(t_hidio* x, t_float f) 
{
    debug_post(LOG_DEBUG,"hidio_poll");
  
/*    if the user sets the delay less than 2, set to block size */
    if( f > 2 )
        x->x_delay = (t_int)f;
    else if( f > 0 ) //TODO make this the actual time between message processing
        x->x_delay = 1.54; 
    if(x->x_device_number > -1) 
    {
        if(!x->x_device_open)
        {
            hidio_open(x,ps_open,0,NULL);
        }
        if(!x->x_started) 
        {
            clock_delay(x->x_clock, x->x_delay);
            debug_post(LOG_DEBUG,"[hidio] polling started");
            x->x_started = 1;
        } 
    }
}

static void hidio_set_from_float(t_hidio *x, t_floatarg f)
{
/* values greater than 1 set the polling delay time */
/* 1 and 0 for start/stop so you can use a [tgl] */
    if(f > 1)
    {
        x->x_delay = (t_int)f;
        hidio_poll(x,f);
    }
    else if(f == 1) 
    {
        if(! x->x_started)
        {
            hidio_poll(x,f);
        }
    }
    else if(f == 0)         
    {
        hidio_stop_poll(x);
    }
}

/* close the device */
static void hidio_close(t_hidio *x) 
{
    debug_post(LOG_DEBUG,"hidio_close");

 /* just to be safe, stop it first */
     hidio_stop_poll(x);

     if(hidio_close_device(x) != 0)
         debug_error(x, LOG_ERR,"[hidio] error closing device %d",x->x_device_number);
     debug_post(LOG_DEBUG,"[hidio] closed device %d",x->x_device_number);
     x->x_device_open = 0;
     output_open_status(x);
}


/* hidio_open behavoir
 * current state                 action
 * ---------------------------------------
 * closed / same device          open 
 * open / same device            no action 
 * closed / different device     open 
 * open / different device       close, open 
 */
static void hidio_open(t_hidio *x, t_symbol *s, int argc, t_atom *argv) 
{
    short new_device_number = get_device_number_from_arguments(argc, argv);
    t_int started = x->x_started; // store state to restore after device is opened
    debug_post(LOG_DEBUG,"hid_%s",s->s_name);
    
    if (new_device_number > -1)
    {
        /* check whether we have to close previous device */
        if (x->x_device_open && new_device_number != x->x_device_number)
        {
            hidio_close(x);
        }
        /* no device open, so open one now */
        if (!x->x_device_open)
        {
            if(hidio_open_device(x, new_device_number) == EXIT_SUCCESS)
            {
                x->x_device_open = 1;
                x->x_device_number = new_device_number;
                /* restore the polling state so that when I [tgl] is used to
                 * start/stop [hidio], the [tgl]'s state will continue to
                 * accurately reflect [hidio]'s state  */
                if (started)
                    hidio_set_from_float(x,x->x_delay); // TODO is this useful?
                debug_post(LOG_DEBUG,"[hidio] set device# to %d",new_device_number);
                output_device_number(x);
            }
            else
            {
                x->x_device_number = -1;
                pd_error(x, "[hidio] can not open device %d",new_device_number);
            }
        }
    }
    else 
        debug_error(x, LOG_WARNING,"[hidio] device does not exist");
    /* always output open result so you can test for success in Pd space */
    output_open_status(x);
}


static void hidio_tick(t_hidio *x)
{
//    debug_post(LOG_DEBUG,"hidio_tick");
    t_hid_element *current_element;
    unsigned int i;
    double right_now;

#ifdef PD
    right_now = clock_getlogicaltime();
#else /* Max */
    clock_getftime(&right_now);
#endif /* PD */

//    debug_post(LOG_DEBUG,"# %u\tnow: %llu\tlast: %llu", x->x_device_number,
//                right_now, last_execute_time[x->x_device_number]);
    if(right_now > last_execute_time[x->x_device_number])
    {
        hidio_get_events(x);
        last_execute_time[x->x_device_number] = right_now;
/*        debug_post(LOG_DEBUG,"executing: instance %d/%d at %llu last: %llu", 
             x->x_instance+1, hidio_instance_count, right_now,
             last_execute_time[x->x_device_number]);*/
    }
    for(i=0; i< element_count[x->x_device_number]; ++i)
    {
        /* TODO: since relative events need to be output every time, they need
         * to be flagged when new relative events arrive.  Otherwise, it'll
         * just spam out relative events no matter if anything new has
         * arrived */
        current_element = element[x->x_device_number][i];
        if(current_element->previous_value != current_element->value)
        {
            hidio_output_event(x, current_element);
            if(!current_element->relative)
                current_element->previous_value = current_element->value;
        }
    }
    if (x->x_started) 
    {
        clock_delay(x->x_clock, x->x_delay);
    }
}

static void hidio_info(t_hidio *x)
{
    output_open_status(x);
    output_device_number(x);
    output_device_count(x);
    output_poll_time(x);
    output_element_ranges(x);
    hidio_platform_specific_info(x);
}

static void hidio_float(t_hidio* x, t_floatarg f) 
{
    debug_post(LOG_DEBUG,"hid_float");

    hidio_set_from_float(x,f);
}

#ifndef PD /* Max */
static void hidio_int(t_hidio* x, long l) 
{
    debug_post(LOG_DEBUG,"hid_int");

    hidio_set_from_float(x, (float)l);
}
#endif /* NOT PD */

static void hidio_debug(t_hidio *x, t_float f)
{
    debug_post(LOG_INFO,"[hidio] set global debug level to %d", (int)f);
    global_debug_level = f;
}


/*------------------------------------------------------------------------------
 * system functions 
 */
static void hidio_free(t_hidio* x) 
{
    debug_post(LOG_DEBUG,"hidio_free");

    hidio_close(x);
    clock_free(x->x_clock);
    hidio_instance_count--;

    hidio_platform_specific_free(x);
}

/* create a new instance of this class */
static void *hidio_new(t_symbol *s, int argc, t_atom *argv) 
{
    unsigned int i;
#ifdef PD
    t_hidio *x = (t_hidio *)pd_new(hidio_class);
    
    x->x_clock = clock_new(x, (t_method)hidio_tick);

    /* create anything outlet used for HID data */ 
    x->x_data_outlet = outlet_new(&x->x_obj, 0);
    x->x_status_outlet = outlet_new(&x->x_obj, 0);
#else /* Max */
    t_hidio *x = (t_hidio *)object_alloc(hidio_class);
    
    x->x_clock = clock_new(x, (method)hidio_tick);

    /* create anything outlet used for HID data */ 
    x->x_status_outlet = outlet_new(x, "anything");
    x->x_data_outlet = outlet_new(x, "anything");
#endif /* PD */

    /* init vars */
    x->x_device_open = 0;
    x->x_started = 0;
    x->x_delay = DEFAULT_DELAY;
    for(i=0; i<MAX_DEVICES; ++i) last_execute_time[i] = 0;
#ifdef _WIN32
    x->x_hid_device = hidio_platform_specific_new(x);
#endif
    x->x_device_number = get_device_number_from_arguments(argc, argv);
  
    x->x_instance = hidio_instance_count;
    hidio_instance_count++;

    return x;
}

#ifdef PD
void hidio_setup(void) 
{
    hidio_class = class_new(gensym("hidio"), 
                                 (t_newmethod)hidio_new, 
                                 (t_method)hidio_free,
                                 sizeof(t_hidio),
                                 CLASS_DEFAULT,
                                 A_GIMME,0);
    
    /* add inlet datatype methods */
    class_addfloat(hidio_class,(t_method) hidio_float);
    class_addbang(hidio_class,(t_method) hidio_tick);
/*     class_addanything(hidio_class,(t_method) hidio_anything); */
    
    /* add inlet message methods */
    class_addmethod(hidio_class,(t_method) hidio_debug,gensym("debug"),A_DEFFLOAT,0);
    class_addmethod(hidio_class,(t_method) hidio_build_device_list,gensym("refresh"),0);
/* TODO: [print( should be dumped for [devices( and [elements( messages */
    class_addmethod(hidio_class,(t_method) hidio_devices,gensym("devices"),0);
    class_addmethod(hidio_class,(t_method) hidio_elements,gensym("elements"),0);
    class_addmethod(hidio_class,(t_method) hidio_info,gensym("info"),0);
    class_addmethod(hidio_class,(t_method) hidio_open,gensym("open"),A_GIMME,0);
    class_addmethod(hidio_class,(t_method) hidio_close,gensym("close"),0);
    class_addmethod(hidio_class,(t_method) hidio_poll,gensym("poll"),A_DEFFLOAT,0);

/* test function for output support */
    class_addmethod(hidio_class,(t_method) hidio_write_event, gensym("write"), A_GIMME ,0);


    post("[hidio] %d.%d: © 2004-2008 by Hans-Christoph Steiner & Olaf Matthes",
         HIDIO_MAJOR_VERSION, HIDIO_MINOR_VERSION);  
    post("\tcompiled on "__DATE__" at "__TIME__ " ");
    
    /* pre-generate often used symbols */
    ps_open = gensym("open");
    ps_device = gensym("device");
    ps_poll = gensym("poll");
    ps_total = gensym("total");
    ps_range = gensym("range");

    generate_type_symbols();
    generate_event_symbols();
}
#else /* Max */
static void hidio_notify(t_hidio *x, t_symbol *s, t_symbol *msg, void *sender, void *data)
{
    if (msg == _sym_free)    // this message is sent when a child object is freeing
    {
        object_detach(gensym("_obex_hidio"), s, x);
        object_unregister(sender); 
    }
}

static void hidio_assist(t_hidio *x, void *b, long m, long a, char *s)
{
    if (m == 2)
    {
        switch (a)
        {    
        case 0:
            sprintf(s, "(list) Received Events");
            break;
        case 1:
            sprintf(s, "(list) Status Info");
            break;
        }
    }
    else
    {
        switch (a)
        {    
        case 0:
            sprintf(s, "Control Messages");
            break;
        case 1:
            sprintf(s, "nothing");
            break;
        }
    }
}

int main()
{
    t_class    *c;
    
    c = class_new("hidio", (method)hidio_new, (method)hidio_free, (short)sizeof(t_hidio), 
        0L, A_GIMME, 0);
    
    /* initialize the common symbols, since we want to use them */
    common_symbols_init();

    /* register the byte offset of obex with the class */
    class_obexoffset_set(c, calcoffset(t_hidio, x_obex));
    
    /* add methods to the class */
    class_addmethod(c, (method)hidio_int,             "int",             A_LONG, 0);  
    class_addmethod(c, (method)hidio_float,            "float",         A_FLOAT, 0);  
    class_addmethod(c, (method)hidio_tick,             "bang",             A_GIMME, 0); 
    
    /* add inlet message methods */
    class_addmethod(c, (method)hidio_debug, "debug",A_DEFFLOAT,0);
    class_addmethod(c, (method)hidio_build_device_list, "refresh",0);
/* TODO: [print( should be dumped for [devices( and [elements( messages */
    class_addmethod(c, (method)hidio_devices, "devices",0);
    class_addmethod(c, (method)hidio_elements, "elements",0);
    class_addmethod(c, (method)hidio_print, "print",0);
    class_addmethod(c, (method)hidio_info, "info",0);
    class_addmethod(c, (method)hidio_open, "open",A_GIMME,0);
    class_addmethod(c, (method)hidio_close, "close",0);
    class_addmethod(c, (method)hidio_poll, "poll",A_DEFFLOAT,0);
    /* perfomrance / system stuff */

    class_addmethod(c, (method)hidio_assist,         "assist",         A_CANT, 0);  

    /* add a notify method, so we get notifications from child objects */
    class_addmethod(c, (method)hidio_notify,        "notify",        A_CANT, 0); 
    // add methods for dumpout and quickref    
    class_addmethod(c, (method)object_obex_dumpout,        "dumpout",        A_CANT, 0); 
    class_addmethod(c, (method)object_obex_quickref,    "quickref",        A_CANT, 0);

    /* we want this class to instantiate inside of the Max UI; ergo CLASS_BOX */
    class_register(CLASS_BOX, c);
    hidio_class = c;

    finder_addclass("Devices", "hidio");
    post("hidio %d.%d: © 2004-2008 by Hans-Christoph Steiner & Olaf Matthes",
         HIDIO_MAJOR_VERSION, HIDIO_MINOR_VERSION);
    post("hidio: compiled on "__DATE__" at "__TIME__ " ");
    
    /* pre-generate often used symbols */
    ps_open = gensym("open");
    ps_device = gensym("device");
    ps_poll = gensym("poll");
    ps_total = gensym("total");
    ps_range = gensym("range");

    generate_type_symbols();
    generate_event_symbols();

    return EXIT_SUCCESS;
}
#endif /* PD */

