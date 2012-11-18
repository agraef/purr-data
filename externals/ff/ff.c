/* forcefeedback externals for linux pd
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

 * the api is unstable. read the README for details.

 *  objects:  
 * ff-constant [device direction duration level ] 
 * ff-periodic [device direction duration level ] 
 * ff-spring   [device duration right-levelx left-level-x right-levely left-level-y] 
 * ff-friction [device duration right-levelx left-level-x right-levely left-level-y] 

 * additional methods for all objects: 
 * bang       :starts 
 * stop       :stops an effect 
 * delay      :sets the delay before the effect is activated 
 * interval   :sets the time to wait before an effect can be re-activated 

 * methods for periodic effects
 * waveform   : square|triangle|sine|saw_up|saw_down
 * period     : period time in ms
 * offset     :
 * phase      :
 *
 * methods for constant and periodic effects: 
 * envelope   : start-level attack end-level decay
 *
 * methods for spring and friction effects:
 * right-coeff :
 * left-coeff  :
 * deadband   :
 * center      : 
 *
 */




#include "m_pd.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include "input.h"



#define FF_DEVICE    "/dev/input/event0"

#define BITS_PER_LONG (sizeof(long) * 8)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)


static t_class *ffConstant_class;
static t_class *ffPeriodic_class;
static t_class *ffFriction_class;
static t_class *ffSpring_class;
static t_class *ffGain_class;
static t_class *ffAutocenter_class;


typedef struct _ff_device {
	char name[64];
	int max_fx;
	int loaded_fx;
} t_ff_device;

static t_ff_device ff_dev[4];




typedef struct _ff {
	t_object x_obj;
	int ff_fd;
	struct ff_effect effects;
	struct input_event do_that;
	unsigned int device;
} t_ff;


struct _waveshapes {
	char* wave;
	unsigned short number;
        } waves[]={{"square",FF_SQUARE}, 
		   {"triangle",FF_TRIANGLE}, 
		   {"sine",FF_SINE}, 
		   {"saw_up",FF_SAW_UP}, 
		   {"saw_down",FF_SAW_DOWN}, 
		   {NULL,0}};

/********************************************************************************************************

general ff methods

*********************************************************************************************************/

void ff_bang(t_ff *x)
{
	if (x->ff_fd < 0) return;
	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	x->do_that.type = EV_FF;
	x->do_that.code = x->effects.id;
	x->do_that.value = 1;

	if (write(x->ff_fd, (const void*) &x->do_that, sizeof(x->do_that)) == -1) {
		perror("Play effect error");
	}
	outlet_float(x->x_obj.ob_outlet, (t_float) x->effects.id);
}

void ff_stop(t_ff *x)
{

	if (x->ff_fd < 0) return;
	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	/* is it still playing ? */

	x->do_that.type = EV_FF_STATUS;
	x->do_that.code = x->effects.id;

	if ((read(x->ff_fd, (void *) &x->do_that, sizeof(x->do_that)))  == -1) {
		perror("couldn't read status of effect");
	}

	if (x->do_that.value == FF_STATUS_PLAYING) {

		x->do_that.type = EV_FF;
		x->do_that.code = x->effects.id;
		x->do_that.value = 0;
		
		if ((write(x->ff_fd, (const void*) &x->do_that, sizeof(x->do_that))) == -1) {
			perror("Stop effect error");
		}
	}
}

void ff_delay(t_ff *x, t_floatarg delay)
{
	if (x->ff_fd < 0) return;
	x->effects.replay.delay = (unsigned short) delay;
 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("Upload effects error");
	}
}

void ff_interval(t_ff *x, t_floatarg interval)
{
	if (x->ff_fd < 0) return;
	x->effects.trigger.interval = (unsigned short) interval;

 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("Upload effects error");
	}
}



void ff_duration(t_ff *x, t_floatarg duration )
{
	if (x->ff_fd < 0) return;
	
	x->effects.replay.length = (unsigned short) duration;

 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("Upload effects error");
	}
}

void ff_direction(t_ff *x,  t_floatarg direction)
{
	if (x->ff_fd < 0) return;
	unsigned short shortdirection;
	shortdirection = (unsigned short)(direction * 182.044444); /*map degrees to 0-0xFFFF */
	
	x->effects.direction = shortdirection;

 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}
	
	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		error("Upload effects error");
	}
}

void ff_unload(t_ff *x)
{

	if (x->ff_fd < 0) return;

 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}
	
	ff_stop(x);

	/* delete effect from the stick */
	if (ioctl(x->ff_fd, EVIOCRMFF, x->effects.id) == -1) {
		perror("deleting effect");
		return;
	}

	x->effects.id = -1;
	ff_dev[x->device].loaded_fx = (ff_dev[x->device].loaded_fx == 0 ? 0 : --ff_dev[x->device].loaded_fx);
	outlet_float(x->x_obj.ob_outlet, (t_float) x->effects.id);

}

