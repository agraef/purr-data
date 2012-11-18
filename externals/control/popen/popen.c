/* :popen: for PD - windows + linux + (probably mac) - carmen rocco */

#include <m_pd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define INBUFSIZE 1024

static t_class *popen_class;

typedef struct _popen
{
     t_object x_obj;
     char buffer[128];
     t_symbol *x_s;
     t_outlet* x_done;
} t_popen;

static void popen_out(t_popen* x)
{
  int i;
  int size = 0;
  size = strlen(x->buffer);
  for (i=0;i<size;i++)
    if (x->buffer[i] == '\n' || x->buffer[i] == '\r') x->buffer[i] = '\0';
  fprintf(stderr, "%s",x->buffer);
  x->x_s = gensym(x->buffer);
  outlet_symbol(x->x_obj.ob_outlet, x->x_s); 
}

static void popen_anything(t_popen *x, t_symbol *s, int argc, t_atom *argv)
{
  int i;
  char arg[MAXPDSTRING];
  char cmd[MAXPDSTRING] = "";
  FILE *pPipe;
  int size = 0;
  for (i=0;i<argc;i++) {
    atom_string(argv,arg+size,MAXPDSTRING - size);
    argv++;
    size=strlen(arg);
    arg[size++] = ' ';
  }
  arg[size-1] = '\0';

  strcat(cmd, s->s_name);
  strcat(cmd, " ");
  strcat(cmd, arg);
  post("sending %s",cmd);

#ifdef _WIN32
  if( (pPipe = _popen( cmd, "rt" )) == NULL )
#else
  if( (pPipe = popen( cmd, "r" )) == NULL )
#endif
    return;

  while( !feof( pPipe ) )
    {
      if( fgets( x->buffer, 128, pPipe ) != NULL )
	popen_out(x);
    }
#ifdef _WIN32
  _pclose( pPipe );
#else
  pclose( pPipe );
#endif

  outlet_bang(x->x_done);
}

void popen_free(t_popen* x)
{
}

static void *popen_new(void)
{
    t_popen *x = (t_popen *)pd_new(popen_class);
    outlet_new(&x->x_obj, &s_symbol);
    x->x_done = outlet_new(&x->x_obj, &s_bang);
    return (x);
}

void popen_setup(void)
{
    popen_class = class_new(gensym("popen"), (t_newmethod)popen_new, 
			    (t_method)popen_free,sizeof(t_popen), 0,0);
    class_addbang(popen_class,popen_out);
    class_addanything(popen_class, popen_anything);
}
