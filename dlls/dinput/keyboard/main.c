/*		DirectInput Keyboard device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

static ICOM_VTABLE(IDirectInputDevice8A) SysKeyboardAvt;

typedef struct SysKeyboardAImpl SysKeyboardAImpl;
struct SysKeyboardAImpl
{
        LPVOID                          lpVtbl;
        DWORD                           ref;
        GUID                            guid;

	IDirectInputAImpl *dinput;

	HANDLE	hEvent;
        /* SysKeyboardAImpl */
	int                             acquired;
        int                             buffersize;  /* set in 'SetProperty'         */
        LPDIDEVICEOBJECTDATA            buffer;      /* buffer for 'GetDeviceData'.
                                                        Alloc at 'Acquire', Free at
                                                        'Unacquire'                  */
        int                             count;       /* number of objects in use in
                                                        'buffer'                     */
        int                             start;       /* 'buffer' rotates. This is the
                                                        first in use (if count > 0)  */
        BOOL                            overflow;    /* return DI_BUFFEROVERFLOW in
                                                        'GetDeviceData'              */
        CRITICAL_SECTION                crit;
};

SysKeyboardAImpl *current; /* Today's acquired device
FIXME: currently this can be only one.
Maybe this should be a linked list or st.
I don't know what the rules are for multiple acquired keyboards,
but 'DI_LOSTFOCUS' and 'DI_UNACQUIRED' exist for a reason.
*/

static BYTE DInputKeyState[256]; /* array for 'GetDeviceState' */

HHOOK keyboard_hook;

LRESULT CALLBACK KeyboardCallback( int code, WPARAM wparam, LPARAM lparam )
{
  TRACE("(%d,%d,%ld)\n", code, wparam, lparam);

  if (code == HC_ACTION)
    {
      BYTE dik_code;
      BOOL down;
      DWORD timestamp;

      {
        KBDLLHOOKSTRUCT *hook = (KBDLLHOOKSTRUCT *)lparam;
        dik_code = hook->scanCode;
        if (hook->flags & LLKHF_EXTENDED) dik_code |= 0x80;
        down = !(hook->flags & LLKHF_UP);
        timestamp = hook->time;
      }

      DInputKeyState[dik_code] = (down ? 0x80 : 0);

      if (current != NULL)
        {
          if (current->hEvent)
            SetEvent(current->hEvent);

          if (current->buffer != NULL)
            {
              int n;

              EnterCriticalSection(&(current->crit));

              n = (current->start + current->count) % current->buffersize;

              current->buffer[n].dwOfs = dik_code;
              current->buffer[n].dwData = down ? 0x80 : 0;
              current->buffer[n].dwTimeStamp = timestamp;
              current->buffer[n].dwSequence = current->dinput->evsequence++;

	      TRACE("Adding event at offset %d : %ld - %ld - %ld - %ld\n", n,
		    current->buffer[n].dwOfs, current->buffer[n].dwData, current->buffer[n].dwTimeStamp, current->buffer[n].dwSequence);

              if (current->count == current->buffersize)
                {
                  current->start++;
                  current->overflow = TRUE;
                }
              else
                current->count++;

              LeaveCriticalSection(&(current->crit));
            }
        }
    }

  return CallNextHookEx(keyboard_hook, code, wparam, lparam);
}

static GUID DInput_Wine_Keyboard_GUID = { /* 0ab8648a-7735-11d2-8c73-71df54a96441 */
  0x0ab8648a,
  0x7735,
  0x11d2,
  {0x8c, 0x73, 0x71, 0xdf, 0x54, 0xa9, 0x64, 0x41}
};

static BOOL keyboarddev_enum_device(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEA lpddi)
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

static SysKeyboardAImpl *alloc_device(REFGUID rguid, LPVOID kvt, IDirectInputAImpl *dinput)
{
    SysKeyboardAImpl* newDevice;
    newDevice = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(SysKeyboardAImpl));
    newDevice->lpVtbl = kvt;
    newDevice->ref = 1;
    memcpy(&(newDevice->guid),rguid,sizeof(*rguid));
    newDevice->dinput = dinput;

    return newDevice;
}


