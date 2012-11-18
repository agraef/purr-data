/**
	@file
	xbee_test - test xbee communication

	@ingroup	examples	
*/

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>

#ifdef PD
#include "m_pd.h"
#include "max2pd.h"
#else
#include "ext.h"							// standard Max include, always required
#include "ext_obex.h"						// required for new style Max object

# ifdef MAC_VERSION
# include "ext_strings.h"
# endif
#endif /* PD */

#include "xbee.h"
#include "xbee_io.h"

////////////////////////// object struct
typedef struct _xbee_test 
{
	t_object			a_ob;			// the object itself (must be first)
	long				baud;
	uint8_t				dest_addr64[8];
	uint8_t				dest_addr16[2];
	xbee_t              *xbee;
} t_xbee_test;


///////////////////////// function prototypes
//// standard set
void *xbee_test_new(t_symbol *s, long argc, t_atom *argv);
void xbee_test_free(t_xbee_test *x);
void xbee_test_assist(t_xbee_test *x, void *b, long m, long a, char *s);
//// additional methods
void xbee_test_bang(t_xbee_test *x); // incoming bang message
void xbee_test_close(t_xbee_test *x);
int xbee_test_open_device(t_xbee_test *x, t_symbol *s);
void xbee_test_dest64(t_xbee_test *x, t_symbol *s);
void xbee_test_dest16(t_xbee_test *x, t_symbol *s);
static void xbee_test_on(t_xbee_test *x, long dpin);
static void xbee_test_off(t_xbee_test *x, long dpin);


//////////////////////// global class pointer variable
void *xbee_test_class;


static int set_baud(int fd, int baud_rate)
{
    struct termios termios;
    speed_t speed;
    
    
    switch(baud_rate) {
        case 50:
            speed = B50;
            break;
        case 75:
            speed = B75;
            break;
        case 110:
            speed = B110;
            break;
        case 300:
            speed = B300;
            break;
        case 600:
            speed = B600;
            break;
        case 1200:
            speed = B1200;
            break;
        case 2400:
            speed = B2400;
            break;
        case 4800:
            speed = B4800;
            break;
        case 9600:
            speed = B9600;
            break;
        case 19200:
            speed = B19200;
            break;
        case 38400:
            speed = B38400;
            break;
        case 57600:
            speed = B57600;
            break;
        case 115200:
            speed = B115200;
            break;
        case 230400:
            speed = B230400;
            break;
#ifdef B256000
        case 256000:
            speed = B256000;
            break;
#endif
        default:
            return -1;
    }
    
    if (tcgetattr(fd, &termios) < 0) {
        error("xbee_test: tcgetattr failed (%d)", errno);
        return -1;
    }
    
	termios.c_cflag |= CLOCAL;
	
    if (cfsetispeed(&termios, speed) < 0) {
        error("xbee_test: cfsetispeed failed (%d)", errno);
        return -1;
    }
    
    if (cfsetospeed(&termios, speed) < 0) {
        error("xbee_test: cfsetospeed failed (%d)", errno);
        return -1;
    }
    
    if (tcsetattr(fd, TCSANOW, &termios) < 0) {
        error("xbee_test: tcsetattr failed (%d)", errno);
        return -1;
    }
    
    return 0;
}

#ifdef PD

void xbee_test_setup(void) 
{
    xbee_test_class = class_new(gensym("xbee_test"), 
                                 (t_newmethod)xbee_test_new, 
                                 (t_method)xbee_test_free,
                                 sizeof(t_xbee_test),
                                 CLASS_DEFAULT,
                                 A_GIMME,0);
    class_addbang(xbee_test_class, (t_method)xbee_test_bang);
	class_addmethod(xbee_test_class, (t_method)xbee_test_dest64, gensym("dest64"),   A_DEFSYMBOL, 0);
	class_addmethod(xbee_test_class, (t_method)xbee_test_dest16, gensym("dest16"),   A_DEFSYMBOL, 0);
	class_addmethod(xbee_test_class, (t_method)xbee_test_on, gensym("on"),		A_DEFFLOAT, 0);
	class_addmethod(xbee_test_class, (t_method)xbee_test_off, gensym("off"),		A_DEFFLOAT, 0);
}
#else
int main(void)
{	
	t_class *c;


	c = class_new("xbee_test", (method)xbee_test_new, (method)xbee_test_free, (long)sizeof(t_xbee_test), 0L, A_GIMME, 0);

    class_addmethod(c, (method)xbee_test_bang,		"bang",		0);
    class_addmethod(c, (method)xbee_test_assist,	"assist",	A_CANT, 0);
	class_addmethod(c, (method)xbee_test_dest64,	"dest64",   A_DEFSYM, 0);
	class_addmethod(c, (method)xbee_test_dest16,	"dest16",   A_DEFSYM, 0);
	class_addmethod(c, (method)xbee_test_on,		"on",		A_LONG, 0);
	class_addmethod(c, (method)xbee_test_off,		"off",		A_LONG, 0);
		
	class_register(CLASS_BOX, c);
	xbee_test_class = c;

	return 0;
}
#endif /* PD */

