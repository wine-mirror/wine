 /*
 * DEC 93 Erik Bos <erik@xs4all.nl>
 *
 * Copyright 1996 Marcus Meissner
 *
 * Mar 3, 1999. Ove Kåven <ovek@arcticnet.no>
 * - Use port indices instead of unixfds for win16
 * - Moved things around (separated win16 and win32 routines)
 * - Added some hints on how to implement buffers and EnableCommNotification.
 *
 * May 26, 1997.  Fixes and comments by Rick Richardson <rick@dgii.com> [RER]
 * - ptr->fd wasn't getting cleared on close.
 * - GetCommEventMask() and GetCommError() didn't do much of anything.
 *   IMHO, they are still wrong, but they at least implement the RXCHAR
 *   event and return I/O queue sizes, which makes the app I'm interested
 *   in (analog devices EZKIT DSP development system) work.
 *
 * August 12, 1997.  Take a bash at SetCommEventMask - Lawson Whitney
 *                                     <lawson_whitney@juno.com>
 * July 6, 1998. Fixes and comments by Valentijn Sessink
 *                                     <vsessink@ic.uva.nl> [V]
 *  I only quick-fixed an error for the output buffers. The thing is this: if a
 * WinApp starts using serial ports, it calls OpenComm, asking it to open two
 * buffers, cbInQueue and cbOutQueue size, to hold data to/from the serial
 * ports. Wine OpenComm only returns "OK". Now the kernel buffer size for
 * serial communication is only 4096 bytes large. Error: (App asks for
 * a 104,000 bytes size buffer, Wine returns "OK", App asks "How many char's
 * are in the buffer", Wine returns "4000" and App thinks "OK, another
 * 100,000 chars left, good!")
 * The solution below is a bad but working quickfix for the transmit buffer:
 * the cbInQueue is saved in a variable; when the program asks how many chars
 * there are in the buffer, GetCommError returns # in buffer PLUS
 * the additional (cbOutQeueu - 4096), which leaves the application thinking
 * "wow, almost full".
 * Sorry for the rather chatty explanation - but I think comm.c needs to be
 * redefined with real working buffers make it work; maybe these comments are
 * of help.
 * Oktober 98, Rein Klazes [RHK]
 * A program that wants to monitor the modem status line (RLSD/DCD) may
 * poll the modem status register in the commMask structure. I update the bit
 * in GetCommError, waiting for an implementation of communication events.
 * 
 */

#include "config.h"

#include <stdlib.h>
#include <termios.h>
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
#include <sys/ioctl.h>
#include <unistd.h>

#include "wine/winuser16.h"
#include "comm.h"
#ifdef HAVE_SYS_MODEM_H
# include <sys/modem.h>
#endif
#ifdef HAVE_SYS_STRTIO_H
# include <sys/strtio.h>
#endif
#include "heap.h"
#include "options.h"

#include "server/request.h"
#include "server.h"
#include "process.h"
#include "winerror.h"
#include "async.h"

#include "debug.h"

#ifndef TIOCINQ
#define	TIOCINQ FIONREAD
#endif
#define COMM_MSR_OFFSET  35       /* see knowledge base Q101417 */

/*
 * [V] If above globals are wrong, the one below will be wrong as well. It
 * should probably be in the DosDeviceStruct on per port basis too.
*/
int iGlobalOutQueueFiller;

#define SERIAL_XMIT_SIZE 4096

struct DosDeviceStruct COM[MAX_PORTS];
struct DosDeviceStruct LPT[MAX_PORTS];
/* pointers to unknown(==undocumented) comm structure */ 
LPCVOID *unknown[MAX_PORTS];
/* save terminal states */
static struct termios m_stat[MAX_PORTS];

void COMM_Init(void)
{
	int x;
	char option[10], temp[256], *btemp;
	struct stat st;

	for (x=0; x!=MAX_PORTS; x++) {
		strcpy(option,"COMx");
		option[3] = '1' + x;
		option[4] = '\0';

		PROFILE_GetWineIniString( "serialports", option, "*",
                                          temp, sizeof(temp) );
		if (!strcmp(temp, "*") || *temp == '\0') 
			COM[x].devicename = NULL;
		else {
		  	btemp = strchr(temp,',');
			if (btemp != NULL) {
			  	*btemp++ = '\0';
				COM[x].baudrate = atoi(btemp);
			} else {
				COM[x].baudrate = -1;
			}
			stat(temp, &st);
			if (!S_ISCHR(st.st_mode)) 
				WARN(comm,"Can't use `%s' as %s !\n", temp, option);
			else
				if ((COM[x].devicename = malloc(strlen(temp)+1)) == NULL) 
					WARN(comm,"Can't malloc for device info!\n");
				else {
					COM[x].fd = 0;
					strcpy(COM[x].devicename, temp);
				}
                TRACE(comm, "%s = %s\n", option, COM[x].devicename);
 		}

		strcpy(option, "LPTx");
		option[3] = '1' + x;
		option[4] = '\0';

		PROFILE_GetWineIniString( "parallelports", option, "*",
                                          temp, sizeof(temp) );
		if (!strcmp(temp, "*") || *temp == '\0')
			LPT[x].devicename = NULL;
		else {
			stat(temp, &st);
			if (!S_ISCHR(st.st_mode)) 
				WARN(comm,"Can't use `%s' as %s !\n", temp, option);
			else 
				if ((LPT[x].devicename = malloc(strlen(temp)+1)) == NULL) 
					WARN(comm,"Can't malloc for device info!\n");
				else {
					LPT[x].fd = 0;
					strcpy(LPT[x].devicename, temp);
				}
                TRACE(comm, "%s = %s\n", option, LPT[x].devicename);
		}

	}
}


struct DosDeviceStruct *GetDeviceStruct_fd(int fd)
{
	int x;

	for (x=0; x!=MAX_PORTS; x++) {
	    if (COM[x].fd == fd)
		return &COM[x];
	    if (LPT[x].fd == fd)
		return &LPT[x];
	}

	return NULL;
}

