/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* g_pdp_colorgrid.c written by Yves Degoyon 2002                                       */
/* pdp_colorgrid control object : two dimensionnal pdp_colorgrid                                 */
/* thanks to Thomas Musil, Miller Puckette, Guenther Geiger and Krzystof Czaja */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include "m_pd.h"
#include "m_imp.h"
#include "g_canvas.h"


#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef __APPLE__
# define COLORGRID_IMG "/Applications/Pd-extended.app/Contents/Resources/doc/examples/pidip/images/colorgrid.pnm"
#endif
#ifdef __gnu_linux__
# define COLORGRID_IMG "/usr/lib/pd/doc/examples/pidip/images/colorgrid.pnm"
#endif

#define DEFAULT_COLORGRID_WIDTH 256
#define DEFAULT_COLORGRID_HEIGHT 50
#define DEFAULT_COLORGRID_NBLINES 10

typedef struct _pdp_colorgrid
{
    t_object x_obj;
    t_glist *x_glist;
    t_symbol *x_name; 
    t_outlet *x_xoutlet; 
    t_outlet *x_youtlet; 
    t_outlet *x_zoutlet; 
    int x_null; 	/* To dissable resize                             */
    int x_height; 	/* height of the pdp_colorgrid                        */
    t_float x_min; 	/* minimum value of x                        */
    t_float x_max; 	/* max value of x                            */
    int x_width; 	/* width of the pdp_colorgrid                         */
    t_float y_min; 	/* minimum value of y                        */
    t_float y_max; 	/* max value of y                            */
    t_float x_current; 	/* x coordinate of current position          */
    t_float y_current; 	/* y coordinate of current position          */
    int x_selected; 	/* stores selected state                     */
    int x_point; 	/* indicates if a point is plotted           */
    int x_pdp_colorgrid; 	/* indicates if a pdp_colorgrid is requested          */
    t_float x_xstep; 	/* sets the step ( grain ) for x             */
    t_float x_ystep; 	/* sets the step ( grain ) for y             */
    int x_xlines; 	/* number of vertical lines                  */
    int x_ylines; 	/* number of horizontal lines                */
    t_symbol*  x_fname;
} t_pdp_colorgrid;

void load_tk_procs (void) {
	// ########### pdp_colorgrid procedures -- ydegoyon@free.fr #########
	// package require Img
	sys_gui("package require base64\n");
	sys_gui("proc pdp_colorgrid_apply {id} {\n");
	// strip "." from the TK id to make a variable name suffix
	sys_gui("set vid [string trimleft $id .]\n");
	// for each variable, make a local variable to hold its name...
	sys_gui("set var_graph_pdp_colorgrid [concat graph_pdp_colorgrid_$vid]\n");
	sys_gui("global $var_graph_pdp_colorgrid\n");
	sys_gui("set var_graph_xlines [concat graph_xlines_$vid]\n");
	sys_gui("global $var_graph_xlines\n");
	sys_gui("set var_graph_ylines [concat graph_ylines_$vid]\n");
	sys_gui("global $var_graph_ylines\n");
	sys_gui("set cmd [concat $id dialog [eval concat $$var_graph_xlines] [eval concat $$var_graph_ylines] [eval concat $$var_graph_pdp_colorgrid] \\;]\n");
	// puts stderr $cmd
	sys_gui("pd $cmd\n");
	sys_gui("}\n");
	sys_gui("proc pdp_colorgrid_cancel {id} {\n");
	sys_gui("set cmd [concat $id cancel \\;]\n");
	// puts stderr $cmd
	sys_gui("pd $cmd\n");
	sys_gui("}\n");
	sys_gui("proc pdp_colorgrid_ok {id} {\n");
	sys_gui("pdp_colorgrid_apply $id\n");
	sys_gui("pdp_colorgrid_cancel $id\n");
	sys_gui("}\n");
	sys_gui("proc pdtk_pdp_colorgrid_dialog {id xlines ylines pdp_colorgrid} {\n");
	sys_gui("set vid [string trimleft $id .]\n");
	sys_gui("set var_graph_pdp_colorgrid [concat graph_pdp_colorgrid_$vid]\n");
	sys_gui("global $var_graph_pdp_colorgrid\n");
	sys_gui("set var_graph_xlines [concat graph_xlines_$vid]\n");
	sys_gui("global $var_graph_xlines\n");
	sys_gui("set var_graph_ylines [concat graph_ylines_$vid]\n");
	sys_gui("global $var_graph_ylines\n");
	sys_gui("set $var_graph_pdp_colorgrid $pdp_colorgrid\n");
	sys_gui("set $var_graph_xlines $xlines\n");
	sys_gui("set $var_graph_ylines $ylines\n");
	sys_gui("toplevel $id\n");
	sys_gui("wm title $id {pdp_colorgrid}\n");
	sys_gui("wm protocol $id WM_DELETE_WINDOW [concat pdp_colorgrid_cancel $id]\n");
	sys_gui("label $id.label -text {COLORGRID PROPERTIES}\n");
	sys_gui("pack $id.label -side top\n");
	sys_gui("frame $id.buttonframe\n");
	sys_gui("pack $id.buttonframe -side bottom -fill x -pady 2m\n");
	sys_gui("button $id.buttonframe.cancel -text {Cancel} -command \"pdp_colorgrid_cancel $id\"\n");
	sys_gui("button $id.buttonframe.apply -text {Apply} -command \"pdp_colorgrid_apply $id\"\n");
	sys_gui("button $id.buttonframe.ok -text {OK} -command \"pdp_colorgrid_ok $id\"\n");
	sys_gui("pack $id.buttonframe.cancel -side left -expand 1\n");
	sys_gui("pack $id.buttonframe.apply -side left -expand 1\n");
	sys_gui("pack $id.buttonframe.ok -side left -expand 1\n");
	sys_gui("frame $id.42rangef\n");
	sys_gui("pack $id.42rangef -side top\n");
	sys_gui("label $id.42rangef.lxlines -text \"X sections :\"\n");
	sys_gui("entry $id.42rangef.xlines -textvariable $var_graph_xlines -width 7\n");
	sys_gui("pack $id.42rangef.lxlines $id.42rangef.xlines -side left\n");
	sys_gui("frame $id.72rangef\n");
	sys_gui("pack $id.72rangef -side top\n");
	sys_gui("label $id.72rangef.lylines -text \"Y sections :\"\n");
	sys_gui("entry $id.72rangef.ylines -textvariable $var_graph_ylines -width 7\n");
	sys_gui("pack $id.72rangef.lylines $id.72rangef.ylines -side left\n");
	sys_gui("checkbutton $id.showpdp_colorgrid -text {Show Grid} -variable $var_graph_pdp_colorgrid -anchor w\n");
	sys_gui("pack $id.showpdp_colorgrid -side top\n");
	sys_gui("bind $id.42rangef.xlines <KeyPress-Return> [concat pdp_colorgrid_ok $id]\n");
	sys_gui("bind $id.72rangef.ylines <KeyPress-Return> [concat pdp_colorgrid_ok $id]\n");
	sys_gui("focus $id.42rangef.xlines\n");
	sys_gui("}\n");
	// ########### pdp_colorgrid procedures END -- lluis@artefacte.org #########
}

t_widgetbehavior pdp_colorgrid_widgetbehavior;
static t_class *pdp_colorgrid_class;
static int pdp_colorgridcount=0;

static int guidebug=0;
static int pointsize = 5;

static char   *pdp_colorgrid_version = "pdp_colorgrid: version 0.4\nby Yves Degoyon (ydegoyon@free.fr) & Lluis Gomez i Bigorda (lluis@artefacte.org)";

#define COLORGRID_SYS_VGUI2(a,b) if (guidebug) \
                         post(a,b);\
                         sys_vgui(a,b)

#define COLORGRID_SYS_VGUI3(a,b,c) if (guidebug) \
                         post(a,b,c);\
                         sys_vgui(a,b,c)

#define COLORGRID_SYS_VGUI4(a,b,c,d) if (guidebug) \
                         post(a,b,c,d);\
                         sys_vgui(a,b,c,d)

#define COLORGRID_SYS_VGUI5(a,b,c,d,e) if (guidebug) \
                         post(a,b,c,d,e);\
                         sys_vgui(a,b,c,d,e)

#define COLORGRID_SYS_VGUI6(a,b,c,d,e,f) if (guidebug) \
                         post(a,b,c,d,e,f);\
                         sys_vgui(a,b,c,d,e,f)

#define COLORGRID_SYS_VGUI7(a,b,c,d,e,f,g) if (guidebug) \
                         post(a,b,c,d,e,f,g );\
                         sys_vgui(a,b,c,d,e,f,g)

#define COLORGRID_SYS_VGUI8(a,b,c,d,e,f,g,h) if (guidebug) \
                         post(a,b,c,d,e,f,g,h );\
                         sys_vgui(a,b,c,d,e,f,g,h)

#define COLORGRID_SYS_VGUI9(a,b,c,d,e,f,g,h,i) if (guidebug) \
                         post(a,b,c,d,e,f,g,h,i );\
                         sys_vgui(a,b,c,d,e,f,g,h,i)

/* drawing functions */
static void pdp_colorgrid_draw_update(t_pdp_colorgrid *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    t_int xpoint=x->x_current, ypoint=x->y_current;

    // later : try to figure out what's this test for ??  
    // if (glist_isvisible(glist))
    // {
       // delete previous point if existing
       if (x->x_point)  
       {
          COLORGRID_SYS_VGUI3(".x%lx.c delete %lxPOINT\n", canvas, x);
       }
        
       if ( x->x_current < x->x_obj.te_xpix ) xpoint = x->x_obj.te_xpix;
       if ( x->x_current > x->x_obj.te_xpix + x->x_width - pointsize ) 
			xpoint = x->x_obj.te_xpix + x->x_width - pointsize;
       if ( x->y_current < x->x_obj.te_ypix ) ypoint = x->x_obj.te_ypix;
       if ( x->y_current > x->x_obj.te_ypix + x->x_height - pointsize ) 
			ypoint = x->x_obj.te_ypix + x->x_height - pointsize;
       // draw the selected point
       COLORGRID_SYS_VGUI7(".x%lx.c create rectangle %d %d %d %d -outline {} -fill #FF0000 -tags %lxPOINT\n",
	     canvas, xpoint, ypoint, xpoint+5, ypoint+5, x);
       x->x_point = 1;
    // }  
    // else 
    // {
    //    post( "pdp_colorgrid : position updated in an invisible pdp_colorgrid" );
    // }
}

