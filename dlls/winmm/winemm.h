/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*****************************************************************************
 * Copyright 1998, Luiz Otavio L. Zorzella
 *           1999, Eric Pouech
 *
 * Purpose:   multimedia declarations (internal to WINMM & MMSYSTEM DLLs)
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
 *****************************************************************************
 */

#include "mmddk.h"

typedef DWORD (WINAPI *MessageProc16)(UINT16 wDevID, UINT16 wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);
typedef DWORD (WINAPI *MessageProc32)(UINT wDevID, UINT wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);

/* Who said goofy boy ? */
#define	WINE_DI_MAGIC	0x900F1B01

typedef struct tagWINE_DRIVER
{
    DWORD			dwMagic;
    /* as usual LPWINE_DRIVER == hDriver32 */
    DWORD			dwFlags;
    union {
	struct {
	    HMODULE			hModule;
	    DRIVERPROC			lpDrvProc;
	    DWORD		  	dwDriverID;
	} d32;
	struct {
	    HDRVR16			hDriver16;
	} d16;
    } d;
    struct tagWINE_DRIVER*	lpPrevItem;
    struct tagWINE_DRIVER*	lpNextItem;
} WINE_DRIVER, *LPWINE_DRIVER;

typedef struct tagWINE_MLD {
/* EPP struct tagWINE_MLD*	lpNext; */		/* not used so far */
       UINT			uDeviceID;
       UINT			type;
       UINT			mmdIndex;		/* index to low-level driver in MMDrvs table */
       DWORD			dwDriverInstance;	/* this value is driver related, as opposed to
							 * opendesc.dwInstance which is client (callback) related */
       WORD			bFrom32;
       WORD			dwFlags;
       DWORD			dwCallback;
       DWORD			dwClientInstance;
} WINE_MLD, *LPWINE_MLD;

typedef struct  {
       WINE_MLD			mld;
} WINE_WAVE, *LPWINE_WAVE;

typedef struct {
       WINE_MLD			mld;
       MIDIOPENDESC		mod;			/* FIXME: should be removed */
} WINE_MIDI, *LPWINE_MIDI;

typedef struct {
       WINE_MLD			mld;
} WINE_MIXER, *LPWINE_MIXER;

extern	BOOL		MMDRV_Init(void);
extern	UINT		MMDRV_GetNum(UINT);
extern	LPWINE_MLD	MMDRV_Alloc(UINT size, UINT type, LPHANDLE hndl, DWORD* dwFlags,
				    DWORD* dwCallback, DWORD* dwInstance, BOOL bFrom32);
extern	void		MMDRV_Free(HANDLE hndl, LPWINE_MLD mld);
extern	DWORD		MMDRV_Open(LPWINE_MLD mld, UINT wMsg, DWORD dwParam1, DWORD dwParam2);
extern	DWORD		MMDRV_Close(LPWINE_MLD mld, UINT wMsg);
extern	LPWINE_MLD	MMDRV_Get(HANDLE hndl, UINT type, BOOL bCanBeID);
extern  LPWINE_MLD	MMDRV_GetRelated(HANDLE hndl, UINT srcType, BOOL bSrcCanBeID, UINT dstTyped);
extern	DWORD		MMDRV_Message(LPWINE_MLD mld, WORD wMsg, DWORD dwParam1, DWORD dwParam2, BOOL bFrom32);
extern	UINT		MMDRV_PhysicalFeatures(LPWINE_MLD mld, UINT uMsg, DWORD dwParam1, DWORD dwParam2);

