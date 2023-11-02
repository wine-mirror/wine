/*
 * Serial device support
 *
 * Copyright 1993 Erik Bos
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef HAVE_SYS_MODEM_H
# include <sys/modem.h>
#endif
#ifdef HAVE_SYS_STRTIO_H
# include <sys/strtio.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winioctl.h"
#include "ddk/ntddser.h"
#include "wine/server.h"
#include "unix_private.h"
#include "wine/debug.h"

#ifdef HAVE_LINUX_SERIAL_H
#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif
#include <linux/serial.h>
#endif

#if !defined(TIOCINQ) && defined(FIONREAD)
#define	TIOCINQ FIONREAD
#endif

WINE_DEFAULT_DEBUG_CHANNEL(comm);

static const char* iocode2str(UINT ioc)
{
    switch (ioc)
    {
#define X(x)    case (x): return #x
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
    default: return wine_dbg_sprintf("IOCTL_SERIAL_%d\n", ioc);
    }
}

static NTSTATUS get_baud_rate(int fd, SERIAL_BAUD_RATE* sbr)
{
    struct termios port;
    int speed;

    if (tcgetattr(fd, &port) == -1)
    {
        ERR("tcgetattr error '%s'\n", strerror(errno));
        return errno_to_status( errno );
    }
    speed = cfgetospeed(&port);
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
#ifdef B500000
    case B500000:	sbr->BaudRate = 500000; break;
#endif
#ifdef B921600
    case B921600:	sbr->BaudRate = 921600; break;
#endif
#ifdef B1000000
    case B1000000:	sbr->BaudRate = 1000000; break;
#endif
#ifdef B1152000
    case B1152000:	sbr->BaudRate = 1152000; break;
#endif
#ifdef B1500000
    case B1500000:	sbr->BaudRate = 1500000; break;
#endif
#ifdef B2000000
    case B2000000:	sbr->BaudRate = 2000000; break;
#endif
#ifdef B2500000
    case B2500000:	sbr->BaudRate = 2500000; break;
#endif
#ifdef B3000000
    case B3000000:	sbr->BaudRate = 3000000; break;
#endif
#ifdef B3500000
    case B3500000:	sbr->BaudRate = 3500000; break;
#endif
#ifdef B4000000
    case B4000000:	sbr->BaudRate = 4000000; break;
#endif
    default:
        ERR("unknown speed %x\n", speed);
        return STATUS_INVALID_PARAMETER;
    }
    return STATUS_SUCCESS;
}

static NTSTATUS get_hand_flow(int fd, SERIAL_HANDFLOW* shf)
{
    int stat = 0;
    struct termios port;

    if (tcgetattr(fd, &port) == -1)
    {
        ERR("tcgetattr error '%s'\n", strerror(errno));
        return errno_to_status( errno );
    }
    /* termios does not support DTR/DSR flow control */
    shf->ControlHandShake = 0;
    shf->FlowReplace = 0;
#ifdef TIOCMGET
    if (ioctl(fd, TIOCMGET, &stat) == -1)
    {
        WARN("ioctl error '%s'\n", strerror(errno));
        shf->ControlHandShake |= SERIAL_DTR_CONTROL;
        shf->FlowReplace |= SERIAL_RTS_CONTROL;
    }
#else
    WARN("Setting DTR/RTS to enabled by default\n");
    shf->ControlHandShake |= SERIAL_DTR_CONTROL;
    shf->FlowReplace |= SERIAL_RTS_CONTROL;
#endif
#ifdef TIOCM_DTR
    if (stat & TIOCM_DTR)
#endif
        shf->ControlHandShake |= SERIAL_DTR_CONTROL;
#ifdef CRTSCTS
    if (port.c_cflag & CRTSCTS)
    {
        shf->FlowReplace |= SERIAL_RTS_CONTROL;
        shf->ControlHandShake |= SERIAL_CTS_HANDSHAKE;
    }
    else
