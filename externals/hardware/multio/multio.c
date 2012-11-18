/*
multio

connects to multIO and listens..
left outlet for analog 
middle for digital

output is a list of 2 floats (channel, value)


*/
#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

#include "m_pd.h"
#include "usb.h"
#include "pthread.h"


#define DEFDELTIME 20 // time between readouts in msec
#define TIMEOUT 1000 // timeout time in usb interrupts reading and writing
#define MAXBUF 1024

/* PICkit USB values */
static const int multio_vendorID=0xdead; // Microchip, Inc
static const int multio_productID=0xbeef; // PICkit 1 FLASH starter kit
static const int multio_configuration=2; /* 1: HID; 2: vendor specific */
static const int multio_interface=0;
static const int reqLen=8;
static char is_open;
typedef unsigned char byte;

static t_class *multio_class;

typedef struct _multio
{
    t_object x_obj; // myself
	usb_dev_handle *d; // handle to multIO
	t_clock *x_clock; // as in metro
	double x_deltime; // as in metro
	int x_hit; // as in metro
	pthread_attr_t multio_thread_attr;
	pthread_t     x_threadid;
	unsigned char double_buffer[2][MAXBUF];	// a double buffer: thread writes one, cyclic read of the other one
						// the second parameter should be deafult 1000 but setable via
						// object parameters
	int  buf_count[2];				// how many bytes are in a buffer
	unsigned char whichbuf;				// which one to read from
	char old_digi[8]; // buffer of digital input, is a byte, 8 values at a time
	char digi_outs[8]; // buffer of digital input, is a byte, 8 values at a time
	int analog_buffer[64]; // buffered analog outs
	int x_verbose;
	t_outlet *a_out, *d_out, *s_out; // outlets
} t_multio;

static void *usb_read_thread(void *w)
{
	t_multio *x = (t_multio*) w;
	int cnt = 0;
	int bytesread = 0;
	unsigned char mybuf = 1;
	unsigned char buffer[8];
	while(1)	// never ending
	{
		pthread_testcancel();
		if(x->d)	// only read if the device is opened
		{
			if(x->buf_count[x->whichbuf] <= 0)		// check if the read buffer is empty
			{
			  mybuf = x->whichbuf;			// if so, use it for writing
			  x->whichbuf = !(x->whichbuf &1);		// and toggle the read buffer
			}
			bytesread = usb_interrupt_read(x->d, 0x81, buffer, 8, 1000);
			if(bytesread > 0)
			{
				if(x->buf_count[mybuf]+bytesread > MAXBUF)
					x->buf_count[mybuf] = 0;
				x->double_buffer[mybuf][x->buf_count[mybuf]++] = bytesread; // store the number of bytes for that message
				for(cnt = 0; cnt < bytesread; cnt++)	// append the message data into the buffer
				{
					x->double_buffer[mybuf][x->buf_count[mybuf]++] = buffer[cnt];
				}
//				if(x->x_verbose)post("thread read %i bytes to buffer %i (now %i bytes)",bytesread, mybuf,x->buf_count[mybuf] );
			}
		}
#ifdef _WIN32
		Sleep(1);
#endif /* _WIN32 */
	}
}

static void start_thread(t_multio *x)
{

// create the worker thread
    if(pthread_attr_init(&x->multio_thread_attr) < 0)
	{
       error("multio: could not launch receive thread");
       return;
    }
    if(pthread_attr_setdetachstate(&x->multio_thread_attr, PTHREAD_CREATE_DETACHED) < 0)
	{
       error("multio: could not launch receive thread");
       return;
    }
    if(pthread_create(&x->x_threadid, &x->multio_thread_attr, usb_read_thread, x) < 0)
	{
       error("multio: could not launch receive thread");
       return;
    }
    else
    {
       if(x->x_verbose)post("multio: thread %d launched", (int)x->x_threadid );
    }
}

// methods invoked by the inlets
static void multio_analog_write(t_multio *x, t_symbol *s, int argc, t_atom *argv)
{
	int channel;
	int value;
	unsigned char buffer[8];
	int bytesread;
	
	if (argc<2)
	{
		error("multio: multio_analog_write error: i need minimum 2 values list");
		return;
	}

	if (!(x->d))
	{
		error("multio: multI/O not initialized");
		return;
	}

	channel = atom_getfloat(argv++);
	value = atom_getfloat(argv);

	 if(channel < 0 || channel > 63)
	{
		error("multio: inconsistent dac output channel: %d", channel);
		return;
	}

	if (value != x->analog_buffer[channel])
	{
		x->analog_buffer[channel] = value;
		buffer[0] = 97 + channel; // channel is 0 based
		buffer[1] = value & 0xff; 
		buffer[2] = (value & 0xff00) >> 8;
		bytesread = usb_interrupt_write(x->d, 1, buffer, 3, TIMEOUT);
	}
}