#define	MMDRV_AUX		0
#define	MMDRV_MIXER		1
#define	MMDRV_MIDIIN		2
#define	MMDRV_MIDIOUT		3
#define	MMDRV_WAVEIN		4
#define	MMDRV_WAVEOUT		5
#define MMDRV_MAX		6

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
        HDRVR			hDriver;
	DRIVERPROC16		driverProc;
	DWORD			dwPrivate;
        YIELDPROC		lpfnYieldProc;
        DWORD	                dwYieldData;
        BOOL			bIs32;
        HTASK16			hCreatorTask;
        DWORD                   CreatorThread;
        UINT			uTypeCmdTable;
        UINT			uSpecificCmdTable;
        struct tagWINE_MCIDRIVER*lpNext;
} WINE_MCIDRIVER, *LPWINE_MCIDRIVER;

#define WINE_TIMER_IS32	0x80

typedef struct tagWINE_TIMERENTRY {
    UINT			wDelay;
    UINT			wResol;
    FARPROC16 			lpFunc;
    DWORD			dwUser;
    UINT16			wFlags;
    UINT16			wTimerID;
    UINT			uCurTime;
    struct tagWINE_TIMERENTRY*	lpNext;
} WINE_TIMERENTRY, *LPWINE_TIMERENTRY;

typedef struct tagWINE_MMIO {
    MMIOINFO			info;
    struct IOProcList*		ioProc;
    BOOL			bTmpIOProc;
    HANDLE			hMem;
    SEGPTR			buffer16;
    struct tagWINE_MMIO*	lpNext;
    BOOL			bBufferLoaded;
    DWORD                       dwFileSize;
} WINE_MMIO, *LPWINE_MMIO;

typedef struct tagWINE_PLAYSOUND {
    BOOL                        bLoop : 1,
                                bAlloc : 1;
    LPCWSTR		        pszSound;
    HMODULE		        hMod;
    DWORD		        fdwSound;
    struct tagWINE_PLAYSOUND*   lpNext;
} WINE_PLAYSOUND, *LPWINE_PLAYSOUND;

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
    LPWINE_MCIDRIVER 		lpMciDrvs;
    /* low level drivers (unused yet) */
    /* LPWINE_WAVE		lpWave; */
    /* LPWINE_MIDI		lpMidi; */
    /* LPWINE_MIXER		lpMixer; */
    /* mmio part */
    LPWINE_MMIO			lpMMIO;
    /* playsound and sndPlaySound */
    WINE_PLAYSOUND*             lpPlaySound;
    HANDLE                      psLastEvent;
    HANDLE                      psStopEvent;
} WINE_MM_IDATA, *LPWINE_MM_IDATA;

/* function prototypes */

typedef LONG			(*MCIPROC16)(DWORD, HDRVR16, WORD, DWORD, DWORD);
typedef LONG			(*MCIPROC)(DWORD, HDRVR, DWORD, DWORD, DWORD);

extern LPWINE_DRIVER		DRIVER_FindFromHDrvr(HDRVR hDrvr);
extern BOOL			DRIVER_GetLibName(LPCSTR keyName, LPCSTR sectName, LPSTR buf, int sz);
extern LPWINE_DRIVER		DRIVER_TryOpenDriver32(LPCSTR fn, LPARAM lParam2);

extern LPWINE_MCIDRIVER		MCI_GetDriver(UINT16 uDevID);
extern UINT			MCI_GetDriverFromString(LPCSTR str);
extern DWORD			MCI_WriteString(LPSTR lpDstStr, DWORD dstSize, LPCSTR lpSrcStr);
extern const char* 		MCI_MessageToString(UINT16 wMsg);

extern UINT		WINAPI	MCI_DefYieldProc(MCIDEVICEID wDevID, DWORD data);

extern LRESULT			MCI_CleanUp(LRESULT dwRet, UINT wMsg, DWORD dwParam2, BOOL bIs32);

extern DWORD			MCI_SendCommand(UINT wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2, BOOL bFrom32);
extern DWORD 			MCI_SendCommandFrom32(UINT wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2);
extern DWORD 			MCI_SendCommandFrom16(UINT wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2);

void		CALLBACK	WINE_mmThreadEntryPoint(DWORD _pmt);

