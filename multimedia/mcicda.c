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
#include "multimedia.h"
#include "debug.h"

#ifdef HAVE_LINUX_CDROM_H
# include <linux/cdrom.h>
#endif
#ifdef HAVE_LINUX_UCDROM_H
# include <linux/ucdrom.h>
#endif
#ifdef HAVE_SYS_CDIO_H
# include <sys/cdio.h>
#endif

#if defined(__NetBSD__)
# define CDAUDIO_DEV "/dev/rcd0d"
#elif defined(__FreeBSD__)
# define CDAUDIO_DEV "/dev/rcd0c"
#else
# define CDAUDIO_DEV "/dev/cdrom"
#endif

#define MAX_CDAUDIODRV 		(1)
#define MAX_CDAUDIO_TRACKS 	256

#define CDFRAMES_PERSEC 	75
#define CDFRAMES_PERMIN 	4500
#define SECONDS_PERMIN	 	60

#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
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
#else
    struct cd_sub_channel_info	sc;
#endif
    int			cdMode;
    int			mciMode;
    UINT16		nCurTrack;
    DWORD		dwCurFrame;
    UINT16		nTracks;
    DWORD		dwTotalLen;
    LPDWORD		lpdwTrackLen;
    LPDWORD		lpdwTrackPos;
    LPBYTE		lpbTrackFlags;
    DWORD		dwFirstOffset;
} WINE_CDAUDIO;

static WINE_CDAUDIO	CDADev[MAX_CDAUDIODRV];
#endif

#ifndef CDROM_DATA_TRACK
#define CDROM_DATA_TRACK 0x04
#endif

/*-----------------------------------------------------------------------*/

/**************************************************************************
 * 				CDAUDIO_mciGetOpenDrv		[internal]	
 */
static WINE_CDAUDIO*  CDAUDIO_mciGetOpenDrv(UINT16 wDevID)
{
    if (wDevID >= MAX_CDAUDIODRV || CDADev[wDevID].nUseCount == 0 || 
	CDADev[wDevID].unixdev == 0) {
	WARN(cdaudio, "Invalid wDevID=%u\n", wDevID);
	return 0;
    }
    return &CDADev[wDevID];
}

/**************************************************************************
 * 				CDAUDIO_GetNumberOfTracks		[internal]
 */
static UINT16 CDAUDIO_GetNumberOfTracks(WINE_CDAUDIO* wcda)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
#ifdef linux
    struct cdrom_tochdr	hdr;
#else
    struct ioc_toc_header	hdr;
#endif
    
    if (wcda->nTracks == 0) {
#ifdef linux
	if (ioctl(wcda->unixdev, CDROMREADTOCHDR, &hdr)) {
#else
	if (ioctl(wcda->unixdev, CDIOREADTOCHEADER, &hdr)) {
#endif
	    WARN(cdaudio, "(%p) -- Error occured !\n", wcda);
	    return (WORD)-1;
	}
#ifdef linux
	wcda->nTracks = hdr.cdth_trk1;
#else
	wcda->nTracks = hdr.ending_track - hdr.starting_track + 1;
#endif
    }
    return wcda->nTracks;
#else
    return (WORD)-1;
#endif
}


/**************************************************************************
 * 				CDAUDIO_GetTracksInfo			[internal]
 */
static BOOL32 CDAUDIO_GetTracksInfo(WINE_CDAUDIO* wcda)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int		i, length;
    int		start, last_start = 0;
    int		total_length = 0;
#ifdef linux
    struct cdrom_tocentry	entry;
#else
    struct ioc_read_toc_entry	entry;
    struct cd_toc_entry             toc_buffer;
