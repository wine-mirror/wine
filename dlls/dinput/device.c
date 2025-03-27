/*		DirectInput Device
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998,1999 Lionel Ulmer
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

#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "winerror.h"
#include "dinput.h"
#include "dinputd.h"
#include "hidusage.h"

#include "initguid.h"
#include "device_private.h"
#include "dinput_private.h"

#include "wine/debug.h"

#define WM_WINE_NOTIFY_ACTIVITY WM_USER

WINE_DEFAULT_DEBUG_CHANNEL(dinput);

/* Windows uses this GUID for guidProduct on non-keyboard/mouse devices.
 * Data1 contains the device VID (low word) and PID (high word).
 * Data4 ends with the ASCII bytes "PIDVID".
 */
DEFINE_GUID( dinput_pidvid_guid, 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 'P', 'I', 'D', 'V', 'I', 'D' );

static inline struct dinput_device *impl_from_IDirectInputDevice8W( IDirectInputDevice8W *iface )
{
    return CONTAINING_RECORD( iface, struct dinput_device, IDirectInputDevice8W_iface );
}

static inline IDirectInputDevice8A *IDirectInputDevice8A_from_impl( struct dinput_device *This )
{
    return &This->IDirectInputDevice8A_iface;
}
static inline IDirectInputDevice8W *IDirectInputDevice8W_from_impl( struct dinput_device *This )
{
    return &This->IDirectInputDevice8W_iface;
}

static inline const char *debugstr_didataformat( const DIDATAFORMAT *data )
{
    if (!data) return "(null)";
    return wine_dbg_sprintf( "%p dwSize %lu, dwObjsize %lu, dwFlags %#lx, dwDataSize %lu, dwNumObjs %lu, rgodf %p",
                             data, data->dwSize, data->dwObjSize, data->dwFlags, data->dwDataSize, data->dwNumObjs, data->rgodf );
}

static inline const char *debugstr_diobjectdataformat( const DIOBJECTDATAFORMAT *data )
{
    if (!data) return "(null)";
    return wine_dbg_sprintf( "%p pguid %s, dwOfs %#lx, dwType %#lx, dwFlags %#lx", data,
                             debugstr_guid( data->pguid ), data->dwOfs, data->dwType, data->dwFlags );
}

static inline BOOL is_exclusively_acquired( struct dinput_device *device )
{
    return device->status == STATUS_ACQUIRED && (device->dwCoopLevel & DISCL_EXCLUSIVE);
}

static void _dump_cooperativelevel_DI(DWORD dwFlags) {
    if (TRACE_ON(dinput)) {
	unsigned int   i;
	static const struct {
	    DWORD       mask;
	    const char  *name;
	} flags[] = {
#define FE(x) { x, #x}
	    FE(DISCL_BACKGROUND),
	    FE(DISCL_EXCLUSIVE),
	    FE(DISCL_FOREGROUND),
	    FE(DISCL_NONEXCLUSIVE),
	    FE(DISCL_NOWINKEY)
#undef FE
	};
	TRACE(" cooperative level : ");
	for (i = 0; i < ARRAY_SIZE(flags); i++)
	    if (flags[i].mask & dwFlags)
		TRACE("%s ",flags[i].name);
	TRACE("\n");
    }
}

BOOL get_app_key(HKEY *defkey, HKEY *appkey)
{
    char buffer[MAX_PATH+16];
    DWORD len;

    *appkey = 0;

    /* @@ Wine registry key: HKCU\Software\Wine\DirectInput */
    if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\DirectInput", defkey))
        *defkey = 0;

    len = GetModuleFileNameA(0, buffer, MAX_PATH);
    if (len && len < MAX_PATH)
    {
        HKEY tmpkey;

        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\DirectInput */
        if (!RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\AppDefaults", &tmpkey))
        {
            char *p, *appname = buffer;
            if ((p = strrchr(appname, '/'))) appname = p + 1;
            if ((p = strrchr(appname, '\\'))) appname = p + 1;
            strcat(appname, "\\DirectInput");

            if (RegOpenKeyA(tmpkey, appname, appkey)) *appkey = 0;
            RegCloseKey(tmpkey);
        }
    }

    return *defkey || *appkey;
}

DWORD get_config_key( HKEY defkey, HKEY appkey, const WCHAR *name, WCHAR *buffer, DWORD size )
{
    if (appkey && !RegQueryValueExW( appkey, name, 0, NULL, (LPBYTE)buffer, &size )) return 0;

    if (defkey && !RegQueryValueExW( defkey, name, 0, NULL, (LPBYTE)buffer, &size )) return 0;

    return ERROR_FILE_NOT_FOUND;
}

BOOL device_instance_is_disabled( DIDEVICEINSTANCEW *instance, BOOL *override )
{
    static const WCHAR disabled_str[] = {'d', 'i', 's', 'a', 'b', 'l', 'e', 'd', 0};
    static const WCHAR override_str[] = {'o', 'v', 'e', 'r', 'r', 'i', 'd', 'e', 0};
    static const WCHAR joystick_key[] = {'J', 'o', 'y', 's', 't', 'i', 'c', 'k', 's', 0};
    WCHAR buffer[MAX_PATH];
    HKEY hkey, appkey, temp;
    BOOL disable = FALSE;

    get_app_key( &hkey, &appkey );
    if (override) *override = FALSE;

    /* Joystick settings are in the 'joysticks' subkey */
    if (appkey)
    {
        if (RegOpenKeyW( appkey, joystick_key, &temp )) temp = 0;
        RegCloseKey( appkey );
        appkey = temp;
    }

    if (hkey)
    {
        if (RegOpenKeyW( hkey, joystick_key, &temp )) temp = 0;
        RegCloseKey( hkey );
        hkey = temp;
    }

    /* Look for the "controllername"="disabled" key */
    if (!get_config_key( hkey, appkey, instance->tszInstanceName, buffer, sizeof(buffer) ))
    {
        if (!wcscmp( disabled_str, buffer ))
        {
            TRACE( "Disabling joystick '%s' based on registry key.\n", debugstr_w(instance->tszInstanceName) );
            disable = TRUE;
        }
        else if (override && !wcscmp( override_str, buffer ))
        {
            TRACE( "Force enabling joystick '%s' based on registry key.\n", debugstr_w(instance->tszInstanceName) );
            *override = TRUE;
        }
    }

    if (appkey) RegCloseKey( appkey );
    if (hkey) RegCloseKey( hkey );

    return disable;
}

static void dinput_device_release_user_format( struct dinput_device *impl )
{
    free( impl->user_format.rgodf );
    impl->user_format.rgodf = NULL;
}

static inline LPDIOBJECTDATAFORMAT dataformat_to_odf(LPCDIDATAFORMAT df, int idx)
{
    if (idx < 0 || idx >= df->dwNumObjs) return NULL;
    return (LPDIOBJECTDATAFORMAT)((LPBYTE)df->rgodf + idx * df->dwObjSize);
}

/* dataformat_to_odf_by_type
 *  Find the Nth object of the selected type in the DataFormat
 */
LPDIOBJECTDATAFORMAT dataformat_to_odf_by_type(LPCDIDATAFORMAT df, int n, DWORD type)
{
    int i, nfound = 0;

    for (i=0; i < df->dwNumObjs; i++)
    {
        LPDIOBJECTDATAFORMAT odf = dataformat_to_odf(df, i);

        if (odf->dwType & type)
        {
            if (n == nfound)
                return odf;

            nfound++;
        }
    }

    return NULL;
}

static BOOL match_device_object( const DIDATAFORMAT *device_format, DIDATAFORMAT *user_format,
                                 const DIOBJECTDATAFORMAT *match_obj, DWORD version, BOOL *identical )
{
    DWORD i, device_instance, instance = DIDFT_GETINSTANCE( match_obj->dwType );
    DIOBJECTDATAFORMAT *device_obj, *user_obj;

    if (version < 0x0700 && instance == 0xff) instance = 0xffff;

    for (i = 0; i < device_format->dwNumObjs; i++)
    {
        user_obj = user_format->rgodf + i;
        device_obj = device_format->rgodf + i;
        device_instance = DIDFT_GETINSTANCE( device_obj->dwType );

        if (!(user_obj->dwType & DIDFT_OPTIONAL)) continue; /* already matched */
        if (match_obj->pguid && device_obj->pguid && !IsEqualGUID( device_obj->pguid, match_obj->pguid )) continue;
        if (instance != DIDFT_GETINSTANCE( DIDFT_ANYINSTANCE ) && instance != device_instance) continue;
        if (!(DIDFT_GETTYPE( match_obj->dwType ) & DIDFT_GETTYPE( device_obj->dwType ))) continue;

        TRACE( "match %s with device %s\n", debugstr_diobjectdataformat( match_obj ),
               debugstr_diobjectdataformat( device_obj ) );

        *user_obj = *device_obj;
        if (user_obj->dwOfs != match_obj->dwOfs) *identical = FALSE;
        user_obj->dwOfs = match_obj->dwOfs;
        return TRUE;
    }

    return FALSE;
}

static HRESULT dinput_device_init_user_format( struct dinput_device *impl, const DIDATAFORMAT *format )
{
    DIDATAFORMAT *user_format = &impl->user_format, *device_format = &impl->device_format;
    DIOBJECTDATAFORMAT *user_obj, *match_obj;
    BOOL identical = TRUE;
    DWORD i;

    *user_format = *device_format;
    user_format->dwFlags = format->dwFlags;
    user_format->dwDataSize = format->dwDataSize;
    user_format->dwNumObjs += format->dwNumObjs;
    if (!(user_format->rgodf = calloc( user_format->dwNumObjs, sizeof(DIOBJECTDATAFORMAT) ))) return DIERR_OUTOFMEMORY;

    user_obj = user_format->rgodf + user_format->dwNumObjs;
    while (user_obj-- > user_format->rgodf) user_obj->dwType |= DIDFT_OPTIONAL;

    for (i = 0; i < device_format->dwNumObjs; i++)
        impl->object_properties[i].app_data = -1;

    for (i = 0; i < format->dwNumObjs; ++i)
    {
        match_obj = format->rgodf + i;

        if (!match_device_object( device_format, user_format, match_obj, impl->dinput->dwVersion, &identical ))
        {
            WARN( "object %s not found\n", debugstr_diobjectdataformat( match_obj ) );
            if (!(match_obj->dwType & DIDFT_OPTIONAL)) goto failed;
            user_obj = user_format->rgodf + device_format->dwNumObjs + i;
            *user_obj = *match_obj;
            identical = FALSE;
        }
    }

    user_obj = user_format->rgodf + user_format->dwNumObjs;
    while (user_obj-- > user_format->rgodf) user_obj->dwType &= ~DIDFT_OPTIONAL;

    if (identical && device_format->dwDataSize <= user_format->dwDataSize)
    {
        memcpy( user_format->rgodf, device_format->rgodf, device_format->dwNumObjs * sizeof(*user_format->rgodf) );
        user_format->dwNumObjs = device_format->dwNumObjs;
    }

    return DI_OK;

failed:
    free( user_format->rgodf );
    user_format->rgodf = NULL;
    return DIERR_INVALIDPARAM;
}

int dinput_device_object_index_from_id( IDirectInputDevice8W *iface, DWORD id )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    const DIDATAFORMAT *format = &impl->device_format;
    const DIOBJECTDATAFORMAT *object;

    if (!format->rgodf) return -1;

    object = format->rgodf + impl->device_format.dwNumObjs;
    while (object-- > format->rgodf)
    {
        if (!object->dwType) continue;
        if ((object->dwType & 0x00ffffff) == (id & 0x00ffffff)) return object - format->rgodf;
    }

    return -1;
}

