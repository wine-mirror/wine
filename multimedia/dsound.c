/*  			DS
 */

#include <stdio.h>
#include "windows.h"
#include "interfaces.h"
#include "mmsystem.h"
#include "dsound.h"

HRESULT WINAPI DirectSoundEnumerate32A(LPDSENUMCALLBACK32A enumcb,LPVOID context) {
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_SetFormat(
	LPDIRECTSOUNDBUFFER this,LPWAVEFORMATEX wfex
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->SetFormat(%p),stub!\n",this,wfex);
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_SetVolume(
	LPDIRECTSOUNDBUFFER this,LONG vol
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->SetVolume(%08lx),stub!\n",this,vol);
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_GetVolume(
	LPDIRECTSOUNDBUFFER this,LPLONG vol
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->GetVolume(%p),stub!\n",this,vol);
	*vol = 100;
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_SetFrequency(
	LPDIRECTSOUNDBUFFER this,DWORD freq
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->SetFrequency(%08lx),stub!\n",this,freq);
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_Play(
	LPDIRECTSOUNDBUFFER this,DWORD x,DWORD y,DWORD z
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->Play(%08lx,%08lx,%08lx),stub!\n",
		this,x,y,z
	);
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_Stop(LPDIRECTSOUNDBUFFER this) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->Stop()\n",this);
	return 0;
}

static DWORD WINAPI IDirectSoundBuffer_AddRef(LPDIRECTSOUNDBUFFER this) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->AddRef()\n",this);
	return ++(this->ref);
}
static DWORD WINAPI IDirectSoundBuffer_Release(LPDIRECTSOUNDBUFFER this) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->Release(),stub!\n",this);
	if (--this->ref)
		return this->ref;
	fprintf(stderr,"	-> IDirectSoundBuffer(%p) freed.\n",this);
	HeapFree(GetProcessHeap(),0,this);
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_GetCurrentPosition(
	LPDIRECTSOUNDBUFFER this,LPDWORD playpos,LPDWORD writepos
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->GetCurrentPosition(%p,%p),stub!\n",this,playpos,writepos);
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_GetStatus(
	LPDIRECTSOUNDBUFFER this,LPDWORD status
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->GetStatus(%p),stub!\n",this,status);
	*status = 0; /* hmm. set playing? or not ? */
	return 0;
}

static HRESULT WINAPI IDirectSoundBuffer_Lock(
	LPDIRECTSOUNDBUFFER this,DWORD x1,DWORD x2,LPVOID p1,LPDWORD x3,LPVOID p2,LPDWORD x4,DWORD x5
) {
	fprintf(stderr,"IDirectSoundBuffer(%p)->Lock(0x%08lx,0x%08lx,%p,%p,%p,%p,0x%08lx,),stub!\n",this,x1,x2,p1,x3,p2,x4,x5);
	return 0x80000000;
}


static struct tagLPDIRECTSOUNDBUFFER_VTABLE dsbvt = {
	(void *)1,
	IDirectSoundBuffer_AddRef,
	IDirectSoundBuffer_Release,
	(void *)4,
	IDirectSoundBuffer_GetCurrentPosition,
	(void *)6,
	IDirectSoundBuffer_GetVolume,
	(void *)8,
        (void *)9,
	IDirectSoundBuffer_GetStatus,
	(void *)11,
	IDirectSoundBuffer_Lock,
	IDirectSoundBuffer_Play,
	(void *)14,
	IDirectSoundBuffer_SetFormat,
	IDirectSoundBuffer_SetVolume,
	(void *)17,
	IDirectSoundBuffer_SetFrequency,
	IDirectSoundBuffer_Stop,
	(void *)20
};



static HRESULT WINAPI IDirectSound_SetCooperativeLevel(
	LPDIRECTSOUND this,HWND32 hwnd,DWORD level
) {
	fprintf(stderr,"IDirectSound(%p)->SetCooperativeLevel(%08lx,%ld),stub!\n",
		this,(DWORD)hwnd,level
	);
	return 0;
}


static HRESULT WINAPI IDirectSound_CreateSoundBuffer(
	LPDIRECTSOUND this,LPDSBUFFERDESC dsbd,LPLPDIRECTSOUNDBUFFER ppdsb,LPUNKNOWN lpunk
) {
	fprintf(stderr,"IDirectSound(%p)->CreateBuffer(%p,%p,%p),stub!\n",this,dsbd,ppdsb,lpunk);
	*ppdsb = (LPDIRECTSOUNDBUFFER)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectSoundBuffer));
	(*ppdsb)->ref =1;
	(*ppdsb)->lpvtbl = &dsbvt;
	return 0;
}

static HRESULT WINAPI IDirectSound_GetCaps(LPDIRECTSOUND this,LPDSCAPS dscaps) {
	fprintf(stderr,"IDirectSound(%p)->GetCaps(%p),stub!\n",this,dscaps);
	return 0;
}

static ULONG WINAPI IDirectSound_AddRef(LPDIRECTSOUND this) {
	fprintf(stderr,"IDirectSound(%p)->AddRef()\n",this);
	return ++(this->ref);
}

static ULONG WINAPI IDirectSound_Release(LPDIRECTSOUND this) {
	fprintf(stderr,"IDirectSound(%p)->Release()\n",this);
	if (!--(this->ref)) {
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static struct tagLPDIRECTSOUND_VTABLE dsvt = {
	(void *)1,
	IDirectSound_AddRef,
	IDirectSound_Release,
	IDirectSound_CreateSoundBuffer,
	IDirectSound_GetCaps,
	(void *)6,
	IDirectSound_SetCooperativeLevel,
	(void *)8,
        (void *)9,
        (void *)10,
        (void *)11
};

HRESULT WINAPI DirectSoundCreate(LPGUID lpGUID,LPDIRECTSOUND *ppDS,IUnknown *pUnkOuter ) {
	fprintf(stderr,"DirectSoundCreate(%p,%p,%p)\n",lpGUID,ppDS,pUnkOuter);
	*ppDS = (LPDIRECTSOUND)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectSound));
	(*ppDS)->ref = 1;
	(*ppDS)->lpvtbl = &dsvt;
	return 0;
}
