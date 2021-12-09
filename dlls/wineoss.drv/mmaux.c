/*
 * Sample AUXILIARY Wine Driver
 *
 * Copyright 1994 Martin Ayotte
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

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "windef.h"
#include "winbase.h"
#include "mmddk.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmaux);

#define MIXER_DEV "/dev/mixer"

static int	NumDev = 6;

/*-----------------------------------------------------------------------*/

static LRESULT OSS_AuxInit(void)
{
    int	mixer;
    TRACE("()\n");

    if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
	WARN("mixer device not available !\n");
	NumDev = 0;
    } else {
	close(mixer);
	NumDev = 6;
    }
    return 0;
}

/*-----------------------------------------------------------------------*/

static LRESULT OSS_AuxExit(void)
{
    TRACE("()\n");
    return 0;
}

/**************************************************************************
 * 				AUX_GetDevCaps			[internal]
 */
static DWORD AUX_GetDevCaps(WORD wDevID, LPAUXCAPSW lpCaps, DWORD dwSize)
{
    int 	mixer, volume;
    static const WCHAR ini[] = {'O','S','S',' ','A','u','x',' ','#','0',0};

    TRACE("(%04X, %p, %u);\n", wDevID, lpCaps, dwSize);
    if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
    if (wDevID >= NumDev) return MMSYSERR_BADDEVICEID;
    if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
	WARN("mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, SOUND_MIXER_READ_LINE, &volume) == -1) {
	close(mixer);
	WARN("unable to read mixer !\n");
	return MMSYSERR_NOTENABLED;
    }
    close(mixer);
    lpCaps->wMid = 0xAA;
    lpCaps->wPid = 0x55 + wDevID;
    lpCaps->vDriverVersion = 0x0100;
    strcpyW(lpCaps->szPname, ini);
    lpCaps->szPname[9] = '0' + wDevID; /* 6  at max */
    lpCaps->wTechnology = wDevID == 2 ? AUXCAPS_CDAUDIO : AUXCAPS_AUXIN;
    lpCaps->wReserved1 = 0;
    lpCaps->dwSupport = AUXCAPS_VOLUME | AUXCAPS_LRVOLUME;

    return MMSYSERR_NOERROR;
}


/**************************************************************************
 * 				AUX_GetVolume			[internal]
 */
static DWORD AUX_GetVolume(WORD wDevID, LPDWORD lpdwVol)
{
    int 	mixer, volume, left, right, cmd;

    TRACE("(%04X, %p);\n", wDevID, lpdwVol);
    if (lpdwVol == NULL) return MMSYSERR_NOTENABLED;
    if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
	WARN("mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }
    switch(wDevID) {
    case 0:
	TRACE("SOUND_MIXER_READ_PCM !\n");
	cmd = SOUND_MIXER_READ_PCM;
	break;
    case 1:
	TRACE("SOUND_MIXER_READ_SYNTH !\n");
	cmd = SOUND_MIXER_READ_SYNTH;
	break;
    case 2:
	TRACE("SOUND_MIXER_READ_CD !\n");
	cmd = SOUND_MIXER_READ_CD;
	break;
    case 3:
	TRACE("SOUND_MIXER_READ_LINE !\n");
	cmd = SOUND_MIXER_READ_LINE;
	break;
    case 4:
	TRACE("SOUND_MIXER_READ_MIC !\n");
	cmd = SOUND_MIXER_READ_MIC;
	break;
    case 5:
	TRACE("SOUND_MIXER_READ_VOLUME !\n");
	cmd = SOUND_MIXER_READ_VOLUME;
	break;
    default:
	WARN("invalid device id=%04X !\n", wDevID);
	close(mixer);
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, cmd, &volume) == -1) {
	WARN("unable to read mixer !\n");
	close(mixer);
	return MMSYSERR_NOTENABLED;
    }
    close(mixer);
    left  = LOBYTE(LOWORD(volume));
    right = HIBYTE(LOWORD(volume));
    TRACE("left=%d right=%d !\n", left, right);
    *lpdwVol = MAKELONG((left * 0xFFFFL) / 100, (right * 0xFFFFL) / 100);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				AUX_SetVolume			[internal]
 */
static DWORD AUX_SetVolume(WORD wDevID, DWORD dwParam)
{
    int 	mixer;
    int		volume, left, right;
    int		cmd;

    TRACE("(%04X, %08X);\n", wDevID, dwParam);

    left   = (LOWORD(dwParam) * 100) >> 16;
    right  = (HIWORD(dwParam) * 100) >> 16;
    volume = (right << 8) | left;

    if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
	WARN("mixer device not available !\n");
	return MMSYSERR_NOTENABLED;
    }

    switch(wDevID) {
    case 0:
	TRACE("SOUND_MIXER_WRITE_PCM !\n");
	cmd = SOUND_MIXER_WRITE_PCM;
	break;
    case 1:
	TRACE("SOUND_MIXER_WRITE_SYNTH !\n");
	cmd = SOUND_MIXER_WRITE_SYNTH;
	break;
    case 2:
	TRACE("SOUND_MIXER_WRITE_CD !\n");
	cmd = SOUND_MIXER_WRITE_CD;
	break;
    case 3:
	TRACE("SOUND_MIXER_WRITE_LINE !\n");
	cmd = SOUND_MIXER_WRITE_LINE;
	break;
    case 4:
	TRACE("SOUND_MIXER_WRITE_MIC !\n");
	cmd = SOUND_MIXER_WRITE_MIC;
	break;
    case 5:
	TRACE("SOUND_MIXER_WRITE_VOLUME !\n");
	cmd = SOUND_MIXER_WRITE_VOLUME;
	break;
    default:
	WARN("invalid device id=%04X !\n", wDevID);
	close(mixer);
	return MMSYSERR_NOTENABLED;
    }
    if (ioctl(mixer, cmd, &volume) == -1) {
	WARN("unable to set mixer !\n");
	close(mixer);
	return MMSYSERR_NOTENABLED;
    }
    close(mixer);
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 *		auxMessage (WINEOSS.2)
 */
DWORD WINAPI OSS_auxMessage(UINT wDevID, UINT wMsg, DWORD_PTR dwUser,
			    DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    TRACE("(%04X, %04X, %08lX, %08lX, %08lX);\n",
	  wDevID, wMsg, dwUser, dwParam1, dwParam2);

    switch (wMsg) {
    case DRVM_INIT:
        return OSS_AuxInit();
    case DRVM_EXIT:
        return OSS_AuxExit();
    case DRVM_ENABLE:
    case DRVM_DISABLE:
	/* FIXME: Pretend this is supported */
	return 0;
    case AUXDM_GETDEVCAPS:
	return AUX_GetDevCaps(wDevID, (LPAUXCAPSW)dwParam1, dwParam2);
    case AUXDM_GETNUMDEVS:
	TRACE("return %d;\n", NumDev);
	return NumDev;
    case AUXDM_GETVOLUME:
	return AUX_GetVolume(wDevID, (LPDWORD)dwParam1);
    case AUXDM_SETVOLUME:
	return AUX_SetVolume(wDevID, dwParam1);
    default:
	WARN("unknown message !\n");
    }
    return MMSYSERR_NOTSUPPORTED;
}
