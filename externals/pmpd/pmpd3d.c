/*
--------------------------  pmpd3d  ---------------------------------------- 
     
      
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

#include "pmpd3d.h"
#include "pmpd3d_core.c"
#include "pmpd3d_set.c"
#include "pmpd3d_get.c"
#include "pmpd3d_list.c"
#include "pmpd3d_tab.c"
#include "pmpd3d_test.c"
#include "pmpd3d_stat.c"
#include "pmpd3d_interactor.c"
#include "pmpd3d_various.c"
#include "pmpd3d_deprecated.c"


void pmpd3d_setup(void) 
{
 pmpd3d_class = class_new(gensym("pmpd3d"),
        (t_newmethod)pmpd3d_new,
        0, sizeof(t_pmpd3d),CLASS_DEFAULT, A_GIMME, 0);
/*
 pmpd3d_core
 --
 Basic functions for creation of the structure
*/
	
    class_addbang(pmpd3d_class, pmpd3d_bang);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_reset,           		gensym("reset"), 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_mass,            		gensym("mass"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_link,            		gensym("link"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_tLink,           		gensym("tLink"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_tabLink,         		gensym("tabLink"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_delLink,         		gensym("delLink"), A_GIMME, 0);    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_delMass,         		gensym("delMass"), A_GIMME, 0);   
/*    
 pmpd3d_set
 --
 Functions to modify the internal state of the pmpd3d object
*/
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setK,            		gensym("setK"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setD,            		gensym("setD"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setPow,            		gensym("setPow"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setDEnv,         		gensym("setDEnv"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setDEnvOffset,   		gensym("setDEnvOffset"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setL,            		gensym("setL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_addL,            		gensym("addL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setLCurrent,            	gensym("setLCurrent"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setLKTab,        		gensym("setLKTab"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setLDTab,        		gensym("setLDTab"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setLinkId,       		gensym("setLinkId"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setMassId,       		gensym("setMassId"), A_GIMME, 0);    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setFixed,        		gensym("setFixed"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setMobile,       		gensym("setMobile"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setSpeed,        		gensym("setSpeed"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setSpeedX,       		gensym("setSpeedX"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setSpeedY,       		gensym("setSpeedY"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setSpeedZ,       		gensym("setSpeedZ"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setForce,        		gensym("setForce"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setForceX,       		gensym("setForceX"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setForceY,       		gensym("setForceY"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setForceZ,       		gensym("setForceZ"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setActive,        		gensym("setActive"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setInactive,       		gensym("setInactive"), A_GIMME, 0);     
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_pos,             		gensym("pos"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_posX,            		gensym("posX"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_posY,            		gensym("posY"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_posZ,            		gensym("posZ"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_pos,             		gensym("setPos"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_posX,            		gensym("setPosX"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_posY,            		gensym("setPosY"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_posZ,          		  	gensym("setPosZ"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_posSpherical,          	gensym("setPosSpherical"), A_GIMME, 0);    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_overdamp,     		  	gensym("setOverdamp"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setEnd1,		 		  	gensym("setEnd1"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setEnd2,		 		  	gensym("setEnd2"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_setEnd,		  		  	gensym("setEnd"), A_GIMME, 0);
          
/*        
 pmpd3d_get
 --
 Basic functions to output internal state of the object
 Output syntax : 1 line by element (mass/link)
*/	
	
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_get,             		gensym("get"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massPos,       			gensym("massPos"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massSpeed,    			gensym("massSpeed"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massForce,    			gensym("massForce"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPos,     	   		gensym("linkPos"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkEnd,	       			gensym("linkEnd"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLength,	       		gensym("linkLength"), A_GIMME, 0);

/*
 pmpd3d_list 
 --
 Fucntions to output internal state of the object in lists
 Output Syntax : 1 list with all elements
*/
	
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massPosL,      			gensym("massPosL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massSpeedL,   			gensym("massSpeedL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massForceL,   			gensym("massForceL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massPosXL,     			gensym("massPosXL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massSpeedXL,  			gensym("massSpeedXL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massForceXL,  			gensym("massForceXL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massPosYL,     			gensym("massPosYL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massSpeedYL,  			gensym("massSpeedYL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massForceYL,  			gensym("massForceYL"), A_GIMME, 0);    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massPosZL,     			gensym("massPosZL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massSpeedZL,  			gensym("massSpeedZL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massForceZL,  			gensym("massForceZL"), A_GIMME, 0);
    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massPosNormL,      		gensym("massPosNormL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massSpeedNormL,   		gensym("massSpeedNormL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massForceNormL,   		gensym("massForceNormL"), A_GIMME, 0);    
    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosL,           		gensym("linkPosL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthL,             gensym("linkLengthL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedL,           gensym("linkPosSpeedL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedL,        gensym("linkLengthSpeedL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosXL,               gensym("linkPosXL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthXL,            gensym("linkLengthXL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedXL,          gensym("linkPosSpeedXL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedXL,       gensym("linkLengthSpeedXL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosYL,               gensym("linkPosYL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthYL,            gensym("linkLengthYL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedYL,          gensym("linkPosSpeedYL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedYL,       gensym("linkLengthSpeedYL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosZL,               gensym("linkPosZL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthZL,            gensym("linkLengthZL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedZL,          gensym("linkPosSpeedZL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedZL,       gensym("linkLengthSpeedZL"), A_GIMME, 0);

    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosNormL,            gensym("linkPosNormL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthNormL,         gensym("linkLengthNormL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedNormL,       gensym("linkPosSpeedNormL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedNormL,    gensym("linkLengthSpeedNormL"), A_GIMME, 0);
    
/*
 pmpd3d_tab
 --
 Functions to copy the internal state of the object in arrays
 Output Syntax : none
*/
	
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosT,      		gensym("massesPosT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsT,   		gensym("massesSpeedsT"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesT,  	 		gensym("massesForcesT"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosXT,     		gensym("massesPosXT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsXT,  		gensym("massesSpeedsXT"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesXT,  		gensym("massesForcesXT"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosYT,     		gensym("massesPosYT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsYT,  		gensym("massesSpeedsYT"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesYT,  		gensym("massesForcesYT"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosZT,     		gensym("massesPosZT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsZT,  		gensym("massesSpeedsZT"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesZT,  		gensym("massesForcesZT"),A_GIMME, 0);
    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosT,      		gensym("massPosT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosSphericalT,     gensym("massPosSphericalT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsT,   		gensym("massSpeedT"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesT,  	 		gensym("massForceT"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosXT,     		gensym("massPosXT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsXT,  		gensym("massSpeedXT"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesXT,  		gensym("massForceXT"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosYT,     		gensym("massPosYT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsYT,  		gensym("massSpeedYT"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesYT,  		gensym("massForceYT"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosZT,     		gensym("massPosZT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsZT,  		gensym("massSpeedZT"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesZT,  		gensym("massForceZT"),A_GIMME, 0);
    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosNormT,      	gensym("massesPosNormT"),  A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsNormT,   	gensym("massesSpeedsNormT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesNormT,   	gensym("massesForcesNormT"), A_GIMME, 0);
		
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosNormT,      	gensym("massPosNormT"),  A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsNormT,   	gensym("massSpeedNormT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesNormT,   	gensym("massForceNormT"), A_GIMME, 0);
    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosT,           		gensym("linksPosT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthT,             gensym("linksLengthT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedT,           gensym("linksPosSpeedT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedT,        gensym("linksLengthSpeedT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosXT,               gensym("linksPosXT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthXT,       		gensym("linksLengthXT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedXT,      	gensym("linksPosSpeedXT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedXT,       gensym("linksLengthSpeedXT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosYT,               gensym("linksPosYT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthYT,            gensym("linksLengthYT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedYT,          gensym("linksPosSpeedYT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedYT,       gensym("linksLengthSpeedYT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosZT,               gensym("linksPosZT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthZT,            gensym("linksLengthZT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedZT,          gensym("linksPosSpeedZT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedZT,       gensym("linksLengthSpeedZT"), A_GIMME, 0);
    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosT,           		gensym("linkPosT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthT,             gensym("linkLengthT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedT,           gensym("linkPosSpeedT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedT,        gensym("linkLengthSpeedT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosXT,               gensym("linkPosXT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthXT,       		gensym("linkLengthXT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedXT,      	gensym("linkPosSpeedXT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedXT,       gensym("linkLengthSpeedXT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosYT,               gensym("linkPosYT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthYT,            gensym("linkLengthYT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedYT,          gensym("linkPosSpeedYT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedYT,       gensym("linkLengthSpeedYT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosZT,               gensym("linkPosZT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthZT,            gensym("linkLengthZT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedZT,          gensym("linkPosSpeedZT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedZT,       gensym("linkLengthSpeedZT"), A_GIMME, 0);

	class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosNormT,            gensym("linksPosNormT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthNormT,         gensym("linksLengthNormT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedNormT,       gensym("linksPosSpeedNormT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedNormT,    gensym("linksLengthSpeedNormT"), A_GIMME, 0);

    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosNormT,            gensym("linkPosNormT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthNormT,         gensym("linkLengthNormT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedNormT,       gensym("linkPosSpeedNormT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedNormT,    gensym("linkLengthSpeedNormT"), A_GIMME, 0);
    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkEndT,                gensym("linkEndT"),   A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkEnd1T,               gensym("linkEnd1T"),  A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkEnd2T,               gensym("linkEnd2T"),  A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkEndXT,               gensym("linkEndXT"),  A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkEnd1XT,              gensym("linkEnd1XT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkEnd2XT,              gensym("linkEnd2XT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkEndYT,               gensym("linkEndYT"),  A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkEnd1YT,              gensym("linkEnd1YT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkEnd2YT,              gensym("linkEnd2YT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkEndZT,               gensym("linkEndZT"),  A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkEnd1ZT,              gensym("linkEnd1ZT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkEnd2ZT,              gensym("linkEnd2ZT"), A_GIMME, 0);
    
/*
 pmpd3d_test
 --
 Functions to list all elements that fit specific conditions
 Output syntax : depends of the function
*/
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_testMass,       			gensym("testMass"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_testLink,       			gensym("testLink"), A_GIMME, 0); 
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_testMassT,       		gensym("testMassT"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_testLinkT,       		gensym("testLinkT"), A_GIMME, 0); 
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_testMassL,       		gensym("testMassL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_testLinkL,       		gensym("testLinkL"), A_GIMME, 0); 
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_testMassN,       		gensym("testMassN"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_testLinkN,       		gensym("testLinkN"), A_GIMME, 0); 
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_testMassNumber,          gensym("testMassNumber"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_testLinkNumber,          gensym("testLinkNumber"), A_GIMME, 0); 
/*    
 pmpd3d_stat
 --
 Functions to get global stats
*/
	
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massPosMean,       		gensym("massPosMean"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massPosStd,        		gensym("massPosStd"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massForceMean,    		gensym("massForceMean"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massForceStd,     		gensym("massForceStd"),A_GIMME, 0);    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massSpeedMean,    		gensym("massSpeedMean"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massSpeedStd,     		gensym("massSpeedStd"),A_GIMME, 0);

    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosMean,				gensym("linkPosMean"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthMean,         	gensym("linkLengthMean"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedMean,       	gensym("linkPosSpeedMean"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedMean,    	gensym("linkLengthSpeedMean"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosStd,             	gensym("linkPosStd"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthStd,          	gensym("linkLengthStd"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkPosSpeedStd,        	gensym("linkPosSpeedStd"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkLengthSpeedStd,		gensym("linkLengthSpeedStd"), A_GIMME, 0);

    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massInfo,                gensym("massInfo"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkInfo,                gensym("linkInfo"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massNumber,              gensym("massNumber"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linkNumber,              gensym("linkNumber"), A_GIMME, 0);
/* 	
 pmpd3d_interactor
 --
 Functions to add a global interaction with a specific shape
*/ 
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_iCylinder,				gensym("iCylinder"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_iPlane,					gensym("iPlane"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_iSphere,					gensym("iSphere"), A_GIMME, 0);
    
/*
 pmpd3d_various
 --
 Others
*/
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_infosL,          		gensym("infosL"), 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_infosL,          		gensym("print"), 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_force,           		gensym("force"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_forceX,          		gensym("forceX"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_forceY,          		gensym("forceY"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_forceZ,          		gensym("forceZ"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_min,             		gensym("min"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_max,             		gensym("max"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_minX,            		gensym("minX"), A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_maxX,            		gensym("maxX"), A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_minY,            		gensym("minY"), A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_maxY,            		gensym("maxY"), A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_minZ,            		gensym("minZ"), A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_maxZ,            		gensym("maxZ"), A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_minX,            		gensym("Xmin"), A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_maxX,            		gensym("Xmax"), A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_minY,            		gensym("Ymin"), A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_maxY,            		gensym("Ymax"), A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_minZ,            		gensym("Zmin"), A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_maxZ,            		gensym("Zmax"), A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_addPos,          		gensym("addPos"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_addPosX,         		gensym("addPosX"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_addPosY,         		gensym("addPosY"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_addPosZ,         		gensym("addPosZ"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_grabMass,        		gensym("grabMass"), A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_closestMass,     		gensym("closestMass"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_closestMassN,     		gensym("closestMassN"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massDistances,	     	gensym("massDistance"), A_GIMME, 0);
    
//    class_addmethod(pmpd3d_class, (t_method)pmpd3d_forcesXT,       			gensym("forceXT"), A_GIMME, 0);
//    class_addmethod(pmpd3d_class, (t_method)pmpd3d_forcesYT,       			gensym("forceYT"), A_GIMME, 0);
//    class_addmethod(pmpd3d_class, (t_method)pmpd3d_forcesZT,       			gensym("forceZT"), A_GIMME, 0);

/*
 pmpd3d_deprecated
 --
 Functions in which the output selector has been modified
 It is now the same as the input slector : all singular
*/
	
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosL,      		gensym("massesPosL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsL,   		gensym("massesSpeedsL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesL,   		gensym("massesForcesL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosXL,     		gensym("massesPosXL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsXL,  		gensym("massesSpeedsXL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesXL,  		gensym("massesForcesXL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosYL,     		gensym("massesPosYL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsYL,  		gensym("massesSpeedsYL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesYL,  		gensym("massesForcesYL"), A_GIMME, 0);    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosZL,     		gensym("massesPosZL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsZL,  		gensym("massesSpeedsZL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesZL,  		gensym("massesForcesZL"), A_GIMME, 0);
    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosNormL,      	gensym("massesPosNormL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsNormL,   	gensym("massesSpeedsNormL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesNormL,   	gensym("massesForcesNormL"), A_GIMME, 0);  
    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksPosL,           	gensym("linksPosL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksLengthL,            gensym("linksLengthL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksPosSpeedL,          gensym("linksPosSpeedL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksLengthSpeedL,       gensym("linksLengthSpeedL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksPosXL,              gensym("linksPosXL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksLengthXL,           gensym("linksLengthXL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksPosSpeedXL,         gensym("linksPosSpeedXL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksLengthSpeedXL,      gensym("linksLengthSpeedXL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksPosYL,              gensym("linksPosYL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksLengthYL,           gensym("linksLengthYL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksPosSpeedYL,         gensym("linksPosSpeedYL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksLengthSpeedYL,      gensym("linksLengthSpeedYL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksPosZL,              gensym("linksPosZL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksLengthZL,           gensym("linksLengthZL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksPosSpeedZL,         gensym("linksPosSpeedZL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksLengthSpeedZL,      gensym("linksLengthSpeedZL"), A_GIMME, 0);
    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksPosNormL,           gensym("linksPosNormL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksLengthNormL,        gensym("linksLengthNormL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksPosSpeedNormL,      gensym("linksPosSpeedNormL"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_linksLengthSpeedNormL,   gensym("linksLengthSpeedNormL"), A_GIMME, 0);
    
  
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosMean,       	gensym("massesPosMean"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesPosStd,        	gensym("massesPosStd"),A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesMean,    	gensym("massesForecesMean"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesForcesStd,     	gensym("massesForcesStd"),A_GIMME, 0);    
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsMean,    	gensym("massesSpeedsMean"), A_GIMME, 0);
    class_addmethod(pmpd3d_class, (t_method)pmpd3d_massesSpeedsStd,     	gensym("massesSpeedsStd"),A_GIMME, 0);
}