static void xbee_test_on(t_xbee_test *x, long dpin)
{
	char cmd[2] = "Dx";
	uint8_t param[] = {5};
	xbee_io_context_t *ctx;
	int r;
	
	
	if (!x->xbee)
		return;
		
	ctx = xbee_user_context(*(x->xbee));
	if (!ctx)
		return;

	if (dpin >= 0 && dpin <= 7) {
		cmd[1] = dpin + '0';
		
		r = xbee_send_remote_at_cmd(x->xbee, cmd, 1, 2, param, x->dest_addr64, x->dest_addr16);
		if (r < 0) {
			post("xbee_test: xbee_send_remote_at_cmd failed (%d)", errno);
		}
	}
		
	// Nudge IO thread
	write(ctx->pipe_fds[1], "!", 1);
}


static void xbee_test_off(t_xbee_test *x, long dpin)
{
	char cmd[2] = "Dx";
	uint8_t param[] = {4};
	xbee_io_context_t *ctx;
	int r;
	
	
	if (!x->xbee)
		return;
		
	ctx = xbee_user_context(*(x->xbee));
	if (!ctx)
		return;

	if (dpin >= 0 && dpin <= 7) {
		cmd[1] = dpin + '0';
		
		r = xbee_send_remote_at_cmd(x->xbee, cmd, 1, 2, param, x->dest_addr64, x->dest_addr16);
		if (r < 0) {
			post ("xbee_test: xbee_send_remote_at_cmd failed (%d)", errno);
		}
	}
		
	// Nudge IO thread
	write(ctx->pipe_fds[1], "!", 1);
}

#ifndef PD
void xbee_test_assist(t_xbee_test *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) { //inlet
		sprintf(s, "I am inlet %ld", a);
	} 
	else {	// outlet
		sprintf(s, "I am outlet %ld", a); 			
	}
}
#endif /* NOT PD */

void xbee_test_bang(t_xbee_test *x)
{
}


void xbee_test_close(t_xbee_test *x)
{
	xbee_t *xbee = x->xbee;
	xbee_io_context_t *ctx;
	
	 
	if (!xbee)
		return;
	
	xbee_kill_io_thread(x->xbee);

	ctx = xbee_user_context(*xbee);
	if (!ctx)
		return;
	
	if (ctx->fd >= 0) {
		close(ctx->fd);
		ctx->fd = -1;
	}

	if (ctx->pipe_fds[0] >= 0) {
		close(ctx->pipe_fds[0]);
		ctx->pipe_fds[0] = -1;
	}
	
	if (ctx->pipe_fds[1] >= 0) {
		close(ctx->pipe_fds[1]);
		ctx->pipe_fds[1] = -1;
	}
}


void xbee_test_free(t_xbee_test *x)
{
	xbee_t *xbee = x->xbee;
	xbee_io_context_t *ctx;
	
	
	xbee_test_close(x);
	if (xbee) {
		ctx = xbee_user_context(*xbee);
		if (ctx) {
			sysmem_freeptr(ctx);
		}
		
		sysmem_freeptr(xbee);
	}
}


void xbee_test_dest64(t_xbee_test *x, t_symbol *s)
{
	uint64_t dest;
	int i;
	
	
	dest = strtoll(s->s_name, NULL, 16);
	for (i = 0; i < 8; i++) {
		x->dest_addr64[7 - i] = dest & 0xff;
		dest >>= 8;
	}
	
	post("xbee_test: set dest64 addr to %02x%02x%02x%02x%02x%02x%02x%02x", 
	  x->dest_addr64[0], x->dest_addr64[1], x->dest_addr64[2], x->dest_addr64[3], 
	  x->dest_addr64[4], x->dest_addr64[5], x->dest_addr64[6], x->dest_addr64[7]); 
}


