/*
 * MCI driver for audio CD (MCICDA)
 *
 * Copyright 1994    Martin Ayotte
 * Copyright 1998-99 Eric Pouech
 * Copyright 2000    Andreas Mohr
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wownt32.h"
#include "mmddk.h"
#include "winioctl.h"
#include "ntddcdrm.h"
#include "winternl.h"
#include "wine/debug.h"
#include "dsound.h"

WINE_DEFAULT_DEBUG_CHANNEL(mcicda);

#define CDFRAMES_PERSEC                 75
#define CDFRAMES_PERMIN                 (CDFRAMES_PERSEC * 60)
#define FRAME_OF_ADDR(a) ((a)[1] * CDFRAMES_PERMIN + (a)[2] * CDFRAMES_PERSEC + (a)[3])
#define FRAME_OF_TOC(toc, idx)  FRAME_OF_ADDR((toc).TrackData[idx - (toc).FirstTrack].Address)

/* Defined by red-book standard; do not change! */
#define RAW_SECTOR_SIZE  (2352)

/* Must be >= RAW_SECTOR_SIZE */
#define CDDA_FRAG_SIZE   (32768)
/* Must be >= 2 */
#define CDDA_FRAG_COUNT  (3)

typedef struct {
    UINT		wDevID;
    int     		nUseCount;          /* Incremented for each shared open */
    BOOL  		fShareable;         /* TRUE if first open was shareable */
    MCIDEVICEID		wNotifyDeviceID;    /* MCI device ID with a pending notification */
    HANDLE 		hCallback;          /* Callback handle for pending notification */
    DWORD		dwTimeFormat;
    HANDLE              handle;

    /* The following are used for digital playback only */
    HANDLE hThread;
    HANDLE stopEvent;
    DWORD start, end;

    IDirectSound *dsObj;
    IDirectSoundBuffer *dsBuf;

    CRITICAL_SECTION cs;
} WINE_MCICDAUDIO;

/*-----------------------------------------------------------------------*/

typedef HRESULT(WINAPI*LPDIRECTSOUNDCREATE)(LPCGUID,LPDIRECTSOUND*,LPUNKNOWN);
static LPDIRECTSOUNDCREATE pDirectSoundCreate;

static BOOL device_io(HANDLE dev, DWORD code, void *inbuffer, DWORD insize, void *outbuffer, DWORD outsize, DWORD *retsize, OVERLAPPED *overlapped)
{
    const char *str;
    BOOL ret = DeviceIoControl(dev, code, inbuffer, insize, outbuffer, outsize, retsize, overlapped);

#define XX(x) case (x): str = #x; break
    switch (code)
    {
        XX(IOCTL_CDROM_RAW_READ);
        XX(IOCTL_CDROM_READ_TOC);
        XX(IOCTL_CDROM_READ_Q_CHANNEL);
        XX(IOCTL_CDROM_SEEK_AUDIO_MSF);
        XX(IOCTL_CDROM_PLAY_AUDIO_MSF);
        XX(IOCTL_CDROM_STOP_AUDIO);
        XX(IOCTL_CDROM_PAUSE_AUDIO);
        XX(IOCTL_CDROM_RESUME_AUDIO);
        XX(IOCTL_STORAGE_EJECT_MEDIA);
        XX(IOCTL_STORAGE_LOAD_MEDIA);
        default: str = wine_dbg_sprintf("UNKNOWN (0x%lx)", code);
    };
#undef XX
    TRACE("Device %p, Code %s -> Return %d, Bytes %lu\n", dev, str, ret, *retsize);
    return ret;
}

static DWORD CALLBACK MCICDA_playLoop(void *ptr)
{
    WINE_MCICDAUDIO *wmcda = (WINE_MCICDAUDIO*)ptr;
    DWORD lastPos, curPos, endPos, br;
    void *cdData;
    DWORD lockLen, fragLen;
    DSBCAPS caps;
    RAW_READ_INFO rdInfo;
    HRESULT hr = DS_OK;

    memset(&caps, 0, sizeof(caps));
    caps.dwSize = sizeof(caps);
    hr = IDirectSoundBuffer_GetCaps(wmcda->dsBuf, &caps);

    fragLen = caps.dwBufferBytes/CDDA_FRAG_COUNT;
    curPos = lastPos = 0;
    endPos = ~0u;
    while (SUCCEEDED(hr) && endPos != lastPos &&
           WaitForSingleObject(wmcda->stopEvent, 0) != WAIT_OBJECT_0) {
        hr = IDirectSoundBuffer_GetCurrentPosition(wmcda->dsBuf, &curPos, NULL);
        if ((curPos-lastPos+caps.dwBufferBytes)%caps.dwBufferBytes < fragLen) {
            Sleep(1);
            continue;
        }

        EnterCriticalSection(&wmcda->cs);
        rdInfo.DiskOffset.QuadPart = wmcda->start<<11;
        rdInfo.SectorCount = min(fragLen/RAW_SECTOR_SIZE, wmcda->end-wmcda->start);
        rdInfo.TrackMode = CDDA;

        hr = IDirectSoundBuffer_Lock(wmcda->dsBuf, lastPos, fragLen, &cdData, &lockLen, NULL, NULL, 0);
        if (hr == DSERR_BUFFERLOST) {
            if(FAILED(IDirectSoundBuffer_Restore(wmcda->dsBuf)) ||
               FAILED(IDirectSoundBuffer_Play(wmcda->dsBuf, 0, 0, DSBPLAY_LOOPING))) {
                LeaveCriticalSection(&wmcda->cs);
                break;
            }
            hr = IDirectSoundBuffer_Lock(wmcda->dsBuf, lastPos, fragLen, &cdData, &lockLen, NULL, NULL, 0);
        }

        if (SUCCEEDED(hr)) {
            if (rdInfo.SectorCount > 0) {
                if (!device_io(wmcda->handle, IOCTL_CDROM_RAW_READ, &rdInfo, sizeof(rdInfo), cdData, lockLen, &br, NULL))
                    WARN("CD read failed at sector %ld: 0x%lx\n", wmcda->start, GetLastError());
            }
            if (rdInfo.SectorCount*RAW_SECTOR_SIZE < lockLen) {
                if(endPos == ~0u) endPos = lastPos;
                memset((BYTE*)cdData + rdInfo.SectorCount*RAW_SECTOR_SIZE, 0,
                       lockLen - rdInfo.SectorCount*RAW_SECTOR_SIZE);
            }
            hr = IDirectSoundBuffer_Unlock(wmcda->dsBuf, cdData, lockLen, NULL, 0);
        }

        lastPos += fragLen;
        lastPos %= caps.dwBufferBytes;
        wmcda->start += rdInfo.SectorCount;

        LeaveCriticalSection(&wmcda->cs);
    }
    IDirectSoundBuffer_Stop(wmcda->dsBuf);
    SetEvent(wmcda->stopEvent);

    /* A design bug in native: the independent CD player called by the
     * MCI has no means to signal end of playing, therefore the MCI
     * notification is left hanging.  MCI_NOTIFY_SUPERSEDED will be
     * signaled by the next command that has MCI_NOTIFY set (or
     * MCI_NOTIFY_ABORTED for MCI_PLAY). */

    return 0;
}



