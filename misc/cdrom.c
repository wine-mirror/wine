/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Sample CDAUDIO Wine Driver for Linux
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1999 Eric Pouech
 */

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "cdrom.h"
#include "drive.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(cdaudio)

#define MAX_CDAUDIO_TRACKS 	256

/**************************************************************************
 * 				CDAUDIO_Open			[internal]
 *
 * drive = 0, 1, ...
 *      or -1 (figure it out)
 */
int	CDAUDIO_Open(WINE_CDAUDIO* wcda, int drive)
{
    int i;
    BOOL avail = FALSE;
    const char *dev;

    if (drive == -1)
    {
	for (i=0; i < MAX_DOS_DRIVES; i++)
	    if (DRIVE_GetType(i) == TYPE_CDROM)
	    {
		drive = i;
		avail = TRUE;
		break;
	    }
    }
    else
	avail = TRUE;
    
    if (avail == FALSE)
    {
	WARN("No CD-ROM #%d found !\n", drive);
	return -1;
    }
    if ((dev = DRIVE_GetDevice(drive)) == NULL)
{
	WARN("No device entry for CD-ROM #%d (drive %c:) found !\n",
		drive, 'A' + drive);
	return -1;
    }

    wcda->unixdev = open(dev, O_RDONLY | O_NONBLOCK, 0);
    if (wcda->unixdev == -1) {
	WARN("can't open '%s'!.  errno=%d\n", dev, errno);
	return -1;
    }
    wcda->cdaMode = WINE_CDA_OPEN;	/* to force reading tracks info */
    wcda->nCurTrack = 0;
    wcda->nTracks = 0;
    wcda->dwFirstFrame = 0;
    wcda->dwLastFrame = 0;
    wcda->lpdwTrackLen = NULL;
    wcda->lpdwTrackPos = NULL;
    wcda->lpbTrackFlags = NULL;
    return 0;
}

/**************************************************************************
 * 				CDAUDIO_GetMediaType		[internal]
 */
int	CDAUDIO_GetMediaType(WINE_CDAUDIO* wcda)
{
#ifdef linux
    return ioctl(wcda->unixdev, CDROM_DISC_STATUS);
#else
    return -1;
#endif
}

/**************************************************************************
 * 				CDAUDIO_Close			[internal]
 */
int	CDAUDIO_Close(WINE_CDAUDIO* wcda)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    if (wcda->lpdwTrackLen != NULL) free(wcda->lpdwTrackLen);
    if (wcda->lpdwTrackPos != NULL) free(wcda->lpdwTrackPos);
    if (wcda->lpbTrackFlags != NULL) free(wcda->lpbTrackFlags);
    close(wcda->unixdev);
    return 0;
#else
    return -1;
#endif
}

/**************************************************************************
 * 				CDAUDIO_GetNumberOfTracks	[internal]
 */
UINT16 CDAUDIO_GetNumberOfTracks(WINE_CDAUDIO* wcda)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
#ifdef linux
    struct cdrom_tochdr		hdr;
#else
    struct ioc_toc_header	hdr;
#endif
    
    if (wcda->nTracks == 0) {
#ifdef linux
	if (ioctl(wcda->unixdev, CDROMREADTOCHDR, &hdr))
#else
	if (ioctl(wcda->unixdev, CDIOREADTOCHEADER, &hdr))
#endif
	{
	    WARN("(%p) -- Error occured (%d)!\n", wcda, errno);
	    return (WORD)-1;
	}
#ifdef linux
	wcda->nFirstTrack = hdr.cdth_trk0;
	wcda->nLastTrack  = hdr.cdth_trk1;
#else	
	wcda->nFirstTrack = hdr.starting_track;
	wcda->nLastTrack  = hdr.ending_track;
#endif
	wcda->nTracks = wcda->nLastTrack - wcda->nFirstTrack + 1;
    }
    return wcda->nTracks;
#else
    return (WORD)-1;
#endif
}

/**************************************************************************
 * 			CDAUDIO_GetTracksInfo			[internal]
 */
BOOL CDAUDIO_GetTracksInfo(WINE_CDAUDIO* wcda)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int		i, length;
    int		start, last_start = 0;
    int		total_length = 0;
#ifdef linux
    struct cdrom_tocentry	entry;
#else
    struct ioc_read_toc_entry	entry;
    struct cd_toc_entry         toc_buffer;