#endif
    
    if (wcda->nTracks == 0) {
	if (CDAUDIO_GetNumberOfTracks(wcda) == (WORD)-1) return FALSE;
    }
    TRACE(cdaudio,"nTracks=%u\n", wcda->nTracks);

    if (wcda->lpdwTrackLen != NULL) 
	free(wcda->lpdwTrackLen);
    wcda->lpdwTrackLen = (LPDWORD)malloc((wcda->nTracks + 1) * sizeof(DWORD));
    if (wcda->lpdwTrackPos != NULL) 
	free(wcda->lpdwTrackPos);
    wcda->lpdwTrackPos = (LPDWORD)malloc((wcda->nTracks + 1) * sizeof(DWORD));
    if (wcda->lpbTrackFlags != NULL)
	free(wcda->lpbTrackFlags);
    wcda->lpbTrackFlags = (LPBYTE)malloc((wcda->nTracks + 1) * sizeof(BYTE));
    if (wcda->lpdwTrackLen == NULL || wcda->lpdwTrackPos == NULL ||
	wcda->lpbTrackFlags == NULL)
    {
	WARN(cdaudio, "error allocating track table !\n");
	return FALSE;
    }
    memset(wcda->lpdwTrackLen, 0, (wcda->nTracks + 1) * sizeof(DWORD));
    memset(wcda->lpdwTrackPos, 0, (wcda->nTracks + 1) * sizeof(DWORD));
    memset(wcda->lpbTrackFlags, 0, (wcda->nTracks + 1) * sizeof(BYTE));
    for (i = 0; i <= wcda->nTracks; i++) {
	if (i == wcda->nTracks)
#ifdef linux
	    entry.cdte_track = CDROM_LEADOUT;
#else
#define LEADOUT 0xaa
	entry.starting_track = LEADOUT; /* XXX */
#endif
	else
#ifdef linux
	    entry.cdte_track = i + 1;
#else
	entry.starting_track = i + 1;
#endif
#ifdef linux
	entry.cdte_format = CDROM_MSF;
#else
	bzero((char *)&toc_buffer, sizeof(toc_buffer));
	entry.address_format = CD_MSF_FORMAT;
	entry.data_len = sizeof(toc_buffer);
	entry.data = &toc_buffer;
#endif
#ifdef linux
	if (ioctl(wcda->unixdev, CDROMREADTOCENTRY, &entry)) {
#else
	if (ioctl(wcda->unixdev, CDIOREADTOCENTRYS, &entry)) {
#endif
	    WARN(cdaudio, "error read entry\n");
	    return FALSE;
	}
#ifdef linux
	start = CDFRAMES_PERSEC * (SECONDS_PERMIN * 
				   entry.cdte_addr.msf.minute + entry.cdte_addr.msf.second) + 
	    entry.cdte_addr.msf.frame;
#else
	start = CDFRAMES_PERSEC * (SECONDS_PERMIN *
				   toc_buffer.addr.msf.minute + toc_buffer.addr.msf.second) +
	    toc_buffer.addr.msf.frame;
#endif
	if (i == 0) {
	    last_start = start;
	    wcda->dwFirstOffset = start;
	    TRACE(cdaudio, "dwFirstOffset=%u\n", start);
	} else {
	    length = start - last_start;
	    last_start = start;
	    start = last_start - length;
	    total_length += length;
	    wcda->lpdwTrackLen[i - 1] = length;
	    wcda->lpdwTrackPos[i - 1] = start;
	    TRACE(cdaudio, "track #%u start=%u len=%u\n", i, start, length);
	}
	wcda->lpbTrackFlags[i] =
#ifdef linux
		(entry.cdte_adr << 4) | (entry.cdte_ctrl & 0x0f);
#else
		(toc_buffer.addr_type << 4) | (toc_buffer.control & 0x0f);
#endif 
	TRACE(cdaudio, "track #%u flags=%02x\n", i + 1, wcda->lpbTrackFlags[i]);
    }
    wcda->dwTotalLen = total_length;
    TRACE(cdaudio,"total_len=%u\n", total_length);
    return TRUE;
#else
    return FALSE;
#endif
}

/**************************************************************************
 * 				CDAUDIO_mciOpen			[internal]
 */
