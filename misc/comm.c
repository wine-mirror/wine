 /*
 * DEC 93 Erik Bos <erik@xs4all.nl>
 *
 * Copyright 1996 Marcus Meissner
 *
 * Mar 31, 1999. Ove Kåven <ovek@arcticnet.no>
 * - Implemented buffers and EnableCommNotification.
 *
 * Apr 3, 1999.  Lawson Whitney <lawson_whitney@juno.com>
 * - Fixed the modem control part of EscapeCommFunction16.
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
 * Oktober 98, Rein Klazes [RHK]
 * A program that wants to monitor the modem status line (RLSD/DCD) may
 * poll the modem status register in the commMask structure. I update the bit
 * in GetCommError, waiting for an implementation of communication events.
 * 
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
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

#include "windef.h"
#include "wingdi.h"
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

#include "server.h"
#include "process.h"
#include "winerror.h"
#include "services.h"
#include "file.h"

#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(comm);

#ifndef TIOCINQ
#define	TIOCINQ FIONREAD
#endif
#define COMM_MSR_OFFSET  35       /* see knowledge base Q101417 */
#define FLAG_LPT 0x80

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
				WARN("Can't use `%s' as %s !\n", temp, option);
			else
				if ((COM[x].devicename = malloc(strlen(temp)+1)) == NULL) 
					WARN("Can't malloc for device info!\n");
				else {
					COM[x].fd = 0;
					strcpy(COM[x].devicename, temp);
				}
                TRACE("%s = %s\n", option, COM[x].devicename);
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
				WARN("Can't use `%s' as %s !\n", temp, option);
			else 
				if ((LPT[x].devicename = malloc(strlen(temp)+1)) == NULL) 
					WARN("Can't malloc for device info!\n");
				else {
					LPT[x].fd = 0;
					strcpy(LPT[x].devicename, temp);
				}
                TRACE("%s = %s\n", option, LPT[x].devicename);
		}

	}
}


static struct DosDeviceStruct *GetDeviceStruct(int fd)
{
	if ((fd&0x7F)<=MAX_PORTS) {
            if (!(fd&FLAG_LPT)) {
		if (COM[fd].fd)
		    return &COM[fd];
	    } else {
		fd &= 0x7f;
		if (LPT[fd].fd)
		    return &LPT[fd];
	    }
	}

	return NULL;
}

static int    GetCommPort_fd(int fd)
{
        int x;
        
        for (x=0; x<MAX_PORTS; x++) {
             if (COM[x].fd == fd)
                 return x;
       }
       
       return -1;
} 

static int ValidCOMPort(int x)
{
	return(x < MAX_PORTS ? (int) COM[x].devicename : 0); 
}

static int ValidLPTPort(int x)
{
	return(x < MAX_PORTS ? (int) LPT[x].devicename : 0); 
}

static int WinError(void)
{
        TRACE("errno = %d\n", errno);
	switch (errno) {
		default:
			return CE_IOE;
		}
}

static unsigned comm_inbuf(struct DosDeviceStruct *ptr)
{
  return ((ptr->ibuf_tail > ptr->ibuf_head) ? ptr->ibuf_size : 0)
    + ptr->ibuf_head - ptr->ibuf_tail;
}

static unsigned comm_outbuf(struct DosDeviceStruct *ptr)
{
  return ((ptr->obuf_tail > ptr->obuf_head) ? ptr->obuf_size : 0)
    + ptr->obuf_head - ptr->obuf_tail;
}

static int COMM_WhackModem(int fd, unsigned int andy, unsigned int orrie)
{
    unsigned int mstat, okay;
    okay = ioctl(fd, TIOCMGET, &mstat);
    if (okay) return okay;
    if (andy) mstat &= andy;
    mstat |= orrie;
    return ioctl(fd, TIOCMSET, &mstat);
}
	
