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

#include "windows.h"
#include "winerror.h"
#include "shell.h"
#include "gdi.h"
#include "heap.h"
#include "win.h"
#include "dinput.h"
#include "objbase.h"
#include "debug.h"
#include "message.h"

#include "mouse.h"
#include "ts_xlib.h"
#include "sysmetrics.h"
#include "x11drv.h"

extern BYTE InputKeyStateTable[256];
extern int min_keycode, max_keycode;
extern WORD keyc2vkey[256];

static IDirectInputA_VTable ddiavt;
static IDirectInputDeviceA_VTable SysKeyboardAvt;
static IDirectInputDeviceA_VTable SysMouseAvt;

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

/* This is ugly and not thread safe :/ */
static LPDIRECTINPUTDEVICE32A current_lock = NULL;

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


/******************************************************************************
 *	DirectInputCreate32A
 */
HRESULT WINAPI DirectInputCreate32A(HINSTANCE32 hinst, DWORD dwVersion, LPDIRECTINPUT32A *ppDI, LPUNKNOWN punkOuter) {
	TRACE(dinput, "(0x%08lx,%04lx,%p,%p)\n",
		(DWORD)hinst,dwVersion,ppDI,punkOuter
	);
	(*ppDI) = (LPDIRECTINPUT32A)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectInput32A));
	(*ppDI)->ref = 1;
	(*ppDI)->lpvtbl = &ddiavt;
	return 0;
}
/******************************************************************************
 *	IDirectInputA_EnumDevices
 */
static HRESULT WINAPI IDirectInputA_EnumDevices(
	LPDIRECTINPUT32A this, DWORD dwDevType, LPDIENUMDEVICESCALLBACK32A lpCallback,
	LPVOID pvRef, DWORD dwFlags
) {
  DIDEVICEINSTANCE32A devInstance;
  int ret;

  TRACE(dinput, "(this=%p,0x%04lx,%p,%p,%04lx)\n", this, dwDevType, lpCallback, pvRef, dwFlags);

  devInstance.dwSize = sizeof(DIDEVICEINSTANCE32A);
  
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
	FIXME(dinput,"(this=%p,%s,%p,%p): stub\n",this,xbuf,pdev,punk);
	if ((!memcmp(&GUID_SysKeyboard,rguid,sizeof(GUID_SysKeyboard))) ||          /* Generic Keyboard */
	    (!memcmp(&DInput_Wine_Keyboard_GUID,rguid,sizeof(GUID_SysKeyboard)))) { /* Wine Keyboard */
		*pdev = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysKeyboard32A));
		(*pdev)->ref = 1;
		(*pdev)->lpvtbl = &SysKeyboardAvt;
		memcpy(&((*pdev)->guid),rguid,sizeof(*rguid));
		memset(((LPSYSKEYBOARD32A)(*pdev))->keystate,0,256);
		return 0;
	}
	if ((!memcmp(&GUID_SysMouse,rguid,sizeof(GUID_SysMouse))) ||             /* Generic Mouse */
	    (!memcmp(&DInput_Wine_Mouse_GUID,rguid,sizeof(GUID_SysMouse)))) { /* Wine Mouse */
		*pdev = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysMouse32A));
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
	TRACE(dinput,"(this=%p,%s,%p)\n",this,xbuf,ppobj);
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
	TRACE(dinput,"(this=%p,%p)\n",this,df);

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

static HRESULT WINAPI IDirectInputDeviceA_SetCooperativeLevel(
	LPDIRECTINPUTDEVICE32A this,HWND32 hwnd,DWORD dwflags
) {
	FIXME(dinput,"(this=%p,0x%08lx,0x%08lx): stub\n",this,(DWORD)hwnd,dwflags);
	if (TRACE_ON(dinput))
	  _dump_cooperativelevel(dwflags);
	return 0;
}

