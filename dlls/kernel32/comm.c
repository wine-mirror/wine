/*
 * DEC 93 Erik Bos <erik@xs4all.nl>
 *
 * Copyright 1996 Marcus Meissner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "winerror.h"
#include "winioctl.h"
#include "ddk/ntddser.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(comm);

/***********************************************************************
 *           COMM_Parse*   (Internal)
 *
 *  The following COMM_Parse* functions are used by the BuildCommDCB
 *  functions to help parse the various parts of the device control string.
 */
static LPCWSTR COMM_ParseStart(LPCWSTR ptr)
{
	/* The device control string may optionally start with "COMx" followed
	   by an optional ':' and spaces. */
	if(!wcsnicmp(ptr, L"COM", 3))
	{
		ptr += 3;

		/* Allow any com port above 0 as Win 9x does (NT only allows
		   values for com ports which are actually present) */
		if(*ptr < '1' || *ptr > '9')
			return NULL;
		
		/* Advance pointer past port number */
		while(*ptr >= '0' && *ptr <= '9') ptr++;
		
		/* The com port number must be followed by a ':' or ' ' */
		if(*ptr != ':' && *ptr != ' ')
			return NULL;

		/* Advance pointer to beginning of next parameter */
		while(*ptr == ' ') ptr++;
		if(*ptr == ':')
		{
			ptr++;
			while(*ptr == ' ') ptr++;
		}
	}
	/* The device control string must not start with a space. */
	else if(*ptr == ' ')
		return NULL;
	
	return ptr;
}
 
static LPCWSTR COMM_ParseNumber(LPCWSTR ptr, LPDWORD lpnumber)
{
	if(*ptr < '0' || *ptr > '9') return NULL;
	*lpnumber = wcstoul(ptr, NULL, 10);
	while(*ptr >= '0' && *ptr <= '9') ptr++;
	return ptr;
}

static LPCWSTR COMM_ParseParity(LPCWSTR ptr, LPBYTE lpparity)
{
	/* Contrary to what you might expect, Windows only sets the Parity
	   member of DCB and not fParity even when parity is specified in the
	   device control string */

	switch(*ptr++)
	{
	case 'e':
	case 'E':
		*lpparity = EVENPARITY;
		break;
	case 'm':
	case 'M':
		*lpparity = MARKPARITY;
		break;
	case 'n':
	case 'N':
		*lpparity = NOPARITY;
		break;
	case 'o':
	case 'O':
		*lpparity = ODDPARITY;
		break;
	case 's':
	case 'S':
		*lpparity = SPACEPARITY;
		break;
	default:
		return NULL;
	}

	return ptr;
}

static LPCWSTR COMM_ParseByteSize(LPCWSTR ptr, LPBYTE lpbytesize)
{
	DWORD temp;

	if(!(ptr = COMM_ParseNumber(ptr, &temp)))
		return NULL;

	if(temp >= 5 && temp <= 8)
	{
		*lpbytesize = temp;
		return ptr;
	}
	else
		return NULL;
}

static LPCWSTR COMM_ParseStopBits(LPCWSTR ptr, LPBYTE lpstopbits)
{
	DWORD temp;

	if(!wcsncmp(L"1.5", ptr, 3))
	{
		ptr += 3;
		*lpstopbits = ONE5STOPBITS;
	}
	else
	{
		if(!(ptr = COMM_ParseNumber(ptr, &temp)))
			return NULL;

		if(temp == 1)
			*lpstopbits = ONESTOPBIT;
		else if(temp == 2)
			*lpstopbits = TWOSTOPBITS;
		else
			return NULL;
	}
	
	return ptr;
}

static LPCWSTR COMM_ParseOnOff(LPCWSTR ptr, LPDWORD lponoff)
{
	if(!wcsnicmp(L"on", ptr, 2))
	{
		ptr += 2;
		*lponoff = 1;
	}
	else if(!wcsnicmp(L"off", ptr, 3))
	{
		ptr += 3;
		*lponoff = 0;
	}
	else
		return NULL;

	return ptr;
}