void ff_load(t_ff *x)
{

	if (x->ff_fd < 0) return;
	if (x->effects.id != -1) {
		post("effect is allready loaded");
		return;
	}

	if (ff_dev[x->device].loaded_fx == ff_dev[x->device].max_fx) {
		post("maximum number of fx is loaded, you have to unload one first");
		return;
	}

	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("Upload effects error");
		return;
	}
	ff_dev[x->device].loaded_fx++;
	outlet_float(x->x_obj.ob_outlet, (t_float) x->effects.id);
}

void ff_free(t_ff *x)
{

	if (x->ff_fd < 0) return;

	if (x->effects.id != -1) {

		/* stop effect */
		ff_stop(x);
		
		/* delete effect from the stick */
		ff_unload(x);
	}

	/* close device */

	close(x->ff_fd);

}





/********************************************************************************************************

ff-constant methods

*********************************************************************************************************/


void ffConstant_level(t_ff *x,  t_floatarg level)
{
	if (x->ff_fd < 0) return;

	short shortlevel;
	level = (level > 1 ? 1:level); 
 	level = (level < -1 ? -1:level); 
	shortlevel = (short) (level * 32767 );   /*map level -1 to 1 to signed short range */

	x->effects.u.constant.level = shortlevel;

 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("level update error:");
	}
}


void ffConstant_envelope(t_ff *x, t_floatarg startlevel, t_floatarg startduration, t_floatarg endlevel, t_floatarg endduration)
{
	if (x->ff_fd < 0) return;
	unsigned short shortattack,shortdecay;
	startlevel = (startlevel > 1 ? 1:startlevel); 
 	startlevel = (startlevel < 0 ? 0:startlevel); 
	endlevel = (endlevel > 1 ? 1:endlevel); 
 	endlevel = (endlevel < 0 ? 0:endlevel); 
	shortattack = (unsigned short) (startlevel * 65534 );   /*map level 0 to 1 to unsigned short range */
	shortdecay = (unsigned short) (endlevel * 65534);
	x->effects.u.constant.envelope.attack_level = shortattack;
	x->effects.u.constant.envelope.fade_level = shortdecay;
	x->effects.u.constant.envelope.attack_length = (unsigned short) startduration;
	x->effects.u.constant.envelope.fade_length = (unsigned short) endduration;

 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("Upload effects error");
	}

}


void *ffConstant_new(t_floatarg device,t_floatarg direction,t_floatarg duration, t_floatarg level)
{

	unsigned short shortdirection,shortduration;
	short shortlevel;
	unsigned long features[4];	
	int device_number;

	t_ff *x = (t_ff *)pd_new(ffConstant_class);

	device = (device > 4 ? 4:device);
	device_number= (int)(device < 0 ? 0:device);

	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("direction"));
	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("duration"));
	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("level"));

	outlet_new(&x->x_obj, &s_float);

	if ((x->ff_fd=open((char *) ff_dev[device_number].name, O_RDWR | O_NONBLOCK)) < 0) {
		error("ff-lib:couldn't open %s, no effect will happen",ff_dev[device_number].name);
		return (void *) x;
	}

	if ((ioctl(x->ff_fd, EVIOCGBIT(EV_FF, sizeof(unsigned long) * 4), features)) == -1) {
		perror("\nCouldn't determine available ff-effects \n FF probably won't work");
		close(x->ff_fd);
		return (void *) x;
	}

	if (!test_bit(FF_CONSTANT, features)) {
		error("Constant force effect doesn't seem to be supported\n"
		      "the external won't do anything");
		close(x->ff_fd);
		x->ff_fd = -1;
		return (void *) x;
	}


	shortdirection = (unsigned short)(direction * 182.044444); /*map degrees to 0-0xFFFF */
	shortduration = (unsigned short) duration;
 	level = (level > 1 ? 1:level); 
 	level = (level < -1 ? -1:level); 
	shortlevel = (short) (level * 32767 );   /*map level -1 to 1 to signed short range */

	x->effects.type = FF_CONSTANT;
	x->effects.id = -1;
	x->effects.direction = shortdirection;	
	x->effects.u.constant.level =shortlevel;
	x->effects.u.constant.envelope.attack_length = 0x000;
	x->effects.u.constant.envelope.attack_level = 0;
	x->effects.u.constant.envelope.fade_length = 0x000;
	x->effects.u.constant.envelope.fade_level = 0;
	x->effects.trigger.button = 0;
	x->effects.trigger.interval = 0;
	x->effects.replay.length =shortduration;  
	x->effects.replay.delay = 0;
	x->device = device_number;
	ff_load(x);
	return (void*)x;
}



/********************************************************************************************************

ff-periodic methods

*********************************************************************************************************/




void ffPeriodic_level(t_ff *x,  t_floatarg level)
{
	if (x->ff_fd < 0) return;
	short shortlevel;
	level = (level > 1 ? 1:level); 
 	level = (level < -1 ? -1:level); 
	shortlevel = (short) (level * 32767 );   /*map level -1 to 1 to signed short range */
	x->effects.u.periodic.magnitude = shortlevel;

 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("Upload effects error");
	}
}


