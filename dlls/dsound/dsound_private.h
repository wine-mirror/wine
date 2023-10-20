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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Linux does not support better timing than 10ms */
#define DS_TIME_RES 2  /* Resolution of multimedia timer */
#define DS_TIME_DEL 10  /* Delay of multimedia timer callback, and duration of HEL fragment */

#include "wingdi.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "mediaobj.h"
#include "mmsystem.h"
#include "uuids.h"

#include "wine/list.h"

#define DS_MAX_CHANNELS 6

extern int ds_hel_buflen;

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectSoundBufferImpl        IDirectSoundBufferImpl;
typedef struct DirectSoundDevice             DirectSoundDevice;

/* dsound_convert.h */
typedef float (*bitsgetfunc)(const IDirectSoundBufferImpl *, BYTE *, DWORD);
typedef void (*bitsputfunc)(const IDirectSoundBufferImpl *, DWORD, DWORD, float);
extern const bitsgetfunc getbpp[5];
void putieee32(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value);
void putieee32_sum(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value);
void mixieee32(float *src, float *dst, unsigned samples);
typedef void (*normfunc)(const void *, void *, unsigned);
extern const normfunc normfunctions[4];

typedef struct _DSVOLUMEPAN
{
    DWORD	dwTotalAmpFactor[DS_MAX_CHANNELS];
    LONG	lVolume;
    LONG	lPan;
} DSVOLUMEPAN,*PDSVOLUMEPAN;

typedef struct DSFilter {
    GUID guid;
    IMediaObject* obj;
    IMediaObjectInPlace* inplace;
} DSFilter;

/*****************************************************************************
 * IDirectSoundDevice implementation structure
 */
struct DirectSoundDevice
{
    LONG                        ref;

    GUID                        guid;
    DSCAPS                      drvcaps;
    DWORD                       priolevel, sleeptime;
    PWAVEFORMATEX               pwfx, primary_pwfx;
    LPBYTE                      buffer;
    DWORD                       writelead, buflen, ac_frames, frag_frames, playpos, pad, stopped;
    int                         nrofbuffers;
    IDirectSoundBufferImpl**    buffers;
    SRWLOCK                     buffer_list_lock;
    CRITICAL_SECTION            mixlock;
    IDirectSoundBufferImpl     *primary;
    DWORD                       speaker_config;
    float                       speaker_angles[DS_MAX_CHANNELS];
    int                         speaker_num[DS_MAX_CHANNELS];
    int                         num_speakers;
    int                         lfe_channel;
    float *tmp_buffer, *cp_buffer;
    DWORD                       tmp_buffer_len, cp_buffer_len;

    DSVOLUMEPAN                 volpan;

    normfunc normfunction;

    /* DirectSound3DListener fields */
    DS3DLISTENER                ds3dl;
    BOOL                        ds3dl_need_recalc;

    IMMDevice *mmdevice;
    IAudioClient *client;
    IAudioStreamVolume *volume;
    IAudioRenderClient *render;

    HANDLE sleepev, thread;
    struct list entry;
};

/* reference counted buffer memory for duplicated buffer memory */
typedef struct BufferMemory
{
    LONG                        ref;
    LONG                        lockedbytes;
    LPBYTE                      memory;
    struct list buffers;
} BufferMemory;

HRESULT DirectSoundDevice_AddBuffer(
    DirectSoundDevice * device,
    IDirectSoundBufferImpl * pDSB);
void DirectSoundDevice_RemoveBuffer(DirectSoundDevice * device, IDirectSoundBufferImpl * pDSB);

/*****************************************************************************
 * IDirectSoundBuffer implementation structure
 */
struct IDirectSoundBufferImpl
{
    IDirectSoundBuffer8         IDirectSoundBuffer8_iface;
    IDirectSoundNotify          IDirectSoundNotify_iface;
    IDirectSound3DListener      IDirectSound3DListener_iface; /* only primary buffer */
    IDirectSound3DBuffer        IDirectSound3DBuffer_iface; /* only secondary buffer */
    IKsPropertySet              IKsPropertySet_iface;
    LONG                        numIfaces; /* "in use interfaces" refcount */
    LONG                        ref, refn, ref3D, refiks;
    /* IDirectSoundBufferImpl fields */
    DirectSoundDevice*          device;
    SRWLOCK                     lock;
    PWAVEFORMATEX               pwfx;
    BufferMemory*               buffer;
    DWORD                       playflags,state,leadin;
    DWORD                       writelead,maxwritelead,buflen;
    DWORD                       freq;
    DSVOLUMEPAN                 volpan;
    DSBUFFERDESC                dsbd;
    /* used for frequency conversion (PerfectPitch) */
    ULONG                       freqneeded;
    DWORD                       firstep;
    float                       firgain;
    LONG64                      freqAdjustNum,freqAdjustDen;
    LONG64                      freqAccNum;
    /* used for mixing */
    DWORD                       sec_mixpos;
    /* Holds a copy of the next 'writelead' bytes, to be used for mixing. This makes it
     * so that these bytes get played once even if this region of the buffer gets overwritten,
     * which is more in-line with native DirectSound behavior. */
    BOOL                        use_committed;
    LPVOID                      committedbuff;
    DWORD                       committed_mixpos;
    /* IDirectSoundNotify fields */
    LPDSBPOSITIONNOTIFY         notifies;
    int                         nrofnotifies;
    /* DirectSound3DBuffer fields */
    DS3DBUFFER                  ds3db_ds3db;
    LONG                        ds3db_lVolume;
    DWORD                       ds3db_freq;
    BOOL                        ds3db_need_recalc;
    /* Used for bit depth conversion */
    int                         mix_channels;
    bitsgetfunc get, get_aux;
    bitsputfunc put, put_aux;
    int                         num_filters;
    DSFilter*                   filters;

