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

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

static const IDirectInputDevice8WVtbl SysKeyboardWvt;
static const struct dinput_device_vtbl keyboard_internal_vtbl;

typedef struct SysKeyboardImpl SysKeyboardImpl;
struct SysKeyboardImpl
{
    struct IDirectInputDeviceImpl base;
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
    if (scanCode & 0x100) scanCode |= 0x80;
    return (BYTE)scanCode;
}

int dinput_keyboard_hook( IDirectInputDevice8W *iface, WPARAM wparam, LPARAM lparam )
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W( iface );
    BYTE new_diks, subtype = GET_DIDEVICE_SUBTYPE( This->base.instance.dwDevType );
    int dik_code, ret = This->base.dwCoopLevel & DISCL_EXCLUSIVE;
    KBDLLHOOKSTRUCT *hook = (KBDLLHOOKSTRUCT *)lparam;
    DWORD scan_code;

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
            scan_code = hook->scanCode & 0xff;
            if (hook->flags & LLKHF_EXTENDED) scan_code |= 0x100;
            dik_code = map_dik_code( scan_code, hook->vkCode, subtype, This->base.dinput->dwVersion );
    }
    new_diks = hook->flags & LLKHF_UP ? 0 : 0x80;

    /* returns now if key event already known */
    if (new_diks == This->base.device_state[dik_code]) return ret;

    This->base.device_state[dik_code] = new_diks;
    TRACE( " setting key %02x to %02x\n", dik_code, This->base.device_state[dik_code] );

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
    BYTE subtype = get_keyboard_subtype();
    SysKeyboardImpl* newDevice;
    HRESULT hr;

    if (FAILED(hr = direct_input_device_alloc( sizeof(SysKeyboardImpl), &SysKeyboardWvt, &keyboard_internal_vtbl,
                                               rguid, dinput, (void **)&newDevice )))
        return hr;
    newDevice->base.crit.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": SysKeyboardImpl*->base.crit");

    fill_keyboard_dideviceinstanceW( &newDevice->base.instance, newDevice->base.dinput->dwVersion, subtype );
    newDevice->base.caps.dwDevType = newDevice->base.instance.dwDevType;
    newDevice->base.caps.dwFirmwareRevision = 100;
    newDevice->base.caps.dwHardwareRevision = 100;

    if (FAILED(hr = direct_input_device_init( &newDevice->base.IDirectInputDevice8W_iface )))
    {
        IDirectInputDevice_Release( &newDevice->base.IDirectInputDevice8W_iface );
        return hr;
    }

    *out = newDevice;
    return DI_OK;
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

static HRESULT keyboard_internal_poll( IDirectInputDevice8W *iface )
{
    check_dinput_events();
    return DI_OK;
}

static HRESULT keyboard_internal_acquire( IDirectInputDevice8W *iface )
{
    return DI_OK;
}

static HRESULT keyboard_internal_unacquire( IDirectInputDevice8W *iface )
{
    SysKeyboardImpl *This = impl_from_IDirectInputDevice8W( iface );
    memset( This->base.device_state, 0, sizeof(This->base.device_state) );
    return DI_OK;
}

static BOOL try_enum_object( const DIPROPHEADER *filter, DWORD flags, LPDIENUMDEVICEOBJECTSCALLBACKW callback,
                             DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    if (flags != DIDFT_ALL && !(flags & DIDFT_GETTYPE( instance->dwType ))) return DIENUM_CONTINUE;

    switch (filter->dwHow)
    {
    case DIPH_DEVICE:
        return callback( instance, data );
    case DIPH_BYOFFSET:
        if (filter->dwObj != instance->dwOfs) return DIENUM_CONTINUE;
        return callback( instance, data );
    case DIPH_BYID:
        if ((filter->dwObj & 0x00ffffff) != (instance->dwType & 0x00ffffff)) return DIENUM_CONTINUE;
        return callback( instance, data );
    }

    return DIENUM_CONTINUE;
}