/**************************************************************************
 * 				MCICDA_drvOpen			[internal]
 */
static	DWORD	MCICDA_drvOpen(LPCWSTR str, LPMCI_OPEN_DRIVER_PARMSW modp)
{
    static HMODULE dsHandle;
    WINE_MCICDAUDIO*	wmcda;

    if (!modp) return 0xFFFFFFFF;
    /* FIXME: MCIERR_CANNOT_LOAD_DRIVER if there's no drive of type CD-ROM */

    wmcda = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINE_MCICDAUDIO));

    if (!wmcda)
	return 0;

    if (!dsHandle) {
        dsHandle = LoadLibraryA("dsound.dll");
        if(dsHandle)
            pDirectSoundCreate = (LPDIRECTSOUNDCREATE)GetProcAddress(dsHandle, "DirectSoundCreate");
    }

    wmcda->wDevID = modp->wDeviceID;
    mciSetDriverData(wmcda->wDevID, (DWORD_PTR)wmcda);
    modp->wCustomCommandTable = MCI_NO_COMMAND_TABLE;
    modp->wType = MCI_DEVTYPE_CD_AUDIO;
    InitializeCriticalSectionEx(&wmcda->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    wmcda->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": WINE_MCICDAUDIO.cs");
    return modp->wDeviceID;
}

/**************************************************************************
 * 				MCICDA_drvClose			[internal]
 */
static	DWORD	MCICDA_drvClose(DWORD dwDevID)
{
    WINE_MCICDAUDIO*  wmcda = (WINE_MCICDAUDIO*)mciGetDriverData(dwDevID);

    if (wmcda) {
	wmcda->cs.DebugInfo->Spare[0] = 0;
	DeleteCriticalSection(&wmcda->cs);
	HeapFree(GetProcessHeap(), 0, wmcda);
	mciSetDriverData(dwDevID, 0);
    }
    return (dwDevID == 0xFFFFFFFF) ? 1 : 0;
}

/**************************************************************************
 * 				MCICDA_GetOpenDrv		[internal]
 */
static WINE_MCICDAUDIO*  MCICDA_GetOpenDrv(UINT wDevID)
{
    WINE_MCICDAUDIO*	wmcda = (WINE_MCICDAUDIO*)mciGetDriverData(wDevID);

    if (wmcda == NULL || wmcda->nUseCount == 0) {
	WARN("Invalid wDevID=%u\n", wDevID);
	return 0;
    }
    return wmcda;
}

/**************************************************************************
 *				MCICDA_mciNotify		[internal]
 *
 * Notifications in MCI work like a 1-element queue.
 * Each new notification request supersedes the previous one.
 */
static void MCICDA_Notify(DWORD_PTR hWndCallBack, WINE_MCICDAUDIO* wmcda, UINT wStatus)
{
    MCIDEVICEID wDevID = wmcda->wNotifyDeviceID;
    HANDLE old = InterlockedExchangePointer(&wmcda->hCallback, NULL);
    if (old) mciDriverNotify(old, wDevID, MCI_NOTIFY_SUPERSEDED);
    mciDriverNotify(HWND_32(LOWORD(hWndCallBack)), wDevID, wStatus);
}

/**************************************************************************
 * 				MCICDA_ReadTOC		[internal]
 */
static BOOL MCICDA_ReadTOC(WINE_MCICDAUDIO* wmcda, CDROM_TOC *toc, DWORD *br)
{
    if (!device_io(wmcda->handle, IOCTL_CDROM_READ_TOC, NULL, 0, toc, sizeof(*toc), br, NULL)) {
        WARN("error reading TOC !\n");
        return FALSE;
    }
    return TRUE;
}

/**************************************************************************
 * 				MCICDA_GetStatus		[internal]
 */
static	DWORD    MCICDA_GetStatus(WINE_MCICDAUDIO* wmcda)
{
    CDROM_SUB_Q_DATA_FORMAT     fmt;
    SUB_Q_CHANNEL_DATA          data;
    DWORD                       br;
    DWORD                       mode = MCI_MODE_NOT_READY;

    fmt.Format = IOCTL_CDROM_CURRENT_POSITION;
    if(wmcda->hThread != 0) {
        DWORD status;
        HRESULT hr;

        hr = IDirectSoundBuffer_GetStatus(wmcda->dsBuf, &status);
        if(SUCCEEDED(hr)) {
            if(!(status&DSBSTATUS_PLAYING)) {
                if(WaitForSingleObject(wmcda->stopEvent, 0) == WAIT_OBJECT_0)
                    mode = MCI_MODE_STOP;
                else
                    mode = MCI_MODE_PAUSE;
            }
            else
                mode = MCI_MODE_PLAY;
        }
    }
    else if (!device_io(wmcda->handle, IOCTL_CDROM_READ_Q_CHANNEL, &fmt, sizeof(fmt),
                              &data, sizeof(data), &br, NULL)) {
        if (GetLastError() == ERROR_NOT_READY) mode = MCI_MODE_OPEN;
    } else {
        switch (data.CurrentPosition.Header.AudioStatus)
        {
        case AUDIO_STATUS_IN_PROGRESS:          mode = MCI_MODE_PLAY;   break;
        case AUDIO_STATUS_PAUSED:               mode = MCI_MODE_PAUSE;  break;
        case AUDIO_STATUS_NO_STATUS:
        case AUDIO_STATUS_PLAY_COMPLETE:        mode = MCI_MODE_STOP;   break;
        case AUDIO_STATUS_PLAY_ERROR:
        case AUDIO_STATUS_NOT_SUPPORTED:
        default:
            break;
        }
    }
    return mode;
}

/**************************************************************************
 * 				MCICDA_GetError			[internal]
 */
static	int	MCICDA_GetError(WINE_MCICDAUDIO* wmcda)
{
    switch (GetLastError())
    {
    case ERROR_NOT_READY:     return MCIERR_DEVICE_NOT_READY;
    case ERROR_NOT_SUPPORTED:
    case ERROR_IO_DEVICE:     return MCIERR_HARDWARE;
    default:
	FIXME("Unknown mode %lu\n", GetLastError());
    }
    return MCIERR_DRIVER_INTERNAL;
}

/**************************************************************************
 * 			MCICDA_CalcFrame			[internal]
 */
