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

enum
{
    SERVICE_ERROR_SUCCESS                    = 0,
    SERVICE_ERROR_NOT_SUPPORTED              = 1,
    SERVICE_ERROR_ACCESS_DENIED              = 2,
    SERVICE_ERROR_DEPENDENT_SERVICES_RUNNING = 3,
    SERVICE_ERROR_INVALID_CONTROL            = 4,
    SERVICE_ERROR_CANNOT_ACCEPT_CTRL         = 5,
    SERVICE_ERROR_NOT_ACTIVE                 = 6,
    SERVICE_ERROR_REQUEST_TIMEOUT            = 7,
    SERVICE_ERROR_UNKNOWN_FAILURE            = 8,
    SERVICE_ERROR_PATH_NOT_FOUND             = 9,
    SERVICE_ERROR_ALREADY_RUNNING            = 10,
    SERVICE_ERROR_DATABASE_LOCKED            = 11,
    SERVICE_ERROR_DEPENDENCY_DELETED         = 12,
    SERVICE_ERROR_DEPENDENCY_FAILURE         = 13,
    SERVICE_ERROR_DISABLED                   = 14,
    SERVICE_ERROR_LOGON_FAILED               = 15,
    SERVICE_ERROR_MARKED_FOR_DELETION        = 16,
    SERVICE_ERROR_NO_THREAD                  = 17,
    SERVICE_ERROR_INVALID_PARAMETER          = 21,
    SERVICE_ERROR_EXISTS                     = 23,
};

static UINT map_error( DWORD error )
{
    switch (error)
    {
    case ERROR_SUCCESS:                     return SERVICE_ERROR_SUCCESS;
    case ERROR_ACCESS_DENIED:               return SERVICE_ERROR_ACCESS_DENIED;
    case ERROR_DEPENDENT_SERVICES_RUNNING:  return SERVICE_ERROR_DEPENDENT_SERVICES_RUNNING;
    case ERROR_INVALID_SERVICE_CONTROL:     return SERVICE_ERROR_INVALID_CONTROL;
    case ERROR_SERVICE_CANNOT_ACCEPT_CTRL:  return SERVICE_ERROR_CANNOT_ACCEPT_CTRL;
    case ERROR_SERVICE_NOT_ACTIVE:          return SERVICE_ERROR_NOT_ACTIVE;
    case ERROR_SERVICE_REQUEST_TIMEOUT:     return SERVICE_ERROR_REQUEST_TIMEOUT;
    case ERROR_SERVICE_ALREADY_RUNNING:     return SERVICE_ERROR_ALREADY_RUNNING;
    case ERROR_SERVICE_DATABASE_LOCKED:     return SERVICE_ERROR_DATABASE_LOCKED;
    case ERROR_SERVICE_DEPENDENCY_DELETED:  return SERVICE_ERROR_DEPENDENCY_DELETED;
    case ERROR_SERVICE_DEPENDENCY_FAIL:     return SERVICE_ERROR_DEPENDENCY_FAILURE;
    case ERROR_SERVICE_DISABLED:            return SERVICE_ERROR_DISABLED;
    case ERROR_SERVICE_LOGON_FAILED:        return SERVICE_ERROR_LOGON_FAILED;
    case ERROR_SERVICE_MARKED_FOR_DELETE:   return SERVICE_ERROR_MARKED_FOR_DELETION;
    case ERROR_SERVICE_NO_THREAD:           return SERVICE_ERROR_NO_THREAD;
    case ERROR_SERVICE_EXISTS:              return SERVICE_ERROR_EXISTS;
    default:
        WARN( "unknown error %lu\n", error );
        break;
    }
    return SERVICE_ERROR_UNKNOWN_FAILURE;
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
    if (!wcsicmp( start_mode, L"Boot" ))      return SERVICE_BOOT_START;
    if (!wcsicmp( start_mode, L"System" ))    return SERVICE_SYSTEM_START;
    if (!wcsicmp( start_mode, L"Automatic" )) return SERVICE_AUTO_START;
    if (!wcsicmp( start_mode, L"Manual" ))    return SERVICE_DEMAND_START;
    if (!wcsicmp( start_mode, L"Disabled" ))  return SERVICE_DISABLED;
    WARN( "unknown mode %s\n", debugstr_w(start_mode) );
    return ~0u;
}

static HRESULT change_start_mode( const WCHAR *name, const WCHAR *start_mode, VARIANT *retval )
{
    SC_HANDLE manager = NULL, service = NULL;
    DWORD start_type = map_start_mode( start_mode );
    UINT error = 0;

    if (start_type == ~0u)
    {
        error = SERVICE_ERROR_INVALID_PARAMETER;
        goto done;
    }
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
