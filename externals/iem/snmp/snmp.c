/******************************************************
 *
 * snmp - implementation file
 *
 * copyleft (c) IOhannes m zmölnig
 *
 *   1603:forum::für::umläute:2007
 *
 *   institute of electronic music and acoustics (iem)
 *
 ******************************************************
 *
 * license: GNU General Public License v.2
 *
 ******************************************************/

#include "m_pd.h"


void snmpget_setup();

void snmp_setup(void)
{
  post("snmp4pd: SimpleNetworkManagementProtocol objects for PureData");
  post("snmp4pd: copyleft (c) IOhannes m zmoelnig 2006-2007");
  snmpget_setup();
}
