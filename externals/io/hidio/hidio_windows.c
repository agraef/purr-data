#ifdef _WINDOWS
/*
 *  Microsoft Windows DDK HID support for Pd/Max [hidio] object
 *
 *  Copyright (c) 2006 Olaf Matthes. All rights reserved.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 */

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <setupapi.h> 

/*
 * Please note that this file needs the Microsoft Driver Developent Kit (DDK)
 * to be installed in order to compile!
 */

#ifdef _MSC_VER
#include <hidsdi.h> 
#else
#include <ddk/hidsdi.h> 
#endif /* _MSC_VER */

#include "hidio.h"

//#define DEBUG(x)
#define DEBUG(x) x 

#define debug_print(d, p) post(p)

typedef struct _hid_device_
{
	HANDLE					fh;	/* file handle to the hid device */
	OVERLAPPED				overlapped;

	PHIDP_PREPARSED_DATA	ppd; // The opaque parser info describing this device
	HIDP_CAPS				caps; // The Capabilities of this hid device.
	HIDD_ATTRIBUTES			attributes;
	char					*inputReportBuffer;
	unsigned long			inputDataLength; // Num elements in this array.
	PHIDP_BUTTON_CAPS		inputButtonCaps;
	PHIDP_VALUE_CAPS		inputValueCaps;

	char					*outputReportBuffer;
	unsigned long			outputDataLength;
	PHIDP_BUTTON_CAPS		outputButtonCaps;
	PHIDP_VALUE_CAPS		outputValueCaps;

	char					*featureReportBuffer;
	unsigned long			featureDataLength;
	PHIDP_BUTTON_CAPS		featureButtonCaps;
	PHIDP_VALUE_CAPS		featureValueCaps;
} t_hid_device;


/*==============================================================================
 *  GLOBAL VARS
 *======================================================================== */

extern t_int hidio_instance_count; // in hidio.h

/* store device pointers so I don't have to query them all the time */
// t_hid_devinfo device_pointer[MAX_DEVICES];

/*==============================================================================
 * FUNCTION PROTOTYPES
 *==============================================================================
 */


/*==============================================================================
 * Event TYPE/CODE CONVERSION FUNCTIONS
 *==============================================================================
 */

/* ============================================================================== */
/* WINDOWS DDK HID SPECIFIC REALLY LOW-LEVEL STUFF */
/* ============================================================================== */
/*
 *  connect to Ith USB device (count starting with 0)
 */

static HANDLE connectDeviceNumber(DWORD deviceIndex)
{
    GUID hidGUID;
    HDEVINFO hardwareDeviceInfoSet;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_INTERFACE_DEVICE_DETAIL_DATA deviceDetail;
    ULONG requiredSize;
    HANDLE deviceHandle = INVALID_HANDLE_VALUE;
    DWORD result;

    //Get the HID GUID value - used as mask to get list of devices
    HidD_GetHidGuid (&hidGUID);

    //Get a list of devices matching the criteria (hid interface, present)
    hardwareDeviceInfoSet = SetupDiGetClassDevs (&hidGUID,
                                                 NULL, // Define no enumerator (global)
                                                 NULL, // Define no
                                                 (DIGCF_PRESENT | // Only Devices present
                                                 DIGCF_DEVICEINTERFACE)); // Function class devices.

    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    //Go through the list and get the interface data
    result = SetupDiEnumDeviceInterfaces (hardwareDeviceInfoSet,
                                          NULL, //infoData,
                                          &hidGUID, //interfaceClassGuid,
                                          deviceIndex, 
                                          &deviceInterfaceData);

    /* Failed to get a device - possibly the index is larger than the number of devices */
    if (result == FALSE)
    {
        SetupDiDestroyDeviceInfoList (hardwareDeviceInfoSet);
		error("%s: failed to get specified device number", CLASS_NAME);
        return INVALID_HANDLE_VALUE;
    }

    //Get the details with null values to get the required size of the buffer
    SetupDiGetDeviceInterfaceDetail (hardwareDeviceInfoSet,
                                     &deviceInterfaceData,
                                     NULL, //interfaceDetail,
                                     0, //interfaceDetailSize,
                                     &requiredSize,
                                     0); //infoData))

    //Allocate the buffer
    deviceDetail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(requiredSize);
    deviceDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

    //Fill the buffer with the device details
    if (!SetupDiGetDeviceInterfaceDetail (hardwareDeviceInfoSet,
                                          &deviceInterfaceData,
                                          deviceDetail,
                                          requiredSize,
                                          &requiredSize,
                                          NULL)) 
    {
        SetupDiDestroyDeviceInfoList (hardwareDeviceInfoSet);
        free (deviceDetail);
		error("%s: failed to get device info", CLASS_NAME);
        return INVALID_HANDLE_VALUE;
    }

    /* Open file on the device (read & write) */
	deviceHandle = CreateFile 
					(deviceDetail->DevicePath, 
					GENERIC_READ|GENERIC_WRITE, 
					FILE_SHARE_READ|FILE_SHARE_WRITE, 
					(LPSECURITY_ATTRIBUTES)NULL,
					OPEN_EXISTING, 
					FILE_FLAG_OVERLAPPED,
					NULL);

	if (deviceHandle == INVALID_HANDLE_VALUE)
	{
		int err = GetLastError();
		LPVOID lpMsgBuf;
		if (err == ERROR_ACCESS_DENIED)
		{
			/* error("%s: can not read from mouse and keyboard", CLASS_NAME); */
			return INVALID_HANDLE_VALUE;
		}
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
		error("%s: could not get device #%i: %s (%d)", CLASS_NAME, deviceIndex + 1, (LPCTSTR)lpMsgBuf, err);
		LocalFree(lpMsgBuf);
 	}

    SetupDiDestroyDeviceInfoList (hardwareDeviceInfoSet);
    free (deviceDetail);
    return deviceHandle;
}