static HRESULT WINAPI IDirectInputDeviceA_SetEventNotification(
	LPDIRECTINPUTDEVICE32A this,HANDLE32 hnd
) {
	FIXME(dinput,"(this=%p,0x%08lx): stub\n",this,(DWORD)hnd);
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
	TRACE(dinput,"(this=%p,%s,%p)\n",this,xbuf,ph);
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

static HRESULT WINAPI SysKeyboardA_GetDeviceState(
	LPDIRECTINPUTDEVICE32A this,DWORD len,LPVOID ptr
) {
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
	WARN(dinput,"whoops, SysKeyboardA_GetDeviceState got len %ld?\n",len);
	return 0;
}

static HRESULT WINAPI SysKeyboardA_GetDeviceData(
	LPDIRECTINPUTDEVICE32A this,DWORD dodsize,LPDIDEVICEOBJECTDATA dod,
	LPDWORD entries,DWORD flags
) {
	int			keyc,n,vkey,xentries;
	LPSYSKEYBOARD32A	kthis = (LPSYSKEYBOARD32A)this;

	TRACE(dinput,"(this=%p,%ld,%p,%p(%ld)),0x%08lx)\n",
    		this,dodsize,dod,entries,entries?*entries:0,flags);
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
                    if (kthis->keystate[vkey] == (InputKeyStateTable[vkey]&0x80))
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
			kthis->keystate[vkey] = InputKeyStateTable[vkey]&0x80;
                    
        }
        
	if (n) fprintf(stderr,"%d entries\n",n);
	*entries = n;
	return 0;
}

static HRESULT WINAPI SysKeyboardA_Acquire(LPDIRECTINPUTDEVICE32A this) {
	TRACE(dinput,"(this=%p): stub\n",this);
	return 0;
}

static HRESULT WINAPI SysKeyboardA_Unacquire(LPDIRECTINPUTDEVICE32A this) {
	TRACE(dinput,"(this=%p): stub\n",this);
	return 0;
}

static HRESULT WINAPI IDirectInputDeviceA_QueryInterface(
	LPDIRECTINPUTDEVICE32A this,REFIID riid,LPVOID *ppobj
) {
	char	xbuf[50];

	WINE_StringFromCLSID(riid,xbuf);
	TRACE(dinput,"(this=%p,%s,%p)\n",this,xbuf,ppobj);
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
	if (!memcmp(&IID_IDirectInputDevice2A,riid,sizeof(*riid))) {
		this->lpvtbl->fnAddRef(this);
		*ppobj = this;
		return 0;
	}
	return E_FAIL;
}

static ULONG WINAPI IDirectInputDeviceA_AddRef(
	LPDIRECTINPUTDEVICE32A this)
{
	return ++this->ref;
}

static HRESULT WINAPI IDirectInputDeviceA_GetCapabilities(
	LPDIRECTINPUTDEVICE32A this,
	LPDIDEVCAPS lpDIDevCaps)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDeviceA_EnumObjects(
	LPDIRECTINPUTDEVICE32A this,
	LPDIENUMDEVICEOBJECTSCALLBACK32A lpCallback,
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
	
static HRESULT WINAPI IDirectInputDeviceA_GetProperty(
	LPDIRECTINPUTDEVICE32A this,
	REFGUID rguid,
	LPDIPROPHEADER pdiph)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDeviceA_GetObjectInfo(
	LPDIRECTINPUTDEVICE32A this,
	LPDIDEVICEOBJECTINSTANCE32A pdidoi,
	DWORD dwObj,
	DWORD dwHow)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}
	
static HRESULT WINAPI IDirectInputDeviceA_GetDeviceInfo(
	LPDIRECTINPUTDEVICE32A this,
	LPDIDEVICEINSTANCE32A pdidi)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}
	
static HRESULT WINAPI IDirectInputDeviceA_RunControlPanel(
	LPDIRECTINPUTDEVICE32A this,
	HWND32 hwndOwner,
	DWORD dwFlags)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}
	
static HRESULT WINAPI IDirectInputDeviceA_Initialize(
	LPDIRECTINPUTDEVICE32A this,
	HINSTANCE32 hinst,
	DWORD dwVersion,
	REFGUID rguid)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}
	
/******************************************************************************
 *	IDirectInputDevice2A
 */

