/*		DirectInput
 *
 * Copyright 1998 Marcus Meissner
 *
 * Additions (mouse support) Copyright 1998 Lionel Ulmer
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

#include "ts_xlib.h"

#include "config.h"
#include <string.h>
#include <unistd.h>
#include <assert.h>
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
#include "message.h"

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
	    (!memcmp(&DInput_Wine_Mouse_GUID,rguid,sizeof(GUID_SysKeyboard)))) { /* Wine Mouse */
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
	TRACE(dinput,"(this=%p,%s,%p)\n",this,xbuf,ph);
	if (!HIWORD(rguid)) {
		switch ((DWORD)rguid) {
		case DIPROP_BUFFERSIZE: {
			LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;

			TRACE(dinput,"buffersize = %ld\n",pd->dwData);
			break;
		}
		default:
			WARN(dinput,"Unknown type %ld\n",(DWORD)rguid);
			break;
		}
	}
	return 0;
}

static HRESULT WINAPI IDirectInputDeviceA_SetEventNotification(
	LPDIRECTINPUTDEVICE32A this,HANDLE32 hnd
) {
	FIXME(dinput,"(this=%p,0x%08lx): stub\n",this,(DWORD)hnd);
	return 0;
}

static HRESULT WINAPI IDirectInputDeviceA_GetDeviceData(
	LPDIRECTINPUTDEVICE32A this,DWORD dodsize,LPDIDEVICEOBJECTDATA dod,
	LPDWORD entries,DWORD flags
) {
  TRACE(dinput,"IDirectInputDeviceA(%p)->GetDeviceData(%ld,%p,%p(0x%08lx),0x%08lx)\n",
	this,dodsize,dod,entries,*entries,flags);
	return 0;
}


static HRESULT WINAPI IDirectInputDeviceA_Acquire(LPDIRECTINPUTDEVICE32A this) {
	TRACE(dinput,"(this=%p): stub\n",this);
	return 0;
}

static HRESULT WINAPI IDirectInputDeviceA_Unacquire(LPDIRECTINPUTDEVICE32A this) {
        TRACE(dinput,"(this=%p): stub\n",this);
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
	return E_FAIL;
}

/******************************************************************************
 *	SysMouseA (DInput Mouse support)
 */

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
  if (df->dwFlags == DIDF_ABSAXIS)
    mthis->absolute = 1;
  else {
    DWORD rx, ry;
    
    /* We need to get the initial "previous" position to be able
       to return deltas */
    mthis->absolute = 0;
  
    /* Get the mouse position */
    EVENT_QueryPointer(&rx, &ry, NULL);

    /* Fill the initial mouse state structure */
    mthis->prevX = rx;
    mthis->prevY = ry;
  }
  
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
  TRACE(dinput,"(X:%d - Y:%d)\n", rx, ry);

  /* Fill the mouse state structure */
  if (mthis->absolute) {
    mstate->lX = rx;
    mstate->lY = ry;
  } else {
    mstate->lX = rx - mthis->prevX;
    mstate->lY = ry - mthis->prevY;
    /* Fill the previous positions */
    mthis->prevX = rx;
    mthis->prevY = ry;
  }
  mstate->lZ = 0;
  mstate->rgbButtons[0] = (state & MK_LBUTTON ? 0xFF : 0x00);
  mstate->rgbButtons[1] = (state & MK_RBUTTON ? 0xFF : 0x00);
  mstate->rgbButtons[2] = (state & MK_MBUTTON ? 0xFF : 0x00);
  mstate->rgbButtons[3] = 0x00;
  
  return 0;
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
	SysMouseA_GetDeviceState,
	IDirectInputDeviceA_GetDeviceData,
	SysMouseA_SetDataFormat,
	IDirectInputDeviceA_SetEventNotification,
	IDirectInputDeviceA_SetCooperativeLevel,
	(void*)15,
	(void*)16,
	(void*)17,
	(void*)18,
};
