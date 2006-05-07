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

static NTSTATUS get_modem_status(int fd, DWORD* lpModemStat)
{
    NTSTATUS    status = STATUS_SUCCESS;
    int         mstat;

#ifdef TIOCMGET
    if (ioctl(fd, TIOCMGET, &mstat) == -1)
    {
        WARN("ioctl failed\n");
        status = FILE_GetNtStatus();
    }
    else
    {
        *lpModemStat = 0;
#ifdef TIOCM_CTS
        if (mstat & TIOCM_CTS)  *lpModemStat |= MS_CTS_ON;
#endif
#ifdef TIOCM_DSR
        if (mstat & TIOCM_DSR)  *lpModemStat |= MS_DSR_ON;
#endif
#ifdef TIOCM_RNG
        if (mstat & TIOCM_RNG)  *lpModemStat |= MS_RING_ON;
#endif
#ifdef TIOCM_CAR
        /* FIXME: Not really sure about RLSD UB 990810 */
        if (mstat & TIOCM_CAR)  *lpModemStat |= MS_RLSD_ON;
#endif
        TRACE("%04x -> %s%s%s%s\n", mstat,
              (*lpModemStat & MS_RLSD_ON) ? "MS_RLSD_ON " : "",
              (*lpModemStat & MS_RING_ON) ? "MS_RING_ON " : "",
              (*lpModemStat & MS_DSR_ON)  ? "MS_DSR_ON  " : "",
              (*lpModemStat & MS_CTS_ON)  ? "MS_CTS_ON  " : "");
    }
#else
    status = STATUS_NOT_SUPPORTED;
#endif
    return status;
}

static NTSTATUS get_status(int fd, SERIAL_STATUS* ss)
{
    NTSTATUS    status = STATUS_SUCCESS;

    ss->Errors = 0;
    ss->HoldReasons = 0;
    ss->EofReceived = FALSE;
    ss->WaitForImmediate = FALSE;
#ifdef TIOCOUTQ
    if (ioctl(fd, TIOCOUTQ, &ss->AmountInOutQueue) == -1)
    {
        WARN("ioctl returned error\n");
        status = FILE_GetNtStatus();
    }
#else
    ss->AmountInOutQueue = 0; /* FIXME: find a different way to find out */
#endif

#ifdef TIOCINQ
    if (ioctl(fd, TIOCINQ, &ss->AmountInInQueue))
    {
        WARN("ioctl returned error\n");
        status = FILE_GetNtStatus();
    }
#else
    ss->AmountInInQueue = 0; /* FIXME: find a different way to find out */
#endif
    return status;
}