/*
 * get_mapping_key
 * Retrieves an open registry key to save the mapping, parametrized for an username,
 * specific device and specific action mapping guid.
 */
static HKEY get_mapping_key( const WCHAR *device, const WCHAR *username, const WCHAR *guid, BOOL delete )
{
    static const WCHAR format[] = L"Software\\Wine\\DirectInput\\Mappings\\%s\\%s\\%s";
    SIZE_T len = wcslen( format ) + wcslen( username ) + wcslen( device ) + wcslen( guid ) + 1;
    WCHAR *keyname;
    HKEY hkey;

    if (!(keyname = malloc( sizeof(WCHAR) * len ))) return 0;

    /* The key used is HKCU\Software\Wine\DirectInput\Mappings\[username]\[device]\[mapping_guid] */
    swprintf( keyname, len, format, username, device, guid );

    if (delete) RegDeleteTreeW( HKEY_CURRENT_USER, keyname );
    if (RegCreateKeyW( HKEY_CURRENT_USER, keyname, &hkey )) hkey = 0;

    free( keyname );

    return hkey;
}

static HRESULT save_mapping_settings(IDirectInputDevice8W *iface, LPDIACTIONFORMATW lpdiaf, LPCWSTR lpszUsername)
{
    WCHAR *guid_str = NULL;
    DIDEVICEINSTANCEW didev;
    HKEY hkey;
    int i;

    didev.dwSize = sizeof(didev);
    IDirectInputDevice8_GetDeviceInfo(iface, &didev);

    if (StringFromCLSID(&lpdiaf->guidActionMap, &guid_str) != S_OK)
        return DI_SETTINGSNOTSAVED;

    if (!(hkey = get_mapping_key( didev.tszInstanceName, lpszUsername, guid_str, TRUE )))
    {
        CoTaskMemFree(guid_str);
        return DI_SETTINGSNOTSAVED;
    }

    /* Write each of the actions mapped for this device.
       Format is "dwSemantic"="dwObjID" and key is of type REG_DWORD
    */
    for (i = 0; i < lpdiaf->dwNumActions; i++)
    {
        WCHAR label[9];

        if (IsEqualGUID(&didev.guidInstance, &lpdiaf->rgoAction[i].guidInstance) &&
            lpdiaf->rgoAction[i].dwHow != DIAH_UNMAPPED)
        {
            swprintf( label, 9, L"%x", lpdiaf->rgoAction[i].dwSemantic );
            RegSetValueExW( hkey, label, 0, REG_DWORD, (const BYTE *)&lpdiaf->rgoAction[i].dwObjID,
                            sizeof(DWORD) );
        }
    }

    RegCloseKey(hkey);
    CoTaskMemFree(guid_str);

    return DI_OK;
}

static BOOL load_mapping_settings( struct dinput_device *This, LPDIACTIONFORMATW lpdiaf, const WCHAR *username )
{
    HKEY hkey;
    WCHAR *guid_str;
    DIDEVICEINSTANCEW didev;
    int i, mapped = 0;

    didev.dwSize = sizeof(didev);
    IDirectInputDevice8_GetDeviceInfo(&This->IDirectInputDevice8W_iface, &didev);

    if (StringFromCLSID(&lpdiaf->guidActionMap, &guid_str) != S_OK)
        return FALSE;

    if (!(hkey = get_mapping_key( didev.tszInstanceName, username, guid_str, FALSE )))
    {
        CoTaskMemFree(guid_str);
        return FALSE;
    }

    /* Try to read each action in the DIACTIONFORMAT from registry */
    for (i = 0; i < lpdiaf->dwNumActions; i++)
    {
        DIACTIONW *action = lpdiaf->rgoAction + i;
        DWORD id, size = sizeof(DWORD);
        WCHAR label[9];

        swprintf( label, 9, L"%x", lpdiaf->rgoAction[i].dwSemantic );

        if (!action->dwHow && !RegQueryValueExW( hkey, label, 0, NULL, (BYTE *)&id, &size ))
        {
            action->dwObjID = id;
            action->guidInstance = didev.guidInstance;
            action->dwHow = DIAH_DEFAULT;
            mapped += 1;
        }
    }

    RegCloseKey(hkey);
    CoTaskMemFree(guid_str);

    return mapped > 0;
}

void queue_event( IDirectInputDevice8W *iface, int index, DWORD data, DWORD time, DWORD seq )
{
    static ULONGLONG notify_ms = 0;
    struct dinput_device *This = impl_from_IDirectInputDevice8W( iface );
    struct object_properties *properties = This->object_properties + index;
    const DIOBJECTDATAFORMAT *user_obj = This->user_format.rgodf + index;
    ULONGLONG time_ms = GetTickCount64();
    int next_pos;

    if (time_ms - notify_ms > 1000)
    {
        PostMessageW(GetDesktopWindow(), WM_WINE_NOTIFY_ACTIVITY, 0, 0);
        notify_ms = time_ms;
    }

    if (!This->queue_len || This->overflow || !user_obj->dwType) return;

    next_pos = (This->queue_head + 1) % This->queue_len;
    if (next_pos == This->queue_tail)
    {
        TRACE(" queue overflowed\n");
        This->overflow = TRUE;
        return;
    }

    TRACE( " queueing %lu at offset %lu (queue head %u / size %u)\n", data, user_obj->dwOfs, This->queue_head, This->queue_len );

    This->data_queue[This->queue_head].dwOfs       = user_obj->dwOfs;
    This->data_queue[This->queue_head].dwData      = data;
    This->data_queue[This->queue_head].dwTimeStamp = time;
    This->data_queue[This->queue_head].dwSequence  = seq;
    This->data_queue[This->queue_head].uAppData    = properties->app_data;

    This->queue_head = next_pos;
    /* Send event if asked */
}

static HRESULT WINAPI dinput_device_Acquire( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr = DI_OK;
    DWORD pid;

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->crit );
    if (impl->status == STATUS_ACQUIRED)
        hr = DI_NOEFFECT;
    else if (!impl->user_format.rgodf)
        hr = DIERR_INVALIDPARAM;
    else if ((impl->dwCoopLevel & DISCL_FOREGROUND) && impl->win != GetForegroundWindow())
        hr = DIERR_OTHERAPPHASPRIO;
    else if ((impl->dwCoopLevel & DISCL_FOREGROUND) && (!GetWindowThreadProcessId( impl->win, &pid ) || pid != GetCurrentProcessId()))
        hr = DIERR_INVALIDPARAM;
    else
    {
        impl->status = STATUS_ACQUIRED;
        if (FAILED(hr = impl->vtbl->acquire( iface ))) impl->status = STATUS_UNACQUIRED;
    }
    LeaveCriticalSection( &impl->crit );
    if (hr != DI_OK) return hr;

    dinput_hooks_acquire_device( iface );

    return hr;
}

static HRESULT WINAPI dinput_device_Unacquire( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr = DI_OK;

    TRACE( "iface %p.\n", iface );

    EnterCriticalSection( &impl->crit );
    if (impl->status != STATUS_ACQUIRED) hr = DI_NOEFFECT;
    else hr = impl->vtbl->unacquire( iface );
    impl->status = STATUS_UNACQUIRED;
    LeaveCriticalSection( &impl->crit );
    if (hr != DI_OK) return hr;

    dinput_hooks_unacquire_device( iface );

    return hr;
}

static HRESULT WINAPI dinput_device_SetDataFormat( IDirectInputDevice8W *iface, const DIDATAFORMAT *format )
{
    struct dinput_device *This = impl_from_IDirectInputDevice8W( iface );
    HRESULT res = DI_OK;
    ULONG i;

    TRACE( "iface %p, format %p.\n", iface, format );

    if (!format) return E_POINTER;
    if (TRACE_ON( dinput ))
    {
        TRACE( "user format %s\n", debugstr_didataformat( format ) );
        for (i = 0; i < format->dwNumObjs; ++i) TRACE( "  %lu: object %s\n", i, debugstr_diobjectdataformat( format->rgodf + i ) );
    }

    if (format->dwSize != sizeof(DIDATAFORMAT)) return DIERR_INVALIDPARAM;
    if (format->dwObjSize != sizeof(DIOBJECTDATAFORMAT)) return DIERR_INVALIDPARAM;
    if (This->status == STATUS_ACQUIRED) return DIERR_ACQUIRED;

    EnterCriticalSection(&This->crit);

    dinput_device_release_user_format( This );
    res = dinput_device_init_user_format( This, format );

    LeaveCriticalSection(&This->crit);
    return res;
}

static HRESULT WINAPI dinput_device_SetCooperativeLevel( IDirectInputDevice8W *iface, HWND hwnd, DWORD flags )
{
    struct dinput_device *This = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr;

    TRACE( "iface %p, hwnd %p, flags %#lx.\n", iface, hwnd, flags );

    _dump_cooperativelevel_DI( flags );

    if ((flags & (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE)) == 0 ||
        (flags & (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE)) == (DISCL_EXCLUSIVE | DISCL_NONEXCLUSIVE) ||
        (flags & (DISCL_FOREGROUND | DISCL_BACKGROUND)) == 0 ||
        (flags & (DISCL_FOREGROUND | DISCL_BACKGROUND)) == (DISCL_FOREGROUND | DISCL_BACKGROUND))
        return DIERR_INVALIDPARAM;

    if (hwnd && GetWindowLongW(hwnd, GWL_STYLE) & WS_CHILD) return E_HANDLE;

    if (!hwnd && flags == (DISCL_NONEXCLUSIVE | DISCL_BACKGROUND)) hwnd = GetDesktopWindow();

    if (!IsWindow(hwnd)) return E_HANDLE;

    /* For security reasons native does not allow exclusive background level
       for mouse and keyboard only */
    if (flags & DISCL_EXCLUSIVE && flags & DISCL_BACKGROUND &&
        (IsEqualGUID( &This->guid, &GUID_SysMouse ) || IsEqualGUID( &This->guid, &GUID_SysKeyboard )))
        return DIERR_UNSUPPORTED;

    EnterCriticalSection(&This->crit);
    if (This->status == STATUS_ACQUIRED) hr = DIERR_ACQUIRED;
    else
    {
        This->win = hwnd;
        This->dwCoopLevel = flags;
        hr = DI_OK;
    }
    LeaveCriticalSection(&This->crit);

    return hr;
}