struct DosDeviceStruct *GetDeviceStruct(int fd)
{
	if ((fd&0x7F)<=MAX_PORTS) {
            if (!(fd&0x80)) {
		if (COM[fd].fd)
		    return &COM[fd];
	    } else {
		if (LPT[fd].fd)
		    return &LPT[fd];
	    }
	}

	return NULL;
}

int    GetCommPort_fd(int fd)
{
        int x;
        
        for (x=0; x<MAX_PORTS; x++) {
             if (COM[x].fd == fd)
                 return x;
       }
       
       return -1;
} 

int ValidCOMPort(int x)
{
	return(x < MAX_PORTS ? (int) COM[x].devicename : 0); 
}

int ValidLPTPort(int x)
{
	return(x < MAX_PORTS ? (int) LPT[x].devicename : 0); 
}

int WinError(void)
{
        TRACE(comm, "errno = %d\n", errno);
	switch (errno) {
		default:
			return CE_IOE;
		}
}

static void WINE_UNUSED comm_notification(int fd,void*private)
{
    /* in here, we need to:
       1. read any data from the comm port
       2. save it into our own internal buffers
          (we need our own buffers to implement notifications properly!)
       3. write data from our own internal buffers to the comm port
       4. if wnd is set, send WM_COMMNOTIFY (using PostMessage) when
          thresholds set by EnableCommNotification are passed */
          
}

/**************************************************************************
 *         BuildCommDCB		(USER.213)
 */
BOOL16 WINAPI BuildCommDCB16(LPCSTR device, LPDCB16 lpdcb)
{
	/* "COM1:9600,n,8,1"	*/
	/*  012345		*/
	int port;
	char *ptr, temp[256];

	TRACE(comm, "(%s), ptr %p\n", device, lpdcb);

	if (!lstrncmpiA(device,"COM",3)) {
		port = device[3] - '0';
	

		if (port-- == 0) {
			ERR(comm, "BUG ! COM0 can't exist!.\n");
			return -1;
		}

		if (!ValidCOMPort(port)) {
			return -1;
		}
		
		memset(lpdcb, 0, sizeof(DCB16)); /* initialize */

		lpdcb->Id = port;
		
		if (!*(device+4))
			return 0;

		if (*(device+4) != ':')
			return -1;
		
		strcpy(temp,device+5);
		ptr = strtok(temp, ", "); 

		if (COM[port].baudrate > 0)
			lpdcb->BaudRate = COM[port].baudrate;
		else
			lpdcb->BaudRate = atoi(ptr);
        	TRACE(comm,"baudrate (%d)\n", lpdcb->BaudRate);

		ptr = strtok(NULL, ", ");
		if (islower(*ptr))
			*ptr = toupper(*ptr);

        	TRACE(comm,"parity (%c)\n", *ptr);
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
			default:
				WARN(comm,"Unknown parity `%c'!\n", *ptr);
				return -1;
		}

		ptr = strtok(NULL, ", "); 
         	TRACE(comm, "charsize (%c)\n", *ptr);
		lpdcb->ByteSize = *ptr - '0';

		ptr = strtok(NULL, ", ");
        	TRACE(comm, "stopbits (%c)\n", *ptr);
		switch (*ptr) {
			case '1':
				lpdcb->StopBits = ONESTOPBIT;
				break;			
			case '2':
				lpdcb->StopBits = TWOSTOPBITS;
				break;			
			default:
				WARN(comm,"Unknown # of stopbits `%c'!\n", *ptr);
				return -1;
		}
	}	

	return 0;
}

/*****************************************************************************
 *	OpenComm		(USER.200)
 */
INT16 WINAPI OpenComm16(LPCSTR device,UINT16 cbInQueue,UINT16 cbOutQueue)
{
	int port,fd;

    	TRACE(comm, "%s, %d, %d\n", device, cbInQueue, cbOutQueue);

	if (!lstrncmpiA(device,"COM",3)) {
		port = device[3] - '0';

		if (port-- == 0) {
			ERR(comm, "BUG ! COM0 doesn't exist !\n");
		}
		
		/* to help GetCommError return left buffsize [V] */
		iGlobalOutQueueFiller = (cbOutQueue - SERIAL_XMIT_SIZE);
		if (iGlobalOutQueueFiller < 0) iGlobalOutQueueFiller = 0;

                TRACE(comm, "%s = %s\n", device, COM[port].devicename);

		if (!ValidCOMPort(port)) {
			return IE_BADID;
		}
		if (COM[port].fd) {
			return IE_OPEN;
		}

		fd = open(COM[port].devicename, O_RDWR | O_NONBLOCK);
		if (fd == -1) {
			return WinError();
		} else {
                        unknown[port] = SEGPTR_ALLOC(40);
                        bzero(unknown[port],40);
			COM[port].fd = fd;
			COM[port].commerror = 0;
			COM[port].eventmask = 0;
                        /* save terminal state */
                        tcgetattr(fd,&m_stat[port]);
                        /* set default parameters */
                        if(COM[port].baudrate>-1){
                            DCB16 dcb;
                            GetCommState16(port, &dcb);
                            dcb.BaudRate=COM[port].baudrate;
                            /* more defaults:
                             * databits, parity, stopbits
                             */
                            SetCommState16( &dcb);
                        }
#if 0
			/* allocate buffers */
			/* ... */
			/* enable async notifications */
			ASYNC_RegisterFD(COM[port].fd,comm_notification,&COM[port]);
#endif
			return port;
		}
	} 
	else 
	if (!lstrncmpiA(device,"LPT",3)) {
		port = device[3] - '0';
	
		if (port-- == 0) {
			ERR(comm, "BUG ! LPT0 doesn't exist !\n");
		}

		if (!ValidLPTPort(port)) {
			return IE_BADID;
		}		
		if (LPT[port].fd) {
			return IE_OPEN;
		}

		fd = open(LPT[port].devicename, O_RDWR | O_NONBLOCK, 0);
		if (fd == -1) {
			return WinError();
		} else {
			LPT[port].fd = fd;
			LPT[port].commerror = 0;
			LPT[port].eventmask = 0;
			return port|0x80;
		}
	}
	return 0;
}

/*****************************************************************************
 *	CloseComm		(USER.207)
 */
