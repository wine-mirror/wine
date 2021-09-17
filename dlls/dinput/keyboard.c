/*		DirectInput Keyboard device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2005 Raphael Junqueira
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "dinput.h"

#include "dinput_private.h"
#include "device_private.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

#define WINE_DINPUT_KEYBOARD_MAX_KEYS 256

static const IDirectInputDevice8WVtbl SysKeyboardWvt;

typedef struct SysKeyboardImpl SysKeyboardImpl;
struct SysKeyboardImpl
{
    struct IDirectInputDeviceImpl base;
    BYTE DInputKeyState[WINE_DINPUT_KEYBOARD_MAX_KEYS];
    DWORD subtype;
};

static inline SysKeyboardImpl *impl_from_IDirectInputDevice8W(IDirectInputDevice8W *iface)
{
    return CONTAINING_RECORD(CONTAINING_RECORD(iface, IDirectInputDeviceImpl, IDirectInputDevice8W_iface), SysKeyboardImpl, base);
}

static BYTE map_dik_code(DWORD scanCode, DWORD vkCode, DWORD subType, DWORD version)
{
    if (!scanCode && version < 0x0800)
        scanCode = MapVirtualKeyW(vkCode, MAPVK_VK_TO_VSC);

    if (subType == DIDEVTYPEKEYBOARD_JAPAN106)
    {
        switch (scanCode)
        {
        case 0x0d: /* ^ */
            scanCode = DIK_CIRCUMFLEX;
            break;
        case 0x1a: /* @ */
            scanCode = DIK_AT;
            break;
        case 0x1b: /* [ */
            scanCode = DIK_LBRACKET;
            break;
        case 0x28: /* : */
            scanCode = DIK_COLON;
            break;
        case 0x29: /* Hankaku/Zenkaku */
            scanCode = DIK_KANJI;
            break;
        case 0x2b: /* ] */
            scanCode = DIK_RBRACKET;
            break;
        case 0x73: /* \ */
            scanCode = DIK_BACKSLASH;
            break;
        }
    }
    return scanCode;
}

int dinput_keyboard_hook( IDirectInputDevice8W *iface, WPARAM wparam, LPARAM lparam )
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W( iface );
    int dik_code, ret = This->base.dwCoopLevel & DISCL_EXCLUSIVE;
    KBDLLHOOKSTRUCT *hook = (KBDLLHOOKSTRUCT *)lparam;
    BYTE new_diks;

    if (wparam != WM_KEYDOWN && wparam != WM_KEYUP &&
        wparam != WM_SYSKEYDOWN && wparam != WM_SYSKEYUP)
        return 0;

    TRACE("(%p) wp %08lx, lp %08lx, vk %02x, scan %02x\n",
          iface, wparam, lparam, hook->vkCode, hook->scanCode);

    switch (hook->vkCode)
    {
        /* R-Shift is special - it is an extended key with separate scan code */
        case VK_RSHIFT  : dik_code = DIK_RSHIFT; break;
        case VK_PAUSE   : dik_code = DIK_PAUSE; break;
        case VK_NUMLOCK : dik_code = DIK_NUMLOCK; break;
        case VK_SUBTRACT: dik_code = DIK_SUBTRACT; break;
        default:
            dik_code = map_dik_code(hook->scanCode & 0xff, hook->vkCode, This->subtype, This->base.dinput->dwVersion);
            if (hook->flags & LLKHF_EXTENDED) dik_code |= 0x80;
    }
    new_diks = hook->flags & LLKHF_UP ? 0 : 0x80;

    /* returns now if key event already known */
    if (new_diks == This->DInputKeyState[dik_code])
        return ret;

    This->DInputKeyState[dik_code] = new_diks;
    TRACE(" setting %02X to %02X\n", dik_code, This->DInputKeyState[dik_code]);

    EnterCriticalSection(&This->base.crit);
    queue_event(iface, DIDFT_MAKEINSTANCE(dik_code) | DIDFT_PSHBUTTON,
                new_diks, GetCurrentTime(), This->base.dinput->evsequence++);
    if (This->base.hEvent) SetEvent( This->base.hEvent );
    LeaveCriticalSection(&This->base.crit);

    return ret;
}

