/* gpio - Pi gpio pins via /sys/etc */
/* see http://elinux.org/RPi_Low-level_peripherals */

/* Copyright Miller Puckette - BSD license */

#include "m_pd.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

static t_class *gpio_class;

#define FILE_PREFIX "/sys/class/gpio/"
#define FILE_EXPORT FILE_PREFIX "export"

typedef struct _gpio
{
    t_object x_obj;
    t_outlet *x_out1;
    int x_pin;
    int x_fdvalue;
} t_gpio;

static void gpio_enable(t_gpio *x, t_float f)
{
    char buf[1024];
    if (f != 0)
         sprintf(buf, "echo %d > /sys/class/gpio/export\n", x->x_pin);
    else sprintf(buf, "echo %d > /sys/class/gpio/unexport\n", x->x_pin);
    system(buf);
}

static void gpio_output(t_gpio *x, t_float f)
{
    char buf[1024];
    if (f != 0)
         sprintf(buf, "echo out > /sys/class/gpio/gpio%d/direction\n",
            x->x_pin);
    else sprintf(buf, "echo in > /sys/class/gpio/gpio%d/direction\n",
        x->x_pin);
    system(buf);
}

static void gpio_open(t_gpio *x, t_float f)
{
    char buf[1024];
    sprintf(buf, "/sys/class/gpio/gpio%d/value", x->x_pin);
    if (f != 0)
    {
        if (x->x_fdvalue >= 0)
            post("gpio: already open");
        else
        {
            x->x_fdvalue = open(buf, O_RDWR);
            if (x->x_fdvalue < 0)
                post("%s: %s", buf, strerror(errno));
        }
    }
    else
    {
        if (x->x_fdvalue < 0)
            post("gpio: already closed");
        else
        {
            close(x->x_fdvalue);
            x->x_fdvalue = -1;
        }
    }
}

static void gpio_float(t_gpio *x, t_float f)
{
    char buf[1024];
    if (x->x_fdvalue < 0)
        pd_error(x, "gpio: not open");
    else
    {
        sprintf(buf, "%d\n", (f != 0));
        if (write(x->x_fdvalue, buf, strlen(buf)) < (int)strlen(buf))
            pd_error(x, "gpio_float: %s", strerror(errno));
    }
}

static void gpio_bang(t_gpio *x)
{
    char buf[1024];
    if (x->x_fdvalue < 0)
        pd_error(x, "gpio: not open");
    else
    {
        int rval = lseek(x->x_fdvalue, 0, 0);
        if (rval < 0)
        {
            pd_error(x, "gpio_bang (seek): %s", strerror(errno));
            return;
        }
        rval = read(x->x_fdvalue, buf, sizeof(buf)-1);
        if (rval < 0)
        {
            pd_error(x, "gpio_bang (read): %s", strerror(errno));
            return;
        }
        buf[rval] = 0;
        if (sscanf(buf, "%d", &rval) < 1)
        {
            pd_error(x, "gpio_bang: couldn't parse string: %s", buf);
            return;
        }
        outlet_float(x->x_out1, rval);
    }
}

static void *gpio_new(t_floatarg f)
{
    t_gpio *x = (t_gpio *)pd_new(gpio_class);
    x->x_out1 = outlet_new(&x->x_obj, gensym("float"));
    x->x_fdvalue = -1;
    x->x_pin = f;
    return (x);
}


void gpio_setup(void)
{
    gpio_class = class_new(gensym("gpio"), (t_newmethod)gpio_new,
        0, sizeof(t_gpio), 0, A_DEFFLOAT, 0);
    class_addmethod(gpio_class, (t_method)gpio_enable, gensym("enable"), 
        A_FLOAT, 0);
    class_addmethod(gpio_class, (t_method)gpio_output, gensym("output"), 
        A_FLOAT, 0);
    class_addmethod(gpio_class, (t_method)gpio_open, gensym("open"), 
        A_FLOAT, 0);
    class_addfloat(gpio_class, gpio_float);
    class_addbang(gpio_class, gpio_bang);
}
