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
#include <time.h>
#include <unistd.h>
#include <assert.h>
#ifdef HAVE_SYS_SIGNAL_H
# include <sys/signal.h>
#endif
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#ifdef HAVE_SYS_ERRNO_H
# include <sys/errno.h>
#endif
#ifdef HAVE_LINUX_JOYSTICK_H
# include <linux/joystick.h>
# define JOYDEV	"/dev/js0"
#endif
#include "wine/obj_base.h"
#include "debugtools.h"
#include "dinput.h"
#include "display.h"
#include "input.h"
#include "keyboard.h"
#include "message.h"
#include "mouse.h"
#include "sysmetrics.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"

DEFAULT_DEBUG_CHANNEL(dinput)


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
        ICOM_VFIELD(IDirectInputDevice2A);
        DWORD                           ref;
        GUID                            guid;
};

struct SysKeyboardAImpl
{
        /* IDirectInputDevice2AImpl */
        ICOM_VFIELD(IDirectInputDevice2A);
        DWORD                           ref;
        GUID                            guid;
        /* SysKeyboardAImpl */
        BYTE                            keystate[256];
	KEYBOARD_CONFIG                 initial_config;
	int                             acquired;
};

#ifdef HAVE_LINUX_22_JOYSTICK_API
typedef struct JoystickAImpl JoystickAImpl;
static ICOM_VTABLE(IDirectInputDevice2A) JoystickAvt;
struct JoystickAImpl
{
        /* IDirectInputDevice2AImpl */
        ICOM_VFIELD(IDirectInputDevice2A);
        DWORD                           ref;
        GUID                            guid;

	/* joystick private */
	int				joyfd;
	LPDIDATAFORMAT			df;
        HANDLE				hEvent;
	LONG				lMin,lMax,deadzone;
        LPDIDEVICEOBJECTDATA 		data_queue;
        int				queue_pos, queue_len;
	DIJOYSTATE			js;
};
#endif

struct SysMouseAImpl
{
        /* IDirectInputDevice2AImpl */
        ICOM_VFIELD(IDirectInputDevice2A);
        DWORD                           ref;
        GUID                            guid;

	LPDIDATAFORMAT			df;
        /* SysMouseAImpl */
        BYTE                            absolute;
        /* Previous position for relative moves */
        LONG				prevX, prevY;
        LPMOUSE_EVENT_PROC		prev_handler;
        HWND				win;
        DWORD				win_centerX, win_centerY;
        LPDIDEVICEOBJECTDATA 		data_queue;
        int				queue_pos, queue_len;
        int				need_warp;
        int				acquired;
        HANDLE				hEvent;
	CRITICAL_SECTION		crit;

	/* This is for mouse reporting. */
	struct DIMOUSESTATE2            m_state;
};

static int evsequence=0;


/* UIDs for Wine "drivers".
   When enumerating each device supporting DInput, they have two UIDs :
    - the 'windows' UID
    - a vendor UID */
#ifdef HAVE_LINUX_22_JOYSTICK_API
static GUID DInput_Wine_Joystick_GUID = { /* 9e573ed9-7734-11d2-8d4a-23903fb6bdf7 */
  0x9e573ed9,
  0x7734,
  0x11d2,
  {0x8d, 0x4a, 0x23, 0x90, 0x3f, 0xb6, 0xbd, 0xf7}
};
#endif
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
      DPRINTF("%s ",flags[i].name);
  DPRINTF("\n");
}

struct IDirectInputAImpl
{
        ICOM_VFIELD(IDirectInputA);
        DWORD                   ref;
};

/******************************************************************************
 *	DirectInputCreate32A
 */
