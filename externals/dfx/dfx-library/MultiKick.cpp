/*------------ by Tom Murphy 7  ][  October 2001 ------------*/

#ifndef __MultiKick
#include "MultiKick.hpp"
#endif

MultiKick::MultiKick (const CRect &size,
		      CControlListener *listener,
		      long tag,
		      int ns,
		      long heightOfOneImage,
		      CBitmap *background,
		      CPoint  &offset) :
  CControl (size, listener, tag, background),
  numstates(ns),
  offset (offset),
  heightOfOneImage (heightOfOneImage),
  buttondown(0), obdown(0), actualstate(0), oactualstate(0) {
setDirty(true); }

MultiKick::~MultiKick () {}

float MultiKick::getValue() {
  if (numstates == 1)
    return 0.0f;
  else if (actualstate >= numstates)
    return 1.0f;
  else
    return ((float)actualstate)/((float)(numstates-1));
}

void MultiKick::setValue(float f) {
  actualstate = (int) (f * (numstates-1));
}

bool MultiKick::isDirty() {
  return (actualstate == oactualstate ||
	  buttondown == obdown);
}

void MultiKick::setDirty(const bool val) {
  if (val) oactualstate = -1;
  else {
    oactualstate = actualstate;
    obdown = buttondown;
  }
}

void MultiKick::draw (CDrawContext *pContext) {
  CPoint where (offset.h, offset.v);

  where.v += heightOfOneImage * ((actualstate<<1) + buttondown);

  if (pBackground) {
    if (bTransparencyEnabled)
      pBackground->drawTransparent (pContext, size, where);
    else
      pBackground->draw (pContext, size, where);
  }

  setDirty (false);
}

//------------------------------------------------------------------------
void MultiKick::mouse (CDrawContext *pContext, CPoint &where) {
  if (!bMouseEnabled) { buttondown = 0; return; }

  long button = pContext->getMouseButtons ();
  if (!(button & kLButton)) {
    buttondown = 0;
    return;
  }

  /* save old value in case the mouse is dragged off while the
     button is still held down. */

  int entrystate = actualstate;

  if (pContext->getMouseButtons ()) {

    // begin of edit parameter
    getParent ()->beginEdit (tag);
    do {
      if (where.h >= size.left && where.v >= size.top  &&
	  where.h <= size.right && where.v <= size.bottom) {
	actualstate = entrystate + 1;
	actualstate %= numstates;
	buttondown = 1 /* 1 */;
      } else {
	actualstate = entrystate;
	buttondown = 0;
      }
      
      if (isDirty ()) {
	listener->valueChanged (pContext, this);
      }

      pContext->getMouseLocation (where);

      doIdleStuff ();
      draw(pContext);
    } while (pContext->getMouseButtons ());

    setDirty(true);
    // end of edit parameter
    getParent ()->endEdit (tag);
  } else {
    actualstate ++;
    actualstate %= numstates;
  }
  draw(pContext);
  buttondown = 0;
  listener->valueChanged (pContext, this);
}