static long _hidio_read(t_hid_device *self)
{
	unsigned long ret, err = 0;
	long bytes = 0;

    if (!ReadFile(self->fh, self->inputReportBuffer, self->caps.InputReportByteLength, &bytes, (LPOVERLAPPED)&self->overlapped)) 
    {
        if (ERROR_IO_PENDING != (err = GetLastError()))
		{
			LPVOID lpMsgBuf;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
			error("%s: read: %s", CLASS_NAME, (LPCTSTR)lpMsgBuf);
			LocalFree(lpMsgBuf);
			return -1;
		}
    }

	ret = WaitForSingleObject(self->overlapped.hEvent, 0);

	if (ret == WAIT_OBJECT_0)	/* hey, we got signalled ! => data */
	{
		return bytes;
	}
	else	/* if no data, cancel read */
	{
		if (!CancelIo(self->fh) && (self->fh != INVALID_HANDLE_VALUE))
			return -1;	/* CancelIo() failed */
	}
	if (!ResetEvent(self->overlapped.hEvent))
		return -1;	/* ResetEvent() failed */

	return bytes;
}

/* count devices by looking into the registry */
short _hid_count_devices(void)
{
    short	i, gNumDevices = 0;
	long	ret;

	HKEY	hKey;
    long	DeviceNameLen, KeyNameLen;
    char	KeyName[MAXPDSTRING];
	char	DeviceName[MAXPDSTRING];

	/* Search in Windows Registry for enumerated HID devices */
    if ((ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\hidusb\\Enum", 0, KEY_QUERY_VALUE, &hKey)) != ERROR_SUCCESS)
	{
		error("hidio: failed to get list of HID devices from registry");
        return EXIT_FAILURE;
    }

    for (i = 0; i < MAX_DEVICES + 3; i++)	/* there are three entries that are no devices */
	{
        DeviceNameLen = 80;
        KeyNameLen = 100;
		ret = RegEnumValue(hKey, i, KeyName, &KeyNameLen, NULL, NULL, DeviceName, &DeviceNameLen);
        if (ret == ERROR_SUCCESS)
		{
			if (!strncmp(KeyName, "Count", 5))
			{
				/* this is the number of devices as HEX DWORD */
				continue;
			}
			else if (!strncmp(DeviceName, "USB\\Vid", 7))
			{
				/* we found a device, DeviceName contains the path */
				// post("device #%d: %s = %s", gNumDevices, KeyName, DeviceName);
				gNumDevices++;
				continue;
			}
		}
		else if (ret == ERROR_NO_MORE_ITEMS)	/* no more entries in registry */
		{
			break;
		}
		else	/* any other error while looking into registry */
		{
			char errbuf[MAXPDSTRING];
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, ret, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							errbuf, MAXPDSTRING, NULL);
			error("hidio: $s", errbuf);
			break;
		}
	}
	RegCloseKey(hKey);
    return gNumDevices;		/* return number of devices */
}

/* get device path for a HID specified by number */
static short _hid_get_device_path(short device_number, char **path, short length)
{
	GUID guid;
	HDEVINFO DeviceInfo;
	SP_DEVICE_INTERFACE_DATA DeviceInterface;
	PSP_INTERFACE_DEVICE_DETAIL_DATA DeviceDetail;
	unsigned long iSize;
	
	HidD_GetHidGuid(&guid);

	DeviceInfo = SetupDiGetClassDevs(&guid, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

	DeviceInterface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	if (!SetupDiEnumDeviceInterfaces(DeviceInfo, NULL, &guid, device_number, &DeviceInterface))
	{
		SetupDiDestroyDeviceInfoList(DeviceInfo);
		return EXIT_FAILURE;
	}

	SetupDiGetDeviceInterfaceDetail( DeviceInfo, &DeviceInterface, NULL, 0, &iSize, 0 );

	DeviceDetail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(iSize);
	DeviceDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

	if (SetupDiGetDeviceInterfaceDetail(DeviceInfo, &DeviceInterface, DeviceDetail, iSize, &iSize, NULL))
	{
		if (!*path && !length)	/* we got no memory passed in, allocate some */
		{						/* WARNING: caller has to free this memory!! */
			*path = (char *)getbytes((short)(strlen(DeviceDetail->DevicePath) * sizeof(char)));
		}
		/* copy path */
		strcpy(*path, DeviceDetail->DevicePath);
	}
	free(DeviceDetail);

	SetupDiDestroyDeviceInfoList(DeviceInfo);
	return 0;
}


/* get capabilities (usage page & usage ID) of an already opened device */
static short _hid_get_capabilities(HANDLE fd, HIDP_CAPS *capabilities)
{
	PHIDP_PREPARSED_DATA	preparsedData;

	if (fd == INVALID_HANDLE_VALUE)
	{
		error("hidio: couldn't get device capabilities due to an invalid handle");
		return EXIT_FAILURE;
	}

	/* returns and allocates a buffer with device info */
	HidD_GetPreparsedData(fd, &preparsedData);

	/* get capabilities of device from above buffer */
	HidP_GetCaps(preparsedData, capabilities);

	/* no need for PreparsedData any more, so free the memory it's using */
	HidD_FreePreparsedData(preparsedData);
	return 0;
}


/* ============================================================================== */
/* WINDOWS DDK HID SPECIFIC SUPPORT FUNCTIONS */
/* ============================================================================== */

short get_device_number_by_id(unsigned short vendor_id, unsigned short product_id)
{
	return EXIT_FAILURE;
}

short get_device_number_from_usage(short device_number, 
										unsigned short usage_page, 
										unsigned short usage)
{
	HANDLE fd = INVALID_HANDLE_VALUE;
	HIDP_CAPS capabilities;
	char path[MAX_PATH];
	char *pp = (char *)path;
	short ret, i;
	short device_count = _hid_count_devices();

	for (i = device_number; i < device_count; i++)
	{
		/* get path for specified device number */
		ret = _hid_get_device_path(i, &pp, MAX_PATH);
		if (ret == -1)
		{
			return EXIT_FAILURE;
		}
		else
		{
			/* open file on the device (read & write, no overlapp) */
			fd = CreateFile(path,
								 GENERIC_READ|GENERIC_WRITE,
								 FILE_SHARE_READ|FILE_SHARE_WRITE,
								 (LPSECURITY_ATTRIBUTES)NULL,
								 OPEN_EXISTING,
								 0,
								 NULL);
			if (fd == INVALID_HANDLE_VALUE)
			{
				return EXIT_FAILURE;
			}

			/* get the capabilities */
			_hid_get_capabilities(fd, &capabilities);

			/* check whether they match with what we want */
			if (capabilities.UsagePage == usage_page && capabilities.Usage == usage)
			{
				CloseHandle(fd);
				return i;
			}
			CloseHandle(fd);
		}
	}
	return EXIT_FAILURE;
}


/*
 * This function is needed to translate the USB HID relative flag into the
 * [hidio]/linux style events
 */
static void convert_axis_to_symbols(t_hid_element *new_element, int array_index)
{
	if (new_element->relative) 
	{ 
		new_element->type = ps_relative; 
		new_element->name = relative_symbols[array_index];
	}
	else 
	{ 
		new_element->type = ps_absolute; 
		new_element->name = absolute_symbols[array_index];
	}
}

static void get_usage_symbols(unsigned short usage_page, unsigned short usage, t_hid_element *new_element) 
{
	char buffer[MAXPDSTRING];

	switch (usage_page)
	{
	case HID_USAGE_PAGE_GENERIC:
		switch (usage)
		{
			case HID_USAGE_GENERIC_X: convert_axis_to_symbols(new_element, 0); break;
			case HID_USAGE_GENERIC_Y: convert_axis_to_symbols(new_element, 1); break;
			case HID_USAGE_GENERIC_Z: convert_axis_to_symbols(new_element, 2); break;
			case HID_USAGE_GENERIC_RX: convert_axis_to_symbols(new_element, 3); break;
			case HID_USAGE_GENERIC_RY: convert_axis_to_symbols(new_element, 4); break;
			case HID_USAGE_GENERIC_RZ: convert_axis_to_symbols(new_element, 5); break;
			case HID_USAGE_GENERIC_SLIDER: convert_axis_to_symbols(new_element, 6); break;
			case HID_USAGE_GENERIC_DIAL: convert_axis_to_symbols(new_element, 7); break;
			case HID_USAGE_GENERIC_WHEEL: convert_axis_to_symbols(new_element, 8); break; 
			case HID_USAGE_GENERIC_HATSWITCH: 
				// TODO: this is still a mystery how to handle, due to USB HID vs. Linux input.h
				new_element->type = ps_absolute; 
				new_element->name = absolute_symbols[9]; /* hatswitch */
				break;
			default:
				new_element->type = gensym("DESKTOP");
				snprintf(buffer, sizeof(buffer), "DESKTOP%ld", usage); 
				new_element->name = gensym(buffer);
		}
		break;
	case HID_USAGE_PAGE_SIMULATION:
		switch (usage)
		{
		case HID_USAGE_SIMULATION_RUDDER: 
			new_element->type = ps_absolute;
			new_element->name = absolute_symbols[5]; /* rz */
			break;
		case HID_USAGE_SIMULATION_THROTTLE:
			new_element->type = ps_absolute;
			new_element->name = absolute_symbols[6]; /* slider */
			break;
		default:
			new_element->type = gensym("SIMULATION");
			snprintf(buffer, sizeof(buffer), "SIMULATION%ld", usage); 
			new_element->name = gensym(buffer);
		}
		break;
	case HID_USAGE_PAGE_KEYBOARD:
		new_element->type = ps_key;
		if( (usage > -1) && 
			(usage < KEY_ARRAY_MAX) )
			new_element->name = key_symbols[usage];
		else /* PowerBook ADB keyboard reports 0xffffffff */
			new_element->name = key_symbols[0];
		break;
	case HID_USAGE_PAGE_BUTTON:
		new_element->type = ps_button;
		new_element->name = button_symbols[usage];
		break;
	case HID_USAGE_PAGE_LED:
		new_element->type = ps_led; 
		new_element->name = led_symbols[usage];
		break;
	case HID_USAGE_PAGE_DIGITIZER:	/* no sure whether this is the right for PID in OS X */
		new_element->type = ps_pid; 
		new_element->name = pid_symbols[usage];
		break;
	default:
		/* the rest are "vendor defined" so no translation table is possible */
		snprintf(buffer, sizeof(buffer), "0x%04x", (unsigned int) usage_page); 
		new_element->type = gensym(buffer); 
		snprintf(buffer, sizeof(buffer), "0x%04x", (unsigned int) usage); 
		new_element->name = gensym(buffer);
	}
}

static void hidio_build_element_list(t_hidio *x) 
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;

	t_hid_element *new_element = NULL;
	unsigned short numButtons = 0;
	unsigned short numValues = 0;
	unsigned short numelem;	/* total number of elements */
    unsigned short numCaps;
	unsigned long i;
    PHIDP_BUTTON_CAPS buttonCaps;
    PHIDP_VALUE_CAPS valueCaps;
    USAGE usage;

	element_count[x->x_device_number] = 0;
	if (self->fh != INVALID_HANDLE_VALUE)
	{
		/* now get device capabilities */
		HidD_GetPreparsedData(self->fh, &self->ppd);
		HidP_GetCaps(self->ppd, &self->caps);

		/* allocate memory for input and output reports */
		/* these buffers get used for sending and receiving */
		self->inputReportBuffer = (char *)getbytes((short)(self->caps.InputReportByteLength * sizeof(char)));
		self->outputReportBuffer = (char *)getbytes((short)(self->caps.OutputReportByteLength * sizeof(char)));
		self->featureReportBuffer = (char *)getbytes((short)(self->caps.FeatureReportByteLength * sizeof(char)));

		/* allocate memory for input info */
		self->inputButtonCaps = buttonCaps = (PHIDP_BUTTON_CAPS)getbytes((short)(self->caps.NumberInputButtonCaps * sizeof(HIDP_BUTTON_CAPS)));
		self->inputValueCaps = valueCaps = (PHIDP_VALUE_CAPS)getbytes((short)(self->caps.NumberInputValueCaps * sizeof(HIDP_VALUE_CAPS)));


		/* get capapbilities for buttons and values from device */
		numCaps = self->caps.NumberInputButtonCaps;
		HidP_GetButtonCaps (HidP_Input,
							buttonCaps,
							&numCaps,
							self->ppd);

		numCaps = self->caps.NumberInputValueCaps;
		HidP_GetValueCaps (HidP_Input,
						   valueCaps,
						   &numCaps,
						   self->ppd);

		/* number of elements is number of values (axes) plus number of buttons */
#if 1
		for (i = 0; i < self->caps.NumberInputValueCaps; i++, valueCaps++) 
		{
			if (valueCaps->IsRange) 
				numValues += valueCaps->Range.UsageMax - valueCaps->Range.UsageMin + 1;
			else
				numValues++;
		}
		for (i = 0; i < self->caps.NumberInputButtonCaps; i++, buttonCaps++) 
		{
			if (buttonCaps->IsRange) 
				numButtons += buttonCaps->Range.UsageMax - buttonCaps->Range.UsageMin + 1;
			else
				numButtons++;
		}
		numelem = numValues + numButtons;

		valueCaps = self->inputValueCaps;
		buttonCaps = self->inputButtonCaps;
#else
		/* maybe this could be done as well? works with my gamepad at least */
		numelem = HidP_MaxDataListLength(HidP_Input, self->ppd);
				   + HidP_MaxUsageListLength(HidP_Input, 0, self->ppd);
#endif

		/* now look through the reported capabilities of the devcie and fill in the elements struct */
		for (i = 0; i < self->caps.NumberInputButtonCaps; i++, buttonCaps++) 
		{
			if (buttonCaps->IsRange) 
			{
				for (usage = buttonCaps->Range.UsageMin; usage <= buttonCaps->Range.UsageMax; usage++)
				{
					new_element = getbytes(sizeof(t_hid_element));
					new_element->usage_page = buttonCaps->UsagePage;
					new_element->usage_id = usage;
					new_element->relative = !buttonCaps->IsAbsolute;	/* buttons always are absolute, no? */
					new_element->min = 0;
					new_element->max = 1;
					new_element->instance = 0;

					get_usage_symbols(new_element->usage_page, new_element->usage_id, new_element);
#ifdef PD
					SETSYMBOL(new_element->output_message, new_element->name); 
					SETFLOAT(new_element->output_message + 1, new_element->instance);
#else /* Max */
					atom_setsym(new_element->output_message, new_element->name);
					atom_setlong(new_element->output_message + 1, (long)new_element->instance);
#endif /* PD */
					element[x->x_device_number][element_count[x->x_device_number]] = new_element;
					++element_count[x->x_device_number];
				}
			}
			else
			{
				new_element = getbytes(sizeof(t_hid_element));
				new_element->usage_page = buttonCaps->UsagePage;
				new_element->usage_id = buttonCaps->NotRange.Usage;
				new_element->relative = !buttonCaps->IsAbsolute;	/* buttons always are absolute, no? */
				new_element->min = 0;
				new_element->max = 1;
				new_element->instance = 0;

				get_usage_symbols(new_element->usage_page, new_element->usage_id, new_element);
#ifdef PD
				SETSYMBOL(new_element->output_message, new_element->name); 
				SETFLOAT(new_element->output_message + 1, new_element->instance);
#else /* Max */
				atom_setsym(new_element->output_message, new_element->name);
				atom_setlong(new_element->output_message + 1, (long)new_element->instance);
#endif /* PD */
				element[x->x_device_number][element_count[x->x_device_number]] = new_element;
				++element_count[x->x_device_number];
			}
		}
		/* get value data */
		for (i = 0; i < numValues; i++, valueCaps++)
		{
			if (valueCaps->IsRange) 
			{
				for (usage = valueCaps->Range.UsageMin; usage <= valueCaps->Range.UsageMax; usage++) 
				{
					new_element = getbytes(sizeof(t_hid_element));
					new_element->usage_page = valueCaps->UsagePage;
					new_element->usage_id = usage;
					new_element->relative = !valueCaps->IsAbsolute;
					new_element->min = valueCaps->LogicalMin;
					new_element->max = valueCaps->LogicalMax;
					new_element->instance = 0;
					
					get_usage_symbols(new_element->usage_page, new_element->usage_id, new_element);
#ifdef PD
					SETSYMBOL(new_element->output_message, new_element->name); 
					SETFLOAT(new_element->output_message + 1, new_element->instance);
#else /* Max */
					atom_setsym(new_element->output_message, new_element->name);
					atom_setlong(new_element->output_message + 1, (long)new_element->instance);
#endif /* PD */
					element[x->x_device_number][element_count[x->x_device_number]] = new_element;
					++element_count[x->x_device_number];
				}
			} 
			else
			{
				new_element = getbytes(sizeof(t_hid_element));
				new_element->usage_page = valueCaps->UsagePage;
				new_element->usage_id = valueCaps->NotRange.Usage;
				new_element->relative = !valueCaps->IsAbsolute;
				new_element->min = valueCaps->LogicalMin;
				new_element->max = valueCaps->LogicalMax;
				new_element->instance = 0;
				
				get_usage_symbols(new_element->usage_page, new_element->usage_id, new_element);
#ifdef PD
				SETSYMBOL(new_element->output_message, new_element->name); 
				SETFLOAT(new_element->output_message + 1, new_element->instance);
#else /* Max */
				atom_setsym(new_element->output_message, new_element->name);
				atom_setlong(new_element->output_message + 1, (long)new_element->instance);
#endif /* PD */
				element[x->x_device_number][element_count[x->x_device_number]] = new_element;
				++element_count[x->x_device_number];
			}
		}
	}
}

t_int hidio_print_element_list(t_hidio *x)
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;
	t_hid_element *current_element;
	int i;

	debug_print(LOG_DEBUG,"hidio_print_element_list");

	post("[hidio] found %d elements:", element_count[x->x_device_number]);
	post("\n  TYPE\tCODE\t#\tEVENT NAME");
	post("-----------------------------------------------------------");
	for (i = 0; i < element_count[x->x_device_number]; i++)
	{
		current_element = element[x->x_device_number][i];
		post("  %s\t%s\t%d", current_element->type->s_name,
			 current_element->name->s_name,(int) current_element->instance);
	}
	post("");

	return EXIT_SUCCESS;	
}