#endif
    {
#ifdef TIOCM_RTS
        if (stat & TIOCM_RTS)
#endif
            shf->FlowReplace |= SERIAL_RTS_CONTROL;
    }
    if (port.c_iflag & IXOFF)
        shf->FlowReplace |= SERIAL_AUTO_RECEIVE;
    if (port.c_iflag & IXON)
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
        return errno_to_status( errno );
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
    case PARENB|PARODD|CMSPAR:  slc->Parity = MARKPARITY;       break;
    case PARENB|CMSPAR:         slc->Parity = SPACEPARITY;      break;
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

static NTSTATUS get_modem_status(int fd, UINT* lpModemStat)
{
    NTSTATUS    status = STATUS_NOT_SUPPORTED;
    int         mstat;

    *lpModemStat = 0;
#ifdef TIOCMGET
    if (!ioctl(fd, TIOCMGET, &mstat))
    {
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
        return STATUS_SUCCESS;
    }
    WARN("TIOCMGET err %s\n", strerror(errno));
    status = errno_to_status( errno );
#endif
    return status;
}

static NTSTATUS get_properties(int fd, SERIAL_COMMPROP *prop)
{
    /* FIXME: get actual properties from the device */
    memset( prop, 0, sizeof(*prop) );
    prop->PacketLength       = 1;
    prop->PacketVersion      = 1;
    prop->ServiceMask       = SP_SERIALCOMM;
    prop->MaxTxQueue        = 4096;
    prop->MaxRxQueue        = 4096;
    prop->MaxBaud           = BAUD_115200;
    prop->ProvSubType       = PST_RS232;
    prop->ProvCapabilities  = PCF_DTRDSR | PCF_PARITY_CHECK | PCF_RTSCTS | PCF_TOTALTIMEOUTS | PCF_INTTIMEOUTS;
    prop->SettableParams    = SP_BAUD | SP_DATABITS | SP_HANDSHAKING |
                              SP_PARITY | SP_PARITY_CHECK | SP_STOPBITS ;
    prop->SettableBaud      = BAUD_075 | BAUD_110 | BAUD_134_5 | BAUD_150 |
                              BAUD_300 | BAUD_600 | BAUD_1200 | BAUD_1800 | BAUD_2400 | BAUD_4800 |
                              BAUD_9600 | BAUD_19200 | BAUD_38400 | BAUD_57600 | BAUD_115200 ;
    prop->SettableData       = DATABITS_5 | DATABITS_6 | DATABITS_7 | DATABITS_8 ;
    prop->SettableStopParity = STOPBITS_10 | STOPBITS_15 | STOPBITS_20 |
                PARITY_NONE | PARITY_ODD |PARITY_EVEN | PARITY_MARK | PARITY_SPACE;
    prop->CurrentTxQueue    = prop->MaxTxQueue;
    prop->CurrentRxQueue    = prop->MaxRxQueue;
    return STATUS_SUCCESS;
}

static NTSTATUS get_special_chars(int fd, SERIAL_CHARS* sc)
{
    struct termios port;

    if (tcgetattr(fd, &port) == -1)
    {
        ERR("tcgetattr error '%s'\n", strerror(errno));
        return errno_to_status( errno );
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
        status = errno_to_status( errno );
    }
#else
    ss->AmountInOutQueue = 0; /* FIXME: find a different way to find out */
#endif

#ifdef TIOCINQ
    if (ioctl(fd, TIOCINQ, &ss->AmountInInQueue))
    {
        WARN("ioctl returned error\n");
        status = errno_to_status( errno );
    }
#else
    ss->AmountInInQueue = 0; /* FIXME: find a different way to find out */
#endif
    return status;
}

static void stop_waiting( HANDLE handle )
{
    unsigned int status;

    SERVER_START_REQ( set_serial_info )
    {
        req->handle = wine_server_obj_handle( handle );
        req->flags = SERIALINFO_PENDING_WAIT;
        if ((status = wine_server_call( req )))
            ERR("failed to clear waiting state: %#x\n", status);
    }
    SERVER_END_REQ;
}