static HRESULT WINAPI dinput_device_GetDeviceInfo( IDirectInputDevice8W *iface, DIDEVICEINSTANCEW *instance )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DWORD size;

    TRACE( "iface %p, instance %p.\n", iface, instance );

    if (!instance) return E_POINTER;
    if (instance->dwSize != sizeof(DIDEVICEINSTANCE_DX3W) &&
        instance->dwSize != sizeof(DIDEVICEINSTANCEW))
        return DIERR_INVALIDPARAM;

    size = instance->dwSize;
    memcpy( instance, &impl->instance, size );
    instance->dwSize = size;

    return S_OK;
}

static HRESULT WINAPI dinput_device_SetEventNotification( IDirectInputDevice8W *iface, HANDLE event )
{
    struct dinput_device *This = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, event %p.\n", iface, event );

    EnterCriticalSection(&This->crit);
    This->hEvent = event;
    LeaveCriticalSection(&This->crit);
    return DI_OK;
}

void dinput_device_internal_addref( struct dinput_device *impl )
{
    ULONG ref = InterlockedIncrement( &impl->internal_ref );
    TRACE( "impl %p, internal ref %lu.\n", impl, ref );
}

void dinput_device_internal_release( struct dinput_device *impl )
{
    ULONG ref = InterlockedDecrement( &impl->internal_ref );
    TRACE( "impl %p, internal ref %lu.\n", impl, ref );

    if (!ref)
    {
        if (impl->vtbl->destroy) impl->vtbl->destroy( &impl->IDirectInputDevice8W_iface );

        free( impl->object_properties );
        free( impl->data_queue );

        free( impl->device_format.rgodf );
        dinput_device_release_user_format( impl );

        dinput_internal_release( impl->dinput );
        impl->crit.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection( &impl->crit );

        free( impl );
    }
}

static ULONG WINAPI dinput_device_Release( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p, ref %lu.\n", iface, ref );

    if (!ref)
    {
        IDirectInputDevice_Unacquire( iface );
        input_thread_remove_user();
        dinput_device_internal_release( impl );
    }

    return ref;
}

static HRESULT WINAPI dinput_device_GetCapabilities( IDirectInputDevice8W *iface, DIDEVCAPS *caps )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DWORD size;

    TRACE( "iface %p, caps %p.\n", iface, caps );

    if (!caps) return E_POINTER;
    if (caps->dwSize != sizeof(DIDEVCAPS_DX3) &&
        caps->dwSize != sizeof(DIDEVCAPS))
        return DIERR_INVALIDPARAM;

    size = caps->dwSize;
    memcpy( caps, &impl->caps, size );
    caps->dwSize = size;

    return DI_OK;
}

static HRESULT WINAPI dinput_device_QueryInterface( IDirectInputDevice8W *iface, const GUID *iid, void **out )
{
    struct dinput_device *This = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( &IID_IDirectInputDeviceA, iid ) ||
        IsEqualGUID( &IID_IDirectInputDevice2A, iid ) ||
        IsEqualGUID( &IID_IDirectInputDevice7A, iid ) ||
        IsEqualGUID( &IID_IDirectInputDevice8A, iid ))
    {
        IDirectInputDevice2_AddRef(iface);
        *out = IDirectInputDevice8A_from_impl( This );
        return DI_OK;
    }

    if (IsEqualGUID( &IID_IUnknown, iid ) ||
        IsEqualGUID( &IID_IDirectInputDeviceW, iid ) ||
        IsEqualGUID( &IID_IDirectInputDevice2W, iid ) ||
        IsEqualGUID( &IID_IDirectInputDevice7W, iid ) ||
        IsEqualGUID( &IID_IDirectInputDevice8W, iid ))
    {
        IDirectInputDevice2_AddRef(iface);
        *out = IDirectInputDevice8W_from_impl( This );
        return DI_OK;
    }

    WARN( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    return E_NOINTERFACE;
}

static ULONG WINAPI dinput_device_AddRef( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p, ref %lu.\n", iface, ref );
    return ref;
}

struct enum_objects_params
{
    LPDIENUMDEVICEOBJECTSCALLBACKW callback;
    void *context;
};

static BOOL enum_objects_callback( struct dinput_device *impl, UINT index, struct hid_value_caps *caps,
                                   const DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct enum_objects_params *params = data;
    if (instance->wUsagePage == HID_USAGE_PAGE_PID && !(instance->dwType & DIDFT_NODATA))
        return DIENUM_CONTINUE;

    /* Applications may return non-zero values instead of DIENUM_CONTINUE. */
    return params->callback( instance, params->context ) ? DIENUM_CONTINUE : DIENUM_STOP;
}

static HRESULT WINAPI dinput_device_EnumObjects( IDirectInputDevice8W *iface, LPDIENUMDEVICEOBJECTSCALLBACKW callback,
                                                 void *context, DWORD flags )
{
    static const DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = DIPH_DEVICE,
    };
    struct enum_objects_params params = {.callback = callback, .context = context};
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr;

    TRACE( "iface %p, callback %p, context %p, flags %#lx.\n", iface, callback, context, flags );

    if (!callback) return DIERR_INVALIDPARAM;
    if (flags & ~(DIDFT_AXIS | DIDFT_POV | DIDFT_BUTTON | DIDFT_NODATA | DIDFT_COLLECTION))
        return DIERR_INVALIDPARAM;

    if (flags == DIDFT_ALL || (flags & DIDFT_AXIS))
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_AXIS, enum_objects_callback, &params );
        if (FAILED(hr)) return hr;
        if (hr != DIENUM_CONTINUE) return DI_OK;
    }
    if (flags == DIDFT_ALL || (flags & DIDFT_POV))
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_POV, enum_objects_callback, &params );
        if (FAILED(hr)) return hr;
        if (hr != DIENUM_CONTINUE) return DI_OK;
    }
    if (flags == DIDFT_ALL || (flags & DIDFT_BUTTON))
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_BUTTON, enum_objects_callback, &params );
        if (FAILED(hr)) return hr;
        if (hr != DIENUM_CONTINUE) return DI_OK;
    }
    if (flags == DIDFT_ALL || (flags & (DIDFT_NODATA | DIDFT_COLLECTION)))
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_NODATA, enum_objects_callback, &params );
        if (FAILED(hr)) return hr;
        if (hr != DIENUM_CONTINUE) return DI_OK;
    }

    return DI_OK;
}

static HRESULT enum_object_filter_init( struct dinput_device *impl, DIPROPHEADER *filter )
{
    DIOBJECTDATAFORMAT *user_objs = impl->user_format.rgodf;
    DWORD i, count = impl->device_format.dwNumObjs;

    if (filter->dwHow > DIPH_BYUSAGE) return DIERR_INVALIDPARAM;
    if (filter->dwHow == DIPH_BYUSAGE && !(impl->instance.dwDevType & DIDEVTYPE_HID)) return DIERR_UNSUPPORTED;
    if (filter->dwHow != DIPH_BYOFFSET) return DI_OK;

    if (!user_objs) return DIERR_NOTFOUND;

    for (i = 0; i < count; i++)
    {
        if (!user_objs[i].dwType) continue;
        if (user_objs[i].dwOfs == filter->dwObj) break;
    }
    if (i == count) return DIERR_NOTFOUND;

    filter->dwObj = impl->device_format.rgodf[i].dwOfs;
    return DI_OK;
}

