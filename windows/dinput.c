/*		DirectInput
 *
 * Copyright 1998 Marcus Meissner
 */
/* Status:
 *
 * - Tomb Raider 2 Demo:
 *   Playable using keyboard only.
 * - WingCommander Prophecy Demo:
 *   Doesn't get Input Focus.
 * 
 * FIXME: The keyboard handling needs to (and will) be merged into keyboard.c
 *	  (The current implementation is currently only a proof of concept and
 *	   an utter mess.)
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <X11/Xlib.h>
#include <sys/signal.h>

#include "windows.h"
#include "winerror.h"
#include "shell.h"
#include "ole.h"
#include "compobj.h"
#include "interfaces.h"
#include "gdi.h"
#include "heap.h"
#include "win.h"
#include "dinput.h"
#include "debug.h"

extern BYTE InputKeyStateTable[256];
extern BYTE vkey2scode[512];

static IDirectInputA_VTable ddiavt;
static IDirectInputDeviceA_VTable SysKeyboardAvt;
static IDirectInputDeviceA_VTable SysMouseAvt;

HRESULT WINAPI DirectInputCreate32A(HINSTANCE32 hinst, DWORD dwVersion, LPDIRECTINPUT32A *ppDI, LPUNKNOWN punkOuter) {
	fprintf(stderr,"DirectInputCreate32A(0x%08lx,%04lx,%p,%p)\n",
		(DWORD)hinst,dwVersion,ppDI,punkOuter
	);
	(*ppDI) = (LPDIRECTINPUT32A)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectInput32A));
	(*ppDI)->ref = 1;
	(*ppDI)->lpvtbl = &ddiavt;
	return 0;
}
/******************************************************************************
 *		IDirectInputA
 */
static HRESULT WINAPI IDirectInputA_EnumDevices(
	LPDIRECTINPUT32A this,DWORD dwFlags,LPDIENUMDEVICESCALLBACK32A cb,
	LPVOID context,DWORD x
) {
	fprintf(stderr,"IDirectInputA(%p)->EnumDevices(0x%08lx,%p,%p,0x%08lx),stub!\n",this,dwFlags,cb,context,x);
	return 0;
}

static ULONG WINAPI IDirectInputA_AddRef(LPDIRECTINPUT32A this) {
	return ++(this->ref);
}

static ULONG WINAPI IDirectInputA_Release(LPDIRECTINPUT32A this) {
	if (!(--this->ref)) {
		HeapFree(GetProcessHeap(),0,this);
		return 0;
	}
	return this->ref;
}

static HRESULT WINAPI IDirectInputA_CreateDevice(
	LPDIRECTINPUT32A this,REFGUID rguid,LPDIRECTINPUTDEVICE32A* pdev,
	LPUNKNOWN punk
) {
	char	xbuf[50];
	
	WINE_StringFromCLSID(rguid,xbuf);
	fprintf(stderr,"IDirectInputA(%p)->CreateDevice(%s,%p,%p),stub!\n",this,xbuf,pdev,punk);
	if (!memcmp(&GUID_SysKeyboard,rguid,sizeof(GUID_SysKeyboard))) {
		*pdev = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysKeyboard32A));
		(*pdev)->ref = 1;
		(*pdev)->lpvtbl = &SysKeyboardAvt;
		memcpy(&((*pdev)->guid),rguid,sizeof(*rguid));
		memset(((LPSYSKEYBOARD32A)(*pdev))->keystate,0,256);
		return 0;
	}
	if (!memcmp(&GUID_SysMouse,rguid,sizeof(GUID_SysMouse))) {
		*pdev = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectInputDevice32A));
		(*pdev)->ref = 1;
		(*pdev)->lpvtbl = &SysMouseAvt;
		memcpy(&((*pdev)->guid),rguid,sizeof(*rguid));
		return 0;
	}
	return E_FAIL;
}