#endif
    
    if (wcda->nTracks == 0) {
	if (CDAUDIO_GetNumberOfTracks(wcda) == (WORD)-1) return FALSE;
    }
    TRACE("nTracks=%u\n", wcda->nTracks);
    
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
	wcda->lpbTrackFlags == NULL) {
	WARN("error allocating track table !\n");
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
	if (ioctl(wcda->unixdev, CDROMREADTOCENTRY, &entry))
#else
	if (ioctl(wcda->unixdev, CDIOREADTOCENTRYS, &entry))
#endif
	{
	    WARN("error read entry (%d)\n", errno);
	    /* update status according to new status */
	    CDAUDIO_GetCDStatus(wcda);

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
	    wcda->dwFirstFrame = start;
	    TRACE("dwFirstOffset=%u\n", start);
	} else {
	    length = start - last_start;
	    last_start = start;
	    start = last_start - length;
	    total_length += length;
	    wcda->lpdwTrackLen[i - 1] = length;
	    wcda->lpdwTrackPos[i - 1] = start;
	    TRACE("track #%u start=%u len=%u\n", i, start, length);
	}
#ifdef linux
	wcda->lpbTrackFlags[i] =
	    (entry.cdte_adr << 4) | (entry.cdte_ctrl & 0x0f);
#else
	wcda->lpbTrackFlags[i] =
	    (toc_buffer.addr_type << 4) | (toc_buffer.control & 0x0f);
#endif 
	TRACE("track #%u flags=%02x\n", i + 1, wcda->lpbTrackFlags[i]);
    }
    wcda->dwLastFrame = last_start;
    TRACE("total_len=%u\n", total_length);
    return TRUE;
#else
    return FALSE;
#endif
}

/**************************************************************************
 * 				CDAUDIO_GetCDStatus		[internal]
 */
BOOL CDAUDIO_GetCDStatus(WINE_CDAUDIO* wcda)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int		oldmode = wcda->cdaMode;
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
    if (ioctl(wcda->unixdev, CDROMSUBCHNL, &wcda->sc))
#else
    if (ioctl(wcda->unixdev, CDIOCREADSUBCHANNEL, &read_sc))
#endif
    {
	TRACE("opened or no_media (%d)!\n", errno);
	wcda->cdaMode = WINE_CDA_OPEN; /* was NOT_READY */
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
	WARN("device doesn't support status.\n");
	wcda->cdaMode = WINE_CDA_DONTKNOW;
	break;
#ifdef linux
    case CDROM_AUDIO_NO_STATUS: 
#else
    case CD_AS_NO_STATUS:
#endif
	wcda->cdaMode = WINE_CDA_STOP;
	TRACE("WINE_CDA_STOP !\n");
	break;
#ifdef linux
    case CDROM_AUDIO_PLAY: 
#else
    case CD_AS_PLAY_IN_PROGRESS:
#endif
	wcda->cdaMode = WINE_CDA_PLAY;
	break;
#ifdef linux
    case CDROM_AUDIO_PAUSED:
#else
    case CD_AS_PLAY_PAUSED:
#endif
	wcda->cdaMode = WINE_CDA_PAUSE;
	TRACE("WINE_CDA_PAUSE !\n");
	break;
    default:
#ifdef linux
	TRACE("status=%02X !\n",
	      wcda->sc.cdsc_audiostatus);
#else
	TRACE("status=%02X !\n",
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
    TRACE("%02u-%02u:%02u:%02u \n",
	  wcda->sc.cdsc_trk,
	  wcda->sc.cdsc_absaddr.msf.minute,
	  wcda->sc.cdsc_absaddr.msf.second,
	  wcda->sc.cdsc_absaddr.msf.frame);
#else
    TRACE("%02u-%02u:%02u:%02u \n",
	  wcda->sc.what.position.track_number,
	  wcda->sc.what.position.absaddr.msf.minute,
	  wcda->sc.what.position.absaddr.msf.second,
	  wcda->sc.what.position.absaddr.msf.frame);
#endif
    
    if (oldmode != wcda->cdaMode && oldmode == WINE_CDA_OPEN) {
	if (!CDAUDIO_GetTracksInfo(wcda)) {
	    WARN("error updating TracksInfo !\n");
	    return FALSE;
	}
    }
    return TRUE;
#else
    return FALSE;
#endif
}

/**************************************************************************
 * 				CDAUDIO_Play			[internal]
 */
int	CDAUDIO_Play(WINE_CDAUDIO* wcda, DWORD start, DWORD end)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
#ifdef linux
    struct 	cdrom_msf	msf;
#else
    struct	ioc_play_msf	msf;
#endif

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
    if (ioctl(wcda->unixdev, CDROMSTART))
#else
    if (ioctl(wcda->unixdev, CDIOCSTART, NULL))
#endif
    {
	WARN("motor doesn't start !\n");
	return -1;
    }
#ifdef linux
    if (ioctl(wcda->unixdev, CDROMPLAYMSF, &msf))
#else
    if (ioctl(wcda->unixdev, CDIOCPLAYMSF, &msf))
#endif
    {
	WARN("device doesn't play !\n");
	return -1;
    }
#ifdef linux
    TRACE("msf = %d:%d:%d %d:%d:%d\n",
	  msf.cdmsf_min0, msf.cdmsf_sec0, msf.cdmsf_frame0,
	  msf.cdmsf_min1, msf.cdmsf_sec1, msf.cdmsf_frame1);
#else
    TRACE("msf = %d:%d:%d %d:%d:%d\n",
	  msf.start_m, msf.start_s, msf.start_f,
	  msf.end_m,   msf.end_s,   msf.end_f);
#endif
    return 0;
#else
    return -1;
#endif
}

