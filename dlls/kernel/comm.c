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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#include <errno.h>
#include <ctype.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#ifdef HAVE_SYS_MODEM_H
# include <sys/modem.h>
#endif
#ifdef HAVE_SYS_STRTIO_H
# include <sys/strtio.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winioctl.h"
#include "ddk/ntddser.h"

#include "wine/server.h"
#include "wine/unicode.h"

#include "wine/debug.h"

#ifdef HAVE_LINUX_SERIAL_H
#include <linux/serial.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(comm);

/* retrieve the Unix handle corresponding to a comm handle */
static int get_comm_fd( HANDLE handle, DWORD access )
{
    int fd, ret;

    ret = wine_server_handle_to_fd( handle, access, &fd, NULL );
    if (ret) SetLastError( RtlNtStatusToDosError(ret) );
    return fd;
}

/* release the Unix handle returned by get_comm_fd */
static inline void release_comm_fd( HANDLE handle, int fd )
{
    wine_server_release_fd( handle, fd );
}

/*             serial_irq_info
 * local structure holding the irq values we need for WaitCommEvent()
 *
 * Stripped down from struct serial_icounter_struct, which may not be available on some systems
 * As the modem line interrupts (cts, dsr, rng, dcd) only get updated with TIOCMIWAIT active,
 * no need to carry them in the internal structure
 *
 */
typedef struct serial_irq_info
{
    int rx , tx, frame, overrun, parity, brk, buf_overrun;
}serial_irq_info;

/***********************************************************************
 * Data needed by the thread polling for the changing CommEvent
 */
typedef struct async_commio
{
    HANDLE              handle;
    char*               buffer;
    HANDLE              hEvent;
    DWORD               evtmask;
    DWORD               mstat;
    serial_irq_info     irq_info;
} async_commio;

/***********************************************************************/

#if !defined(TIOCINQ) && defined(FIONREAD)
#define	TIOCINQ FIONREAD
#endif

/***********************************************************************
 *   Get extended interrupt count info, needed for WaitCommEvent
 */
static int COMM_GetEInfo(int fd, serial_irq_info *irq_info)
{
#ifdef TIOCGICOUNT
    struct serial_icounter_struct einfo;
    if (!ioctl(fd,TIOCGICOUNT, &einfo))
    {
        irq_info->rx          = einfo.rx;
        irq_info->tx          = einfo.tx;
        irq_info->frame       = einfo.frame;
        irq_info->overrun     = einfo.overrun;
        irq_info->parity      = einfo.parity;
        irq_info->brk         = einfo.brk;
        irq_info->buf_overrun = einfo.buf_overrun;
        return 0;
    }
#endif
    memset(irq_info,0, sizeof(serial_irq_info));
    return -1;
}

static int COMM_WhackModem(int fd, unsigned int andy, unsigned int orrie)
{
#ifdef TIOCMGET
    unsigned int mstat, okay;
    okay = ioctl(fd, TIOCMGET, &mstat);
    if (okay) return okay;
    if (andy) mstat &= andy;
    mstat |= orrie;
    return ioctl(fd, TIOCMSET, &mstat);
#else
    return 0;
#endif
}

/***********************************************************************
 *           COMM_Parse*   (Internal)
 *
 *  The following COMM_Parse* functions are used by the BuildCommDCB
 *  functions to help parse the various parts of the device control string.
 */
static LPCWSTR COMM_ParseStart(LPCWSTR ptr)
{
	static const WCHAR comW[] = {'C','O','M',0};

	/* The device control string may optionally start with "COMx" followed
	   by an optional ':' and spaces. */
	if(!strncmpiW(ptr, comW, 3))
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
	*lpnumber = strtoulW(ptr, NULL, 10);
	while(*ptr >= '0' && *ptr <= '9') ptr++;
	return ptr;
}

