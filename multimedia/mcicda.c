/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Sample MCI CDAUDIO Wine Driver for Linux
 *
 * Copyright 1994 Martin Ayotte
 */

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "windows.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"
#include "debug.h"

#ifdef HAVE_LINUX_CDROM_H
# include <linux/cdrom.h>
#endif
#ifdef HAVE_MACHINE_SOUNDCARD_H
# include <machine/soundcard.h>
#endif
#ifdef HAVE_SYS_CDIO_H
# include <sys/cdio.h>
#endif

#ifdef __FreeBSD__
# define CDAUDIO_DEV "/dev/rcd0c"
#else
# define CDAUDIO_DEV "/dev/cdrom"
#endif

#ifdef SOUND_VERSION
# define IOCTL(a,b,c)		ioctl(a,b,&c)
#else
# define IOCTL(a,b,c)		(c = ioctl(a,b,c) )
#endif

#define MAX_CDAUDIODRV 		(1)
#define MAX_CDAUDIO_TRACKS 	256

#define CDFRAMES_PERSEC 	75
#define CDFRAMES_PERMIN 	4500
#define SECONDS_PERMIN	 	60

#if defined(linux) || defined(__FreeBSD__)
typedef struct {
    int     		nUseCount;          /* Incremented for each shared open */
    BOOL16  		fShareable;         /* TRUE if first open was shareable */
    WORD    		wNotifyDeviceID;    /* MCI device ID with a pending notification */
    HANDLE16 		hCallback;          /* Callback handle for pending notification */
    MCI_OPEN_PARMS16 	openParms;
    DWORD		dwTimeFormat;
    int			unixdev;
#ifdef linux
    struct cdrom_subchnl	sc;
#elif __FreeBSD__
    struct cd_sub_channel_info	sc;
#endif
    int			mode;
    UINT16		nCurTrack;
    DWORD		dwCurFrame;
    UINT16		nTracks;
    DWORD		dwTotalLen;
    LPDWORD		lpdwTrackLen;
    LPDWORD		lpdwTrackPos;
    DWORD		dwFirstOffset;
} WINE_CDAUDIO;

static WINE_CDAUDIO	CDADev[MAX_CDAUDIODRV];
#endif

/*-----------------------------------------------------------------------*/


/**************************************************************************
 * 				CDAUDIO_GetNumberOfTracks		[internal]
 */
static UINT16 CDAUDIO_GetNumberOfTracks(UINT16 wDevID)
{
#if defined(linux) || defined(__FreeBSD__)
#ifdef linux
    struct cdrom_tochdr	hdr;
#elif __FreeBSD__
    struct ioc_toc_header	hdr;
#endif
    
    if (CDADev[wDevID].nTracks == 0) {
	if (ioctl(CDADev[wDevID].unixdev,
#ifdef linux
		  CDROMREADTOCHDR
#elif __FreeBSD__
		  CDIOREADTOCHEADER
#endif
		  , &hdr)) {
	    WARN(cdaudio, "(%04X) -- Error occured !\n", 
		 wDevID);
	    return (WORD)-1;
	}
#ifdef linux
	CDADev[wDevID].nTracks = hdr.cdth_trk1;
#elif __FreeBSD__
	CDADev[wDevID].nTracks = hdr.ending_track - hdr.starting_track + 1;
#endif
    }
    return CDADev[wDevID].nTracks;
#else
    return (WORD)-1;
#endif
}


/**************************************************************************
 * 				CDAUDIO_GetTracksInfo			[internal]
 */
static BOOL32 CDAUDIO_GetTracksInfo(UINT16 wDevID)
{
#if defined(linux) || defined(__FreeBSD__)
    int		i, length;
    int		start, last_start = 0;
    int		total_length = 0;
#ifdef linux
    struct cdrom_tocentry	entry;
#elif __FreeBSD__
    struct ioc_read_toc_entry	entry;
    struct cd_toc_entry             toc_buffer;
#endif
    
    if (CDADev[wDevID].nTracks == 0) {
	if (CDAUDIO_GetNumberOfTracks(wDevID) == (WORD)-1) return FALSE;
    }
    TRACE(cdaudio,"nTracks=%u\n", 
	  CDADev[wDevID].nTracks);
    if (CDADev[wDevID].lpdwTrackLen != NULL) 
	free(CDADev[wDevID].lpdwTrackLen);
    CDADev[wDevID].lpdwTrackLen = (LPDWORD)malloc(
						  (CDADev[wDevID].nTracks + 1) * sizeof(DWORD));
    if (CDADev[wDevID].lpdwTrackPos != NULL) 
	free(CDADev[wDevID].lpdwTrackPos);
    CDADev[wDevID].lpdwTrackPos = (LPDWORD)malloc(
						  (CDADev[wDevID].nTracks + 1) * sizeof(DWORD));
    if (CDADev[wDevID].lpdwTrackLen == NULL ||
	CDADev[wDevID].lpdwTrackPos == NULL) {
	WARN(cdaudio, "error allocating track table !\n");
	return FALSE;
    }
    memset(CDADev[wDevID].lpdwTrackLen, 0, 
	   (CDADev[wDevID].nTracks + 1) * sizeof(DWORD));
    memset(CDADev[wDevID].lpdwTrackPos, 0, 
	   (CDADev[wDevID].nTracks + 1) * sizeof(DWORD));
    for (i = 0; i <= CDADev[wDevID].nTracks; i++) {
	if (i == CDADev[wDevID].nTracks)
#ifdef linux
	    entry.cdte_track = CDROM_LEADOUT;
#elif __FreeBSD__
#define LEADOUT 0xaa
	entry.starting_track = LEADOUT; /* XXX */
#endif
	else
#ifdef linux
	    entry.cdte_track = i + 1;
#elif __FreeBSD__
	entry.starting_track = i + 1;
#endif
#ifdef linux
	entry.cdte_format = CDROM_MSF;
#elif __FreeBSD__
	bzero((char *)&toc_buffer, sizeof(toc_buffer));
	entry.address_format = CD_MSF_FORMAT;
	entry.data_len = sizeof(toc_buffer);
	entry.data = &toc_buffer;
#endif
	if (ioctl(CDADev[wDevID].unixdev, 
#ifdef linux
		  CDROMREADTOCENTRY
#elif __FreeBSD__
		  CDIOREADTOCENTRYS
#endif
		  , &entry)) {
	    WARN(cdaudio, "error read entry\n");
	    return FALSE;
	}
#ifdef linux
	start = CDFRAMES_PERSEC * (SECONDS_PERMIN * 
				   entry.cdte_addr.msf.minute + entry.cdte_addr.msf.second) + 
	    entry.cdte_addr.msf.frame;
#elif __FreeBSD__
	start = CDFRAMES_PERSEC * (SECONDS_PERMIN *
				   toc_buffer.addr.msf.minute + toc_buffer.addr.msf.second) +
	    toc_buffer.addr.msf.frame;
#endif
	if (i == 0) {
	    last_start = start;
	    CDADev[wDevID].dwFirstOffset = start;
	    TRACE(cdaudio, "dwFirstOffset=%u\n", 
		  start);
	}
	else {
	    length = start - last_start;
	    last_start = start;
	    start = last_start - length;
	    total_length += length;
	    CDADev[wDevID].lpdwTrackLen[i - 1] = length;
	    CDADev[wDevID].lpdwTrackPos[i - 1] = start;
	    TRACE(cdaudio, "track #%u start=%u len=%u\n",
		  i, start, length);
	}
    }
    CDADev[wDevID].dwTotalLen = total_length;
    TRACE(cdaudio,"total_len=%u\n", 
	  total_length);
    return TRUE;
#else
    return FALSE;
#endif
}