static void CALLBACK comm_notification( ULONG_PTR private )
{
  struct DosDeviceStruct *ptr = (struct DosDeviceStruct *)private;
  int prev, bleft, len;
  WORD mask = 0;
  int cid = GetCommPort_fd(ptr->fd);

  TRACE("async notification\n");
  /* read data from comm port */
  prev = comm_inbuf(ptr);
  do {
    bleft = ((ptr->ibuf_tail > ptr->ibuf_head) ? (ptr->ibuf_tail-1) : ptr->ibuf_size)
      - ptr->ibuf_head;
    len = read(ptr->fd, ptr->inbuf + ptr->ibuf_head, bleft?bleft:1);
    if (len > 0) {
      if (!bleft) {
	ptr->commerror = CE_RXOVER;
      } else {
	/* check for events */
	if ((ptr->eventmask & EV_RXFLAG) &&
	    memchr(ptr->inbuf + ptr->ibuf_head, ptr->evtchar, len)) {
	  *(WORD*)(unknown[cid]) |= EV_RXFLAG;
	  mask |= CN_EVENT;
	}
	if (ptr->eventmask & EV_RXCHAR) {
	  *(WORD*)(unknown[cid]) |= EV_RXCHAR;
	  mask |= CN_EVENT;
	}
	/* advance buffer position */
	ptr->ibuf_head += len;
	if (ptr->ibuf_head >= ptr->ibuf_size)
	  ptr->ibuf_head = 0;
      }
    }
  } while (len > 0);
  /* check for notification */
  if (ptr->wnd && (ptr->n_read>0) && (prev<ptr->n_read) &&
      (comm_inbuf(ptr)>=ptr->n_read)) {
    /* passed the receive notification threshold */
    mask |= CN_RECEIVE;
  }

  /* write any TransmitCommChar character */
  if (ptr->xmit>=0) {
    len = write(ptr->fd, &(ptr->xmit), 1);
    if (len > 0) ptr->xmit = -1;
  }
  /* write from output queue */
  prev = comm_outbuf(ptr);
  do {
    bleft = ((ptr->obuf_tail <= ptr->obuf_head) ? ptr->obuf_head : ptr->obuf_size)
      - ptr->obuf_tail;
    len = bleft ? write(ptr->fd, ptr->outbuf + ptr->obuf_tail, bleft) : 0;
    if (len > 0) {
      ptr->obuf_tail += len;
      if (ptr->obuf_tail >= ptr->obuf_size)
	ptr->obuf_tail = 0;
      /* flag event */
      if (ptr->obuf_tail == ptr->obuf_head) {
	if (ptr->s_write) {
	  SERVICE_Delete( ptr->s_write );
	  ptr->s_write = INVALID_HANDLE_VALUE;
	}
        if (ptr->eventmask & EV_TXEMPTY) {
	  *(WORD*)(unknown[cid]) |= EV_TXEMPTY;
	  mask |= CN_EVENT;
	}
      }
    }
  } while (len > 0);
  /* check for notification */
  if (ptr->wnd && (ptr->n_write>0) && (prev>=ptr->n_write) &&
      (comm_outbuf(ptr)<ptr->n_write)) {
    /* passed the transmit notification threshold */
    mask |= CN_TRANSMIT;
  }

  /* send notifications, if any */
  if (ptr->wnd && mask) {
    TRACE("notifying %04x: cid=%d, mask=%02x\n", ptr->wnd, cid, mask);
    PostMessage16(ptr->wnd, WM_COMMNOTIFY, cid, mask);
  }
}

static void comm_waitread(struct DosDeviceStruct *ptr)
{
  if (ptr->s_read != INVALID_HANDLE_VALUE) return;
  ptr->s_read = SERVICE_AddObject( FILE_DupUnixHandle( ptr->fd,
                                     GENERIC_READ | SYNCHRONIZE ),
                                    comm_notification,
                                    (ULONG_PTR)ptr );
}

