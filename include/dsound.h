#ifndef __WINE_DSOUND_H
#define __WINE_DSOUND_H

#include "mmsystem.h"

DEFINE_GUID(CLSID_DirectSound,		0x47d4d946, 0x62e8, 0x11cf, 0x93, 0xbc, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);

DEFINE_GUID(IID_IDirectSound,		0x279AFA83,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
DEFINE_GUID(IID_IDirectSoundBuffer,	0x279AFA85,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60); 
DEFINE_GUID(IID_IDirectSoundNotify,	0xB0210783,0x89cd,0x11d0,0xAF,0x08,0x00,0xA0,0xC9,0x25,0xCD,0x16);
DEFINE_GUID(IID_IDirectSound3DListener,	0x279AFA84,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
DEFINE_GUID(IID_IDirectSound3DBuffer,	0x279AFA86,0x4981,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60); 
DEFINE_GUID(IID_IDirectSoundCapture,	0xB0210781,0x89CD,0x11D0,0xAF,0x08,0x00,0xA0,0xC9,0x25,0xCD,0x16);
DEFINE_GUID(IID_IDirectSoundCaptureBuffer,0xB0210782,0x89CD,0x11D0,0xAF,0x08,0x00,0xA0,0xC9,0x25,0xCD,0x16);
DEFINE_GUID(IID_IKsPropertySet,		0x31EFAC30,0x515C,0x11D0,0xA9,0xAA,0x00,0xAA,0x00,0x61,0xBE,0x93);



typedef struct IDirectSound IDirectSound,*LPDIRECTSOUND;
typedef struct IDirectSoundNotify IDirectSoundNotify,*LPDIRECTSOUNDNOTIFY;
typedef struct IDirectSoundBuffer IDirectSoundBuffer,*LPDIRECTSOUNDBUFFER,**LPLPDIRECTSOUNDBUFFER;

#define	_FACDS		0x878
#define	MAKE_DSHRESULT(code)		MAKE_HRESULT(1,_FACDS,code)

#define DS_OK				0
#define DSERR_ALLOCATED			MAKE_DSHRESULT(10)
#define DSERR_CONTROLUNAVAIL		MAKE_DSHRESULT(30)
#define DSERR_INVALIDPARAM		E_INVALIDARG
#define DSERR_INVALIDCALL		MAKE_DSHRESULT(50)
#define DSERR_GENERIC			E_FAIL
#define DSERR_PRIOLEVELNEEDED		MAKE_DSHRESULT(70)
#define DSERR_OUTOFMEMORY		E_OUTOFMEMORY
#define DSERR_BADFORMAT			MAKE_DSHRESULT(100)
#define DSERR_UNSUPPORTED		E_NOTIMPL
#define DSERR_NODRIVER			MAKE_DSHRESULT(120)
#define DSERR_ALREADYINITIALIZED	MAKE_DSHRESULT(130)
#define DSERR_NOAGGREGATION		CLASS_E_NOAGGREGATION
#define DSERR_BUFFERLOST		MAKE_DSHRESULT(150)
#define DSERR_OTHERAPPHASPRIO		MAKE_DSHRESULT(160)
#define DSERR_UNINITIALIZED		MAKE_DSHRESULT(170)

#define DSCAPS_PRIMARYMONO          0x00000001
#define DSCAPS_PRIMARYSTEREO        0x00000002
#define DSCAPS_PRIMARY8BIT          0x00000004
#define DSCAPS_PRIMARY16BIT         0x00000008
#define DSCAPS_CONTINUOUSRATE       0x00000010
#define DSCAPS_EMULDRIVER           0x00000020
#define DSCAPS_CERTIFIED            0x00000040
#define DSCAPS_SECONDARYMONO        0x00000100
#define DSCAPS_SECONDARYSTEREO      0x00000200
#define DSCAPS_SECONDARY8BIT        0x00000400
#define DSCAPS_SECONDARY16BIT       0x00000800

#define	DSSCL_NORMAL		1
#define	DSSCL_PRIORITY		2
#define	DSSCL_EXCLUSIVE		3
#define	DSSCL_WRITEPRIMARY	4

typedef struct _DSCAPS
{
    DWORD	dwSize;
    DWORD	dwFlags;
    DWORD	dwMinSecondarySampleRate;
    DWORD	dwMaxSecondarySampleRate;
    DWORD	dwPrimaryBuffers;
    DWORD	dwMaxHwMixingAllBuffers;
    DWORD	dwMaxHwMixingStaticBuffers;
    DWORD	dwMaxHwMixingStreamingBuffers;
    DWORD	dwFreeHwMixingAllBuffers;
    DWORD	dwFreeHwMixingStaticBuffers;
    DWORD	dwFreeHwMixingStreamingBuffers;
    DWORD	dwMaxHw3DAllBuffers;
    DWORD	dwMaxHw3DStaticBuffers;
    DWORD	dwMaxHw3DStreamingBuffers;
    DWORD	dwFreeHw3DAllBuffers;
    DWORD	dwFreeHw3DStaticBuffers;
    DWORD	dwFreeHw3DStreamingBuffers;
    DWORD	dwTotalHwMemBytes;
    DWORD	dwFreeHwMemBytes;
    DWORD	dwMaxContigFreeHwMemBytes;
    DWORD	dwUnlockTransferRateHwBuffers;
    DWORD	dwPlayCpuOverheadSwBuffers;
    DWORD	dwReserved1;
    DWORD	dwReserved2;
} DSCAPS,*LPDSCAPS;

#define DSBPLAY_LOOPING         0x00000001

#define DSBSTATUS_PLAYING           0x00000001
#define DSBSTATUS_BUFFERLOST        0x00000002
#define DSBSTATUS_LOOPING           0x00000004


#define DSBLOCK_FROMWRITECURSOR         0x00000001

#define DSBCAPS_PRIMARYBUFFER       0x00000001
#define DSBCAPS_STATIC              0x00000002
#define DSBCAPS_LOCHARDWARE         0x00000004
#define DSBCAPS_LOCSOFTWARE         0x00000008
#define DSBCAPS_CTRLFREQUENCY       0x00000020
#define DSBCAPS_CTRLPAN             0x00000040
#define DSBCAPS_CTRLVOLUME          0x00000080
#define DSBCAPS_CTRLDEFAULT         0x000000E0  /* Pan + volume + frequency. */
#define DSBCAPS_CTRLALL             0x000000E0  /* All control capabilities */
#define DSBCAPS_STICKYFOCUS         0x00004000
#define DSBCAPS_GETCURRENTPOSITION2 0x00010000  /* More accurate play cursor under emulation*/

typedef struct _DSBCAPS
{
    DWORD	dwSize;
    DWORD	dwFlags;
    DWORD	dwBufferBytes;
    DWORD	dwUnlockTransferRate;
    DWORD	dwPlayCpuOverhead;
} DSBCAPS,*LPDSBCAPS;

#define DSSCL_NORMAL                1
#define DSSCL_PRIORITY              2
#define DSSCL_EXCLUSIVE             3
#define DSSCL_WRITEPRIMARY          4

typedef struct _DSBUFFERDESC
{
    DWORD		dwSize;
    DWORD		dwFlags;
    DWORD		dwBufferBytes;
    DWORD		dwReserved;
    LPWAVEFORMATEX	lpwfxFormat;
} DSBUFFERDESC,*LPDSBUFFERDESC;

typedef struct _DSBPOSITIONNOTIFY
{
	DWORD		dwOffset;
	HANDLE32	hEventNotify;
} DSBPOSITIONNOTIFY,*LPDSBPOSITIONNOTIFY;

typedef const DSBPOSITIONNOTIFY *LPCDSBPOSITIONNOTIFY;

#define DSSPEAKER_HEADPHONE     1
#define DSSPEAKER_MONO          2
#define DSSPEAKER_QUAD          3
#define DSSPEAKER_STEREO        4
#define DSSPEAKER_SURROUND      5


typedef LPVOID* LPLPVOID;

typedef BOOL32 (CALLBACK *LPDSENUMCALLBACK32W)(LPGUID,LPWSTR,LPWSTR,LPVOID);
typedef BOOL32 (CALLBACK *LPDSENUMCALLBACK32A)(LPGUID,LPSTR,LPSTR,LPVOID);

extern HRESULT WINAPI DirectSoundCreate(LPGUID lpGUID,LPDIRECTSOUND * ppDS,IUnknown *pUnkOuter );

#define STDMETHOD(xfn) HRESULT (CALLBACK *fn##xfn)
#define STDMETHOD_(ret,xfn) ret (CALLBACK *fn##xfn)
#define PURE
#define FAR
#define THIS_ THIS,

#define THIS LPDIRECTSOUND this
typedef struct tagLPDIRECTSOUND_VTABLE
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectSound methods ***/

    STDMETHOD( CreateSoundBuffer)(THIS_ LPDSBUFFERDESC, LPLPDIRECTSOUNDBUFFER, IUnknown FAR *) PURE;
    STDMETHOD( GetCaps)(THIS_ LPDSCAPS ) PURE;
    STDMETHOD( DuplicateSoundBuffer)(THIS_ LPDIRECTSOUNDBUFFER, LPLPDIRECTSOUNDBUFFER ) PURE;
    STDMETHOD( SetCooperativeLevel)(THIS_ HWND32, DWORD ) PURE;
    STDMETHOD( Compact)(THIS ) PURE;
    STDMETHOD( GetSpeakerConfig)(THIS_ LPDWORD ) PURE;
    STDMETHOD( SetSpeakerConfig)(THIS_ DWORD ) PURE;
    STDMETHOD( Initialize)(THIS_ GUID FAR * ) PURE;
} *LPDIRECTSOUND_VTABLE;

struct IDirectSound {
	LPDIRECTSOUND_VTABLE	lpvtbl;
	DWORD			ref;
	int			nrofbuffers;
	LPDIRECTSOUNDBUFFER	*buffers;
	WAVEFORMATEX		wfx; /* current main waveformat */
};
#undef THIS

#define THIS LPDIRECTSOUNDBUFFER this
typedef struct tagLPDIRECTSOUNDBUFFER_VTABLE
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectSoundBuffer methods ***/

    STDMETHOD(           GetCaps)(THIS_ LPDSBCAPS ) PURE;
    STDMETHOD(GetCurrentPosition)(THIS_ LPDWORD,LPDWORD ) PURE;
    STDMETHOD(         GetFormat)(THIS_ LPWAVEFORMATEX, DWORD, LPDWORD ) PURE;
    STDMETHOD(         GetVolume)(THIS_ LPLONG ) PURE;
    STDMETHOD(            GetPan)(THIS_ LPLONG ) PURE;
    STDMETHOD(      GetFrequency)(THIS_ LPDWORD ) PURE;
    STDMETHOD(         GetStatus)(THIS_ LPDWORD ) PURE;
    STDMETHOD(        Initialize)(THIS_ LPDIRECTSOUND, LPDSBUFFERDESC ) PURE;
    STDMETHOD(              Lock)(THIS_ DWORD,DWORD,LPVOID,LPDWORD,LPVOID,LPDWORD,DWORD ) PURE;
    STDMETHOD(              Play)(THIS_ DWORD,DWORD,DWORD ) PURE;
    STDMETHOD(SetCurrentPosition)(THIS_ DWORD ) PURE;
    STDMETHOD(         SetFormat)(THIS_ LPWAVEFORMATEX ) PURE;
    STDMETHOD(         SetVolume)(THIS_ LONG ) PURE;
    STDMETHOD(            SetPan)(THIS_ LONG ) PURE;
    STDMETHOD(      SetFrequency)(THIS_ DWORD ) PURE;
    STDMETHOD(              Stop)(THIS  ) PURE;
    STDMETHOD(            Unlock)(THIS_ LPVOID,DWORD,LPVOID,DWORD ) PURE;
    STDMETHOD(           Restore)(THIS  ) PURE;
} *LPDIRECTSOUNDBUFFER_VTABLE;