static DWORD MCICDA_CalcFrame(WINE_MCICDAUDIO* wmcda, DWORD dwTime)
{
    DWORD	dwFrame = 0;
    UINT	wTrack;
    CDROM_TOC   toc;
    DWORD       br;
    BYTE*       addr;

    TRACE("(%p, %08lX, %lu);\n", wmcda, wmcda->dwTimeFormat, dwTime);

    switch (wmcda->dwTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	dwFrame = ((dwTime - 1) * CDFRAMES_PERSEC + 500) / 1000;
	TRACE("MILLISECONDS %lu\n", dwFrame);
	break;
    case MCI_FORMAT_MSF:
	TRACE("MSF %02u:%02u:%02u\n",
	      MCI_MSF_MINUTE(dwTime), MCI_MSF_SECOND(dwTime), MCI_MSF_FRAME(dwTime));
	dwFrame += CDFRAMES_PERMIN * MCI_MSF_MINUTE(dwTime);
	dwFrame += CDFRAMES_PERSEC * MCI_MSF_SECOND(dwTime);
	dwFrame += MCI_MSF_FRAME(dwTime);
	break;
    case MCI_FORMAT_TMSF:
    default: /* unknown format ! force TMSF ! ... */
	wTrack = MCI_TMSF_TRACK(dwTime);
        if (!device_io(wmcda->handle, IOCTL_CDROM_READ_TOC, NULL, 0,
                             &toc, sizeof(toc), &br, NULL))
            return 0;
        if (wTrack < toc.FirstTrack || wTrack > toc.LastTrack)
            return 0;
        TRACE("MSF %02u-%02u:%02u:%02u\n",
              MCI_TMSF_TRACK(dwTime), MCI_TMSF_MINUTE(dwTime),
              MCI_TMSF_SECOND(dwTime), MCI_TMSF_FRAME(dwTime));
        addr = toc.TrackData[wTrack - toc.FirstTrack].Address;
        TRACE("TMSF trackpos[%u]=%d:%d:%d\n",
              wTrack, addr[1], addr[2], addr[3]);
        dwFrame = CDFRAMES_PERMIN * (addr[1] + MCI_TMSF_MINUTE(dwTime)) +
            CDFRAMES_PERSEC * (addr[2] + MCI_TMSF_SECOND(dwTime)) +
            addr[3] + MCI_TMSF_FRAME(dwTime);
	break;
    }
    return dwFrame;
}

/**************************************************************************
 * 			MCICDA_CalcTime				[internal]
 */
static DWORD MCICDA_CalcTime(WINE_MCICDAUDIO* wmcda, DWORD tf, DWORD dwFrame, LPDWORD lpRet)
{
    DWORD	dwTime = 0;
    UINT	wTrack;
    UINT	wMinutes;
    UINT	wSeconds;
    UINT	wFrames;
    CDROM_TOC   toc;
    DWORD       br;

    TRACE("(%p, %08lX, %lu);\n", wmcda, tf, dwFrame);

    switch (tf) {
    case MCI_FORMAT_MILLISECONDS:
	dwTime = (dwFrame * 1000) / CDFRAMES_PERSEC + 1;
	TRACE("MILLISECONDS %lu\n", dwTime);
	*lpRet = 0;
	break;
    case MCI_FORMAT_MSF:
	wMinutes = dwFrame / CDFRAMES_PERMIN;
	wSeconds = (dwFrame - CDFRAMES_PERMIN * wMinutes) / CDFRAMES_PERSEC;
	wFrames = dwFrame - CDFRAMES_PERMIN * wMinutes - CDFRAMES_PERSEC * wSeconds;
	dwTime = MCI_MAKE_MSF(wMinutes, wSeconds, wFrames);
	TRACE("MSF %02u:%02u:%02u -> dwTime=%lu\n",
	      wMinutes, wSeconds, wFrames, dwTime);
	*lpRet = MCI_COLONIZED3_RETURN;
	break;
    case MCI_FORMAT_TMSF:
    default:	/* unknown format ! force TMSF ! ... */
        if (!device_io(wmcda->handle, IOCTL_CDROM_READ_TOC, NULL, 0,
                             &toc, sizeof(toc), &br, NULL))
            return 0;
	if (dwFrame < FRAME_OF_TOC(toc, toc.FirstTrack) ||
            dwFrame > FRAME_OF_TOC(toc, toc.LastTrack + 1)) {
            ERR("Out of range value %lu [%u,%u]\n",
		dwFrame, FRAME_OF_TOC(toc, toc.FirstTrack),
                FRAME_OF_TOC(toc, toc.LastTrack + 1));
	    *lpRet = 0;
	    return 0;
	}
	for (wTrack = toc.FirstTrack; wTrack <= toc.LastTrack; wTrack++) {
	    if (FRAME_OF_TOC(toc, wTrack) > dwFrame)
		break;
	}
        wTrack--;
	dwFrame -= FRAME_OF_TOC(toc, wTrack);
	wMinutes = dwFrame / CDFRAMES_PERMIN;
	wSeconds = (dwFrame - CDFRAMES_PERMIN * wMinutes) / CDFRAMES_PERSEC;
	wFrames = dwFrame - CDFRAMES_PERMIN * wMinutes - CDFRAMES_PERSEC * wSeconds;
	dwTime = MCI_MAKE_TMSF(wTrack, wMinutes, wSeconds, wFrames);
	TRACE("%02u-%02u:%02u:%02u\n", wTrack, wMinutes, wSeconds, wFrames);
	*lpRet = MCI_COLONIZED4_RETURN;
	break;
    }
    return dwTime;
}

static DWORD MCICDA_Stop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);

/**************************************************************************
 * 				MCICDA_Open			[internal]
 */
