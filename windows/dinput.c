/*		DirectInput
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 *
 */
/* Status:
 *
 * - Tomb Raider 2 Demo:
 *   Playable using keyboard only.
 * - WingCommander Prophecy Demo:
 *   Doesn't get Input Focus.
 * 
 * - Fallout : works great in X and DGA mode
 *
 * FIXME: The keyboard handling needs to (and will) be merged into keyboard.c
 *	  (The current implementation is currently only a proof of concept and
 *	   an utter mess.)
 */

#include "config.h"
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/signal.h>

#include "winuser.h"
#include "winerror.h"
#include "wine/obj_base.h"
#include "gdi.h"
#include "dinput.h"
#include "debug.h"
#include "message.h"

#include "mouse.h"
#include "ts_xlib.h"
#include "sysmetrics.h"
#include "x11drv.h"

extern BYTE InputKeyStateTable[256];
extern int min_keycode, max_keycode;
extern WORD keyc2vkey[256];

static ICOM_VTABLE(IDirectInputA) ddiavt;
static ICOM_VTABLE(IDirectInputDevice2A) SysKeyboardAvt;
static ICOM_VTABLE(IDirectInputDevice2A) SysMouseAvt;

typedef struct IDirectInputAImpl IDirectInputAImpl;
typedef struct IDirectInputDevice2AImpl IDirectInputDevice2AImpl;
typedef struct SysKeyboardAImpl SysKeyboardAImpl;
typedef struct SysMouseAImpl SysMouseAImpl;

struct IDirectInputDevice2AImpl
{
        ICOM_VTABLE(IDirectInputDevice2A)* lpvtbl;
        DWORD                           ref;
        GUID                            guid;
};

struct SysKeyboardAImpl
{
        /* IDirectInputDevice2AImpl */
        ICOM_VTABLE(IDirectInputDevice2A)* lpvtbl;
        DWORD                           ref;
        GUID                            guid;
        /* SysKeyboardAImpl */
        BYTE                            keystate[256];
};

struct SysMouseAImpl
{
        /* IDirectInputDevice2AImpl */
        ICOM_VTABLE(IDirectInputDevice2A)* lpvtbl;
        DWORD                           ref;
        GUID                            guid;
        /* SysMouseAImpl */
        BYTE                            absolute;
        /* Previous position for relative moves */
        LONG prevX, prevY;
        LPMOUSE_EVENT_PROC prev_handler;
        HWND win;
        int xwin;
        DWORD win_centerX, win_centerY;
        LPDIDEVICEOBJECTDATA data_queue;
        int queue_pos, queue_len;
        int need_warp;
        int acquired;
};


/* UIDs for Wine "drivers".
   When enumerating each device supporting DInput, they have two UIDs :
    - the 'windows' UID
    - a vendor UID */
static GUID DInput_Wine_Mouse_GUID = { /* 9e573ed8-7734-11d2-8d4a-23903fb6bdf7 */
  0x9e573ed8,
  0x7734,
  0x11d2,
  {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};
static GUID DInput_Wine_Keyboard_GUID = { /* 0ab8648a-7735-11d2-8c73-71df54a96441 */
  0x0ab8648a,
  0x7735,
  0x11d2,
  {0x8c, 0x73, 0x71, 0xdf, 0x54, 0xa9, 0x64, 0x41}
};

/* FIXME: This is ugly and not thread safe :/ */
static IDirectInputDevice2A* current_lock = NULL;

/******************************************************************************
 *	Various debugging tools
 */
static void _dump_cooperativelevel(DWORD dwFlags) {
  int   i;
  const struct {
    DWORD       mask;
    char        *name;
  } flags[] = {
#define FE(x) { x, #x},
    FE(DISCL_BACKGROUND)
    FE(DISCL_EXCLUSIVE)
    FE(DISCL_FOREGROUND)
    FE(DISCL_NONEXCLUSIVE)
  };
  for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
    if (flags[i].mask & dwFlags)
      DUMP("%s ",flags[i].name);
  DUMP("\n");
}

struct IDirectInputAImpl
{
        ICOM_VTABLE(IDirectInputA)* lpvtbl;
        DWORD                   ref;
};

/******************************************************************************
 *	DirectInputCreate32A
 */
HRESULT WINAPI DirectInputCreateA(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA *ppDI, LPUNKNOWN punkOuter)
{
	IDirectInputAImpl* This;
	TRACE(dinput, "(0x%08lx,%04lx,%p,%p)\n",
		(DWORD)hinst,dwVersion,ppDI,punkOuter
	);
	This = (IDirectInputAImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectInputAImpl));
	This->ref = 1;
	This->lpvtbl = &ddiavt;
	*ppDI=(IDirectInputA*)This;
	return 0;
}
/******************************************************************************
 *	IDirectInputA_EnumDevices
 */
