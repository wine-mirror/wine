/*
 * Sample AUXILARY Wine Driver for Linux
 *
 * Copyright 1994 Martin Ayotte
 */
static char Copyright[] = "Copyright  Martin Ayotte, 1994";

#ifndef WINELIB
#define BUILTIN_MMSYSTEM
#endif 

#ifdef BUILTIN_MMSYSTEM

#define EMULATE_SB16

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "win.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"

#ifdef linux
#include <linux/soundcard.h>
#endif

#define SOUND_DEV "/dev/dsp"
#define MIXER_DEV "/dev/mixer"

#ifdef SOUND_VERSION
#define IOCTL(a,b,c)		ioctl(a,b,&c)
#else
#define IOCTL(a,b,c)		(c = ioctl(a,b,c) )
#endif


/*-----------------------------------------------------------------------*/


/**************************************************************************
* 				AUX_GetDevCaps			[internal]
*/
DWORD AUX_GetDevCaps(WORD wDevID, LPAUXCAPS lpCaps, DWORD dwSize)
{
#ifdef linux
	int 	mixer;
	int		volume;
	printf("AUX_GetDevCaps(%u, %08X, %u);\n", wDevID, lpCaps, dwSize);
	if (lpCaps == NULL) return MMSYSERR_NOTENABLED;
	if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
		printf("AUX_GetDevCaps // mixer device not available !\n");
		return MMSYSERR_NOTENABLED;
		}
    if (ioctl(mixer, SOUND_MIXER_READ_LINE, &volume) == -1) {
		close(mixer);
		printf("AUX_GetDevCaps // unable read mixer !\n");
		return MMSYSERR_NOTENABLED;
		}
	close(mixer);
#ifdef EMULATE_SB16
	lpCaps->wMid = 0x0002;
	lpCaps->vDriverVersion = 0x0200;
	lpCaps->dwSupport = AUXCAPS_VOLUME | AUXCAPS_LRVOLUME;
	switch (wDevID) {
		case 0:
			lpCaps->wPid = 0x0196;
			strcpy(lpCaps->szPname, "SB16 Aux: Wave");
			lpCaps->wTechnology = AUXCAPS_AUXIN;
			break;
		case 1:
			lpCaps->wPid = 0x0197;
			strcpy(lpCaps->szPname, "SB16 Aux: Midi Synth");
			lpCaps->wTechnology = AUXCAPS_AUXIN;
			break;
		case 2:
			lpCaps->wPid = 0x0191;
			strcpy(lpCaps->szPname, "SB16 Aux: CD");
			lpCaps->wTechnology = AUXCAPS_CDAUDIO;
			break;
		case 3:
			lpCaps->wPid = 0x0192;
			strcpy(lpCaps->szPname, "SB16 Aux: Line-In");
			lpCaps->wTechnology = AUXCAPS_AUXIN;
			break;
		case 4:
			lpCaps->wPid = 0x0193;
			strcpy(lpCaps->szPname, "SB16 Aux: Mic");
			lpCaps->wTechnology = AUXCAPS_AUXIN;
			break;
		case 5:
			lpCaps->wPid = 0x0194;
			strcpy(lpCaps->szPname, "SB16 Aux: Master");
			lpCaps->wTechnology = AUXCAPS_AUXIN;
			break;
		}
#else
	lpCaps->wMid = 0xAA;
	lpCaps->wPid = 0x55;
	lpCaps->vDriverVersion = 0x0100;
	strcpy(lpCaps->szPname, "Generic Linux Auxiliary Driver");
	lpCaps->wTechnology = AUXCAPS_CDAUDIO;
	lpCaps->dwSupport = AUXCAPS_VOLUME | AUXCAPS_LRVOLUME;
#endif
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				AUX_GetVolume			[internal]
*/
DWORD AUX_GetVolume(WORD wDevID, LPDWORD lpdwVol)
{
#ifdef linux
	int 	mixer;
	int		volume;
	printf("AUX_GetVolume(%u, %08X);\n", wDevID, lpdwVol);
	if (lpdwVol == NULL) return MMSYSERR_NOTENABLED;
	if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
		printf("Linux 'AUX_GetVolume' // mixer device not available !\n");
		return MMSYSERR_NOTENABLED;
		}
    if (ioctl(mixer, SOUND_MIXER_READ_LINE, &volume) == -1) {
		printf("Linux 'AUX_GetVolume' // unable read mixer !\n");
		return MMSYSERR_NOTENABLED;
		}
	close(mixer);
	*lpdwVol = MAKELONG(volume, volume);
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}

/**************************************************************************
* 				AUX_SetVolume			[internal]
*/
DWORD AUX_SetVolume(WORD wDevID, DWORD dwParam)
{
#ifdef linux
	int 	mixer;
	int		volume = 50;
	printf("AUX_SetVolume(%u, %08X);\n", wDevID, dwParam);
	if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
		printf("Linux 'AUX_SetVolume' // mixer device not available !\n");
		return MMSYSERR_NOTENABLED;
		}
    if (ioctl(mixer, SOUND_MIXER_WRITE_LINE, &volume) == -1) {
		printf("Linux 'AUX_SetVolume' // unable set mixer !\n");
		return MMSYSERR_NOTENABLED;
		}
	close(mixer);
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				auxMessage			[sample driver]
*/
DWORD auxMessage(WORD wDevID, WORD wMsg, DWORD dwUser, 
					DWORD dwParam1, DWORD dwParam2)
{
	printf("auxMessage(%u, %04X, %08X, %08X, %08X);\n", 
			wDevID, wMsg, dwUser, dwParam1, dwParam2);
	switch(wMsg) {
		case AUXDM_GETDEVCAPS:
			return AUX_GetDevCaps(wDevID, (LPAUXCAPS)dwParam1, dwParam2);
		case AUXDM_GETNUMDEVS:
			printf("AUX_GetNumDevs();\n");
			return 1L;
		case AUXDM_GETVOLUME:
			return AUX_GetVolume(wDevID, (LPDWORD)dwParam1);
		case AUXDM_SETVOLUME:
			return AUX_SetVolume(wDevID, dwParam1);
		}
	return MMSYSERR_NOTSUPPORTED;
}


#endif /* #ifdef BUILTIN_MMSYSTEM */