static HRESULT WINAPI IDirectInputA_QueryInterface(
	LPDIRECTINPUT32A this,REFIID riid,LPVOID *ppobj
) {
	char	xbuf[50];

	WINE_StringFromCLSID(riid,xbuf);
	fprintf(stderr,"IDirectInputA(%p)->QueryInterface(%s,%p)\n",this,xbuf,ppobj);
	if (!memcmp(&IID_IUnknown,riid,sizeof(*riid))) {
		this->lpvtbl->fnAddRef(this);
		*ppobj = this;
		return 0;
	}
	if (!memcmp(&IID_IDirectInputA,riid,sizeof(*riid))) {
		this->lpvtbl->fnAddRef(this);
		*ppobj = this;
		return 0;
	}
	return E_FAIL;
}

static HRESULT WINAPI IDirectInputA_Initialize(
	LPDIRECTINPUT32A this,HINSTANCE32 hinst,DWORD x
) {
	return DIERR_ALREADYINITIALIZED;
}

static IDirectInputA_VTable ddiavt= {
	IDirectInputA_QueryInterface,
	IDirectInputA_AddRef,
	IDirectInputA_Release,
	IDirectInputA_CreateDevice,
	IDirectInputA_EnumDevices,
	(void*)6,
	(void*)7,
	IDirectInputA_Initialize
};

/******************************************************************************
 *	IDirectInputDeviceA
 */
static HRESULT WINAPI IDirectInputDeviceA_SetDataFormat(
	LPDIRECTINPUTDEVICE32A this,LPCDIDATAFORMAT df
) {
	/*
	int i;
	fprintf(stderr,"IDirectInputDeviceA(%p)->SetDataFormat(%p)\n",this,df);

	fprintf(stderr,"df.dwSize %ld\n",df->dwSize);
	fprintf(stderr,"df.dwObjsize %ld\n",df->dwObjSize);
	fprintf(stderr,"df.dwFlags 0x%08lx\n",df->dwFlags);
	fprintf(stderr,"df.dwDataSize %ld\n",df->dwDataSize);
	fprintf(stderr,"df.dwNumObjs %ld\n",df->dwNumObjs);

	for (i=0;i<df->dwNumObjs;i++) {
		char	xbuf[50];

		if (df->rgodf[i].pguid)
			WINE_StringFromCLSID(df->rgodf[i].pguid,xbuf);
		else
			strcpy(xbuf,"<no guid>");
		fprintf(stderr,"df.rgodf[%d].guid %s\n",i,xbuf);
		fprintf(stderr,"df.rgodf[%d].dwOfs %ld\n",i,df->rgodf[i].dwOfs);
		fprintf(stderr,"	dwType 0x%02lx,dwInstance %ld\n",DIDFT_GETTYPE(df->rgodf[i].dwType),DIDFT_GETINSTANCE(df->rgodf[i].dwType));
		fprintf(stderr,"df.rgodf[%d].dwFlags 0x%08lx\n",i,df->rgodf[i].dwFlags);
	}
	*/
	return 0;
}

static HRESULT WINAPI IDirectInputDeviceA_SetCooperativeLevel(
	LPDIRECTINPUTDEVICE32A this,HWND32 hwnd,DWORD dwflags
) {
	fprintf(stderr,"IDirectInputDeviceA(%p)->SetCooperativeLevel(0x%08lx,0x%08lx)\n",this,(DWORD)hwnd,dwflags);
	return 0;
}

static HRESULT WINAPI IDirectInputDeviceA_SetProperty(
	LPDIRECTINPUTDEVICE32A this,REFGUID rguid,LPCDIPROPHEADER ph
) {
	char	xbuf[50];

	if (HIWORD(rguid))
		WINE_StringFromCLSID(rguid,xbuf);
	else
		sprintf(xbuf,"<special guid %ld>",(DWORD)rguid);
	fprintf(stderr,"IDirectInputDeviceA(%p)->SetProperty(%s,%p)\n",this,xbuf,ph);
	if (!HIWORD(rguid)) {
		switch ((DWORD)rguid) {
		case DIPROP_BUFFERSIZE: {
			LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;

			fprintf(stderr,"	buffersize = %ld\n",pd->dwData);
			break;
		}
		default:
			fprintf(stderr,"	unknown type %ld\n",(DWORD)rguid);
			break;
		}
	}
	return 0;
}

