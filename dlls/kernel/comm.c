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
 *
 * History:
 *
 * Apr 3, 1999.  Lawson Whitney <lawson_whitney@juno.com>
 * - Fixed the modem control part of EscapeCommFunction16.
 *
 * Mar 31, 1999. Ove Kåven <ovek@arcticnet.no>
 * - Implemented buffers and EnableCommNotification.
 *
 * Mar 3, 1999. Ove Kåven <ovek@arcticnet.no>
 * - Use port indices instead of unixfds for win16
 * - Moved things around (separated win16 and win32 routines)
 * - Added some hints on how to implement buffers and EnableCommNotification.
 *
 * Oktober 98, Rein Klazes [RHK]
 * A program that wants to monitor the modem status line (RLSD/DCD) may
 * poll the modem status register in the commMask structure. I update the bit
 * in GetCommError, waiting for an implementation of communication events.
 *
 * July 6, 1998. Fixes and comments by Valentijn Sessink
 *                                     <vsessink@ic.uva.nl> [V]
 *
 * August 12, 1997.  Take a bash at SetCommEventMask - Lawson Whitney
 *                                     <lawson_whitney@juno.com>
 *
 * May 26, 1997.  Fixes and comments by Rick Richardson <rick@dgii.com> [RER]
 * - ptr->fd wasn't getting cleared on close.
 * - GetCommEventMask() and GetCommError() didn't do much of anything.
 *   IMHO, they are still wrong, but they at least implement the RXCHAR
 *   event and return I/O queue sizes, which makes the app I'm interested
 *   in (analog devices EZKIT DSP development system) work.
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
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
#include <sys/stat.h>
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
#include "winbase.h"
#include "winerror.h"

#include "wine/server.h"
#include "async.h"
#include "file.h"
#include "heap.h"

#include "wine/debug.h"

#ifdef HAVE_LINUX_SERIAL_H
#include <linux/serial.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(comm);

/***********************************************************************
 * Asynchronous I/O for asynchronous wait requests                     *
 */

static DWORD commio_get_async_count (const async_private *ovp);
static void commio_async_cleanup  (async_private *ovp);

static async_ops commio_async_ops =
{
    commio_get_async_count,        /* get_count */
    NULL,                          /* call_completion */
    commio_async_cleanup           /* cleanup */
};

typedef struct async_commio
{
    struct async_private             async;
    char                             *buffer;
} async_commio;

static DWORD commio_get_async_count (const struct async_private *ovp)
{
    return 0;
}

static void commio_async_cleanup  (async_private *ovp)
{
    HeapFree(GetProcessHeap(), 0, ovp );
}

/***********************************************************************/

#if !defined(TIOCINQ) && defined(FIONREAD)
#define	TIOCINQ FIONREAD
#endif

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
 *           COMM_BuildOldCommDCB   (Internal)
 *
 *  Build a DCB using the old style settings string eg: "COMx:96,n,8,1"
 *  We ignore the COM port index, since we can support more than 4 ports.
 */