static HRESULT WINAPI IDirectInputAImpl_EnumDevices(
	LPDIRECTINPUTA iface, DWORD dwDevType, LPDIENUMDEVICESCALLBACKA lpCallback,
	LPVOID pvRef, DWORD dwFlags
)
{
  ICOM_THIS(IDirectInputAImpl,iface);
  DIDEVICEINSTANCEA devInstance;
  int ret;

  TRACE(dinput, "(this=%p,0x%04lx,%p,%p,%04lx)\n", This, dwDevType, lpCallback, pvRef, dwFlags);

  devInstance.dwSize = sizeof(DIDEVICEINSTANCEA);
  
  if ((dwDevType == 0) || (dwDevType == DIDEVTYPE_KEYBOARD)) {
  /* Return keyboard */
    devInstance.guidInstance = GUID_SysKeyboard;         /* DInput's GUID */
    devInstance.guidProduct = DInput_Wine_Keyboard_GUID; /* Vendor's GUID */
    devInstance.dwDevType = DIDEVTYPE_KEYBOARD | (DIDEVTYPEKEYBOARD_UNKNOWN << 8);
  strcpy(devInstance.tszInstanceName, "Keyboard");
  strcpy(devInstance.tszProductName, "Wine Keyboard");
  
  ret = lpCallback(&devInstance, pvRef);
    TRACE(dinput, "Keyboard registered\n");
  
    if (ret == DIENUM_STOP)
    return 0;
  }
  
  if ((dwDevType == 0) || (dwDevType == DIDEVTYPE_KEYBOARD)) {
  /* Return mouse */
    devInstance.guidInstance = GUID_SysMouse;         /* DInput's GUID */
    devInstance.guidProduct = DInput_Wine_Mouse_GUID; /* Vendor's GUID */
    devInstance.dwDevType = DIDEVTYPE_MOUSE | (DIDEVTYPEMOUSE_UNKNOWN << 8);
  strcpy(devInstance.tszInstanceName, "Mouse");
  strcpy(devInstance.tszProductName, "Wine Mouse");
  
  ret = lpCallback(&devInstance, pvRef);
    TRACE(dinput, "Mouse registered\n");
  }

  /* Should also do joystick enumerations.... */
  
	return 0;
}

static ULONG WINAPI IDirectInputAImpl_AddRef(LPDIRECTINPUTA iface)
{
	ICOM_THIS(IDirectInputAImpl,iface);
	return ++(This->ref);
}

static ULONG WINAPI IDirectInputAImpl_Release(LPDIRECTINPUTA iface)
{
	ICOM_THIS(IDirectInputAImpl,iface);
	if (!(--This->ref)) {
		HeapFree(GetProcessHeap(),0,This);
		return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IDirectInputAImpl_CreateDevice(
	LPDIRECTINPUTA iface,REFGUID rguid,LPDIRECTINPUTDEVICEA* pdev,
	LPUNKNOWN punk
) {
	ICOM_THIS(IDirectInputAImpl,iface);
	char	xbuf[50];
	
	WINE_StringFromCLSID(rguid,xbuf);
	FIXME(dinput,"(this=%p,%s,%p,%p): stub\n",This,xbuf,pdev,punk);
	if ((!memcmp(&GUID_SysKeyboard,rguid,sizeof(GUID_SysKeyboard))) ||          /* Generic Keyboard */
	    (!memcmp(&DInput_Wine_Keyboard_GUID,rguid,sizeof(GUID_SysKeyboard)))) { /* Wine Keyboard */
                SysKeyboardAImpl* newDevice;
		newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysKeyboardAImpl));
		newDevice->ref = 1;
		newDevice->lpvtbl = &SysKeyboardAvt;
		memcpy(&(newDevice->guid),rguid,sizeof(*rguid));
		memset(newDevice->keystate,0,256);
                *pdev=(IDirectInputDeviceA*)newDevice;
		return DI_OK;
	}
	if ((!memcmp(&GUID_SysMouse,rguid,sizeof(GUID_SysMouse))) ||             /* Generic Mouse */
	    (!memcmp(&DInput_Wine_Mouse_GUID,rguid,sizeof(GUID_SysMouse)))) { /* Wine Mouse */
                SysKeyboardAImpl* newDevice;
		newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysMouseAImpl));
		newDevice->ref = 1;
		newDevice->lpvtbl = &SysMouseAvt;
		memcpy(&(newDevice->guid),rguid,sizeof(*rguid));
                *pdev=(IDirectInputDeviceA*)newDevice;
		return DI_OK;
	}
	return E_FAIL;
}

static HRESULT WINAPI IDirectInputAImpl_QueryInterface(
	LPDIRECTINPUTA iface,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IDirectInputAImpl,iface);
	char	xbuf[50];

	WINE_StringFromCLSID(riid,xbuf);
	TRACE(dinput,"(this=%p,%s,%p)\n",This,xbuf,ppobj);
	if (!memcmp(&IID_IUnknown,riid,sizeof(*riid))) {
		IDirectInputA_AddRef(iface);
		*ppobj = This;
		return 0;
	}
	if (!memcmp(&IID_IDirectInputA,riid,sizeof(*riid))) {
		IDirectInputA_AddRef(iface);
		*ppobj = This;
		return 0;
	}
	return E_FAIL;
}

