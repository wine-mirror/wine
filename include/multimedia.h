/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*****************************************************************************
 * Copyright 1998, Luiz Otavio L. Zorzella
 *           1999, Eric Pouech
 *
 * File:      multimedia.h
 * Purpose:   multimedia declarations (internal to multimedia DLLs)
 *
 *****************************************************************************
 */
#ifndef __WINE_MULTIMEDIA_H 
#define __WINE_MULTIMEDIA_H

#include "mmsystem.h"
#include "winbase.h"

#define MAX_MIDIINDRV 	(16)
/* For now I'm making 16 the maximum number of midi devices one can
 * have. This should be more than enough for everybody. But as a purist,
 * I intend to make it unbounded in the future, as soon as I figure
 * a good way to do so.
 */
#define MAX_MIDIOUTDRV 	(16)

#if defined(HAVE_SYS_SOUNDCARD_H)
# include <sys/soundcard.h>
#elif defined(HAVE_MACHINE_SOUNDCARD_H)
# include <machine/soundcard.h>
#elif defined(HAVE_SOUNDCARD_H)
# include <soundcard.h>
#endif

#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#endif

#ifdef HAVE_OSS
#define MIDI_SEQ "/dev/sequencer"
#else
#define MIDI_DEV "/dev/midi"
#endif

#ifdef SOUND_VERSION
#define IOCTL(a,b,c)		ioctl(a,b,&c)
#else
#define IOCTL(a,b,c)		(c = ioctl(a,b,c))
#endif

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
    UINT			wCurTime;
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
    /* mm timer part */
    HANDLE			hMMTimer;
    DWORD			mmSysTimeMS;
    LPWINE_TIMERENTRY 		lpTimerList;
    CRITICAL_SECTION		cs;
    int				nSizeLpTimers;
    LPWINE_TIMERENTRY		lpTimers;
    /* mci part */
    LPWINE_MCIDRIVER 		lpMciDrv;
} WINE_MM_IDATA, *LPWINE_MM_IDATA;

/* function prototypes */

extern MCI_MapType		MCI_MapMsg16To32A(WORD uDevType, WORD wMsg, DWORD* lParam);
extern MCI_MapType		MCI_UnMapMsg16To32A(WORD uDevTyp, WORD wMsg, DWORD lParam);

extern DWORD 			MCI_Open(DWORD dwParam, LPMCI_OPEN_PARMSA lpParms);
extern DWORD 			MCI_Close(UINT16 wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms);
extern DWORD 			MCI_SysInfo(UINT uDevID, DWORD dwFlags, LPMCI_SYSINFO_PARMSA lpParms);
extern DWORD 			MCI_Break(UINT uDevID, DWORD dwFlags, LPMCI_BREAK_PARMS lpParms);

typedef LONG			(*MCIPROC16)(DWORD, HDRVR16, WORD, DWORD, DWORD);
typedef LONG			(*MCIPROC)(DWORD, HDRVR, DWORD, DWORD, DWORD);

extern WORD		   	MCI_GetDevTypeFromString(LPCSTR str);
extern LPCSTR		   	MCI_GetStringFromDevType(WORD type);
extern LPWINE_MCIDRIVER		MCI_GetDriver(UINT16 uDevID);
extern UINT			MCI_GetDriverFromString(LPCSTR str);
extern DWORD			MCI_WriteString(LPSTR lpDstStr, DWORD dstSize, LPCSTR lpSrcStr);
extern const char* 		MCI_CommandToString(UINT16 wMsg);

extern int			mciInstalledCount;
extern int			mciInstalledListLen;
extern LPSTR			lpmciInstallNames;

extern UINT16		WINAPI	MCI_DefYieldProc(UINT16 wDevID, DWORD data);

extern LRESULT			MCI_CleanUp(LRESULT dwRet, UINT wMsg, DWORD dwParam2, BOOL bIs32);

extern DWORD 			MCI_SendCommandFrom32(UINT wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2);
extern DWORD 			MCI_SendCommandFrom16(UINT wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2);
extern DWORD 			MCI_SendCommandAsync(UINT wDevID, UINT wMsg, DWORD dwParam1, DWORD dwParam2, UINT size);

HINSTANCE16		WINAPI	mmTaskCreate16(SEGPTR spProc, HINSTANCE16 *lphMmTask, DWORD dwPmt);
void    		WINAPI  mmTaskBlock16(HINSTANCE16 hInst);
LRESULT 		WINAPI 	mmTaskSignal16(HTASK16 ht);
void    		WINAPI  mmTaskYield16(void);

void		CALLBACK	WINE_mmThreadEntryPoint(DWORD _pmt);
LRESULT 		WINAPI 	mmThreadCreate16(FARPROC16 fpThreadAddr, LPHANDLE lpHndl, DWORD dwPmt, DWORD dwFlags);
void 			WINAPI 	mmThreadSignal16(HANDLE16 hndl);
void    		WINAPI	mmThreadBlock16(HANDLE16 hndl);
HANDLE16 		WINAPI	mmThreadGetTask16(HANDLE16 hndl);
BOOL16   		WINAPI	mmThreadIsValid16(HANDLE16 hndl);
BOOL16  		WINAPI	mmThreadIsCurrent16(HANDLE16 hndl);

BOOL				MULTIMEDIA_MciInit(void);
LPWINE_MM_IDATA			MULTIMEDIA_GetIData(void);

BOOL				MULTIMEDIA_MidiInit(void);

#endif /* __WINE_MULTIMEDIA_H */