static void pdp_colorgrid_draw_new(t_pdp_colorgrid *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    char fname[MAXPDSTRING]=COLORGRID_IMG;

    char *fdata="UDYKIyBDUkVBVE9SOiBUaGUgR0lNUCdzIFBOTSBGaWx0ZXIgVmVyc2lvbiAx\
LjAKMjU2IDUwCjI1NQpMBwdMCQdMCgdMDAdMDgdMDwdMEQdMEwdMFAdMFgdM\
FwdMGQdMGwdMHAdMHgdMHwdMIQdMIwdMJAdMJgdMKAdMKQdMKwdMLAdMLgdM\
MAdMMQdMMwdMNQdMNgdMOAdMOQdMOwdMPQdMPgdMQAdMQgdMQwdMRQdMRgdM\
SAdMSgdMSwdMTAdKTAdJTAdHTAdFTAdETAdCTAdATAc/TAc9TAc8TAc6TAc4\
TAc3TAc1TAczTAcyTAcwTAcvTActTAcrTAcqTAcoTAcnTAclTAcjTAciTAcg\
TAceTAcdTAcbTAcaTAcYTAcWTAcVTAcTTAcRTAcQTAcOTAcNTAcLTAcJTAcI\
TAcHTAgHTAoHTAwHTA0HTA8HTBAHTBIHTBQHTBUHTBcHTBgHTBoHTBwHTB0H\
TB8HTCEHTCIHTCQHTCUHTCcHTCkHTCoHTCwHTC4HTC8HTDEHTDIHTDQHTDYH\
TDcHTDkHTDoHTDwHTD4HTD8HTEEHTEMHTEQHTEYHTEcHTEkHTEsHTEwHS0wH\
SUwHR0wHRkwHREwHQ0wHQUwHP0wHPkwHPEwHOkwHOUwHN0wHNkwHNEwHMkwH\
MUwHL0wHLkwHLEwHKkwHKUwHJ0wHJUwHJEwHIkwHIUwHH0wHHUwHHEwHGkwH\
GEwHF0wHFUwHFEwHEkwHEEwHD0wHDUwHDEwHCkwHCEwIB0wJB0wLB0wNB0wO\
B0wQB0wRB0wTB0wVB0wWB0wYB0waB0wbB0wdB0weB0wgB0wiB0wjB0wlB0wn\
B0woB0wqB0wrB0wtB0wvB0wwB0wyB0wzB0w1B0w3B0w4B0w6B0w8B0w9B0w/\
B0xAB0xCB0xEB0xFB0xHB0xJB0xKB0xMB0xMB0tMB0pMB0hMB0ZMB0VMB0NM\
B0JMB0BMBz5MBz1MBztMBzlMBzhMBzZMBzVMBzNMBzFMBzBMBy5MByxMBytM\
BylMByhMByZMByRMByNMByFMBx9MBx5MBxxMBxtMBxlMBxdMBxZMBxRMBxNM\
BxFMBw9MBw5MBwxMBwpMBwlTCAhTCghTCwhTDQhTDwhTEQhTEwhTFAhTFghT\
GAhTGghTGwhTHQhTHwhTIQhTIghTJAhTJghTKAhTKghTKwhTLQhTLwhTMQhT\
MghTNAhTNghTOAhTOQhTOwhTPQhTPwhTQQhTQghTRAhTRghTSAhTSQhTSwhT\
TQhTTwhTUQhTUghTUwhRUwhPUwhOUwhMUwhKUwhIUwhGUwhFUwhDUwhBUwg/\
Uwg+Uwg8Uwg6Uwg4Uwg3Uwg1UwgzUwgxUwgvUwguUwgsUwgqUwgoUwgnUwgl\
UwgjUwghUwggUwgeUwgcUwgaUwgYUwgXUwgVUwgTUwgRUwgQUwgOUwgMUwgK\
UwgIUwgIUwkIUwsIUw0IUw4IUxAIUxIIUxQIUxUIUxcIUxkIUxsIUx0IUx4I\
UyAIUyIIUyQIUyUIUycIUykIUysIUy0IUy4IUzAIUzIIUzQIUzUIUzcIUzkI\
UzsIUzwIUz4IU0AIU0IIU0QIU0UIU0cIU0kIU0sIU0wIU04IU1AIU1IIU1MI\
UlMIUFMITlMITFMIS1MISVMIR1MIRVMIRFMIQlMIQFMIPlMIPFMIO1MIOVMI\
N1MINVMINFMIMlMIMFMILlMILVMIK1MIKVMIJ1MIJVMIJFMIIlMIIFMIHlMI\
HVMIG1MIGVMIF1MIFVMIFFMIElMIEFMIDlMIDVMIC1MICVMICFMKCFMMCFMO\
CFMQCFMRCFMTCFMVCFMXCFMYCFMaCFMcCFMeCFMgCFMhCFMjCFMlCFMnCFMo\
CFMqCFMsCFMuCFMvCFMxCFMzCFM1CFM3CFM4CFM6CFM8CFM+CFM/CFNBCFND\
CFNFCFNGCFNICFNKCFNMCFNOCFNPCFNRCFNTCFNTCFJTCFFTCE9TCE1TCEtT\
CElTCEhTCEZTCERTCEJTCEFTCD9TCD1TCDtTCDlTCDhTCDZTCDRTCDJTCDFT\
CC9TCC1TCCtTCCpTCChTCCZTCCRTCCJTCCFTCB9TCB1TCBtTCBpTCBhTCBZT\
CBRTCBNTCBFTCA9TCA1TCAtTCApbCQlbCwlbDAlbDglbEAlbEglbFAlbFglb\
GAlbGglbHAlbHglbIAlbIglbJAlbJQlbJwlbKQlbKwlbLQlbLwlbMQlbMwlb\
NQlbNwlbOQlbOwlbPQlbPglbQAlbQglbRAlbRglbSAlbSglbTAlbTglbUAlb\
UglbVAlbVglbVwlbWQlaWwlYWwlWWwlUWwlSWwlQWwlOWwlNWwlLWwlJWwlH\
WwlFWwlDWwlBWwk/Wwk9Wwk7Wwk5Wwk3Wwk1Wwk0WwkyWwkwWwkuWwksWwkq\
WwkoWwkmWwkkWwkiWwkgWwkeWwkcWwkbWwkZWwkXWwkVWwkTWwkRWwkPWwkN\
WwkLWwkJWwkJWwoJWwwJWw4JWxAJWxIJWxQJWxUJWxcJWxkJWxsJWx0JWx8J\
WyEJWyMJWyUJWycJWykJWysJWywJWy4JWzAJWzIJWzQJWzYJWzgJWzoJWzwJ\
Wz4JW0AJW0IJW0QJW0UJW0cJW0kJW0sJW00JW08JW1EJW1MJW1UJW1cJW1kJ\
W1sJWVsJV1sJVVsJU1sJUVsJT1sJTVsJS1sJSVsJR1sJRVsJRFsJQlsJQFsJ\
PlsJPFsJOlsJOFsJNlsJNFsJMlsJMFsJLlsJLFsJK1sJKVsJJ1sJJVsJI1sJ\
IVsJH1sJHVsJG1sJGVsJF1sJFVsJFFsJElsJEFsJDlsJDFsJClsJCVsLCVsN\
CVsPCVsRCVsTCVsVCVsXCVsZCVsbCVscCVseCVsgCVsiCVskCVsmCVsoCVsq\
CVssCVsuCVswCVsyCVs0CVs1CVs3CVs5CVs7CVs9CVs/CVtBCVtDCVtFCVtH\
CVtJCVtLCVtNCVtOCVtQCVtSCVtUCVtWCVtYCVtaCVtbCVlbCVdbCVZbCVRb\
CVJbCVBbCU5bCUxbCUpbCUhbCUZbCURbCUJbCUBbCT5bCT1bCTtbCTlbCTdb\
CTVbCTNbCTFbCS9bCS1bCStbCSlbCSdbCSVbCSRbCSJbCSBbCR5bCRxbCRpb\
CRhbCRZbCRRbCRJbCRBbCQ5bCQxbCQtiCQliCwliDQliEAliEgliFAliFgli\
GAliGgliHAliHgliIAliIgliJAliJgliKAliKwliLQliLwliMQliMwliNQli\
NwliOQliOwliPQliPwliQQliQwliRQliSAliSgliTAliTgliUAliUgliVAli\
VgliWAliWgliXAliXgliYAlhYglfYgldYglbYglZYglXYglVYglTYglRYglO\
YglMYglKYglIYglGYglEYglCYglAYgk+Ygk8Ygk6Ygk4Ygk2YgkzYgkxYgkv\
YgktYgkrYgkpYgknYgklYgkjYgkhYgkfYgkdYgkbYgkZYgkWYgkUYgkSYgkQ\
YgkOYgkMYgkKYgkJYgsJYg0JYg8JYhEJYhMJYhUJYhcJYhkJYhsJYh0JYh8J\
YiIJYiQJYiYJYigJYioJYiwJYi4JYjAJYjIJYjQJYjYJYjgJYjoJYjwJYj8J\
YkEJYkMJYkUJYkcJYkkJYksJYk0JYk8JYlEJYlMJYlUJYlcJYloJYlwJYl4J\
YmAJYmIJYGIJXmIJXGIJWmIJV2IJVWIJU2IJUWIJT2IJTWIJS2IJSWIJR2IJ\
RWIJQ2IJQWIJP2IJPGIJOmIJOGIJNmIJNGIJMmIJMGIJLmIJLGIJKmIJKGIJ\
JmIJJGIJImIJH2IJHWIJG2IJGWIJF2IJFWIJE2IJEWIJD2IJDWIJC2IKCWIM\
CWIOCWIQCWISCWIUCWIWCWIZCWIbCWIdCWIfCWIhCWIjCWIlCWInCWIpCWIr\
CWItCWIvCWIxCWIzCWI2CWI4CWI6CWI8CWI+CWJACWJCCWJECWJGCWJICWJK\
CWJMCWJOCWJRCWJTCWJVCWJXCWJZCWJbCWJdCWJfCWJhCWJiCWBiCV5iCVxi\
CVpiCVhiCVZiCVRiCVJiCVBiCU5iCUxiCUpiCUhiCUViCUNiCUFiCT9iCT1i\
CTtiCTliCTdiCTViCTNiCTFiCS9iCS1iCStiCShiCSZiCSRiCSJiCSBiCR5i\
CRxiCRpiCRhiCRZiCRRiCRJiCRBiCQ1iCQtpCgppDAppDgppEQppEwppFQpp\
FwppGgppHAppHgppIAppIwppJQppJwppKQppKwppLgppMAppMgppNAppNwpp\
OQppOwppPQppPwppQgppRAppRgppSAppSwppTQppTwppUQppUwppVgppWApp\
WgppXAppXwppYQppYwppZQppZwpoaQpmaQpkaQpiaQpfaQpdaQpbaQpZaQpW\
aQpUaQpSaQpQaQpOaQpLaQpJaQpHaQpFaQpCaQpAaQo+aQo8aQo6aQo3aQo1\
aQozaQoxaQouaQosaQoqaQooaQolaQojaQohaQofaQodaQoaaQoYaQoWaQoU\
aQoRaQoPaQoNaQoLaQoKaQwKaQ4KaRAKaRIKaRQKaRcKaRkKaRsKaR0KaSAK\
aSIKaSQKaSYKaSgKaSsKaS0KaS8KaTEKaTQKaTYKaTgKaToKaTwKaT8KaUEK\
aUMKaUUKaUgKaUoKaUwKaU4KaVAKaVMKaVUKaVcKaVkKaVwKaV4KaWAKaWIK\
aWUKaWcKaWkKZ2kKZWkKYmkKYGkKXmkKXGkKWWkKV2kKVWkKU2kKUGkKTmkK\
TGkKSmkKSGkKRWkKQ2kKQWkKP2kKPGkKOmkKOGkKNmkKNGkKMWkKL2kKLWkK\
K2kKKGkKJmkKJGkKImkKIGkKHWkKG2kKGWkKF2kKFGkKEmkKEGkKDmkKDGkL\
CmkNCmkPCmkRCmkUCmkWCmkYCmkaCmkdCmkfCmkhCmkjCmklCmkoCmkqCmks\
CmkuCmkxCmkzCmk1Cmk3Cmk6Cmk8Cmk+CmlACmlCCmlFCmlHCmlJCmlLCmlO\
CmlQCmlSCmlUCmlWCmlZCmlbCmldCmlfCmliCmlkCmlmCmloCmlpCmdpCmVp\
CmNpCmFpCl9pClxpClppClhpClZpClNpClFpCk9pCk1pCktpCkhpCkZpCkRp\
CkJpCj9pCj1pCjtpCjlpCjdpCjRpCjJpCjBpCi5pCitpCilpCidpCiVpCiNp\
CiBpCh5pChxpChppChdpChVpChNpChFpCg5pCgxwCwtwDQtwEAtwEgtwFAtw\
FwtwGQtwGwtwHgtwIAtwIwtwJQtwJwtwKgtwLAtwLgtwMQtwMwtwNgtwOAtw\
OgtwPQtwPwtwQQtwRAtwRgtwSQtwSwtwTQtwUAtwUgtwVAtwVwtwWQtwXAtw\
XgtwYAtwYwtwZQtwZwtwagtwbAtwbwtvcAttcAtrcAtocAtmcAtjcAthcAtf\
cAtccAtacAtYcAtVcAtTcAtQcAtOcAtMcAtJcAtHcAtFcAtCcAtAcAs9cAs7\
cAs5cAs2cAs0cAsycAsvcAstcAsqcAsocAsmcAsjcAshcAsfcAsccAsacAsX\
cAsVcAsTcAsQcAsOcAsMcAsLcAwLcA8LcBELcBMLcBYLcBgLcBsLcB0LcB8L\
cCILcCQLcCYLcCkLcCsLcC4LcDALcDILcDULcDcLcDkLcDwLcD4LcEELcEML\
cEULcEgLcEoLcE0LcE8LcFELcFQLcFYLcFgLcFsLcF0LcGALcGILcGQLcGcL\
cGkLcGsLcG4LcHALbnALa3ALaXALZ3ALZHALYnALYHALXXALW3ALWHALVnAL\
VHALUXALT3ALTXALSnALSHALRXALQ3ALQXALPnALPHALOXALN3ALNXALMnAL\
MHALLnALK3ALKXALJnALJHALInALH3ALHXALG3ALGHALFnALE3ALEXALD3AL\
DHAMC3AOC3AQC3ATC3AVC3AXC3AaC3AcC3AfC3AhC3AjC3AmC3AoC3AqC3At\
C3AvC3AyC3A0C3A2C3A5C3A7C3A9C3BAC3BCC3BFC3BHC3BJC3BMC3BOC3BQ\
C3BTC3BVC3BYC3BaC3BcC3BfC3BhC3BjC3BmC3BoC3BrC3BtC3BvC3BwC29w\
C2xwC2pwC2dwC2VwC2NwC2BwC15wC1xwC1lwC1dwC1RwC1JwC1BwC01wC0tw\
C0lwC0ZwC0RwC0FwCz9wCz1wCzpwCzhwCzZwCzNwCzFwCy5wCyxwCypwCydw\
CyVwCyNwCyBwCx5wCxtwCxlwCxdwCxRwCxJwCxBwCw13Cwt3Dgt3EQt3Ewt3\
Fgt3GAt3Gwt3HQt3IAt3Igt3JQt3Jwt3Kgt3LAt3Lwt3MQt3NAt3Ngt3OQt3\
Owt3Pgt3QQt3Qwt3Rgt3SAt3Swt3TQt3UAt3Ugt3VQt3Vwt3Wgt3XAt3Xwt3\
YQt3ZAt3Zgt3aQt3bAt3bgt3cQt3cwt3dgt2dwt0dwtxdwtvdwtsdwtqdwtn\
dwtldwtidwtgdwtddwtbdwtYdwtWdwtTdwtRdwtOdwtMdwtJdwtGdwtEdwtB\
dws/dws8dws6dws3dws1dwsydwswdwstdwsrdwsodwsmdwsjdwshdwsedwsb\
dwsZdwsWdwsUdwsRdwsPdwsMdwsLdw0LdxALdxILdxULdxcLdxoLdxwLdx8L\
dyELdyQLdyYLdykLdysLdy4LdzELdzMLdzYLdzgLdzsLdz0Ld0ALd0ILd0UL\
d0cLd0oLd0wLd08Ld1ELd1QLd1YLd1kLd1wLd14Ld2ELd2MLd2YLd2gLd2sL\
d20Ld3ALd3ILd3ULd3cLdXcLcncLcHcLbXcLa3cLaHcLZncLY3cLYXcLXncL\
XHcLWXcLVncLVHcLUXcLT3cLTHcLSncLR3cLRXcLQncLQHcLPXcLO3cLOHcL\
NncLM3cLMXcLLncLK3cLKXcLJncLJHcLIXcLH3cLHHcLGncLF3cLFXcLEncL\
EHcLDXcMC3cPC3cRC3cUC3cWC3cZC3cbC3ceC3chC3cjC3cmC3coC3crC3ct\
C3cwC3cyC3c1C3c3C3c6C3c8C3c/C3dBC3dEC3dGC3dJC3dMC3dOC3dRC3dT\
C3dWC3dYC3dbC3ddC3dgC3diC3dlC3dnC3dqC3dsC3dvC3dxC3d0C3d2C3d3\
C3Z3C3N3C3F3C253C2x3C2l3C2Z3C2R3C2F3C193C1x3C1p3C1d3C1V3C1J3\
C1B3C013C0t3C0h3C0Z3C0N3C0F3Cz53Czt3Czl3CzZ3CzR3CzF3Cy93Cyx3\
Cyp3Cyd3CyV3CyJ3CyB3Cx13Cxt3Cxh3CxZ3CxN3CxF3Cw5+DAx+Dwx+Egx+\
FAx+Fwx+Ggx+HAx+Hwx+Igx+JAx+Jwx+Kgx+LAx+Lwx+Mgx+NAx+Nwx+Ogx+\
PAx+Pwx+Qgx+RAx+Rwx+Sgx+TAx+Twx+Ugx+VQx+Vwx+Wgx+XQx+Xwx+Ygx+\
ZQx+Zwx+agx+bQx+bwx+cgx+dQx+dwx+egx+fQx+fgx7fgx4fgx2fgxzfgxw\
fgxufgxrfgxofgxlfgxjfgxgfgxdfgxbfgxYfgxVfgxTfgxQfgxNfgxLfgxI\
fgxFfgxDfgxAfgw9fgw7fgw4fgw1fgwzfgwwfgwtfgwrfgwofgwlfgwjfgwg\
fgwdfgwafgwYfgwVfgwSfgwQfgwNfgwMfg4MfhEMfhMMfhYMfhkMfhsMfh4M\
fiEMfiMMfiYMfikMfisMfi4MfjEMfjMMfjYMfjkMfjwMfj4MfkEMfkQMfkYM\
fkkMfkwMfk4MflEMflQMflYMflkMflwMfl4MfmEMfmQMfmYMfmkMfmwMfm4M\
fnEMfnQMfnYMfnkMfnwMfn4MfH4MeX4Mdn4MdH4McX4Mbn4MbH4MaX4MZn4M\
ZH4MYX4MXn4MXH4MWX4MVn4MVH4MUX4MTn4MTH4MSX4MRn4MRH4MQX4MPn4M\
PH4MOX4MNn4MM34MMX4MLn4MK34MKX4MJn4MI34MIX4MHn4MG34MGX4MFn4M\
E34MEX4MDn4NDH4QDH4SDH4VDH4YDH4aDH4dDH4gDH4jDH4lDH4oDH4rDH4t\
DH4wDH4zDH41DH44DH47DH49DH5ADH5DDH5FDH5IDH5LDH5NDH5QDH5TDH5V\
DH5YDH5bDH5dDH5gDH5jDH5lDH5oDH5rDH5uDH5wDH5zDH52DH54DH57DH5+\
DH5+DH1+DHp+DHd+DHV+DHJ+DG9+DG1+DGp+DGd+DGV+DGJ+DF9+DF1+DFp+\
DFd+DFV+DFJ+DE9+DEx+DEp+DEd+DER+DEJ+DD9+DDx+DDp+DDd+DDR+DDJ+\
DC9+DCx+DCp+DCd+DCR+DCJ+DB9+DBx+DBp+DBd+DBR+DBJ+DA+GDQ2GEA2G\
Ew2GFQ2GGA2GGw2GHg2GIQ2GJA2GJg2GKQ2GLA2GLw2GMg2GNQ2GNw2GOg2G\
PQ2GQA2GQw2GRg2GSA2GSw2GTg2GUQ2GVA2GVg2GWQ2GXA2GXw2GYg2GZQ2G\
Zw2Gag2GbQ2GcA2Gcw2Gdg2GeA2Gew2Gfg2GgQ2GhA2Fhg2Chg1/hg18hg15\
hg13hg10hg1xhg1uhg1rhg1ohg1mhg1jhg1ghg1dhg1ahg1Xhg1Vhg1Shg1P\
hg1Mhg1Jhg1Ghg1Ehg1Bhg0+hg07hg04hg01hg0zhg0whg0thg0qhg0nhg0k\
hg0ihg0fhg0chg0Zhg0Whg0Uhg0Rhg0Ohg0Nhg8NhhINhhQNhhcNhhoNhh0N\
hiANhiMNhiUNhigNhisNhi4NhjENhjQNhjYNhjkNhjwNhj8NhkINhkUNhkcN\
hkoNhk0NhlANhlMNhlYNhlgNhlsNhl4NhmENhmQNhmcNhmkNhmwNhm8NhnIN\
hnUNhncNhnoNhn0NhoANhoMNhoYNg4YNgIYNfYYNeoYNd4YNdYYNcoYNb4YN\
bIYNaYYNZ4YNZIYNYYYNXoYNW4YNWIYNVoYNU4YNUIYNTYYNSoYNR4YNRYYN\
QoYNP4YNPIYNOYYNNoYNNIYNMYYNLoYNK4YNKIYNJYYNI4YNIIYNHYYNGoYN\
F4YNFIYNEoYND4YODYYRDYYUDYYWDYYZDYYcDYYfDYYiDYYkDYYnDYYqDYYt\
DYYwDYYzDYY1DYY4DYY7DYY+DYZBDYZEDYZGDYZJDYZMDYZPDYZSDYZVDYZX\
DYZaDYZdDYZgDYZjDYZmDYZoDYZrDYZuDYZxDYZ0DYZ3DYZ5DYZ8DYZ/DYaC\
DYaFDYaGDYSGDYGGDX6GDXuGDXiGDXaGDXOGDXCGDW2GDWqGDWeGDWWGDWKG\
DV+GDVyGDVmGDVaGDVSGDVGGDU6GDUuGDUiGDUaGDUOGDUCGDT2GDTqGDTeG\
DTWGDTKGDS+GDSyGDSmGDSaGDSSGDSGGDR6GDRuGDRiGDRWGDROGDRCNDg6N\
EQ6NFA6NFw6NGg6NHQ6NIA6NIg6NJQ6NKA6NKw6NLg6NMQ6NNA6NNw6NOg6N\
PQ6NQA6NQw6NRg6NSQ6NTA6NTw6NUg6NVQ6NWA6NWw6NXg6NYQ6NZA6NZw6N\
ag6NbQ6NcA6Ncw6Ndg6NeQ6NfA6Nfw6Ngg6NhQ6NiA6Niw6MjQ6JjQ6GjQ6D\
jQ6AjQ59jQ56jQ53jQ50jQ5xjQ5ujQ5rjQ5ojQ5ljQ5ijQ5fjQ5cjQ5ZjQ5W\
jQ5TjQ5QjQ5NjQ5KjQ5HjQ5EjQ5BjQ4+jQ47jQ44jQ41jQ4yjQ4vjQ4sjQ4p\
jQ4mjQ4jjQ4hjQ4ejQ4bjQ4YjQ4VjQ4SjQ4PjQ4OjRAOjRMOjRYOjRkOjRwO\
jR8OjSIOjSQOjScOjSoOjS0OjTAOjTMOjTYOjTkOjTwOjT8OjUIOjUUOjUgO\
jUsOjU4OjVEOjVQOjVcOjVoOjV0OjWAOjWMOjWYOjWkOjWwOjW8OjXIOjXUO\
jXgOjXsOjX4OjYEOjYQOjYcOjYoOjY0Oio0Oh40OhI0OgY0Ofo0Oe40OeI0O\
dY0Oco0Ob40ObI0OaY0OZo0OY40OYI0OXY0OWo0OV40OVI0OUY0OTo0OS40O\
SI0ORY0OQo0OP40OPI0OOY0ONo0OM40OMI0OLY0OKo0OJ40OJI0OIo0OH40O\
HI0OGY0OFo0OE40OEI0PDo0SDo0VDo0YDo0bDo0eDo0hDo0jDo0mDo0pDo0s\
Do0vDo0yDo01Do04Do07Do0+Do1BDo1EDo1HDo1KDo1NDo1QDo1TDo1WDo1Z\
Do1cDo1fDo1iDo1lDo1oDo1rDo1uDo1xDo10Do13Do16Do19Do2ADo2DDo2G\
Do2JDo2MDo2NDouNDoiNDoWNDoKNDn+NDnyNDnmNDnaNDnONDnCNDm2NDmqN\
DmeNDmSNDmGNDl6NDluNDliNDlWNDlKNDk+NDkyNDkmNDkaNDkONDkCNDj2N\
DjqNDjeNDjSNDjGNDi6NDiuNDiiNDiWNDiKNDiCNDh2NDhqNDheNDhSNDhGU\
Dg6UEQ6UFQ6UGA6UGw6UHg6UIQ6UJA6UJw6UKw6ULg6UMQ6UNA6UNw6UOg6U\
PQ6UQA6URA6URw6USg6UTQ6UUA6UUw6UVg6UWg6UXQ6UYA6UYw6UZg6UaQ6U\
bA6Ubw6Ucw6Udg6UeQ6UfA6Ufw6Ugg6UhQ6UiA6UjA6Ujw6Ukg6TlA6QlA6N\
lA6KlA6GlA6DlA6AlA59lA56lA53lA50lA5wlA5tlA5qlA5nlA5klA5hlA5e\
lA5blA5XlA5UlA5RlA5OlA5LlA5IlA5FlA5ClA4+lA47lA44lA41lA4ylA4v\
lA4slA4olA4llA4ilA4flA4clA4ZlA4WlA4TlA4PlA4OlBAOlBQOlBcOlBoO\
lB0OlCAOlCMOlCYOlCkOlC0OlDAOlDMOlDYOlDkOlDwOlD8OlEMOlEYOlEkO\
lEwOlE8OlFIOlFUOlFgOlFwOlF8OlGIOlGUOlGgOlGsOlG4OlHIOlHUOlHgO\
lHsOlH4OlIEOlIQOlIcOlIsOlI4OlJEOlJQOkZQOjpQOi5QOh5QOhJQOgZQO\
fpQOe5QOeJQOdZQOcpQObpQOa5QOaJQOZZQOYpQOX5QOXJQOWJQOVZQOUpQO\
T5QOTJQOSZQORpQOQ5QOP5QOPJQOOZQONpQOM5QOMJQOLZQOKZQOJpQOI5QO\
IJQOHZQOGpQOF5QOFJQOEJQPDpQTDpQWDpQZDpQcDpQfDpQiDpQlDpQoDpQs\
DpQvDpQyDpQ1DpQ4DpQ7DpQ+DpRCDpRFDpRIDpRLDpRODpRRDpRUDpRXDpRb\
DpReDpRhDpRkDpRnDpRqDpRtDpRwDpR0DpR3DpR6DpR9DpSADpSDDpSGDpSK\
DpSNDpSQDpSTDpSUDpKUDo+UDoyUDoiUDoWUDoKUDn+UDnyUDnmUDnaUDnOU\
Dm+UDmyUDmmUDmaUDmOUDmCUDl2UDlqUDlaUDlOUDlCUDk2UDkqUDkeUDkSU\
DkCUDj2UDjqUDjeUDjSUDjGUDi6UDiuUDieUDiSUDiGUDh6UDhuUDhiUDhWU\
DhGbDw+bEg+bFg+bGQ+bHA+bHw+bIw+bJg+bKQ+bLQ+bMA+bMw+bNg+bOg+b\
PQ+bQA+bRA+bRw+bSg+bTQ+bUQ+bVA+bVw+bWw+bXg+bYQ+bZA+baA+baw+b\
bg+bcg+bdQ+beA+bew+bfw+bgg+bhQ+biQ+bjA+bjw+bkg+blg+bmQ+amw+X\
mw+Tmw+Qmw+Nmw+Kmw+Gmw+Dmw+Amw99mw95mw92mw9zmw9vmw9smw9pmw9m\
mw9imw9fmw9cmw9Ymw9Vmw9Smw9Pmw9Lmw9Imw9Fmw9Bmw8+mw87mw84mw80\
mw8xmw8umw8qmw8nmw8kmw8hmw8dmw8amw8Xmw8Tmw8Qmw8PmxEPmxUPmxgP\
mxsPmx4PmyIPmyUPmygPmywPmy8PmzIPmzUPmzkPmzwPmz8Pm0MPm0YPm0kP\
m0wPm1APm1MPm1YPm1kPm10Pm2APm2MPm2cPm2oPm20Pm3APm3QPm3cPm3oP\
m34Pm4EPm4QPm4cPm4sPm44Pm5EPm5UPm5gPm5sPmJsPlZsPkZsPjpsPi5sP\
h5sPhJsPgZsPfpsPepsPd5sPdJsPcJsPbZsPapsPZ5sPY5sPYJsPXZsPWZsP\
VpsPU5sPUJsPTJsPSZsPRpsPQ5sPP5sPPJsPOZsPNZsPMpsPL5sPLJsPKJsP\
JZsPIpsPHpsPG5sPGJsPFZsPEZsQD5sTD5sXD5saD5sdD5shD5skD5snD5sq\
D5suD5sxD5s0D5s4D5s7D5s+D5tBD5tFD5tID5tLD5tPD5tSD5tVD5tYD5tc\
D5tfD5tiD5tmD5tpD5tsD5tvD5tzD5t2D5t5D5t9D5uAD5uDD5uGD5uKD5uN\
D5uQD5uTD5uXD5uaD5ubD5mbD5abD5KbD4+bD4ybD4mbD4WbD4KbD3+bD3ub\
D3ibD3WbD3KbD26bD2ubD2ibD2SbD2GbD16bD1ubD1ebD1SbD1GbD02bD0qb\
D0ebD0SbD0CbDz2bDzqbDzabDzObDzCbDy2bDymbDyabDyObDx+bDxybDxmb\
DxabDxKiEBCiExCiFxCiGhCiHhCiIRCiJBCiKBCiKxCiLxCiMhCiNhCiORCi\
PBCiQBCiQxCiRxCiShCiThCiURCiVBCiWBCiWxCiXxCiYhCiZhCiaRCibRCi\
cBCicxCidxCiehCifhCigRCihRCiiBCiixCijxCikhCilhCimRCinRCioBCh\
ohCeohCaohCXohCTohCQohCNohCJohCGohCCohB/ohB7ohB4ohB1ohBxohBu\
ohBqohBnohBjohBgohBcohBZohBWohBSohBPohBLohBIohBEohBBohA+ohA6\
ohA3ohAzohAwohAsohApohAmohAiohAfohAbohAYohAUohARohAQohIQohYQ\
ohkQohwQoiAQoiMQoicQoioQoi4QojEQojQQojgQojsQoj8QokIQokYQokkQ\
okwQolAQolMQolcQoloQol4QomEQomQQomgQomsQom8QonIQonYQonkQon0Q\
ooAQooMQoocQoooQoo4QopEQopUQopgQopsQop8QoqIQn6IQm6IQmKIQlaIQ\
kaIQjqIQiqIQh6IQg6IQgKIQfaIQeaIQdqIQcqIQb6IQa6IQaKIQZKIQYaIQ\
XqIQWqIQV6IQU6IQUKIQTKIQSaIQRqIQQqIQP6IQO6IQOKIQNKIQMaIQLqIQ\
KqIQJ6IQI6IQIKIQHKIQGaIQFqIQEqIREKIUEKIYEKIbEKIfEKIiEKImEKIp\
EKIsEKIwEKIzEKI3EKI6EKI+EKJBEKJEEKJIEKJLEKJPEKJSEKJWEKJZEKJc\
EKJgEKJjEKJnEKJqEKJuEKJxEKJ1EKJ4EKJ7EKJ/EKKCEKKGEKKJEKKNEKKQ\
EKKTEKKXEKKaEKKeEKKhEKKiEKCiEJ2iEJmiEJaiEJKiEI+iEIuiEIiiEIWi\
EIGiEH6iEHqiEHeiEHOiEHCiEG2iEGmiEGaiEGKiEF+iEFuiEFiiEFSiEFGi\
EE6iEEqiEEeiEEOiEECiEDyiEDmiEDaiEDKiEC+iECuiECiiECSiECGiEB6i\
EBqiEBeiEBOpEBCpFBCpGBCpGxCpHxCpIhCpJhCpKhCpLRCpMRCpNBCpOBCp\
PBCpPxCpQxCpRhCpShCpTRCpURCpVRCpWBCpXBCpXxCpYxCpZxCpahCpbhCp\
cRCpdRCpeBCpfBCpgBCpgxCphxCpihCpjhCpkhCplRCpmRCpnBCpoBCppBCp\
pxCoqRClqRChqRCeqRCaqRCWqRCTqRCPqRCMqRCIqRCEqRCBqRB9qRB6qRB2\
qRBzqRBvqRBrqRBoqRBkqRBhqRBdqRBZqRBWqRBSqRBPqRBLqRBHqRBEqRBA\
qRA9qRA5qRA2qRAyqRAuqRArqRAnqRAkqRAgqRAcqRAZqRAVqRASqRAQqRMQ\
qRYQqRoQqR4QqSEQqSUQqSgQqSwQqTAQqTMQqTcQqToQqT4QqUIQqUUQqUkQ\
qUwQqVAQqVMQqVcQqVsQqV4QqWIQqWUQqWkQqW0QqXAQqXQQqXcQqXsQqX4Q\
qYIQqYYQqYkQqY0QqZAQqZQQqZgQqZsQqZ8QqaIQqaYQqakQpqkQoqkQn6kQ\
m6kQmKkQlKkQkKkQjakQiakQhqkQgqkQfqkQe6kQd6kQdKkQcKkQbakQaakQ\
ZakQYqkQXqkQW6kQV6kQU6kQUKkQTKkQSakQRakQQqkQPqkQOqkQN6kQM6kQ\
MKkQLKkQKKkQJakQIakQHqkQGqkQFqkQE6kSEKkVEKkZEKkcEKkgEKkkEKkn\
EKkrEKkuEKkyEKk2EKk5EKk9EKlAEKlEEKlHEKlLEKlPEKlSEKlWEKlZEKld\
EKlhEKlkEKloEKlrEKlvEKlzEKl2EKl6EKl9EKmBEKmEEKmIEKmMEKmPEKmT\
EKmWEKmaEKmeEKmhEKmlEKmoEKmpEKepEKSpEKCpEJypEJmpEJWpEJKpEI6p\
EIqpEIepEIOpEICpEHypEHipEHWpEHGpEG6pEGqpEGepEGOpEF+pEFypEFip\
EFWpEFGpEE2pEEqpEEapEEOpED+pEDypEDipEDSpEDGpEC2pECqpECapECKp\
EB+pEBupEBipEBSxERGxFRGxGRGxHBGxIBGxJBGxKBGxKxGxLxGxMxGxNxGx\
OhGxPhGxQhGxRhGxSRGxTRGxURGxVBGxWBGxXBGxYBGxYxGxZxGxaxGxbxGx\
chGxdhGxehGxfhGxgRGxhRGxiRGxjRGxkBGxlBGxmBGxmxGxnxGxoxGxpxGx\
qhGxrhGvsRGssRGosRGksRGgsRGdsRGZsRGVsRGSsRGOsRGKsRGGsRGDsRF/\
sRF7sRF3sRF0sRFwsRFssRFosRFlsRFhsRFdsRFZsRFWsRFSsRFOsRFLsRFH\
sRFDsRE/sRE8sRE4sRE0sREwsREtsREpsRElsREhsREesREasREWsRESsRER\
sRQRsRcRsRsRsR8RsSMRsSYRsSoRsS4RsTIRsTURsTkRsT0RsUERsUQRsUgR\
sUwRsU8RsVMRsVcRsVsRsV4RsWIRsWYRsWoRsW0RsXERsXURsXkRsXwRsYAR\
sYQRsYgRsYsRsY8RsZMRsZYRsZoRsZ4RsaIRsaURsakRsa0RsbERrbERqbER\
pbERorERnrERmrERlrERk7ERj7ERi7ERiLERhLERgLERfLERebERdbERcbER\
bbERarERZrERYrERXrERW7ERV7ERU7ERT7ERTLERSLERRLERQbERPbERObER\
NbERMrERLrERKrERJrERI7ERH7ERG7ERF7ERFLESEbEWEbEaEbEeEbEhEbEl\
EbEpEbEtEbEwEbE0EbE4EbE8EbE/EbFDEbFHEbFLEbFOEbFSEbFWEbFZEbFd\
EbFhEbFlEbFoEbFsEbFwEbF0EbF3EbF7EbF/EbGDEbGGEbGKEbGOEbGSEbGV\
EbGZEbGdEbGgEbGkEbGoEbGsEbGvEbGxEa6xEaqxEaexEaOxEZ+xEZuxEZix\
EZSxEZCxEY2xEYmxEYWxEYGxEX6xEXqxEXaxEXKxEW+xEWuxEWexEWOxEWCx\
EVyxEVixEVSxEVGxEU2xEUmxEUaxEUKxET6xETqxETexETOxES+xESuxESix\
ESSxESCxERyxERmxERW4EhK4FhK4GhK4HhK4IRK4JRK4KRK4LRK4MRK4NRK4\
ORK4PRK4QRK4RBK4SBK4TBK4UBK4VBK4WBK4XBK4YBK4ZBK4ZxK4axK4bxK4\
cxK4dxK4exK4fxK4gxK4hxK4ihK4jhK4khK4lhK4mhK4nhK4ohK4phK4qhK4\
rRK4sRK4tRK3uBKzuBKvuBKruBKnuBKjuBKfuBKbuBKXuBKUuBKQuBKMuBKI\
uBKEuBKAuBJ8uBJ4uBJ0uBJxuBJtuBJpuBJluBJhuBJduBJZuBJVuBJRuBJO\
uBJKuBJGuBJCuBI+uBI6uBI2uBIyuBIuuBIruBInuBIjuBIfuBIbuBIXuBIT\
uBISuBUSuBgSuBwSuCASuCQSuCgSuCwSuDASuDQSuDgSuDsSuD8SuEMSuEcS\
uEsSuE8SuFMSuFcSuFsSuF4SuGISuGYSuGoSuG4SuHISuHYSuHoSuH4SuIES\
uIUSuIkSuI0SuJESuJUSuJkSuJ0SuKASuKQSuKgSuKwSuLASuLQSuLgStLgS\
sLgSrLgSqLgSpLgSoLgSnbgSmbgSlbgSkbgSjbgSibgShbgSgbgSfrgSergS\
drgScrgSbrgSargSZrgSYrgSXrgSW7gSV7gSU7gST7gSS7gSR7gSQ7gSP7gS\
O7gSOLgSNLgSMLgSLLgSKLgSJLgSILgSHLgSGLgSFbgTErgXErgbErgfErgj\
ErgnErgrErguErgyErg2Erg6Erg+ErhCErhGErhKErhOErhRErhVErhZErhd\
ErhhErhlErhpErhtErhxErh0Erh4Erh8EriAEriEEriIEriMEriQEriUEriX\
EribErifErijErinErirErivErizEri3Eri4ErW4ErG4Eq24Eqq4Eqa4EqK4\
Ep64Epq4Epa4EpK4Eo64Eoq4Eoe4EoO4En+4Enu4Ene4EnO4Em+4Emu4Eme4\
EmS4EmC4Ely4Eli4ElS4ElC4Eky4Eki4EkS4EkG4Ej24Ejm4EjW4EjG4Ei24\
Eim4EiW4EiG4Eh64Ehq4Eha/ExO/FxO/GxO/HxO/IxO/JxO/KxO/LxO/MxO/\
NxO/OxO/PxO/QxO/RxO/SxO/TxO/UxO/VxO/WxO/XxO/YxO/ZxO/bBO/cBO/\
dBO/eBO/fBO/gBO/hBO/iBO/jBO/kBO/lBO/mBO/nBO/oBO/pBO/qBO/rBO/\
sBO/tBO/uBO/vBO+vxO6vxO2vxOyvxOtvxOpvxOlvxOhvxOdvxOZvxOVvxOR\
vxONvxOJvxOFvxOBvxN9vxN5vxN1vxNxvxNtvxNpvxNlvxNhvxNdvxNZvxNV\
vxNRvxNNvxNJvxNEvxNAvxM8vxM4vxM0vxMwvxMsvxMovxMkvxMgvxMcvxMY\
vxMUvxMTvxUTvxkTvx0TvyETvyUTvyoTvy4TvzITvzYTvzoTvz4Tv0ITv0YT\
v0oTv04Tv1ITv1YTv1oTv14Tv2ITv2YTv2oTv24Tv3ITv3YTv3oTv34Tv4IT\
v4YTv4oTv48Tv5MTv5cTv5sTv58Tv6MTv6cTv6sTv68Tv7MTv7cTv7sTv78T\
u78Tt78Ts78Tr78Tq78Tp78To78Tn78Tm78Tl78Tk78Tj78Tir8Thr8Tgr8T\
fr8Ter8Tdr8Tcr8Tbr8Tar8TZr8TYr8TXr8TWr8TVr8TUr8TTr8TSr8TRr8T\
Qr8TPr8TOr8TNr8TMr8TLr8TKr8TJb8TIb8THb8TGb8TFb8UE78YE78cE78g\
E78kE78oE78sE78wE780E784E788E79AE79EE79JE79NE79RE79VE79ZE79d\
E79hE79lE79pE79tE79xE791E795E799E7+BE7+FE7+JE7+NE7+RE7+VE7+Z\
E7+dE7+hE7+lE7+pE7+tE7+yE7+2E7+6E7++E7+/E7y/E7i/E7S/E7C/E6y/\
E6i/E6S/E6C/E5y/E5i/E5S/E5C/E4y/E4i/E4S/E4C/E3y/E3i/E3S/E3C/\
E2y/E2e/E2O/E1+/E1u/E1e/E1O/E0+/E0u/E0e/E0O/Ez+/Ezu/Eze/EzO/\
Ey+/Eyu/Eye/EyO/Ex+/Exu/ExfGExPGGBPGHBPGIBPGJBPGKBPGLRPGMRPG\
NRPGORPGPRPGQRPGRhPGShPGThPGUhPGVhPGWxPGXxPGYxPGZxPGaxPGcBPG\
dBPGeBPGfBPGgBPGhRPGiRPGjRPGkRPGlRPGmRPGnhPGohPGphPGqhPGrhPG\
sxPGtxPGuxPGvxPGwxPFxhPBxhO8xhO4xhO0xhOwxhOsxhOnxhOjxhOfxhOb\
xhOXxhOSxhOOxhOKxhOGxhOCxhN+xhN5xhN1xhNxxhNtxhNpxhNkxhNgxhNc\
xhNYxhNUxhNPxhNLxhNHxhNDxhM/xhM6xhM2xhMyxhMuxhMqxhMmxhMhxhMd\
xhMZxhMVxhMTxhYTxhoTxh8TxiMTxicTxisTxi8TxjMTxjgTxjwTxkATxkQT\
xkgTxk0TxlETxlUTxlkTxl0TxmITxmYTxmoTxm4TxnITxncTxnsTxn8TxoMT\
xocTxosTxpATxpQTxpgTxpwTxqATxqUTxqkTxq0TxrETxrUTxroTxr4TxsIT\
xsYTwsYTvsYTusYTtcYTscYTrcYTqcYTpcYToMYTnMYTmMYTlMYTkMYTi8YT\
h8YTg8YTf8YTe8YTd8YTcsYTbsYTasYTZsYTYsYTXcYTWcYTVcYTUcYTTcYT\
SMYTRMYTQMYTPMYTOMYTM8YTL8YTK8YTJ8YTI8YTH8YTGsYTFsYVE8YZE8Yd\
E8YhE8YmE8YqE8YuE8YyE8Y2E8Y6E8Y/E8ZDE8ZHE8ZLE8ZPE8ZUE8ZYE8Zc\
E8ZgE8ZkE8ZpE8ZtE8ZxE8Z1E8Z5E8Z+E8aCE8aGE8aKE8aOE8aSE8aXE8ab\
E8afE8ajE8anE8asE8awE8a0E8a4E8a8E8bBE8bFE8bGE8PGE7/GE7vGE7fG\
E7PGE67GE6rGE6bGE6LGE57GE5nGE5XGE5HGE43GE4nGE4XGE4DGE3zGE3jG\
E3TGE3DGE2vGE2fGE2PGE1/GE1vGE1bGE1LGE07GE0rGE0bGE0HGEz3GEznG\
EzXGEzHGEy3GEyjGEyTGEyDGExzGExjNFBTNGBTNHRTNIRTNJRTNKhTNLhTN\
MhTNNxTNOxTNPxTNRBTNSBTNTRTNURTNVRTNWhTNXhTNYhTNZxTNaxTNbxTN\
dBTNeBTNfBTNgRTNhRTNiRTNjhTNkhTNlhTNmxTNnxTNoxTNqBTNrBTNsBTN\
tRTNuRTNvRTNwhTNxhTNyhTMzRTIzRTDzRS/zRS7zRS2zRSyzRStzRSpzRSl\
zRSgzRSczRSYzRSTzRSPzRSLzRSGzRSCzRR+zRR5zRR1zRRxzRRszRRozRRk\
zRRfzRRbzRRXzRRSzRROzRRKzRRFzRRBzRQ9zRQ4zRQ0zRQwzRQrzRQnzRQj\
zRQezRQazRQWzRQUzRcUzRsUzSAUzSQUzSgUzS0UzTEUzTUUzToUzT4UzUIU\
zUcUzUsUzU8UzVQUzVgUzVwUzWEUzWUUzWkUzW4UzXIUzXYUzXsUzX8UzYQU\
zYgUzYwUzZEUzZUUzZkUzZ4UzaIUzaYUzasUza8UzbMUzbgUzbwUzcAUzcUU\
zckUzc0Uyc0Uxc0UwM0UvM0UuM0Us80Ur80Uq80Ups0Uos0Uns0Umc0Ulc0U\
kc0UjM0UiM0UhM0Uf80Ue80Uds0Ucs0Ubs0Uac0UZc0UYc0UXM0UWM0UVM0U\
T80US80UR80UQs0UPs0UOs0UNc0UMc0ULc0UKM0UJM0UIM0UG80UF80WFM0a\
FM0eFM0jFM0nFM0rFM0wFM00FM04FM09FM1BFM1FFM1KFM1OFM1SFM1XFM1b\
FM1fFM1kFM1oFM1sFM1xFM11FM15FM1+FM2CFM2GFM2LFM2PFM2TFM2YFM2c\
FM2gFM2lFM2pFM2tFM2yFM22FM27FM2/FM3DFM3IFM3MFM3NFMrNFMbNFMLN\
FL3NFLnNFLXNFLDNFKzNFKjNFKPNFJ/NFJvNFJbNFJLNFI7NFInNFIXNFIHN\
FHzNFHjNFHTNFG/NFGvNFGfNFGLNFF7NFFrNFFXNFFHNFE3NFEjNFETNFD/N\
FDvNFDfNFDLNFC7NFCrNFCXNFCHNFB3NFBjUFRXUGRXUHhXUIhXUJxXUKxXU\
MBXUNBXUORXUPRXUQhXURhXUSxXUTxXUVBXUWBXUXRXUYRXUZhXUahXUbxXU\
cxXUeBXUfBXUgRXUhRXUihXUjhXUkxXUlxXUnBXUoBXUpRXUqRXUrhXUshXU\
txXUuxXUwBXUxBXUyRXUzRXU0RXT1BXP1BXK1BXG1BXB1BW91BW41BW01BWv\
1BWr1BWm1BWi1BWd1BWZ1BWU1BWQ1BWL1BWH1BWC1BV+1BV51BV11BVw1BVs\
1BVn1BVj1BVe1BVa1BVV1BVR1BVM1BVI1BVD1BU/1BU61BU21BUx1BUt1BUo\
1BUk1BUf1BUb1BUW1BUV1BgV1BwV1CEV1CUV1CoV1C4V1DMV1DcV1DwV1EAV\
1EUV1EkV1E4V1FIV1FcV1FsV1GAV1GQV1GkV1G0V1HIV1HYV1HsV1H8V1IQV\
1IgV1I0V1JEV1JYV1JoV1J8V1KMV1KgV1KwV1LEV1LUV1LoV1L4V1MMV1McV\
1MwV1NAV1NQV0NQVzNQVx9QVw9QVvtQVutQVtdQVsdQVrNQVqNQVo9QVn9QV\
mtQVltQVkdQVjdQViNQVhNQVf9QVe9QVdtQVctQVbdQVadQVZNQVYNQVW9QV\
V9QVUtQVTtQVSdQVRdQVQNQVPNQVN9QVM9QVLtQVKtQVJdQVIdQVHNQVGNQW\
FdQbFdQfFdQkFdQoFdQtFdQxFdQ2FdQ6FdQ/FdRDFdRIFdRMFdRRFdRVFdRa\
FdReFdRjFdRnFdRsFdRwFdR1FdR5FdR+FdSCFdSHFdSLFdSQFdSUFdSZFdSd\
FdSiFdSmFdSrFdSvFdS0FdS4FdS9FdTBFdTGFdTKFdTPFdTTFdTUFdHUFc3U\
FcnUFcTUFcDUFbvUFbfUFbLUFa7UFanUFaXUFaDUFZzUFZfUFZPUFY7UFYrU\
FYXUFYHUFXzUFXjUFXPUFW/UFWrUFWbUFWHUFV3UFVjUFVTUFU/UFUvUFUbU\
FULUFT3UFTnUFTTUFTDUFSvUFSfUFSLUFR7UFRncFhbcGhbcHxbcIxbcKBbc\
LRbcMRbcNhbcOxbcPxbcRBbcSRbcTRbcUhbcVxbcWxbcYBbcZBbcaRbcbhbc\
chbcdxbcfBbcgBbchRbcihbcjhbckxbcmBbcnBbcoRbcpRbcqhbcrxbcsxbc\
uBbcvRbcwRbcxhbcyxbczxbc1Bbc2Rba3BbV3BbR3BbM3BbI3BbD3Ba+3Ba6\
3Ba13Baw3Bas3Ban3Bai3Bae3BaZ3BaU3BaQ3BaL3BaH3BaC3BZ93BZ53BZ0\
3BZv3BZr3BZm3BZh3BZd3BZY3BZT3BZP3BZK3BZG3BZB3BY83BY43BYz3BYu\
3BYq3BYl3BYg3BYc3BYX3BYW3BkW3B0W3CIW3CcW3CsW3DAW3DQW3DkW3D4W\
3EIW3EcW3EwW3FAW3FUW3FoW3F4W3GMW3GgW3GwW3HEW3HUW3HoW3H8W3IMW\
3IgW3I0W3JEW3JYW3JsW3J8W3KQW3KkW3K0W3LIW3LcW3LsW3MAW3MQW3MkW\
3M4W3NIW3NcW3NwW19wW0twWztwWydwWxNwWwNwWu9wWt9wWstwWrdwWqdwW\
pNwWn9wWm9wWltwWkdwWjdwWiNwWg9wWf9wWetwWddwWcdwWbNwWaNwWY9wW\
XtwWWtwWVdwWUNwWTNwWR9wWQtwWPtwWOdwWNNwWMNwWK9wWJ9wWItwWHdwW\
GdwXFtwcFtwgFtwlFtwqFtwuFtwzFtw4Ftw8FtxBFtxGFtxKFtxPFtxTFtxY\
FtxdFtxhFtxmFtxrFtxvFtx0Ftx5Ftx9FtyCFtyHFtyLFtyQFtyUFtyZFtye\
FtyiFtynFtysFtywFty1Fty6Fty+FtzDFtzIFtzMFtzRFtzVFtzaFtzcFtnc\
FtTcFs/cFsvcFsbcFsHcFr3cFrjcFrPcFq/cFqrcFqXcFqHcFpzcFpjcFpPc\
Fo7cForcFoXcFoDcFnzcFnfcFnLcFm7cFmncFmTcFmDcFlvcFlfcFlLcFk3c\
FkncFkTcFj/cFjvcFjbcFjHcFi3cFijcFiPcFh/cFhrjFhbjGxbjIBbjJRbj\
KRbjLhbjMxbjOBbjPRbjQRbjRhbjSxbjUBbjVRbjWRbjXhbjYxbjaBbjbRbj\
cRbjdhbjexbjgBbjhRbjiRbjjhbjkxbjmBbjnBbjoRbjphbjqxbjsBbjtBbj\
uRbjvhbjwxbjyBbjzBbj0Rbj1hbj2xbj4Bbh4xbc4xbY4xbT4xbO4xbJ4xbE\
4xbA4xa74xa24xax4xas4xao4xaj4xae4xaZ4xaV4xaQ4xaL4xaG4xaB4xZ9\
4xZ44xZz4xZu4xZp4xZl4xZg4xZb4xZW4xZR4xZN4xZI4xZD4xY+4xY54xY1\
4xYw4xYr4xYm4xYh4xYd4xYY4xYW4xkW4x4W4yMW4ygW4y0W4zEW4zYW4zsW\
40AW40UW40kW404W41MW41gW410W42EW42YW42sW43AW43UW43kW434W44MW\
44gW440W45EW45YW45sW46AW46QW46kW464W47MW47gW47wW48EW48YW48sW\
49AW49QW49kW494W4+MW3uMW2eMW1OMW0OMWy+MWxuMWweMWvOMWuOMWs+MW\
ruMWqeMWpOMWoOMWm+MWluMWkeMWjeMWiOMWg+MWfuMWeeMWdeMWcOMWa+MW\
ZuMWYeMWXeMWWOMWU+MWTuMWSeMWReMWQOMWO+MWNuMWMeMWLeMWKOMWI+MW\
HuMWGeMYFuMdFuMhFuMmFuMrFuMwFuM1FuM5FuM+FuNDFuNIFuNNFuNRFuNW\
FuNbFuNgFuNlFuNpFuNuFuNzFuN4FuN9FuOBFuOGFuOLFuOQFuOVFuOZFuOe\
FuOjFuOoFuOsFuOxFuO2FuO7FuPAFuPEFuPJFuPOFuPTFuPYFuPcFuPhFuPj\
FuDjFtvjFtbjFtHjFszjFsjjFsPjFr7jFrnjFrTjFrDjFqvjFqbjFqHjFpzj\
FpjjFpPjFo7jFonjFoXjFoDjFnvjFnbjFnHjFm3jFmjjFmPjFl7jFlnjFlXj\
FlDjFkvjFkbjFkHjFj3jFjjjFjPjFi7jFinjFiXjFiDjFhvqFxfqHBfqIRfq\
JhfqKxfqMBfqNRfqOhfqPxfqQxfqSBfqTRfqUhfqVxfqXBfqYRfqZhfqaxfq\
cBfqdRfqehfqfxfqhBfqiRfqjhfqkxfqmBfqnRfqoRfqphfqqxfqsBfqtRfq\
uhfqvxfqxBfqyRfqzhfq0xfq2Bfq3Rfq4hfq5xfo6hfj6hfe6hfa6hfV6hfQ\
6hfL6hfG6hfB6he86he36hey6het6heo6hej6hee6heZ6heU6heP6heK6heF\
6heA6hd86hd36hdy6hdt6hdo6hdj6hde6hdZ6hdU6hdP6hdK6hdF6hdA6hc7\
6hc26hcx6hcs6hcn6hci6hce6hcZ6hcX6hoX6h8X6iQX6ikX6i4X6jMX6jgX\
6j0X6kIX6kcX6kwX6lEX6lYX6lsX6l8X6mQX6mkX6m4X6nMX6ngX6n0X6oIX\
6ocX6owX6pEX6pYX6psX6qAX6qUX6qoX6q8X6rQX6rkX6r0X6sIX6scX6swX\
6tEX6tYX6tsX6uAX6uUX6uoX5eoX4OoX2+oX1uoX0eoXzOoXx+oXwuoXveoX\
ueoXtOoXr+oXquoXpeoXoOoXm+oXluoXkeoXjOoXh+oXguoXfeoXeOoXc+oX\
buoXaeoXZOoXX+oXW+oXVuoXUeoXTOoXR+oXQuoXPeoXOOoXM+oXLuoXKeoX\
JOoXH+oXGuoZF+oeF+oiF+onF+osF+oxF+o2F+o7F+pAF+pFF+pKF+pPF+pU\
F+pZF+peF+pjF+poF+ptF+pyF+p3F+p8F+qAF+qFF+qKF+qPF+qUF+qZF+qe\
F+qjF+qoF+qtF+qyF+q3F+q8F+rBF+rGF+rLF+rQF+rVF+raF+reF+rjF+ro\
F+rqF+fqF+LqF93qF9jqF9PqF87qF8nqF8TqF7/qF7rqF7XqF7DqF6vqF6bq\
F6HqF53qF5jqF5PqF47qF4nqF4TqF3/qF3rqF3XqF3DqF2vqF2bqF2HqF1zq\
F1fqF1LqF03qF0jqF0PqFz/qFzrqFzXqFzDqFyvqFybqFyHqFxzxGBjxHRjx\
IhjxJxjxLBjxMRjxNhjxOxjxQBjxRhjxSxjxUBjxVRjxWhjxXxjxZBjxaRjx\
bhjxcxjxeRjxfhjxgxjxiBjxjRjxkhjxlxjxnBjxoRjxphjxqxjxsRjxthjx\
uxjxwBjxxRjxyhjxzxjx1Bjx2Rjx3hjx5Bjx6Rjx7hjv8Rjq8Rjl8Rjg8Rjb\
8RjW8RjR8RjM8RjH8RjC8Ri88Ri38Riy8Rit8Rio8Rij8Rie8RiZ8RiU8RiP\
8RiK8RiE8Rh/8Rh68Rh18Rhw8Rhr8Rhm8Rhh8Rhc8RhX8RhR8RhM8RhH8RhC\
8Rg98Rg48Rgz8Rgu8Rgp8Rgk8Rge8RgZ8RgY8RsY8SAY8SUY8SoY8S8Y8TUY\
8ToY8T8Y8UQY8UkY8U4Y8VMY8VgY8V0Y8WIY8WgY8W0Y8XIY8XcY8XwY8YEY\
8YYY8YsY8ZAY8ZUY8ZsY8aAY8aUY8aoY8a8Y8bQY8bkY8b4Y8cMY8cgY8c0Y\
8dMY8dgY8d0Y8eIY8ecY8ewY8fEY7PEY5/EY4vEY3fEY2PEY0/EYzfEYyPEY\
w/EYvvEYufEYtPEYr/EYqvEYpfEYoPEYm/EYlfEYkPEYi/EYhvEYgfEYfPEY\
d/EYcvEYbfEYaPEYYvEYXfEYWPEYU/EYTvEYSfEYRPEYP/EYOvEYNfEYL/EY\
KvEYJfEYIPEYG/EZGPEeGPEkGPEpGPEuGPEzGPE4GPE9GPFCGPFHGPFMGPFR\
GPFXGPFcGPFhGPFmGPFrGPFwGPF1GPF6GPF/GPGEGPGKGPGPGPGUGPGZGPGe\
GPGjGPGoGPGtGPGyGPG3GPG8GPHCGPHHGPHMGPHRGPHWGPHbGPHgGPHlGPHq\
GPHvGPHxGO7xGOnxGOTxGN7xGNnxGNTxGM/xGMrxGMXxGMDxGLvxGLbxGLHx\
GKvxGKbxGKHxGJzxGJfxGJLxGI3xGIjxGIPxGH7xGHnxGHPxGG7xGGnxGGTx\
GF/xGFrxGFXxGFDxGEvxGEbxGEDxGDvxGDbxGDHxGCzxGCfxGCLxGB34GBj4\
Hhj4Ixj4KBj4LRj4Mxj4OBj4PRj4Qhj4SBj4TRj4Uhj4Vxj4XRj4Yhj4Zxj4\
bBj4chj4dxj4fBj4gRj4hxj4jBj4kRj4lhj4nBj4oRj4phj4qxj4sRj4thj4\
uxj4wBj4xhj4yxj40Bj41Rj42xj44Bj45Rj46hj48Bj49Rj3+Bjx+Bjs+Bjn\
+Bji+Bjc+BjX+BjS+BjN+BjH+BjC+Bi9+Bi4+Biy+Bit+Bio+Bij+Bid+BiY\
+BiT+BiO+BiI+BiD+Bh++Bh5+Bhz+Bhu+Bhp+Bhk+Bhe+BhZ+BhU+BhP+BhJ\
+BhE+Bg/+Bg6+Bg0+Bgv+Bgq+Bgl+Bgf+Bga+BgY+BwY+CEY+CYY+CwY+DEY\
+DYY+DsY+EEY+EYY+EsY+FAY+FYY+FsY+GAY+GUY+GsY+HAY+HUY+HoY+IAY\
+IUY+IoY+I8Y+JUY+JoY+J8Y+KQY+KoY+K8Y+LQY+LkY+L8Y+MQY+MkY+M4Y\
+NQY+NkY+N4Y+OMY+OkY+O4Y+PMY+PgY8/gY7vgY6fgY4/gY3vgY2fgY1PgY\
zvgYyfgYxPgYv/gYufgYtPgYr/gYqvgYpPgYn/gYmvgYlfgYj/gYivgYhfgY\
gPgYevgYdfgYcPgYa/gYZfgYYPgYW/gYVvgYUPgYS/gYRvgYQfgYO/gYNvgY\
MfgYLPgYJvgYIfgYHPgaGPgfGPglGPgqGPgvGPg0GPg6GPg/GPhEGPhJGPhP\
GPhUGPhZGPheGPhkGPhpGPhuGPhzGPh5GPh+GPiDGPiIGPiOGPiTGPiYGPid\
GPijGPioGPitGPiyGPi4GPi9GPjCGPjHGPjNGPjSGPjXGPjcGPjiGPjnGPjs\
GPjxGPj3GPj4GPX4GPD4GOr4GOX4GOD4GNv4GNX4GND4GMv4GMb4GMD4GLv4\
GLb4GLH4GKv4GKb4GKH4GJz4GJb4GJH4GIz4GIf4GIH4GHz4GHf4GHL4GGz4\
GGf4GGL4GF34GFf4GFL4GE34GEj4GEL4GD34GDj4GDP4GC34GCj4GCP4GB74\
GBj4Hhj4Ixj4KBj4LRj4Mxj4OBj4PRj4Qhj4SBj4TRj4Uhj4Vxj4XRj4Yhj4\
Zxj4bBj4chj4dxj4fBj4gRj4hxj4jBj4kRj4lhj4nBj4oRj4phj4qxj4sRj4\
thj4uxj4wBj4xhj4yxj40Bj41Rj42xj44Bj45Rj46hj48Bj49Rj3+Bjx+Bjs\
+Bjn+Bji+Bjc+BjX+BjS+BjN+BjH+BjC+Bi9+Bi4+Biy+Bit+Bio+Bij+Bid\
+BiY+BiT+BiO+BiI+BiD+Bh++Bh5+Bhz+Bhu+Bhp+Bhk+Bhe+BhZ+BhU+BhP\
+BhJ+BhE+Bg/+Bg6+Bg0+Bgv+Bgq+Bgl+Bgf+Bga+BgY+BwY+CEY+CYY+CwY\
+DEY+DYY+DsY+EEY+EYY+EsY+FAY+FYY+FsY+GAY+GUY+GsY+HAY+HUY+HoY\
+IAY+IUY+IoY+I8Y+JUY+JoY+J8Y+KQY+KoY+K8Y+LQY+LkY+L8Y+MQY+MkY\
+M4Y+NQY+NkY+N4Y+OMY+OkY+O4Y+PMY+PgY8/gY7vgY6fgY4/gY3vgY2fgY\
1PgYzvgYyfgYxPgYv/gYufgYtPgYr/gYqvgYpPgYn/gYmvgYlfgYj/gYivgY\
hfgYgPgYevgYdfgYcPgYa/gYZfgYYPgYW/gYVvgYUPgYS/gYRvgYQfgYO/gY\
NvgYMfgYLPgYJvgYIfgYHPgaGPgfGPglGPgqGPgvGPg0GPg6GPg/GPhEGPhJ\
GPhPGPhUGPhZGPheGPhkGPhpGPhuGPhzGPh5GPh+GPiDGPiIGPiOGPiTGPiY\
GPidGPijGPioGPitGPiyGPi4GPi9GPjCGPjHGPjNGPjSGPjXGPjcGPjiGPjn\
GPjsGPjxGPj3GPj4GPX4GPD4GOr4GOX4GOD4GNv4GNX4GND4GMv4GMb4GMD4\
GLv4GLb4GLH4GKv4GKb4GKH4GJz4GJb4GJH4GIz4GIf4GIH4GHz4GHf4GHL4\
GGz4GGf4GGL4GF34GFf4GFL4GE34GEj4GEL4GD34GDj4GDP4GC34GCj4GCP4\
GB74ISH4JiH4KyH4MCH4NSH4OyH4QCH4RSH4SiH4TyH4VCH4WSH4XiH4YyH4\
aCH4bSH4ciH4dyH4fCH4gSH4hiH4iyH4kCH4lSH4miH4nyH4pCH4qSH4riH4\
syH4uSH4viH4wyH4yCH4zSH40iH41yH43CH44SH45iH46yH48CH49SH3+CHy\
+CHt+CHo+CHi+CHd+CHY+CHT+CHO+CHJ+CHE+CG/+CG6+CG1+CGw+CGr+CGm\
+CGh+CGc+CGX+CGS+CGN+CGI+CGD+CF++CF5+CF0+CFv+CFq+CFl+CFf+CFa\
+CFV+CFQ+CFL+CFG+CFB+CE8+CE3+CEy+CEt+CEo+CEj+CEh+CUh+Coh+C8h\
+DQh+Dkh+D4h+EMh+Egh+E0h+FIh+Fch+Fwh+GEh+GYh+Gsh+HAh+HUh+Hoh\
+H8h+IQh+Ikh+I8h+JQh+Jkh+J4h+KMh+Kgh+K0h+LIh+Lch+Lwh+MEh+MYh\
+Msh+NAh+NUh+Noh+N8h+OQh+Okh+O4h+PMh+Pgh8/gh7vgh6fgh5Pgh3/gh\
2vgh1fgh0Pghy/ghxvghwfghvPght/ghsvghrfghqPgho/ghnvghmfghlPgh\
j/ghifghhPghf/ghevghdfghcPgha/ghZvghYfghXPghV/ghUvghTfghSPgh\
Q/ghPvghOfghNPghL/ghKvghJfgjIfgoIfgtIfgyIfg3Ifg8IfhBIfhGIfhL\
IfhQIfhVIfhaIfhfIfhlIfhqIfhvIfh0Ifh5Ifh+IfiDIfiIIfiNIfiSIfiX\
IficIfihIfimIfirIfiwIfi1Ifi6Ifi/IfjEIfjJIfjOIfjTIfjYIfjdIfji\
IfjoIfjtIfjyIfj3Ifj4IfX4IfD4Iev4Ieb4IeH4Idz4Idf4IdL4Ic34Icj4\
IcP4Ib74Ibn4IbP4Ia74Ian4IaT4IZ/4IZr4IZX4IZD4IYv4IYb4IYH4IXz4\
IXf4IXL4IW34IWj4IWP4IV74IVn4IVT4IU/4IUr4IUX4IUD4ITv4ITX4ITD4\
ISv4ISb4Kir4Lyr4NCr4OSr4Pir4Qir4Ryr4TCr4USr4Vir4Wyr4Xyr4ZCr4\
aSr4bir4cyr4eCr4fCr4gSr4hir4iyr4kCr4lSr4mSr4nir4oyr4qCr4rSr4\
sir4tir4uyr4wCr4xSr4yir4zir40yr42Cr43Sr44ir45yr46yr48Cr49Sr3\
+Cry+Crt+Cro+Crj+Crf+Cra+CrV+CrQ+CrL+CrG+CrC+Cq9+Cq4+Cqz+Cqu\
+Cqp+Cql+Cqg+Cqb+CqW+CqR+CqM+CqI+CqD+Cp++Cp5+Cp0+Cpw+Cpr+Cpm\
+Cph+Cpc+CpX+CpT+CpO+CpJ+CpE+Co/+Co6+Co2+Cox+Cos+Coq+C4q+DIq\
+Dcq+Dwq+EEq+EYq+Eoq+E8q+FQq+Fkq+F4q+GMq+Gcq+Gwq+HEq+HYq+Hsq\
+IAq+IQq+Ikq+I4q+JMq+Jgq+J0q+KEq+KYq+Ksq+LAq+LUq+Loq+L4q+MMq\
+Mgq+M0q+NIq+Ncq+Nsq+OAq+OUq+Ooq+O8q+PQq+Pgq9Pgq7/gq6vgq5fgq\
4Pgq2/gq1/gq0vgqzfgqyPgqw/gqvvgquvgqtfgqsPgqq/gqpvgqofgqnfgq\
mPgqk/gqjvgqifgqhPgqgPgqe/gqdvgqcfgqbPgqZ/gqY/gqXvgqWfgqVPgq\
T/gqSvgqRvgqQfgqPPgqN/gqMvgqLvgsKvgxKvg2Kvg6Kvg/KvhEKvhJKvhO\
KvhTKvhXKvhcKvhhKvhmKvhrKvhwKvh0Kvh5Kvh+KviDKviIKviMKviRKviW\
KvibKvigKvilKvipKviuKvizKvi4Kvi9KvjCKvjGKvjLKvjQKvjVKvjaKvjf\
KvjjKvjoKvjtKvjyKvj3Kvj4KvX4KvD4Kuv4Kuf4KuL4Kt34Ktj4KtP4Ks74\
Ksr4KsX4KsD4Krv4Krb4KrL4Kq34Kqj4KqP4Kp74Kpn4KpX4KpD4Kov4Kob4\
KoH4Knz4Knj4KnP4Km74Kmn4KmT4Kl/4Klv4Klb4KlH4Kkz4Kkf4KkL4Kj74\
Kjn4KjT4Ki/4MzP4ODP4PDP4QTP4RjP4SjP4TzP4VDP4WDP4XTP4YTP4ZjP4\
azP4bzP4dDP4eTP4fTP4gjP4hjP4izP4kDP4lDP4mTP4nTP4ojP4pzP4qzP4\
sDP4tTP4uTP4vjP4wjP4xzP4zDP40DP41TP42jP43jP44zP45zP47DP48TP4\
9TP3+DPy+DPu+DPp+DPk+DPg+DPb+DPW+DPS+DPN+DPJ+DPE+DO/+DO7+DO2\
+DOy+DOt+DOo+DOk+DOf+DOa+DOW+DOR+DON+DOI+DOD+DN/+DN6+DN1+DNx\
+DNs+DNo+DNj+DNe+DNa+DNV+DNR+DNM+DNH+DND+DM++DM5+DM1+DMz+DYz\
+Dsz+EAz+EQz+Ekz+E0z+FIz+Fcz+Fsz+GAz+GUz+Gkz+G4z+HIz+Hcz+Hwz\
+IAz+IUz+Ikz+I4z+JMz+Jcz+Jwz+KEz+KUz+Koz+K4z+LMz+Lgz+Lwz+MEz\
+MYz+Moz+M8z+NMz+Ngz+N0z+OEz+OYz+Ooz+O8z+PQz+Pgz9Pgz7/gz6vgz\
5vgz4fgz3fgz2Pgz0/gzz/gzyvgzxvgzwfgzvPgzuPgzs/gzrvgzqvgzpfgz\
ofgznPgzl/gzk/gzjvgzifgzhfgzgPgzfPgzd/gzcvgzbvgzafgzZfgzYPgz\
W/gzV/gzUvgzTfgzSfgzRPgzQPgzO/gzNvg1M/g5M/g+M/hDM/hHM/hMM/hR\
M/hVM/haM/heM/hjM/hoM/hsM/hxM/h1M/h6M/h/M/iDM/iIM/iNM/iRM/iW\
M/iaM/ifM/ikM/ioM/itM/iyM/i2M/i7M/i/M/jEM/jJM/jNM/jSM/jWM/jb\
M/jgM/jkM/jpM/juM/jyM/j3M/j4M/X4M/H4M+z4M+f4M+P4M974M9r4M9X4\
M9D4M8z4M8f4M8L4M774M7n4M7X4M7D4M6v4M6f4M6L4M534M5n4M5T4M5D4\
M4v4M4b4M4L4M334M3n4M3T4M2/4M2v4M2b4M2H4M134M1j4M1T4M0/4M0r4\
M0b4M0H4Mzz4Mzj4PDz4QTz4RTz4STz4Tjz4Ujz4Vzz4Wzz4Xzz4ZDz4aDz4\
bTz4cTz4djz4ejz4fjz4gzz4hzz4jDz4kDz4lDz4mTz4nTz4ojz4pjz4qjz4\
rzz4szz4uDz4vDz4wDz4xTz4yTz4zjz40jz41zz42zz43zz45Dz46Dz47Tz4\
8Tz49Tz3+Dzy+Dzu+Dzq+Dzl+Dzh+Dzc+DzY+DzU+DzP+DzL+DzG+DzC+Dy+\
+Dy5+Dy1+Dyw+Dys+Dyo+Dyj+Dyf+Dya+DyW+DyR+DyN+DyJ+DyE+DyA+Dx7\
+Dx3+Dxz+Dxu+Dxq+Dxl+Dxh+Dxd+DxY+DxU+DxP+DxL+DxH+DxC+Dw++Dw8\
+D88+EQ8+Eg8+Ew8+FE8+FU8+Fo8+F48+GI8+Gc8+Gs8+HA8+HQ8+Hg8+H08\
+IE8+IY8+Io8+I88+JM8+Jc8+Jw8+KA8+KU8+Kk8+K08+LI8+LY8+Ls8+L88\
+MM8+Mg8+Mw8+NE8+NU8+Nk8+N48+OI8+Oc8+Os8+PA8+PQ8+Pg89Pg88Pg8\
6/g85/g84vg83vg82fg81fg80fg8zPg8yPg8w/g8v/g8u/g8tvg8svg8rfg8\
qfg8pfg8oPg8nPg8l/g8k/g8j/g8ivg8hvg8gfg8ffg8ePg8dPg8cPg8a/g8\
Z/g8Yvg8Xvg8Wvg8Vfg8Ufg8TPg8SPg8RPg8P/g+PPhCPPhHPPhLPPhPPPhU\
PPhYPPhdPPhhPPhlPPhqPPhuPPhzPPh3PPh7PPiAPPiEPPiJPPiNPPiRPPiW\
PPiaPPifPPijPPioPPisPPiwPPi1PPi5PPi+PPjCPPjGPPjLPPjPPPjUPPjY\
PPjcPPjhPPjlPPjqPPjuPPjyPPj3PPj4PPX4PPH4PO34POj4POT4PN/4PNv4\
PNf4PNL4PM74PMn4PMX4PMD4PLz4PLj4PLP4PK/4PKr4PKb4PKL4PJ34PJn4\
PJT4PJD4PIz4PIf4PIP4PH74PHr4PHb4PHH4PG34PGj4PGT4PF/4PFv4PFf4\
PFL4PE74PEn4PEX4PEH4RUX4SUX4TkX4UkX4VkX4WkX4XkX4Y0X4Z0X4a0X4\
b0X4c0X4eEX4fEX4gEX4hEX4iEX4jUX4kUX4lUX4mUX4nUX4okX4pkX4qkX4\
rkX4skX4t0X4u0X4v0X4w0X4x0X4zEX40EX41EX42EX43EX44UX45UX46UX4\
7UX48UX49kX3+EXz+EXv+EXq+EXm+EXi+EXe+EXa+EXV+EXR+EXN+EXJ+EXF\
+EXA+EW8+EW4+EW0+EWw+EWr+EWn+EWj+EWf+EWb+EWW+EWS+EWO+EWK+EWG\
+EWB+EV9+EV5+EV1+EVx+EVs+EVo+EVk+EVg+EVc+EVX+EVT+EVP+EVL+EVH\
+EVF+EhF+ExF+FBF+FVF+FlF+F1F+GFF+GVF+GpF+G5F+HJF+HZF+HpF+H9F\
+INF+IdF+ItF+I9F+JRF+JhF+JxF+KBF+KRF+KlF+K1F+LFF+LVF+LlF+L5F\
+MJF+MZF+MpF+M5F+NNF+NdF+NtF+N9F+ONF+OhF+OxF+PBF+PRF+PhF9PhF\
8PhF7PhF6PhF4/hF3/hF2/hF1/hF0/hFzvhFyvhFxvhFwvhFvvhFufhFtfhF\
sfhFrfhFqfhFpPhFoPhFnPhFmPhFlPhFj/hFi/hFh/hFg/hFf/hFevhFdvhF\
cvhFbvhFavhFZfhFYfhFXfhFWfhFVfhFUPhFTPhFSPhHRfhLRfhPRfhTRfhX\
RfhcRfhgRfhkRfhoRfhsRfhxRfh1Rfh5Rfh9RfiBRfiGRfiKRfiORfiSRfiW\
RfibRfifRfijRfinRfirRfiwRfi0Rfi4Rfi8RfjARfjFRfjJRfjNRfjRRfjV\
RfjaRfjeRfjiRfjmRfjqRfjvRfjzRfj3Rfj4Rfb4RfH4Re34Ren4ReX4ReH4\
Rdz4Rdj4RdT4RdD4Rcz4Rcf4RcP4Rb/4Rbv4Rbf4RbL4Ra74Rar4Rab4RaL4\
RZ34RZn4RZX4RZH4RY34RYj4RYT4RYD4RXz4RXj4RXP4RW/4RWv4RWf4RWP4\
RV74RVr4RVb4RVL4RU74RUn4Tk74Uk74Vk74Wk74Xk74Yk74Zk74ak74bk74\
ck74dk74ek74fk74gk74hk74ik74jk74kk74lk74mk74nk74ok74pk74qk74\
rk74sk74tk74uk74vk74wk74xk74yk74zk740k741k742k743k744k745k74\
6k747k748k749k73+E7z+E7v+E7r+E7n+E7j+E7f+E7b+E7X+E7T+E7P+E7L\
+E7H+E7D+E6/+E67+E63+E6z+E6v+E6r+E6n+E6j+E6f+E6b+E6X+E6T+E6P\
+E6L+E6H+E6D+E5/+E57+E53+E5z+E5v+E5r+E5n+E5j+E5f+E5b+E5X+E5T\
+E5P+E5O+FFO+FVO+FlO+F1O+GFO+GVO+GlO+G1O+HFO+HVO+HlO+H1O+IFO\
+IVO+IlO+I1O+JFO+JVO+JlO+J1O+KFO+KVO+KlO+K1O+LFO+LVO+LlO+LxO\
+MBO+MRO+MhO+MxO+NBO+NRO+NhO+NxO+OBO+ORO+OhO+OxO+PBO+PRO+PhO\
9PhO8PhO7PhO6PhO5PhO4PhO3PhO2PhO1PhO0PhOzPhOyPhOxPhOwPhOvPhO\
ufhOtfhOsfhOrfhOqfhOpfhOofhOnfhOmfhOlfhOkfhOjfhOifhOhfhOgfhO\
ffhOefhOdfhOcfhObfhOafhOZfhOYfhOXfhOWfhOVfhOUfhPTvhTTvhXTvhb\
TvhfTvhjTvhnTvhrTvhvTvhzTvh3Tvh7Tvh/TviDTviHTviLTviPTviTTviX\
TvibTvifTvijTvinTvirTvivTvizTvi3Tvi7Tvi/TvjDTvjHTvjLTvjPTvjT\
TvjXTvjbTvjfTvjjTvjnTvjrTvjvTvjzTvj3Tvj4Tvb4TvL4Tu74Tur4Tub4\
TuL4Tt74Ttr4Ttb4TtL4Ts74Tsr4Tsb4TsL4Tr74Trr4Trb4TrL4Tq74Tqr4\
Tqb4TqL4Tp74Tpr4Tpb4TpL4To74Tor4Tob4ToL4Tn74Tnr4Tnb4TnL4Tm74\
Tmr4Tmb4TmL4Tl74Tlr4Tlb4TlL4V1f4W1f4X1f4Ylf4Zlf4alf4blf4clf4\
dVf4eVf4fVf4gVf4hFf4iFf4jFf4kFf4lFf4l1f4m1f4n1f4o1f4plf4qlf4\
rlf4slf4tlf4uVf4vVf4wVf4xVf4yFf4zFf40Ff41Ff42Ff421f431f441f4\
51f46lf47lf48lf49lf3+Ffz+Ffw+Ffs+Ffo+Ffk+Ffg+Ffd+FfZ+FfV+FfR\
+FfO+FfK+FfG+FfC+Fe++Fe7+Fe3+Fez+Fev+Fer+Feo+Fek+Feg+Fec+FeZ\
+FeV+FeR+FeN+FeJ+FeG+FeC+Fd++Fd6+Fd3+Fdz+Fdv+Fdr+Fdn+Fdk+Fdg\
+Fdc+FdY+FdX+FpX+F1X+GFX+GVX+GlX+G1X+HBX+HRX+HhX+HxX+H9X+INX\
+IdX+ItX+I9X+JJX+JZX+JpX+J5X+KFX+KVX+KlX+K1X+LFX+LRX+LhX+LxX\
+MBX+MNX+MdX+MtX+M9X+NNX+NZX+NpX+N5X+OJX+OVX+OlX+O1X+PFX+PVX\
+PhX9fhX8fhX7fhX6fhX5fhX4vhX3vhX2vhX1vhX0/hXz/hXy/hXx/hXw/hX\
wPhXvPhXuPhXtPhXsfhXrfhXqfhXpfhXofhXnvhXmvhXlvhXkvhXj/hXi/hX\
h/hXg/hXf/hXfPhXePhXdPhXcPhXbfhXafhXZfhXYfhXXfhXWvhYV/hcV/hg\
V/hkV/hnV/hrV/hvV/hzV/h3V/h6V/h+V/iCV/iGV/iJV/iNV/iRV/iVV/iZ\
V/icV/igV/ikV/ioV/irV/ivV/izV/i3V/i7V/i+V/jCV/jGV/jKV/jOV/jR\
V/jVV/jZV/jdV/jgV/jkV/joV/jsV/jwV/jzV/j3V/j4V/b4V/L4V+74V+r4\
V+f4V+P4V9/4V9v4V9j4V9T4V9D4V8z4V8j4V8X4V8H4V734V7n4V7b4V7L4\
V674V6r4V6b4V6P4V5/4V5v4V5f4V5T4V5D4V4z4V4j4V4T4V4H4V334V3n4\
V3X4V3L4V274V2r4V2b4V2L4V1/4V1v4YGD4ZGD4Z2D4a2D4bmD4cmD4dWD4\
eWD4fWD4gGD4hGD4h2D4i2D4jmD4kmD4lmD4mWD4nWD4oGD4pGD4p2D4q2D4\
r2D4smD4tmD4uWD4vWD4wGD4xGD4yGD4y2D4z2D40mD41mD42WD43WD44WD4\
5GD46GD462D472D48mD49mD3+GD0+GDw+GDs+GDp+GDl+GDi+GDe+GDb+GDX\
+GDT+GDQ+GDM+GDJ+GDF+GDC+GC++GC6+GC3+GCz+GCw+GCs+GCp+GCl+GCh\
+GCe+GCa+GCX+GCT+GCQ+GCM+GCI+GCF+GCB+GB++GB6+GB3+GBz+GBw+GBs\
+GBo+GBl+GBh+GBg+GJg+GZg+Gpg+G1g+HFg+HRg+Hhg+Htg+H9g+INg+IZg\
+Ipg+I1g+JFg+JRg+Jhg+Jxg+J9g+KNg+KZg+Kpg+K1g+LFg+LVg+Lhg+Lxg\
+L9g+MNg+MZg+Mpg+M5g+NFg+NVg+Nhg+Nxg+N9g+ONg+OZg+Opg+O5g+PFg\
+PVg+Phg9fhg8fhg7vhg6vhg5vhg4/hg3/hg3Phg2Phg1fhg0fhgzvhgyvhg\
xvhgw/hgv/hgvPhguPhgtfhgsfhgrfhgqvhgpvhgo/hgn/hgnPhgmPhglPhg\
kfhgjfhgivhghvhgg/hgf/hge/hgePhgdPhgcfhgbfhgavhgZvhgYvhhYPhl\
YPhoYPhsYPhwYPhzYPh3YPh6YPh+YPiBYPiFYPiIYPiMYPiQYPiTYPiXYPia\
YPieYPihYPilYPipYPisYPiwYPizYPi3YPi6YPi+YPjCYPjFYPjJYPjMYPjQ\
YPjTYPjXYPjbYPjeYPjiYPjlYPjpYPjsYPjwYPj0YPj3YPj4YPb4YPL4YO/4\
YOv4YOj4YOT4YOH4YN34YNn4YNb4YNL4YM/4YMv4YMj4YMT4YMD4YL34YLn4\
YLb4YLL4YK/4YKv4YKf4YKT4YKD4YJ34YJn4YJb4YJL4YI74YIv4YIf4YIT4\
YID4YH34YHn4YHX4YHL4YG74YGv4YGf4YGT4aWn4bGn4cGn4c2n4dmn4emn4\
fWn4gWn4hGn4h2n4i2n4jmn4kWn4lWn4mGn4m2n4n2n4omn4pWn4qWn4rGn4\
sGn4s2n4tmn4umn4vWn4wGn4xGn4x2n4ymn4zmn40Wn41Wn42Gn422n432n4\
4mn45Wn46Wn47Gn472n482n49mn3+Gn0+Gnw+Gnt+Gnq+Gnm+Gnj+Gng+Gnc\
+GnZ+GnW+GnS+GnP+GnM+GnI+GnF+GnB+Gm++Gm7+Gm3+Gm0+Gmx+Gmt+Gmq\
+Gmn+Gmj+Gmg+Gmd+GmZ+GmW+GmS+GmP+GmM+GmI+GmF+GmC+Gl++Gl7+Gl4\
+Gl0+Glx+Glt+Glq+Glp+Gtp+G9p+HJp+HVp+Hlp+Hxp+H9p+INp+IZp+Ilp\
+I1p+JBp+JRp+Jdp+Jpp+J5p+KFp+KRp+Khp+Ktp+K5p+LJp+LVp+Llp+Lxp\
+L9p+MNp+MZp+Mlp+M1p+NBp+NNp+Ndp+Npp+N1p+OFp+ORp+Ohp+Otp+O5p\
+PJp+PVp+Php9fhp8vhp7vhp6/hp6Php5Php4fhp3fhp2vhp1/hp0/hp0Php\
zfhpyfhpxvhpw/hpv/hpvPhpufhptfhpsvhprvhpq/hpqPhppPhpofhpnvhp\
mvhpl/hplPhpkPhpjfhpifhphvhpg/hpf/hpfPhpefhpdfhpcvhpb/hpa/hq\
afhtafhxafh0afh4afh7afh+afiCafiFafiIafiMafiPafiSafiWafiZafid\
afigafijafinafiqafitafixafi0afi3afi7afi+afjBafjFafjIafjMafjP\
afjSafjWafjZafjcafjgafjjafjmafjqafjtafjwafj0afj3afj4afb4afP4\
ae/4aez4aen4aeX4aeL4ad/4adv4adj4adX4adH4ac74acr4acf4acT4acD4\
ab34abr4abb4abP4abD4aaz4aan4aaX4aaL4aZ/4aZv4aZj4aZX4aZH4aY74\
aYv4aYf4aYT4aYH4aX34aXr4aXb4aXP4aXD4aWz4cnL4dXL4eHL4e3L4f3L4\
gnL4hXL4iHL4i3L4jnL4kXL4lXL4mHL4m3L4nnL4oXL4pHL4qHL4q3L4rnL4\
sXL4tHL4t3L4unL4vnL4wXL4xHL4x3L4ynL4zXL40HL41HL413L42nL43XL4\
4HL443L45nL46nL47XL48HL483L49nL3+HL0+HLx+HLu+HLr+HLo+HLk+HLh\
+HLe+HLb+HLY+HLV+HLR+HLO+HLL+HLI+HLF+HLC+HK/+HK7+HK4+HK1+HKy\
+HKv+HKs+HKp+HKl+HKi+HKf+HKc+HKZ+HKW+HKT+HKP+HKM+HKJ+HKG+HKD\
+HKA+HJ8+HJ5+HJ2+HJz+HJy+HRy+Hdy+Hpy+H5y+IFy+IRy+Idy+Ipy+I1y\
+JBy+JRy+Jdy+Jpy+J1y+KBy+KNy+KZy+Kpy+K1y+LBy+LNy+LZy+Lly+Lxy\
+MBy+MNy+MZy+Mly+Mxy+M9y+NNy+NZy+Nly+Nxy+N9y+OJy+OVy+Oly+Oxy\
+O9y+PJy+PVy+Phy9fhy8vhy7/hy7Phy6fhy5fhy4vhy3/hy3Phy2fhy1vhy\
0/hyz/hyzPhyyfhyxvhyw/hywPhyvPhyufhytvhys/hysPhyrfhyqvhypvhy\
o/hyoPhynfhymvhyl/hylPhykPhyjfhyivhyh/hyhPhygfhyfvhyevhyd/hy\
dPhzcvh2cvh5cvh8cviAcviDcviGcviJcviMcviPcviTcviWcviZcviccvif\
cviicvilcvipcviscvivcviycvi1cvi4cvi7cvi/cvjCcvjFcvjIcvjLcvjO\
cvjRcvjVcvjYcvjbcvjecvjhcvjkcvjocvjrcvjucvjxcvj0cvj3cvj4cvb4\
cvP4cvD4cu34cur4cub4cuP4cuD4ct34ctr4ctf4ctT4ctD4cs34csr4csf4\
csT4csH4cr74crr4crf4crT4crH4cq74cqv4cqj4cqT4cqH4cp74cpv4cpj4\
cpX4cpH4co74cov4coj4coX4coL4cn/4cnv4cnj4cnX4e3v4fnv4gXv4hHv4\
h3v4inv4jXv4j3v4knv4lXv4mHv4m3v4nnv4oXv4pHv4p3v4qnv4rXv4sHv4\
s3v4tnv4uXv4vHv4v3v4wXv4xHv4x3v4ynv4zXv40Hv403v41nv42Xv43Hv4\
33v44nv45Xv46Hv463v47nv48Hv483v49nv3+Hv0+Hvx+Hvv+Hvs+Hvp+Hvm\
+Hvj+Hvg+Hvd+Hva+HvX+HvU+HvR+HvO+HvL+HvI+HvF+HvC+HvA+Hu9+Hu6\
+Hu3+Hu0+Hux+Huu+Hur+Huo+Hul+Hui+Huf+Huc+HuZ+HuW+HuT+HuQ+HuO\
+HuL+HuI+HuF+HuC+Ht/+Ht8+Ht7+H17+IB7+IN7+IZ7+Il7+Ix7+I97+JF7\
+JR7+Jd7+Jp7+J17+KB7+KN7+KZ7+Kl7+Kx7+K97+LJ7+LV7+Lh7+Lt7+L57\
+MB7+MN7+MZ7+Ml7+Mx7+M97+NJ7+NV7+Nh7+Nt7+N57+OF7+OR7+Od7+Op7\
+O17+PB7+PJ7+PV7+Ph79fh78vh78Ph77fh76vh75/h75Ph74fh73vh72/h7\
2Ph71fh70vh7z/h7zPh7yfh7xvh7w/h7wPh7vvh7u/h7uPh7tfh7svh7r/h7\
rPh7qfh7pvh7o/h7oPh7nfh7mvh7l/h7lPh7kfh7j/h7jPh7ifh7hvh7g/h7\
gPh7ffh8e/h/e/iCe/iFe/iIe/iLe/iOe/iQe/iTe/iWe/iZe/ice/ife/ii\
e/ile/ioe/ire/iue/ixe/i0e/i3e/i6e/i9e/jAe/jCe/jFe/jIe/jLe/jO\
e/jRe/jUe/jXe/jae/jde/jge/jje/jme/jpe/jse/jve/jxe/j0e/j3e/j4\
e/b4e/P4e/D4e+74e+v4e+j4e+X4e+L4e9/4e9z4e9n4e9b4e9P4e9D4e834\
e8r4e8f4e8T4e8H4e7/4e7z4e7n4e7b4e7P4e7D4e634e6r4e6f4e6T4e6H4\
e574e5v4e5j4e5X4e5L4e4/4e434e4r4e4f4e4T4e4H4e374hIT4h4T4iYT4\
jIT4j4T4koT4lIT4l4T4moT4nIT4n4T4ooT4pYT4p4T4qoT4rYT4sIT4soT4\
tYT4uIT4uoT4vYT4wIT4w4T4xYT4yIT4y4T4zoT40IT404T41oT42IT424T4\
3oT44YT444T45oT46YT47IT47oT48YT49IT494T3+IT1+ITy+ITv+ITt+ITq\
+ITn+ITk+ITi+ITf+ITc+ITZ+ITX+ITU+ITR+ITO+ITM+ITJ+ITG+ITE+ITB\
+IS++IS7+IS5+IS2+ISz+ISw+ISu+ISr+ISo+ISm+ISj+ISg+ISd+ISb+ISY\
+ISV+ISS+ISQ+ISN+ISK+ISI+ISF+ISE+IaE+IiE+IuE+I6E+JGE+JOE+JaE\
+JmE+JyE+J6E+KGE+KSE+KaE+KmE+KyE+K+E+LGE+LSE+LeE+LqE+LyE+L+E\
+MKE+MSE+MeE+MqE+M2E+M+E+NKE+NWE+NiE+NqE+N2E+OCE+OKE+OWE+OiE\
+OuE+O2E+PCE+POE+PaE+PiE9viE8/iE8PiE7fiE6/iE6PiE5fiE4viE4PiE\
3fiE2viE2PiE1fiE0viEz/iEzfiEyviEx/iExPiEwviEv/iEvPiEuviEt/iE\
tPiEsfiEr/iErPiEqfiEpviEpPiEofiEnviEnPiEmfiElviEk/iEkfiEjviE\
i/iEiPiEhviFhPiIhPiKhPiNhPiQhPiShPiVhPiYhPibhPidhPighPijhPim\
hPiohPirhPiuhPiwhPizhPi2hPi5hPi7hPi+hPjBhPjEhPjGhPjJhPjMhPjO\
hPjRhPjUhPjXhPjZhPjchPjfhPjihPjkhPjnhPjqhPjthPjvhPjyhPj1hPj3\
hPj4hPf4hPT4hPH4hO74hOz4hOn4hOb4hOP4hOH4hN74hNv4hNj4hNb4hNP4\
hND4hM74hMv4hMj4hMX4hMP4hMD4hL34hLr4hLj4hLX4hLL4hLD4hK34hKr4\
hKf4hKX4hKL4hJ/4hJz4hJr4hJf4hJT4hJL4hI/4hIz4hIn4hIf4jY34j434\
ko34lI34l434mY34nI34no34oY34pI34po34qY34q434ro34sI34s434tY34\
uI34uo34vY34v434wo34xI34x434yY34zI34zo340Y3404341o342I342434\
3Y344I344o345Y346I346o347Y3474348o349I349433+I31+I3y+I3w+I3t\
+I3r+I3o+I3m+I3j+I3h+I3e+I3c+I3Z+I3X+I3U+I3S+I3P+I3N+I3K+I3I\
+I3F+I3D+I3A+I2++I27+I25+I22+I2z+I2x+I2u+I2s+I2p+I2n+I2k+I2i\
+I2f+I2d+I2a+I2Y+I2V+I2T+I2Q+I2O+I2N+I+N+JGN+JSN+JaN+JmN+JuN\
+J6N+KCN+KON+KWN+KiN+KqN+K2N+K+N+LKN+LSN+LeN+LmN+LyN+L6N+MGN\
+MON+MaN+MiN+MuN+M6N+NCN+NON+NWN+NiN+NqN+N2N+N+N+OKN+OSN+OeN\
+OmN+OyN+O6N+PGN+PON+PaN+PiN9viN8/iN8fiN7viN7PiN6fiN5/iN5PiN\
4viN3/iN3fiN2viN2PiN1fiN0/iN0PiNzviNy/iNyPiNxviNw/iNwfiNvviN\
vPiNufiNt/iNtPiNsviNr/iNrfiNqviNqPiNpfiNo/iNoPiNnviNm/iNmfiN\
lviNlPiNkfiNj/iOjfiQjfiTjfiVjfiYjfiajfidjfifjfiijfikjfinjfip\
jfisjfiujfixjfizjfi2jfi5jfi7jfi+jfjAjfjDjfjFjfjIjfjKjfjNjfjP\
jfjSjfjUjfjXjfjZjfjcjfjejfjhjfjjjfjmjfjojfjrjfjtjfjwjfjyjfj1\
jfj3jfj4jff4jfT4jfL4je/4je34jer4jej4jeX4jeL4jeD4jd34jdv4jdj4\
jdb4jdP4jdH4jc74jcz4jcn4jcf4jcT4jcL4jb/4jb34jbr4jbj4jbX4jbP4\
jbD4ja74jav4jan4jab4jaT4jaH4jZ74jZz4jZn4jZf4jZT4jZL4jY/4lpb4\
mJb4mpb4nZb4n5b4oZb4pJb4ppb4qJb4q5b4rZb4r5b4spb4tJb4tpb4uJb4\
u5b4vZb4v5b4wpb4xJb4xpb4yZb4y5b4zZb40Jb40pb41Jb41pb42Zb425b4\
3Zb44Jb44pb45Jb455b46Zb465b47pb48Jb48pb49Jb495b4+Jb1+Jbz+Jbx\
+Jbu+Jbs+Jbq+Jbn+Jbl+Jbj+Jbg+Jbe+Jbc+Jba+JbX+JbV+JbT+JbQ+JbO\
+JbM+JbJ+JbH+JbF+JbC+JbA+Ja++Ja8+Ja5+Ja3+Ja1+Jay+Jaw+Jau+Jar\
+Jap+Jan+Jak+Jai+Jag+Jad+Jab+JaZ+JaX+JaW+JeW+JqW+JyW+J6W+KGW\
+KOW+KWW+KiW+KqW+KyW+K6W+LGW+LOW+LWW+LiW+LqW+LyW+L+W+MGW+MOW\
+MaW+MiW+MqW+MyW+M+W+NGW+NOW+NaW+NiW+NqW+N2W+N+W+OGW+OSW+OaW\
+OiW+OqW+O2W+O+W+PGW+PSW+PaW+PiW9viW9PiW8fiW7/iW7fiW6viW6PiW\
5viW5PiW4fiW3/iW3fiW2viW2PiW1viW0/iW0fiWz/iWzPiWyviWyPiWxviW\
w/iWwfiWv/iWvPiWuviWuPiWtfiWs/iWsfiWrviWrPiWqviWqPiWpfiWo/iW\
ofiWnviWnPiWmviWl/iXlviZlviblvidlviglviilviklvinlviplvirlviu\
lviwlviylvi1lvi3lvi5lvi8lvi+lvjAlvjClvjFlvjHlvjJlvjMlvjOlvjQ\
lvjTlvjVlvjXlvjalvjclvjelvjglvjjlvjllvjnlvjqlvjslvjulvjxlvjz\
lvj1lvj4lvj4lvf4lvT4lvL4lvD4lu74luv4lun4luf4luT4luL4luD4lt34\
ltv4ltn4ltb4ltT4ltL4ltD4ls34lsv4lsn4lsb4lsT4lsL4lr/4lr34lrv4\
lrj4lrb4lrT4lrL4lq/4lq34lqv4lqj4lqb4lqT4lqH4lp/4lp34lpr4lpj4\
n5/4oZ/4o5/4pZ/4p5/4qZ/4q5/4rZ/4sJ/4sp/4tJ/4tp/4uJ/4up/4vJ/4\
vp/4wJ/4wp/4xZ/4x5/4yZ/4y5/4zZ/4z5/40Z/405/41Z/415/42p/43J/4\
3p/44J/44p/45J/45p/46J/46p/47J/475/48Z/485/49Z/495/4+J/2+J/z\
+J/x+J/v+J/t+J/r+J/p+J/n+J/l+J/j+J/h+J/e+J/c+J/a+J/Y+J/W+J/U\
+J/S+J/Q+J/O+J/M+J/J+J/H+J/F+J/D+J/B+J+/+J+9+J+7+J+5+J+3+J+0\
+J+y+J+w+J+u+J+s+J+q+J+o+J+m+J+k+J+i+J+f+J+f+KCf+KKf+KSf+Kaf\
+Kmf+Kuf+K2f+K+f+LGf+LOf+LWf+Lef+Lmf+Luf+L6f+MCf+MKf+MSf+Maf\
+Mif+Mqf+Myf+M6f+NCf+NOf+NWf+Nef+Nmf+Nuf+N2f+N+f+OGf+OOf+OWf\
+Oif+Oqf+Oyf+O6f+PCf+PKf+PSf+Paf+Pif9vif9Pif8vif8Pif7vif7Pif\
6vif6Pif5fif4/if4fif3/if3fif2/if2fif1/if1fif0/if0PifzvifzPif\
yvifyPifxvifxPifwvifwPifvvifu/ifufift/iftfifs/ifsfifr/ifrfif\
q/ifqfifpvifpPifovifoPifn/iin/ikn/imn/ion/iqn/isn/iun/iwn/iy\
n/i0n/i3n/i5n/i7n/i9n/i/n/jBn/jDn/jFn/jHn/jJn/jMn/jOn/jQn/jS\
n/jUn/jWn/jYn/jan/jcn/jen/jhn/jjn/jln/jnn/jpn/jrn/jtn/jvn/jx\
n/jzn/j2n/j4n/j4n/f4n/X4n/P4n/H4n+/4n+z4n+r4n+j4n+b4n+T4n+L4\
n+D4n974n9z4n9r4n9f4n9X4n9P4n9H4n8/4n834n8v4n8n4n8f4n8X4n8L4\
n8D4n774n7z4n7r4n7j4n7b4n7T4n7L4n7D4n634n6v4n6n4n6f4n6X4n6P4\
n6H4qKj4qqj4q6j4raj4r6j4saj4s6j4taj4t6j4uaj4u6j4vKj4vqj4wKj4\
wqj4xKj4xqj4yKj4yqj4zKj4zqj4z6j40aj406j41aj416j42aj426j43aj4\
36j44Kj44qj45Kj45qj46Kj46qj47Kj47qj48Kj48aj486j49aj496j4+Kj2\
+Kj0+Kjy+Kjw+Kju+Kjs+Kjq+Kjp+Kjn+Kjl+Kjj+Kjh+Kjf+Kjd+Kjb+KjZ\
+KjY+KjW+KjU+KjS+KjQ+KjO+KjM+KjK+KjI+KjH+KjF+KjD+KjB+Ki/+Ki9\
+Ki7+Ki5+Ki3+Ki2+Ki0+Kiy+Kiw+Kiu+Kis+Kiq+Kio+Kio+Kmo+Kuo+K2o\
+K+o+LGo+LKo+LSo+Lao+Lio+Lqo+Lyo+L6o+MCo+MKo+MOo+MWo+Meo+Mmo\
+Muo+M2o+M+o+NGo+NOo+NSo+Nao+Nio+Nqo+Nyo+N6o+OCo+OKo+OSo+OWo\
+Oeo+Omo+Ouo+O2o+O+o+PGo+POo+PWo+Pao+Pio9vio9fio8/io8fio7/io\
7fio6/io6fio5/io5fio5Pio4vio4Pio3vio3Pio2vio2Pio1vio1Pio0/io\
0fioz/iozfioy/ioyfiox/ioxfiow/iowviowPiovviovPiouviouPiotvio\
tPiosviosfior/iorfioq/ioqfioqPiqqPisqPiuqPiwqPiyqPi0qPi2qPi3\
qPi5qPi7qPi9qPi/qPjBqPjDqPjFqPjHqPjIqPjKqPjMqPjOqPjQqPjSqPjU\
qPjWqPjYqPjZqPjbqPjdqPjfqPjhqPjjqPjlqPjnqPjpqPjqqPjsqPjuqPjw\
qPjyqPj0qPj2qPj4qPj4qPf4qPX4qPP4qPH4qPD4qO74qOz4qOr4qOj4qOb4\
qOT4qOL4qOD4qN/4qN34qNv4qNn4qNf4qNX4qNP4qNH4qM/4qM74qMz4qMr4\
qMj4qMb4qMT4qML4qMD4qL74qLz4qLv4qLn4qLf4qLX4qLP4qLH4qK/4qK34\
qKv4qKr4sbH4srH4tLH4trH4t7H4ubH4u7H4vLH4vrH4wLH4wbH4w7H4xbH4\
x7H4yLH4yrH4zLH4zbH4z7H40bH40rH41LH41rH417H42bH427H43LH43rH4\
4LH44bH447H45bH45rH46LH46rH467H47bH477H48LH48rH49LH49rH497H4\
+LH2+LH0+LHz+LHx+LHv+LHu+LHs+LHq+LHp+LHn+LHl+LHk+LHi+LHg+LHf\
+LHd+LHb+LHa+LHY+LHW+LHV+LHT+LHR+LHP+LHO+LHM+LHK+LHJ+LHH+LHF\
+LHE+LHC+LHA+LG/+LG9+LG7+LG6+LG4+LG2+LG1+LGz+LGx+LGx+LKx+LOx\
+LWx+Lex+Lmx+Lqx+Lyx+L6x+L+x+MGx+MOx+MSx+Max+Mix+Mmx+Mux+M2x\
+M6x+NCx+NKx+NOx+NWx+Nex+Nix+Nqx+Nyx+N2x+N+x+OGx+OKx+OSx+Oax\
+Oix+Omx+Oux+O2x+O6x+PCx+PKx+POx+PWx+Pex+Pix9/ix9fix8/ix8vix\
8Pix7vix7fix6/ix6fix6Pix5vix5Pix4vix4fix3/ix3fix3Pix2vix2Pix\
1/ix1fix0/ix0vix0Pixzvixzfixy/ixyfixyPixxvixxPixw/ixwfixv/ix\
vvixvPixuvixufixt/ixtfixs/ixsvixsfizsfi1sfi2sfi4sfi6sfi7sfi9\
sfi/sfjAsfjCsfjEsfjFsfjHsfjJsfjKsfjMsfjOsfjPsfjRsfjTsfjVsfjW\
sfjYsfjasfjbsfjdsfjfsfjgsfjisfjksfjlsfjnsfjpsfjqsfjssfjusfjv\
sfjxsfjzsfj0sfj2sfj4sfj4sff4sfb4sfT4sfL4sfD4se/4se34sev4ser4\
sej4seb4seX4seP4seH4seD4sd74sdz4sdv4sdn4sdf4sdb4sdT4sdL4sdH4\
sc/4sc34scz4scr4scj4scf4scX4scP4scH4scD4sb74sbz4sbv4sbn4sbf4\
sbb4sbT4sbL4urr4u7r4vbr4vrr4wLr4wbr4wrr4xLr4xbr4x7r4yLr4yrr4\
y7r4zbr4zrr40Lr40br407r41Lr41rr417r42Lr42rr427r43br43rr44Lr4\
4br447r45Lr45rr457r46br46rr47Lr47br477r48Lr48br487r49Lr49rr4\
97r4+Lr2+Lr1+Lrz+Lry+Lrw+Lrv+Lru+Lrs+Lrr+Lrp+Lro+Lrm+Lrl+Lrj\
+Lri+Lrg+Lrf+Lrd+Lrc+Lra+LrZ+LrY+LrW+LrV+LrT+LrS+LrQ+LrP+LrN\
+LrM+LrK+LrJ+LrH+LrG+LrE+LrD+LrB+LrA+Lq/+Lq9+Lq8+Lq6+Lq6+Lu6\
+Ly6+L66+L+6+MC6+MK6+MO6+MW6+Ma6+Mi6+Mm6+Mu6+My6+M66+M+6+NG6\
+NK6+NS6+NW6+Ne6+Ni6+Nm6+Nu6+Ny6+N66+N+6+OG6+OK6+OS6+OW6+Oe6\
+Oi6+Oq6+Ou6+O26+O66+PC6+PG6+PK6+PS6+PW6+Pe6+Pi69/i69fi69Pi6\
8vi68fi68Pi67vi67fi66/i66vi66Pi65/i65fi65Pi64vi64fi63/i63vi6\
3Pi62/i62fi62Pi61/i61fi61Pi60vi60fi6z/i6zvi6zPi6y/i6yfi6yPi6\
xvi6xfi6w/i6wvi6wPi6v/i6vvi6vPi6u/i6uvi8uvi9uvi/uvjAuvjBuvjD\
uvjEuvjGuvjHuvjJuvjKuvjMuvjNuvjPuvjQuvjSuvjTuvjVuvjWuvjYuvjZ\
uvjauvjcuvjduvjfuvjguvjiuvjjuvjluvjmuvjouvjpuvjruvjsuvjuuvjv\
uvjwuvjyuvjzuvj1uvj2uvj4uvj4uvf4uvb4uvT4uvP4uvH4uvD4uu/4uu34\
uuz4uur4uun4uuf4uub4uuT4uuP4uuH4uuD4ut74ut34utv4utr4utj4utf4\
utb4utT4utP4utH4utD4us74us34usv4usr4usj4usf4usX4usT4usL4usH4\
usD4ur74ur34urv4w8P4xMP4xcP4xsP4yMP4ycP4ysP4y8P4zcP4zsP4z8P4\
0MP40sP408P41MP41cP418P42MP42cP428P43MP43cP43sP44MP44cP44sP4\
48P45cP45sP458P46MP46sP468P47MP47cP478P48MP48cP48sP49MP49cP4\
9sP498P4+MP3+MP1+MP0+MPz+MPy+MPw+MPv+MPu+MPt+MPr+MPq+MPp+MPo\
+MPm+MPl+MPk+MPi+MPh+MPg+MPf+MPd+MPc+MPb+MPa+MPY+MPX+MPW+MPV\
+MPT+MPS+MPR+MPQ+MPO+MPN+MPM+MPL+MPJ+MPI+MPH+MPG+MPE+MPD+MPD\
+MPD+MXD+MbD+MfD+MjD+MrD+MvD+MzD+M7D+M/D+NDD+NHD+NPD+NTD+NXD\
+NbD+NjD+NnD+NrD+NvD+N3D+N7D+N/D+ODD+OLD+OPD+OTD+OXD+OfD+OjD\
+OnD+OrD+OzD+O3D+O7D+PDD+PHD+PLD+PPD+PXD+PbD+PfD+PjD9/jD9vjD\
9fjD8/jD8vjD8fjD8PjD7vjD7fjD7PjD6vjD6fjD6PjD5/jD5fjD5PjD4/jD\
4vjD4PjD3/jD3vjD3fjD2/jD2vjD2fjD2PjD1vjD1fjD1PjD0/jD0fjD0PjD\
z/jDzvjDzPjDy/jDyvjDyPjDx/jDxvjDxfjDw/jDw/jEw/jGw/jHw/jIw/jJ\
w/jLw/jMw/jNw/jOw/jQw/jRw/jSw/jTw/jVw/jWw/jXw/jYw/jaw/jbw/jc\
w/jdw/jfw/jgw/jhw/jiw/jkw/jlw/jmw/jow/jpw/jqw/jrw/jtw/juw/jv\
w/jww/jyw/jzw/j0w/j1w/j3w/j4w/j4w/f4w/b4w/X4w/T4w/L4w/H4w/D4\
w+/4w+34w+z4w+v4w+r4w+j4w+f4w+b4w+X4w+P4w+L4w+H4w+D4w974w934\
w9z4w9v4w9n4w9j4w9f4w9X4w9T4w9P4w9L4w9D4w8/4w874w834w8v4w8r4\
w8n4w8j4w8b4w8X4w8T4zMz4zcz4zsz4z8z40Mz40cz40sz408z41Mz41cz4\
1sz418z42Mz42cz42sz428z43Mz43cz43sz438z44cz44sz448z45Mz45cz4\
5sz458z46Mz46cz46sz468z47Mz47cz47sz478z48Mz48cz48sz488z49Mz4\
9sz498z4+Mz4+Mz3+Mz2+Mz1+Mz0+Mzz+Mzy+Mzx+Mzw+Mzv+Mzt+Mzs+Mzr\
+Mzq+Mzp+Mzo+Mzn+Mzm+Mzl+Mzk+Mzj+Mzi+Mzh+Mzg+Mzf+Mze+Mzd+Mzc\
+Mzb+Mza+MzY+MzX+MzW+MzV+MzU+MzT+MzS+MzR+MzQ+MzP+MzO+MzN+MzM\
+MzM+MzM+M3M+M7M+M/M+NDM+NHM+NPM+NTM+NXM+NbM+NfM+NjM+NnM+NrM\
+NvM+NzM+N3M+N7M+N/M+ODM+OHM+OLM+OPM+OTM+OXM+ObM+OjM+OnM+OrM\
+OvM+OzM+O3M+O7M+O/M+PDM+PHM+PLM+PPM+PTM+PXM+PbM+PfM+PjM9/jM\
9vjM9fjM9PjM8/jM8vjM8fjM8PjM7/jM7vjM7fjM7PjM6/jM6vjM6fjM6PjM\
5vjM5fjM5PjM4/jM4vjM4fjM4PjM3/jM3vjM3fjM3PjM2/jM2vjM2fjM2PjM\
1/jM1vjM1fjM1PjM0/jM0fjM0PjMz/jMzvjMzfjMzPjMzPjNzPjOzPjPzPjQ\
zPjRzPjSzPjTzPjUzPjVzPjWzPjXzPjYzPjazPjbzPjczPjdzPjezPjfzPjg\
zPjhzPjizPjjzPjkzPjlzPjmzPjnzPjozPjpzPjqzPjrzPjszPjtzPjvzPjw\
zPjxzPjyzPjzzPj0zPj1zPj2zPj3zPj4zPj4zPj4zPf4zPb4zPT4zPP4zPL4\
zPH4zPD4zO/4zO74zO34zOz4zOv4zOr4zOn4zOj4zOf4zOb4zOX4zOT4zOP4\
zOL4zOH4zN/4zN74zN34zNz4zNv4zNr4zNn4zNj4zNf4zNb4zNX4zNT4zNP4\
zNL4zNH4zND4zM/4zM74zM341dX41dX41tX419X42NX42dX42tX42tX429X4\
3NX43dX43tX439X439X44NX44dX44tX449X45NX45NX45dX45tX459X46NX4\
6dX46dX46tX469X47NX47dX47tX479X479X48NX48dX48tX489X49NX49NX4\
9dX49tX499X4+NX4+NX3+NX2+NX2+NX1+NX0+NXz+NXy+NXx+NXw+NXw+NXv\
+NXu+NXt+NXs+NXr+NXr+NXq+NXp+NXo+NXn+NXm+NXm+NXl+NXk+NXj+NXi\
+NXh+NXh+NXg+NXf+NXe+NXd+NXc+NXb+NXb+NXa+NXZ+NXY+NXX+NXW+NXW\
+NXV+NXV+NXV+NbV+NfV+NjV+NjV+NnV+NrV+NvV+NzV+N3V+N3V+N7V+N/V\
+ODV+OHV+OLV+OLV+OPV+OTV+OXV+ObV+OfV+OjV+OjV+OnV+OrV+OvV+OzV\
+O3V+O3V+O7V+O/V+PDV+PHV+PLV+PLV+PPV+PTV+PXV+PbV+PfV+PfV+PjV\
9/jV9/jV9vjV9fjV9PjV8/jV8vjV8vjV8fjV8PjV7/jV7vjV7fjV7fjV7PjV\
6/jV6vjV6fjV6PjV6PjV5/jV5vjV5fjV5PjV4/jV4vjV4vjV4fjV4PjV3/jV\
3vjV3fjV3fjV3PjV2/jV2vjV2fjV2PjV2PjV1/jV1vjV1fjV1fjW1fjW1fjX\
1fjY1fjZ1fja1fjb1fjb1fjc1fjd1fje1fjf1fjg1fjh1fjh1fji1fjj1fjk\
1fjl1fjm1fjm1fjn1fjo1fjp1fjq1fjr1fjr1fjs1fjt1fju1fjv1fjw1fjw\
1fjx1fjy1fjz1fj01fj11fj21fj21fj31fj41fj41fj41ff41fb41fX41fT4\
1fT41fP41fL41fH41fD41e/41e/41e741e341ez41ev41er41en41en41ej4\
1ef41eb41eX41eT41eT41eP41eL41eH41eD41d/41d/41d741d341dz41dv4\
1dr41dr41dn41dj41df41db41dX43d343t34393439344N344d344d344t34\
4t3449345N345N345d345t345t3459346N346N346d346d346t3469346934\
7N347d347d347t347t3479348N348N348d348t348t3489349N349N349d34\
9d349t3499349934+N34+N33+N33+N32+N32+N31+N30+N30+N3z+N3y+N3y\
+N3x+N3x+N3w+N3v+N3v+N3u+N3t+N3t+N3s+N3s+N3r+N3q+N3q+N3p+N3o\
+N3o+N3n+N3m+N3m+N3l+N3l+N3k+N3j+N3j+N3i+N3h+N3h+N3g+N3g+N3f\
+N3e+N3e+N3d+N7d+N/d+N/d+ODd+ODd+OHd+OLd+OLd+OPd+OTd+OTd+OXd\
+OXd+Obd+Ofd+Ofd+Ojd+Ond+Ond+Ord+Ord+Ovd+Ozd+Ozd+O3d+O7d+O7d\
+O/d+PDd+PDd+PHd+PHd+PLd+PPd+PPd+PTd+PXd+PXd+Pbd+Pbd+Pfd+Pjd\
+Pjd+Pjd9/jd9vjd9vjd9fjd9fjd9Pjd8/jd8/jd8vjd8fjd8fjd8Pjd8Pjd\
7/jd7vjd7vjd7fjd7Pjd7Pjd6/jd6vjd6vjd6fjd6fjd6Pjd5/jd5/jd5vjd\
5fjd5fjd5Pjd5Pjd4/jd4vjd4vjd4fjd4Pjd4Pjd3/jd3/jd3vje3fje3fjf\
3fjg3fjg3fjh3fjh3fji3fjj3fjj3fjk3fjl3fjl3fjm3fjm3fjn3fjo3fjo\
3fjp3fjq3fjq3fjr3fjs3fjs3fjt3fjt3fju3fjv3fjv3fjw3fjx3fjx3fjy\
3fjy3fjz3fj03fj03fj13fj23fj23fj33fj33fj43fj43fj43ff43ff43fb4\
3fX43fX43fT43fT43fP43fL43fL43fH43fD43fD43e/43e743e743e343e34\
3ez43ev43ev43er43en43en43ej43ej43ef43eb43eb43eX43eT43eT43eP4\
3eL43eL43eH43eH43eD43d/43d/43d745ub45+b45+b46Ob46Ob46eb46eb4\
6eb46ub46ub46+b46+b46+b47Ob47Ob47eb47eb47ub47ub47ub47+b47+b4\
8Ob48Ob48Ob48eb48eb48ub48ub48+b48+b48+b49Ob49Ob49eb49eb49ub4\
9ub49ub49+b49+b4+Ob4+Ob4+Ob4+Ob3+Ob3+Ob3+Ob2+Ob2+Ob1+Ob1+Ob0\
+Ob0+Ob0+Obz+Obz+Oby+Oby+Obx+Obx+Obx+Obw+Obw+Obv+Obv+Obv+Obu\
+Obu+Obt+Obt+Obs+Obs+Obs+Obr+Obr+Obq+Obq+Obp+Obp+Obp+Obo+Obo\
+Obn+Obn+Obn+Obm+Ofm+Ofm+Ojm+Ojm+Ojm+Onm+Onm+Orm+Orm+Orm+Ovm\
+Ovm+Ozm+Ozm+O3m+O3m+O3m+O7m+O7m+O/m+O/m+PDm+PDm+PDm+PHm+PHm\
+PLm+PLm+PLm+PPm+PPm+PTm+PTm+PXm+PXm+PXm+Pbm+Pbm+Pfm+Pfm+Pfm\
+Pjm+Pjm+Pjm9/jm9/jm9/jm9vjm9vjm9fjm9fjm9fjm9Pjm9Pjm8/jm8/jm\
8vjm8vjm8vjm8fjm8fjm8Pjm8Pjm8Pjm7/jm7/jm7vjm7vjm7fjm7fjm7fjm\
7Pjm7Pjm6/jm6/jm6vjm6vjm6vjm6fjm6fjm6Pjm6Pjm6Pjm5/jm5/jn5vjn\
5vjn5vjo5vjo5vjp5vjp5vjp5vjq5vjq5vjr5vjr5vjs5vjs5vjs5vjt5vjt\
5vju5vju5vjv5vjv5vjv5vjw5vjw5vjx5vjx5vjx5vjy5vjy5vjz5vjz5vj0\
5vj05vj05vj15vj15vj25vj25vj35vj35vj35vj45vj45vj45vj45vj45vf4\
5vf45vb45vb45vb45vX45vX45vT45vT45vP45vP45vP45vL45vL45vH45vH4\
5vD45vD45vD45u/45u/45u745u745u745u345u345uz45uz45uv45uv45uv4\
5ur45ur45un45un45un45uj45uj45uf45uf47+/48O/48O/48O/48O/48O/4\
8e/48e/48e/48e/48e/48u/48u/48u/48u/48+/48+/48+/48+/48+/49O/4\
9O/49O/49O/49O/49e/49e/49e/49e/49e/49u/49u/49u/49u/49+/49+/4\
9+/49+/49+/4+O/4+O/4+O/4+O/4+O/4+O/4+O/4+O/3+O/3+O/3+O/3+O/3\
+O/2+O/2+O/2+O/2+O/2+O/1+O/1+O/1+O/1+O/0+O/0+O/0+O/0+O/0+O/z\
+O/z+O/z+O/z+O/z+O/y+O/y+O/y+O/y+O/y+O/x+O/x+O/x+O/x+O/w+O/w\
+O/w+O/w+O/w+O/v+O/v+PDv+PDv+PDv+PDv+PDv+PHv+PHv+PHv+PHv+PHv\
+PLv+PLv+PLv+PLv+PLv+PPv+PPv+PPv+PPv+PTv+PTv+PTv+PTv+PTv+PXv\
+PXv+PXv+PXv+PXv+Pbv+Pbv+Pbv+Pbv+Pbv+Pfv+Pfv+Pfv+Pfv+Pfv+Pjv\
+Pjv+Pjv+Pjv+Pjv+Pjv+Pjv9/jv9/jv9/jv9/jv9/jv9vjv9vjv9vjv9vjv\
9vjv9fjv9fjv9fjv9fjv9fjv9Pjv9Pjv9Pjv9Pjv9Pjv8/jv8/jv8/jv8/jv\
8vjv8vjv8vjv8vjv8vjv8fjv8fjv8fjv8fjv8fjv8Pjv8Pjv8Pjv8Pjv8Pjv\
7/jw7/jw7/jw7/jw7/jw7/jx7/jx7/jx7/jx7/jy7/jy7/jy7/jy7/jy7/jz\
7/jz7/jz7/jz7/jz7/j07/j07/j07/j07/j07/j17/j17/j17/j17/j27/j2\
7/j27/j27/j27/j37/j37/j37/j37/j37/j47/j47/j47/j47/j47/j47/j4\
7/j47/j47/f47/f47/f47/f47/f47/b47/b47/b47/b47/X47/X47/X47/X4\
7/X47/T47/T47/T47/T47/T47/P47/P47/P47/P47/P47/L47/L47/L47/L4\
7/H47/H47/H47/H47/H47/D47/D47/D47/D47/A=";


    COLORGRID_SYS_VGUI3("image create photo img%lx -data [base64::decode {%s}]\n",x,fdata);
    
    COLORGRID_SYS_VGUI6(".x%lx.c create image %d %d -image img%lx -tags %lxS\n", canvas,text_xpix(&x->x_obj, glist), text_ypix(&x->x_obj, glist),x,x);
    COLORGRID_SYS_VGUI5(".x%lx.c coords %lxS %d %d \n",
	     canvas, x,
	     x->x_obj.te_xpix + 128, x->x_obj.te_ypix + 25);
				  
    COLORGRID_SYS_VGUI7(".x%lx.c create rectangle %d %d %d %d -tags %lxo0\n",
	     canvas, x->x_obj.te_xpix, x->x_obj.te_ypix + x->x_height+1,
	     x->x_obj.te_xpix+7, x->x_obj.te_ypix + x->x_height+2,
	     x);
    COLORGRID_SYS_VGUI7(".x%lx.c create rectangle %d %d %d %d -tags %lxo1\n",
	     canvas, x->x_obj.te_xpix+x->x_width-7, x->x_obj.te_ypix + x->x_height+1,
	     x->x_obj.te_xpix+x->x_width, x->x_obj.te_ypix + x->x_height+2,
	     x);
    COLORGRID_SYS_VGUI7(".x%lx.c create rectangle %d %d %d %d -tags %lxo2\n",
	     canvas, x->x_obj.te_xpix+x->x_width-131, x->x_obj.te_ypix + x->x_height+1,
	     x->x_obj.te_xpix+x->x_width-126, x->x_obj.te_ypix + x->x_height+2,
	     x);

    if ( x->x_pdp_colorgrid ) 
    {
       int xlpos = x->x_obj.te_xpix+x->x_width/x->x_xlines;
       int ylpos = x->x_obj.te_ypix+x->x_height/x->x_ylines;
       int xcount = 1;
       int ycount = 1;
       while ( xlpos < x->x_obj.te_xpix+x->x_width )
       {
         COLORGRID_SYS_VGUI9(".x%lx.c create line %d %d %d %d -fill #FFFFFF -tags %lxLINE%d%d\n",
	     canvas, xlpos, x->x_obj.te_ypix,
	     xlpos, x->x_obj.te_ypix+x->x_height,
	     x, xcount, 0 );
         xlpos+=x->x_width/x->x_xlines;
         xcount++;
       }
       while ( ylpos < x->x_obj.te_ypix+x->x_height )
       {
         COLORGRID_SYS_VGUI9(".x%lx.c create line %d %d %d %d -fill #FFFFFF -tags %lxLINE%d%d\n",
	     canvas, x->x_obj.te_xpix, ylpos,
	     x->x_obj.te_xpix+x->x_width, ylpos,
	     x, 0, ycount);
         ylpos+=x->x_height/x->x_ylines;
         ycount++;
       }
    }
    canvas_fixlinesfor( canvas, (t_text*)x );
}