/***********************************************************************
 *           COMM_BuildOldCommDCB   (Internal)
 *
 *  Build a DCB using the old style settings string eg: "96,n,8,1"
 */
static BOOL COMM_BuildOldCommDCB(LPCWSTR device, LPDCB lpdcb)
{
	WCHAR last = 0;

	if(!(device = COMM_ParseNumber(device, &lpdcb->BaudRate)))
		return FALSE;
	
	switch(lpdcb->BaudRate)
	{
	case 11:
	case 30:
	case 60:
		lpdcb->BaudRate *= 10;
		break;
	case 12:
	case 24:
	case 48:
	case 96:
		lpdcb->BaudRate *= 100;
		break;
	case 19:
		lpdcb->BaudRate = 19200;
		break;
	}

	while(*device == ' ') device++;
	if(*device++ != ',') return FALSE;
	while(*device == ' ') device++;

	if(!(device = COMM_ParseParity(device, &lpdcb->Parity)))
		return FALSE;

	while(*device == ' ') device++;
	if(*device++ != ',') return FALSE;
	while(*device == ' ') device++;
		
	if(!(device = COMM_ParseByteSize(device, &lpdcb->ByteSize)))
		return FALSE;

	while(*device == ' ') device++;
	if(*device++ != ',') return FALSE;
	while(*device == ' ') device++;

	if(!(device = COMM_ParseStopBits(device, &lpdcb->StopBits)))
		return FALSE;

	/* The last parameter for flow control is optional. */
	while(*device == ' ') device++;
	if(*device == ',')
	{
		device++;
		while(*device == ' ') device++;
		if(*device) last = *device++;
		while(*device == ' ') device++;
	}

	/* Win NT sets the flow control members based on (or lack of) the last
	   parameter.  Win 9x does not set these members. */
	switch(last)
	{
	case 0:
		lpdcb->fInX = FALSE;
		lpdcb->fOutX = FALSE;
		lpdcb->fOutxCtsFlow = FALSE;
		lpdcb->fOutxDsrFlow = FALSE;
		lpdcb->fDtrControl = DTR_CONTROL_ENABLE;
		lpdcb->fRtsControl = RTS_CONTROL_ENABLE;
		break;
	case 'x':
	case 'X':
		lpdcb->fInX = TRUE;
		lpdcb->fOutX = TRUE;
		lpdcb->fOutxCtsFlow = FALSE;
		lpdcb->fOutxDsrFlow = FALSE;
		lpdcb->fDtrControl = DTR_CONTROL_ENABLE;
		lpdcb->fRtsControl = RTS_CONTROL_ENABLE;
		break;
	case 'p':
	case 'P':
		lpdcb->fInX = FALSE;
		lpdcb->fOutX = FALSE;
		lpdcb->fOutxCtsFlow = TRUE;
		lpdcb->fOutxDsrFlow = TRUE;
		lpdcb->fDtrControl = DTR_CONTROL_HANDSHAKE;
		lpdcb->fRtsControl = RTS_CONTROL_HANDSHAKE;
		break;
	default:
		return FALSE;
	}

	/* This should be the end of the string. */
	if(*device) return FALSE;
	
	return TRUE;
}

/***********************************************************************
 *           COMM_BuildNewCommDCB   (Internal)
 *
 *  Build a DCB using the new style settings string.
 *   eg: "baud=9600 parity=n data=8 stop=1 xon=on to=on"
 */
