/* xgui */

#include "m_pd.h"
#include "seg.c"
#include "number.c"

void seg_setup();
void number_setup();

void xgui_lib_setup(void)
{
  post("++ Xgui - Damien HENRY");
  post("++ v0.08");
  seg_setup();
  number_setup();
}

