/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2001 TransGaming Technologies, Inc.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Linux does not support better timing than 10ms */
#define DS_TIME_RES 10  /* Resolution of multimedia timer */
#define DS_TIME_DEL 10  /* Delay of multimedia timer callback, and duration of HEL fragment */

#define DS_HEL_FRAGS 48 /* HEL only: number of waveOut fragments in primary buffer
			 * (changing this won't help you) */

/* direct sound hardware acceleration levels */
#define DS_HW_ACCEL_FULL        0	/* default on Windows 98 */
#define DS_HW_ACCEL_STANDARD    1	/* default on Windows 2000 */
#define DS_HW_ACCEL_BASIC       2
#define DS_HW_ACCEL_EMULATION   3

extern int ds_emuldriver;
extern int ds_hel_margin;
extern int ds_hel_queue;
extern int ds_snd_queue_max;
extern int ds_snd_queue_min;
extern int ds_hw_accel;
extern int ds_default_playback;
extern int ds_default_capture;

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectSoundImpl IDirectSoundImpl;
typedef struct IDirectSoundBufferImpl IDirectSoundBufferImpl;
typedef struct IDirectSoundCaptureImpl IDirectSoundCaptureImpl;
typedef struct IDirectSoundCaptureBufferImpl IDirectSoundCaptureBufferImpl;
typedef struct IDirectSoundFullDuplexImpl IDirectSoundFullDuplexImpl;
typedef struct IDirectSoundNotifyImpl IDirectSoundNotifyImpl;
typedef struct IDirectSound3DListenerImpl IDirectSound3DListenerImpl;
typedef struct IDirectSound3DBufferImpl IDirectSound3DBufferImpl;
typedef struct IKsPropertySetImpl IKsPropertySetImpl;
typedef struct PrimaryBufferImpl PrimaryBufferImpl;

/*****************************************************************************
 * IDirectSound implementation structure
 */
struct IDirectSoundImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSound8);
    DWORD                       ref;
    /* IDirectSoundImpl fields */
    GUID                        guid;
    PIDSDRIVER                  driver;
    DSDRIVERDESC                drvdesc;
    DSDRIVERCAPS                drvcaps;
    DWORD                       priolevel;
    WAVEFORMATEX                wfx; /* current main waveformat */
    HWAVEOUT                    hwo;
    LPWAVEHDR                   pwave[DS_HEL_FRAGS];
    UINT                        timerID, pwplay, pwwrite, pwqueue, prebuf, precount;
    DWORD                       fraglen;
    PIDSDRIVERBUFFER            hwbuf;
    LPBYTE                      buffer;
    DWORD                       writelead, buflen, state, playpos, mixpos;
    BOOL                        need_remix;
    int                         nrofbuffers;
    IDirectSoundBufferImpl**    buffers;
    RTL_RWLOCK                  lock;
    CRITICAL_SECTION            mixlock;
    DSVOLUMEPAN                 volpan;
    PrimaryBufferImpl*          primary;
    DSBUFFERDESC                dsbd;

    /* DirectSound3DListener fields */
    IDirectSound3DListenerImpl*	listener;
    DS3DLISTENER                ds3dl;
    CRITICAL_SECTION            ds3dl_lock;
    BOOL                        ds3dl_need_recalc;
};

/* reference counted buffer memory for duplicated buffer memory */
typedef struct BufferMemory
{
    DWORD                       ref;
    LPBYTE                      memory;
} BufferMemory;

/*****************************************************************************
 * IDirectSoundBuffer implementation structure
 */
struct IDirectSoundBufferImpl
{
    /* FIXME: document */
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSoundBuffer8);
    DWORD                       ref;
    /* IDirectSoundBufferImpl fields */
    IDirectSoundImpl*           dsound;
    IDirectSound3DBufferImpl*   ds3db;
    IKsPropertySetImpl*         iks;
    CRITICAL_SECTION            lock;
    PIDSDRIVERBUFFER            hwbuf;
    WAVEFORMATEX                wfx;
    BufferMemory*               buffer;
    DWORD                       playflags,state,leadin;
    DWORD                       playpos,startpos,writelead,buflen;
    DWORD                       nAvgBytesPerSec;
    DWORD                       freq;
    DSVOLUMEPAN                 volpan, cvolpan;
    DSBUFFERDESC                dsbd;
    /* used for frequency conversion (PerfectPitch) */
    ULONG                       freqAdjust, freqAcc;
    /* used for intelligent (well, sort of) prebuffering */
    DWORD                       probably_valid_to, last_playpos;
    DWORD                       primary_mixpos, buf_mixpos;
    BOOL                        need_remix;
    /* IDirectSoundNotifyImpl fields */
    IDirectSoundNotifyImpl*     notify;

    /* DirectSound3DBuffer fields */
    DS3DBUFFER                  ds3db_ds3db;
    LONG                        ds3db_lVolume;
    BOOL                        ds3db_need_recalc;
};

HRESULT WINAPI SecondaryBuffer_Create(
	IDirectSoundImpl *This,
	IDirectSoundBufferImpl **pdsb,
	LPDSBUFFERDESC dsbd);

struct PrimaryBufferImpl {
    ICOM_VFIELD(IDirectSoundBuffer8);
    DWORD                       ref;
    IDirectSoundImpl*           dsound;
};

HRESULT WINAPI PrimaryBuffer_Create(
	IDirectSoundImpl *This,
	PrimaryBufferImpl **pdsb,
	LPDSBUFFERDESC dsbd);

/*****************************************************************************
 * IDirectSoundCapture implementation structure
 */
