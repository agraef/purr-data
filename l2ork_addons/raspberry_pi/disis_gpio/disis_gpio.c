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
#include <pthread.h>
#include "g_canvas.h"
#include "wiringPi/wiringPi/wiringPi.h"
#include "wiringPi/wiringPi/softPwm.h"

static t_class *disis_gpio_class;

//#define FILE_PREFIX "/sys/class/gpio/"
//#define FILE_EXPORT FILE_PREFIX "export"

// ============================== HELPER FUNCTIONS ================================= //

typedef struct _params
{
    int *p_pin;
    int *p_val;
    int *p_thread;
} t_params;

/*
 * softPwmThread:
 *  Thread to do the actual PWM output
 ***************************************
 */

//  The softPWM Frequency is derived from the "pulse time" below. Essentially,
//  the frequency is a function of the range and this pulse time.
//  The total period will be range * pulse time in uS, so a pulse time
//  of 10 and a range of 1000 gives a period of 100 * 100 = 10,000 uS
//  which is a frequency of 1000Hz.
//
//  It's possible to get a higher frequency by lowering the pulse time,
//  however CPU uage will skyrocket as wiringPi uses a hard-loop to time
//  periods under 10uS - this is because the Linux timer calls are just
//  accurate at all, and have an overhead.
//
//  Another way to increase the frequency is to reduce the range - however
//  that reduces the overall output accuracy...

static void *disisSoftPwmThread(void *val)
{
  t_params *p = (t_params *)val;

  //printf("thread %d\n", *(p->p_thread));

  piHiPri (50);

  while (!*(p->p_thread))
  {
    if (*(p->p_val) != 0)
      digitalWrite (*(p->p_pin), HIGH);
    delayMicroseconds (*(p->p_val) * 10);

    if ((1000 - *(p->p_val)) != 0)
      digitalWrite (*(p->p_pin), LOW) ;
    delayMicroseconds ((1000 - *(p->p_val)) * 10);
  }
  //printf("thread exit %d\n", *(p->p_thread));
  *(p->p_thread) = 2;
  return NULL ;
}

/*
 * softPwmCreate:
 *  Create a new PWM thread.
 ***************************************
 */

int disisSoftPwmCreate (t_params *p)
{
  int res;
  pthread_t myThread;

  pinMode      (*p->p_pin, OUTPUT);
  digitalWrite (*p->p_pin, LOW);

  res = pthread_create (&myThread, NULL, (void *) &disisSoftPwmThread, (void *)p);

  //while (disisNewPin != -1)
  //  delay (1) ;

  return res;
}

// ============================ END HELPER FUNCTIONS =============================== //

typedef struct _disis_gpio
{
    t_object x_obj;
    t_outlet *x_out1;
    t_outlet *x_out2;
    int x_pin;
    int x_export;
    int x_fdvalue;
    int x_dir;
    int x_pwm;
    int x_softpwm;
    int x_softpwmval;
    int x_softpwm_thread; // -1 = not necessary, 0 = start when necessary, 1 = close thread, 2 = thread has been closed
    t_params x_params;
    //t_symbol *x_chown;
} t_disis_gpio;

static void disis_gpio_close(t_disis_gpio *x);
static void disis_gpio_togglesoftpwm(t_disis_gpio *x, t_float on);
static void disis_gpio_togglepwm(t_disis_gpio *x, t_float on);

static void disis_gpio_reflectstatus(t_disis_gpio *x) {
    if (x->x_fdvalue >= 0) {
        outlet_float(x->x_out2, 1);
    } else {
        outlet_float(x->x_out2, 0);
    }
}

static int disis_gpio_isvalidpin(int pin) {
    int ret;
    switch (pin) {
        case 0:
            ret = 1;
            break;
        case 4:
        case 7:
        case 8:
        case 17:
        case 18:
        case 22:
        case 23:
        case 24:
        case 25:
        case 27:
            ret = 2;
            break;
        default:
            ret = 0;
            post("disis_gpio: invalid pin number");
            break;
    }
    return(ret);
}

