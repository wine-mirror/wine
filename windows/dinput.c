/*		DirectInput
 *
 * Copyright 1998 Marcus Meissner
 */
/* Status:
 *
 * - Tomb Raider 2 Demo:
 *   Doesn't accept input yet. Although I am sure I've done everything right...
 * - WingCommander Prophecy Demo:
 *   dito.
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
#include "stddebug.h"
#include "debug.h"

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
	
	StringFromCLSID(rguid,xbuf);
	fprintf(stderr,"IDirectInputA(%p)->CreateDevice(%s,%p,%p),stub!\n",this,xbuf,pdev,punk);
	if (!memcmp(&GUID_SysKeyboard,rguid,sizeof(GUID_SysKeyboard))) {
		*pdev = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectInputDevice32A));
		(*pdev)->ref = 1;
		(*pdev)->lpvtbl = &SysKeyboardAvt;
		memcpy(&((*pdev)->guid),rguid,sizeof(*rguid));
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

	StringFromCLSID(riid,xbuf);
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

static IDirectInputA_VTable ddiavt= {
	IDirectInputA_QueryInterface,
	IDirectInputA_AddRef,
	IDirectInputA_Release,
	IDirectInputA_CreateDevice,
	IDirectInputA_EnumDevices,
	(void*)6,
	(void*)7,
	(void*)8
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
			StringFromCLSID(df->rgodf[i].pguid,xbuf);
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
		StringFromCLSID(rguid,xbuf);
	else
		strcpy(xbuf,"<no guid>");
	fprintf(stderr,"IDirectInputDeviceA(%p)->SetProperty(%s,%p)\n",this,xbuf,ph);
	return 0;
}

static HRESULT WINAPI IDirectInputDeviceA_SetEventNotification(
	LPDIRECTINPUTDEVICE32A this,HANDLE32 hnd
) {
	fprintf(stderr,"IDirectInputDeviceA(%p)->SetEventNotification(0x%08lx)\n",this,(DWORD)hnd);
	return 0;
}

static HRESULT WINAPI IDirectInputDeviceA_GetDeviceData(
	LPDIRECTINPUTDEVICE32A this,DWORD x,LPDIDEVICEOBJECTDATA dod,
	LPDWORD y,DWORD z
) {
/*
	fprintf(stderr,"IDirectInputDeviceA(%p)->GetDeviceData(0x%08lx,%p,%p,0x%08lx)\n",this,x,dod,y,z);
 */
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


extern BYTE InputKeyStateTable[256];
extern BYTE vkey2scode[512];

static HRESULT WINAPI IDirectInputDeviceA_GetDeviceState(
	LPDIRECTINPUTDEVICE32A this,DWORD len,LPVOID ptr
) {
/*
	fprintf(stderr,"IDirectInputDeviceA(%p)->GetDeviceState(0x%08lx,%p)\n",
		this,len,ptr
	);
 */
	if (len==256) {/* && this_is_a_keyboard */
		int	i;

		memset(ptr,0,256);
		for (i=0;i<256;i++) {
			if (InputKeyStateTable[i]&0x80)
				fprintf(stderr,"VKEY %d pressed (DIK 0x%02x\n",i,vkey2scode[i]);
			((LPBYTE)ptr)[i]=vkey2scode[InputKeyStateTable[i]]&0x80;
		}
		return 0;
	}
	return 0;
}

static HRESULT WINAPI IDirectInputDeviceA_QueryInterface(
	LPDIRECTINPUTDEVICE32A this,REFIID riid,LPVOID *ppobj
) {
	char	xbuf[50];

	StringFromCLSID(riid,xbuf);
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
