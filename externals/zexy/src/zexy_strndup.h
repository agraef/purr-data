/* ********************************************** */
/* the ZEXY external                              */
/* ********************************************** */
/*                            forum::für::umläute */
/* ********************************************** */

/* the ZEXY external is a runtime-library for miller s. puckette's realtime-computermusic-software "pure data"
 * therefore you NEED "pure data" to make any use of the ZEXY external
 * (except if you want to use the code for other things)
 * download "pure data" at

 http://pd.iem.at
 ftp://iem.at/pd

 *
 * if you are looking for the latest release of the ZEXY-external you should have another look at

 http://puredata.info/community/projects/software/zexy/
 ftp://iem.at/pd/Externals/ZEXY

 *
 * ZEXY is published under the GNU GeneralPublicLicense, that must be shipped with ZEXY.
 * if you are using Debian GNU/linux, the GNU-GPL can be found under /usr/share/common-licenses/GPL
 * if you still haven't found a copy of the GNU-GPL, have a look at http://www.gnu.org
 *
 * "pure data" has it's own license, that comes shipped with "pure data".
 *
 * there are ABSOLUTELY NO WARRANTIES for anything
 */

#ifndef INCLUDE_ZEXY_STRNDUP_H__
#define INCLUDE_ZEXY_STRNDUP_H__

#include <stdlib.h>
#include <string.h>
static char *zexy_strndup(const char *s, size_t n)
{
  char *result=0;
  size_t len = strlen(s) + 1;
  if(len>n) {
    len=n+1;
  }

  result = malloc(len);
  if(!result) {
    return result;
  }
  memcpy(result, s, len);
  result[len-1] = 0;
  return result;
}
#endif /* INCLUDE_ZEXY_STRNDUP_H__ */