static HRESULT check_property( struct dinput_device *impl, const GUID *guid, const DIPROPHEADER *header, BOOL set )
{
    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_VIDPID:
    case (DWORD_PTR)DIPROP_TYPENAME:
    case (DWORD_PTR)DIPROP_USERNAME:
    case (DWORD_PTR)DIPROP_KEYNAME:
    case (DWORD_PTR)DIPROP_LOGICALRANGE:
    case (DWORD_PTR)DIPROP_PHYSICALRANGE:
    case (DWORD_PTR)DIPROP_APPDATA:
    case (DWORD_PTR)DIPROP_SCANCODE:
        if (impl->dinput->dwVersion < 0x0800) return DIERR_UNSUPPORTED;
        break;
    }

    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_INSTANCENAME:
    case (DWORD_PTR)DIPROP_KEYNAME:
    case (DWORD_PTR)DIPROP_PRODUCTNAME:
    case (DWORD_PTR)DIPROP_TYPENAME:
    case (DWORD_PTR)DIPROP_USERNAME:
        if (header->dwSize != sizeof(DIPROPSTRING)) return DIERR_INVALIDPARAM;
        break;

    case (DWORD_PTR)DIPROP_AUTOCENTER:
    case (DWORD_PTR)DIPROP_AXISMODE:
    case (DWORD_PTR)DIPROP_BUFFERSIZE:
    case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
    case (DWORD_PTR)DIPROP_DEADZONE:
    case (DWORD_PTR)DIPROP_FFGAIN:
    case (DWORD_PTR)DIPROP_FFLOAD:
    case (DWORD_PTR)DIPROP_GRANULARITY:
    case (DWORD_PTR)DIPROP_JOYSTICKID:
    case (DWORD_PTR)DIPROP_SATURATION:
    case (DWORD_PTR)DIPROP_SCANCODE:
    case (DWORD_PTR)DIPROP_VIDPID:
        if (header->dwSize != sizeof(DIPROPDWORD)) return DIERR_INVALIDPARAM;
        break;

    case (DWORD_PTR)DIPROP_APPDATA:
        if (header->dwSize != sizeof(DIPROPPOINTER)) return DIERR_INVALIDPARAM;
        break;

    case (DWORD_PTR)DIPROP_PHYSICALRANGE:
    case (DWORD_PTR)DIPROP_LOGICALRANGE:
    case (DWORD_PTR)DIPROP_RANGE:
        if (header->dwSize != sizeof(DIPROPRANGE)) return DIERR_INVALIDPARAM;
        break;

    case (DWORD_PTR)DIPROP_GUIDANDPATH:
        if (header->dwSize != sizeof(DIPROPGUIDANDPATH)) return DIERR_INVALIDPARAM;
        break;
    }

    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_PRODUCTNAME:
    case (DWORD_PTR)DIPROP_INSTANCENAME:
    case (DWORD_PTR)DIPROP_VIDPID:
    case (DWORD_PTR)DIPROP_JOYSTICKID:
    case (DWORD_PTR)DIPROP_GUIDANDPATH:
    case (DWORD_PTR)DIPROP_BUFFERSIZE:
    case (DWORD_PTR)DIPROP_FFGAIN:
    case (DWORD_PTR)DIPROP_TYPENAME:
    case (DWORD_PTR)DIPROP_USERNAME:
    case (DWORD_PTR)DIPROP_AUTOCENTER:
    case (DWORD_PTR)DIPROP_AXISMODE:
    case (DWORD_PTR)DIPROP_FFLOAD:
        if (header->dwHow != DIPH_DEVICE) return DIERR_UNSUPPORTED;
        if (header->dwObj) return DIERR_INVALIDPARAM;
        break;

    case (DWORD_PTR)DIPROP_PHYSICALRANGE:
    case (DWORD_PTR)DIPROP_LOGICALRANGE:
    case (DWORD_PTR)DIPROP_RANGE:
    case (DWORD_PTR)DIPROP_DEADZONE:
    case (DWORD_PTR)DIPROP_SATURATION:
    case (DWORD_PTR)DIPROP_GRANULARITY:
    case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
        if (header->dwHow == DIPH_DEVICE && !set) return DIERR_UNSUPPORTED;
        break;

    case (DWORD_PTR)DIPROP_KEYNAME:
    case (DWORD_PTR)DIPROP_SCANCODE:
        if (header->dwHow == DIPH_DEVICE) return DIERR_INVALIDPARAM;
        break;

    case (DWORD_PTR)DIPROP_APPDATA:
        if (header->dwHow == DIPH_DEVICE) return DIERR_UNSUPPORTED;
        break;
    }

    if (set)
    {
        switch (LOWORD( guid ))
        {
        case (DWORD_PTR)DIPROP_AUTOCENTER:
            if (impl->status == STATUS_ACQUIRED) return DIERR_ACQUIRED;
            break;
        case (DWORD_PTR)DIPROP_AXISMODE:
        case (DWORD_PTR)DIPROP_BUFFERSIZE:
        case (DWORD_PTR)DIPROP_PHYSICALRANGE:
        case (DWORD_PTR)DIPROP_LOGICALRANGE:
            if (impl->status == STATUS_ACQUIRED) return DIERR_ACQUIRED;
            break;
        case (DWORD_PTR)DIPROP_FFLOAD:
        case (DWORD_PTR)DIPROP_GRANULARITY:
        case (DWORD_PTR)DIPROP_VIDPID:
        case (DWORD_PTR)DIPROP_TYPENAME:
        case (DWORD_PTR)DIPROP_USERNAME:
        case (DWORD_PTR)DIPROP_GUIDANDPATH:
            return DIERR_READONLY;
        }

        switch (LOWORD( guid ))
        {
        case (DWORD_PTR)DIPROP_RANGE:
        {
            const DIPROPRANGE *value = (const DIPROPRANGE *)header;
            if (value->lMin > value->lMax) return DIERR_INVALIDPARAM;
            break;
        }
        case (DWORD_PTR)DIPROP_DEADZONE:
        case (DWORD_PTR)DIPROP_SATURATION:
        case (DWORD_PTR)DIPROP_FFGAIN:
        {
            const DIPROPDWORD *value = (const DIPROPDWORD *)header;
            if (value->dwData > 10000) return DIERR_INVALIDPARAM;
            break;
        }
        case (DWORD_PTR)DIPROP_AUTOCENTER:
        case (DWORD_PTR)DIPROP_AXISMODE:
        case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
        {
            const DIPROPDWORD *value = (const DIPROPDWORD *)header;
            if (value->dwData > 1) return DIERR_INVALIDPARAM;
            break;
        }
        case (DWORD_PTR)DIPROP_PHYSICALRANGE:
        case (DWORD_PTR)DIPROP_LOGICALRANGE:
            return DIERR_UNSUPPORTED;
        }
    }
    else
    {
        switch (LOWORD( guid ))
        {
        case (DWORD_PTR)DIPROP_RANGE:
        case (DWORD_PTR)DIPROP_GRANULARITY:
            if (!impl->caps.dwAxes) return DIERR_UNSUPPORTED;
            break;

        case (DWORD_PTR)DIPROP_KEYNAME:
            /* not supported on the mouse */
            if (impl->caps.dwAxes && !(impl->caps.dwDevType & DIDEVTYPE_HID)) return DIERR_UNSUPPORTED;
            break;

        case (DWORD_PTR)DIPROP_PHYSICALRANGE:
        case (DWORD_PTR)DIPROP_LOGICALRANGE:
        case (DWORD_PTR)DIPROP_DEADZONE:
        case (DWORD_PTR)DIPROP_SATURATION:
        case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
            /* not supported on the mouse or keyboard */
            if (!(impl->caps.dwDevType & DIDEVTYPE_HID)) return DIERR_UNSUPPORTED;
            break;

        case (DWORD_PTR)DIPROP_FFLOAD:
            if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;
            if (!is_exclusively_acquired( impl )) return DIERR_NOTEXCLUSIVEACQUIRED;
            /* fallthrough */
        case (DWORD_PTR)DIPROP_PRODUCTNAME:
        case (DWORD_PTR)DIPROP_INSTANCENAME:
        case (DWORD_PTR)DIPROP_VIDPID:
        case (DWORD_PTR)DIPROP_JOYSTICKID:
        case (DWORD_PTR)DIPROP_GUIDANDPATH:
            if (!impl->vtbl->get_property) return DIERR_UNSUPPORTED;
            break;
        }
    }

    return DI_OK;
}

struct get_object_property_params
{
    IDirectInputDevice8W *iface;
    DIPROPHEADER *header;
    DWORD property;
};

static BOOL get_object_property( struct dinput_device *device, UINT index, struct hid_value_caps *caps,
                                 const DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct get_object_property_params *params = data;
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( params->iface );
    const struct object_properties *properties;

    if (index == -1) return DIENUM_STOP;
    properties = impl->object_properties + index;

    switch (params->property)
    {
    case (DWORD_PTR)DIPROP_PHYSICALRANGE:
    {
        DIPROPRANGE *value = (DIPROPRANGE *)params->header;
        value->lMin = properties->physical_min;
        value->lMax = properties->physical_max;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_LOGICALRANGE:
    {
        DIPROPRANGE *value = (DIPROPRANGE *)params->header;
        value->lMin = properties->logical_min;
        value->lMax = properties->logical_max;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_RANGE:
    {
        DIPROPRANGE *value = (DIPROPRANGE *)params->header;
        value->lMin = properties->range_min;
        value->lMax = properties->range_max;
        return DIENUM_STOP;
    }
    case (DWORD_PTR)DIPROP_DEADZONE:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)params->header;
        value->dwData = properties->deadzone;
        return DIENUM_STOP;
    }
    case (DWORD_PTR)DIPROP_SATURATION:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)params->header;
        value->dwData = properties->saturation;
        return DIENUM_STOP;
    }
    case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)params->header;
        value->dwData = properties->calibration_mode;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_GRANULARITY:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)params->header;
        value->dwData = properties->granularity;
        return DIENUM_STOP;
    }
    case (DWORD_PTR)DIPROP_KEYNAME:
    {
        DIPROPSTRING *value = (DIPROPSTRING *)params->header;
        lstrcpynW( value->wsz, instance->tszName, ARRAY_SIZE(value->wsz) );
        return DIENUM_STOP;
    }
    case (DWORD_PTR)DIPROP_APPDATA:
    {
        DIPROPPOINTER *value = (DIPROPPOINTER *)params->header;
        value->uData = properties->app_data;
        return DIENUM_STOP;
    }
    case (DWORD_PTR)DIPROP_SCANCODE:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)params->header;
        value->dwData = properties->scan_code;
        return DI_OK;
    }
    }

    return DIENUM_STOP;
}

static HRESULT dinput_device_get_property( IDirectInputDevice8W *iface, const GUID *guid, DIPROPHEADER *header )
{
    struct get_object_property_params params = {.iface = iface, .header = header, .property = LOWORD( guid )};
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DWORD object_mask = DIDFT_AXIS | DIDFT_BUTTON | DIDFT_POV;
    DIPROPHEADER filter;
    HRESULT hr;

    filter = *header;
    if (FAILED(hr = enum_object_filter_init( impl, &filter ))) return hr;
    if (FAILED(hr = check_property( impl, guid, header, FALSE ))) return hr;

    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_PRODUCTNAME:
    case (DWORD_PTR)DIPROP_INSTANCENAME:
    case (DWORD_PTR)DIPROP_VIDPID:
    case (DWORD_PTR)DIPROP_JOYSTICKID:
    case (DWORD_PTR)DIPROP_GUIDANDPATH:
    case (DWORD_PTR)DIPROP_FFLOAD:
        return impl->vtbl->get_property( iface, LOWORD( guid ), header, NULL );

    case (DWORD_PTR)DIPROP_RANGE:
    case (DWORD_PTR)DIPROP_PHYSICALRANGE:
    case (DWORD_PTR)DIPROP_LOGICALRANGE:
    case (DWORD_PTR)DIPROP_DEADZONE:
    case (DWORD_PTR)DIPROP_SATURATION:
    case (DWORD_PTR)DIPROP_GRANULARITY:
    case (DWORD_PTR)DIPROP_KEYNAME:
    case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
    case (DWORD_PTR)DIPROP_APPDATA:
    case (DWORD_PTR)DIPROP_SCANCODE:
        hr = impl->vtbl->enum_objects( iface, &filter, object_mask, get_object_property, &params );
        if (FAILED(hr)) return hr;
        if (hr == DIENUM_CONTINUE) return DIERR_NOTFOUND;
        return DI_OK;

    case (DWORD_PTR)DIPROP_AUTOCENTER:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;
        value->dwData = impl->autocenter;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_BUFFERSIZE:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        value->dwData = impl->buffersize;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_USERNAME:
    {
        DIPROPSTRING *value = (DIPROPSTRING *)header;
        struct DevicePlayer *device_player;
        LIST_FOR_EACH_ENTRY( device_player, &impl->dinput->device_players, struct DevicePlayer, entry )
        {
            if (IsEqualGUID( &device_player->instance_guid, &impl->guid ))
            {
                if (!*device_player->username) break;
                lstrcpynW( value->wsz, device_player->username, ARRAY_SIZE(value->wsz) );
                return DI_OK;
            }
        }
        return S_FALSE;
    }
    case (DWORD_PTR)DIPROP_FFGAIN:
    {
        DIPROPDWORD *value = (DIPROPDWORD *)header;
        value->dwData = impl->device_gain;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_CALIBRATION:
        return DIERR_INVALIDPARAM;
    default:
        FIXME( "Unknown property %s\n", debugstr_guid( guid ) );
        return DIERR_UNSUPPORTED;
    }

    return DI_OK;
}