t_int hidio_print_device_list(t_hidio *x) 
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;
	struct _GUID GUID;
	SP_INTERFACE_DEVICE_DATA DeviceInterfaceData;
	struct
	{
		DWORD cbSize;
		char DevicePath[MAX_PATH];
	} FunctionClassDeviceData;
	HIDD_ATTRIBUTES HIDAttributes;
	SECURITY_ATTRIBUTES SecurityAttributes;
	int i;
	HANDLE PnPHandle, HIDHandle;
	ULONG BytesReturned;
	int Success, ManufacturerName, ProductName;
	PWCHAR widestring[MAXPDSTRING];
	char ManufacturerBuffer[MAXPDSTRING];
	char ProductBuffer[MAXPDSTRING];
	const char NotSupplied[] = "NULL";
	DWORD lastError = 0;

	/* Initialize the GUID array and setup the security attributes for Win2000 */
	HidD_GetHidGuid(&GUID);
	SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	SecurityAttributes.lpSecurityDescriptor = NULL;
	SecurityAttributes.bInheritHandle = FALSE;

	/* Get a handle for the Plug and Play node and request currently active devices */
	PnPHandle = SetupDiGetClassDevs(&GUID, NULL, NULL, 
											  DIGCF_PRESENT|DIGCF_INTERFACEDEVICE);

	if ((int)PnPHandle == -1) 
	{ 
		error("[hidio] ERROR: Could not attach to PnP node");
		return (t_int) GetLastError();
	}

	post("");

	/* Lets look for a maximum of 32 Devices */
	for (i = 0; i < MAX_DEVICES; i++)
	{
		/* Initialize our data */
		DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
		/* Is there a device at this table entry */
		Success = SetupDiEnumDeviceInterfaces(PnPHandle, NULL, &GUID, i, 
														  &DeviceInterfaceData);
		if (Success)
		{
			/* There is a device here, get it's name */
			FunctionClassDeviceData.cbSize = 5;
			Success = SetupDiGetDeviceInterfaceDetail(PnPHandle, 
					&DeviceInterfaceData,
					(PSP_INTERFACE_DEVICE_DETAIL_DATA)&FunctionClassDeviceData, 
					256, &BytesReturned, NULL);
			if (!Success) 
			{ 
				error("[hidio] ERROR: Could not find the system name for device %d",i); 
				return GetLastError();
			}
			/* Can now open this device */
			HIDHandle = CreateFile(FunctionClassDeviceData.DevicePath, 
										  0, 
										  FILE_SHARE_READ|FILE_SHARE_WRITE, 
										  &SecurityAttributes, OPEN_EXISTING, 0, NULL);
			lastError =  GetLastError();
			if (HIDHandle == INVALID_HANDLE_VALUE) 
			{
				error("[hidio] ERROR: Could not open HID #%d, Errorcode = %d", i, (int)lastError);
				return lastError;
			}
			
			/* Get the information about this HID */
			Success = HidD_GetAttributes(HIDHandle, &HIDAttributes);
			if (!Success) 
			{ 
				error("[hidio] ERROR: Could not get HID attributes"); 
				return GetLastError(); 
			}
			ManufacturerName = HidD_GetManufacturerString(HIDHandle, widestring, MAXPDSTRING);
			wcstombs(ManufacturerBuffer, (const unsigned short *)widestring, MAXPDSTRING);
			ProductName = HidD_GetProductString(HIDHandle, widestring, MAXPDSTRING);
			wcstombs(ProductBuffer, (const unsigned short *)widestring, MAXPDSTRING);

			/* And display it! */
			post("__________________________________________________");
			post("Device %d: '%s' '%s' version %d", i, 
				 ManufacturerName ? ManufacturerBuffer : NotSupplied, ProductName ? ProductBuffer : NotSupplied, 
				 HIDAttributes.VersionNumber);
			post("    vendorID: 0x%04x    productID: 0x%04x",
				 HIDAttributes.VendorID, HIDAttributes.ProductID);

			CloseHandle(HIDHandle);
		} // if (SetupDiEnumDeviceInterfaces . .
	} // for (i = 0; i < 32; i++)
	SetupDiDestroyDeviceInfoList(PnPHandle);

	post("");

	return EXIT_SUCCESS;
}