static NTSTATUS get_wait_mask(HANDLE hDevice, DWORD* mask)
{
    NTSTATUS    status;

    SERVER_START_REQ( get_serial_info )
    {
        req->handle = hDevice;
        if (!(status = wine_server_call( req )))
            *mask = reply->eventmask;
    }
    SERVER_END_REQ;
    return status;
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

static NTSTATUS set_baud_rate(int fd, const SERIAL_BAUD_RATE* sbr)
{
    struct termios port;

    if (tcgetattr(fd, &port) == -1)
    {
        ERR("tcgetattr error '%s'\n", strerror(errno));
        return FILE_GetNtStatus();
    }

#ifdef CBAUD
    port.c_cflag &= ~CBAUD;
    switch (sbr->BaudRate)
    {
    case 0:             port.c_cflag |= B0;     break;
    case 50:            port.c_cflag |= B50;    break;
    case 75:            port.c_cflag |= B75;    break;
    case 110:   
    case CBR_110:       port.c_cflag |= B110;   break;
    case 134:           port.c_cflag |= B134;   break;
    case 150:           port.c_cflag |= B150;   break;
    case 200:           port.c_cflag |= B200;   break;
    case 300:           
    case CBR_300:       port.c_cflag |= B300;   break;
    case 600:
    case CBR_600:       port.c_cflag |= B600;   break;
    case 1200:
    case CBR_1200:	port.c_cflag |= B1200;  break;
    case 1800:		port.c_cflag |= B1800;	break;
    case 2400:
    case CBR_2400:	port.c_cflag |= B2400;	break;
    case 4800:
    case CBR_4800:	port.c_cflag |= B4800;	break;
    case 9600:
    case CBR_9600:	port.c_cflag |= B9600;	break;
    case 19200:
    case CBR_19200:	port.c_cflag |= B19200;	break;
    case 38400:
    case CBR_38400:	port.c_cflag |= B38400;	break;
#ifdef B57600
    case 57600:		port.c_cflag |= B57600;	break;
#endif
#ifdef B115200
    case 115200:	port.c_cflag |= B115200;break;
#endif
#ifdef B230400
    case 230400:	port.c_cflag |= B230400;break;
#endif
#ifdef B460800
    case 460800:	port.c_cflag |= B460800;break;
#endif
    default:
#if defined (HAVE_LINUX_SERIAL_H) && defined (TIOCSSERIAL)
        {
            struct serial_struct nuts;
            int arby;
	
            ioctl(fd, TIOCGSERIAL, &nuts);
            nuts.custom_divisor = nuts.baud_base / sbr->BaudRate;
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
                 "reset it and it will probably recover.\n", sbr->BaudRate, arby);
            ioctl(fd, TIOCSSERIAL, &nuts);
            port.c_cflag |= B38400;
        }
        break;
#endif    /* Don't have linux/serial.h or lack TIOCSSERIAL */
        ERR("baudrate %ld\n", sbr->BaudRate);
        return STATUS_NOT_SUPPORTED;
    }
#elif !defined(__EMX__)
    switch (sbr->BaudRate)
    {
    case 0:             port.c_ospeed = B0;     break;
    case 50:            port.c_ospeed = B50;    break;
    case 75:            port.c_ospeed = B75;    break;
    case 110:
    case CBR_110:       port.c_ospeed = B110;   break;
    case 134:           port.c_ospeed = B134;   break;
    case 150:           port.c_ospeed = B150;   break;
    case 200:           port.c_ospeed = B200;   break;
    case 300:
    case CBR_300:       port.c_ospeed = B300;   break;
    case 600:
    case CBR_600:       port.c_ospeed = B600;   break;
    case 1200:
    case CBR_1200:      port.c_ospeed = B1200;  break;
    case 1800:          port.c_ospeed = B1800;  break;
    case 2400:
    case CBR_2400:      port.c_ospeed = B2400;  break;
    case 4800:
    case CBR_4800:      port.c_ospeed = B4800;  break;
    case 9600:
    case CBR_9600:      port.c_ospeed = B9600;  break;
    case 19200:
    case CBR_19200:     port.c_ospeed = B19200; break;
    case 38400:
    case CBR_38400:     port.c_ospeed = B38400; break;
#ifdef B57600
    case 57600:
    case CBR_57600:     port.c_cflag |= B57600; break;
#endif
#ifdef B115200
    case 115200:
    case CBR_115200:    port.c_cflag |= B115200;break;
#endif
#ifdef B230400
    case 230400:	port.c_cflag |= B230400;break;
#endif
#ifdef B460800
    case 460800:	port.c_cflag |= B460800;break;
#endif
    default:
        ERR("baudrate %ld\n", sbr->BaudRate);
        return STATUS_NOT_SUPPORTED;
    }
    port.c_ispeed = port.c_ospeed;
#endif
    if (tcsetattr(fd, TCSANOW, &port) == -1)
    {
        ERR("tcsetattr error '%s'\n", strerror(errno));
        return FILE_GetNtStatus();
    }
    return STATUS_SUCCESS;
}

