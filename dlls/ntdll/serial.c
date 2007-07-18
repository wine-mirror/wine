/* Main file for COMM support
 *
 * DEC 93 Erik Bos <erik@xs4all.nl>
 * Copyright 1996 Marcus Meissner
 * Copyright 2005,2006 Eric Pouech
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

#if !defined(TIOCINQ) && defined(FIONREAD)
#define	TIOCINQ FIONREAD
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
    default: { static char tmp[32]; sprintf(tmp, "IOCTL_SERIAL_%d\n", ioc); return tmp; }
    }
}

static NTSTATUS get_baud_rate(int fd, SERIAL_BAUD_RATE* sbr)
{
    struct termios port;
    int speed;
    
    if (tcgetattr(fd, &port) == -1)
    {
        ERR("tcgetattr error '%s'\n", strerror(errno));
        return FILE_GetNtStatus();
    }
#ifndef __EMX__
#ifdef CBAUD
    speed = port.c_cflag & CBAUD;
#else
    speed = cfgetospeed(&port);
#endif
    switch (speed)
    {
    case B0:            sbr->BaudRate = 0;      break;
    case B50:           sbr->BaudRate = 50;	break;
    case B75:		sbr->BaudRate = 75;	break;
    case B110:		sbr->BaudRate = 110;	break;
    case B134:		sbr->BaudRate = 134;	break;
    case B150:		sbr->BaudRate = 150;	break;
    case B200:		sbr->BaudRate = 200;	break;
    case B300:		sbr->BaudRate = 300;	break;
    case B600:		sbr->BaudRate = 600;	break;
    case B1200:		sbr->BaudRate = 1200;	break;
    case B1800:		sbr->BaudRate = 1800;	break;
    case B2400:		sbr->BaudRate = 2400;	break;
    case B4800:		sbr->BaudRate = 4800;	break;
    case B9600:		sbr->BaudRate = 9600;	break;
    case B19200:	sbr->BaudRate = 19200;	break;
    case B38400:	sbr->BaudRate = 38400;	break;
#ifdef B57600
    case B57600:	sbr->BaudRate = 57600;	break;
#endif
#ifdef B115200
    case B115200:	sbr->BaudRate = 115200;	break;
#endif
#ifdef B230400
    case B230400:	sbr->BaudRate = 230400;	break;
#endif
#ifdef B460800
    case B460800:	sbr->BaudRate = 460800;	break;
#endif
    default:
        ERR("unknown speed %x\n", speed);
        return STATUS_INVALID_PARAMETER;
    }
#else
    return STATUS_INVALID_PARAMETER;
#endif
    return STATUS_SUCCESS;
}

static NTSTATUS get_hand_flow(int fd, SERIAL_HANDFLOW* shf)
{
    int stat;
    struct termios port;
    
    if (tcgetattr(fd, &port) == -1)
    {
        ERR("tcgetattr error '%s'\n", strerror(errno));
        return FILE_GetNtStatus();
    }
#ifdef TIOCMGET
    if (ioctl(fd, TIOCMGET, &stat) == -1)
    {
        WARN("ioctl error '%s'\n", strerror(errno));
        stat = DTR_CONTROL_ENABLE | RTS_CONTROL_ENABLE;
    }
#endif
    /* termios does not support DTR/DSR flow control */
    shf->ControlHandShake = 0;
    shf->FlowReplace = 0;
#ifdef TIOCM_DTR
    if (stat & TIOCM_DTR)
#endif
        shf->ControlHandShake |= SERIAL_DTR_CONTROL;
#ifdef CRTSCTS
    if (port.c_cflag & CRTSCTS)
    {
        shf->ControlHandShake |= SERIAL_DTR_CONTROL | SERIAL_DTR_HANDSHAKE;
        shf->ControlHandShake |= SERIAL_CTS_HANDSHAKE;
    }
    else
