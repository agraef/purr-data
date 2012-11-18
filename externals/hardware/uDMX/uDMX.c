/*
	uDMX.c
	
	pd-Interface to the [ a n y m a | uDMX - Open Source USB Sensor Box ]
	
	Authors:	Michael Egger
	Copyright:	2007 [ a n y m a ]
	Website:	www.anyma.ch
	
	License:	GNU GPL 2.0 www.gnu.org
	
	Version:	0.2	2009-06-30	
				0.1 2007-01-28
	*/

#include "m_pd.h"

#include "uDMX_cmds.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>    /* this is libusb, see http://libusb.sourceforge.net/ */

#define USBDEV_SHARED_VENDOR    0x16C0  /* VOTI */
#define USBDEV_SHARED_PRODUCT   0x05DC  /* Obdev's free shared PID */

typedef struct _uDMX				// defines our object's internal variables for each instance in a patch
{
	t_object p_ob;					// object header - ALL objects MUST begin with this...
	usb_dev_handle	*dev_handle;	// handle to the uDMX usb device
	int	debug_flag;
	int channel;					// int value - received from the right inlet and stored internally for each object instance
} t_uDMX;

void *uDMX_class;					// global pointer to the object class - so max can reference the object 



// these are prototypes for the methods that are defined below
void uDMX_int(t_uDMX *x, long n);
void uDMX_setchannel(t_uDMX *x, t_floatarg f);
void uDMX_debug(t_uDMX *x,  t_symbol *s, short ac, t_atom *av);
void uDMX_list(t_uDMX *x, t_symbol *s, short ac, t_atom *av);
void uDMX_open(t_uDMX *x);
void uDMX_close(t_uDMX *x);
void *uDMX_new(long n);
static int  usbGetStringAscii(usb_dev_handle *dev, int dex, int langid, char *buf, int buflen);
void find_device(t_uDMX *x);

//--------------------------------------------------------------------------

void uDMX_setup(void)
{

	uDMX_class = class_new ( gensym("uDMX"),(t_newmethod)uDMX_new, 0, sizeof(t_uDMX), 	CLASS_DEFAULT,0);
	
	class_addfloat(uDMX_class, (t_method)uDMX_int);			// the method for an int in the left inlet (inlet 0)
	class_addmethod(uDMX_class, (t_method)uDMX_debug,gensym("debug"), A_GIMME, 0);
	class_addlist(uDMX_class, (t_method)uDMX_list);
	class_addmethod(uDMX_class, (t_method)uDMX_open, gensym("open"), 0);		
	class_addmethod(uDMX_class, (t_method)uDMX_close, gensym("close"), 0);	
	class_addmethod(uDMX_class, (t_method)uDMX_setchannel, gensym("setchannel"), A_FLOAT, 0);	

	post("uDMX version 0.9 - (c) 2006 [ a n y m a ]",0);	// post any important info to the max window when our object is laoded
}
//--------------------------------------------------------------------------

void uDMX_int(t_uDMX *x, long n)	// x = the instance of the object; n = the int received in the left inlet 
{
	unsigned char       buffer[8];
	int                 nBytes;

	if (n > 255) n=255;
	if (n < 0) n=0;
	if (x->channel > 512) x->channel=512;
	if (x->channel < 0) x->channel=0;
	
	if (!(x->dev_handle)) find_device(x);
	else {
		nBytes = usb_control_msg(x->dev_handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
									cmd_SetSingleChannel, n, x->channel, buffer, sizeof(buffer), 5000);
		if(nBytes < 0)
			 if (x->debug_flag) error("uDMX: USB error: %s", usb_strerror());
	}
}

void uDMX_setchannel(t_uDMX *x, t_floatarg f)
{
	x->channel = f;
}

//--------------------------------------------------------------------------

void uDMX_list(t_uDMX *x, t_symbol *s, short ac, t_atom *av)
{
	int i;
	unsigned char* buf = malloc(ac);
	int                 nBytes;
	int 		n;

	if (x->channel > 512) x->channel=512;
	if (x->channel < 0) x->channel=0;

	if (!(x->dev_handle)) find_device(x);
	else {

		if (x->debug_flag) post("uDMX: ac: %i\n", ac);
		for(i=0; i<ac; ++i,av++) {
			if (av->a_type==A_FLOAT) {
				n = (int) av->a_w.w_float;
				if (n > 255) n=255;
				if (n < 0) n=0;

				buf[i] = n;
			} else
				buf[i] = 0;
		}
		nBytes = usb_control_msg(x->dev_handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
									cmd_SetChannelRange, ac, x->channel, buf, ac, 5000);
		if (x->debug_flag) post( "bytes returned: %i\n", nBytes);
		if(nBytes < 0)
           if (x->debug_flag) error("uDMX: USB error: %s\n", usb_strerror());
		else if(nBytes > 0)  if (x->debug_flag) post("uDMX: returned: %i\n", (int)(buf[0]));
		free(buf);
	}
}