void xbee_test_dest16(t_xbee_test *x, t_symbol *s)
{
	uint16_t dest;
	int i;
	
	
	dest = strtol(s->s_name, NULL, 16);
	for (i = 0; i < 2; i++) {
		x->dest_addr16[1 - i] = dest & 0xff;
		dest >>= 8;
	}
	
	post("xbee_test: set dest16 addr to %02x%02x", x->dest_addr16[0], x->dest_addr16[1]); 
}


int xbee_test_open_device(t_xbee_test *x, t_symbol *s)
{			
	xbee_io_context_t *xbee_io_context;
	int fd;
	
	
	if (s == gensym("")) {
		return 0;
	}

	xbee_test_close(x);

	xbee_io_context = (xbee_io_context_t *)sysmem_newptr(sizeof(xbee_io_context_t));
	if (xbee_io_context == NULL) {
		error("xbee_test: %s: can't allocate memory", s->s_name);
		return -1;
	}
	
	memset(xbee_io_context, 0, sizeof(xbee_io_context_t));

	// Pipe for nudging io thread when there's more data to write
	if (pipe(xbee_io_context->pipe_fds) < 0) {
		error("xbee_test: %s: can't open pipe (%d)", s->s_name, errno);
		sysmem_freeptr(xbee_io_context);
		return -1;
	}	

	fd = open(s->s_name, O_RDWR | O_NONBLOCK, 0);
	if (fd < 0) {
		error("xbee_test: %s: can't open device file (error %d)", s->s_name, errno);
		close(xbee_io_context->pipe_fds[0]);
		close(xbee_io_context->pipe_fds[1]);
		sysmem_freeptr(xbee_io_context);
		return -1;
	}

	post("xbee_test: opened %s", s->s_name);
						
	if (set_baud(fd, x->baud) < 0) {
		post("xbee_test: %s: error setting baud to %d", s->s_name, x->baud);
	} else {
		post("xbee_test: baud set to %d", x->baud);
	}

	xbee_io_context->fd = fd;
		
	xbee_user_context(*(x->xbee)) = xbee_io_context;

	if (xbee_new_io_thread(x->xbee) < 0) {
		xbee_user_context(*(x->xbee)) = NULL;
		close(fd);
		close(xbee_io_context->pipe_fds[0]);
		close(xbee_io_context->pipe_fds[1]);
		sysmem_freeptr(xbee_io_context);
		error("xbee_test: error starting new IO thread");
		return -1;
	}
	
	return 0;
}


void *xbee_test_new(t_symbol *s, long argc, t_atom *argv)
{
	t_xbee_test *x = NULL;
	t_symbol *a;


	if (argc < 1) {
		error("xbee_test: No device name specified");
		return NULL;
	}
	
	x = (t_xbee_test *)object_alloc(xbee_test_class);
	
	x->baud = 9600;

	x->xbee = (xbee_t *)sysmem_newptr(sizeof(*x->xbee));
	if (x->xbee == NULL) {
		object_free(x);
		return NULL;
	}
	
	xbee_init(x->xbee);

	// Set 64-bit destination to 0x0000000000ffff (broadcast)
	memset(x->dest_addr64, 0, sizeof(x->dest_addr64));
	x->dest_addr64[7] = 0xff;
	x->dest_addr64[6] = 0xff;
	
	// Set 16-bit destination to 0xfffe (ZNet broadcast)
	x->dest_addr16[0] = 0xff;
	x->dest_addr16[1] = 0xfe;

	if (argc >= 1) {
		if (argc >= 2) {
			x->baud = atom_getlong(&argv[1]);
				
			if (x->baud == 0) {
				error("xbee_test: second argument should be a valid baud rate.");
				sysmem_freeptr(x->xbee);
				object_free(x);
				return NULL;
			}
		}

		a = atom_getsym(&argv[0]);
		if (a != gensym("")) {
			if (xbee_test_open_device(x, a) < 0) {
				sysmem_freeptr(x->xbee);
				object_free(x);
				return NULL;
			}
		}
		
	}
	
	return (x);
}