static HRESULT keyboard_internal_enum_objects( IDirectInputDevice8W *iface, const DIPROPHEADER *filter, DWORD flags,
                                               LPDIENUMDEVICEOBJECTSCALLBACKW callback, void *context )
{
    SysKeyboardImpl *impl = impl_from_IDirectInputDevice8W( iface );
    BYTE subtype = GET_DIDEVICE_SUBTYPE( impl->base.instance.dwDevType );
    DIDEVICEOBJECTINSTANCEW instance =
    {
        .dwSize = sizeof(DIDEVICEOBJECTINSTANCEW),
        .guidType = GUID_Key,
        .dwOfs = DIK_ESCAPE,
        .dwType = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( DIK_ESCAPE ),
    };
    DWORD i, dik;
    BOOL ret;

    for (i = 0; i < 512; ++i)
    {
        if (!GetKeyNameTextW( i << 16, instance.tszName, ARRAY_SIZE(instance.tszName) )) continue;
        if (!(dik = map_dik_code( i, 0, subtype, impl->base.dinput->dwVersion ))) continue;
        instance.dwOfs = dik;
        instance.dwType = DIDFT_PSHBUTTON | DIDFT_MAKEINSTANCE( dik );
        ret = try_enum_object( filter, flags, callback, &instance, context );
        if (ret != DIENUM_CONTINUE) return DIENUM_STOP;
    }

    return DIENUM_CONTINUE;
}

static HRESULT keyboard_internal_get_property( IDirectInputDevice8W *iface, DWORD property, DIPROPHEADER *header,
                                               DIDEVICEOBJECTINSTANCEW *instance )
{
    switch (property)
    {
    case (DWORD_PTR)DIPROP_KEYNAME:
    {
        DIPROPSTRING *value = (DIPROPSTRING *)header;
        lstrcpynW( value->wsz, instance->tszName, ARRAY_SIZE(value->wsz) );
        return DI_OK;
    }
    }
    return DIERR_UNSUPPORTED;
}

static HRESULT keyboard_internal_set_property( IDirectInputDevice8W *iface, DWORD property, const DIPROPHEADER *header,
                                               const DIDEVICEOBJECTINSTANCEW *instance )
{
    return DIERR_UNSUPPORTED;
}

static const struct dinput_device_vtbl keyboard_internal_vtbl =
{
    NULL,
    keyboard_internal_poll,
    NULL,
    keyboard_internal_acquire,
    keyboard_internal_unacquire,
    keyboard_internal_enum_objects,
    keyboard_internal_get_property,
    keyboard_internal_set_property,
    NULL,
    NULL,
    NULL,
    NULL,
};

static const IDirectInputDevice8WVtbl SysKeyboardWvt =
{
    IDirectInputDevice2WImpl_QueryInterface,
    IDirectInputDevice2WImpl_AddRef,
    IDirectInputDevice2WImpl_Release,
    IDirectInputDevice2WImpl_GetCapabilities,
    IDirectInputDevice2WImpl_EnumObjects,
    IDirectInputDevice2WImpl_GetProperty,
    IDirectInputDevice2WImpl_SetProperty,
    IDirectInputDevice2WImpl_Acquire,
    IDirectInputDevice2WImpl_Unacquire,
    IDirectInputDevice2WImpl_GetDeviceState,
    IDirectInputDevice2WImpl_GetDeviceData,
    IDirectInputDevice2WImpl_SetDataFormat,
    IDirectInputDevice2WImpl_SetEventNotification,
    IDirectInputDevice2WImpl_SetCooperativeLevel,
    IDirectInputDevice2WImpl_GetObjectInfo,
    IDirectInputDevice2WImpl_GetDeviceInfo,
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
    IDirectInputDevice8WImpl_BuildActionMap,
    IDirectInputDevice8WImpl_SetActionMap,
    IDirectInputDevice8WImpl_GetImageInfo
};