void hidio_output_device_name(t_hidio *x, char *manufacturer, char *product) 
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;
	char      *device_name;
	t_symbol  *device_name_symbol;

	device_name = malloc( strlen(manufacturer) + 1 + strlen(product) + 1 );
//	device_name = malloc( 7 + strlen(manufacturer) + 1 + strlen(product) + 1 );
//	strcpy( device_name, "append " );
	strcat( device_name, manufacturer );
	strcat ( device_name, " ");
	strcat( device_name, product );
//	outlet_anything( x->x_status_outlet, gensym( device_name ),0,NULL );
#ifdef PD
	outlet_symbol( x->x_status_outlet, gensym( device_name ) );
#else
	outlet_anything( x->x_status_outlet, gensym( device_name ),0,NULL );
#endif
}

/* ------------------------------------------------------------------------------ */
/*  FORCE FEEDBACK FUNCTIONS */
/* ------------------------------------------------------------------------------ */

/* cross-platform force feedback functions */
t_int hidio_ff_autocenter( t_hidio *x, t_float value )
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;
	return EXIT_SUCCESS;
}


t_int hidio_ff_gain( t_hidio *x, t_float value )
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;
	return EXIT_SUCCESS;
}


t_int hidio_ff_motors( t_hidio *x, t_float value )
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;
	return EXIT_SUCCESS;
}