static LPCWSTR COMM_ParseParity(LPCWSTR ptr, LPBYTE lpparity)
{
	/* Contrary to what you might expect, Windows only sets the Parity
	   member of DCB and not fParity even when parity is specified in the
	   device control string */

	switch(toupperW(*ptr++))
	{
	case 'E':
		*lpparity = EVENPARITY;
		break;
	case 'M':
		*lpparity = MARKPARITY;
		break;
	case 'N':
		*lpparity = NOPARITY;
		break;
	case 'O':
		*lpparity = ODDPARITY;
		break;
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
	static const WCHAR stopbits15W[] = {'1','.','5',0};

	if(!strncmpW(stopbits15W, ptr, 3))
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
	static const WCHAR onW[] = {'o','n',0};
	static const WCHAR offW[] = {'o','f','f',0};

	if(!strncmpiW(onW, ptr, 2))
	{
		ptr += 2;
		*lponoff = 1;
	}
	else if(!strncmpiW(offW, ptr, 3))
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
		if(*device) last = toupperW(*device++);
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
	case 'X':
		lpdcb->fInX = TRUE;
		lpdcb->fOutX = TRUE;
		lpdcb->fOutxCtsFlow = FALSE;
		lpdcb->fOutxDsrFlow = FALSE;
		lpdcb->fDtrControl = DTR_CONTROL_ENABLE;
		lpdcb->fRtsControl = RTS_CONTROL_ENABLE;
		break;
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
	static const WCHAR baudW[] = {'b','a','u','d','=',0};
	static const WCHAR parityW[] = {'p','a','r','i','t','y','=',0};
	static const WCHAR dataW[] = {'d','a','t','a','=',0};
	static const WCHAR stopW[] = {'s','t','o','p','=',0};
	static const WCHAR toW[] = {'t','o','=',0};
	static const WCHAR xonW[] = {'x','o','n','=',0};
	static const WCHAR odsrW[] = {'o','d','s','r','=',0};
	static const WCHAR octsW[] = {'o','c','t','s','=',0};
	static const WCHAR dtrW[] = {'d','t','r','=',0};
	static const WCHAR rtsW[] = {'r','t','s','=',0};
	static const WCHAR idsrW[] = {'i','d','s','r','=',0};

	while(*device)
	{
		while(*device == ' ') device++;

		if(!strncmpiW(baudW, device, 5))
		{
			baud = TRUE;
			
			if(!(device = COMM_ParseNumber(device + 5, &lpdcb->BaudRate)))
				return FALSE;
		}
		else if(!strncmpiW(parityW, device, 7))
		{
			if(!(device = COMM_ParseParity(device + 7, &lpdcb->Parity)))
				return FALSE;
		}
		else if(!strncmpiW(dataW, device, 5))
		{
			if(!(device = COMM_ParseByteSize(device + 5, &lpdcb->ByteSize)))
				return FALSE;
		}
		else if(!strncmpiW(stopW, device, 5))
		{
			stop = TRUE;
			
			if(!(device = COMM_ParseStopBits(device + 5, &lpdcb->StopBits)))
				return FALSE;
		}
		else if(!strncmpiW(toW, device, 3))
		{
			if(!(device = COMM_ParseOnOff(device + 3, &temp)))
				return FALSE;

			lptimeouts->ReadIntervalTimeout = 0;
			lptimeouts->ReadTotalTimeoutMultiplier = 0;
			lptimeouts->ReadTotalTimeoutConstant = 0;
			lptimeouts->WriteTotalTimeoutMultiplier = 0;
			lptimeouts->WriteTotalTimeoutConstant = temp ? 60000 : 0;
		}
		else if(!strncmpiW(xonW, device, 4))
		{
			if(!(device = COMM_ParseOnOff(device + 4, &temp)))
				return FALSE;

			lpdcb->fOutX = temp;
			lpdcb->fInX = temp;
		}
		else if(!strncmpiW(odsrW, device, 5))
		{
			if(!(device = COMM_ParseOnOff(device + 5, &temp)))
				return FALSE;

			lpdcb->fOutxDsrFlow = temp;
		}
		else if(!strncmpiW(octsW, device, 5))
		{
			if(!(device = COMM_ParseOnOff(device + 5, &temp)))
				return FALSE;

			lpdcb->fOutxCtsFlow = temp;
		}
		else if(!strncmpiW(dtrW, device, 4))
		{
			if(!(device = COMM_ParseOnOff(device + 4, &temp)))
				return FALSE;

			lpdcb->fDtrControl = temp;
		}
		else if(!strncmpiW(rtsW, device, 4))
		{
			if(!(device = COMM_ParseOnOff(device + 4, &temp)))
				return FALSE;

			lpdcb->fRtsControl = temp;
		}
		else if(!strncmpiW(idsrW, device, 5))
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
 *  ascii device control string.  The device control string has two forms
 *  normal and extended, it must be exclusively in one or the other form.
 *
 * RETURNS
 *
 *  True on success, false on a malformed control string.
 */
BOOL WINAPI BuildCommDCBA(
    LPCSTR device, /* [in] The ascii device control string used to update the DCB. */
    LPDCB  lpdcb)  /* [out] The device control block to be updated. */
{
	return BuildCommDCBAndTimeoutsA(device,lpdcb,NULL);
}

/**************************************************************************
 *         BuildCommDCBAndTimeoutsA		(KERNEL32.@)
 *
 *  Updates a device control block data structure with values from an
 *  ascii device control string.  Taking timeout values from a timeouts
 *  struct if desired by the control string.
 *
 * RETURNS
 *
 *  True on success, false bad handles etc.
 */
BOOL WINAPI BuildCommDCBAndTimeoutsA(
    LPCSTR         device,     /* [in] The ascii device control string. */
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

	/* Set DCBlength. (Windows NT does not do this, but 9x does) */
	lpdcb->DCBlength = sizeof(DCB);

	/* Make a copy of the original data structures to work with since if
	   if there is an error in the device control string the originals
	   should not be modified (except possibly DCBlength) */
	memcpy(&dcb, lpdcb, sizeof(DCB));
	if(lptimeouts) memcpy(&timeouts, lptimeouts, sizeof(COMMTIMEOUTS));

	ptr = COMM_ParseStart(ptr);

	if(ptr == NULL)
		result = FALSE;
	else if(strchrW(ptr, ','))
		result = COMM_BuildOldCommDCB(ptr, &dcb);
	else
		result = COMM_BuildNewCommDCB(ptr, &dcb, &timeouts);

	if(result)
	{
		memcpy(lpdcb, &dcb, sizeof(DCB));
		if(lptimeouts) memcpy(lptimeouts, &timeouts, sizeof(COMMTIMEOUTS));
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

/*****************************************************************************
 *	SetCommBreak		(KERNEL32.@)
 *
 *  Halts the transmission of characters to a communications device.
 *
 * PARAMS
 *      handle  [in] The communications device to suspend
 *
 * RETURNS
 *
 *  True on success, and false if the communications device could not be found,
 *  the control is not supported.
 *
 * BUGS
 *
 *  Only TIOCSBRK and TIOCCBRK are supported.
 */
BOOL WINAPI SetCommBreak(HANDLE handle)
{
    return DeviceIoControl(handle, IOCTL_SERIAL_SET_BREAK_ON, NULL, 0, NULL, 0, NULL, NULL);
}

/*****************************************************************************
 *	ClearCommBreak		(KERNEL32.@)
 *
 *  Resumes character transmission from a communication device.
 *
 * PARAMS
 *
 *      handle [in] The halted communication device whose character transmission is to be resumed
 *
 * RETURNS
 *
 *  True on success and false if the communications device could not be found.
 *
 * BUGS
 *
 *  Only TIOCSBRK and TIOCCBRK are supported.
 */
BOOL WINAPI ClearCommBreak(HANDLE handle)
{
    return DeviceIoControl(handle, IOCTL_SERIAL_SET_BREAK_OFF, NULL, 0, NULL, 0, NULL, NULL);
}

/*****************************************************************************
 *	EscapeCommFunction	(KERNEL32.@)
 *
 *  Directs a communication device to perform an extended function.
 *
 * RETURNS
 *
 *  True or requested data on successful completion of the command,
 *  false if the device is not present cannot execute the command
 *  or the command failed.
 */
BOOL WINAPI EscapeCommFunction(
    HANDLE handle,    /* [in] The communication device to perform the extended function. */
    UINT   nFunction) /* [in] The extended function to be performed. */
{
	int fd,direct=FALSE,result=FALSE;
	struct termios	port;

    	TRACE("handle %p, function=%d\n", handle, nFunction);
	fd = get_comm_fd( handle, FILE_READ_DATA );
	if(fd<0) return FALSE;

	if (tcgetattr(fd,&port) == -1) {
		release_comm_fd( handle, fd );
		return FALSE;
	}

	switch (nFunction) {
		case RESETDEV:
		        TRACE("\n");
			break;

		case CLRDTR:
		        TRACE("CLRDTR\n");
#ifdef TIOCM_DTR
			direct=TRUE;
			result= COMM_WhackModem(fd, ~TIOCM_DTR, 0);
			break;
#endif

		case CLRRTS:
		        TRACE("CLRRTS\n");
#ifdef TIOCM_RTS
			direct=TRUE;
			result= COMM_WhackModem(fd, ~TIOCM_RTS, 0);
			break;
#endif

		case SETDTR:
		        TRACE("SETDTR\n");
#ifdef TIOCM_DTR
			direct=TRUE;
			result= COMM_WhackModem(fd, 0, TIOCM_DTR);
			break;
#endif

		case SETRTS:
		        TRACE("SETRTS\n");
#ifdef TIOCM_RTS
			direct=TRUE;
			result= COMM_WhackModem(fd, 0, TIOCM_RTS);
			break;
#endif

		case SETXOFF:
		        TRACE("SETXOFF\n");
			port.c_iflag |= IXOFF;
			break;

		case SETXON:
		        TRACE("SETXON\n");
			port.c_iflag |= IXON;
			break;
		case SETBREAK:
			TRACE("setbreak\n");
#ifdef 	TIOCSBRK
			direct=TRUE;
			result = ioctl(fd,TIOCSBRK,0);
			break;
#endif
		case CLRBREAK:
			TRACE("clrbreak\n");
#ifdef 	TIOCSBRK
			direct=TRUE;
			result = ioctl(fd,TIOCCBRK,0);
			break;
#endif
		default:
			WARN("(handle=%p,nFunction=%d): Unknown function\n",
			handle, nFunction);
			break;
	}

	if (!direct)
	  if (tcsetattr(fd, TCSADRAIN, &port) == -1) {
		release_comm_fd( handle, fd );
		return FALSE;
	  } else
	        result= TRUE;
	else
	  {
	    if (result == -1)
	      {
		result= FALSE;
	      }
	    else
	      result = TRUE;
	  }
	release_comm_fd( handle, fd );
	return result;
}

/********************************************************************
 *      PurgeComm        (KERNEL32.@)
 *
 *  Terminates pending operations and/or discards buffers on a
 *  communication resource.
 *
 * PARAMS
 *
 *      handle  [in] The communication resource to be purged
 *      flags   [in] Flags for clear pending/buffer on input/output
 *
 * RETURNS
 *
 *  True on success and false if the communications handle is bad.
 */
BOOL WINAPI PurgeComm(HANDLE handle, DWORD flags)
{
    return DeviceIoControl(handle, IOCTL_SERIAL_PURGE, &flags, sizeof(flags),
                           NULL, 0, NULL, NULL);
}

/*****************************************************************************
 *	ClearCommError	(KERNEL32.@)
 *
 *  Enables further I/O operations on a communications resource after
 *  supplying error and current status information.
 *
 * PARAMS
 *
 *      handle  [in]    The communication resource with the error
 *      errors  [out]   Flags indicating error the resource experienced
 *      lpStat  [out] The status of the communication resource
 * RETURNS
 *
 *  True on success, false if the communication resource handle is bad.
 */
BOOL WINAPI ClearCommError(HANDLE handle, LPDWORD errors, LPCOMSTAT lpStat)
{
    SERIAL_STATUS       ss;

    if (!DeviceIoControl(handle, IOCTL_SERIAL_GET_COMMSTATUS, NULL, 0,
                         &ss, sizeof(ss), NULL, NULL))
        return FALSE;

    if (errors)
    {
        *errors = 0;
        if (ss.Errors & SERIAL_ERROR_BREAK)             *errors |= CE_BREAK;
        if (ss.Errors & SERIAL_ERROR_FRAMING)           *errors |= CE_FRAME;
        if (ss.Errors & SERIAL_ERROR_OVERRUN)           *errors |= CE_OVERRUN;
        if (ss.Errors & SERIAL_ERROR_QUEUEOVERRUN)      *errors |= CE_RXOVER;
        if (ss.Errors & SERIAL_ERROR_PARITY)            *errors |= CE_RXPARITY;
    }
 
    if (lpStat)
    {
        memset(lpStat, 0, sizeof(*lpStat));

        if (ss.HoldReasons & SERIAL_TX_WAITING_FOR_CTS)         lpStat->fCtsHold = TRUE;
        if (ss.HoldReasons & SERIAL_TX_WAITING_FOR_DSR)         lpStat->fDsrHold = TRUE;
        if (ss.HoldReasons & SERIAL_TX_WAITING_FOR_DCD)         lpStat->fRlsdHold = TRUE;
        if (ss.HoldReasons & SERIAL_TX_WAITING_FOR_XON)         lpStat->fXoffHold = TRUE;
        if (ss.HoldReasons & SERIAL_TX_WAITING_XOFF_SENT)       lpStat->fXoffSent = TRUE;
        if (ss.EofReceived)                                     lpStat->fEof = TRUE;
        if (ss.WaitForImmediate)                                lpStat->fTxim = TRUE;
        lpStat->cbInQue = ss.AmountInInQueue;
        lpStat->cbOutQue = ss.AmountInOutQueue;
    }
    return TRUE;
}

/*****************************************************************************
 *      SetupComm       (KERNEL32.@)
 *
 *  Called after CreateFile to hint to the communication resource to use
 *  specified sizes for input and output buffers rather than the default values.
 *
 * RETURNS
 *
 *  True if successful, false if the communications resource handle is bad.
 *
 * BUGS
 *
 *  Stub.
 */
BOOL WINAPI SetupComm(
    HANDLE handle,  /* [in] The just created communication resource handle. */
    DWORD  insize,  /* [in] The suggested size of the communication resources input buffer in bytes. */
    DWORD  outsize) /* [in] The suggested size of the communication resources output buffer in bytes. */
{
    int fd;

    FIXME("insize %ld outsize %ld unimplemented stub\n", insize, outsize);
    fd=get_comm_fd( handle, FILE_READ_DATA );
    if(0>fd) return FALSE;
    release_comm_fd( handle, fd );
    return TRUE;
}

/*****************************************************************************
 *	GetCommMask	(KERNEL32.@)
 *
 *  Obtain the events associated with a communication device that will cause
 *  a call WaitCommEvent to return.
 *
 *  PARAMS
 *
 *      handle  [in]    The communications device
 *      evtmask [out]   The events which cause WaitCommEvent to return
 *
 *  RETURNS
 *
 *   True on success, fail on bad device handle etc.
 */
BOOL WINAPI GetCommMask(HANDLE handle, LPDWORD evtmask)
{
    TRACE("handle %p, mask %p\n", handle, evtmask);
    return DeviceIoControl(handle, IOCTL_SERIAL_GET_WAIT_MASK,
                           NULL, 0, evtmask, sizeof(*evtmask), NULL, NULL);
}

/*****************************************************************************
 *	SetCommMask	(KERNEL32.@)
 *
 *  There be some things we need to hear about yon there communications device.
 *  (Set which events associated with a communication device should cause
 *  a call WaitCommEvent to return.)
 *
 * PARAMS
 *
 *      handle  [in]    The communications device
 *      evtmask [in]    The events that are to be monitored
 *
 * RETURNS
 *
 *  True on success, false on bad handle etc.
 */
BOOL WINAPI SetCommMask(HANDLE handle, DWORD evtmask)
{
    TRACE("handle %p, mask %lx\n", handle, evtmask);
    return DeviceIoControl(handle, IOCTL_SERIAL_SET_WAIT_MASK,
                           &evtmask, sizeof(evtmask), NULL, 0, NULL, NULL);
}

static void dump_dcb(const DCB* lpdcb)
{
    TRACE("bytesize=%d baudrate=%ld fParity=%d Parity=%d stopbits=%d\n",
          lpdcb->ByteSize, lpdcb->BaudRate, lpdcb->fParity, lpdcb->Parity,
          (lpdcb->StopBits == ONESTOPBIT) ? 1 :
          (lpdcb->StopBits == TWOSTOPBITS) ? 2 : 0);
    TRACE("%sIXON %sIXOFF\n", (lpdcb->fInX) ? "" : "~", (lpdcb->fOutX) ? "" : "~");
    TRACE("fOutxCtsFlow=%d fRtsControl=%d\n", lpdcb->fOutxCtsFlow, lpdcb->fRtsControl);
    TRACE("fOutxDsrFlow=%d fDtrControl=%d\n", lpdcb->fOutxDsrFlow, lpdcb->fDtrControl);
    if (lpdcb->fOutxCtsFlow || lpdcb->fRtsControl == RTS_CONTROL_HANDSHAKE)
        TRACE("CRTSCTS\n");
    else
        TRACE("~CRTSCTS\n");
}

/*****************************************************************************
 *	SetCommState    (KERNEL32.@)
 *
 *  Re-initializes all hardware and control settings of a communications device,
 *  with values from a device control block without effecting the input and output
 *  queues.
 *
 * PARAMS
 *
 *      handle  [in]    The communications device
 *      lpdcb   [out]   The device control block
 *
 * RETURNS
 *
 *  True on success, false on failure eg if the XonChar is equal to the XoffChar.
 */
BOOL WINAPI SetCommState( HANDLE handle, LPDCB lpdcb)
{
     struct termios port;
     int fd;
     BOOL ret;

    SERIAL_BAUD_RATE           sbr;
    SERIAL_LINE_CONTROL        slc;
    SERIAL_HANDFLOW            shf;

    if (lpdcb == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    dump_dcb(lpdcb);

    sbr.BaudRate = lpdcb->BaudRate;
             
    slc.StopBits = lpdcb->StopBits;
    slc.Parity = lpdcb->Parity;
    slc.WordLength = lpdcb->ByteSize;

    shf.ControlHandShake = 0;
    shf.FlowReplace = 0;
    if (lpdcb->fOutxCtsFlow)      shf.ControlHandShake |= SERIAL_CTS_HANDSHAKE;
    if (lpdcb->fOutxDsrFlow)      shf.ControlHandShake |= SERIAL_DSR_HANDSHAKE;
    switch (lpdcb->fDtrControl)
    {
    case DTR_CONTROL_DISABLE:                                                  break;
    case DTR_CONTROL_ENABLE:      shf.ControlHandShake |= SERIAL_DTR_CONTROL;  break;
    case DTR_CONTROL_HANDSHAKE:   shf.ControlHandShake |= SERIAL_DTR_HANDSHAKE;break;
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    switch (lpdcb->fDtrControl)
    {
    case RTS_CONTROL_DISABLE:                                                  break;
    case RTS_CONTROL_ENABLE:      shf.FlowReplace |= SERIAL_RTS_CONTROL;       break;
    case RTS_CONTROL_HANDSHAKE:   shf.FlowReplace |= SERIAL_RTS_HANDSHAKE;     break;
    case RTS_CONTROL_TOGGLE:      shf.FlowReplace |= SERIAL_RTS_CONTROL | 
                                                     SERIAL_RTS_HANDSHAKE;     break;
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (lpdcb->fDsrSensitivity)   shf.ControlHandShake |= SERIAL_DSR_SENSITIVITY;
    if (lpdcb->fAbortOnError)     shf.ControlHandShake |= SERIAL_ERROR_ABORT;

    if (lpdcb->fErrorChar)        shf.FlowReplace |= SERIAL_ERROR_CHAR;
    if (lpdcb->fNull)             shf.FlowReplace |= SERIAL_NULL_STRIPPING;
    if (lpdcb->fTXContinueOnXoff) shf.FlowReplace |= SERIAL_XOFF_CONTINUE;
    if (lpdcb->fOutX)             shf.FlowReplace |= SERIAL_AUTO_TRANSMIT;
    if (lpdcb->fInX)              shf.FlowReplace |= SERIAL_AUTO_RECEIVE;

    shf.XonLimit = lpdcb->XonLim;
    shf.XoffLimit = lpdcb->XoffLim;

    /* note: change DTR/RTS lines after setting the comm attributes,
     * so flow control does not interfere.
     */
    if (!DeviceIoControl(handle, IOCTL_SERIAL_SET_BAUD_RATE,
                         &sbr, sizeof(sbr), NULL, 0, NULL, NULL))
        return FALSE;
    if (!DeviceIoControl(handle, IOCTL_SERIAL_SET_LINE_CONTROL,
                         &slc, sizeof(slc), NULL, 0, NULL, NULL))
        return FALSE;

    if (!DeviceIoControl(handle, IOCTL_SERIAL_SET_HANDFLOW,
                         &shf, sizeof(shf), NULL, 0, NULL, NULL))
        return FALSE;

     fd = get_comm_fd( handle, FILE_READ_DATA );
     if (fd < 0) return FALSE;

     if ((tcgetattr(fd,&port)) == -1) {
         int save_error = errno;
         release_comm_fd( handle, fd );
         ERR("tcgetattr error '%s'\n", strerror(save_error));
         return FALSE;
     }

	port.c_cc[VMIN] = 0;
	port.c_cc[VTIME] = 1;

	if (tcsetattr(fd,TCSANOW,&port)==-1) { /* otherwise it hangs with pending input*/
                ERR("tcsetattr error '%s'\n", strerror(errno));
		ret = FALSE;
	} else {
                ClearCommError(handle, NULL, NULL);
		ret = TRUE;
	}

        release_comm_fd( handle, fd );
        return ret;

}


/*****************************************************************************
 *	GetCommState	(KERNEL32.@)
 *
 *  Fills in a device control block with information from a communications device.
 *
 * RETURNS
 *
 *  True on success, false if the communication device handle is bad etc
 *
 * BUGS
 *
 *  XonChar and XoffChar are not set.
 */
BOOL WINAPI GetCommState(
    HANDLE handle, /* [in] The communications device. */
    LPDCB  lpdcb)  /* [out] The device control block. */
{
     struct termios port;
     int fd,speed;
     int stat = DTR_CONTROL_ENABLE | RTS_CONTROL_ENABLE;

     TRACE("handle %p, ptr %p\n", handle, lpdcb);

     fd = get_comm_fd( handle, FILE_READ_DATA );
     if (fd < 0) return FALSE;
     if (tcgetattr(fd, &port) == -1) {
                int save_error=errno;
                ERR("tcgetattr error '%s'\n", strerror(save_error));
                release_comm_fd( handle, fd );
		return FALSE;
	}
	
#ifdef TIOCMGET
     if (ioctl(fd, TIOCMGET, &stat) == -1)
     {
          int save_error=errno;
          WARN("ioctl error '%s'\n", strerror(save_error));
          stat = DTR_CONTROL_ENABLE | RTS_CONTROL_ENABLE;
     }
#endif
     release_comm_fd( handle, fd );
#ifndef __EMX__
#ifdef CBAUD
     speed= (port.c_cflag & CBAUD);
#else
     speed= (cfgetospeed(&port));
#endif
     switch (speed) {
		case B0:
			lpdcb->BaudRate = 0;
			break;
		case B50:
			lpdcb->BaudRate = 50;
			break;
		case B75:
			lpdcb->BaudRate = 75;
			break;
		case B110:
			lpdcb->BaudRate = 110;
			break;
		case B134:
			lpdcb->BaudRate = 134;
			break;
		case B150:
			lpdcb->BaudRate = 150;
			break;
		case B200:
			lpdcb->BaudRate = 200;
			break;
		case B300:
			lpdcb->BaudRate = 300;
			break;
		case B600:
			lpdcb->BaudRate = 600;
			break;
		case B1200:
			lpdcb->BaudRate = 1200;
			break;
		case B1800:
			lpdcb->BaudRate = 1800;
			break;
		case B2400:
			lpdcb->BaudRate = 2400;
			break;
		case B4800:
			lpdcb->BaudRate = 4800;
			break;
		case B9600:
			lpdcb->BaudRate = 9600;
			break;
		case B19200:
			lpdcb->BaudRate = 19200;
			break;
		case B38400:
			lpdcb->BaudRate = 38400;
			break;
#ifdef B57600
		case B57600:
			lpdcb->BaudRate = 57600;
			break;
#endif
#ifdef B115200
		case B115200:
			lpdcb->BaudRate = 115200;
			break;
#endif
#ifdef B230400
                case B230400:
			lpdcb->BaudRate = 230400;
			break;
#endif
#ifdef B460800
                case B460800:
			lpdcb->BaudRate = 460800;
			break;
#endif
	        default:
		        ERR("unknown speed %x\n", speed);
	}
#endif
	switch (port.c_cflag & CSIZE) {
		case CS5:
			lpdcb->ByteSize = 5;
			break;
		case CS6:
			lpdcb->ByteSize = 6;
			break;
		case CS7:
			lpdcb->ByteSize = 7;
			break;
		case CS8:
			lpdcb->ByteSize = 8;
			break;
	        default:
		        ERR("unknown size %x\n", port.c_cflag & CSIZE);
	}

        if(port.c_iflag & INPCK)
            lpdcb->fParity = TRUE;
        else
            lpdcb->fParity = FALSE;
#ifdef CMSPAR
	switch (port.c_cflag & (PARENB | PARODD | CMSPAR))
#else
	switch (port.c_cflag & (PARENB | PARODD))
#endif
	{
		case 0:
			lpdcb->Parity = NOPARITY;
			break;
		case PARENB:
			lpdcb->Parity = EVENPARITY;
			break;
		case (PARENB | PARODD):
			lpdcb->Parity = ODDPARITY;
			break;
#ifdef CMSPAR
		case (PARENB | CMSPAR):
			lpdcb->Parity = MARKPARITY;
			break;
                case (PARENB | PARODD | CMSPAR):
			lpdcb->Parity = SPACEPARITY;
			break;
#endif
	}

	if (port.c_cflag & CSTOPB)
            if(lpdcb->ByteSize == 5)
                lpdcb->StopBits = ONE5STOPBITS;
            else
                lpdcb->StopBits = TWOSTOPBITS;
	else
            lpdcb->StopBits = ONESTOPBIT;

	lpdcb->fNull = 0;
	lpdcb->fBinary = 1;

	/* termios does not support DTR/DSR flow control */
	lpdcb->fOutxDsrFlow = 0;
	lpdcb->fDtrControl =
#ifdef TIOCM_DTR
            !(stat & TIOCM_DTR) ?  DTR_CONTROL_DISABLE:
#endif
                DTR_CONTROL_ENABLE  ;

#ifdef CRTSCTS

	if (port.c_cflag & CRTSCTS) {
		lpdcb->fRtsControl = RTS_CONTROL_HANDSHAKE;
		lpdcb->fOutxCtsFlow = 1;
	} else
#endif
	{
		lpdcb->fRtsControl = 
#ifdef TIOCM_RTS
                    !(stat & TIOCM_RTS) ?  RTS_CONTROL_DISABLE :
#endif
                    RTS_CONTROL_ENABLE ;
		lpdcb->fOutxCtsFlow = 0;
	}
	if (port.c_iflag & IXON)
		lpdcb->fInX = 1;
	else
		lpdcb->fInX = 0;

	if (port.c_iflag & IXOFF)
		lpdcb->fOutX = 1;
	else
		lpdcb->fOutX = 0;
/*
	lpdcb->XonChar =
	lpdcb->XoffChar =
 */
	lpdcb->XonLim = 10;
	lpdcb->XoffLim = 10;

        TRACE("OK\n");

	TRACE("bytesize %d baudrate %ld fParity %d Parity %d stopbits %d\n",
	      lpdcb->ByteSize,lpdcb->BaudRate,lpdcb->fParity, lpdcb->Parity,
	      (lpdcb->StopBits == ONESTOPBIT)?1:
	      (lpdcb->StopBits == TWOSTOPBITS)?2:0);
	TRACE("%s %s\n",(lpdcb->fInX)?"IXON":"~IXON",
	      (lpdcb->fOutX)?"IXOFF":"~IXOFF");
         TRACE("fOutxCtsFlow %d fRtsControl %d\n", lpdcb->fOutxCtsFlow,
                 lpdcb->fRtsControl);
         TRACE("fOutxDsrFlow %d fDtrControl%d\n", lpdcb->fOutxDsrFlow,
                 lpdcb->fDtrControl);
#ifdef CRTSCTS
	if (	lpdcb->fOutxCtsFlow 			||
		lpdcb->fRtsControl == RTS_CONTROL_HANDSHAKE
		)
	  TRACE("CRTSCTS\n");
        else

	  TRACE("~CRTSCTS\n");
#endif
	return TRUE;
}

/*****************************************************************************
 *	TransmitCommChar	(KERNEL32.@)
 *
 *  Transmits a single character in front of any pending characters in the
 *  output buffer.  Usually used to send an interrupt character to a host.
 *
 * PARAMS
 *      hComm           [in]    The communication device in need of a command character
 *      chTransmit      [in]    The character to transmit
 *
 * RETURNS
 *
 *  True if the call succeeded, false if the previous command character to the
 *  same device has not been sent yet the handle is bad etc.
 *
 */
BOOL WINAPI TransmitCommChar(HANDLE hComm, CHAR chTransmit)
{
    return DeviceIoControl(hComm, IOCTL_SERIAL_IMMEDIATE_CHAR,
                           &chTransmit, sizeof(chTransmit), NULL, 0, NULL, NULL);
}


/*****************************************************************************
 *	GetCommTimeouts		(KERNEL32.@)
 *
 *  Obtains the request timeout values for the communications device.
 *
 * RETURNS
 *
 *  True on success, false if communications device handle is bad
 *  or the target structure is null.
 */
BOOL WINAPI GetCommTimeouts(
    HANDLE         hComm,      /* [in] The communications device. */
    LPCOMMTIMEOUTS lptimeouts) /* [out] The struct of request timeouts. */
{
    BOOL ret;

    TRACE("(%p,%p)\n",hComm,lptimeouts);

    if(!lptimeouts)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SERVER_START_REQ( get_serial_info )
    {
        req->handle = hComm;
        if ((ret = !wine_server_call_err( req )))
        {
            lptimeouts->ReadIntervalTimeout         = reply->readinterval;
            lptimeouts->ReadTotalTimeoutMultiplier  = reply->readmult;
            lptimeouts->ReadTotalTimeoutConstant    = reply->readconst;
            lptimeouts->WriteTotalTimeoutMultiplier = reply->writemult;
            lptimeouts->WriteTotalTimeoutConstant   = reply->writeconst;
        }
    }
    SERVER_END_REQ;
    return ret;
}

/*****************************************************************************
 *	SetCommTimeouts		(KERNEL32.@)
 *
 * Sets the timeouts used when reading and writing data to/from COMM ports.
 *
 * ReadIntervalTimeout
 *     - converted and passes to linux kernel as c_cc[VTIME]
 * ReadTotalTimeoutMultiplier, ReadTotalTimeoutConstant
 *     - used in ReadFile to calculate GetOverlappedResult's timeout
 * WriteTotalTimeoutMultiplier, WriteTotalTimeoutConstant
 *     - used in WriteFile to calculate GetOverlappedResult's timeout
 *
 * RETURNS
 *
 *  True if the timeouts were set, false otherwise.
 */
BOOL WINAPI SetCommTimeouts(
    HANDLE hComm,              /* [in] handle of COMM device */
    LPCOMMTIMEOUTS lptimeouts) /* [in] pointer to COMMTIMEOUTS structure */
{
    BOOL ret;
    int fd;
    struct termios tios;

    TRACE("(%p,%p)\n",hComm,lptimeouts);

    if(!lptimeouts)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SERVER_START_REQ( set_serial_info )
    {
        req->handle       = hComm;
        req->flags        = SERIALINFO_SET_TIMEOUTS;
        req->readinterval = lptimeouts->ReadIntervalTimeout ;
        req->readmult     = lptimeouts->ReadTotalTimeoutMultiplier ;
        req->readconst    = lptimeouts->ReadTotalTimeoutConstant ;
        req->writemult    = lptimeouts->WriteTotalTimeoutMultiplier ;
        req->writeconst   = lptimeouts->WriteTotalTimeoutConstant ;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    if (!ret) return FALSE;

    /* FIXME: move this stuff to the server */
    fd = get_comm_fd( hComm, FILE_READ_DATA );
    if (fd < 0) return FALSE;

    if (-1==tcgetattr(fd,&tios)) {
        FIXME("tcgetattr on fd %d failed!\n",fd);
        release_comm_fd( hComm, fd );
        return FALSE;
    }

    /* VTIME is in 1/10 seconds */
	{
		unsigned int ux_timeout;

		if(lptimeouts->ReadIntervalTimeout == 0) /* 0 means no timeout */
		{
			ux_timeout = 0;
		}
		else
		{
			ux_timeout = (lptimeouts->ReadIntervalTimeout+99)/100;
			if(ux_timeout == 0)
			{
				ux_timeout = 1; /* must be at least some timeout */
			}
		}
		tios.c_cc[VTIME] = ux_timeout;
	}

    if (-1==tcsetattr(fd,0,&tios)) {
        FIXME("tcsetattr on fd %d failed!\n",fd);
        release_comm_fd( hComm, fd );
        return FALSE;
    }
    release_comm_fd( hComm, fd );
    return TRUE;
}

/***********************************************************************
 *           GetCommModemStatus   (KERNEL32.@)
 *
 *  Obtains the four control register bits if supported by the hardware.
 *
 * PARAMS
 *
 *      hFile           [in]    The communications device
 *      lpModemStat     [out]   The control register bits
 *
 * RETURNS
 *
 *  True if the communications handle was good and for hardware that
 *  control register access, false otherwise.
 */
BOOL WINAPI GetCommModemStatus(HANDLE hFile, LPDWORD lpModemStat)
{
    return DeviceIoControl(hFile, IOCTL_SERIAL_GET_MODEMSTATUS,
                           NULL, 0, lpModemStat, sizeof(DWORD), NULL, NULL);
}

static DWORD WINAPI Comm_CheckEvents(int fd, DWORD mask, serial_irq_info *new, serial_irq_info *old, DWORD new_mstat, DWORD old_mstat)
{
    DWORD ret = 0, queue;

    TRACE("mask 0x%08lx\n", mask);
    TRACE("old->rx          0x%08x vs. new->rx          0x%08x\n", old->rx, new->rx);
    TRACE("old->tx          0x%08x vs. new->tx          0x%08x\n", old->tx, new->tx);
    TRACE("old->frame       0x%08x vs. new->frame       0x%08x\n", old->frame, new->frame);
    TRACE("old->overrun     0x%08x vs. new->overrun     0x%08x\n", old->overrun, new->overrun);
    TRACE("old->parity      0x%08x vs. new->parity      0x%08x\n", old->parity, new->parity);
    TRACE("old->brk         0x%08x vs. new->brk         0x%08x\n", old->brk, new->brk);
    TRACE("old->buf_overrun 0x%08x vs. new->buf_overrun 0x%08x\n", old->buf_overrun, new->buf_overrun);

    ret |= ((mask & EV_BREAK) && ( old->brk != new->brk))?EV_BREAK:0;
    ret |= ((mask & EV_CTS  ) && ((old_mstat&MS_CTS_ON )!=(new_mstat&MS_CTS_ON )))?EV_CTS  :0;
    ret |= ((mask & EV_DSR  ) && ((old_mstat&MS_DSR_ON )!=(new_mstat&MS_DSR_ON )))?EV_DSR  :0;
    ret |= ((mask & EV_RING ) && ((old_mstat&MS_RING_ON)!=(new_mstat&MS_RING_ON)))?EV_RING :0;
    ret |= ((mask & EV_RLSD ) && ((old_mstat&MS_RLSD_ON)!=(new_mstat&MS_RLSD_ON)))?EV_RLSD :0;
    ret |= ((mask & EV_ERR  ) && (( old->frame != new->frame) ||(old->overrun != new->overrun)
		|| (old->parity != new->parity)) )?EV_ERR  :0;
    if (mask & EV_RXCHAR)
    {
	queue = 0;
#ifdef TIOCINQ
	if(ioctl(fd, TIOCINQ, &queue))
	    WARN("TIOCINQ returned error\n");
#endif
	if (queue)
	    ret |= EV_RXCHAR;
    }
    if (mask & EV_TXEMPTY)
    {
	queue = 0;
/* We really want to know when all characters have gone out of the transmitter */
#if defined(TIOCSERGETLSR) 
	if(ioctl(fd, TIOCSERGETLSR, &queue))
	    WARN("TIOCSERGETLSR returned error\n");
	if (queue)
/* TIOCINQ only checks for an empty buffer */
#elif defined(TIOCINQ)
	if(ioctl(fd, TIOCOUTQ, &queue))
	    WARN("TIOCOUTQ returned error\n");
	if (!queue)
#endif
           ret |= EV_TXEMPTY;
    }
    TRACE("OUTQUEUE %ld, Transmitter %sempty\n", queue, (ret & EV_TXEMPTY)?"":"not ");
    return ret;
    
}

/***********************************************************************
 *             COMM_WaitCommEventService      (INTERNAL)
 *
 *  We need to poll for what is interesting
 *  TIOCMIWAIT only checks modem status line and may not be aborted by a changing mask
 *
 */
static DWORD WINAPI COMM_WaitCommEventService(LPVOID arg)
{
    async_commio *commio = (async_commio*) arg;
    int waitmask = 0;
    int rc, fd, abort;
    serial_irq_info new_irq_info;
    DWORD new_mstat, new_evtmask;

    fd=get_comm_fd( commio->handle, FILE_READ_DATA );

    TRACE("handle %p fd 0x%08x, mask 0x%08lx buffer %p event %p irq_info %p waitmask 0x%08x\n", 
	  commio->handle, fd, commio->evtmask, commio->buffer, commio->hEvent, &commio->irq_info, waitmask);
    do
    {
	/*
	 * TIOCMIWAIT is not adequate
	 *
	 * FIXME:
	 * We don't handle the EV_RXFLAG (the eventchar)
	 */
	Sleep(1);
	rc= COMM_GetEInfo(fd,&new_irq_info);
	if (rc)
	    TRACE("TIOCGICOUNT err %s\n", strerror(errno));
	rc = GetCommModemStatus(commio->handle, &new_mstat);
	if (!rc)
	    TRACE("GetCommModemStatus failed\n");
	rc = Comm_CheckEvents(fd, commio->evtmask,&new_irq_info,&commio->irq_info, new_mstat, commio->mstat);
	GetCommMask(commio->handle, &new_evtmask);
	abort = (commio->evtmask != new_evtmask);
	TRACE("resulting Eventmask 0x%08x\n", rc);
    } while (!rc && ! abort);
    if (abort) rc = 0;
    release_comm_fd( commio->handle, fd );
    *commio->buffer = rc;
    if (commio->hEvent != INVALID_HANDLE_VALUE )
        NtSetEvent( commio->hEvent, NULL );
    HeapFree(GetProcessHeap(), 0, commio );
    return 0;
}


/***********************************************************************
 *             COMM_WaitCommEvent         (INTERNAL)
 *
 *  This function must have an lpOverlapped.
 */
static BOOL COMM_WaitCommEvent(
    HANDLE hFile,              /* [in] handle of comm port to wait for */
    LPDWORD lpdwEvents,        /* [out] event(s) that were detected */
    LPOVERLAPPED lpOverlapped) /* [in/out] for Asynchronous waiting */
{
    int                 fd;
    async_commio*       commio;
    DWORD               result_mask;
    BOOL res;

    if (!lpOverlapped)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (NtResetEvent(lpOverlapped->hEvent,NULL))
        return FALSE;

    fd = get_comm_fd( hFile, FILE_WRITE_DATA );
    if (fd < 0) return FALSE;

    commio = HeapAlloc(GetProcessHeap(), 0, sizeof (async_commio));
    if (!commio)
    {
        release_comm_fd( hFile, fd );
        return FALSE;
    }

    commio->handle = hFile;
    commio->buffer = (char *)lpdwEvents;
    commio->hEvent = lpOverlapped->hEvent;
    GetCommMask(hFile, &commio->evtmask);

/* We may never return, if some capabilities miss
 * Return error in that case
 */
#if !defined(TIOCINQ)
    if(commio->evtmask & EV_RXCHAR)
	goto error;
#endif
#if !(defined(TIOCSERGETLSR) && defined(TIOCSER_TEMT)) || !defined(TIOCINQ)
    if(commio->evtmask & EV_TXEMPTY)
	goto error;
#endif
#if !defined(TIOCMGET)
    if(commio->evtmask & (EV_CTS | EV_DSR| EV_RING| EV_RLSD))
	goto error;
#endif
#if !defined(TIOCM_CTS)
    if(commio->evtmask & EV_CTS)
	goto error;
#endif
#if !defined(TIOCM_DSR)
    if(commio->evtmask & EV_DSR)
	goto error;
#endif
#if !defined(TIOCM_RNG)
    if(commio->evtmask & EV_RING)
	goto error;
#endif
#if !defined(TIOCM_CAR)
    if(commio->evtmask & EV_RLSD)
	goto error;
#endif
    if(commio->evtmask & EV_RXFLAG)
	FIXME("EV_RXFLAG not handled\n");
    COMM_GetEInfo(fd,&commio->irq_info);
    GetCommModemStatus(hFile, &commio->mstat);
    /* We might have received something or the TX bufffer is delivered*/
    result_mask = Comm_CheckEvents( fd, commio->evtmask, &commio->irq_info, &commio->irq_info,commio->mstat,commio->mstat);
    if (result_mask) 
    {
	TRACE("Event already met\n");
	*lpdwEvents = result_mask;
	HeapFree(GetProcessHeap(), 0, commio );
	res = TRUE;
    }
    else
    {
	CreateThread(NULL, 0, COMM_WaitCommEventService, (LPVOID)commio, 0, NULL);
	SetLastError(ERROR_IO_PENDING);
	res = FALSE;
    }
    release_comm_fd( hFile, fd );
    return res;
#if !defined(TIOCINQ) || (!(defined(TIOCSERGETLSR) && defined(TIOCSER_TEMT)) || !defined(TIOCINQ)) || !defined(TIOCMGET) || !defined(TIOCM_CTS) ||!defined(TIOCM_DSR) || !defined(TIOCM_RNG) || !defined(TIOCM_CAR)
 error:
    FIXME("Returning error because of missing capabilities\n");
    release_comm_fd( hFile, fd );
    HeapFree(GetProcessHeap(), 0, commio );
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
#endif
}
/***********************************************************************
 *           WaitCommEvent   (KERNEL32.@)
 *
 * Wait until something interesting happens on a COMM port.
 * Interesting things (events) are set by calling SetCommMask before
 * this function is called.
 *
 * RETURNS
 *   TRUE if successful
 *   FALSE if failure
 *
 *   The set of detected events will be written to *lpdwEventMask
 *   ERROR_IO_PENDING will be returned the overlapped structure was passed
 *
 * BUGS:
 *  Only supports EV_RXCHAR and EV_TXEMPTY
 */
BOOL WINAPI WaitCommEvent(
    HANDLE hFile,              /* [in] handle of comm port to wait for */
    LPDWORD lpdwEvents,        /* [out] event(s) that were detected */
    LPOVERLAPPED lpOverlapped) /* [in/out] for Asynchronous waiting */
{
    OVERLAPPED ov;
    int ret = 0;
    DWORD res, err;

    TRACE("(%p %p %p )\n",hFile, lpdwEvents,lpOverlapped);

    if(lpOverlapped)
        return COMM_WaitCommEvent(hFile, lpdwEvents, lpOverlapped);

    /* if there is no overlapped structure, create our own */
    ov.hEvent = CreateEventW(NULL,FALSE,FALSE,NULL);

    res = COMM_WaitCommEvent(hFile, lpdwEvents, &ov);
    err = GetLastError();
    if (!res)
    {
	if (err == ERROR_IO_PENDING)
	{
	    do 
	    {
		res = WaitForSingleObjectEx(ov.hEvent, INFINITE, FALSE);
	    } while (res != WAIT_OBJECT_0);
	    TRACE("Event met\n:");
	    ret = TRUE;
	}
	else
	{
	    FIXME("Unknown error 0x%08lx\n", err);
	    ret = FALSE;
	}
    }
    else
	ret = TRUE;
    CloseHandle(ov.hEvent);

    return ret;
}

/***********************************************************************
 *           GetCommProperties   (KERNEL32.@)
 *
 * This function fills in a structure with the capabilities of the
 * communications port driver.
 *
 * RETURNS
 *
 *  TRUE on success, FALSE on failure
 *  If successful, the lpCommProp structure be filled in with
 *  properties of the comm port.
 */
BOOL WINAPI GetCommProperties(
    HANDLE hFile,          /* [in] handle of the comm port */
    LPCOMMPROP lpCommProp) /* [out] pointer to struct to be filled */
{
    FIXME("(%p %p )\n",hFile,lpCommProp);
    if(!lpCommProp)
        return FALSE;

    /*
     * These values should be valid for LINUX's serial driver
     * FIXME: Perhaps they deserve an #ifdef LINUX
     */
    memset(lpCommProp,0,sizeof(COMMPROP));
    lpCommProp->wPacketLength       = 1;
    lpCommProp->wPacketVersion      = 1;
    lpCommProp->dwServiceMask       = SP_SERIALCOMM;
    lpCommProp->dwReserved1         = 0;
    lpCommProp->dwMaxTxQueue        = 4096;
    lpCommProp->dwMaxRxQueue        = 4096;
    lpCommProp->dwMaxBaud           = BAUD_115200;
    lpCommProp->dwProvSubType       = PST_RS232;
    lpCommProp->dwProvCapabilities  = PCF_DTRDSR | PCF_PARITY_CHECK | PCF_RTSCTS | PCF_TOTALTIMEOUTS;
    lpCommProp->dwSettableParams    = SP_BAUD | SP_DATABITS | SP_HANDSHAKING |
                                      SP_PARITY | SP_PARITY_CHECK | SP_STOPBITS ;
    lpCommProp->dwSettableBaud      = BAUD_075 | BAUD_110 | BAUD_134_5 | BAUD_150 |
                BAUD_300 | BAUD_600 | BAUD_1200 | BAUD_1800 | BAUD_2400 | BAUD_4800 |
                BAUD_9600 | BAUD_19200 | BAUD_38400 | BAUD_57600 | BAUD_115200 ;
    lpCommProp->wSettableData       = DATABITS_5 | DATABITS_6 | DATABITS_7 | DATABITS_8 ;
    lpCommProp->wSettableStopParity = STOPBITS_10 | STOPBITS_15 | STOPBITS_20 |
                PARITY_NONE | PARITY_ODD |PARITY_EVEN | PARITY_MARK | PARITY_SPACE;
    lpCommProp->dwCurrentTxQueue    = lpCommProp->dwMaxTxQueue;
    lpCommProp->dwCurrentRxQueue    = lpCommProp->dwMaxRxQueue;

    return TRUE;
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
static const WCHAR lpszSerialUI[] = { 
   's','e','r','i','a','l','u','i','.','d','l','l',0 };


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
    FARPROC lpfnCommDialog;
    HMODULE hConfigModule;
    BOOL r = FALSE;

    TRACE("(%p %p %p)\n",lpszDevice, hWnd, lpCommConfig);

    hConfigModule = LoadLibraryW(lpszSerialUI);
    if(!hConfigModule)
        return FALSE;

    lpfnCommDialog = GetProcAddress(hConfigModule, "drvCommConfigDialogA");

    if(lpfnCommDialog)
        r = lpfnCommDialog(lpszDevice,hWnd,lpCommConfig);

    FreeLibrary(hConfigModule);

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
    FARPROC lpfnCommDialog;
    HMODULE hConfigModule;
    BOOL r = FALSE;

    TRACE("(%p %p %p)\n",lpszDevice, hWnd, lpCommConfig);

    hConfigModule = LoadLibraryW(lpszSerialUI);
    if(!hConfigModule)
        return FALSE;

    lpfnCommDialog = GetProcAddress(hConfigModule, "drvCommConfigDialogW");

    if(lpfnCommDialog)
        r = lpfnCommDialog(lpszDevice,hWnd,lpCommConfig);

    FreeLibrary(hConfigModule);

    return r;
}

/***********************************************************************
 *           GetCommConfig     (KERNEL32.@)
 *
 * Fill in the COMMCONFIG structure for the comm port hFile
 *
 * RETURNS
 *
 *  TRUE on success, FALSE on failure
 *  If successful, lpCommConfig contains the comm port configuration.
 *
 * BUGS
 *
 */
BOOL WINAPI GetCommConfig(
    HANDLE       hFile,        /* [in] The communications device. */
    LPCOMMCONFIG lpCommConfig, /* [out] The communications configuration of the device (if it fits). */
    LPDWORD      lpdwSize)     /* [in/out] Initially the size of the configuration buffer/structure,
                                  afterwards the number of bytes copied to the buffer or
                                  the needed size of the buffer. */
{
    BOOL r;

    TRACE("(%p %p)\n",hFile,lpCommConfig);

    if(lpCommConfig == NULL)
        return FALSE;
    r = *lpdwSize < sizeof(COMMCONFIG); /* TRUE if not enough space */
    *lpdwSize = sizeof(COMMCONFIG);
    if(r)
        return FALSE;

    lpCommConfig->dwSize = sizeof(COMMCONFIG);
    lpCommConfig->wVersion = 1;
    lpCommConfig->wReserved = 0;
    r = GetCommState(hFile,&lpCommConfig->dcb);
    lpCommConfig->dwProviderSubType = PST_RS232;
    lpCommConfig->dwProviderOffset = 0;
    lpCommConfig->dwProviderSize = 0;

    return r;
}

/***********************************************************************
 *           SetCommConfig     (KERNEL32.@)
 *
 *  Sets the configuration of the communications device.
 *
 * RETURNS
 *
 *  True on success, false if the handle was bad is not a communications device.
 */
BOOL WINAPI SetCommConfig(
    HANDLE       hFile,		/* [in] The communications device. */
    LPCOMMCONFIG lpCommConfig,	/* [in] The desired configuration. */
    DWORD dwSize) 		/* [in] size of the lpCommConfig struct */
{
    TRACE("(%p %p)\n",hFile,lpCommConfig);
    return SetCommState(hFile,&lpCommConfig->dcb);
}

/***********************************************************************
 *           SetDefaultCommConfigA   (KERNEL32.@)
 *
 *  Initializes the default configuration for the specified communication
 *  device. (ascii)
 *
 * RETURNS
 *
 *  True if the device was found and the defaults set, false otherwise
 */
BOOL WINAPI SetDefaultCommConfigW(
    LPCWSTR       lpszDevice,  /* [in] The ascii name of the device targeted for configuration. */
    LPCOMMCONFIG lpCommConfig, /* [in] The default configuration for the device. */
    DWORD        dwSize)       /* [in] The number of bytes in the configuration structure. */
{
    FARPROC lpfnSetDefaultCommConfig;
    HMODULE hConfigModule;
    BOOL r = FALSE;

    TRACE("(%p %p %lx)\n",lpszDevice, lpCommConfig, dwSize);

    hConfigModule = LoadLibraryW(lpszSerialUI);
    if(!hConfigModule)
        return r;

    lpfnSetDefaultCommConfig = GetProcAddress(hConfigModule, "drvSetDefaultCommConfigW");
    if (lpfnSetDefaultCommConfig)
        r = lpfnSetDefaultCommConfig(lpszDevice, lpCommConfig, dwSize);

    FreeLibrary(hConfigModule);

    return r;
}


/***********************************************************************
 *           SetDefaultCommConfigW     (KERNEL32.@)
 *
 *  Initializes the default configuration for the specified
 *  communication device. (unicode)
 *
 * RETURNS
 *
 */
BOOL WINAPI SetDefaultCommConfigA(
    LPCSTR      lpszDevice,    /* [in] The unicode name of the device targeted for configuration. */
    LPCOMMCONFIG lpCommConfig, /* [in] The default configuration for the device. */
    DWORD        dwSize)       /* [in] The number of bytes in the configuration structure. */
{
    BOOL r;
    LPWSTR lpDeviceW = NULL;
    DWORD len;

    TRACE("(%s %p %lx)\n",debugstr_a(lpszDevice),lpCommConfig,dwSize);

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
     LPDCB lpdcb = &(lpCC->dcb);
     WCHAR temp[40];
     static const WCHAR comW[] = {'C','O','M',0};
     static const WCHAR formatW[] = {'C','O','M','%','c',':','3','8','4','0','0',',','n',',','8',',','1',0};

     if (strncmpiW(lpszName,comW,3)) {
        ERR("not implemented for <%s>\n", debugstr_w(lpszName));
        return FALSE;
     }

     TRACE("(%s %p %ld)\n", debugstr_w(lpszName), lpCC, *lpdwSize );
     if (*lpdwSize < sizeof(COMMCONFIG)) {
         *lpdwSize = sizeof(COMMCONFIG);
         return FALSE;
       }

     *lpdwSize = sizeof(COMMCONFIG);

     lpCC->dwSize = sizeof(COMMCONFIG);
     lpCC->wVersion = 1;
     lpCC->dwProviderSubType = PST_RS232;
     lpCC->dwProviderOffset = 0L;
     lpCC->dwProviderSize = 0L;

     sprintfW( temp, formatW, lpszName[3]);
     FIXME("setting %s as default\n", debugstr_w(temp));

     return BuildCommDCBW( temp, lpdcb);
}

/**************************************************************************
 *         GetDefaultCommConfigA		(KERNEL32.@)
 *
 *   Acquires the default configuration of the specified communication device. (ascii)
 *
 *  RETURNS
 *
 *   True on successful reading of the default configuration,
 *   if the device is not found or the buffer is too small.
 */
BOOL WINAPI GetDefaultCommConfigA(
    LPCSTR       lpszName, /* [in] The ascii name of the device targeted for configuration. */
    LPCOMMCONFIG lpCC,     /* [out] The default configuration for the device. */
    LPDWORD      lpdwSize) /* [in/out] Initially the size of the default configuration buffer,
			      afterwards the number of bytes copied to the buffer or
                              the needed size of the buffer. */
{
	BOOL ret = FALSE;
	UNICODE_STRING lpszNameW;

	TRACE("(%s,%p,%ld)\n",lpszName,lpCC,*lpdwSize);
	if(lpszName) RtlCreateUnicodeStringFromAsciiz(&lpszNameW,lpszName);
	else lpszNameW.Buffer = NULL;

	if(lpszNameW.Buffer) ret = GetDefaultCommConfigW(lpszNameW.Buffer,lpCC,lpdwSize);

	RtlFreeUnicodeString(&lpszNameW);
	return ret;
}