static void multio_digi_write(t_multio *x, t_symbol *s, int argc, t_atom *argv)
{
	int channel;
	int value;
	unsigned char buffer[8];
	int bytesread;
	char testbit = 0x01;
	int count;
	int group;
	int channel_in_group;
	char ctmp;
	char bitmask;

	if (argc<2)
	{
		error("multio: multio_digi_write error: i need minimum 2 values list");
		return;
	}
	channel = atom_getfloat(argv++);
	value = atom_getfloat(argv);

	if(channel < 0 || channel > 63)
	{
		error("multio: inconsistent digital output channel: %d", channel);
		return;
	}

	group = channel / 8 ; 
	channel_in_group = channel % 8;

	bitmask = 0x01 << channel_in_group;
	ctmp = x->digi_outs[group] & ~bitmask;
	if (value)
		ctmp = ctmp | bitmask;
	if(ctmp != x->digi_outs[group])
	{
		x->digi_outs[group] = ctmp;
		buffer[0] = group + 1; // + 1 is the offset for digi outs (1..9) 
		buffer[1] = ctmp;
		bytesread = usb_interrupt_write(x->d, 1, buffer, 3, TIMEOUT);
		if(x->x_verbose)post("multio: writing %i to group %i", ctmp, group);
	}

}

static void multio_system_write(t_multio *x, t_symbol *s, int argc, t_atom *argv)
{

	unsigned char cmd, bvalue, smallvalue;
	unsigned short int value;
	unsigned char buffer[5];

	if (argc<3)
	{
		error("multio: multio_system_write error: i need minimum 3 values list");
		return;
	}
	cmd = atom_getfloat(argv++);
	bvalue = atom_getfloat(argv++);

	buffer[0] = 161;
	buffer[1] = cmd;
	buffer[2] = bvalue;

	switch(cmd)
	{
		case  0:
		case  1: usb_interrupt_write(x->d, 1, buffer, 3, TIMEOUT); break;
		case  2:
		case  3:
		case  4: value = atom_getfloat(argv);
			 buffer[3] = value & 0x00ff;
			 buffer[4] = value >> 8;
			 usb_interrupt_write(x->d, 1, buffer, 5, TIMEOUT);
			 break;
		case  5:
		case  6:
		case  7:
		case  8: buffer[3] = atom_getfloat(argv);
			 usb_interrupt_write(x->d, 1, buffer, 4, TIMEOUT);
			 break;
		case  9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17: usb_interrupt_write(x->d, 1, buffer, 3, TIMEOUT); break;
		default: error("multio: unknown system command"); break;
	}
}

// read & process the buffer
// this will be called each tick in the pd object
static void process_buffer(t_multio *x)
{
  int cnt;
  if(x->buf_count[x->whichbuf] > 0)
  {
//    post("process %i bytes buffer\n", x->buf_count[x->whichbuf]);
    for(cnt = 0; cnt < x->buf_count[x->whichbuf]; cnt++)
    {
      if(x->double_buffer[x->whichbuf][cnt] == 2)
      {
//        post("process buf: %i  msglen: %i  id: %i  val: %i", x->whichbuf, x->double_buffer[x->whichbuf][cnt],x->double_buffer[x->whichbuf][cnt+1],x->double_buffer[x->whichbuf][cnt+2]);
		if (x->double_buffer[x->whichbuf][cnt+1]>=1 && x->double_buffer[x->whichbuf][cnt+1] <= 8)
		{
			// digital input
			//send_digi(x, first-1, buffer[1]);
			char testbit = 0x01;
			t_atom lista[2];
			int count;
			int group = x->double_buffer[x->whichbuf][cnt+1]-1;

			for(count = 0; count < 8; count++)
			{
				if((x->double_buffer[x->whichbuf][cnt+2] & testbit) != (x->old_digi[group] & testbit))
				{
				SETFLOAT(lista, (group*8)+count);
				if(x->double_buffer[x->whichbuf][cnt+2] & testbit)
					SETFLOAT(lista+1, 1);
				else
					SETFLOAT(lista+1, 0);
				outlet_anything(x->d_out, gensym("list") , 2, lista);
				}
				testbit <<= 1;
			}
			x->old_digi[group] = x->double_buffer[x->whichbuf][cnt+2];
		}
	cnt += 2;
      }
      else if(x->double_buffer[x->whichbuf][cnt] == 3)
      {
//        post("process buf: %i msglen: %i  id: %i  val: %i", x->whichbuf, x->double_buffer[x->whichbuf][cnt],x->double_buffer[x->whichbuf][cnt+1],x->double_buffer[x->whichbuf][cnt+2] + (x->double_buffer[x->whichbuf][cnt+3] << 8));
		if (x->double_buffer[x->whichbuf][cnt+1]>=9 && x->double_buffer[x->whichbuf][cnt+1] <=96)
		{
			// analog input
			t_atom lista[2];
			int result;
			int channel = x->double_buffer[x->whichbuf][cnt+1]-9;

			result = x->double_buffer[x->whichbuf][cnt+2] + (x->double_buffer[x->whichbuf][cnt+3] << 8);
			x->analog_buffer[channel] = result;

			SETFLOAT(lista, channel);
			SETFLOAT(lista+1, result);
			outlet_anything(x->a_out, gensym("list"),2 , lista);
		}
	cnt += 3;
      }
      else
        cnt += x->double_buffer[x->whichbuf][cnt];
    }
    x->buf_count[x->whichbuf] = 0;
  }
}

