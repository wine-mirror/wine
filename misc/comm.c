/*
 * DEC 93 Erik Bos <erik@xs4all.nl>
 *
 * Copyright 1996 Marcus Meissner
 * FIXME: use HFILEs instead of unixfds
 *	  the win32 functions here get HFILEs already.
 */

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/ioctl.h>
#endif
#include <unistd.h>

#include "windows.h"
#include "comm.h"
#include "options.h"
#include "stddebug.h"
#include "debug.h"
#include "handle32.h"
#include "string32.h"

int commerror = 0, eventmask = 0;

struct DosDeviceStruct COM[MAX_PORTS];
struct DosDeviceStruct LPT[MAX_PORTS];

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
				fprintf(stderr,"comm: can't use `%s' as %s !\n", temp, option);
			else
				if ((COM[x].devicename = malloc(strlen(temp)+1)) == NULL) 
					fprintf(stderr,"comm: can't malloc for device info!\n");
				else {
					COM[x].fd = 0;
					strcpy(COM[x].devicename, temp);
				}
                dprintf_comm(stddeb,
                        "Comm_Init: %s = %s\n", option, COM[x].devicename);
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
				fprintf(stderr,"comm: can't use `%s' as %s !\n", temp, option);
			else 
				if ((LPT[x].devicename = malloc(strlen(temp)+1)) == NULL) 
					fprintf(stderr,"comm: can't malloc for device info!\n");
				else {
					LPT[x].fd = 0;
					strcpy(LPT[x].devicename, temp);
				}
                dprintf_comm(stddeb,
                        "Comm_Init: %s = %s\n", option, LPT[x].devicename);
		}

	}
}


struct DosDeviceStruct *GetDeviceStruct(int fd)
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
        dprintf_comm(stddeb, "WinError: errno = %d\n", errno);
	switch (errno) {
		default:
			return CE_IOE;
		}
}

/**************************************************************************
 *         BuildCommDCB		(USER.213)
 */
BOOL16 BuildCommDCB16(LPCSTR device, LPDCB16 lpdcb)
{
	/* "COM1:9600,n,8,1"	*/
	/*  012345		*/
	int port;
	char *ptr, temp[256];

	dprintf_comm(stddeb,
		"BuildCommDCB: (%s), ptr %p\n", device, lpdcb);
	commerror = 0;

	if (!lstrncmpi32A(device,"COM",3)) {
		port = device[3] - '0';
	

		if (port-- == 0) {
			fprintf(stderr, "comm: BUG ! COM0 can't exists!.\n");
			commerror = IE_BADID;
		}

		if (!ValidCOMPort(port)) {
			commerror = IE_BADID;
			return -1;
		}
		
		if (!COM[port].fd) {
		    OpenComm(device, 0, 0);
		}
		lpdcb->Id = COM[port].fd;
		
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
        	dprintf_comm(stddeb,"BuildCommDCB: baudrate (%d)\n", lpdcb->BaudRate);

		ptr = strtok(NULL, ", ");
		if (islower(*ptr))
			*ptr = toupper(*ptr);

        	dprintf_comm(stddeb,"BuildCommDCB: parity (%c)\n", *ptr);
		lpdcb->fParity = 1;
		switch (*ptr) {
			case 'N':
				lpdcb->Parity = NOPARITY;
				lpdcb->fParity = 0;
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
				fprintf(stderr,"comm: unknown parity `%c'!\n", *ptr);
				return -1;
		}

		ptr = strtok(NULL, ", "); 
         	dprintf_comm(stddeb, "BuildCommDCB: charsize (%c)\n", *ptr);
		lpdcb->ByteSize = *ptr - '0';

		ptr = strtok(NULL, ", ");
        	dprintf_comm(stddeb, "BuildCommDCB: stopbits (%c)\n", *ptr);
		switch (*ptr) {
			case '1':
				lpdcb->StopBits = ONESTOPBIT;
				break;			
			case '2':
				lpdcb->StopBits = TWOSTOPBITS;
				break;			
			default:
				fprintf(stderr,"comm: unknown # of stopbits `%c'!\n", *ptr);
				return -1;
		}
	}	

	return 0;
}

/**************************************************************************
 *         BuildCommDCBA		(KERNEL32.14)
 */
BOOL32 BuildCommDCB32A(LPCSTR device,LPDCB32 lpdcb) {
	return BuildCommDCBAndTimeouts32A(device,lpdcb,NULL);
}

/**************************************************************************
 *         BuildCommDCBAndTimeoutsA	(KERNEL32.15)
 */
