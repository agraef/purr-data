
/******************************************************
 *
 * dmx4pd - header file
 *
 * copyleft (c) IOhannes m zm-bölnig-A
 *
 *   0603:forum::f-bür::umläute:2008-A
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/

#ifndef INCLUDE_DMX4PD_H__
#define INCLUDE_DMX4PD_H__

#ifndef DMX4PD_VERSION
# define DMX4PD_VERSION __DATE__
#endif



#include "m_pd.h"
#include <dmx/dmx.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define DMX4PD_POSTBANNER                                               \
  do {									\
    post("DMX4PD ("DMX4PD_VERSION"): (c) 2008 IOhannes m zmölnig - iem @ kug"); \
  } while(0)


#endif /* INCLUDE_DMX4PD_H__ */
