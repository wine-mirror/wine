/*		DirectInput Keyboard device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 *
 */

#include "config.h"
#include <string.h>
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif

#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(dinput);

static ICOM_VTABLE(IDirectInputDevice2A) SysKeyboardAvt;
static ICOM_VTABLE(IDirectInputDevice7A) SysKeyboard7Avt;
     
typedef struct SysKeyboardAImpl SysKeyboardAImpl;
struct SysKeyboardAImpl
{
        /* IDirectInputDevice2AImpl */
        ICOM_VFIELD(IDirectInputDevice2A);
        DWORD                           ref;
        GUID                            guid;

	IDirectInputAImpl *dinput;
	
        /* SysKeyboardAImpl */
        BYTE                            keystate[256];
	int                             acquired;
};

static GUID DInput_Wine_Keyboard_GUID = { /* 0ab8648a-7735-11d2-8c73-71df54a96441 */
  0x0ab8648a,
  0x7735,
  0x11d2,
  {0x8c, 0x73, 0x71, 0xdf, 0x54, 0xa9, 0x64, 0x41}
};

static BOOL keyboarddev_enum_device(DWORD dwDevType, DWORD dwFlags, LPCDIDEVICEINSTANCEA lpddi)
{
  if ((dwDevType == 0) || (dwDevType == DIDEVTYPE_KEYBOARD)) {
    TRACE("Enumerating the Keyboard device\n");
    
    lpddi->guidInstance = GUID_SysKeyboard;/* DInput's GUID */
    lpddi->guidProduct = DInput_Wine_Keyboard_GUID; /* Vendor's GUID */
    lpddi->dwDevType = DIDEVTYPE_KEYBOARD | (DIDEVTYPEKEYBOARD_UNKNOWN << 8);
    strcpy(lpddi->tszInstanceName, "Keyboard");
    strcpy(lpddi->tszProductName, "Wine Keyboard");
    
    return TRUE;
  }

  return FALSE;
}

static SysKeyboardAImpl *alloc_device(REFGUID rguid, ICOM_VTABLE(IDirectInputDevice2A) *kvt, IDirectInputAImpl *dinput)
{
    SysKeyboardAImpl* newDevice;
    newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysKeyboardAImpl));
    newDevice->ref = 1;
    ICOM_VTBL(newDevice) = kvt;
    memcpy(&(newDevice->guid),rguid,sizeof(*rguid));
    memset(newDevice->keystate,0,256);
    newDevice->dinput = dinput;

    return newDevice;
}


static HRESULT keyboarddev_create_device(IDirectInputAImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev)
{
  if ((IsEqualGUID(&GUID_SysKeyboard,rguid)) ||          /* Generic Keyboard */
      (IsEqualGUID(&DInput_Wine_Keyboard_GUID,rguid))) { /* Wine Keyboard */
    if ((riid == NULL) || (IsEqualGUID(&IID_IDirectInputDevice2A,riid)) || (IsEqualGUID(&IID_IDirectInputDevice2A,riid))) {
      *pdev=(IDirectInputDeviceA*) alloc_device(rguid, &SysKeyboardAvt, dinput);
    
      TRACE("Creating a Keyboard device (%p)\n", *pdev);
      return DI_OK;
    } else if (IsEqualGUID(&IID_IDirectInputDevice7A,riid)) {
      *pdev=(IDirectInputDeviceA*) alloc_device(rguid, (ICOM_VTABLE(IDirectInputDevice2A) *) &SysKeyboard7Avt, dinput);
    
      TRACE("Creating a Keyboard DInput7A device (%p)\n", *pdev);
      return DI_OK;
    } else
      return DIERR_NOINTERFACE;
  }

  return DIERR_DEVICENOTREG;
}

static dinput_device keyboarddev = {
  100,
  keyboarddev_enum_device,
  keyboarddev_create_device
};

DECL_GLOBAL_CONSTRUCTOR(keyboarddev_register) { dinput_register_device(&keyboarddev); }

