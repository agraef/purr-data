/* --------------------------- k_jack~  ----------------------------------- */
/*   ;; Kjetil S. Matheussen, 2004.                                             */
/*                                                                              */
/* This program is free software; you can redistribute it and/or                */
/* modify it under the terms of the GNU General Public License                  */
/* as published by the Free Software Foundation; either version 2               */
/* of the License, or (at your option) any later version.                       */
/*                                                                              */
/* This program is distributed in the hope that it will be useful,              */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/* GNU General Public License for more details.                                 */
/*                                                                              */
/* You should have received a copy of the GNU General Public License            */
/* along with this program; if not, write to the Free Software                  */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.  */
/*                                                                              */
/* ---------------------------------------------------------------------------- */



#include <m_pd.h>
#include <s_stuff.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include <jack/jack.h>


// Currently, the jack implementation in PD only supports 2 pd instances.
// However, it doesn't hurt to use 100, so we use 100 to be prepared for
// the future.

#define MAX_PD_JACKCLIENTS 100


static char *version = 
"k_jack~ v0.0.2, written by Kjetil S. Matheussen, k.s.matheussen@notam02.no";





/***********************************************************/
/********************* Jack part ***************************/
/***********************************************************/

struct JackPort{
  struct JackPort *next;
  char *name;
  bool is_input;
  int num;
};

static struct JackPort *jackports=NULL;

/* Name(+":") of the client we belong to. ("pure_data_0:", "pure_data_1:", ...) */
static char *clientname=NULL;



static void set_pd_channels(int inc_ins,int inc_outs){
  int num_recs=sys_get_inchannels();
  int num_plays=sys_get_outchannels();

  int t1[1]={0};
  int t2[1]={0};
  int t3[1]={num_recs+inc_ins};
  int t4[1]={num_plays+inc_outs};
  sys_close_audio();
  sys_open_audio(1,t1,
		 1,t3,
		 1,t2,
		 1,t4,
		 sys_getsr(),sys_schedadvance/1000,1);
}



static bool find_clientname(jack_client_t *client){
  bool ret=false;

  if(clientname!=NULL){
    ret=true;
  }else{
    int lokke;
    int num_clients=0;
    int num_ports[MAX_PD_JACKCLIENTS]={0};
    char temp[500];
    const char **ports;

    for(lokke=0;lokke<MAX_PD_JACKCLIENTS;lokke++){
      sprintf(temp,"pure_data_%d",lokke);
      ports=jack_get_ports(client,temp,"",0);
      if(ports!=NULL){
	while(ports[num_ports[lokke]]!=NULL){
	  num_ports[lokke]+=1;
	}
      }
    }

    set_pd_channels(1,1);    
      
    for(lokke=0;lokke<MAX_PD_JACKCLIENTS;lokke++){
      int num_ports2[MAX_PD_JACKCLIENTS]={0};
      sprintf(temp,"pure_data_%d",lokke);
      ports=jack_get_ports(client,temp,"",0);
      if(ports!=NULL){
	while(ports[num_ports2[lokke]]!=NULL){
	  num_ports2[lokke]+=1;
	}
      }
      if(num_ports2[lokke]!=num_ports[lokke]){
	clientname=malloc(500);
	sprintf(clientname,"pure_data_%d:",lokke);
	//post("Got clientname: -%s-",clientname);
	ret=true;
	break;
      }
    }
    set_pd_channels(-1,-1);
  }

  return ret;
}


/* Nearly the same as jack_get_ports, but filter out pure_data_%d ports. */
static const char **my_jack_get_ports(jack_client_t *client,char *name){
  const char **ret;
  const char **ports;
  int lokke=0;
  int to=0;
  int num_ports=0;

  if(!strcmp("*",name))
    ports=jack_get_ports(client,"","",0);
  else
    ports=jack_get_ports(client,name,"",0);
  if(ports==NULL) return NULL;

  while(ports[num_ports]!=NULL){
    num_ports++;
  }
  ret=calloc(sizeof(char *),num_ports);

  while(ports[lokke]!=NULL){
    if(strstr(ports[lokke],clientname)==NULL){
      ret[to]=ports[lokke];
      to++;
    }
    lokke++;
  }
  if(to==0){
    free(ret);
    ret=NULL;
  }

 exit:
  free(ports);
  return ret;
}


