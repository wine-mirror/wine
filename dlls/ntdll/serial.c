/* Main file for COMM support
 *
 * DEC 93 Erik Bos <erik@xs4all.nl>
 * Copyright 1996 Marcus Meissner
 * Copyright 2005 Eric Pouech
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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_IO_H
# include <io.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#include <fcntl.h>
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
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
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/ntddser.h"
#include "ntdll_misc.h"
#include "wine/server.h"
#include "wine/library.h"
#include "wine/debug.h"

#ifdef HAVE_LINUX_SERIAL_H
#include <linux/serial.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(comm);

static const char* iocode2str(DWORD ioc)
{
    switch (ioc)
    {
#define X(x)    case (x): return #x;
        X(IOCTL_SERIAL_CLEAR_STATS);
        X(IOCTL_SERIAL_CLR_DTR);
        X(IOCTL_SERIAL_CLR_RTS);
        X(IOCTL_SERIAL_CONFIG_SIZE);
        X(IOCTL_SERIAL_GET_BAUD_RATE);
        X(IOCTL_SERIAL_GET_CHARS);
        X(IOCTL_SERIAL_GET_COMMSTATUS);
        X(IOCTL_SERIAL_GET_DTRRTS);
        X(IOCTL_SERIAL_GET_HANDFLOW);
        X(IOCTL_SERIAL_GET_LINE_CONTROL);
        X(IOCTL_SERIAL_GET_MODEM_CONTROL);
        X(IOCTL_SERIAL_GET_MODEMSTATUS);
        X(IOCTL_SERIAL_GET_PROPERTIES);
        X(IOCTL_SERIAL_GET_STATS);
        X(IOCTL_SERIAL_GET_TIMEOUTS);
        X(IOCTL_SERIAL_GET_WAIT_MASK);
        X(IOCTL_SERIAL_IMMEDIATE_CHAR);
        X(IOCTL_SERIAL_LSRMST_INSERT);
        X(IOCTL_SERIAL_PURGE);
        X(IOCTL_SERIAL_RESET_DEVICE);
        X(IOCTL_SERIAL_SET_BAUD_RATE);
        X(IOCTL_SERIAL_SET_BREAK_ON);
        X(IOCTL_SERIAL_SET_BREAK_OFF);
        X(IOCTL_SERIAL_SET_CHARS);
        X(IOCTL_SERIAL_SET_DTR);
        X(IOCTL_SERIAL_SET_FIFO_CONTROL);
        X(IOCTL_SERIAL_SET_HANDFLOW);
        X(IOCTL_SERIAL_SET_LINE_CONTROL);
        X(IOCTL_SERIAL_SET_MODEM_CONTROL);
        X(IOCTL_SERIAL_SET_QUEUE_SIZE);
        X(IOCTL_SERIAL_SET_RTS);
        X(IOCTL_SERIAL_SET_TIMEOUTS);
        X(IOCTL_SERIAL_SET_WAIT_MASK);
        X(IOCTL_SERIAL_SET_XOFF);
        X(IOCTL_SERIAL_SET_XON);
        X(IOCTL_SERIAL_WAIT_ON_MASK);
        X(IOCTL_SERIAL_XOFF_COUNTER);
#undef X
    default: { static char tmp[32]; sprintf(tmp, "IOCTL_SERIAL_%ld\n", ioc); return tmp; }
    }
}

static NTSTATUS purge(int fd, DWORD flags)
{
    /*
    ** not exactly sure how these are different
    ** Perhaps if we had our own internal queues, one flushes them
    ** and the other flushes the kernel's buffers.
    */
    if (flags & PURGE_TXABORT) tcflush(fd, TCOFLUSH);
    if (flags & PURGE_RXABORT) tcflush(fd, TCIFLUSH);
    if (flags & PURGE_TXCLEAR) tcflush(fd, TCOFLUSH);
    if (flags & PURGE_RXCLEAR) tcflush(fd, TCIFLUSH);
    return STATUS_SUCCESS;
}

/******************************************************************
 *		COMM_DeviceIoControl
 *
 *
 */
NTSTATUS COMM_DeviceIoControl(HANDLE hDevice, 
                              HANDLE hEvent, PIO_APC_ROUTINE UserApcRoutine,
                              PVOID UserApcContext, 
                              PIO_STATUS_BLOCK piosb, 
                              ULONG dwIoControlCode,
                              LPVOID lpInBuffer, DWORD nInBufferSize,
                              LPVOID lpOutBuffer, DWORD nOutBufferSize)
{
    DWORD       sz = 0, access = FILE_READ_DATA;
    NTSTATUS    status = STATUS_SUCCESS;
    int         fd;

    TRACE("%p %s %p %ld %p %ld %p\n",
          hDevice, iocode2str(dwIoControlCode), lpInBuffer, nInBufferSize,
          lpOutBuffer, nOutBufferSize, piosb);

    piosb->Information = 0;

    if ((status = wine_server_handle_to_fd( hDevice, access, &fd, NULL ))) goto error;

    switch (dwIoControlCode)
    {
    case IOCTL_SERIAL_PURGE:
        if (lpInBuffer && nInBufferSize == sizeof(DWORD))
            status = purge(fd, *(DWORD*)lpInBuffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_SET_BREAK_OFF:
#if defined(TIOCSBRK) && defined(TIOCCBRK) /* check if available for compilation */
	if (ioctl(fd, TIOCCBRK, 0) == -1)
        {
            TRACE("ioctl failed\n");
            status = FILE_GetNtStatus();
        }
#else
	FIXME("ioctl not available\n");
	status = STATUS_NOT_SUPPORTED;
#endif
        break;
    case IOCTL_SERIAL_SET_BREAK_ON:
#if defined(TIOCSBRK) && defined(TIOCCBRK) /* check if available for compilation */
	if (ioctl(fd, TIOCSBRK, 0) == -1)
        {
            TRACE("ioctl failed\n");
            status = FILE_GetNtStatus();
        }
#else
	FIXME("ioctl not available\n");
	status = STATUS_NOT_SUPPORTED;
#endif
        break;
    default:
        FIXME("Unsupported IOCTL %lx (type=%lx access=%lx func=%lx meth=%lx)\n", 
              dwIoControlCode, dwIoControlCode >> 16, (dwIoControlCode >> 14) & 3,
              (dwIoControlCode >> 2) & 0xFFF, dwIoControlCode & 3);
        sz = 0;
        status = STATUS_INVALID_PARAMETER;
        break;
    }
    wine_server_release_fd( hDevice, fd );
 error:
    piosb->u.Status = status;
    piosb->Information = sz;
    if (hEvent) NtSetEvent(hEvent, NULL);
    return status;
}