void ffPeriodic_waveform(t_ff *x, t_symbol* waveform)
{
	if (x->ff_fd < 0) return;
	unsigned short shortwave = 0;
	int n = 0;

	while (waves[n].wave) {
		if (strcmp( waveform->s_name,waves[n].wave)) shortwave = waves[n].number;
		n++;
	}
	
	x->effects.u.periodic.waveform = shortwave;

 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("Upload effects error");
	}

}

void ffPeriodic_period(t_ff *x,  t_floatarg period)
{
	if (x->ff_fd < 0) return;
	x->effects.u.periodic.period = (unsigned short) period;
	
	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("Upload effects error");
	}
}

void ffPeriodic_offset(t_ff *x,  t_floatarg offset)
{
	if (x->ff_fd < 0) return;
	short shortoffset;
	offset = (offset > 1 ? 1:offset); 
 	offset = (offset < -1 ? -1:offset); 
	shortoffset = (short) (offset * 32767 );   /*map level -1 to 1 to signed short range */
	x->effects.u.periodic.offset = shortoffset;

 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}
	
	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("Upload effects error");
	}
}

void ffPeriodic_phase(t_ff *x,  t_floatarg phase)
{	
	if (x->ff_fd < 0) return;
	unsigned short shortphase;
	shortphase = (unsigned short)(phase * 182.044444); /*map degrees to 0-0xFFFF */
	x->effects.u.periodic.phase = shortphase;

 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}
	
	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		error("Upload effects error");
	}

}

void ffPeriodic_envelope(t_ff *x, t_floatarg startlevel, t_floatarg startduration, t_floatarg endlevel, t_floatarg endduration)
{
	if (x->ff_fd < 0) return;
	unsigned short shortattack,shortdecay;
	startlevel = (startlevel > 1 ? 1:startlevel); 
 	startlevel = (startlevel < 0 ? 0:startlevel); 
	endlevel = (endlevel > 1 ? 1:endlevel); 
 	endlevel = (endlevel < 0 ? 0:endlevel); 
	shortattack = (unsigned short) (startlevel * 65534 );   /*map level 0 to 1 to unsigned short range */
	shortdecay = (unsigned short) (endlevel * 65534);
	x->effects.u.periodic.envelope.attack_level = shortattack;
	x->effects.u.periodic.envelope.fade_level = shortdecay;
	x->effects.u.periodic.envelope.attack_length = (unsigned short) startduration;
	x->effects.u.periodic.envelope.fade_length = (unsigned short) endduration;

 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("Upload effects error");
	}

}




void *ffPeriodic_new(t_floatarg device,t_floatarg direction,t_floatarg duration, t_floatarg level)
{
	unsigned short shortdirection,shortduration;
	short shortlevel;
	unsigned long features[4];	
	int device_number;

	t_ff *x = (t_ff *)pd_new(ffPeriodic_class);	

	device = (device > 4 ? 4:device);
	device_number= (int)(device < 0 ? 0:device);

	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("direction"));
	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("duration"));
	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("level"));

	outlet_new(&x->x_obj, &s_float);

	if ((x->ff_fd=open((char *) ff_dev[device_number].name, O_RDWR | O_NONBLOCK)) < 0 ){
		error("ff-lib:couldn't open %s, no effect will happen",ff_dev[device_number].name);
		return (void *) x;
	}

	if ((ioctl(x->ff_fd, EVIOCGBIT(EV_FF, sizeof(unsigned long) * 4), features)) == -1) {
		error("Couldn't determine available ff-effects \n FF probably won't work");
		close(x->ff_fd);
		return (void *) x;
	}

	if (!test_bit(FF_PERIODIC, features)) {
		error("Periodic effect doesn't seem to be supported\n"
		      "the external won't do anything");
		close(x->ff_fd);
		x->ff_fd = -1;
		return (void *) x;
	}
	
	shortdirection = (unsigned short)(direction * 182.044444); /*map degrees to 0-0xFFFF */
	shortduration = (unsigned short) duration;
 	level = (level > 1 ? 1:level); 
 	level = (level < -1 ? -1:level); 
	shortlevel = (short) (level * 32767 );   /*map level -1 to 1 to signed short range */

	x->effects.type = FF_PERIODIC;
	x->effects.id = -1;
	x->effects.direction = shortdirection;	
	x->effects.u.periodic.waveform = FF_SQUARE;
	x->effects.u.periodic.period = 1000;
	x->effects.u.periodic.magnitude =  shortlevel;
	x->effects.u.periodic.offset = 0;
	x->effects.u.periodic.phase = 0;
	x->effects.u.periodic.envelope.attack_length = 0x000;
	x->effects.u.periodic.envelope.attack_level = 0;
	x->effects.u.periodic.envelope.fade_length = 0x000;
	x->effects.u.periodic.envelope.fade_level = 0;
	x->effects.trigger.button = 0;
	x->effects.trigger.interval = 0;
	x->effects.replay.length = shortduration;  
	x->effects.replay.delay = 0;
	x->device = device_number;
	
	ff_load(x);

	return (void*)x;
}