static BOOL COMM_BuildNewCommDCB(LPCWSTR device, LPDCB lpdcb, LPCOMMTIMEOUTS lptimeouts)
{
	DWORD temp;
	BOOL baud = FALSE, stop = FALSE;

	while(*device)
	{
		while(*device == ' ') device++;

		if(!wcsnicmp(L"baud=", device, 5))
		{
			baud = TRUE;

			if(!(device = COMM_ParseNumber(device + 5, &lpdcb->BaudRate)))
				return FALSE;
		}
		else if(!wcsnicmp(L"parity=", device, 7))
		{
			if(!(device = COMM_ParseParity(device + 7, &lpdcb->Parity)))
				return FALSE;
		}
		else if(!wcsnicmp(L"data=", device, 5))
		{
			if(!(device = COMM_ParseByteSize(device + 5, &lpdcb->ByteSize)))
				return FALSE;
		}
		else if(!wcsnicmp(L"stop=", device, 5))
		{
			stop = TRUE;

			if(!(device = COMM_ParseStopBits(device + 5, &lpdcb->StopBits)))
				return FALSE;
		}
		else if(!wcsnicmp(L"to=", device, 3))
		{
			if(!(device = COMM_ParseOnOff(device + 3, &temp)))
				return FALSE;

			lptimeouts->ReadIntervalTimeout = 0;
			lptimeouts->ReadTotalTimeoutMultiplier = 0;
			lptimeouts->ReadTotalTimeoutConstant = 0;
			lptimeouts->WriteTotalTimeoutMultiplier = 0;
			lptimeouts->WriteTotalTimeoutConstant = temp ? 60000 : 0;
		}
		else if(!wcsnicmp(L"xon=", device, 4))
		{
			if(!(device = COMM_ParseOnOff(device + 4, &temp)))
				return FALSE;

			lpdcb->fOutX = temp;
			lpdcb->fInX = temp;
		}
		else if(!wcsnicmp(L"odsr=", device, 5))
		{
			if(!(device = COMM_ParseOnOff(device + 5, &temp)))
				return FALSE;

			lpdcb->fOutxDsrFlow = temp;
		}
		else if(!wcsnicmp(L"octs=", device, 5))
		{
			if(!(device = COMM_ParseOnOff(device + 5, &temp)))
				return FALSE;

			lpdcb->fOutxCtsFlow = temp;
		}
		else if(!wcsnicmp(L"dtr=", device, 4))
		{
			if(!(device = COMM_ParseOnOff(device + 4, &temp)))
				return FALSE;

			lpdcb->fDtrControl = temp;
		}
		else if(!wcsnicmp(L"rts=", device, 4))
		{
			if(!(device = COMM_ParseOnOff(device + 4, &temp)))
				return FALSE;

			lpdcb->fRtsControl = temp;
		}
		else if(!wcsnicmp(L"idsr=", device, 5))
		{
			if(!(device = COMM_ParseOnOff(device + 5, &temp)))
				return FALSE;

			/* Win NT sets the fDsrSensitivity member based on the
			   idsr parameter.  Win 9x sets fOutxDsrFlow instead. */
			lpdcb->fDsrSensitivity = temp;
		}
		else
			return FALSE;

		/* After the above parsing, the next character (if not the end of
		   the string) should be a space */
		if(*device && *device != ' ')
			return FALSE;
	}

	/* If stop bits were not specified, a default is always supplied. */
	if(!stop)
	{
		if(baud && lpdcb->BaudRate == 110)
			lpdcb->StopBits = TWOSTOPBITS;
		else
			lpdcb->StopBits = ONESTOPBIT;
	}

	return TRUE;
}

/**************************************************************************
 *         BuildCommDCBA		(KERNEL32.@)
 *
 *  Updates a device control block data structure with values from an
 *  ANSI device control string.  The device control string has two forms
 *  normal and extended, it must be exclusively in one or the other form.
 *
 * RETURNS
 *
 *  True on success, false on a malformed control string.
 */
BOOL WINAPI BuildCommDCBA(
    LPCSTR device, /* [in] The ANSI device control string used to update the DCB. */
    LPDCB  lpdcb)  /* [out] The device control block to be updated. */
{
	return BuildCommDCBAndTimeoutsA(device,lpdcb,NULL);
}

/**************************************************************************
 *         BuildCommDCBAndTimeoutsA		(KERNEL32.@)
 *
 *  Updates a device control block data structure with values from an
 *  ANSI device control string.  Taking timeout values from a timeouts
 *  struct if desired by the control string.
 *
 * RETURNS
 *
 *  True on success, false bad handles etc.
 */