static HRESULT WINAPI SysKeyboardAImpl_SetProperty(
	LPDIRECTINPUTDEVICE2A iface,REFGUID rguid,LPCDIPROPHEADER ph
)
{
	ICOM_THIS(SysKeyboardAImpl,iface);

	TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(rguid),ph);
	TRACE("(size=%ld,headersize=%ld,obj=%ld,how=%ld\n",
            ph->dwSize,ph->dwHeaderSize,ph->dwObj,ph->dwHow);
	if (!HIWORD(rguid)) {
		switch ((DWORD)rguid) {
		case (DWORD) DIPROP_BUFFERSIZE: {
			LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;

			TRACE("(buffersize=%ld)\n",pd->dwData);
			break;
		}
		default:
			WARN("Unknown type %ld\n",(DWORD)rguid);
			break;
		}
	}
	return 0;
}

static HRESULT WINAPI SysKeyboardAImpl_GetDeviceState(
	LPDIRECTINPUTDEVICE2A iface,DWORD len,LPVOID ptr
)
{
    DWORD i;

    memset( ptr, 0, len );
    if (len != 256)
    {
        WARN("whoops, got len %ld?\n", len);
        return DI_OK;
    }
    for (i = 0; i < 0x80; i++)
    {
        WORD vkey = MapVirtualKeyA( i, 1 );
        if (vkey && (GetAsyncKeyState( vkey ) & 0x8000))
        {
            ((LPBYTE)ptr)[i] = 0x80;
            ((LPBYTE)ptr)[i | 0x80] = 0x80;
        }
    }
    return DI_OK;
}

static HRESULT WINAPI SysKeyboardAImpl_GetDeviceData(
	LPDIRECTINPUTDEVICE2A iface,DWORD dodsize,LPDIDEVICEOBJECTDATA dod,
	LPDWORD entries,DWORD flags
)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	int i, n;

	TRACE("(this=%p,%ld,%p,%p(%ld)),0x%08lx)\n",
	      This,dodsize,dod,entries,entries?*entries:0,flags);


        for (i = n = 0; (i < 0x80) && (n < *entries); i++)
        {
            WORD state, vkey = MapVirtualKeyA( i, 1 );
            if (!vkey) continue;
            state = (GetAsyncKeyState( vkey ) >> 8) & 0x80;
            if (state != This->keystate[vkey])
            {
                if (dod)
                {
                    /* add an entry */
                    dod[n].dwOfs       = i; /* scancode */
                    dod[n].dwData      = state;
                    dod[n].dwTimeStamp = GetCurrentTime(); /* umm */
                    dod[n].dwSequence  = This->dinput->evsequence++;
                    n++;
                }
                if (!(flags & DIGDD_PEEK)) This->keystate[vkey] = state;
            }
        }
        if (n) TRACE_(dinput)("%d entries\n",n);
        *entries = n;
        return DI_OK;
}

static HRESULT WINAPI SysKeyboardAImpl_Acquire(LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	
	TRACE("(this=%p)\n",This);
	
	if (This->acquired == 0) {
	  This->acquired = 1;
	}
	
	return DI_OK;
}

static HRESULT WINAPI SysKeyboardAImpl_Unacquire(LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	TRACE("(this=%p)\n",This);

	if (This->acquired == 1) {
	  This->acquired = 0;
	} else {
	  ERR("Unacquiring a not-acquired device !!!\n");
	}

	return DI_OK;
}

/******************************************************************************
  *     GetCapabilities : get the device capablitites
  */