static DWORD get_keyboard_subtype(void)
{
    DWORD kbd_type, kbd_subtype, dev_subtype;
    kbd_type = GetKeyboardType(0);
    kbd_subtype = GetKeyboardType(1);

    if (kbd_type == 4 || (kbd_type == 7 && kbd_subtype == 0))
        dev_subtype = DIDEVTYPEKEYBOARD_PCENH;
    else if (kbd_type == 7 && kbd_subtype == 2)
        dev_subtype = DIDEVTYPEKEYBOARD_JAPAN106;
    else {
        FIXME("Unknown keyboard type=%u, subtype=%u\n", kbd_type, kbd_subtype);
        dev_subtype = DIDEVTYPEKEYBOARD_PCENH;
    }
    return dev_subtype;
}

static void fill_keyboard_dideviceinstanceW(LPDIDEVICEINSTANCEW lpddi, DWORD version, DWORD subtype) {
    DWORD dwSize;
    DIDEVICEINSTANCEW ddi;
    
    dwSize = lpddi->dwSize;

    TRACE("%d %p\n", dwSize, lpddi);
    
    memset(lpddi, 0, dwSize);
    memset(&ddi, 0, sizeof(ddi));
 
    ddi.dwSize = dwSize;
    ddi.guidInstance = GUID_SysKeyboard;/* DInput's GUID */
    ddi.guidProduct = GUID_SysKeyboard;
    if (version >= 0x0800)
        ddi.dwDevType = DI8DEVTYPE_KEYBOARD | (subtype << 8);
    else
        ddi.dwDevType = DIDEVTYPE_KEYBOARD | (subtype << 8);
    MultiByteToWideChar(CP_ACP, 0, "Keyboard", -1, ddi.tszInstanceName, MAX_PATH);
    MultiByteToWideChar(CP_ACP, 0, "Wine Keyboard", -1, ddi.tszProductName, MAX_PATH);

    memcpy(lpddi, &ddi, (dwSize < sizeof(ddi) ? dwSize : sizeof(ddi)));
}
 
static HRESULT keyboarddev_enum_device(DWORD dwDevType, DWORD dwFlags, LPDIDEVICEINSTANCEW lpddi, DWORD version, int id)
{
  if (id != 0)
    return E_FAIL;

  if (dwFlags & DIEDFL_FORCEFEEDBACK)
    return S_FALSE;

  if ((dwDevType == 0) ||
      ((dwDevType == DIDEVTYPE_KEYBOARD) && (version < 0x0800)) ||
      (((dwDevType == DI8DEVCLASS_KEYBOARD) || (dwDevType == DI8DEVTYPE_KEYBOARD)) && (version >= 0x0800))) {
    TRACE("Enumerating the Keyboard device\n");

    fill_keyboard_dideviceinstanceW(lpddi, version, get_keyboard_subtype());
    
    return S_OK;
  }

  return S_FALSE;
}

static HRESULT alloc_device( REFGUID rguid, IDirectInputImpl *dinput, SysKeyboardImpl **out )
{
    SysKeyboardImpl* newDevice;
    LPDIDATAFORMAT df = NULL;
    int i, idx = 0;
    HRESULT hr;

    if (FAILED(hr = direct_input_device_alloc( sizeof(SysKeyboardImpl), &SysKeyboardWvt, rguid, dinput, (void **)&newDevice )))
        return hr;
    df = newDevice->base.data_format.wine_df;
    newDevice->base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": SysKeyboardImpl*->base.crit");

    newDevice->subtype = get_keyboard_subtype();

    /* Create copy of default data format */
    memcpy(df, &c_dfDIKeyboard, c_dfDIKeyboard.dwSize);
    if (!(df->rgodf = HeapAlloc(GetProcessHeap(), 0, df->dwNumObjs * df->dwObjSize))) goto failed;

    for (i = 0; i < df->dwNumObjs; i++)
    {
        char buf[MAX_PATH];
        BYTE dik_code;

        if (!GetKeyNameTextA(((i & 0x7f) << 16) | ((i & 0x80) << 17), buf, sizeof(buf)))
            continue;

        dik_code = map_dik_code(i, 0, newDevice->subtype, dinput->dwVersion);
        memcpy(&df->rgodf[idx], &c_dfDIKeyboard.rgodf[dik_code], df->dwObjSize);
        df->rgodf[idx++].dwType = DIDFT_MAKEINSTANCE(dik_code) | DIDFT_PSHBUTTON;
    }
    df->dwNumObjs = idx;

    *out = newDevice;
    return DI_OK;