BOOL32 BuildCommDCBAndTimeouts32A(LPCSTR device, LPDCB32 lpdcb,LPCOMMTIMEOUTS lptimeouts) {
	int	port;
	char	*ptr,*temp;

	dprintf_comm(stddeb,"BuildCommDCBAndTimeouts32A(%s,%p,%p)\n",device,lpdcb,lptimeouts);
	commerror = 0;

	if (!lstrncmpi32A(device,"COM",3)) {
		port=device[3]-'0';
		if (port--==0) {
			fprintf(stderr,"comm:BUG! COM0 can't exists!.\n");
			return FALSE;
		}
		if (!ValidCOMPort(port))
			return FALSE;
		if (*(device+4)!=':')
			return FALSE;
		temp=(LPSTR)(device+5);
	} else
		temp=(LPSTR)device;
	lpdcb->DCBlength	= sizeof(DCB32);
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
				fprintf(stderr,"BuildCommDCB32A:Couldn't parse %s\n",ptr);
			lpdcb->BaudRate = x;
			flag=1;
		}
		if (!strncmp("stop=",ptr,5)) {
			if (!sscanf(ptr+5,"%ld",&x))
				fprintf(stderr,"BuildCommDCB32A:Couldn't parse %s\n",ptr);
			lpdcb->StopBits = x;
			flag=1;
		}
		if (!strncmp("data=",ptr,5)) {
			if (!sscanf(ptr+5,"%ld",&x))
				fprintf(stderr,"BuildCommDCB32A:Couldn't parse %s\n",ptr);
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
			fprintf(stderr,"BuildCommDCB32A: Unhandled specifier '%s', please report.\n",ptr);
		ptr=strtok(NULL," ");
	}
	if (lpdcb->BaudRate==110)
		lpdcb->StopBits = 2;
	return TRUE;
}

/**************************************************************************
 *         BuildCommDCBAndTimeoutsW		(KERNEL32.16)
 */
BOOL32 BuildCommDCBAndTimeouts32W(
	LPCWSTR devid,LPDCB32 lpdcb,LPCOMMTIMEOUTS lptimeouts
) {
	LPSTR	devidA;
	BOOL32	ret;

	dprintf_comm(stddeb,"BuildCommDCBAndTimeouts32W(%p,%p,%p)\n",devid,lpdcb,lptimeouts);
	devidA = STRING32_DupUniToAnsi(devid);
	ret=BuildCommDCBAndTimeouts32A(devidA,lpdcb,lptimeouts);
	free(devidA);
	return ret;
}

/**************************************************************************
 *         BuildCommDCBW		(KERNEL32.17)
 */
BOOL32 BuildCommDCB32W(LPCWSTR devid,LPDCB32 lpdcb) {
	return BuildCommDCBAndTimeouts32W(devid,lpdcb,NULL);
}

/*****************************************************************************
 *	OpenComm		(USER.200)
 */
INT16 OpenComm(LPCSTR device,UINT16 cbInQueue,UINT16 cbOutQueue)
{
	int port,fd;

    	dprintf_comm(stddeb,
		"OpenComm: %s, %d, %d\n", device, cbInQueue, cbOutQueue);
	commerror = 0;

	if (!lstrncmpi32A(device,"COM",3)) {
		port = device[3] - '0';

		if (port-- == 0) {
			fprintf(stderr, "comm: BUG ! COM0 doesn't exists!.\n");
			commerror = IE_BADID;
		}

                dprintf_comm(stddeb,
                       "OpenComm: %s = %s\n", device, COM[port].devicename);

		if (!ValidCOMPort(port)) {
			commerror = IE_BADID;
			return -1;
		}
		if (COM[port].fd) {
			return COM[port].fd;
		}

		fd = open(COM[port].devicename, O_RDWR | O_NONBLOCK);
		if (fd == -1) {
			commerror = WinError();
			return -1;
		} else {
			COM[port].fd = fd;	
			return fd;
		}
	} 
	else 
	if (!lstrncmpi32A(device,"LPT",3)) {
		port = device[3] - '0';
	
		if (!ValidLPTPort(port)) {
			commerror = IE_BADID;
			return -1;
		}		
		if (LPT[port].fd) {
			commerror = IE_OPEN;
			return -1;
		}

		fd = open(LPT[port].devicename, O_RDWR | O_NONBLOCK, 0);
		if (fd == -1) {
			commerror = WinError();
			return -1;	
		} else {
			LPT[port].fd = fd;
			return fd;
		}
	}
	return 0;
}