static NTSTATUS set_line_control(int fd, const SERIAL_LINE_CONTROL* slc)
{
    struct termios port;
    unsigned bytesize, stopbits;
    
    if (tcgetattr(fd, &port) == -1)
    {
        ERR("tcgetattr error '%s'\n", strerror(errno));
        return FILE_GetNtStatus();
    }
    
#ifdef IMAXBEL
    port.c_iflag &= ~(ISTRIP|BRKINT|IGNCR|ICRNL|INLCR|PARMRK|IMAXBEL);
#else
    port.c_iflag &= ~(ISTRIP|BRKINT|IGNCR|ICRNL|INLCR|PARMRK);
#endif
    port.c_iflag |= IGNBRK | INPCK;
    
    port.c_oflag &= ~(OPOST);
    
    port.c_cflag &= ~(HUPCL);
    port.c_cflag |= CLOCAL | CREAD;
    
    port.c_lflag &= ~(ICANON|ECHO|ISIG);
    port.c_lflag |= NOFLSH;
    
    bytesize = slc->WordLength;
    stopbits = slc->StopBits;
    
#ifdef CMSPAR
    port.c_cflag &= ~(PARENB | PARODD | CMSPAR);
#else
    port.c_cflag &= ~(PARENB | PARODD);
#endif

    switch (slc->Parity)
    {
    case NOPARITY:      port.c_iflag &= ~INPCK;                         break;
    case ODDPARITY:     port.c_cflag |= PARENB | PARODD;                break;
    case EVENPARITY:    port.c_cflag |= PARENB;                         break;
#ifdef CMSPAR
        /* Linux defines mark/space (stick) parity */
    case MARKPARITY:    port.c_cflag |= PARENB | CMSPAR;                break;
    case SPACEPARITY:   port.c_cflag |= PARENB | PARODD |  CMSPAR;      break;
#else
        /* try the POSIX way */
    case MARKPARITY:
        if (slc->StopBits == ONESTOPBIT)
        {
            stopbits = TWOSTOPBITS;
            port.c_iflag &= ~INPCK;
        }
        else
        {
            ERR("Cannot set MARK Parity\n");
            return STATUS_NOT_SUPPORTED;
        }
        break;
    case SPACEPARITY:
        if (slc->WordLength < 8)
        {
            bytesize +=1;
            port.c_iflag &= ~INPCK;
        }
        else
        {
            ERR("Cannot set SPACE Parity\n");
            return STATUS_NOT_SUPPORTED;
        }
        break;
#endif
    default:
        ERR("Parity\n");
        return STATUS_NOT_SUPPORTED;
    }
    
    port.c_cflag &= ~CSIZE;
    switch (bytesize)
    {
    case 5:     port.c_cflag |= CS5;    break;
    case 6:     port.c_cflag |= CS6;    break;
    case 7:	port.c_cflag |= CS7;	break;
    case 8:	port.c_cflag |= CS8;	break;
    default:
        ERR("ByteSize\n");
        return STATUS_NOT_SUPPORTED;
    }
    
    switch (stopbits)
    {
    case ONESTOPBIT:    port.c_cflag &= ~CSTOPB;        break;
    case ONE5STOPBITS: /* will be selected if bytesize is 5 */
    case TWOSTOPBITS:	port.c_cflag |= CSTOPB;		break;
    default:
        ERR("StopBits\n");
        return STATUS_NOT_SUPPORTED;
    }
    /* otherwise it hangs with pending input*/
    if (tcsetattr(fd, TCSANOW, &port) == -1)
    {
        ERR("tcsetattr error '%s'\n", strerror(errno));
        return FILE_GetNtStatus();
    }
    return STATUS_SUCCESS;
}

static NTSTATUS set_wait_mask(HANDLE hDevice, DWORD mask)
{
    NTSTATUS status;

    SERVER_START_REQ( set_serial_info )
    {
        req->handle    = hDevice;
        req->flags     = SERIALINFO_SET_MASK;
        req->eventmask = mask;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    return status;
}

static NTSTATUS xmit_immediate(HANDLE hDevice, int fd, char* ptr)
{
    /* FIXME: not perfect as it should bypass the in-queue */
    WARN("(%p,'%c') not perfect!\n", hDevice, *ptr);
    if (write(fd, ptr, 1) != 1)
        return FILE_GetNtStatus();
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
    case IOCTL_SERIAL_GET_COMMSTATUS:
        if (lpOutBuffer && nOutBufferSize == sizeof(SERIAL_STATUS))
        {
            if (!(status = get_status(fd, (SERIAL_STATUS*)lpOutBuffer)))
                sz = sizeof(SERIAL_STATUS);
        }
        else status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_GET_MODEMSTATUS:
        if (lpOutBuffer && nOutBufferSize == sizeof(DWORD))
        {
            if (!(status = get_modem_status(fd, (DWORD*)lpOutBuffer)))
                sz = sizeof(DWORD);
        }
        else status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_GET_WAIT_MASK:
        if (lpOutBuffer && nOutBufferSize == sizeof(DWORD))
        {
            if (!(status = get_wait_mask(hDevice, (DWORD*)lpOutBuffer)))
                sz = sizeof(DWORD);
        }
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_IMMEDIATE_CHAR:
        if (lpInBuffer && nInBufferSize == sizeof(CHAR))
            status = xmit_immediate(hDevice, fd, lpInBuffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_PURGE:
        if (lpInBuffer && nInBufferSize == sizeof(DWORD))
            status = purge(fd, *(DWORD*)lpInBuffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_SET_BAUD_RATE:
        if (lpInBuffer && nInBufferSize == sizeof(SERIAL_BAUD_RATE))
            status = set_baud_rate(fd, (const SERIAL_BAUD_RATE*)lpInBuffer);
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
    case IOCTL_SERIAL_SET_LINE_CONTROL:
        if (lpInBuffer && nInBufferSize == sizeof(SERIAL_LINE_CONTROL))
            status = set_line_control(fd, (const SERIAL_LINE_CONTROL*)lpInBuffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_SET_WAIT_MASK:
        if (lpInBuffer && nInBufferSize == sizeof(DWORD))
        {
            status = set_wait_mask(hDevice, *(DWORD*)lpInBuffer);
        }
        else status = STATUS_INVALID_PARAMETER;
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
