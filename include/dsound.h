#ifndef __WINE_DSOUND_H
#define __WINE_DSOUND_H

#include "winbase.h"     /* for CRITICAL_SECTION */
#include "mmsystem.h"
#include "d3d.h"			/*FIXME: Need to break out d3dtypes.h */

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
typedef struct IDirectSound3DListener IDirectSound3DListener,*LPDIRECTSOUND3DLISTENER,**LPLPDIRECTSOUND3DLISTENER;
typedef struct IDirectSound3DBuffer IDirectSound3DBuffer,*LPDIRECTSOUND3DBUFFER,**LPLPDIRECTSOUND3DBUFFER;

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


#define DSBLOCK_FROMWRITECURSOR     0x00000001
#define DSBLOCK_ENTIREBUFFER        0x00000002

#define DSBCAPS_PRIMARYBUFFER       0x00000001
#define DSBCAPS_STATIC              0x00000002
#define DSBCAPS_LOCHARDWARE         0x00000004
#define DSBCAPS_LOCSOFTWARE         0x00000008
#define DSBCAPS_CTRL3D              0x00000010
#define DSBCAPS_CTRLFREQUENCY       0x00000020
#define DSBCAPS_CTRLPAN             0x00000040
#define DSBCAPS_CTRLVOLUME          0x00000080
#define DSBCAPS_CTRLPOSITIONNOTIFY  0x00000100
#define DSBCAPS_CTRLDEFAULT         0x000000E0  /* Pan + volume + frequency. */
#define DSBCAPS_CTRLALL             0x000001F0  /* All control capabilities */
#define DSBCAPS_STICKYFOCUS         0x00004000
#define DSBCAPS_GLOBALFOCUS         0x00008000
#define DSBCAPS_GETCURRENTPOSITION2 0x00010000  /* More accurate play cursor under emulation*/
#define DSBCAPS_MUTE3DATMAXDISTANCE 0x00020000

#define DSBPAN_LEFT                 -10000
#define DSBPAN_RIGHT                 10000
#define DSBVOLUME_MAX                    0
#define DSBVOLUME_MIN               -10000
#define DSBFREQUENCY_MIN            100
#define DSBFREQUENCY_MAX            100000
#define DSBFREQUENCY_ORIGINAL       0

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
	HANDLE	hEventNotify;
} DSBPOSITIONNOTIFY,*LPDSBPOSITIONNOTIFY;

typedef const DSBPOSITIONNOTIFY *LPCDSBPOSITIONNOTIFY;

#define DSSPEAKER_HEADPHONE     1
#define DSSPEAKER_MONO          2
#define DSSPEAKER_QUAD          3
#define DSSPEAKER_STEREO        4
#define DSSPEAKER_SURROUND      5

#define DSSPEAKER_GEOMETRY_MIN      0x00000005  /* 5 degrees */
#define DSSPEAKER_GEOMETRY_NARROW   0x0000000A  /* 10 degrees */
#define DSSPEAKER_GEOMETRY_WIDE     0x00000014  /* 20 degrees */
#define DSSPEAKER_GEOMETRY_MAX      0x000000B4  /* 180 degrees */


typedef LPVOID* LPLPVOID;

typedef BOOL (CALLBACK *LPDSENUMCALLBACKW)(LPGUID,LPWSTR,LPWSTR,LPVOID);
typedef BOOL (CALLBACK *LPDSENUMCALLBACKA)(LPGUID,LPSTR,LPSTR,LPVOID);

extern HRESULT WINAPI DirectSoundCreate(REFGUID lpGUID,LPDIRECTSOUND * ppDS,IUnknown *pUnkOuter );

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
    STDMETHOD( SetCooperativeLevel)(THIS_ HWND, DWORD ) PURE;
    STDMETHOD( Compact)(THIS ) PURE;
    STDMETHOD( GetSpeakerConfig)(THIS_ LPDWORD ) PURE;
    STDMETHOD( SetSpeakerConfig)(THIS_ DWORD ) PURE;
    STDMETHOD( Initialize)(THIS_ LPGUID ) PURE;
} *LPDIRECTSOUND_VTABLE;