static void pdp_colorgrid_draw_move(t_pdp_colorgrid *x, t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    COLORGRID_SYS_VGUI7(".x%lx.c coords %lxCOLORGRID %d %d %d %d\n",
	     canvas, x,
	     x->x_obj.te_xpix, x->x_obj.te_ypix,
	     x->x_obj.te_xpix+x->x_width, x->x_obj.te_ypix+x->x_height);
    COLORGRID_SYS_VGUI5(".x%lx.c coords %lxS %d %d \n",
	     canvas, x,
	     x->x_obj.te_xpix + 128, x->x_obj.te_ypix + 25);
    COLORGRID_SYS_VGUI7(".x%lx.c coords %lxo0 %d %d %d %d\n",
	     canvas, x,
	     x->x_obj.te_xpix, x->x_obj.te_ypix + x->x_height+1,
	     x->x_obj.te_xpix+7, x->x_obj.te_ypix + x->x_height+2 );
    COLORGRID_SYS_VGUI7(".x%lx.c coords %lxo1 %d %d %d %d\n",
	     canvas, x,
	     x->x_obj.te_xpix+x->x_width-7, x->x_obj.te_ypix + x->x_height+1,
	     x->x_obj.te_xpix+x->x_width, x->x_obj.te_ypix + x->x_height+2 );
    COLORGRID_SYS_VGUI7(".x%lx.c coords %lxo2 %d %d %d %d\n",
	     canvas, x,
	     x->x_obj.te_xpix+x->x_width-131, x->x_obj.te_ypix + x->x_height+1,
	     x->x_obj.te_xpix+x->x_width-126, x->x_obj.te_ypix + x->x_height+2 );
    if ( x->x_point ) 
    {
       pdp_colorgrid_draw_update(x, glist);
    }
    if ( x->x_pdp_colorgrid ) 
    {
       int xlpos = x->x_obj.te_xpix+x->x_width/x->x_xlines;
       int ylpos = x->x_obj.te_ypix+x->x_height/x->x_ylines;
       int xcount = 1;
       int ycount = 1;
       while ( xlpos < x->x_obj.te_xpix+x->x_width )
       {
         COLORGRID_SYS_VGUI9(".x%lx.c coords %lxLINE%d%d %d %d %d %d\n",
	     canvas, x, xcount, 0, xlpos, x->x_obj.te_ypix,
	     xlpos, x->x_obj.te_ypix + x->x_height);
         xlpos+=x->x_width/x->x_xlines;
         xcount++;
       }
       while ( ylpos < x->x_obj.te_ypix+x->x_height )
       {
         COLORGRID_SYS_VGUI9(".x%lx.c coords %lxLINE%d%d %d %d %d %d\n",
	     canvas, x, 0, ycount, x->x_obj.te_xpix, ylpos,
	     x->x_obj.te_xpix + x->x_width, ylpos);
         ylpos+=x->x_height/x->x_ylines;
         ycount++;
       }
    }
    canvas_fixlinesfor( canvas, (t_text*)x );
}