static DWORD CDAUDIO_mciOpen(UINT16 wDevID, DWORD dwFlags, LPMCI_OPEN_PARMS32A lpOpenParms)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    DWORD	dwDeviceID;
    WINE_CDAUDIO* 	wcda;

    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpOpenParms);
    
    if (lpOpenParms == NULL) 		return MCIERR_NULL_PARAMETER_BLOCK;
    if (wDevID > MAX_CDAUDIODRV)	return MCIERR_INVALID_DEVICE_ID;

    dwDeviceID = lpOpenParms->wDeviceID;

    wcda = &CDADev[wDevID];

    if (wcda->nUseCount > 0) {
	/* The driver already open on this channel */
	/* If the driver was opened shareable before and this open specifies */
	/* shareable then increment the use count */
	if (wcda->fShareable && (dwFlags & MCI_OPEN_SHAREABLE))
	    ++wcda->nUseCount;
	else
	    return MCIERR_MUST_USE_SHAREABLE;
    } else {
	wcda->nUseCount = 1;
	wcda->fShareable = dwFlags & MCI_OPEN_SHAREABLE;
    }
    if (dwFlags & MCI_OPEN_ELEMENT) {
	TRACE(cdaudio,"MCI_OPEN_ELEMENT !\n");
	/*		return MCIERR_NO_ELEMENT_ALLOWED; */
    }

    wcda->openParms.dwCallback = lpOpenParms->dwCallback;
    wcda->openParms.wDeviceID  = (WORD)lpOpenParms->wDeviceID;
    wcda->openParms.wReserved0 = 0; /*????*/
    wcda->openParms.lpstrDeviceType = lpOpenParms->lpstrDeviceType;
    wcda->openParms.lpstrElementName = lpOpenParms->lpstrElementName;
    wcda->openParms.lpstrAlias = lpOpenParms->lpstrAlias;

    wcda->wNotifyDeviceID = dwDeviceID;
    wcda->unixdev = open (CDAUDIO_DEV, O_RDONLY, 0);
    if (wcda->unixdev == -1) {
	WARN(cdaudio,"can't open '%s'!.  errno=%d\n", CDAUDIO_DEV, errno );
        perror( "can't open\n" );
	return MCIERR_HARDWARE;
    }
    wcda->cdMode = 0;
    wcda->mciMode = MCI_MODE_STOP;
    wcda->dwTimeFormat = MCI_FORMAT_TMSF;
    wcda->nCurTrack = 0;
    wcda->nTracks = 0;
    wcda->dwTotalLen = 0;
    wcda->dwFirstOffset = 0;
    wcda->lpdwTrackLen = NULL;
    wcda->lpdwTrackPos = NULL;
    wcda->lpbTrackFlags = NULL;
    if (!CDAUDIO_GetTracksInfo(&CDADev[wDevID])) {
	WARN(cdaudio,"error reading TracksInfo !\n");
	/*		return MCIERR_INTERNAL; */
    }
    
    /*
      Moved to mmsystem.c mciOpen routine
      
      if (dwFlags & MCI_NOTIFY) {
      TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
      lpParms->dwCallback);
      mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
      wcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
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
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    WINE_CDAUDIO*	wcda = CDAUDIO_mciGetOpenDrv(wDevID);

    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwParam, lpParms);

    if (wcda == NULL) 	return MCIERR_INVALID_DEVICE_ID;
    
    if (--wcda->nUseCount == 0) {
	if (wcda->lpdwTrackLen != NULL) free(wcda->lpdwTrackLen);
	if (wcda->lpdwTrackPos != NULL) free(wcda->lpdwTrackPos);
	if (wcda->lpbTrackFlags != NULL) free(wcda->lpbTrackFlags);
	close(wcda->unixdev);
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
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;

    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	TRACE(cdaudio, "MCI_GETDEVCAPS_ITEM dwItem=%08lX;\n", lpParms->dwItem);

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
    TRACE(cdaudio, "lpParms->dwReturn=%08lX;\n", lpParms->dwReturn);
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
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    DWORD		ret = 0;
    LPSTR		str = 0;
    WINE_CDAUDIO*	wcda = CDAUDIO_mciGetOpenDrv(wDevID);

    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL || lpParms->lpstrReturn == NULL) {
	ret = MCIERR_NULL_PARAMETER_BLOCK;
    } else if (wcda == NULL) {
	ret = MCIERR_INVALID_DEVICE_ID;
    } else {
	TRACE(cdaudio, "buf=%p, len=%lu\n", lpParms->lpstrReturn, lpParms->dwRetSize);

    switch(dwFlags) {
    case MCI_INFO_PRODUCT:
	    str = "Wine's audio CDROM";
	break;
    default:
	    WARN(cdaudio, "Don't know this info command (%lu)\n", dwFlags);
	    ret = MCIERR_UNRECOGNIZED_COMMAND;
    }
    }
    if (str) {
	ret = MCI_WriteString(lpParms->lpstrReturn, lpParms->dwRetSize, str);
    } else {
	lpParms->lpstrReturn[0] = 0;
    }

    return ret;
#else
    return MCIERR_INTERNAL;
#endif
}


/**************************************************************************
 * 				CDAUDIO_CalcFrame			[internal]
 */