/********************************************************************************************************

ff-spring and ff-friction methods

*********************************************************************************************************/


void ffCondition_setLevel(t_ff *x,  t_floatarg level, int axis)
{
	unsigned short shortlevel;

	
	if (x->ff_fd < 0) return;
	
	level = (level > 1 ? 1:level); 
 	level = (level < 0 ? 0:level); 
	shortlevel = (unsigned short) (level * 65534 );   /*map level 0 to 1 to unsigned short range */
	
	switch (axis) {
	case 0:	x->effects.u.condition[0].right_saturation = shortlevel;
		break;
	case 1:	x->effects.u.condition[0].left_saturation = shortlevel;
		break;
	case 2:	x->effects.u.condition[1].right_saturation = shortlevel;
		break;
	case 3:	x->effects.u.condition[1].left_saturation = shortlevel;
		break;
	}

 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("Upload effects error");
	}


}



void ffCondition_setCoeff(t_ff *x,  t_floatarg coeff, int axis)
{
	short shortcoeff;
	if (x->ff_fd < 0) return;
	
	coeff = (coeff > 1 ? 1:coeff); 
 	coeff = (coeff < -1 ? -1:coeff); 
	shortcoeff = (short) (coeff * 32767 );   /*map level -1 to 1 to unsigned short range */
	switch (axis) {
	case 0:	x->effects.u.condition[0].right_coeff = shortcoeff;
		break;

	case 1:	x->effects.u.condition[0].left_coeff = shortcoeff;
		break;

	case 2:	x->effects.u.condition[1].right_coeff = shortcoeff;
		break;


	case 3:	x->effects.u.condition[1].left_coeff = shortcoeff;
		break;
	}

 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("Upload effects error");
	}

}


void ffCondition_deadband(t_ff *x,  t_floatarg deadband, int axis) 
{
	if (x->ff_fd < 0) return;
	x->effects.u.condition[axis].deadband = (unsigned short)deadband;

 	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("Upload effects error");
	}

}

void ffCondition_center(t_ff *x,  t_floatarg center, int axis)
{
	if (x->ff_fd < 0) return;
	x->effects.u.condition[axis].center = (short)center;

	if (x->effects.id == -1) {
		post("effect is not loaded, use a \"load\" message to upload to device");
		return;
	}

	if (ioctl(x->ff_fd, EVIOCSFF, &x->effects) == -1) {
		perror("Upload effects error");
	}


}

void ffCondition_rightLevel(t_ff *x,  t_floatarg rightLevel)
{

	ffCondition_setLevel(x,rightLevel,0);
	
}

void ffCondition_leftLevel(t_ff *x,  t_floatarg leftLevel)
{
	ffCondition_setLevel(x,leftLevel,1);

}
void ffCondition_upLevel(t_ff *x,  t_floatarg upLevel)
{
	ffCondition_setLevel(x,upLevel,3);
	
}

void ffCondition_downLevel(t_ff *x,  t_floatarg downLevel)
{
	ffCondition_setLevel(x,downLevel,2);

}

void ffCondition_rightCoeff(t_ff *x,  t_floatarg rightCoeff)
{
	ffCondition_setCoeff(x,rightCoeff,0);
}

void ffCondition_leftCoeff(t_ff *x,  t_floatarg leftCoeff)
{
	ffCondition_setCoeff(x,leftCoeff,1);
}
void ffCondition_upCoeff(t_ff *x,  t_floatarg upCoeff)
{
	ffCondition_setCoeff(x,upCoeff,3);
}

void ffCondition_downCoeff(t_ff *x,  t_floatarg downCoeff)
{
	ffCondition_setCoeff(x,downCoeff,2);
}


void ffCondition_deadbandx(t_ff *x,  t_floatarg deadband) 
{
	ffCondition_deadband(x,deadband,0);
}

void ffCondition_deadbandy(t_ff *x,  t_floatarg deadband) 
{
	ffCondition_deadband(x,deadband,1);
}


void ffCondition_centerx(t_ff *x,  t_floatarg center)
{
	ffCondition_center(x,center,0);
}


void ffCondition_centery(t_ff *x,  t_floatarg center)
{
	ffCondition_center(x,center,1);
}



