/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*****************************************************************************
 * Copyright 1998, Luiz Otavio L. Zorzella
 *           1999, Eric Pouech
 *
 * File:      mmddk.h
 * Purpose:   multimedia declarations (external to WINMM & MMSYSTEM DLLs
 *                                     for other DLLs (MCI, drivers...))
 *
 *****************************************************************************
 */
#ifndef __MMDDK_H 
#define __MMDDK_H

#include "mmsystem.h"
#include "winbase.h"

#define MAX_MIDIINDRV 	(16)
/* For now I'm making 16 the maximum number of midi devices one can
 * have. This should be more than enough for everybody. But as a purist,
 * I intend to make it unbounded in the future, as soon as I figure
 * a good way to do so.
 */
#define MAX_MIDIOUTDRV 	(16)

/* ================================== 
 *   Multimedia DDK compatible part
 * ================================== */

#include "pshpack1.h"

#define DRVM_INIT		100
#define DRVM_EXIT		101
#define DRVM_DISABLE		102
#define DRVM_ENABLE		103

/* messages that have IOCTL format
 *    dw1 = NULL or handle
 *    dw2 = NULL or ptr to DRVM_IOCTL_DATA
 *    return is MMRESULT
 */
#define DRVM_IOCTL		0x100
#define DRVM_ADD_THRU		(DRVM_IOCTL+1)
#define DRVM_REMOVE_THRU	(DRVM_IOCTL+2)
#define DRVM_IOCTL_LAST		(DRVM_IOCTL+5)
typedef struct {
    DWORD  dwSize; 	/* size of this structure */
    DWORD  dwCmd;  	/* IOCTL command code, 0x80000000 and above reserved for system */
} DRVM_IOCTL_DATA, *LPDRVM_IOCTL_DATA;

/* command code ranges for dwCmd field of DRVM_IOCTL message
 * - codes from 0 to 0x7FFFFFFF are user defined
 * - codes from 0x80000000 to 0xFFFFFFFF are reserved for future definition by microsoft
 */
#define DRVM_IOCTL_CMD_USER   0x00000000L
#define DRVM_IOCTL_CMD_SYSTEM 0x80000000L

#define DRVM_MAPPER		0x2000
#define DRVM_USER               0x4000
#define DRVM_MAPPER_STATUS      (DRVM_MAPPER+0)
#define DRVM_MAPPER_RECONFIGURE (DRVM_MAPPER+1)

#define WODM_INIT		DRVM_INIT
#define WODM_GETNUMDEVS		 3
#define WODM_GETDEVCAPS		 4
#define WODM_OPEN		 5
#define WODM_CLOSE		 6
#define WODM_PREPARE		 7
#define WODM_UNPREPARE		 8
#define WODM_WRITE		 9
#define WODM_PAUSE		10
#define WODM_RESTART		11
#define WODM_RESET		12 
#define WODM_GETPOS		13
#define WODM_GETPITCH		14
#define WODM_SETPITCH		15
#define WODM_GETVOLUME		16
#define WODM_SETVOLUME		17
#define WODM_GETPLAYBACKRATE	18
#define WODM_SETPLAYBACKRATE	19
#define WODM_BREAKLOOP		20

#define WODM_MAPPER_STATUS      (DRVM_MAPPER_STATUS + 0)
#define WAVEOUT_MAPPER_STATUS_DEVICE    0
#define WAVEOUT_MAPPER_STATUS_MAPPED    1
#define WAVEOUT_MAPPER_STATUS_FORMAT    2

#define WIDM_INIT		DRVM_INIT
#define WIDM_GETNUMDEVS		50
#define WIDM_GETDEVCAPS		51
#define WIDM_OPEN		52
#define WIDM_CLOSE		53
#define WIDM_PREPARE		54
#define WIDM_UNPREPARE		55
#define WIDM_ADDBUFFER		56
#define WIDM_START		57
#define WIDM_STOP		58
#define WIDM_RESET		59
#define WIDM_GETPOS		60

#define WIDM_MAPPER_STATUS      (DRVM_MAPPER_STATUS + 0)
#define WAVEIN_MAPPER_STATUS_DEVICE     0
#define WAVEIN_MAPPER_STATUS_MAPPED     1
#define WAVEIN_MAPPER_STATUS_FORMAT     2

#define MODM_INIT		DRVM_INIT
#define MODM_GETNUMDEVS		1
#define MODM_GETDEVCAPS		2
#define MODM_OPEN		3
#define MODM_CLOSE		4
#define MODM_PREPARE		5
#define MODM_UNPREPARE		6
#define MODM_DATA		7
#define MODM_LONGDATA		8
#define MODM_RESET          	9
#define MODM_GETVOLUME		10
#define MODM_SETVOLUME		11
#define MODM_CACHEPATCHES	12      
#define MODM_CACHEDRUMPATCHES	13     