static HRESULT WINAPI IDirectInputAImpl_Initialize(
	LPDIRECTINPUTA iface,HINSTANCE hinst,DWORD x
) {
	return DIERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectInputAImpl_GetDeviceStatus(LPDIRECTINPUTA iface,
							REFGUID rguid) {
  ICOM_THIS(IDirectInputAImpl,iface);
  char xbuf[50];
  
  WINE_StringFromCLSID(rguid,xbuf);
  FIXME(dinput,"(%p)->(%s): stub\n",This,xbuf);
  
  return DI_OK;
}

static HRESULT WINAPI IDirectInputAImpl_RunControlPanel(LPDIRECTINPUTA iface,
							HWND hwndOwner,
							DWORD dwFlags) {
  ICOM_THIS(IDirectInputAImpl,iface);
  FIXME(dinput,"(%p)->(%08lx,%08lx): stub\n",This, (DWORD) hwndOwner, dwFlags);
  
  return DI_OK;
}

static ICOM_VTABLE(IDirectInputA) ddiavt= {
	IDirectInputAImpl_QueryInterface,
	IDirectInputAImpl_AddRef,
	IDirectInputAImpl_Release,
	IDirectInputAImpl_CreateDevice,
	IDirectInputAImpl_EnumDevices,
	IDirectInputAImpl_GetDeviceStatus,
	IDirectInputAImpl_RunControlPanel,
	IDirectInputAImpl_Initialize
};

/******************************************************************************
 *	IDirectInputDeviceA
 */

static HRESULT WINAPI IDirectInputDevice2AImpl_SetDataFormat(
	LPDIRECTINPUTDEVICE2A iface,LPCDIDATAFORMAT df
) {
	/*
	int i;
	TRACE(dinput,"(this=%p,%p)\n",This,df);

	TRACE(dinput,"df.dwSize=%ld\n",df->dwSize);
	TRACE(dinput,"(df.dwObjsize=%ld)\n",df->dwObjSize);
	TRACE(dinput,"(df.dwFlags=0x%08lx)\n",df->dwFlags);
	TRACE(dinput,"(df.dwDataSize=%ld)\n",df->dwDataSize);
	TRACE(dinput,"(df.dwNumObjs=%ld)\n",df->dwNumObjs);

	for (i=0;i<df->dwNumObjs;i++) {
		char	xbuf[50];

		if (df->rgodf[i].pguid)
			WINE_StringFromCLSID(df->rgodf[i].pguid,xbuf);
		else
			strcpy(xbuf,"<no guid>");
		TRACE(dinput,"df.rgodf[%d].guid %s\n",i,xbuf);
		TRACE(dinput,"df.rgodf[%d].dwOfs %ld\n",i,df->rgodf[i].dwOfs);
		TRACE(dinput,"dwType 0x%02lx,dwInstance %ld\n",DIDFT_GETTYPE(df->rgodf[i].dwType),DIDFT_GETINSTANCE(df->rgodf[i].dwType));
		TRACE(dinput,"df.rgodf[%d].dwFlags 0x%08lx\n",i,df->rgodf[i].dwFlags);
	}
	*/
	return 0;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_SetCooperativeLevel(
	LPDIRECTINPUTDEVICE2A iface,HWND hwnd,DWORD dwflags
) {
	ICOM_THIS(IDirectInputDevice2AImpl,iface);
	FIXME(dinput,"(this=%p,0x%08lx,0x%08lx): stub\n",This,(DWORD)hwnd,dwflags);
	if (TRACE_ON(dinput))
	  _dump_cooperativelevel(dwflags);
	return 0;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_SetEventNotification(
	LPDIRECTINPUTDEVICE2A iface,HANDLE hnd
) {
	ICOM_THIS(IDirectInputDevice2AImpl,iface);
	FIXME(dinput,"(this=%p,0x%08lx): stub\n",This,(DWORD)hnd);
	return 0;
}

static ULONG WINAPI IDirectInputDevice2AImpl_Release(LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(IDirectInputDevice2AImpl,iface);
	This->ref--;
	if (This->ref)
		return This->ref;
	HeapFree(GetProcessHeap(),0,This);
	return 0;
}

static HRESULT WINAPI SysKeyboardAImpl_SetProperty(
	LPDIRECTINPUTDEVICE2A iface,REFGUID rguid,LPCDIPROPHEADER ph
)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	char			xbuf[50];

	if (HIWORD(rguid))
		WINE_StringFromCLSID(rguid,xbuf);
	else
		sprintf(xbuf,"<special guid %ld>",(DWORD)rguid);
	TRACE(dinput,"(this=%p,%s,%p)\n",This,xbuf,ph);
	TRACE(dinput,"(size=%ld,headersize=%ld,obj=%ld,how=%ld\n",
            ph->dwSize,ph->dwHeaderSize,ph->dwObj,ph->dwHow);
	if (!HIWORD(rguid)) {
		switch ((DWORD)rguid) {
		case DIPROP_BUFFERSIZE: {
			LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;

			TRACE(dinput,"(buffersize=%ld)\n",pd->dwData);
			break;
		}
		default:
			WARN(dinput,"Unknown type %ld\n",(DWORD)rguid);
			break;
		}
	}
	return 0;
}

static HRESULT WINAPI SysKeyboardAImpl_GetDeviceState(
	LPDIRECTINPUTDEVICE2A iface,DWORD len,LPVOID ptr
)
{
	if (len==256) {
		int keyc,vkey;

		memset(ptr,0,256);
		for (keyc=min_keycode;keyc<max_keycode;keyc++)
                {
                    /* X keycode to virtual key */
                    vkey = keyc2vkey[keyc] & 0xFF;
                    /* The windows scancode is keyc-min_keycode */
                    if (InputKeyStateTable[vkey]&0x80) {
                        ((LPBYTE)ptr)[keyc-min_keycode]=0x80;
                        ((LPBYTE)ptr)[(keyc-min_keycode)|0x80]=0x80;
                    }
                }
		return 0;
	}
	WARN(dinput,"whoops, SysKeyboardAImpl_GetDeviceState got len %ld?\n",len);
	return 0;
}

static HRESULT WINAPI SysKeyboardAImpl_GetDeviceData(
	LPDIRECTINPUTDEVICE2A iface,DWORD dodsize,LPDIDEVICEOBJECTDATA dod,
	LPDWORD entries,DWORD flags
)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	int			keyc,n,vkey,xentries;

	TRACE(dinput,"(this=%p,%ld,%p,%p(%ld)),0x%08lx)\n",
    		This,dodsize,dod,entries,entries?*entries:0,flags);
	EVENT_WaitNetEvent(FALSE,TRUE);
	if (entries)
		xentries = *entries; 
	else
		xentries = 1;

        n = 0;

        for (keyc=min_keycode;(keyc<max_keycode) && (n<*entries);keyc++)
        {
                    /* X keycode to virtual key */
                    vkey = keyc2vkey[keyc] & 0xFF;
                    if (This->keystate[vkey] == (InputKeyStateTable[vkey]&0x80))
			continue;
		if (dod) {
			/* add an entry */
                        dod[n].dwOfs		= keyc-min_keycode; /* scancode */
			dod[n].dwData		= InputKeyStateTable[vkey]&0x80;
			dod[n].dwTimeStamp	= 0; /* umm */
			dod[n].dwSequence	= 0; /* umm */
			n++;
		}
		if (!(flags & DIGDD_PEEK))
			This->keystate[vkey] = InputKeyStateTable[vkey]&0x80;
                    
        }
        
	if (n) fprintf(stderr,"%d entries\n",n);
	*entries = n;
	return 0;
}

static HRESULT WINAPI SysKeyboardAImpl_Acquire(LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	TRACE(dinput,"(this=%p): stub\n",This);
	return 0;
}

static HRESULT WINAPI SysKeyboardAImpl_Unacquire(LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	TRACE(dinput,"(this=%p): stub\n",This);
	return 0;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_QueryInterface(
	LPDIRECTINPUTDEVICE2A iface,REFIID riid,LPVOID *ppobj
)
{
	ICOM_THIS(IDirectInputDevice2AImpl,iface);
	char	xbuf[50];

	WINE_StringFromCLSID(riid,xbuf);
	TRACE(dinput,"(this=%p,%s,%p)\n",This,xbuf,ppobj);
	if (!memcmp(&IID_IUnknown,riid,sizeof(*riid))) {
		IDirectInputDevice2_AddRef(iface);
		*ppobj = This;
		return 0;
	}
	if (!memcmp(&IID_IDirectInputDeviceA,riid,sizeof(*riid))) {
		IDirectInputDevice2_AddRef(iface);
		*ppobj = This;
		return 0;
	}
	if (!memcmp(&IID_IDirectInputDevice2A,riid,sizeof(*riid))) {
		IDirectInputDevice2_AddRef(iface);
		*ppobj = This;
		return 0;
	}
	return E_FAIL;
}

static ULONG WINAPI IDirectInputDevice2AImpl_AddRef(
	LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(IDirectInputDevice2AImpl,iface);
	return ++This->ref;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_GetCapabilities(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIDEVCAPS lpDIDevCaps)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_EnumObjects(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
	FIXME(dinput, "stub!\n");
#if 0
	if (lpCallback)
		lpCallback(NULL, lpvRef);
#endif
	return DI_OK;
}
	
static HRESULT WINAPI IDirectInputDevice2AImpl_GetProperty(
	LPDIRECTINPUTDEVICE2A iface,
	REFGUID rguid,
	LPDIPROPHEADER pdiph)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_GetObjectInfo(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIDEVICEOBJECTINSTANCEA pdidoi,
	DWORD dwObj,
	DWORD dwHow)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}
	
static HRESULT WINAPI IDirectInputDevice2AImpl_GetDeviceInfo(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIDEVICEINSTANCEA pdidi)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}
	
static HRESULT WINAPI IDirectInputDevice2AImpl_RunControlPanel(
	LPDIRECTINPUTDEVICE2A iface,
	HWND hwndOwner,
	DWORD dwFlags)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}
	