BOOL WINAPI BuildCommDCBAndTimeoutsA(
    LPCSTR         device,     /* [in] The ANSI device control string. */
    LPDCB          lpdcb,      /* [out] The device control block to be updated. */
    LPCOMMTIMEOUTS lptimeouts) /* [in] The COMMTIMEOUTS structure to be updated. */
{
	BOOL ret = FALSE;
	UNICODE_STRING deviceW;

	TRACE("(%s,%p,%p)\n",device,lpdcb,lptimeouts);
	if(device) RtlCreateUnicodeStringFromAsciiz(&deviceW,device);
	else deviceW.Buffer = NULL;

	if(deviceW.Buffer) ret = BuildCommDCBAndTimeoutsW(deviceW.Buffer,lpdcb,lptimeouts);

	RtlFreeUnicodeString(&deviceW);
	return ret;
}

/**************************************************************************
 *         BuildCommDCBAndTimeoutsW	(KERNEL32.@)
 *
 *  Updates a device control block data structure with values from a
 *  unicode device control string.  Taking timeout values from a timeouts
 *  struct if desired by the control string.
 *
 * RETURNS
 *
 *  True on success, false bad handles etc
 */
BOOL WINAPI BuildCommDCBAndTimeoutsW(
    LPCWSTR        devid,      /* [in] The unicode device control string. */
    LPDCB          lpdcb,      /* [out] The device control block to be updated. */
    LPCOMMTIMEOUTS lptimeouts) /* [in] The COMMTIMEOUTS structure to be updated. */
{
	DCB dcb;
	COMMTIMEOUTS timeouts;
	BOOL result;
	LPCWSTR ptr = devid;

	TRACE("(%s,%p,%p)\n",debugstr_w(devid),lpdcb,lptimeouts);

	memset(&timeouts, 0, sizeof timeouts);

	/* Set DCBlength. (Windows NT does not do this, but 9x does) */
	lpdcb->DCBlength = sizeof(DCB);

	/* Make a copy of the original data structures to work with since if
	   if there is an error in the device control string the originals
	   should not be modified (except possibly DCBlength) */
	dcb = *lpdcb;
	if(lptimeouts) timeouts = *lptimeouts;

	ptr = COMM_ParseStart(ptr);

	if(ptr == NULL)
		result = FALSE;
	else if(wcschr(ptr, ','))
		result = COMM_BuildOldCommDCB(ptr, &dcb);
	else
		result = COMM_BuildNewCommDCB(ptr, &dcb, &timeouts);

	if(result)
	{
		*lpdcb = dcb;
		if(lptimeouts) *lptimeouts = timeouts;
		return TRUE;
	}
	else
	{
		WARN("Invalid device control string: %s\n", debugstr_w(devid));
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}	
}

/**************************************************************************
 *         BuildCommDCBW		(KERNEL32.@)
 *
 *  Updates a device control block structure with values from an
 *  unicode device control string.  The device control string has two forms
 *  normal and extended, it must be exclusively in one or the other form.
 *
 * RETURNS
 *
 *  True on success, false on a malformed control string.
 */
BOOL WINAPI BuildCommDCBW(
    LPCWSTR devid, /* [in] The unicode device control string. */
    LPDCB   lpdcb) /* [out] The device control block to be updated. */
{
	return BuildCommDCBAndTimeoutsW(devid,lpdcb,NULL);
}

/***********************************************************************
 * FIXME:
 * The functionality of CommConfigDialogA, GetDefaultCommConfig and
 * SetDefaultCommConfig is implemented in a DLL (usually SERIALUI.DLL).
 * This is dependent on the type of COMM port, but since it is doubtful
 * anybody will get around to implementing support for fancy serial
 * ports in WINE, this is hardcoded for the time being.  The name of
 * this DLL should be stored in and read from the system registry in
 * the hive HKEY_LOCAL_MACHINE, key
 * System\\CurrentControlSet\\Services\\Class\\Ports\\????
 * where ???? is the port number... that is determined by PNP
 * The DLL should be loaded when the COMM port is opened, and closed
 * when the COMM port is closed. - MJM 20 June 2000
 ***********************************************************************/
