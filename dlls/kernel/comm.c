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
#include <sys/poll.h>

#include "windef.h"
#ifdef HAVE_SYS_MODEM_H
# include <sys/modem.h>
#endif
#ifdef HAVE_SYS_STRTIO_H
# include <sys/strtio.h>
#endif
#include "heap.h"
#include "options.h"
#include "wine/port.h"
#include "server.h"
#include "winerror.h"
#include "services.h"
#include "callback.h"
#include "file.h"

#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(comm);

#if !defined(TIOCINQ) && defined(FIONREAD)
#define	TIOCINQ FIONREAD
#endif

/* window's semi documented modem status register */
#define COMM_MSR_OFFSET  35
#define MSR_CTS  0x10
#define MSR_DSR  0x20
#define MSR_RI   0x40
#define MSR_RLSD 0x80
#define MSR_MASK (MSR_CTS|MSR_DSR|MSR_RI|MSR_RLSD)

#define FLAG_LPT 0x80

#ifdef linux
#define CMSPAR 0x40000000 /* stick parity */
#endif

#define MAX_PORTS   9

struct DosDeviceStruct {
    char *devicename;   /* /dev/ttyS0 */
    int fd;
    int suspended;
    int unget,xmit;
    int baudrate;
    int evtchar;
    /* events */
    int commerror, eventmask;
    /* buffers */
    char *inbuf,*outbuf;
    unsigned ibuf_size,ibuf_head,ibuf_tail;
    unsigned obuf_size,obuf_head,obuf_tail;
    /* notifications */
    int wnd, n_read, n_write;
    HANDLE s_read, s_write;
};


static struct DosDeviceStruct COM[MAX_PORTS];
static struct DosDeviceStruct LPT[MAX_PORTS];
/* pointers to unknown(==undocumented) comm structure */ 
static LPCVOID *unknown[MAX_PORTS];
/* save terminal states */
static struct termios m_stat[MAX_PORTS];

/* update window's semi documented modem status register */
/* see knowledge base Q101417 */
static void COMM_MSRUpdate( UCHAR * pMsr, unsigned int mstat)
{
    UCHAR tmpmsr=0;
#ifdef TIOCM_CTS
    if(mstat & TIOCM_CTS) tmpmsr |= MSR_CTS;
#endif
#ifdef TIOCM_DSR
    if(mstat & TIOCM_DSR) tmpmsr |= MSR_DSR;
#endif
#ifdef TIOCM_RI
    if(mstat & TIOCM_RI)  tmpmsr |= MSR_RI;
#endif
#ifdef TIOCM_CAR
    if(mstat & TIOCM_CAR) tmpmsr |= MSR_RLSD;
#endif
    *pMsr = (*pMsr & ~MSR_MASK) | tmpmsr;
}

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
    if (Callout.PostMessageA) Callout.PostMessageA(ptr->wnd, WM_COMMNOTIFY, cid, mask);
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
 *         BuildCommDCB16		(USER.213)
 *
 * According to the ECMA-234 (368.3) the function will return FALSE on 
 * success, otherwise it will return -1. 
 * IF THIS IS NOT CORRECT THE RETURNVALUE CHECK IN BuildCommDCBAndTimeoutsA
 * NEEDS TO BE FIXED
 */