HRESULT WINAPI DirectInputCreateA(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA *ppDI, LPUNKNOWN punkOuter)
{
	IDirectInputAImpl* This;
	TRACE("(0x%08lx,%04lx,%p,%p)\n",
		(DWORD)hinst,dwVersion,ppDI,punkOuter
	);
	This = (IDirectInputAImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDirectInputAImpl));
	This->ref = 1;
	ICOM_VTBL(This) = &ddiavt;
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

	TRACE("(this=%p,0x%04lx,%p,%p,%04lx)\n", This, dwDevType, lpCallback, pvRef, dwFlags);

	devInstance.dwSize = sizeof(DIDEVICEINSTANCEA);
	if ((dwDevType == 0) || (dwDevType == DIDEVTYPE_KEYBOARD)) {
		/* Return keyboard */
		devInstance.guidInstance = GUID_SysKeyboard;/* DInput's GUID */
		devInstance.guidProduct = DInput_Wine_Keyboard_GUID; /* Vendor's GUID */
		devInstance.dwDevType = DIDEVTYPE_KEYBOARD | (DIDEVTYPEKEYBOARD_UNKNOWN << 8);
		strcpy(devInstance.tszInstanceName, "Keyboard");
		strcpy(devInstance.tszProductName, "Wine Keyboard");

		ret = lpCallback(&devInstance, pvRef);
		TRACE("Keyboard registered\n");
		if (ret == DIENUM_STOP)
			return 0;
	}
  
	if ((dwDevType == 0) || (dwDevType == DIDEVTYPE_MOUSE)) {
		/* Return mouse */
		devInstance.guidInstance = GUID_SysMouse;/* DInput's GUID */
		devInstance.guidProduct = DInput_Wine_Mouse_GUID; /* Vendor's GUID */
		devInstance.dwDevType = DIDEVTYPE_MOUSE | (DIDEVTYPEMOUSE_UNKNOWN << 8);
		strcpy(devInstance.tszInstanceName, "Mouse");
		strcpy(devInstance.tszProductName, "Wine Mouse");

		ret = lpCallback(&devInstance, pvRef);
		TRACE("Mouse registered\n");
		if (ret == DIENUM_STOP)
			return 0;
	}
	if ((dwDevType == 0) || (dwDevType == DIDEVTYPE_JOYSTICK)) {
		/* check whether we have a joystick */
#ifdef HAVE_LINUX_22_JOYSTICK_API
		if (	(access(JOYDEV,O_RDONLY)!=-1)		||
			(errno!=ENODEV && errno!=ENOENT)
		) {
		    /* Return joystick */
		    devInstance.guidInstance	= GUID_Joystick;
		    devInstance.guidProduct	= DInput_Wine_Joystick_GUID;
		    /* we only support traditional joysticks for now */
		    devInstance.dwDevType	= DIDEVTYPE_JOYSTICK | DIDEVTYPEJOYSTICK_TRADITIONAL;
		    strcpy(devInstance.tszInstanceName,	"Joystick");
		    /* ioctl JSIOCGNAME(len) */
		    strcpy(devInstance.tszProductName,	"Wine Joystick");

		    ret = lpCallback(&devInstance,pvRef);
		    TRACE("Joystick registered\n");
		    if (ret == DIENUM_STOP)
			    return 0;
		}
#endif
	}
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
	
	TRACE("(this=%p,%s,%p,%p)\n",This,debugstr_guid(rguid),pdev,punk);
	if ((!memcmp(&GUID_SysKeyboard,rguid,sizeof(GUID_SysKeyboard))) ||          /* Generic Keyboard */
	    (!memcmp(&DInput_Wine_Keyboard_GUID,rguid,sizeof(GUID_SysKeyboard)))) { /* Wine Keyboard */
                SysKeyboardAImpl* newDevice;
		newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysKeyboardAImpl));
		newDevice->ref = 1;
		ICOM_VTBL(newDevice) = &SysKeyboardAvt;
		memcpy(&(newDevice->guid),rguid,sizeof(*rguid));
		memset(newDevice->keystate,0,256);
                *pdev=(IDirectInputDeviceA*)newDevice;
		return DI_OK;
	}
	if ((!memcmp(&GUID_SysMouse,rguid,sizeof(GUID_SysMouse))) ||             /* Generic Mouse */
	    (!memcmp(&DInput_Wine_Mouse_GUID,rguid,sizeof(GUID_SysMouse)))) { /* Wine Mouse */
                SysMouseAImpl* newDevice;
		newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysMouseAImpl));
		newDevice->ref = 1;
		ICOM_VTBL(newDevice) = &SysMouseAvt;
		InitializeCriticalSection(&(newDevice->crit));
		MakeCriticalSectionGlobal(&(newDevice->crit));
		memcpy(&(newDevice->guid),rguid,sizeof(*rguid));
                *pdev=(IDirectInputDeviceA*)newDevice;
		return DI_OK;
	}
#ifdef HAVE_LINUX_22_JOYSTICK_API
	if ((!memcmp(&GUID_Joystick,rguid,sizeof(GUID_Joystick))) ||
	    (!memcmp(&DInput_Wine_Joystick_GUID,rguid,sizeof(GUID_Joystick)))) {
                JoystickAImpl* newDevice;
		newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(JoystickAImpl));
		newDevice->ref		= 1;
		ICOM_VTBL(newDevice)    = &JoystickAvt;
		newDevice->joyfd	= -1;
		memcpy(&(newDevice->guid),rguid,sizeof(*rguid));
                *pdev=(IDirectInputDeviceA*)newDevice;
		return DI_OK;
	}
#endif
	return E_FAIL;
}

static HRESULT WINAPI IDirectInputAImpl_QueryInterface(
	LPDIRECTINPUTA iface,REFIID riid,LPVOID *ppobj
) {
	ICOM_THIS(IDirectInputAImpl,iface);

	TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
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
  
  FIXME("(%p)->(%s): stub\n",This,debugstr_guid(rguid));
  
  return DI_OK;
}

static HRESULT WINAPI IDirectInputAImpl_RunControlPanel(LPDIRECTINPUTA iface,
							HWND hwndOwner,
							DWORD dwFlags) {
  ICOM_THIS(IDirectInputAImpl,iface);
  FIXME("(%p)->(%08lx,%08lx): stub\n",This, (DWORD) hwndOwner, dwFlags);
  
  return DI_OK;
}

