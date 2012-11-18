/*------------ by Tom Murphy 7  ][  October 2001 ------------*/

#ifndef __MultiKick
#define __MultiKick

#ifndef __vstgui__
#include "vstgui.h"
#endif


/* idea for multikick */

class MultiKick : public CControl {
public:
  MultiKick (const CRect &size,
	     CControlListener *listener,
	     long tag,
	     int numstates_,
	     long heightOfOneImage,  // pixel
	     CBitmap *background,
	     CPoint &offset);
  virtual ~MultiKick ();

  virtual void draw (CDrawContext*);
  virtual void mouse (CDrawContext *pContext, CPoint &where);

  virtual void setValue(float);
  virtual float getValue();
  virtual bool isDirty();
  virtual void setDirty(const bool val = true);

protected:
  int numstates;
  CPoint offset;
  long heightOfOneImage;
  int buttondown; /* is a button down? */
  int obdown;
  int actualstate;
  int oactualstate;

};
#endif