void *ffFriction_new(t_floatarg device,t_floatarg duration, t_floatarg rightLevel, t_floatarg leftLevel,
		     t_floatarg upLevel, t_floatarg downLevel)
{
	unsigned short shortduration,shortrightLevel,shortleftLevel,shortupLevel,shortdownLevel;
	unsigned long features[4];	
	int device_number;

	t_ff *x = (t_ff *)pd_new(ffFriction_class);	

	device = (device > 4 ? 4:device);
	device_number= (int)(device < 0 ? 0:device);

	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("duration"));
	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("right-level"));
	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("left-level"));
	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("up-level"));
	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("down-level"));

	outlet_new(&x->x_obj, &s_float);

	if ((x->ff_fd=open((char *) ff_dev[device_number].name, O_RDWR | O_NONBLOCK)) < 0) {
		error("ff-lib:couldn't open %s, no effect will happen",ff_dev[device_number].name);
		return (void *) x;
	}

	if ((ioctl(x->ff_fd, EVIOCGBIT(EV_FF, sizeof(unsigned long) * 4), features)) == -1) {
		error("Couldn't determine available ff-effects \n FF probably won't work");
		close(x->ff_fd);
		return (void *) x;
	}

	if (!test_bit(FF_FRICTION, features)) {
		error("Friction effect doesn't seem to be supported\n"
		      "the external won't do anything");
		close(x->ff_fd);
		x->ff_fd = -1;
		return (void *) x;
	}


	shortduration = (unsigned short) duration;

 	rightLevel = (rightLevel > 1 ? 1:rightLevel); 
 	rightLevel = (rightLevel < 0 ? 0:rightLevel); 
	shortrightLevel = (unsigned short) (rightLevel * 65534 );   /*map level 0 to 1 to unsigned short range */

	leftLevel = (leftLevel > 1 ? 1:leftLevel); 
 	leftLevel = (leftLevel < 0 ? 0:leftLevel); 
	shortleftLevel = (unsigned short) (leftLevel * 65534 );   /*map level 0 to 1 to unsigned short range */

	upLevel = (upLevel > 1 ? 1:upLevel); 
 	upLevel = (upLevel < 0 ? 0:upLevel); 
	shortupLevel = (unsigned short) (upLevel * 65534 );   /*map level 0 to 1 to unsigned short range */
	downLevel = (downLevel > 1 ? 1:downLevel); 
 	downLevel = (downLevel < 0 ? 0:downLevel); 
	shortdownLevel = (unsigned short) (downLevel * 65534 );   /*map level 0 to 1 to unsigned short range */


	x->effects.type = FF_FRICTION;
	x->effects.id = -1;
	x->effects.u.condition[0].right_saturation = shortrightLevel;
	x->effects.u.condition[0].left_saturation = shortleftLevel;
	x->effects.u.condition[0].right_coeff = 0x8000;
	x->effects.u.condition[0].left_coeff = 0x8000;
	x->effects.u.condition[0].deadband = 0;
	x->effects.u.condition[0].center = 0;
	x->effects.u.condition[1].right_saturation = shortdownLevel;
	x->effects.u.condition[1].left_saturation = shortupLevel;
	x->effects.u.condition[1].right_coeff = 0x8000;
	x->effects.u.condition[1].left_coeff = 0x8000;
	x->effects.u.condition[1].deadband = 0;
	x->effects.u.condition[1].center = 0;
	x->effects.trigger.button = 0;
	x->effects.trigger.interval = 0;
	x->effects.replay.length = shortduration;  
	x->effects.replay.delay = 0;
	x->device = device_number;


	ff_load(x);

	return (void*)x;
}

void *ffSpring_new(t_floatarg device,t_floatarg duration, t_floatarg rightLevel, t_floatarg leftLevel,
		     t_floatarg upLevel, t_floatarg downLevel)
{
	unsigned short shortduration,shortrightLevel,shortleftLevel,shortupLevel,shortdownLevel;
	unsigned long features[4];	
	int device_number;
	
	t_ff *x = (t_ff *)pd_new(ffFriction_class);

	device = (device > 4 ? 4:device);
	device_number= (int)(device < 0 ? 0:device);

	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("duration"));
	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("right-level"));
	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("left-level"));
	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("up-level"));
	inlet_new(&x->x_obj,
		  &x->x_obj.ob_pd,
		  gensym("float"),
		  gensym("down-level"));

	outlet_new(&x->x_obj, &s_float);

	if ((x->ff_fd=open((char *) ff_dev[device_number].name, O_RDWR | O_NONBLOCK)) < 0) {
		error("ff-lib:couldn't open %s, no effect will happen",ff_dev[device_number].name);
		return (void *) x;
	}

	if ((ioctl(x->ff_fd, EVIOCGBIT(EV_FF, sizeof(unsigned long) * 4), features)) == -1) {
		error("Couldn't determine available ff-effects \n FF probably won't work");
		close(x->ff_fd);
		return (void *) x;
	}

	if (!test_bit(FF_SPRING, features)) {
		error("Spring effect doesn't seem to be supported\n"
		      "the external won't do anything");
		close(x->ff_fd);
		x->ff_fd = -1;
		return (void *) x;
	}

	shortduration = (unsigned short) duration;
 
	leftLevel = (leftLevel > 1 ? 1:leftLevel); 
 	leftLevel = (leftLevel < 0 ? 0:leftLevel); 
	shortleftLevel = (unsigned short) (leftLevel * 65534 );   /*map level 0 to 1 to unsigned short range */
	rightLevel = (rightLevel > 1 ? 1:rightLevel); 
 	rightLevel = (rightLevel < 0 ? 0:rightLevel); 
	shortrightLevel = (unsigned short) (rightLevel * 65534 );   /*map level 0 to 1 to unsigned short range */
	upLevel = (upLevel > 1 ? 1:upLevel); 
 	upLevel = (upLevel < 0 ? 0:upLevel); 
	shortupLevel = (unsigned short) (upLevel * 65534 );   /*map level 0 to 1 to unsigned short range */
	downLevel = (downLevel > 1 ? 1:downLevel); 
 	downLevel = (downLevel < 0 ? 0:downLevel); 
	shortdownLevel = (unsigned short) (downLevel * 65534 );   /*map level 0 to 1 to unsigned short range */


	x->effects.type = FF_SPRING;
	x->effects.id = -1;
	x->effects.u.condition[0].right_saturation = shortrightLevel;
	x->effects.u.condition[0].left_saturation = shortleftLevel;
	x->effects.u.condition[0].right_coeff = 0x8000;
	x->effects.u.condition[0].left_coeff = 0x8000;
	x->effects.u.condition[0].deadband = 0;
	x->effects.u.condition[0].center = 0;
	x->effects.u.condition[1].right_saturation = shortdownLevel;
	x->effects.u.condition[1].left_saturation = shortupLevel;
	x->effects.u.condition[1].right_coeff = 0x8000;
	x->effects.u.condition[1].left_coeff = 0x8000;
	x->effects.u.condition[1].deadband = 0;
	x->effects.u.condition[1].center = 0;
	x->effects.trigger.button = 0;
	x->effects.trigger.interval = 0;
	x->effects.replay.length = shortduration;  
	x->effects.replay.delay = 0;
	x->device = device_number;

	ff_load(x);

	return (void*)x;
}