static ICOM_VTABLE(IDirectInputA) ddiavt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
		TRACE(dinput,"df.rgodf[%d].guid %s\n",i,debugstr_guid(df->rgodf[i].pguid));
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
	TRACE("(this=%p,0x%08lx,0x%08lx)\n",This,(DWORD)hwnd,dwflags);
	if (TRACE_ON(dinput))
	  _dump_cooperativelevel(dwflags);
	return 0;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_SetEventNotification(
	LPDIRECTINPUTDEVICE2A iface,HANDLE hnd
) {
	ICOM_THIS(IDirectInputDevice2AImpl,iface);
	FIXME("(this=%p,0x%08lx): stub\n",This,(DWORD)hnd);
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
	return KEYBOARD_Driver->pGetDIState(len, ptr)?DI_OK:E_FAIL;
}

static HRESULT WINAPI SysKeyboardAImpl_GetDeviceData(
	LPDIRECTINPUTDEVICE2A iface,DWORD dodsize,LPDIDEVICEOBJECTDATA dod,
	LPDWORD entries,DWORD flags
)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	HRESULT	ret;
	int	i;

	TRACE("(this=%p,%ld,%p,%p(%ld)),0x%08lx)\n",
	      This,dodsize,dod,entries,entries?*entries:0,flags);

	ret=KEYBOARD_Driver->pGetDIData(
		This->keystate, dodsize, dod, entries, flags)?DI_OK:E_FAIL;
	for (i=0;i<*entries;i++) {
		dod[i].dwTimeStamp = GetTickCount();
		dod[i].dwSequence = evsequence++;
	}
	return ret;
}

static HRESULT WINAPI SysKeyboardAImpl_Acquire(LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	
	TRACE("(this=%p)\n",This);
	
	if (This->acquired == 0) {
	  KEYBOARD_CONFIG no_auto;
	  
	  /* Save the original config */
	  KEYBOARD_Driver->pGetKeyboardConfig(&(This->initial_config));
	  
	  /* Now, remove auto-repeat */
	  no_auto.auto_repeat = FALSE;
	  KEYBOARD_Driver->pSetKeyboardConfig(&no_auto, WINE_KEYBOARD_CONFIG_AUTO_REPEAT);

	  This->acquired = 1;
	}
	
	return DI_OK;
}