static DWORD MCICDA_Open(UINT wDevID, DWORD dwFlags, LPMCI_OPEN_PARMSW lpOpenParms)
{
    MCIDEVICEID		dwDeviceID;
    DWORD               ret;
    WINE_MCICDAUDIO* 	wmcda = (WINE_MCICDAUDIO*)mciGetDriverData(wDevID);
    WCHAR               root[7], drive = 0;

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpOpenParms);

    if (lpOpenParms == NULL) 		return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmcda == NULL)			return MCIERR_INVALID_DEVICE_ID;

    dwDeviceID = lpOpenParms->wDeviceID;

    if (wmcda->nUseCount > 0) {
	/* The driver is already open on this channel */
	/* If the driver was opened shareable before and this open specifies */
	/* shareable then increment the use count */
	if (wmcda->fShareable && (dwFlags & MCI_OPEN_SHAREABLE))
	    ++wmcda->nUseCount;
	else
	    return MCIERR_MUST_USE_SHAREABLE;
    } else {
	wmcda->nUseCount = 1;
	wmcda->fShareable = dwFlags & MCI_OPEN_SHAREABLE;
    }
    if (dwFlags & MCI_OPEN_ELEMENT) {
        if (dwFlags & MCI_OPEN_ELEMENT_ID) {
            WARN("MCI_OPEN_ELEMENT_ID %p! Abort\n", lpOpenParms->lpstrElementName);
            ret = MCIERR_FLAGS_NOT_COMPATIBLE;
            goto the_error;
        }
        TRACE("MCI_OPEN_ELEMENT element name: %s\n", debugstr_w(lpOpenParms->lpstrElementName));
        /* Only the first letter counts since w2k
         * Win9x-NT accept only d: and w98SE accepts d:\foobar as well.
         * Play d:\Track03.cda plays from the first track, not #3. */
        if (!isalpha(lpOpenParms->lpstrElementName[0]))
        {
            ret = MCIERR_INVALID_FILE;
            goto the_error;
        }
        drive = toupper(lpOpenParms->lpstrElementName[0]);
        root[0] = drive; root[1] = ':'; root[2] = '\\'; root[3] = '\0';
        if (GetDriveTypeW(root) != DRIVE_CDROM)
        {
            ret = MCIERR_INVALID_FILE;
            goto the_error;
        }
    }
    else
    {
        root[0] = 'A'; root[1] = ':'; root[2] = '\\'; root[3] = '\0';
        for ( ; root[0] <= 'Z'; root[0]++)
        {
            if (GetDriveTypeW(root) == DRIVE_CDROM)
            {
                drive = root[0];
                break;
            }
        }
        if (!drive)
        {
            ret = MCIERR_CANNOT_LOAD_DRIVER; /* drvOpen should return this */
            goto the_error;
        }
    }

    wmcda->wNotifyDeviceID = dwDeviceID;
    wmcda->dwTimeFormat = MCI_FORMAT_MSF;

    /* now, open the handle */
    root[0] = root[1] = '\\'; root[2] = '.'; root[3] = '\\'; root[4] = drive; root[5] = ':'; root[6] = '\0';
    wmcda->handle = CreateFileW(root, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
    if (wmcda->handle == INVALID_HANDLE_VALUE)
    {
        ret = MCIERR_MUST_USE_SHAREABLE;
        goto the_error;
    }

    if (dwFlags & MCI_NOTIFY) {
	mciDriverNotify(HWND_32(LOWORD(lpOpenParms->dwCallback)),
			dwDeviceID, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;

 the_error:
    --wmcda->nUseCount;
    return ret;
}

/**************************************************************************
 * 				MCICDA_Close			[internal]
 */
static DWORD MCICDA_Close(UINT wDevID, DWORD dwParam, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = MCICDA_GetOpenDrv(wDevID);

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwParam, lpParms);

    if (wmcda == NULL) 	return MCIERR_INVALID_DEVICE_ID;

    MCICDA_Stop(wDevID, MCI_WAIT, NULL);

    if (--wmcda->nUseCount == 0) {
	CloseHandle(wmcda->handle);
    }
    if ((dwParam & MCI_NOTIFY) && lpParms)
	MCICDA_Notify(lpParms->dwCallback, wmcda, MCI_NOTIFY_SUCCESSFUL);
    return 0;
}

/**************************************************************************
 * 				MCICDA_GetDevCaps		[internal]
 */
static DWORD MCICDA_GetDevCaps(UINT wDevID, DWORD dwFlags,
				   LPMCI_GETDEVCAPS_PARMS lpParms)
{
    WINE_MCICDAUDIO* 	wmcda = (WINE_MCICDAUDIO*)mciGetDriverData(wDevID);
    DWORD	ret = 0;

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmcda == NULL)			return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	TRACE("MCI_GETDEVCAPS_ITEM dwItem=%08lX;\n", lpParms->dwItem);

	switch (lpParms->dwItem) {
	case MCI_GETDEVCAPS_CAN_RECORD:
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_HAS_AUDIO:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_HAS_VIDEO:
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    lpParms->dwReturn = MAKEMCIRESOURCE(MCI_DEVTYPE_CD_AUDIO, MCI_DEVTYPE_CD_AUDIO);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_USES_FILES:
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_EJECT:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_PLAY:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_SAVE:
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	default:
            WARN("Unsupported %lx devCaps item\n", lpParms->dwItem);
	    return MCIERR_UNSUPPORTED_FUNCTION;
	}
    } else {
	TRACE("No GetDevCaps-Item !\n");
	return MCIERR_MISSING_PARAMETER;
    }
    TRACE("lpParms->dwReturn=%08lX;\n", lpParms->dwReturn);
    if (dwFlags & MCI_NOTIFY) {
	MCICDA_Notify(lpParms->dwCallback, wmcda, MCI_NOTIFY_SUCCESSFUL);
    }
    return ret;
}

static DWORD CDROM_Audio_GetSerial(CDROM_TOC* toc)
{
    DWORD serial = 0;
    int i;
    WORD wMagic;
    DWORD dwStart, dwEnd;

    /*
     * wMagic collects the wFrames from track 1
     * dwStart, dwEnd collect the beginning and end of the disc respectively, in
     * frames.
     * There it is collected for correcting the serial when there are less than
     * 3 tracks.
     */
    wMagic = toc->TrackData[0].Address[3];
    dwStart = FRAME_OF_TOC(*toc, toc->FirstTrack);

    for (i = 0; i <= toc->LastTrack - toc->FirstTrack; i++) {
        serial += (toc->TrackData[i].Address[1] << 16) |
            (toc->TrackData[i].Address[2] << 8) | toc->TrackData[i].Address[3];
    }
    dwEnd = FRAME_OF_TOC(*toc, toc->LastTrack + 1);

    if (toc->LastTrack - toc->FirstTrack + 1 < 3)
        serial += wMagic + (dwEnd - dwStart);

    return serial;
}


/**************************************************************************
 * 				MCICDA_Info			[internal]
 */
static DWORD MCICDA_Info(UINT wDevID, DWORD dwFlags, LPMCI_INFO_PARMSW lpParms)
{
    LPCWSTR		str = NULL;
    WINE_MCICDAUDIO*	wmcda = MCICDA_GetOpenDrv(wDevID);
    DWORD		ret = 0;
    WCHAR		buffer[16];

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL || lpParms->lpstrReturn == NULL)
	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmcda == NULL) return MCIERR_INVALID_DEVICE_ID;

    TRACE("buf=%p, len=%lu\n", lpParms->lpstrReturn, lpParms->dwRetSize);

    if (dwFlags & MCI_INFO_PRODUCT) {
        str = L"Wine's audio CD";
    } else if (dwFlags & MCI_INFO_MEDIA_UPC) {
	ret = MCIERR_NO_IDENTITY;
    } else if (dwFlags & MCI_INFO_MEDIA_IDENTITY) {
	DWORD	    res = 0;
        CDROM_TOC   toc;
        DWORD       br;

        if (!device_io(wmcda->handle, IOCTL_CDROM_READ_TOC, NULL, 0,
                             &toc, sizeof(toc), &br, NULL)) {
	    return MCICDA_GetError(wmcda);
	}

	res = CDROM_Audio_GetSerial(&toc);
	swprintf(buffer, ARRAY_SIZE(buffer), L"%lu", res);
	str = buffer;
    } else {
	WARN("Don't know this info command (%lu)\n", dwFlags);
	ret = MCIERR_MISSING_PARAMETER;
    }
    if (!ret) {
	TRACE("=> %s\n", debugstr_w(str));
	if (lpParms->dwRetSize) {
	    /* FIXME? Since NT, mciwave, mciseq and mcicda set dwRetSize
	     *        to the number of characters written, excluding \0. */
	    lstrcpynW(lpParms->lpstrReturn, str, lpParms->dwRetSize);
	} else ret = MCIERR_PARAM_OVERFLOW;
    }
    if (MMSYSERR_NOERROR==ret && (dwFlags & MCI_NOTIFY))
	MCICDA_Notify(lpParms->dwCallback, wmcda, MCI_NOTIFY_SUCCESSFUL);
    return ret;
}

/**************************************************************************
 * 				MCICDA_Status			[internal]
 */