/* Disconnect all the ports connection to an alsa_pcm port. */
static void disconnect_all_alsa(jack_client_t *client,const char *portname){
  int lokke=0;
  jack_port_t *port=jack_port_by_name(client,portname);
  const char **ports=jack_port_get_all_connections(client,port);
  if(ports==NULL) return;

  while(ports[lokke]!=NULL){
    if(strstr(ports[lokke],"alsa_pcm:")==ports[lokke]){
      if(jack_port_flags(port)&JackPortIsOutput){
	jack_disconnect(client,portname,ports[lokke]);
      }else{
	jack_disconnect(client,ports[lokke],portname);
      }
    }
    lokke++;
  }
  free(ports);
}


static int add_pd_channel(bool is_input){
  set_pd_channels(is_input==true?0:1,is_input==true?1:0);
  return is_input==true
    ?sys_get_outchannels()-1
    :sys_get_inchannels()-1;
}


/* Returns the dac/adc index the jackport with name 'portname' has in pd. Create if it doesn't exist. */
static int get_portindex(jack_client_t *client,const char *portname, bool is_input){
  struct JackPort *jp=jackports;

  while(jp!=NULL){
    if(!strcmp(jp->name,portname)) break;
    jp=jp->next;
  }
  if(jp==NULL){
    char temp[500];
    char temp2[500];
    jp=calloc(1,sizeof(struct JackPort));
    jp->name=strdup(portname);
    jp->is_input=is_input;
    jp->num=add_pd_channel(is_input);
    jp->next=jackports;
    jackports=jp;
    disconnect_all_alsa(client,portname);
    if(is_input){
      sprintf(temp,"%soutput%d",clientname,jp->num);
      jack_connect(client,temp,portname);
      sprintf(temp2,"to_%s",portname);
    }else{
      sprintf(temp,"%sinput%d",clientname,jp->num);
      jack_connect(client,portname,temp);
      sprintf(temp2,"from_%s",portname);
    }
    while(strstr(temp2,":")) strstr(temp2,":")[0]='-';
    if(strlen(temp2)+strlen(clientname)<32)
      jack_port_set_name(jack_port_by_name(client,temp),temp2);
  }
  return jp->num;
}




/***********************************************************/
/********************* PD part *****************************/
/***********************************************************/

typedef struct _k_jack
{
  t_object x_obj;
  int num_recs;
  int num_plays;
  int *rec_nums;
  int *play_nums;
  float x_float;
} t_k_jack;


static t_class *k_jack_class;



static t_int *k_jack_perform_add(t_int *w){
  t_float *in = (t_float *)(w[1]);
  t_float *out = (t_float *)(w[2]);
  int n = (int)(w[3]);
  int lokke;

  for(lokke=0;lokke<DEFDACBLKSIZE;lokke++){
    out[lokke]+=in[lokke];
  }

  return (w+3);
}


static t_int *k_jack_perform_copy(t_int *w){
  t_float *in = (t_float *)(w[1]);
  t_float *out = (t_float *)(w[2]);
  int lokke;
  
  for(lokke=0;lokke<DEFDACBLKSIZE;lokke++){
    out[lokke]=in[lokke];
  }

  return (w+3);
}


static void k_jack_dsp(t_k_jack *x, t_signal **sp)
{
  int lokke;
  int ch=0;

  for(lokke=0;lokke<x->num_recs;lokke++){
    if(sp[ch]->s_n!=DEFDACBLKSIZE)
      post("k_jack~ wrong framesize. Is this possible?");
    else
      dsp_add(
	      k_jack_perform_add,
	      2,
	      sp[ch]->s_vec,
	      sys_soundout + x->rec_nums[lokke]*DEFDACBLKSIZE
	      );
    ch++;
  }
  for(lokke=0;lokke<x->num_plays;lokke++){
    if(sp[ch]->s_n!=DEFDACBLKSIZE)
      post("k_jack~ wrong framesize. Is this possible?");
    else
      dsp_add(
	      k_jack_perform_copy,
	      2,
	      sys_soundin + x->play_nums[lokke]*DEFDACBLKSIZE,
	      sp[ch]->s_vec
	      );
    ch++;
  }
}


