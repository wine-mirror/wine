/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*****************************************************************************
 * Copyright 1998, Luiz Otavio L. Zorzella
 *           1999, Eric Pouech
 *
 * File:      winemm.h
 * Purpose:   multimedia declarations (internal to MMSYSTEM and WINMM DLL)
 *
 *****************************************************************************
 */
#ifndef __WINE_MULTIMEDIA_H 
#define __WINE_MULTIMEDIA_H

#include "mmddk.h"

#define WINE_MMTHREAD_CREATED	0x4153494C	/* "BSIL" */
#define WINE_MMTHREAD_DELETED	0xDEADDEAD

typedef struct {
       DWORD			dwSignature;		/* 00 "BSIL" when ok, 0xDEADDEAD when being deleted */
       DWORD			dwCounter;		/* 04 > 1 when in mmThread functions */
       HANDLE			hThread;		/* 08 hThread */
       DWORD                    dwThreadID;     	/* 0C */
       FARPROC16		fpThread;		/* 10 address of thread proc (segptr or lin depending on dwFlags) */
       DWORD			dwThreadPmt;    	/* 14 parameter to be passed upon thread creation to fpThread */
       DWORD                    dwSignalCount;	     	/* 18 counter used for signaling */
       HANDLE                   hEvent;     		/* 1C event */
       HANDLE                   hVxD;		     	/* 20 return from OpenVxDHandle */
       DWORD                    dwStatus;       	/* 24 0x00, 0x10, 0x20, 0x30 */
       DWORD			dwFlags;		/* 28 dwFlags upon creation */
       HANDLE16			hTask;          	/* 2C handle to created task */
} WINE_MMTHREAD;

typedef struct tagWINE_MCIDRIVER {
        UINT			wDeviceID;
        UINT			wType;
	LPSTR			lpstrElementName;
        LPSTR			lpstrDeviceType;
        LPSTR			lpstrAlias;
	HDRVR			hDrv;
	DRIVERPROC16		driverProc;
	DWORD			dwPrivate;
        YIELDPROC		lpfnYieldProc;
        DWORD	                dwYieldData;
        BOOL			bIs32;
        HTASK16			hCreatorTask;
        UINT			uTypeCmdTable;
        UINT			uSpecificCmdTable;
        struct tagWINE_MCIDRIVER*	lpNext;
} WINE_MCIDRIVER, *LPWINE_MCIDRIVER;

typedef enum {
    MCI_MAP_NOMEM, 	/* ko, memory problem */
    MCI_MAP_MSGERROR, 	/* ko, unknown message */
    MCI_MAP_OK, 	/* ok, no memory allocated. to be sent to 16 bit proc. */
    MCI_MAP_OKMEM, 	/* ok, some memory allocated, need to call MCI_UnMapMsg32ATo16. to be sent to 16 bit proc. */
    MCI_MAP_PASS	/* ok, no memory allocated. to be sent to 32 bit proc */
} MCI_MapType;

#define WINE_TIMER_IS32	0x80

typedef struct tagTIMERENTRY {
    UINT			wDelay;
    UINT			wResol;
    FARPROC16 			lpFunc;
    DWORD			dwUser;
    UINT16			wFlags;
    UINT16			wTimerID;
    UINT			uCurTime;
    struct tagTIMERENTRY*	lpNext;
} WINE_TIMERENTRY, *LPWINE_TIMERENTRY;

typedef struct tagWINE_MM_IDATA {
    /* iData reference */
    DWORD			dwThisProcess;
    struct tagWINE_MM_IDATA*	lpNextIData;
    /* winmm part */
    HANDLE			hWinMM32Instance;
    HANDLE			hWinMM16Instance;
    HANDLE			h16Module32;
    CRITICAL_SECTION		cs;
    /* mm timer part */
    HANDLE			hMMTimer;
    DWORD			mmSysTimeMS;
    LPWINE_TIMERENTRY 		lpTimerList;
    int				nSizeLpTimers;
    LPWINE_TIMERENTRY		lpTimers;
    /* mci part */
    LPWINE_MCIDRIVER 		lpMciDrv;
} WINE_MM_IDATA, *LPWINE_MM_IDATA;

/* function prototypes */

typedef LONG			(*MCIPROC16)(DWORD, HDRVR16, WORD, DWORD, DWORD);
typedef LONG			(*MCIPROC)(DWORD, HDRVR, DWORD, DWORD, DWORD);

extern LPWINE_MCIDRIVER		MCI_GetDriver(UINT16 uDevID);
extern UINT			MCI_GetDriverFromString(LPCSTR str);
extern DWORD			MCI_WriteString(LPSTR lpDstStr, DWORD dstSize, LPCSTR lpSrcStr);
extern const char* 		MCI_MessageToString(UINT16 wMsg);

extern UINT16		WINAPI	MCI_DefYieldProc(UINT16 wDevID, DWORD data);

extern LRESULT			MCI_CleanUp(LRESULT dwRet, UINT wMsg, DWORD dwParam2, BOOL bIs32);

extern DWORD			MCI_SendCommand(UINT wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2, BOOL bFrom32);
extern DWORD 			MCI_SendCommandFrom32(UINT wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2);
extern DWORD 			MCI_SendCommandFrom16(UINT wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2);
extern DWORD 			MCI_SendCommandAsync(UINT wDevID, UINT wMsg, DWORD dwParam1, DWORD dwParam2, UINT size);

void		        WINAPI  WINE_mmThreadEntryPoint(DWORD _pmt);

BOOL				MULTIMEDIA_MciInit(void);
LPWINE_MM_IDATA			MULTIMEDIA_GetIData(void);

/* the following definitions shall be removed ASAP (when low level drivers are available) */
DWORD		WINAPI	auxMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
				   DWORD dwParam1, DWORD dwParam2);
DWORD		WINAPI	mixMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
				   DWORD dwParam1, DWORD dwParam2);
DWORD		WINAPI	midMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
				   DWORD dwParam1, DWORD dwParam2);
DWORD		WINAPI	modMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
				   DWORD dwParam1, DWORD dwParam2);
DWORD		WINAPI	widMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
				   DWORD dwParam1, DWORD dwParam2);
DWORD		WINAPI	wodMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
				   DWORD dwParam1, DWORD dwParam2);

#define DCB_FUNC32		0x0007		/* (ugly hack) 32-bit FARPROC */
#define CALLBACK_FUNC32     	0x00070000l    	/* (ugly hack) 32-bit FARPROC */
#define CALLBACK32CONV(x)   ((((x)&CALLBACK_TYPEMASK)==CALLBACK_FUNCTION) ? \
                             (((x)&~CALLBACK_TYPEMASK)|CALLBACK_FUNC32) : (x))

extern	BOOL	OSS_MidiInit(void);

/* end of ugly definitions */

#endif /* __WINE_MULTIMEDIA_H */

