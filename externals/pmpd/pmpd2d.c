/*
--------------------------  pmpd2d  ---------------------------------------- 
     
      
 pmpd = physical modeling for pure data                                     
 Written by Cyrille Henry (ch@chnry.net)
 with help from Nicolas Mongermont
 
 Get sources on pd svn on sourceforge.

 This program is free software; you can redistribute it and/or                
 modify it under the terms of the GNU General Public License                  
 as published by the Free Software Foundation; either version 2               
 of the License, or (at your option) any later version.                       
                                                                             
 This program is distributed in the hope that it will be useful,              
 but WITHOUT ANY WARRANTY; without even the implied warranty of               
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                
 GNU General Public License for more details.                           
                                                                              
 You should have received a copy of the GNU General Public License           
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
                                                                              
 Based on PureData by Miller Puckette and others.                             
                                                                             

----------------------------------------------------------------------------
*/

#include "m_pd.h"
#include "stdio.h"
#include "math.h"

#include "pmpd2d.h"
#include "pmpd2d_core.c"
#include "pmpd2d_set.c"
#include "pmpd2d_get.c"
#include "pmpd2d_list.c"
#include "pmpd2d_tab.c"
#include "pmpd2d_test.c"
#include "pmpd2d_stat.c"
#include "pmpd2d_interactor.c"
#include "pmpd2d_various.c"
#include "pmpd2d_deprecated.c"

