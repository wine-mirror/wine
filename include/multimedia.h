/*****************************************************************************
 * Copyright 1998, Luiz Otavio L. Zorzella
 *
 * File:      multimedia.h
 * Purpose:   multimedia declarations
 *
 *****************************************************************************
 */
#ifndef __WINE_MULTIMEDIA_H 
#define __WINE_MULTIMEDIA_H

#include "mmsystem.h"

#define MAX_MIDIINDRV 	(16)
/* For now I'm making 16 the maximum number of midi devices one can
 * have. This should be more than enough for everybody. But as a purist,
 * I intend to make it unbounded in the future, as soon as I figure
 * a good way to do so.
 */
#define MAX_MIDIOUTDRV 	(16)
#define MAX_MCIMIDIDRV 	(1)

#ifdef HAVE_SYS_SOUNDCARD_H
# include <sys/soundcard.h>
#endif
#ifdef HAVE_MACHINE_SOUNDCARD_H
# include <machine/soundcard.h>
#endif

#include <sys/errno.h>

#ifdef HAVE_OSS
#define MIDI_SEQ "/dev/sequencer"
#else
#define MIDI_DEV "/dev/midi"
#endif

#ifdef SOUND_VERSION
#define IOCTL(a,b,c)		ioctl(a,b,&c)
#else
#define IOCTL(a,b,c)		(c = ioctl(a,b,c) )
#endif

typedef struct {
#ifndef HAVE_OSS
	int		unixdev;
#endif
	int		state;
	DWORD		bufsize;
	LPMIDIOPENDESC	midiDesc;
	WORD		wFlags;
	LPMIDIHDR 	lpQueueHdr;
	DWORD		dwTotalPlayed;
#ifdef HAVE_OSS
	unsigned char	incoming[3];
	unsigned char	incPrev;
	char		incLen;
	DWORD		startTime;
#endif
} LINUX_MIDIIN;

typedef struct {
#ifndef HAVE_OSS
	int		unixdev;
#endif
	int		state;
	DWORD		bufsize;
	LPMIDIOPENDESC	midiDesc;
	WORD		wFlags;
	LPMIDIHDR 	lpQueueHdr;
	DWORD		dwTotalPlayed;
#ifdef HAVE_OSS
	void*		lpExtra;	 	/* according to port type (MIDI, FM...), extra data when needed */
#endif
} LINUX_MIDIOUT;

typedef struct {
	int		nUseCount;          	/* Incremented for each shared open */
	BOOL16		fShareable;         	/* TRUE if first open was shareable */
	WORD		wNotifyDeviceID;    	/* MCI device ID with a pending notification */
	HANDLE16 	hCallback;         	/* Callback handle for pending notification */
	HMMIO16		hFile;	            	/* mmio file handle open as Element		*/
	DWORD		dwBeginData;
	DWORD		dwTotalLen;
	WORD		wFormat;
	WORD		nTracks;
	WORD		nTempo;
	MCI_OPEN_PARMS16 openParms;
/* 	MIDIHDR	MidiHdr; */
	HLOCAL16	hMidiHdr;
	WORD		dwStatus;
} LINUX_MCIMIDI;

/* function prototypes */
extern BOOL32 MULTIMEDIA_Init( void );

#endif /* __WINE_MULTIMEDIA_H */