static void pdp_colorgrid_draw_erase(t_pdp_colorgrid* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    int i;

    COLORGRID_SYS_VGUI3(".x%lx.c delete %lxCOLORGRID\n", canvas, x);
    COLORGRID_SYS_VGUI3(".x%lx.c delete %lxS\n", canvas, x);
    COLORGRID_SYS_VGUI3(".x%lx.c delete %lxo0\n", canvas, x);
    COLORGRID_SYS_VGUI3(".x%lx.c delete %lxo1\n", canvas, x);
    COLORGRID_SYS_VGUI3(".x%lx.c delete %lxo2\n", canvas, x);
    if (x->x_pdp_colorgrid)  
    {
       for (i=1; i<x->x_xlines; i++ )
       {
           COLORGRID_SYS_VGUI4(".x%lx.c delete %lxLINE%d0\n", canvas, x, i);
       }
       for (i=1; i<x->x_ylines; i++ )
       {
           COLORGRID_SYS_VGUI4(".x%lx.c delete %lxLINE0%d\n", canvas, x, i);
       }
    }
    if (x->x_point)  
    {
          COLORGRID_SYS_VGUI3(".x%lx.c delete %lxPOINT\n", canvas, x);
          x->x_point = 0;
    }
    rtext_free(glist_findrtext(glist, (t_text *)x));
}

