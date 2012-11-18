/* 
vst~ - VST plugin object for PD 
based on the work of Jarno Seppänen and Mark Williamson

Copyright (c)2003-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "vsthost.h"
#include <ctype.h>

static void striptrail(char *txt)
{
    // strip trailing whitespace
    for(int i = strlen(txt)-1; i >= 0; --i) 
        // cast to unsigned char since isspace functions don't want characters like 0x80 = -128
        if(isspace(((unsigned char *)txt)[i])) txt[i] = 0;
        else break;
}

void VSTPlugin::GetParamName(int numparam,char *name) const
{
    if(numparam < GetNumParams()) {
        name[0] = 0;
        Dispatch(effGetParamName,numparam,0,name);
        striptrail(name);
    }
	else 
        name[0] = 0;
}

bool VSTPlugin::SetParamFloat(int parameter,float value)
{
	if(Is() && parameter >= 0 && parameter < GetNumParams()) {
		effect->setParameter(effect,parameter,value);
		return true;
	}
    else
	    return false;
}

void VSTPlugin::GetParamValue(int numparam,char *parval) const
{
    if(Is()) {
        if(numparam < GetNumParams()) {
            // how many chars needed?
            char par_display[64]; par_display[0] = 0;
			Dispatch(effGetParamDisplay,numparam,0,par_display);
//            if(par_display[7]) par_display[8] = 0; // set trailing zero

            // how many chars needed?
			char par_label[64]; par_label[0] = 0;
            Dispatch(effGetParamLabel,numparam,0,par_label);
            striptrail(par_label);
//            if(par_label[7]) par_label[8] = 0; // set trailing zero

			sprintf(parval,"%s%s",par_display,par_label);
        }
	    else 
            strcpy(parval,"Index out of range");
    }
	else		
        strcpy(parval,"Plugin not loaded");
}

float VSTPlugin::GetParamValue(int numparam) const
{
	if(Is() && numparam < GetNumParams()) 
        return effect->getParameter(effect,numparam);
	else 
        return -1.0;
}

void VSTPlugin::ScanParams(int cnt)
{
    if(cnt < 0) cnt = GetNumParams();
    if(paramnamecnt >= cnt) return;
    if(cnt >= GetNumParams()) cnt = GetNumParams();

    char name[64];
    for(int i = paramnamecnt; i < cnt; ++i) {
        GetParamName(i,name);
        if(*name) paramnames[std::string(name)] = i;
    }
    paramnamecnt = cnt;
}

int VSTPlugin::GetParamIx(const char *p) const
{
    NameMap::const_iterator it = paramnames.find(std::string(p));
    return it == paramnames.end()?-1:it->second;
}

bool VSTPlugin::GetProgramName(int cat,int p,char *buf) const
{
    buf[0] = 0;
	int parameter = p;
	if(parameter < GetNumPrograms() && cat < GetNumCategories()) {
		Dispatch(effGetProgramNameIndexed,parameter,cat,buf);
        striptrail(buf);
		return true;
	}
	else
        return false;
}
