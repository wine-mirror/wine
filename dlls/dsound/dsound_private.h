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
typedef struct IDirectSoundImpl              IDirectSoundImpl;
typedef struct IDirectSound_IUnknown         IDirectSound_IUnknown;
typedef struct IDirectSound_IDirectSound     IDirectSound_IDirectSound;
typedef struct IDirectSound8_IUnknown        IDirectSound8_IUnknown;
typedef struct IDirectSound8_IDirectSound    IDirectSound8_IDirectSound;
typedef struct IDirectSound8_IDirectSound8   IDirectSound8_IDirectSound8;
typedef struct IDirectSoundBufferImpl        IDirectSoundBufferImpl;
typedef struct IDirectSoundCaptureImpl       IDirectSoundCaptureImpl;
typedef struct IDirectSoundCaptureBufferImpl IDirectSoundCaptureBufferImpl;
typedef struct IDirectSoundFullDuplexImpl    IDirectSoundFullDuplexImpl;
typedef struct IDirectSoundFullDuplex_IUnknown IDirectSoundFullDuplex_IUnknown;
typedef struct IDirectSoundFullDuplex_IDirectSound IDirectSoundFullDuplex_IDirectSound;
typedef struct IDirectSoundFullDuplex_IDirectSound8 IDirectSoundFullDuplex_IDirectSound8;
typedef struct IDirectSoundFullDuplex_IDirectSoundCapture IDirectSoundFullDuplex_IDirectSoundCapture;
typedef struct IDirectSoundNotifyImpl        IDirectSoundNotifyImpl;
typedef struct IDirectSoundCaptureNotifyImpl IDirectSoundCaptureNotifyImpl;
typedef struct IDirectSound3DListenerImpl    IDirectSound3DListenerImpl;
typedef struct IDirectSound3DBufferImpl      IDirectSound3DBufferImpl;
typedef struct IKsBufferPropertySetImpl      IKsBufferPropertySetImpl;
typedef struct IKsPrivatePropertySetImpl     IKsPrivatePropertySetImpl;
typedef struct PrimaryBufferImpl             PrimaryBufferImpl;
typedef struct SecondaryBufferImpl           SecondaryBufferImpl;
typedef struct IClassFactoryImpl             IClassFactoryImpl;
typedef struct DirectSoundDevice             DirectSoundDevice;
typedef struct DirectSoundCaptureDevice      DirectSoundCaptureDevice;

/*****************************************************************************
 * IDirectSound implementation structure
 */
struct IDirectSoundImpl
{
    LONG                        ref;

    DirectSoundDevice          *device;
    LPUNKNOWN                   pUnknown;
    LPDIRECTSOUND               pDS;
    LPDIRECTSOUND8              pDS8;
};

HRESULT IDirectSoundImpl_Create(
    LPDIRECTSOUND8 * ppds);

/*****************************************************************************
 * IDirectSoundDevice implementation structure
 */
struct DirectSoundDevice
{
    LONG                        ref;

    GUID                        guid;
    PIDSDRIVER                  driver;
    DSDRIVERDESC                drvdesc;
    DSDRIVERCAPS                drvcaps;
    DWORD                       priolevel;
    PWAVEFORMATEX               pwfx;
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
    RTL_RWLOCK                  buffer_list_lock;
    CRITICAL_SECTION            mixlock;
    PrimaryBufferImpl*          primary;
    DSBUFFERDESC                dsbd;
    DWORD                       speaker_config;
    LPBYTE                      tmp_buffer;
    DWORD                       tmp_buffer_len;

    /* DirectSound3DListener fields */
    IDirectSound3DListenerImpl*	listener;
    DS3DLISTENER                ds3dl;
    BOOL                        ds3dl_need_recalc;
};

/* reference counted buffer memory for duplicated buffer memory */
typedef struct BufferMemory
{
    LONG                        ref;
    LPBYTE                      memory;
} BufferMemory;

HRESULT DirectSoundDevice_Create(DirectSoundDevice ** ppDevice);
ULONG DirectSoundDevice_AddRef(DirectSoundDevice * device);
ULONG DirectSoundDevice_Release(DirectSoundDevice * device);
HRESULT DirectSoundDevice_Initialize(
    DirectSoundDevice ** ppDevice,
    LPCGUID lpcGUID);
HRESULT DirectSoundDevice_AddBuffer(
    DirectSoundDevice * device,
    IDirectSoundBufferImpl * pDSB);