static void pdp_colorgrid_draw_select(t_pdp_colorgrid* x,t_glist* glist)
{
    t_canvas *canvas=glist_getcanvas(glist);

    if(x->x_selected)
    {
	pd_bind(&x->x_obj.ob_pd, x->x_name);
        /* sets the item in blue */
	COLORGRID_SYS_VGUI3(".x%lx.c itemconfigure %lxCOLORGRID -outline #0000FF\n", canvas, x);
    }
    else
    {
	pd_unbind(&x->x_obj.ob_pd, x->x_name);
	COLORGRID_SYS_VGUI3(".x%lx.c itemconfigure %lxCOLORGRID -outline #000000\n", canvas, x);
    }
}

static void pdp_colorgrid_hsv2rgb(t_float hue, t_float saturation, t_float value, t_float *red, t_float *green, t_float *blue)
{
   t_float i=0, f=0, p=0, q=0, t=0;
	
   if (saturation == 0) {
       *red = value;
       *green = value;
       *blue = value;
   } else {
   if (hue == 6) hue = 0;
   i = (int)hue ;  /* the integer part of hue */
   f = hue - i;
   p = value * (1 - saturation);
   q = value * (1 - (saturation * f));
   t = value * (1 - (saturation * (1 - f)));
   switch ((int)i) {
           case 0:
            *red = value;
            *green = t;
            *blue = p;
            break;
           case 1:
            *red = q;
            *green = value;
            *blue = p;
            break;
           case 2:
            *red = p;
            *green = value;
            *blue = t;
            break;
           case 3:
            *red = p;
            *green = q;
            *blue = value;
            break;
           case 4:
            *red = t;
            *green = p;
            *blue = value;
	    break;
	   case 5:
	    *red = value;
            *green = p;
            *blue = q;
            break;
   } 
   }
}