INT16 WINAPI CloseComm16(INT16 cid)
{
	struct DosDeviceStruct *ptr;
        
    	TRACE(comm,"cid=%d\n", cid);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		return -1;
	}
	if (!(cid&0x80)) {
		/* COM port */
		SEGPTR_FREE(unknown[cid]); /* [LW] */
#if 0
		/* disable async notifications */
		ASYNC_UnregisterFD(COM[cid].fd,comm_notification);
		/* free buffers */
		/* ... */
#endif
		/* reset modem lines */
		tcsetattr(ptr->fd,TCSANOW,&m_stat[cid]);
	}

	if (close(ptr->fd) == -1) {
		ptr->commerror = WinError();
		/* FIXME: should we clear ptr->fd here? */
		return -1;
	} else {
		ptr->commerror = 0;
		ptr->fd = 0;
		return 0;
	}
}

/*****************************************************************************
 *	SetCommBreak		(USER.210)
 */
INT16 WINAPI SetCommBreak16(INT16 cid)
{
	struct DosDeviceStruct *ptr;

	TRACE(comm,"cid=%d\n", cid);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		return -1;
	}

	ptr->suspended = 1;
	ptr->commerror = 0;
	return 0;
}

/*****************************************************************************
 *	ClearCommBreak		(USER.211)
 */
INT16 WINAPI ClearCommBreak16(INT16 cid)
{
	struct DosDeviceStruct *ptr;

    	TRACE(comm,"cid=%d\n", cid);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		return -1;
	}

	ptr->suspended = 0;
	ptr->commerror = 0;
	return 0;
}

/*****************************************************************************
 *	EscapeCommFunction	(USER.214)
 */
LONG WINAPI EscapeCommFunction16(UINT16 cid,UINT16 nFunction)
{
	int	max;
	struct DosDeviceStruct *ptr;
	struct termios port;

    	TRACE(comm,"cid=%d, function=%d\n", cid, nFunction);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		return -1;
	}
	if (tcgetattr(ptr->fd,&port) == -1) {
		ptr->commerror=WinError();	
		return -1;
	}

	switch (nFunction) {
		case RESETDEV:
			break;					

		case GETMAXCOM:
			for (max = MAX_PORTS;!COM[max].devicename;max--)
				;
			return max;
			break;

		case GETMAXLPT:
			for (max = MAX_PORTS;!LPT[max].devicename;max--)
				;
			return 0x80 + max;
			break;

#ifdef TIOCM_DTR
		case CLRDTR:
			port.c_cflag &= TIOCM_DTR;
			break;
#endif

#ifdef TIOCM_RTS
		case CLRRTS:
			port.c_cflag &= TIOCM_RTS;
			break;
#endif
	
#ifdef CRTSCTS
		case SETDTR:
			port.c_cflag |= CRTSCTS;
			break;

		case SETRTS:
			port.c_cflag |= CRTSCTS;
			break;
#endif

		case SETXOFF:
			port.c_iflag |= IXOFF;
			break;

		case SETXON:
			port.c_iflag |= IXON;
			break;

		default:
			WARN(comm,"(cid=%d,nFunction=%d): Unknown function\n", 
			cid, nFunction);
			break;				
	}
	
	if (tcsetattr(ptr->fd, TCSADRAIN, &port) == -1) {
		ptr->commerror = WinError();
		return -1;	
	} else {
		ptr->commerror = 0;
		return 0;
	}
}

/*****************************************************************************
 *	FlushComm	(USER.215)
 */
INT16 WINAPI FlushComm16(INT16 cid,INT16 fnQueue)
{
	int queue;
	struct DosDeviceStruct *ptr;

    	TRACE(comm,"cid=%d, queue=%d\n", cid, fnQueue);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		return -1;
	}
	switch (fnQueue) {
		case 0:	queue = TCOFLUSH;
			break;
		case 1:	queue = TCIFLUSH;
			break;
		default:WARN(comm,"(cid=%d,fnQueue=%d):Unknown queue\n", 
				cid, fnQueue);
			return -1;
		}
	if (tcflush(ptr->fd, queue)) {
		ptr->commerror = WinError();
		return -1;	
	} else {
		ptr->commerror = 0;
		return 0;
	}
}  

/********************************************************************
 *	GetCommError	(USER.203)
 */
INT16 WINAPI GetCommError16(INT16 cid,LPCOMSTAT16 lpStat)
{
	int		temperror;
	unsigned long	cnt;
	int		rc;
	struct DosDeviceStruct *ptr;
        unsigned char *stol;
        unsigned int mstat;

	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		return -1;
	}
        if (cid&0x80) {
            WARN(comm," cid %d not comm port\n",cid);
            return CE_MODE;
        }
        stol = (unsigned char *)unknown[cid] + COMM_MSR_OFFSET;
        ioctl(ptr->fd,TIOCMGET,&mstat);
        if( mstat&TIOCM_CAR ) 
            *stol |= 0x80;
        else 
            *stol &=0x7f;

	if (lpStat) {
		lpStat->status = 0;

		rc = ioctl(ptr->fd, TIOCOUTQ, &cnt);
		if (rc) WARN(comm, "Error !\n");
		lpStat->cbOutQue = cnt + iGlobalOutQueueFiller;

		rc = ioctl(ptr->fd, TIOCINQ, &cnt);
                if (rc) WARN(comm, "Error !\n");
		lpStat->cbInQue = cnt;

    		TRACE(comm, "cid %d, error %d, lpStat %d %d %d stol %x\n",
			     cid, ptr->commerror, lpStat->status, lpStat->cbInQue, 
			     lpStat->cbOutQue, *stol);
	}
	else
		TRACE(comm, "cid %d, error %d, lpStat NULL stol %x\n",
			     cid, ptr->commerror, *stol);

	/* Return any errors and clear it */
	temperror = ptr->commerror;
	ptr->commerror = 0;
	return(temperror);
}

/*****************************************************************************
 *	SetCommEventMask	(USER.208)
 */