/*****************************************************************************
 *	CloseComm		(USER.207)
 */
INT16 CloseComm(INT16 fd)
{
    	dprintf_comm(stddeb,"CloseComm: fd %d\n", fd);
	if (close(fd) == -1) {
		commerror = WinError();
		return -1;
	} else {
		commerror = 0;
		return 0;
	}
}

/*****************************************************************************
 *	SetCommBreak		(USER.210)
 */
INT16 SetCommBreak16(INT16 fd)
{
	struct DosDeviceStruct *ptr;

	dprintf_comm(stddeb,"SetCommBreak: fd: %d\n", fd);
	if ((ptr = GetDeviceStruct(fd)) == NULL) {
		commerror = IE_BADID;
		return -1;
	}

	ptr->suspended = 1;
	commerror = 0;
	return 0;
}

/*****************************************************************************
 *	SetCommBreak		(KERNEL32.449)
 */
BOOL32 SetCommBreak32(INT32 fd)
{

	struct DosDeviceStruct *ptr;

	dprintf_comm(stddeb,"SetCommBreak: fd: %d\n", fd);
	if ((ptr = GetDeviceStruct(fd)) == NULL) {
		commerror = IE_BADID;
		return FALSE;
	}

	ptr->suspended = 1;
	commerror = 0;
	return TRUE;
}

/*****************************************************************************
 *	ClearCommBreak		(USER.211)
 */
INT16 ClearCommBreak16(INT16 fd)
{
	struct DosDeviceStruct *ptr;

    	dprintf_comm(stddeb,"ClearCommBreak: fd: %d\n", fd);
	if ((ptr = GetDeviceStruct(fd)) == NULL) {
		commerror = IE_BADID;
		return -1;
	}

	ptr->suspended = 0;
	commerror = 0;
	return 0;
}

/*****************************************************************************
 *	ClearCommBreak		(KERNEL32.20)
 */
BOOL32 ClearCommBreak32(INT32 fd)
{
	struct DosDeviceStruct *ptr;

    	dprintf_comm(stddeb,"ClearCommBreak: fd: %d\n", fd);
	if ((ptr = GetDeviceStruct(fd)) == NULL) {
		commerror = IE_BADID;
		return FALSE;
	}

	ptr->suspended = 0;
	commerror = 0;
	return TRUE;
}

/*****************************************************************************
 *	EscapeCommFunction	(USER.214)
 */
LONG EscapeCommFunction16(UINT16 fd,UINT16 nFunction)
{
	int	max;
	struct termios port;

    	dprintf_comm(stddeb,"EscapeCommFunction fd: %d, function: %d\n", fd, nFunction);
	if (tcgetattr(fd,&port) == -1) {
		commerror=WinError();	
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
			fprintf(stderr,
			"EscapeCommFunction fd: %d, unknown function: %d\n", 
			fd, nFunction);
			break;				
	}
	
	if (tcsetattr(fd, TCSADRAIN, &port) == -1) {
		commerror = WinError();
		return -1;	
	} else {
		commerror = 0;
		return 0;
	}
}

/*****************************************************************************
 *	EscapeCommFunction	(KERNEL32.214)
 */
BOOL32 EscapeCommFunction32(INT32 fd,UINT32 nFunction)
{
	struct termios	port;
	struct DosDeviceStruct *ptr;

    	dprintf_comm(stddeb,"EscapeCommFunction fd: %d, function: %d\n", fd, nFunction);
	if (tcgetattr(fd,&port) == -1) {
		commerror=WinError();	
		return FALSE;
	}
	if ((ptr = GetDeviceStruct(fd)) == NULL) {
		commerror = IE_BADID;
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
			ptr->suspended = 1;
			break;
		case CLRBREAK:
			ptr->suspended = 0;
			break;
		default:
			fprintf(stderr,
			"EscapeCommFunction32 fd: %d, unknown function: %d\n", 
			fd, nFunction);
			break;				
	}
	
	if (tcsetattr(fd, TCSADRAIN, &port) == -1) {
		commerror = WinError();
		return FALSE;	
	} else {
		commerror = 0;
		return TRUE;
	}
}

/*****************************************************************************
 *	FlushComm	(USER.215)
 */