static HRESULT keyboarddev_create_device(IDirectInputAImpl *dinput, REFGUID rguid, REFIID riid, LPDIRECTINPUTDEVICEA* pdev)
{
  if ((IsEqualGUID(&GUID_SysKeyboard,rguid)) ||          /* Generic Keyboard */
      (IsEqualGUID(&DInput_Wine_Keyboard_GUID,rguid))) { /* Wine Keyboard */
    if ((riid == NULL) ||
	IsEqualGUID(&IID_IDirectInputDeviceA,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice2A,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice7A,riid) ||
	IsEqualGUID(&IID_IDirectInputDevice8A,riid)) {
      *pdev=(IDirectInputDeviceA*) alloc_device(rguid, &SysKeyboardAvt, dinput);
      TRACE("Creating a Keyboard device (%p)\n", *pdev);
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
	LPDIRECTINPUTDEVICE8A iface,REFGUID rguid,LPCDIPROPHEADER ph
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

                        if (This->acquired)
                           return DIERR_INVALIDPARAM;

                        This->buffersize = pd->dwData;

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
	LPDIRECTINPUTDEVICE8A iface,DWORD len,LPVOID ptr
)
{
    /* Note: device does not need to be acquired */
    if (len != 256)
      return DIERR_INVALIDPARAM;

    memcpy(ptr, DInputKeyState, 256);
    return DI_OK;
}

static HRESULT WINAPI SysKeyboardAImpl_GetDeviceData(
	LPDIRECTINPUTDEVICE8A iface,DWORD dodsize,LPDIDEVICEOBJECTDATA dod,
	LPDWORD entries,DWORD flags
)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	int ret = DI_OK, i = 0;

	TRACE("(this=%p,%ld,%p,%p(%ld)),0x%08lx)\n",
	      This,dodsize,dod,entries,entries?*entries:0,flags);

	if (This->acquired == 0)
	  return DIERR_NOTACQUIRED;

        if (This->buffer == NULL)
          return DIERR_NOTBUFFERED;

        if (dodsize < sizeof(*dod))
          return DIERR_INVALIDPARAM;

        EnterCriticalSection(&(This->crit));

        /* Copy item at a time for the case dodsize > sizeof(buffer[n]) */
        while ((i < *entries || *entries == INFINITE) && i < This->count)
          {
            if (dod != NULL)
              {
                int n = (This->start + i) % This->buffersize;
                LPDIDEVICEOBJECTDATA pd
                   = (LPDIDEVICEOBJECTDATA)((BYTE *)dod + dodsize * i);
                pd->dwOfs       = This->buffer[n].dwOfs;
                pd->dwData      = This->buffer[n].dwData;
                pd->dwTimeStamp = This->buffer[n].dwTimeStamp;
                pd->dwSequence  = This->buffer[n].dwSequence;
              }
            i++;
          }

        *entries = i;

        if (This->overflow)
          ret = DI_BUFFEROVERFLOW;

        if (!(flags & DIGDD_PEEK))
          {
            /* Empty buffer */
            This->count -= i;
            This->start = (This->start + i) % This->buffersize;
            This->overflow = FALSE;
          }

        LeaveCriticalSection(&(This->crit));

	TRACE("Returning %ld events queued\n", *entries);

        return ret;
}

static HRESULT WINAPI SysKeyboardAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface);

static HRESULT WINAPI SysKeyboardAImpl_Acquire(LPDIRECTINPUTDEVICE8A iface)
{
	ICOM_THIS(SysKeyboardAImpl,iface);

	TRACE("(this=%p)\n",This);

        if (This->acquired)
          return S_FALSE;

        This->acquired = 1;

        if (current != NULL)
          {
            FIXME("Not more than one keyboard can be acquired at the same time.\n");
            SysKeyboardAImpl_Unacquire(iface);
          }

        current = This;

        if (This->buffersize > 0)
          {
            This->buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                     This->buffersize * sizeof(*(This->buffer)));
            This->start = 0;
            This->count = 0;
            This->overflow = FALSE;
            InitializeCriticalSection(&(This->crit));
          }
        else
          This->buffer = NULL;

	return DI_OK;
}

static HRESULT WINAPI SysKeyboardAImpl_Unacquire(LPDIRECTINPUTDEVICE8A iface)
{
	ICOM_THIS(SysKeyboardAImpl,iface);
	TRACE("(this=%p)\n",This);

        if (This->acquired == 0)
          return DI_NOEFFECT;

        if (current == This)
          current = NULL;
        else
          ERR("this != current\n");

        This->acquired = 0;

        if (This->buffersize >= 0)
          {
            HeapFree(GetProcessHeap(), 0, This->buffer);
            This->buffer = NULL;
            DeleteCriticalSection(&(This->crit));
          }

	return DI_OK;
}

static HRESULT WINAPI SysKeyboardAImpl_SetEventNotification(LPDIRECTINPUTDEVICE8A iface,
							    HANDLE hnd) {
  ICOM_THIS(SysKeyboardAImpl,iface);

  TRACE("(this=%p,0x%08lx)\n",This,(DWORD)hnd);

  This->hEvent = hnd;
  return DI_OK;
}

/******************************************************************************
  *     GetCapabilities : get the device capablitites
  */
static HRESULT WINAPI SysKeyboardAImpl_GetCapabilities(
	LPDIRECTINPUTDEVICE8A iface,
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

static ICOM_VTABLE(IDirectInputDevice8A) SysKeyboardAvt =
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
	SysKeyboardAImpl_SetEventNotification,
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
        IDirectInputDevice7AImpl_EnumEffectsInFile,
        IDirectInputDevice7AImpl_WriteEffectToFile,
        IDirectInputDevice8AImpl_BuildActionMap,
        IDirectInputDevice8AImpl_SetActionMap,
        IDirectInputDevice8AImpl_GetImageInfo
};