struct IDirectSound {
	LPDIRECTSOUND_VTABLE	lpvtbl;
	DWORD			ref;
	DWORD			priolevel;
	int			nrofbuffers;
	LPDIRECTSOUNDBUFFER	*buffers;
	LPDIRECTSOUNDBUFFER	primary;
	LPDIRECTSOUND3DLISTENER	listener;
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
    STDMETHOD(GetCurrentPosition16)(THIS_ LPDWORD,LPDWORD ) PURE;
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
	LPDIRECTSOUND3DBUFFER		ds3db;
	DWORD				playflags,playing;
	DWORD				playpos,writepos,buflen;
	DWORD				nAvgBytesPerSec;
	DWORD				freq;
	ULONG				freqAdjust;
	LONG				volume,pan;
	LONG				lVolAdjust,rVolAdjust;
	LPDIRECTSOUNDBUFFER		parent;		/* for duplicates */
	LPDIRECTSOUND			dsound;
	DSBUFFERDESC			dsbd;
	LPDSBPOSITIONNOTIFY		notifies;
	int				nrofnotifies;
	CRITICAL_SECTION		lock;
};
#undef THIS

#define WINE_NOBUFFER                   0x80000000

#define DSBPN_OFFSETSTOP		-1

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

#define DS3DMODE_NORMAL             0x00000000
#define DS3DMODE_HEADRELATIVE       0x00000001
#define DS3DMODE_DISABLE            0x00000002

#define DS3D_IMMEDIATE              0x00000000
#define DS3D_DEFERRED               0x00000001

#define DS3D_MINDISTANCEFACTOR      0.0f
#define DS3D_MAXDISTANCEFACTOR      10.0f
#define DS3D_DEFAULTDISTANCEFACTOR  1.0f

#define DS3D_MINROLLOFFFACTOR       0.0f
#define DS3D_MAXROLLOFFFACTOR       10.0f
#define DS3D_DEFAULTROLLOFFFACTOR   1.0f

#define DS3D_MINDOPPLERFACTOR       0.0f
#define DS3D_MAXDOPPLERFACTOR       10.0f
#define DS3D_DEFAULTDOPPLERFACTOR   1.0f

#define DS3D_DEFAULTMINDISTANCE     1.0f
#define DS3D_DEFAULTMAXDISTANCE     1000000000.0f

#define DS3D_MINCONEANGLE           0
#define DS3D_MAXCONEANGLE           360
#define DS3D_DEFAULTCONEANGLE       360

#define DS3D_DEFAULTCONEOUTSIDEVOLUME   0

typedef struct _DS3DLISTENER {
	DWORD				dwSize;
	D3DVECTOR			vPosition;
	D3DVECTOR			vVelocity;
	D3DVECTOR			vOrientFront;
	D3DVECTOR			vOrientTop;
	D3DVALUE			flDistanceFactor;
	D3DVALUE			flRolloffFactor;
	D3DVALUE			flDopplerFactor;
} DS3DLISTENER, *LPDS3DLISTENER;

typedef const DS3DLISTENER *LPCDS3DLISTENER;
	
#define THIS LPDIRECTSOUND3DLISTENER this
typedef struct IDirectSound3DListener_VTable
{
	/* IUnknown methods */
	STDMETHOD(QueryInterface)           (THIS_ REFIID, LPVOID FAR *) PURE;
	STDMETHOD_(ULONG,AddRef)            (THIS) PURE;
	STDMETHOD_(ULONG,Release)           (THIS) PURE;

	/* IDirectSound3DListener methods */
	STDMETHOD(GetAllParameters)         (THIS_ LPDS3DLISTENER) PURE;
	STDMETHOD(GetDistanceFactor)        (THIS_ LPD3DVALUE) PURE;
	STDMETHOD(GetDopplerFactor)         (THIS_ LPD3DVALUE) PURE;
	STDMETHOD(GetOrientation)           (THIS_ LPD3DVECTOR, LPD3DVECTOR) PURE;
	STDMETHOD(GetPosition)              (THIS_ LPD3DVECTOR) PURE;
	STDMETHOD(GetRolloffFactor)         (THIS_ LPD3DVALUE) PURE;
	STDMETHOD(GetVelocity)              (THIS_ LPD3DVECTOR) PURE;
	STDMETHOD(SetAllParameters)         (THIS_ LPCDS3DLISTENER, DWORD) PURE;
	STDMETHOD(SetDistanceFactor)        (THIS_ D3DVALUE, DWORD) PURE;
	STDMETHOD(SetDopplerFactor)         (THIS_ D3DVALUE, DWORD) PURE;
	STDMETHOD(SetOrientation)           (THIS_ D3DVALUE, D3DVALUE, D3DVALUE, D3DVALUE, D3DVALUE, D3DVALUE, DWORD) PURE;
	STDMETHOD(SetPosition)              (THIS_ D3DVALUE, D3DVALUE, D3DVALUE, DWORD) PURE;
	STDMETHOD(SetRolloffFactor)         (THIS_ D3DVALUE, DWORD) PURE;
	STDMETHOD(SetVelocity)              (THIS_ D3DVALUE, D3DVALUE, D3DVALUE, DWORD) PURE;
	STDMETHOD(CommitDeferredSettings)   (THIS) PURE;
} *LPDIRECTSOUND3DLISTENER_VTABLE, IDirectSound3DListener_VTable;

