/*
--------------------------  pmpd  ---------------------------------------- 
     
      
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

#include "m_pd.h"

#include "pmpd.h"
#include "pmpd_core.c"
#include "pmpd_set.c"
#include "pmpd_get.c"
#include "pmpd_list.c"
#include "pmpd_tab.c"
#include "pmpd_test.c"
#include "pmpd_stat.c"
#include "pmpd_various.c"
#include "pmpd_deprecated.c"

void pmpd_setup(void) 
{
 pmpd_class = class_new(gensym("pmpd"),
        (t_newmethod)pmpd_new,
        0, sizeof(t_pmpd),CLASS_DEFAULT, A_GIMME, 0);
/*
 pmpd_core
 --
 Basic functions for creation of the structure
*/
    class_addbang(pmpd_class, pmpd_bang);
    class_addmethod(pmpd_class, (t_method)pmpd_reset,                   gensym("reset"), 0);
    class_addmethod(pmpd_class, (t_method)pmpd_mass,                    gensym("mass"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_link,                    gensym("link"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_tabLink,                 gensym("tabLink"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_delLink,                 gensym("delLink"), A_GIMME, 0);    
    class_addmethod(pmpd_class, (t_method)pmpd_delMass,                 gensym("delMass"), A_GIMME, 0);   

/*    
 pmpd_set
 --
 Functions to modify the internal state of the pmpd object
*/
    class_addmethod(pmpd_class, (t_method)pmpd_setK,                    gensym("setK"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_setD,                    gensym("setD"), A_GIMME, 0);    
    class_addmethod(pmpd_class, (t_method)pmpd_setPow,                  gensym("setPow"), A_GIMME, 0);    
    class_addmethod(pmpd_class, (t_method)pmpd_setD2,                   gensym("setDEnv"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_setL,                    gensym("setL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_addL,                    gensym("addL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_setLCurrent,             gensym("setLCurrent"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_setLKTab,                gensym("setLKTab"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_setLDTab,                gensym("setLDTab"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_setLinkId,               gensym("setLinkId"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_setMassId,               gensym("setMassId"), A_GIMME, 0);  
    class_addmethod(pmpd_class, (t_method)pmpd_setFixed,                gensym("setFixed"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_setMobile,               gensym("setMobile"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_setSpeedX,               gensym("setSpeed"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_setSpeedX,               gensym("setSpeedX"), A_GIMME, 0);  
    class_addmethod(pmpd_class, (t_method)pmpd_setForceX,               gensym("setForce"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_setForceX,               gensym("setForceX"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_posX,                    gensym("pos"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_posX,                    gensym("posX"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_posX,                    gensym("setPos"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_posX,                    gensym("setPosX"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_overdamp,                gensym("setOverdamp"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_setEnd1,		            gensym("setEnd1"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_setEnd2,		            gensym("setEnd2"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_setEnd,		            gensym("setEnd"), A_GIMME, 0);

/*        
 pmpd_get
 --
 Basic functions to output internal state of the object
 Output syntax : 1 line by element (mass/link)
*/
    class_addmethod(pmpd_class, (t_method)pmpd_get,                     gensym("get"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massPos,       	        gensym("massPos"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massSpeed,    	        gensym("massSpeed"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massForce,    	        gensym("massForce"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkPos,     	        gensym("linkPos"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkEnd,	                gensym("linkEnd"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkLength,	            gensym("linkLength"), A_GIMME, 0);

/*
 pmpd_list 
 --
 Fucntions to output internal state of the object in lists
 Output Syntax : 1 list with all elements
*/

    class_addmethod(pmpd_class, (t_method)pmpd_massPosXL,      			gensym("massPosL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massSpeedXL,   			gensym("massSpeedL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massForceXL,   			gensym("massForceL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massPosXL,     			gensym("massPosXL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massSpeedXL,  			gensym("massSpeedXL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massForceXL,  			gensym("massForceXL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkPosXL,           	gensym("linkPosL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkLengthXL,            gensym("linkLengthL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkPosSpeedXL,          gensym("linkPosSpeedL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkLengthSpeedXL,       gensym("linkLengthSpeedL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkPosXL,               gensym("linkPosXL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkLengthXL,            gensym("linkLengthXL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkPosSpeedXL,          gensym("linkPosSpeedXL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkLengthSpeedXL,       gensym("linkLengthSpeedXL"), A_GIMME, 0);

/*
 pmpd_tab
 --
 Functions to copy the internal state of the object in arrays
 Output Syntax : none
*/
    class_addmethod(pmpd_class, (t_method)pmpd_massesPosT,              gensym("massesPosT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesSpeedsT,           gensym("massesSpeedsT"),A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesForcesT,           gensym("massesForcesT"),A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesPosT,              gensym("massesPosXT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesSpeedsT,           gensym("massesSpeedsXT"),A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesForcesT,           gensym("massesForcesXT"),A_GIMME, 0);

    class_addmethod(pmpd_class, (t_method)pmpd_massesPosT,              gensym("massPosT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesSpeedsT,           gensym("massSpeedT"),A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesForcesT,           gensym("massForceT"),A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesPosT,              gensym("massPosXT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesSpeedsT,           gensym("massSpeedXT"),A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesForcesT,           gensym("massForceXT"),A_GIMME, 0);

    class_addmethod(pmpd_class, (t_method)pmpd_linksPosT,               gensym("linksPosT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksLengthT,            gensym("linksLengthT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksPosSpeedT,          gensym("linksPosSpeedT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksLengthSpeedT,       gensym("linksLengthSpeedT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksPosT,               gensym("linksPosXT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksLengthT,            gensym("linksLengthXT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksPosSpeedT,          gensym("linksPosSpeedXT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksLengthSpeedT,       gensym("linksLengthSpeedXT"), A_GIMME, 0);

    class_addmethod(pmpd_class, (t_method)pmpd_linksPosT,               gensym("linkPosT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksLengthT,            gensym("linkLengthT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksPosSpeedT,          gensym("linkPosSpeedT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksLengthSpeedT,       gensym("linkLengthSpeedT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksPosT,               gensym("linkPosXT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksLengthT,            gensym("linkLengthXT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksPosSpeedT,          gensym("linkPosSpeedXT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksLengthSpeedT,       gensym("linkLengthSpeedXT"), A_GIMME, 0);

    class_addmethod(pmpd_class, (t_method)pmpd_linkEndT,                gensym("linksEndT"),   A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkEnd1T,               gensym("linkEnd1T"),  A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkEnd2T,               gensym("linkEnd2T"),  A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkEndT,                gensym("linkEndXT"),  A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkEnd1T,               gensym("linkEnd1XT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkEnd2T,               gensym("linkEnd2XT"), A_GIMME, 0);

/*
 pmpd_test
 --
 Functions to list all elements that fit specific conditions
 Output syntax : depends of the function
*/
    class_addmethod(pmpd_class, (t_method)pmpd_testMass,       			gensym("testMass"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_testLink,       			gensym("testLink"), A_GIMME, 0); 
    class_addmethod(pmpd_class, (t_method)pmpd_testMassT,       		gensym("testMassT"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_testLinkT,       		gensym("testLinkT"), A_GIMME, 0); 
    class_addmethod(pmpd_class, (t_method)pmpd_testMassL,       		gensym("testMassL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_testLinkL,       		gensym("testLinkL"), A_GIMME, 0); 
    class_addmethod(pmpd_class, (t_method)pmpd_testMassN,       		gensym("testMassN"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_testLinkN,       		gensym("testLinkN"), A_GIMME, 0); 
    class_addmethod(pmpd_class, (t_method)pmpd_testMassNumber,          gensym("testMassNumber"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_testLinkNumber,          gensym("testLinkNumber"), A_GIMME, 0); 
/*    
 pmpd_stat
 --
 Functions to get global stats
*/
    class_addmethod(pmpd_class, (t_method)pmpd_massPosMean,       		gensym("massPosMean"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massPosStd,        		gensym("massPosStd"),A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massForceMean,    		gensym("massForceMean"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massForceStd,     		gensym("massForceStd"),A_GIMME, 0);    
    class_addmethod(pmpd_class, (t_method)pmpd_massSpeedMean,    		gensym("massSpeedMean"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massSpeedStd,     		gensym("massSpeedStd"),A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkPosMean,				gensym("linkPosMean"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkLengthMean,         	gensym("linkLengthMean"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkPosSpeedMean,       	gensym("linkPosSpeedMean"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkLengthSpeedMean,    	gensym("linkLengthSpeedMean"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkPosStd,             	gensym("linkPosStd"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkLengthStd,          	gensym("linkLengthStd"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkPosSpeedStd,        	gensym("linkPosSpeedStd"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkLengthSpeedStd,		gensym("linkLengthSpeedStd"), A_GIMME, 0);

    class_addmethod(pmpd_class, (t_method)pmpd_massInfo,                gensym("massInfo"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkInfo,                gensym("linkInfo"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massNumber,              gensym("massNumber"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linkNumber,              gensym("linkNumber"), A_GIMME, 0);

/*
 pmpd_various
 --
 Others
*/
    class_addmethod(pmpd_class, (t_method)pmpd_infosL,                  gensym("infosL"), 0);
    class_addmethod(pmpd_class, (t_method)pmpd_infosL,                  gensym("print"), 0);
    class_addmethod(pmpd_class, (t_method)pmpd_forceX,                  gensym("force"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_forceX,                  gensym("forceX"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_minX,                    gensym("Xmin"), A_DEFFLOAT, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_minX,                    gensym("min"), A_DEFFLOAT, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_minX,                    gensym("minX"), A_DEFFLOAT, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_maxX,                    gensym("Xmax"), A_DEFFLOAT, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_maxX,                    gensym("max"), A_DEFFLOAT, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_maxX,                    gensym("maxX"), A_DEFFLOAT, 0);

    class_addmethod(pmpd_class, (t_method)pmpd_addPosX,                 gensym("addPos"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_addPosX,                 gensym("addPosX"), A_GIMME, 0);

    class_addmethod(pmpd_class, (t_method)pmpd_grabMass,                gensym("grabMass"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_closestMass,             gensym("closestMass"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_closestMassN,            gensym("closestMassN"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massDistances,	        gensym("massDistance"), A_GIMME, 0);  
//    class_addmethod(pmpd_class, (t_method)pmpd_forcesXT,              gensym("forceXT"), A_GIMME, 0);

/*
 pmpd_deprecated
 --
 Functions in which the output selector has been modified
 It is now the same as the input slector : all singular
*/
    class_addmethod(pmpd_class, (t_method)pmpd_massesPosL,              gensym("massesPosL"), 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesSpeedsL,           gensym("massesSpeedsL"), 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesForcesL,           gensym("massesForcesL"), 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesPosL,              gensym("massesPosXL"), 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesSpeedsL,           gensym("massesSpeedsXL"), 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesForcesL,           gensym("massesForcesXL"), 0);

    class_addmethod(pmpd_class, (t_method)pmpd_linksPosL,               gensym("linksPosL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksLengthL,            gensym("linksLengthL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksPosSpeedL,          gensym("linksPosSpeedL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksLengthSpeedL,       gensym("linksLengthSpeedL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksPosL,               gensym("linksPosXL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksLengthL,            gensym("linksLengthXL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksPosSpeedL,          gensym("linksPosSpeedXL"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_linksLengthSpeedL,       gensym("linksLengthSpeedXL"), A_GIMME, 0);

    class_addmethod(pmpd_class, (t_method)pmpd_massesPosMean,           gensym("massesPosMean"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesPosStd,            gensym("massesPosStd"),A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesForcesMean,        gensym("massesForcesMean"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesForcesStd,         gensym("massesForcesStd"),A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesSpeedsMean,        gensym("massesSpeedsMean"), A_GIMME, 0);
    class_addmethod(pmpd_class, (t_method)pmpd_massesSpeedsStd,         gensym("massesSpeedsStd"),A_GIMME, 0);
}

