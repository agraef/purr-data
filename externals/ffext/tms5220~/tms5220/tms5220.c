/**********************************************************************************************

     TMS5220 simulator

     Written for MAME by Frank Palazzolo
     With help from Neill Corlett
     Additional tweaking by Aaron Giles

***********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "tms5220.h"


/* Pull in the ROM tables */
#include "tms5220r.c"


/* these contain data that describes the 128-bit data FIFO */
#define FIFO_SIZE 16
static unsigned char fifo[FIFO_SIZE];
static int fifo_head;
static int fifo_tail;
static int fifo_count;
static int bits_taken;


/* these contain global status bits */
static int speak_external;
static int talk_status;
static int buffer_low;
static int buffer_empty;
static int irq_pin;

static void (*irq_func)(void);


/* these contain data describing the current and previous voice frames */
static unsigned short old_energy = 0;
static unsigned short old_pitch = 0;
static int old_k[10] = {0,0,0,0,0,0,0,0,0,0};

static unsigned short new_energy = 0;
static unsigned short new_pitch = 0;
static int new_k[10] = {0,0,0,0,0,0,0,0,0,0};


/* these are all used to contain the current state of the sound generation */
static unsigned short current_energy = 0;
static unsigned short current_pitch = 0;
static int current_k[10] = {0,0,0,0,0,0,0,0,0,0};

static unsigned short target_energy = 0;
static unsigned short target_pitch = 0;
static int target_k[10] = {0,0,0,0,0,0,0,0,0,0};

static int interp_count = 0;       /* number of interp periods (0-7) */
static int sample_count = 0;       /* sample number within interp (0-24) */
static int pitch_count = 0;

static int u[11] = {0,0,0,0,0,0,0,0,0,0,0};
static int x[10] = {0,0,0,0,0,0,0,0,0,0};

static int randbit = 0;


/* Static function prototypes */
static void process_command (void);
static int extract_bits (int count);
static int parse_frame (int removeit);
static void check_buffer_low (void);
static void cause_interrupt (void);


/*#define DEBUG_5220*/
#ifdef DEBUG_5220
    static FILE *f;
#endif


/**********************************************************************************************

     tms5220_reset -- resets the TMS5220

***********************************************************************************************/

void tms5220_reset (void)
{
    /* initialize the FIFO */
    memset (fifo, 0, sizeof (fifo));
    fifo_head = fifo_tail = fifo_count = bits_taken = 0;

    /* initialize the chip state */
    speak_external = talk_status = buffer_empty = irq_pin = 0;
    buffer_low = 1;

    /* initialize the energy/pitch/k states */
    old_energy = new_energy = current_energy = target_energy = 0;
    old_pitch = new_pitch = current_pitch = target_pitch = 0;
    memset (old_k, 0, sizeof (old_k));
    memset (new_k, 0, sizeof (new_k));
    memset (current_k, 0, sizeof (current_k));
    memset (target_k, 0, sizeof (target_k));

    /* initialize the sample generators */
    interp_count = sample_count = pitch_count = 0;
    randbit = 0;
    memset (u, 0, sizeof (u));
    memset (x, 0, sizeof (x));

    #ifdef DEBUG_5220
        f = fopen ("tms.log", "w");
    #endif
}



/**********************************************************************************************

     tms5220_reset -- reset the TMS5220

***********************************************************************************************/

void tms5220_set_irq (void (*func)(void))
{
    irq_func = func;
}


/**********************************************************************************************

     tms5220_data_write -- handle a write to the TMS5220

***********************************************************************************************/

void tms5220_data_write (int data)
{
    /* add this byte to the FIFO */
    if (fifo_count < FIFO_SIZE)
    {
        fifo[fifo_tail] = data;
        fifo_tail = (fifo_tail + 1) % FIFO_SIZE;
        fifo_count++;

        #ifdef DEBUG_5220
            if (f) fprintf (f, "Added byte to FIFO (size=%2d)\n", fifo_count);
        #endif
    }
    else
    {
        #ifdef DEBUG_5220
            if (f) fprintf (f, "Ran out of room in the FIFO!\n");
        #endif
    }

    /* update the buffer low state */
    check_buffer_low ();
}


