/*
 *  usb2dmx_cmds.h
 *  
 *
 *  Created by Max Egger on 14.02.06.
 *
 */

#define cmd_SetSingleChannel 1
/* usb request for cmd_SetSingleChannel:
	bmRequestType:	ignored by device, should be USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT
	bRequest:		cmd_SetSingleChannel
	wValue:			value of channel to set [0 .. 255]
	wIndex:			channel index to set [0 .. 511]
	wLength:		ignored
*/
#define cmd_SetChannelRange 2
/* usb request for cmd_SetChannelRange:
	bmRequestType:	ignored by device, should be USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT
	bRequest:		cmd_SetChannelRange
	wValue:			number of channels to set [1 .. 512-wIndex]
	wIndex:			index of first channel to set [0 .. 511]
	wLength:		length of data, must be >= wValue
*/

#define cmd_StartBootloader 0xf8
// Start Bootloader for Software updates


#define err_BadChannel 1
#define err_BadValue 2