failed:
    if (df) HeapFree(GetProcessHeap(), 0, df->rgodf);
    HeapFree(GetProcessHeap(), 0, df);
    HeapFree(GetProcessHeap(), 0, newDevice);
    return DIERR_OUTOFMEMORY;
}

static HRESULT keyboarddev_create_device( IDirectInputImpl *dinput, REFGUID rguid, IDirectInputDevice8W **out )
{
    TRACE( "%p %s %p\n", dinput, debugstr_guid( rguid ), out );
    *out = NULL;

    if (IsEqualGUID(&GUID_SysKeyboard, rguid)) /* Wine Keyboard */
    {
        SysKeyboardImpl *This;
        HRESULT hr;

        if (FAILED(hr = alloc_device( rguid, dinput, &This ))) return hr;

        TRACE( "Created a Keyboard device (%p)\n", This );

        *out = &This->base.IDirectInputDevice8W_iface;
        return DI_OK;
    }

    return DIERR_DEVICENOTREG;
}

const struct dinput_device keyboard_device = {
  "Wine keyboard driver",
  keyboarddev_enum_device,
  keyboarddev_create_device
};

static HRESULT WINAPI SysKeyboardWImpl_GetDeviceState(LPDIRECTINPUTDEVICE8W iface, DWORD len, LPVOID ptr)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W(iface);
    TRACE("(%p)->(%d,%p)\n", This, len, ptr);

    if (!This->base.acquired) return DIERR_NOTACQUIRED;

    if (len != This->base.data_format.user_df->dwDataSize )
        return DIERR_INVALIDPARAM;

    check_dinput_events();

    EnterCriticalSection(&This->base.crit);

    if (TRACE_ON(dinput)) {
	int i;
	for (i = 0; i < WINE_DINPUT_KEYBOARD_MAX_KEYS; i++) {
	    if (This->DInputKeyState[i] != 0x00)
		TRACE(" - %02X: %02x\n", i, This->DInputKeyState[i]);
	}
    }

    fill_DataFormat(ptr, len, This->DInputKeyState, &This->base.data_format);
    LeaveCriticalSection(&This->base.crit);

    return DI_OK;
}

/******************************************************************************
  *     GetCapabilities : get the device capabilities
  */
static HRESULT WINAPI SysKeyboardWImpl_GetCapabilities(LPDIRECTINPUTDEVICE8W iface, LPDIDEVCAPS lpDIDevCaps)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W(iface);
    DIDEVCAPS devcaps;

    TRACE("(this=%p,%p)\n",This,lpDIDevCaps);

    if ((lpDIDevCaps->dwSize != sizeof(DIDEVCAPS)) && (lpDIDevCaps->dwSize != sizeof(DIDEVCAPS_DX3))) {
        WARN("invalid parameter\n");
        return DIERR_INVALIDPARAM;
    }

    devcaps.dwSize = lpDIDevCaps->dwSize;
    devcaps.dwFlags = DIDC_ATTACHED | DIDC_EMULATED;
    if (This->base.dinput->dwVersion >= 0x0800)
        devcaps.dwDevType = DI8DEVTYPE_KEYBOARD | (This->subtype << 8);
    else
        devcaps.dwDevType = DIDEVTYPE_KEYBOARD | (This->subtype << 8);
    devcaps.dwAxes = 0;
    devcaps.dwButtons = This->base.data_format.wine_df->dwNumObjs;
    devcaps.dwPOVs = 0;
    devcaps.dwFFSamplePeriod = 0;
    devcaps.dwFFMinTimeResolution = 0;
    devcaps.dwFirmwareRevision = 100;
    devcaps.dwHardwareRevision = 100;
    devcaps.dwFFDriverVersion = 0;

    memcpy(lpDIDevCaps, &devcaps, lpDIDevCaps->dwSize);

    return DI_OK;
}