INT16 FlushComm(INT16 fd,INT16 fnQueue)
{
	int queue;

    	dprintf_comm(stddeb,"FlushComm fd: %d, queue: %d\n", fd, fnQueue);
	switch (fnQueue) {
		case 0:	queue = TCOFLUSH;
			break;
		case 1:	queue = TCIFLUSH;
			break;
		default:fprintf(stderr,
				"FlushComm fd: %d, UNKNOWN queue: %d\n", 
				fd, fnQueue);
			return -1;
		}
	if (tcflush(fd, fnQueue)) {
		commerror = WinError();
		return -1;	
	} else {
		commerror = 0;
		return 0;
	}
}  

/*****************************************************************************
 *	GetCommError	(USER.203)
 */
INT16 GetCommError(INT16 fd,LPCOMSTAT lpStat)
{
	int temperror;

    	dprintf_comm(stddeb,
		"GetCommError: fd %d (current error %d)\n", fd, commerror);
	temperror = commerror;
	commerror = 0;
	return(temperror);
}

/*****************************************************************************
 *	ClearCommError	(KERNEL32.21)
 */
BOOL32 ClearCommError(INT32 fd,LPDWORD errors,LPCOMSTAT lpStat)
{
	int temperror;

    	dprintf_comm(stddeb,
		"ClearCommError: fd %d (current error %d)\n", fd, commerror);
	temperror = commerror;
	commerror = 0;
	return TRUE;
}

/*****************************************************************************
 *	SetCommEventMask	(USER.208)
 */
UINT16	*SetCommEventMask(INT16 fd,UINT16 fuEvtMask)
{
    	dprintf_comm(stddeb,"SetCommEventMask:fd %d,mask %d\n",fd,fuEvtMask);
	eventmask |= fuEvtMask;
	return (UINT *)&eventmask;	/* FIXME, should be SEGPTR */
}

/*****************************************************************************
 *	GetCommEventMask	(USER.209)
 */
UINT16 GetCommEventMask(INT16 fd,UINT16 fnEvtClear)
{
    	dprintf_comm(stddeb,
		"GetCommEventMask: fd %d, mask %d\n", fd, fnEvtClear);
	eventmask &= ~fnEvtClear;
	return eventmask;
}

/*****************************************************************************
 *	GetCommMask	(KERNEL32.156)
 */
BOOL32 GetCommMask(INT32 fd,LPDWORD evtmask)
{
    	dprintf_comm(stddeb,
		"GetCommMask: fd %d, mask %p\n", fd, evtmask);
	*evtmask = eventmask;
	return TRUE;
}

/*****************************************************************************
 *	SetCommMask	(KERNEL32.451)
 */
BOOL32 SetCommMask(INT32 fd,DWORD evtmask)
{
    	dprintf_comm(stddeb,
		"SetCommMask: fd %d, mask %lx\n", fd, evtmask);
	eventmask = evtmask;
	return TRUE;
}

/*****************************************************************************
 *	SetCommState16	(USER.201)
 */
INT16 SetCommState16(LPDCB16 lpdcb)
{
	struct termios port;
	struct DosDeviceStruct *ptr;

    	dprintf_comm(stddeb,
		"SetCommState: fd %d, ptr %p\n", lpdcb->Id, lpdcb);
	if (tcgetattr(lpdcb->Id, &port) == -1) {
		commerror = WinError();	
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

	if ((ptr = GetDeviceStruct(lpdcb->Id)) == NULL) {
		commerror = IE_BADID;
		return -1;
	}
	if (ptr->baudrate > 0)
	  	lpdcb->BaudRate = ptr->baudrate;
    	dprintf_comm(stddeb,"SetCommState: baudrate %d\n",lpdcb->BaudRate);
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
			return -1;
	}
#else
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
                        return -1;
        }
        port.c_ispeed = port.c_ospeed;
#endif
    	dprintf_comm(stddeb,"SetCommState: bytesize %d\n",lpdcb->ByteSize);
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
			return -1;
	}

    	dprintf_comm(stddeb,"SetCommState: parity %d\n",lpdcb->Parity);
	port.c_cflag &= ~(PARENB | PARODD);
	if (lpdcb->fParity)
		switch (lpdcb->Parity) {
			case NOPARITY:
				port.c_iflag &= ~INPCK;
				break;
			case ODDPARITY:
				port.c_cflag |= (PARENB | PARODD);
				port.c_iflag |= INPCK;
				break;
			case EVENPARITY:
				port.c_cflag |= PARENB;
				port.c_iflag |= INPCK;
				break;
			default:
				commerror = IE_BYTESIZE;
				return -1;
		}
	

    	dprintf_comm(stddeb,"SetCommState: stopbits %d\n",lpdcb->StopBits);
	switch (lpdcb->StopBits) {
		case ONESTOPBIT:
				port.c_cflag &= ~CSTOPB;
				break;
		case TWOSTOPBITS:
				port.c_cflag |= CSTOPB;
				break;
		default:
			commerror = IE_BYTESIZE;
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
	if (lpdcb->fOutX)
		port.c_iflag |= IXOFF;

	if (tcsetattr(lpdcb->Id, TCSADRAIN, &port) == -1) {
		commerror = WinError();	
		return -1;
	} else {
		commerror = 0;
		return 0;
	}
}