struct IDirectSound3DListener {
	LPDIRECTSOUND3DLISTENER_VTABLE	lpvtbl;
	DWORD				ref;
	LPDIRECTSOUNDBUFFER		dsb;
	DS3DLISTENER			ds3dl;
	CRITICAL_SECTION		lock;	
};
#undef THIS

typedef struct  _DS3DBUFFER {
	DWORD				dwSize;
	D3DVECTOR			vPosition;
	D3DVECTOR			vVelocity;
	DWORD				dwInsideConeAngle;
	DWORD				dwOutsideConeAngle;
	D3DVECTOR			vConeOrientation;
	LONG				lConeOutsideVolume;
	D3DVALUE			flMinDistance;
	D3DVALUE			flMaxDistance;
	DWORD				dwMode;
} DS3DBUFFER, *LPDS3DBUFFER;

typedef const DS3DBUFFER *LPCDS3DBUFFER;

#define THIS LPDIRECTSOUND3DBUFFER this
typedef struct IDirectSound3DBuffer_VTable
{
	/* IUnknown methods */
	STDMETHOD(QueryInterface)           (THIS_ REFIID, LPVOID FAR *) PURE;
	STDMETHOD_(ULONG,AddRef)            (THIS) PURE;
	STDMETHOD_(ULONG,Release)           (THIS) PURE;

	/* IDirectSound3DBuffer methods */
	STDMETHOD(GetAllParameters)         (THIS_ LPDS3DBUFFER) PURE;
	STDMETHOD(GetConeAngles)            (THIS_ LPDWORD, LPDWORD) PURE;
	STDMETHOD(GetConeOrientation)       (THIS_ LPD3DVECTOR) PURE;
	STDMETHOD(GetConeOutsideVolume)     (THIS_ LPLONG) PURE;
	STDMETHOD(GetMaxDistance)           (THIS_ LPD3DVALUE) PURE;
	STDMETHOD(GetMinDistance)           (THIS_ LPD3DVALUE) PURE;
	STDMETHOD(GetMode)                  (THIS_ LPDWORD) PURE;
	STDMETHOD(GetPosition)              (THIS_ LPD3DVECTOR) PURE;
	STDMETHOD(GetVelocity)              (THIS_ LPD3DVECTOR) PURE;
	STDMETHOD(SetAllParameters)         (THIS_ LPCDS3DBUFFER, DWORD) PURE;
	STDMETHOD(SetConeAngles)            (THIS_ DWORD, DWORD, DWORD) PURE;
	STDMETHOD(SetConeOrientation)       (THIS_ D3DVALUE, D3DVALUE, D3DVALUE, DWORD) PURE;
	STDMETHOD(SetConeOutsideVolume)     (THIS_ LONG, DWORD) PURE;
	STDMETHOD(SetMaxDistance)           (THIS_ D3DVALUE, DWORD) PURE;
	STDMETHOD(SetMinDistance)           (THIS_ D3DVALUE, DWORD) PURE;
	STDMETHOD(SetMode)                  (THIS_ DWORD, DWORD) PURE;
	STDMETHOD(SetPosition)              (THIS_ D3DVALUE, D3DVALUE, D3DVALUE, DWORD) PURE;
	STDMETHOD(SetVelocity)              (THIS_ D3DVALUE, D3DVALUE, D3DVALUE, DWORD) PURE;
} *LPDIRECTSOUND3DBUFFER_VTABLE, IDirectSound3DBuffer_VTable;

struct IDirectSound3DBuffer {
	LPDIRECTSOUND3DBUFFER_VTABLE	lpvtbl;
	DWORD				ref;
	LPDIRECTSOUNDBUFFER		dsb;
	DS3DBUFFER			ds3db;
	LPBYTE				buffer;
	DWORD				buflen;
	CRITICAL_SECTION		lock;
};
#undef THIS
#undef STDMETHOD
#undef STDMETHOD_
#undef PURE
#undef FAR
#undef THIS_
#endif