/**************************************************************************
 * 				CDAUDIO_Stop			[internal]
 */
int	CDAUDIO_Stop(WINE_CDAUDIO* wcda)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int	ret = 0;
#ifdef linux
    ret = ioctl(wcda->unixdev, CDROMSTOP);
#else
    ret = ioctl(wcda->unixdev, CDIOCSTOP, NULL);
#endif
    return ret;
#else
    return -1;
#endif
}

/**************************************************************************
 * 				CDAUDIO_Pause			[internal]
 */
int	CDAUDIO_Pause(WINE_CDAUDIO* wcda, int pauseOn)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int	ret = 0;    
#ifdef linux
    ret = ioctl(wcda->unixdev, pauseOn ? CDROMPAUSE : CDROMRESUME);
#else
    ret = ioctl(wcda->unixdev, pauseOn ? CDIOCPAUSE : CDIOCRESUME, NULL);
#endif
    return ret;
#else
    return -1;
#endif
}

/**************************************************************************
 * 				CDAUDIO_Seek			[internal]
 */
int	CDAUDIO_Seek(WINE_CDAUDIO* wcda, DWORD at)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int				ret = 0;    
#ifdef linux
    struct cdrom_msf0		msf;
    msf.minute = at / CDFRAMES_PERMIN;
    msf.second = (at % CDFRAMES_PERMIN) / CDFRAMES_PERSEC;
    msf.frame  = at % CDFRAMES_PERSEC;

    ret = ioctl(wcda->unixdev, CDROMSEEK, &msf);
#else
   /* FIXME: the current end for play is lost 
    * use end of CD ROM instead
    */
   FIXME("Could a BSD expert implement the seek function ?\n");
   CDAUDIO_Play(wcda, at, wcda->lpdwTrackPos[wcda->nTracks] + wcda->lpdwTrackLen[wcda->nTracks]);
   
#endif
    return ret;
#else
    return -1;
#endif
}

/**************************************************************************
 * 				CDAUDIO_SetDoor			[internal]
 */
int	CDAUDIO_SetDoor(WINE_CDAUDIO* wcda, int open)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int	ret = 0;    
#ifdef linux
    if (open) {
	ret = ioctl(wcda->unixdev, CDROMEJECT);
    } else {
	ret = ioctl(wcda->unixdev, CDROMEJECT, 1);
    }
#else
    ret = (ioctl(wcda->unixdev, CDIOCALLOW, NULL)) || 
	(ioctl(wcda->unixdev, open ? CDIOCEJECT : CDIOCCLOSE, NULL)) ||
	(ioctl(wcda->unixdev, CDIOCPREVENT, NULL));
#endif
    wcda->nTracks = 0;
    return ret;
#else
    return -1;
#endif
}

/**************************************************************************
 * 				CDAUDIO_Reset			[internal]
 */
int	CDAUDIO_Reset(WINE_CDAUDIO* wcda) 
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int	ret = 0;
#ifdef linux
    ret = ioctl(wcda->unixdev, CDROMRESET);
#else
    ret = ioctl(wcda->unixdev, CDIOCRESET, NULL);
#endif
    return ret;
#else
    return -1;
#endif
}