static DWORD MCICDA_Status(UINT wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
    WINE_MCICDAUDIO*	        wmcda = MCICDA_GetOpenDrv(wDevID);
    DWORD	                ret = 0;
    CDROM_SUB_Q_DATA_FORMAT     fmt;
    SUB_Q_CHANNEL_DATA          data;
    CDROM_TOC                   toc;
    DWORD                       br;

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmcda == NULL) return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_STATUS_ITEM) {
	TRACE("dwItem = %lx\n", lpParms->dwItem);
	switch (lpParms->dwItem) {
	case MCI_STATUS_CURRENT_TRACK:
            fmt.Format = IOCTL_CDROM_CURRENT_POSITION;
            if (!device_io(wmcda->handle, IOCTL_CDROM_READ_Q_CHANNEL, &fmt, sizeof(fmt),
                                 &data, sizeof(data), &br, NULL))
            {
		return MCICDA_GetError(wmcda);
		/* alt. data.CurrentPosition.TrackNumber = 1; -- what native yields */
	    }
	    lpParms->dwReturn = data.CurrentPosition.TrackNumber;
            TRACE("CURRENT_TRACK=%Iu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_LENGTH:
	    if (!MCICDA_ReadTOC(wmcda, &toc, &br))
                return MCICDA_GetError(wmcda);

	    if (dwFlags & MCI_TRACK) {
		TRACE("MCI_TRACK #%lu LENGTH=??? !\n", lpParms->dwTrack);
		if (lpParms->dwTrack < toc.FirstTrack || lpParms->dwTrack > toc.LastTrack)
		    return MCIERR_OUTOFRANGE;
		lpParms->dwReturn = FRAME_OF_TOC(toc, lpParms->dwTrack + 1) -
                    FRAME_OF_TOC(toc, lpParms->dwTrack);
		/* Windows returns one frame less than the total track length for the
		   last track on the CD.  See CDDB HOWTO.  Verified on Win95OSR2. */
		if (lpParms->dwTrack == toc.LastTrack)
		    lpParms->dwReturn--;
	    } else {
		/* Sum of the lengths of all of the tracks.  Inherits the
		   'off by one frame' behavior from the length of the last track.
		   See above comment. */
		lpParms->dwReturn = FRAME_OF_TOC(toc, toc.LastTrack + 1) -
                    FRAME_OF_TOC(toc, toc.FirstTrack) - 1;
	    }
	    lpParms->dwReturn = MCICDA_CalcTime(wmcda,
						 (wmcda->dwTimeFormat == MCI_FORMAT_TMSF)
						    ? MCI_FORMAT_MSF : wmcda->dwTimeFormat,
						 lpParms->dwReturn,
						 &ret);
            TRACE("LENGTH=%Iu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_MODE:
            lpParms->dwReturn = MCICDA_GetStatus(wmcda);
            TRACE("MCI_STATUS_MODE=%08IX\n", lpParms->dwReturn);
	    lpParms->dwReturn = MAKEMCIRESOURCE(lpParms->dwReturn, lpParms->dwReturn);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_MEDIA_PRESENT:
	    lpParms->dwReturn = (MCICDA_GetStatus(wmcda) == MCI_MODE_OPEN) ?
		MAKEMCIRESOURCE(FALSE, MCI_FALSE) : MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    TRACE("MCI_STATUS_MEDIA_PRESENT =%c!\n", LOWORD(lpParms->dwReturn) ? 'Y' : 'N');
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_NUMBER_OF_TRACKS:
	    if (!MCICDA_ReadTOC(wmcda, &toc, &br))
                return MCICDA_GetError(wmcda);

	    lpParms->dwReturn = toc.LastTrack - toc.FirstTrack + 1;
            TRACE("MCI_STATUS_NUMBER_OF_TRACKS = %Iu\n", lpParms->dwReturn);
	    if (lpParms->dwReturn == (WORD)-1)
		return MCICDA_GetError(wmcda);
	    break;
	case MCI_STATUS_POSITION:
            switch (dwFlags & (MCI_STATUS_START | MCI_TRACK)) {
            case MCI_STATUS_START:
                if (!MCICDA_ReadTOC(wmcda, &toc, &br))
                    return MCICDA_GetError(wmcda);

		lpParms->dwReturn = FRAME_OF_TOC(toc, toc.FirstTrack);
		TRACE("get MCI_STATUS_START !\n");
                break;
            case MCI_TRACK:
                if (!MCICDA_ReadTOC(wmcda, &toc, &br))
                    return MCICDA_GetError(wmcda);

		if (lpParms->dwTrack < toc.FirstTrack || lpParms->dwTrack > toc.LastTrack)
		    return MCIERR_OUTOFRANGE;
		lpParms->dwReturn = FRAME_OF_TOC(toc, lpParms->dwTrack);
		TRACE("get MCI_TRACK #%lu !\n", lpParms->dwTrack);
                break;
            case 0:
                fmt.Format = IOCTL_CDROM_CURRENT_POSITION;
                if (!device_io(wmcda->handle, IOCTL_CDROM_READ_Q_CHANNEL, &fmt, sizeof(fmt),
                                     &data, sizeof(data), &br, NULL)) {
                    return MCICDA_GetError(wmcda);
                }
                lpParms->dwReturn = FRAME_OF_ADDR(data.CurrentPosition.AbsoluteAddress);
                break;
            default:
                return MCIERR_FLAGS_NOT_COMPATIBLE;
            }
	    lpParms->dwReturn = MCICDA_CalcTime(wmcda, wmcda->dwTimeFormat, lpParms->dwReturn, &ret);
            TRACE("MCI_STATUS_POSITION=%08IX\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_READY:
	    TRACE("MCI_STATUS_READY !\n");
            switch (MCICDA_GetStatus(wmcda))
            {
            case MCI_MODE_NOT_READY:
            case MCI_MODE_OPEN:
                lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
                break;
            default:
                lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
                break;
            }
	    TRACE("MCI_STATUS_READY=%u!\n", LOWORD(lpParms->dwReturn));
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_TIME_FORMAT:
	    lpParms->dwReturn = MAKEMCIRESOURCE(wmcda->dwTimeFormat, MCI_FORMAT_RETURN_BASE + wmcda->dwTimeFormat);
	    TRACE("MCI_STATUS_TIME_FORMAT=%08x!\n", LOWORD(lpParms->dwReturn));
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case 4001: /* FIXME: for bogus FullCD */
	case MCI_CDA_STATUS_TYPE_TRACK:
	    if (!(dwFlags & MCI_TRACK))
		ret = MCIERR_MISSING_PARAMETER;
	    else {
                if (!MCICDA_ReadTOC(wmcda, &toc, &br))
                    return MCICDA_GetError(wmcda);

		if (lpParms->dwTrack < toc.FirstTrack || lpParms->dwTrack > toc.LastTrack)
		    ret = MCIERR_OUTOFRANGE;
		else
		    lpParms->dwReturn = (toc.TrackData[lpParms->dwTrack - toc.FirstTrack].Control & 0x04) ?
                                         MCI_CDA_TRACK_OTHER : MCI_CDA_TRACK_AUDIO;
		    /* FIXME: MAKEMCIRESOURCE "audio" | "other", localised */
	    }
            TRACE("MCI_CDA_STATUS_TYPE_TRACK[%ld]=%Id\n", lpParms->dwTrack, lpParms->dwReturn);
	    break;
	default:
            FIXME("unknown command %08lX !\n", lpParms->dwItem);
	    return MCIERR_UNSUPPORTED_FUNCTION;
	}
    } else return MCIERR_MISSING_PARAMETER;
    if ((dwFlags & MCI_NOTIFY) && HRESULT_CODE(ret)==0)
	MCICDA_Notify(lpParms->dwCallback, wmcda, MCI_NOTIFY_SUCCESSFUL);
    return ret;
}

/**************************************************************************
 * 				MCICDA_SkipDataTracks		[internal]
 */
static DWORD MCICDA_SkipDataTracks(WINE_MCICDAUDIO* wmcda,DWORD *frame)
{
  int i;
  DWORD br;
  CDROM_TOC toc;
  if (!MCICDA_ReadTOC(wmcda, &toc, &br))
    return MCICDA_GetError(wmcda);

  if (*frame < FRAME_OF_TOC(toc,toc.FirstTrack) ||
      *frame >= FRAME_OF_TOC(toc,toc.LastTrack+1)) /* lead-out */
    return MCIERR_OUTOFRANGE;
  for(i=toc.LastTrack+1;i>toc.FirstTrack;i--)
    if ( FRAME_OF_TOC(toc, i) <= *frame ) break;
  /* i points to last track whose start address is not greater than frame.
   * Now skip non-audio tracks */
  for(;i<=toc.LastTrack;i++)
    if ( ! (toc.TrackData[i-toc.FirstTrack].Control & 4) )
      break;
  /* The frame will be an address in the next audio track or
   * address of lead-out. */
  if ( FRAME_OF_TOC(toc, i) > *frame )
    *frame = FRAME_OF_TOC(toc, i);
  /* Lead-out is an invalid seek position (on Linux as well). */
  if (*frame == FRAME_OF_TOC(toc,toc.LastTrack+1))
     (*frame)--;
  return 0;
}

/**************************************************************************
 * 				MCICDA_Play			[internal]
 */
static DWORD MCICDA_Play(UINT wDevID, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
    WINE_MCICDAUDIO*	        wmcda = MCICDA_GetOpenDrv(wDevID);
    DWORD		        ret = 0, start, end;
    HANDLE                      oldcb;
    DWORD                       br;
    CDROM_PLAY_AUDIO_MSF        play;
    CDROM_SUB_Q_DATA_FORMAT     fmt;
    SUB_Q_CHANNEL_DATA          data;
    CDROM_TOC			toc;

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL)
	return MCIERR_NULL_PARAMETER_BLOCK;

    if (wmcda == NULL)
	return MCIERR_INVALID_DEVICE_ID;

    if (!MCICDA_ReadTOC(wmcda, &toc, &br))
        return MCICDA_GetError(wmcda);

    if (dwFlags & MCI_FROM) {
	start = MCICDA_CalcFrame(wmcda, lpParms->dwFrom);
	if ( (ret=MCICDA_SkipDataTracks(wmcda, &start)) )
	  return ret;
	TRACE("MCI_FROM=%08lX -> %lu\n", lpParms->dwFrom, start);
    } else {
        fmt.Format = IOCTL_CDROM_CURRENT_POSITION;
        if (!device_io(wmcda->handle, IOCTL_CDROM_READ_Q_CHANNEL, &fmt, sizeof(fmt),
                             &data, sizeof(data), &br, NULL)) {
            return MCICDA_GetError(wmcda);
        }
        start = FRAME_OF_ADDR(data.CurrentPosition.AbsoluteAddress);
	if ( (ret=MCICDA_SkipDataTracks(wmcda, &start)) )
	  return ret;
    }
    if (dwFlags & MCI_TO) {
	end = MCICDA_CalcFrame(wmcda, lpParms->dwTo);
	if ( (ret=MCICDA_SkipDataTracks(wmcda, &end)) )
	  return ret;
	TRACE("MCI_TO=%08lX -> %lu\n", lpParms->dwTo, end);
    } else {
	end = FRAME_OF_TOC(toc, toc.LastTrack + 1) - 1;
    }
    if (end < start) return MCIERR_OUTOFRANGE;
    TRACE("Playing from %lu to %lu\n", start, end);

    oldcb = InterlockedExchangePointer(&wmcda->hCallback,
	(dwFlags & MCI_NOTIFY) ? HWND_32(LOWORD(lpParms->dwCallback)) : NULL);
    if (oldcb) mciDriverNotify(oldcb, wmcda->wNotifyDeviceID, MCI_NOTIFY_ABORTED);

    if (start == end || start == FRAME_OF_TOC(toc,toc.LastTrack+1)-1) {
        if (dwFlags & MCI_NOTIFY) {
            oldcb = InterlockedExchangePointer(&wmcda->hCallback, NULL);
            if (oldcb) mciDriverNotify(oldcb, wDevID, MCI_NOTIFY_SUCCESSFUL);
        }
        return MMSYSERR_NOERROR;
    }

    if (wmcda->hThread != 0) {
        SetEvent(wmcda->stopEvent);
        WaitForSingleObject(wmcda->hThread, INFINITE);

        CloseHandle(wmcda->hThread);
        wmcda->hThread = 0;
        CloseHandle(wmcda->stopEvent);
        wmcda->stopEvent = 0;

        IDirectSoundBuffer_Stop(wmcda->dsBuf);
        IDirectSoundBuffer_Release(wmcda->dsBuf);
        wmcda->dsBuf = NULL;
        IDirectSound_Release(wmcda->dsObj);
        wmcda->dsObj = NULL;
    }

    if (pDirectSoundCreate) {
        WAVEFORMATEX format;
        DSBUFFERDESC desc;
        DWORD lockLen;
        void *cdData;
        HRESULT hr;

        hr = pDirectSoundCreate(NULL, &wmcda->dsObj, NULL);
        if (SUCCEEDED(hr)) {
            IDirectSound_SetCooperativeLevel(wmcda->dsObj, GetDesktopWindow(), DSSCL_PRIORITY);

            /* The "raw" frame is relative to the start of the first track */
            wmcda->start = start - FRAME_OF_TOC(toc, toc.FirstTrack);
            wmcda->end = end - FRAME_OF_TOC(toc, toc.FirstTrack);

            memset(&format, 0, sizeof(format));
            format.wFormatTag = WAVE_FORMAT_PCM;
            format.nChannels = 2;
            format.nSamplesPerSec = 44100;
            format.wBitsPerSample = 16;
            format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
            format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
            format.cbSize = 0;

            memset(&desc, 0, sizeof(desc));
            desc.dwSize = sizeof(desc);
            desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
            desc.dwBufferBytes = (CDDA_FRAG_SIZE - (CDDA_FRAG_SIZE%RAW_SECTOR_SIZE)) * CDDA_FRAG_COUNT;
            desc.lpwfxFormat = &format;

            hr = IDirectSound_CreateSoundBuffer(wmcda->dsObj, &desc, &wmcda->dsBuf, NULL);
        }
        if (SUCCEEDED(hr)) {
            hr = IDirectSoundBuffer_Lock(wmcda->dsBuf, 0, 0, &cdData, &lockLen,
                                         NULL, NULL, DSBLOCK_ENTIREBUFFER);
        }
        if (SUCCEEDED(hr)) {
            RAW_READ_INFO rdInfo;
            int readok;

            rdInfo.DiskOffset.QuadPart = wmcda->start<<11;
            rdInfo.SectorCount = min(desc.dwBufferBytes/RAW_SECTOR_SIZE,
                                     wmcda->end-wmcda->start);
            rdInfo.TrackMode = CDDA;

            readok = device_io(wmcda->handle, IOCTL_CDROM_RAW_READ,
                                     &rdInfo, sizeof(rdInfo), cdData, lockLen,
                                     &br, NULL);
            IDirectSoundBuffer_Unlock(wmcda->dsBuf, cdData, lockLen, NULL, 0);

            if (readok) {
                wmcda->start += rdInfo.SectorCount;
                wmcda->stopEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
            }
            if (wmcda->stopEvent != 0)
                wmcda->hThread = CreateThread(NULL, 0, MCICDA_playLoop, wmcda, 0, &br);
            if (wmcda->hThread != 0) {
                hr = IDirectSoundBuffer_Play(wmcda->dsBuf, 0, 0, DSBPLAY_LOOPING);
                if (SUCCEEDED(hr)) {
                    /* FIXME: implement MCI_WAIT and send notification only in that case */
                    if (0) {
                        oldcb = InterlockedExchangePointer(&wmcda->hCallback, NULL);
                        if (oldcb) mciDriverNotify(oldcb, wmcda->wNotifyDeviceID,
                            FAILED(hr) ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL);
                    }
                    return ret;
                }

                SetEvent(wmcda->stopEvent);
                WaitForSingleObject(wmcda->hThread, INFINITE);
                CloseHandle(wmcda->hThread);
                wmcda->hThread = 0;
            }
        }

        if (wmcda->stopEvent != 0) {
            CloseHandle(wmcda->stopEvent);
            wmcda->stopEvent = 0;
        }
        if (wmcda->dsBuf) {
            IDirectSoundBuffer_Release(wmcda->dsBuf);
            wmcda->dsBuf = NULL;
        }
        if (wmcda->dsObj) {
            IDirectSound_Release(wmcda->dsObj);
            wmcda->dsObj = NULL;
        }
    }

    play.StartingM = start / CDFRAMES_PERMIN;
    play.StartingS = (start / CDFRAMES_PERSEC) % 60;
    play.StartingF = start % CDFRAMES_PERSEC;
    play.EndingM   = end / CDFRAMES_PERMIN;
    play.EndingS   = (end / CDFRAMES_PERSEC) % 60;
    play.EndingF   = end % CDFRAMES_PERSEC;
    if (!device_io(wmcda->handle, IOCTL_CDROM_PLAY_AUDIO_MSF, &play, sizeof(play),
                         NULL, 0, &br, NULL)) {
	wmcda->hCallback = NULL;
	ret = MCIERR_HARDWARE;
    }
    /* The independent CD player has no means to signal MCI_NOTIFY when it's done.
     * Native sends a notification with MCI_WAIT only. */
    return ret;
}

/**************************************************************************
 * 				MCICDA_Stop			[internal]
 */
static DWORD MCICDA_Stop(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = MCICDA_GetOpenDrv(wDevID);
    HANDLE		oldcb;
    DWORD               br;

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;

    oldcb = InterlockedExchangePointer(&wmcda->hCallback, NULL);
    if (oldcb) mciDriverNotify(oldcb, wmcda->wNotifyDeviceID, MCI_NOTIFY_ABORTED);

    if (wmcda->hThread != 0) {
        SetEvent(wmcda->stopEvent);
        WaitForSingleObject(wmcda->hThread, INFINITE);

        CloseHandle(wmcda->hThread);
        wmcda->hThread = 0;
        CloseHandle(wmcda->stopEvent);
        wmcda->stopEvent = 0;

        IDirectSoundBuffer_Release(wmcda->dsBuf);
        wmcda->dsBuf = NULL;
        IDirectSound_Release(wmcda->dsObj);
        wmcda->dsObj = NULL;
    }
    else if (!device_io(wmcda->handle, IOCTL_CDROM_STOP_AUDIO, NULL, 0, NULL, 0, &br, NULL))
        return MCIERR_HARDWARE;

    if ((dwFlags & MCI_NOTIFY) && lpParms)
	MCICDA_Notify(lpParms->dwCallback, wmcda, MCI_NOTIFY_SUCCESSFUL);
    return 0;
}

/**************************************************************************
 * 				MCICDA_Pause			[internal]
 */
static DWORD MCICDA_Pause(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = MCICDA_GetOpenDrv(wDevID);
    HANDLE		oldcb;
    DWORD               br;

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;

    oldcb = InterlockedExchangePointer(&wmcda->hCallback, NULL);
    if (oldcb) mciDriverNotify(oldcb, wmcda->wNotifyDeviceID, MCI_NOTIFY_ABORTED);

    if (wmcda->hThread != 0) {
        /* Don't bother calling stop if the playLoop thread has already stopped */
        if(WaitForSingleObject(wmcda->stopEvent, 0) != WAIT_OBJECT_0 &&
           FAILED(IDirectSoundBuffer_Stop(wmcda->dsBuf)))
            return MCIERR_HARDWARE;
    }
    else if (!device_io(wmcda->handle, IOCTL_CDROM_PAUSE_AUDIO, NULL, 0, NULL, 0, &br, NULL))
        return MCIERR_HARDWARE;

    if ((dwFlags & MCI_NOTIFY) && lpParms)
	MCICDA_Notify(lpParms->dwCallback, wmcda, MCI_NOTIFY_SUCCESSFUL);
    return 0;
}

/**************************************************************************
 * 				MCICDA_Resume			[internal]
 */
static DWORD MCICDA_Resume(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = MCICDA_GetOpenDrv(wDevID);
    DWORD               br;

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;

    if (wmcda->hThread != 0) {
        /* Don't restart if the playLoop thread has already stopped */
        if(WaitForSingleObject(wmcda->stopEvent, 0) != WAIT_OBJECT_0 &&
           FAILED(IDirectSoundBuffer_Play(wmcda->dsBuf, 0, 0, DSBPLAY_LOOPING)))
            return MCIERR_HARDWARE;
    }
    else if (!device_io(wmcda->handle, IOCTL_CDROM_RESUME_AUDIO, NULL, 0, NULL, 0, &br, NULL))
        return MCIERR_HARDWARE;

    if ((dwFlags & MCI_NOTIFY) && lpParms)
	MCICDA_Notify(lpParms->dwCallback, wmcda, MCI_NOTIFY_SUCCESSFUL);
    return 0;
}

/**************************************************************************
 * 				MCICDA_Seek			[internal]
 */
static DWORD MCICDA_Seek(UINT wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
    DWORD		        at;
    WINE_MCICDAUDIO*	        wmcda = MCICDA_GetOpenDrv(wDevID);
    CDROM_SEEK_AUDIO_MSF        seek;
    DWORD                       br, position, ret;
    CDROM_TOC			toc;

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;
    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;

    position = dwFlags & (MCI_SEEK_TO_START|MCI_SEEK_TO_END|MCI_TO);
    if (!position)		return MCIERR_MISSING_PARAMETER;
    if (position&(position-1))	return MCIERR_FLAGS_NOT_COMPATIBLE;

    /* Stop sends MCI_NOTIFY_ABORTED when needed.
     * Tests show that native first sends ABORTED and reads the TOC,
     * then only checks the position flags, then stops and seeks. */
    MCICDA_Stop(wDevID, MCI_WAIT, 0);

    if (!MCICDA_ReadTOC(wmcda, &toc, &br))
        return MCICDA_GetError(wmcda);

    switch (position) {
    case MCI_SEEK_TO_START:
	TRACE("Seeking to start\n");
	at = FRAME_OF_TOC(toc,toc.FirstTrack);
	if ( (ret=MCICDA_SkipDataTracks(wmcda, &at)) )
	  return ret;
	break;
    case MCI_SEEK_TO_END:
	TRACE("Seeking to end\n");
	/* End is prior to lead-out
	 * yet Win9X seeks to even one frame less than that. */
	at = FRAME_OF_TOC(toc, toc.LastTrack + 1) - 1;
	if ( (ret=MCICDA_SkipDataTracks(wmcda, &at)) )
	  return ret;
	break;
    case MCI_TO:
	TRACE("Seeking to %lu\n", lpParms->dwTo);
        at = MCICDA_CalcFrame(wmcda, lpParms->dwTo);
	if ( (ret=MCICDA_SkipDataTracks(wmcda, &at)) )
	  return ret;
	break;
    default:
	return MCIERR_FLAGS_NOT_COMPATIBLE;
    }

    {
        seek.M = at / CDFRAMES_PERMIN;
        seek.S = (at / CDFRAMES_PERSEC) % 60;
        seek.F = at % CDFRAMES_PERSEC;
        if (!device_io(wmcda->handle, IOCTL_CDROM_SEEK_AUDIO_MSF, &seek, sizeof(seek),
                             NULL, 0, &br, NULL))
            return MCIERR_HARDWARE;
    }

    if (dwFlags & MCI_NOTIFY)
	MCICDA_Notify(lpParms->dwCallback, wmcda, MCI_NOTIFY_SUCCESSFUL);
    return 0;
}

/**************************************************************************
 * 				MCICDA_SetDoor			[internal]
 */
static DWORD	MCICDA_SetDoor(UINT wDevID, BOOL open)
{
    WINE_MCICDAUDIO*	wmcda = MCICDA_GetOpenDrv(wDevID);
    DWORD               br;

    TRACE("(%04x, %s) !\n", wDevID, (open) ? "OPEN" : "CLOSE");

    if (wmcda == NULL) return MCIERR_INVALID_DEVICE_ID;

    if (!device_io(wmcda->handle,
                         (open) ? IOCTL_STORAGE_EJECT_MEDIA : IOCTL_STORAGE_LOAD_MEDIA,
                         NULL, 0, NULL, 0, &br, NULL))
	return MCIERR_HARDWARE;

    return 0;
}

/**************************************************************************
 * 				MCICDA_Set			[internal]
 */
static DWORD MCICDA_Set(UINT wDevID, DWORD dwFlags, LPMCI_SET_PARMS lpParms)
{
    WINE_MCICDAUDIO*	wmcda = MCICDA_GetOpenDrv(wDevID);

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wmcda == NULL)	return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_SET_DOOR_OPEN) {
	MCICDA_SetDoor(wDevID, TRUE);
    }
    if (dwFlags & MCI_SET_DOOR_CLOSED) {
	MCICDA_SetDoor(wDevID, FALSE);
    }

    /* only functions which require valid lpParms below this line ! */
    if (lpParms == NULL) return MCIERR_NULL_PARAMETER_BLOCK;
    /*
      TRACE("dwTimeFormat=%08lX\n", lpParms->dwTimeFormat);
    */
    if (dwFlags & MCI_SET_TIME_FORMAT) {
	switch (lpParms->dwTimeFormat) {
	case MCI_FORMAT_MILLISECONDS:
	    TRACE("MCI_FORMAT_MILLISECONDS !\n");
	    break;
	case MCI_FORMAT_MSF:
	    TRACE("MCI_FORMAT_MSF !\n");
	    break;
	case MCI_FORMAT_TMSF:
	    TRACE("MCI_FORMAT_TMSF !\n");
	    break;
	default:
	    return MCIERR_BAD_TIME_FORMAT;
	}
	wmcda->dwTimeFormat = lpParms->dwTimeFormat;
    }
    if (dwFlags & MCI_SET_AUDIO) /* one xp machine ignored it */
	TRACE("SET_AUDIO %lX %lx\n", dwFlags, lpParms->dwAudio);

    if (dwFlags & MCI_NOTIFY)
	MCICDA_Notify(lpParms->dwCallback, wmcda, MCI_NOTIFY_SUCCESSFUL);
    return 0;
}