/**********************************************************************************************

     tms5220_status_read -- read status from the TMS5220

	  From the data sheet:
        bit 0 = TS - Talk Status is active (high) when the VSP is processing speech data.
                Talk Status goes active at the initiation of a Speak command or after nine
                bytes of data are loaded into the FIFO following a Speak External command. It
                goes inactive (low) when the stop code (Energy=1111) is processed, or
                immediately by a buffer empty condition or a reset command.
        bit 1 = BL - Buffer Low is active (high) when the FIFO buffer is more than half empty.
                Buffer Low is set when the "Last-In" byte is shifted down past the half-full
                boundary of the stack. Buffer Low is cleared when data is loaded to the stack
                so that the "Last-In" byte lies above the half-full boundary and becomes the
                ninth data byte of the stack.
        bit 2 = BE - Buffer Empty is active (high) when the FIFO buffer has run out of data
                while executing a Speak External command. Buffer Empty is set when the last bit
                of the "Last-In" byte is shifted out to the Synthesis Section. This causes
                Talk Status to be cleared. Speed is terminated at some abnormal point and the
                Speak External command execution is terminated.

***********************************************************************************************/

int tms5220_status_read (void)
{
    /* clear the interrupt pin */
    irq_pin = 0;

    #ifdef DEBUG_5220
        if (f) fprintf (f, "Status read: TS=%d BL=%d BE=%d\n", talk_status, buffer_low, buffer_empty);
    #endif

    return (talk_status << 7) | (buffer_low << 6) | (buffer_empty << 5);
}



/**********************************************************************************************

     tms5220_ready_read -- returns the ready state of the TMS5220

***********************************************************************************************/

int tms5220_ready_read (void)
{
    return (fifo_count < FIFO_SIZE-1);
}



/**********************************************************************************************

     tms5220_int_read -- returns the interrupt state of the TMS5220

***********************************************************************************************/

int tms5220_int_read (void)
{
    return irq_pin;
}



/**********************************************************************************************

     tms5220_process -- fill the buffer with a specific number of samples

***********************************************************************************************/

void tms5220_process(unsigned char *buffer, unsigned int size)
{
    int buf_count=0;
    int i, interp_period;

tryagain:

    /* if we're not speaking, parse commands */
    while (!speak_external && fifo_count > 0)
        process_command ();

    /* if there's nothing to do, bail */
    if (!size)
        return;

    /* if we're empty and still not speaking, fill with nothingness */
    if (!speak_external)
        goto empty;

    /* if we're to speak, but haven't started, wait for the 9th byte */
    if (!talk_status)
    {
        if (fifo_count < 9)
           goto empty;

        /* parse but don't remove the first frame, and set the status to 1 */
        parse_frame (0);
        talk_status = 1;
        buffer_empty = 0;
    }

    /* loop until the buffer is full or we've stopped speaking */
    while ((size > 0) && speak_external)
    {
        int current_val;

        /* if we're ready for a new frame */
        if ((interp_count == 0) && (sample_count == 0))
        {
            /* Parse a new frame */
            if (!parse_frame (1))
                break;

            /* Set old target as new start of frame */
            current_energy = old_energy;
            current_pitch = old_pitch;
            for (i = 0; i < 10; i++)
                current_k[i] = old_k[i];

            /* is this a zero energy frame? */
            if (current_energy == 0)
            {
                //printf("processing frame: zero energy\n");
                target_energy = 0;
                target_pitch = current_pitch;
                for (i = 0; i < 10; i++)
                    target_k[i] = current_k[i];
            }

            /* is this a stop frame? */
            else if (current_energy == (energytable[15] >> 6))
            {
                //printf("processing frame: stop frame\n");
                current_energy = energytable[0] >> 6;
                target_energy = current_energy;
                speak_external = talk_status = 0;
                interp_count = sample_count = pitch_count = 0;

                /* generate an interrupt if necessary */
                cause_interrupt ();

                /* try to fetch commands again */
                goto tryagain;
            }
            else
            {
                /* is this the ramp down frame? */
                if (new_energy == (energytable[15] >> 6))
                {
                    //printf("processing frame: ramp down\n");
                    target_energy = 0;
                    target_pitch = current_pitch;
                    for (i = 0; i < 10; i++)
                        target_k[i] = current_k[i];
                }
                /* Reset the step size */
                else
                {
                    //printf("processing frame: Normal\n");
                    //printf("*** Energy = %d\n",current_energy);
                    //printf("proc: %d %d\n",last_fbuf_head,fbuf_head);

                    target_energy = new_energy;
                    target_pitch = new_pitch;

                    for (i = 0; i < 4; i++)
                        target_k[i] = new_k[i];
                    if (current_pitch == 0)
                        for (i = 4; i < 10; i++)
                        {
                            target_k[i] = current_k[i] = 0;
                        }
                    else
                        for (i = 4; i < 10; i++)
                            target_k[i] = new_k[i];
                }
            }
        }
        else if (interp_count == 0)
        {
            /* Update values based on step values */
            //printf("\n");

            interp_period = sample_count / 25;
            current_energy += (target_energy - current_energy) / interp_coeff[interp_period];
            if (old_pitch != 0)
                current_pitch += (target_pitch - current_pitch) / interp_coeff[interp_period];

            //printf("*** Energy = %d\n",current_energy);

            for (i = 0; i < 10; i++)
            {
                current_k[i] += (target_k[i] - current_k[i]) / interp_coeff[interp_period];
            }
        }

        if (old_energy == 0)
        {
            /* generate silent samples here */
            current_val = 0x00;
        }
        else if (old_pitch == 0)
        {
            /* generate unvoiced samples here */
            randbit = (rand () % 2) * 2 - 1;
            current_val = (randbit * current_energy) / 4;
        }
        else
        {
            /* generate voiced samples here */
            if (pitch_count < sizeof (chirptable))
                current_val = (chirptable[pitch_count] * current_energy) / 256;
            else
                current_val = 0x00;
        }

        /* Lattice filter here */

        u[10] = current_val;

        for (i = 9; i >= 0; i--)
        {
            u[i] = u[i+1] - ((current_k[i] * x[i]) / 32768);
        }
        for (i = 9; i >= 1; i--)
        {
            x[i] = x[i-1] + ((current_k[i-1] * u[i-1]) / 32768);
        }

        x[0] = u[0];

        /* clipping, just like the chip */

        if (u[0] > 511)
            buffer[buf_count] = 127;
        else if (u[0] < -512)
            buffer[buf_count] = -128;
        else
            buffer[buf_count] = u[0] >> 2;

        /* Update all counts */

        size--;
        sample_count = (sample_count + 1) % 200;

        if (current_pitch != 0)
            pitch_count = (pitch_count + 1) % current_pitch;
        else
            pitch_count = 0;

        interp_count = (interp_count + 1) % 25;
        buf_count++;
    }

empty:

    while (size > 0)
    {
        buffer[buf_count] = 0x00;
        buf_count++;
        size--;
    }
}