static HRESULT WINAPI IDirectInputDevice2A_CreateEffect(
	LPDIRECTINPUTDEVICE32A this,
	REFGUID rguid,
	LPCDIEFFECT lpeff,
	LPDIRECTINPUTEFFECT *ppdef,
	LPUNKNOWN pUnkOuter)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2A_EnumEffects(
	LPDIRECTINPUTDEVICE32A this,
	LPDIENUMEFFECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
	FIXME(dinput, "stub!\n");
	if (lpCallback)
		lpCallback(NULL, lpvRef);
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2A_GetEffectInfo(
	LPDIRECTINPUTDEVICE32A this,
	LPDIEFFECTINFOA lpdei,
	REFGUID rguid)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2A_GetForceFeedbackState(
	LPDIRECTINPUTDEVICE32A this,
	LPDWORD pdwOut)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2A_SendForceFeedbackCommand(
	LPDIRECTINPUTDEVICE32A this,
	DWORD dwFlags)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2A_EnumCreatedEffectObjects(
	LPDIRECTINPUTDEVICE32A this,
	LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
	FIXME(dinput, "stub!\n");
	if (lpCallback)
		lpCallback(NULL, lpvRef);
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2A_Escape(
	LPDIRECTINPUTDEVICE32A this,
	LPDIEFFESCAPE lpDIEEsc)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2A_Poll(
	LPDIRECTINPUTDEVICE32A this)
{
	FIXME(dinput, "stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2A_SendDeviceData(
	LPDIRECTINPUTDEVICE32A this,
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
static ULONG WINAPI SysMouseA_Release(LPDIRECTINPUTDEVICE32A this) {
	LPSYSMOUSE32A mthis = (LPSYSMOUSE32A) this;

	this->ref--;
	if (this->ref)
		return this->ref;

	/* Free the data queue */
	if (mthis->data_queue != NULL)
	  HeapFree(GetProcessHeap(),0,mthis->data_queue);

	/* Install the previous event handler (in case of releasing an aquired
	   mouse device) */
	if (mthis->prev_handler != NULL)
	  MOUSE_Enable(mthis->prev_handler);
	
	HeapFree(GetProcessHeap(),0,this);
	return 0;
}


/******************************************************************************
  *     SetCooperativeLevel : store the window in which we will do our
  *   grabbing.
  */
static HRESULT WINAPI SysMouseA_SetCooperativeLevel(
	LPDIRECTINPUTDEVICE32A this,HWND32 hwnd,DWORD dwflags
) {
  LPSYSMOUSE32A mthis = (LPSYSMOUSE32A) this;

  TRACE(dinput,"(this=%p,0x%08lx,0x%08lx): stub\n",this,(DWORD)hwnd,dwflags);

  if (TRACE_ON(dinput))
    _dump_cooperativelevel(dwflags);

  /* Store the window which asks for the mouse */
  mthis->win = hwnd;
  
  return 0;
}


/******************************************************************************
  *     SetDataFormat : the application can choose the format of the data
  *   the device driver sends back with GetDeviceState.
  *
  *   For the moment, only the "standard" configuration (c_dfDIMouse) is supported
  *   in absolute and relative mode.
  */
static HRESULT WINAPI SysMouseA_SetDataFormat(
	LPDIRECTINPUTDEVICE32A this,LPCDIDATAFORMAT df
) {
  int i;
  LPSYSMOUSE32A mthis = (LPSYSMOUSE32A) this;
  
  TRACE(dinput,"(this=%p,%p)\n",this,df);

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
    mthis->absolute = 1;
  else 
    mthis->absolute = 0;
  
  return 0;
}

#define GEN_EVENT(offset,data,time,seq)						\
{										\
  if (mthis->queue_pos < mthis->queue_len) {					\
    mthis->data_queue[mthis->queue_pos].dwOfs = offset;				\
    mthis->data_queue[mthis->queue_pos].dwData = data;				\
    mthis->data_queue[mthis->queue_pos].dwTimeStamp = time;			\
    mthis->data_queue[mthis->queue_pos].dwSequence = seq;			\
    mthis->queue_pos++;								\
  }										\
  }
  
/* Our private mouse event handler */
static void WINAPI dinput_mouse_event( DWORD dwFlags, DWORD dx, DWORD dy,
				      DWORD cButtons, DWORD dwExtraInfo )
{
  DWORD posX, posY, keyState, time, extra;
  LPSYSMOUSE32A mthis = (LPSYSMOUSE32A) current_lock;
  
  if (   !IsBadReadPtr32( (LPVOID)dwExtraInfo, sizeof(WINE_MOUSEEVENT) )
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
    if (mthis->absolute) {
      if (posX != mthis->prevX)
	GEN_EVENT(DIMOFS_X, posX, time, 0);
      if (posY != mthis->prevY)
	GEN_EVENT(DIMOFS_Y, posY, time, 0);
    } else {
      /* Relative mouse input : the real fun starts here... */
      if (mthis->need_warp) {
	if (posX != mthis->prevX)
	  GEN_EVENT(DIMOFS_X, posX - mthis->prevX, time, 0);
	if (posY != mthis->prevY)
	  GEN_EVENT(DIMOFS_Y, posY - mthis->prevY, time, 0);
      } else {
	/* This is the first time the event handler has been called after a
	   GetData of GetState. */
	if (posX != mthis->win_centerX) {
	  GEN_EVENT(DIMOFS_X, posX - mthis->win_centerX, time, 0);
	  mthis->need_warp = 1;
	}
	  
	if (posY != mthis->win_centerY) {
	  GEN_EVENT(DIMOFS_Y, posY - mthis->win_centerY, time, 0);
	  mthis->need_warp = 1;
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

  mthis->prevX = posX;
  mthis->prevY = posY;
}


/******************************************************************************
  *     Acquire : gets exclusive control of the mouse
  */
static HRESULT WINAPI SysMouseA_Acquire(LPDIRECTINPUTDEVICE32A this) {
  LPSYSMOUSE32A mthis = (LPSYSMOUSE32A) this;
  RECT32	rect;
  
  TRACE(dinput,"(this=%p)\n",this);

  if (mthis->acquired == 0) {
    /* This stores the current mouse handler.
       FIXME : need to be fixed for native USER use */
    mthis->prev_handler = mouse_event;
    
    /* Store (in a global variable) the current lock */
    current_lock = this;
    
    /* Install our own mouse event handler */
    MOUSE_Enable(dinput_mouse_event);
    
    /* Get the window dimension and find the center */
    GetWindowRect32(mthis->win, &rect);
    mthis->xwin = ((X11DRV_WND_DATA *) WIN_FindWndPtr(mthis->win)->pDriverData)->window;
    mthis->win_centerX = (rect.right  - rect.left) / 2;
    mthis->win_centerY = (rect.bottom - rect.top ) / 2;
    /* Warp the mouse to the center of the window */
    TRACE(dinput, "Warping mouse to %ld - %ld\n", mthis->win_centerX, mthis->win_centerY);
    TSXWarpPointer(display, DefaultRootWindow(display),
		   mthis->xwin, 0, 0, 0, 0,
		   mthis->win_centerX, mthis->win_centerY);
    
    mthis->acquired = 1;
  }
  return 0;
}

/******************************************************************************
  *     Unacquire : frees the mouse
  */
static HRESULT WINAPI SysMouseA_Unacquire(LPDIRECTINPUTDEVICE32A this) {
  LPSYSMOUSE32A mthis = (LPSYSMOUSE32A) this;

  TRACE(dinput,"(this=%p)\n",this);

  /* Reinstall previous mouse event handler */
  MOUSE_Enable(mthis->prev_handler);
  mthis->prev_handler = NULL;
  
  /* No more locks */
  current_lock = NULL;

  /* Unacquire device */
  mthis->acquired = 0;
  
  return 0;
}

/******************************************************************************
  *     GetDeviceState : returns the "state" of the mouse.
  *
  *   For the moment, only the "standard" return structure (DIMOUSESTATE) is
  *   supported.
  */
static HRESULT WINAPI SysMouseA_GetDeviceState(
	LPDIRECTINPUTDEVICE32A this,DWORD len,LPVOID ptr
) {
  DWORD rx, ry, state;
  struct DIMOUSESTATE *mstate = (struct DIMOUSESTATE *) ptr;
  LPSYSMOUSE32A mthis = (LPSYSMOUSE32A) this;
  
  TRACE(dinput,"(this=%p,0x%08lx,%p): \n",this,len,ptr);
  
  /* Check if the buffer is big enough */
  if (len < sizeof(struct DIMOUSESTATE)) {
    FIXME(dinput, "unsupported state structure.");
    return DIERR_INVALIDPARAM;
  }
  
  /* Get the mouse position */
  EVENT_QueryPointer(&rx, &ry, &state);
  TRACE(dinput,"(X:%ld - Y:%ld)\n", rx, ry);

  /* Fill the mouse state structure */
  if (mthis->absolute) {
    mstate->lX = rx;
    mstate->lY = ry;
  } else {
    mstate->lX = rx - mthis->win_centerX;
    mstate->lY = ry - mthis->win_centerY;

    if ((mstate->lX != 0) || (mstate->lY != 0))
      mthis->need_warp = 1;
  }
  mstate->lZ = 0;
  mstate->rgbButtons[0] = (state & MK_LBUTTON ? 0xFF : 0x00);
  mstate->rgbButtons[1] = (state & MK_RBUTTON ? 0xFF : 0x00);
  mstate->rgbButtons[2] = (state & MK_MBUTTON ? 0xFF : 0x00);
  mstate->rgbButtons[3] = 0x00;
  
  /* Check if we need to do a mouse warping */
  if (mthis->need_warp) {
    TRACE(dinput, "Warping mouse to %ld - %ld\n", mthis->win_centerX, mthis->win_centerY);
    TSXWarpPointer(display, DefaultRootWindow(display),
		   mthis->xwin, 0, 0, 0, 0,
		   mthis->win_centerX, mthis->win_centerY);
    mthis->need_warp = 0;
  }
  
  TRACE(dinput, "(X: %ld - Y: %ld   L: %02x M: %02x R: %02x)\n",
	mstate->lX, mstate->lY,
	mstate->rgbButtons[0], mstate->rgbButtons[2], mstate->rgbButtons[1]);
  
  return 0;
}

/******************************************************************************
  *     GetDeviceState : gets buffered input data.
  */
static HRESULT WINAPI SysMouseA_GetDeviceData(LPDIRECTINPUTDEVICE32A this,
					      DWORD dodsize,
					      LPDIDEVICEOBJECTDATA dod,
					      LPDWORD entries,
					      DWORD flags
) {
  LPSYSMOUSE32A mthis = (LPSYSMOUSE32A) this;
  
  TRACE(dinput,"(%p)->(%ld,%p,%p(0x%08lx),0x%08lx)\n",
	this,dodsize,dod,entries,*entries,flags);

  if (flags & DIGDD_PEEK)
    TRACE(dinput, "DIGDD_PEEK\n");

  if (dod == NULL) {
    *entries = mthis->queue_pos;
    mthis->queue_pos = 0;
  } else {
    /* Check for buffer overflow */
    if (mthis->queue_pos > *entries) {
      WARN(dinput, "Buffer overflow not handled properly yet...\n");
      mthis->queue_pos = *entries;
    }
    if (dodsize != sizeof(DIDEVICEOBJECTDATA)) {
      ERR(dinput, "Wrong structure size !\n");
      return DIERR_INVALIDPARAM;
    }
    
    /* Copy the buffered data into the application queue */
    memcpy(dod, mthis->data_queue, mthis->queue_pos * dodsize);
    
    /* Reset the event queue */
    mthis->queue_pos = 0;
  }
  
  /* Check if we need to do a mouse warping */
  if (mthis->need_warp) {
    TRACE(dinput, "Warping mouse to %ld - %ld\n", mthis->win_centerX, mthis->win_centerY);
    TSXWarpPointer(display, DefaultRootWindow(display),
		   mthis->xwin, 0, 0, 0, 0,
		   mthis->win_centerX, mthis->win_centerY);
    mthis->need_warp = 0;
  }
  
  return 0;
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
static HRESULT WINAPI SysMouseA_SetProperty(LPDIRECTINPUTDEVICE32A this,
					    REFGUID rguid,
					    LPCDIPROPHEADER ph) {
  char	xbuf[50];
  LPSYSMOUSE32A mthis = (LPSYSMOUSE32A) this;

  if (HIWORD(rguid))
    WINE_StringFromCLSID(rguid,xbuf);
  else
    sprintf(xbuf,"<special guid %ld>",(DWORD)rguid);

  TRACE(dinput,"(this=%p,%s,%p)\n",this,xbuf,ph);
  
  if (!HIWORD(rguid)) {
    switch ((DWORD)rguid) {
    case DIPROP_BUFFERSIZE: {
      LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;
      
      TRACE(dinput,"buffersize = %ld\n",pd->dwData);

      mthis->data_queue = (LPDIDEVICEOBJECTDATA)HeapAlloc(GetProcessHeap(),0,
							  pd->dwData * sizeof(DIDEVICEOBJECTDATA));
      mthis->queue_pos  = 0;
      mthis->queue_len  = pd->dwData;
      break;
    }
    default:
      WARN(dinput,"Unknown type %ld\n",(DWORD)rguid);
      break;
    }
  }
  
  return 0;
}


static IDirectInputDeviceA_VTable SysKeyboardAvt={
	IDirectInputDeviceA_QueryInterface,
	IDirectInputDeviceA_AddRef,
	IDirectInputDeviceA_Release,
	IDirectInputDeviceA_GetCapabilities,
	IDirectInputDeviceA_EnumObjects,
	IDirectInputDeviceA_GetProperty,
	SysKeyboardA_SetProperty,
	SysKeyboardA_Acquire,
	SysKeyboardA_Unacquire,
	SysKeyboardA_GetDeviceState,
	SysKeyboardA_GetDeviceData,
	IDirectInputDeviceA_SetDataFormat,
	IDirectInputDeviceA_SetEventNotification,
	IDirectInputDeviceA_SetCooperativeLevel,
	IDirectInputDeviceA_GetObjectInfo,
	IDirectInputDeviceA_GetDeviceInfo,
	IDirectInputDeviceA_RunControlPanel,
	IDirectInputDeviceA_Initialize,
	IDirectInputDevice2A_CreateEffect,
	IDirectInputDevice2A_EnumEffects,
	IDirectInputDevice2A_GetEffectInfo,
	IDirectInputDevice2A_GetForceFeedbackState,
	IDirectInputDevice2A_SendForceFeedbackCommand,
	IDirectInputDevice2A_EnumCreatedEffectObjects,
	IDirectInputDevice2A_Escape,
	IDirectInputDevice2A_Poll,
	IDirectInputDevice2A_SendDeviceData,
};

static IDirectInputDeviceA_VTable SysMouseAvt={
	IDirectInputDeviceA_QueryInterface,
	IDirectInputDeviceA_AddRef,
	SysMouseA_Release,
	IDirectInputDeviceA_GetCapabilities,
	IDirectInputDeviceA_EnumObjects,
	IDirectInputDeviceA_GetProperty,
	SysMouseA_SetProperty,
	SysMouseA_Acquire,
	SysMouseA_Unacquire,
	SysMouseA_GetDeviceState,
	SysMouseA_GetDeviceData,
	SysMouseA_SetDataFormat,
	IDirectInputDeviceA_SetEventNotification,
	SysMouseA_SetCooperativeLevel,
	IDirectInputDeviceA_GetObjectInfo,
	IDirectInputDeviceA_GetDeviceInfo,
	IDirectInputDeviceA_RunControlPanel,
	IDirectInputDeviceA_Initialize,
	IDirectInputDevice2A_CreateEffect,
	IDirectInputDevice2A_EnumEffects,
	IDirectInputDevice2A_GetEffectInfo,
	IDirectInputDevice2A_GetForceFeedbackState,
	IDirectInputDevice2A_SendForceFeedbackCommand,
	IDirectInputDevice2A_EnumCreatedEffectObjects,
	IDirectInputDevice2A_Escape,
	IDirectInputDevice2A_Poll,
	IDirectInputDevice2A_SendDeviceData,
};