/********************************************************************************************************

ff-gain methods

*********************************************************************************************************/


void ffGain_set(t_ff *x, t_floatarg gain)
{
	gain = (gain > 1 ? 1:gain);
	gain = (gain < 0 ? 0:gain);

	x->do_that.type = EV_FF;
	x->do_that.code = FF_GAIN;
	x->do_that.value = (unsigned int)(65536.0 * gain);
	if (x->ff_fd > 0)
		if ((write(x->ff_fd, (const void*) &x->do_that, sizeof(x->do_that))) == -1)
			error("ff-lib: couldn't set gain");

}





void *ffGain_new(t_floatarg device,t_floatarg gain)
{
	int device_number;
	unsigned short shortgain;	
	t_ff *x = (t_ff *)pd_new(ffGain_class);

	device = (device > 4 ? 4:device);
	device_number= (int)(device < 0 ? 0:device);

	gain = (gain > 1 ? 1:gain);
	gain = (gain < 0 ? 0:gain);
	shortgain = (unsigned short) (gain * 65536);
	if ((x->ff_fd=open((char *) ff_dev[device_number].name, O_RDWR | O_NONBLOCK)) < 0) {
		error("ff-lib:couldn't open %s, no effect will happen",ff_dev[device_number].name);
		return (void *) x;
	}
	x->do_that.type = EV_FF;
	x->do_that.code = FF_GAIN;
	x->do_that.value = shortgain;

	if ((write(x->ff_fd, (const void*) &x->do_that, sizeof(x->do_that))) == -1)
		error("ff-lib: couldn't set gain");
	return (void*)x;
}


/********************************************************************************************************

ff-autocenter methods

*********************************************************************************************************/

void ffAutocenter_set(t_ff *x, t_floatarg autocenter)
{
	
	autocenter = (autocenter > 1 ? 1:autocenter);
	autocenter = (autocenter < -1 ? -1:autocenter);

	x->do_that.type = EV_FF;
	x->do_that.code = FF_AUTOCENTER;
	x->do_that.value = (short)(32767.0 * autocenter);
	if (x->ff_fd > 0) 
		if ((write(x->ff_fd, (const void*) &x->do_that, sizeof(x->do_that))) == -1)
			error("ff-lib:couldn't set autocenter");

}


void *ffAutocenter_new(t_floatarg device,t_floatarg autocenter)
{
	int device_number;
	t_ff *x = (t_ff *)pd_new(ffAutocenter_class);
	device = (device > 4 ? 4:device);
	device_number= (int)(device < 0 ? 0:device);

	autocenter = (autocenter > 1 ? 1:autocenter);
	autocenter = (autocenter < 0 ? 0:autocenter);

	if ((x->ff_fd=open((char *) ff_dev[device_number].name, O_RDWR | O_NONBLOCK)) < 0) {
		error("ff-lib:couldn't open %s, no effect will happen", ff_dev[device_number].name);
		return (void *) x;
	}
	x->do_that.type = EV_FF;
	x->do_that.code = FF_AUTOCENTER;
	x->do_that.value = (short)(32767.0 * autocenter );
	
	if ((write(x->ff_fd, (const void*) &x->do_that, sizeof(x->do_that))) == -1)
		error("ff-lib:couldn't set autocenter");
	return (void *) x;

}








/******************************************************************************************
initialisation functions
*******************************************************************************************/

