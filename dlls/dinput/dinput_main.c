/*		DirectInput
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
 * Copyright 2000-2002 TransGaming Technologies Inc.
 * Copyright 2007 Vitaliy Margolen
 *
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
/* Status:
 *
 * - Tomb Raider 2 Demo:
 *   Playable using keyboard only.
 * - WingCommander Prophecy Demo:
 *   Doesn't get Input Focus.
 *
 * - Fallout : works great in X and DGA mode
 */

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "initguid.h"
#include "devguid.h"
#include "dinputd.h"

#include "dinput_private.h"
#include "device_private.h"

#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

static const IDirectInput7WVtbl ddi7wvt;
static const IDirectInput8WVtbl ddi8wvt;
static const IDirectInputJoyConfig8Vtbl JoyConfig8vt;

static inline IDirectInputImpl *impl_from_IDirectInput7W( IDirectInput7W *iface )
{
    return CONTAINING_RECORD( iface, IDirectInputImpl, IDirectInput7W_iface );
}

static inline IDirectInputImpl *impl_from_IDirectInput8W( IDirectInput8W *iface )
{
    return CONTAINING_RECORD( iface, IDirectInputImpl, IDirectInput8W_iface );
}

static inline struct dinput_device *impl_from_IDirectInputDevice8W( IDirectInputDevice8W *iface )
{
    return CONTAINING_RECORD( iface, struct dinput_device, IDirectInputDevice8W_iface );
}

HINSTANCE DINPUT_instance;

static HWND di_em_win;

static HANDLE dinput_thread;
static DWORD dinput_thread_id;

static CRITICAL_SECTION dinput_hook_crit;
static CRITICAL_SECTION_DEBUG dinput_critsect_debug =
{
    0, 0, &dinput_hook_crit,
    { &dinput_critsect_debug.ProcessLocksList, &dinput_critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dinput_hook_crit") }
};
static CRITICAL_SECTION dinput_hook_crit = { &dinput_critsect_debug, -1, 0, 0, 0, 0 };

static struct list acquired_mouse_list = LIST_INIT( acquired_mouse_list );
static struct list acquired_rawmouse_list = LIST_INIT( acquired_rawmouse_list );
static struct list acquired_keyboard_list = LIST_INIT( acquired_keyboard_list );
static struct list acquired_device_list = LIST_INIT( acquired_device_list );

static HRESULT initialize_directinput_instance(IDirectInputImpl *This, DWORD dwVersion);
static void uninitialize_directinput_instance(IDirectInputImpl *This);

void dinput_hooks_acquire_device( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );

    EnterCriticalSection( &dinput_hook_crit );
    if (IsEqualGUID( &impl->guid, &GUID_SysMouse ))
        list_add_tail( impl->use_raw_input ? &acquired_rawmouse_list : &acquired_mouse_list, &impl->entry );
    else if (IsEqualGUID( &impl->guid, &GUID_SysKeyboard ))
        list_add_tail( &acquired_keyboard_list, &impl->entry );
    else
        list_add_tail( &acquired_device_list, &impl->entry );
    LeaveCriticalSection( &dinput_hook_crit );
}

void dinput_hooks_unacquire_device( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );

    EnterCriticalSection( &dinput_hook_crit );
    list_remove( &impl->entry );
    LeaveCriticalSection( &dinput_hook_crit );
}

static void dinput_device_internal_unacquire( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->crit );
    if (impl->status == STATUS_ACQUIRED)
    {
        impl->vtbl->unacquire( iface );
        impl->status = STATUS_UNACQUIRED;
        list_remove( &impl->entry );
    }
    LeaveCriticalSection( &impl->crit );
}

static HRESULT create_directinput_instance(REFIID riid, LPVOID *ppDI, IDirectInputImpl **out)
{
    IDirectInputImpl *This = calloc( 1, sizeof(IDirectInputImpl) );
    HRESULT hr;

    if (!This)
        return E_OUTOFMEMORY;

    This->IDirectInput7A_iface.lpVtbl = &dinput7_a_vtbl;
    This->IDirectInput7W_iface.lpVtbl = &ddi7wvt;
    This->IDirectInput8A_iface.lpVtbl = &dinput8_a_vtbl;
    This->IDirectInput8W_iface.lpVtbl = &ddi8wvt;
    This->IDirectInputJoyConfig8_iface.lpVtbl = &JoyConfig8vt;

    hr = IDirectInput_QueryInterface( &This->IDirectInput7A_iface, riid, ppDI );
    if (FAILED(hr))
    {
        free( This );
        return hr;
    }

    if (out) *out = This;
    return DI_OK;
}

/******************************************************************************
 *	DirectInputCreateEx (DINPUT.@)
 */
HRESULT WINAPI DirectInputCreateEx(
	HINSTANCE hinst, DWORD dwVersion, REFIID riid, LPVOID *ppDI,
	LPUNKNOWN punkOuter)
{
    IDirectInputImpl *This;
    HRESULT hr;

    TRACE("(%p,%04x,%s,%p,%p)\n", hinst, dwVersion, debugstr_guid(riid), ppDI, punkOuter);

    if (IsEqualGUID( &IID_IDirectInputA,  riid ) ||
        IsEqualGUID( &IID_IDirectInput2A, riid ) ||
        IsEqualGUID( &IID_IDirectInput7A, riid ) ||
        IsEqualGUID( &IID_IDirectInputW,  riid ) ||
        IsEqualGUID( &IID_IDirectInput2W, riid ) ||
        IsEqualGUID( &IID_IDirectInput7W, riid ))
    {
        hr = create_directinput_instance(riid, ppDI, &This);
        if (FAILED(hr))
            return hr;
    }
    else
        return DIERR_NOINTERFACE;

    hr = IDirectInput_Initialize( &This->IDirectInput7A_iface, hinst, dwVersion );
    if (FAILED(hr))
    {
        IDirectInput_Release( &This->IDirectInput7A_iface );
        *ppDI = NULL;
        return hr;
    }

    return DI_OK;
}

/******************************************************************************
 *	DirectInput8Create (DINPUT8.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH DirectInput8Create(HINSTANCE hinst,
    DWORD version, REFIID iid, void **out, IUnknown *outer)
{
    IDirectInputImpl *This;
    HRESULT hr;

    TRACE("hinst %p, version %#x, iid %s, out %p, outer %p.\n",
        hinst, version, debugstr_guid(iid), out, outer);

    if (!out)
        return E_POINTER;

    if (!IsEqualGUID(&IID_IDirectInput8A, iid) &&
        !IsEqualGUID(&IID_IDirectInput8W, iid) &&
        !IsEqualGUID(&IID_IUnknown, iid))
    {
        *out = NULL;
        return DIERR_NOINTERFACE;
    }

    hr = create_directinput_instance(iid, out, &This);

    if (FAILED(hr))
    {
        ERR("Failed to create DirectInput, hr %#x.\n", hr);
        return hr;
    }

    /* When aggregation is used, the application needs to manually call Initialize(). */
    if (!outer && IsEqualGUID(&IID_IDirectInput8A, iid))
    {
        hr = IDirectInput8_Initialize(&This->IDirectInput8A_iface, hinst, version);
        if (FAILED(hr))
        {
            IDirectInput8_Release(&This->IDirectInput8A_iface);
            *out = NULL;
            return hr;
        }
    }

    if (!outer && IsEqualGUID(&IID_IDirectInput8W, iid))
    {
        hr = IDirectInput8_Initialize(&This->IDirectInput8W_iface, hinst, version);
        if (FAILED(hr))
        {
            IDirectInput8_Release(&This->IDirectInput8W_iface);
            *out = NULL;
            return hr;
        }
    }

    return S_OK;
}