SEGPTR WINAPI SetCommEventMask16(INT16 cid,UINT16 fuEvtMask)
{
	struct DosDeviceStruct *ptr;
        unsigned char *stol;
        int repid;
        unsigned int mstat;

    	TRACE(comm,"cid %d,mask %d\n",cid,fuEvtMask);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		return -1;
	}
	ptr->eventmask |= fuEvtMask;
        if (cid&0x80) {
            WARN(comm," cid %d not comm port\n",cid);
            return SEGPTR_GET(NULL);
        }
        stol = (unsigned char *)unknown[cid] + COMM_MSR_OFFSET;
	repid = ioctl(ptr->fd,TIOCMGET,&mstat);
	TRACE(comm, " ioctl  %d, msr %x at %p %p\n",repid,mstat,stol,unknown[cid]);
	if ((mstat&TIOCM_CAR)) {*stol |= 0x80;}
	     else {*stol &=0x7f;}
	TRACE(comm," modem dcd construct %x\n",*stol);
	return SEGPTR_GET(unknown[cid]);
}

/*****************************************************************************
 *	GetCommEventMask	(USER.209)
 */
UINT16 WINAPI GetCommEventMask16(INT16 cid,UINT16 fnEvtClear)
{
	int	events = 0;
	struct DosDeviceStruct *ptr;

    	TRACE(comm, "cid %d, mask %d\n", cid, fnEvtClear);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		return -1;
	}

	/*
	 *	Determine if any characters are available
	 */
	if (fnEvtClear & EV_RXCHAR)
	{
		int		rc;
		unsigned long	cnt;

		rc = ioctl(ptr->fd, TIOCINQ, &cnt);
		if (cnt) events |= EV_RXCHAR;

		TRACE(comm, "rxchar %ld\n", cnt);
	}

	/*
	 *	There are other events that need to be checked for
	 */
	/* TODO */

	TRACE(comm, "return events %d\n", events);
	return events;

	/*
	 * [RER] The following was gibberish
	 */
#if 0
	tempmask = ptr->eventmask;
	ptr->eventmask &= ~fnEvtClear;
	return ptr->eventmask;
#endif
}

/*****************************************************************************
 *	SetCommState16	(USER.201)
 */
INT16 WINAPI SetCommState16(LPDCB16 lpdcb)
{
	struct termios port;
	struct DosDeviceStruct *ptr;

    	TRACE(comm, "cid %d, ptr %p\n", lpdcb->Id, lpdcb);
	if ((ptr = GetDeviceStruct(lpdcb->Id)) == NULL) {
		return -1;
	}
	if (tcgetattr(ptr->fd, &port) == -1) {
		ptr->commerror = WinError();	
		return -1;
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

    	TRACE(comm,"baudrate %d\n",lpdcb->BaudRate);
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
		case 57601:
			port.c_cflag |= B115200;
			break;		
#endif
		default:
			ptr->commerror = IE_BAUDRATE;
			return -1;
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
                default:
                        ptr->commerror = IE_BAUDRATE;
                        return -1;
        }
        port.c_ispeed = port.c_ospeed;
#endif
    	TRACE(comm,"bytesize %d\n",lpdcb->ByteSize);
	port.c_cflag &= ~CSIZE;
	switch (lpdcb->ByteSize) {
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
			ptr->commerror = IE_BYTESIZE;
			return -1;
	}

    	TRACE(comm,"fParity %d Parity %d\n",lpdcb->fParity, lpdcb->Parity);
	port.c_cflag &= ~(PARENB | PARODD);
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
                default:
                        ptr->commerror = IE_BYTESIZE;
                        return -1;
        }
	

    	TRACE(comm,"stopbits %d\n",lpdcb->StopBits);

	switch (lpdcb->StopBits) {
		case ONESTOPBIT:
				port.c_cflag &= ~CSTOPB;
				break;
		case TWOSTOPBITS:
				port.c_cflag |= CSTOPB;
				break;
		default:
			ptr->commerror = IE_BYTESIZE;
			return -1;
	}
#ifdef CRTSCTS

	if (lpdcb->fDtrflow || lpdcb->fRtsflow || lpdcb->fOutxCtsFlow)
		port.c_cflag |= CRTSCTS;

	if (lpdcb->fDtrDisable) 
		port.c_cflag &= ~CRTSCTS;
#endif	
	if (lpdcb->fInX)
		port.c_iflag |= IXON;
	else
		port.c_iflag &= ~IXON;
	if (lpdcb->fOutX)
		port.c_iflag |= IXOFF;
	else
		port.c_iflag &= ~IXOFF;

	if (tcsetattr(ptr->fd, TCSADRAIN, &port) == -1) {
		ptr->commerror = WinError();	
		return FALSE;
	} else {
		ptr->commerror = 0;
		return 0;
	}
}

/*****************************************************************************
 *	GetCommState	(USER.202)
 */
INT16 WINAPI GetCommState16(INT16 cid, LPDCB16 lpdcb)
{
	struct DosDeviceStruct *ptr;
	struct termios port;

    	TRACE(comm,"cid %d, ptr %p\n", cid, lpdcb);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		return -1;
	}
	if (tcgetattr(ptr->fd, &port) == -1) {
		ptr->commerror = WinError();	
		return -1;
	}
	lpdcb->Id = cid;
#ifndef __EMX__
#ifdef CBAUD
        switch (port.c_cflag & CBAUD) {
#else
        switch (port.c_ospeed) {
#endif
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
			lpdcb->BaudRate = 57601;
			break;
#endif
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
	}	
	
        if(port.c_iflag & INPCK)
            lpdcb->fParity = TRUE;
        else
            lpdcb->fParity = FALSE;
	switch (port.c_cflag & (PARENB | PARODD)) {
		case 0:
			lpdcb->Parity = NOPARITY;
			break;
		case PARENB:
			lpdcb->Parity = EVENPARITY;
			break;
		case (PARENB | PARODD):
			lpdcb->Parity = ODDPARITY;		
			break;
	}

	if (port.c_cflag & CSTOPB)
		lpdcb->StopBits = TWOSTOPBITS;
	else
		lpdcb->StopBits = ONESTOPBIT;

	lpdcb->RlsTimeout = 50;
	lpdcb->CtsTimeout = 50; 
	lpdcb->DsrTimeout = 50;
	lpdcb->fNull = 0;
	lpdcb->fChEvt = 0;
	lpdcb->fBinary = 1;
	lpdcb->fDtrDisable = 0;

#ifdef CRTSCTS

	if (port.c_cflag & CRTSCTS) {
		lpdcb->fDtrflow = 1;
		lpdcb->fRtsflow = 1;
		lpdcb->fOutxCtsFlow = 1;
		lpdcb->fOutxDsrFlow = 1;
	} else 
#endif
		lpdcb->fDtrDisable = 1;

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

	ptr->commerror = 0;
	return 0;
}