#define MIDM_INIT		DRVM_INIT
#define MIDM_GETNUMDEVS  	53
#define MIDM_GETDEVCAPS  	54
#define MIDM_OPEN        	55
#define MIDM_CLOSE       	56
#define MIDM_PREPARE     	57
#define MIDM_UNPREPARE   	58
#define MIDM_ADDBUFFER   	59
#define MIDM_START       	60
#define MIDM_STOP        	61
#define MIDM_RESET       	62


#define AUXM_INIT             DRVM_INIT
#define AUXDM_GETNUMDEVS    	3
#define AUXDM_GETDEVCAPS    	4
#define AUXDM_GETVOLUME     	5
#define AUXDM_SETVOLUME     	6

#define	MXDM_GETNUMDEVS		1
#define	MXDM_GETDEVCAPS		2
#define	MXDM_OPEN		3
#define	MXDM_CLOSE		4
#define	MXDM_GETLINEINFO	5
#define	MXDM_GETLINECONTROLS	6
#define	MXDM_GETCONTROLDETAILS	7
#define	MXDM_SETCONTROLDETAILS	8

#define MCI_MAX_DEVICE_TYPE_LENGTH 80

#define MCI_FALSE                       (MCI_STRING_OFFSET + 19)
#define MCI_TRUE                        (MCI_STRING_OFFSET + 20)

#define MCI_FORMAT_RETURN_BASE          MCI_FORMAT_MILLISECONDS_S
#define MCI_FORMAT_MILLISECONDS_S       (MCI_STRING_OFFSET + 21)
#define MCI_FORMAT_HMS_S                (MCI_STRING_OFFSET + 22)
#define MCI_FORMAT_MSF_S                (MCI_STRING_OFFSET + 23)
#define MCI_FORMAT_FRAMES_S             (MCI_STRING_OFFSET + 24)
#define MCI_FORMAT_SMPTE_24_S           (MCI_STRING_OFFSET + 25)
#define MCI_FORMAT_SMPTE_25_S           (MCI_STRING_OFFSET + 26)
#define MCI_FORMAT_SMPTE_30_S           (MCI_STRING_OFFSET + 27)
#define MCI_FORMAT_SMPTE_30DROP_S       (MCI_STRING_OFFSET + 28)
#define MCI_FORMAT_BYTES_S              (MCI_STRING_OFFSET + 29)
#define MCI_FORMAT_SAMPLES_S            (MCI_STRING_OFFSET + 30)
#define MCI_FORMAT_TMSF_S               (MCI_STRING_OFFSET + 31)

#define MCI_VD_FORMAT_TRACK_S           (MCI_VD_OFFSET + 5)

#define WAVE_FORMAT_PCM_S               (MCI_WAVE_OFFSET + 0)
#define WAVE_MAPPER_S                   (MCI_WAVE_OFFSET + 1)

#define MCI_SEQ_MAPPER_S                (MCI_SEQ_OFFSET + 5)
#define MCI_SEQ_FILE_S                  (MCI_SEQ_OFFSET + 6)
#define MCI_SEQ_MIDI_S                  (MCI_SEQ_OFFSET + 7)
#define MCI_SEQ_SMPTE_S                 (MCI_SEQ_OFFSET + 8)
#define MCI_SEQ_FORMAT_SONGPTR_S        (MCI_SEQ_OFFSET + 9)
#define MCI_SEQ_NONE_S                  (MCI_SEQ_OFFSET + 10)
#define MIDIMAPPER_S                    (MCI_SEQ_OFFSET + 11)

#define MCI_RESOURCE_RETURNED       0x00010000  /* resource ID */
#define MCI_COLONIZED3_RETURN       0x00020000  /* colonized ID, 3 bytes data */
#define MCI_COLONIZED4_RETURN       0x00040000  /* colonized ID, 4 bytes data */
#define MCI_INTEGER_RETURNED        0x00080000  /* integer conversion needed */
#define MCI_RESOURCE_DRIVER         0x00100000  /* driver owns returned resource */

#define MCI_NO_COMMAND_TABLE    0xFFFF

#define MCI_COMMAND_HEAD        0
#define MCI_STRING              1
#define MCI_INTEGER             2
#define MCI_END_COMMAND         3
#define MCI_RETURN              4
#define MCI_FLAG                5
#define MCI_END_COMMAND_LIST    6
#define MCI_RECT                7
#define MCI_CONSTANT            8
#define MCI_END_CONSTANT        9

