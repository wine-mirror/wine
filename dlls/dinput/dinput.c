/*
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

#include <stddef.h>
#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"

#include "dinput_private.h"
#include "device_private.h"

#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

static inline struct dinput *impl_from_IDirectInput7W( IDirectInput7W *iface )
{
    return CONTAINING_RECORD( iface, struct dinput, IDirectInput7W_iface );
}

static inline struct dinput *impl_from_IDirectInput8W( IDirectInput8W *iface )
{
    return CONTAINING_RECORD( iface, struct dinput, IDirectInput8W_iface );
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

static HRESULT WINAPI dinput7_EnumDevices( IDirectInput7W *iface, DWORD type, LPDIENUMDEVICESCALLBACKW callback,
                                           void *context, DWORD flags )
{
    struct dinput *impl = impl_from_IDirectInput7W( iface );

    TRACE( "iface %p, type %#lx, callback %p, context %p, flags %#lx.\n", iface, type, callback, context, flags );

    if (!callback) return DIERR_INVALIDPARAM;

    if (type > DIDEVTYPE_JOYSTICK) return DIERR_INVALIDPARAM;
    if (flags & ~(DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK | DIEDFL_INCLUDEALIASES | DIEDFL_INCLUDEPHANTOMS))
        return DIERR_INVALIDPARAM;

    return IDirectInput8_EnumDevices( &impl->IDirectInput8W_iface, type, callback, context, flags );
}

void dinput_internal_addref( struct dinput *impl )
{
    ULONG ref = InterlockedIncrement( &impl->internal_ref );
    TRACE( "impl %p, internal ref %lu.\n", impl, ref );
}

void dinput_internal_release( struct dinput *impl )
{
    ULONG ref = InterlockedDecrement( &impl->internal_ref );
    TRACE( "impl %p, internal ref %lu.\n", impl, ref );

    if (!ref)
    {
        struct DevicePlayer *device_player, *device_player2;

        LIST_FOR_EACH_ENTRY_SAFE( device_player, device_player2, &impl->device_players, struct DevicePlayer, entry )
            free( device_player );

        free( impl );
    }
}

static ULONG WINAPI dinput7_AddRef( IDirectInput7W *iface )
{
    struct dinput *impl = impl_from_IDirectInput7W( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI dinput7_Release( IDirectInput7W *iface )
{
    struct dinput *impl = impl_from_IDirectInput7W( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        input_thread_remove_user();
        dinput_internal_release( impl );
    }

    return ref;
}

static HRESULT WINAPI dinput7_QueryInterface( IDirectInput7W *iface, REFIID iid, void **out )
{
    struct dinput *impl = impl_from_IDirectInput7W( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (!iid || !out) return E_POINTER;

    *out = NULL;

#if DIRECTINPUT_VERSION == 0x0700
    if (IsEqualGUID( &IID_IDirectInputA, iid ) ||
        IsEqualGUID( &IID_IDirectInput2A, iid ) ||
        IsEqualGUID( &IID_IDirectInput7A, iid ))
        *out = &impl->IDirectInput7A_iface;
    else if (IsEqualGUID( &IID_IUnknown, iid ) ||
             IsEqualGUID( &IID_IDirectInputW, iid ) ||
             IsEqualGUID( &IID_IDirectInput2W, iid ) ||
             IsEqualGUID( &IID_IDirectInput7W, iid ))
        *out = &impl->IDirectInput7W_iface;
#else
    if (IsEqualGUID( &IID_IDirectInput8A, iid ))
        *out = &impl->IDirectInput8A_iface;
    else if (IsEqualGUID( &IID_IUnknown, iid ) ||
             IsEqualGUID( &IID_IDirectInput8W, iid ))
        *out = &impl->IDirectInput8W_iface;
#endif

    if (IsEqualGUID( &IID_IDirectInputJoyConfig8, iid ))
        *out = &impl->IDirectInputJoyConfig8_iface;

    if (*out)
    {
        IUnknown_AddRef( (IUnknown *)*out );
        return DI_OK;
    }

    WARN( "Unsupported interface: %s\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
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

static HRESULT WINAPI dinput7_Initialize( IDirectInput7W *iface, HINSTANCE hinst, DWORD version )
{
    struct dinput *impl = impl_from_IDirectInput7W( iface );

    TRACE( "iface %p, hinst %p, version %#lx.\n", iface, hinst, version );

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

    if (!impl->dwVersion)
    {
        impl->dwVersion = version;
        impl->evsequence = 1;
    }

    return DI_OK;
}

static HRESULT WINAPI dinput7_GetDeviceStatus( IDirectInput7W *iface, const GUID *guid )
{
    struct dinput *impl = impl_from_IDirectInput7W( iface );
    HRESULT hr;
    IDirectInputDeviceW *device;

    TRACE( "iface %p, guid %s.\n", iface, debugstr_guid( guid ) );

    if (!guid) return E_POINTER;
    if (!impl->dwVersion) return DIERR_NOTINITIALIZED;

    hr = IDirectInput_CreateDevice( iface, guid, &device, NULL );
    if (hr != DI_OK) return DI_NOTATTACHED;

    IUnknown_Release( device );

    return DI_OK;
}

static HRESULT WINAPI dinput7_RunControlPanel( IDirectInput7W *iface, HWND owner, DWORD flags )
{
    struct dinput *impl = impl_from_IDirectInput7W( iface );
    WCHAR control_exe[] = {L"control.exe"};
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi;

    TRACE( "iface %p, owner %p, flags %#lx.\n", iface, owner, flags );

    if (owner && !IsWindow( owner )) return E_HANDLE;
    if (flags) return DIERR_INVALIDPARAM;
    if (!impl->dwVersion) return DIERR_NOTINITIALIZED;

    if (!CreateProcessW( NULL, control_exe, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi ))
        return HRESULT_FROM_WIN32(GetLastError());

    return DI_OK;
}

static HRESULT WINAPI dinput7_FindDevice( IDirectInput7W *iface, const GUID *guid, const WCHAR *name, GUID *instance_guid )
{
    FIXME( "iface %p, guid %s, name %s, instance_guid %s stub!\n", iface, debugstr_guid( guid ),
           debugstr_w(name), debugstr_guid( instance_guid ) );
    return DI_OK;
}

static HRESULT WINAPI dinput7_CreateDeviceEx( IDirectInput7W *iface, const GUID *guid,
                                              REFIID iid, void **out, IUnknown *outer )
{
    struct dinput *impl = impl_from_IDirectInput7W( iface );
    IDirectInputDevice8W *device;
    HRESULT hr;

    TRACE( "iface %p, guid %s, iid %s, out %p, outer %p.\n", iface, debugstr_guid( guid ),
           debugstr_guid( iid ), out, outer );

    if (!out) return E_POINTER;
    *out = NULL;

    if (!guid) return E_POINTER;
    if (!impl->dwVersion) return DIERR_NOTINITIALIZED;

    if (IsEqualGUID( &GUID_SysKeyboard, guid )) hr = keyboard_create_device( impl, guid, &device );
    else if (IsEqualGUID( &GUID_SysMouse, guid )) hr = mouse_create_device( impl, guid, &device );
    else hr = hid_joystick_create_device( impl, guid, &device );

    if (FAILED(hr)) return hr;

    hr = IDirectInputDevice8_QueryInterface( device, iid, out );
    IDirectInputDevice8_Release( device );
    return hr;
}

static HRESULT WINAPI dinput7_CreateDevice( IDirectInput7W *iface, const GUID *guid,
                                            IDirectInputDeviceW **out, IUnknown *outer )
{
    return IDirectInput7_CreateDeviceEx( iface, guid, &IID_IDirectInputDeviceW, (void **)out, outer );
}

static ULONG WINAPI dinput8_AddRef( IDirectInput8W *iface )
{
    struct dinput *impl = impl_from_IDirectInput8W( iface );
    return IDirectInput7_AddRef( &impl->IDirectInput7W_iface );
}

static HRESULT WINAPI dinput8_QueryInterface( IDirectInput8W *iface, REFIID iid, void **out )
{
    struct dinput *impl = impl_from_IDirectInput8W( iface );
    return IDirectInput7_QueryInterface( &impl->IDirectInput7W_iface, iid, out );
}

static ULONG WINAPI dinput8_Release( IDirectInput8W *iface )
{
    struct dinput *impl = impl_from_IDirectInput8W( iface );
    return IDirectInput7_Release( &impl->IDirectInput7W_iface );
}

static HRESULT WINAPI dinput8_CreateDevice( IDirectInput8W *iface, const GUID *guid,
                                            IDirectInputDevice8W **out, IUnknown *outer )
{
    struct dinput *impl = impl_from_IDirectInput8W( iface );
    return IDirectInput7_CreateDeviceEx( &impl->IDirectInput7W_iface, guid,
                                         &IID_IDirectInputDevice8W, (void **)out, outer );
}

static BOOL try_enum_device( DWORD type, LPDIENUMDEVICESCALLBACKW callback,
                             DIDEVICEINSTANCEW *instance, void *context, DWORD flags )
{
    if (type && (instance->dwDevType & 0xff) != type) return DIENUM_CONTINUE;
    if ((flags & DIEDFL_FORCEFEEDBACK) && IsEqualGUID( &instance->guidFFDriver, &GUID_NULL ))
        return DIENUM_CONTINUE;
    return enum_callback_wrapper( callback, instance, context );
}

static HRESULT WINAPI dinput8_EnumDevices( IDirectInput8W *iface, DWORD type, LPDIENUMDEVICESCALLBACKW callback, void *context,
                                           DWORD flags )
{
    DIDEVICEINSTANCEW instance = {.dwSize = sizeof(DIDEVICEINSTANCEW)};
    struct dinput *impl = impl_from_IDirectInput8W( iface );
    DWORD device_class = 0, device_type = 0;
    unsigned int i = 0;
    HRESULT hr;

    TRACE( "iface %p, type %#lx, callback %p, context %p, flags %#lx.\n", iface, type, callback, context, flags );

    if (!callback) return DIERR_INVALIDPARAM;

    if ((type > DI8DEVCLASS_GAMECTRL && type < DI8DEVTYPE_DEVICE) || type > DI8DEVTYPE_SUPPLEMENTAL)
        return DIERR_INVALIDPARAM;
    if (flags & ~(DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK | DIEDFL_INCLUDEALIASES |
                  DIEDFL_INCLUDEPHANTOMS | DIEDFL_INCLUDEHIDDEN))
        return DIERR_INVALIDPARAM;

    if (!impl->dwVersion) return DIERR_NOTINITIALIZED;

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

static HRESULT WINAPI dinput8_GetDeviceStatus( IDirectInput8W *iface, const GUID *guid )
{
    struct dinput *impl = impl_from_IDirectInput8W( iface );
    return IDirectInput7_GetDeviceStatus( &impl->IDirectInput7W_iface, guid );
}

static HRESULT WINAPI dinput8_RunControlPanel( IDirectInput8W *iface, HWND owner, DWORD flags )
{
    struct dinput *impl = impl_from_IDirectInput8W( iface );
    return IDirectInput7_RunControlPanel( &impl->IDirectInput7W_iface, owner, flags );
}

static HRESULT WINAPI dinput8_Initialize( IDirectInput8W *iface, HINSTANCE hinst, DWORD version )
{
    struct dinput *impl = impl_from_IDirectInput8W( iface );

    TRACE( "iface %p, hinst %p, version %#lx.\n", iface, hinst, version );

    if (!hinst)
        return DIERR_INVALIDPARAM;
    else if (version == 0)
        return DIERR_NOTINITIALIZED;
    else if (version < DIRECTINPUT_VERSION)
        return DIERR_BETADIRECTINPUTVERSION;
    else if (version > DIRECTINPUT_VERSION)
        return DIERR_OLDDIRECTINPUTVERSION;

    if (!impl->dwVersion)
    {
        impl->dwVersion = version;
        impl->evsequence = 1;
    }

    return DI_OK;
}

static HRESULT WINAPI dinput8_FindDevice( IDirectInput8W *iface, const GUID *guid, const WCHAR *name, GUID *instance_guid )
{
    struct dinput *impl = impl_from_IDirectInput8W( iface );
    return IDirectInput7_FindDevice( &impl->IDirectInput7W_iface, guid, name, instance_guid );
}

struct enum_device_by_semantics_params
{
    IDirectInput8W *iface;
    const WCHAR *username;
    DWORD flags;

    IDirectInputDevice8W *devices[128];
    DWORD device_count;
};

static BOOL CALLBACK enum_device_by_semantics( const DIDEVICEINSTANCEW *instance, void *context )
{
    struct enum_device_by_semantics_params *params = context;
    DIDEVCAPS caps = {.dwSize = sizeof(caps)};
    DIPROPSTRING prop_username =
    {
        .diph =
        {
            .dwSize = sizeof(DIPROPSTRING),
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwHow = DIPH_DEVICE,
        },
    };
    IDirectInputDevice8W *device;
    BOOL ret = DIENUM_CONTINUE;
    HRESULT hr;

    if (params->device_count >= ARRAY_SIZE(params->devices)) return DIENUM_STOP;

    if (FAILED(hr = IDirectInput8_CreateDevice( params->iface, &instance->guidInstance, &device, NULL )))
    {
        WARN( "Failed to create device, hr %#lx\n", hr );
        return DIENUM_CONTINUE;
    }

    if (FAILED(hr = IDirectInputDevice8_GetCapabilities( device, &caps )))
        WARN( "Failed to get device capabilities, hr %#lx\n", hr );
    if ((params->flags & DIEDBSFL_FORCEFEEDBACK) && !caps.dwFFDriverVersion) goto done;

    if (FAILED(hr = IDirectInputDevice8_GetProperty( device, DIPROP_USERNAME, &prop_username.diph )))
        WARN( "Failed to get device capabilities, hr %#lx\n", hr );

    if ((params->flags & DIEDBSFL_AVAILABLEDEVICES) && !*prop_username.wsz)
    {
        params->devices[params->device_count++] = device;
        return DIENUM_CONTINUE;
    }
    if ((params->flags & DIEDBSFL_THISUSER) && *prop_username.wsz &&
        (!*params->username || !wcscmp( params->username, prop_username.wsz )))
    {
        params->devices[params->device_count++] = device;
        return DIENUM_CONTINUE;
    }
    if (!params->flags)
    {
        params->devices[params->device_count++] = device;
        return DIENUM_CONTINUE;
    }

done:
    IDirectInputDevice8_Release( device );
    return ret;
}

struct enum_device_object_semantics_params
{
    DIDEVICEINSTANCEW instance;
    DIACTIONFORMATW *format;
    DWORD flags;
};

static BOOL CALLBACK enum_device_object_semantics( const DIDEVICEOBJECTINSTANCEW *obj, void *args )
{
    struct enum_device_object_semantics_params *params = args;
    DIACTIONFORMATW *format = params->format;
    UINT i;

    for (i = 0; format && i < format->dwNumActions; i++)
    {
        DIOBJECTDATAFORMAT object_format = {.dwType = obj->dwType, .dwOfs = obj->dwOfs};
        BYTE dev_type = params->instance.dwDevType & 0xf;
        DIACTIONW *action = format->rgoAction + i;

        if (!device_object_matches_semantic( &params->instance, &object_format, action->dwSemantic, FALSE )) continue;
        if (!(action->dwSemantic & 0x4000)) params->flags |= DIEDBS_MAPPEDPRI1;
        else if (dev_type != DIDEVTYPE_KEYBOARD && dev_type != DIDEVTYPE_MOUSE) params->flags |= DIEDBS_MAPPEDPRI2;
    }

    return DIENUM_CONTINUE;
}

static HRESULT WINAPI dinput8_EnumDevicesBySemantics( IDirectInput8W *iface, const WCHAR *username, DIACTIONFORMATW *action_format,
                                                      LPDIENUMDEVICESBYSEMANTICSCBW callback, void *context, DWORD flags )
{
    struct enum_device_by_semantics_params params = {.iface = iface, .username = username ? username : L"", .flags = flags};
    DWORD enum_flags = DIEDFL_ATTACHEDONLY | (flags & DIEDFL_FORCEFEEDBACK);
    struct dinput *impl = impl_from_IDirectInput8W( iface );
    unsigned int i = 0;
    HRESULT hr;

    TRACE( "iface %p, username %s, action_format %p, callback %p, context %p, flags %#lx\n",
           iface, debugstr_w(username), action_format, callback, context, flags );

    if (!action_format) return DIERR_INVALIDPARAM;

    TRACE( "format guid %s, genre %#lx, name %s\n", debugstr_guid(&action_format->guidActionMap),
           action_format->dwGenre, debugstr_w(action_format->tszActionMap) );
    for (i = 0; i < action_format->dwNumActions; i++)
    {
        DIACTIONW *action = action_format->rgoAction + i;
        TRACE( "  %u: app_data %#Ix, semantic %#lx, flags %#lx, instance %s, obj_id %#lx, how %#lx, name %s\n",
               i, action->uAppData, action->dwSemantic, action->dwFlags, debugstr_guid(&action->guidInstance),
               action->dwObjID, action->dwHow, debugstr_w(action->lptszActionName) );
    }

    if (FAILED(hr = IDirectInput8_EnumDevices( &impl->IDirectInput8W_iface, DI8DEVCLASS_ALL,
                                               enum_device_by_semantics, &params, enum_flags )))
    {
        WARN( "Failed to enumerate devices, hr %#lx\n", hr );
        goto cleanup;
    }

    while (params.device_count--)
    {
        struct enum_device_object_semantics_params object_params = {.instance = {.dwSize = sizeof(DIDEVICEINSTANCEW)}, .format = action_format};
        IDirectInputDevice8W *device = params.devices[params.device_count];
        BOOL ret = DIENUM_STOP;

        if (FAILED(hr = IDirectInputDevice8_GetDeviceInfo( device, &object_params.instance )))
            WARN( "Failed to get device %p info, hr %#lx\n", device, hr );
        else if (FAILED(hr = IDirectInputDevice8_EnumObjects( device, enum_device_object_semantics, &object_params, DIDFT_ALL )))
            WARN( "Failed to enumerate device %p objects, hr %#lx\n", device, hr );
        else
            ret = callback( &object_params.instance, device, object_params.flags, params.device_count, context );

        IDirectInputDevice8_Release( device );
        if (ret == DIENUM_STOP) goto cleanup;
    }

    return DI_OK;

cleanup:
    while (params.device_count--) IDirectInputDevice8_Release( params.devices[params.device_count] );
    return hr;
}

static HRESULT WINAPI dinput8_ConfigureDevices( IDirectInput8W *iface, LPDICONFIGUREDEVICESCALLBACK callback,
                                                DICONFIGUREDEVICESPARAMSW *params, DWORD flags, void *context )
{
    FIXME( "iface %p, callback %p, params %p, flags %#lx, context %p stub!\n", iface, callback,
           params, flags, context );

    /* Call helper function in config.c to do the real work */
    return _configure_devices( iface, callback, params, flags, context );
}