/*
// method invoked by the timer
static void multio_read(t_multio *x)
{
	unsigned char buffer[8];
	int first;
	int bytesread;
	byte retData[8];
	int reads=64;

	if (x->d)
	{
	} else
	{
		error("multIO: connection not inizialized");
		return;
	}

	while(usb_interrupt_read(x->d, 0x81, buffer, 8, 2) > 0)

	{
//		reads--;
//if (usb_interrupt_read(x->d, 0x81, buffer, 8, TIMEOUT) > 0)
//{
		first = buffer[0];

		if (first>=1 && first <= 8)
		{
			// digital input
			//send_digi(x, first-1, buffer[1]);
			char testbit = 0x01;
			t_atom lista[2];
			int count;
			int group = first-1;

			for(count = 0; count < 8; count++)
			{
				if((buffer[1] & testbit) != (x->old_digi[group] & testbit))
				{
				SETFLOAT(lista, (group*8)+count);
				if(buffer[1] & testbit)
					SETFLOAT(lista+1, 1);
				else
					SETFLOAT(lista+1, 0);
				outlet_anything(x->d_out, gensym("list") , 2, lista);
				}
				testbit <<= 1;
			}
			x->old_digi[group] = buffer[1];
		}

		if (first>=9 && first <=96)
		{
			// analog input
			t_atom lista[2];
			int result;
			int channel = first-9;

			result = buffer[1] + (buffer[2] << 8);
			x->analog_buffer[channel] = result;

			SETFLOAT(lista, channel);
			SETFLOAT(lista+1, result);
			outlet_anything(x->a_out, gensym("list"),2 , lista);
		}

		if (first==161)
		{
			t_atom list2[2];
			t_atom list3[3];

			switch(buffer[1])
			{
			  case  0:
			  case  1: SETFLOAT(list2, buffer[1]);
			  	   SETFLOAT(list2+1, buffer[2]);
				   outlet_anything(x->s_out, gensym("list"),2 , list2);
				   break;
			  case  2:
			  case  3:
			  case  4: SETFLOAT(list3, buffer[1]);
			  	   SETFLOAT(list3+1, buffer[2]);
				   SETFLOAT(list3+2, (float)(buffer[3] + (buffer[4]<<8)) );
				   outlet_anything(x->s_out, gensym("list"),3 , list3);
				   break;
			  case  5:
			  case  6:
			  case  7:
			  case  8: SETFLOAT(list3, buffer[1]);
			  	   SETFLOAT(list3+1, buffer[2]);
				   SETFLOAT(list3+2, buffer[3]);
				   outlet_anything(x->s_out, gensym("list"),3 , list3);
				   break;
			  case  9: SETFLOAT(list2, buffer[1] - 9);
				   outlet_anything(x->a_out, gensym("list"),2 , list2);
				   break;
			  case 10:
			  case 11:
			  case 12:
			  case 13:
			  case 14:
			  case 15:
			  case 16:
			  case 17: SETFLOAT(list3, buffer[1]);
			  	   SETFLOAT(list3+1, buffer[2]);
				   SETFLOAT(list3+2, buffer[3]);
				   outlet_anything(x->s_out, gensym("list"),3 , list3);
				   break;
			  default: error("unknown system command echo"); break;
			}
			// system input
		}
	}
}
*/