static HRESULT WINAPI dinput_device_GetProperty( IDirectInputDevice8W *iface, const GUID *guid, DIPROPHEADER *header )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr;

    TRACE( "iface %p, guid %s, header %p\n", iface, debugstr_guid( guid ), header );

    if (!header) return DIERR_INVALIDPARAM;
    if (header->dwHeaderSize != sizeof(DIPROPHEADER)) return DIERR_INVALIDPARAM;
    if (!IS_DIPROP( guid )) return DI_OK;

    EnterCriticalSection( &impl->crit );
    hr = dinput_device_get_property( iface, guid, header );
    LeaveCriticalSection( &impl->crit );

    return hr;
}

struct set_object_property_params
{
    IDirectInputDevice8W *iface;
    const DIPROPHEADER *header;
    DWORD property;
};

static BOOL set_object_property( struct dinput_device *device, UINT index, struct hid_value_caps *caps,
                                 const DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct set_object_property_params *params = data;
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( params->iface );
    struct object_properties *properties;

    if (index == -1) return DIENUM_STOP;
    properties = impl->object_properties + index;

    switch (params->property)
    {
    case (DWORD_PTR)DIPROP_RANGE:
    {
        const DIPROPRANGE *value = (const DIPROPRANGE *)params->header;
        properties->range_min = value->lMin;
        properties->range_max = value->lMax;
        return DIENUM_CONTINUE;
    }
    case (DWORD_PTR)DIPROP_DEADZONE:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)params->header;
        properties->deadzone = value->dwData;
        return DIENUM_CONTINUE;
    }
    case (DWORD_PTR)DIPROP_SATURATION:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)params->header;
        properties->saturation = value->dwData;
        return DIENUM_CONTINUE;
    }
    case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)params->header;
        properties->calibration_mode = value->dwData;
        return DIENUM_CONTINUE;
    }
    case (DWORD_PTR)DIPROP_APPDATA:
    {
        DIPROPPOINTER *value = (DIPROPPOINTER *)params->header;
        properties->app_data = value->uData;
        return DIENUM_CONTINUE;
    }
    }

    return DIENUM_STOP;
}

static BOOL reset_object_value( struct dinput_device *impl, UINT index, struct hid_value_caps *caps,
                                const DIDEVICEOBJECTINSTANCEW *instance, void *context )
{
    struct object_properties *properties;
    LONG tmp = -1;

    if (index == -1) return DIENUM_STOP;
    properties = impl->object_properties + index;

    if (instance->dwType & DIDFT_AXIS)
    {
        LONG range_min = 0, range_max = 0xfffe;
        if (properties->range_min != DIPROPRANGE_NOMIN) range_min = properties->range_min;
        if (properties->range_max != DIPROPRANGE_NOMAX) range_max = properties->range_max;
        tmp = round( (range_min + range_max) / 2.0 );
    }

    *(LONG *)(impl->device_state + instance->dwOfs) = tmp;
    return DIENUM_CONTINUE;
}

static void reset_device_state( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DIPROPHEADER filter =
    {
        .dwHeaderSize = sizeof(DIPROPHEADER),
        .dwSize = sizeof(DIPROPHEADER),
        .dwHow = DIPH_DEVICE,
        .dwObj = 0,
    };

    impl->vtbl->enum_objects( iface, &filter, DIDFT_AXIS | DIDFT_POV, reset_object_value, impl );
}

static HRESULT dinput_device_set_property( IDirectInputDevice8W *iface, const GUID *guid,
                                           const DIPROPHEADER *header )
{
    struct set_object_property_params params = {.iface = iface, .header = header, .property = LOWORD( guid )};
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DIPROPHEADER filter;
    HRESULT hr;

    filter = *header;
    if (FAILED(hr = enum_object_filter_init( impl, &filter ))) return hr;
    if (FAILED(hr = check_property( impl, guid, header, TRUE ))) return hr;

    switch (LOWORD( guid ))
    {
    case (DWORD_PTR)DIPROP_RANGE:
    case (DWORD_PTR)DIPROP_DEADZONE:
    case (DWORD_PTR)DIPROP_SATURATION:
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_AXIS, set_object_property, &params );
        if (FAILED(hr)) return hr;
        reset_device_state( iface );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_CALIBRATIONMODE:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)header;
        if (value->dwData > DIPROPCALIBRATIONMODE_RAW) return DIERR_INVALIDPARAM;
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_AXIS, set_object_property, &params );
        if (FAILED(hr)) return hr;
        reset_device_state( iface );
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_AUTOCENTER:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)header;
        if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;
        impl->autocenter = value->dwData;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_FFGAIN:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)header;
        impl->device_gain = value->dwData;
        if (!is_exclusively_acquired( impl )) return DI_OK;
        if (!impl->vtbl->send_device_gain) return DI_OK;
        return impl->vtbl->send_device_gain( iface, impl->device_gain );
    }
    case (DWORD_PTR)DIPROP_AXISMODE:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)header;

        TRACE( "Axis mode: %s\n", value->dwData == DIPROPAXISMODE_ABS ? "absolute" : "relative" );

        impl->user_format.dwFlags &= ~DIDFT_AXIS;
        impl->user_format.dwFlags |= value->dwData == DIPROPAXISMODE_ABS ? DIDF_ABSAXIS : DIDF_RELAXIS;

        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_BUFFERSIZE:
    {
        const DIPROPDWORD *value = (const DIPROPDWORD *)header;

        TRACE( "buffersize %lu\n", value->dwData );

        impl->buffersize = value->dwData;
        impl->queue_len = min( impl->buffersize, 1024 );
        free( impl->data_queue );

        impl->data_queue = impl->queue_len ? malloc( impl->queue_len * sizeof(DIDEVICEOBJECTDATA) ) : NULL;
        impl->queue_head = impl->queue_tail = impl->overflow = 0;
        return DI_OK;
    }
    case (DWORD_PTR)DIPROP_APPDATA:
    {
        hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_ALL, set_object_property, &params );
        if (FAILED(hr)) return hr;
        return DI_OK;
    }
    default:
        FIXME( "Unknown property %s\n", debugstr_guid( guid ) );
        return DIERR_UNSUPPORTED;
    }

    return DI_OK;
}

static HRESULT WINAPI dinput_device_SetProperty( IDirectInputDevice8W *iface, const GUID *guid,
                                                 const DIPROPHEADER *header )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr;

    TRACE( "iface %p, guid %s, header %p\n", iface, debugstr_guid( guid ), header );

    if (!header) return DIERR_INVALIDPARAM;
    if (header->dwHeaderSize != sizeof(DIPROPHEADER)) return DIERR_INVALIDPARAM;
    if (!IS_DIPROP( guid )) return DI_OK;

    EnterCriticalSection( &impl->crit );
    hr = dinput_device_set_property( iface, guid, header );
    LeaveCriticalSection( &impl->crit );

    return hr;
}

static void dinput_device_set_username( struct dinput_device *impl, const DIPROPSTRING *value )
{
    struct DevicePlayer *device_player;
    BOOL found = FALSE;

    LIST_FOR_EACH_ENTRY( device_player, &impl->dinput->device_players, struct DevicePlayer, entry )
    {
        if (IsEqualGUID( &device_player->instance_guid, &impl->guid ))
        {
            found = TRUE;
            break;
        }
    }
    if (!found && (device_player = malloc( sizeof(struct DevicePlayer) )))
    {
        list_add_tail( &impl->dinput->device_players, &device_player->entry );
        device_player->instance_guid = impl->guid;
    }
    if (device_player)
        lstrcpynW( device_player->username, value->wsz, ARRAY_SIZE(device_player->username) );
}

static BOOL get_object_info( struct dinput_device *device, UINT index, struct hid_value_caps *caps,
                             const DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DIDEVICEOBJECTINSTANCEW *dest = data;
    DWORD size = dest->dwSize;

    memcpy( dest, instance, size );
    dest->dwSize = size;

    return DIENUM_STOP;
}

static HRESULT WINAPI dinput_device_GetObjectInfo( IDirectInputDevice8W *iface,
                                                   DIDEVICEOBJECTINSTANCEW *instance, DWORD obj, DWORD how )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = how,
        .dwObj = obj
    };
    HRESULT hr;

    TRACE( "iface %p, instance %p, obj %#lx, how %#lx.\n", iface, instance, obj, how );

    if (!instance) return E_POINTER;
    if (instance->dwSize != sizeof(DIDEVICEOBJECTINSTANCE_DX3W) && instance->dwSize != sizeof(DIDEVICEOBJECTINSTANCEW))
        return DIERR_INVALIDPARAM;
    if (how == DIPH_DEVICE) return DIERR_INVALIDPARAM;
    if (FAILED(hr = enum_object_filter_init( impl, &filter ))) return hr;

    hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_ALL, get_object_info, instance );
    if (FAILED(hr)) return hr;
    if (hr == DIENUM_CONTINUE) return DIERR_NOTFOUND;
    return DI_OK;
}

static HRESULT WINAPI dinput_device_GetDeviceState( IDirectInputDevice8W *iface, DWORD size, void *data )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DIDATAFORMAT *device_format = &impl->device_format, *user_format = &impl->user_format;
    DIOBJECTDATAFORMAT *device_obj, *user_obj;
    BYTE *user_state = data;
    DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = DIPH_DEVICE,
        .dwObj = 0,
    };
    HRESULT hr;

    TRACE( "iface %p, size %lu, data %p.\n", iface, size, data );

    if (!data) return DIERR_INVALIDPARAM;

    IDirectInputDevice2_Poll( iface );

    EnterCriticalSection( &impl->crit );
    if (impl->status == STATUS_UNPLUGGED)
        hr = DIERR_INPUTLOST;
    else if (impl->status != STATUS_ACQUIRED)
        hr = DIERR_NOTACQUIRED;
    else if (!user_format->rgodf)
        hr = DIERR_INVALIDPARAM;
    else if (size != user_format->dwDataSize)
        hr = DIERR_INVALIDPARAM;
    else
    {
        memset( user_state, 0, size );

        user_obj = user_format->rgodf + device_format->dwNumObjs;
        device_obj = device_format->rgodf + device_format->dwNumObjs;
        while (user_obj-- > user_format->rgodf && device_obj-- > device_format->rgodf)
        {
            if (user_obj->dwType & DIDFT_BUTTON)
                user_state[user_obj->dwOfs] = impl->device_state[device_obj->dwOfs];
        }

        /* reset optional POVs to their default */
        user_obj = user_format->rgodf + user_format->dwNumObjs;
        while (user_obj-- > user_format->rgodf + device_format->dwNumObjs)
            if (user_obj->dwType & DIDFT_POV) *(ULONG *)(user_state + user_obj->dwOfs) = 0xffffffff;

        user_obj = user_format->rgodf + device_format->dwNumObjs;
        device_obj = device_format->rgodf + device_format->dwNumObjs;
        while (user_obj-- > user_format->rgodf && device_obj-- > device_format->rgodf)
        {
            if (user_obj->dwType & (DIDFT_POV | DIDFT_AXIS))
                *(ULONG *)(user_state + user_obj->dwOfs) = *(ULONG *)(impl->device_state + device_obj->dwOfs);
            if (!(user_format->dwFlags & DIDF_ABSAXIS) && (device_obj->dwType & DIDFT_RELAXIS))
                *(ULONG *)(impl->device_state + device_obj->dwOfs) = 0;
        }

        hr = DI_OK;
    }
    LeaveCriticalSection( &impl->crit );

    return hr;
}