static void pdp_colorgrid_output_current(t_pdp_colorgrid* x)
{
  t_float ox=0, oy=0, hue, saturation, value, red=0, green=0, blue=0;

/* These values need to be the same as those that produced the spectrum image:*/

  t_float box_x = 256;
  t_float box_y = 25;

  t_float min_value = 0.3;
  t_float max_value = 1.0;
  t_float value_inc = (max_value - min_value) / box_y;

  t_float min_hue = 0;
  t_float max_hue = 6;
  t_float hue_inc = (max_hue - min_hue) / box_x;

  t_float max_saturation = 0.9;
  t_float min_saturation = 0.0;
  t_float saturation_inc = (max_saturation - min_saturation) / box_y;

  t_float xvalue, yvalue, rvalue, gvalue, bvalue;
  t_float xmodstep, ymodstep;

  xvalue = x->x_min + (x->x_current - x->x_obj.te_xpix) * (x->x_max-x->x_min) / x->x_width ;
  if (xvalue < x->x_min ) xvalue = x->x_min;
  if (xvalue > x->x_max ) xvalue = x->x_max;
  xmodstep = ((float)((int)(xvalue*10000) % (int)(x->x_xstep*10000))/10000.);
  xvalue = xvalue - xmodstep;
  yvalue = x->y_max - (x->y_current - x->x_obj.te_ypix ) * (x->y_max-x->y_min) / x->x_height ;
  if (yvalue < x->y_min ) yvalue = x->y_min;
  if (yvalue > x->y_max ) yvalue = x->y_max;
  ymodstep = ((float)((int)(yvalue*10000) % (int)(x->x_ystep*10000))/10000.);
  yvalue = yvalue - ymodstep;
  yvalue = 50 - yvalue;

  /* Use the coordinates only if they are non-zero: */

  if ((xvalue >= 0) && (yvalue >= 0)) {
    ox = xvalue;
    oy = yvalue;
  } else {
    xvalue = ox;
    yvalue = oy;
  } 

  if ((yvalue != 0)&&(yvalue!=50))
  {
   /* Calculate HSV based on given coordinates and convert to RGB: */
   hue = hue_inc * xvalue;
   if (yvalue <= box_y) {
    saturation = max_saturation;
    value = min_value + (value_inc * yvalue);
   } else {
    value = max_value - value_inc;
    saturation = max_saturation - (saturation_inc * (yvalue - box_y));
   } 

   pdp_colorgrid_hsv2rgb(hue, saturation, value, &red, &green, &blue);
 } else {
  if (yvalue == 0) {
    red     = 0;
    green   = 0;
    blue    = 0;
  } else {
    red     = 1;
    green   = 1;
    blue    = 1;
  }
 }
  
 /* The RGB values are returned in the interval [0..1] so we
    need to multiply by 256 to get "normal" color values.*/

  red     = red * 256;
  green   = green * 256;
  blue    = blue * 256;

    outlet_float( x->x_xoutlet, red );
    outlet_float( x->x_youtlet, green );
    outlet_float( x->x_zoutlet, blue );
}

