/* 
vst~ - VST plugin object for PD 
based on the work of Jarno Seppänen and Mark Williamson

Copyright (c)2003-2005 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#include "vsthost.h"
#include "editor.h"


void VSTPlugin::Edit(bool open)
{	
	if(Is()) { 	
        if(open) {
		    if(HasEditor() && !IsEdited())
                StartEditor(this);
        }
        else if(IsEdited())
            StopEditor(this);
	}
}

void VSTPlugin::StartEditing(WHandle h)
{
    FLEXT_ASSERT(h != NULL);
	Dispatch(effEditOpen,0,0,hwnd = h);

    TitleEditor(this,title.c_str());
}

void VSTPlugin::StopEditing() 
{ 
    if(Is() && IsEdited()) 
        Dispatch(effEditClose);					
}

void VSTPlugin::Visible(bool vis,bool upd)
{	
    visible = vis;
	if(upd && Is() && IsEdited()) ShowEditor(this,vis);
}

void VSTPlugin::SetPos(int x,int y,bool upd)
{
    posx = x; posy = y; 
    if(upd && Is() && IsEdited()) MoveEditor(this,posx,posy);
}

void VSTPlugin::SetSize(int x,int y,bool upd)
{
    sizex = x; sizey = y; 
    if(upd && Is() && IsEdited()) SizeEditor(this,sizex,sizey);
}

void VSTPlugin::SetCaption(bool c) 
{
    caption = c; 
    if(Is() && IsEdited()) CaptionEditor(this,c);
}

void VSTPlugin::SetHandle(bool h) 
{
    handle = h; 
    if(Is() && IsEdited()) HandleEditor(this,h);
}

void VSTPlugin::SetTitle(const char *t)
{
    title = t; 
    if(Is() && IsEdited()) TitleEditor(this,t);
}

void VSTPlugin::ToFront()
{
    if(Is() && IsEdited()) {
        FrontEditor(this);
	    Dispatch(effEditTop,0,0,vendorname);
    }
}

void VSTPlugin::BelowFront()
{
    if(Is() && IsEdited())
        BelowEditor(this);
}