void 				MMSYSTEM_MMTIME16to32(LPMMTIME mmt32, const MMTIME16* mmt16);
void 				MMSYSTEM_MMTIME32to16(LPMMTIME16 mmt16, const MMTIME* mmt32);

UINT                            MMSYSTEM_mixerOpen(LPHMIXER lphMix, UINT uDeviceID, DWORD dwCallback,
                                                   DWORD dwInstance, DWORD fdwOpen, BOOL bFrom32);
UINT                            MMSYSTEM_midiOutOpen(HMIDIOUT* lphMidiOut, UINT uDeviceID, DWORD dwCallback,
                                                     DWORD dwInstance, DWORD dwFlags, BOOL bFrom32);
UINT                            MMSYSTEM_midiInOpen(HMIDIIN* lphMidiIn, UINT uDeviceID, DWORD dwCallback,
                                                    DWORD dwInstance, DWORD dwFlags, BOOL bFrom32);
MMRESULT                        MMSYSTEM_MidiStream_Open(HMIDISTRM* lphMidiStrm, LPUINT lpuDeviceID,
                                                         DWORD cMidi, DWORD dwCallback,
                                                         DWORD dwInstance, DWORD fdwOpen, BOOL bFrom32);
UINT                            MMSYSTEM_waveOpen(HANDLE* lphndl, UINT uDeviceID, UINT uType,
                                                  const LPWAVEFORMATEX lpFormat, DWORD dwCallback, 
                                                  DWORD dwInstance, DWORD dwFlags, BOOL bFrom32);

BOOL				MULTIMEDIA_MciInit(void);
LPWINE_MM_IDATA			MULTIMEDIA_GetIData(void);
BOOL                            MULTIMEDIA_PlaySound(const void* pszSound, HMODULE hmod, DWORD fdwSound, BOOL bUnicode);

LPWINE_MM_IDATA			TIME_MMTimeStart(void);
void				TIME_MMTimeStop(void);

/* HANDLE16 -> HANDLE conversions */
#define HDRVR_32(h16)		((HDRVR)(ULONG_PTR)(h16))
#define HMIDI_32(h16)		((HMIDI)(ULONG_PTR)(h16))
#define HMIDIIN_32(h16)		((HMIDIIN)(ULONG_PTR)(h16))
#define HMIDIOUT_32(h16)	((HMIDIOUT)(ULONG_PTR)(h16))
#define HMIDISTRM_32(h16)	((HMIDISTRM)(ULONG_PTR)(h16))
#define HMIXER_32(h16)		((HMIXER)(ULONG_PTR)(h16))
#define HMIXEROBJ_32(h16)	((HMIXEROBJ)(ULONG_PTR)(h16))
#define HMMIO_32(h16)		((HMMIO)(ULONG_PTR)(h16))
#define HWAVE_32(h16)		((HWAVE)(ULONG_PTR)(h16))
#define HWAVEIN_32(h16)		((HWAVEIN)(ULONG_PTR)(h16))
#define HWAVEOUT_32(h16)	((HWAVEOUT)(ULONG_PTR)(h16))
#define HWND_32(h16)		((HWND)(ULONG_PTR)(h16))

/* HANDLE -> HANDLE16 conversions */
#define HDRVR_16(h32)		(LOWORD(h32))
#define HMIDI_16(h32)		(LOWORD(h32))
#define HMIDIIN_16(h32)		(LOWORD(h32))
#define HMIDIOUT_16(h32)	(LOWORD(h32))
#define HMIDISTRM_16(h32)	(LOWORD(h32))
#define HMIXER_16(h32)		(LOWORD(h32))
#define HMIXEROBJ_16(h32)	(LOWORD(h32))
#define HMMIO_16(h32)		(LOWORD(h32))
#define HWAVE_16(h32)		(LOWORD(h32))
#define HWAVEIN_16(h32)		(LOWORD(h32))
#define HWAVEOUT_16(h32)	(LOWORD(h32))
#define HWND_16(h32)		(LOWORD(h32))