static DWORD map_dik_to_scan(DWORD dik_code, DWORD subtype)
{
    if (dik_code == DIK_PAUSE || dik_code == DIK_NUMLOCK) dik_code ^= 0x80;
    if (subtype == DIDEVTYPEKEYBOARD_JAPAN106)
    {
        switch (dik_code)
        {
        case DIK_CIRCUMFLEX:
            dik_code = 0x0d;
            break;
        case DIK_AT:
            dik_code = 0x1a;
            break;
        case DIK_LBRACKET:
            dik_code = 0x1b;
            break;
        case DIK_COLON:
            dik_code = 0x28;
            break;
        case DIK_KANJI:
            dik_code = 0x29;
            break;
        case DIK_RBRACKET:
            dik_code = 0x2b;
            break;
        case DIK_BACKSLASH:
            dik_code = 0x73;
            break;
        }
    }

    return dik_code;
}

/******************************************************************************
  *     GetObjectInfo : get information about a device object such as a button
  *                     or axis
  */
static HRESULT WINAPI SysKeyboardWImpl_GetObjectInfo(LPDIRECTINPUTDEVICE8W iface,
						     LPDIDEVICEOBJECTINSTANCEW pdidoi,
						     DWORD dwObj,
						     DWORD dwHow)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT res;
    LONG scan;

    res = IDirectInputDevice2WImpl_GetObjectInfo(iface, pdidoi, dwObj, dwHow);
    if (res != DI_OK) return res;

    scan = map_dik_to_scan(DIDFT_GETINSTANCE(pdidoi->dwType), This->subtype);
    if (!GetKeyNameTextW((scan & 0x80) << 17 | (scan & 0x7f) << 16,
                         pdidoi->tszName, ARRAY_SIZE(pdidoi->tszName)))
        return DIERR_OBJECTNOTFOUND;

    _dump_OBJECTINSTANCEW(pdidoi);
    return res;
}

/******************************************************************************
  *     GetDeviceInfo : get information about a device's identity
  */
static HRESULT WINAPI SysKeyboardWImpl_GetDeviceInfo(LPDIRECTINPUTDEVICE8W iface, LPDIDEVICEINSTANCEW pdidi)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W(iface);
    TRACE("(this=%p,%p)\n", This, pdidi);

    if (pdidi->dwSize != sizeof(DIDEVICEINSTANCEW)) {
        WARN(" dinput3 not supported yet...\n");
	return DI_OK;
    }

    fill_keyboard_dideviceinstanceW(pdidi, This->base.dinput->dwVersion, This->subtype);
    
    return DI_OK;
}

/******************************************************************************
 *      GetProperty : Retrieves information about the input device.
 */
static HRESULT WINAPI SysKeyboardWImpl_GetProperty(LPDIRECTINPUTDEVICE8W iface,
                                                   REFGUID rguid, LPDIPROPHEADER pdiph)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W(iface);

    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(rguid), pdiph);
    _dump_DIPROPHEADER(pdiph);

    if (!IS_DIPROP(rguid)) return DI_OK;

    switch (LOWORD(rguid))
    {
        case (DWORD_PTR)DIPROP_KEYNAME:
        {
            HRESULT hr;
            LPDIPROPSTRING ps = (LPDIPROPSTRING)pdiph;
            DIDEVICEOBJECTINSTANCEW didoi;

            if (pdiph->dwSize != sizeof(DIPROPSTRING))
                return DIERR_INVALIDPARAM;

            didoi.dwSize = sizeof(DIDEVICEOBJECTINSTANCEW);

            hr = SysKeyboardWImpl_GetObjectInfo(iface, &didoi, ps->diph.dwObj, ps->diph.dwHow);
            if (hr == DI_OK)
                memcpy(ps->wsz, didoi.tszName, sizeof(ps->wsz));
            return hr;
        }
        case (DWORD_PTR) DIPROP_VIDPID:
        case (DWORD_PTR) DIPROP_RANGE:
            return DIERR_UNSUPPORTED;
        default:
            return IDirectInputDevice2WImpl_GetProperty( iface, rguid, pdiph );
    }
    return DI_OK;
}

