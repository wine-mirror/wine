/*
 * Win32_Service methods implementation
 *
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wbemcli.h"
#include "winsvc.h"

#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

static UINT map_error( DWORD error )
{
    switch (error)
    {
    case ERROR_SUCCESS:       return 0;
    case ERROR_ACCESS_DENIED: return 2;
    case ERROR_DEPENDENT_SERVICES_RUNNING: return 3;
    case ERROR_INVALID_SERVICE_CONTROL:    return 4;
    case ERROR_SERVICE_CANNOT_ACCEPT_CTRL: return 5;
    case ERROR_SERVICE_NOT_ACTIVE:         return 6;
    case ERROR_SERVICE_REQUEST_TIMEOUT:    return 7;
    case ERROR_SERVICE_ALREADY_RUNNING:    return 10;
    default:
        WARN( "unknown error %lu\n", error );
        break;
    }
    return 8;
}

static HRESULT control_service( const WCHAR *name, DWORD control, VARIANT *retval )
{
    SC_HANDLE manager, service = NULL;
    SERVICE_STATUS status;
    UINT error = 0;

    if (!(manager = OpenSCManagerW( NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE )))
    {
        error = map_error( GetLastError() );
        goto done;
    }
    if (!(service = OpenServiceW( manager, name, SERVICE_STOP|SERVICE_START|SERVICE_PAUSE_CONTINUE )))
    {
        error = map_error( GetLastError() );
        goto done;
    }
    if (!ControlService( service, control, &status )) error = map_error( GetLastError() );
    CloseServiceHandle( service );

done:
    set_variant( VT_UI4, error, NULL, retval );
    if (manager) CloseServiceHandle( manager );
    return S_OK;
}

HRESULT service_pause_service( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT name, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = IWbemClassObject_Get( obj, L"Name", 0, &name, NULL, NULL );
    if (hr != S_OK) return hr;

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"Win32_Service", L"PauseService", PARAM_OUT, &sig );
    if (hr != S_OK)
    {
        VariantClear( &name );
        return hr;
    }
    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            VariantClear( &name );
            IWbemClassObject_Release( sig );
            return hr;
        }
    }
    hr = control_service( V_BSTR(&name), SERVICE_CONTROL_PAUSE, &retval );
    if (hr != S_OK) goto done;

    if (out_params)
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );

done:
    VariantClear( &name );
    IWbemClassObject_Release( sig );
    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );
    return hr;
}

HRESULT service_resume_service( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT name, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = IWbemClassObject_Get( obj, L"Name", 0, &name, NULL, NULL );
    if (hr != S_OK) return hr;

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"Win32_Service", L"ResumeService", PARAM_OUT, &sig );
    if (hr != S_OK)
    {
        VariantClear( &name );
        return hr;
    }
    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            VariantClear( &name );
            IWbemClassObject_Release( sig );
            return hr;
        }
    }
    hr = control_service( V_BSTR(&name), SERVICE_CONTROL_CONTINUE, &retval );
    if (hr != S_OK) goto done;

    if (out_params)
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );

done:
    VariantClear( &name );
    IWbemClassObject_Release( sig );
    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );
    return hr;
}

static HRESULT start_service( const WCHAR *name, VARIANT *retval )
{
    SC_HANDLE manager, service = NULL;
    UINT error = 0;

    if (!(manager = OpenSCManagerW( NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE )))
    {
        error = map_error( GetLastError() );
        goto done;
    }
    if (!(service = OpenServiceW( manager, name, SERVICE_START )))
    {
        error = map_error( GetLastError() );
        goto done;
    }
    if (!StartServiceW( service, 0, NULL )) error = map_error( GetLastError() );
    CloseServiceHandle( service );

done:
    set_variant( VT_UI4, error, NULL, retval );
    if (manager) CloseServiceHandle( manager );
    return S_OK;
}

HRESULT service_start_service( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT name, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = IWbemClassObject_Get( obj, L"Name", 0, &name, NULL, NULL );
    if (hr != S_OK) return hr;

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"Win32_Service", L"StartService", PARAM_OUT, &sig );
    if (hr != S_OK)
    {
        VariantClear( &name );
        return hr;
    }
    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            VariantClear( &name );
            IWbemClassObject_Release( sig );
            return hr;
        }
    }
    hr = start_service( V_BSTR(&name), &retval );
    if (hr != S_OK) goto done;

    if (out_params)
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );

done:
    VariantClear( &name );
    IWbemClassObject_Release( sig );
    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );
    return hr;
}

HRESULT service_stop_service( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT name, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = IWbemClassObject_Get( obj, L"Name", 0, &name, NULL, NULL );
    if (hr != S_OK) return hr;

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"Win32_Service", L"StopService", PARAM_OUT, &sig );
    if (hr != S_OK)
    {
        VariantClear( &name );
        return hr;
    }
    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            VariantClear( &name );
            IWbemClassObject_Release( sig );
            return hr;
        }
    }
    hr = control_service( V_BSTR(&name), SERVICE_CONTROL_STOP, &retval );
    if (hr != S_OK) goto done;

    if (out_params)
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );

done:
    VariantClear( &name );
    IWbemClassObject_Release( sig );
    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );
    return hr;
}

static DWORD map_start_mode( const WCHAR *start_mode )
{
    if (!wcsicmp( start_mode, L"Boot" ))     return SERVICE_BOOT_START;
    if (!wcsicmp( start_mode, L"System" ))   return SERVICE_SYSTEM_START;
    if (!wcsicmp( start_mode, L"Auto" ))     return SERVICE_AUTO_START;
    if (!wcsicmp( start_mode, L"Manual" ))   return SERVICE_DEMAND_START;
    if (!wcsicmp( start_mode, L"Disabled" )) return SERVICE_DISABLED;
    WARN( "unknown mode %s\n", debugstr_w(start_mode) );
    return ~0u;
}

static HRESULT change_start_mode( const WCHAR *name, const WCHAR *start_mode, VARIANT *retval )
{
    SC_HANDLE manager, service = NULL;
    DWORD start_type = map_start_mode( start_mode );
    UINT error = 0;

    if (start_type == ~0u) return E_INVALIDARG;
    if (!(manager = OpenSCManagerW( NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE )))
    {
        error = map_error( GetLastError() );
        goto done;
    }
    if (!(service = OpenServiceW( manager, name, SERVICE_CHANGE_CONFIG )))
    {
        error = map_error( GetLastError() );
        goto done;
    }
    if (!ChangeServiceConfigW( service, SERVICE_NO_CHANGE, start_type, SERVICE_NO_CHANGE, NULL, NULL, NULL,
                               NULL, NULL, NULL, NULL ))
    {
        error = map_error( GetLastError() );
    }

done:
    set_variant( VT_UI4, error, NULL, retval );
    if (service) CloseServiceHandle( service );
    if (manager) CloseServiceHandle( manager );
    return S_OK;
}

HRESULT service_change_start_mode( IWbemClassObject *obj, IWbemContext *context, IWbemClassObject *in, IWbemClassObject **out )
{
    VARIANT name, start_mode, retval;
    IWbemClassObject *sig, *out_params = NULL;
    HRESULT hr;

    TRACE("%p, %p, %p, %p\n", obj, context, in, out);

    hr = IWbemClassObject_Get( obj, L"Name", 0, &name, NULL, NULL );
    if (hr != S_OK) return hr;

    hr = IWbemClassObject_Get( in, L"StartMode", 0, &start_mode, NULL, NULL );
    if (hr != S_OK)
    {
        VariantClear( &name );
        return hr;
    }

    hr = create_signature( WBEMPROX_NAMESPACE_CIMV2, L"Win32_Service", L"ChangeStartMode", PARAM_OUT, &sig );
    if (hr != S_OK)
    {
        VariantClear( &name );
        return hr;
    }
    if (out)
    {
        hr = IWbemClassObject_SpawnInstance( sig, 0, &out_params );
        if (hr != S_OK)
        {
            VariantClear( &name );
            IWbemClassObject_Release( sig );
            return hr;
        }
    }
    hr = change_start_mode( V_BSTR(&name), V_BSTR(&start_mode), &retval );
    if (hr != S_OK) goto done;

    if (out_params)
        hr = IWbemClassObject_Put( out_params, L"ReturnValue", 0, &retval, CIM_UINT32 );

done:
    VariantClear( &name );
    VariantClear( &start_mode );
    IWbemClassObject_Release( sig );
    if (hr == S_OK && out)
    {
        *out = out_params;
        IWbemClassObject_AddRef( out_params );
    }
    if (out_params) IWbemClassObject_Release( out_params );
    return hr;
}