/**********************************************************************************************

     process_command -- extract a byte from the FIFO and interpret it as a command

***********************************************************************************************/

static void process_command (void)
{
    unsigned char cmd;

    /* if there are stray bits, ignore them */
    if (bits_taken)
    {
        bits_taken = 0;
        fifo_count--;
        fifo_head = (fifo_head + 1) % FIFO_SIZE;
    }

    /* grab a full byte from the FIFO */
    if (fifo_count > 0)
    {
        cmd = fifo[fifo_head] & 0x70;
        fifo_count--;
        fifo_head = (fifo_head + 1) % FIFO_SIZE;

        /* only real command we handle now is speak external */
        if (cmd == 0x60)
        {
            speak_external = 1;

            /* according to the datasheet, this will cause an interrupt due to a BE condition */
            if (!buffer_empty)
            {
                buffer_empty = 1;
                cause_interrupt ();
            }
        }
    }

    /* update the buffer low state */
    check_buffer_low ();
}



/**********************************************************************************************

     extract_bits -- extract a specific number of bits from the FIFO

***********************************************************************************************/

static int extract_bits (int count)
{
    int val = 0;

    while (count--)
    {
        val = (val << 1) | ((fifo[fifo_head] >> bits_taken) & 1);
        bits_taken++;
        if (bits_taken >= 8)
        {
            fifo_count--;
            fifo_head = (fifo_head + 1) % FIFO_SIZE;
            bits_taken = 0;
        }
    }
    return val;
}



/**********************************************************************************************

     parse_frame -- parse a new frame's worth of data; returns 0 if not enough bits in buffer

***********************************************************************************************/

