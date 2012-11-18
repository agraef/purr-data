/******************************************************
 *
 * snmp/get - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   2006:forum::für::umläute:2006
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/

/* 2402:forum::für::umläute:2006 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/utilities.h>
#include <net-snmp/net-snmp-includes.h>

#include "m_pd.h"

/* ------------------------- snmpget ------------------------------- */

static t_class *snmpget_class;

typedef struct _snmpget
{
  t_object x_obj;

  netsnmp_session*x_session;

  t_outlet*out_data, *out_err;
  int x_raw; /* if set, the raw string is output; no interpretation is done */
} t_snmpget;

static void snmpget_disconnect(t_snmpget *x);

static void snmpget_get(t_snmpget *x, t_symbol *s)
{ 
  if(NULL!=x->x_session){
    oid             name[MAX_OID_LEN];
    size_t          name_length=MAX_OID_LEN;
    netsnmp_variable_list *vars;
    netsnmp_pdu     *pdu,*response;
    int err=0;
    char*symname=s->s_name;
    
    if(!snmp_parse_oid(symname, name, &name_length)){
      error("snmpget: bad OID %d", name_length);
      snmp_perror(symname);
      return;
    }
    pdu = snmp_pdu_create(SNMP_MSG_GET);
    snmp_add_null_var(pdu, name, name_length);
    err=snmp_synch_response(x->x_session, pdu, &response);
    if(STAT_SUCCESS == err){
      if(SNMP_ERR_NOERROR == response->errstat){
        for (vars = response->variables; vars;
             vars = vars->next_variable) {
          char mybuf[MAXPDSTRING];
          t_binbuf *bbuf = binbuf_new();
          int type=vars->type;
          //print_variable(vars->name, vars->name_length, vars);
          //print_value(vars->name, vars->name_length, vars);
          /*
            INTEGER     = 2
            Counter32   = 65
            Gauge32     = 66
            
            Hex-STRING  = 4
            STRING      = 4
            
            IpAddress   = 64
            Network Address: 64
            
            OID         = 6
            Timeticks   = 67
          */
          //post("type: %d", vars->type);
          if(0==x->x_raw) {
            switch(type) {
            case ASN_TIMETICKS:
            case ASN_GAUGE:  case ASN_COUNTER:
            case ASN_INTEGER:
              {
		t_atom aflist[2];
                long v=(long)(*vars->val.integer);
		unsigned short lo=(unsigned short)v;
		unsigned short hi=(unsigned short)(v>>16);
		SETFLOAT(aflist+0, hi);
		SETFLOAT(aflist+1, lo);
		outlet_anything(x->out_data, 
				s, 
				2, aflist);
		//outlet_float(x->out_data, (t_float)v);
              }
              break;
            default:
              snprint_value(mybuf, sizeof(mybuf), vars->name, vars->name_length, vars);
              binbuf_text(bbuf, mybuf, strlen(mybuf)); 
              int ac=binbuf_getnatom(bbuf);
              t_atom*av=binbuf_getvec(bbuf);
              outlet_list(x->out_data, 
                          s, 
                          ac, av);
            }
          } else {
            t_atom atm, *ap;
            ap=&atm;
            snprint_value(mybuf, sizeof(mybuf), vars->name, vars->name_length, vars);
            SETSYMBOL(ap, gensym(mybuf));
            outlet_anything(x->out_data, s, 1, ap);
          }
          if(bbuf)binbuf_free(bbuf);
        }
      }
    } else {
      error("[snmp/get] error while synching");
      snmpget_disconnect(x);
    }
    snmp_free_pdu(response);
  } else {
    pd_error(x, "[snmp/get] not connected");
  }
}


/*
  connect <host>[:<port>] <community> 
*/
static void snmpget_connect(t_snmpget *x, t_symbol *hostport, t_symbol*comm)
{ 
  if(x->x_session==NULL) {
    SOCK_STARTUP;
    netsnmp_session session;

    int len=0;
    char*cdummy;

    char*peername=0;
    int peerport=-1;

    char*community=0;

    if(0!=comm && &s_!=comm)
      community=comm->s_name;

    for(cdummy=hostport->s_name; *cdummy!=0 && *cdummy!=':'; cdummy++, len++);

    if(*cdummy==':') {
      // there is a port hidden in this string!
      peerport=(int)strtol(cdummy+1, (char **)NULL, 10);
    }

    peername=(char*)getbytes(sizeof(char)*len+1);
    strncpy(peername, hostport->s_name, len);
    peername[len]=0;
    snmp_sess_init(&session);

    session.version=SNMP_VERSION_1;
    session.peername=peername;

    if(peerport>0 && peerport<65536) {
      session.remote_port=peerport;
    }

    if(0!=community) {
      session.community=(unsigned char*)community;
      session.community_len=strlen(community);
    }
    x->x_session = snmp_open(&session);
    freebytes(peername, len);

    if(x->x_session==NULL){
      SOCK_CLEANUP;
      outlet_float(x->out_err, 0);
      return;
    }
  } else {
    pd_error(x, "[snmp/get] already connected");
  }
    outlet_float(x->out_err, 1);
}
static void snmpget_dodisconnect(t_snmpget *x)
{
  if(x->x_session){
    snmp_close(x->x_session);
    SOCK_CLEANUP;
  }
  x->x_session=NULL;
  outlet_float(x->out_err, 0);
}
static void snmpget_disconnect(t_snmpget *x) {
  if(x->x_session)
    snmpget_dodisconnect(x);
  else
    pd_error(x, "[snmp/get] not connected!");
}

static void snmpget_raw(t_snmpget *x, t_float f) {
  int i=(int)f;
  x->x_raw=(i>0);
}

static void *snmpget_free(t_snmpget *x)
{
  snmpget_disconnect(x);
  outlet_free(x->out_err);
  outlet_free(x->out_data);
}

static void *snmpget_new(t_symbol*s, int argc, t_atom*argv)
{
  t_snmpget *x = (t_snmpget *)pd_new(snmpget_class);

  x->out_data=outlet_new(&x->x_obj, 0);
  x->out_err=outlet_new(&x->x_obj, &s_float);

  x->x_session=0;
  x->x_raw=0;

  return (x);
}

void snmpget_setup(void)
{
  init_snmp("snmp4pd");

  snmpget_class = class_new(gensym("get"), (t_newmethod)snmpget_new, 
			   0, sizeof(t_snmpget), 0, A_GIMME, 0);
  class_addcreator((t_newmethod)snmpget_new, gensym("snmp/get"), 0);
  class_addcreator((t_newmethod)snmpget_new, gensym("snmpget"), 0);

  class_addmethod(snmpget_class, (t_method)snmpget_get, gensym("get"), A_SYMBOL, 0);

  class_addmethod(snmpget_class, (t_method)snmpget_connect, gensym("connect"), A_SYMBOL, A_DEFSYM, 0);
  class_addmethod(snmpget_class, (t_method)snmpget_disconnect, gensym("disconnect"), 0);
  class_addmethod(snmpget_class, (t_method)snmpget_raw, gensym("raw"), A_FLOAT, 0);
}

void get_setup(void)
{
  snmpget_setup();
}