/*****************************************************************************
 *	SetCommState32	(KERNEL32.452)
 */
BOOL32 SetCommState32(INT32 fd,LPDCB32 lpdcb)
{
	struct termios port;
	struct DosDeviceStruct *ptr;

    	dprintf_comm(stddeb,"SetCommState: fd %d, ptr %p\n",fd,lpdcb);
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

	if ((ptr = GetDeviceStruct(fd)) == NULL) {
		commerror = IE_BADID;
		return FALSE;
	}
	if (ptr->baudrate > 0)
	  	lpdcb->BaudRate = ptr->baudrate;
    	dprintf_comm(stddeb,"SetCommState: baudrate %ld\n",lpdcb->BaudRate);
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
#else
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
    	dprintf_comm(stddeb,"SetCommState: bytesize %d\n",lpdcb->ByteSize);
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

    	dprintf_comm(stddeb,"SetCommState: parity %d\n",lpdcb->Parity);
	port.c_cflag &= ~(PARENB | PARODD);
	if (lpdcb->fParity)
		switch (lpdcb->Parity) {
			case NOPARITY:
				port.c_iflag &= ~INPCK;
				break;
			case ODDPARITY:
				port.c_cflag |= (PARENB | PARODD);
				port.c_iflag |= INPCK;
				break;
			case EVENPARITY:
				port.c_cflag |= PARENB;
				port.c_iflag |= INPCK;
				break;
			default:
				commerror = IE_BYTESIZE;
				return FALSE;
		}
	

    	dprintf_comm(stddeb,"SetCommState: stopbits %d\n",lpdcb->StopBits);
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
	if (lpdcb->fOutX)
		port.c_iflag |= IXOFF;

	if (tcsetattr(fd,TCSADRAIN,&port)==-1) {
		commerror = WinError();	
		return FALSE;
	} else {
		commerror = 0;
		return TRUE;
	}
}


/*****************************************************************************
 *	GetCommState	(USER.202)
 */