void add_general_ff_methods(t_class* ff_class)
{
	class_addbang(ff_class,ff_bang);
	class_addmethod(ff_class, (t_method)ff_stop,gensym("stop"),0);
	class_addmethod(ff_class, (t_method)ff_duration,gensym("duration"),A_DEFFLOAT,0);
	class_addmethod(ff_class, (t_method)ff_interval,gensym("interval"),A_DEFFLOAT,0);
	class_addmethod(ff_class, (t_method)ff_delay,gensym("delay"),A_DEFFLOAT,0);
	class_addmethod(ff_class, (t_method)ff_load,gensym("load"),0);
	class_addmethod(ff_class, (t_method)ff_unload,gensym("unload"),0);
}

void init_ffConstant(void)
{

	ffConstant_class = class_new(gensym("ff-constant"),
			     (t_newmethod)ffConstant_new,
			     (t_method)ff_free,
			     sizeof(t_ff),
			     CLASS_DEFAULT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     0);

	add_general_ff_methods(ffConstant_class);
	class_addmethod(ffConstant_class, 
			(t_method)ff_direction,
			gensym("direction"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffConstant_class,
			(t_method)ffConstant_level,
			gensym("level"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffConstant_class,
			(t_method)ffConstant_envelope,
			gensym("envelope"),
			A_DEFFLOAT,
			A_DEFFLOAT,
			A_DEFFLOAT,
			A_DEFFLOAT,
			0);

}

void init_ffPeriodic(void)
{
	ffPeriodic_class = class_new(gensym("ff-periodic"),
			     (t_newmethod)ffPeriodic_new,
			     (t_method)ff_free,
			     sizeof(t_ff),
			     CLASS_DEFAULT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     0);
	add_general_ff_methods(ffPeriodic_class);
	class_addmethod(ffPeriodic_class, 
			(t_method)ff_direction,
			gensym("direction"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffPeriodic_class,
			(t_method)ffPeriodic_level,
			gensym("level"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffPeriodic_class,
			(t_method)ffPeriodic_envelope,
			gensym("envelope"),
			A_DEFFLOAT,
			A_DEFFLOAT,
			A_DEFFLOAT,
			A_DEFFLOAT,
			0);
	class_addmethod(ffPeriodic_class,
			(t_method)ffPeriodic_waveform,
			gensym("waveform"),
			A_DEFSYMBOL,
			0);
	class_addmethod(ffPeriodic_class,
			(t_method)ffPeriodic_period,
			gensym("period"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffPeriodic_class,
			(t_method)ffPeriodic_offset,
			gensym("offset"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffPeriodic_class,
			(t_method)ffPeriodic_phase,
			gensym("phase"),
			A_DEFFLOAT,
			0);




}

void init_ffSpring(void)
{
	ffSpring_class = class_new(gensym("ff-spring"),
			     (t_newmethod)ffSpring_new,
			     (t_method)ff_free,
			     sizeof(t_ff),
			     CLASS_DEFAULT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     0);
	add_general_ff_methods(ffSpring_class);

	class_addmethod(ffSpring_class,
			(t_method)ffCondition_rightLevel,
			gensym("right-level"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffSpring_class,
			(t_method)ffCondition_leftLevel,
			gensym("left-level"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffSpring_class,
			(t_method)ffCondition_upLevel,
			gensym("up-level"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffSpring_class,
			(t_method)ffCondition_downLevel,
			gensym("down-level"),
			A_DEFFLOAT,
			0);


	class_addmethod(ffSpring_class,
			(t_method)ffCondition_rightCoeff,
			gensym("right-coeff"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffSpring_class,
			(t_method)ffCondition_leftCoeff,
			gensym("left-coeff"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffSpring_class,
			(t_method)ffCondition_upCoeff,
			gensym("up-coeff"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffSpring_class,
			(t_method)ffCondition_downCoeff,
			gensym("down-coeff"),
			A_DEFFLOAT,
			0);

	class_addmethod(ffSpring_class,
			(t_method)ffCondition_deadbandx,
			gensym("deadband-x"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffSpring_class,
			(t_method)ffCondition_deadbandy,
			gensym("deadband-y"),
			A_DEFFLOAT,
			0);

	class_addmethod(ffSpring_class,
			(t_method)ffCondition_centerx,
			gensym("center-x"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffSpring_class,
			(t_method)ffCondition_centery,
			gensym("center-y"),
			A_DEFFLOAT,
			0);


}

void init_ffFriction(void)
{
	ffFriction_class = class_new(gensym("ff-friction"),
			     (t_newmethod)ffFriction_new,
			     (t_method)ff_free,
			     sizeof(t_ff),
			     CLASS_DEFAULT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     0);
	add_general_ff_methods(ffFriction_class);

	class_addmethod(ffFriction_class,
			(t_method)ffCondition_rightLevel,
			gensym("right-level"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffFriction_class,
			(t_method)ffCondition_leftLevel,
			gensym("left-level"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffFriction_class,
			(t_method)ffCondition_upLevel,
			gensym("up-level"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffFriction_class,
			(t_method)ffCondition_downLevel,
			gensym("down-level"),
			A_DEFFLOAT,
			0);

	class_addmethod(ffFriction_class,
			(t_method)ffCondition_rightCoeff,
			gensym("right-coeff"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffFriction_class,
			(t_method)ffCondition_leftCoeff,
			gensym("left-coeff"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffFriction_class,
			(t_method)ffCondition_upCoeff,
			gensym("up-coeff"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffFriction_class,
			(t_method)ffCondition_downCoeff,
			gensym("down-coeff"),
			A_DEFFLOAT,
			0);

	class_addmethod(ffFriction_class,
			(t_method)ffCondition_deadbandx,
			gensym("deadband-x"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffFriction_class,
			(t_method)ffCondition_centerx,
			gensym("center-x"),
			A_DEFFLOAT,
			0);


	class_addmethod(ffFriction_class,
			(t_method)ffCondition_deadbandy,
			gensym("deadband-y"),
			A_DEFFLOAT,
			0);
	class_addmethod(ffFriction_class,
			(t_method)ffCondition_centery,
			gensym("center-y"),
			A_DEFFLOAT,
			0);

}

void init_ffGain(void)

{
	ffGain_class = class_new(gensym("ff-gain"),
			     (t_newmethod)ffGain_new,
			     0,
			     sizeof(t_ff),
			     CLASS_DEFAULT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     0);
	
	class_addfloat(ffGain_class,(t_method)ffGain_set);

}

void init_ffAutocenter(void)
{
	ffAutocenter_class = class_new(gensym("ff-autocenter"),
			     (t_newmethod)ffAutocenter_new,
			     0,
			     sizeof(t_ff),
			     CLASS_DEFAULT,
			     A_DEFFLOAT,
			     A_DEFFLOAT,
			     0);
	
	class_addfloat(ffAutocenter_class,(t_method)ffAutocenter_set);
			
}


void ff_setup(void)
{
	/* open event device and determine available effects and memory */
	/* the externals themselves also check, this is just to give some info to the user on startup */


	char device_file_name[4][18];
	unsigned long features[4];
	int n_effects;	/* Number of effects the device can play at the same time */
	int j,ffdevice_count,fftest,fd;

	post("//////////////////////////////////////////\n"
	     "/////Force feedback external library///// \n"
	     "////Gerard van Dongen, gml@xs4all.nl//// \n"
	     "///testing for available ff devices////.\n"
	     "//////////////////////////////////////");


	ffdevice_count = 0;
	for (j=0;j<4;j++){
		fftest = 0;
		sprintf(device_file_name[j], "/dev/input/event%i",j);

		/* Open device */
		fd = open(device_file_name[j], O_RDWR | O_NONBLOCK);
		if (fd == -1) {
			continue;
		}
		post("Device %s opened\n", device_file_name[j]);

		/* Query device */
		if (ioctl(fd, EVIOCGBIT(EV_FF, sizeof(unsigned long) * 4), features) == -1) {
			error("Couldn't determine available ff-effects \n FF probablz won't work");
			close(fd);
			continue;
		}
		post("the following externals will work on %s",device_file_name[j]); 
		
		if (test_bit(FF_CONSTANT, features)) {
			post("ff-constant ");
			fftest++;
		}
		if (test_bit(FF_PERIODIC, features)) {
			post("ff-periodic ");
			fftest++;
		}
		if (test_bit(FF_SPRING, features)) {
			post("ff-spring ");
			fftest++;
		}
		if (test_bit(FF_FRICTION, features)) {
			post("ff-friction ");
			fftest++;
		}
		if (test_bit(FF_RUMBLE, features)) {
			post("The rumble effect is supported by the device,\n"
			     "but there is no external to control this in pd (yet) ");
			fftest++;
		}
		if (test_bit(FF_RAMP, features)) {
			post("The ramp effect is supported by the device,\n"
			     "but there is no external to control this in pd (yet) ");
			fftest++;
		}
		if (test_bit(FF_DAMPER, features)){
			post("The damper effect is supported by the device,\n"
			     "but there is no external to control this in pd (yet) ");
			fftest++;
		}
		if (test_bit(FF_INERTIA, features)){ 
			post("The inertia effect is supported by the device,\n"
			     "but there is no external to control this in pd (yet) ");
			fftest++;
		}


		if (ioctl(fd, EVIOCGEFFECTS, &n_effects) == -1) {
			error("Ioctl number of effects");
		}
		post("Number of simultaneous effects: %i",n_effects);

		close(fd);
		if (fftest != 0 && n_effects !=0) {
			ffdevice_count++;
			ff_dev[j].max_fx = n_effects;
			ff_dev[j].loaded_fx = 0;
			strncpy(ff_dev[j].name,device_file_name[j],64);
		}

	}

	if (ffdevice_count >0) 
		post("%i ff-device(s) found",ffdevice_count);
	else
		post("NO ff capable devices found");


	init_ffConstant();
	init_ffPeriodic();
	init_ffSpring();
	init_ffFriction();
	init_ffGain();
	init_ffAutocenter();
}