static NTSTATUS get_wait_mask(HANDLE hDevice, UINT *mask, UINT *cookie, BOOL *pending_write, BOOL start_wait)
{
    unsigned int status;

    SERVER_START_REQ( get_serial_info )
    {
        req->handle = wine_server_obj_handle( hDevice );
        req->flags = pending_write ? SERIALINFO_PENDING_WRITE : 0;
        if (start_wait) req->flags |= SERIALINFO_PENDING_WAIT;
        if (!(status = wine_server_call( req )))
        {
            *mask = reply->eventmask;
            if (cookie) *cookie = reply->cookie;
            if (pending_write) *pending_write = reply->pending_write;
        }
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
        return errno_to_status( errno );
    }

    switch (sbr->BaudRate)
    {
    case 0:         cfsetospeed( &port, B0 ); break;
    case 50:        cfsetospeed( &port, B50 ); break;
    case 75:        cfsetospeed( &port, B75 ); break;
    case 110:
    case CBR_110:   cfsetospeed( &port, B110 ); break;
    case 134:       cfsetospeed( &port, B134 ); break;
    case 150:       cfsetospeed( &port, B150 ); break;
    case 200:       cfsetospeed( &port, B200 ); break;
    case 300:
    case CBR_300:   cfsetospeed( &port, B300 ); break;
    case 600:
    case CBR_600:   cfsetospeed( &port, B600 ); break;
    case 1200:
    case CBR_1200:  cfsetospeed( &port, B1200 ); break;
    case 1800:      cfsetospeed( &port, B1800 ); break;
    case 2400:
    case CBR_2400:  cfsetospeed( &port, B2400 ); break;
    case 4800:
    case CBR_4800:  cfsetospeed( &port, B4800 ); break;
    case 9600:
    case CBR_9600:  cfsetospeed( &port, B9600 ); break;
    case 19200:
    case CBR_19200: cfsetospeed( &port, B19200 ); break;
    case 38400:
    case CBR_38400: cfsetospeed( &port, B38400 ); break;
#ifdef B57600
    case 57600: cfsetospeed( &port, B57600 ); break;
#endif
#ifdef B115200
    case 115200: cfsetospeed( &port, B115200 ); break;
#endif
#ifdef B230400
    case 230400: cfsetospeed( &port, B230400 ); break;
#endif
#ifdef B460800
    case 460800: cfsetospeed( &port, B460800 ); break;
#endif
#ifdef B500000
    case 500000: cfsetospeed( &port, B500000 ); break;
#endif
#ifdef B921600
    case 921600: cfsetospeed( &port, B921600 ); break;
#endif
#ifdef B1000000
    case 1000000: cfsetospeed( &port, B1000000 ); break;
#endif
#ifdef B1152000
    case 1152000: cfsetospeed( &port, B1152000 ); break;
#endif
#ifdef B1500000
    case 1500000: cfsetospeed( &port, B1500000 ); break;
#endif
#ifdef B2000000
    case 2000000: cfsetospeed( &port, B2000000 ); break;
#endif
#ifdef B2500000
    case 2500000: cfsetospeed( &port, B2500000 ); break;
#endif
#ifdef B3000000
    case 3000000: cfsetospeed( &port, B3000000 ); break;
#endif
#ifdef B3500000
    case 3500000: cfsetospeed( &port, B3500000 ); break;
#endif
#ifdef B4000000
    case 4000000: cfsetospeed( &port, B4000000 ); break;
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
                 "has caused to your linux system can be undone with setserial\n"
                 "(see man setserial). If you have incapacitated a Hayes type modem,\n"
                 "reset it and it will probably recover.\n", (int)sbr->BaudRate, arby);
            ioctl(fd, TIOCSSERIAL, &nuts);
            cfsetospeed( &port, B38400 );
        }
        break;
#else     /* Don't have linux/serial.h or lack TIOCSSERIAL */
        ERR("baudrate %d\n", (int)sbr->BaudRate);
        return STATUS_NOT_SUPPORTED;
