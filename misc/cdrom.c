/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Main file for CD-ROM support
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1999 Eric Pouech
 * Copyright 2000 Andreas Mohr
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "winnls.h"
#include "cdrom.h"
#include "drive.h"
#include "debugtools.h"
#include "winbase.h"

DEFAULT_DEBUG_CHANNEL(cdrom);

#define MAX_CDAUDIO_TRACKS 	256

#define CDROM_OPEN(wcda,parentdev) \
    (((parentdev) == -1) ? CDROM_OpenDev(wcda) : (parentdev))

#define CDROM_CLOSE(dev,parentdev) \
    (((parentdev) == -1) ? CDROM_CloseDev(dev) : 0)

/**************************************************************************
 * 				CDROM_Open			[internal]
 *
 * drive = 0, 1, ...
 *      or -1 (figure it out)
 */
int	CDROM_Open(WINE_CDAUDIO* wcda, int drive)
{
    int i, dev;
    BOOL avail = FALSE;

    if (drive == -1)
    {
        char root[] = "A:\\";
	for (i=0; i < MAX_DOS_DRIVES; i++, root[0]++)
            if (GetDriveTypeA(root) == DRIVE_CDROM)
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
    if ((wcda->devname = DRIVE_GetDevice(drive)) == NULL)
{
	WARN("No device entry for CD-ROM #%d (drive %c:) found !\n",
		drive, 'A' + drive);
	return -1;
    }

    /* Test whether device can be opened */
    dev = CDROM_OpenDev(wcda);
    if (dev == -1)
	return -1;
    else
        CDROM_CloseDev(dev);

    wcda->cdaMode = WINE_CDA_OPEN;	/* to force reading tracks info */
    wcda->nCurTrack = 0;
    wcda->nTracks = 0;
    wcda->dwFirstFrame = 0;
    wcda->dwLastFrame = 0;
    wcda->lpdwTrackLen = NULL;
    wcda->lpdwTrackPos = NULL;
    wcda->lpbTrackFlags = NULL;
    TRACE("opened drive %c: (device %s)\n", 'A' + drive, wcda->devname);
    return 0;
}

/**************************************************************************
 * 				CDROM_OpenDev			[internal]
 *
 */
int	CDROM_OpenDev(WINE_CDAUDIO* wcda)
{
    int dev = open(wcda->devname, O_RDONLY | O_NONBLOCK, 0);
    if (dev == -1)
	WARN("can't open device '%s'! (%s)\n", wcda->devname, strerror(errno));

    TRACE("-> %d\n", dev);
    return dev;
}

/**************************************************************************
 * 				CDROM_GetMediaType		[internal]
 */
int	CDROM_GetMediaType(WINE_CDAUDIO* wcda, int parentdev)
{
    int type = -1;
#ifdef linux
    int dev = CDROM_OPEN( wcda, parentdev );
    type = ioctl(dev, CDROM_DISC_STATUS);
    CDROM_CLOSE( dev, parentdev );
#endif
    TRACE("-> %d\n", type);
    return type;
}

/**************************************************************************
 * 				CDROM_Close			[internal]
 */
int	CDROM_CloseDev(int dev)
{
    TRACE("%d\n", dev);
    return close(dev);
}

/**************************************************************************
 * 				CDROM_Close			[internal]
 */
int	CDROM_Close(WINE_CDAUDIO* wcda)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    if (wcda->lpdwTrackLen != NULL) free(wcda->lpdwTrackLen);
    if (wcda->lpdwTrackPos != NULL) free(wcda->lpdwTrackPos);
    if (wcda->lpbTrackFlags != NULL) free(wcda->lpbTrackFlags);
    TRACE("%s\n", wcda->devname);
    return 0;
#else
    return -1;
#endif
}

/**************************************************************************
 *				CDROM_Get_UPC			[internal]
 *
 * upc has to be 14 bytes long
 */