static HRESULT WINAPI SysKeyboardWImpl_Acquire(LPDIRECTINPUTDEVICE8W iface)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W(iface);
    HRESULT res;

    TRACE("(%p)\n", This);

    res = IDirectInputDevice2WImpl_Acquire(iface);
    if (res == DI_OK)
    {
        TRACE("clearing keystate\n");
        memset(This->DInputKeyState, 0, sizeof(This->DInputKeyState));
    }

    return res;
}

static HRESULT WINAPI SysKeyboardWImpl_BuildActionMap(LPDIRECTINPUTDEVICE8W iface,
                                                      LPDIACTIONFORMATW lpdiaf,
                                                      LPCWSTR lpszUserName,
                                                      DWORD dwFlags)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W(iface);
    FIXME("(%p)->(%p,%s,%08x): semi-stub !\n", This, lpdiaf, debugstr_w(lpszUserName), dwFlags);

    return  _build_action_map(iface, lpdiaf, lpszUserName, dwFlags, DIKEYBOARD_MASK, &c_dfDIKeyboard);
}

static HRESULT WINAPI SysKeyboardWImpl_SetActionMap(LPDIRECTINPUTDEVICE8W iface,
                                                    LPDIACTIONFORMATW lpdiaf,
                                                    LPCWSTR lpszUserName,
                                                    DWORD dwFlags)
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W(iface);
    FIXME("(%p)->(%p,%s,%08x): semi-stub !\n", This, lpdiaf, debugstr_w(lpszUserName), dwFlags);

    return _set_action_map(iface, lpdiaf, lpszUserName, dwFlags, &c_dfDIKeyboard);
}

static const IDirectInputDevice8WVtbl SysKeyboardWvt =
{
    IDirectInputDevice2WImpl_QueryInterface,
    IDirectInputDevice2WImpl_AddRef,
    IDirectInputDevice2WImpl_Release,
    SysKeyboardWImpl_GetCapabilities,
    IDirectInputDevice2WImpl_EnumObjects,
    SysKeyboardWImpl_GetProperty,
    IDirectInputDevice2WImpl_SetProperty,
    SysKeyboardWImpl_Acquire,
    IDirectInputDevice2WImpl_Unacquire,
    SysKeyboardWImpl_GetDeviceState,
    IDirectInputDevice2WImpl_GetDeviceData,
    IDirectInputDevice2WImpl_SetDataFormat,
    IDirectInputDevice2WImpl_SetEventNotification,
    IDirectInputDevice2WImpl_SetCooperativeLevel,
    SysKeyboardWImpl_GetObjectInfo,
    SysKeyboardWImpl_GetDeviceInfo,
    IDirectInputDevice2WImpl_RunControlPanel,
    IDirectInputDevice2WImpl_Initialize,
    IDirectInputDevice2WImpl_CreateEffect,
    IDirectInputDevice2WImpl_EnumEffects,
    IDirectInputDevice2WImpl_GetEffectInfo,
    IDirectInputDevice2WImpl_GetForceFeedbackState,
    IDirectInputDevice2WImpl_SendForceFeedbackCommand,
    IDirectInputDevice2WImpl_EnumCreatedEffectObjects,
    IDirectInputDevice2WImpl_Escape,
    IDirectInputDevice2WImpl_Poll,
    IDirectInputDevice2WImpl_SendDeviceData,
    IDirectInputDevice7WImpl_EnumEffectsInFile,
    IDirectInputDevice7WImpl_WriteEffectToFile,
    SysKeyboardWImpl_BuildActionMap,
    SysKeyboardWImpl_SetActionMap,
    IDirectInputDevice8WImpl_GetImageInfo
};