static const WCHAR lpszSerialUI[] = L"serialui.dll";


/***********************************************************************
 *           CommConfigDialogA   (KERNEL32.@)
 *
 * Raises a dialog that allows the user to configure a comm port.
 * Fills the COMMCONFIG struct with information specified by the user.
 * This function should call a similar routine in the COMM driver...
 *
 * RETURNS
 *
 *  TRUE on success, FALSE on failure
 *  If successful, the lpCommConfig structure will contain a new
 *  configuration for the comm port, as specified by the user.
 *
 * BUGS
 *  The library with the CommConfigDialog code is never unloaded.
 * Perhaps this should be done when the comm port is closed?
 */
BOOL WINAPI CommConfigDialogA(
    LPCSTR lpszDevice,         /* [in] name of communications device */
    HWND hWnd,                 /* [in] parent window for the dialog */
    LPCOMMCONFIG lpCommConfig) /* [out] pointer to struct to fill */
{
    LPWSTR  lpDeviceW = NULL;
    DWORD   len;
    BOOL    r;

    TRACE("(%s, %p, %p)\n", debugstr_a(lpszDevice), hWnd, lpCommConfig);

    if (lpszDevice)
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszDevice, -1, NULL, 0 );
        lpDeviceW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszDevice, -1, lpDeviceW, len );
    }
    r = CommConfigDialogW(lpDeviceW, hWnd, lpCommConfig);
    HeapFree( GetProcessHeap(), 0, lpDeviceW );
    return r;
}

/***********************************************************************
 *           CommConfigDialogW   (KERNEL32.@)
 *
 * See CommConfigDialogA.
 */
BOOL WINAPI CommConfigDialogW(
    LPCWSTR lpszDevice,        /* [in] name of communications device */
    HWND hWnd,                 /* [in] parent window for the dialog */
    LPCOMMCONFIG lpCommConfig) /* [out] pointer to struct to fill */
{
    DWORD (WINAPI *pCommConfigDialog)(LPCWSTR, HWND, LPCOMMCONFIG);
    HMODULE hConfigModule;
    DWORD   res = ERROR_INVALID_PARAMETER;

    TRACE("(%s, %p, %p)\n", debugstr_w(lpszDevice), hWnd, lpCommConfig);
    hConfigModule = LoadLibraryW(lpszSerialUI);

    if (hConfigModule) {
        pCommConfigDialog = (void *)GetProcAddress(hConfigModule, "drvCommConfigDialogW");
        if (pCommConfigDialog) {
            res = pCommConfigDialog(lpszDevice, hWnd, lpCommConfig);
        }
        FreeLibrary(hConfigModule);
    }

    if (res) SetLastError(res);
    return (res == ERROR_SUCCESS);
}

/***********************************************************************
 *           SetDefaultCommConfigW  (KERNEL32.@)
 *
 * Initializes the default configuration for a communication device.
 *
 * PARAMS
 *  lpszDevice   [I] Name of the device targeted for configuration
 *  lpCommConfig [I] PTR to a buffer with the configuration for the device
 *  dwSize       [I] Number of bytes in the buffer
 *
 * RETURNS
 *  Failure: FALSE
 *  Success: TRUE, and default configuration saved
 *
 */
BOOL WINAPI SetDefaultCommConfigW(LPCWSTR lpszDevice, LPCOMMCONFIG lpCommConfig, DWORD dwSize)
{
    BOOL (WINAPI *lpfnSetDefaultCommConfig)(LPCWSTR, LPCOMMCONFIG, DWORD);
    HMODULE hConfigModule;
    BOOL r = FALSE;

    TRACE("(%s, %p, %lu)\n", debugstr_w(lpszDevice), lpCommConfig, dwSize);

    hConfigModule = LoadLibraryW(lpszSerialUI);
    if(!hConfigModule)
        return r;

    lpfnSetDefaultCommConfig = (void *)GetProcAddress(hConfigModule, "drvSetDefaultCommConfigW");
    if (lpfnSetDefaultCommConfig)
        r = lpfnSetDefaultCommConfig(lpszDevice, lpCommConfig, dwSize);

    FreeLibrary(hConfigModule);

    return r;
}