struct IDirectSoundCaptureImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSoundCapture);
    DWORD                              ref;

    /* IDirectSoundCaptureImpl fields */
    GUID                               guid;
    BOOL                               initialized;

    /* DirectSound driver stuff */
    PIDSCDRIVER                        driver;
    DSDRIVERDESC                       drvdesc;
    DSCDRIVERCAPS                      drvcaps;
    PIDSCDRIVERBUFFER                  hwbuf;

    /* wave driver info */
    HWAVEIN                            hwi;

    /* more stuff */
    LPBYTE                             buffer;
    DWORD                              buflen;
    DWORD                              read_position;

    /* FIXME: this should be a pointer because it can be bigger */
    WAVEFORMATEX                       wfx;

    IDirectSoundCaptureBufferImpl*     capture_buffer;
    DWORD                              state;
    LPWAVEHDR                          pwave;
    int                                nrofpwaves;
    int                                index;
    CRITICAL_SECTION                   lock;
};

/*****************************************************************************
 * IDirectSoundCaptureBuffer implementation structure
 */
struct IDirectSoundCaptureBufferImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSoundCaptureBuffer8);
    DWORD                               ref;

    /* IDirectSoundCaptureBufferImpl fields */
    IDirectSoundCaptureImpl*            dsound;
    /* FIXME: don't need this */
    LPDSCBUFFERDESC                     pdscbd;
    DWORD                               flags;
    /* IDirectSoundNotifyImpl fields */
    IDirectSoundNotifyImpl*             notify;
};

/*****************************************************************************
 * IDirectSoundFullDuplex implementation structure
 */
struct IDirectSoundFullDuplexImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSoundFullDuplex);
    DWORD                       ref;

    /* IDirectSoundFullDuplexImpl fields */
    CRITICAL_SECTION            lock;
};

/*****************************************************************************
 * IDirectSoundNotify implementation structure
 */
struct IDirectSoundNotifyImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSoundNotify);
    DWORD                       ref;
    /* IDirectSoundNotifyImpl fields */
    LPDSBPOSITIONNOTIFY         notifies;
    int                         nrofnotifies;

    PIDSDRIVERNOTIFY            hwnotify;
};

/*****************************************************************************
 *  IDirectSound3DListener implementation structure
 */
struct IDirectSound3DListenerImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSound3DListener);
    DWORD                       ref;
    /* IDirectSound3DListenerImpl fields */
    IDirectSoundImpl*           dsound;
};

HRESULT WINAPI IDirectSound3DListenerImpl_Create(
	PrimaryBufferImpl *This,
	IDirectSound3DListenerImpl **pdsl);

/*****************************************************************************
 *  IKsPropertySet implementation structure
 */
struct IKsPropertySetImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IKsPropertySet);
    DWORD			ref;
    /* IKsPropertySetImpl fields */
    IDirectSoundBufferImpl*	dsb;
};

HRESULT WINAPI IKsPropertySetImpl_Create(
	IDirectSoundBufferImpl *This,
	IKsPropertySetImpl **piks);

/*****************************************************************************
 * IDirectSound3DBuffer implementation structure
 */
struct IDirectSound3DBufferImpl
{
    /* IUnknown fields */
    ICOM_VFIELD(IDirectSound3DBuffer);
    DWORD                       ref;
    /* IDirectSound3DBufferImpl fields */
    IDirectSoundBufferImpl*     dsb;
    CRITICAL_SECTION            lock;
};

HRESULT WINAPI IDirectSound3DBufferImpl_Create(
	IDirectSoundBufferImpl *This,
	IDirectSound3DBufferImpl **pds3db);

void DSOUND_RecalcVolPan(PDSVOLUMEPAN volpan);
void DSOUND_RecalcFormat(IDirectSoundBufferImpl *dsb);

/* primary.c */

HRESULT DSOUND_PrimaryCreate(IDirectSoundImpl *This);
HRESULT DSOUND_PrimaryDestroy(IDirectSoundImpl *This);
HRESULT DSOUND_PrimaryPlay(IDirectSoundImpl *This);
HRESULT DSOUND_PrimaryStop(IDirectSoundImpl *This);
HRESULT DSOUND_PrimaryGetPosition(IDirectSoundImpl *This, LPDWORD playpos, LPDWORD writepos);

/* buffer.c */

DWORD DSOUND_CalcPlayPosition(IDirectSoundBufferImpl *This,
			      DWORD state, DWORD pplay, DWORD pwrite, DWORD pmix, DWORD bmix);

/* mixer.c */

void DSOUND_CheckEvent(IDirectSoundBufferImpl *dsb, int len);
void DSOUND_ForceRemix(IDirectSoundBufferImpl *dsb);
void DSOUND_MixCancelAt(IDirectSoundBufferImpl *dsb, DWORD buf_writepos);
void DSOUND_WaveQueue(IDirectSoundImpl *dsound, DWORD mixq);
void DSOUND_PerformMix(void);
void CALLBACK DSOUND_timer(UINT timerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2);
void CALLBACK DSOUND_callback(HWAVEOUT hwo, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2);

/* sound3d.c */

void DSOUND_Calc3DBuffer(IDirectSoundBufferImpl *dsb);

#define STATE_STOPPED   0
#define STATE_STARTING  1
#define STATE_PLAYING   2
#define STATE_CAPTURING 2
#define STATE_STOPPING  3

#define DSOUND_FREQSHIFT (14)

extern IDirectSoundImpl* dsound;
extern IDirectSoundCaptureImpl* dsound_capture;

extern ICOM_VTABLE(IDirectSoundNotify) dsnvt;
extern HRESULT mmErr(UINT err);
extern void setup_dsound_options(void);