HRESULT DirectSoundDevice_RemoveBuffer(
    DirectSoundDevice * device,
    IDirectSoundBufferImpl * pDSB);
HRESULT DirectSoundDevice_GetCaps(DirectSoundDevice * device, LPDSCAPS lpDSCaps);
HRESULT DirectSoundDevice_CreateSoundBuffer(
    DirectSoundDevice * device,
    LPCDSBUFFERDESC dsbd,
    LPLPDIRECTSOUNDBUFFER ppdsb,
    LPUNKNOWN lpunk,
    BOOL from8);
HRESULT DirectSoundDevice_DuplicateSoundBuffer(
    DirectSoundDevice * device,
    LPDIRECTSOUNDBUFFER psb,
    LPLPDIRECTSOUNDBUFFER ppdsb);
HRESULT DirectSoundDevice_SetCooperativeLevel(
    DirectSoundDevice * devcie,
    HWND hwnd,
    DWORD level);
HRESULT DirectSoundDevice_Compact(DirectSoundDevice * device);
HRESULT DirectSoundDevice_GetSpeakerConfig(
    DirectSoundDevice * device,
    LPDWORD lpdwSpeakerConfig);
HRESULT DirectSoundDevice_SetSpeakerConfig(
    DirectSoundDevice * device,
    DWORD config);
HRESULT DirectSoundDevice_VerifyCertification(
    DirectSoundDevice * device,
    LPDWORD pdwCertified);

/*****************************************************************************
 * IDirectSound COM components
 */
struct IDirectSound_IUnknown {
    const IUnknownVtbl         *lpVtbl;
    LONG                        ref;
    LPDIRECTSOUND8              pds;
};

HRESULT IDirectSound_IUnknown_Create(
    LPDIRECTSOUND8 pds,
    LPUNKNOWN * ppunk);

struct IDirectSound_IDirectSound {
    const IDirectSoundVtbl     *lpVtbl;
    LONG                        ref;
    LPDIRECTSOUND8              pds;
};

HRESULT IDirectSound_IDirectSound_Create(
    LPDIRECTSOUND8 pds,
    LPDIRECTSOUND * ppds);

/*****************************************************************************
 * IDirectSound8 COM components
 */
struct IDirectSound8_IUnknown {
    const IUnknownVtbl         *lpVtbl;
    LONG                        ref;
    LPDIRECTSOUND8              pds;
};

HRESULT IDirectSound8_IUnknown_Create(
    LPDIRECTSOUND8 pds,
    LPUNKNOWN * ppunk);

struct IDirectSound8_IDirectSound {
    const IDirectSoundVtbl     *lpVtbl;
    LONG                        ref;
    LPDIRECTSOUND8              pds;
};

HRESULT IDirectSound8_IDirectSound_Create(
    LPDIRECTSOUND8 pds,
    LPDIRECTSOUND * ppds);

struct IDirectSound8_IDirectSound8 {
    const IDirectSound8Vtbl    *lpVtbl;
    LONG                        ref;
    LPDIRECTSOUND8              pds;
};

HRESULT IDirectSound8_IDirectSound8_Create(
    LPDIRECTSOUND8 pds,
    LPDIRECTSOUND8 * ppds);

/*****************************************************************************
 * IDirectSoundBuffer implementation structure
 */
struct IDirectSoundBufferImpl
{
    /* FIXME: document */
    /* IUnknown fields */
    const IDirectSoundBuffer8Vtbl *lpVtbl;
    LONG                        ref;
    /* IDirectSoundBufferImpl fields */
    SecondaryBufferImpl*        secondary;
    DirectSoundDevice*          device;
    CRITICAL_SECTION            lock;
    PIDSDRIVERBUFFER            hwbuf;
    PWAVEFORMATEX               pwfx;
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
    LPDSBPOSITIONNOTIFY         notifies;
    int                         nrofnotifies;
    PIDSDRIVERNOTIFY            hwnotify;

    /* DirectSound3DBuffer fields */
    IDirectSound3DBufferImpl*   ds3db;
    DS3DBUFFER                  ds3db_ds3db;
    LONG                        ds3db_lVolume;
    BOOL                        ds3db_need_recalc;

    /* IKsPropertySet fields */
    IKsBufferPropertySetImpl*   iks;
};

HRESULT IDirectSoundBufferImpl_Create(
    DirectSoundDevice *device,
    IDirectSoundBufferImpl **ppdsb,
    LPCDSBUFFERDESC dsbd);
HRESULT IDirectSoundBufferImpl_Destroy(
    IDirectSoundBufferImpl *pdsb);