/***********************************************************************
 *           SetDefaultCommConfigA     (KERNEL32.@)
 *
 * Initializes the default configuration for a communication device.
 *
 * See SetDefaultCommConfigW.
 *
 */
BOOL WINAPI SetDefaultCommConfigA(LPCSTR lpszDevice, LPCOMMCONFIG lpCommConfig, DWORD dwSize)
{
    BOOL r;
    LPWSTR lpDeviceW = NULL;
    DWORD len;

    TRACE("(%s, %p, %lu)\n", debugstr_a(lpszDevice), lpCommConfig, dwSize);

    if (lpszDevice)
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpszDevice, -1, NULL, 0 );
        lpDeviceW = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpszDevice, -1, lpDeviceW, len );
    }
    r = SetDefaultCommConfigW(lpDeviceW,lpCommConfig,dwSize);
    HeapFree( GetProcessHeap(), 0, lpDeviceW );
    return r;
}


/***********************************************************************
 *           GetDefaultCommConfigW   (KERNEL32.@)
 *
 *   Acquires the default configuration of the specified communication device. (unicode)
 *
 *  RETURNS
 *
 *   True on successful reading of the default configuration,
 *   if the device is not found or the buffer is too small.
 */
BOOL WINAPI GetDefaultCommConfigW(
    LPCWSTR      lpszName, /* [in] The unicode name of the device targeted for configuration. */
    LPCOMMCONFIG lpCC,     /* [out] The default configuration for the device. */
    LPDWORD      lpdwSize) /* [in/out] Initially the size of the default configuration buffer,
                              afterwards the number of bytes copied to the buffer or
                              the needed size of the buffer. */
{
    DWORD (WINAPI *pGetDefaultCommConfig)(LPCWSTR, LPCOMMCONFIG, LPDWORD);
    HMODULE hConfigModule;
    DWORD   res = ERROR_INVALID_PARAMETER;

    TRACE("(%s, %p, %p)  *lpdwSize: %lu\n", debugstr_w(lpszName), lpCC, lpdwSize, lpdwSize ? *lpdwSize : 0 );
    hConfigModule = LoadLibraryW(lpszSerialUI);

    if (hConfigModule) {
        pGetDefaultCommConfig = (void *)GetProcAddress(hConfigModule, "drvGetDefaultCommConfigW");
        if (pGetDefaultCommConfig) {
            res = pGetDefaultCommConfig(lpszName, lpCC, lpdwSize);
        }
        FreeLibrary(hConfigModule);
    }

    if (res) SetLastError(res);
    return (res == ERROR_SUCCESS);
}

/**************************************************************************
 *         GetDefaultCommConfigA		(KERNEL32.@)
 *
 *   Acquires the default configuration of the specified communication device.
 *
 *  RETURNS
 *
 *   True on successful reading of the default configuration,
 *   if the device is not found or the buffer is too small.
 */
BOOL WINAPI GetDefaultCommConfigA(
    LPCSTR       lpszName, /* [in] The ANSI name of the device targeted for configuration. */
    LPCOMMCONFIG lpCC,     /* [out] The default configuration for the device. */
    LPDWORD      lpdwSize) /* [in/out] Initially the size of the default configuration buffer,
			      afterwards the number of bytes copied to the buffer or
                              the needed size of the buffer. */
{
	BOOL ret = FALSE;
	UNICODE_STRING lpszNameW;

	TRACE("(%s, %p, %p)  *lpdwSize: %lu\n", debugstr_a(lpszName), lpCC, lpdwSize, lpdwSize ? *lpdwSize : 0 );
	if(lpszName) RtlCreateUnicodeStringFromAsciiz(&lpszNameW,lpszName);
	else lpszNameW.Buffer = NULL;

	ret = GetDefaultCommConfigW(lpszNameW.Buffer,lpCC,lpdwSize);

	RtlFreeUnicodeString(&lpszNameW);
	return ret;
}