void pmpd2d_setup(void) 
{
pmpd2d_class = class_new(gensym("pmpd2d"),
    (t_newmethod)pmpd2d_new,
    0, sizeof(t_pmpd2d),CLASS_DEFAULT, A_GIMME, 0);

/*
 pmpd2d_core
 --
 Basic functions for creation of the structure
*/

    class_addbang(pmpd2d_class, pmpd2d_bang);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_reset,               gensym("reset"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_mass,                gensym("mass"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_link,                gensym("link"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_hinge,               gensym("hinge"), A_GIMME, 0);    
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_tLink,               gensym("tLink"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_tabLink,             gensym("tabLink"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_delLink,             gensym("delLink"), A_GIMME, 0);    
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_delMass,             gensym("delMass"), A_GIMME, 0);    

/*    
 pmpd2d_set
 --
 Functions to modify the internal state of the pmpd3d object
*/

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setK,                gensym("setK"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setD,                gensym("setD"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setPow,              gensym("setPow"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setDEnv,             gensym("setDEnv"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setDEnvOffset,       gensym("setDEnvOffset"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setL,                gensym("setL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_addL,                gensym("addL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setLCurrent,         gensym("setLCurrent"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setLKTab,            gensym("setLKTab"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setLDTab,            gensym("setLDTab"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setLinkId,           gensym("setLinkId"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setMassId,           gensym("setMassId"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setFixed,            gensym("setFixed"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setMobile,           gensym("setMobile"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setSpeed,            gensym("setSpeed"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setSpeedX,           gensym("setSpeedX"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setSpeedY,           gensym("setSpeedY"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setForce,            gensym("setForce"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setForceX,           gensym("setForceX"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setForceY,           gensym("setForceY"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setActive,           gensym("setActive"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setInactive,         gensym("setInactive"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_pos,                 gensym("pos"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_posX,                gensym("posX"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_posY,                gensym("posY"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_pos,                 gensym("setPos"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_posX,                gensym("setPosX"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_posY,                gensym("setPosY"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_overdamp,	  	    gensym("setOverdamp"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setEnd1,		        gensym("setEnd1"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setEnd2,		 	    gensym("setEnd2"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_setEnd,		  	    gensym("setEnd"), A_GIMME, 0);
/*        
 pmpd2d_get
 --
 Basic functions to output internal state of the object
 Output syntax : 1 line by element (mass/link)
*/	

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_get,                 gensym("get"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massPos,       	    gensym("massPos"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massSpeed,    	    gensym("massSpeed"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massForce,    	    gensym("massForce"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkPos,     	    gensym("linkPos"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkEnd,	       	    gensym("linkEnd"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkLength,     	    gensym("linkLength"), A_GIMME, 0);

/*
 pmpd2d_list 
 --
 Fucntions to output internal state of the object in lists
 Output Syntax : 1 list with all elements
*/

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massPosL,            gensym("massPosL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massSpeedL,          gensym("massSpeedL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massForceL,          gensym("massForceL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massPosXL,           gensym("massPosXL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massSpeedXL,         gensym("massSpeedXL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massForceXL,         gensym("massForceXL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massPosYL,           gensym("massPosYL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massSpeedYL,         gensym("massSpeedYL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massForceYL,         gensym("massForceYL"), 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massPosNormL,        gensym("massPosNormL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massSpeedNormL,      gensym("massSpeedNormL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massForceNormL,      gensym("massForceNormL"), 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkPosL,            gensym("linkPosL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkLengthL,         gensym("linkLengthL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkPosSpeedL,       gensym("linkPosSpeedL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkLengthSpeedL,    gensym("linkLengthSpeedL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkPosXL,           gensym("linkPosXL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkLengthXL,        gensym("linkLengthXL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkPosSpeedXL,      gensym("linkPosSpeedXL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkLengthSpeedXL,   gensym("linkLengthSpeedXL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkPosYL,           gensym("linkPosYL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkLengthYL,        gensym("linkLengthYL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkPosSpeedYL,      gensym("linkPosSpeedYL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkLengthSpeedYL,   gensym("linkLengthSpeedYL"), A_GIMME, 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkPosNormL,        gensym("linkPosNormL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkLengthNormL,     gensym("linkLengthNormL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkPosSpeedNormL,   gensym("linkPosSpeedNormL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkLengthSpeedNormL,gensym("linkLengthSpeedNormL"), A_GIMME, 0);

/*
 pmpd2d_tab
 --
 Functions to copy the internal state of the object in arrays
 Output Syntax : none
*/

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesPosT,          gensym("massesPosT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesSpeedsT,       gensym("massesSpeedsT"),A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesForcesT,       gensym("massesForcesT"),A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesPosXT,         gensym("massesPosXT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesSpeedsXT,      gensym("massesSpeedsXT"),A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesForcesXT,      gensym("massesForcesXT"),A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesPosYT,         gensym("massesPosYT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesSpeedsYT,      gensym("massesSpeedsYT"),A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesForcesYT,      gensym("massesForcesYT"),A_GIMME, 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesPosT,          gensym("massPosT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesSpeedsT,       gensym("massSpeedT"),A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesForcesT,       gensym("massForceT"),A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesPosXT,         gensym("massPosXT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesSpeedsXT,      gensym("massSpeedXT"),A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesForcesXT,      gensym("massForceXT"),A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesPosYT,         gensym("massPosYT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesSpeedsYT,      gensym("massSpeedYT"),A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesForcesYT,      gensym("massForceYT"),A_GIMME, 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesPosNormT,      gensym("massesPosNormT"),  A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesSpeedsNormT,   gensym("massesSpeedsNormT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesForcesNormT,   gensym("massesForcesNormT"), A_GIMME, 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesPosNormT,      gensym("massPosNormT"),  A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesSpeedsNormT,   gensym("massSpeedNormT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesForcesNormT,   gensym("massForceNormT"), A_GIMME, 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosT,           gensym("linksPosT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthT,        gensym("linksLengthT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosSpeedT,      gensym("linksPosSpeedT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthSpeedT,   gensym("linksLengthSpeedT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosXT,          gensym("linksPosXT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthXT,       gensym("linksLengthXT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosSpeedXT,     gensym("linksPosSpeedXT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthSpeedXT,  gensym("linksLengthSpeedXT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosYT,          gensym("linksPosYT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthYT,       gensym("linksLengthYT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosSpeedYT,     gensym("linksPosSpeedYT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthSpeedYT,  gensym("linksLengthSpeedYT"), A_GIMME, 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosT,           gensym("linkPosT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthT,        gensym("linkLengthT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosSpeedT,      gensym("linkPosSpeedT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthSpeedT,   gensym("linkLengthSpeedT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosXT,          gensym("linkPosXT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthXT,       gensym("linkLengthXT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosSpeedXT,     gensym("linkPosSpeedXT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthSpeedXT,  gensym("linkLengthSpeedXT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosYT,          gensym("linkPosYT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthYT,       gensym("linkLengthYT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosSpeedYT,     gensym("linkPosSpeedYT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthSpeedYT,  gensym("linkLengthSpeedYT"), A_GIMME, 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosNormT,       gensym("linksPosNormT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthNormT,    gensym("linksLengthNormT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosSpeedNormT,  gensym("linksPosSpeedNormT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthSpeedNormT,gensym("linksLengthSpeedNormT"), A_GIMME, 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosNormT,       gensym("linkPosNormT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthNormT,    gensym("linkLengthNormT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosSpeedNormT,  gensym("linkPosSpeedNormT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthSpeedNormT,gensym("linkLengthSpeedNormT"), A_GIMME, 0);
    
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkEndT,            gensym("linkEndT"),   A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkEnd1T,           gensym("linkEnd1T"),  A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkEnd2T,           gensym("linkEnd2T"),  A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkEndXT,           gensym("linkEndXT"),  A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkEnd1XT,          gensym("linkEnd1XT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkEnd2XT,          gensym("linkEnd2XT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkEndYT,           gensym("linkEndYT"),  A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkEnd1YT,          gensym("linkEnd1YT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkEnd2YT,          gensym("linkEnd2YT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkEndNormeForce,   gensym("linkEndForceNorm"), A_GIMME, 0);

/*
 pmpd2d_test
 --
 Functions to list all elements that fit specific conditions
 Output syntax : depends of the function
*/
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_testMass,       		gensym("testMass"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_testLink,       		gensym("testLink"), A_GIMME, 0); 
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_testMassT,       	gensym("testMassT"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_testLinkT,       	gensym("testLinkT"), A_GIMME, 0); 
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_testMassL,       	gensym("testMassL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_testLinkL,       	gensym("testLinkL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_testMassN,       	gensym("testMassN"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_testLinkN,       	gensym("testLinkN"), A_GIMME, 0); 
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_testMassNumber,      gensym("testMassNumber"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_testLinkNumber,      gensym("testLinkNumber"), A_GIMME, 0); 
/*    
 pmpd2d_stat
 --
 Functions to get global stats
*/

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massPosMean,         gensym("massPosMean"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massPosStd,          gensym("massPosStd"),A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massForcesMean,      gensym("massForceMean"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massForcesStd,       gensym("massForceStd"),A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massSpeedsMean,      gensym("massSpeedMean"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massSpeedsStd,       gensym("massSpeedStd"),A_GIMME, 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkPosMean,         gensym("linkPosMean"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkLengthMean,      gensym("linkLengthMean"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkPosSpeedMean,    gensym("linkPosSpeedMean"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkLengthSpeedMean, gensym("linkLengthSpeedMean"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkPosStd,          gensym("linkPosStd"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkLengthStd,       gensym("linkLengthStd"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkPosSpeedStd,     gensym("linkPosSpeedStd"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkLengthSpeedStd,  gensym("linkLengthSpeedStd"), A_GIMME, 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massInfo,            gensym("massInfo"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkInfo,            gensym("linkInfo"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massNumber,          gensym("massNumber"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linkNumber,          gensym("linkNumber"), A_GIMME, 0);

/* 	
 pmpd2d_interactor
 --
 Functions to add a global interaction with a specific shape
*/ 
 
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_iCircle,				gensym("iCircle"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_iLine,				gensym("iLine"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_iMatrix,				gensym("iMatrix"), A_GIMME, 0);

/*
 pmpd2d_various
 --
 Others
*/

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_infosL,              gensym("infosL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_infosL,              gensym("print"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_force,               gensym("force"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_forceX,              gensym("forceX"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_forceY,              gensym("forceY"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_min,                 gensym("min"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_max,                 gensym("max"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_minX,                gensym("Xmin"), A_DEFFLOAT, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_maxX,                gensym("Xmax"), A_DEFFLOAT, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_minY,                gensym("Ymin"), A_DEFFLOAT, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_maxY,                gensym("Ymax"), A_DEFFLOAT, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_minX,                gensym("minX"), A_DEFFLOAT, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_maxX,                gensym("maxX"), A_DEFFLOAT, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_minY,                gensym("minY"), A_DEFFLOAT, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_maxY,                gensym("maxY"), A_DEFFLOAT, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_addPos,              gensym("addPos"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_addPosX,             gensym("addPosX"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_addPosY,             gensym("addPosY"), A_GIMME, 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_grabMass,            gensym("grabMass"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_closestMass,         gensym("closestMass"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_closestMassN,        gensym("closestMassN"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massDistances,     	gensym("massDistance"), A_GIMME, 0);

//    class_addmethod(pmpd2d_class, (t_method)pmpd2d_forcesXT,       	gensym("forceXT"), A_GIMME, 0);
//    class_addmethod(pmpd2d_class, (t_method)pmpd2d_forcesYT,       	gensym("forceYT"), A_GIMME, 0);

/*
 pmpd3d_deprecated
 --
 Functions in which the output selector has been modified
 It is now the same as the input slector : all singular
*/

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesPosL,          gensym("massesPosL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesSpeedsL,       gensym("massesSpeedsL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesForcesL,       gensym("massesForcesL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesPosXL,         gensym("massesPosXL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesSpeedsXL,      gensym("massesSpeedsXL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesForcesXL,      gensym("massesForcesXL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesPosYL,         gensym("massesPosYL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesSpeedsYL,      gensym("massesSpeedsYL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesForcesYL,      gensym("massesForcesYL"), 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesPosNormL,      gensym("massesPosNormL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesSpeedsNormL,   gensym("massesSpeedsNormL"), 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesForcesNormL,   gensym("massesForcesNormL"), 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosL,           gensym("linksPosL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthL,        gensym("linksLengthL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosSpeedL,      gensym("linksPosSpeedL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthSpeedL,   gensym("linksLengthSpeedL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosXL,          gensym("linksPosXL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthXL,       gensym("linksLengthXL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosSpeedXL,     gensym("linksPosSpeedXL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthSpeedXL,  gensym("linksLengthSpeedXL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosYL,          gensym("linksPosYL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthYL,       gensym("linksLengthYL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosSpeedYL,     gensym("linksPosSpeedYL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthSpeedYL,  gensym("linksLengthSpeedYL"), A_GIMME, 0);

    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosNormL,       gensym("linksPosNormL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthNormL,    gensym("linksLengthNormL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksPosSpeedNormL,  gensym("linksPosSpeedNormL"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_linksLengthSpeedNormL,gensym("linksLengthSpeedNormL"), A_GIMME, 0);

    
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesPosMean,       gensym("massesPosMean"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesPosStd,        gensym("massesPosStd"),A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesForcesMean,    gensym("massesForcesMean"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesForcesStd,     gensym("massesForcesStd"),A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesSpeedsMean,    gensym("massesSpeedsMean"), A_GIMME, 0);
    class_addmethod(pmpd2d_class, (t_method)pmpd2d_massesSpeedsStd,     gensym("massesSpeedsStd"),A_GIMME, 0);
}