struct IDirectSoundBuffer {
	LPDIRECTSOUNDBUFFER_VTABLE	lpvtbl;
	WAVEFORMATEX			wfx;
	DWORD				ref;
	LPBYTE				buffer;
	DWORD				playflags,playing,playpos,writepos,buflen;
	LONG				volume,pan;
	LPDIRECTSOUND			dsound;
	DSBUFFERDESC			dsbd;
	LPDSBPOSITIONNOTIFY		notifies;
	int				nrofnotifies;
};
#undef THIS

#define THIS LPDIRECTSOUNDNOTIFY this
typedef struct IDirectSoundNotify_VTable {
	/* IUnknown methods */
	STDMETHOD(QueryInterface)           (THIS_ REFIID, LPVOID *) PURE;
	STDMETHOD_(ULONG,AddRef)            (THIS) PURE;
	STDMETHOD_(ULONG,Release)           (THIS) PURE;

	/* IDirectSoundNotify methods */
	STDMETHOD(SetNotificationPositions) (THIS_ DWORD, LPCDSBPOSITIONNOTIFY) PURE;
} *LPDIRECTSOUNDNOTIFY_VTABLE,IDirectSoundNotify_VTable;

struct IDirectSoundNotify {
	LPDIRECTSOUNDNOTIFY_VTABLE	lpvtbl;
	DWORD				ref;
	LPDIRECTSOUNDBUFFER		dsb;
};
#undef THIS

#undef STDMETHOD
#undef STDMETHOD_
#undef PURE
#undef FAR
#undef THIS_
#endif
