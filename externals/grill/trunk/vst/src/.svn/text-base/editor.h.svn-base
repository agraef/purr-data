/* 
vst~ - VST plugin object for PD 
based on the work of Jarno Seppänen and Mark Williamson

Copyright (c)2003-2006 Thomas Grill (gr@grrrr.org)
For information on usage and redistribution, and for a DISCLAIMER OF ALL
WARRANTIES, see the file, "license.txt," in this distribution.  
*/

#ifndef __EDITOR_H
#define __EDITOR_H

class VSTPlugin;

void SetupEditor();
void StartEditor(VSTPlugin *p);
void StopEditor(VSTPlugin *p);
void ShowEditor(VSTPlugin *p,bool show);
void MoveEditor(VSTPlugin *p,int x,int y);
void SizeEditor(VSTPlugin *p,int x,int y);
void TitleEditor(VSTPlugin *p,const char *t); 
void CaptionEditor(VSTPlugin *p,bool c); 
void HandleEditor(VSTPlugin *p,bool h);
void FrontEditor(VSTPlugin *p);
void BelowEditor(VSTPlugin *p);

#endif // __EDITOR_H