HRESULT IDirectSoundBufferImpl_Duplicate(
    DirectSoundDevice *device,
    IDirectSoundBufferImpl **ppdsb,
    IDirectSoundBufferImpl *pdsb);

/*****************************************************************************
 * SecondaryBuffer implementation structure
 */
struct SecondaryBufferImpl
{
    const IDirectSoundBuffer8Vtbl *lpVtbl;
    LONG                        ref;
    IDirectSoundBufferImpl*     dsb;
};

HRESULT SecondaryBufferImpl_Create(
    IDirectSoundBufferImpl *dsb,
    SecondaryBufferImpl **pdsb);
HRESULT SecondaryBufferImpl_Destroy(
    SecondaryBufferImpl *pdsb);

/*****************************************************************************
 * PrimaryBuffer implementation structure
 */
struct PrimaryBufferImpl
{
    const IDirectSoundBuffer8Vtbl *lpVtbl;
    LONG                        ref;
    DirectSoundDevice*          device;
};

HRESULT PrimaryBufferImpl_Create(
    DirectSoundDevice * device,
    PrimaryBufferImpl **ppdsb,
    LPCDSBUFFERDESC dsbd);

/*****************************************************************************
 * IDirectSoundCapture implementation structure
 */
struct IDirectSoundCaptureImpl
{
    /* IUnknown fields */
    const IDirectSoundCaptureVtbl     *lpVtbl;
    LONG                               ref;

    DirectSoundCaptureDevice          *device;
};

HRESULT IDirectSoundCaptureImpl_Create(
    LPDIRECTSOUNDCAPTURE8 * ppds);

/*****************************************************************************
 * DirectSoundCaptureDevice implementation structure
 */
struct DirectSoundCaptureDevice
{
    /* IDirectSoundCaptureImpl fields */
    GUID                               guid;
    LONG                               ref;

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

    PWAVEFORMATEX                      pwfx;

    IDirectSoundCaptureBufferImpl*     capture_buffer;
    DWORD                              state;
    LPWAVEHDR                          pwave;
    int                                nrofpwaves;
    int                                index;
    CRITICAL_SECTION                   lock;
};

HRESULT DirectSoundCaptureDevice_Create(
    DirectSoundCaptureDevice ** ppDevice);
HRESULT DirectSoundCaptureDevice_Initialize(
    DirectSoundCaptureDevice ** ppDevice,
    LPCGUID lpcGUID);
ULONG DirectSoundCaptureDevice_AddRef(
    DirectSoundCaptureDevice * device);
ULONG DirectSoundCaptureDevice_Release(
    DirectSoundCaptureDevice * device);

/*****************************************************************************
 * IDirectSoundCaptureBuffer implementation structure
 */
struct IDirectSoundCaptureBufferImpl
{
    /* IUnknown fields */
    const IDirectSoundCaptureBuffer8Vtbl *lpVtbl;
    LONG                                ref;

    /* IDirectSoundCaptureBufferImpl fields */
    DirectSoundCaptureDevice*           device;
    /* FIXME: don't need this */
    LPDSCBUFFERDESC                     pdscbd;
    DWORD                               flags;

    /* IDirectSoundCaptureNotifyImpl fields */
    IDirectSoundCaptureNotifyImpl*      notify;
    LPDSBPOSITIONNOTIFY                 notifies;
    int                                 nrofnotifies;
    PIDSDRIVERNOTIFY                    hwnotify;
};

HRESULT IDirectSoundCaptureBufferImpl_Create(
    DirectSoundCaptureDevice *device,
    IDirectSoundCaptureBufferImpl ** ppobj,
    LPCDSCBUFFERDESC lpcDSCBufferDesc);

/*****************************************************************************
 * IDirectSoundFullDuplex implementation structure
 */
struct IDirectSoundFullDuplexImpl
{
    /* IUnknown fields */
    const IDirectSoundFullDuplexVtbl *lpVtbl;
    LONG                              ref;

    /* IDirectSoundFullDuplexImpl fields */
    DirectSoundDevice                *renderer_device;
    DirectSoundCaptureDevice         *capture_device;

    LPUNKNOWN                         pUnknown;
    LPDIRECTSOUND                     pDS;
    LPDIRECTSOUND8                    pDS8;
    LPDIRECTSOUNDCAPTURE              pDSC;
};

/*****************************************************************************
 * IDirectSoundFullDuplex COM components
 */