static HRESULT WINAPI SysKeyboardAImpl_Unacquire(LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	TRACE("(this=%p)\n",This);

	if (This->acquired == 1) {
	  /* Restore the original configuration */
	  KEYBOARD_Driver->pSetKeyboardConfig(&(This->initial_config), 0xFFFFFFFF);
	  This->acquired = 0;
	} else {
	  ERR("Unacquiring a not-acquired device !!!\n");
	}

	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_QueryInterface(
	LPDIRECTINPUTDEVICE2A iface,REFIID riid,LPVOID *ppobj
)
{
	ICOM_THIS(IDirectInputDevice2AImpl,iface);

	TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
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
	lpDIDevCaps->dwFlags = DIDC_ATTACHED;
	FIXME("stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_EnumObjects(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIENUMDEVICEOBJECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
	FIXME("stub!\n");
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
	FIXME("stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_GetObjectInfo(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIDEVICEOBJECTINSTANCEA pdidoi,
	DWORD dwObj,
	DWORD dwHow)
{
	FIXME("stub!\n");
	return DI_OK;
}
	
static HRESULT WINAPI IDirectInputDevice2AImpl_GetDeviceInfo(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIDEVICEINSTANCEA pdidi)
{
	FIXME("stub!\n");
	return DI_OK;
}
	
static HRESULT WINAPI IDirectInputDevice2AImpl_RunControlPanel(
	LPDIRECTINPUTDEVICE2A iface,
	HWND hwndOwner,
	DWORD dwFlags)
{
	FIXME("stub!\n");
	return DI_OK;
}
	
static HRESULT WINAPI IDirectInputDevice2AImpl_Initialize(
	LPDIRECTINPUTDEVICE2A iface,
	HINSTANCE hinst,
	DWORD dwVersion,
	REFGUID rguid)
{
	FIXME("stub!\n");
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
	FIXME("stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_EnumEffects(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIENUMEFFECTSCALLBACKA lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
	FIXME("stub!\n");
	if (lpCallback)
		lpCallback(NULL, lpvRef);
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_GetEffectInfo(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIEFFECTINFOA lpdei,
	REFGUID rguid)
{
	FIXME("stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_GetForceFeedbackState(
	LPDIRECTINPUTDEVICE2A iface,
	LPDWORD pdwOut)
{
	FIXME("stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_SendForceFeedbackCommand(
	LPDIRECTINPUTDEVICE2A iface,
	DWORD dwFlags)
{
	FIXME("stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_EnumCreatedEffectObjects(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback,
	LPVOID lpvRef,
	DWORD dwFlags)
{
	FIXME("stub!\n");
	if (lpCallback)
		lpCallback(NULL, lpvRef);
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_Escape(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIEFFESCAPE lpDIEEsc)
{
	FIXME("stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_Poll(
	LPDIRECTINPUTDEVICE2A iface)
{
	FIXME("stub!\n");
	return DI_OK;
}

static HRESULT WINAPI IDirectInputDevice2AImpl_SendDeviceData(
	LPDIRECTINPUTDEVICE2A iface,
	DWORD cbObjectData,
	LPDIDEVICEOBJECTDATA rgdod,
	LPDWORD pdwInOut,
	DWORD dwFlags)
{
	FIXME("stub!\n");
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
	DeleteCriticalSection(&(This->crit));
	
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

  TRACE("(this=%p,0x%08lx,0x%08lx)\n",This,(DWORD)hwnd,dwflags);

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
  
  TRACE("(this=%p,%p)\n",This,df);

  TRACE("(df.dwSize=%ld)\n",df->dwSize);
  TRACE("(df.dwObjsize=%ld)\n",df->dwObjSize);
  TRACE("(df.dwFlags=0x%08lx)\n",df->dwFlags);
  TRACE("(df.dwDataSize=%ld)\n",df->dwDataSize);
  TRACE("(df.dwNumObjs=%ld)\n",df->dwNumObjs);

  for (i=0;i<df->dwNumObjs;i++) {

    TRACE("df.rgodf[%d].guid %s (%p)\n",i, debugstr_guid(df->rgodf[i].pguid), df->rgodf[i].pguid);
    TRACE("df.rgodf[%d].dwOfs %ld\n",i,df->rgodf[i].dwOfs);
    TRACE("dwType 0x%02x,dwInstance %d\n",DIDFT_GETTYPE(df->rgodf[i].dwType),DIDFT_GETINSTANCE(df->rgodf[i].dwType));
    TRACE("df.rgodf[%d].dwFlags 0x%08lx\n",i,df->rgodf[i].dwFlags);
  }

  /* Check size of data format to prevent crashes if the applications
     sends a smaller buffer */
  if ((df->dwDataSize != sizeof(struct DIMOUSESTATE)) &&
      (df->dwDataSize != sizeof(struct DIMOUSESTATE2))) {
    FIXME("non-standard mouse configuration not supported yet.");
    return DIERR_INVALIDPARAM;
  }
  
  /* For the moment, ignore these fields and return always as if
     c_dfDIMouse was passed as format... */

  /* Check if the mouse is in absolute or relative mode */
  if (df->dwFlags == DIDF_ABSAXIS)
    This->absolute = 1;
  else if (df->dwFlags == DIDF_RELAXIS)
    This->absolute = 0;
  else
    ERR("Neither absolute nor relative flag set.");
  
  This->df = HeapAlloc(GetProcessHeap(),0,df->dwSize+(df->dwNumObjs*df->dwObjSize));
  memcpy(This->df,df,df->dwSize+(df->dwNumObjs*df->dwObjSize));
  return 0;
}

#define GEN_EVENT(offset,data,xtime,seq)			\
{								\
  if (This->queue_pos < This->queue_len) {			\
    This->data_queue[This->queue_pos].dwOfs = offset;		\
    This->data_queue[This->queue_pos].dwData = data;		\
    This->data_queue[This->queue_pos].dwTimeStamp = xtime;	\
    This->data_queue[This->queue_pos].dwSequence = seq;		\
    This->queue_pos++;						\
  }								\
}

  
/* Our private mouse event handler */
static void WINAPI dinput_mouse_event( DWORD dwFlags, DWORD dx, DWORD dy,
				      DWORD cButtons, DWORD dwExtraInfo )
{
  DWORD posX, posY, keyState, xtime, extra;
  SysMouseAImpl* This = (SysMouseAImpl*) current_lock;
  
  EnterCriticalSection(&(This->crit));
  /* Mouse moved -> send event if asked */
  if (This->hEvent)
    SetEvent(This->hEvent);
  
  if (   !IsBadReadPtr( (LPVOID)dwExtraInfo, sizeof(WINE_MOUSEEVENT) )
      && ((WINE_MOUSEEVENT *)dwExtraInfo)->magic == WINE_MOUSEEVENT_MAGIC ) {
    WINE_MOUSEEVENT *wme = (WINE_MOUSEEVENT *)dwExtraInfo;
    keyState = wme->keyState;
    xtime = wme->time;
    extra = (DWORD)wme->hWnd;
    
    if ((dwFlags & MOUSEEVENTF_MOVE) &&
	(dwFlags & MOUSEEVENTF_ABSOLUTE)) {
      posX = (dx * GetSystemMetrics(SM_CXSCREEN)) >> 16;
      posY = (dy * GetSystemMetrics(SM_CYSCREEN)) >> 16;
    } else {
      posX = This->prevX;
      posY = This->prevY;
    }
  } else {
    ERR("Mouse event not supported...\n");
    LeaveCriticalSection(&(This->crit));
    return ;
  }

  TRACE(" %ld %ld ", posX, posY);

  if ( dwFlags & MOUSEEVENTF_MOVE ) {
    if (This->absolute) {
      if (posX != This->prevX)
	GEN_EVENT(DIMOFS_X, posX, xtime, 0);
      if (posY != This->prevY)
	GEN_EVENT(DIMOFS_Y, posY, xtime, 0);
    } else {
      /* Relative mouse input : the real fun starts here... */
      if (This->need_warp) {
	if (posX != This->prevX)
	  GEN_EVENT(DIMOFS_X, posX - This->prevX, xtime, evsequence++);
	if (posY != This->prevY)
	  GEN_EVENT(DIMOFS_Y, posY - This->prevY, xtime, evsequence++);
      } else {
	/* This is the first time the event handler has been called after a
	   GetData of GetState. */
	if (posX != This->win_centerX) {
	  GEN_EVENT(DIMOFS_X, posX - This->win_centerX, xtime, evsequence++);
	  This->need_warp = 1;
	}
	  
	if (posY != This->win_centerY) {
	  GEN_EVENT(DIMOFS_Y, posY - This->win_centerY, xtime, evsequence++);
	  This->need_warp = 1;
	}
      }
    }
  }
  if ( dwFlags & MOUSEEVENTF_LEFTDOWN ) {
    if (TRACE_ON(dinput))
      DPRINTF(" LD ");

    GEN_EVENT(DIMOFS_BUTTON0, 0xFF, xtime, evsequence++);
    This->m_state.rgbButtons[0] = 0xFF;
  }
  if ( dwFlags & MOUSEEVENTF_LEFTUP ) {
    if (TRACE_ON(dinput))
      DPRINTF(" LU ");

    GEN_EVENT(DIMOFS_BUTTON0, 0x00, xtime, evsequence++);
    This->m_state.rgbButtons[0] = 0x00;
  }
  if ( dwFlags & MOUSEEVENTF_RIGHTDOWN ) {
    if (TRACE_ON(dinput))
      DPRINTF(" RD ");

    GEN_EVENT(DIMOFS_BUTTON1, 0xFF, xtime, evsequence++);
    This->m_state.rgbButtons[1] = 0xFF;
  }
  if ( dwFlags & MOUSEEVENTF_RIGHTUP ) {
    if (TRACE_ON(dinput))
      DPRINTF(" RU ");

    GEN_EVENT(DIMOFS_BUTTON1, 0x00, xtime, evsequence++);
    This->m_state.rgbButtons[1] = 0x00;
  }
  if ( dwFlags & MOUSEEVENTF_MIDDLEDOWN ) {
    if (TRACE_ON(dinput))
      DPRINTF(" MD ");

    GEN_EVENT(DIMOFS_BUTTON2, 0xFF, xtime, evsequence++);
    This->m_state.rgbButtons[2] = 0xFF;
  }
  if ( dwFlags & MOUSEEVENTF_MIDDLEUP ) {
    if (TRACE_ON(dinput))
      DPRINTF(" MU ");

    GEN_EVENT(DIMOFS_BUTTON2, 0x00, xtime, evsequence++);
    This->m_state.rgbButtons[2] = 0x00;
  }
  if (TRACE_ON(dinput))
    DPRINTF("\n");
  
  This->prevX = posX;
  This->prevY = posY;

  if (This->absolute) {
    This->m_state.lX = posX;
    This->m_state.lY = posY;
  } else {
    This->m_state.lX = posX - This->win_centerX;
    This->m_state.lY = posY - This->win_centerY;
  }
  
  LeaveCriticalSection(&(This->crit));

}


/******************************************************************************
  *     Acquire : gets exclusive control of the mouse
  */
static HRESULT WINAPI SysMouseAImpl_Acquire(LPDIRECTINPUTDEVICE2A iface)
{
  ICOM_THIS(SysMouseAImpl,iface);
  RECT	rect;
  
  TRACE("(this=%p)\n",This);

  if (This->acquired == 0) {
    POINT       point;
    int i;
    
    /* This stores the current mouse handler. */
    This->prev_handler = mouse_event;
    
    /* Store (in a global variable) the current lock */
    current_lock = (IDirectInputDevice2A*)This;

    /* Init the mouse state */
    This->m_state.lX = PosX;
    This->m_state.lY = PosY;
    This->m_state.lZ = 0;
    This->m_state.rgbButtons[0] = (MouseButtonsStates[0] ? 0xFF : 0x00);
    This->m_state.rgbButtons[1] = (MouseButtonsStates[1] ? 0xFF : 0x00);
    This->m_state.rgbButtons[2] = (MouseButtonsStates[2] ? 0xFF : 0x00);
    for (i = 0; i < 8; i++) 
      This->m_state.rgbButtons[i] = 0x00;

    /* Install our own mouse event handler */
    MOUSE_Enable(dinput_mouse_event);
    
    /* Get the window dimension and find the center */
    GetWindowRect(This->win, &rect);
    This->win_centerX = (rect.right  - rect.left) / 2;
    This->win_centerY = (rect.bottom - rect.top ) / 2;

    /* Warp the mouse to the center of the window */
    TRACE("Warping mouse to %ld - %ld\n", This->win_centerX, This->win_centerY);
    point.x = This->win_centerX;
    point.y = This->win_centerY;
    MapWindowPoints(This->win, HWND_DESKTOP, &point, 1);
    DISPLAY_MoveCursor(point.x, point.y);

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

  TRACE("(this=%p)\n",This);

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
  
  EnterCriticalSection(&(This->crit));
  TRACE("(this=%p,0x%08lx,%p): \n",This,len,ptr);
  
  /* Check if the buffer is big enough */
  if ((len != sizeof(struct DIMOUSESTATE)) &&
      (len != sizeof(struct DIMOUSESTATE2))) {
    FIXME("unsupported state structure.");
    LeaveCriticalSection(&(This->crit));
    return DIERR_INVALIDPARAM;
  }

  /* Copy the current mouse state */
  memcpy(ptr, &(This->m_state), len);
  
  /* Check if we need to do a mouse warping */
  if (This->need_warp) {
    POINT point;

    TRACE("Warping mouse to %ld - %ld\n", This->win_centerX, This->win_centerY);
    point.x = This->win_centerX;
    point.y = This->win_centerY;
    MapWindowPoints(This->win, HWND_DESKTOP, &point, 1);
    DISPLAY_MoveCursor(point.x, point.y);

    This->need_warp = 0;
  }

  LeaveCriticalSection(&(This->crit));
  
  TRACE("(X: %ld - Y: %ld   L: %02x M: %02x R: %02x)\n",
	This->m_state.lX, This->m_state.lY,
	This->m_state.rgbButtons[0], This->m_state.rgbButtons[2], This->m_state.rgbButtons[1]);
  
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
  
  EnterCriticalSection(&(This->crit));
  TRACE("(%p)->(dods=%ld,entries=%ld,fl=0x%08lx)\n",This,dodsize,*entries,flags);

  if (flags & DIGDD_PEEK)
    FIXME("DIGDD_PEEK\n");

  if (dod == NULL) {
    *entries = This->queue_pos;
    This->queue_pos = 0;
  } else {
    /* Check for buffer overflow */
    if (This->queue_pos > *entries) {
      WARN("Buffer overflow not handled properly yet...\n");
      This->queue_pos = *entries;
    }
    if (dodsize != sizeof(DIDEVICEOBJECTDATA)) {
      ERR("Wrong structure size !\n");
      LeaveCriticalSection(&(This->crit));
      return DIERR_INVALIDPARAM;
    }

    if (This->queue_pos)
    	TRACE("Application retrieving %d event(s).\n", This->queue_pos); 
    
    /* Copy the buffered data into the application queue */
    memcpy(dod, This->data_queue, This->queue_pos * dodsize);
    *entries = This->queue_pos;

    /* Reset the event queue */
    This->queue_pos = 0;
  }
  LeaveCriticalSection(&(This->crit));
  
#if 0 /* FIXME: seems to create motion events, which fire back at us. */
  /* Check if we need to do a mouse warping */
  if (This->need_warp) {
    POINT point;

    TRACE("Warping mouse to %ld - %ld\n", This->win_centerX, This->win_centerY);
    point.x = This->win_centerX;
    point.y = This->win_centerY;
    MapWindowPoints(This->win, HWND_DESKTOP, &point, 1);

    DISPLAY_MoveCursor(point.x, point.y);

    This->need_warp = 0;
  }
#endif
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

  TRACE("(this=%p,%s,%p)\n",This,debugstr_guid(rguid),ph);
  
  if (!HIWORD(rguid)) {
    switch ((DWORD)rguid) {
    case (DWORD) DIPROP_BUFFERSIZE: {
      LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;
      
      TRACE("buffersize = %ld\n",pd->dwData);

      This->data_queue = (LPDIDEVICEOBJECTDATA)HeapAlloc(GetProcessHeap(),0,
							  pd->dwData * sizeof(DIDEVICEOBJECTDATA));
      This->queue_pos  = 0;
      This->queue_len  = pd->dwData;
      break;
    }
    default:
      FIXME("Unknown type %ld (%s)\n",(DWORD)rguid,debugstr_guid(rguid));
      break;
    }
  }
  
  return 0;
}

/******************************************************************************
  *     SetEventNotification : specifies event to be sent on state change
  */
static HRESULT WINAPI SysMouseAImpl_SetEventNotification(LPDIRECTINPUTDEVICE2A iface,
							 HANDLE hnd) {
  ICOM_THIS(SysMouseAImpl,iface);

  TRACE("(this=%p,0x%08lx)\n",This,(DWORD)hnd);

  This->hEvent = hnd;

  return DI_OK;
}

#ifdef HAVE_LINUX_22_JOYSTICK_API
/******************************************************************************
 *	Joystick
 */
static ULONG WINAPI JoystickAImpl_Release(LPDIRECTINPUTDEVICE2A iface)
{
	ICOM_THIS(JoystickAImpl,iface);

	This->ref--;
	if (This->ref)
		return This->ref;
	HeapFree(GetProcessHeap(),0,This);
	return 0;
}

/******************************************************************************
  *   SetDataFormat : the application can choose the format of the data
  *   the device driver sends back with GetDeviceState.
  */
static HRESULT WINAPI JoystickAImpl_SetDataFormat(
	LPDIRECTINPUTDEVICE2A iface,LPCDIDATAFORMAT df
)
{
  ICOM_THIS(JoystickAImpl,iface);
  int i;
  
  TRACE("(this=%p,%p)\n",This,df);

  TRACE("(df.dwSize=%ld)\n",df->dwSize);
  TRACE("(df.dwObjsize=%ld)\n",df->dwObjSize);
  TRACE("(df.dwFlags=0x%08lx)\n",df->dwFlags);
  TRACE("(df.dwDataSize=%ld)\n",df->dwDataSize);
  TRACE("(df.dwNumObjs=%ld)\n",df->dwNumObjs);

  for (i=0;i<df->dwNumObjs;i++) {
    TRACE("df.rgodf[%d].guid %s (%p)\n",i,debugstr_guid(df->rgodf[i].pguid), df->rgodf[i].pguid);
    TRACE("df.rgodf[%d].dwOfs %ld\n",i,df->rgodf[i].dwOfs);
    TRACE("dwType 0x%02x,dwInstance %d\n",DIDFT_GETTYPE(df->rgodf[i].dwType),DIDFT_GETINSTANCE(df->rgodf[i].dwType));
    TRACE("df.rgodf[%d].dwFlags 0x%08lx\n",i,df->rgodf[i].dwFlags);
  }
  This->df = HeapAlloc(GetProcessHeap(),0,df->dwSize+(df->dwNumObjs*df->dwObjSize));
  memcpy(This->df,df,df->dwSize+(df->dwNumObjs*df->dwObjSize));
  return 0;
}

/******************************************************************************
  *     Acquire : gets exclusive control of the joystick
  */
static HRESULT WINAPI JoystickAImpl_Acquire(LPDIRECTINPUTDEVICE2A iface)
{
    ICOM_THIS(JoystickAImpl,iface);
  
    TRACE("(this=%p)\n",This);
    if (This->joyfd!=-1)
    	return 0;
    This->joyfd=open(JOYDEV,O_RDONLY);
    if (This->joyfd==-1)
    	return DIERR_NOTFOUND;
    return 0;
}

/******************************************************************************
  *     Unacquire : frees the joystick
  */
static HRESULT WINAPI JoystickAImpl_Unacquire(LPDIRECTINPUTDEVICE2A iface)
{
    ICOM_THIS(JoystickAImpl,iface);

    TRACE("(this=%p)\n",This);
    if (This->joyfd!=-1) {
  	close(This->joyfd);
	This->joyfd = -1;
    }
    return 0;
}

#define map_axis(val) ((val+32768)*(This->lMax-This->lMin)/65536+This->lMin)

static void joy_polldev(JoystickAImpl *This) {
    struct timeval tv;
    fd_set	readfds;
    struct	js_event jse;

    if (This->joyfd==-1)
	return;
    while (1) {
	memset(&tv,0,sizeof(tv));
	FD_ZERO(&readfds);FD_SET(This->joyfd,&readfds);
	if (1>select(This->joyfd+1,&readfds,NULL,NULL,&tv))
	    return;
	/* we have one event, so we can read */
	if (sizeof(jse)!=read(This->joyfd,&jse,sizeof(jse))) {
	    return;
	}
	TRACE("js_event: type 0x%x, number %d, value %d\n",jse.type,jse.number,jse.value);
	if (jse.type & JS_EVENT_BUTTON) {
	    GEN_EVENT(DIJOFS_BUTTON(jse.number),jse.value?0x80:0x00,jse.time,evsequence++);
	    This->js.rgbButtons[jse.number] = jse.value?0x80:0x00;
	}
	if (jse.type & JS_EVENT_AXIS) {
	    switch (jse.number) {
	    case 0:
		GEN_EVENT(jse.number*4,jse.value,jse.time,evsequence++);
		This->js.lX = map_axis(jse.value);
		break;
	    case 1:
		GEN_EVENT(jse.number*4,jse.value,jse.time,evsequence++);
		This->js.lY = map_axis(jse.value);
		break;
	    case 2:
		GEN_EVENT(jse.number*4,jse.value,jse.time,evsequence++);
		This->js.lZ = map_axis(jse.value);
		break;
	    default:
		FIXME("more then 3 axes (%d) not handled!\n",jse.number);
		break;
	    }
	}
    }
}

/******************************************************************************
  *     GetDeviceState : returns the "state" of the joystick.
  *
  */
static HRESULT WINAPI JoystickAImpl_GetDeviceState(
	LPDIRECTINPUTDEVICE2A iface,DWORD len,LPVOID ptr
) {
    ICOM_THIS(JoystickAImpl,iface);
  
    joy_polldev(This);
    TRACE("(this=%p,0x%08lx,%p)\n",This,len,ptr);
    if (len != sizeof(DIJOYSTATE)) {
    	FIXME("len %ld is not sizeof(DIJOYSTATE), unsupported format.\n",len);
    }
    memcpy(ptr,&(This->js),len);
    This->queue_pos = 0;
    return 0;
}

/******************************************************************************
  *     GetDeviceState : gets buffered input data.
  */
static HRESULT WINAPI JoystickAImpl_GetDeviceData(LPDIRECTINPUTDEVICE2A iface,
					      DWORD dodsize,
					      LPDIDEVICEOBJECTDATA dod,
					      LPDWORD entries,
					      DWORD flags
) {
  ICOM_THIS(JoystickAImpl,iface);
  
  FIXME("(%p)->(dods=%ld,entries=%ld,fl=0x%08lx),STUB!\n",This,dodsize,*entries,flags);

  joy_polldev(This);
  if (flags & DIGDD_PEEK)
    FIXME("DIGDD_PEEK\n");

  if (dod == NULL) {
  } else {
  }
  return 0;
}

/******************************************************************************
  *     SetProperty : change input device properties
  */
static HRESULT WINAPI JoystickAImpl_SetProperty(LPDIRECTINPUTDEVICE2A iface,
					    REFGUID rguid,
					    LPCDIPROPHEADER ph)
{
  ICOM_THIS(JoystickAImpl,iface);

  FIXME("(this=%p,%s,%p)\n",This,debugstr_guid(rguid),ph);
  FIXME("ph.dwSize = %ld, ph.dwHeaderSize =%ld, ph.dwObj = %ld, ph.dwHow= %ld\n",ph->dwSize, ph->dwHeaderSize,ph->dwObj,ph->dwHow);
  
  if (!HIWORD(rguid)) {
    switch ((DWORD)rguid) {
    case (DWORD) DIPROP_BUFFERSIZE: {
      LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;

      FIXME("buffersize = %ld\n",pd->dwData);
      break;
    }
    case (DWORD)DIPROP_RANGE: {
      LPCDIPROPRANGE	pr = (LPCDIPROPRANGE)ph;

      FIXME("proprange(%ld,%ld)\n",pr->lMin,pr->lMax);
      This->lMin = pr->lMin;
      This->lMax = pr->lMax;
      break;
    }
    case (DWORD)DIPROP_DEADZONE: {
      LPCDIPROPDWORD	pd = (LPCDIPROPDWORD)ph;

      FIXME("deadzone(%ld)\n",pd->dwData);
      This->deadzone = pd->dwData;
      break;
    }
    default:
      FIXME("Unknown type %ld (%s)\n",(DWORD)rguid,debugstr_guid(rguid));
      break;
    }
  }
  return 0;
}

/******************************************************************************
  *     SetEventNotification : specifies event to be sent on state change
  */
static HRESULT WINAPI JoystickAImpl_SetEventNotification(
	LPDIRECTINPUTDEVICE2A iface, HANDLE hnd
) {
    ICOM_THIS(JoystickAImpl,iface);

    TRACE("(this=%p,0x%08lx)\n",This,(DWORD)hnd);
    This->hEvent = hnd;
    return DI_OK;
}

static HRESULT WINAPI JoystickAImpl_GetCapabilities(
	LPDIRECTINPUTDEVICE2A iface,
	LPDIDEVCAPS lpDIDevCaps)
{
    ICOM_THIS(JoystickAImpl,iface);
    BYTE	axes,buttons;
    int		xfd = This->joyfd;

    TRACE("%p->(%p)\n",iface,lpDIDevCaps);
    if (xfd==-1)
    	xfd = open(JOYDEV,O_RDONLY);
    lpDIDevCaps->dwFlags	= DIDC_ATTACHED;
    lpDIDevCaps->dwDevType	= DIDEVTYPE_JOYSTICK;
#ifdef JSIOCGAXES
    if (-1==ioctl(xfd,JSIOCGAXES,&axes))
    	axes = 2;
    lpDIDevCaps->dwAxes = axes;
#endif
#ifdef JSIOCGBUTTONS
    if (-1==ioctl(xfd,JSIOCGAXES,&buttons))
    	buttons = 2;
    lpDIDevCaps->dwButtons = buttons;
#endif
    if (xfd!=This->joyfd)
    	close(xfd);
    return DI_OK;
}
static HRESULT WINAPI JoystickAImpl_Poll(LPDIRECTINPUTDEVICE2A iface) {
    ICOM_THIS(JoystickAImpl,iface);
    TRACE("(),stub!\n");

    joy_polldev(This);
    return DI_OK;
}
#endif

/****************************************************************************/
/****************************************************************************/

static ICOM_VTABLE(IDirectInputDevice2A) SysKeyboardAvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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

static ICOM_VTABLE(IDirectInputDevice2A) SysMouseAvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
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
	SysMouseAImpl_SetEventNotification,
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

#ifdef HAVE_LINUX_22_JOYSTICK_API
static ICOM_VTABLE(IDirectInputDevice2A) JoystickAvt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDirectInputDevice2AImpl_QueryInterface,
	IDirectInputDevice2AImpl_AddRef,
	JoystickAImpl_Release,
	JoystickAImpl_GetCapabilities,
	IDirectInputDevice2AImpl_EnumObjects,
	IDirectInputDevice2AImpl_GetProperty,
	JoystickAImpl_SetProperty,
	JoystickAImpl_Acquire,
	JoystickAImpl_Unacquire,
	JoystickAImpl_GetDeviceState,
	JoystickAImpl_GetDeviceData,
	JoystickAImpl_SetDataFormat,
	JoystickAImpl_SetEventNotification,
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
	JoystickAImpl_Poll,
	IDirectInputDevice2AImpl_SendDeviceData,
};
#endif