    struct list entry;
};

float get_mono(const IDirectSoundBufferImpl *dsb, BYTE *base, DWORD channel);
void put_mono2stereo(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value);
void put_mono2quad(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value);
void put_stereo2quad(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value);
void put_mono2surround51(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value);
void put_stereo2surround51(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value);
void put_surround512stereo(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value);
void put_surround712stereo(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value);
void put_quad2stereo(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value);

HRESULT secondarybuffer_create(DirectSoundDevice *device, const DSBUFFERDESC *dsbd,
        IDirectSoundBuffer **buffer);
HRESULT IDirectSoundBufferImpl_Duplicate(
    DirectSoundDevice *device,
    IDirectSoundBufferImpl **ppdsb,
    IDirectSoundBufferImpl *pdsb);
void secondarybuffer_destroy(IDirectSoundBufferImpl *This);
BOOL secondarybuffer_is_audible(IDirectSoundBufferImpl *This);
extern const IDirectSound3DListenerVtbl ds3dlvt;
extern const IDirectSound3DBufferVtbl ds3dbvt;
extern const IKsPropertySetVtbl iksbvt;

HRESULT IKsPrivatePropertySetImpl_Create(REFIID riid, void **ppv);

/*******************************************************************************
 */

/* dsound.c */

HRESULT DSOUND_Create(REFIID riid, void **ppv);
HRESULT DSOUND_Create8(REFIID riid, void **ppv);
HRESULT IDirectSoundImpl_Create(IUnknown *outer_unk, REFIID riid, void **ppv, BOOL has_ds8);
void DSOUND_ParseSpeakerConfig(DirectSoundDevice *device);

/* primary.c */

HRESULT DSOUND_PrimaryDestroy(DirectSoundDevice *device);
HRESULT DSOUND_PrimaryPlay(DirectSoundDevice *device);
HRESULT DSOUND_PrimaryStop(DirectSoundDevice *device);
LPWAVEFORMATEX DSOUND_CopyFormat(LPCWAVEFORMATEX wfex);
HRESULT DSOUND_ReopenDevice(DirectSoundDevice *device, BOOL forcewave);
HRESULT primarybuffer_create(DirectSoundDevice *device, IDirectSoundBufferImpl **ppdsb,
    const DSBUFFERDESC *dsbd);
void primarybuffer_destroy(IDirectSoundBufferImpl *This);
HRESULT primarybuffer_SetFormat(DirectSoundDevice *device, LPCWAVEFORMATEX wfex);
LONG capped_refcount_dec(LONG *ref);

/* duplex.c */

HRESULT DSOUND_FullDuplexCreate(REFIID riid, void **ppv);

/* mixer.c */
void DSOUND_CheckEvent(const IDirectSoundBufferImpl *dsb, DWORD playpos, int len);
void DSOUND_RecalcVolPan(PDSVOLUMEPAN volpan);
void DSOUND_AmpFactorToVolPan(PDSVOLUMEPAN volpan);
void DSOUND_RecalcFormat(IDirectSoundBufferImpl *dsb);
DWORD DSOUND_secpos_to_bufpos(const IDirectSoundBufferImpl *dsb, DWORD secpos, DWORD secmixpos, float *overshot);

DWORD CALLBACK DSOUND_mixthread(void *ptr);

/* sound3d.c */

void DSOUND_Calc3DBuffer(IDirectSoundBufferImpl *dsb);

/* capture.c */
 
HRESULT DSOUND_CaptureCreate(REFIID riid, void **ppv);
HRESULT DSOUND_CaptureCreate8(REFIID riid, void **ppv);
HRESULT IDirectSoundCaptureImpl_Create(IUnknown *outer_unk, REFIID riid, void **ppv, BOOL has_dsc8);

#define STATE_STOPPED   0
#define STATE_STARTING  1
#define STATE_PLAYING   2
#define STATE_CAPTURING 2
#define STATE_STOPPING  3

extern CRITICAL_SECTION DSOUND_renderers_lock;
extern struct list DSOUND_renderers;

extern GUID *DSOUND_renderer_guids;
extern GUID *DSOUND_capture_guids;

extern const WCHAR wine_vxd_drv[];

void setup_dsound_options(void);

HRESULT get_mmdevice(EDataFlow flow, const GUID *tgt, IMMDevice **device);

BOOL DSOUND_check_supported(IAudioClient *client, DWORD rate,
        DWORD depth, WORD channels);
HRESULT enumerate_mmdevices(EDataFlow flow, GUID *guids,
        LPDSENUMCALLBACKW cb, void *user);
