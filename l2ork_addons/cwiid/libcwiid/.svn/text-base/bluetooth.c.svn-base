/* Copyright (C) 2007 L. Donnie Smith <cwiid@abstrakraft.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  ChangeLog:
 *  2007-04-24 L. Donnie Smith (cwiid@abstrakraft.org>
 *  * revised error messages
 *
 *  2007-04-12 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * streamlined wiimote filter
 *
 *  2007-04-09 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * renamed wiimote to libcwiid, renamed structures accordingly
 *
 *  2007-04-07 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * changed cwiid_info.class to btclass
 *
 *  2007-04-03 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * fixed cwiid_find_wiimote seg fault
 *
 *  2007-04-02 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * exception handling bugs
 *
 *  2007-04-01 L. Donnie Smith <cwiid@abstrakraft.org>
 *  * created file
 */

#include <stdlib.h>
#include <string.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include "cwiid_internal.h"

/* When filtering wiimotes, in order to avoid having to store the
 * remote names before the blue_dev array is malloced (because we don't
 * yet know how many wiimotes there are, we'll assume there are no more
 * than dev_count, and realloc to the actual number afterwards, since
 * reallocing to a smaller chunk should be fast. */
#define BT_MAX_INQUIRY 256
/* timeout in 2 second units */
int cwiid_get_bdinfo_array(int dev_id, unsigned int timeout, int max_bdinfo,
                           struct cwiid_bdinfo **bdinfo, uint8_t flags)
{
	inquiry_info *dev_list = NULL;
	int max_inquiry;
	int dev_count;
	int sock = -1;
	int bdinfo_count;
	int i, j;
	int err = 0;
	int ret;

	/* NULLify for the benefit of error handling */
	*bdinfo = NULL;

	/* If not given (=-1), get the first available Bluetooth interface */
	if (dev_id == -1) {
		if ((dev_id = hci_get_route(NULL)) == -1) {
			cwiid_err(NULL, "No Bluetooth interface found");
			return -1;
		}
	}

	/* Get Bluetooth Device List */
	if ((flags & BT_NO_WIIMOTE_FILTER) && (max_bdinfo != -1)) {
		max_inquiry = max_bdinfo;
	}
	else {
		max_inquiry = BT_MAX_INQUIRY;
	}
	if ((dev_count = hci_inquiry(dev_id, timeout, max_inquiry, NULL,
	                             &dev_list, IREQ_CACHE_FLUSH)) == -1) {
		cwiid_err(NULL, "Bluetooth device inquiry error");
		err = 1;
		goto CODA;
	}

	if (dev_count == 0) {
		bdinfo_count = 0;
		goto CODA;
	}

	/* Open connection to Bluetooth Interface */
	if ((sock = hci_open_dev(dev_id)) == -1) {
		cwiid_err(NULL, "Bluetooth interface open error");
		err = 1;
		goto CODA;
	}

	/* Allocate info list */
	if (max_bdinfo == -1) {
		max_bdinfo = dev_count;
	}
	if ((*bdinfo = malloc(max_bdinfo * sizeof **bdinfo)) == NULL) {
		cwiid_err(NULL, "Memory allocation error (bdinfo array)");
		err = 1;
		goto CODA;
	}

	/* Copy dev_list to bdinfo */
	for (bdinfo_count=i=0; (i < dev_count) && (bdinfo_count < max_bdinfo);
	     i++) {
		/* Filter by class */
		if (!(flags & BT_NO_WIIMOTE_FILTER) &&
		  ((dev_list[i].dev_class[0] != WIIMOTE_CLASS_0) ||
		   (dev_list[i].dev_class[1] != WIIMOTE_CLASS_1) ||
		   (dev_list[i].dev_class[2] != WIIMOTE_CLASS_2))) {
			continue;
		}

		/* timeout (10000) in milliseconds */
		if (hci_read_remote_name(sock, &dev_list[i].bdaddr, BT_NAME_LEN,
		                         (*bdinfo)[bdinfo_count].name, 10000)) {
			cwiid_err(NULL, "Bluetooth name read error");
			err = 1;
			goto CODA;
		}

		/* Filter by name */
		if (!(flags & BT_NO_WIIMOTE_FILTER) &&
		  strncmp((*bdinfo)[bdinfo_count].name, WIIMOTE_NAME, BT_NAME_LEN) &&
		  strncmp((*bdinfo)[bdinfo_count].name, WIIBALANCE_NAME, BT_NAME_LEN)) {
			continue;
		}

		/* Passed filter, add to bdinfo */
		bacpy(&(*bdinfo)[bdinfo_count].bdaddr, &dev_list[i].bdaddr);
		for (j=0; j<3; j++) {
			(*bdinfo)[bdinfo_count].btclass[j] =
			            dev_list[i].dev_class[j];
		}
		bdinfo_count++;
	}

	if (bdinfo_count == 0) {
		free(*bdinfo);
	}
	else if (bdinfo_count < max_bdinfo) {
		if ((*bdinfo = realloc(*bdinfo, bdinfo_count * sizeof **bdinfo))
		  == NULL) {
			cwiid_err(NULL, "Memory reallocation error (bdinfo array)");
			err = 1;
			goto CODA;
		}
	}

CODA:
	if (dev_list) free(dev_list);
	if (sock != -1) hci_close_dev(sock);
	if (err) {
		if (*bdinfo) free(*bdinfo);
		ret = -1;
	}
	else {
		ret = bdinfo_count;
	}
	return ret;
}

int cwiid_find_wiimote(bdaddr_t *bdaddr, int timeout)
{
	struct cwiid_bdinfo *bdinfo;
	int bdinfo_count;

	if (timeout == -1) {
		while ((bdinfo_count = cwiid_get_bdinfo_array(-1, 2, 1, &bdinfo, 0))
		       == 0);
		if (bdinfo_count == -1) {
			return -1;
		}
	}
	else {
		bdinfo_count = cwiid_get_bdinfo_array(-1, timeout, 1, &bdinfo, 0);
		if (bdinfo_count == -1) {
			return -1;
		}
		else if (bdinfo_count == 0) {
			cwiid_err(NULL, "No wiimotes found");
			return -1;
		}
	}

	bacpy(bdaddr, &bdinfo[0].bdaddr);
	free(bdinfo);
	return 0;
}
