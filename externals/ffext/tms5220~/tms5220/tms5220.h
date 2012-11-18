#ifndef tms5220_h
#define tms5220_h

void tms5220_reset (void);
void tms5220_set_irq (void (*func)(void));

void tms5220_data_write (int data);
int tms5220_status_read (void);
int tms5220_ready_read (void);
int tms5220_int_read (void);

void tms5220_process (unsigned char *buffer, unsigned int size);

#endif