INT16 GetCommState16(INT16 fd, LPDCB16 lpdcb)
{
	struct termios port;

    	dprintf_comm(stddeb,"GetCommState: fd %d, ptr %p\n", fd, lpdcb);
	if (tcgetattr(fd, &port) == -1) {
		commerror = WinError();	
		return -1;
	}
	lpdcb->Id = fd;
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
	
	switch (port.c_cflag & ~(PARENB | PARODD)) {
		case 0:
			lpdcb->fParity = NOPARITY;
			break;
		case PARENB:
			lpdcb->fParity = EVENPARITY;
			break;
		case (PARENB | PARODD):
			lpdcb->fParity = ODDPARITY;		
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

	commerror = 0;
	return 0;
}

/*****************************************************************************
 *	GetCommState	(KERNEL32.159)
 */
BOOL32 GetCommState32(INT32 fd, LPDCB32 lpdcb)
{
	struct termios	port;


    	dprintf_comm(stddeb,"GetCommState32: fd %d, ptr %p\n", fd, lpdcb);
	if (tcgetattr(fd, &port) == -1) {
		commerror = WinError();	
		return FALSE;
	}
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
	
	switch (port.c_cflag & ~(PARENB | PARODD)) {
		case 0:
			lpdcb->fParity = NOPARITY;
			break;
		case PARENB:
			lpdcb->fParity = EVENPARITY;
			break;
		case (PARENB | PARODD):
			lpdcb->fParity = ODDPARITY;		
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
	return TRUE;
}

/*****************************************************************************
 *	TransmitCommChar	(USER.206)
 */
INT16 TransmitCommChar16(INT16 fd,CHAR chTransmit)
{
	struct DosDeviceStruct *ptr;

    	dprintf_comm(stddeb,
		"TransmitCommChar: fd %d, data %d \n", fd, chTransmit);
	if ((ptr = GetDeviceStruct(fd)) == NULL) {
		commerror = IE_BADID;
		return -1;
	}

	if (ptr->suspended) {
		commerror = IE_HARDWARE;
		return -1;
	}	

	if (write(fd, (void *) &chTransmit, 1) == -1) {
		commerror = WinError();
		return -1;	
	}  else {
		commerror = 0;
		return 0;
	}
}

/*****************************************************************************
 *	TransmitCommChar	(KERNEL32.535)
 */
BOOL32 TransmitCommChar32(INT32 fd,CHAR chTransmit)
{
	struct DosDeviceStruct *ptr;

    	dprintf_comm(stddeb,"TransmitCommChar32(%d,'%c')\n",fd,chTransmit);
	if ((ptr = GetDeviceStruct(fd)) == NULL) {
		commerror = IE_BADID;
		return FALSE;
	}

	if (ptr->suspended) {
		commerror = IE_HARDWARE;
		return FALSE;
	}
	if (write(fd, (void *) &chTransmit, 1) == -1) {
		commerror = WinError();
		return FALSE;
	}  else {
		commerror = 0;
		return TRUE;
	}
}

/*****************************************************************************
 *	UngetCommChar	(USER.212)
 */
INT16 UngetCommChar(INT16 fd,CHAR chUnget)
{
	struct DosDeviceStruct *ptr;

    	dprintf_comm(stddeb,"UngetCommChar: fd %d (char %d)\n", fd, chUnget);
	if ((ptr = GetDeviceStruct(fd)) == NULL) {
		commerror = IE_BADID;
		return -1;
	}

	if (ptr->suspended) {
		commerror = IE_HARDWARE;
		return -1;
	}	

	ptr->unget = 1;
	ptr->unget_byte = chUnget;
	commerror = 0;
	return 0;
}

/*****************************************************************************
 *	ReadComm	(USER.204)
 */
INT16 ReadComm(INT16 fd,LPSTR lpvBuf,INT16 cbRead)
{
	int status, length;
	struct DosDeviceStruct *ptr;

    	dprintf_comm(stddeb,
	    "ReadComm: fd %d, ptr %p, length %d\n", fd, lpvBuf, cbRead);
	if ((ptr = GetDeviceStruct(fd)) == NULL) {
		commerror = IE_BADID;
		return -1;
	}

	if (ptr->suspended) {
		commerror = IE_HARDWARE;
		return -1;
	}	

	if (ptr->unget) {
		*lpvBuf = ptr->unget_byte;
		lpvBuf++;
		ptr->unget = 0;

		length = 1;
	} else
	 	length = 0;

	status = read(fd, (void *) lpvBuf, cbRead);

	if (status == -1) {
                if (errno != EAGAIN) {
                       commerror = WinError();
                       return -1 - length;
                } else {
                        commerror = 0;
                        return length;
                }
 	} else {
		commerror = 0;
		return length + status;
	}
}

/*****************************************************************************
 *	WriteComm	(USER.205)
 */
INT16 WriteComm(INT16 fd, LPSTR lpvBuf, INT16 cbWrite)
{
	int x, length;
	struct DosDeviceStruct *ptr;

    	dprintf_comm(stddeb,"WriteComm: fd %d, ptr %p, length %d\n", 
		fd, lpvBuf, cbWrite);
	if ((ptr = GetDeviceStruct(fd)) == NULL) {
		commerror = IE_BADID;
		return -1;
	}

	if (ptr->suspended) {
		commerror = IE_HARDWARE;
		return -1;
	}	
	
	for (x=0; x != cbWrite ; x++)
        dprintf_comm(stddeb,"%c", *(lpvBuf + x) );

	length = write(fd, (void *) lpvBuf, cbWrite);
	
	if (length == -1) {
		commerror = WinError();
		return -1;	
	} else {
		commerror = 0;	
		return length;
	}
}


/*****************************************************************************
 *	GetCommTimeouts		(KERNEL32.160)
 */
BOOL32 GetCommTimeouts(INT32 fd,LPCOMMTIMEOUTS lptimeouts) {
	dprintf_comm(stddeb,"GetCommTimeouts(%x,%p), empty stub.\n",
		fd,lptimeouts
	);
	return TRUE;
}

/*****************************************************************************
 *	SetCommTimeouts		(KERNEL32.453)
 */
BOOL32 SetCommTimeouts(INT32 fd,LPCOMMTIMEOUTS lptimeouts) {
	dprintf_comm(stddeb,"SetCommTimeouts(%x,%p), empty stub.\n",
		fd,lptimeouts
	);
	return TRUE;
}