static HRESULT WINAPI SysKeyboardAImpl_GetCapabilities(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIDEVCAPS lpDIDevCaps)
{
  ICOM_THIS(SysKeyboardAImpl,iface);

  TRACE("(this=%p,%p)\n",This,lpDIDevCaps);

  if (lpDIDevCaps->dwSize == sizeof(DIDEVCAPS)) {
    lpDIDevCaps->dwFlags = DIDC_ATTACHED;
    lpDIDevCaps->dwDevType = DIDEVTYPE_KEYBOARD;
    lpDIDevCaps->dwAxes = 0;
    lpDIDevCaps->dwButtons = 0;
    lpDIDevCaps->dwPOVs = 0;
    lpDIDevCaps->dwFFSamplePeriod = 0;
    lpDIDevCaps->dwFFMinTimeResolution = 0;
    lpDIDevCaps->dwFirmwareRevision = 100;
    lpDIDevCaps->dwHardwareRevision = 100;
    lpDIDevCaps->dwFFDriverVersion = 0;
  } else {
    /* DirectX 3.0 */
    FIXME("DirectX 3.0 not supported....\n");
  }
  
  return DI_OK;
}

static ICOM_VTABLE(IDirectInputDevice2A) SysKeyboardAvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectInputDevice2AImpl_QueryInterface,
	IDirectInputDevice2AImpl_AddRef,
	IDirectInputDevice2AImpl_Release,
	SysKeyboardAImpl_GetCapabilities,
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
	IDirectInputDevice2AImpl_SendDeviceData
};

#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)	(typeof(SysKeyboard7Avt.fun))
#else
# define XCAST(fun)	(void*)
#endif

static ICOM_VTABLE(IDirectInputDevice7A) SysKeyboard7Avt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	XCAST(QueryInterface)IDirectInputDevice2AImpl_QueryInterface,
	XCAST(AddRef)IDirectInputDevice2AImpl_AddRef,
	XCAST(Release)IDirectInputDevice2AImpl_Release,
	XCAST(GetCapabilities)SysKeyboardAImpl_GetCapabilities,
	XCAST(EnumObjects)IDirectInputDevice2AImpl_EnumObjects,
	XCAST(GetProperty)IDirectInputDevice2AImpl_GetProperty,
	XCAST(SetProperty)SysKeyboardAImpl_SetProperty,
	XCAST(Acquire)SysKeyboardAImpl_Acquire,
	XCAST(Unacquire)SysKeyboardAImpl_Unacquire,
	XCAST(GetDeviceState)SysKeyboardAImpl_GetDeviceState,
	XCAST(GetDeviceData)SysKeyboardAImpl_GetDeviceData,
	XCAST(SetDataFormat)IDirectInputDevice2AImpl_SetDataFormat,
	XCAST(SetEventNotification)IDirectInputDevice2AImpl_SetEventNotification,
	XCAST(SetCooperativeLevel)IDirectInputDevice2AImpl_SetCooperativeLevel,
	XCAST(GetObjectInfo)IDirectInputDevice2AImpl_GetObjectInfo,
	XCAST(GetDeviceInfo)IDirectInputDevice2AImpl_GetDeviceInfo,
	XCAST(RunControlPanel)IDirectInputDevice2AImpl_RunControlPanel,
	XCAST(Initialize)IDirectInputDevice2AImpl_Initialize,
	XCAST(CreateEffect)IDirectInputDevice2AImpl_CreateEffect,
	XCAST(EnumEffects)IDirectInputDevice2AImpl_EnumEffects,
	XCAST(GetEffectInfo)IDirectInputDevice2AImpl_GetEffectInfo,
	XCAST(GetForceFeedbackState)IDirectInputDevice2AImpl_GetForceFeedbackState,
	XCAST(SendForceFeedbackCommand)IDirectInputDevice2AImpl_SendForceFeedbackCommand,
	XCAST(EnumCreatedEffectObjects)IDirectInputDevice2AImpl_EnumCreatedEffectObjects,
	XCAST(Escape)IDirectInputDevice2AImpl_Escape,
	XCAST(Poll)IDirectInputDevice2AImpl_Poll,
	XCAST(SendDeviceData)IDirectInputDevice2AImpl_SendDeviceData,
	IDirectInputDevice7AImpl_EnumEffectsInFile,
	IDirectInputDevice7AImpl_WriteEffectToFile
};

#undef XCAST