#endif    /* Don't have linux/serial.h or lack TIOCSSERIAL */
    }
    cfsetispeed( &port, cfgetospeed(&port) );
    if (tcsetattr(fd, TCSANOW, &port) == -1)
    {
        ERR("tcsetattr error '%s'\n", strerror(errno));
        return errno_to_status( errno );
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
        return errno_to_status( errno );
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
        port.c_iflag |= IXOFF;
    else
        port.c_iflag &= ~IXOFF;
    if (shf->FlowReplace & SERIAL_AUTO_TRANSMIT)
        port.c_iflag |= IXON;
    else
        port.c_iflag &= ~IXON;
    if (tcsetattr(fd, TCSANOW, &port) == -1)
    {
        ERR("tcsetattr error '%s'\n", strerror(errno));
        return errno_to_status( errno );
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
        return errno_to_status( errno );
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

    /*
     * on FreeBSD, turning off ICANON does not disable IEXTEN,
     * so we must turn it off explicitly. No harm done on Linux.
     */
    port.c_lflag &= ~(ICANON|ECHO|ISIG|IEXTEN);
    port.c_lflag |= NOFLSH;

    bytesize = slc->WordLength;
    stopbits = slc->StopBits;

#ifdef CMSPAR
    port.c_cflag &= ~(PARENB | PARODD | CMSPAR);
#else
    port.c_cflag &= ~(PARENB | PARODD);
#endif

    /* make sure that reads don't block */
    port.c_cc[VMIN] = 0;
    port.c_cc[VTIME] = 0;

    switch (slc->Parity)
    {
    case NOPARITY:      port.c_iflag &= ~INPCK;                         break;
    case ODDPARITY:     port.c_cflag |= PARENB | PARODD;                break;
    case EVENPARITY:    port.c_cflag |= PARENB;                         break;
#ifdef CMSPAR
        /* Linux defines mark/space (stick) parity */
    case MARKPARITY:    port.c_cflag |= PARENB | PARODD | CMSPAR;       break;
    case SPACEPARITY:   port.c_cflag |= PARENB | CMSPAR;                break;
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
            FIXME("Cannot set MARK Parity\n");
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
            FIXME("Cannot set SPACE Parity\n");
            return STATUS_NOT_SUPPORTED;
        }
        break;
#endif
    default:
        FIXME("Parity %d is not supported\n", slc->Parity);
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
        FIXME("ByteSize %d is not supported\n", bytesize);
        return STATUS_NOT_SUPPORTED;
    }

    switch (stopbits)
    {
    case ONESTOPBIT:    port.c_cflag &= ~CSTOPB;        break;
    case ONE5STOPBITS: /* will be selected if bytesize is 5 */
    case TWOSTOPBITS:	port.c_cflag |= CSTOPB;		break;
    default:
        FIXME("StopBits %d is not supported\n", stopbits);
        return STATUS_NOT_SUPPORTED;
    }
    /* otherwise it hangs with pending input*/
    if (tcsetattr(fd, TCSANOW, &port) == -1)
    {
        ERR("tcsetattr error '%s'\n", strerror(errno));
        return errno_to_status( errno );
    }
    return STATUS_SUCCESS;
}

static NTSTATUS set_queue_size(int fd, const SERIAL_QUEUE_SIZE* sqs)
{
    FIXME("insize %d outsize %d unimplemented stub\n", (int)sqs->InSize, (int)sqs->OutSize);
    return STATUS_SUCCESS;
}

static NTSTATUS set_special_chars(int fd, const SERIAL_CHARS* sc)
{
    struct termios port;

    if (tcgetattr(fd, &port) == -1)
    {
        ERR("tcgetattr error '%s'\n", strerror(errno));
        return errno_to_status( errno );
    }

    port.c_cc[VEOF  ] = sc->EofChar;
    /* FIXME: sc->ErrorChar is not supported */
    /* FIXME: sc->BreakChar is not supported */
    /* FIXME: sc->EventChar is not supported */
    port.c_cc[VSTART] = sc->XonChar;
    port.c_cc[VSTOP ] = sc->XoffChar;

    if (tcsetattr(fd, TCSANOW, &port) == -1)
    {
        ERR("tcsetattr error '%s'\n", strerror(errno));
        return errno_to_status( errno );
    }
    return STATUS_SUCCESS;
}