/* ------------------------ pdp_colorgrid widgetbehaviour----------------------------- */


static void pdp_colorgrid_getrect(t_gobj *z, t_glist *owner,
			    int *xp1, int *yp1, int *xp2, int *yp2)
{
   t_pdp_colorgrid* x = (t_pdp_colorgrid*)z;

   *xp1 = x->x_obj.te_xpix;
   *yp1 = x->x_obj.te_ypix;
   *xp2 = x->x_obj.te_xpix+x->x_width;
   *yp2 = x->x_obj.te_ypix+x->x_height;
}

static void pdp_colorgrid_save(t_gobj *z, t_binbuf *b)
{
   t_pdp_colorgrid *x = (t_pdp_colorgrid *)z;

   // post( "saving pdp_colorgrid : %s", x->x_name->s_name );
   binbuf_addv(b, "ssiissiffiffiffiiff", gensym("#X"),gensym("obj"),
		(t_int)x->x_obj.te_xpix, (t_int)x->x_obj.te_ypix,
		gensym("pdp_colorgrid"), x->x_name, x->x_width, x->x_min,
		x->x_max, x->x_height,
                x->y_min, x->y_max,
                x->x_pdp_colorgrid, x->x_xstep, 
                x->x_ystep, x->x_xlines, x->x_ylines, 
                x->x_current, x->y_current );
   binbuf_addv(b, ";");
}