static HRESULT WINAPI IDirectInputDeviceA_SetEventNotification(
	LPDIRECTINPUTDEVICE32A this,HANDLE32 hnd
) {
	fprintf(stderr,"IDirectInputDeviceA(%p)->SetEventNotification(0x%08lx)\n",this,(DWORD)hnd);
	return 0;
}

static HRESULT WINAPI IDirectInputDeviceA_GetDeviceData(
	LPDIRECTINPUTDEVICE32A this,DWORD dodsize,LPDIDEVICEOBJECTDATA dod,
	LPDWORD entries,DWORD flags
) {
/*	fprintf(stderr,"IDirectInputDeviceA(%p)->GetDeviceData(%ld,%p,%p(0x%08lx),0x%08lx)\n",this,dodsize,dod,entries,*entries,flags);*/
	return 0;
}


static HRESULT WINAPI IDirectInputDeviceA_Acquire(LPDIRECTINPUTDEVICE32A this) {
	fprintf(stderr,"IDirectInputDeviceA(%p)->Aquire()\n",this);
	return 0;
}

static HRESULT WINAPI IDirectInputDeviceA_Unacquire(LPDIRECTINPUTDEVICE32A this) {
	fprintf(stderr,"IDirectInputDeviceA(%p)->Unaquire()\n",this);
	return 0;
}

static ULONG WINAPI IDirectInputDeviceA_Release(LPDIRECTINPUTDEVICE32A this) {
	this->ref--;
	if (this->ref)
		return this->ref;
	HeapFree(GetProcessHeap(),0,this);
	return 0;
}

static HRESULT WINAPI SysKeyboardA_SetProperty(
	LPDIRECTINPUTDEVICE32A this,REFGUID rguid,LPCDIPROPHEADER ph
) {
	char			xbuf[50];

	if (HIWORD(rguid))
		WINE_StringFromCLSID(rguid,xbuf);
	else
		sprintf(xbuf,"<special guid %ld>",(DWORD)rguid);
	fprintf(stderr,"SysKeyboardA(%p)->SetProperty(%s,%p)\n",this,xbuf,ph);
	fprintf(stderr,"	size is %ld, headersize is %ld,obj is %ld,how is %ld\n",ph->dwSize,ph->dwHeaderSize,ph->dwObj,ph->dwHow);
	if (!HIWORD(rguid)) {
		switch ((DWORD)rguid) {
		case DIPROP_BUFFERSIZE: {
			LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;

			fprintf(stderr,"	buffersize = %ld\n",pd->dwData);
			break;
		}
		default:
			fprintf(stderr,"	unknown type %ld\n",(DWORD)rguid);
			break;
		}
	}
	return 0;
}

static HRESULT WINAPI SysKeyboardA_GetDeviceState(
	LPDIRECTINPUTDEVICE32A this,DWORD len,LPVOID ptr
) {
	if (len==256) {
		int	i;

		memset(ptr,0,256);
		for (i=0;i<256;i++)
			if (InputKeyStateTable[i]&0x80) {
				((LPBYTE)ptr)[vkey2scode[i]     ]=0x80;
				((LPBYTE)ptr)[vkey2scode[i]|0x80]=0x80;
			}
		return 0;
	}
	fprintf(stderr,"whoops, SysKeyboardA_GetDeviceState got len %ld?\n",len);
	return 0;
}

