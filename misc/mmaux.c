/*
 * Sample AUXILARY Wine Driver for Linux
 *
 * Copyright 1994 Martin Ayotte
 */
#ifndef WINELIB
static char Copyright[] = "Copyright  Martin Ayotte, 1994";

#include "stdio.h"
#include "win.h"
#include "user.h"
#include "driver.h"
#include "mmsystem.h"

#include <fcntl.h>
#include <sys/ioctl.h>
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
		printf("AUX_GetDevCaps // unable read mixer !\n");
		return MMSYSERR_NOTENABLED;
		}
	close(mixer);
	return MMSYSERR_NOERROR;
#else
	return MMSYSERR_NOTENABLED;
#endif
}


/**************************************************************************
* 				AUX_GetVolume			[internal]
*/
DWORD AUX_GetVolume(WORD wDevID, DWORD dwParam)
{
#ifdef linux
	int 	mixer;
	int		volume;
	printf("AUX_GetVolume(%u, %08X);\n", wDevID, dwParam);
	if ((mixer = open(MIXER_DEV, O_RDWR)) < 0) {
		printf("Linux 'AUX_GetVolume' // mixer device not available !\n");
		return MMSYSERR_NOTENABLED;
		}
    if (ioctl(mixer, SOUND_MIXER_READ_LINE, &volume) == -1) {
		printf("Linux 'AUX_GetVolume' // unable read mixer !\n");
		return MMSYSERR_NOTENABLED;
		}
	close(mixer);
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
			return 0L;
		case AUXDM_GETVOLUME:
			return AUX_GetVolume(wDevID, dwParam1);
		case AUXDM_SETVOLUME:
			return AUX_SetVolume(wDevID, dwParam1);
		}
	return MMSYSERR_NOTSUPPORTED;
}


#endif /* !WINELIB */