#endif
    {
#ifdef TIOCM_RTS
        if (stat & TIOCM_RTS)
#endif
            shf->ControlHandShake |= SERIAL_RTS_CONTROL;
    }
    if (port.c_iflag & IXON)
        shf->FlowReplace |= SERIAL_AUTO_RECEIVE;
    if (port.c_iflag & IXOFF)
        shf->FlowReplace |= SERIAL_AUTO_TRANSMIT;

    shf->XonLimit = 10;
    shf->XoffLimit = 10;
    return STATUS_SUCCESS;
}

static NTSTATUS get_line_control(int fd, SERIAL_LINE_CONTROL* slc)
{
    struct termios port;
    
    if (tcgetattr(fd, &port) == -1)
    {
        ERR("tcgetattr error '%s'\n", strerror(errno));
        return FILE_GetNtStatus();
    }
    
#ifdef CMSPAR
    switch (port.c_cflag & (PARENB | PARODD | CMSPAR))
#else
    switch (port.c_cflag & (PARENB | PARODD))
#endif
    {
    case 0:                     slc->Parity = NOPARITY;         break;
    case PARENB:                slc->Parity = EVENPARITY;       break;
    case PARENB|PARODD:         slc->Parity = ODDPARITY;        break;
#ifdef CMSPAR
    case PARENB|CMSPAR:         slc->Parity = MARKPARITY;       break;
    case PARENB|PARODD|CMSPAR:  slc->Parity = SPACEPARITY;      break;
        break;
#endif
    }
    switch (port.c_cflag & CSIZE)
    {
    case CS5:   slc->WordLength = 5;    break;
    case CS6:   slc->WordLength = 6;    break;
    case CS7:   slc->WordLength = 7;	break;
    case CS8:	slc->WordLength = 8;	break;
    default: ERR("unknown size %x\n", (UINT)(port.c_cflag & CSIZE));
    }
    
    if (port.c_cflag & CSTOPB)
    {
        if (slc->WordLength == 5)
            slc->StopBits = ONE5STOPBITS;
        else
            slc->StopBits = TWOSTOPBITS;
    }
    else
        slc->StopBits = ONESTOPBIT;

    return STATUS_SUCCESS;
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

static NTSTATUS get_special_chars(int fd, SERIAL_CHARS* sc)
{
    struct termios port;
    
    if (tcgetattr(fd, &port) == -1)
    {
        ERR("tcgetattr error '%s'\n", strerror(errno));
        return FILE_GetNtStatus();
    }
    sc->EofChar   = port.c_cc[VEOF];
    sc->ErrorChar = 0xFF;
    sc->BreakChar = 0; /* FIXME */
    sc->EventChar = 0; /* FIXME */
    sc->XonChar   = port.c_cc[VSTART];
    sc->XoffChar  = port.c_cc[VSTOP];

    return STATUS_SUCCESS;
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

static NTSTATUS get_timeouts(HANDLE handle, SERIAL_TIMEOUTS* st)
{
    NTSTATUS    status;
    SERVER_START_REQ( get_serial_info )
    {
        req->handle = handle;
        if (!(status = wine_server_call( req )))
        {
            st->ReadIntervalTimeout         = reply->readinterval;
            st->ReadTotalTimeoutMultiplier  = reply->readmult;
            st->ReadTotalTimeoutConstant    = reply->readconst;
            st->WriteTotalTimeoutMultiplier = reply->writemult;
            st->WriteTotalTimeoutConstant   = reply->writeconst;
        }
    }
    SERVER_END_REQ;
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
                 "a non-standard baud rate %d.  Wine will set the rate to %d,\n"
                 "which is as close as we can get by our present understanding of your\n"
                 "hardware. I hope you know what you are doing.  Any disruption Wine\n"
                 "has caused to your linux system can be undone with setserial \n"
                 "(see man setserial). If you have incapacitated a Hayes type modem,\n"
                 "reset it and it will probably recover.\n", sbr->BaudRate, arby);
            ioctl(fd, TIOCSSERIAL, &nuts);
            port.c_cflag |= B38400;
        }
        break;
#else     /* Don't have linux/serial.h or lack TIOCSSERIAL */
        ERR("baudrate %d\n", sbr->BaudRate);
        return STATUS_NOT_SUPPORTED;
#endif    /* Don't have linux/serial.h or lack TIOCSSERIAL */
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
        ERR("baudrate %d\n", sbr->BaudRate);
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

static int whack_modem(int fd, unsigned int andy, unsigned int orrie)
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

static NTSTATUS set_handflow(int fd, const SERIAL_HANDFLOW* shf)
{
    struct termios port;

    if ((shf->FlowReplace & (SERIAL_RTS_CONTROL | SERIAL_RTS_HANDSHAKE)) == 
        (SERIAL_RTS_CONTROL | SERIAL_RTS_HANDSHAKE))
        return STATUS_NOT_SUPPORTED;

    if (tcgetattr(fd, &port) == -1)
    {
        ERR("tcgetattr error '%s'\n", strerror(errno));
        return FILE_GetNtStatus();
    }
    
#ifdef CRTSCTS
    if ((shf->ControlHandShake & SERIAL_CTS_HANDSHAKE) ||
        (shf->FlowReplace & SERIAL_RTS_HANDSHAKE))
    {
        port.c_cflag |= CRTSCTS;
        TRACE("CRTSCTS\n");
    }
    else
        port.c_cflag &= ~CRTSCTS;
#endif
#ifdef TIOCM_DTR
    if (shf->ControlHandShake & SERIAL_DTR_HANDSHAKE)
    {
        WARN("DSR/DTR flow control not supported\n");
    } else if (!(shf->ControlHandShake & SERIAL_DTR_CONTROL))
        whack_modem(fd, ~TIOCM_DTR, 0);
    else
        whack_modem(fd, 0, TIOCM_DTR);
#endif
#ifdef TIOCM_RTS
    if (!(shf->ControlHandShake & SERIAL_CTS_HANDSHAKE))
    {
        if ((shf->FlowReplace & (SERIAL_RTS_CONTROL|SERIAL_RTS_HANDSHAKE)) == 0)
            whack_modem(fd, ~TIOCM_RTS, 0);
        else    
            whack_modem(fd, 0, TIOCM_RTS);
    }
#endif

    if (shf->FlowReplace & SERIAL_AUTO_RECEIVE)
        port.c_iflag |= IXON;
    else
        port.c_iflag &= ~IXON;
    if (shf->FlowReplace & SERIAL_AUTO_TRANSMIT)
        port.c_iflag |= IXOFF;
    else
        port.c_iflag &= ~IXOFF;
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

static NTSTATUS set_queue_size(int fd, const SERIAL_QUEUE_SIZE* sqs)
{
    FIXME("insize %d outsize %d unimplemented stub\n", sqs->InSize, sqs->OutSize);
    return STATUS_SUCCESS;
}

static NTSTATUS set_special_chars(int fd, const SERIAL_CHARS* sc)
{
    struct termios port;
    
    if (tcgetattr(fd, &port) == -1)
    {
        ERR("tcgetattr error '%s'\n", strerror(errno));
        return FILE_GetNtStatus();
    }
    
    port.c_cc[VMIN  ] = 0;
    port.c_cc[VTIME ] = 1;
    
    port.c_cc[VEOF  ] = sc->EofChar;
    /* FIXME: sc->ErrorChar is not supported */
    /* FIXME: sc->BreakChar is not supported */
    /* FIXME: sc->EventChar is not supported */
    port.c_cc[VSTART] = sc->XonChar;
    port.c_cc[VSTOP ] = sc->XoffChar;
    
    if (tcsetattr(fd, TCSANOW, &port) == -1)
    {
        ERR("tcsetattr error '%s'\n", strerror(errno));
        return FILE_GetNtStatus();
    }
    return STATUS_SUCCESS;
}

static NTSTATUS set_timeouts(HANDLE handle, int fd, const SERIAL_TIMEOUTS* st)
{
    NTSTATUS            status;
    struct termios      port;
    unsigned int        ux_timeout;

    SERVER_START_REQ( set_serial_info )
    {
        req->handle       = handle;
        req->flags        = SERIALINFO_SET_TIMEOUTS;
        req->readinterval = st->ReadIntervalTimeout ;
        req->readmult     = st->ReadTotalTimeoutMultiplier ;
        req->readconst    = st->ReadTotalTimeoutConstant ;
        req->writemult    = st->WriteTotalTimeoutMultiplier ;
        req->writeconst   = st->WriteTotalTimeoutConstant ;
        status = wine_server_call( req );
    }
    SERVER_END_REQ;
    if (status) return status;

    if (tcgetattr(fd, &port) == -1)
    {
        FIXME("tcgetattr on fd %d failed (%s)!\n", fd, strerror(errno));
        return FILE_GetNtStatus();
    }

    /* VTIME is in 1/10 seconds */
    if (st->ReadIntervalTimeout == 0) /* 0 means no timeout */
        ux_timeout = 0;
    else
    {
        ux_timeout = (st->ReadIntervalTimeout + 99) / 100;
        if (ux_timeout == 0)
            ux_timeout = 1; /* must be at least some timeout */
    }
    port.c_cc[VTIME] = ux_timeout;

    if (tcsetattr(fd, 0, &port) == -1)
    {
        FIXME("tcsetattr on fd %d failed (%s)!\n", fd, strerror(errno));
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

static NTSTATUS set_XOff(int fd)
{
    struct termios      port;

    if (tcgetattr(fd,&port) == -1)
    {
        FIXME("tcgetattr on fd %d failed (%s)!\n", fd, strerror(errno));
        return FILE_GetNtStatus();


    }
    port.c_iflag |= IXOFF;
    if (tcsetattr(fd, TCSADRAIN, &port) == -1)
    {
        FIXME("tcsetattr on fd %d failed (%s)!\n", fd, strerror(errno));
        return FILE_GetNtStatus();
    }
    return STATUS_SUCCESS;
}

static NTSTATUS set_XOn(int fd)
{
    struct termios      port;

    if (tcgetattr(fd,&port) == -1)
    {
        FIXME("tcgetattr on fd %d failed (%s)!\n", fd, strerror(errno));
        return FILE_GetNtStatus();
    }
    port.c_iflag |= IXON;
    if (tcsetattr(fd, TCSADRAIN, &port) == -1)
    {
        FIXME("tcsetattr on fd %d failed (%s)!\n", fd, strerror(errno));
        return FILE_GetNtStatus();
    }
    return STATUS_SUCCESS;
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
    HANDLE              hDevice;
    DWORD*              events;
    HANDLE              hEvent;
    DWORD               evtmask;
    DWORD               mstat;
    serial_irq_info     irq_info;
} async_commio;

/***********************************************************************
 *   Get extended interrupt count info, needed for wait_on
 */
static NTSTATUS get_irq_info(int fd, serial_irq_info *irq_info)
{
#ifdef TIOCGICOUNT
    struct serial_icounter_struct einfo;
    if (!ioctl(fd, TIOCGICOUNT, &einfo))
    {
        irq_info->rx          = einfo.rx;
        irq_info->tx          = einfo.tx;
        irq_info->frame       = einfo.frame;
        irq_info->overrun     = einfo.overrun;
        irq_info->parity      = einfo.parity;
        irq_info->brk         = einfo.brk;
        irq_info->buf_overrun = einfo.buf_overrun;
        return STATUS_SUCCESS;
    }
    TRACE("TIOCGICOUNT err %s\n", strerror(errno));
    return FILE_GetNtStatus();
#else
    memset(irq_info,0, sizeof(serial_irq_info));
    return STATUS_NOT_IMPLEMENTED;
#endif
}


static DWORD WINAPI check_events(int fd, DWORD mask, 
                                 const serial_irq_info *new, 
                                 const serial_irq_info *old,
                                 DWORD new_mstat, DWORD old_mstat)
{
    DWORD ret = 0, queue;

    TRACE("mask 0x%08x\n", mask);
    TRACE("old->rx          0x%08x vs. new->rx          0x%08x\n", old->rx, new->rx);
    TRACE("old->tx          0x%08x vs. new->tx          0x%08x\n", old->tx, new->tx);
    TRACE("old->frame       0x%08x vs. new->frame       0x%08x\n", old->frame, new->frame);
    TRACE("old->overrun     0x%08x vs. new->overrun     0x%08x\n", old->overrun, new->overrun);
    TRACE("old->parity      0x%08x vs. new->parity      0x%08x\n", old->parity, new->parity);
    TRACE("old->brk         0x%08x vs. new->brk         0x%08x\n", old->brk, new->brk);
    TRACE("old->buf_overrun 0x%08x vs. new->buf_overrun 0x%08x\n", old->buf_overrun, new->buf_overrun);

    if (old->brk != new->brk) ret |= EV_BREAK;
    if ((old_mstat & MS_CTS_ON ) != (new_mstat & MS_CTS_ON )) ret |= EV_CTS;
    if ((old_mstat & MS_DSR_ON ) != (new_mstat & MS_DSR_ON )) ret |= EV_DSR;
    if ((old_mstat & MS_RING_ON) != (new_mstat & MS_RING_ON)) ret |= EV_RING;
    if ((old_mstat & MS_RLSD_ON) != (new_mstat & MS_RLSD_ON)) ret |= EV_RLSD;
    if (old->frame != new->frame || old->overrun != new->overrun || old->parity != new->parity) ret |= EV_ERR;
    if (mask & EV_RXCHAR)
    {
	queue = 0;
#ifdef TIOCINQ
	if (ioctl(fd, TIOCINQ, &queue))
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
	if (ioctl(fd, TIOCSERGETLSR, &queue))
	    WARN("TIOCSERGETLSR returned error\n");
	if (queue)
/* TIOCOUTQ only checks for an empty buffer */
#elif defined(TIOCOUTQ)
	if (ioctl(fd, TIOCOUTQ, &queue))
	    WARN("TIOCOUTQ returned error\n");
	if (!queue)
#endif
           ret |= EV_TXEMPTY;
	TRACE("OUTQUEUE %d, Transmitter %sempty\n",
              queue, (ret & EV_TXEMPTY) ? "" : "not ");
    }
    return ret & mask;
}

/***********************************************************************
 *             wait_for_event      (INTERNAL)
 *
 *  We need to poll for what is interesting
 *  TIOCMIWAIT only checks modem status line and may not be aborted by a changing mask
 *
 */
static DWORD CALLBACK wait_for_event(LPVOID arg)
{
    async_commio *commio = (async_commio*) arg;
    int fd, needs_close;

    if (!server_get_unix_fd( commio->hDevice, FILE_READ_DATA | FILE_WRITE_DATA, &fd, &needs_close, NULL, NULL ))
    {
        serial_irq_info new_irq_info;
        DWORD new_mstat, new_evtmask;
        LARGE_INTEGER time;
        
        TRACE("device=%p fd=0x%08x mask=0x%08x buffer=%p event=%p irq_info=%p\n", 
              commio->hDevice, fd, commio->evtmask, commio->events, commio->hEvent, &commio->irq_info);

        time.QuadPart = (ULONGLONG)10000;
        time.QuadPart = -time.QuadPart;
        for (;;)
        {
            /*
             * TIOCMIWAIT is not adequate
             *
             * FIXME:
             * We don't handle the EV_RXFLAG (the eventchar)
             */
            NtDelayExecution(FALSE, &time);
            get_irq_info(fd, &new_irq_info);
            if (get_modem_status(fd, &new_mstat))
                TRACE("get_modem_status failed\n");
            *commio->events = check_events(fd, commio->evtmask,
                                           &new_irq_info, &commio->irq_info,
                                           new_mstat, commio->mstat);
            if (*commio->events) break;
            get_wait_mask(commio->hDevice, &new_evtmask);
            if (commio->evtmask != new_evtmask)
            {
                *commio->events = 0;
                break;
            }
        }
        if (needs_close) close( fd );
    }
    if (commio->hEvent) NtSetEvent(commio->hEvent, NULL);
    RtlFreeHeap(GetProcessHeap(), 0, commio);
    return 0;
}

static NTSTATUS wait_on(HANDLE hDevice, int fd, HANDLE hEvent, DWORD* events)
{
    async_commio*       commio;
    NTSTATUS            status;

    if ((status = NtResetEvent(hEvent, NULL)))
        return status;

    commio = RtlAllocateHeap(GetProcessHeap(), 0, sizeof (async_commio));
    if (!commio) return STATUS_NO_MEMORY;

    commio->hDevice = hDevice;
    commio->events  = events;
    commio->hEvent  = hEvent;
    get_wait_mask(commio->hDevice, &commio->evtmask);

/* We may never return, if some capabilities miss
 * Return error in that case
 */
#if !defined(TIOCINQ)
    if (commio->evtmask & EV_RXCHAR)
	goto error_caps;
#endif
#if !(defined(TIOCSERGETLSR) && defined(TIOCSER_TEMT)) || !defined(TIOCINQ)
    if (commio->evtmask & EV_TXEMPTY)
	goto error_caps;
#endif
#if !defined(TIOCMGET)
    if (commio->evtmask & (EV_CTS | EV_DSR| EV_RING| EV_RLSD))
	goto error_caps;
#endif
#if !defined(TIOCM_CTS)
    if (commio->evtmask & EV_CTS)
	goto error_caps;
#endif
#if !defined(TIOCM_DSR)
    if (commio->evtmask & EV_DSR)
	goto error_caps;
#endif
#if !defined(TIOCM_RNG)
    if (commio->evtmask & EV_RING)
	goto error_caps;
#endif
#if !defined(TIOCM_CAR)
    if (commio->evtmask & EV_RLSD)
	goto error_caps;
#endif
    if (commio->evtmask & EV_RXFLAG)
	FIXME("EV_RXFLAG not handled\n");
    if ((status = get_irq_info(fd, &commio->irq_info)) ||
        (status = get_modem_status(fd, &commio->mstat)))
        goto out_now;

    /* We might have received something or the TX bufffer is delivered */
    *events = check_events(fd, commio->evtmask,
                               &commio->irq_info, &commio->irq_info,
                               commio->mstat, commio->mstat);
    if (*events) goto out_now;

    /* create the worker for the task */
    status = RtlQueueWorkItem(wait_for_event, commio, 0 /* FIXME */);
    if (status != STATUS_SUCCESS) goto out_now;
    return STATUS_PENDING;

#if !defined(TIOCINQ) || (!(defined(TIOCSERGETLSR) && defined(TIOCSER_TEMT)) || !defined(TIOCINQ)) || !defined(TIOCMGET) || !defined(TIOCM_CTS) ||!defined(TIOCM_DSR) || !defined(TIOCM_RNG) || !defined(TIOCM_CAR)
error_caps:
    FIXME("Returning error because of missing capabilities\n");
    status = STATUS_INVALID_PARAMETER;
#endif
out_now:
    RtlFreeHeap(GetProcessHeap(), 0, commio);
    return status;
}

static NTSTATUS xmit_immediate(HANDLE hDevice, int fd, const char* ptr)
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
static inline NTSTATUS io_control(HANDLE hDevice, 
                                  HANDLE hEvent, PIO_APC_ROUTINE UserApcRoutine,
                                  PVOID UserApcContext, 
                                  PIO_STATUS_BLOCK piosb, 
                                  ULONG dwIoControlCode,
                                  LPVOID lpInBuffer, DWORD nInBufferSize,
                                  LPVOID lpOutBuffer, DWORD nOutBufferSize)
{
    DWORD       sz = 0, access = FILE_READ_DATA;
    NTSTATUS    status = STATUS_SUCCESS;
    int         fd = -1, needs_close = 0;

    TRACE("%p %s %p %d %p %d %p\n",
          hDevice, iocode2str(dwIoControlCode), lpInBuffer, nInBufferSize,
          lpOutBuffer, nOutBufferSize, piosb);

    piosb->Information = 0;

    if (dwIoControlCode != IOCTL_SERIAL_GET_TIMEOUTS)
        if ((status = server_get_unix_fd( hDevice, access, &fd, &needs_close, NULL, NULL )))
            goto error;

    switch (dwIoControlCode)
    {
    case IOCTL_SERIAL_CLR_DTR:
#ifdef TIOCM_DTR
        if (whack_modem(fd, ~TIOCM_DTR, 0) == -1) status = FILE_GetNtStatus();
#else
        status = STATUS_NOT_SUPPORTED;
#endif
        break;
    case IOCTL_SERIAL_CLR_RTS:
#ifdef TIOCM_RTS
        if (whack_modem(fd, ~TIOCM_RTS, 0) == -1) status = FILE_GetNtStatus();
#else
        status = STATUS_NOT_SUPPORTED;
#endif
        break;
    case IOCTL_SERIAL_GET_BAUD_RATE:
        if (lpOutBuffer && nOutBufferSize == sizeof(SERIAL_BAUD_RATE))
        {
            if (!(status = get_baud_rate(fd, (SERIAL_BAUD_RATE*)lpOutBuffer)))
                sz = sizeof(SERIAL_BAUD_RATE);
        }
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_GET_CHARS:
        if (lpOutBuffer && nOutBufferSize == sizeof(SERIAL_CHARS))
        {
            if (!(status = get_special_chars(fd, (SERIAL_CHARS*)lpOutBuffer)))
                sz = sizeof(SERIAL_CHARS);
        }
        else
            status = STATUS_INVALID_PARAMETER;
        break;
     case IOCTL_SERIAL_GET_COMMSTATUS:
        if (lpOutBuffer && nOutBufferSize == sizeof(SERIAL_STATUS))
        {
            if (!(status = get_status(fd, (SERIAL_STATUS*)lpOutBuffer)))
                sz = sizeof(SERIAL_STATUS);
        }
        else status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_GET_HANDFLOW:
        if (lpOutBuffer && nOutBufferSize == sizeof(SERIAL_HANDFLOW))
        {
            if (!(status = get_hand_flow(fd, (SERIAL_HANDFLOW*)lpOutBuffer)))
                sz = sizeof(SERIAL_HANDFLOW);
        }
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_GET_LINE_CONTROL:
        if (lpOutBuffer && nOutBufferSize == sizeof(SERIAL_LINE_CONTROL))
        {
            if (!(status = get_line_control(fd, (SERIAL_LINE_CONTROL*)lpOutBuffer)))
                sz = sizeof(SERIAL_LINE_CONTROL);
        }
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_GET_MODEMSTATUS:
        if (lpOutBuffer && nOutBufferSize == sizeof(DWORD))
        {
            if (!(status = get_modem_status(fd, (DWORD*)lpOutBuffer)))
                sz = sizeof(DWORD);
        }
        else status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_GET_TIMEOUTS:
        if (lpOutBuffer && nOutBufferSize == sizeof(SERIAL_TIMEOUTS))
        {
            if (!(status = get_timeouts(hDevice, (SERIAL_TIMEOUTS*)lpOutBuffer)))
                sz = sizeof(SERIAL_TIMEOUTS);
        }
        else
            status = STATUS_INVALID_PARAMETER;
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
    case IOCTL_SERIAL_RESET_DEVICE:
        FIXME("Unsupported\n");
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
    case IOCTL_SERIAL_SET_CHARS:
        if (lpInBuffer && nInBufferSize == sizeof(SERIAL_CHARS))
            status = set_special_chars(fd, (const SERIAL_CHARS*)lpInBuffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_SET_DTR:
#ifdef TIOCM_DTR
        if (whack_modem(fd, 0, TIOCM_DTR) == -1) status = FILE_GetNtStatus();
#else
        status = STATUS_NOT_SUPPORTED;
#endif
        break;
    case IOCTL_SERIAL_SET_HANDFLOW:
        if (lpInBuffer && nInBufferSize == sizeof(SERIAL_HANDFLOW))
            status = set_handflow(fd, (const SERIAL_HANDFLOW*)lpInBuffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_SET_LINE_CONTROL:
        if (lpInBuffer && nInBufferSize == sizeof(SERIAL_LINE_CONTROL))
            status = set_line_control(fd, (const SERIAL_LINE_CONTROL*)lpInBuffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_SET_QUEUE_SIZE:
        if (lpInBuffer && nInBufferSize == sizeof(SERIAL_QUEUE_SIZE))
            status = set_queue_size(fd, (const SERIAL_QUEUE_SIZE*)lpInBuffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_SET_RTS:
#ifdef TIOCM_RTS
        if (whack_modem(fd, 0, TIOCM_RTS) == -1) status = FILE_GetNtStatus();
#else
        status = STATUS_NOT_SUPPORTED;
#endif
        break;
    case IOCTL_SERIAL_SET_TIMEOUTS:
        if (lpInBuffer && nInBufferSize == sizeof(SERIAL_TIMEOUTS))
            status = set_timeouts(hDevice, fd, (const SERIAL_TIMEOUTS*)lpInBuffer);
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
    case IOCTL_SERIAL_SET_XOFF:
        status = set_XOff(fd);
        break;
    case IOCTL_SERIAL_SET_XON:
        status = set_XOn(fd);
        break;
    case IOCTL_SERIAL_WAIT_ON_MASK:
        if (lpOutBuffer && nOutBufferSize == sizeof(DWORD))
        {
            if (!(status = wait_on(hDevice, fd, hEvent, (DWORD*)lpOutBuffer)))
                sz = sizeof(DWORD);
        }
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    default:
        FIXME("Unsupported IOCTL %x (type=%x access=%x func=%x meth=%x)\n", 
              dwIoControlCode, dwIoControlCode >> 16, (dwIoControlCode >> 14) & 3,
              (dwIoControlCode >> 2) & 0xFFF, dwIoControlCode & 3);
        sz = 0;
        status = STATUS_INVALID_PARAMETER;
        break;
    }
    if (needs_close) close( fd );
 error:
    piosb->u.Status = status;
    piosb->Information = sz;
    if (hEvent && status != STATUS_PENDING) NtSetEvent(hEvent, NULL);
    return status;
}

NTSTATUS COMM_DeviceIoControl(HANDLE hDevice, 
                              HANDLE hEvent, PIO_APC_ROUTINE UserApcRoutine,
                              PVOID UserApcContext, 
                              PIO_STATUS_BLOCK piosb, 
                              ULONG dwIoControlCode,
                              LPVOID lpInBuffer, DWORD nInBufferSize,
                              LPVOID lpOutBuffer, DWORD nOutBufferSize)
{
    NTSTATUS    status;

    if (dwIoControlCode == IOCTL_SERIAL_WAIT_ON_MASK)
    {
        HANDLE          hev = hEvent;

        /* this is an ioctl we implement in a non blocking way if hEvent is not
         * null
         * so we have to explicitely wait if no hEvent is provided
         */
        if (!hev)
        {
            OBJECT_ATTRIBUTES   attr;
            
            attr.Length                   = sizeof(attr);
            attr.RootDirectory            = 0;
            attr.ObjectName               = NULL;
            attr.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_OPENIF;
            attr.SecurityDescriptor       = NULL;
            attr.SecurityQualityOfService = NULL;
            status = NtCreateEvent(&hev, EVENT_ALL_ACCESS, &attr, FALSE, FALSE);

            if (status) goto done;
        }
        status = io_control(hDevice, hev, UserApcRoutine, UserApcContext,
                            piosb, dwIoControlCode, lpInBuffer, nInBufferSize,
                            lpOutBuffer, nOutBufferSize);
        if (hev != hEvent)
        {
            if (status == STATUS_PENDING)
            {
                NtWaitForSingleObject(hev, FALSE, NULL);
                status = STATUS_SUCCESS;
            }
            NtClose(hev);
        }
    }
    else status = io_control(hDevice, hEvent, UserApcRoutine, UserApcContext,
                             piosb, dwIoControlCode, lpInBuffer, nInBufferSize,
                             lpOutBuffer, nOutBufferSize);
done:
    return status;
}