/*****************************************************************************
 *	TransmitCommChar	(USER.206)
 */
INT16 WINAPI TransmitCommChar16(INT16 cid,CHAR chTransmit)
{
	struct DosDeviceStruct *ptr;

    	TRACE(comm, "cid %d, data %d \n", cid, chTransmit);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		return -1;
	}

	if (ptr->suspended) {
		ptr->commerror = IE_HARDWARE;
		return -1;
	}	

	if (write(ptr->fd, (void *) &chTransmit, 1) == -1) {
		ptr->commerror = WinError();
		return -1;	
	}  else {
		ptr->commerror = 0;
		return 0;
	}
}

/*****************************************************************************
 *	UngetCommChar	(USER.212)
 */
INT16 WINAPI UngetCommChar16(INT16 cid,CHAR chUnget)
{
	struct DosDeviceStruct *ptr;

    	TRACE(comm,"cid %d (char %d)\n", cid, chUnget);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		return -1;
	}

	if (ptr->suspended) {
		ptr->commerror = IE_HARDWARE;
		return -1;
	}	

	ptr->unget = 1;
	ptr->unget_byte = chUnget;
	ptr->commerror = 0;
	return 0;
}

/*****************************************************************************
 *	ReadComm	(USER.204)
 */
INT16 WINAPI ReadComm16(INT16 cid,LPSTR lpvBuf,INT16 cbRead)
{
	int status, length;
	struct DosDeviceStruct *ptr;

    	TRACE(comm, "cid %d, ptr %p, length %d\n", cid, lpvBuf, cbRead);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		return -1;
	}

	if (ptr->suspended) {
		ptr->commerror = IE_HARDWARE;
		return -1;
	}	

	if (ptr->unget) {
		*lpvBuf = ptr->unget_byte;
		lpvBuf++;
		ptr->unget = 0;

		length = 1;
	} else
	 	length = 0;

	status = read(ptr->fd, (void *) lpvBuf, cbRead);

	if (status == -1) {
                if (errno != EAGAIN) {
			ptr->commerror = WinError();
			return -1 - length;
                } else {
			ptr->commerror = 0;
			return length;
                }
 	} else {
	        TRACE(comm,"%.*s\n", length+status, lpvBuf);
		ptr->commerror = 0;
		return length + status;
	}
}

/*****************************************************************************
 *	WriteComm	(USER.205)
 */
INT16 WINAPI WriteComm16(INT16 cid, LPSTR lpvBuf, INT16 cbWrite)
{
	int length;
	struct DosDeviceStruct *ptr;

    	TRACE(comm,"cid %d, ptr %p, length %d\n", 
		cid, lpvBuf, cbWrite);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		return -1;
	}

	if (ptr->suspended) {
		ptr->commerror = IE_HARDWARE;
		return -1;
	}	
	
	TRACE(comm,"%.*s\n", cbWrite, lpvBuf );
	length = write(ptr->fd, (void *) lpvBuf, cbWrite);
	
	if (length == -1) {
		ptr->commerror = WinError();
		return -1;	
	} else {
		ptr->commerror = 0;	
		return length;
	}
}

/***********************************************************************
 *           EnableCommNotification   (USER.246)
 */
BOOL16 WINAPI EnableCommNotification16( INT16 cid, HWND16 hwnd,
                                      INT16 cbWriteNotify, INT16 cbOutQueue )
{
	struct DosDeviceStruct *ptr;

	FIXME(comm, "(%d, %x, %d, %d):stub.\n", cid, hwnd, cbWriteNotify, cbOutQueue);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		ptr->commerror = IE_BADID;
		return -1;
	}
	ptr->wnd = hwnd;
	ptr->n_read = cbWriteNotify;
	ptr->n_write = cbOutQueue;
	return TRUE;
}


/**************************************************************************
 *         BuildCommDCBA		(KERNEL32.14)
 */
BOOL WINAPI BuildCommDCBA(LPCSTR device,LPDCB lpdcb)
{
	return BuildCommDCBAndTimeoutsA(device,lpdcb,NULL);
}

/**************************************************************************
 *         BuildCommDCBAndTimeoutsA	(KERNEL32.15)
 */