/*
 * does not change IXOFF but simulates that IXOFF has been received:
 */
static NTSTATUS set_XOff(int fd)
{
    if (tcflow(fd, TCOOFF))
    {
        return errno_to_status( errno );
    }
    return STATUS_SUCCESS;
}

/*
 * does not change IXON but simulates that IXON has been received:
 */
static NTSTATUS set_XOn(int fd)
{
    if (tcflow(fd, TCOON))
    {
        return errno_to_status( errno );
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
    int rx, tx, frame, overrun, parity, brk, buf_overrun, temt;
}serial_irq_info;

/***********************************************************************
 * Data needed by the thread polling for the changing CommEvent
 */
typedef struct async_commio
{
    HANDLE              hDevice;
    DWORD*              events;
    client_ptr_t        iosb;
    HANDLE              hEvent;
    UINT                evtmask;
    UINT                cookie;
    UINT                mstat;
    BOOL                pending_write;
    serial_irq_info     irq_info;
} async_commio;

/***********************************************************************
 *   Get extended interrupt count info, needed for wait_on
 */
static NTSTATUS get_irq_info(int fd, serial_irq_info *irq_info)
{
    int out;

#if defined (HAVE_LINUX_SERIAL_H) && defined (TIOCGICOUNT)
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
    }
    else
    {
        TRACE("TIOCGICOUNT err %s\n", strerror(errno));
        memset(irq_info,0, sizeof(serial_irq_info));
    }
#else
    memset(irq_info,0, sizeof(serial_irq_info));
#endif

    irq_info->temt = 0;
    /* Generate a single TX_TXEMPTY event when the TX Buffer turns empty*/
#ifdef TIOCSERGETLSR  /* prefer to log the state TIOCSERGETLSR */
    if (!ioctl(fd, TIOCSERGETLSR, &out))
    {
        irq_info->temt = (out & TIOCSER_TEMT) != 0;
        return STATUS_SUCCESS;
    }

    TRACE("TIOCSERGETLSR err %s\n", strerror(errno));
#endif
#ifdef TIOCOUTQ  /* otherwise we log when the out queue gets empty */
    if (!ioctl(fd, TIOCOUTQ, &out))
    {
        irq_info->temt = out == 0;
        return STATUS_SUCCESS;
    }
    TRACE("TIOCOUTQ err %s\n", strerror(errno));
    return errno_to_status( errno );
#endif
    return STATUS_SUCCESS;
}


static DWORD check_events(int fd, UINT mask,
                          const serial_irq_info *new,
                          const serial_irq_info *old,
                          UINT new_mstat, UINT old_mstat, BOOL pending_write)
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
    TRACE("old->temt        0x%08x vs. new->temt        0x%08x\n", old->temt, new->temt);

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
        if ((!old->temt || pending_write) && new->temt)
            ret |= EV_TXEMPTY;
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
static void CALLBACK wait_for_event(LPVOID arg)
{
    async_commio *commio = arg;
    int fd, needs_close;

    if (!server_get_unix_fd( commio->hDevice, FILE_READ_DATA | FILE_WRITE_DATA, &fd, &needs_close, NULL, NULL ))
    {
        serial_irq_info new_irq_info;
        UINT new_mstat, dummy, cookie;
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
            {
                TRACE("get_modem_status failed\n");
                *commio->events = 0;
                break;
            }
            *commio->events = check_events(fd, commio->evtmask,
                                           &new_irq_info, &commio->irq_info,
                                           new_mstat, commio->mstat, commio->pending_write);
            if (*commio->events) break;
            get_wait_mask(commio->hDevice, &dummy, &cookie, (commio->evtmask & EV_TXEMPTY) ? &commio->pending_write : NULL, FALSE);
            if (commio->cookie != cookie)
            {
                *commio->events = 0;
                break;
            }
        }
        if (needs_close) close( fd );
    }
    if (*commio->events) set_async_iosb( commio->iosb, STATUS_SUCCESS, sizeof(DWORD) );
    else set_async_iosb( commio->iosb, STATUS_CANCELLED, 0 );
    stop_waiting(commio->hDevice);
    if (commio->hEvent) NtSetEvent(commio->hEvent, NULL);
    free( commio );
    NtTerminateThread( GetCurrentThread(), 0 );
}