static int parse_frame (int removeit)
{
    int old_head, old_taken, old_count;
    int bits, indx, i, rep_flag;

    /* remember previous frame */
    old_energy = new_energy;
    old_pitch = new_pitch;
    for (i = 0; i < 10; i++)
        old_k[i] = new_k[i];

    /* clear out the new frame */
    new_energy = 0;
    new_pitch = 0;
    for (i = 0; i < 10; i++)
        new_k[i] = 0;

    /* if the previous frame was a stop frame, don't do anything */
    if (old_energy == (energytable[15] >> 6))
        return 1;

    /* remember the original FIFO counts, in case we don't have enough bits */
    old_count = fifo_count;
    old_head = fifo_head;
    old_taken = bits_taken;

    /* count the total number of bits available */
    bits = fifo_count * 8 - bits_taken;

    /* attempt to extract the energy index */
    bits -= 4;
    if (bits < 0)
        goto ranout;
    indx = extract_bits (4);
    new_energy = energytable[indx] >> 6;

	/* if the index is 0 or 15, we're done */
	if (indx == 0 || indx == 15)
	{
		#ifdef DEBUG_5220
			if (f) fprintf (f, "  (4-bit energy=%d frame)\n",new_energy);
		#endif

		/* clear fifo if stop frame encountered */
		if (indx == 15)
		{
			fifo_head = fifo_tail = fifo_count = bits_taken = 0;
			removeit = 1;
		}
		goto done;
	}

    /* attempt to extract the repeat flag */
    bits -= 1;
    if (bits < 0)
        goto ranout;
    rep_flag = extract_bits (1);

    /* attempt to extract the pitch */
    bits -= 6;
    if (bits < 0)
        goto ranout;
    indx = extract_bits (6);
    new_pitch = pitchtable[indx] / 256;

    /* if this is a repeat frame, just copy the k's */
    if (rep_flag)
    {
        for (i = 0; i < 10; i++)
            new_k[i] = old_k[i];

        #ifdef DEBUG_5220
            if (f) fprintf (f, "  (11-bit energy=%d pitch=%d rep=%d frame)\n", new_energy, new_pitch, rep_flag);
        #endif
        goto done;
    }

    /* if the pitch index was zero, we need 4 k's */
    if (indx == 0)
    {
        /* attempt to extract 4 K's */
        bits -= 18;
        if (bits < 0)
            goto ranout;
        new_k[0] = k1table[extract_bits (5)];
        new_k[1] = k2table[extract_bits (5)];
        new_k[2] = k3table[extract_bits (4)];
        new_k[3] = k4table[extract_bits (4)];

        #ifdef DEBUG_5220
            if (f) fprintf (f, "  (29-bit energy=%d pitch=%d rep=%d 4K frame)\n", new_energy, new_pitch, rep_flag);
        #endif
        goto done;
    }

    /* else we need 10 K's */
    bits -= 39;
    if (bits < 0)
        goto ranout;
    new_k[0] = k1table[extract_bits (5)];
    new_k[1] = k2table[extract_bits (5)];
    new_k[2] = k3table[extract_bits (4)];
    new_k[3] = k4table[extract_bits (4)];
    new_k[4] = k5table[extract_bits (4)];
    new_k[5] = k6table[extract_bits (4)];
    new_k[6] = k7table[extract_bits (4)];
    new_k[7] = k8table[extract_bits (3)];
    new_k[8] = k9table[extract_bits (3)];
    new_k[9] = k10table[extract_bits (3)];

    #ifdef DEBUG_5220
        if (f) fprintf (f, "  (50-bit energy=%d pitch=%d rep=%d 10K frame)\n", new_energy, new_pitch, rep_flag);
    #endif

done:

    #ifdef DEBUG_5220
        if (f) fprintf (f, "Parsed a frame successfully - %d bits remaining\n", bits);
    #endif

    /* if we're not to remove this one, restore the FIFO */
    if (!removeit)
    {
        fifo_count = old_count;
        fifo_head = old_head;
        bits_taken = old_taken;
    }

    /* update the buffer_low status */
    check_buffer_low ();
    return 1;

ranout:

    #ifdef DEBUG_5220
        if (f) fprintf (f, "Ran out of bits on a parse!\n");
    #endif

    /* this is an error condition; mark the buffer empty and turn off speaking */
    buffer_empty = 1;
    talk_status = speak_external = 0;
    fifo_count = fifo_head = fifo_tail = 0;

    /* generate an interrupt if necessary */
    cause_interrupt ();
    return 0;
}



/**********************************************************************************************

     check_buffer_low -- check to see if the buffer low flag should be on or off

***********************************************************************************************/

static void check_buffer_low (void)
{
    /* did we just become low? */
    if (fifo_count < 8)
    {
        /* generate an interrupt if necessary */
        if (!buffer_low)
            cause_interrupt ();
        buffer_low = 1;

        #ifdef DEBUG_5220
            if (f) fprintf (f, "Buffer low set\n");
        #endif
    }

    /* did we just become full? */
    else
    {
        buffer_low = 0;

        #ifdef DEBUG_5220
            if (f) fprintf (f, "Buffer low cleared\n");
        #endif
    }
}



/**********************************************************************************************

     cause_interrupt -- generate an interrupt

***********************************************************************************************/

static void cause_interrupt (void)
{
    irq_pin = 1;
    if (irq_func) irq_func ();
}