static void disis_gpio_export(t_disis_gpio *x, t_float f)
{
    if (x->x_export) {
        post("disis_gpio: already exported");
        return;
    } 
    if ((int)f != 0 || x->x_pin != 0) {
        if ( ((int)f != 0 && disis_gpio_isvalidpin((int)f)) || ((int)f == 0 && disis_gpio_isvalidpin(x->x_pin)) ) {
            if ((int)f != 0)
                x->x_pin = (int)f;
            x->x_export = 1;
            post("disis_gpio: exporting pin %d", x->x_pin);
            char buf[1024];
            //sprintf(buf, "gpio export %d %s\n", x->x_pin, dir->s_name);
            sprintf(buf, "echo %d > /sys/class/gpio/export\n", x->x_pin);
            if (system(buf) < 0) {
                post("disis_gpio: failed to export requested pin");
                x->x_export = 0;
            }
            //if (system(x->x_chown->s_name) < 0) { // to adjust permissions within the exported gpio pin
            //    post("disis_gpio: failed setting permissions to /sys/class/gpio/gpio* subfolders");
            //    x->x_export = 0;
            //}
        }
    } else {
        post("disis_gpio: invalid pin number (0)");
    }
}

static void disis_gpio_unexport(t_disis_gpio *x)
{
    if (x->x_export) {
        char buf[1024];
        if (x->x_fdvalue >= 0) {
            disis_gpio_close(x);
        }
        //sprintf(buf, "gpio unexport %d\n", x->x_pin);
        sprintf(buf, "echo %d > /sys/class/gpio/unexport\n", x->x_pin);
        if (system(buf) < 0) {
            post("disis_gpio: failed to unexport requested pin");          
        }
	x->x_export = 0;
    }
}

static void disis_gpio_direction(t_disis_gpio *x, t_symbol *dir)
{
    if (strlen(dir->s_name) > 0) {
        char buf[1024];
        int d = -1;
	if (!strcmp(dir->s_name, "in")) d = 0;
	else if (!strcmp(dir->s_name, "out")) d = 1;
        if (d >= 0) {
            x->x_dir = d;
            //sprintf(buf, "gpio -g mode %d %s\n", x->x_pin, dir->s_name);
            sprintf(buf, "echo %s > /sys/class/gpio/gpio%d/direction\n", dir->s_name, x->x_pin);
            if (system(buf) < 0) {
                post("disis_gpio: failed to set pin direction");
            }
        }
    }
}

static void disis_gpio_open(t_disis_gpio *x)
{
    char buf[1024];
    sprintf(buf, "/sys/class/gpio/gpio%d/value", x->x_pin);
    if (x->x_fdvalue >= 0)
        post("disis_gpio: already open");
    else
    {
        x->x_fdvalue = open(buf, O_RDWR);
        if (x->x_fdvalue < 0) {
            post("%s: %s", buf, strerror(errno));
        }
    }
    disis_gpio_reflectstatus(x);
}