static HRESULT WINAPI IDirectInputDevice2AImpl_Initialize(
	LPDIRECTINPUTDEVICE2A iface,
	HINSTANCE hinst,
	DWORD dwVersion,
	REFGUID rguid)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}
	
/******************************************************************************
 *	IDirectInputDevice2A
 */

static HRESULT WINAPI IDirectInputDevice2AImpl_CreateEffect(
	LPDIRECTINPUTDEVICE2A iface,
	REFGUID rguid,
	LPCDIEFFECT lpeff,
	LPDIRECTINPUTEFFECT *ppdef,
	LPUNKNOWN pUnkOuter)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_EnumEffects(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIENUMEFFECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
	FIXME(dinput, "stub!\n");
	if (lpCallback)
		lpCallback(NULL, lpvRef);
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_GetEffectInfo(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIEFFECTINFOA lpdei,
	REFGUID rguid)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_GetForceFeedbackState(
	LPDIRECTINPUTDEVICE2A iface,
	LPDWORD pdwOut)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_SendForceFeedbackCommand(
	LPDIRECTINPUTDEVICE2A iface,
	DWORD dwFlags)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_EnumCreatedEffectObjects(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
	FIXME(dinput, "stub!\n");
	if (lpCallback)
		lpCallback(NULL, lpvRef);
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_Escape(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIEFFESCAPE lpDIEEsc)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_Poll(
	LPDIRECTINPUTDEVICE2A iface)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_SendDeviceData(
	LPDIRECTINPUTDEVICE2A iface,
	DWORD cbObjectData,
	LPDIDEVICEOBJECTDATA rgdod,
	LPDWORD pdwInOut,
	DWORD dwFlags)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

/******************************************************************************
 *	SysMouseA (DInput Mouse support)
 */

/******************************************************************************
  *     Release : release the mouse buffer.
  */
static ULONG WINAPI SysMouseAImpl_Release(LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(SysMouseAImpl,iface);

	This->ref--;
	if (This->ref)
		return This->ref;

	/* Free the data queue */
	if (This->data_queue != NULL)
	  HeapFree(GetProcessHeap(),0,This->data_queue);

	/* Install the previous event handler (in case of releasing an aquired
	   mouse device) */
	if (This->prev_handler != NULL)
	  MOUSE_Enable(This->prev_handler);
	
	HeapFree(GetProcessHeap(),0,This);
	return 0;
}


/******************************************************************************
  *     SetCooperativeLevel : store the window in which we will do our
  *   grabbing.
  */
static HRESULT WINAPI SysMouseAImpl_SetCooperativeLevel(
	LPDIRECTINPUTDEVICE2A iface,HWND hwnd,DWORD dwflags
)
{
  ICOM_THIS(SysMouseAImpl,iface);

  TRACE(dinput,"(this=%p,0x%08lx,0x%08lx): stub\n",This,(DWORD)hwnd,dwflags);

  if (TRACE_ON(dinput))
    _dump_cooperativelevel(dwflags);

  /* Store the window which asks for the mouse */
  This->win = hwnd;
  
  return 0;
}


/******************************************************************************
  *     SetDataFormat : the application can choose the format of the data
  *   the device driver sends back with GetDeviceState.
  *
  *   For the moment, only the "standard" configuration (c_dfDIMouse) is supported
  *   in absolute and relative mode.
  */
static HRESULT WINAPI SysMouseAImpl_SetDataFormat(
	LPDIRECTINPUTDEVICE2A iface,LPCDIDATAFORMAT df
)
{
  ICOM_THIS(SysMouseAImpl,iface);
  int i;
  
  TRACE(dinput,"(this=%p,%p)\n",This,df);

  TRACE(dinput,"(df.dwSize=%ld)\n",df->dwSize);
  TRACE(dinput,"(df.dwObjsize=%ld)\n",df->dwObjSize);
  TRACE(dinput,"(df.dwFlags=0x%08lx)\n",df->dwFlags);
  TRACE(dinput,"(df.dwDataSize=%ld)\n",df->dwDataSize);
  TRACE(dinput,"(df.dwNumObjs=%ld)\n",df->dwNumObjs);

  for (i=0;i<df->dwNumObjs;i++) {
    char	xbuf[50];
    
    if (df->rgodf[i].pguid)
      WINE_StringFromCLSID(df->rgodf[i].pguid,xbuf);
    else
      strcpy(xbuf,"<no guid>");
    TRACE(dinput,"df.rgodf[%d].guid %s (%p)\n",i,xbuf, df->rgodf[i].pguid);
    TRACE(dinput,"df.rgodf[%d].dwOfs %ld\n",i,df->rgodf[i].dwOfs);
    TRACE(dinput,"dwType 0x%02x,dwInstance %d\n",DIDFT_GETTYPE(df->rgodf[i].dwType),DIDFT_GETINSTANCE(df->rgodf[i].dwType));
    TRACE(dinput,"df.rgodf[%d].dwFlags 0x%08lx\n",i,df->rgodf[i].dwFlags);
  }

  /* Check size of data format to prevent crashes if the applications
     sends a smaller buffer */
  if (df->dwDataSize != sizeof(struct DIMOUSESTATE)) {
    FIXME(dinput, "non-standard mouse configuration not supported yet.");
    return DIERR_INVALIDPARAM;
  }
  
  /* For the moment, ignore these fields and return always as if
     c_dfDIMouse was passed as format... */

  /* Check if the mouse is in absolute or relative mode */
  if (df->dwFlags == DIDF_ABSAXIS)
    This->absolute = 1;
  else 
    This->absolute = 0;
  
  return 0;
}

#define GEN_EVENT(offset,data,time,seq)						\
{										\
  if (This->queue_pos < This->queue_len) {					\
    This->data_queue[This->queue_pos].dwOfs = offset;				\
    This->data_queue[This->queue_pos].dwData = data;				\
    This->data_queue[This->queue_pos].dwTimeStamp = time;			\
    This->data_queue[This->queue_pos].dwSequence = seq;			\
    This->queue_pos++;								\
  }										\
  }
  
/* Our private mouse event handler */
static void WINAPI dinput_mouse_event( DWORD dwFlags, DWORD dx, DWORD dy,
				      DWORD cButtons, DWORD dwExtraInfo )
{
  DWORD posX, posY, keyState, time, extra;
  SysMouseAImpl* This = (SysMouseAImpl*) current_lock;
  
  if (   !IsBadReadPtr( (LPVOID)dwExtraInfo, sizeof(WINE_MOUSEEVENT) )
      && ((WINE_MOUSEEVENT *)dwExtraInfo)->magic == WINE_MOUSEEVENT_MAGIC ) {
    WINE_MOUSEEVENT *wme = (WINE_MOUSEEVENT *)dwExtraInfo;
    keyState = wme->keyState;
    time = wme->time;
    extra = (DWORD)wme->hWnd;
    
    assert( dwFlags & MOUSEEVENTF_ABSOLUTE );
    posX = (dx * SYSMETRICS_CXSCREEN) >> 16;
    posY = (dy * SYSMETRICS_CYSCREEN) >> 16;
  } else {
    ERR(dinput, "Mouse event not supported...\n");
    return ;
  }

  TRACE(dinput, " %ld %ld ", posX, posY);

  if ( dwFlags & MOUSEEVENTF_MOVE ) {
    if (This->absolute) {
      if (posX != This->prevX)
	GEN_EVENT(DIMOFS_X, posX, time, 0);
      if (posY != This->prevY)
	GEN_EVENT(DIMOFS_Y, posY, time, 0);
    } else {
      /* Relative mouse input : the real fun starts here... */
      if (This->need_warp) {
	if (posX != This->prevX)
	  GEN_EVENT(DIMOFS_X, posX - This->prevX, time, 0);
	if (posY != This->prevY)
	  GEN_EVENT(DIMOFS_Y, posY - This->prevY, time, 0);
      } else {
	/* This is the first time the event handler has been called after a
	   GetData of GetState. */
	if (posX != This->win_centerX) {
	  GEN_EVENT(DIMOFS_X, posX - This->win_centerX, time, 0);
	  This->need_warp = 1;
	}
	  
	if (posY != This->win_centerY) {
	  GEN_EVENT(DIMOFS_Y, posY - This->win_centerY, time, 0);
	  This->need_warp = 1;
	}
      }
    }
  }
  if ( dwFlags & MOUSEEVENTF_LEFTDOWN ) {
    if (TRACE_ON(dinput))
      DUMP(" LD ");

    GEN_EVENT(DIMOFS_BUTTON0, 0xFF, time, 0);
  }
  if ( dwFlags & MOUSEEVENTF_LEFTUP ) {
    if (TRACE_ON(dinput))
      DUMP(" LU ");

    GEN_EVENT(DIMOFS_BUTTON0, 0x00, time, 0);
  }
  if ( dwFlags & MOUSEEVENTF_RIGHTDOWN ) {
    if (TRACE_ON(dinput))
      DUMP(" RD ");

    GEN_EVENT(DIMOFS_BUTTON1, 0xFF, time, 0);
  }
  if ( dwFlags & MOUSEEVENTF_RIGHTUP ) {
    if (TRACE_ON(dinput))
      DUMP(" RU ");

    GEN_EVENT(DIMOFS_BUTTON1, 0x00, time, 0);
  }
  if ( dwFlags & MOUSEEVENTF_MIDDLEDOWN ) {
    if (TRACE_ON(dinput))
      DUMP(" MD ");

    GEN_EVENT(DIMOFS_BUTTON2, 0xFF, time, 0);
  }
  if ( dwFlags & MOUSEEVENTF_MIDDLEUP ) {
    if (TRACE_ON(dinput))
      DUMP(" MU ");

    GEN_EVENT(DIMOFS_BUTTON2, 0x00, time, 0);
  }
  if (TRACE_ON(dinput))
    DUMP("\n");

  This->prevX = posX;
  This->prevY = posY;
}


/******************************************************************************
  *     Acquire : gets exclusive control of the mouse
  */
static HRESULT WINAPI SysMouseAImpl_Acquire(LPDIRECTINPUTDEVICE2A iface)
{
  ICOM_THIS(SysMouseAImpl,iface);
  RECT	rect;
  
  TRACE(dinput,"(this=%p)\n",This);

  if (This->acquired == 0) {
    /* This stores the current mouse handler.
       FIXME : need to be fixed for native USER use */
    This->prev_handler = mouse_event;
    
    /* Store (in a global variable) the current lock */
    current_lock = (IDirectInputDevice2A*)This;
    
    /* Install our own mouse event handler */
    MOUSE_Enable(dinput_mouse_event);
    
    /* Get the window dimension and find the center */
    GetWindowRect(This->win, &rect);
    This->xwin = ((X11DRV_WND_DATA *) WIN_FindWndPtr(This->win)->pDriverData)->window;
    This->win_centerX = (rect.right  - rect.left) / 2;
    This->win_centerY = (rect.bottom - rect.top ) / 2;
    /* Warp the mouse to the center of the window */
    TRACE(dinput, "Warping mouse to %ld - %ld\n", This->win_centerX, This->win_centerY);
    TSXWarpPointer(display, DefaultRootWindow(display),
		   This->xwin, 0, 0, 0, 0,
		   This->win_centerX, This->win_centerY);
    
    This->acquired = 1;
  }
  return 0;
}

/******************************************************************************
  *     Unacquire : frees the mouse
  */
static HRESULT WINAPI SysMouseAImpl_Unacquire(LPDIRECTINPUTDEVICE2A iface)
{
  ICOM_THIS(SysMouseAImpl,iface);

  TRACE(dinput,"(this=%p)\n",This);

  /* Reinstall previous mouse event handler */
  MOUSE_Enable(This->prev_handler);
  This->prev_handler = NULL;
  
  /* No more locks */
  current_lock = NULL;

  /* Unacquire device */
  This->acquired = 0;
  
  return 0;
}

/******************************************************************************
  *     GetDeviceState : returns the "state" of the mouse.
  *
  *   For the moment, only the "standard" return structure (DIMOUSESTATE) is
  *   supported.
  */
static HRESULT WINAPI SysMouseAImpl_GetDeviceState(
	LPDIRECTINPUTDEVICE2A iface,DWORD len,LPVOID ptr
) {
  ICOM_THIS(SysMouseAImpl,iface);
  DWORD rx, ry, state;
  struct DIMOUSESTATE *mstate = (struct DIMOUSESTATE *) ptr;
  
  TRACE(dinput,"(this=%p,0x%08lx,%p): \n",This,len,ptr);
  
  /* Check if the buffer is big enough */
  if (len < sizeof(struct DIMOUSESTATE)) {
    FIXME(dinput, "unsupported state structure.");
    return DIERR_INVALIDPARAM;
  }
  
  /* Get the mouse position */
  EVENT_QueryPointer(&rx, &ry, &state);
  TRACE(dinput,"(X:%ld - Y:%ld)\n", rx, ry);

  /* Fill the mouse state structure */
  if (This->absolute) {
    mstate->lX = rx;
    mstate->lY = ry;
  } else {
    mstate->lX = rx - This->win_centerX;
    mstate->lY = ry - This->win_centerY;

    if ((mstate->lX != 0) || (mstate->lY != 0))
      This->need_warp = 1;
  }
  mstate->lZ = 0;
  mstate->rgbButtons[0] = (state & MK_LBUTTON ? 0xFF : 0x00);
  mstate->rgbButtons[1] = (state & MK_RBUTTON ? 0xFF : 0x00);
  mstate->rgbButtons[2] = (state & MK_MBUTTON ? 0xFF : 0x00);
  mstate->rgbButtons[3] = 0x00;
  
  /* Check if we need to do a mouse warping */
  if (This->need_warp) {
    TRACE(dinput, "Warping mouse to %ld - %ld\n", This->win_centerX, This->win_centerY);
    TSXWarpPointer(display, DefaultRootWindow(display),
		   This->xwin, 0, 0, 0, 0,
		   This->win_centerX, This->win_centerY);
    This->need_warp = 0;
  }
  
  TRACE(dinput, "(X: %ld - Y: %ld   L: %02x M: %02x R: %02x)\n",
	mstate->lX, mstate->lY,
	mstate->rgbButtons[0], mstate->rgbButtons[2], mstate->rgbButtons[1]);
  
  return 0;
}

/******************************************************************************
  *     GetDeviceState : gets buffered input data.
  */
static HRESULT WINAPI SysMouseAImpl_GetDeviceData(LPDIRECTINPUTDEVICE2A iface,
					      DWORD dodsize,
					      LPDIDEVICEOBJECTDATA dod,
					      LPDWORD entries,
					      DWORD flags
) {
  ICOM_THIS(SysMouseAImpl,iface);
  
  TRACE(dinput,"(%p)->(%ld,%p,%p(0x%08lx),0x%08lx)\n",
	This,dodsize,dod,entries,*entries,flags);

  if (flags & DIGDD_PEEK)
    TRACE(dinput, "DIGDD_PEEK\n");

  if (dod == NULL) {
    *entries = This->queue_pos;
    This->queue_pos = 0;
  } else {
    /* Check for buffer overflow */
    if (This->queue_pos > *entries) {
      WARN(dinput, "Buffer overflow not handled properly yet...\n");
      This->queue_pos = *entries;
    }
    if (dodsize != sizeof(DIDEVICEOBJECTDATA)) {
      ERR(dinput, "Wrong structure size !\n");
      return DIERR_INVALIDPARAM;
    }

    TRACE(dinput, "Application retrieving %d event(s).\n", This->queue_pos); 
    
    /* Copy the buffered data into the application queue */
    memcpy(dod, This->data_queue, This->queue_pos * dodsize);
    
    /* Reset the event queue */
    This->queue_pos = 0;
  }
  
  /* Check if we need to do a mouse warping */
  if (This->need_warp) {
    TRACE(dinput, "Warping mouse to %ld - %ld\n", This->win_centerX, This->win_centerY);
    TSXWarpPointer(display, DefaultRootWindow(display),
		   This->xwin, 0, 0, 0, 0,
		   This->win_centerX, This->win_centerY);
    This->need_warp = 0;
  }
  
  return 0;
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
static HRESULT WINAPI SysMouseAImpl_SetProperty(LPDIRECTINPUTDEVICE2A iface,
					    REFGUID rguid,
					    LPCDIPROPHEADER ph)
{
  ICOM_THIS(SysMouseAImpl,iface);
  char	xbuf[50];

  if (HIWORD(rguid))
    WINE_StringFromCLSID(rguid,xbuf);
  else
    sprintf(xbuf,"<special guid %ld>",(DWORD)rguid);

  TRACE(dinput,"(this=%p,%s,%p)\n",This,xbuf,ph);
  
  if (!HIWORD(rguid)) {
    switch ((DWORD)rguid) {
    case DIPROP_BUFFERSIZE: {
      LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;
      
      TRACE(dinput,"buffersize = %ld\n",pd->dwData);

      This->data_queue = (LPDIDEVICEOBJECTDATA)HeapAlloc(GetProcessHeap(),0,
							  pd->dwData * sizeof(DIDEVICEOBJECTDATA));
      This->queue_pos  = 0;
      This->queue_len  = pd->dwData;
      break;
    }
    default:
      WARN(dinput,"Unknown type %ld\n",(DWORD)rguid);
      break;
    }
  }
  
  return 0;
}


static ICOM_VTABLE(IDirectInputDevice2A) SysKeyboardAvt={
	IDirectInputDevice2AImpl_QueryInterface,
	IDirectInputDevice2AImpl_AddRef,
	IDirectInputDevice2AImpl_Release,
	IDirectInputDevice2AImpl_GetCapabilities,
	IDirectInputDevice2AImpl_EnumObjects,
	IDirectInputDevice2AImpl_GetProperty,
	SysKeyboardAImpl_SetProperty,
	SysKeyboardAImpl_Acquire,
	SysKeyboardAImpl_Unacquire,
	SysKeyboardAImpl_GetDeviceState,
	SysKeyboardAImpl_GetDeviceData,
	IDirectInputDevice2AImpl_SetDataFormat,
	IDirectInputDevice2AImpl_SetEventNotification,
	IDirectInputDevice2AImpl_SetCooperativeLevel,
	IDirectInputDevice2AImpl_GetObjectInfo,
	IDirectInputDevice2AImpl_GetDeviceInfo,
	IDirectInputDevice2AImpl_RunControlPanel,
	IDirectInputDevice2AImpl_Initialize,
	IDirectInputDevice2AImpl_CreateEffect,
	IDirectInputDevice2AImpl_EnumEffects,
	IDirectInputDevice2AImpl_GetEffectInfo,
	IDirectInputDevice2AImpl_GetForceFeedbackState,
	IDirectInputDevice2AImpl_SendForceFeedbackCommand,
	IDirectInputDevice2AImpl_EnumCreatedEffectObjects,
	IDirectInputDevice2AImpl_Escape,
	IDirectInputDevice2AImpl_Poll,
	IDirectInputDevice2AImpl_SendDeviceData,
};

static ICOM_VTABLE(IDirectInputDevice2A) SysMouseAvt={
	IDirectInputDevice2AImpl_QueryInterface,
	IDirectInputDevice2AImpl_AddRef,
	SysMouseAImpl_Release,
	IDirectInputDevice2AImpl_GetCapabilities,
	IDirectInputDevice2AImpl_EnumObjects,
	IDirectInputDevice2AImpl_GetProperty,
	SysMouseAImpl_SetProperty,
	SysMouseAImpl_Acquire,
	SysMouseAImpl_Unacquire,
	SysMouseAImpl_GetDeviceState,
	SysMouseAImpl_GetDeviceData,
	SysMouseAImpl_SetDataFormat,
	IDirectInputDevice2AImpl_SetEventNotification,
	SysMouseAImpl_SetCooperativeLevel,
	IDirectInputDevice2AImpl_GetObjectInfo,
	IDirectInputDevice2AImpl_GetDeviceInfo,
	IDirectInputDevice2AImpl_RunControlPanel,
	IDirectInputDevice2AImpl_Initialize,
	IDirectInputDevice2AImpl_CreateEffect,
	IDirectInputDevice2AImpl_EnumEffects,
	IDirectInputDevice2AImpl_GetEffectInfo,
	IDirectInputDevice2AImpl_GetForceFeedbackState,
	IDirectInputDevice2AImpl_SendForceFeedbackCommand,
	IDirectInputDevice2AImpl_EnumCreatedEffectObjects,
	IDirectInputDevice2AImpl_Escape,
	IDirectInputDevice2AImpl_Poll,
	IDirectInputDevice2AImpl_SendDeviceData,
};