static void k_jack_free(t_k_jack *x){
  free(x->rec_nums);
  free(x->play_nums);
}


static void *k_jack_new(t_symbol *s){
  int num_recs=0;
  int num_plays=0;
  int lokke=0;
  static jack_client_t *client=NULL;
  const char **ports=NULL;
  t_k_jack *x=NULL;


  if(sys_audioapi!=API_JACK){
    post("Error. k_jack~ will not work without jack as the sound API.");
    goto exit;
  }

  if(client==NULL){
    for(lokke=0;lokke<MAX_PD_JACKCLIENTS;lokke++){
      char temp[500];
      sprintf(temp,"k_jack_tilde_%d",lokke);
      client=jack_client_new(temp);
      if(client!=NULL) break;
    }
  }

  if(client==NULL){
    post("k_jack~: Could not make jack client.");
    goto exit;
  }

  if(find_clientname(client)==false){
    post("k_jack~: Could not find the name of pure data jack client.");
    goto exit;
  }

  ports=my_jack_get_ports(client,s->s_name);
  if(ports==NULL){
    post("k_jack~: Client \"%s\" not found.\n",s->s_name);
    goto exit;
  }
  while(ports[lokke]!=NULL){
    jack_port_t* port=jack_port_by_name(client,ports[lokke]);
    //post("%s, type: %s, flags: %d",ports[lokke],jack_port_type(port),jack_port_flags(port));
    if(jack_port_flags(port)&JackPortIsInput){
      num_recs++;
    }else{
      if(jack_port_flags(port)&JackPortIsOutput){
	num_plays++;
      }
    }
    lokke++;
  }

  if(num_plays==0 && num_recs==0){
    post("Client(s) containing the name \"%s\" have no input or output ports.",s->s_name);
    goto exit;
  }

  x = (t_k_jack *)pd_new(k_jack_class);

  x->rec_nums=calloc(sizeof(int),num_recs);
  x->play_nums=calloc(sizeof(int),num_plays);

  //post("recs: %d, plays: %d\n",num_recs,num_plays);
  lokke=0;
  while(ports[lokke]!=NULL){
    jack_port_t* port=jack_port_by_name(client,ports[lokke]);
    if(jack_port_flags(port)&JackPortIsInput){
      x->rec_nums[x->num_recs]=get_portindex(client,ports[lokke],true);
      if(x->num_recs>0)
	  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
      //post("Made inlet %s %d",ports[lokke],x->rec_nums[x->num_recs]);
      x->num_recs++;
    }else{
      if(jack_port_flags(port)&JackPortIsOutput){
	x->play_nums[x->num_plays]=get_portindex(client,ports[lokke],false);
	outlet_new(&x->x_obj, gensym("signal"));
	//post("Made outlet %s %d",ports[lokke],x->play_nums[x->num_plays]);
	x->num_plays++;
      }
    }
    lokke++;
  }


 exit:

  if(ports!=NULL) free(ports);

  /* Program crash if client is closed. (as a workaround, I made it static for reuse. -Kjetil) (I thought this gruesome bug was fixed!?!)*/
  //if(client!=NULL) jack_client_close(client);

  return (x);
}
 


void k_jack_tilde_setup(void){
  k_jack_class = class_new(gensym("k_jack~"), (t_newmethod)k_jack_new, (t_method)k_jack_free,
			  sizeof(t_k_jack), 0, A_SYMBOL, 0);
  CLASS_MAINSIGNALIN(k_jack_class, t_k_jack, x_float);
  class_addmethod(k_jack_class, (t_method)k_jack_dsp, gensym("dsp"), 0);
  
  class_sethelpsymbol(k_jack_class, gensym("help-k_jack~.pd"));

  post(version);
}