static void comm_waitwrite(struct DosDeviceStruct *ptr)
{
  if (ptr->s_write != INVALID_HANDLE_VALUE) return;
  ptr->s_write = SERVICE_AddObject( FILE_DupUnixHandle( ptr->fd,
                                      GENERIC_WRITE | SYNCHRONIZE ),
                                     comm_notification,
                                     (ULONG_PTR)ptr );
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

	TRACE("(%s), ptr %p\n", device, lpdcb);

	if (!lstrncmpiA(device,"COM",3)) {
		port = device[3] - '0';
	

		if (port-- == 0) {
			ERR("BUG ! COM0 can't exist!.\n");
			return -1;
		}

		if (!ValidCOMPort(port)) {
			FIXME("invalid COM port %d?\n",port);
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
        	TRACE("baudrate (%d)\n", lpdcb->BaudRate);

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
			default:
				WARN("Unknown parity `%c'!\n", *ptr);
				return -1;
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

    	TRACE("%s, %d, %d\n", device, cbInQueue, cbOutQueue);

	if (strlen(device) < 4)
	   return IE_BADID;

	port = device[3] - '0';

	if (port-- == 0)
		ERR("BUG ! COM0 or LPT0 don't exist !\n");

	if (!lstrncmpiA(device,"COM",3)) {
		
                TRACE("%s = %s\n", device, COM[port].devicename);

		if (!ValidCOMPort(port))
			return IE_BADID;

		if (COM[port].fd)
			return IE_OPEN;

		fd = open(COM[port].devicename, O_RDWR | O_NONBLOCK);
		if (fd == -1) {
			ERR("error=%d\n", errno);
			return IE_HARDWARE;
		} else {
                        unknown[port] = SEGPTR_ALLOC(40);
                        bzero(unknown[port],40);
			COM[port].fd = fd;
			COM[port].commerror = 0;
			COM[port].eventmask = 0;
			COM[port].evtchar = 0; /* FIXME: default? */
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
			/* init priority characters */
			COM[port].unget = -1;
			COM[port].xmit = -1;
			/* allocate buffers */
			COM[port].ibuf_size = cbInQueue;
			COM[port].ibuf_head = COM[port].ibuf_tail= 0;
			COM[port].obuf_size = cbOutQueue;
			COM[port].obuf_head = COM[port].obuf_tail = 0;

			COM[port].inbuf = malloc(cbInQueue);
			if (COM[port].inbuf) {
			  COM[port].outbuf = malloc(cbOutQueue);
			  if (!COM[port].outbuf)
			    free(COM[port].inbuf);
			} else COM[port].outbuf = NULL;
			if (!COM[port].outbuf) {
			  /* not enough memory */
			  tcsetattr(COM[port].fd,TCSANOW,&m_stat[port]);
			  close(COM[port].fd);
			  ERR("out of memory");
			  return IE_MEMORY;
			}

                        COM[port].s_read = INVALID_HANDLE_VALUE;
                        COM[port].s_write = INVALID_HANDLE_VALUE;
                        comm_waitread( &COM[port] );
			return port;
		}
	} 
	else 
	if (!lstrncmpiA(device,"LPT",3)) {
	
		if (!ValidLPTPort(port))
			return IE_BADID;

		if (LPT[port].fd)
			return IE_OPEN;

		fd = open(LPT[port].devicename, O_RDWR | O_NONBLOCK, 0);
		if (fd == -1) {
			return IE_HARDWARE;
		} else {
			LPT[port].fd = fd;
			LPT[port].commerror = 0;
			LPT[port].eventmask = 0;
			return port|FLAG_LPT;
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
        
    	TRACE("cid=%d\n", cid);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no cid=%d found!\n", cid);
		return -1;
	}
	if (!(cid&FLAG_LPT)) {
		/* COM port */
		SEGPTR_FREE(unknown[cid]); /* [LW] */

		SERVICE_Delete( COM[cid].s_write );
		SERVICE_Delete( COM[cid].s_read );
		/* free buffers */
		free(ptr->outbuf);
		free(ptr->inbuf);

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

	TRACE("cid=%d\n", cid);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no cid=%d found!\n", cid);
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

    	TRACE("cid=%d\n", cid);
	if (!(ptr = GetDeviceStruct(cid))) {
		FIXME("no cid=%d found!\n", cid);
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

    	TRACE("cid=%d, function=%d\n", cid, nFunction);
	if ((nFunction != GETMAXCOM) && (nFunction != GETMAXLPT)) {
		if ((ptr = GetDeviceStruct(cid)) == NULL) {
			FIXME("no cid=%d found!\n", cid);
			return -1;
		}
		if (tcgetattr(ptr->fd,&port) == -1) {
		        TRACE("tcgetattr failed\n");
			ptr->commerror=WinError();	
			return -1;
		}
	} else ptr = NULL;

	switch (nFunction) {
		case RESETDEV:
		        TRACE("RESETDEV\n");
			break;					

		case GETMAXCOM:
		        TRACE("GETMAXCOM\n"); 
			for (max = MAX_PORTS;!COM[max].devicename;max--)
				;
			return max;
			break;

		case GETMAXLPT:
		        TRACE("GETMAXLPT\n"); 
			for (max = MAX_PORTS;!LPT[max].devicename;max--)
				;
			return FLAG_LPT + max;
			break;

		case GETBASEIRQ:
		        TRACE("GETBASEIRQ\n"); 
			/* FIXME: use tables */
			/* just fake something for now */
			if (cid & FLAG_LPT) {
				/* LPT1: irq 7, LPT2: irq 5 */
				return (cid & 0x7f) ? 5 : 7;
			} else {
				/* COM1: irq 4, COM2: irq 3,
				   COM3: irq 4, COM4: irq 3 */
				return 4 - (cid & 1);
			}
			break;

		case CLRDTR:
		        TRACE("CLRDTR\n"); 
#ifdef TIOCM_DTR
			return COMM_WhackModem(ptr->fd, ~TIOCM_DTR, 0);
#endif
		case CLRRTS:
		        TRACE("CLRRTS\n"); 
#ifdef TIOCM_RTS
			return COMM_WhackModem(ptr->fd, ~TIOCM_RTS, 0);
#endif
	
		case SETDTR:
		        TRACE("SETDTR\n"); 
#ifdef TIOCM_DTR
			return COMM_WhackModem(ptr->fd, 0, TIOCM_DTR);
#endif

		case SETRTS:
		        TRACE("SETRTS\n"); 
#ifdef TIOCM_RTS			
			return COMM_WhackModem(ptr->fd, 0, TIOCM_RTS);
#endif

		case SETXOFF:
		        TRACE("SETXOFF\n"); 
			port.c_iflag |= IXOFF;
			break;

		case SETXON:
		        TRACE("SETXON\n"); 
			port.c_iflag |= IXON;
			break;

		default:
			WARN("(cid=%d,nFunction=%d): Unknown function\n", 
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

    	TRACE("cid=%d, queue=%d\n", cid, fnQueue);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no cid=%d found!\n", cid);
		return -1;
	}
	switch (fnQueue) {
		case 0:
		  queue = TCOFLUSH;
		  ptr->obuf_tail = ptr->obuf_head;
		  break;
		case 1:
		  queue = TCIFLUSH;
		  ptr->ibuf_head = ptr->ibuf_tail;
		  break;
		default:
		  WARN("(cid=%d,fnQueue=%d):Unknown queue\n", 
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
	struct DosDeviceStruct *ptr;
        unsigned char *stol;
        unsigned int mstat;

	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!.\n",cid);
		return -1;
	}
        if (cid&FLAG_LPT) {
            WARN(" cid %d not comm port\n",cid);
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

		lpStat->cbOutQue = comm_outbuf(ptr);
		lpStat->cbInQue = comm_inbuf(ptr);

    		TRACE("cid %d, error %d, lpStat %d %d %d stol %x\n",
			     cid, ptr->commerror, lpStat->status, lpStat->cbInQue, 
			     lpStat->cbOutQue, *stol);
	}
	else
		TRACE("cid %d, error %d, lpStat NULL stol %x\n",
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

    	TRACE("cid %d,mask %d\n",cid,fuEvtMask);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!.\n",cid);
	    return (SEGPTR)NULL;
	}

	ptr->eventmask = fuEvtMask;

        if ((cid&FLAG_LPT) || !ValidCOMPort(cid)) {
            WARN(" cid %d not comm port\n",cid);
            return (SEGPTR)NULL;
        }
        /* it's a COM port ? -> modify flags */
        stol = (unsigned char *)unknown[cid] + COMM_MSR_OFFSET;
	repid = ioctl(ptr->fd,TIOCMGET,&mstat);
	TRACE(" ioctl  %d, msr %x at %p %p\n",repid,mstat,stol,unknown[cid]);
	if ((mstat&TIOCM_CAR))
	    *stol |= 0x80;
	else
	    *stol &=0x7f;

	TRACE(" modem dcd construct %x\n",*stol);
	return SEGPTR_GET(unknown[cid]);
}

/*****************************************************************************
 *	GetCommEventMask	(USER.209)
 */
UINT16 WINAPI GetCommEventMask16(INT16 cid,UINT16 fnEvtClear)
{
	struct DosDeviceStruct *ptr;
	WORD events;

    	TRACE("cid %d, mask %d\n", cid, fnEvtClear);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!.\n",cid);
	    return 0;
	}

        if ((cid&FLAG_LPT) || !ValidCOMPort(cid)) {
            WARN(" cid %d not comm port\n",cid);
            return 0;
        }

	events = *(WORD*)(unknown[cid]) & fnEvtClear;
	*(WORD*)(unknown[cid]) &= ~fnEvtClear;
	return events;
}

/*****************************************************************************
 *	SetCommState16	(USER.201)
 */
INT16 WINAPI SetCommState16(LPDCB16 lpdcb)
{
	struct termios port;
	struct DosDeviceStruct *ptr;

    	TRACE("cid %d, ptr %p\n", lpdcb->Id, lpdcb);
	if ((ptr = GetDeviceStruct(lpdcb->Id)) == NULL) {
		FIXME("no handle for cid = %0x!.\n",lpdcb->Id);
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

    	TRACE("baudrate %d\n",lpdcb->BaudRate);
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
    	TRACE("bytesize %d\n",lpdcb->ByteSize);
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

    	TRACE("fParity %d Parity %d\n",lpdcb->fParity, lpdcb->Parity);
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
	

    	TRACE("stopbits %d\n",lpdcb->StopBits);

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

	ptr->evtchar = lpdcb->EvtChar;

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
	int speed;
	struct DosDeviceStruct *ptr;
	struct termios port;

    	TRACE("cid %d, ptr %p\n", cid, lpdcb);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!.\n",cid);
		return -1;
	}
	if (tcgetattr(ptr->fd, &port) == -1) {
		ptr->commerror = WinError();	
		return -1;
	}
	lpdcb->Id = cid;
#ifndef __EMX__
#ifdef CBAUD
        speed = port.c_cflag & CBAUD;
#else
        speed = port.c_ospeed;
#endif
        switch(speed) {
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

	lpdcb->EvtChar = ptr->evtchar;

	ptr->commerror = 0;
	return 0;
}

/*****************************************************************************
 *	TransmitCommChar	(USER.206)
 */
INT16 WINAPI TransmitCommChar16(INT16 cid,CHAR chTransmit)
{
	struct DosDeviceStruct *ptr;

    	TRACE("cid %d, data %d \n", cid, chTransmit);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!.\n",cid);
		return -1;
	}

	if (ptr->suspended) {
		ptr->commerror = IE_HARDWARE;
		return -1;
	}	

	if (ptr->xmit >= 0) {
	  /* character already queued */
	  /* FIXME: which error would Windows return? */
	  ptr->commerror = CE_TXFULL;
	  return -1;
	}

	if (ptr->obuf_head == ptr->obuf_tail) {
	  /* transmit queue empty, try to transmit directly */
	  if (write(ptr->fd, &chTransmit, 1) == -1) {
	    /* didn't work, queue it */
	    ptr->xmit = chTransmit;
	    comm_waitwrite(ptr);
	  }
	} else {
	  /* data in queue, let this char be transmitted next */
	  ptr->xmit = chTransmit;
	  comm_waitwrite(ptr);
	}

	ptr->commerror = 0;
	return 0;
}

/*****************************************************************************
 *	UngetCommChar	(USER.212)
 */
INT16 WINAPI UngetCommChar16(INT16 cid,CHAR chUnget)
{
	struct DosDeviceStruct *ptr;

    	TRACE("cid %d (char %d)\n", cid, chUnget);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!.\n",cid);
		return -1;
	}

	if (ptr->suspended) {
		ptr->commerror = IE_HARDWARE;
		return -1;
	}	

	if (ptr->unget>=0) {
	  /* character already queued */
	  /* FIXME: which error would Windows return? */
	  ptr->commerror = CE_RXOVER;
	  return -1;
	}

	ptr->unget = chUnget;

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
	LPSTR orgBuf = lpvBuf;

    	TRACE("cid %d, ptr %p, length %d\n", cid, lpvBuf, cbRead);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!.\n",cid);
		return -1;
	}

	if (ptr->suspended) {
		ptr->commerror = IE_HARDWARE;
		return -1;
	}	

	/* read unget character */
	if (ptr->unget>=0) {
		*lpvBuf++ = ptr->unget;
		ptr->unget = -1;

		length = 1;
	} else
	 	length = 0;

	/* read from receive buffer */
	while (length < cbRead) {
	  status = ((ptr->ibuf_head < ptr->ibuf_tail) ?
		    ptr->ibuf_size : ptr->ibuf_head) - ptr->ibuf_tail;
	  if (!status) break;
	  if ((cbRead - length) < status)
	    status = cbRead - length;

	  memcpy(lpvBuf, ptr->inbuf + ptr->ibuf_tail, status);
	  ptr->ibuf_tail += status;
	  if (ptr->ibuf_tail >= ptr->ibuf_size)
	    ptr->ibuf_tail = 0;
	  lpvBuf += status;
	  length += status;
	}

	TRACE("%.*s\n", length, orgBuf);
	ptr->commerror = 0;
	return length;
}

/*****************************************************************************
 *	WriteComm	(USER.205)
 */
INT16 WINAPI WriteComm16(INT16 cid, LPSTR lpvBuf, INT16 cbWrite)
{
	int status, length;
	struct DosDeviceStruct *ptr;

    	TRACE("cid %d, ptr %p, length %d\n", 
		cid, lpvBuf, cbWrite);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!.\n",cid);
		return -1;
	}

	if (ptr->suspended) {
		ptr->commerror = IE_HARDWARE;
		return -1;
	}	
	
	TRACE("%.*s\n", cbWrite, lpvBuf );

	length = 0;
	while (length < cbWrite) {
	  if ((ptr->obuf_head == ptr->obuf_tail) && (ptr->xmit < 0)) {
	    /* no data queued, try to write directly */
	    status = write(ptr->fd, lpvBuf, cbWrite - length);
	    if (status > 0) {
	      lpvBuf += status;
	      length += status;
	      continue;
	    }
	  }
	  /* can't write directly, put into transmit buffer */
	  status = ((ptr->obuf_tail > ptr->obuf_head) ?
		    (ptr->obuf_tail-1) : ptr->obuf_size) - ptr->obuf_head;
	  if (!status) break;
	  if ((cbWrite - length) < status)
	    status = cbWrite - length;
	  memcpy(lpvBuf, ptr->outbuf + ptr->obuf_head, status);
	  ptr->obuf_head += status;
	  if (ptr->obuf_head >= ptr->obuf_size)
	    ptr->obuf_head = 0;
	  lpvBuf += status;
	  length += status;
	  comm_waitwrite(ptr);
	}

	ptr->commerror = 0;	
	return length;
}

/***********************************************************************
 *           EnableCommNotification   (USER.246)
 */
BOOL16 WINAPI EnableCommNotification16( INT16 cid, HWND16 hwnd,
                                      INT16 cbWriteNotify, INT16 cbOutQueue )
{
	struct DosDeviceStruct *ptr;

	TRACE("(%d, %x, %d, %d)\n", cid, hwnd, cbWriteNotify, cbOutQueue);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!.\n",cid);
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

	TRACE("(%s,%p,%p)\n",device,lpdcb,lptimeouts);

	if (!lstrncmpiA(device,"COM",3)) {
		port=device[3]-'0';
		if (port--==0) {
			ERR("BUG! COM0 can't exists!.\n");
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
				WARN("Couldn't parse %s\n",ptr);
			lpdcb->BaudRate = x;
			flag=1;
		}
		if (!strncmp("stop=",ptr,5)) {
			if (!sscanf(ptr+5,"%ld",&x))
				WARN("Couldn't parse %s\n",ptr);
			lpdcb->StopBits = x;
			flag=1;
		}
		if (!strncmp("data=",ptr,5)) {
			if (!sscanf(ptr+5,"%ld",&x))
				WARN("Couldn't parse %s\n",ptr);
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
			ERR("Unhandled specifier '%s', please report.\n",ptr);
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

	TRACE("(%p,%p,%p)\n",devid,lpdcb,lptimeouts);
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
 *      COMM_GetReadFd
 *  Returns a file descriptor for reading.
 *  Make sure to close the handle afterwards!
 */
static int COMM_GetReadFd( HANDLE handle)
{
    int fd;
    struct get_read_fd_request *req = get_req_buffer();
    req->handle = handle;
    server_call_fd( REQ_GET_READ_FD, -1, &fd );
    return fd;
}

/*****************************************************************************
 *      COMM_GetWriteFd
 *  Returns a file descriptor for writing.
 *  Make sure to close the handle afterwards!
 */
static int COMM_GetWriteFd( HANDLE handle)
{
    int fd = -1;
    struct get_write_fd_request *req = get_req_buffer();
    req->handle = handle;
    server_call_fd( REQ_GET_WRITE_FD, -1, &fd );
    return fd;
}

/* FIXME: having these global for win32 for now */
int commerror=0,eventmask=0;

/*****************************************************************************
 *	SetCommBreak		(KERNEL32.449)
 */
BOOL WINAPI SetCommBreak(HANDLE handle)
{
#if defined(TIOCSBRK) && defined(TIOCCBRK) /* check if available for compilation */
        int fd,result;
 
	fd = COMM_GetWriteFd(handle);
	if(fd<0) {
	        TRACE("COMM_GetWriteFd failed\n");
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
 *	ClearCommBreak		(KERNEL32.20)
 */
BOOL WINAPI ClearCommBreak(HANDLE handle)
{
#if defined(TIOCSBRK) && defined(TIOCCBRK) /* check if available for compilation */
        int fd,result;
 
	fd = COMM_GetWriteFd(handle);
	if(fd<0) {
	        TRACE("COMM_GetWriteFd failed\n");
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
 *	EscapeCommFunction	(KERNEL32.214)
 */
BOOL WINAPI EscapeCommFunction(HANDLE handle,UINT nFunction)
{
	int fd,direct=FALSE,result=FALSE;
	struct termios	port;

    	TRACE("handle %d, function=%d\n", handle, nFunction);
	fd = COMM_GetWriteFd(handle);
	if(fd<0) {
		FIXME("handle %d not found.\n",handle);
		return FALSE;
	}

	if (tcgetattr(fd,&port) == -1) {
		commerror=WinError();
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
#ifdef TIOCM_DTR
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
			WARN("(handle=%d,nFunction=%d): Unknown function\n", 
			handle, nFunction);
			break;				
	}
	
	if (!direct)
	  if (tcsetattr(fd, TCSADRAIN, &port) == -1) {
		commerror = WinError();
		close(fd);
		return FALSE;	
	  } else 
	        result= TRUE;
	else
	  {
	    if (result == -1)
	      {
		result= FALSE;
		commerror=WinError();
	      }
	    else
	      result = TRUE;
	  }
	close(fd);
	return result;
}

/********************************************************************
 *      PurgeComm        (KERNEL32.557)
 */
BOOL WINAPI PurgeComm( HANDLE handle, DWORD flags) 
{
     int fd;

     TRACE("handle %d, flags %lx\n", handle, flags);

     fd = COMM_GetWriteFd(handle);
     if(fd<0) {
	FIXME("no handle %d found\n",handle);
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
 *	ClearCommError	(KERNEL32.21)
 */
BOOL WINAPI ClearCommError(INT handle,LPDWORD errors,LPCOMSTAT lpStat)
{
    int fd;

    fd=COMM_GetReadFd(handle);
    if(0>fd) 
    {
	FIXME("no handle %d found\n",handle);
        return FALSE;
    }

    if (lpStat) 
    {
	lpStat->status = 0;

#ifdef TIOCOUTQ
	if(ioctl(fd, TIOCOUTQ, &lpStat->cbOutQue))
	    WARN("ioctl returned error\n");
#else
	lpStat->cbOutQue = 0; /* FIXME: find a different way to find out */
#endif

	if(ioctl(fd, TIOCINQ, &lpStat->cbInQue))
	    WARN("ioctl returned error\n");

	TRACE("handle %d cbInQue = %ld cbOutQue = %ld\n",
	      handle, lpStat->cbInQue, lpStat->cbOutQue);
    }

    close(fd);

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

    FIXME("insize %ld outsize %ld unimplemented stub\n", insize, outsize);
    fd=COMM_GetWriteFd(handle);
    if(0>fd) {
	FIXME("handle %d not found?\n",handle);
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

    TRACE("handle %d, mask %p\n", handle, evtmask);
    if(0>(fd=COMM_GetReadFd(handle))) 
    {
	FIXME("no handle %d found\n",handle);
        return FALSE;
    }
    close(fd);
    *evtmask = eventmask;
    TRACE("%s%s%s%s%s%s%s%s%s\n",
	  (eventmask&EV_BREAK)?"EV_BREAK":"",
	  (eventmask&EV_CTS)?"EV_CTS":"",
	  (eventmask&EV_DSR)?"EV_DSR":"",
	  (eventmask&EV_ERR)?"EV_ERR":"",
	  (eventmask&EV_RING)?"EV_RING":"",
	  (eventmask&EV_RLSD)?"EV_RLSD":"",
	  (eventmask&EV_RXCHAR)?"EV_RXCHAR":"",
	  (eventmask&EV_RXFLAG)?"EV_RXFLAG":"",
	  (eventmask&EV_TXEMPTY)?"EV_TXEMPTY":"");
	  
    return TRUE;
}

/*****************************************************************************
 *	SetCommMask	(KERNEL32.451)
 */
BOOL WINAPI SetCommMask(INT handle,DWORD evtmask)
{
    int fd;

    TRACE("handle %d, mask %lx\n", handle, evtmask);
    TRACE("%s%s%s%s%s%s%s%s%s\n",
	  (evtmask&EV_BREAK)?"EV_BREAK":"",
	  (evtmask&EV_CTS)?"EV_CTS":"",
	  (evtmask&EV_DSR)?"EV_DSR":"",
	  (evtmask&EV_ERR)?"EV_ERR":"",
	  (evtmask&EV_RING)?"EV_RING":"",
	  (evtmask&EV_RLSD)?"EV_RLSD":"",
	  (evtmask&EV_RXCHAR)?"EV_RXCHAR":"",
	  (evtmask&EV_RXFLAG)?"EV_RXFLAG":"",
	  (evtmask&EV_TXEMPTY)?"EV_TXEMPTY":"");
	  
    if(0>(fd=COMM_GetWriteFd(handle))) {
	FIXME("no handle %d found\n",handle);
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

     TRACE("handle %d, ptr %p\n", handle, lpdcb);
     TRACE("bytesize %d baudrate %ld fParity %d Parity %d stopbits %d\n",
	   lpdcb->ByteSize,lpdcb->BaudRate,lpdcb->fParity, lpdcb->Parity,
	   (lpdcb->StopBits == ONESTOPBIT)?1:
	   (lpdcb->StopBits == TWOSTOPBITS)?2:0);
     TRACE("%s %s\n",(lpdcb->fInX)?"IXON":"~IXON",
	   (lpdcb->fOutX)?"IXOFF":"~IXOFF");

     if ((fd = COMM_GetWriteFd(handle)) < 0)  {
	FIXME("no handle %d found\n",handle);
	return FALSE;
     }

     if ((tcgetattr(fd,&port)) == -1) {
         int save_error = errno;
         commerror = WinError();
         close( fd );
#ifdef HAVE_STRERROR
         ERR("tcgetattr error '%s'\n", strerror(save_error));
#else
         ERR("tcgetattr error %d\n", save_error);
#endif
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
		case 460600:
			port.c_cflag |= B460800;
			break;		
#endif
       	        default:
			commerror = IE_BAUDRATE;
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
                default:
                        commerror = IE_BAUDRATE;
                        close( fd );
			ERR("baudrate %d \n",lpdcb->BaudRate);
                        return FALSE;
        }
        port.c_ispeed = port.c_ospeed;
#endif
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
                        close( fd );
			ERR("ByteSize\n");
			return FALSE;
	}

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
                        close( fd );
			ERR("Parity\n");
                        return FALSE;
        }
	

	switch (lpdcb->StopBits) {
		case ONESTOPBIT:
				port.c_cflag &= ~CSTOPB;
				break;
		case TWOSTOPBITS:
				port.c_cflag |= CSTOPB;
				break;
		default:
			commerror = IE_BYTESIZE;
                        close( fd );
			ERR("StopBits\n");
			return FALSE;
	}
#ifdef CRTSCTS
	if (	lpdcb->fOutxCtsFlow 			||
		lpdcb->fDtrControl == DTR_CONTROL_ENABLE||
		lpdcb->fRtsControl == RTS_CONTROL_ENABLE
	) 
	  {
	    port.c_cflag |= CRTSCTS;
	    TRACE("CRTSCTS\n");
	  }
	
	if (lpdcb->fDtrControl == DTR_CONTROL_DISABLE)
	  {
	    port.c_cflag &= ~CRTSCTS;
	    TRACE("~CRTSCTS\n");
	  }

#endif	
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
		commerror = WinError();	
                close( fd );
#ifdef HAVE_STRERROR
                ERR("tcgetattr error '%s'\n", strerror(save_error));
#else
                ERR("tcgetattr error %d\n", save_error);
#endif
		return FALSE;
	} else {
		commerror = 0;
                close( fd );
		return TRUE;
	}
}


/*****************************************************************************
 *	GetCommState	(KERNEL32.159)
 */
BOOL WINAPI GetCommState(INT handle, LPDCB lpdcb)
{
     struct termios port;
     int fd,speed;

     TRACE("handle %d, ptr %p\n", handle, lpdcb);

     if ((fd = COMM_GetReadFd(handle)) < 0) 
       {
	 ERR("can't get COMM_GetReadFd\n");
	 return FALSE;
       }
     if (tcgetattr(fd, &port) == -1) {
                int save_error=errno;
#ifdef HAVE_STRERROR
                ERR("tcgetattr error '%s'\n", strerror(save_error));
#else
                ERR("tcgetattr error %d\n", save_error);
#endif
		commerror = WinError();	
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

        TRACE("OK\n");
 
	TRACE("bytesize %d baudrate %ld fParity %d Parity %d stopbits %d\n",
	      lpdcb->ByteSize,lpdcb->BaudRate,lpdcb->fParity, lpdcb->Parity,
	      (lpdcb->StopBits == ONESTOPBIT)?1:
	      (lpdcb->StopBits == TWOSTOPBITS)?2:0);
	TRACE("%s %s\n",(lpdcb->fInX)?"IXON":"~IXON",
	      (lpdcb->fOutX)?"IXOFF":"~IXOFF");
#ifdef CRTSCTS
	if (	lpdcb->fOutxCtsFlow 			||
		lpdcb->fDtrControl == DTR_CONTROL_ENABLE||
		lpdcb->fRtsControl == RTS_CONTROL_ENABLE
		) 
	  TRACE("CRTSCTS\n");
	
	if (lpdcb->fDtrControl == DTR_CONTROL_DISABLE)
	  TRACE("~CRTSCTS\n");
	
#endif	
	return TRUE;
}

/*****************************************************************************
 *	TransmitCommChar	(KERNEL32.535)
 */
BOOL WINAPI TransmitCommChar(INT cid,CHAR chTransmit)
{
	struct DosDeviceStruct *ptr;

    	FIXME("(%d,'%c'), use win32 handle!\n",cid,chTransmit);
	if ((ptr = GetDeviceStruct(cid)) == NULL)
		FIXME("no handle for cid = %0x!.\n",cid);
		return FALSE;

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
BOOL WINAPI GetCommTimeouts(HANDLE hcom,LPCOMMTIMEOUTS lptimeouts)
{
	FIXME("(%x,%p):stub.\n",hcom,lptimeouts);
	return TRUE;
}

/*****************************************************************************
 *	SetCommTimeouts		(KERNEL32.453)
 */
BOOL WINAPI SetCommTimeouts(HANDLE hcom,LPCOMMTIMEOUTS lptimeouts) {
	/* struct DosDeviceStruct *ptr; */
	struct termios	tios;
	int	fd;

	FIXME("(%x,%p):stub.\n",hcom,lptimeouts);
	/* 
	if ((ptr = GetDeviceStruct(hcom)) == NULL) {
		FIXME("no handle for cid = %0x!.\n",hcom);
		return FALSE;
	}
	 */

	fd = COMM_GetWriteFd(hcom);
	if (fd < 0) {
		FIXME("no fd for cid = %0x!.\n",hcom);
		return FALSE;
	}


	FIXME("ReadIntervalTimeout %ld\n",lptimeouts->ReadIntervalTimeout);
	FIXME("ReadTotalTimeoutMultiplier %ld\n",lptimeouts->ReadTotalTimeoutMultiplier);
	FIXME("ReadTotalTimeoutConstant %ld\n",lptimeouts->ReadTotalTimeoutConstant);
	FIXME("WriteTotalTimeoutMultiplier %ld\n",lptimeouts->WriteTotalTimeoutMultiplier);
	FIXME("WriteTotalTimeoutConstant %ld\n",lptimeouts->WriteTotalTimeoutConstant);


	if (-1==tcgetattr(fd,&tios)) {
		FIXME("tcgetattr on fd %d failed!\n",fd);
		return FALSE;
	}
	/* VTIME is in 1/10 seconds */
	tios.c_cc[VTIME]= (lptimeouts->ReadIntervalTimeout+99)/100;
	if (-1==tcsetattr(fd,0,&tios)) {
		FIXME("tcsetattr on fd %d failed!\n",fd);
		return FALSE;
	}
	return TRUE;
}

/***********************************************************************
 *           GetCommModemStatus   (KERNEL32.285)
 */
BOOL WINAPI GetCommModemStatus(HANDLE hFile,LPDWORD lpModemStat )
{
	int fd,mstat, result=FALSE;
	
	*lpModemStat=0;
#ifdef TIOCMGET
	fd = COMM_GetWriteFd(hFile);
	if(fd<0)
		return FALSE;
	result = ioctl(fd, TIOCMGET, &mstat);
	close(fd);
	if (result == -1)
	  {
	    TRACE("ioctl failed\n");
	    return FALSE;
	  }
	if (mstat & TIOCM_CTS)
	    *lpModemStat |= MS_CTS_ON;
	if (mstat & TIOCM_DSR)
	  *lpModemStat |= MS_DSR_ON;
	if (mstat & TIOCM_RNG)
	  *lpModemStat |= MS_RING_ON;
	/*FIXME:  Not really sure about RLSD  UB 990810*/
	if (mstat & TIOCM_CAR)
	  *lpModemStat |= MS_RLSD_ON;
	TRACE("%s%s%s%s\n",
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
 *           WaitCommEvent   (KERNEL32.719)
 */
BOOL WINAPI WaitCommEvent(HANDLE hFile,LPDWORD eventmask ,LPOVERLAPPED overlapped)
{
	FIXME("(%d %p %p )\n",hFile, eventmask,overlapped);
	return TRUE;
}

/***********************************************************************
 *           GetCommProperties   (KERNEL32.???)
 */
BOOL WINAPI GetCommProperties(HANDLE hFile, LPDCB *dcb)
{
    FIXME("(%d %p )\n",hFile,dcb);
    return TRUE;
}

/***********************************************************************
 *           SetCommProperties   (KERNEL32.???)
 */
BOOL WINAPI SetCommProperties(HANDLE hFile, LPDCB dcb)
{
    FIXME("(%d %p )\n",hFile,dcb);
    return TRUE;
}