struct IDirectSoundFullDuplex_IUnknown {
    const IUnknownVtbl         *lpVtbl;
    LONG                        ref;
    IDirectSoundFullDuplexImpl *pdsfd;
};

HRESULT IDirectSoundFullDuplex_IUnknown_Create(
    LPDIRECTSOUNDFULLDUPLEX pdsfd,
    LPUNKNOWN * ppunk);

struct IDirectSoundFullDuplex_IDirectSound {
    const IDirectSoundVtbl     *lpVtbl;
    LONG                        ref;
    IDirectSoundFullDuplexImpl *pdsfd;
};

HRESULT IDirectSoundFullDuplex_IDirectSound_Create(
    LPDIRECTSOUNDFULLDUPLEX pdsfd,
    LPDIRECTSOUND * ppds);

struct IDirectSoundFullDuplex_IDirectSound8 {
    const IDirectSound8Vtbl    *lpVtbl;
    LONG                        ref;
    IDirectSoundFullDuplexImpl *pdsfd;
};

HRESULT IDirectSoundFullDuplex_IDirectSound8_Create(
    LPDIRECTSOUNDFULLDUPLEX pdsfd,
    LPDIRECTSOUND8 * ppds8);

struct IDirectSoundFullDuplex_IDirectSoundCapture {
    const IDirectSoundCaptureVtbl *lpVtbl;
    LONG                           ref;
    IDirectSoundFullDuplexImpl    *pdsfd;
};

HRESULT IDirectSoundFullDuplex_IDirectSoundCapture_Create(
    LPDIRECTSOUNDFULLDUPLEX pdsfd,
    LPDIRECTSOUNDCAPTURE8 * ppdsc8);

/*****************************************************************************
 * IDirectSoundNotify implementation structure
 */
struct IDirectSoundNotifyImpl
{
    /* IUnknown fields */
    const IDirectSoundNotifyVtbl *lpVtbl;
    LONG                        ref;
    IDirectSoundBufferImpl*     dsb;
};

HRESULT IDirectSoundNotifyImpl_Create(
    IDirectSoundBufferImpl *dsb,
    IDirectSoundNotifyImpl **pdsn);
HRESULT IDirectSoundNotifyImpl_Destroy(
    IDirectSoundNotifyImpl *pdsn);

/*****************************************************************************
 * IDirectSoundCaptureNotify implementation structure
 */
struct IDirectSoundCaptureNotifyImpl
{
    /* IUnknown fields */
    const IDirectSoundNotifyVtbl       *lpVtbl;
    LONG                                ref;
    IDirectSoundCaptureBufferImpl*      dscb;
};

HRESULT IDirectSoundCaptureNotifyImpl_Create(
    IDirectSoundCaptureBufferImpl *dscb,
    IDirectSoundCaptureNotifyImpl ** pdscn);

/*****************************************************************************
 *  IDirectSound3DListener implementation structure
 */
struct IDirectSound3DListenerImpl
{
    /* IUnknown fields */
    const IDirectSound3DListenerVtbl *lpVtbl;
    LONG                        ref;
    /* IDirectSound3DListenerImpl fields */
    DirectSoundDevice*          device;
};

HRESULT IDirectSound3DListenerImpl_Create(
    DirectSoundDevice           *device,
    IDirectSound3DListenerImpl **pdsl);

/*****************************************************************************
 *  IKsBufferPropertySet implementation structure
 */
struct IKsBufferPropertySetImpl
{
    /* IUnknown fields */
    const IKsPropertySetVtbl   *lpVtbl;
    LONG 			ref;
    /* IKsPropertySetImpl fields */
    IDirectSoundBufferImpl*	dsb;
};

HRESULT IKsBufferPropertySetImpl_Create(
    IDirectSoundBufferImpl *dsb,
    IKsBufferPropertySetImpl **piks);
HRESULT IKsBufferPropertySetImpl_Destroy(
    IKsBufferPropertySetImpl *piks);

/*****************************************************************************
 *  IKsPrivatePropertySet implementation structure
 */
struct IKsPrivatePropertySetImpl
{
    /* IUnknown fields */
    const IKsPropertySetVtbl   *lpVtbl;
    LONG 			ref;
};

HRESULT IKsPrivatePropertySetImpl_Create(
    IKsPrivatePropertySetImpl **piks);

/*****************************************************************************
 * IDirectSound3DBuffer implementation structure
 */
struct IDirectSound3DBufferImpl
{
    /* IUnknown fields */
    const IDirectSound3DBufferVtbl *lpVtbl;
    LONG                        ref;
    /* IDirectSound3DBufferImpl fields */
    IDirectSoundBufferImpl*     dsb;
};