//--------------------------------------------------------------------------

void uDMX_debug(t_uDMX *x, t_symbol *s, short ac, t_atom *av)	// x = the instance of the object; n = the int received in the left inlet 
{
	x->debug_flag = 1;
	if (ac) {
		if (av->a_type==A_FLOAT) x->debug_flag = av->a_w.w_float;
	}
}



//--------------------------------------------------------------------------

void uDMX_free(t_uDMX *x)
{
	if (x->dev_handle)
		usb_close(x->dev_handle);
}

//--------------------------------------------------------------------------

void uDMX_open(t_uDMX *x)
{
	if (x->dev_handle) {
		post("uDMX: There is already a connection to www.anyma.ch/uDMX",0);
	} else find_device(x);
}

//--------------------------------------------------------------------------

void uDMX_close(t_uDMX *x)
{
	if (x->dev_handle) {
		usb_close(x->dev_handle);
		x->dev_handle = NULL;
		post("uDMX: Closed connection to www.anyma.ch/uDMX",0);
	} else
		post("uDMX: There was no open connection to www.anyma.ch/uDMX",0);
}


//--------------------------------------------------------------------------

void *uDMX_new(long n)		// n = int argument typed into object box (A_DEFLONG) -- defaults to 0 if no args are typed
{
	t_uDMX *x;				// local variable (pointer to a t_uDMX data structure)

	x = (t_uDMX *)pd_new(uDMX_class); // create a new instance of this object
	
	// create a second int inlet (leftmost inlet is automatic - all objects have one inlet by default)
	// floatinlet_new(x, x->channel); //crashes on PD .... assigns float in inlet 2 directly to channel
	inlet_new(&x->p_ob, &x->p_ob.ob_pd, gensym("float"), gensym("setchannel"));

	x->channel = 0;
	x->debug_flag = 0;
	x->dev_handle = NULL;
	
	find_device(x);

	return(x);					// return a reference to the object instance 
}

//--------------------------------------------------------------------------


static int  usbGetStringAscii(usb_dev_handle *dev, int dex, int langid, char *buf, int buflen)
{
char    buffer[256];
int     rval, i;

    if((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + dex, langid, buffer, sizeof(buffer), 1000)) < 0)
        return rval;
    if(buffer[1] != USB_DT_STRING)
        return 0;
    if((unsigned char)buffer[0] < rval)
        rval = (unsigned char)buffer[0];
    rval /= 2;
    /* lossy conversion to ISO Latin1 */
    for(i=1;i<rval;i++){
        if(i > buflen)  /* destination buffer overflow */
            break;
        buf[i-1] = buffer[2 * i];
        if(buffer[2 * i + 1] != 0)  /* outside of ISO Latin1 range */
            buf[i-1] = '?';
    }
    buf[i-1] = 0;
    return i-1;
}

//--------------------------------------------------------------------------


void find_device(t_uDMX *x)
{
	usb_dev_handle      *handle = NULL;
	struct usb_bus      *bus;
	struct usb_device   *dev;

 	usb_init();
	usb_find_busses();
    usb_find_devices();
	 for(bus=usb_get_busses(); bus; bus=bus->next){
        for(dev=bus->devices; dev; dev=dev->next){
            if(dev->descriptor.idVendor == USBDEV_SHARED_VENDOR && dev->descriptor.idProduct == USBDEV_SHARED_PRODUCT){
                char    string[256];
                int     len;
                handle = usb_open(dev); /* we need to open the device in order to query strings */
                if(!handle){
                    error ("Warning: cannot open USB device: %s", usb_strerror());
                    continue;
                }
                /* now find out whether the device actually is uDMX */
                len = usbGetStringAscii(handle, dev->descriptor.iManufacturer, 0x0409, string, sizeof(string));
                if(len < 0){
                    post("uDMX: warning: cannot query manufacturer for device: %s", usb_strerror());
                    goto skipDevice;
                }
                
			//	post("uDMX: seen device from vendor ->%s<-", string); 
                if(strcmp(string, "www.anyma.ch") != 0)
                    goto skipDevice;
                len = usbGetStringAscii(handle, dev->descriptor.iProduct, 0x0409, string, sizeof(string));
                if(len < 0){
                    post("uDMX: warning: cannot query product for device: %s", usb_strerror());
                    goto skipDevice;
                }
              //  post("uDMX: seen product ->%s<-", string);
                if(strcmp(string, "uDMX") == 0)
                    break;
skipDevice:
                usb_close(handle);
                handle = NULL;
            }
        }
        if(handle)
            break;
    }
	
    if(!handle){
        post("uDMX: Could not find USB device www.anyma.ch/uDMX");
		x->dev_handle = NULL;
	} else {
		x->dev_handle = handle;
		 post("uDMX: Found USB device www.anyma.ch/uDMX");
	}
}
