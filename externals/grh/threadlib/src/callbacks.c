/* 
* 
* callbacks.c
* implementation of the callback FIFO
*
* this is the (modified) FIFO of the idle callbacks from pd_devel_0.38
* (implemented in m_sched.c)
*/

#include "threadlib.h"

// global callback fifo
t_fifo * h_callback_fifo = NULL;

// global clock callback to trigger
// the callback fifo
t_clock *h_callback_clock = NULL;

/* linked list of callbacks 
 * callback will be freed after returning 0 */
typedef struct _sched_callback
{
  struct _sched_callback* next; /* next callback in ringbuffer / in fifo */
  t_int (*function) (t_int* argv);
  t_int* argv;
  t_int argc;
} t_sched_callback;

// forward declaration
static void h_run_callbacks();

void h_init_callbacks()
{
  h_callback_fifo = threadlib_fifo_init();
  h_callback_clock = clock_new(NULL, (t_method)h_run_callbacks);
}

void h_free_callbacks()
{
  clock_free(h_callback_clock);
  threadlib_fifo_destroy(h_callback_fifo);
}

void sys_callback(t_int (*callback) (t_int* argv), t_int* argv, t_int argc)
{
  t_sched_callback* new = (t_sched_callback*) getbytes
      (sizeof(t_sched_callback));

  new->function = callback;
  new->argv = (t_int*) copybytes (argv, argc * sizeof (t_int));
  new->argc = argc;
  new->next = NULL;
	
  threadlib_fifo_put(h_callback_fifo, new);
  
  // TODO find solution without lock
  sys_lock();
  clock_delay(h_callback_clock, 0);
  sys_unlock();
}

static t_sched_callback *ringbuffer_head;

void h_run_callbacks()
{
  t_sched_callback * new_callback;
  
  sys_unlock();
  
  /* append idle callback to ringbuffer */
  
  while ( (new_callback = (t_sched_callback*) threadlib_fifo_get(h_callback_fifo)) )
  {
    t_sched_callback * next;
    
    /* set the next field to NULL ... it might be set in the fifo */
    new_callback->next = NULL;
    if (ringbuffer_head == NULL)
    {
      ringbuffer_head = new_callback;
    }
    else
    {
      next = ringbuffer_head;
      while (next->next != 0)
	next = next->next;
      next->next = new_callback;
    }
  }

  if (ringbuffer_head != NULL)
  {
    t_sched_callback * idle_callback = ringbuffer_head;
    t_sched_callback * last = NULL;
    t_sched_callback * next;

    do
    {
      int status;
    
      sys_lock();
      status = (idle_callback->function)(idle_callback->argv);
      sys_unlock();
    
      switch (status)
      {
        /* callbacks returning 0 will be deleted */
        case 0:
          next = idle_callback->next;
          freebytes (idle_callback->argv, idle_callback->argc);
          freebytes ((void*)idle_callback, sizeof(t_sched_callback));

          if (last == NULL)
            ringbuffer_head = next;
          else
            last->next = next;

          idle_callback = next;

        /* callbacks returning 1 will be run again */
        case 1:
          break;

        /* callbacks returning 2 will be run during the next idle callback */
        case 2:
          last = idle_callback;
          idle_callback = idle_callback->next;
      }
    }
    while (idle_callback != NULL);
  }

  sys_lock();
}