/**************************************************************************
 * 			DriverProc (MCICDA.@)
 */
LRESULT CALLBACK MCICDA_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                   LPARAM dwParam1, LPARAM dwParam2)
{
    switch(wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return MCICDA_drvOpen((LPCWSTR)dwParam1, (LPMCI_OPEN_DRIVER_PARMSW)dwParam2);
    case DRV_CLOSE:		return MCICDA_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "MCI audio CD driver !", "Wine Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    }

    if (dwDevID == 0xFFFFFFFF) return MCIERR_UNSUPPORTED_FUNCTION;

    switch (wMsg) {
    case MCI_OPEN_DRIVER:	return MCICDA_Open(dwDevID, dwParam1, (LPMCI_OPEN_PARMSW)dwParam2);
    case MCI_CLOSE_DRIVER:	return MCICDA_Close(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_GETDEVCAPS:	return MCICDA_GetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
    case MCI_INFO:		return MCICDA_Info(dwDevID, dwParam1, (LPMCI_INFO_PARMSW)dwParam2);
    case MCI_STATUS:		return MCICDA_Status(dwDevID, dwParam1, (LPMCI_STATUS_PARMS)dwParam2);
    case MCI_SET:		return MCICDA_Set(dwDevID, dwParam1, (LPMCI_SET_PARMS)dwParam2);
    case MCI_PLAY:		return MCICDA_Play(dwDevID, dwParam1, (LPMCI_PLAY_PARMS)dwParam2);
    case MCI_STOP:		return MCICDA_Stop(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_PAUSE:		return MCICDA_Pause(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_RESUME:		return MCICDA_Resume(dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)dwParam2);
    case MCI_SEEK:		return MCICDA_Seek(dwDevID, dwParam1, (LPMCI_SEEK_PARMS)dwParam2);
    /* commands that should report an error as they are not supported in
     * the native version */
    case MCI_RECORD:
    case MCI_LOAD:
    case MCI_SAVE:
	return MCIERR_UNSUPPORTED_FUNCTION;
    case MCI_BREAK:
    case MCI_FREEZE:
    case MCI_PUT:
    case MCI_REALIZE:
    case MCI_UNFREEZE:
    case MCI_UPDATE:
    case MCI_WHERE:
    case MCI_STEP:
    case MCI_SPIN:
    case MCI_ESCAPE:
    case MCI_COPY:
    case MCI_CUT:
    case MCI_DELETE:
    case MCI_PASTE:
    case MCI_WINDOW:
	TRACE("Unsupported command [0x%x]\n", wMsg);
	break;
    case MCI_OPEN:
    case MCI_CLOSE:
	ERR("Shouldn't receive a MCI_OPEN or CLOSE message\n");
	break;
    default:
	TRACE("Sending msg [0x%x] to default driver proc\n", wMsg);
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MCIERR_UNRECOGNIZED_COMMAND;
}

/*-----------------------------------------------------------------------*/