static NTSTATUS wait_on(HANDLE hDevice, int fd, HANDLE hEvent, client_ptr_t iosb_ptr, DWORD* events)
{
    async_commio*       commio;
    NTSTATUS            status;
    HANDLE handle;

    if ((status = NtResetEvent(hEvent, NULL)))
        return status;

    commio = malloc( sizeof(async_commio) );
    if (!commio) return STATUS_NO_MEMORY;

    commio->hDevice = hDevice;
    commio->events  = events;
    commio->iosb    = iosb_ptr;
    commio->hEvent  = hEvent;
    commio->pending_write = 0;
    status = get_wait_mask(commio->hDevice, &commio->evtmask, &commio->cookie, (commio->evtmask & EV_TXEMPTY) ? &commio->pending_write : NULL, TRUE);
    if (status)
    {
        free( commio );
        return status;
    }

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

    if ((status = get_irq_info(fd, &commio->irq_info)) &&
        (commio->evtmask & (EV_BREAK | EV_ERR)))
	goto out_now;

    if ((status = get_modem_status(fd, &commio->mstat)) &&
        (commio->evtmask & (EV_CTS | EV_DSR| EV_RING| EV_RLSD)))
	goto out_now;

    /* We might have received something or the TX buffer is delivered */
    *events = check_events(fd, commio->evtmask,
                               &commio->irq_info, &commio->irq_info,
                               commio->mstat, commio->mstat, commio->pending_write);
    if (*events)
    {
        status = STATUS_SUCCESS;
        goto out_now;
    }

    /* create the worker thread for the task */
    /* FIXME: should use async I/O instead */
    status = NtCreateThreadEx( &handle, THREAD_ALL_ACCESS, NULL, GetCurrentProcess(),
                               wait_for_event, commio, 0, 0, 0, 0, NULL );
    if (status != STATUS_SUCCESS) goto out_now;
    NtClose( handle );
    return STATUS_PENDING;

#if !defined(TIOCINQ) || (!(defined(TIOCSERGETLSR) && defined(TIOCSER_TEMT)) || !defined(TIOCINQ)) || !defined(TIOCMGET) || !defined(TIOCM_CTS) ||!defined(TIOCM_DSR) || !defined(TIOCM_RNG) || !defined(TIOCM_CAR)
error_caps:
    FIXME("Returning error because of missing capabilities\n");
    status = STATUS_INVALID_PARAMETER;
#endif
out_now:
    stop_waiting(commio->hDevice);
    free( commio );
    return status;
}

static NTSTATUS xmit_immediate(HANDLE hDevice, int fd, const char* ptr)
{
    /* FIXME: not perfect as it should bypass the in-queue */
    WARN("(%p,'%c') not perfect!\n", hDevice, *ptr);
    if (write(fd, ptr, 1) != 1)
        return errno_to_status( errno );
    return STATUS_SUCCESS;
}