static HRESULT WINAPI dinput_device_GetDeviceData( IDirectInputDevice8W *iface, DWORD size, DIDEVICEOBJECTDATA *data,
                                                   DWORD *count, DWORD flags )
{
    struct dinput_device *This = impl_from_IDirectInputDevice8W( iface );
    HRESULT ret = DI_OK;
    int len;

    TRACE( "iface %p, size %lu, data %p, count %p, flags %#lx.\n", iface, size, data, count, flags );

    if (This->dinput->dwVersion == 0x0800 || size == sizeof(DIDEVICEOBJECTDATA_DX3))
    {
        if (!This->queue_len) return DIERR_NOTBUFFERED;
        if (This->status == STATUS_UNPLUGGED) return DIERR_INPUTLOST;
        if (This->status != STATUS_ACQUIRED) return DIERR_NOTACQUIRED;
    }

    if (!This->queue_len)
        return DI_OK;
    if (size < sizeof(DIDEVICEOBJECTDATA_DX3)) return DIERR_INVALIDPARAM;

    IDirectInputDevice2_Poll(iface);
    EnterCriticalSection(&This->crit);

    len = This->queue_head - This->queue_tail;
    if (len < 0) len += This->queue_len;

    if ((*count != INFINITE) && (len > *count)) len = *count;

    if (data)
    {
        int i;
        for (i = 0; i < len; i++)
        {
            int n = (This->queue_tail + i) % This->queue_len;
            memcpy( (char *)data + size * i, This->data_queue + n, size );
        }
    }
    *count = len;

    if (This->overflow && This->dinput->dwVersion == 0x0800)
        ret = DI_BUFFEROVERFLOW;

    if (!(flags & DIGDD_PEEK))
    {
        /* Advance reading position */
        This->queue_tail = (This->queue_tail + len) % This->queue_len;
        This->overflow = FALSE;
    }

    LeaveCriticalSection(&This->crit);

    TRACE( "Returning %lu events queued\n", *count );
    return ret;
}

static HRESULT WINAPI dinput_device_RunControlPanel( IDirectInputDevice8W *iface, HWND hwnd, DWORD flags )
{
    FIXME( "iface %p, hwnd %p, flags %#lx stub!\n", iface, hwnd, flags );
    return DI_OK;
}

static HRESULT WINAPI dinput_device_Initialize( IDirectInputDevice8W *iface, HINSTANCE instance,
                                                DWORD version, const GUID *guid )
{
    FIXME( "iface %p, instance %p, version %#lx, guid %s stub!\n", iface, instance, version,
           debugstr_guid( guid ) );
    return DI_OK;
}

static HRESULT WINAPI dinput_device_CreateEffect( IDirectInputDevice8W *iface, const GUID *guid,
                                                  const DIEFFECT *params, IDirectInputEffect **out,
                                                  IUnknown *outer )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DWORD flags;
    HRESULT hr;

    TRACE( "iface %p, guid %s, params %p, out %p, outer %p\n", iface, debugstr_guid( guid ),
           params, out, outer );

    if (!out) return E_POINTER;
    *out = NULL;

    if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;
    if (!impl->vtbl->create_effect) return DIERR_UNSUPPORTED;
    if (FAILED(hr = impl->vtbl->create_effect( iface, out ))) return hr;

    hr = IDirectInputEffect_Initialize( *out, DINPUT_instance, impl->dinput->dwVersion, guid );
    if (FAILED(hr)) goto failed;
    if (!params) return DI_OK;

    flags = params->dwSize == sizeof(DIEFFECT_DX6) ? DIEP_ALLPARAMS : DIEP_ALLPARAMS_DX5;
    if (!is_exclusively_acquired( impl )) flags |= DIEP_NODOWNLOAD;
    hr = IDirectInputEffect_SetParameters( *out, params, flags );
    if (FAILED(hr)) goto failed;
    return DI_OK;

failed:
    IDirectInputEffect_Release( *out );
    *out = NULL;
    return hr;
}

static HRESULT WINAPI dinput_device_EnumEffects( IDirectInputDevice8W *iface, LPDIENUMEFFECTSCALLBACKW callback,
                                                 void *context, DWORD type )
{
    DIEFFECTINFOW info = {.dwSize = sizeof(info)};
    HRESULT hr;

    TRACE( "iface %p, callback %p, context %p, type %#lx.\n", iface, callback, context, type );

    if (!callback) return DIERR_INVALIDPARAM;

    type = DIEFT_GETTYPE( type );

    if (type == DIEFT_ALL || type == DIEFT_CONSTANTFORCE)
    {
        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_ConstantForce );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;
    }

    if (type == DIEFT_ALL || type == DIEFT_RAMPFORCE)
    {
        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_RampForce );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;
    }

    if (type == DIEFT_ALL || type == DIEFT_PERIODIC)
    {
        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Square );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Sine );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Triangle );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_SawtoothUp );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_SawtoothDown );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;
    }

    if (type == DIEFT_ALL || type == DIEFT_CONDITION)
    {
        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Spring );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Damper );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Inertia );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;

        hr = IDirectInputDevice8_GetEffectInfo( iface, &info, &GUID_Friction );
        if (FAILED(hr) && hr != DIERR_DEVICENOTREG) return hr;
        if (hr == DI_OK && callback( &info, context ) == DIENUM_STOP) return DI_OK;
    }

    return DI_OK;
}

static HRESULT WINAPI dinput_device_GetEffectInfo( IDirectInputDevice8W *iface, DIEFFECTINFOW *info,
                                                   const GUID *guid )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, info %p, guid %s.\n", iface, info, debugstr_guid( guid ) );

    if (!info) return E_POINTER;
    if (info->dwSize != sizeof(DIEFFECTINFOW)) return DIERR_INVALIDPARAM;
    if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_DEVICENOTREG;
    if (!impl->vtbl->get_effect_info) return DIERR_UNSUPPORTED;
    return impl->vtbl->get_effect_info( iface, info, guid );
}

static HRESULT WINAPI dinput_device_GetForceFeedbackState( IDirectInputDevice8W *iface, DWORD *out )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr = DI_OK;

    TRACE( "iface %p, out %p.\n", iface, out );

    if (!out) return E_POINTER;
    *out = 0;

    if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;

    EnterCriticalSection( &impl->crit );
    if (!is_exclusively_acquired( impl )) hr = DIERR_NOTEXCLUSIVEACQUIRED;
    else *out = impl->force_feedback_state;
    LeaveCriticalSection( &impl->crit );

    return hr;
}

static HRESULT WINAPI dinput_device_SendForceFeedbackCommand( IDirectInputDevice8W *iface, DWORD command )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr;

    TRACE( "iface %p, command %#lx.\n", iface, command );

    switch (command)
    {
    case DISFFC_RESET: break;
    case DISFFC_STOPALL: break;
    case DISFFC_PAUSE: break;
    case DISFFC_CONTINUE: break;
    case DISFFC_SETACTUATORSON: break;
    case DISFFC_SETACTUATORSOFF: break;
    default: return DIERR_INVALIDPARAM;
    }

    if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DIERR_UNSUPPORTED;
    if (!impl->vtbl->send_force_feedback_command) return DIERR_UNSUPPORTED;

    EnterCriticalSection( &impl->crit );
    if (!is_exclusively_acquired( impl )) hr = DIERR_NOTEXCLUSIVEACQUIRED;
    else hr = impl->vtbl->send_force_feedback_command( iface, command, FALSE );
    LeaveCriticalSection( &impl->crit );

    return hr;
}

static HRESULT WINAPI dinput_device_EnumCreatedEffectObjects( IDirectInputDevice8W *iface,
                                                              LPDIENUMCREATEDEFFECTOBJECTSCALLBACK callback,
                                                              void *context, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );

    TRACE( "iface %p, callback %p, context %p, flags %#lx.\n", iface, callback, context, flags );

    if (!callback) return DIERR_INVALIDPARAM;
    if (flags) return DIERR_INVALIDPARAM;
    if (!(impl->caps.dwFlags & DIDC_FORCEFEEDBACK)) return DI_OK;
    if (!impl->vtbl->enum_created_effect_objects) return DIERR_UNSUPPORTED;

    return impl->vtbl->enum_created_effect_objects( iface, callback, context, flags );
}

static HRESULT WINAPI dinput_device_Escape( IDirectInputDevice8W *iface, DIEFFESCAPE *escape )
{
    FIXME( "iface %p, escape %p stub!\n", iface, escape );
    return DI_OK;
}

static HRESULT WINAPI dinput_device_Poll( IDirectInputDevice8W *iface )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    HRESULT hr = DI_NOEFFECT;

    EnterCriticalSection( &impl->crit );
    if (impl->status == STATUS_UNPLUGGED) hr = DIERR_INPUTLOST;
    else if (impl->status != STATUS_ACQUIRED) hr = DIERR_NOTACQUIRED;
    LeaveCriticalSection( &impl->crit );
    if (FAILED(hr)) return hr;

    if (impl->vtbl->poll) return impl->vtbl->poll( iface );
    return hr;
}

static HRESULT WINAPI dinput_device_SendDeviceData( IDirectInputDevice8W *iface, DWORD size,
                                                    const DIDEVICEOBJECTDATA *data, DWORD *count, DWORD flags )
{
    FIXME( "iface %p, size %lu, data %p, count %p, flags %#lx stub!\n", iface, size, data, count, flags );
    return DI_OK;
}

static HRESULT WINAPI dinput_device_EnumEffectsInFile( IDirectInputDevice8W *iface, const WCHAR *filename,
                                                       LPDIENUMEFFECTSINFILECALLBACK callback,
                                                       void *context, DWORD flags )
{
    FIXME( "iface %p, filename %s, callback %p, context %p, flags %#lx stub!\n", iface,
           debugstr_w(filename), callback, context, flags );
    return DI_OK;
}