INT16 WINAPI BuildCommDCB16(LPCSTR device, LPDCB16 lpdcb)
{
	/* "COM1:96,n,8,1"	*/
	/*  012345		*/
	int port;
	char *ptr, temp[256];

	TRACE("(%s), ptr %p\n", device, lpdcb);

	if (!strncasecmp(device,"COM",3)) {
		port = device[3] - '0';
	

		if (port-- == 0) {
			ERR("BUG ! COM0 can't exist!\n");
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
		{
			int rate;
		        /* DOS/Windows only compares the first two numbers
			 * and assigns an appropriate baud rate.
			 * You can supply 961324245, it still returns 9600 ! */
			if (strlen(ptr) < 2)
			{
			    WARN("Unknown baudrate string '%s' !\n", ptr);
			    return -1; /* error: less than 2 chars */
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
					return -1;
			}
			
		        lpdcb->BaudRate = rate;
		}
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
 *	OpenComm16		(USER.200)
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

	if (!strncasecmp(device,"COM",3)) {
		
                TRACE("%s = %s\n", device, COM[port].devicename);

		if (!ValidCOMPort(port))
			return IE_BADID;

		if (COM[port].fd)
			return IE_OPEN;

		fd = open(COM[port].devicename, O_RDWR | O_NONBLOCK);
		if (fd == -1) {
			ERR("Couldn't open %s ! (%s)\n", COM[port].devicename, strerror(errno));
			return IE_HARDWARE;
		} else {
                        unknown[port] = SEGPTR_ALLOC(40);
			memset(unknown[port], 0, 40);
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
			COM[port].ibuf_head = COM[port].ibuf_tail = 0;
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
			  ERR("out of memory\n");
			  return IE_MEMORY;
			}

                        COM[port].s_read = INVALID_HANDLE_VALUE;
                        COM[port].s_write = INVALID_HANDLE_VALUE;
                        comm_waitread( &COM[port] );
			return port;
		}
	} 
	else 
	if (!strncasecmp(device,"LPT",3)) {
	
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
	return IE_BADID;
}

/*****************************************************************************
 *	CloseComm16		(USER.207)
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
 *	SetCommBreak16		(USER.210)
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
 *	ClearCommBreak16	(USER.211)
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
 *	EscapeCommFunction16	(USER.214)
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
 *	FlushComm16	(USER.215)
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
 *	GetCommError16	(USER.203)
 */
INT16 WINAPI GetCommError16(INT16 cid,LPCOMSTAT16 lpStat)
{
	int		temperror;
	struct DosDeviceStruct *ptr;
        unsigned char *stol;
        unsigned int mstat;

	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
		return -1;
	}
        if (cid&FLAG_LPT) {
            WARN(" cid %d not comm port\n",cid);
            return CE_MODE;
        }
        stol = (unsigned char *)unknown[cid] + COMM_MSR_OFFSET;
        ioctl(ptr->fd,TIOCMGET,&mstat);
        COMM_MSRUpdate( stol, mstat);

	if (lpStat) {
		lpStat->status = 0;

		lpStat->cbOutQue = comm_outbuf(ptr);
		lpStat->cbInQue = comm_inbuf(ptr);

    		TRACE("cid %d, error %d, stat %d in %d out %d, stol %x\n",
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
 *	SetCommEventMask16	(USER.208)
 */
SEGPTR WINAPI SetCommEventMask16(INT16 cid,UINT16 fuEvtMask)
{
	struct DosDeviceStruct *ptr;
        unsigned char *stol;
        int repid;
        unsigned int mstat;

    	TRACE("cid %d,mask %d\n",cid,fuEvtMask);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
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
        COMM_MSRUpdate( stol, mstat);

	TRACE(" modem dcd construct %x\n",*stol);
	return SEGPTR_GET(unknown[cid]);
}

/*****************************************************************************
 *	GetCommEventMask16	(USER.209)
 */
UINT16 WINAPI GetCommEventMask16(INT16 cid,UINT16 fnEvtClear)
{
	struct DosDeviceStruct *ptr;
	WORD events;

    	TRACE("cid %d, mask %d\n", cid, fnEvtClear);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
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
        int bytesize, stopbits;
        int fail=0;

    	TRACE("cid %d, ptr %p\n", lpdcb->Id, lpdcb);
	if ((ptr = GetDeviceStruct(lpdcb->Id)) == NULL) {
		FIXME("no handle for cid = %0x!\n",lpdcb->Id);
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
			fail=1;
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
                        fail=1;
        }
        port.c_ispeed = port.c_ospeed;
#endif
        bytesize=lpdcb->ByteSize;
        stopbits=lpdcb->StopBits;

    	TRACE("fParity %d Parity %d\n",lpdcb->fParity, lpdcb->Parity);
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
                            ptr->commerror = IE_BYTESIZE;
                            fail=1;
                        }
                        break;
                case SPACEPARITY:
                        if( bytesize < 8) {
                            bytesize +=1;
                            port.c_iflag &= ~INPCK;
                        } else {
                            ptr->commerror = IE_BYTESIZE;
                            fail=1;
                        }
                        break;
#endif
                default:
                        ptr->commerror = IE_BYTESIZE;
                        fail=1;
        }
	
    	TRACE("bytesize %d\n",bytesize);
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
			ptr->commerror = IE_BYTESIZE;
			fail=1;
	}

    	TRACE("stopbits %d\n",stopbits);

	switch (stopbits) {
		case ONESTOPBIT:
				port.c_cflag &= ~CSTOPB;
				break;
		case ONE5STOPBITS: /* wil be selected if bytesize is 5 */
		case TWOSTOPBITS:
				port.c_cflag |= CSTOPB;
				break;
		default:
			ptr->commerror = IE_BYTESIZE;
			fail=1;
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

        if(fail)
            return -1;
        
	if (tcsetattr(ptr->fd, TCSADRAIN, &port) == -1) {
		ptr->commerror = WinError();	
		return -1;
	} else {
		ptr->commerror = 0;
		return 0;
	}
}

/*****************************************************************************
 *	GetCommState16	(USER.202)
 */
INT16 WINAPI GetCommState16(INT16 cid, LPDCB16 lpdcb)
{
	int speed;
	struct DosDeviceStruct *ptr;
	struct termios port;

    	TRACE("cid %d, ptr %p\n", cid, lpdcb);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
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
 *	TransmitCommChar16	(USER.206)
 */
INT16 WINAPI TransmitCommChar16(INT16 cid,CHAR chTransmit)
{
	struct DosDeviceStruct *ptr;

    	TRACE("cid %d, data %d \n", cid, chTransmit);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
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
 *	UngetCommChar16	(USER.212)
 */
INT16 WINAPI UngetCommChar16(INT16 cid,CHAR chUnget)
{
	struct DosDeviceStruct *ptr;

    	TRACE("cid %d (char %d)\n", cid, chUnget);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
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
 *	ReadComm16	(USER.204)
 */
INT16 WINAPI ReadComm16(INT16 cid,LPSTR lpvBuf,INT16 cbRead)
{
	int status, length;
	struct DosDeviceStruct *ptr;
	LPSTR orgBuf = lpvBuf;

    	TRACE("cid %d, ptr %p, length %d\n", cid, lpvBuf, cbRead);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
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
 *	WriteComm16	(USER.205)
 */
INT16 WINAPI WriteComm16(INT16 cid, LPSTR lpvBuf, INT16 cbWrite)
{
	int status, length;
	struct DosDeviceStruct *ptr;

    	TRACE("cid %d, ptr %p, length %d\n", 
		cid, lpvBuf, cbWrite);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
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
 *           EnableCommNotification   (USER.245)
 */
BOOL16 WINAPI EnableCommNotification16( INT16 cid, HWND16 hwnd,
                                      INT16 cbWriteNotify, INT16 cbOutQueue )
{
	struct DosDeviceStruct *ptr;

	TRACE("(%d, %x, %d, %d)\n", cid, hwnd, cbWriteNotify, cbOutQueue);
	if ((ptr = GetDeviceStruct(cid)) == NULL) {
		FIXME("no handle for cid = %0x!\n",cid);
		return -1;
	}
	ptr->wnd = hwnd;
	ptr->n_read = cbWriteNotify;
	ptr->n_write = cbOutQueue;
	return TRUE;
}


/**************************************************************************
 *         BuildCommDCBA		(KERNEL32.113)
 *
 *  Updates a device control block data structure with values from an
 *  ascii device control string.  The device control string has two forms
 *  normal and extended, it must be exclusively in one or the other form.
 *
 * RETURNS
 *
 *  True on success, false on an malformed control string.
 */
BOOL WINAPI BuildCommDCBA(
    LPCSTR device, /* [in] The ascii device control string used to update the DCB. */
    LPDCB  lpdcb)  /* [out] The device control block to be updated. */
{
	return BuildCommDCBAndTimeoutsA(device,lpdcb,NULL);
}

/**************************************************************************
 *         BuildCommDCBAndTimeoutsA	(KERNEL32.114)
 *
 *  Updates a device control block data structure with values from an
 *  ascii device control string.  Taking time out values from a time outs
 *  struct if desired by the control string.
 *
 * RETURNS
 *
 *  True on success, false bad handles etc
 */
BOOL WINAPI BuildCommDCBAndTimeoutsA(
    LPCSTR         device,     /* [in] The ascii device control string. */
    LPDCB          lpdcb,      /* [out] The device control block to be updated. */
    LPCOMMTIMEOUTS lptimeouts) /* [in] The time outs to use if asked to set them by the control string. */
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
		if (!ValidCOMPort(port))
			return FALSE;
		if (*(device+4)!=':')
			return FALSE;
		temp=(LPSTR)(device+5);
	} else
		temp=(LPSTR)device;

	lpdcb->DCBlength	= sizeof(DCB);
	if (strchr(temp,',')) {	/* old style */
		DCB16	dcb16;
		BOOL16	ret;
		char	last=temp[strlen(temp)-1];

		ret=BuildCommDCB16(device,&dcb16);
		if (ret)
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
 *         BuildCommDCBAndTimeoutsW		(KERNEL32.115)
 *
 *  Updates a device control block data structure with values from an
 *  unicode device control string.  Taking time out values from a time outs
 *  struct if desired by the control string.
 *
 * RETURNS
 *
 *  True on success, false bad handles etc.
 */
BOOL WINAPI BuildCommDCBAndTimeoutsW(
    LPCWSTR        devid,      /* [in] The unicode device control string. */
    LPDCB          lpdcb,      /* [out] The device control block to be updated. */
    LPCOMMTIMEOUTS lptimeouts) /* [in] The time outs to use if asked to set them by the control string. */
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
 *         BuildCommDCBW		(KERNEL32.116)
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

/* FIXME: having these global for win32 for now */
int commerror=0;

/*****************************************************************************
 *	SetCommBreak		(KERNEL32.616)
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
 *	ClearCommBreak		(KERNEL32.135)
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
 *	EscapeCommFunction	(KERNEL32.213)
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

    	TRACE("handle %d, function=%d\n", handle, nFunction);
	fd = FILE_GetUnixHandle( handle, GENERIC_READ );
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
 *      PurgeComm        (KERNEL32.558)
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

     TRACE("handle %d, flags %lx\n", handle, flags);

     fd = FILE_GetUnixHandle( handle, GENERIC_READ );
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
 *	ClearCommError	(KERNEL32.136)
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

#ifdef TIOCINQ
	if(ioctl(fd, TIOCINQ, &lpStat->cbInQue))
	    WARN("ioctl returned error\n");
#endif

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
 *      SetupComm       (KERNEL32.677)
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
	FIXME("handle %d not found?\n",handle);
        return FALSE;
    }
    close(fd);
    return TRUE;
} 

/*****************************************************************************
 *	GetCommMask	(KERNEL32.284)
 *
 *  Obtain the events associated with a communication device that will cause a call
 *  WaitCommEvent to return.
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

    TRACE("handle %d, mask %p\n", handle, evtmask);

    SERVER_START_REQ( get_serial_info )
    {
        req->handle = handle;
        if ((ret = !SERVER_CALL_ERR()))
        {
            if (evtmask) *evtmask = req->eventmask;
        }
    }
    SERVER_END_REQ;
    return ret;
}

/*****************************************************************************
 *	SetCommMask	(KERNEL32.618)
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
    DWORD  evtmask) /* [in] The events that to be monitored. */
{
    BOOL ret;

    TRACE("handle %d, mask %lx\n", handle, evtmask);

    SERVER_START_REQ( set_serial_info )
    {
        req->handle    = handle;
        req->flags     = SERIALINFO_SET_MASK;
        req->eventmask = evtmask;
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    return ret;
}

/*****************************************************************************
 *	SetCommState    (KERNEL32.619)
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

     TRACE("handle %d, ptr %p\n", handle, lpdcb);
     TRACE("bytesize %d baudrate %ld fParity %d Parity %d stopbits %d\n",
	   lpdcb->ByteSize,lpdcb->BaudRate,lpdcb->fParity, lpdcb->Parity,
	   (lpdcb->StopBits == ONESTOPBIT)?1:
	   (lpdcb->StopBits == TWOSTOPBITS)?2:0);
     TRACE("%s %s\n",(lpdcb->fInX)?"IXON":"~IXON",
	   (lpdcb->fOutX)?"IXOFF":"~IXOFF");

     fd = FILE_GetUnixHandle( handle, GENERIC_READ );
     if (fd < 0)  {
	FIXME("no handle %d found\n",handle);
	return FALSE;
     }

     if ((tcgetattr(fd,&port)) == -1) {
         int save_error = errno;
         commerror = WinError();
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
                            commerror = IE_BYTESIZE;
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
                            commerror = IE_BYTESIZE;
                            close( fd );
                            ERR("Cannot set SPACE Parity\n");
                            return FALSE;
                        }
                        break;
#endif
               default:
                        commerror = IE_BYTESIZE;
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
			commerror = IE_BYTESIZE;
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
			commerror = IE_BYTESIZE;
                        close( fd );
			ERR("StopBits\n");
			return FALSE;
	}
#ifdef CRTSCTS
	if (	lpdcb->fOutxCtsFlow 			||
		lpdcb->fRtsControl == RTS_CONTROL_ENABLE
	) 
	  {
	    port.c_cflag |= CRTSCTS;
	    TRACE("CRTSCTS\n");
	  }
	
	if (lpdcb->fDtrControl == DTR_CONTROL_ENABLE)
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
                ERR("tcsetattr error '%s'\n", strerror(save_error));
		return FALSE;
	} else {
		commerror = 0;
                close( fd );
		return TRUE;
	}
}


/*****************************************************************************
 *	GetCommState	(KERNEL32.287)
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

     TRACE("handle %d, ptr %p\n", handle, lpdcb);

     fd = FILE_GetUnixHandle( handle, GENERIC_READ );
     if (fd < 0) 
       {
	 ERR("FILE_GetUnixHandle failed\n");
	 return FALSE;
       }
     if (tcgetattr(fd, &port) == -1) {
                int save_error=errno;
                ERR("tcgetattr error '%s'\n", strerror(save_error));
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
 *	TransmitCommChar	(KERNEL32.697)
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
    	FIXME("(%x,'%c'), use win32 handle!\n",hComm,chTransmit);
	return TRUE;
}

/*****************************************************************************
 *	GetCommTimeouts		(KERNEL32.288)
 *
 *  Obtains the request time out values for the communications device.
 *
 * RETURNS
 *
 *  True on success, false if communications device handle is bad
 *  or the target structure is null.
 */
BOOL WINAPI GetCommTimeouts(
    HANDLE         hComm,      /* [in] The communications device. */
    LPCOMMTIMEOUTS lptimeouts) /* [out] The struct of request time outs. */
{
    BOOL ret;

    TRACE("(%x,%p)\n",hComm,lptimeouts);

    if(!lptimeouts)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SERVER_START_REQ( get_serial_info )
    {
        req->handle = hComm;
        if ((ret = !SERVER_CALL_ERR()))
        {
            lptimeouts->ReadIntervalTimeout         = req->readinterval;
            lptimeouts->ReadTotalTimeoutMultiplier  = req->readmult;
            lptimeouts->ReadTotalTimeoutConstant    = req->readconst;
            lptimeouts->WriteTotalTimeoutMultiplier = req->writemult;
            lptimeouts->WriteTotalTimeoutConstant   = req->writeconst;
        }
    }
    SERVER_END_REQ;
    return ret;
}

/*****************************************************************************
 *	SetCommTimeouts		(KERNEL32.620)
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
 *  True if the time outs were set, false otherwise.
 */
BOOL WINAPI SetCommTimeouts(
    HANDLE hComm,              /* [in] handle of COMM device */
    LPCOMMTIMEOUTS lptimeouts) /* [in] pointer to COMMTIMEOUTS structure */
{
    BOOL ret;
    int fd;
    struct termios tios;

    TRACE("(%x,%p)\n",hComm,lptimeouts);

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
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    if (!ret) return FALSE;

    /* FIXME: move this stuff to the server */
    fd = FILE_GetUnixHandle( hComm, GENERIC_READ );
    if (fd < 0) {
       FIXME("no fd for handle = %0x!.\n",hComm);
       return FALSE;
    }

    if (-1==tcgetattr(fd,&tios)) {
        FIXME("tcgetattr on fd %d failed!\n",fd);
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
        return FALSE;
    }
    close(fd);
    return TRUE;
}

/***********************************************************************
 *           GetCommModemStatus   (KERNEL32.285)
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
static void COMM_WaitCommEventService(async_private *ovp, int events)
{
    LPOVERLAPPED lpOverlapped = ovp->lpOverlapped;

    TRACE("overlapped %p wait complete %p <- %x\n",lpOverlapped,ovp->buffer,events);
    if(events&POLLNVAL)
    {
        lpOverlapped->Internal = STATUS_HANDLES_CLOSED;
        return;
    }
    if(ovp->buffer)
    {
        if(events&POLLIN)
            *ovp->buffer = EV_RXCHAR;
    }
 
    lpOverlapped->Internal = STATUS_SUCCESS;
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
    int fd,ret;
    async_private *ovp;

    if(!lpOverlapped)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(NtResetEvent(lpOverlapped->hEvent,NULL))
        return FALSE;

    lpOverlapped->Internal = STATUS_PENDING;
    lpOverlapped->InternalHigh = 0;
    lpOverlapped->Offset = 0;
    lpOverlapped->OffsetHigh = 0;

    /* start an ASYNCHRONOUS WaitCommEvent */
    SERVER_START_REQ( create_async )
    {
        req->file_handle = hFile;
        req->count = 0;
        req->type = ASYNC_TYPE_WAIT;

        ret=SERVER_CALL_ERR();
    }
    SERVER_END_REQ;

    if (ret)
        return FALSE;
  
    fd = FILE_GetUnixHandle( hFile, GENERIC_WRITE );
    if(fd<0)
	return FALSE;

    ovp = (async_private *) HeapAlloc(GetProcessHeap(), 0, sizeof (async_private));
    if(!ovp)
    {
        close(fd);
        return FALSE;
    }
    ovp->lpOverlapped = lpOverlapped;
    ovp->timeout = 0;
    ovp->tv.tv_sec = 0;
    ovp->tv.tv_usec = 0;
    ovp->event = POLLIN;
    ovp->func = COMM_WaitCommEventService;
    ovp->buffer = (char *)lpdwEvents;
    ovp->fd = fd;
  
    ovp->next = NtCurrentTeb()->pending_list;
    ovp->prev = NULL;
    if(ovp->next)
        ovp->next->prev=ovp;
    NtCurrentTeb()->pending_list = ovp;
  
    SetLastError(ERROR_IO_PENDING);

    return FALSE;
}

/***********************************************************************
 *           WaitCommEvent   (KERNEL32.719)
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

    TRACE("(%x %p %p )\n",hFile, lpdwEvents,lpOverlapped);

    if(lpOverlapped)
        return COMM_WaitCommEvent(hFile, lpdwEvents, lpOverlapped);

    /* if there is no overlapped structure, create our own */
    ov.hEvent = CreateEventA(NULL,FALSE,FALSE,NULL);

    COMM_WaitCommEvent(hFile, lpdwEvents, &ov);

    if(GetLastError()!=STATUS_PENDING)
    {
        CloseHandle(ov.hEvent);
        return FALSE;
    }

    /* wait for the overlapped to complete */
    ret = GetOverlappedResult(hFile, &ov, NULL, TRUE);
    CloseHandle(ov.hEvent);

    return ret;
}
  
/***********************************************************************
 *           GetCommProperties   (KERNEL32.286)
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
    FIXME("(%d %p )\n",hFile,lpCommProp);
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
    lpCommProp->dwProvCapabilities  = PCF_DTRDSR | PCF_PARITY_CHECK | PCF_RTSCTS ;
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
 *           CommConfigDialogA   (KERNEL32.140)
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

    TRACE("(%p %x %p)\n",lpszDevice, hWnd, lpCommConfig);

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
 *           CommConfigDialogW   (KERNEL32.141)
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
 *           GetCommConfig     (KERNEL32.283)
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
 *  The signature is missing a the parameter for the size of the COMMCONFIG
 *  structure/buffer it should be
 *  BOOL WINAPI GetCommConfig(HANDLE hFile,LPCOMMCONFIG lpCommConfig,LPDWORD lpdwSize)
 */
BOOL WINAPI GetCommConfig(
    HANDLE       hFile,        /* [in] The communications device. */
    LPCOMMCONFIG lpCommConfig) /* [out] The communications configuration of the device (if it fits). */
#if 0 /* FIXME: Why is this "commented" out? */
    LPDWORD      lpdwSize)     /* [in/out] Initially the size of the configuration buffer/structure,
                                  afterwards the number of bytes copied to the buffer or 
                                  the needed size of the buffer. */
#endif
{
    BOOL r;

    TRACE("(%x %p)\n",hFile,lpCommConfig);

    if(lpCommConfig == NULL)
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
 *           SetCommConfig     (KERNEL32.617)
 *
 *  Sets the configuration of the commications device.
 *
 * RETURNS
 *
 *  True on success, false if the handle was bad is not a communications device.
 */
BOOL WINAPI SetCommConfig(
    HANDLE       hFile,        /* [in] The communications device. */
    LPCOMMCONFIG lpCommConfig) /* [in] The desired configuration. */
{
    TRACE("(%x %p)\n",hFile,lpCommConfig);
    return SetCommState(hFile,&lpCommConfig->dcb);
}

/***********************************************************************
 *           SetDefaultCommConfigA   (KERNEL32.638)
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
 *           SetDefaultCommConfigW     (KERNEL32.639)
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
 *           GetDefaultCommConfigA   (KERNEL32.313)
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

     if (!ValidCOMPort(lpszName[3]-'1'))
        return FALSE;

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

     (void) sprintf( temp, "COM%c:38400,n,8,1", lpszName[3]);
     FIXME("setting %s as default\n", temp);

     return BuildCommDCBA( temp, lpdcb);
}

/**************************************************************************
 *         GetDefaultCommConfigW		(KERNEL32.314)
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