BOOL WINAPI BuildCommDCBAndTimeoutsA(LPCSTR device, LPDCB lpdcb,
                                         LPCOMMTIMEOUTS lptimeouts)
{
	int	port;
	char	*ptr,*temp;

	TRACE(comm,"(%s,%p,%p)\n",device,lpdcb,lptimeouts);

	if (!lstrncmpiA(device,"COM",3)) {
		port=device[3]-'0';
		if (port--==0) {
			ERR(comm,"BUG! COM0 can't exists!.\n");
			return FALSE;
		}
		if (!ValidCOMPort(port))
			return FALSE;
		if (*(device+4)!=':')
			return FALSE;
		temp=(LPSTR)(device+5);
	} else
		temp=(LPSTR)device;

	memset(lpdcb, 0, sizeof(DCB)); /* initialize */

	lpdcb->DCBlength	= sizeof(DCB);
	if (strchr(temp,',')) {	/* old style */
		DCB16	dcb16;
		BOOL16	ret;
		char	last=temp[strlen(temp)-1];

		ret=BuildCommDCB16(device,&dcb16);
		if (!ret)
			return FALSE;
		lpdcb->BaudRate		= dcb16.BaudRate;
		lpdcb->ByteSize		= dcb16.ByteSize;
		lpdcb->fBinary		= dcb16.fBinary;
		lpdcb->Parity		= dcb16.Parity;
		lpdcb->fParity		= dcb16.fParity;
		lpdcb->fNull		= dcb16.fNull;
		lpdcb->StopBits		= dcb16.StopBits;
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
			lpdcb->fOutxDsrFlow	= TRUE;
			lpdcb->fDtrControl	= DTR_CONTROL_HANDSHAKE;
			lpdcb->fRtsControl	= RTS_CONTROL_HANDSHAKE;
		} else {
			lpdcb->fInX		= FALSE;
			lpdcb->fOutX		= FALSE;
			lpdcb->fOutxCtsFlow	= FALSE;
			lpdcb->fOutxDsrFlow	= FALSE;
			lpdcb->fDtrControl	= DTR_CONTROL_ENABLE;
			lpdcb->fRtsControl	= RTS_CONTROL_ENABLE;
		}
		lpdcb->XonChar	= dcb16.XonChar;
		lpdcb->XoffChar	= dcb16.XoffChar;
		lpdcb->ErrorChar= dcb16.PeChar;
		lpdcb->fErrorChar= dcb16.fPeChar;
		lpdcb->EofChar	= dcb16.EofChar;
		lpdcb->EvtChar	= dcb16.EvtChar;
		lpdcb->XonLim	= dcb16.XonLim;
		lpdcb->XoffLim	= dcb16.XoffLim;
		return TRUE;
	}
	ptr=strtok(temp," "); 
	while (ptr) {
		DWORD	flag,x;

		flag=0;
		if (!strncmp("baud=",ptr,5)) {
			if (!sscanf(ptr+5,"%ld",&x))
				WARN(comm,"Couldn't parse %s\n",ptr);
			lpdcb->BaudRate = x;
			flag=1;
		}
		if (!strncmp("stop=",ptr,5)) {
			if (!sscanf(ptr+5,"%ld",&x))
				WARN(comm,"Couldn't parse %s\n",ptr);
			lpdcb->StopBits = x;
			flag=1;
		}
		if (!strncmp("data=",ptr,5)) {
			if (!sscanf(ptr+5,"%ld",&x))
				WARN(comm,"Couldn't parse %s\n",ptr);
			lpdcb->ByteSize = x;
			flag=1;
		}
		if (!strncmp("parity=",ptr,7)) {
			lpdcb->fParity	= TRUE;
			switch (ptr[8]) {
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
			}
			flag=1;
		}
		if (!flag)
			ERR(comm,"Unhandled specifier '%s', please report.\n",ptr);
		ptr=strtok(NULL," ");
	}
	if (lpdcb->BaudRate==110)
		lpdcb->StopBits = 2;
	return TRUE;
}

/**************************************************************************
 *         BuildCommDCBAndTimeoutsW		(KERNEL32.16)
 */
BOOL WINAPI BuildCommDCBAndTimeoutsW( LPCWSTR devid, LPDCB lpdcb,
                                          LPCOMMTIMEOUTS lptimeouts )
{
	LPSTR	devidA;
	BOOL	ret;

	TRACE(comm,"(%p,%p,%p)\n",devid,lpdcb,lptimeouts);
	devidA = HEAP_strdupWtoA( GetProcessHeap(), 0, devid );
	ret=BuildCommDCBAndTimeoutsA(devidA,lpdcb,lptimeouts);
        HeapFree( GetProcessHeap(), 0, devidA );
	return ret;
}

/**************************************************************************
 *         BuildCommDCBW		(KERNEL32.17)
 */
BOOL WINAPI BuildCommDCBW(LPCWSTR devid,LPDCB lpdcb)
{
	return BuildCommDCBAndTimeoutsW(devid,lpdcb,NULL);
}

/*****************************************************************************
 *      COMM_Handle2fd
 *  returns a file descriptor for reading from or writing to
 *  mode is GENERIC_READ or GENERIC_WRITE. Make sure to close
 *  the handle afterwards!
 */
int COMM_Handle2fd(HANDLE handle, int mode) {
    struct get_read_fd_request r_req;
    struct get_write_fd_request w_req;
    int fd;

    w_req.handle = r_req.handle = handle;

    switch(mode) {
    case GENERIC_WRITE:
        CLIENT_SendRequest( REQ_GET_WRITE_FD, -1, 1, &w_req, sizeof(w_req) );
        break;
    case GENERIC_READ:
        CLIENT_SendRequest( REQ_GET_READ_FD, -1, 1, &r_req, sizeof(r_req) );
        break;
    default:
        ERR(comm,"COMM_Handle2fd: Don't know what type of fd is required.\n");
        return -1;
    }
    CLIENT_WaitReply( NULL, &fd, 0 );

    return fd;
}

/* FIXME: having these global for win32 for now */
int commerror=0,eventmask=0;

/*****************************************************************************
 *	SetCommBreak		(KERNEL32.449)
 */
BOOL WINAPI SetCommBreak(HANDLE handle)
{
	FIXME(comm,"handle %d, stub!\n", handle);
	return TRUE;
}

/*****************************************************************************
 *	ClearCommBreak		(KERNEL32.20)
 */
BOOL WINAPI ClearCommBreak(HANDLE handle)
{
	FIXME(comm,"handle %d, stub!\n", handle);
	return TRUE;
}

/*****************************************************************************
 *	EscapeCommFunction	(KERNEL32.214)
 */