HRESULT IDirectSound3DBufferImpl_Create(
    IDirectSoundBufferImpl *dsb,
    IDirectSound3DBufferImpl **pds3db);
HRESULT IDirectSound3DBufferImpl_Destroy(
    IDirectSound3DBufferImpl *pds3db);

/*******************************************************************************
 * DirectSound ClassFactory implementation structure
 */
struct IClassFactoryImpl
{
    /* IUnknown fields */
    const IClassFactoryVtbl    *lpVtbl;
    LONG                        ref;
};

extern IClassFactoryImpl DSOUND_CAPTURE_CF;
extern IClassFactoryImpl DSOUND_FULLDUPLEX_CF;

/*******************************************************************************
 */

/* dsound.c */

HRESULT DSOUND_Create(LPDIRECTSOUND *ppDS, IUnknown *pUnkOuter);
HRESULT DSOUND_Create8(LPDIRECTSOUND8 *ppDS, IUnknown *pUnkOuter);

/* primary.c */

HRESULT DSOUND_PrimaryCreate(DirectSoundDevice *device);
HRESULT DSOUND_PrimaryDestroy(DirectSoundDevice *device);
HRESULT DSOUND_PrimaryPlay(DirectSoundDevice *device);
HRESULT DSOUND_PrimaryStop(DirectSoundDevice *device);
HRESULT DSOUND_PrimaryGetPosition(DirectSoundDevice *device, LPDWORD playpos, LPDWORD writepos);
HRESULT DSOUND_PrimarySetFormat(DirectSoundDevice *device, LPCWAVEFORMATEX wfex);

/* buffer.c */

DWORD DSOUND_CalcPlayPosition(IDirectSoundBufferImpl *This, DWORD pplay, DWORD pwrite);

/* mixer.c */

void DSOUND_CheckEvent(IDirectSoundBufferImpl *dsb, int len);
void DSOUND_ForceRemix(IDirectSoundBufferImpl *dsb);
void DSOUND_MixCancelAt(IDirectSoundBufferImpl *dsb, DWORD buf_writepos);
void DSOUND_WaveQueue(DirectSoundDevice *device, DWORD mixq);
void DSOUND_PerformMix(DirectSoundDevice *device);
void DSOUND_RecalcVolPan(PDSVOLUMEPAN volpan);
void DSOUND_AmpFactorToVolPan(PDSVOLUMEPAN volpan);
void DSOUND_RecalcFormat(IDirectSoundBufferImpl *dsb);
void CALLBACK DSOUND_timer(UINT timerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2);
void CALLBACK DSOUND_callback(HWAVEOUT hwo, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2);

/* sound3d.c */

void DSOUND_Calc3DBuffer(IDirectSoundBufferImpl *dsb);

/* duplex.c */

HRESULT DSOUND_FullDuplexCreate(LPDIRECTSOUNDFULLDUPLEX* ppDSFD, IUnknown *pUnkOuter);

/* capture.c */

HRESULT WINAPI IDirectSoundCaptureImpl_CreateCaptureBuffer(
    LPDIRECTSOUNDCAPTURE iface,
    LPCDSCBUFFERDESC lpcDSCBufferDesc,
    LPDIRECTSOUNDCAPTUREBUFFER* lplpDSCaptureBuffer,
    LPUNKNOWN pUnk);
HRESULT WINAPI IDirectSoundCaptureImpl_GetCaps(
    LPDIRECTSOUNDCAPTURE iface,
    LPDSCCAPS lpDSCCaps);
HRESULT WINAPI IDirectSoundCaptureImpl_Initialize(
    LPDIRECTSOUNDCAPTURE iface,
    LPCGUID lpcGUID);

#define STATE_STOPPED   0
#define STATE_STARTING  1
#define STATE_PLAYING   2
#define STATE_CAPTURING 2
#define STATE_STOPPING  3

#define DSOUND_FREQSHIFT (14)

extern DirectSoundDevice* DSOUND_renderer[MAXWAVEDRIVERS];
extern GUID DSOUND_renderer_guids[MAXWAVEDRIVERS];

extern DirectSoundCaptureDevice * DSOUND_capture[MAXWAVEDRIVERS];
extern GUID DSOUND_capture_guids[MAXWAVEDRIVERS];

HRESULT mmErr(UINT err);
void setup_dsound_options(void);
const char * get_device_id(LPCGUID pGuid);
const char * dumpCooperativeLevel(DWORD level);