static HRESULT WINAPI dinput_device_WriteEffectToFile( IDirectInputDevice8W *iface, const WCHAR *filename,
                                                       DWORD count, DIFILEEFFECT *effects, DWORD flags )
{
    FIXME( "iface %p, filename %s, count %lu, effects %p, flags %#lx stub!\n", iface,
           debugstr_w(filename), count, effects, flags );
    return DI_OK;
}

BOOL device_object_matches_semantic( const DIDEVICEINSTANCEW *instance, const DIOBJECTDATAFORMAT *object,
                                     DWORD semantic, BOOL exact )
{
    DWORD value = semantic & 0xff, axis = (semantic >> 15) & 15, type;

    switch (semantic & 0x700)
    {
    case 0x200: type = DIDFT_ABSAXIS; break;
    case 0x300: type = DIDFT_RELAXIS; break;
    case 0x400: type = DIDFT_BUTTON; break;
    case 0x600: type = DIDFT_POV; break;
    default: return FALSE;
    }

    if (!(DIDFT_GETTYPE( object->dwType ) & type)) return FALSE;
    switch (semantic & 0xff000000)
    {
    case 0x81000000: return (instance->dwDevType & 0xf) == DIDEVTYPE_KEYBOARD && object->dwOfs == value;
    case 0x82000000: return (instance->dwDevType & 0xf) == DIDEVTYPE_MOUSE && object->dwOfs == value;
    case 0x83000000: return FALSE;

    default:
        if ((instance->dwDevType & 0xf) == DIDEVTYPE_KEYBOARD) return FALSE;
        if ((instance->dwDevType & 0xf) == DIDEVTYPE_MOUSE) return FALSE;
        /* fallthrough */
    case 0xff000000:
        if (axis && (axis - 1) != DIDFT_GETINSTANCE( object->dwType )) return FALSE;
        return !exact || !value || value == DIDFT_GETINSTANCE( object->dwType ) + 1;
    }
}

static HRESULT WINAPI dinput_device_BuildActionMap( IDirectInputDevice8W *iface, DIACTIONFORMATW *format,
                                                    const WCHAR *username, DWORD flags )
{
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DIOBJECTDATAFORMAT *object, *object_end;
    DIACTIONW *action, *action_end;
    DWORD i, username_len = MAX_PATH;
    WCHAR username_buf[MAX_PATH];
    BOOL *mapped;

    TRACE( "iface %p, format %p, username %s, flags %#lx\n", iface, format,
           debugstr_w(username), flags );

    if (!format) return DIERR_INVALIDPARAM;
    if (flags != DIDBAM_DEFAULT && flags != DIDBAM_PRESERVE &&
        flags != DIDBAM_INITIALIZE && flags != DIDBAM_HWDEFAULTS)
        return DIERR_INVALIDPARAM;
    if (format->dwNumActions * 4 != format->dwDataSize)
        return DIERR_INVALIDPARAM;

    TRACE( "format guid %s, genre %#lx, name %s\n", debugstr_guid(&format->guidActionMap),
           format->dwGenre, debugstr_w(format->tszActionMap) );
    for (i = 0; i < format->dwNumActions; i++)
    {
        DIACTIONW *action = format->rgoAction + i;
        TRACE( "  %lu: app_data %#Ix, semantic %#lx, flags %#lx, instance %s, obj_id %#lx, how %#lx, name %s\n",
               i, action->uAppData, action->dwSemantic, action->dwFlags, debugstr_guid(&action->guidInstance),
               action->dwObjID, action->dwHow, debugstr_w(action->lptszActionName) );
    }

    action_end = format->rgoAction + format->dwNumActions;
    for (action = format->rgoAction; action < action_end; action++)
    {
        if (!action->dwSemantic) return DIERR_INVALIDPARAM;
        if (flags == DIDBAM_PRESERVE && !IsEqualCLSID( &action->guidInstance, &GUID_NULL ) &&
            !IsEqualCLSID( &action->guidInstance, &impl->guid )) continue;
        if (action->dwFlags & DIA_APPMAPPED) action->dwHow = DIAH_APPREQUESTED;
        else action->dwHow = 0;
        if (action->dwHow == DIAH_APPREQUESTED || action->dwHow == DIAH_USERCONFIG) continue;
        if ((action->dwSemantic & 0xf0000000) == 0x80000000) action->dwFlags &= ~DIA_APPNOMAP;
        if (!(action->dwFlags & DIA_APPNOMAP)) action->guidInstance = GUID_NULL;
    }

    /* Unless asked the contrary by these flags, try to load a previous mapping */
    if (!(flags & DIDBAM_HWDEFAULTS))
    {
        /* Retrieve logged user name if necessary */
        if (username == NULL) GetUserNameW( username_buf, &username_len );
        else lstrcpynW( username_buf, username, MAX_PATH );
        load_mapping_settings( impl, format, username_buf );
    }

    if (!(mapped = calloc( impl->device_format.dwNumObjs, sizeof(*mapped) ))) return DIERR_OUTOFMEMORY;

    /* check already mapped objects */
    action_end = format->rgoAction + format->dwNumActions;
    for (action = format->rgoAction; action < action_end; action++)
    {
        if (!action->dwHow || !action->dwObjID) continue;
        if (!IsEqualGUID(&action->guidInstance, &impl->guid)) continue;

        object_end = impl->device_format.rgodf + impl->device_format.dwNumObjs;
        for (object = impl->device_format.rgodf; object < object_end; object++)
        {
            if (action->dwObjID != object->dwType) continue;
            mapped[object - impl->device_format.rgodf] = TRUE;
            break;
        }
    }

    /* map any unmapped priority 1 objects */
    action_end = format->rgoAction + format->dwNumActions;
    for (action = format->rgoAction; action < action_end; action++)
    {
        if (action->dwHow || (action->dwFlags & DIA_APPNOMAP)) continue; /* already mapped */
        if (action->dwSemantic & 0x4000) continue; /* priority 1 */

        object_end = impl->device_format.rgodf + impl->device_format.dwNumObjs;
        for (object = impl->device_format.rgodf; object < object_end; object++)
        {
            if (mapped[object - impl->device_format.rgodf]) continue;
            if (!device_object_matches_semantic( &impl->instance, object, action->dwSemantic, TRUE )) continue;
            if ((action->dwFlags & DIA_FORCEFEEDBACK) && !(object->dwType & DIDFT_FFACTUATOR)) continue;
            action->dwObjID = object->dwType;
            action->guidInstance = impl->guid;
            action->dwHow = DIAH_DEFAULT;
            mapped[object - impl->device_format.rgodf] = TRUE;
            break;
        }
    }

    /* map any unmapped priority 2 objects */
    for (action = format->rgoAction; action < action_end; action++)
    {
        if (action->dwHow || (action->dwFlags & DIA_APPNOMAP)) continue; /* already mapped */
        if (!(action->dwSemantic & 0x4000)) continue; /* priority 2 */

        object_end = impl->device_format.rgodf + impl->device_format.dwNumObjs;
        for (object = impl->device_format.rgodf; object < object_end; object++)
        {
            if (mapped[object - impl->device_format.rgodf]) continue;
            if (!device_object_matches_semantic( &impl->instance, object, action->dwSemantic, FALSE )) continue;
            if ((action->dwFlags & DIA_FORCEFEEDBACK) && !(object->dwType & DIDFT_FFACTUATOR)) continue;
            action->dwObjID = object->dwType;
            action->guidInstance = impl->guid;
            action->dwHow = DIAH_DEFAULT;
            mapped[object - impl->device_format.rgodf] = TRUE;
            break;
        }
    }

    for (i = 0; i < impl->device_format.dwNumObjs; ++i) if (mapped[i]) break;
    free( mapped );

    if (i == impl->device_format.dwNumObjs) return DI_NOEFFECT;
    return DI_OK;
}

static BOOL init_object_app_data( struct dinput_device *device, UINT index, struct hid_value_caps *caps,
                                  const DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    struct object_properties *properties;
    const DIACTIONFORMATW *format = data;
    const DIACTIONW *action = format->rgoAction + format->dwNumActions;

    if (index == -1) return DIENUM_STOP;
    if (instance->wUsagePage == HID_USAGE_PAGE_PID) return DIENUM_CONTINUE;

    properties = device->object_properties + index;
    properties->app_data = 0;

    while (action-- > format->rgoAction)
    {
        if (action->dwObjID != instance->dwType) continue;
        properties->app_data = action->uAppData;
        break;
    }

    return DIENUM_CONTINUE;
}