static NTSTATUS io_control( HANDLE device, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                            IO_STATUS_BLOCK *io, UINT code, void *in_buffer,
                            UINT in_size, void *out_buffer, UINT out_size )
{
    DWORD sz = 0, access = FILE_READ_DATA;
    NTSTATUS status = STATUS_SUCCESS;
    int fd = -1, needs_close = 0;
    enum server_fd_type type;

    TRACE("%p %s %p %d %p %d %p\n",
          device, iocode2str(code), in_buffer, in_size, out_buffer, out_size, io);

    switch (code)
    {
    case IOCTL_SERIAL_GET_TIMEOUTS:
    case IOCTL_SERIAL_SET_TIMEOUTS:
    case IOCTL_SERIAL_GET_WAIT_MASK:
    case IOCTL_SERIAL_SET_WAIT_MASK:
        /* these are handled on the server side */
        return STATUS_NOT_SUPPORTED;
    }

    io->Information = 0;

    if ((status = server_get_unix_fd( device, access, &fd, &needs_close, &type, NULL ))) goto error;
    if (type != FD_TYPE_SERIAL)
    {
        if (needs_close) close( fd );
        status = STATUS_OBJECT_TYPE_MISMATCH;
        goto error;
    }

    switch (code)
    {
    case IOCTL_SERIAL_CLR_DTR:
#ifdef TIOCM_DTR
        if (whack_modem(fd, ~TIOCM_DTR, 0) == -1) status = errno_to_status( errno );
#else
        status = STATUS_NOT_SUPPORTED;
#endif
        break;
    case IOCTL_SERIAL_CLR_RTS:
#ifdef TIOCM_RTS
        if (whack_modem(fd, ~TIOCM_RTS, 0) == -1) status = errno_to_status( errno );
#else
        status = STATUS_NOT_SUPPORTED;
#endif
        break;
    case IOCTL_SERIAL_GET_BAUD_RATE:
        if (out_buffer && out_size == sizeof(SERIAL_BAUD_RATE))
        {
            if (!(status = get_baud_rate(fd, out_buffer)))
                sz = sizeof(SERIAL_BAUD_RATE);
        }
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_GET_CHARS:
        if (out_buffer && out_size == sizeof(SERIAL_CHARS))
        {
            if (!(status = get_special_chars(fd, out_buffer)))
                sz = sizeof(SERIAL_CHARS);
        }
        else
            status = STATUS_INVALID_PARAMETER;
        break;
     case IOCTL_SERIAL_GET_COMMSTATUS:
        if (out_buffer && out_size == sizeof(SERIAL_STATUS))
        {
            if (!(status = get_status(fd, out_buffer)))
                sz = sizeof(SERIAL_STATUS);
        }
        else status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_GET_HANDFLOW:
        if (out_buffer && out_size == sizeof(SERIAL_HANDFLOW))
        {
            if (!(status = get_hand_flow(fd, out_buffer)))
                sz = sizeof(SERIAL_HANDFLOW);
        }
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_GET_LINE_CONTROL:
        if (out_buffer && out_size == sizeof(SERIAL_LINE_CONTROL))
        {
            if (!(status = get_line_control(fd, out_buffer)))
                sz = sizeof(SERIAL_LINE_CONTROL);
        }
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_GET_MODEMSTATUS:
        if (out_buffer && out_size == sizeof(DWORD))
        {
            if (!(status = get_modem_status(fd, out_buffer)))
                sz = sizeof(DWORD);
        }
        else status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_GET_PROPERTIES:
        if (out_buffer && out_size == sizeof(SERIAL_COMMPROP))
        {
            if (!(status = get_properties(fd, out_buffer)))
                sz = sizeof(SERIAL_COMMPROP);
        }
        else status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_IMMEDIATE_CHAR:
        if (in_buffer && in_size == sizeof(CHAR))
            status = xmit_immediate(device, fd, in_buffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_PURGE:
        if (in_buffer && in_size == sizeof(DWORD))
            status = purge(fd, *(DWORD*)in_buffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_RESET_DEVICE:
        FIXME("Unsupported\n");
        break;
    case IOCTL_SERIAL_SET_BAUD_RATE:
        if (in_buffer && in_size == sizeof(SERIAL_BAUD_RATE))
            status = set_baud_rate(fd, in_buffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_SET_BREAK_OFF:
#if defined(TIOCSBRK) && defined(TIOCCBRK) /* check if available for compilation */
	if (ioctl(fd, TIOCCBRK, 0) == -1)
        {
            TRACE("ioctl failed\n");
            status = errno_to_status( errno );
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
            status = errno_to_status( errno );
        }
#else
	FIXME("ioctl not available\n");
	status = STATUS_NOT_SUPPORTED;
#endif
        break;
    case IOCTL_SERIAL_SET_CHARS:
        if (in_buffer && in_size == sizeof(SERIAL_CHARS))
            status = set_special_chars(fd, in_buffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_SET_DTR:
#ifdef TIOCM_DTR
        if (whack_modem(fd, 0, TIOCM_DTR) == -1) status = errno_to_status( errno );
#else
        status = STATUS_NOT_SUPPORTED;
#endif
        break;
    case IOCTL_SERIAL_SET_HANDFLOW:
        if (in_buffer && in_size == sizeof(SERIAL_HANDFLOW))
            status = set_handflow(fd, in_buffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_SET_LINE_CONTROL:
        if (in_buffer && in_size == sizeof(SERIAL_LINE_CONTROL))
            status = set_line_control(fd, in_buffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_SET_QUEUE_SIZE:
        if (in_buffer && in_size == sizeof(SERIAL_QUEUE_SIZE))
            status = set_queue_size(fd, in_buffer);
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    case IOCTL_SERIAL_SET_RTS:
#ifdef TIOCM_RTS
        if (whack_modem(fd, 0, TIOCM_RTS) == -1) status = errno_to_status( errno );
#else
        status = STATUS_NOT_SUPPORTED;
#endif
        break;
    case IOCTL_SERIAL_SET_XOFF:
        status = set_XOff(fd);
        break;
    case IOCTL_SERIAL_SET_XON:
        status = set_XOn(fd);
        break;
    case IOCTL_SERIAL_WAIT_ON_MASK:
        if (out_buffer && out_size == sizeof(DWORD))
        {
            if (!(status = wait_on(device, fd, event, iosb_client_ptr(io), out_buffer)))
                sz = sizeof(DWORD);
        }
        else
            status = STATUS_INVALID_PARAMETER;
        break;
    default:
        FIXME("Unsupported IOCTL %x (type=%x access=%x func=%x meth=%x)\n",
              code, code >> 16, (code >> 14) & 3, (code >> 2) & 0xFFF, code & 3);
        sz = 0;
        status = STATUS_INVALID_PARAMETER;
        break;
    }
    if (needs_close) close( fd );
 error:
    io->Status = status;
    io->Information = sz;
    if (event && status != STATUS_PENDING) NtSetEvent(event, NULL);
    return status;
}

/******************************************************************
 *		serial_DeviceIoControl
 */
NTSTATUS serial_DeviceIoControl( HANDLE device, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                 IO_STATUS_BLOCK *io, UINT code, void *in_buffer,
                                 UINT in_size, void *out_buffer, UINT out_size )
{
    NTSTATUS    status;

    if (code == IOCTL_SERIAL_WAIT_ON_MASK)
    {
        HANDLE hev = event;

        /* this is an ioctl we implement in a non blocking way if event is not null
         * so we have to explicitly wait if no event is provided
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
            status = NtCreateEvent(&hev, EVENT_ALL_ACCESS, &attr, SynchronizationEvent, FALSE);

            if (status) return status;
        }
        status = io_control( device, hev, apc, apc_user, io, code,
                             in_buffer, in_size, out_buffer, out_size );
        if (hev != event)
        {
            if (status == STATUS_PENDING)
            {
                NtWaitForSingleObject(hev, FALSE, NULL);
                status = STATUS_SUCCESS;
            }
            NtClose(hev);
        }
    }
    else status = io_control( device, event, apc, apc_user, io, code,
                              in_buffer, in_size, out_buffer, out_size );
    return status;
}

NTSTATUS serial_FlushBuffersFile( int fd )
{
#ifdef HAVE_TCDRAIN
    while (tcdrain( fd ) == -1)
    {
        if (errno != EINTR) return errno_to_status( errno );
    }
    return STATUS_SUCCESS;
#elif defined(TIOCDRAIN)
    while (ioctl( fd, TIOCDRAIN ) == -1)
    {
        if (errno != EINTR) return errno_to_status( errno );
    }
    return STATUS_SUCCESS;
#elif defined(TCSBRK)
    while (ioctl( fd, TCSBRK, 1 ) == -1)
    {
        if (errno != EINTR) return errno_to_status( errno );
    }
    return STATUS_SUCCESS;
#else
    ERR( "not supported\n" );
    return STATUS_NOT_IMPLEMENTED;
#endif
}