static void pdp_colorgrid_properties(t_gobj *z, t_glist *owner)
{
   char buf[800];
   t_pdp_colorgrid *x=(t_pdp_colorgrid *)z;

   sprintf(buf, "pdtk_pdp_colorgrid_dialog %%s %d %d %d\n",
                 x->x_xlines, x->x_ylines, x->x_pdp_colorgrid );
   // post("pdp_colorgrid_properties : %s", buf );
   gfxstub_new(&x->x_obj.ob_pd, x, buf);
}

static void pdp_colorgrid_select(t_gobj *z, t_glist *glist, int selected)
{
   t_pdp_colorgrid *x = (t_pdp_colorgrid *)z;

   x->x_selected = selected;
   pdp_colorgrid_draw_select( x, glist );
}

static void pdp_colorgrid_vis(t_gobj *z, t_glist *glist, int vis)
{
   t_pdp_colorgrid *x = (t_pdp_colorgrid *)z;

   if (vis)
   {
      pdp_colorgrid_draw_new( x, glist );
      pdp_colorgrid_draw_update( x, glist );
      pdp_colorgrid_output_current(x);
   }
   else
   {
      pdp_colorgrid_draw_erase( x, glist );
   }
}

static void pdp_colorgrid_dialog(t_pdp_colorgrid *x, t_symbol *s, int argc, t_atom *argv)
{
   if ( !x ) {
     post( "pdp_colorgrid : error :tried to set properties on an unexisting object" );
   }
   if ( argv[0].a_type != A_FLOAT || argv[1].a_type != A_FLOAT ||
           argv[2].a_type != A_FLOAT ) 
   { 
      post( "pdp_colorgrid : wrong arguments" );
      return;
   }
   x->x_xlines = argv[0].a_w.w_float;
   x->x_ylines = argv[1].a_w.w_float;
   x->x_pdp_colorgrid = argv[2].a_w.w_float;
   pdp_colorgrid_draw_erase(x, x->x_glist);
   pdp_colorgrid_draw_new(x, x->x_glist);
}

static void pdp_colorgrid_delete(t_gobj *z, t_glist *glist)
{
    canvas_deletelinesfor(glist, (t_text *)z);
}

static void pdp_colorgrid_displace(t_gobj *z, t_glist *glist, int dx, int dy)
{
    t_pdp_colorgrid *x = (t_pdp_colorgrid *)z;
    int xold = x->x_obj.te_xpix;
    int yold = x->x_obj.te_ypix;

    // post( "pdp_colorgrid_displace dx=%d dy=%d", dx, dy );

    x->x_obj.te_xpix += dx;
    x->x_current += dx;
    x->x_obj.te_ypix += dy;
    x->y_current += dy;
    if(xold != x->x_obj.te_xpix || yold != x->x_obj.te_ypix)
    {
	pdp_colorgrid_draw_move(x, x->x_glist);
    }
}

static void pdp_colorgrid_motion(t_pdp_colorgrid *x, t_floatarg dx, t_floatarg dy)
{
    int xold = x->x_current;
    int yold = x->y_current;

    // post( "pdp_colorgrid_motion dx=%f dy=%f", dx, dy );

    x->x_current += dx;
    x->y_current += dy;
    if(xold != x->x_current || yold != x->y_current)
    {
        pdp_colorgrid_output_current(x);
	pdp_colorgrid_draw_update(x, x->x_glist);
    }
}

static int pdp_colorgrid_click(t_gobj *z, struct _glist *glist,
			    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_pdp_colorgrid* x = (t_pdp_colorgrid *)z;

    // post( "pdp_colorgrid_click doit=%d x=%d y=%d", doit, xpix, ypix );
    if ( doit) 
    {
      x->x_current = xpix;
      x->y_current = ypix;
      pdp_colorgrid_output_current(x);
      pdp_colorgrid_draw_update(x, glist);
      glist_grab(glist, &x->x_obj.te_g, (t_glistmotionfn)pdp_colorgrid_motion,
	       0, xpix, ypix);
    }
    return (1);
}

static void pdp_colorgrid_goto(t_pdp_colorgrid *x, t_floatarg newx, t_floatarg newy)
{
    int xold = x->x_current;
    int yold = x->y_current;

    if ( newx > x->x_width-1 ) newx = x->x_width-1;
    if ( newx < 0 ) newx = 0;
    if ( newy > x->x_height-1 ) newy = x->x_height-1;
    if ( newy < 0 ) newy = 0;

    // post( "pdp_colorgrid_set x=%f y=%f", newx, newy );

    x->x_current = newx + x->x_obj.te_xpix;
    x->y_current = newy + x->x_obj.te_ypix;
    if(xold != x->x_current || yold != x->y_current)
    {
        pdp_colorgrid_output_current(x);
        pdp_colorgrid_draw_update(x, x->x_glist);
    }
}

static void pdp_colorgrid_xgoto(t_pdp_colorgrid *x, t_floatarg newx, t_floatarg newy)
{
    int xold = x->x_current;
    int yold = x->y_current;

    if ( newx > x->x_width-1 ) newx = x->x_width-1;
    if ( newx < 0 ) newx = 0;
    if ( newy > x->x_height-1 ) newy = x->x_height-1;
    if ( newy < 0 ) newy = 0;

    // post( "pdp_colorgrid_set x=%f y=%f", newx, newy );

    x->x_current = newx + x->x_obj.te_xpix;
    x->y_current = newy + x->x_obj.te_ypix;
    if(xold != x->x_current || yold != x->y_current)
    {
        pdp_colorgrid_draw_update(x, x->x_glist);
    }
}

static void pdp_colorgrid_bang(t_pdp_colorgrid *x) {
  pdp_colorgrid_output_current(x);
}

static t_pdp_colorgrid *pdp_colorgrid_new(t_symbol *s, int argc, t_atom *argv)
{
    int i, zz;
    t_pdp_colorgrid *x;
    t_pd *x2;
    char *str;
 
    // post( "pdp_colorgrid_new : create : %s argc =%d", s->s_name, argc );

    x = (t_pdp_colorgrid *)pd_new(pdp_colorgrid_class);
    // new pdp_colorgrid created from the gui 
    if ( argc != 0 )
    {
      if ( argc != 14 )
      {
        post( "pdp_colorgrid : error in the number of arguments ( %d instead of 14 )", argc );
        return NULL;
      }
      if ( argv[0].a_type != A_SYMBOL || argv[1].a_type != A_FLOAT ||
        argv[2].a_type != A_FLOAT || argv[3].a_type != A_FLOAT ||
        argv[4].a_type != A_FLOAT || argv[5].a_type != A_FLOAT ||
        argv[6].a_type != A_FLOAT || argv[7].a_type != A_FLOAT || 
        argv[8].a_type != A_FLOAT || argv[9].a_type != A_FLOAT || 
        argv[10].a_type != A_FLOAT || argv[11].a_type != A_FLOAT || 
        argv[12].a_type != A_FLOAT || argv[13].a_type != A_FLOAT ) {
        post( "pdp_colorgrid : wrong arguments" );
        return NULL;
      }

      // update pdp_colorgrid count
      if (!strncmp((str = argv[0].a_w.w_symbol->s_name), "pdp_colorgrid", 5)
    	 && (zz = atoi(str + 5)) > pdp_colorgridcount) 
      {
        pdp_colorgridcount = zz;
      }
      x->x_name = argv[0].a_w.w_symbol;
      pd_bind(&x->x_obj.ob_pd, x->x_name);
      x->x_width = argv[1].a_w.w_float;
      x->x_min = argv[2].a_w.w_float;
      x->x_max = argv[3].a_w.w_float;
      x->x_height = argv[4].a_w.w_float;
      x->y_min = argv[5].a_w.w_float;
      x->y_max = argv[6].a_w.w_float;
      x->x_pdp_colorgrid = argv[7].a_w.w_float;
      x->x_xstep = argv[8].a_w.w_float;
      x->x_ystep = argv[9].a_w.w_float;
      x->x_xlines = argv[10].a_w.w_float;
      x->x_ylines = argv[11].a_w.w_float;
      x->x_current = argv[12].a_w.w_float;
      x->y_current = argv[13].a_w.w_float;
      x->x_point = 1;
    }
    else
    {
      char buf[40];

      sprintf(buf, "pdp_colorgrid%d", ++pdp_colorgridcount);
      s = gensym(buf);    	

      x->x_name = s;
      pd_bind(&x->x_obj.ob_pd, x->x_name);

      x->x_width = DEFAULT_COLORGRID_WIDTH;
      x->x_min = 0;
      x->x_max = DEFAULT_COLORGRID_WIDTH;
      x->x_height = DEFAULT_COLORGRID_HEIGHT;
      x->y_min = 0;
      x->y_max = DEFAULT_COLORGRID_HEIGHT;
      x->x_pdp_colorgrid = 0;	
      x->x_xstep = 1.0;	
      x->x_ystep = 1.0;	
      x->x_xlines = DEFAULT_COLORGRID_NBLINES;	
      x->x_ylines = DEFAULT_COLORGRID_NBLINES;	
      x->x_current = 0;
      x->y_current = 0;	

    }

    // common fields for new and restored pdp_colorgrids
    x->x_point = 0;	
    x->x_selected = 0;	
    x->x_glist = (t_glist *) canvas_getcurrent();
    x->x_xoutlet = outlet_new(&x->x_obj, &s_float ); 
    x->x_youtlet = outlet_new(&x->x_obj, &s_float ); 
    x->x_zoutlet = outlet_new(&x->x_obj, &s_float ); 

    // post( "pdp_colorgrid_new name : %s width: %d height : %d", x->x_name->s_name, x->x_width, x->x_height );

    return (x);
}

static void pdp_colorgrid_free(t_pdp_colorgrid *x)
{
    post( "pdp_colorgrid~: freeing ressources [NULL]" );
}

void pdp_colorgrid_setup(void)
{
    load_tk_procs();
    // post ( pdp_colorgrid_version );
    pdp_colorgrid_class = class_new(gensym("pdp_colorgrid"), (t_newmethod)pdp_colorgrid_new,
			      (t_method)pdp_colorgrid_free, sizeof(t_pdp_colorgrid), 0, A_GIMME, 0);
    class_addmethod(pdp_colorgrid_class, (t_method)pdp_colorgrid_click, gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(pdp_colorgrid_class, (t_method)pdp_colorgrid_motion, gensym("motion"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(pdp_colorgrid_class, (t_method)pdp_colorgrid_bang, gensym("bang"), 0);
    class_addmethod(pdp_colorgrid_class, (t_method)pdp_colorgrid_goto, gensym("goto"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(pdp_colorgrid_class, (t_method)pdp_colorgrid_xgoto, gensym("xgoto"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(pdp_colorgrid_class, (t_method)pdp_colorgrid_dialog, gensym("dialog"), A_GIMME, 0);
    pdp_colorgrid_widgetbehavior.w_getrectfn =    pdp_colorgrid_getrect;
    pdp_colorgrid_widgetbehavior.w_displacefn =   pdp_colorgrid_displace;
    pdp_colorgrid_widgetbehavior.w_selectfn =     pdp_colorgrid_select;
    pdp_colorgrid_widgetbehavior.w_activatefn =   NULL;
    pdp_colorgrid_widgetbehavior.w_deletefn =     pdp_colorgrid_delete;
    pdp_colorgrid_widgetbehavior.w_visfn =        pdp_colorgrid_vis;
    pdp_colorgrid_widgetbehavior.w_clickfn =      pdp_colorgrid_click;
    class_setwidget(pdp_colorgrid_class, &pdp_colorgrid_widgetbehavior);
    class_setpropertiesfn(pdp_colorgrid_class, pdp_colorgrid_properties);
    class_setsavefn(pdp_colorgrid_class, pdp_colorgrid_save);
    class_sethelpsymbol(pdp_colorgrid_class, gensym("pdp_colorgrid-help.pd"));
}