#define MAKEMCIRESOURCE(wRet, wRes) MAKELRESULT((wRet), (wRes))

typedef struct {
	DWORD   		dwCallback;
	DWORD   		dwInstance;
	HMIDIOUT		hMidi;
	DWORD   		dwFlags;
} PORTALLOC16, *LPPORTALLOC16;

typedef struct {
	DWORD   		dwCallback;
	DWORD   		dwInstance;
	HMIDIOUT		hMidi;
	DWORD   		dwFlags;
} PORTALLOC, *LPPORTALLOC;

#if 0
/* FIXME: the ???OPENDESC structs will be fixed when low
 * level driver loading is available
 * for now comment them out, and use non corrrect ones in winemm.h
 * to be uncommented ASAP
 */
typedef struct {
	HWAVE16			hWave;
	LPWAVEFORMATEX		lpFormat;
	DWORD			dwCallback;
	DWORD			dwInstance;
	UINT16			uMappedDeviceID;
        DWORD			dnDevNode;
} WAVEOPENDESC16, *LPWAVEOPENDESC16;

typedef struct {
	HWAVE			hWave;
	LPWAVEFORMATEX		lpFormat;
	DWORD			dwCallback;
	DWORD			dwInstance;
	UINT			uMappedDeviceID;
        DWORD			dnDevNode;
} WAVEOPENDESC, *LPWAVEOPENDESC;

typedef struct {
        DWORD  			dwStreamID;
        WORD   			wDeviceID;
} MIDIOPENSTRMID;

typedef struct {
	HMIDI16			hMidi;
	DWORD			dwCallback;
	DWORD			dwInstance;
        UINT16		        reserved;
        DWORD          		dnDevNode;
        DWORD          		cIds;
        MIDIOPENSTRMID 		rgIds;
} MIDIOPENDESC16, *LPMIDIOPENDESC16;

typedef struct {
	HMIDI			hMidi;
	DWORD			dwCallback;
	DWORD			dwInstance;
        DWORD          		dnDevNode;
        DWORD          		cIds;
        MIDIOPENSTRMID 		rgIds;
} MIDIOPENDESC, *LPMIDIOPENDESC;

#if 0
typedef struct {
	UINT16			wDelay;
	UINT16			wResolution;
	LPTIMECALLBACK16	lpFunction;
	DWORD			dwUser;
	UINT16			wFlags;
} TIMEREVENT, *LPTIMEREVENT;
#endif

typedef struct tMIXEROPENDESC16
{
	HMIXEROBJ16		hmx;
        LPVOID			pReserved0;
	DWORD			dwCallback;
	DWORD			dwInstance;
} MIXEROPENDESC16, *LPMIXEROPENDESC16;

typedef struct tMIXEROPENDESC
{
	HMIXEROBJ		hmx;
        LPVOID			pReserved0;
	DWORD			dwCallback;
	DWORD			dwInstance;
} MIXEROPENDESC, *LPMIXEROPENDESC;
#else
/* those definitions are still wine tainted
 * keep them in a temporary phase
 */
typedef struct {
	HWAVE16			hWave;
	LPWAVEFORMAT		lpFormat;
	DWORD			dwCallBack;
	DWORD			dwInstance;
	UINT16			uDeviceID;
} WAVEOPENDESC, *LPWAVEOPENDESC;

typedef struct {
        DWORD  			dwStreamID;
        WORD   			wDeviceID;
} MIDIOPENSTRMID;

/* FIXME: this structure has a different mapping in 16 & 32 bit mode
 * Since, I don't plan to add support for native 16 bit low level
 * multimedia drivers, it'll do.
 */
typedef struct {
	HMIDI16			hMidi;
	DWORD			dwCallback;
	DWORD			dwInstance;
	UINT16			wDevID;
        DWORD          		dnDevNode;
        DWORD          		cIds;
        MIDIOPENSTRMID 		rgIds;
} MIDIOPENDESC, *LPMIDIOPENDESC;

typedef struct tMIXEROPENDESC
{
	HMIXEROBJ16		hmx;
	DWORD			dwCallback;
	DWORD			dwInstance;
	UINT16			uDeviceID;
} MIXEROPENDESC,*LPMIXEROPENDESC;
#endif

typedef struct {
	UINT16			wDeviceID;		/* device ID */
	LPSTR			lpstrParams;		/* parameter string for entry in SYSTEM.INI */
	UINT16			wCustomCommandTable;	/* custom command table (0xFFFF if none)
							 * filled in by the driver */
	UINT16			wType;			/* driver type (filled in by the driver) */
} MCI_OPEN_DRIVER_PARMS16, *LPMCI_OPEN_DRIVER_PARMS16;