/******************************************************************************
 *	DirectInputCreateA (DINPUT.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH DirectInputCreateA(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA *ppDI, LPUNKNOWN punkOuter)
{
    return DirectInputCreateEx(hinst, dwVersion, &IID_IDirectInput7A, (LPVOID *)ppDI, punkOuter);
}

/******************************************************************************
 *	DirectInputCreateW (DINPUT.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH DirectInputCreateW(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTW *ppDI, LPUNKNOWN punkOuter)
{
    return DirectInputCreateEx(hinst, dwVersion, &IID_IDirectInput7W, (LPVOID *)ppDI, punkOuter);
}

static DWORD diactionformat_priorityW(LPDIACTIONFORMATW lpdiaf, DWORD genre)
{
    int i;
    DWORD priorityFlags = 0;

    /* If there's at least one action for the device it's priority 1 */
    for(i=0; i < lpdiaf->dwNumActions; i++)
        if ((lpdiaf->rgoAction[i].dwSemantic & genre) == genre)
            priorityFlags |= DIEDBS_MAPPEDPRI1;

    return priorityFlags;
}

#if defined __i386__ && defined _MSC_VER
__declspec(naked) BOOL enum_callback_wrapper(void *callback, const void *instance, void *ref)
{
    __asm
    {
        push ebp
        mov ebp, esp
        push [ebp+16]
        push [ebp+12]
        call [ebp+8]
        leave
        ret
    }
}
#elif defined __i386__ && defined __GNUC__
extern BOOL enum_callback_wrapper(void *callback, const void *instance, void *ref);
__ASM_GLOBAL_FUNC( enum_callback_wrapper,
    "pushl %ebp\n\t"
    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
    "movl %esp,%ebp\n\t"
    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
    "pushl 16(%ebp)\n\t"
    "pushl 12(%ebp)\n\t"
    "call *8(%ebp)\n\t"
    "leave\n\t"
    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
    __ASM_CFI(".cfi_same_value %ebp\n\t")
    "ret" )
#else
#define enum_callback_wrapper(callback, instance, ref) (callback)((instance), (ref))
#endif

/******************************************************************************
 *	IDirectInputW_EnumDevices
 */
static HRESULT WINAPI IDirectInputWImpl_EnumDevices( IDirectInput7W *iface, DWORD type, LPDIENUMDEVICESCALLBACKW callback,
                                                     void *context, DWORD flags )
{
    IDirectInputImpl *impl = impl_from_IDirectInput7W( iface );

    TRACE( "iface %p, type %#x, callback %p, context %p, flags %#x\n", iface, type, callback, context, flags );

    if (!callback) return DIERR_INVALIDPARAM;

    if (type > DIDEVTYPE_JOYSTICK) return DIERR_INVALIDPARAM;
    if (flags & ~(DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK | DIEDFL_INCLUDEALIASES | DIEDFL_INCLUDEPHANTOMS))
        return DIERR_INVALIDPARAM;

    return IDirectInput8_EnumDevices( &impl->IDirectInput8W_iface, type, callback, context, flags );
}

static ULONG WINAPI IDirectInputWImpl_AddRef( IDirectInput7W *iface )
{
    IDirectInputImpl *This = impl_from_IDirectInput7W( iface );
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE( "(%p) ref %d\n", This, ref );
    return ref;
}

static ULONG WINAPI IDirectInputWImpl_Release( IDirectInput7W *iface )
{
    IDirectInputImpl *This = impl_from_IDirectInput7W( iface );
    ULONG ref = InterlockedDecrement( &This->ref );

    TRACE( "(%p) ref %d\n", This, ref );

    if (ref == 0)
    {
        uninitialize_directinput_instance( This );
        free( This );
    }

    return ref;
}