BOOL WINAPI EscapeCommFunction(HANDLE handle,UINT nFunction)
{
	int fd;
	struct termios	port;

    	TRACE(comm,"handle %d, function=%d\n", handle, nFunction);
	fd = COMM_Handle2fd(handle, GENERIC_WRITE);
	if(fd<0)
		return FALSE;

	if (tcgetattr(fd,&port) == -1) {
		commerror=WinError();
		close(fd);
		return FALSE;
	}

	switch (nFunction) {
		case RESETDEV:
			break;					

#ifdef TIOCM_DTR
		case CLRDTR:
			port.c_cflag &= TIOCM_DTR;
			break;
#endif

#ifdef TIOCM_RTS
		case CLRRTS:
			port.c_cflag &= TIOCM_RTS;
			break;
#endif
	
#ifdef CRTSCTS
		case SETDTR:
			port.c_cflag |= CRTSCTS;
			break;

		case SETRTS:
			port.c_cflag |= CRTSCTS;
			break;
#endif

		case SETXOFF:
			port.c_iflag |= IXOFF;
			break;

		case SETXON:
			port.c_iflag |= IXON;
			break;
		case SETBREAK:
			FIXME(comm,"setbreak, stub\n");
/*			ptr->suspended = 1; */
			break;
		case CLRBREAK:
			FIXME(comm,"clrbreak, stub\n");
/*			ptr->suspended = 0; */
			break;
		default:
			WARN(comm,"(handle=%d,nFunction=%d): Unknown function\n", 
			handle, nFunction);
			break;				
	}
	
	if (tcsetattr(fd, TCSADRAIN, &port) == -1) {
		commerror = WinError();
		close(fd);
		return FALSE;	
	} else {
		commerror = 0;
		close(fd);
		return TRUE;
	}
}

/********************************************************************
 *      PurgeComm        (KERNEL32.557)
 */
BOOL WINAPI PurgeComm( HANDLE handle, DWORD flags) 
{
     int fd;

     TRACE(comm,"handle %d, flags %lx\n", handle, flags);

     fd = COMM_Handle2fd(handle, GENERIC_WRITE);
     if(fd<0)
         return FALSE;

     /*
     ** not exactly sure how these are different
     ** Perhaps if we had our own internal queues, one flushes them
     ** and the other flushes the kernel's buffers.
     */
     if(flags&PURGE_TXABORT)
     {
         tcflush(fd,TCOFLUSH);
     }
     if(flags&PURGE_RXABORT)
     {
         tcflush(fd,TCIFLUSH);
     }
     if(flags&PURGE_TXCLEAR)
     {
         tcflush(fd,TCOFLUSH);
     }
     if(flags&PURGE_RXCLEAR)
     {
         tcflush(fd,TCIFLUSH);
     }
     close(fd);

     return 1;
}

/*****************************************************************************
 *	ClearCommError	(KERNEL32.21)
 */
BOOL WINAPI ClearCommError(INT handle,LPDWORD errors,LPCOMSTAT lpStat)
{
    int fd;

    fd=COMM_Handle2fd(handle,GENERIC_READ);
    if(0>fd) 
    {
        return FALSE;
    }

    if (lpStat) 
    {
	lpStat->status = 0;

	if(ioctl(fd, TIOCOUTQ, &lpStat->cbOutQue))
	    WARN(comm, "ioctl returned error\n");

	if(ioctl(fd, TIOCINQ, &lpStat->cbInQue))
	    WARN(comm, "ioctl returned error\n");
    }

    close(fd);

    TRACE(comm,"handle %d cbInQue = %ld cbOutQue = %ld\n",
                handle,
                lpStat->cbInQue,
                lpStat->cbOutQue);

    if(errors)
        *errors = 0;

    /*
    ** After an asynchronous write opperation, the
    ** app will call ClearCommError to see if the
    ** results are ready yet. It waits for ERROR_IO_PENDING
    */
    commerror = ERROR_IO_PENDING;

    return TRUE;
}

/*****************************************************************************
 *      SetupComm       (KERNEL32.676)
 */
BOOL WINAPI SetupComm( HANDLE handle, DWORD insize, DWORD outsize)
{
    int fd;

    FIXME(comm, "insize %ld outsize %ld unimplemented stub\n", insize, outsize);
    fd=COMM_Handle2fd(handle,GENERIC_WRITE);
    if(0>fd)
    {
        return FALSE;
    }
    close(fd);
    return TRUE;
} 

/*****************************************************************************
 *	GetCommMask	(KERNEL32.156)
 */
BOOL WINAPI GetCommMask(HANDLE handle,LPDWORD evtmask)
{
    int fd;

    TRACE(comm, "handle %d, mask %p\n", handle, evtmask);
    if(0>(fd=COMM_Handle2fd(handle,GENERIC_READ))) 
    {
        return FALSE;
    }
    close(fd);
    *evtmask = eventmask;
    return TRUE;
}

/*****************************************************************************
 *	SetCommMask	(KERNEL32.451)
 */
BOOL WINAPI SetCommMask(INT handle,DWORD evtmask)
{
    int fd;

    TRACE(comm, "handle %d, mask %lx\n", handle, evtmask);
    if(0>(fd=COMM_Handle2fd(handle,GENERIC_WRITE))) {
        return FALSE;
    }
    close(fd);
    eventmask = evtmask;
    return TRUE;
}

/*****************************************************************************
 *	SetCommState    (KERNEL32.452)
 */
BOOL WINAPI SetCommState(INT handle,LPDCB lpdcb)
{
     struct termios port;
     int fd;
     struct get_write_fd_request req;

     TRACE(comm,"handle %d, ptr %p\n", handle, lpdcb);

     req.handle = handle;
     CLIENT_SendRequest( REQ_GET_WRITE_FD, -1, 1, &req, sizeof(req) );
     CLIENT_WaitReply( NULL, &fd, 0 );

     if(fd<0)
         return FALSE;

     if (tcgetattr(fd,&port) == -1) {
         commerror = WinError();	
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

     /*
     ** MJM - removed default baudrate settings
     ** TRACE(comm,"baudrate %ld\n",lpdcb->BaudRate);
     */
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
		default:
			commerror = IE_BAUDRATE;
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
                default:
                        commerror = IE_BAUDRATE;
                        return FALSE;
        }
        port.c_ispeed = port.c_ospeed;
#endif
    	TRACE(comm,"bytesize %d\n",lpdcb->ByteSize);
	port.c_cflag &= ~CSIZE;
	switch (lpdcb->ByteSize) {
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
			commerror = IE_BYTESIZE;
			return FALSE;
	}

    	TRACE(comm,"fParity %d Parity %d\n",lpdcb->fParity, lpdcb->Parity);
	port.c_cflag &= ~(PARENB | PARODD);
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
                default:
                        commerror = IE_BYTESIZE;
                        return FALSE;
        }
	

    	TRACE(comm,"stopbits %d\n",lpdcb->StopBits);
	switch (lpdcb->StopBits) {
		case ONESTOPBIT:
				port.c_cflag &= ~CSTOPB;
				break;
		case TWOSTOPBITS:
				port.c_cflag |= CSTOPB;
				break;
		default:
			commerror = IE_BYTESIZE;
			return FALSE;
	}