t_int hidio_ff_continue( t_hidio *x )
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;
	return EXIT_SUCCESS;
}


t_int hidio_ff_pause( t_hidio *x )
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;
	return EXIT_SUCCESS;
}


t_int hidio_ff_reset( t_hidio *x )
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;
	return EXIT_SUCCESS;
}


t_int hidio_ff_stopall( t_hidio *x )
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;
	return EXIT_SUCCESS;
}



// these are just for testing...
t_int hidio_ff_fftest ( t_hidio *x, t_float value)
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;
	return EXIT_SUCCESS;
}


void hidio_ff_print( t_hidio *x )
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;
}

/* ============================================================================== */
/* Pd [hidio] FUNCTIONS */
/* ============================================================================== */

void hidio_platform_specific_info(t_hidio *x)
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;
	//debug_print(LOG_DEBUG,"hidio_platform_specific_info");
}

void hidio_get_events(t_hidio *x)
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;
	t_hid_element *current_element = NULL;
	long bytesRead;

	debug_print(LOG_DEBUG,"hidio_get_events");

	while (_hidio_read(self) > 0)
	{
		unsigned long i;
		unsigned long size, length;
		unsigned short *usages;

		for (i = 0; i < element_count[x->x_device_number]; i++)
		{
			current_element = element[x->x_device_number][i];

			/* first try getting value data */
			if (HIDP_STATUS_SUCCESS == HidP_GetUsageValue(HidP_Input, current_element[i].usage_page, 0, current_element[i].usage_id, &current_element[i].value, 
										self->ppd, self->inputReportBuffer, self->caps.InputReportByteLength))
			{
				continue;
			}
			/* now try getting scaled value data */
			if (HIDP_STATUS_SUCCESS == HidP_GetScaledUsageValue(HidP_Input, current_element[i].usage_page, 0, current_element[i].usage_id, &current_element[i].value, 
										self->ppd, self->inputReportBuffer, self->caps.InputReportByteLength))
			{
				continue;
			}

			/* ask Windows how many usages we might expect at max. */
			length = size = HidP_MaxUsageListLength(HidP_Input, current_element[i].usage_page, self->ppd);
			if (size)
			{
				/* uh, can't I alloc this memory in advance? but we might have several button usages on
				   different usage pages, so I'd have to figure out the largest one */
				usages = (unsigned short *)getbytes((short)(size * sizeof(unsigned short)));
				/* finally try getting button data */
				if (HIDP_STATUS_SUCCESS == HidP_GetUsages(HidP_Input, current_element[i].usage_page, 0, usages, &length, 
											self->ppd, self->inputReportBuffer, self->caps.InputReportByteLength))
				{
					unsigned long j;

					current_element[i].value = 0;

					for (j = 0; j < length, usages[j] != 0; j++)
					{
						if (current_element[i].usage_id == usages[j])
						{
							current_element[i].value = 1;
							break;
						}
					}
				}
				freebytes(usages, (short)(size * sizeof(unsigned short)));
			}
		}
	}
}


