/*
 * Sample Wine Driver for Open Sound System (featured in Linux and FreeBSD)
 *
 * Copyright 1994 Martin Ayotte
 *           1999 Eric Pouech (async playing in waveOut/waveIn)
 *	     2000 Eric Pouech (loops in waveOut)
 *           2002 Eric Pouech (full duplex)
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

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifdef HAVE_OSS

/* unless someone makes a wineserver kernel module, Unix pipes are faster than win32 events */
#define USE_PIPE_SYNC

#define MAX_WAVEDRV 	(6)
#define MAX_CHANNELS	6

/* states of the playing device */
#define	WINE_WS_PLAYING		0
#define	WINE_WS_PAUSED		1
#define	WINE_WS_STOPPED		2
#define WINE_WS_CLOSED		3

/* events to be send to device */
enum win_wm_message {
    WINE_WM_PAUSING = WM_USER + 1, WINE_WM_RESTARTING, WINE_WM_RESETTING, WINE_WM_HEADER,
    WINE_WM_UPDATE, WINE_WM_BREAKLOOP, WINE_WM_CLOSING, WINE_WM_STARTING, WINE_WM_STOPPING
};

#ifdef USE_PIPE_SYNC
#define SIGNAL_OMR(omr) do { int x = 0; write((omr)->msg_pipe[1], &x, sizeof(x)); } while (0)
#define CLEAR_OMR(omr) do { int x = 0; read((omr)->msg_pipe[0], &x, sizeof(x)); } while (0)
#define RESET_OMR(omr) do { } while (0)
#define WAIT_OMR(omr, sleep) \
  do { struct pollfd pfd; pfd.fd = (omr)->msg_pipe[0]; \
       pfd.events = POLLIN; poll(&pfd, 1, sleep); } while (0)
#else
#define SIGNAL_OMR(omr) do { SetEvent((omr)->msg_event); } while (0)
#define CLEAR_OMR(omr) do { } while (0)
#define RESET_OMR(omr) do { ResetEvent((omr)->msg_event); } while (0)
#define WAIT_OMR(omr, sleep) \
  do { WaitForSingleObject((omr)->msg_event, sleep); } while (0)
#endif

typedef struct {
    enum win_wm_message 	msg;	/* message identifier */
    DWORD	                param;  /* parameter for this message */
    HANDLE	                hEvent;	/* if message is synchronous, handle of event for synchro */
} OSS_MSG;

/* implement an in-process message ring for better performance
 * (compared to passing thru the server)
 * this ring will be used by the input (resp output) record (resp playback) routine
 */
#define OSS_RING_BUFFER_INCREMENT	64
typedef struct {
    int                         ring_buffer_size;
    OSS_MSG			* messages;
    int				msg_tosave;
    int				msg_toget;
#ifdef USE_PIPE_SYNC
    int				msg_pipe[2];
#else
    HANDLE			msg_event;
#endif
    CRITICAL_SECTION		msg_crst;
} OSS_MSG_RING;

typedef struct tagOSS_DEVICE {
    char*                       dev_name;
    char*                       mixer_name;
    char*                       interface_name;
    unsigned                    open_count;
    WAVEOUTCAPSW                out_caps;
    WAVEOUTCAPSW                duplex_out_caps;
    WAVEINCAPSW                 in_caps;
    DWORD                       in_caps_support;
    unsigned                    open_access;
    int                         fd;
    DWORD                       owner_tid;
    int                         sample_rate;
    int                         channels;
    int                         format;
    unsigned                    audio_fragment;
    BOOL                        full_duplex;
    BOOL                        bTriggerSupport;
    BOOL                        bOutputEnabled;
    BOOL                        bInputEnabled;
    DSDRIVERDESC                ds_desc;
    DSDRIVERCAPS                ds_caps;
    DSCDRIVERCAPS               dsc_caps;
} OSS_DEVICE;

typedef struct {
    OSS_DEVICE ossdev;
    volatile int		state;			/* one of the WINE_WS_ manifest constants */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    WAVEFORMATPCMEX             waveFormat;
    DWORD			volume;

    /* OSS information */
    DWORD			dwFragmentSize;		/* size of OSS buffer fragment */
    DWORD                       dwBufferSize;           /* size of whole OSS buffer in bytes */
    LPWAVEHDR			lpQueuePtr;		/* start of queued WAVEHDRs (waiting to be notified) */
    LPWAVEHDR			lpPlayPtr;		/* start of not yet fully played buffers */
    DWORD			dwPartialOffset;	/* Offset of not yet written bytes in lpPlayPtr */

    LPWAVEHDR			lpLoopPtr;              /* pointer of first buffer in loop, if any */
    DWORD			dwLoops;		/* private copy of loop counter */

    DWORD			dwPlayedTotal;		/* number of bytes actually played since opening */
    DWORD                       dwWrittenTotal;         /* number of bytes written to OSS buffer since opening */
    BOOL                        bNeedPost;              /* whether audio still needs to be physically started */

    /* synchronization stuff */
    HANDLE			hStartUpEvent;
    HANDLE			hThread;
    DWORD			dwThreadID;
    OSS_MSG_RING		msgRing;

    /* make accomodation for the inacuraccy of OSS when reporting buffer size remaining by using the clock instead of GETOSPACE */
    DWORD                       dwProjectedFinishTime;

} WINE_WAVEOUT;

typedef struct {
    OSS_DEVICE ossdev;
    volatile int		state;
    DWORD			dwFragmentSize;		/* OpenSound '/dev/dsp' give us that size */
    WAVEOPENDESC		waveDesc;
    WORD			wFlags;
    WAVEFORMATPCMEX             waveFormat;
    LPWAVEHDR			lpQueuePtr;
    DWORD			dwTotalRecorded;
    DWORD			dwTotalRead;

    /* synchronization stuff */
    HANDLE			hThread;
    DWORD			dwThreadID;
    HANDLE			hStartUpEvent;
    OSS_MSG_RING		msgRing;
} WINE_WAVEIN;

extern WINE_WAVEOUT	WOutDev[MAX_WAVEDRV];
extern WINE_WAVEIN	WInDev[MAX_WAVEDRV];
extern unsigned         numOutDev;
extern unsigned         numInDev;

extern int getEnables(OSS_DEVICE *ossdev);
extern void copy_format(LPWAVEFORMATEX wf1, LPWAVEFORMATPCMEX wf2);

extern DWORD OSS_OpenDevice(OSS_DEVICE* ossdev, unsigned req_access,
                            int* frag, int strict_format,
                            int sample_rate, int stereo, int fmt);

extern void OSS_CloseDevice(OSS_DEVICE* ossdev);

extern DWORD wodOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags);
extern DWORD wodSetVolume(WORD wDevID, DWORD dwParam);
extern DWORD widOpen(WORD wDevID, LPWAVEOPENDESC lpDesc, DWORD dwFlags);

/* dscapture.c */
extern DWORD widDsCreate(UINT wDevID, PIDSCDRIVER* drv);
extern DWORD widDsDesc(UINT wDevID, PDSDRIVERDESC desc);

/* dsrender.c */
extern DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv);
extern DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc);

#endif /* HAVE_OSS */