BOOL WINAPI COMM_BuildOldCommDCB(LPCSTR device, LPDCB lpdcb)
{
	/* "COM1:96,n,8,1"	*/
	/*  012345		*/
	char *ptr, temp[256], last;
	int rate;

	TRACE("(%s), ptr %p\n", device, lpdcb);

        /* Some applications call this function with "9600,n,8,1"
         * not sending the "COM1:" parameter at left of string */
        if (!strncasecmp(device,"COM",3))
        {
            if (!device[3]) return FALSE;
            if (device[4] != ':' && device[4] != ' ') return FALSE;
            strcpy(temp,device+5);
        }
        else strcpy(temp,device);

	last=temp[strlen(temp)-1];
	ptr = strtok(temp, ", ");

        /* DOS/Windows only compares the first two numbers
	 * and assigns an appropriate baud rate.
	 * You can supply 961324245, it still returns 9600 ! */
	if (strlen(ptr) < 2)
	{
		WARN("Unknown baudrate string '%s' !\n", ptr);
		return FALSE; /* error: less than 2 chars */
	}
	ptr[2] = '\0';
	rate = atoi(ptr);

	switch (rate) {
	case 11:
	case 30:
	case 60:
		rate *= 10;
		break;
	case 12:
	case 24:
	case 48:
	case 96:
		rate *= 100;
		break;
	case 19:
		rate = 19200;
		break;
	default:
		WARN("Unknown baudrate indicator %d !\n", rate);
		return FALSE;
	}

        lpdcb->BaudRate = rate;
	TRACE("baudrate (%ld)\n", lpdcb->BaudRate);

	ptr = strtok(NULL, ", ");
	if (islower(*ptr))
		*ptr = toupper(*ptr);

       	TRACE("parity (%c)\n", *ptr);
	lpdcb->fParity = TRUE;
	switch (*ptr) {
	case 'N':
		lpdcb->Parity = NOPARITY;
		lpdcb->fParity = FALSE;
		break;
	case 'E':
		lpdcb->Parity = EVENPARITY;
		break;
	case 'M':
		lpdcb->Parity = MARKPARITY;
		break;
	case 'O':
		lpdcb->Parity = ODDPARITY;
		break;
        case 'S':
                lpdcb->Parity = SPACEPARITY;
                break;
	default:
		WARN("Unknown parity `%c'!\n", *ptr);
		return FALSE;
	}

	ptr = strtok(NULL, ", ");
       	TRACE("charsize (%c)\n", *ptr);
	lpdcb->ByteSize = *ptr - '0';

	ptr = strtok(NULL, ", ");
       	TRACE("stopbits (%c)\n", *ptr);
	switch (*ptr) {
	case '1':
		lpdcb->StopBits = ONESTOPBIT;
		break;
	case '2':
		lpdcb->StopBits = TWOSTOPBITS;
		break;
	default:
		WARN("Unknown # of stopbits `%c'!\n", *ptr);
		return FALSE;
	}

	if (last == 'x') {
		lpdcb->fInX		= TRUE;
		lpdcb->fOutX		= TRUE;
		lpdcb->fOutxCtsFlow	= FALSE;
		lpdcb->fOutxDsrFlow	= FALSE;
		lpdcb->fDtrControl	= DTR_CONTROL_ENABLE;
		lpdcb->fRtsControl	= RTS_CONTROL_ENABLE;
	} else if (last=='p') {
		lpdcb->fInX		= FALSE;
		lpdcb->fOutX		= FALSE;
		lpdcb->fOutxCtsFlow	= TRUE;
		lpdcb->fOutxDsrFlow	= FALSE;
		lpdcb->fDtrControl	= DTR_CONTROL_ENABLE;
		lpdcb->fRtsControl	= RTS_CONTROL_HANDSHAKE;
	} else {
		lpdcb->fInX		= FALSE;
		lpdcb->fOutX		= FALSE;
		lpdcb->fOutxCtsFlow	= FALSE;
		lpdcb->fOutxDsrFlow	= FALSE;
		lpdcb->fDtrControl	= DTR_CONTROL_ENABLE;
		lpdcb->fRtsControl	= RTS_CONTROL_ENABLE;
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
 *         BuildCommDCBAndTimeoutsA	(KERNEL32.@)
 *
 *  Updates a device control block data structure with values from an
 *  ascii device control string.  Taking timeout values from a timeouts
 *  struct if desired by the control string.
 *
 * RETURNS
 *
 *  True on success, false bad handles etc
 */
BOOL WINAPI BuildCommDCBAndTimeoutsA(
    LPCSTR         device,     /* [in] The ascii device control string. */
    LPDCB          lpdcb,      /* [out] The device control block to be updated. */
    LPCOMMTIMEOUTS lptimeouts) /* [in] The timeouts to use if asked to set them by the control string. */
{
	int	port;
	char	*ptr,*temp;

	TRACE("(%s,%p,%p)\n",device,lpdcb,lptimeouts);

	if (!strncasecmp(device,"COM",3)) {
		port=device[3]-'0';
		if (port--==0) {
			ERR("BUG! COM0 can't exist!\n");
			return FALSE;
		}
		if ((*(device+4)!=':') && (*(device+4)!=' '))
			return FALSE;
		temp=(LPSTR)(device+5);
	} else
		temp=(LPSTR)device;

	memset(lpdcb,0,sizeof (DCB));
	lpdcb->DCBlength	= sizeof(DCB);
	if (strchr(temp,',')) {	/* old style */

		return COMM_BuildOldCommDCB(device,lpdcb);
	}
	ptr=strtok(temp," ");
	while (ptr) {
		DWORD	flag,x;

		flag=0;
		if (!strncasecmp("baud=",ptr,5)) {
			if (!sscanf(ptr+5,"%ld",&x))
				WARN("Couldn't parse %s\n",ptr);
			lpdcb->BaudRate = x;
			flag=1;
		}
		if (!strncasecmp("stop=",ptr,5)) {
			if (!sscanf(ptr+5,"%ld",&x))
				WARN("Couldn't parse %s\n",ptr);
			lpdcb->StopBits = x;
			flag=1;
		}
		if (!strncasecmp("data=",ptr,5)) {
			if (!sscanf(ptr+5,"%ld",&x))
				WARN("Couldn't parse %s\n",ptr);
			lpdcb->ByteSize = x;
			flag=1;
		}
		if (!strncasecmp("parity=",ptr,7)) {
			lpdcb->fParity	= TRUE;
			switch (ptr[7]) {
			case 'N':case 'n':
				lpdcb->fParity	= FALSE;
				lpdcb->Parity	= NOPARITY;
				break;
			case 'E':case 'e':
				lpdcb->Parity	= EVENPARITY;
				break;
			case 'O':case 'o':
				lpdcb->Parity	= ODDPARITY;
				break;
			case 'M':case 'm':
				lpdcb->Parity	= MARKPARITY;
				break;
                        case 'S':case 's':
                                lpdcb->Parity   = SPACEPARITY;
                                break;
			}
			flag=1;
		}
		if (!flag)
			ERR("Unhandled specifier '%s', please report.\n",ptr);
		ptr=strtok(NULL," ");
	}
	if (lpdcb->BaudRate==110)
		lpdcb->StopBits = 2;
	return TRUE;
}

/**************************************************************************
 *         BuildCommDCBAndTimeoutsW		(KERNEL32.@)
 *
 *  Updates a device control block data structure with values from an
 *  unicode device control string.  Taking timeout values from a timeouts
 *  struct if desired by the control string.
 *
 * RETURNS
 *
 *  True on success, false bad handles etc.
 */
BOOL WINAPI BuildCommDCBAndTimeoutsW(
    LPCWSTR        devid,      /* [in] The unicode device control string. */
    LPDCB          lpdcb,      /* [out] The device control block to be updated. */
    LPCOMMTIMEOUTS lptimeouts) /* [in] The timeouts to use if asked to set them by the control string. */
{
	BOOL ret = FALSE;
	LPSTR	devidA;

	TRACE("(%p,%p,%p)\n",devid,lpdcb,lptimeouts);
	devidA = HEAP_strdupWtoA( GetProcessHeap(), 0, devid );
	if (devidA)
	{
	ret=BuildCommDCBAndTimeoutsA(devidA,lpdcb,lptimeouts);
        HeapFree( GetProcessHeap(), 0, devidA );
	}
	return ret;
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
 *  True on success, false on an malformed control string.
 */
BOOL WINAPI BuildCommDCBW(
    LPCWSTR devid, /* [in] The unicode device control string. */
    LPDCB   lpdcb) /* [out] The device control block to be updated. */
{
	return BuildCommDCBAndTimeoutsW(devid,lpdcb,NULL);
}

static BOOL COMM_SetCommError(HANDLE handle, DWORD error)
{
    DWORD ret;

    SERVER_START_REQ( set_serial_info )
    {
        req->handle = handle;
        req->flags = SERIALINFO_SET_ERROR;
        req->commerror = error;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

static BOOL COMM_GetCommError(HANDLE handle, LPDWORD lperror)
{
    DWORD ret;

    if(!lperror)
        return FALSE;

    SERVER_START_REQ( get_serial_info )
    {
        req->handle = handle;
        ret = !wine_server_call_err( req );
        *lperror = reply->commerror;
    }
    SERVER_END_REQ;

    return ret;
}

/*****************************************************************************
 *	SetCommBreak		(KERNEL32.@)
 *
 *  Halts the transmission of characters to a communications device.
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
BOOL WINAPI SetCommBreak(
    HANDLE handle) /* [in] The communictions device to suspend. */
{
#if defined(TIOCSBRK) && defined(TIOCCBRK) /* check if available for compilation */
        int fd,result;

	fd = FILE_GetUnixHandle( handle, GENERIC_READ );
	if(fd<0) {
	        TRACE("FILE_GetUnixHandle failed\n");
		return FALSE;
	}
	result = ioctl(fd,TIOCSBRK,0);
	close(fd);
	if (result ==-1)
	  {
	        TRACE("ioctl failed\n");
		SetLastError(ERROR_NOT_SUPPORTED);
		return FALSE;
	  }
	return TRUE;
#else
	FIXME("ioctl not available\n");
	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
#endif
}

/*****************************************************************************
 *	ClearCommBreak		(KERNEL32.@)
 *
 *  Resumes character transmission from a communication device.
 *
 * RETURNS
 *
 *  True on success and false if the communications device could not be found.
 *
 * BUGS
 *
 *  Only TIOCSBRK and TIOCCBRK are supported.
 */
BOOL WINAPI ClearCommBreak(
    HANDLE handle) /* [in] The halted communication device whose character transmission is to be resumed. */
{
#if defined(TIOCSBRK) && defined(TIOCCBRK) /* check if available for compilation */
        int fd,result;

	fd = FILE_GetUnixHandle( handle, GENERIC_READ );
	if(fd<0) {
	        TRACE("FILE_GetUnixHandle failed\n");
		return FALSE;
	}
	result = ioctl(fd,TIOCCBRK,0);
	close(fd);
	if (result ==-1)
	  {
	        TRACE("ioctl failed\n");
		SetLastError(ERROR_NOT_SUPPORTED);
		return FALSE;
	  }
	return TRUE;
#else
	FIXME("ioctl not available\n");
	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
#endif
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
	fd = FILE_GetUnixHandle( handle, GENERIC_READ );
	if(fd<0) {
		FIXME("handle %p not found.\n",handle);
		return FALSE;
	}

	if (tcgetattr(fd,&port) == -1) {
		COMM_SetCommError(handle,CE_IOE);
		close(fd);
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
		close(fd);
		COMM_SetCommError(handle,CE_IOE);
		return FALSE;
	  } else
	        result= TRUE;
	else
	  {
	    if (result == -1)
	      {
		result= FALSE;
		COMM_SetCommError(handle,CE_IOE);
	      }
	    else
	      result = TRUE;
	  }
	close(fd);
	return result;
}

/********************************************************************
 *      PurgeComm        (KERNEL32.@)
 *
 *  Terminates pending operations and/or discards buffers on a
 *  communication resource.
 *
 * RETURNS
 *
 *  True on success and false if the communications handle is bad.
 */
BOOL WINAPI PurgeComm(
    HANDLE handle, /* [in] The communication resource to be purged. */
    DWORD  flags)  /* [in] Flags for clear pending/buffer on input/output. */
{
     int fd;

     TRACE("handle %p, flags %lx\n", handle, flags);

     fd = FILE_GetUnixHandle( handle, GENERIC_READ );
     if(fd<0) {
	FIXME("no handle %p found\n",handle);
	return FALSE;
     }

     /*
     ** not exactly sure how these are different
     ** Perhaps if we had our own internal queues, one flushes them
     ** and the other flushes the kernel's buffers.
     */
     if(flags&PURGE_TXABORT)
         tcflush(fd,TCOFLUSH);
     if(flags&PURGE_RXABORT)
         tcflush(fd,TCIFLUSH);
     if(flags&PURGE_TXCLEAR)
         tcflush(fd,TCOFLUSH);
     if(flags&PURGE_RXCLEAR)
         tcflush(fd,TCIFLUSH);
     close(fd);

     return 1;
}

/*****************************************************************************
 *	ClearCommError	(KERNEL32.@)
 *
 *  Enables further I/O operations on a communications resource after
 *  supplying error and current status information.
 *
 * RETURNS
 *
 *  True on success, false if the communication resource handle is bad.
 */
BOOL WINAPI ClearCommError(
    HANDLE    handle, /* [in] The communication resource with the error. */
    LPDWORD   errors, /* [out] Flags indicating error the resource experienced. */
    LPCOMSTAT lpStat) /* [out] The status of the communication resource. */
{
    int fd;

    fd=FILE_GetUnixHandle( handle, GENERIC_READ );
    if(0>fd)
    {
	FIXME("no handle %p found\n",handle);
        return FALSE;
    }

    if (lpStat)
    {
        lpStat->fCtsHold = 0;
	lpStat->fDsrHold = 0;
	lpStat->fRlsdHold = 0;
	lpStat->fXoffHold = 0;
	lpStat->fXoffSent = 0;
	lpStat->fEof = 0;
	lpStat->fTxim = 0;
	lpStat->fReserved = 0;

#ifdef TIOCOUTQ
	if(ioctl(fd, TIOCOUTQ, &lpStat->cbOutQue))
	    WARN("ioctl returned error\n");
#else
	lpStat->cbOutQue = 0; /* FIXME: find a different way to find out */
#endif

#ifdef TIOCINQ
	if(ioctl(fd, TIOCINQ, &lpStat->cbInQue))
	    WARN("ioctl returned error\n");
#endif

	TRACE("handle %p cbInQue = %ld cbOutQue = %ld\n",
	      handle, lpStat->cbInQue, lpStat->cbOutQue);
    }

    close(fd);

    COMM_GetCommError(handle, errors);
    COMM_SetCommError(handle, 0);

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
    fd=FILE_GetUnixHandle( handle, GENERIC_READ );
    if(0>fd) {
	FIXME("handle %p not found?\n",handle);
        return FALSE;
    }
    close(fd);
    return TRUE;
}

/*****************************************************************************
 *	GetCommMask	(KERNEL32.@)
 *
 *  Obtain the events associated with a communication device that will cause
 *  a call WaitCommEvent to return.
 *
 *  RETURNS
 *
 *   True on success, fail on bad device handle etc.
 */
BOOL WINAPI GetCommMask(
    HANDLE  handle,  /* [in] The communications device. */
    LPDWORD evtmask) /* [out] The events which cause WaitCommEvent to return. */
{
    BOOL ret;

    TRACE("handle %p, mask %p\n", handle, evtmask);

    SERVER_START_REQ( get_serial_info )
    {
        req->handle = handle;
        if ((ret = !wine_server_call_err( req )))
        {
            if (evtmask) *evtmask = reply->eventmask;
        }
    }
    SERVER_END_REQ;
    return ret;
}

/*****************************************************************************
 *	SetCommMask	(KERNEL32.@)
 *
 *  There be some things we need to hear about yon there communications device.
 *  (Set which events associated with a communication device should cause
 *  a call WaitCommEvent to return.)
 *
 * RETURNS
 *
 *  True on success, false on bad handle etc.
 */
BOOL WINAPI SetCommMask(
    HANDLE handle,  /* [in] The communications device.  */
    DWORD  evtmask) /* [in] The events that are to be monitored. */
{
    BOOL ret;

    TRACE("handle %p, mask %lx\n", handle, evtmask);

    SERVER_START_REQ( set_serial_info )
    {
        req->handle    = handle;
        req->flags     = SERIALINFO_SET_MASK;
        req->eventmask = evtmask;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/*****************************************************************************
 *	SetCommState    (KERNEL32.@)
 *
 *  Re-initializes all hardware and control settings of a communications device,
 *  with values from a device control block without effecting the input and output
 *  queues.
 *
 * RETURNS
 *
 *  True on success, false on failure eg if the XonChar is equal to the XoffChar.
 */
BOOL WINAPI SetCommState(
    HANDLE handle, /* [in] The communications device. */
    LPDCB  lpdcb)  /* [out] The device control block. */
{
     struct termios port;
     int fd, bytesize, stopbits;

     TRACE("handle %p, ptr %p\n", handle, lpdcb);
     TRACE("bytesize %d baudrate %ld fParity %d Parity %d stopbits %d\n",
	   lpdcb->ByteSize,lpdcb->BaudRate,lpdcb->fParity, lpdcb->Parity,
	   (lpdcb->StopBits == ONESTOPBIT)?1:
	   (lpdcb->StopBits == TWOSTOPBITS)?2:0);
     TRACE("%s %s\n",(lpdcb->fInX)?"IXON":"~IXON",
	   (lpdcb->fOutX)?"IXOFF":"~IXOFF");

     fd = FILE_GetUnixHandle( handle, GENERIC_READ );
     if (fd < 0)  {
	FIXME("no handle %p found\n",handle);
	return FALSE;
     }

     if ((tcgetattr(fd,&port)) == -1) {
         int save_error = errno;
         COMM_SetCommError(handle,CE_IOE);
         close( fd );
         ERR("tcgetattr error '%s'\n", strerror(save_error));
         return FALSE;
     }

	port.c_cc[VMIN] = 0;
	port.c_cc[VTIME] = 1;

#ifdef IMAXBEL
	port.c_iflag &= ~(ISTRIP|BRKINT|IGNCR|ICRNL|INLCR|IMAXBEL);
#else
	port.c_iflag &= ~(ISTRIP|BRKINT|IGNCR|ICRNL|INLCR);
#endif
	port.c_iflag |= (IGNBRK);

	port.c_oflag &= ~(OPOST);

	port.c_cflag &= ~(HUPCL);
	port.c_cflag |= CLOCAL | CREAD;

	port.c_lflag &= ~(ICANON|ECHO|ISIG);
	port.c_lflag |= NOFLSH;

#ifdef CBAUD
	port.c_cflag &= ~CBAUD;
	switch (lpdcb->BaudRate) {
		case 110:
		case CBR_110:
			port.c_cflag |= B110;
			break;
		case 300:
		case CBR_300:
			port.c_cflag |= B300;
			break;
		case 600:
		case CBR_600:
			port.c_cflag |= B600;
			break;
		case 1200:
		case CBR_1200:
			port.c_cflag |= B1200;
			break;
		case 2400:
		case CBR_2400:
			port.c_cflag |= B2400;
			break;
		case 4800:
		case CBR_4800:
			port.c_cflag |= B4800;
			break;
		case 9600:
		case CBR_9600:
			port.c_cflag |= B9600;
			break;
		case 19200:
		case CBR_19200:
			port.c_cflag |= B19200;
			break;
		case 38400:
		case CBR_38400:
			port.c_cflag |= B38400;
			break;
#ifdef B57600
		case 57600:
			port.c_cflag |= B57600;
			break;
#endif
#ifdef B115200
		case 115200:
			port.c_cflag |= B115200;
			break;
#endif
#ifdef B230400
		case 230400:
			port.c_cflag |= B230400;
			break;
#endif
#ifdef B460800
		case 460800:
			port.c_cflag |= B460800;
			break;
#endif
       	        default:
#if defined (HAVE_LINUX_SERIAL_H) && defined (TIOCSSERIAL)
			{   struct serial_struct nuts;
			    int arby;
			    ioctl(fd, TIOCGSERIAL, &nuts);
			    nuts.custom_divisor = nuts.baud_base / lpdcb->BaudRate;
			    if (!(nuts.custom_divisor)) nuts.custom_divisor = 1;
			    arby = nuts.baud_base / nuts.custom_divisor;
			    nuts.flags &= ~ASYNC_SPD_MASK;
			    nuts.flags |= ASYNC_SPD_CUST;
			    WARN("You (or a program acting at your behest) have specified\n"
                                 "a non-standard baud rate %ld.  Wine will set the rate to %d,\n"
                                 "which is as close as we can get by our present understanding of your\n"
                                 "hardware. I hope you know what you are doing.  Any disruption Wine\n"
                                 "has caused to your linux system can be undone with setserial \n"
                                 "(see man setserial). If you have incapacitated a Hayes type modem,\n"
                                 "reset it and it will probably recover.\n", lpdcb->BaudRate, arby);
  			    ioctl(fd, TIOCSSERIAL, &nuts);
			    port.c_cflag |= B38400;
 			}
 			break;
#endif    /* Don't have linux/serial.h or lack TIOCSSERIAL */


                        COMM_SetCommError(handle,IE_BAUDRATE);
			close( fd );
			ERR("baudrate %ld\n",lpdcb->BaudRate);
			return FALSE;
	}
#elif !defined(__EMX__)
        switch (lpdcb->BaudRate) {
                case 110:
                case CBR_110:
                        port.c_ospeed = B110;
                        break;
                case 300:
                case CBR_300:
                        port.c_ospeed = B300;
                        break;
                case 600:
                case CBR_600:
                        port.c_ospeed = B600;
                        break;
                case 1200:
                case CBR_1200:
                        port.c_ospeed = B1200;
                        break;
                case 2400:
                case CBR_2400:
                        port.c_ospeed = B2400;
                        break;
                case 4800:
                case CBR_4800:
                        port.c_ospeed = B4800;
                        break;
                case 9600:
                case CBR_9600:
                        port.c_ospeed = B9600;
                        break;
                case 19200:
                case CBR_19200:
                        port.c_ospeed = B19200;
                        break;
                case 38400:
                case CBR_38400:
                        port.c_ospeed = B38400;
                        break;
#ifdef B57600
		case 57600:
		case CBR_57600:
			port.c_cflag |= B57600;
			break;
#endif
#ifdef B115200
		case 115200:
		case CBR_115200:
			port.c_cflag |= B115200;
			break;
#endif
#ifdef B230400
		case 230400:
			port.c_cflag |= B230400;
			break;
#endif
#ifdef B460800
		case 460800:
			port.c_cflag |= B460800;
			break;
#endif
                default:
                        COMM_SetCommError(handle,IE_BAUDRATE);
                        close( fd );
			ERR("baudrate %ld\n",lpdcb->BaudRate);
                        return FALSE;
        }
        port.c_ispeed = port.c_ospeed;
#endif
        bytesize=lpdcb->ByteSize;
        stopbits=lpdcb->StopBits;

#ifdef CMSPAR
	port.c_cflag &= ~(PARENB | PARODD | CMSPAR);
#else
	port.c_cflag &= ~(PARENB | PARODD);
#endif
	if (lpdcb->fParity)
            port.c_iflag |= INPCK;
        else
            port.c_iflag &= ~INPCK;
        switch (lpdcb->Parity) {
                case NOPARITY:
                        break;
                case ODDPARITY:
                        port.c_cflag |= (PARENB | PARODD);
                        break;
                case EVENPARITY:
                        port.c_cflag |= PARENB;
                        break;
#ifdef CMSPAR
                /* Linux defines mark/space (stick) parity */
                case MARKPARITY:
                        port.c_cflag |= (PARENB | CMSPAR);
                        break;
                case SPACEPARITY:
                        port.c_cflag |= (PARENB | PARODD |  CMSPAR);
                        break;
#else
                /* try the POSIX way */
                case MARKPARITY:
                        if( stopbits == ONESTOPBIT) {
                            stopbits = TWOSTOPBITS;
                            port.c_iflag &= ~INPCK;
                        } else {
                            COMM_SetCommError(handle,IE_BYTESIZE);
                            close( fd );
                            ERR("Cannot set MARK Parity\n");
                            return FALSE;
                        }
                        break;
                case SPACEPARITY:
                        if( bytesize < 8) {
                            bytesize +=1;
                            port.c_iflag &= ~INPCK;
                        } else {
                            COMM_SetCommError(handle,IE_BYTESIZE);
                            close( fd );
                            ERR("Cannot set SPACE Parity\n");
                            return FALSE;
                        }
                        break;
#endif
               default:
                        COMM_SetCommError(handle,IE_BYTESIZE);
                        close( fd );
			ERR("Parity\n");
                        return FALSE;
        }


	port.c_cflag &= ~CSIZE;
	switch (bytesize) {
		case 5:
			port.c_cflag |= CS5;
			break;
		case 6:
			port.c_cflag |= CS6;
			break;
		case 7:
			port.c_cflag |= CS7;
			break;
		case 8:
			port.c_cflag |= CS8;
			break;
		default:
                        COMM_SetCommError(handle,IE_BYTESIZE);
                        close( fd );
			ERR("ByteSize\n");
			return FALSE;
	}

	switch (stopbits) {
		case ONESTOPBIT:
				port.c_cflag &= ~CSTOPB;
				break;
		case ONE5STOPBITS: /* wil be selected if bytesize is 5 */
		case TWOSTOPBITS:
				port.c_cflag |= CSTOPB;
				break;
		default:
                        COMM_SetCommError(handle,IE_BYTESIZE);
                        close( fd );
			ERR("StopBits\n");
			return FALSE;
	}
#ifdef CRTSCTS
	if (	lpdcb->fOutxCtsFlow 			||
		lpdcb->fRtsControl == RTS_CONTROL_HANDSHAKE
	)
	  {
	    port.c_cflag |= CRTSCTS;
	    TRACE("CRTSCTS\n");
	  }
#endif

	if (lpdcb->fDtrControl == DTR_CONTROL_HANDSHAKE)
	  {
             WARN("DSR/DTR flow control not supported\n");
	  }

	if (lpdcb->fInX)
		port.c_iflag |= IXON;
	else
		port.c_iflag &= ~IXON;
	if (lpdcb->fOutX)
		port.c_iflag |= IXOFF;
	else
		port.c_iflag &= ~IXOFF;

	if (tcsetattr(fd,TCSANOW,&port)==-1) { /* otherwise it hangs with pending input*/
	        int save_error=errno;
                COMM_SetCommError(handle,CE_IOE);
                close( fd );
                ERR("tcsetattr error '%s'\n", strerror(save_error));
		return FALSE;
	} else {
                COMM_SetCommError(handle,0);
                close( fd );
		return TRUE;
	}
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

     TRACE("handle %p, ptr %p\n", handle, lpdcb);

     fd = FILE_GetUnixHandle( handle, GENERIC_READ );
     if (fd < 0)
       {
	 ERR("FILE_GetUnixHandle failed\n");
	 return FALSE;
       }
     if (tcgetattr(fd, &port) == -1) {
                int save_error=errno;
                ERR("tcgetattr error '%s'\n", strerror(save_error));
                COMM_SetCommError(handle,CE_IOE);
                close( fd );
		return FALSE;
	}
     close( fd );
#ifndef __EMX__
#ifdef CBAUD
     speed= (port.c_cflag & CBAUD);
#else
     speed= (cfgetospeed(&port));
#endif
     switch (speed) {
		case B110:
			lpdcb->BaudRate = 110;
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
		        ERR("unknown speed %x \n",speed);
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
		        ERR("unknown size %x \n",port.c_cflag & CSIZE);
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
	lpdcb->fDtrControl = DTR_CONTROL_ENABLE;

#ifdef CRTSCTS

	if (port.c_cflag & CRTSCTS) {
		lpdcb->fRtsControl = RTS_CONTROL_HANDSHAKE;
		lpdcb->fOutxCtsFlow = 1;
	} else
#endif
	{
		lpdcb->fRtsControl = RTS_CONTROL_ENABLE;
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

        COMM_SetCommError(handle,0);

        TRACE("OK\n");

	TRACE("bytesize %d baudrate %ld fParity %d Parity %d stopbits %d\n",
	      lpdcb->ByteSize,lpdcb->BaudRate,lpdcb->fParity, lpdcb->Parity,
	      (lpdcb->StopBits == ONESTOPBIT)?1:
	      (lpdcb->StopBits == TWOSTOPBITS)?2:0);
	TRACE("%s %s\n",(lpdcb->fInX)?"IXON":"~IXON",
	      (lpdcb->fOutX)?"IXOFF":"~IXOFF");
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
 * RETURNS
 *
 *  True if the call succeeded, false if the previous command character to the
 *  same device has not been sent yet the handle is bad etc.
 *
 * BUGS
 *
 *  Stub.
 */
BOOL WINAPI TransmitCommChar(
    HANDLE hComm,      /* [in] The communication device in need of a command character. */
    CHAR   chTransmit) /* [in] The character to transmit. */
{
    BOOL r = FALSE;
    int fd;

    WARN("(%p,'%c') not perfect!\n",hComm,chTransmit);

    fd = FILE_GetUnixHandle( hComm, GENERIC_READ );
    if ( fd < 0 )
        SetLastError ( ERROR_INVALID_PARAMETER );
    else
    {
        r = (1 == write(fd, &chTransmit, 1));
        close(fd);
    }

    return r;
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
    fd = FILE_GetUnixHandle( hComm, GENERIC_READ );
    if (fd < 0) {
       FIXME("no fd for handle = %p!.\n",hComm);
       return FALSE;
    }

    if (-1==tcgetattr(fd,&tios)) {
        FIXME("tcgetattr on fd %d failed!\n",fd);
        close(fd);
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
        close(fd);
        return FALSE;
    }
    close(fd);
    return TRUE;
}

/***********************************************************************
 *           GetCommModemStatus   (KERNEL32.@)
 *
 *  Obtains the four control register bits if supported by the hardware.
 *
 * RETURNS
 *
 *  True if the communications handle was good and for hardware that
 *  control register access, false otherwise.
 */
BOOL WINAPI GetCommModemStatus(
    HANDLE  hFile,       /* [in] The communications device. */
    LPDWORD lpModemStat) /* [out] The control register bits. */
{
	int fd,mstat, result=FALSE;

	*lpModemStat=0;
#ifdef TIOCMGET
	fd = FILE_GetUnixHandle( hFile, GENERIC_READ );
	if(fd<0)
		return FALSE;
	result = ioctl(fd, TIOCMGET, &mstat);
	close(fd);
	if (result == -1)
	  {
	    WARN("ioctl failed\n");
	    return FALSE;
	  }
#ifdef TIOCM_CTS
	if (mstat & TIOCM_CTS)
	    *lpModemStat |= MS_CTS_ON;
#endif
#ifdef TIOCM_DSR
	if (mstat & TIOCM_DSR)
	  *lpModemStat |= MS_DSR_ON;
#endif
#ifdef TIOCM_RNG
	if (mstat & TIOCM_RNG)
	  *lpModemStat |= MS_RING_ON;
#endif
#ifdef TIOCM_CAR
	/*FIXME:  Not really sure about RLSD  UB 990810*/
	if (mstat & TIOCM_CAR)
	  *lpModemStat |= MS_RLSD_ON;
#endif
	TRACE("%04x -> %s%s%s%s\n", mstat,
	      (*lpModemStat &MS_RLSD_ON)?"MS_RLSD_ON ":"",
	      (*lpModemStat &MS_RING_ON)?"MS_RING_ON ":"",
	      (*lpModemStat &MS_DSR_ON)?"MS_DSR_ON ":"",
	      (*lpModemStat &MS_CTS_ON)?"MS_CTS_ON ":"");
	return TRUE;
#else
	return FALSE;
#endif
}

/***********************************************************************
 *             COMM_WaitCommEventService      (INTERNAL)
 *
 *  This function is called while the client is waiting on the
 *  server, so we can't make any server calls here.
 */
static void COMM_WaitCommEventService(async_private *ovp)
{
    async_commio *commio = (async_commio*) ovp;
    IO_STATUS_BLOCK* iosb = commio->async.iosb;

    TRACE("iosb %p\n",iosb);

    /* FIXME: detect other events */
    *commio->buffer = EV_RXCHAR;

    iosb->u.Status = STATUS_SUCCESS;
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
    int fd;
    async_commio *ovp;

    if(!lpOverlapped)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(NtResetEvent(lpOverlapped->hEvent,NULL))
        return FALSE;

    fd = FILE_GetUnixHandle( hFile, GENERIC_WRITE );
    if(fd<0)
	return FALSE;

    ovp = (async_commio*) HeapAlloc(GetProcessHeap(), 0, sizeof (async_commio));
    if(!ovp)
    {
        close(fd);
        return FALSE;
    }

    ovp->async.ops = &commio_async_ops;
    ovp->async.handle = hFile;
    ovp->async.fd = fd;
    ovp->async.type = ASYNC_TYPE_WAIT;
    ovp->async.func = COMM_WaitCommEventService;
    ovp->async.event = lpOverlapped->hEvent;
    ovp->async.iosb = (IO_STATUS_BLOCK*)lpOverlapped;
    ovp->buffer = (char *)lpdwEvents;

    lpOverlapped->InternalHigh = 0;
    lpOverlapped->Offset = 0;
    lpOverlapped->OffsetHigh = 0;

    if ( !register_new_async (&ovp->async) )
        SetLastError( ERROR_IO_PENDING );

    return FALSE;
}

/***********************************************************************
 *           WaitCommEvent   (KERNEL32.@)
 *
 * Wait until something interesting happens on a COMM port.
 * Interesting things (events) are set by calling SetCommMask before
 * this function is called.
 *
 * RETURNS:
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
    int ret;

    TRACE("(%p %p %p )\n",hFile, lpdwEvents,lpOverlapped);

    if(lpOverlapped)
        return COMM_WaitCommEvent(hFile, lpdwEvents, lpOverlapped);

    /* if there is no overlapped structure, create our own */
    ov.hEvent = CreateEventA(NULL,FALSE,FALSE,NULL);

    COMM_WaitCommEvent(hFile, lpdwEvents, &ov);

    /* wait for the overlapped to complete */
    ret = GetOverlappedResult(hFile, &ov, NULL, TRUE);
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
static CHAR lpszSerialUI[] = "serialui.dll";


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
    HANDLE hWnd,               /* [in] parent window for the dialog */
    LPCOMMCONFIG lpCommConfig) /* [out] pointer to struct to fill */
{
    FARPROC lpfnCommDialog;
    HMODULE hConfigModule;
    BOOL r;

    TRACE("(%p %p %p)\n",lpszDevice, hWnd, lpCommConfig);

    hConfigModule = LoadLibraryA(lpszSerialUI);
    if(!hConfigModule)
        return FALSE;

    lpfnCommDialog = GetProcAddress(hConfigModule, (LPCSTR)3L);

    if(!lpfnCommDialog)
        return FALSE;

    r = lpfnCommDialog(lpszDevice,hWnd,lpCommConfig);

    /* UnloadLibrary(hConfigModule); */

    return r;
}

/***********************************************************************
 *           CommConfigDialogW   (KERNEL32.@)
 *
 * see CommConfigDialogA for more info
 */
BOOL WINAPI CommConfigDialogW(
    LPCWSTR lpszDevice,        /* [in] name of communications device */
    HANDLE hWnd,               /* [in] parent window for the dialog */
    LPCOMMCONFIG lpCommConfig) /* [out] pointer to struct to fill */
{
    BOOL r;
    LPSTR lpDeviceA;

    lpDeviceA = HEAP_strdupWtoA( GetProcessHeap(), 0, lpszDevice );
    if(lpDeviceA)
        return FALSE;
    r = CommConfigDialogA(lpDeviceA,hWnd,lpCommConfig);
    HeapFree( GetProcessHeap(), 0, lpDeviceA );
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

    r = *lpdwSize < sizeof(COMMCONFIG);
    *lpdwSize = sizeof(COMMCONFIG);
    if(!r)
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
BOOL WINAPI SetDefaultCommConfigA(
    LPCSTR       lpszDevice,   /* [in] The ascii name of the device targeted for configuration. */
    LPCOMMCONFIG lpCommConfig, /* [in] The default configuration for the device. */
    DWORD        dwSize)       /* [in] The number of bytes in the configuration structure. */
{
    FARPROC lpfnSetDefaultCommConfig;
    HMODULE hConfigModule;
    BOOL r;

    TRACE("(%p %p %lx)\n",lpszDevice, lpCommConfig, dwSize);

    hConfigModule = LoadLibraryA(lpszSerialUI);
    if(!hConfigModule)
        return FALSE;

    lpfnSetDefaultCommConfig = GetProcAddress(hConfigModule, (LPCSTR)4L);

    if(! lpfnSetDefaultCommConfig)
	return TRUE;

    r = lpfnSetDefaultCommConfig(lpszDevice, lpCommConfig, dwSize);

    /* UnloadLibrary(hConfigModule); */

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
BOOL WINAPI SetDefaultCommConfigW(
    LPCWSTR      lpszDevice,   /* [in] The unicode name of the device targeted for configuration. */
    LPCOMMCONFIG lpCommConfig, /* [in] The default configuration for the device. */
    DWORD        dwSize)       /* [in] The number of bytes in the configuration structure. */
{
    BOOL r;
    LPSTR lpDeviceA;

    TRACE("(%s %p %lx)\n",debugstr_w(lpszDevice),lpCommConfig,dwSize);

    lpDeviceA = HEAP_strdupWtoA( GetProcessHeap(), 0, lpszDevice );
    if(lpDeviceA)
        return FALSE;
    r = SetDefaultCommConfigA(lpDeviceA,lpCommConfig,dwSize);
    HeapFree( GetProcessHeap(), 0, lpDeviceA );
    return r;
}


/***********************************************************************
 *           GetDefaultCommConfigA   (KERNEL32.@)
 *
 *   Acquires the default configuration of the specified communication device. (unicode)
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
     LPDCB lpdcb = &(lpCC->dcb);
     char  temp[40];

     if (strncasecmp(lpszName,"COM",3)) {
        ERR("not implemented for <%s>\n", lpszName);
        return FALSE;
     }

     TRACE("(%s %p %ld)\n", lpszName, lpCC, *lpdwSize );
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

     sprintf( temp, "COM%c:38400,n,8,1", lpszName[3]);
     FIXME("setting %s as default\n", temp);

     return BuildCommDCBA( temp, lpdcb);
}

/**************************************************************************
 *         GetDefaultCommConfigW		(KERNEL32.@)
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
	BOOL ret = FALSE;
	LPSTR	lpszNameA;

	TRACE("(%p,%p,%ld)\n",lpszName,lpCC,*lpdwSize);
	lpszNameA = HEAP_strdupWtoA( GetProcessHeap(), 0, lpszName );
	if (lpszNameA)
	{
	ret=GetDefaultCommConfigA(lpszNameA,lpCC,lpdwSize);
        HeapFree( GetProcessHeap(), 0, lpszNameA );
	}
	return ret;
}