t_int hidio_open_device(t_hidio *x, short device_number)
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;

	if (device_number >= 0)
	{
		// open new device
		self->fh = connectDeviceNumber(device_number);

		if (self->fh != INVALID_HANDLE_VALUE)
		{
			hidio_build_element_list(x);

			/* prepare overlapped structure */
			self->overlapped.Offset     = 0; 
			self->overlapped.OffsetHigh = 0; 
			self->overlapped.hEvent     = CreateEvent(NULL, TRUE, FALSE, "");
 
			return EXIT_SUCCESS;
		}
		else
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


t_int hidio_close_device(t_hidio *x)
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;

	debug_print(LOG_DEBUG, "hidio_close_device");
	
	if (x->x_device_number > -1)
	{
		if (self->fh != INVALID_HANDLE_VALUE)
		{
			unsigned long i;

			CloseHandle(self->fh);
			self->fh = INVALID_HANDLE_VALUE;

			/* free element list */
			for (i = 0; i < element_count[x->x_device_number]; i++);
				freebytes(element[x->x_device_number][i], sizeof(t_hid_element));
			element_count[x->x_device_number] = 0;

			/* free allocated memory */
			if (self->inputButtonCaps)
				freebytes(self->inputButtonCaps, (short)(self->caps.NumberInputButtonCaps * sizeof(HIDP_BUTTON_CAPS)));
			if (self->inputValueCaps)
				freebytes(self->inputValueCaps, (short)(self->caps.NumberInputValueCaps * sizeof(HIDP_VALUE_CAPS)));
			
			/* free report buffers */
			if (self->inputReportBuffer)
				freebytes(self->inputReportBuffer, (short)(self->caps.InputReportByteLength * sizeof(char)));
			if (self->outputReportBuffer)
				freebytes(self->outputReportBuffer, (short)(self->caps.OutputReportByteLength * sizeof(char)));
			if (self->featureReportBuffer)
				freebytes(self->featureReportBuffer, (short)(self->caps.FeatureReportByteLength * sizeof(char)));

			/* free preparsed data */
			if (self->ppd)
				HidD_FreePreparsedData(self->ppd);
		}
	}

	return EXIT_SUCCESS;
}


void hidio_build_device_list(void)
{
	debug_print(LOG_DEBUG,"hidio_build_device_list");
}


void hidio_print(t_hidio *x)
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;

	hidio_print_device_list(x);
	
	if (x->x_device_open) 
	{
		hidio_print_element_list(x);
		hidio_ff_print( x );
	}
}


void hidio_platform_specific_free(t_hidio *x)
{
	t_hid_device *self = (t_hid_device *)x->x_hid_device;

	debug_print(LOG_DEBUG,"hidio_platform_specific_free");

	if (self)
		freebytes(self, sizeof(t_hid_device));
}


void *hidio_platform_specific_new(t_hidio *x)
{
	t_hid_device *self;

	debug_print(LOG_DEBUG,"hidio_platform_specific_new");

	/* alloc memory for our instance */
	self = (t_hid_device *)getbytes(sizeof(t_hid_device));
	self->fh = INVALID_HANDLE_VALUE;

	return (void *)self;	/* return void pointer to our data struct */
}

#endif  /* _WINDOWS */