static HRESULT WINAPI SysKeyboardA_GetDeviceData(
	LPDIRECTINPUTDEVICE32A this,DWORD dodsize,LPDIDEVICEOBJECTDATA dod,
	LPDWORD entries,DWORD flags
) {
	int			i,n,xentries;
	LPSYSKEYBOARD32A	kthis = (LPSYSKEYBOARD32A)this;

/*	fprintf(stderr,"SysKeyboardA(%p)->GetDeviceData(%ld,%p,%p(%ld),0x%08lx)\n",this,dodsize,dod,entries,entries?*entries:0,flags);*/
	if (entries)
		xentries = *entries; 
	else
		xentries = 1;

	for (n=i=0;(i<256) && (n<*entries);i++) {
		if (kthis->keystate[i] == (InputKeyStateTable[i]&0x80))
			continue;
		if (dod) {
			/* add an entry */
			dod[n].dwOfs		= vkey2scode[i];
			dod[n].dwData		= InputKeyStateTable[i]&0x80;
			dod[n].dwTimeStamp	= 0; /* umm */
			dod[n].dwSequence	= 0; /* umm */
			n++;
		}
		if (!(flags & DIGDD_PEEK))
			kthis->keystate[i] = InputKeyStateTable[i]&0x80;
	}
	*entries = n;
	return 0;
}

static HRESULT WINAPI SysKeyboardA_Acquire(LPDIRECTINPUTDEVICE32A this) {
	fprintf(stderr,"SysKeyboardA(%p)->Aquire()\n",this);
	return 0;
}

static HRESULT WINAPI SysKeyboardA_Unacquire(LPDIRECTINPUTDEVICE32A this) {
	fprintf(stderr,"SysKeyboardA(%p)->Unaquire()\n",this);
	return 0;
}


static HRESULT WINAPI IDirectInputDeviceA_GetDeviceState(
	LPDIRECTINPUTDEVICE32A this,DWORD len,LPVOID ptr
) {
	fprintf(stderr,"IDirectInputDeviceA(%p)->GetDeviceState(0x%08lx,%p)\n",
		this,len,ptr
	);
	return 0;
}

static HRESULT WINAPI IDirectInputDeviceA_QueryInterface(
	LPDIRECTINPUTDEVICE32A this,REFIID riid,LPVOID *ppobj
) {
	char	xbuf[50];

	WINE_StringFromCLSID(riid,xbuf);
	fprintf(stderr,"IDirectInputA(%p)->QueryInterface(%s,%p)\n",this,xbuf,ppobj);
	if (!memcmp(&IID_IUnknown,riid,sizeof(*riid))) {
		this->lpvtbl->fnAddRef(this);
		*ppobj = this;
		return 0;
	}
	if (!memcmp(&IID_IDirectInputDeviceA,riid,sizeof(*riid))) {
		this->lpvtbl->fnAddRef(this);
		*ppobj = this;
		return 0;
	}
	return E_FAIL;
}

static IDirectInputDeviceA_VTable SysKeyboardAvt={
	IDirectInputDeviceA_QueryInterface,
	(void*)2,
	IDirectInputDeviceA_Release,
	(void*)4,
	(void*)5,
	(void*)6,
	SysKeyboardA_SetProperty,
	SysKeyboardA_Acquire,
	SysKeyboardA_Unacquire,
	SysKeyboardA_GetDeviceState,
	SysKeyboardA_GetDeviceData,
	IDirectInputDeviceA_SetDataFormat,
	IDirectInputDeviceA_SetEventNotification,
	IDirectInputDeviceA_SetCooperativeLevel,
	(void*)15,
	(void*)16,
	(void*)17,
	(void*)18,
};

static IDirectInputDeviceA_VTable SysMouseAvt={
	IDirectInputDeviceA_QueryInterface,
	(void*)2,
	IDirectInputDeviceA_Release,
	(void*)4,
	(void*)5,
	(void*)6,
	IDirectInputDeviceA_SetProperty,
	IDirectInputDeviceA_Acquire,
	IDirectInputDeviceA_Unacquire,
	IDirectInputDeviceA_GetDeviceState,
	IDirectInputDeviceA_GetDeviceData,
	IDirectInputDeviceA_SetDataFormat,
	IDirectInputDeviceA_SetEventNotification,
	IDirectInputDeviceA_SetCooperativeLevel,
	(void*)15,
	(void*)16,
	(void*)17,
	(void*)18,
};