#ifdef CRTSCTS
	if (	lpdcb->fOutxCtsFlow 			||
		lpdcb->fDtrControl == DTR_CONTROL_ENABLE||
		lpdcb->fRtsControl == RTS_CONTROL_ENABLE
	)
		port.c_cflag |= CRTSCTS;
	if (lpdcb->fDtrControl == DTR_CONTROL_DISABLE)
		port.c_cflag &= ~CRTSCTS;

#endif	
	if (lpdcb->fInX)
		port.c_iflag |= IXON;
	else
		port.c_iflag &= ~IXON;
	if (lpdcb->fOutX)
		port.c_iflag |= IXOFF;
	else
		port.c_iflag &= ~IXOFF;

	if (tcsetattr(fd,TCSADRAIN,&port)==-1) {
		commerror = WinError();	
		return FALSE;
	} else {
		commerror = 0;
		return TRUE;
	}
}


/*****************************************************************************
 *	GetCommState	(KERNEL32.159)
 */
BOOL WINAPI GetCommState(INT handle, LPDCB lpdcb)
{
     struct termios port;
     int fd;
     struct get_read_fd_request req;

     TRACE(comm,"handle %d, ptr %p\n", handle, lpdcb);
     req.handle = handle;
     CLIENT_SendRequest( REQ_GET_READ_FD, -1, 1, &req, sizeof(req) );
     CLIENT_WaitReply( NULL, &fd, 0 );

     if(fd<0)
         return FALSE;

     if (tcgetattr(fd, &port) == -1) {
        TRACE(comm,"tcgetattr(%d, ...) returned -1",fd);
		commerror = WinError();	
		return FALSE;
	}
#ifndef __EMX__
#ifdef CBAUD
        switch (port.c_cflag & CBAUD) {
#else
        switch (port.c_ospeed) {
#endif
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
	}	
	
        if(port.c_iflag & INPCK)
            lpdcb->fParity = TRUE;
        else
            lpdcb->fParity = FALSE;
	switch (port.c_cflag & (PARENB | PARODD)) {
		case 0:
			lpdcb->Parity = NOPARITY;
			break;
		case PARENB:
			lpdcb->Parity = EVENPARITY;
			break;
		case (PARENB | PARODD):
			lpdcb->Parity = ODDPARITY;		
			break;
	}

	if (port.c_cflag & CSTOPB)
		lpdcb->StopBits = TWOSTOPBITS;
	else
		lpdcb->StopBits = ONESTOPBIT;

	lpdcb->fNull = 0;
	lpdcb->fBinary = 1;

#ifdef CRTSCTS

	if (port.c_cflag & CRTSCTS) {
		lpdcb->fDtrControl = DTR_CONTROL_ENABLE;
		lpdcb->fRtsControl = RTS_CONTROL_ENABLE;
		lpdcb->fOutxCtsFlow = 1;
		lpdcb->fOutxDsrFlow = 1;
	} else 
#endif
	{
		lpdcb->fDtrControl = DTR_CONTROL_DISABLE;
		lpdcb->fRtsControl = RTS_CONTROL_DISABLE;
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

	commerror = 0;

     TRACE(comm,"OK\n");
 
	return TRUE;
}

/*****************************************************************************
 *	TransmitCommChar	(KERNEL32.535)
 */
BOOL WINAPI TransmitCommChar(INT cid,CHAR chTransmit)
{
	struct DosDeviceStruct *ptr;

    	FIXME(comm,"(%d,'%c'), use win32 handle!\n",cid,chTransmit);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		return FALSE;
	}

	if (ptr->suspended) {
		ptr->commerror = IE_HARDWARE;
		return FALSE;
	}
	if (write(ptr->fd, (void *) &chTransmit, 1) == -1) {
		ptr->commerror = WinError();
		return FALSE;
	}  else {
		ptr->commerror = 0;
		return TRUE;
	}
}

/*****************************************************************************
 *	GetCommTimeouts		(KERNEL32.160)
 */
BOOL WINAPI GetCommTimeouts(INT cid,LPCOMMTIMEOUTS lptimeouts)
{
	FIXME(comm,"(%x,%p):stub.\n",cid,lptimeouts);
	return TRUE;
}

/*****************************************************************************
 *	SetCommTimeouts		(KERNEL32.453)
 */
BOOL WINAPI SetCommTimeouts(INT cid,LPCOMMTIMEOUTS lptimeouts) {
	FIXME(comm,"(%x,%p):stub.\n",cid,lptimeouts);
	return TRUE;
}

/***********************************************************************
 *           GetCommModemStatus   (KERNEL32.285)
 */
BOOL WINAPI GetCommModemStatus(HANDLE hFile,LPDWORD lpModemStat )
{
	FIXME(comm, "(%d %p)\n",hFile,lpModemStat );
	return TRUE;
}
/***********************************************************************
 *           WaitCommEvent   (KERNEL32.719)
 */
BOOL WINAPI WaitCommEvent(HANDLE hFile,LPDWORD eventmask ,LPOVERLAPPED overlapped)
{
	FIXME(comm, "(%d %p %p )\n",hFile, eventmask,overlapped);
	return TRUE;
}

/***********************************************************************
 *           GetCommProperties   (KERNEL32.???)
 */
BOOL WINAPI GetCommProperties(HANDLE hFile, LPDCB *dcb)
{
    FIXME(comm, "(%d %p )\n",hFile,dcb);
    return TRUE;
}

/***********************************************************************
 *           SetCommProperties   (KERNEL32.???)
 */
BOOL WINAPI SetCommProperties(HANDLE hFile, LPDCB dcb)
{
    FIXME(comm, "(%d %p )\n",hFile,dcb);
    return TRUE;
}