/**************************************************************************
 * 				CDAUDIO_mciOpen			[internal]
 */
static DWORD CDAUDIO_mciOpen(UINT16 wDevID, DWORD dwFlags, void* lp, BOOL32 is32)
{
#if defined(linux) || defined(__FreeBSD__)
    DWORD	dwDeviceID;

    TRACE(cdaudio,"(%04X, %08lX, %p);\n", 
	  wDevID, dwFlags, lp);
    if (lp == NULL) return MCIERR_INTERNAL;
    
    if (is32) 	dwDeviceID = ((LPMCI_OPEN_PARMS32A)lp)->wDeviceID;
    else	dwDeviceID = ((LPMCI_OPEN_PARMS16)lp)->wDeviceID;

    if (CDADev[wDevID].nUseCount > 0) {
	/* The driver already open on this channel */
	/* If the driver was opened shareable before and this open specifies */
	/* shareable then increment the use count */
	if (CDADev[wDevID].fShareable && (dwFlags & MCI_OPEN_SHAREABLE))
	    ++CDADev[wDevID].nUseCount;
	else
	    return MCIERR_MUST_USE_SHAREABLE;
    } else {
	CDADev[wDevID].nUseCount = 1;
	CDADev[wDevID].fShareable = dwFlags & MCI_OPEN_SHAREABLE;
    }
    if (dwFlags & MCI_OPEN_ELEMENT) {
	TRACE(cdaudio,"MCI_OPEN_ELEMENT !\n");
	/*		return MCIERR_NO_ELEMENT_ALLOWED; */
    }

    if( is32 )
    {
      /* memcpy(&CDADev[wDevID].openParms, lp, sizeof(MCI_OPEN_PARMS32A)); */
      /* This is an ugly temporary, hopefully, fix. Slap it in....*/

      LPMCI_OPEN_PARMS32A lpOpenParams = (LPMCI_OPEN_PARMS32A)lp;

      CDADev[wDevID].openParms.dwCallback = lpOpenParams->dwCallback;
      CDADev[wDevID].openParms.wDeviceID  = (WORD)lpOpenParams->wDeviceID;
      CDADev[wDevID].openParms.wReserved0 = 0; /*????*/
      CDADev[wDevID].openParms.lpstrDeviceType = lpOpenParams->lpstrDeviceType;
      CDADev[wDevID].openParms.lpstrElementName = lpOpenParams->lpstrElementName;
      CDADev[wDevID].openParms.lpstrAlias = lpOpenParams->lpstrAlias;
    } 
    else
    {
      memcpy(&CDADev[wDevID].openParms, lp, sizeof(MCI_OPEN_PARMS16));
    }

    CDADev[wDevID].wNotifyDeviceID = dwDeviceID;
    CDADev[wDevID].unixdev = open (CDAUDIO_DEV, O_RDONLY, 0);
    if (CDADev[wDevID].unixdev == -1) {
	WARN(cdaudio,"can't open '%s'!.  errno=%d\n", CDAUDIO_DEV, errno );
        perror( "can't open\n" );
	return MCIERR_HARDWARE;
    }
    CDADev[wDevID].mode = 0;
    CDADev[wDevID].dwTimeFormat = MCI_FORMAT_TMSF;
    CDADev[wDevID].nCurTrack = 0;
    CDADev[wDevID].nTracks = 0;
    CDADev[wDevID].dwTotalLen = 0;
    CDADev[wDevID].dwFirstOffset = 0;
    CDADev[wDevID].lpdwTrackLen = NULL;
    CDADev[wDevID].lpdwTrackPos = NULL;
    if (!CDAUDIO_GetTracksInfo(wDevID)) {
	WARN(cdaudio,"error reading TracksInfo !\n");
	/*		return MCIERR_INTERNAL; */
    }
    
    /*
      Moved to mmsystem.c mciOpen routine
      
      if (dwFlags & MCI_NOTIFY) {
      TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
      lpParms->dwCallback);
      mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
      CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
      }
    */
    return 0;
#else
    return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
 * 				CDAUDIO_mciClose		[internal]
 */
static DWORD CDAUDIO_mciClose(UINT16 wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", 
	  wDevID, dwParam, lpParms);
    if (--CDADev[wDevID].nUseCount == 0) {
	if (CDADev[wDevID].lpdwTrackLen != NULL) free(CDADev[wDevID].lpdwTrackLen);
	if (CDADev[wDevID].lpdwTrackPos != NULL) free(CDADev[wDevID].lpdwTrackPos);
	close(CDADev[wDevID].unixdev);
    }
#endif
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciGetDevCaps	[internal]
 */
static DWORD CDAUDIO_mciGetDevCaps(UINT16 wDevID, DWORD dwFlags, 
				   LPMCI_GETDEVCAPS_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", 
	  wDevID, dwFlags, lpParms);
    if (lpParms == NULL) return MCIERR_INTERNAL;
    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	TRACE(cdaudio, "MCI_GETDEVCAPS_ITEM dwItem=%08lX;\n",
	      lpParms->dwItem);
	switch(lpParms->dwItem) {
	case MCI_GETDEVCAPS_CAN_RECORD:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_HAS_AUDIO:
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_HAS_VIDEO:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    lpParms->dwReturn = MCI_DEVTYPE_CD_AUDIO;
	    break;
	case MCI_GETDEVCAPS_USES_FILES:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    lpParms->dwReturn = FALSE;
	    break;
	case MCI_GETDEVCAPS_CAN_EJECT:
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_PLAY:
	    lpParms->dwReturn = TRUE;
	    break;
	case MCI_GETDEVCAPS_CAN_SAVE:
	    lpParms->dwReturn = FALSE;
	    break;
	default:
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    }
    TRACE(cdaudio, "lpParms->dwReturn=%08lX;\n", 
	  lpParms->dwReturn);
    return 0;
#else
    return MCIERR_INTERNAL;
#endif
}