static inline struct dinput *impl_from_IDirectInputJoyConfig8( IDirectInputJoyConfig8 *iface )
{
    return CONTAINING_RECORD( iface, struct dinput, IDirectInputJoyConfig8_iface );
}

static HRESULT WINAPI joy_config_QueryInterface( IDirectInputJoyConfig8 *iface, REFIID iid, void **out )
{
    struct dinput *impl = impl_from_IDirectInputJoyConfig8( iface );
    return IDirectInput7_QueryInterface( &impl->IDirectInput7W_iface, iid, out );
}

static ULONG WINAPI joy_config_AddRef( IDirectInputJoyConfig8 *iface )
{
    struct dinput *impl = impl_from_IDirectInputJoyConfig8( iface );
    return IDirectInput7_AddRef( &impl->IDirectInput7W_iface );
}

static ULONG WINAPI joy_config_Release( IDirectInputJoyConfig8 *iface )
{
    struct dinput *impl = impl_from_IDirectInputJoyConfig8( iface );
    return IDirectInput7_Release( &impl->IDirectInput7W_iface );
}

static HRESULT WINAPI joy_config_Acquire( IDirectInputJoyConfig8 *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI joy_config_Unacquire( IDirectInputJoyConfig8 *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI joy_config_SetCooperativeLevel( IDirectInputJoyConfig8 *iface, HWND hwnd, DWORD flags )
{
    FIXME( "iface %p, hwnd %p, flags %#lx stub!\n", iface, hwnd, flags );
    return E_NOTIMPL;
}

static HRESULT WINAPI joy_config_SendNotify( IDirectInputJoyConfig8 *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI joy_config_EnumTypes( IDirectInputJoyConfig8 *iface, LPDIJOYTYPECALLBACK callback, void *context )
{
    FIXME( "iface %p, callback %p, context %p stub!\n", iface, callback, context );
    return E_NOTIMPL;
}

static HRESULT WINAPI joy_config_GetTypeInfo( IDirectInputJoyConfig8 *iface, const WCHAR *name,
                                              DIJOYTYPEINFO *info, DWORD flags )
{
    FIXME( "iface %p, name %s, info %p, flags %#lx stub!\n", iface, debugstr_w(name), info, flags );
    return E_NOTIMPL;
}

static HRESULT WINAPI joy_config_SetTypeInfo( IDirectInputJoyConfig8 *iface, const WCHAR *name,
                                              const DIJOYTYPEINFO *info, DWORD flags, WCHAR *new_name )
{
    FIXME( "iface %p, name %s, info %p, flags %#lx, new_name %s stub!\n", iface, debugstr_w(name),
           info, flags, debugstr_w(new_name) );
    return E_NOTIMPL;
}

static HRESULT WINAPI joy_config_DeleteType( IDirectInputJoyConfig8 *iface, const WCHAR *name )
{
    FIXME( "iface %p, name %s stub!\n", iface, debugstr_w(name) );
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

static HRESULT WINAPI joy_config_GetConfig( IDirectInputJoyConfig8 *iface, UINT id, DIJOYCONFIG *info, DWORD flags )
{
    struct dinput *impl = impl_from_IDirectInputJoyConfig8( iface );
    struct find_device_from_index_params params = {.index = id};
    HRESULT hr;

    FIXME( "iface %p, id %u, info %p, flags %#lx stub!\n", iface, id, info, flags );

#define X(x) if (flags & x) FIXME("\tflags |= "#x"\n");
    X(DIJC_GUIDINSTANCE)
    X(DIJC_REGHWCONFIGTYPE)
    X(DIJC_GAIN)
    X(DIJC_CALLOUT)
#undef X

    hr = IDirectInput8_EnumDevices( &impl->IDirectInput8W_iface, DI8DEVCLASS_GAMECTRL,
                                    find_device_from_index, &params, 0 );
    if (FAILED(hr)) return hr;
    if (params.index != ~0) return DIERR_NOMOREITEMS;
    if (flags & DIJC_GUIDINSTANCE) info->guidInstance = params.instance.guidInstance;
    return DI_OK;
}

static HRESULT WINAPI joy_config_SetConfig( IDirectInputJoyConfig8 *iface, UINT id, const DIJOYCONFIG *info, DWORD flags )
{
    FIXME( "iface %p, id %u, info %p, flags %#lx stub!\n", iface, id, info, flags );
    return E_NOTIMPL;
}

static HRESULT WINAPI joy_config_DeleteConfig( IDirectInputJoyConfig8 *iface, UINT id )
{
    FIXME( "iface %p, id %u stub!\n", iface, id );
    return E_NOTIMPL;
}

static HRESULT WINAPI joy_config_GetUserValues( IDirectInputJoyConfig8 *iface, DIJOYUSERVALUES *info, DWORD flags )
{
    FIXME( "iface %p, info %p, flags %#lx stub!\n", iface, info, flags );
    return E_NOTIMPL;
}

static HRESULT WINAPI joy_config_SetUserValues( IDirectInputJoyConfig8 *iface, const DIJOYUSERVALUES *info, DWORD flags )
{
    FIXME( "iface %p, info %p, flags %#lx stub!\n", iface, info, flags );
    return E_NOTIMPL;
}

static HRESULT WINAPI joy_config_AddNewHardware( IDirectInputJoyConfig8 *iface, HWND hwnd, const GUID *guid )
{
    FIXME( "iface %p, hwnd %p, guid %s stub!\n", iface, hwnd, debugstr_guid( guid ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI joy_config_OpenTypeKey( IDirectInputJoyConfig8 *iface, const WCHAR *name, DWORD security, HKEY *key )
{
    FIXME( "iface %p, name %s, security %lu, key %p stub!\n", iface, debugstr_w(name), security, key );
    return E_NOTIMPL;
}

static HRESULT WINAPI joy_config_OpenAppStatusKey( IDirectInputJoyConfig8 *iface, HKEY *key )
{
    FIXME( "iface %p, key %p stub!\n", iface, key );
    return E_NOTIMPL;
}

static const IDirectInput7WVtbl dinput7_vtbl =
{
    dinput7_QueryInterface,
    dinput7_AddRef,
    dinput7_Release,
    dinput7_CreateDevice,
    dinput7_EnumDevices,
    dinput7_GetDeviceStatus,
    dinput7_RunControlPanel,
    dinput7_Initialize,
    dinput7_FindDevice,
    dinput7_CreateDeviceEx,
};

static const IDirectInput8WVtbl dinput8_vtbl =
{
    dinput8_QueryInterface,
    dinput8_AddRef,
    dinput8_Release,
    dinput8_CreateDevice,
    dinput8_EnumDevices,
    dinput8_GetDeviceStatus,
    dinput8_RunControlPanel,
    dinput8_Initialize,
    dinput8_FindDevice,
    dinput8_EnumDevicesBySemantics,
    dinput8_ConfigureDevices,
};

static const IDirectInputJoyConfig8Vtbl joy_config_vtbl =
{
    joy_config_QueryInterface,
    joy_config_AddRef,
    joy_config_Release,
    joy_config_Acquire,
    joy_config_Unacquire,
    joy_config_SetCooperativeLevel,
    joy_config_SendNotify,
    joy_config_EnumTypes,
    joy_config_GetTypeInfo,
    joy_config_SetTypeInfo,
    joy_config_DeleteType,
    joy_config_GetConfig,
    joy_config_SetConfig,
    joy_config_DeleteConfig,
    joy_config_GetUserValues,
    joy_config_SetUserValues,
    joy_config_AddNewHardware,
    joy_config_OpenTypeKey,
    joy_config_OpenAppStatusKey,
};

static HRESULT dinput_create( IUnknown **out )
{
    struct dinput *impl;

    if (!(impl = calloc( 1, sizeof(struct dinput) ))) return E_OUTOFMEMORY;
    impl->IDirectInput7A_iface.lpVtbl = &dinput7_a_vtbl;
    impl->IDirectInput7W_iface.lpVtbl = &dinput7_vtbl;
    impl->IDirectInput8A_iface.lpVtbl = &dinput8_a_vtbl;
    impl->IDirectInput8W_iface.lpVtbl = &dinput8_vtbl;
    impl->IDirectInputJoyConfig8_iface.lpVtbl = &joy_config_vtbl;
    impl->internal_ref = 1;
    impl->ref = 1;

    list_init( &impl->device_players );

#if DIRECTINPUT_VERSION == 0x0700
    *out = (IUnknown *)&impl->IDirectInput7W_iface;
#else
    *out = (IUnknown *)&impl->IDirectInput8W_iface;
#endif

    input_thread_add_user();
    return DI_OK;
}

struct class_factory
{
    IClassFactory IClassFactory_iface;
};

static inline struct class_factory *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD( iface, struct class_factory, IClassFactory_iface );
}

static HRESULT WINAPI class_factory_QueryInterface( IClassFactory *iface, REFIID iid, void **out )
{
    struct class_factory *impl = impl_from_IClassFactory( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IClassFactory ))
        *out = &impl->IClassFactory_iface;
    else
    {
        *out = NULL;
        WARN( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown *)*out );
    return S_OK;
}

static ULONG WINAPI class_factory_AddRef( IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI class_factory_Release( IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI class_factory_CreateInstance( IClassFactory *iface, IUnknown *outer, REFIID iid, void **out )
{
    IUnknown *unknown;
    HRESULT hr;

    TRACE( "iface %p, outer %p, iid %s, out %p.\n", iface, outer, debugstr_guid( iid ), out );

    if (outer) return CLASS_E_NOAGGREGATION;

    if (FAILED(hr = dinput_create( &unknown ))) return hr;
    hr = IUnknown_QueryInterface( unknown, iid, out );
    IUnknown_Release( unknown );

    return hr;
}

static HRESULT WINAPI class_factory_LockServer( IClassFactory *iface, BOOL lock )
{
    FIXME( "iface %p, lock %d stub!\n", iface, lock );
    return S_OK;
}

static const IClassFactoryVtbl class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    class_factory_CreateInstance,
    class_factory_LockServer,
};

static struct class_factory class_factory = {{&class_factory_vtbl}};

HRESULT WINAPI DllGetClassObject( REFCLSID clsid, REFIID iid, void **out )
{
    TRACE( "clsid %s, iid %s, out %p.\n", debugstr_guid( clsid ), debugstr_guid( iid ), out );

#if DIRECTINPUT_VERSION == 0x0700
    if (IsEqualCLSID( &CLSID_DirectInput, clsid ))
        return IClassFactory_QueryInterface( &class_factory.IClassFactory_iface, iid, out );
#else
    if (IsEqualCLSID( &CLSID_DirectInput8, clsid ))
        return IClassFactory_QueryInterface( &class_factory.IClassFactory_iface, iid, out );
#endif

    WARN( "%s not implemented, returning CLASS_E_CLASSNOTAVAILABLE.\n", debugstr_guid( clsid ) );
    return CLASS_E_CLASSNOTAVAILABLE;
}

#if DIRECTINPUT_VERSION == 0x0700

HRESULT WINAPI DirectInputCreateEx( HINSTANCE hinst, DWORD version, REFIID iid, void **out, IUnknown *outer )
{
    IUnknown *unknown;
    HRESULT hr;

    TRACE( "hinst %p, version %#lx, iid %s, out %p, outer %p.\n", hinst, version, debugstr_guid( iid ), out, outer );

    if (!IsEqualGUID( &IID_IDirectInputA, iid ) &&
        !IsEqualGUID( &IID_IDirectInputW, iid ) &&
        !IsEqualGUID( &IID_IDirectInput2A, iid ) &&
        !IsEqualGUID( &IID_IDirectInput2W, iid ) &&
        !IsEqualGUID( &IID_IDirectInput7A, iid ) &&
        !IsEqualGUID( &IID_IDirectInput7W, iid ))
        return DIERR_NOINTERFACE;

    if (FAILED(hr = dinput_create( &unknown ))) return hr;
    hr = IUnknown_QueryInterface( unknown, iid, out );
    IUnknown_Release( unknown );
    if (FAILED(hr)) return hr;

    if (outer || FAILED(hr = IDirectInput7_Initialize( (IDirectInput7W *)unknown, hinst, version )))
    {
        IUnknown_Release( unknown );
        *out = NULL;
        return hr;
    }

    return DI_OK;
}

HRESULT WINAPI DECLSPEC_HOTPATCH DirectInputCreateA( HINSTANCE hinst, DWORD version, IDirectInputA **out, IUnknown *outer )
{
    return DirectInputCreateEx( hinst, version, &IID_IDirectInput7A, (void **)out, outer );
}

HRESULT WINAPI DECLSPEC_HOTPATCH DirectInputCreateW( HINSTANCE hinst, DWORD version, IDirectInputW **out, IUnknown *outer )
{
    return DirectInputCreateEx( hinst, version, &IID_IDirectInput7W, (void **)out, outer );
}

#else

HRESULT WINAPI DECLSPEC_HOTPATCH DirectInput8Create( HINSTANCE hinst, DWORD version, REFIID iid, void **out, IUnknown *outer )
{
    IUnknown *unknown;
    HRESULT hr;

    TRACE( "hinst %p, version %#lx, iid %s, out %p, outer %p.\n", hinst, version, debugstr_guid( iid ), out, outer );

    if (!out) return E_POINTER;

    if (!IsEqualGUID( &IID_IDirectInput8A, iid ) &&
        !IsEqualGUID( &IID_IDirectInput8W, iid ) &&
        !IsEqualGUID( &IID_IUnknown, iid ))
    {
        *out = NULL;
        return DIERR_NOINTERFACE;
    }

    if (FAILED(hr = dinput_create( &unknown ))) return hr;
    hr = IUnknown_QueryInterface( unknown, iid, out );
    IUnknown_Release( unknown );
    if (FAILED(hr)) return hr;

    if (outer || FAILED(hr = IDirectInput8_Initialize( (IDirectInput8W *)unknown, hinst, version )))
    {
        IUnknown_Release( unknown );
        *out = NULL;
        return hr;
    }

    return S_OK;
}

#endif