static void disis_gpio_close(t_disis_gpio *x)
{
    if (x->x_fdvalue < 0)
        post("disis_gpio: already closed");
    else
    {
        if (x->x_softpwm_thread == 0) {
            x->x_softpwm_thread = 1;
            while (x->x_softpwm_thread != 2)
                usleep(100);
            x->x_softpwm_thread = -1;
        }
        close(x->x_fdvalue);
        x->x_fdvalue = -1;

    }
    disis_gpio_reflectstatus(x);
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

static void disis_gpio_pwm(t_disis_gpio *x, t_float val)
{
    int out;
    if (x->x_fdvalue != -1 && (x->x_pwm || x->x_softpwm)) {
        if ((int)val < 0)
            out = 0 ;
        else if ((int)val > 1023)
            out = 1023 ;
        else out = (int)val;
        if (x->x_pwm)
            pwmWrite (x->x_pin, out);
        else {
            x->x_softpwmval = (int)((t_float)out/1.023+0.5);
        }
    } else {
        post("disis_gpio: pwm messages can be only sent to an opened pin with togglepwm enabled");
    }
}

static void disis_gpio_togglesoftpwm(t_disis_gpio *x, t_float state)
{
    if (x->x_fdvalue != -1) {
        if (x->x_softpwm == (int)state) {
            if (x->x_softpwm)
                post("disis_gpio: soft pwm already enabled");
            else
                post("disis_gpio: soft pwm already disabled");
        } else {
            if (disis_gpio_isvalidpin(x->x_pin) == 2) {
                if (state && x->x_pwm) {
                    disis_gpio_togglepwm(x, 0);
                    post("disis_gpio: disabling hardware pwm");
                }
                x->x_softpwm = (int)state;
                if (x->x_softpwm) {
                    x->x_softpwm_thread = 0;
                    x->x_softpwmval = 0;
                    disisSoftPwmCreate (&x->x_params);
                }
                else {
                    x->x_softpwm_thread = 1;
                    while (x->x_softpwm_thread != 2)
                        usleep(100);
                    x->x_softpwm_thread = -1;
                }
            }
        }    
    }
}

static void disis_gpio_togglepwm(t_disis_gpio *x, t_float state)
{
    if (x->x_fdvalue != -1 && x->x_pin == 18) {
        if (x->x_pwm == (int)state) {
            if (x->x_pwm)
                post("disis_gpio: pwm already enabled");
            else
                post("disis_gpio: pwm already disabled");
        } else {
            if (disis_gpio_isvalidpin(x->x_pin) == 2) {
                if (state && x->x_softpwm) {
                    disis_gpio_togglesoftpwm(x, 0);
                    post("disis_gpio: disabling software pwm");
                }
                x->x_pwm = (int)state;                   
                if (x->x_pwm) {
                    pinMode(18, PWM_OUTPUT);
                }
                else {
                    pinMode(x->x_pin, OUTPUT);
                }
            }
        }    
    } else {
        post("disis_gpio: hardware pwm can be only enabled on the exported and opened pin 18");
    }
}

/*static void disis_gpio_pwmrange(t_disis_gpio *x, t_float r)
{
    if (x->x_fdvalue != -1 && x->x_pwm && x->x_pin == 18) {
        pwmSetRange((int)r);
        x->x_pwmrange = (int)r;
    } else {
        post("disis_gpio: you can toggle pwm only on opened pin 18 with togglepwm enabled");
    }
}*/

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

static void disis_gpio_free(t_disis_gpio *x) {
    disis_gpio_unexport(x);
}

static void *disis_gpio_new(t_floatarg f)
{
    if (!disis_gpio_isvalidpin((int)f)) return(NULL);
    if (geteuid () != 0)
    {
        post("error: disis_gpio external requires pd-l2ork to be run with root privileges. You can achieve this by doing 'sudo pd-l2ork'. Alternately, if running pd-l2ork remotely via ssh use 'sudo -E pd-l2ork' to preserve the enviroment.") ;
        return(NULL);
    }
    //char buf[FILENAME_MAX];
    //canvas_makefilename(glist_getcanvas((t_glist*)canvas_getcurrent()), "@pd_extra/disis_gpio/chown_gpio&", buf, FILENAME_MAX);
    //if (system(buf) < 0) { // first to adjust permissions for /sys/class/gpio so that we can export
    //    post("disis_gpio: failed setting permissions to /sys/class/gpio folder");
    //    return(NULL);
    //}
    t_disis_gpio *x = (t_disis_gpio *)pd_new(disis_gpio_class);
    x->x_out1 = outlet_new(&x->x_obj, gensym("float"));
    x->x_out2 = outlet_new(&x->x_obj, gensym("float"));
    x->x_fdvalue = -1;
    x->x_pin = f;
    x->x_export = 0;
    x->x_dir = 0;
    x->x_pwm = 0;
    x->x_softpwm_thread = -1;
 
    x->x_params.p_pin = &(x->x_pin);
    x->x_params.p_val = &(x->x_softpwmval);
    x->x_params.p_thread = &(x->x_softpwm_thread);

    wiringPiSetupGpio();
    //x->x_pwmrange = 0;
    //x->x_chown = gensym(buf);
    return (x);
}


void disis_gpio_setup(void)
{
    disis_gpio_class = class_new(gensym("disis_gpio"), (t_newmethod)disis_gpio_new,
        (t_method)disis_gpio_free, sizeof(t_disis_gpio), 0, A_DEFFLOAT, 0);
    class_addmethod(disis_gpio_class, (t_method)disis_gpio_export, gensym("export"), 
        A_DEFFLOAT, 0);
    class_addmethod(disis_gpio_class, (t_method)disis_gpio_unexport, gensym("unexport"), 0);
    class_addmethod(disis_gpio_class, (t_method)disis_gpio_direction, gensym("direction"), 
        A_DEFSYMBOL, 0);
    class_addmethod(disis_gpio_class, (t_method)disis_gpio_open, gensym("open"), 0);
    class_addmethod(disis_gpio_class, (t_method)disis_gpio_close, gensym("close"), 0);
    class_addmethod(disis_gpio_class, (t_method)disis_gpio_pwm, gensym("pwm"), 
        A_FLOAT, 0);
    class_addmethod(disis_gpio_class, (t_method)disis_gpio_togglepwm, gensym("togglepwm"), 
        A_FLOAT, 0);
    class_addmethod(disis_gpio_class, (t_method)disis_gpio_togglesoftpwm, gensym("togglesoftpwm"), 
        A_FLOAT, 0);
    //class_addmethod(disis_gpio_class, (t_method)disis_gpio_pwmrange, gensym("pwmrange"), 
    //    A_FLOAT, 0);
    class_addfloat(disis_gpio_class, disis_gpio_float);
    class_addbang(disis_gpio_class, disis_gpio_bang);
}