/**************************************************************************
 * 				CDAUDIO_mciInfo			[internal]
 */
static DWORD CDAUDIO_mciInfo(UINT16 wDevID, DWORD dwFlags, LPMCI_INFO_PARMS16 lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", 
	  wDevID, dwFlags, lpParms);
    if (lpParms == NULL) return MCIERR_INTERNAL;
    lpParms->lpstrReturn = NULL;
    switch(dwFlags) {
    case MCI_INFO_PRODUCT:
	lpParms->lpstrReturn = "Linux CDROM 0.5";
	break;
    default:
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if (lpParms->lpstrReturn != NULL)
	lpParms->dwRetSize = strlen(lpParms->lpstrReturn);
    else
	lpParms->dwRetSize = 0;
    return 0;
#else
    return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
 * 				CDAUDIO_CalcFrame			[internal]
 */
static DWORD CDAUDIO_CalcFrame(UINT16 wDevID, DWORD dwFormatType, DWORD dwTime)
{
    DWORD	dwFrame = 0;
#if defined(linux) || defined(__FreeBSD__)
    UINT16	wTrack;
    
    TRACE(cdaudio,"(%04X, %08lX, %lu);\n", 
	  wDevID, dwFormatType, dwTime);
    
    switch (dwFormatType) {
    case MCI_FORMAT_MILLISECONDS:
	dwFrame = dwTime * CDFRAMES_PERSEC / 1000;
	TRACE(cdaudio, "MILLISECONDS %lu\n", 
	      dwFrame);
	break;
    case MCI_FORMAT_MSF:
	TRACE(cdaudio, "MSF %02u:%02u:%02u\n",
	      MCI_MSF_MINUTE(dwTime), MCI_MSF_SECOND(dwTime), 
	      MCI_MSF_FRAME(dwTime));
	dwFrame += CDFRAMES_PERMIN * MCI_MSF_MINUTE(dwTime);
	dwFrame += CDFRAMES_PERSEC * MCI_MSF_SECOND(dwTime);
	dwFrame += MCI_MSF_FRAME(dwTime);
	break;
    default:
	/* unknown format ! force TMSF ! ... */
	dwFormatType = MCI_FORMAT_TMSF;
    case MCI_FORMAT_TMSF:
	wTrack = MCI_TMSF_TRACK(dwTime);
	TRACE(cdaudio, "MSF %02u-%02u:%02u:%02u\n",
	      MCI_TMSF_TRACK(dwTime), MCI_TMSF_MINUTE(dwTime), 
	      MCI_TMSF_SECOND(dwTime), MCI_TMSF_FRAME(dwTime));
	TRACE(cdaudio, "TMSF trackpos[%u]=%lu\n",
	      wTrack, CDADev[wDevID].lpdwTrackPos[wTrack - 1]);
	dwFrame = CDADev[wDevID].lpdwTrackPos[wTrack - 1];
	dwFrame += CDFRAMES_PERMIN * MCI_TMSF_MINUTE(dwTime);
	dwFrame += CDFRAMES_PERSEC * MCI_TMSF_SECOND(dwTime);
	dwFrame += MCI_TMSF_FRAME(dwTime);
	break;
    }
#endif
    return dwFrame;
}


/**************************************************************************
 * 				CDAUDIO_GetCDStatus				[internal]
 */
static BOOL32 CDAUDIO_GetCDStatus(UINT16 wDevID)
{
#if defined(linux) || defined(__FreeBSD__)
    int		oldmode = CDADev[wDevID].mode;
#ifdef __FreeBSD__
    struct ioc_read_subchannel	read_sc;
    
    read_sc.address_format = CD_MSF_FORMAT;
    read_sc.data_format    = CD_CURRENT_POSITION;
    read_sc.track          = 0;
    read_sc.data_len       = sizeof(CDADev[wDevID].sc);
    read_sc.data	       = (struct cd_sub_channel_info *)&CDADev[wDevID].sc;
#elif linux
    CDADev[wDevID].sc.cdsc_format = CDROM_MSF;
#endif
    if (ioctl(CDADev[wDevID].unixdev,
#ifdef linux
	      CDROMSUBCHNL, &CDADev[wDevID].sc
#elif __FreeBSD__
	      CDIOCREADSUBCHANNEL, &read_sc
#endif
	      )) {
	TRACE(cdaudio,"opened or no_media !\n");
	CDADev[wDevID].mode = MCI_MODE_NOT_READY;
	return TRUE;
    }
    switch (
#ifdef linux
	    CDADev[wDevID].sc.cdsc_audiostatus
#elif __FreeBSD__
	    CDADev[wDevID].sc.header.audio_status
#endif
	    ) {
#ifdef linux
    case CDROM_AUDIO_INVALID:
#elif __FreeBSD__
    case CD_AS_AUDIO_INVALID:
#endif
	WARN(cdaudio, "device doesn't support status, returning NOT_READY.\n");
#ifdef linux
	CDADev[wDevID].mode = MCI_MODE_NOT_READY;
#elif __FreeBSD__
	CDADev[wDevID].mode = MCI_MODE_STOP;
#endif
	break;
#ifdef linux
    case CDROM_AUDIO_NO_STATUS: 
#elif __FreeBSD__
    case CD_AS_NO_STATUS:
#endif
	CDADev[wDevID].mode = MCI_MODE_STOP;
	TRACE(cdaudio,"MCI_MODE_STOP !\n");
	break;
#ifdef linux
    case CDROM_AUDIO_PLAY: 
#elif __FreeBSD__
    case CD_AS_PLAY_IN_PROGRESS:
#endif
	CDADev[wDevID].mode = MCI_MODE_PLAY;
	TRACE(cdaudio,"MCI_MODE_PLAY !\n");
	break;
#ifdef linux
    case CDROM_AUDIO_PAUSED:
#elif __FreeBSD__
    case CD_AS_PLAY_PAUSED:
#endif
	CDADev[wDevID].mode = MCI_MODE_PAUSE;
	TRACE(cdaudio,"MCI_MODE_PAUSE !\n");
	break;
    default:
#ifdef linux
	TRACE(cdaudio,"status=%02X !\n",
	      CDADev[wDevID].sc.cdsc_audiostatus);
#elif __FreeBSD__
	TRACE(cdaudio,"status=%02X !\n",
	      CDADev[wDevID].sc.header.audio_status);
#endif
    }
#ifdef linux
    CDADev[wDevID].nCurTrack = CDADev[wDevID].sc.cdsc_trk;
    CDADev[wDevID].dwCurFrame = 
	CDFRAMES_PERMIN * CDADev[wDevID].sc.cdsc_absaddr.msf.minute +
	CDFRAMES_PERSEC * CDADev[wDevID].sc.cdsc_absaddr.msf.second +
	CDADev[wDevID].sc.cdsc_absaddr.msf.frame;
#elif __FreeBSD__
    CDADev[wDevID].nCurTrack = CDADev[wDevID].sc.what.position.track_number;
    CDADev[wDevID].dwCurFrame = 
	CDFRAMES_PERMIN * CDADev[wDevID].sc.what.position.absaddr.msf.minute +
	CDFRAMES_PERSEC * CDADev[wDevID].sc.what.position.absaddr.msf.second +
	CDADev[wDevID].sc.what.position.absaddr.msf.frame;
#endif
#ifdef linux
    TRACE(cdaudio,"%02u-%02u:%02u:%02u \n",
	  CDADev[wDevID].sc.cdsc_trk,
	  CDADev[wDevID].sc.cdsc_absaddr.msf.minute,
	  CDADev[wDevID].sc.cdsc_absaddr.msf.second,
	  CDADev[wDevID].sc.cdsc_absaddr.msf.frame);
#elif __FreeBSD__
    TRACE(cdaudio,"%02u-%02u:%02u:%02u \n",
	  CDADev[wDevID].sc.what.position.track_number,
	  CDADev[wDevID].sc.what.position.absaddr.msf.minute,
	  CDADev[wDevID].sc.what.position.absaddr.msf.second,
	  CDADev[wDevID].sc.what.position.absaddr.msf.frame);
#endif
    
    if (oldmode != CDADev[wDevID].mode && oldmode == MCI_MODE_OPEN) {
	if (!CDAUDIO_GetTracksInfo(wDevID)) {
	    WARN(cdaudio, "error updating TracksInfo !\n");
	    return MCIERR_INTERNAL;
	}
    }
    return TRUE;
#else
    return FALSE;
#endif
}


/**************************************************************************
 * 				CDAUDIO_CalcTime			[internal]
 */
static DWORD CDAUDIO_CalcTime(UINT16 wDevID, DWORD dwFormatType, DWORD dwFrame)
{
    DWORD	dwTime = 0;
#if defined(linux) || defined(__FreeBSD__)
    UINT16	wTrack;
    UINT16	wMinutes;
    UINT16	wSeconds;
    UINT16	wFrames;
    
    TRACE(cdaudio,"(%04X, %08lX, %lu);\n", 
	  wDevID, dwFormatType, dwFrame);
    
    switch (dwFormatType) {
    case MCI_FORMAT_MILLISECONDS:
	dwTime = dwFrame / CDFRAMES_PERSEC * 1000;
	TRACE(cdaudio, "MILLISECONDS %lu\n", 
	      dwTime);
	break;
    case MCI_FORMAT_MSF:
	wMinutes = dwFrame / CDFRAMES_PERMIN;
	wSeconds = (dwFrame - CDFRAMES_PERMIN * wMinutes) / CDFRAMES_PERSEC;
	wFrames = dwFrame - CDFRAMES_PERMIN * wMinutes - 
	    CDFRAMES_PERSEC * wSeconds;
	dwTime = MCI_MAKE_MSF(wMinutes, wSeconds, wFrames);
	TRACE(cdaudio,"MSF %02u:%02u:%02u -> dwTime=%lu\n",
	      wMinutes, wSeconds, wFrames, dwTime);
	break;
    default:
	/* unknown format ! force TMSF ! ... */
	dwFormatType = MCI_FORMAT_TMSF;
    case MCI_FORMAT_TMSF:
	for (wTrack = 0; wTrack < CDADev[wDevID].nTracks; wTrack++) {
	    /*				dwTime += CDADev[wDevID].lpdwTrackLen[wTrack - 1];
					TRACE(cdaudio, "Adding trk#%u curpos=%u \n", dwTime);
					if (dwTime >= dwFrame) break; */
	    if (CDADev[wDevID].lpdwTrackPos[wTrack - 1] >= dwFrame) break;
	}
	wMinutes = dwFrame / CDFRAMES_PERMIN;
	wSeconds = (dwFrame - CDFRAMES_PERMIN * wMinutes) / CDFRAMES_PERSEC;
	wFrames = dwFrame - CDFRAMES_PERMIN * wMinutes - 
	    CDFRAMES_PERSEC * wSeconds;
	dwTime = MCI_MAKE_TMSF(wTrack, wMinutes, wSeconds, wFrames);
	TRACE(cdaudio, "%02u-%02u:%02u:%02u\n",
	      wTrack, wMinutes, wSeconds, wFrames);
	break;
    }
#endif
    return dwTime;
}


/**************************************************************************
 * 				CDAUDIO_mciStatus		[internal]
 */
static DWORD CDAUDIO_mciStatus(UINT16 wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", 
	  wDevID, dwFlags, lpParms);
    if (lpParms == NULL) return MCIERR_INTERNAL;
    if (CDADev[wDevID].unixdev == 0) return MMSYSERR_NOTENABLED;
    if (dwFlags & MCI_NOTIFY) {
	TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    if (dwFlags & MCI_STATUS_ITEM) {
	switch(lpParms->dwItem) {
	case MCI_STATUS_CURRENT_TRACK:
	    if (!CDAUDIO_GetCDStatus(wDevID)) return MCIERR_INTERNAL;
	    lpParms->dwReturn = CDADev[wDevID].nCurTrack;
	    TRACE(cdaudio,"CURRENT_TRACK=%lu!\n", lpParms->dwReturn);
	    return 0;
	case MCI_STATUS_LENGTH:
	    if (CDADev[wDevID].nTracks == 0) {
		if (!CDAUDIO_GetTracksInfo(wDevID)) {
		    WARN(cdaudio, "error reading TracksInfo !\n");
		    return MCIERR_INTERNAL;
		}
	    }
	    if (dwFlags & MCI_TRACK) {
		TRACE(cdaudio,"MCI_TRACK #%lu LENGTH=??? !\n",
		      lpParms->dwTrack);
		if (lpParms->dwTrack > CDADev[wDevID].nTracks)
		    return MCIERR_OUTOFRANGE;
		lpParms->dwReturn = CDADev[wDevID].lpdwTrackLen[lpParms->dwTrack];
	    }
	    else
		lpParms->dwReturn = CDADev[wDevID].dwTotalLen;
	    lpParms->dwReturn = CDAUDIO_CalcTime(wDevID, 
						 CDADev[wDevID].dwTimeFormat, lpParms->dwReturn);
	    TRACE(cdaudio,"LENGTH=%lu !\n", lpParms->dwReturn);
	    return 0;
	case MCI_STATUS_MODE:
	    if (!CDAUDIO_GetCDStatus(wDevID)) return MCIERR_INTERNAL;
	    lpParms->dwReturn = CDADev[wDevID].mode;
	    TRACE(cdaudio,"MCI_STATUS_MODE=%08lX !\n",
		  lpParms->dwReturn);
	    return 0;
	case MCI_STATUS_MEDIA_PRESENT:
	    lpParms->dwReturn = (CDADev[wDevID].nTracks > 0) ? TRUE : FALSE;
	    if (lpParms->dwReturn == FALSE)
		TRACE(cdaudio,"MEDIA_NOT_PRESENT !\n");
	    else
		TRACE(cdaudio,"MCI_STATUS_MEDIA_PRESENT !\n");
	    return 0;
	case MCI_STATUS_NUMBER_OF_TRACKS:
	    lpParms->dwReturn = CDAUDIO_GetNumberOfTracks(wDevID);
	    TRACE(cdaudio,"MCI_STATUS_NUMBER_OF_TRACKS = %lu !\n",
		  lpParms->dwReturn);
	    if (lpParms->dwReturn == (WORD)-1) return MCIERR_INTERNAL;
	    return 0;
	case MCI_STATUS_POSITION:
	    if (!CDAUDIO_GetCDStatus(wDevID)) return MCIERR_INTERNAL;
	    lpParms->dwReturn = CDADev[wDevID].dwCurFrame;
	    if (dwFlags & MCI_STATUS_START) {
		lpParms->dwReturn = CDADev[wDevID].dwFirstOffset;
		TRACE(cdaudio,"get MCI_STATUS_START !\n");
	    }
	    if (dwFlags & MCI_TRACK) {
		if (lpParms->dwTrack > CDADev[wDevID].nTracks)
		    return MCIERR_OUTOFRANGE;
		lpParms->dwReturn = CDADev[wDevID].lpdwTrackPos[lpParms->dwTrack - 1];
		TRACE(cdaudio,"get MCI_TRACK #%lu !\n", lpParms->dwTrack);
	    }
	    lpParms->dwReturn = CDAUDIO_CalcTime(wDevID,
						 CDADev[wDevID].dwTimeFormat, lpParms->dwReturn);
	    TRACE(cdaudio,"MCI_STATUS_POSITION=%08lX !\n",
		  lpParms->dwReturn);
	    return 0;
	case MCI_STATUS_READY:
	    TRACE(cdaudio,"MCI_STATUS_READY !\n");
	    lpParms->dwReturn = TRUE;
	    return 0;
	case MCI_STATUS_TIME_FORMAT:
	    TRACE(cdaudio,"MCI_STATUS_TIME_FORMAT !\n");
	    lpParms->dwReturn = CDADev[wDevID].dwTimeFormat;
	    return 0;
	default:
	    WARN(cdaudio, "unknown command %08lX !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    }
    WARN(cdaudio, "not MCI_STATUS_ITEM !\n");
    return 0;
#else
    return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
 * 				CDAUDIO_mciPlay			[internal]
 */
static DWORD CDAUDIO_mciPlay(UINT16 wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
    int 	start, end;
#ifdef linux
    struct 	cdrom_msf	msf;
#elif __FreeBSD__
    struct	ioc_play_msf	msf;
#endif
    
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", 
	  wDevID, dwFlags, lpParms);
    if (lpParms == NULL) return MCIERR_INTERNAL;
    if (CDADev[wDevID].unixdev == 0) return MMSYSERR_NOTENABLED;
    start = 0; 		end = CDADev[wDevID].dwTotalLen;
    CDADev[wDevID].nCurTrack = 1;
    if (dwFlags & MCI_FROM) {
	start = CDAUDIO_CalcFrame(wDevID, 
				  CDADev[wDevID].dwTimeFormat, lpParms->dwFrom); 
        TRACE(cdaudio,"MCI_FROM=%08lX -> %u \n",
	      lpParms->dwFrom, start);
    }
    if (dwFlags & MCI_TO) {
	end = CDAUDIO_CalcFrame(wDevID, 
				CDADev[wDevID].dwTimeFormat, lpParms->dwTo);
	TRACE(cdaudio, "MCI_TO=%08lX -> %u \n",
	      lpParms->dwTo, end);
    }
    start += CDADev[wDevID].dwFirstOffset;	
    end += CDADev[wDevID].dwFirstOffset;
#ifdef linux
    msf.cdmsf_min0 = start / CDFRAMES_PERMIN;
    msf.cdmsf_sec0 = (start % CDFRAMES_PERMIN) / CDFRAMES_PERSEC;
    msf.cdmsf_frame0 = start % CDFRAMES_PERSEC;
    msf.cdmsf_min1 = end / CDFRAMES_PERMIN;
    msf.cdmsf_sec1 = (end % CDFRAMES_PERMIN) / CDFRAMES_PERSEC;
    msf.cdmsf_frame1 = end % CDFRAMES_PERSEC;
#elif __FreeBSD__
    msf.start_m     = start / CDFRAMES_PERMIN;
    msf.start_s     = (start % CDFRAMES_PERMIN) / CDFRAMES_PERSEC;
    msf.start_f     = start % CDFRAMES_PERSEC;
    msf.end_m       = end / CDFRAMES_PERMIN;
    msf.end_s       = (end % CDFRAMES_PERMIN) / CDFRAMES_PERSEC;
    msf.end_f       = end % CDFRAMES_PERSEC;
#endif
    if (ioctl(CDADev[wDevID].unixdev,
#ifdef linux
	      CDROMSTART
#elif __FreeBSD__
	      CDIOCSTART
#endif
	      )) {
	WARN(cdaudio, "motor doesn't start !\n");
	return MCIERR_HARDWARE;
    }
    if (ioctl(CDADev[wDevID].unixdev, 
#ifdef linux
	      CDROMPLAYMSF
#elif __FreeBSD__
	      CDIOCPLAYMSF
#endif
	      , &msf)) {
	WARN(cdaudio, "device doesn't play !\n");
	return MCIERR_HARDWARE;
    }
#ifdef linux
    TRACE(cdaudio,"msf = %d:%d:%d %d:%d:%d\n",
	  msf.cdmsf_min0, msf.cdmsf_sec0, msf.cdmsf_frame0,
	  msf.cdmsf_min1, msf.cdmsf_sec1, msf.cdmsf_frame1);
#elif __FreeBSD__
    TRACE(cdaudio,"msf = %d:%d:%d %d:%d:%d\n",
	  msf.start_m, msf.start_s, msf.start_f,
	  msf.end_m,   msf.end_s,   msf.end_f);
#endif
    CDADev[wDevID].mode = MCI_MODE_PLAY;
    if (dwFlags & MCI_NOTIFY) {
	TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	/*
	  mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
	  CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
	*/
    }
    return 0;
#else
    return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
 * 				CDAUDIO_mciStop			[internal]
 */
static DWORD CDAUDIO_mciStop(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", 
	  wDevID, dwFlags, lpParms);
    if (lpParms == NULL) return MCIERR_INTERNAL;
    if (ioctl(CDADev[wDevID].unixdev,
#ifdef linux
	      CDROMSTOP
#elif __FreeBSD__
	      CDIOCSTOP
#endif
	      )) return MCIERR_HARDWARE;
    CDADev[wDevID].mode = MCI_MODE_STOP;
    if (dwFlags & MCI_NOTIFY) {
	TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
#else
    return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
 * 				CDAUDIO_mciPause		[internal]
 */
static DWORD CDAUDIO_mciPause(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", 
	  wDevID, dwFlags, lpParms);
    if (lpParms == NULL) return MCIERR_INTERNAL;
    if (ioctl(CDADev[wDevID].unixdev,
#ifdef linux
	      CDROMPAUSE
#elif __FreeBSD__
	      CDIOCPAUSE
#endif
	      )) return MCIERR_HARDWARE;
    CDADev[wDevID].mode = MCI_MODE_PAUSE;
    if (dwFlags & MCI_NOTIFY) {
        TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
#else
    return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
 * 				CDAUDIO_mciResume		[internal]
 */
static DWORD CDAUDIO_mciResume(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", 
	  wDevID, dwFlags, lpParms);
    if (lpParms == NULL) return MCIERR_INTERNAL;
    if (ioctl(CDADev[wDevID].unixdev, 
#ifdef linux
	      CDROMRESUME
#elif __FreeBSD__
	      CDIOCRESUME
#endif
	      )) return MCIERR_HARDWARE;
    CDADev[wDevID].mode = MCI_MODE_STOP;
    if (dwFlags & MCI_NOTIFY) {
	TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
#else
    return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
 * 				CDAUDIO_mciSeek			[internal]
 */
static DWORD CDAUDIO_mciSeek(UINT16 wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
    DWORD	dwRet;
    MCI_PLAY_PARMS PlayParms;
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", 
	  wDevID, dwFlags, lpParms);
    if (lpParms == NULL) return MCIERR_INTERNAL;
    if (ioctl(CDADev[wDevID].unixdev,
#ifdef linux
	      CDROMRESUME
#elif __FreeBSD__
	      CDIOCRESUME
#endif
	      )) {
	perror("ioctl CDROMRESUME");
	return MCIERR_HARDWARE;
    }
    CDADev[wDevID].mode = MCI_MODE_SEEK;
    switch(dwFlags) {
    case MCI_SEEK_TO_START:
	PlayParms.dwFrom = 0;
	break;
    case MCI_SEEK_TO_END:
	PlayParms.dwFrom = CDADev[wDevID].dwTotalLen;
	break;
    case MCI_TO:
	PlayParms.dwFrom = lpParms->dwTo;
	break;
    }
    dwRet = CDAUDIO_mciPlay(wDevID, MCI_WAIT | MCI_FROM, &PlayParms);
    if (dwRet != 0) return dwRet;
    dwRet = CDAUDIO_mciStop(wDevID, MCI_WAIT, (LPMCI_GENERIC_PARMS)&PlayParms);
    if (dwFlags & MCI_NOTIFY) {
	TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return dwRet;
#else
    return MCIERR_HARDWARE;
#endif
}


/**************************************************************************
 * 				CDAUDIO_mciSet			[internal]
 */
static DWORD CDAUDIO_mciSet(UINT16 wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__)
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    if (lpParms == NULL) return MCIERR_INTERNAL;
    /*
      TRACE(cdaudio,"dwTimeFormat=%08lX\n", lpParms->dwTimeFormat);
      TRACE(cdaudio,"dwAudio=%08lX\n", lpParms->dwAudio);
    */
    if (dwFlags & MCI_SET_TIME_FORMAT) {
	switch (lpParms->dwTimeFormat) {
	case MCI_FORMAT_MILLISECONDS:
	    TRACE(cdaudio, "MCI_FORMAT_MILLISECONDS !\n");
	    break;
	case MCI_FORMAT_MSF:
	    TRACE(cdaudio,"MCI_FORMAT_MSF !\n");
	    break;
	case MCI_FORMAT_TMSF:
	    TRACE(cdaudio,"MCI_FORMAT_TMSF !\n");
	    break;
	default:
	    WARN(cdaudio, "bad time format !\n");
	    return MCIERR_BAD_TIME_FORMAT;
	}
	CDADev[wDevID].dwTimeFormat = lpParms->dwTimeFormat;
    }
    if (dwFlags & MCI_SET_DOOR_OPEN) {
	TRACE(cdaudio, "MCI_SET_DOOR_OPEN !\n");
#ifdef __FreeBSD__
	if (ioctl(CDADev[wDevID].unixdev, CDIOCALLOW)) return MCIERR_HARDWARE;
	if (ioctl(CDADev[wDevID].unixdev, CDIOCEJECT)) return MCIERR_HARDWARE;
	if (ioctl(CDADev[wDevID].unixdev, CDIOCPREVENT)) return MCIERR_HARDWARE;
#elif linux
	if (ioctl(CDADev[wDevID].unixdev, CDROMEJECT)) return MCIERR_HARDWARE;
#endif
	CDADev[wDevID].nTracks = 0;
    }
    if (dwFlags & MCI_SET_DOOR_CLOSED) {
	TRACE(cdaudio, "MCI_SET_DOOR_CLOSED !\n");
#ifdef __FreeBSD__
	if (ioctl(CDADev[wDevID].unixdev, CDIOCALLOW)) return MCIERR_HARDWARE;
	if (ioctl(CDADev[wDevID].unixdev, CDIOCCLOSE)) return MCIERR_HARDWARE;
	if (ioctl(CDADev[wDevID].unixdev, CDIOCPREVENT)) return MCIERR_HARDWARE;
#elif linux
	if (ioctl(CDADev[wDevID].unixdev, CDROMEJECT)) return MCIERR_HARDWARE;
	/* XXX should it be ",1" ??? */
#endif
	CDADev[wDevID].nTracks = 0;
    }
    if (dwFlags & MCI_SET_VIDEO) return MCIERR_UNSUPPORTED_FUNCTION;
    if (dwFlags & MCI_SET_ON) return MCIERR_UNSUPPORTED_FUNCTION;
    if (dwFlags & MCI_SET_OFF) return MCIERR_UNSUPPORTED_FUNCTION;
    if (dwFlags & MCI_NOTIFY) {
	TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
			CDADev[wDevID].wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
#else
    return MCIERR_HARDWARE;
#endif
}


/**************************************************************************
 * 				CDAUDIO_DriverProc16		[sample driver]
 */
LONG CDAUDIO_DriverProc16(DWORD dwDevID, HDRVR16 hDriv, WORD wMsg, 
			  DWORD dwParam1, DWORD dwParam2)
{
#if defined(linux) || defined(__FreeBSD__)
    switch(wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:
    case MCI_OPEN_DRIVER:	
    case MCI_OPEN:		return CDAUDIO_mciOpen(dwDevID, dwParam1, (LPMCI_OPEN_PARMS16)PTR_SEG_TO_LIN(dwParam2), FALSE); 
    case DRV_CLOSE:
    case MCI_CLOSE_DRIVER:
    case MCI_CLOSE:		return CDAUDIO_mciClose(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case DRV_ENABLE:		return 1;	
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBox16(0, "Sample MultiMedia Linux Driver !", "MMLinux Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    case MCI_GETDEVCAPS:	return CDAUDIO_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_INFO:		return CDAUDIO_mciInfo(dwDevID, dwParam1, (LPMCI_INFO_PARMS16)PTR_SEG_TO_LIN(dwParam2));
    case MCI_STATUS:		return CDAUDIO_mciStatus(dwDevID, dwParam1, (LPMCI_STATUS_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_SET:		return CDAUDIO_mciSet(dwDevID, dwParam1, (LPMCI_SET_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_PLAY:		return CDAUDIO_mciPlay(dwDevID, dwParam1, (LPMCI_PLAY_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_STOP:		return CDAUDIO_mciStop(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_PAUSE:		return CDAUDIO_mciPause(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_RESUME:		return CDAUDIO_mciResume(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_SEEK:		return CDAUDIO_mciSeek(dwDevID, dwParam1, (LPMCI_SEEK_PARMS)PTR_SEG_TO_LIN(dwParam2));
    case MCI_SET_DOOR_OPEN:	TRACE(cdaudio, "MCI_SET_DOOR_OPEN !\n");
#ifdef __FreeBSD__
	if (ioctl(CDADev[dwDevID].unixdev, CDIOCALLOW)) return MCIERR_HARDWARE;
	if (ioctl(CDADev[dwDevID].unixdev, CDIOCEJECT)) return MCIERR_HARDWARE;
	if (ioctl(CDADev[dwDevID].unixdev, CDIOCPREVENT)) return MCIERR_HARDWARE;
#elif linux
	if (ioctl(CDADev[dwDevID].unixdev, CDROMEJECT)) return MCIERR_HARDWARE;
#endif
	CDADev[dwDevID].nTracks = 0;
	return 0;
    case MCI_SET_DOOR_CLOSED:	TRACE(cdaudio,"MCI_SET_DOOR_CLOSED !\n");
#ifdef __FreeBSD__
	if (ioctl(CDADev[dwDevID].unixdev, CDIOCALLOW)) return MCIERR_HARDWARE;
	if (ioctl(CDADev[dwDevID].unixdev, CDIOCCLOSE)) return MCIERR_HARDWARE;
	if (ioctl(CDADev[dwDevID].unixdev, CDIOCPREVENT)) return MCIERR_HARDWARE;
#elif linux
	if (ioctl(CDADev[dwDevID].unixdev, CDROMEJECT, 1)) return MCIERR_HARDWARE;
#endif
	CDADev[dwDevID].nTracks = 0;
	return 0;
    default:
	return DefDriverProc16(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
#else
    return MCIERR_HARDWARE;
#endif
}
/**************************************************************************
 * 				CDAUDIO_DriverProc32		[sample driver]
 */
LONG CDAUDIO_DriverProc32(DWORD dwDevID, HDRVR16 hDriv, DWORD wMsg, 
			  DWORD dwParam1, DWORD dwParam2)
{
#if defined(linux) || defined(__FreeBSD__)
    switch(wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:
    case MCI_OPEN_DRIVER:	
    case MCI_OPEN:		return CDAUDIO_mciOpen(dwDevID, dwParam1, (LPMCI_OPEN_PARMS32A)dwParam2, TRUE); 
    case DRV_CLOSE:
    case MCI_CLOSE_DRIVER:
    case MCI_CLOSE:		return CDAUDIO_mciClose(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case DRV_ENABLE:		return 1;	
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBox16(0, "Sample MultiMedia Linux Driver !", "MMLinux Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    case MCI_GETDEVCAPS:	return CDAUDIO_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
    case MCI_INFO:		return CDAUDIO_mciInfo(dwDevID, dwParam1, (LPMCI_INFO_PARMS16)dwParam2);
    case MCI_STATUS:		return CDAUDIO_mciStatus(dwDevID, dwParam1, (LPMCI_STATUS_PARMS)dwParam2);
    case MCI_SET:		return CDAUDIO_mciSet(dwDevID, dwParam1, (LPMCI_SET_PARMS)dwParam2);
    case MCI_PLAY:		return CDAUDIO_mciPlay(dwDevID, dwParam1, (LPMCI_PLAY_PARMS)dwParam2);
    case MCI_STOP:		return CDAUDIO_mciStop(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_PAUSE:		return CDAUDIO_mciPause(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_RESUME:		return CDAUDIO_mciResume(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_SEEK:		return CDAUDIO_mciSeek(dwDevID, dwParam1, (LPMCI_SEEK_PARMS)dwParam2);
    case MCI_SET_DOOR_OPEN:	TRACE(cdaudio, "MCI_SET_DOOR_OPEN !\n");
#ifdef __FreeBSD__
	if (ioctl(CDADev[dwDevID].unixdev, CDIOCALLOW)) return MCIERR_HARDWARE;
	if (ioctl(CDADev[dwDevID].unixdev, CDIOCEJECT)) return MCIERR_HARDWARE;
	if (ioctl(CDADev[dwDevID].unixdev, CDIOCPREVENT)) return MCIERR_HARDWARE;
#elif linux
	if (ioctl(CDADev[dwDevID].unixdev, CDROMEJECT)) return MCIERR_HARDWARE;
#endif
	CDADev[dwDevID].nTracks = 0;
	return 0;
    case MCI_SET_DOOR_CLOSED:	TRACE(cdaudio,"MCI_SET_DOOR_CLOSED !\n");
#ifdef __FreeBSD__
	if (ioctl(CDADev[dwDevID].unixdev, CDIOCALLOW)) return MCIERR_HARDWARE;
	if (ioctl(CDADev[dwDevID].unixdev, CDIOCCLOSE)) return MCIERR_HARDWARE;
	if (ioctl(CDADev[dwDevID].unixdev, CDIOCPREVENT)) return MCIERR_HARDWARE;
#elif linux
	if (ioctl(CDADev[dwDevID].unixdev, CDROMEJECT, 1)) return MCIERR_HARDWARE;
#endif
	CDADev[dwDevID].nTracks = 0;
	return 0;
    default:
	return DefDriverProc32(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
#else
    return MCIERR_HARDWARE;
#endif
}


/*-----------------------------------------------------------------------*/