static HRESULT WINAPI dinput_device_SetActionMap( IDirectInputDevice8W *iface, DIACTIONFORMATW *format,
                                                  const WCHAR *username, DWORD flags )
{
    static const DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = DIPH_DEVICE,
    };
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DIDATAFORMAT data_format =
    {
        .dwSize = sizeof(DIDATAFORMAT),
        .dwObjSize = sizeof(DIOBJECTDATAFORMAT),
        .dwFlags = DIDF_RELAXIS,
    };
    DIPROPDWORD prop_buffer =
    {
        .diph =
        {
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwSize = sizeof(DIPROPDWORD),
            .dwHow = DIPH_DEVICE,
        }
    };
    DIPROPRANGE prop_range =
    {
        .diph =
        {
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwSize = sizeof(DIPROPRANGE),
            .dwHow = DIPH_DEVICE,
        }
    };
    DIPROPSTRING prop_username =
    {
        .diph =
        {
            .dwHeaderSize = sizeof(DIPROPHEADER),
            .dwSize = sizeof(DIPROPSTRING),
            .dwHow = DIPH_DEVICE,
        }
    };
    WCHAR username_buf[MAX_PATH];
    DWORD username_len = MAX_PATH;
    unsigned int offset = 0;
    int i, index;
    HRESULT hr;

    TRACE( "iface %p, format %p, username %s, flags %#lx\n", iface, format,
           debugstr_w(username), flags );

    if (!format) return DIERR_INVALIDPARAM;
    if (flags != DIDSAM_DEFAULT && flags != DIDSAM_FORCESAVE && flags != DIDSAM_NOUSER) return DIERR_INVALIDPARAM;

    TRACE( "format guid %s, genre %#lx, name %s\n", debugstr_guid(&format->guidActionMap),
           format->dwGenre, debugstr_w(format->tszActionMap) );
    for (i = 0; i < format->dwNumActions; i++)
    {
        DIACTIONW *action = format->rgoAction + i;
        TRACE( "  %u: app_data %#Ix, semantic %#lx, flags %#lx, instance %s, obj_id %#lx, how %#lx, name %s\n",
               i, action->uAppData, action->dwSemantic, action->dwFlags, debugstr_guid(&action->guidInstance),
               action->dwObjID, action->dwHow, debugstr_w(action->lptszActionName) );
    }

    if (!(data_format.rgodf = malloc( sizeof(DIOBJECTDATAFORMAT) * format->dwNumActions ))) return DIERR_OUTOFMEMORY;
    data_format.dwDataSize = format->dwDataSize;

    for (i = 0; i < format->dwNumActions; i++, offset += sizeof(ULONG))
    {
        if (format->rgoAction[i].dwFlags & DIA_APPNOMAP) continue;
        if (!IsEqualGUID( &impl->guid, &format->rgoAction[i].guidInstance )) continue;
        if ((index = dinput_device_object_index_from_id( iface, format->rgoAction[i].dwObjID )) < 0) continue;

        data_format.rgodf[data_format.dwNumObjs] = impl->device_format.rgodf[index];
        data_format.rgodf[data_format.dwNumObjs].dwOfs = offset;
        data_format.dwNumObjs++;
    }

    EnterCriticalSection( &impl->crit );

    if (FAILED(hr = IDirectInputDevice8_SetDataFormat( iface, &data_format )))
        WARN( "Failed to set data format from action map, hr %#lx\n", hr );
    else
    {
        if (FAILED(impl->vtbl->enum_objects( iface, &filter, DIDFT_ALL, init_object_app_data, format )))
            WARN( "Failed to initialize action map app data\n" );

        if (format->lAxisMin != format->lAxisMax)
        {
            prop_range.lMin = format->lAxisMin;
            prop_range.lMax = format->lAxisMax;
            IDirectInputDevice8_SetProperty( iface, DIPROP_RANGE, &prop_range.diph );
        }

        prop_buffer.dwData = format->dwBufferSize;
        IDirectInputDevice8_SetProperty( iface, DIPROP_BUFFERSIZE, &prop_buffer.diph );

        if (username == NULL) GetUserNameW( username_buf, &username_len );
        else lstrcpynW( username_buf, username, MAX_PATH );

        if (flags & DIDSAM_NOUSER) prop_username.wsz[0] = '\0';
        else lstrcpynW( prop_username.wsz, username_buf, ARRAY_SIZE(prop_username.wsz) );
        dinput_device_set_username( impl, &prop_username );

        save_mapping_settings( iface, format, username_buf );
    }

    LeaveCriticalSection( &impl->crit );

    free( data_format.rgodf );

    if (FAILED(hr)) return hr;
    if (flags == DIDSAM_FORCESAVE) return DI_SETTINGSNOTSAVED;
    if (!data_format.dwNumObjs) return DI_NOEFFECT;
    return hr;
}

static HRESULT WINAPI dinput_device_GetImageInfo( IDirectInputDevice8W *iface, DIDEVICEIMAGEINFOHEADERW *header )
{
    FIXME( "iface %p, header %p stub!\n", iface, header );
    return DI_OK;
}

extern const IDirectInputDevice8AVtbl dinput_device_a_vtbl;
static const IDirectInputDevice8WVtbl dinput_device_w_vtbl =
{
    /*** IUnknown methods ***/
    dinput_device_QueryInterface,
    dinput_device_AddRef,
    dinput_device_Release,
    /*** IDirectInputDevice methods ***/
    dinput_device_GetCapabilities,
    dinput_device_EnumObjects,
    dinput_device_GetProperty,
    dinput_device_SetProperty,
    dinput_device_Acquire,
    dinput_device_Unacquire,
    dinput_device_GetDeviceState,
    dinput_device_GetDeviceData,
    dinput_device_SetDataFormat,
    dinput_device_SetEventNotification,
    dinput_device_SetCooperativeLevel,
    dinput_device_GetObjectInfo,
    dinput_device_GetDeviceInfo,
    dinput_device_RunControlPanel,
    dinput_device_Initialize,
    /*** IDirectInputDevice2 methods ***/
    dinput_device_CreateEffect,
    dinput_device_EnumEffects,
    dinput_device_GetEffectInfo,
    dinput_device_GetForceFeedbackState,
    dinput_device_SendForceFeedbackCommand,
    dinput_device_EnumCreatedEffectObjects,
    dinput_device_Escape,
    dinput_device_Poll,
    dinput_device_SendDeviceData,
    /*** IDirectInputDevice7 methods ***/
    dinput_device_EnumEffectsInFile,
    dinput_device_WriteEffectToFile,
    /*** IDirectInputDevice8 methods ***/
    dinput_device_BuildActionMap,
    dinput_device_SetActionMap,
    dinput_device_GetImageInfo,
};

void dinput_device_init( struct dinput_device *device, const struct dinput_device_vtbl *vtbl,
                         const GUID *guid, struct dinput *dinput )
{
    device->IDirectInputDevice8A_iface.lpVtbl = &dinput_device_a_vtbl;
    device->IDirectInputDevice8W_iface.lpVtbl = &dinput_device_w_vtbl;
    device->internal_ref = 1;
    device->ref = 1;
    device->guid = *guid;
    device->instance.dwSize = sizeof(DIDEVICEINSTANCEW);
    device->caps.dwSize = sizeof(DIDEVCAPS);
    device->caps.dwFlags = DIDC_ATTACHED | DIDC_EMULATED;
    device->device_gain = 10000;
    device->autocenter = DIPROPAUTOCENTER_ON;
    device->force_feedback_state = DIGFFS_STOPPED | DIGFFS_EMPTY;
    InitializeCriticalSectionEx( &device->crit, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    dinput_internal_addref( (device->dinput = dinput) );
    device->vtbl = vtbl;

    input_thread_add_user();
}

static const GUID *object_instance_guid( const DIDEVICEOBJECTINSTANCEW *instance )
{
    if (IsEqualGUID( &instance->guidType, &GUID_XAxis )) return &GUID_XAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_YAxis )) return &GUID_YAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_ZAxis )) return &GUID_ZAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_RxAxis )) return &GUID_RxAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_RyAxis )) return &GUID_RyAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_RzAxis )) return &GUID_RzAxis;
    if (IsEqualGUID( &instance->guidType, &GUID_Slider )) return &GUID_Slider;
    if (IsEqualGUID( &instance->guidType, &GUID_Button )) return &GUID_Button;
    if (IsEqualGUID( &instance->guidType, &GUID_Key )) return &GUID_Key;
    if (IsEqualGUID( &instance->guidType, &GUID_POV )) return &GUID_POV;
    return &GUID_Unknown;
}

static BOOL enum_objects_count( struct dinput_device *impl, UINT index, struct hid_value_caps *caps,
                                const DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    DIDATAFORMAT *format = &impl->device_format;

    if (index == -1) return DIENUM_STOP;
    format->dwNumObjs++;
    if (instance->wUsagePage == HID_USAGE_PAGE_PID) return DIENUM_CONTINUE;

    format->dwDataSize = max( format->dwDataSize, instance->dwOfs + sizeof(LONG) );
    if (instance->dwType & DIDFT_BUTTON) impl->caps.dwButtons++;
    if (instance->dwType & DIDFT_AXIS) impl->caps.dwAxes++;
    if (instance->dwType & DIDFT_POV) impl->caps.dwPOVs++;
    if (instance->dwType & (DIDFT_BUTTON|DIDFT_AXIS|DIDFT_POV))
    {
        if (!impl->device_state_report_id)
            impl->device_state_report_id = instance->wReportId;
        else if (impl->device_state_report_id != instance->wReportId)
            FIXME( "multiple device state reports found!\n" );
    }

    return DIENUM_CONTINUE;
}

static BOOL enum_objects_init( struct dinput_device *impl, UINT index, struct hid_value_caps *caps,
                               const DIDEVICEOBJECTINSTANCEW *instance, void *data )
{
    static const struct object_properties default_properties =
    {
        .range_min = DIPROPRANGE_NOMIN,
        .range_max = DIPROPRANGE_NOMAX,
        .granularity = 1,
        .app_data = -1,
    };
    DIDATAFORMAT *format = &impl->device_format;
    DIOBJECTDATAFORMAT *object_format;

    if (index == -1) return DIENUM_STOP;
    if (instance->wUsagePage == HID_USAGE_PAGE_PID) return DIENUM_CONTINUE;

    object_format = format->rgodf + index;
    object_format->pguid = object_instance_guid( instance );
    object_format->dwOfs = instance->dwOfs;
    object_format->dwType = instance->dwType;
    object_format->dwFlags = instance->dwFlags;

    impl->object_properties[index] = default_properties;
    if (instance->dwType & (DIDFT_AXIS | DIDFT_POV)) reset_object_value( impl, index, caps, instance, NULL );

    return DIENUM_CONTINUE;
}

HRESULT dinput_device_init_device_format( IDirectInputDevice8W *iface )
{
    static const DIPROPHEADER filter =
    {
        .dwSize = sizeof(filter),
        .dwHeaderSize = sizeof(filter),
        .dwHow = DIPH_DEVICE,
    };
    struct dinput_device *impl = impl_from_IDirectInputDevice8W( iface );
    DIDATAFORMAT *format = &impl->device_format;
    HRESULT hr;
    ULONG i;

    hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_ALL, enum_objects_count, NULL );
    if (FAILED(hr)) return hr;

    if (format->dwDataSize > DEVICE_STATE_MAX_SIZE)
    {
        FIXME( "unable to create device, state is too large\n" );
        return DIERR_OUTOFMEMORY;
    }

    if (!(impl->object_properties = calloc( format->dwNumObjs, sizeof(*impl->object_properties) ))) return DIERR_OUTOFMEMORY;
    if (!(format->rgodf = calloc( format->dwNumObjs, sizeof(*format->rgodf) ))) return DIERR_OUTOFMEMORY;

    format->dwSize = sizeof(*format);
    format->dwObjSize = sizeof(*format->rgodf);
    format->dwFlags = DIDF_ABSAXIS;

    hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_AXIS, enum_objects_init, NULL );
    if (FAILED(hr)) return hr;
    hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_POV, enum_objects_init, NULL );
    if (FAILED(hr)) return hr;
    hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_BUTTON, enum_objects_init, NULL );
    if (FAILED(hr)) return hr;
    hr = impl->vtbl->enum_objects( iface, &filter, DIDFT_NODATA, enum_objects_init, NULL );
    if (FAILED(hr)) return hr;

    if (TRACE_ON( dinput ))
    {
        TRACE( "device format %s\n", debugstr_didataformat( format ) );
        for (i = 0; i < format->dwNumObjs; ++i) TRACE( "  %lu: object %s\n", i, debugstr_diobjectdataformat( format->rgodf + i ) );
    }

    return DI_OK;
}
