/* gpio - Pi gpio pins via /sys/etc */
/* see http://elinux.org/RPi_Low-level_peripherals */

/* Copyright Ivica Ico Bukic <ico@vt.edu> */
/* Based on Miller Puckette's example - Copyright Miller Puckette - BSD license */
/* with changes to make external rely on the gpio executable to sidestep permmission issues */

#include "m_pd.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

static t_class *disis_gpio_class;

//#define FILE_PREFIX "/sys/class/gpio/"
//#define FILE_EXPORT FILE_PREFIX "export"

typedef struct _disis_gpio
{
    t_object x_obj;
    t_outlet *x_out1;
    int x_pin;
    int x_fdvalue;
    int x_dir;
} t_disis_gpio;

static void disis_gpio_enable(t_disis_gpio *x, t_float f, t_float dir)
{
    char buf[1024];
    if (f != 0) {
        if (dir) {
            sprintf(buf, "gpio export %d out\n", x->x_pin);
        } else {
            sprintf(buf, "gpio export %d in\n", x->x_pin);
        }
    } else sprintf(buf, "gpio unexport %d\n", x->x_pin);
    system(buf);
}

static void disis_gpio_output(t_disis_gpio *x, t_float f)
{
    char buf[1024];
    x->x_dir = f;
    if (f != 0)
         sprintf(buf, "gpio -g mode %d out\n",
            x->x_pin);
    else sprintf(buf, "gpio -g mode %d in\n",
        x->x_pin);
    system(buf);
}

static void disis_gpio_open(t_disis_gpio *x, t_float f)
{
    char buf[1024];
    sprintf(buf, "/sys/class/gpio/gpio%d/value", x->x_pin);
    if (f != 0)
    {
        if (x->x_fdvalue >= 0)
            post("disis_gpio: already open");
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
            post("disis_gpio: already closed");
        else
        {
            close(x->x_fdvalue);
            x->x_fdvalue = -1;
        }
    }
}

static void disis_gpio_float(t_disis_gpio *x, t_float f)
{
    char buf[1024];
    if (x->x_fdvalue < 0)
        pd_error(x, "disis_gpio: not open");
    else
    {
        sprintf(buf, "%d\n", (f != 0));
        if (write(x->x_fdvalue, buf, strlen(buf)) < (int)strlen(buf))
            pd_error(x, "disis_gpio_float: %s", strerror(errno));
    }
}

static void disis_gpio_bang(t_disis_gpio *x)
{
    char buf[1024];
    if (x->x_fdvalue < 0)
        pd_error(x, "disis_gpio: not open");
    else
    {
        int rval = lseek(x->x_fdvalue, 0, 0);
        if (rval < 0)
        {
            pd_error(x, "disis_gpio_bang (seek): %s", strerror(errno));
            return;
        }
        rval = read(x->x_fdvalue, buf, sizeof(buf)-1);
        if (rval < 0)
        {
            pd_error(x, "disis_gpio_bang (read): %s", strerror(errno));
            return;
        }
        buf[rval] = 0;
        if (sscanf(buf, "%d", &rval) < 1)
        {
            pd_error(x, "disis_gpio_bang: couldn't parse string: %s", buf);
            return;
        }
        outlet_float(x->x_out1, rval);
    }
}

static void *disis_gpio_new(t_floatarg f)
{
    t_disis_gpio *x = (t_disis_gpio *)pd_new(disis_gpio_class);
    x->x_out1 = outlet_new(&x->x_obj, gensym("float"));
    x->x_fdvalue = -1;
    x->x_pin = f;
    x->x_dir = 0;
    return (x);
}


void disis_gpio_setup(void)
{
    disis_gpio_class = class_new(gensym("disis_gpio"), (t_newmethod)disis_gpio_new,
        0, sizeof(t_disis_gpio), 0, A_DEFFLOAT, 0);
    class_addmethod(disis_gpio_class, (t_method)disis_gpio_enable, gensym("enable"), 
        A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(disis_gpio_class, (t_method)disis_gpio_output, gensym("output"), 
        A_FLOAT, 0);
    class_addmethod(disis_gpio_class, (t_method)disis_gpio_open, gensym("open"), 
        A_DEFFLOAT, 0);
    class_addfloat(disis_gpio_class, disis_gpio_float);
    class_addbang(disis_gpio_class, disis_gpio_bang);
}