typedef struct {
	UINT			wDeviceID;		/* device ID */
	LPSTR			lpstrParams;		/* parameter string for entry in SYSTEM.INI */
	UINT			wCustomCommandTable;	/* custom command table (0xFFFF if none) * filled in by the driver */
	UINT			wType;			/* driver type (filled in by the driver) */
} MCI_OPEN_DRIVER_PARMSA, *LPMCI_OPEN_DRIVER_PARMSA;

typedef struct {
	UINT			wDeviceID;		/* device ID */
	LPWSTR			lpstrParams;		/* parameter string for entry in SYSTEM.INI */
	UINT			wCustomCommandTable;	/* custom command table (0xFFFF if none) * filled in by the driver */
	UINT			wType;			/* driver type (filled in by the driver) */
} MCI_OPEN_DRIVER_PARMSW, *LPMCI_OPEN_DRIVER_PARMSW;
DECL_WINELIB_TYPE_AW(MCI_OPEN_DRIVER_PARMS)
DECL_WINELIB_TYPE_AW(LPMCI_OPEN_DRIVER_PARMS)

DWORD 			WINAPI	mciGetDriverData16(UINT16 uDeviceID);
DWORD 			WINAPI	mciGetDriverData(UINT uDeviceID);

BOOL16			WINAPI	mciSetDriverData16(UINT16 uDeviceID, DWORD dwData);
BOOL			WINAPI	mciSetDriverData(UINT uDeviceID, DWORD dwData);

UINT16			WINAPI	mciDriverYield16(UINT16 uDeviceID);
UINT			WINAPI	mciDriverYield(UINT uDeviceID);

BOOL16			WINAPI	mciDriverNotify16(HWND16 hwndCallback, UINT16 uDeviceID,
						  UINT16 uStatus);
BOOL			WINAPI	mciDriverNotify(HWND hwndCallback, UINT uDeviceID,
						UINT uStatus);

UINT16			WINAPI	mciLoadCommandResource16(HINSTANCE16 hInstance,
						 LPCSTR lpResName, UINT16 uType);
UINT			WINAPI	mciLoadCommandResource(HINSTANCE hInstance,
					       LPCWSTR lpResName, UINT uType);

BOOL16			WINAPI	mciFreeCommandResource16(UINT16 uTable);
BOOL			WINAPI	mciFreeCommandResource(UINT uTable);

HINSTANCE16		WINAPI	mmTaskCreate16(SEGPTR spProc, HINSTANCE16 *lphMmTask, DWORD dwPmt);
void    		WINAPI  mmTaskBlock16(HINSTANCE16 hInst);
LRESULT 		WINAPI 	mmTaskSignal16(HTASK16 ht);
void    		WINAPI  mmTaskYield16(void);

LRESULT 		WINAPI 	mmThreadCreate16(FARPROC16 fpThreadAddr, LPHANDLE lpHndl,
						 DWORD dwPmt, DWORD dwFlags);
void 			WINAPI 	mmThreadSignal16(HANDLE16 hndl);
void    		WINAPI	mmThreadBlock16(HANDLE16 hndl);
HANDLE16 		WINAPI	mmThreadGetTask16(HANDLE16 hndl);
BOOL16   		WINAPI	mmThreadIsValid16(HANDLE16 hndl);
BOOL16  		WINAPI	mmThreadIsCurrent16(HANDLE16 hndl);

#define DCB_NULL		0x0000
#define DCB_WINDOW		0x0001			/* dwCallback is a HWND */
#define DCB_TASK		0x0002			/* dwCallback is a HTASK */
#define DCB_FUNCTION		0x0003			/* dwCallback is a FARPROC */
#define DCB_EVENT		0x0005			/* dwCallback is an EVENT Handler */
#define DCB_TYPEMASK		0x0007
#define DCB_NOSWITCH		0x0008			/* don't switch stacks for callback */

BOOL16			WINAPI	DriverCallback16(DWORD dwCallBack, UINT16 uFlags, HANDLE16 hDev, 
						 WORD wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);
BOOL		 	WINAPI	DriverCallback(DWORD dwCallBack, UINT uFlags, HANDLE hDev, 
					       UINT wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);

/* FIXME: the Wine builtin MCI drivers still use those winmm internal functions
 * 		remove them ASAP
 */
extern DWORD			MCI_WriteString(LPSTR lpDstStr, DWORD dstSize, LPCSTR lpSrcStr);
extern const char* 		MCI_MessageToString(UINT16 wMsg);
extern DWORD 			MCI_SendCommandAsync(UINT wDevID, UINT wMsg, DWORD dwParam1, DWORD dwParam2, UINT size);


#include "poppack.h"

#endif /* __MMDDK_H */