static void multio_open(t_multio *x)
{
	struct usb_device *device;
	struct usb_bus* bus;
	unsigned char buffer[8];
	usb_init();
	usb_find_busses();
	usb_find_devices();


	for (bus=usb_get_busses();bus!=NULL;bus=bus->next)
	{
		struct usb_device* usb_devices = bus->devices;


		for(device=usb_devices;device;device=device->next)
		{

			if (device->descriptor.idVendor == multio_vendorID
				&&device->descriptor.idProduct == multio_productID)
			{

				post( "multio: Found mamalala multI/O as device '%s' on USB bus %s",
					device->filename,
					device->bus->dirname);
					x->d=usb_open(device);

				if (x->d)
				{ /* This is our device-- claim it */

					if (usb_set_configuration(x->d,multio_configuration)) {
						post("multio: Error setting USB configuration.");
						usb_close(x->d);
						return;
					}
					else post("multio: Selecting non-HID config");


					if (usb_claim_interface(x->d,multio_interface)) {
						post("multio: Claim failed-- the mamalala multI/O is in use by another driver.\n"
		  				"multio: Do a `dmesg` to see which kernel driver has claimed it--\n"
						"multio: You may need to `rmmod hid` or patch your kernel's hid driver.");
						usb_close(x->d);
						return;

					}
					else post("multio: Claimed interface");
					while(usb_interrupt_read(x->d, 0x81, buffer, 8, 10) > 0)
					{

					}
					return;
				}
			}
		}
	}
	// if i am here then i couldn't find mutlIO!
	error("multio: unable to find multI/O !");
}




static void multio_tick(t_multio *x)
{
    x->x_hit = 0;
//	multio_read(x);
process_buffer(x);
    if (!x->x_hit) clock_delay(x->x_clock, x->x_deltime);
}

static void multio_float(t_multio *x, t_float f)
{
    if (f != 0) multio_tick(x);
    else clock_unset(x->x_clock);
    x->x_hit = 1;
}

static void multio_start(t_multio *x)
{
    multio_float(x, 1);
}

static void multio_stop(t_multio *x)
{
    multio_float(x, 0);
}

static void multio_ft1(t_multio *x, t_floatarg g)
{
    if (g < 1) g = 1;
    x->x_deltime = g;
}
static void multio_verbose(t_multio *x, t_floatarg g)
{
    x->x_verbose=(g > 0) ;
}



static void multio_free(t_multio *x) 
{
if(is_open)
 {
	clock_free(x->x_clock);
	if(x->d)
	{
		while(pthread_cancel(x->x_threadid) < 0)
			if(x->x_verbose)post("multio: killing thread\n");
		if(x->x_verbose)post("multio: thread canceled\n");
    		usb_close(x->d);
	}
    is_open = 0;
 }
 else
  if(x->x_verbose)post("multio: not active object");
}


static void *multio_new(t_symbol *s, int argc, t_atom *argv)
{
if(!is_open)
{
    t_multio *x = (t_multio *)pd_new(multio_class);
	x->x_clock = clock_new(x, (t_method)multio_tick);
	x->x_deltime = DEFDELTIME;
	x->x_verbose = 0;
	x->a_out = outlet_new(&x->x_obj, &s_list);
	x->d_out = outlet_new(&x->x_obj, &s_list);
	x->s_out = outlet_new(&x->x_obj, &s_list);
	// look for multIO

	multio_open(x);
	if(x->d)
		start_thread(x);
	// inlets for digital and system
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("list"), gensym("digi_write")); // remap to digi_write
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("list"), gensym("system_write"));
	is_open = 1;
	multio_start(x);
    return (x);
}
else
{
error("multio: object already exists");
return(0);
}
}

void multio_setup(void)
{
    multio_class = class_new(gensym("multio"), (t_newmethod)multio_new,
        (t_method)multio_free, sizeof(t_multio), CLASS_DEFAULT, A_GIMME, 0);
    //class_addbang(multio_class, (t_method)multio_bang);
	// to set the time between 2 readouts
    class_addmethod(multio_class, (t_method)multio_ft1, gensym("readout_time"),
        A_FLOAT, 0);
	class_addfloat(multio_class, (t_method)multio_float); // start/stop using a toggle
	// to stop reading
	class_addmethod(multio_class, (t_method)multio_stop, gensym("stop"), 0);
	// to start reading
	class_addmethod(multio_class, (t_method)multio_start, gensym("start"), 0);
	// open the device
	class_addmethod(multio_class, (t_method)multio_open, gensym("open"), 0);
	// write analog data using leftmost inlet
	class_addlist(multio_class, (t_method)multio_analog_write);
	class_addmethod(multio_class, (t_method)multio_digi_write, gensym("digi_write"),
        A_GIMME, 0);
	class_addmethod(multio_class, (t_method)multio_system_write, gensym("system_write"),
        A_GIMME, 0);
is_open = 0;

	class_addmethod(multio_class, (t_method)multio_verbose, gensym("verbose"), A_FLOAT, 0);

	// welcome message
	post("\nmultio: a pd driver for the multI/O USB device");
	post("multio: www.davidemorelli.it - multio.mamalala.de");


}