int CDROM_Get_UPC(WINE_CDAUDIO* wcda, LPSTR upc, int parentdev)
{
#ifdef linux
    struct cdrom_mcn mcn;
    int dev = CDROM_OPEN( wcda, parentdev );
    int status = ioctl(dev, CDROM_GET_MCN, &mcn);
    CDROM_CLOSE( dev, parentdev );
    if (status)
{
	ERR("ioctl() failed with code %d\n",status);
	return -1;
    }
    strcpy(upc, mcn.medium_catalog_number);
    return 0;
#else
    return -1;
#endif
}

/**************************************************************************
 * 				CDROM_Audio_GetNumberOfTracks	[internal]
 */
UINT16 CDROM_Audio_GetNumberOfTracks(WINE_CDAUDIO* wcda, int parentdev)
{
    UINT16 ret = (UINT16)-1;
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
#ifdef linux
    struct cdrom_tochdr		hdr;
#else
    struct ioc_toc_header	hdr;
#endif
    int dev = CDROM_OPEN( wcda, parentdev );

    if (wcda->nTracks == 0) {
#ifdef linux
	if (ioctl(dev, CDROMREADTOCHDR, &hdr))
#else
	if (ioctl(dev, CDIOREADTOCHEADER, &hdr))
#endif
	{
	    WARN("(%p) -- Error occurred (%s)!\n", wcda, strerror(errno));
	    goto end;
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
    ret = wcda->nTracks;
end:
    CDROM_CLOSE( dev, parentdev );
#endif
    return ret;
}

/**************************************************************************
 * 			CDROM_Audio_GetTracksInfo		[internal]
 */
BOOL CDROM_Audio_GetTracksInfo(WINE_CDAUDIO* wcda, int parentdev)
{
    BOOL ret = FALSE;
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
    int dev = CDROM_OPEN( wcda, parentdev );
    
    if (wcda->nTracks == 0) {
	if (CDROM_Audio_GetNumberOfTracks(wcda, dev) == (WORD)-1)
	goto end;
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
	goto end;
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
	entry.starting_track = LEADOUT; /* FIXME */
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
	memset((char *)&toc_buffer, 0, sizeof(toc_buffer));
	entry.address_format = CD_MSF_FORMAT;
	entry.data_len = sizeof(toc_buffer);
	entry.data = &toc_buffer;
#endif
#ifdef linux
	if (ioctl(dev, CDROMREADTOCENTRY, &entry))
#else
	if (ioctl(dev, CDIOREADTOCENTRYS, &entry))
#endif
	{
	    WARN("error read entry (%s)\n", strerror(errno));
	    /* update status according to new status */
	    CDROM_Audio_GetCDStatus(wcda, dev);

	    goto end;
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
    ret = TRUE;
end:
    CDROM_CLOSE( dev, parentdev );
#endif
    return ret;
}

/**************************************************************************
 * 				CDROM_Audio_GetCDStatus		[internal]
 */
BOOL CDROM_Audio_GetCDStatus(WINE_CDAUDIO* wcda, int parentdev)
{
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int oldmode = wcda->cdaMode;
    int ret = FALSE;
    int dev = CDROM_OPEN( wcda, parentdev );
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
    if (ioctl(dev, CDROMSUBCHNL, &wcda->sc))
#else
    if (ioctl(dev, CDIOCREADSUBCHANNEL, &read_sc))
#endif
    {
	TRACE("opened or no_media (%s)!\n", strerror(errno));
	wcda->cdaMode = WINE_CDA_OPEN; /* was NOT_READY */
	goto end;
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
	/* seems that this means stop for ide drives */
	wcda->cdaMode = WINE_CDA_STOP;
	TRACE("AUDIO_INVALID -> WINE_CDA_STOP\n");
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
    TRACE("%02u-%02u:%02u:%02u\n",
	  wcda->sc.cdsc_trk,
	  wcda->sc.cdsc_absaddr.msf.minute,
	  wcda->sc.cdsc_absaddr.msf.second,
	  wcda->sc.cdsc_absaddr.msf.frame);
#else
    TRACE("%02u-%02u:%02u:%02u\n",
	  wcda->sc.what.position.track_number,
	  wcda->sc.what.position.absaddr.msf.minute,
	  wcda->sc.what.position.absaddr.msf.second,
	  wcda->sc.what.position.absaddr.msf.frame);
#endif
    
    if (oldmode != wcda->cdaMode && oldmode == WINE_CDA_OPEN) {
	if (!CDROM_Audio_GetTracksInfo(wcda, dev)) {
	    WARN("error updating TracksInfo !\n");
	    goto end;
	}
    }
    if (wcda->cdaMode != WINE_CDA_OPEN)
        ret = TRUE;
end:
    CDROM_CLOSE( dev, parentdev );
    return ret;
#else
    return FALSE;
#endif
}

/**************************************************************************
 * 				CDROM_Audio_Play		[internal]
 */
int	CDROM_Audio_Play(WINE_CDAUDIO* wcda, DWORD start, DWORD end, int parentdev)
{
    int ret = -1;
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
#ifdef linux
    struct 	cdrom_msf	msf;
#else
    struct	ioc_play_msf	msf;
#endif
    int dev = CDROM_OPEN( wcda, parentdev );

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
    if (ioctl(dev, CDROMSTART))
#else
    if (ioctl(dev, CDIOCSTART, NULL))
#endif
    {
	WARN("motor doesn't start !\n");
	goto end;
    }
#ifdef linux
    if (ioctl(dev, CDROMPLAYMSF, &msf))
#else
    if (ioctl(dev, CDIOCPLAYMSF, &msf))
#endif
    {
	WARN("device doesn't play !\n");
	goto end;
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
    ret = 0;
end:
    CDROM_CLOSE( dev, parentdev );
#endif
    return ret;
}

/**************************************************************************
 * 				CDROM_Audio_Stop		[internal]
 */
int	CDROM_Audio_Stop(WINE_CDAUDIO* wcda, int parentdev)
{
    int	ret = -1;
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int dev = CDROM_OPEN( wcda, parentdev );
#ifdef linux
    ret = ioctl(dev, CDROMSTOP);
#else
    ret = ioctl(dev, CDIOCSTOP, NULL);
#endif
    CDROM_CLOSE( dev, parentdev );
#endif
    return ret;
}

/**************************************************************************
 * 				CDROM_Audio_Pause		[internal]
 */
int	CDROM_Audio_Pause(WINE_CDAUDIO* wcda, int pauseOn, int parentdev)
{
    int	ret = -1;
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int dev = CDROM_OPEN( wcda, parentdev );
#ifdef linux
    ret = ioctl(dev, pauseOn ? CDROMPAUSE : CDROMRESUME);
#else
    ret = ioctl(dev, pauseOn ? CDIOCPAUSE : CDIOCRESUME, NULL);
#endif
    CDROM_CLOSE( dev, parentdev );
#endif
    return ret;
}

/**************************************************************************
 * 				CDROM_Audio_Seek		[internal]
 */
int	CDROM_Audio_Seek(WINE_CDAUDIO* wcda, DWORD at, int parentdev)
{
    int	ret = -1;
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int dev = CDROM_OPEN( wcda, parentdev );
#ifdef linux
    struct cdrom_msf0		msf;
    msf.minute = at / CDFRAMES_PERMIN;
    msf.second = (at % CDFRAMES_PERMIN) / CDFRAMES_PERSEC;
    msf.frame  = at % CDFRAMES_PERSEC;

    ret = ioctl(dev, CDROMSEEK, &msf);
#else
   /* FIXME: the current end for play is lost 
    * use end of CD ROM instead
    */
   FIXME("Could a BSD expert implement the seek function ?\n");
   CDROM_Audio_Play(wcda, at, wcda->lpdwTrackPos[wcda->nTracks] + wcda->lpdwTrackLen[wcda->nTracks], dev);
#endif
    CDROM_CLOSE( dev, parentdev );
#endif
    return ret;
}

/**************************************************************************
 * 				CDROM_SetDoor			[internal]
 */
int	CDROM_SetDoor(WINE_CDAUDIO* wcda, int open, int parentdev)
{
    int	ret = -1;
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int dev = CDROM_OPEN( wcda, parentdev );

    TRACE("%d\n", open);
#ifdef linux
    if (open) {
	ret = ioctl(dev, CDROMEJECT);
    } else {
 	ret = ioctl(dev, CDROMCLOSETRAY);
    }
#else
    ret = (ioctl(dev, CDIOCALLOW, NULL)) || 
	(ioctl(dev, open ? CDIOCEJECT : CDIOCCLOSE, NULL)) ||
	(ioctl(dev, CDIOCPREVENT, NULL));
#endif
    wcda->nTracks = 0;
    if (ret == -1)
        WARN("failed (%s)\n", strerror(errno));
    CDROM_CLOSE( dev, parentdev );
#endif
    return ret;
}

/**************************************************************************
 * 				CDROM_Reset			[internal]
 */
int	CDROM_Reset(WINE_CDAUDIO* wcda, int parentdev)
{
    int	ret = -1;
#if defined(linux) || defined(__FreeBSD__) || defined(__NetBSD__)
    int dev = CDROM_OPEN( wcda, parentdev );
#ifdef linux
    ret = ioctl(dev, CDROMRESET);
#else
    ret = ioctl(dev, CDIOCRESET, NULL);
#endif
    CDROM_CLOSE( dev, parentdev );
#endif
    return ret;
}

/**************************************************************************
 *                             CDROM_Data_FindBestVoldesc       [internal]
 */
WORD CDROM_Data_FindBestVoldesc(int fd)
{
    BYTE cur_vd_type, max_vd_type = 0;
    unsigned int offs, best_offs=0, extra_offs = 0;
    char sig[3];


    for (offs=0x8000; offs <= 0x9800; offs += 0x800)
    {
	/* if 'CDROM' occurs at position 8, this is a pre-iso9660 cd, and 
	 * the volume label is displaced forward by 8
	 */
	lseek(fd, offs+11, SEEK_SET); /* check for non-ISO9660 signature */
	read(fd, &sig, 3);
	if ((sig[0]=='R')&&(sig[1]=='O')&&(sig[2]=='M'))
	{
	    extra_offs=8;
	}
        lseek(fd, offs+extra_offs, SEEK_SET);
        read(fd, &cur_vd_type, 1);
        if (cur_vd_type == 0xff) /* voldesc set terminator */
            break;
        if (cur_vd_type > max_vd_type)
        {
            max_vd_type = cur_vd_type;
            best_offs = offs + extra_offs;
        }
    }
    return best_offs;
}

/**************************************************************************
 *				CDROM_Audio_GetSerial		[internal]
 */
DWORD CDROM_Audio_GetSerial(WINE_CDAUDIO* wcda)
{
    unsigned long serial = 0;
    int i;
    DWORD dwFrame, msf;
    WORD wMinutes, wSeconds, wFrames;
	WORD wMagic;
	DWORD dwStart, dwEnd;

	/*
	 * wMagic collects the wFrames from track 1
	 * dwStart, dwEnd collect the beginning and end of the disc respectively, in
	 * frames.
	 * There it is collected for correcting the serial when there are less than
	 * 3 tracks.
	 */
	wMagic = 0;
	dwStart = dwEnd = 0;

    for (i = 0; i < wcda->nTracks; i++) {
	dwFrame = wcda->lpdwTrackPos[i];
	wMinutes = dwFrame / CDFRAMES_PERMIN;
	wSeconds = (dwFrame - CDFRAMES_PERMIN * wMinutes) / CDFRAMES_PERSEC;
	wFrames = dwFrame - CDFRAMES_PERMIN * wMinutes - CDFRAMES_PERSEC * wSeconds;
	msf = CDROM_MAKE_MSF(wMinutes, wSeconds, wFrames);

	serial += (CDROM_MSF_MINUTE(msf) << 16) +
	(CDROM_MSF_SECOND(msf) << 8) +
	(CDROM_MSF_FRAME(msf));

	if (i==0)
	{
		wMagic = wFrames;
		dwStart = dwFrame;
	}
	dwEnd = dwFrame + wcda->lpdwTrackLen[i];
	
	}

	if (wcda->nTracks < 3)
	{
		serial += wMagic + (dwEnd - dwStart);
	}
    return serial;
}

/**************************************************************************
 *				CDROM_Data_GetSerial		[internal]
 */
DWORD CDROM_Data_GetSerial(WINE_CDAUDIO* wcda, int parentdev)
{
    int dev = CDROM_OPEN( wcda, parentdev );
    WORD offs = CDROM_Data_FindBestVoldesc(dev);
    union {
	unsigned long val;
	unsigned char p[4];
    } serial;
    BYTE b0 = 0, b1 = 1, b2 = 2, b3 = 3;
    
    serial.val = 0;
    if (offs)
    {
	BYTE buf[2048];
	OSVERSIONINFOA ovi;
	int i;

	lseek(dev,offs,SEEK_SET);
	read(dev,buf,2048);
	/*
	 * OK, another braindead one... argh. Just believe it.
	 * Me$$ysoft chose to reverse the serial number in NT4/W2K.
	 * It's true and nobody will ever be able to change it.
	 */
	ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	GetVersionExA(&ovi);
	if ((ovi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	&&  (ovi.dwMajorVersion >= 4))
	{
	    b0 = 3; b1 = 2; b2 = 1; b3 = 0;
	}
	for(i=0; i<2048; i+=4)
	{
	    /* DON'T optimize this into DWORD !! (breaks overflow) */
	    serial.p[b0] += buf[i+b0];
	    serial.p[b1] += buf[i+b1];
	    serial.p[b2] += buf[i+b2];
	    serial.p[b3] += buf[i+b3];
	}
    }
    CDROM_CLOSE( dev, parentdev );
    return serial.val;
}

/**************************************************************************
 *				CDROM_GetSerial			[internal]
 */
DWORD CDROM_GetSerial(int drive)
{
    WINE_CDAUDIO wcda;
    DWORD serial = 0;

    /* EXPIRES 01.01.2002 */
    WARN("CD-ROM serial number calculation might fail.\n");
    WARN("Please test with as many exotic CDs as possible !\n");

    if (!(CDROM_Open(&wcda, drive)))
    {
	int dev = CDROM_OpenDev(&wcda);
	int media = CDROM_GetMediaType(&wcda, dev);

	switch (media)
	{
	    case CDS_AUDIO:
	    case CDS_MIXED: /* mixed is basically a mountable audio CD */
		if (!(CDROM_Audio_GetCDStatus(&wcda, dev))) {
		    ERR("couldn't get CD status !\n");
		    goto end;
		}
		serial = CDROM_Audio_GetSerial(&wcda);
		break;
	    case CDS_DATA_1:
	    case CDS_DATA_2:
	    case CDS_XA_2_1:
	    case CDS_XA_2_2:
	    case -1: /* ioctl() error: ISO9660 image file given ? */
		/* hopefully a data CD */
		serial = CDROM_Data_GetSerial(&wcda, dev);
		break;
	    default:
		WARN("Strange CD type (%d) or empty ?\n", media);
	}
	if (serial)
	    TRACE("CD serial number is %04x-%04x.\n",
		HIWORD(serial), LOWORD(serial));
	else
	    if (media >= CDS_AUDIO)
		ERR("couldn't get CD serial !\n");
end:
	CDROM_CloseDev(dev);
	CDROM_Close(&wcda);
    }
    return serial;
}

/**************************************************************************
 *				CDROM_Data_GetLabel		[internal]
 */
DWORD CDROM_Data_GetLabel(WINE_CDAUDIO* wcda, char *label, int parentdev)
{
#define LABEL_LEN	32+1
    int dev = CDROM_OPEN( wcda, parentdev );
    WORD offs = CDROM_Data_FindBestVoldesc(dev);
    WCHAR label_read[LABEL_LEN]; /* Unicode possible, too */
    DWORD unicode_id = 0;

    if (offs)
    {
	if ((lseek(dev, offs+0x58, SEEK_SET) == offs+0x58)
	&&  (read(dev, &unicode_id, 3) == 3))
	{
	    int ver = (unicode_id & 0xff0000) >> 16;

	    if ((lseek(dev, offs+0x28, SEEK_SET) != offs+0x28)
	    ||  (read(dev, &label_read, LABEL_LEN) != LABEL_LEN))
		goto failure;

	    CDROM_CLOSE( dev, parentdev );
	    if ((LOWORD(unicode_id) == 0x2f25) /* Unicode ID */
	    &&  ((ver == 0x40) || (ver == 0x43) || (ver == 0x45)))
	    { /* yippee, unicode */
		int i;
		WORD ch;
		for (i=0; i<LABEL_LEN;i++)
		{ /* Motorola -> Intel Unicode conversion :-\ */
		     ch = label_read[i];
		     label_read[i] = (ch << 8) | (ch >> 8);
		}
                WideCharToMultiByte( CP_ACP, 0, label_read, -1, label, 12, NULL, NULL );
                label[11] = 0;
	    }
	    else
	    {
		strncpy(label, (LPSTR)label_read, 11);
		label[11] = '\0';
	    }
	    return 1;
	}
    }
failure:
    CDROM_CLOSE( dev, parentdev );
    ERR("error reading label !\n");
    return 0;
}

/**************************************************************************
 *				CDROM_GetLabel			[internal]
 */
DWORD CDROM_GetLabel(int drive, char *label)
{
    WINE_CDAUDIO wcda;
    DWORD ret = 1;

    if (!(CDROM_Open(&wcda, drive)))
    {
	int dev = CDROM_OpenDev(&wcda);
	int media = CDROM_GetMediaType(&wcda, dev);
	LPSTR cdname = NULL;

	switch (media)
	{
	    case CDS_AUDIO:
		cdname = "Audio";
		strcpy(label, "Audio CD   ");
		break;

	    case CDS_DATA_1: /* fall through for all data CD types !! */
		if (!cdname) cdname = "Data_1";
	    case CDS_DATA_2:
		if (!cdname) cdname = "Data_2";
	    case CDS_XA_2_1:
		if (!cdname) cdname = "XA 2.1";
	    case CDS_XA_2_2:
		if (!cdname) cdname = "XA 2.2";
	    case CDS_MIXED:
		if (!cdname) cdname = "Mixed mode";
	    case -1:
		if (!cdname) cdname = "Unknown/ISO file";

		/* common code *here* !! */
		/* hopefully a data CD */
		if (!CDROM_Data_GetLabel(&wcda, label, dev))
			ret = 0;
		break;

	    case CDS_NO_INFO:
		if (!cdname) cdname = "No_info";
		ret = 0;
		break;

	    default:
                WARN("Strange CD type (%d) or empty ?\n", media);
		cdname = "Strange/empty";
	        ret = 0;
		break;
	}
	
	CDROM_CloseDev(dev);
	CDROM_Close(&wcda);
	TRACE("%s CD: label is '%s'.\n",
	    cdname, label);
    }
    else
        ret = 0;

    return ret;
}