static HRESULT WINAPI IDirectInputWImpl_QueryInterface( IDirectInput7W *iface, REFIID riid, LPVOID *ppobj )
{
    IDirectInputImpl *This = impl_from_IDirectInput7W( iface );

    TRACE( "(%p)->(%s,%p)\n", This, debugstr_guid(riid), ppobj );

    if (!riid || !ppobj)
        return E_POINTER;

    *ppobj = NULL;

#if DIRECTINPUT_VERSION == 0x0700
    if (IsEqualGUID( &IID_IDirectInputA,  riid ) ||
        IsEqualGUID( &IID_IDirectInput2A, riid ) ||
        IsEqualGUID( &IID_IDirectInput7A, riid ))
        *ppobj = &This->IDirectInput7A_iface;
    else if (IsEqualGUID( &IID_IUnknown, riid ) ||
             IsEqualGUID( &IID_IDirectInputW,  riid ) ||
             IsEqualGUID( &IID_IDirectInput2W, riid ) ||
             IsEqualGUID( &IID_IDirectInput7W, riid ))
        *ppobj = &This->IDirectInput7W_iface;

#else
    if (IsEqualGUID( &IID_IDirectInput8A, riid ))
        *ppobj = &This->IDirectInput8A_iface;

    else if (IsEqualGUID( &IID_IUnknown, riid ) ||
             IsEqualGUID( &IID_IDirectInput8W, riid ))
        *ppobj = &This->IDirectInput8W_iface;

#endif

    if (IsEqualGUID( &IID_IDirectInputJoyConfig8, riid ))
        *ppobj = &This->IDirectInputJoyConfig8_iface;

    if(*ppobj)
    {
        IUnknown_AddRef( (IUnknown*)*ppobj );
        return DI_OK;
    }

    WARN( "Unsupported interface: %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static LRESULT WINAPI di_em_win_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    struct dinput_device *impl;
    RAWINPUT ri;
    UINT size = sizeof(ri);
    int rim = GET_RAWINPUT_CODE_WPARAM( wparam );

    TRACE( "%p %d %lx %lx\n", hwnd, msg, wparam, lparam );

    if (msg == WM_INPUT && (rim == RIM_INPUT || rim == RIM_INPUTSINK))
    {
        size = GetRawInputData( (HRAWINPUT)lparam, RID_INPUT, &ri, &size, sizeof(RAWINPUTHEADER) );
        if (size == (UINT)-1 || size < sizeof(RAWINPUTHEADER))
            WARN( "Unable to read raw input data\n" );
        else if (ri.header.dwType == RIM_TYPEMOUSE)
        {
            EnterCriticalSection( &dinput_hook_crit );
            LIST_FOR_EACH_ENTRY( impl, &acquired_rawmouse_list, struct dinput_device, entry )
                dinput_mouse_rawinput_hook( &impl->IDirectInputDevice8W_iface, wparam, lparam, &ri );
            LeaveCriticalSection( &dinput_hook_crit );
        }
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static void register_di_em_win_class(void)
{
    WNDCLASSEXW class;

    memset(&class, 0, sizeof(class));
    class.cbSize = sizeof(class);
    class.lpfnWndProc = di_em_win_wndproc;
    class.hInstance = DINPUT_instance;
    class.lpszClassName = L"DIEmWin";

    if (!RegisterClassExW( &class ) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        WARN( "Unable to register message window class\n" );
}

static void unregister_di_em_win_class(void)
{
    if (!UnregisterClassW( L"DIEmWin", NULL ) && GetLastError() != ERROR_CLASS_DOES_NOT_EXIST)
        WARN( "Unable to unregister message window class\n" );
}

static HRESULT initialize_directinput_instance(IDirectInputImpl *This, DWORD dwVersion)
{
    if (!This->initialized)
    {
        This->dwVersion = dwVersion;
        This->evsequence = 1;

        list_init( &This->device_players );

        This->initialized = TRUE;
    }

    return DI_OK;
}

static void uninitialize_directinput_instance(IDirectInputImpl *This)
{
    if (This->initialized)
    {
        struct DevicePlayer *device_player, *device_player2;

        LIST_FOR_EACH_ENTRY_SAFE( device_player, device_player2,
                &This->device_players, struct DevicePlayer, entry )
            free( device_player );

        This->initialized = FALSE;
    }
}

enum directinput_versions
{
    DIRECTINPUT_VERSION_300 = 0x0300,
    DIRECTINPUT_VERSION_500 = 0x0500,
    DIRECTINPUT_VERSION_50A = 0x050A,
    DIRECTINPUT_VERSION_5B2 = 0x05B2,
    DIRECTINPUT_VERSION_602 = 0x0602,
    DIRECTINPUT_VERSION_61A = 0x061A,
    DIRECTINPUT_VERSION_700 = 0x0700,
};

static HRESULT WINAPI IDirectInputWImpl_Initialize( IDirectInput7W *iface, HINSTANCE hinst, DWORD version )
{
    IDirectInputImpl *This = impl_from_IDirectInput7W( iface );

    TRACE("(%p)->(%p, 0x%04x)\n", This, hinst, version);

    if (!hinst)
        return DIERR_INVALIDPARAM;
    else if (version == 0)
        return DIERR_NOTINITIALIZED;
    else if (version > DIRECTINPUT_VERSION_700)
        return DIERR_OLDDIRECTINPUTVERSION;
    else if (version != DIRECTINPUT_VERSION_300 && version != DIRECTINPUT_VERSION_500 &&
             version != DIRECTINPUT_VERSION_50A && version != DIRECTINPUT_VERSION_5B2 &&
             version != DIRECTINPUT_VERSION_602 && version != DIRECTINPUT_VERSION_61A &&
             version != DIRECTINPUT_VERSION_700 && version != DIRECTINPUT_VERSION)
        return DIERR_BETADIRECTINPUTVERSION;

    return initialize_directinput_instance(This, version);
}

static HRESULT WINAPI IDirectInputWImpl_GetDeviceStatus( IDirectInput7W *iface, REFGUID rguid )
{
    IDirectInputImpl *This = impl_from_IDirectInput7W( iface );
    HRESULT hr;
    IDirectInputDeviceW *device;

    TRACE( "(%p)->(%s)\n", This, debugstr_guid(rguid) );

    if (!rguid) return E_POINTER;
    if (!This->initialized)
        return DIERR_NOTINITIALIZED;

    hr = IDirectInput_CreateDevice( iface, rguid, &device, NULL );
    if (hr != DI_OK) return DI_NOTATTACHED;

    IUnknown_Release( device );

    return DI_OK;
}

static HRESULT WINAPI IDirectInputWImpl_RunControlPanel( IDirectInput7W *iface, HWND hwndOwner, DWORD dwFlags )
{
    IDirectInputImpl *This = impl_from_IDirectInput7W( iface );
    WCHAR control_exe[] = {L"control.exe"};
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi;

    TRACE( "(%p)->(%p, %08x)\n", This, hwndOwner, dwFlags );

    if (hwndOwner && !IsWindow(hwndOwner))
        return E_HANDLE;

    if (dwFlags)
        return DIERR_INVALIDPARAM;

    if (!This->initialized)
        return DIERR_NOTINITIALIZED;

    if (!CreateProcessW( NULL, control_exe, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi ))
        return HRESULT_FROM_WIN32(GetLastError());

    return DI_OK;
}

static HRESULT WINAPI IDirectInput2WImpl_FindDevice(LPDIRECTINPUT7W iface, REFGUID rguid,
						    LPCWSTR pszName, LPGUID pguidInstance)
{
    IDirectInputImpl *This = impl_from_IDirectInput7W( iface );

    FIXME( "(%p)->(%s, %s, %p): stub\n", This, debugstr_guid(rguid), debugstr_w(pszName), pguidInstance );

    return DI_OK;
}

static HRESULT WINAPI IDirectInput7WImpl_CreateDeviceEx( IDirectInput7W *iface, const GUID *guid,
                                                         const GUID *iid, void **out, IUnknown *outer )
{
    IDirectInputImpl *impl = impl_from_IDirectInput7W( iface );
    IDirectInputDevice8W *device;
    HRESULT hr;

    TRACE( "iface %p, guid %s, iid %s, out %p, outer %p\n", iface, debugstr_guid( guid ),
           debugstr_guid( iid ), out, outer );

    if (!out) return E_POINTER;
    *out = NULL;

    if (!guid) return E_POINTER;
    if (!impl->initialized) return DIERR_NOTINITIALIZED;

    if (IsEqualGUID( &GUID_SysKeyboard, guid )) hr = keyboard_create_device( impl, guid, &device );
    else if (IsEqualGUID( &GUID_SysMouse, guid )) hr = mouse_create_device( impl, guid, &device );
    else hr = hid_joystick_create_device( impl, guid, &device );

    if (FAILED(hr)) return hr;
    hr = IDirectInputDevice8_QueryInterface( device, iid, out );
    IDirectInputDevice8_Release( device );
    return hr;
}

static HRESULT WINAPI IDirectInputWImpl_CreateDevice(LPDIRECTINPUT7W iface, REFGUID rguid,
                                                     LPDIRECTINPUTDEVICEW* pdev, LPUNKNOWN punk)
{
    return IDirectInput7_CreateDeviceEx( iface, rguid, &IID_IDirectInputDeviceW, (LPVOID *)pdev, punk );
}

/*******************************************************************************
 *      DirectInput8
 */

static ULONG WINAPI IDirectInput8WImpl_AddRef(LPDIRECTINPUT8W iface)
{
    IDirectInputImpl *This = impl_from_IDirectInput8W( iface );
    return IDirectInput_AddRef( &This->IDirectInput7W_iface );
}

static HRESULT WINAPI IDirectInput8WImpl_QueryInterface(LPDIRECTINPUT8W iface, REFIID riid, LPVOID *ppobj)
{
    IDirectInputImpl *This = impl_from_IDirectInput8W( iface );
    return IDirectInput_QueryInterface( &This->IDirectInput7W_iface, riid, ppobj );
}

static ULONG WINAPI IDirectInput8WImpl_Release(LPDIRECTINPUT8W iface)
{
    IDirectInputImpl *This = impl_from_IDirectInput8W( iface );
    return IDirectInput_Release( &This->IDirectInput7W_iface );
}

static HRESULT WINAPI IDirectInput8WImpl_CreateDevice(LPDIRECTINPUT8W iface, REFGUID rguid,
                                                      LPDIRECTINPUTDEVICE8W* pdev, LPUNKNOWN punk)
{
    IDirectInputImpl *This = impl_from_IDirectInput8W( iface );
    return IDirectInput7_CreateDeviceEx( &This->IDirectInput7W_iface, rguid, &IID_IDirectInputDevice8W, (LPVOID *)pdev, punk );
}

static BOOL try_enum_device( DWORD type, LPDIENUMDEVICESCALLBACKW callback,
                             DIDEVICEINSTANCEW *instance, void *context, DWORD flags )
{
    if (type && (instance->dwDevType & 0xff) != type) return DIENUM_CONTINUE;
    if ((flags & DIEDFL_FORCEFEEDBACK) && IsEqualGUID( &instance->guidFFDriver, &GUID_NULL ))
        return DIENUM_CONTINUE;
    return enum_callback_wrapper( callback, instance, context );
}

static HRESULT WINAPI IDirectInput8WImpl_EnumDevices( IDirectInput8W *iface, DWORD type, LPDIENUMDEVICESCALLBACKW callback,
                                                      void *context, DWORD flags )
{
    DIDEVICEINSTANCEW instance = {.dwSize = sizeof(DIDEVICEINSTANCEW)};
    IDirectInputImpl *impl = impl_from_IDirectInput8W( iface );
    DWORD device_class = 0, device_type = 0;
    unsigned int i = 0;
    HRESULT hr;

    TRACE( "iface %p, type %#x, callback %p, context %p, flags %#x\n", iface, type, callback, context, flags );

    if (!callback) return DIERR_INVALIDPARAM;

    if ((type > DI8DEVCLASS_GAMECTRL && type < DI8DEVTYPE_DEVICE) || type > DI8DEVTYPE_SUPPLEMENTAL)
        return DIERR_INVALIDPARAM;
    if (flags & ~(DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK | DIEDFL_INCLUDEALIASES |
                  DIEDFL_INCLUDEPHANTOMS | DIEDFL_INCLUDEHIDDEN))
        return DIERR_INVALIDPARAM;

    if (!impl->initialized) return DIERR_NOTINITIALIZED;

    if (type <= DI8DEVCLASS_GAMECTRL) device_class = type;
    else device_type = type;

    if (device_class == DI8DEVCLASS_ALL || device_class == DI8DEVCLASS_POINTER)
    {
        hr = mouse_enum_device( type, flags, &instance, impl->dwVersion );
        if (hr == DI_OK && try_enum_device( device_type, callback, &instance, context, flags ) == DIENUM_STOP)
            return DI_OK;
    }

    if (device_class == DI8DEVCLASS_ALL || device_class == DI8DEVCLASS_KEYBOARD)
    {
        hr = keyboard_enum_device( type, flags, &instance, impl->dwVersion );
        if (hr == DI_OK && try_enum_device( device_type, callback, &instance, context, flags ) == DIENUM_STOP)
            return DI_OK;
    }

    if (device_class == DI8DEVCLASS_ALL || device_class == DI8DEVCLASS_GAMECTRL)
    {
        do
        {
            hr = hid_joystick_enum_device( type, flags, &instance, impl->dwVersion, i++ );
            if (hr == DI_OK && try_enum_device( device_type, callback, &instance, context, flags ) == DIENUM_STOP)
                return DI_OK;
        } while (SUCCEEDED(hr));
    }

    return DI_OK;
}

static HRESULT WINAPI IDirectInput8WImpl_GetDeviceStatus(LPDIRECTINPUT8W iface, REFGUID rguid)
{
    IDirectInputImpl *This = impl_from_IDirectInput8W( iface );
    return IDirectInput_GetDeviceStatus( &This->IDirectInput7W_iface, rguid );
}

static HRESULT WINAPI IDirectInput8WImpl_RunControlPanel(LPDIRECTINPUT8W iface, HWND hwndOwner, DWORD dwFlags)
{
    IDirectInputImpl *This = impl_from_IDirectInput8W( iface );
    return IDirectInput_RunControlPanel( &This->IDirectInput7W_iface, hwndOwner, dwFlags );
}

static HRESULT WINAPI IDirectInput8WImpl_Initialize( IDirectInput8W *iface, HINSTANCE hinst, DWORD version )
{
    IDirectInputImpl *This = impl_from_IDirectInput8W( iface );

    TRACE("(%p)->(%p, 0x%04x)\n", This, hinst, version);

    if (!hinst)
        return DIERR_INVALIDPARAM;
    else if (version == 0)
        return DIERR_NOTINITIALIZED;
    else if (version < DIRECTINPUT_VERSION)
        return DIERR_BETADIRECTINPUTVERSION;
    else if (version > DIRECTINPUT_VERSION)
        return DIERR_OLDDIRECTINPUTVERSION;

    return initialize_directinput_instance(This, version);
}

static HRESULT WINAPI IDirectInput8WImpl_FindDevice(LPDIRECTINPUT8W iface, REFGUID rguid, LPCWSTR pszName, LPGUID pguidInstance)
{
    IDirectInputImpl *This = impl_from_IDirectInput8W( iface );
    return IDirectInput2_FindDevice( &This->IDirectInput7W_iface, rguid, pszName, pguidInstance );
}

static BOOL should_enumerate_device(const WCHAR *username, DWORD dwFlags,
    struct list *device_players, REFGUID guid)
{
    BOOL should_enumerate = TRUE;
    struct DevicePlayer *device_player;

    /* Check if user owns this device */
    if (dwFlags & DIEDBSFL_THISUSER && username && *username)
    {
        should_enumerate = FALSE;
        LIST_FOR_EACH_ENTRY(device_player, device_players, struct DevicePlayer, entry)
        {
            if (IsEqualGUID(&device_player->instance_guid, guid))
            {
                if (*device_player->username && !wcscmp( username, device_player->username ))
                    return TRUE; /* Device username matches */
                break;
            }
        }
    }

    /* Check if this device is not owned by anyone */
    if (dwFlags & DIEDBSFL_AVAILABLEDEVICES) {
        BOOL found = FALSE;
        should_enumerate = FALSE;
        LIST_FOR_EACH_ENTRY(device_player, device_players, struct DevicePlayer, entry)
        {
            if (IsEqualGUID(&device_player->instance_guid, guid))
            {
                if (*device_player->username)
                    found = TRUE;
                break;
            }
        }
        if (!found)
            return TRUE; /* Device does not have a username */
    }

    return should_enumerate;
}

struct enum_device_by_semantics_params
{
    IDirectInput8W *iface;
    const WCHAR *username;
    DWORD flags;

    DIDEVICEINSTANCEW *instances;
    DWORD instance_count;
};

static BOOL CALLBACK enum_device_by_semantics( const DIDEVICEINSTANCEW *instance, void *context )
{
    struct enum_device_by_semantics_params *params = context;
    IDirectInputImpl *This = impl_from_IDirectInput8W( params->iface );

    if (should_enumerate_device( params->username, params->flags, &This->device_players, &instance->guidInstance ))
    {
        params->instance_count++;
        params->instances = realloc( params->instances, sizeof(DIDEVICEINSTANCEW) * params->instance_count );
        params->instances[params->instance_count - 1] = *instance;
    }

    return DIENUM_CONTINUE;
}

static HRESULT WINAPI IDirectInput8WImpl_EnumDevicesBySemantics(
      LPDIRECTINPUT8W iface, LPCWSTR ptszUserName, LPDIACTIONFORMATW lpdiActionFormat,
      LPDIENUMDEVICESBYSEMANTICSCBW lpCallback,
      LPVOID pvRef, DWORD dwFlags
)
{
    struct enum_device_by_semantics_params params = {.iface = iface, .username = ptszUserName, .flags = dwFlags};
    DWORD callbackFlags, enum_flags = DIEDFL_ATTACHEDONLY | (dwFlags & DIEDFL_FORCEFEEDBACK);
    static REFGUID guids[2] = { &GUID_SysKeyboard, &GUID_SysMouse };
    static const DWORD actionMasks[] = { DIKEYBOARD_MASK, DIMOUSE_MASK };
    IDirectInputImpl *This = impl_from_IDirectInput8W(iface);
    DIDEVICEINSTANCEW didevi;
    LPDIRECTINPUTDEVICE8W lpdid;
    unsigned int i = 0;
    HRESULT hr;
    int remain;

    FIXME("(this=%p,%s,%p,%p,%p,%04x): semi-stub\n", This, debugstr_w(ptszUserName), lpdiActionFormat,
          lpCallback, pvRef, dwFlags);

    didevi.dwSize = sizeof(didevi);

    hr = IDirectInput8_EnumDevices( &This->IDirectInput8W_iface, DI8DEVCLASS_GAMECTRL,
                                    enum_device_by_semantics, &params, enum_flags );
    if (FAILED(hr))
    {
        free( params.instances );
        return hr;
    }

    remain = params.instance_count;
    /* Add keyboard and mouse to remaining device count */
    if (!(dwFlags & DIEDBSFL_FORCEFEEDBACK))
    {
        for (i = 0; i < ARRAY_SIZE(guids); i++)
        {
            if (should_enumerate_device(ptszUserName, dwFlags, &This->device_players, guids[i]))
                remain++;
        }
    }

    for (i = 0; i < params.instance_count; i++)
    {
        callbackFlags = diactionformat_priorityW(lpdiActionFormat, lpdiActionFormat->dwGenre);
        IDirectInput_CreateDevice( iface, &params.instances[i].guidInstance, &lpdid, NULL );

        if (lpCallback( &params.instances[i], lpdid, callbackFlags, --remain, pvRef ) == DIENUM_STOP)
        {
            free( params.instances );
            IDirectInputDevice_Release(lpdid);
            return DI_OK;
        }
        IDirectInputDevice_Release(lpdid);
    }

    free( params.instances );

    if (dwFlags & DIEDBSFL_FORCEFEEDBACK) return DI_OK;

    /* Enumerate keyboard and mouse */
    for (i = 0; i < ARRAY_SIZE(guids); i++)
    {
        if (should_enumerate_device(ptszUserName, dwFlags, &This->device_players, guids[i]))
        {
            callbackFlags = diactionformat_priorityW(lpdiActionFormat, actionMasks[i]);

            IDirectInput_CreateDevice(iface, guids[i], &lpdid, NULL);
            IDirectInputDevice_GetDeviceInfo(lpdid, &didevi);

            if (lpCallback(&didevi, lpdid, callbackFlags, --remain, pvRef) == DIENUM_STOP)
            {
                IDirectInputDevice_Release(lpdid);
                return DI_OK;
            }
            IDirectInputDevice_Release(lpdid);
        }
    }

    return DI_OK;
}

static HRESULT WINAPI IDirectInput8WImpl_ConfigureDevices(
      LPDIRECTINPUT8W iface, LPDICONFIGUREDEVICESCALLBACK lpdiCallback,
      LPDICONFIGUREDEVICESPARAMSW lpdiCDParams, DWORD dwFlags, LPVOID pvRefData
)
{
    IDirectInputImpl *This = impl_from_IDirectInput8W(iface);

    FIXME("(this=%p,%p,%p,%04x,%p): stub\n", This, lpdiCallback, lpdiCDParams, dwFlags, pvRefData);

    /* Call helper function in config.c to do the real work */
    return _configure_devices(iface, lpdiCallback, lpdiCDParams, dwFlags, pvRefData);
}

/*****************************************************************************
 * IDirectInputJoyConfig8 interface
 */

static inline IDirectInputImpl *impl_from_IDirectInputJoyConfig8(IDirectInputJoyConfig8 *iface)
{
    return CONTAINING_RECORD( iface, IDirectInputImpl, IDirectInputJoyConfig8_iface );
}

static HRESULT WINAPI JoyConfig8Impl_QueryInterface(IDirectInputJoyConfig8 *iface, REFIID riid, void** ppobj)
{
    IDirectInputImpl *This = impl_from_IDirectInputJoyConfig8( iface );
    return IDirectInput_QueryInterface( &This->IDirectInput7W_iface, riid, ppobj );
}

static ULONG WINAPI JoyConfig8Impl_AddRef(IDirectInputJoyConfig8 *iface)
{
    IDirectInputImpl *This = impl_from_IDirectInputJoyConfig8( iface );
    return IDirectInput_AddRef( &This->IDirectInput7W_iface );
}

static ULONG WINAPI JoyConfig8Impl_Release(IDirectInputJoyConfig8 *iface)
{
    IDirectInputImpl *This = impl_from_IDirectInputJoyConfig8( iface );
    return IDirectInput_Release( &This->IDirectInput7W_iface );
}

static HRESULT WINAPI JoyConfig8Impl_Acquire(IDirectInputJoyConfig8 *iface)
{
    FIXME( "(%p): stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI JoyConfig8Impl_Unacquire(IDirectInputJoyConfig8 *iface)
{
    FIXME( "(%p): stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI JoyConfig8Impl_SetCooperativeLevel(IDirectInputJoyConfig8 *iface, HWND hwnd, DWORD flags)
{
    FIXME( "(%p)->(%p, 0x%08x): stub!\n", iface, hwnd, flags );
    return E_NOTIMPL;
}

static HRESULT WINAPI JoyConfig8Impl_SendNotify(IDirectInputJoyConfig8 *iface)
{
    FIXME( "(%p): stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI JoyConfig8Impl_EnumTypes(IDirectInputJoyConfig8 *iface, LPDIJOYTYPECALLBACK cb, void *ref)
{
    FIXME( "(%p)->(%p, %p): stub!\n", iface, cb, ref );
    return E_NOTIMPL;
}

static HRESULT WINAPI JoyConfig8Impl_GetTypeInfo(IDirectInputJoyConfig8 *iface, LPCWSTR name, LPDIJOYTYPEINFO info, DWORD flags)
{
    FIXME( "(%p)->(%s, %p, 0x%08x): stub!\n", iface, debugstr_w(name), info, flags );
    return E_NOTIMPL;
}

static HRESULT WINAPI JoyConfig8Impl_SetTypeInfo(IDirectInputJoyConfig8 *iface, LPCWSTR name, LPCDIJOYTYPEINFO info, DWORD flags,
                                                 LPWSTR new_name)
{
    FIXME( "(%p)->(%s, %p, 0x%08x, %s): stub!\n", iface, debugstr_w(name), info, flags, debugstr_w(new_name) );
    return E_NOTIMPL;
}

static HRESULT WINAPI JoyConfig8Impl_DeleteType(IDirectInputJoyConfig8 *iface, LPCWSTR name)
{
    FIXME( "(%p)->(%s): stub!\n", iface, debugstr_w(name) );
    return E_NOTIMPL;
}

struct find_device_from_index_params
{
    UINT index;
    DIDEVICEINSTANCEW instance;
};

static BOOL CALLBACK find_device_from_index( const DIDEVICEINSTANCEW *instance, void *context )
{
    struct find_device_from_index_params *params = context;
    params->instance = *instance;
    if (!params->index--) return DIENUM_STOP;
    return DIENUM_CONTINUE;
}

static HRESULT WINAPI JoyConfig8Impl_GetConfig(IDirectInputJoyConfig8 *iface, UINT id, LPDIJOYCONFIG info, DWORD flags)
{
    IDirectInputImpl *di = impl_from_IDirectInputJoyConfig8(iface);
    struct find_device_from_index_params params = {.index = id};
    HRESULT hr;

    FIXME("(%p)->(%d, %p, 0x%08x): semi-stub!\n", iface, id, info, flags);

#define X(x) if (flags & x) FIXME("\tflags |= "#x"\n");
    X(DIJC_GUIDINSTANCE)
    X(DIJC_REGHWCONFIGTYPE)
    X(DIJC_GAIN)
    X(DIJC_CALLOUT)
#undef X

    hr = IDirectInput8_EnumDevices( &di->IDirectInput8W_iface, DI8DEVCLASS_GAMECTRL, find_device_from_index, &params, 0 );
    if (FAILED(hr)) return hr;
    if (params.index != ~0) return DIERR_NOMOREITEMS;
    if (flags & DIJC_GUIDINSTANCE) info->guidInstance = params.instance.guidInstance;
    return DI_OK;
}

static HRESULT WINAPI JoyConfig8Impl_SetConfig(IDirectInputJoyConfig8 *iface, UINT id, LPCDIJOYCONFIG info, DWORD flags)
{
    FIXME( "(%p)->(%d, %p, 0x%08x): stub!\n", iface, id, info, flags );
    return E_NOTIMPL;
}

static HRESULT WINAPI JoyConfig8Impl_DeleteConfig(IDirectInputJoyConfig8 *iface, UINT id)
{
    FIXME( "(%p)->(%d): stub!\n", iface, id );
    return E_NOTIMPL;
}

static HRESULT WINAPI JoyConfig8Impl_GetUserValues(IDirectInputJoyConfig8 *iface, LPDIJOYUSERVALUES info, DWORD flags)
{
    FIXME( "(%p)->(%p, 0x%08x): stub!\n", iface, info, flags );
    return E_NOTIMPL;
}

static HRESULT WINAPI JoyConfig8Impl_SetUserValues(IDirectInputJoyConfig8 *iface, LPCDIJOYUSERVALUES info, DWORD flags)
{
    FIXME( "(%p)->(%p, 0x%08x): stub!\n", iface, info, flags );
    return E_NOTIMPL;
}

static HRESULT WINAPI JoyConfig8Impl_AddNewHardware(IDirectInputJoyConfig8 *iface, HWND hwnd, REFGUID guid)
{
    FIXME( "(%p)->(%p, %s): stub!\n", iface, hwnd, debugstr_guid(guid) );
    return E_NOTIMPL;
}

static HRESULT WINAPI JoyConfig8Impl_OpenTypeKey(IDirectInputJoyConfig8 *iface, LPCWSTR name, DWORD security, PHKEY key)
{
    FIXME( "(%p)->(%s, 0x%08x, %p): stub!\n", iface, debugstr_w(name), security, key );
    return E_NOTIMPL;
}

static HRESULT WINAPI JoyConfig8Impl_OpenAppStatusKey(IDirectInputJoyConfig8 *iface, PHKEY key)
{
    FIXME( "(%p)->(%p): stub!\n", iface, key );
    return E_NOTIMPL;
}

static const IDirectInput7WVtbl ddi7wvt = {
    IDirectInputWImpl_QueryInterface,
    IDirectInputWImpl_AddRef,
    IDirectInputWImpl_Release,
    IDirectInputWImpl_CreateDevice,
    IDirectInputWImpl_EnumDevices,
    IDirectInputWImpl_GetDeviceStatus,
    IDirectInputWImpl_RunControlPanel,
    IDirectInputWImpl_Initialize,
    IDirectInput2WImpl_FindDevice,
    IDirectInput7WImpl_CreateDeviceEx
};

static const IDirectInput8WVtbl ddi8wvt = {
    IDirectInput8WImpl_QueryInterface,
    IDirectInput8WImpl_AddRef,
    IDirectInput8WImpl_Release,
    IDirectInput8WImpl_CreateDevice,
    IDirectInput8WImpl_EnumDevices,
    IDirectInput8WImpl_GetDeviceStatus,
    IDirectInput8WImpl_RunControlPanel,
    IDirectInput8WImpl_Initialize,
    IDirectInput8WImpl_FindDevice,
    IDirectInput8WImpl_EnumDevicesBySemantics,
    IDirectInput8WImpl_ConfigureDevices
};

static const IDirectInputJoyConfig8Vtbl JoyConfig8vt =
{
    JoyConfig8Impl_QueryInterface,
    JoyConfig8Impl_AddRef,
    JoyConfig8Impl_Release,
    JoyConfig8Impl_Acquire,
    JoyConfig8Impl_Unacquire,
    JoyConfig8Impl_SetCooperativeLevel,
    JoyConfig8Impl_SendNotify,
    JoyConfig8Impl_EnumTypes,
    JoyConfig8Impl_GetTypeInfo,
    JoyConfig8Impl_SetTypeInfo,
    JoyConfig8Impl_DeleteType,
    JoyConfig8Impl_GetConfig,
    JoyConfig8Impl_SetConfig,
    JoyConfig8Impl_DeleteConfig,
    JoyConfig8Impl_GetUserValues,
    JoyConfig8Impl_SetUserValues,
    JoyConfig8Impl_AddNewHardware,
    JoyConfig8Impl_OpenTypeKey,
    JoyConfig8Impl_OpenAppStatusKey
};

/*******************************************************************************
 * DirectInput ClassFactory
 */
typedef struct
{
    /* IUnknown fields */
    IClassFactory IClassFactory_iface;
    LONG          ref;
} IClassFactoryImpl;

static inline IClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
        return CONTAINING_RECORD(iface, IClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI DICF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
	IClassFactoryImpl *This = impl_from_IClassFactory(iface);

	FIXME("(%p)->(%s,%p),stub!\n",This,debugstr_guid(riid),ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI DICF_AddRef(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = impl_from_IClassFactory(iface);
	return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI DICF_Release(LPCLASSFACTORY iface) {
	IClassFactoryImpl *This = impl_from_IClassFactory(iface);
	/* static class, won't be  freed */
	return InterlockedDecrement(&(This->ref));
}

static HRESULT WINAPI DICF_CreateInstance(
	LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
	IClassFactoryImpl *This = impl_from_IClassFactory(iface);

	TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,debugstr_guid(riid),ppobj);
        if ( IsEqualGUID( &IID_IUnknown, riid ) ||
             IsEqualGUID( &IID_IDirectInputA, riid ) ||
	     IsEqualGUID( &IID_IDirectInputW, riid ) ||
	     IsEqualGUID( &IID_IDirectInput2A, riid ) ||
	     IsEqualGUID( &IID_IDirectInput2W, riid ) ||
	     IsEqualGUID( &IID_IDirectInput7A, riid ) ||
            IsEqualGUID( &IID_IDirectInput7W, riid ) ||
            IsEqualGUID( &IID_IDirectInput8A, riid ) ||
	     IsEqualGUID( &IID_IDirectInput8W, riid ) )
        {
		return create_directinput_instance(riid, ppobj, NULL);
	}

	FIXME("(%p,%p,%s,%p) Interface not found!\n",This,pOuter,debugstr_guid(riid),ppobj);	
	return E_NOINTERFACE;
}

static HRESULT WINAPI DICF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
	IClassFactoryImpl *This = impl_from_IClassFactory(iface);
	FIXME("(%p)->(%d),stub!\n",This,dolock);
	return S_OK;
}

static const IClassFactoryVtbl DICF_Vtbl = {
	DICF_QueryInterface,
	DICF_AddRef,
	DICF_Release,
	DICF_CreateInstance,
	DICF_LockServer
};
static IClassFactoryImpl DINPUT_CF = {{&DICF_Vtbl}, 1 };

/***********************************************************************
 *		DllGetClassObject (DINPUT.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    if ( IsEqualCLSID( &IID_IClassFactory, riid ) ) {
        *ppv = &DINPUT_CF;
	IClassFactory_AddRef((IClassFactory*)*ppv);
    return S_OK;
    }

    FIXME("(%s,%s,%p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/******************************************************************************
 *	DInput hook thread
 */

static LRESULT CALLBACK LL_hook_proc( int code, WPARAM wparam, LPARAM lparam )
{
    struct dinput_device *impl;
    int skip = 0;

    if (code != HC_ACTION) return CallNextHookEx( 0, code, wparam, lparam );

    EnterCriticalSection( &dinput_hook_crit );
    LIST_FOR_EACH_ENTRY( impl, &acquired_mouse_list, struct dinput_device, entry )
    {
        TRACE( "calling dinput_mouse_hook (%p %lx %lx)\n", impl, wparam, lparam );
        skip |= dinput_mouse_hook( &impl->IDirectInputDevice8W_iface, wparam, lparam );
    }
    LIST_FOR_EACH_ENTRY( impl, &acquired_keyboard_list, struct dinput_device, entry )
    {
        if (impl->use_raw_input) continue;
        TRACE( "calling dinput_keyboard_hook (%p %lx %lx)\n", impl, wparam, lparam );
        skip |= dinput_keyboard_hook( &impl->IDirectInputDevice8W_iface, wparam, lparam );
    }
    LeaveCriticalSection( &dinput_hook_crit );

    return skip ? 1 : CallNextHookEx( 0, code, wparam, lparam );
}

static LRESULT CALLBACK callwndproc_proc( int code, WPARAM wparam, LPARAM lparam )
{
    struct dinput_device *impl, *next;
    CWPSTRUCT *msg = (CWPSTRUCT *)lparam;
    HWND foreground;

    if (code != HC_ACTION || (msg->message != WM_KILLFOCUS &&
        msg->message != WM_ACTIVATEAPP && msg->message != WM_ACTIVATE))
        return CallNextHookEx( 0, code, wparam, lparam );

    foreground = GetForegroundWindow();

    EnterCriticalSection( &dinput_hook_crit );
    LIST_FOR_EACH_ENTRY_SAFE( impl, next, &acquired_device_list, struct dinput_device, entry )
    {
        if (msg->hwnd == impl->win && msg->hwnd != foreground)
        {
            TRACE( "%p window is not foreground - unacquiring %p\n", impl->win, impl );
            dinput_device_internal_unacquire( &impl->IDirectInputDevice8W_iface );
        }
    }
    LIST_FOR_EACH_ENTRY_SAFE( impl, next, &acquired_mouse_list, struct dinput_device, entry )
    {
        if (msg->hwnd == impl->win && msg->hwnd != foreground)
        {
            TRACE( "%p window is not foreground - unacquiring %p\n", impl->win, impl );
            dinput_device_internal_unacquire( &impl->IDirectInputDevice8W_iface );
        }
    }
    LIST_FOR_EACH_ENTRY_SAFE( impl, next, &acquired_rawmouse_list, struct dinput_device, entry )
    {
        if (msg->hwnd == impl->win && msg->hwnd != foreground)
        {
            TRACE( "%p window is not foreground - unacquiring %p\n", impl->win, impl );
            dinput_device_internal_unacquire( &impl->IDirectInputDevice8W_iface );
        }
    }
    LIST_FOR_EACH_ENTRY_SAFE( impl, next, &acquired_keyboard_list, struct dinput_device, entry )
    {
        if (msg->hwnd == impl->win && msg->hwnd != foreground)
        {
            TRACE( "%p window is not foreground - unacquiring %p\n", impl->win, impl );
            dinput_device_internal_unacquire( &impl->IDirectInputDevice8W_iface );
        }
    }
    LeaveCriticalSection( &dinput_hook_crit );

    return CallNextHookEx( 0, code, wparam, lparam );
}

static DWORD WINAPI dinput_thread_proc( void *params )
{
    HANDLE events[128], start_event = params;
    static HHOOK kbd_hook, mouse_hook;
    struct dinput_device *impl, *next;
    SIZE_T events_count = 0;
    HANDLE finished_event;
    DWORD ret;
    MSG msg;

    di_em_win = CreateWindowW( L"DIEmWin", L"DIEmWin", 0, 0, 0, 0, 0, HWND_MESSAGE, 0, DINPUT_instance, NULL );

    /* Force creation of the message queue */
    PeekMessageW( &msg, 0, 0, 0, PM_NOREMOVE );
    SetEvent( start_event );

    while ((ret = MsgWaitForMultipleObjectsEx( events_count, events, INFINITE, QS_ALLINPUT, 0 )) <= events_count)
    {
        UINT kbd_cnt = 0, mice_cnt = 0;

        if (ret < events_count)
        {
            EnterCriticalSection( &dinput_hook_crit );
            LIST_FOR_EACH_ENTRY_SAFE( impl, next, &acquired_device_list, struct dinput_device, entry )
            {
                if (impl->read_event == events[ret])
                {
                    if (FAILED( impl->vtbl->read( &impl->IDirectInputDevice8W_iface ) ))
                    {
                        dinput_device_internal_unacquire( &impl->IDirectInputDevice8W_iface );
                        impl->status = STATUS_UNPLUGGED;
                    }
                    break;
                }
            }
            LeaveCriticalSection( &dinput_hook_crit );
        }

        while (PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ))
        {
            if (msg.message != WM_USER+0x10)
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
                continue;
            }

            finished_event = (HANDLE)msg.lParam;

            TRACE( "Processing hook change notification wp:%ld lp:%#lx\n", msg.wParam, msg.lParam );

            if (!msg.wParam)
            {
                if (kbd_hook) UnhookWindowsHookEx( kbd_hook );
                if (mouse_hook) UnhookWindowsHookEx( mouse_hook );
                kbd_hook = mouse_hook = NULL;
                goto done;
            }

            EnterCriticalSection( &dinput_hook_crit );
            kbd_cnt = list_count( &acquired_keyboard_list );
            mice_cnt = list_count( &acquired_mouse_list );
            LeaveCriticalSection( &dinput_hook_crit );

            if (kbd_cnt && !kbd_hook)
                kbd_hook = SetWindowsHookExW( WH_KEYBOARD_LL, LL_hook_proc, DINPUT_instance, 0 );
            else if (!kbd_cnt && kbd_hook)
            {
                UnhookWindowsHookEx( kbd_hook );
                kbd_hook = NULL;
            }

            if (mice_cnt && !mouse_hook)
                mouse_hook = SetWindowsHookExW( WH_MOUSE_LL, LL_hook_proc, DINPUT_instance, 0 );
            else if (!mice_cnt && mouse_hook)
            {
                UnhookWindowsHookEx( mouse_hook );
                mouse_hook = NULL;
            }

            SetEvent(finished_event);
        }

        events_count = 0;
        EnterCriticalSection( &dinput_hook_crit );
        LIST_FOR_EACH_ENTRY( impl, &acquired_device_list, struct dinput_device, entry )
        {
            if (!impl->read_event || !impl->vtbl->read) continue;
            if (events_count >= ARRAY_SIZE(events)) break;
            events[events_count++] = impl->read_event;
        }
        LeaveCriticalSection( &dinput_hook_crit );
    }

    if (ret != events_count) ERR("Unexpected termination, ret %#x\n", ret);

done:
    DestroyWindow( di_em_win );
    di_em_win = NULL;
    return 0;
}

static BOOL WINAPI dinput_thread_start_once( INIT_ONCE *once, void *param, void **context )
{
    HANDLE start_event;

    start_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    if (!start_event) ERR( "failed to create start event, error %u\n", GetLastError() );

    dinput_thread = CreateThread( NULL, 0, dinput_thread_proc, start_event, 0, &dinput_thread_id );
    if (!dinput_thread) ERR( "failed to create internal thread, error %u\n", GetLastError() );

    WaitForSingleObject( start_event, INFINITE );
    CloseHandle( start_event );

    return TRUE;
}

static void dinput_thread_start(void)
{
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;
    InitOnceExecuteOnce( &init_once, dinput_thread_start_once, NULL, NULL );
}

static void dinput_thread_stop(void)
{
    PostThreadMessageW( dinput_thread_id, WM_USER + 0x10, 0, 0 );
    if (WaitForSingleObject( dinput_thread, 500 ) == WAIT_TIMEOUT)
        WARN("Timeout while waiting for internal thread\n");
    CloseHandle( dinput_thread );
}

void check_dinput_hooks( IDirectInputDevice8W *iface, BOOL acquired )
{
    static HHOOK callwndproc_hook;
    static ULONG foreground_cnt;
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HANDLE hook_change_finished_event = NULL;

    dinput_thread_start();

    EnterCriticalSection(&dinput_hook_crit);

    if (impl->dwCoopLevel & DISCL_FOREGROUND)
    {
        if (acquired)
            foreground_cnt++;
        else
            foreground_cnt--;
    }

    if (foreground_cnt && !callwndproc_hook)
        callwndproc_hook = SetWindowsHookExW( WH_CALLWNDPROC, callwndproc_proc,
                                              DINPUT_instance, GetCurrentThreadId() );
    else if (!foreground_cnt && callwndproc_hook)
    {
        UnhookWindowsHookEx( callwndproc_hook );
        callwndproc_hook = NULL;
    }

    if (impl->use_raw_input)
    {
        if (acquired)
        {
            impl->raw_device.dwFlags = 0;
            if (impl->dwCoopLevel & DISCL_BACKGROUND)
                impl->raw_device.dwFlags |= RIDEV_INPUTSINK;
            if (impl->dwCoopLevel & DISCL_EXCLUSIVE)
                impl->raw_device.dwFlags |= RIDEV_NOLEGACY;
            if ((impl->dwCoopLevel & DISCL_EXCLUSIVE) && impl->raw_device.usUsage == 2)
                impl->raw_device.dwFlags |= RIDEV_CAPTUREMOUSE;
            if ((impl->dwCoopLevel & DISCL_EXCLUSIVE) && impl->raw_device.usUsage == 6)
                impl->raw_device.dwFlags |= RIDEV_NOHOTKEYS;
            impl->raw_device.hwndTarget = di_em_win;
        }
        else
        {
            impl->raw_device.dwFlags = RIDEV_REMOVE;
            impl->raw_device.hwndTarget = NULL;
        }

        if (!RegisterRawInputDevices( &impl->raw_device, 1, sizeof(RAWINPUTDEVICE) ))
            WARN( "Unable to (un)register raw device %x:%x\n", impl->raw_device.usUsagePage, impl->raw_device.usUsage );
    }

    hook_change_finished_event = CreateEventW( NULL, FALSE, FALSE, NULL );
    PostThreadMessageW( dinput_thread_id, WM_USER + 0x10, 1, (LPARAM)hook_change_finished_event );

    LeaveCriticalSection(&dinput_hook_crit);

    WaitForSingleObject(hook_change_finished_event, INFINITE);
    CloseHandle(hook_change_finished_event);
}

void check_dinput_events(void)
{
    /* Windows does not do that, but our current implementation of winex11
     * requires periodic event polling to forward events to the wineserver.
     *
     * We have to call this function from multiple places, because:
     * - some games do not explicitly poll for mouse events
     *   (for example Culpa Innata)
     * - some games only poll the device, and neither keyboard nor mouse
     *   (for example Civilization: Call to Power 2)
     * - some games do not explicitly poll for keyboard events
     *   (for example Morrowind in its key binding page)
     */
    MsgWaitForMultipleObjectsEx(0, NULL, 0, QS_ALLINPUT, 0);
}

BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved)
{
    switch(reason)
    {
      case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(inst);
        DINPUT_instance = inst;
        register_di_em_win_class();
        break;
      case DLL_PROCESS_DETACH:
        if (reserved) break;
        dinput_thread_stop();
        unregister_di_em_win_class();
        DeleteCriticalSection(&dinput_hook_crit);
        break;
    }
    return TRUE;
}