static DWORD CDAUDIO_CalcFrame(WINE_CDAUDIO* wcda, DWORD dwTime)
{
    DWORD	dwFrame = 0;
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    UINT16	wTrack;
    
    TRACE(cdaudio,"(%p, %08lX, %lu);\n", wcda, wcda->dwTimeFormat, dwTime);
    
    switch (wcda->dwTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	dwFrame = dwTime * CDFRAMES_PERSEC / 1000;
	TRACE(cdaudio, "MILLISECONDS %lu\n", dwFrame);
	break;
    case MCI_FORMAT_MSF:
	TRACE(cdaudio, "MSF %02u:%02u:%02u\n",
	      MCI_MSF_MINUTE(dwTime), MCI_MSF_SECOND(dwTime), MCI_MSF_FRAME(dwTime));
	dwFrame += CDFRAMES_PERMIN * MCI_MSF_MINUTE(dwTime);
	dwFrame += CDFRAMES_PERSEC * MCI_MSF_SECOND(dwTime);
	dwFrame += MCI_MSF_FRAME(dwTime);
	break;
    case MCI_FORMAT_TMSF:
    default:
	/* unknown format ! force TMSF ! ... */
	wTrack = MCI_TMSF_TRACK(dwTime);
	TRACE(cdaudio, "MSF %02u-%02u:%02u:%02u\n",
	      MCI_TMSF_TRACK(dwTime), MCI_TMSF_MINUTE(dwTime), 
	      MCI_TMSF_SECOND(dwTime), MCI_TMSF_FRAME(dwTime));
	TRACE(cdaudio, "TMSF trackpos[%u]=%lu\n",
	      wTrack, wcda->lpdwTrackPos[wTrack - 1]);
	dwFrame = wcda->lpdwTrackPos[wTrack - 1];
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
static BOOL32 CDAUDIO_GetCDStatus(WINE_CDAUDIO*	wcda)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int		oldmode = wcda->cdMode;
#ifdef linux
    wcda->sc.cdsc_format = CDROM_MSF;
#else
    struct ioc_read_subchannel	read_sc;
    
    read_sc.address_format = CD_MSF_FORMAT;
    read_sc.data_format    = CD_CURRENT_POSITION;
    read_sc.track          = 0;
    read_sc.data_len       = sizeof(wcda->sc);
    read_sc.data	   = (struct cd_sub_channel_info *)&wcda->sc;
#endif
#ifdef linux
    if (ioctl(wcda->unixdev, CDROMSUBCHNL, &wcda->sc)) {
#else
    if (ioctl(wcda->unixdev, CDIOCREADSUBCHANNEL, &read_sc)) {
#endif
	TRACE(cdaudio,"opened or no_media !\n");
	wcda->cdMode = MCI_MODE_OPEN; /* was NOT_READY */
	return TRUE;
    }
    switch (
#ifdef linux
	    wcda->sc.cdsc_audiostatus
#else
	    wcda->sc.header.audio_status
#endif
	    ) {
#ifdef linux
    case CDROM_AUDIO_INVALID:
#else
    case CD_AS_AUDIO_INVALID:
#endif
	WARN(cdaudio, "device doesn't support status, using MCI status.\n");
	wcda->cdMode = 0;
	break;
#ifdef linux
    case CDROM_AUDIO_NO_STATUS: 
#else
    case CD_AS_NO_STATUS:
#endif
	wcda->cdMode = MCI_MODE_STOP;
	TRACE(cdaudio,"MCI_MODE_STOP !\n");
	break;
#ifdef linux
    case CDROM_AUDIO_PLAY: 
#else
    case CD_AS_PLAY_IN_PROGRESS:
#endif
	wcda->cdMode = MCI_MODE_PLAY;
	TRACE(cdaudio,"MCI_MODE_PLAY !\n");
	break;
#ifdef linux
    case CDROM_AUDIO_PAUSED:
#else
    case CD_AS_PLAY_PAUSED:
#endif
	wcda->cdMode = MCI_MODE_PAUSE;
	TRACE(cdaudio,"MCI_MODE_PAUSE !\n");
	break;
    default:
#ifdef linux
	TRACE(cdaudio,"status=%02X !\n",
	      wcda->sc.cdsc_audiostatus);
#else
	TRACE(cdaudio,"status=%02X !\n",
	      wcda->sc.header.audio_status);
#endif
    }
#ifdef linux
    wcda->nCurTrack = wcda->sc.cdsc_trk;
    wcda->dwCurFrame = 
	CDFRAMES_PERMIN * wcda->sc.cdsc_absaddr.msf.minute +
	CDFRAMES_PERSEC * wcda->sc.cdsc_absaddr.msf.second +
	wcda->sc.cdsc_absaddr.msf.frame;
#else
    wcda->nCurTrack = wcda->sc.what.position.track_number;
    wcda->dwCurFrame = 
	CDFRAMES_PERMIN * wcda->sc.what.position.absaddr.msf.minute +
	CDFRAMES_PERSEC * wcda->sc.what.position.absaddr.msf.second +
	wcda->sc.what.position.absaddr.msf.frame;
#endif
#ifdef linux
    TRACE(cdaudio,"%02u-%02u:%02u:%02u \n",
	  wcda->sc.cdsc_trk,
	  wcda->sc.cdsc_absaddr.msf.minute,
	  wcda->sc.cdsc_absaddr.msf.second,
	  wcda->sc.cdsc_absaddr.msf.frame);
#else
    TRACE(cdaudio,"%02u-%02u:%02u:%02u \n",
	  wcda->sc.what.position.track_number,
	  wcda->sc.what.position.absaddr.msf.minute,
	  wcda->sc.what.position.absaddr.msf.second,
	  wcda->sc.what.position.absaddr.msf.frame);
#endif
    
    if (oldmode != wcda->cdMode && oldmode == MCI_MODE_OPEN) {
	if (!CDAUDIO_GetTracksInfo(wcda)) {
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
static DWORD CDAUDIO_CalcTime(WINE_CDAUDIO* wcda, DWORD dwFrame)
{
    DWORD	dwTime = 0;
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    UINT16	wTrack;
    UINT16	wMinutes;
    UINT16	wSeconds;
    UINT16	wFrames;
    
    TRACE(cdaudio,"(%p, %08lX, %lu);\n", wcda, wcda->dwTimeFormat, dwFrame);
    
    switch (wcda->dwTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	dwTime = dwFrame / CDFRAMES_PERSEC * 1000;
	TRACE(cdaudio, "MILLISECONDS %lu\n", dwTime);
	break;
    case MCI_FORMAT_MSF:
	wMinutes = dwFrame / CDFRAMES_PERMIN;
	wSeconds = (dwFrame - CDFRAMES_PERMIN * wMinutes) / CDFRAMES_PERSEC;
	wFrames = dwFrame - CDFRAMES_PERMIN * wMinutes - CDFRAMES_PERSEC * wSeconds;
	dwTime = MCI_MAKE_MSF(wMinutes, wSeconds, wFrames);
	TRACE(cdaudio,"MSF %02u:%02u:%02u -> dwTime=%lu\n",wMinutes, wSeconds, wFrames, dwTime);
	break;
    case MCI_FORMAT_TMSF:
    default:
	/* unknown format ! force TMSF ! ... */
	for (wTrack = 0; wTrack < wcda->nTracks; wTrack++) {
	    /*				dwTime += wcda->lpdwTrackLen[wTrack - 1];
					TRACE(cdaudio, "Adding trk#%u curpos=%u \n", dwTime);
					if (dwTime >= dwFrame) break; */
	    if (wcda->lpdwTrackPos[wTrack - 1] >= dwFrame) break;
	}
	wMinutes = dwFrame / CDFRAMES_PERMIN;
	wSeconds = (dwFrame - CDFRAMES_PERMIN * wMinutes) / CDFRAMES_PERSEC;
	wFrames = dwFrame - CDFRAMES_PERMIN * wMinutes - CDFRAMES_PERSEC * wSeconds;
	dwTime = MCI_MAKE_TMSF(wTrack, wMinutes, wSeconds, wFrames);
	TRACE(cdaudio, "%02u-%02u:%02u:%02u\n", wTrack, wMinutes, wSeconds, wFrames);
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
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    WINE_CDAUDIO*	wcda = CDAUDIO_mciGetOpenDrv(wDevID);
    DWORD	        ret = 0;

    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) {
	ret = MCIERR_NULL_PARAMETER_BLOCK;
    } else if (wcda == NULL) {
	ret = MCIERR_INVALID_DEVICE_ID;
    } else {
    if (dwFlags & MCI_NOTIFY) {
	    TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	    mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			      wcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    if (dwFlags & MCI_STATUS_ITEM) {
	switch(lpParms->dwItem) {
	case MCI_STATUS_CURRENT_TRACK:
		if (!CDAUDIO_GetCDStatus(wcda)) return MCIERR_INTERNAL;
		lpParms->dwReturn = wcda->nCurTrack;
	    TRACE(cdaudio,"CURRENT_TRACK=%lu!\n", lpParms->dwReturn);
	    return 0;
	case MCI_STATUS_LENGTH:
		if (wcda->nTracks == 0) {
		    if (!CDAUDIO_GetTracksInfo(wcda)) {
		    WARN(cdaudio, "error reading TracksInfo !\n");
		    return MCIERR_INTERNAL;
		}
	    }
	    if (dwFlags & MCI_TRACK) {
		    TRACE(cdaudio,"MCI_TRACK #%lu LENGTH=??? !\n", lpParms->dwTrack);
		    if (lpParms->dwTrack > wcda->nTracks)
		    return MCIERR_OUTOFRANGE;
		    lpParms->dwReturn = wcda->lpdwTrackLen[lpParms->dwTrack - 1];
		} else {
		    lpParms->dwReturn = wcda->dwTotalLen;
	    }
		lpParms->dwReturn = CDAUDIO_CalcTime(wcda, lpParms->dwReturn);
	    TRACE(cdaudio,"LENGTH=%lu !\n", lpParms->dwReturn);
	    return 0;
	case MCI_STATUS_MODE:
		if (!CDAUDIO_GetCDStatus(wcda)) return MCIERR_INTERNAL;
		lpParms->dwReturn = wcda->cdMode;
		if (!lpParms->dwReturn) lpParms->dwReturn = wcda->mciMode;
		TRACE(cdaudio,"MCI_STATUS_MODE=%08lX !\n", lpParms->dwReturn);
	    return 0;
	case MCI_STATUS_MEDIA_PRESENT:
		if (!CDAUDIO_GetCDStatus(wcda)) return MCIERR_INTERNAL;
		lpParms->dwReturn = (wcda->nTracks > 0) ? TRUE : FALSE;
		TRACE(cdaudio,"MCI_STATUS_MEDIA_PRESENT =%s!\n", lpParms->dwReturn ? "Y" : "N");
	    return 0;
	case MCI_STATUS_NUMBER_OF_TRACKS:
		lpParms->dwReturn = CDAUDIO_GetNumberOfTracks(wcda);
		TRACE(cdaudio,"MCI_STATUS_NUMBER_OF_TRACKS = %lu !\n", lpParms->dwReturn);
	    if (lpParms->dwReturn == (WORD)-1) return MCIERR_INTERNAL;
	    return 0;
	case MCI_STATUS_POSITION:
		if (!CDAUDIO_GetCDStatus(wcda)) return MCIERR_INTERNAL;
		lpParms->dwReturn = wcda->dwCurFrame;
	    if (dwFlags & MCI_STATUS_START) {
		    lpParms->dwReturn = wcda->dwFirstOffset;
		TRACE(cdaudio,"get MCI_STATUS_START !\n");
	    }
	    if (dwFlags & MCI_TRACK) {
		    if (lpParms->dwTrack > wcda->nTracks)
		    return MCIERR_OUTOFRANGE;
		    lpParms->dwReturn = wcda->lpdwTrackPos[lpParms->dwTrack - 1];
		TRACE(cdaudio,"get MCI_TRACK #%lu !\n", lpParms->dwTrack);
	    }
		lpParms->dwReturn = CDAUDIO_CalcTime(wcda, lpParms->dwReturn);
		TRACE(cdaudio,"MCI_STATUS_POSITION=%08lX !\n", lpParms->dwReturn);
	    return 0;
	case MCI_STATUS_READY:
	    TRACE(cdaudio,"MCI_STATUS_READY !\n");
	    lpParms->dwReturn = TRUE;
	    return 0;
	case MCI_STATUS_TIME_FORMAT:
		lpParms->dwReturn = wcda->dwTimeFormat;
		TRACE(cdaudio,"MCI_STATUS_TIME_FORMAT =%08lx!\n", lpParms->dwReturn);
		return 0;
		/* FIXME: the constant used here are not defined in mmsystem.h */
		/* MCI_CDA_STATUS_TYPE_TRACK is very likely to be 0x00004001 */
	    case MCI_CDA_STATUS_TYPE_TRACK:
		if (!(dwFlags & MCI_TRACK)) return MCIERR_MISSING_PARAMETER;
		lpParms->dwReturn = (wcda->lpbTrackFlags[lpParms->dwTrack - 1] & 
		    CDROM_DATA_TRACK) ? MCI_CDA_TRACK_OTHER : MCI_CDA_TRACK_AUDIO;
		TRACE(cdaudio, "MCI_CDA_STATUS_TYPE_TRACK[%ld]=%08lx\n", lpParms->dwTrack, lpParms->dwReturn);
	    return 0;
	default:
	    WARN(cdaudio, "unknown command %08lX !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
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
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int 	start, end;
    WINE_CDAUDIO*	wcda = CDAUDIO_mciGetOpenDrv(wDevID);
    DWORD		ret = 0;
#ifdef linux
    struct 	cdrom_msf	msf;
#else
    struct	ioc_play_msf	msf;
#endif
    
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (lpParms == NULL) {
	ret = MCIERR_NULL_PARAMETER_BLOCK;
    } else if (wcda == NULL) {
	ret = MCIERR_INVALID_DEVICE_ID;
    } else {
	start = 0; 		
	end = wcda->dwTotalLen;
	wcda->nCurTrack = 1;
    if (dwFlags & MCI_FROM) {
	    start = CDAUDIO_CalcFrame(wcda, lpParms->dwFrom); 
	    TRACE(cdaudio,"MCI_FROM=%08lX -> %u \n", lpParms->dwFrom, start);
    }
    if (dwFlags & MCI_TO) {
	    end = CDAUDIO_CalcFrame(wcda, lpParms->dwTo);
	    TRACE(cdaudio, "MCI_TO=%08lX -> %u \n", lpParms->dwTo, end);
    }
	start += wcda->dwFirstOffset;	
	end += wcda->dwFirstOffset;
#ifdef linux
    msf.cdmsf_min0 = start / CDFRAMES_PERMIN;
    msf.cdmsf_sec0 = (start % CDFRAMES_PERMIN) / CDFRAMES_PERSEC;
    msf.cdmsf_frame0 = start % CDFRAMES_PERSEC;
    msf.cdmsf_min1 = end / CDFRAMES_PERMIN;
    msf.cdmsf_sec1 = (end % CDFRAMES_PERMIN) / CDFRAMES_PERSEC;
    msf.cdmsf_frame1 = end % CDFRAMES_PERSEC;
#else
    msf.start_m     = start / CDFRAMES_PERMIN;
    msf.start_s     = (start % CDFRAMES_PERMIN) / CDFRAMES_PERSEC;
    msf.start_f     = start % CDFRAMES_PERSEC;
    msf.end_m       = end / CDFRAMES_PERMIN;
    msf.end_s       = (end % CDFRAMES_PERMIN) / CDFRAMES_PERSEC;
    msf.end_f       = end % CDFRAMES_PERSEC;
#endif
#ifdef linux
    if (ioctl(wcda->unixdev, CDROMSTART)) {
#else
    if (ioctl(wcda->unixdev, CDIOCSTART, NULL)) {
#endif
	WARN(cdaudio, "motor doesn't start !\n");
	return MCIERR_HARDWARE;
    }
#ifdef linux
    if (ioctl(wcda->unixdev, CDROMPLAYMSF, &msf)) {
#else
    if (ioctl(wcda->unixdev, CDIOCPLAYMSF, &msf)) {
#endif
	WARN(cdaudio, "device doesn't play !\n");
	return MCIERR_HARDWARE;
    }
#ifdef linux
    TRACE(cdaudio,"msf = %d:%d:%d %d:%d:%d\n",
	  msf.cdmsf_min0, msf.cdmsf_sec0, msf.cdmsf_frame0,
	  msf.cdmsf_min1, msf.cdmsf_sec1, msf.cdmsf_frame1);
#else
    TRACE(cdaudio,"msf = %d:%d:%d %d:%d:%d\n",
	  msf.start_m, msf.start_s, msf.start_f,
	  msf.end_m,   msf.end_s,   msf.end_f);
#endif
	wcda->mciMode = MCI_MODE_PLAY;
    if (dwFlags & MCI_NOTIFY) {
	    TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	/*
	  mciDriverNotify((HWND16)LOWORD(lpParms->dwCallback), 
	      wcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
	*/
    }
    }
    return ret;

#else
    return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
 * 				CDAUDIO_mciStop			[internal]
 */
static DWORD CDAUDIO_mciStop(UINT16 wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    WINE_CDAUDIO*	wcda = CDAUDIO_mciGetOpenDrv(wDevID);

    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
#ifdef linux
    if (ioctl(wcda->unixdev, CDROMSTOP))
#else
    if (ioctl(wcda->unixdev, CDIOCSTOP, NULL))
#endif
	return MCIERR_HARDWARE;
    wcda->mciMode = MCI_MODE_STOP;
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
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
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    WINE_CDAUDIO*	wcda = CDAUDIO_mciGetOpenDrv(wDevID);

    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wcda == NULL)	return MCIERR_INVALID_DEVICE_ID;

#ifdef linux
    if (ioctl(wcda->unixdev, CDROMPAUSE))
#else
    if (ioctl(wcda->unixdev, CDIOCPAUSE, NULL))
#endif
	return MCIERR_HARDWARE;
    wcda->mciMode = MCI_MODE_PAUSE;
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
        TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
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
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    WINE_CDAUDIO*	wcda = CDAUDIO_mciGetOpenDrv(wDevID);

    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    
#ifdef linux
    if (ioctl(wcda->unixdev, CDROMRESUME))
#else
    if (ioctl(wcda->unixdev, CDIOCRESUME, NULL))
#endif
	return MCIERR_HARDWARE;
    wcda->mciMode = MCI_MODE_STOP;
    if (lpParms && (dwFlags & MCI_NOTIFY)) {
	TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
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
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    DWORD	dwRet;
    MCI_PLAY_PARMS PlayParms;
    WINE_CDAUDIO*	wcda = CDAUDIO_mciGetOpenDrv(wDevID);

    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;
    
#ifdef linux
    if (ioctl(wcda->unixdev, CDROMRESUME)) {
#else
    if (ioctl(wcda->unixdev, CDIOCRESUME, NULL)) {
#endif
	perror("ioctl CDROMRESUME");
	return MCIERR_HARDWARE;
    }
    wcda->mciMode = MCI_MODE_SEEK;
    switch(dwFlags) {
    case MCI_SEEK_TO_START:
	PlayParms.dwFrom = 0;
	break;
    case MCI_SEEK_TO_END:
	PlayParms.dwFrom = wcda->dwTotalLen;
	break;
    case MCI_TO:
	PlayParms.dwFrom = lpParms->dwTo;
	break;
    }
    dwRet = CDAUDIO_mciPlay(wDevID, MCI_WAIT | MCI_FROM, &PlayParms);
    if (dwRet != 0) return dwRet;
    dwRet = CDAUDIO_mciStop(wDevID, MCI_WAIT, (LPMCI_GENERIC_PARMS)&PlayParms);
    if (dwFlags & MCI_NOTIFY) {
	TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return dwRet;
#else
    return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
 * 				CDAUDIO_mciSetDoor		[internal]
 */
static DWORD	CDAUDIO_mciSetDoor(UINT16 wDevID, int open)
{
    WINE_CDAUDIO*	wcda = CDAUDIO_mciGetOpenDrv(wDevID);

    TRACE(cdaudio, "(%04x, %s) !\n", wDevID, (open) ? "OPEN" : "CLOSE");

    if (wcda == NULL) return MCIERR_INVALID_DEVICE_ID;

#ifdef linux
    if (open) {
	if (ioctl(wcda->unixdev, CDROMEJECT)) return MCIERR_HARDWARE;
	wcda->mciMode = MCI_MODE_OPEN;
    } else {
	if (ioctl(wcda->unixdev, CDROMEJECT, 1)) return MCIERR_HARDWARE;
	wcda->mciMode = MCI_MODE_STOP;
    }
#else
    if (ioctl(wcda->unixdev, CDIOCALLOW, NULL)) return MCIERR_HARDWARE;
    if (open) {
	if (ioctl(wcda->unixdev, CDIOCEJECT, NULL)) return MCIERR_HARDWARE;
	wcda->mciMode = MCI_MODE_OPEN;
    } else {
	if (ioctl(wcda->unixdev, CDIOCCLOSE, NULL)) return MCIERR_HARDWARE;
	wcda->mciMode = MCI_MODE_STOP;
    }
    if (ioctl(wcda->unixdev, CDIOCPREVENT, NULL)) return MCIERR_HARDWARE;
#endif
    wcda->nTracks = 0;
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_mciSet			[internal]
 */
static DWORD CDAUDIO_mciSet(UINT16 wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    WINE_CDAUDIO*	wcda = CDAUDIO_mciGetOpenDrv(wDevID);
    
    TRACE(cdaudio,"(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    
    if (wcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;;
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
	wcda->dwTimeFormat = lpParms->dwTimeFormat;
    }
    if (dwFlags & MCI_SET_DOOR_OPEN) {
	CDAUDIO_mciSetDoor(wDevID, TRUE);
    }
    if (dwFlags & MCI_SET_DOOR_CLOSED) {
	CDAUDIO_mciSetDoor(wDevID, TRUE);
    }
    if (dwFlags & MCI_SET_VIDEO) return MCIERR_UNSUPPORTED_FUNCTION;
    if (dwFlags & MCI_SET_ON) return MCIERR_UNSUPPORTED_FUNCTION;
    if (dwFlags & MCI_SET_OFF) return MCIERR_UNSUPPORTED_FUNCTION;
    if (dwFlags & MCI_NOTIFY) {
	TRACE(cdaudio, "MCI_NOTIFY_SUCCESSFUL %08lX !\n", 
	      lpParms->dwCallback);
	mciDriverNotify16((HWND16)LOWORD(lpParms->dwCallback), 
			  wcda->wNotifyDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
#else
    return MCIERR_HARDWARE;
#endif
}

/**************************************************************************
 * 			MCICDAUDIO_DriverProc32		[sample driver]
 */
LONG MCICDAUDIO_DriverProc32(DWORD dwDevID, HDRVR16 hDriv, DWORD wMsg, 
			  DWORD dwParam1, DWORD dwParam2)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    switch(wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return 1;
    case DRV_CLOSE:		return 1;
    case DRV_ENABLE:		return 1;	
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBox16(0, "Sample MultiMedia Linux Driver !", "MMLinux Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;

    case MCI_OPEN:
    case MCI_OPEN_DRIVER:	return CDAUDIO_mciOpen(dwDevID, dwParam1, (LPMCI_OPEN_PARMS32A)dwParam2);
    case MCI_CLOSE:
    case MCI_CLOSE_DRIVER:	return CDAUDIO_mciClose(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_GETDEVCAPS:	return CDAUDIO_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
    case MCI_INFO:		return CDAUDIO_mciInfo(dwDevID, dwParam1, (LPMCI_INFO_PARMS16)dwParam2);
    case MCI_STATUS:		return CDAUDIO_mciStatus(dwDevID, dwParam1, (LPMCI_STATUS_PARMS)dwParam2);
    case MCI_SET:		return CDAUDIO_mciSet(dwDevID, dwParam1, (LPMCI_SET_PARMS)dwParam2);
    case MCI_PLAY:		return CDAUDIO_mciPlay(dwDevID, dwParam1, (LPMCI_PLAY_PARMS)dwParam2);
    case MCI_STOP:		return CDAUDIO_mciStop(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_PAUSE:		return CDAUDIO_mciPause(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_RESUME:		return CDAUDIO_mciResume(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_SEEK:		return CDAUDIO_mciSeek(dwDevID, dwParam1, (LPMCI_SEEK_PARMS)dwParam2);
	/* FIXME: I wonder if those two next items are really called ? */
    case MCI_SET_DOOR_OPEN:	return CDAUDIO_mciSetDoor(dwDevID, TRUE);
    case MCI_SET_DOOR_CLOSED:	return CDAUDIO_mciSetDoor(dwDevID, FALSE);
    case MCI_LOAD:		
    case MCI_SAVE:		
    case MCI_FREEZE:		
    case MCI_PUT:		
    case MCI_REALIZE:		
    case MCI_UNFREEZE:		
    case MCI_UPDATE:		
    case MCI_WHERE:		
    case MCI_WINDOW:		
    case MCI_STEP:		
    case MCI_SPIN:		
    case MCI_ESCAPE:		
    case MCI_COPY:		
    case MCI_CUT:		
    case MCI_DELETE:		
    case MCI_PASTE:		
	WARN(cdaudio, "Unsupported command=%s\n", MCI_CommandToString(wMsg));
	break;
/*
 *	This is incorrect according to Microsoft...
 *	http://support.microsoft.com/support/kb/articles/q137/5/79.asp
 *
    case MCI_OPEN:
    case MCI_CLOSE:
	FIXME(cdaudio, "Shouldn't receive a MCI_OPEN or CLOSE message\n");
	break;
 */
    default:
	TRACE(cdaudio, "Sending msg=%s to default driver proc\n", MCI_CommandToString(wMsg));
	return DefDriverProc32(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MCIERR_UNRECOGNIZED_COMMAND;
#else
    return MCIERR_HARDWARE;
#endif
}

/*-----------------------------------------------------------------------*/
