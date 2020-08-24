/*
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

#include "windef.h"
#include "winbase.h"
#include "winsvc.h"

#include "wine/debug.h"
#include "wine/exception.h"
#include "wine/heap.h"

#include "irpcss.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

void * __RPC_USER MIDL_user_allocate(SIZE_T size)
{
    return heap_alloc(size);
}

void __RPC_USER MIDL_user_free(void *p)
{
    heap_free(p);
}

static LONG WINAPI rpc_filter(EXCEPTION_POINTERS *eptr)
{
    return I_RpcExceptionFilter(eptr->ExceptionRecord->ExceptionCode);
}

static BOOL start_rpcss(void)
{
    SERVICE_STATUS_PROCESS status;
    SC_HANDLE scm, service;
    BOOL ret = FALSE;

    TRACE("\n");

    if (!(scm = OpenSCManagerW(NULL, NULL, 0)))
    {
        ERR("Failed to open service manager\n");
        return FALSE;
    }

    if (!(service = OpenServiceW(scm, L"RpcSs", SERVICE_START | SERVICE_QUERY_STATUS)))
    {
        ERR("Failed to open RpcSs service\n");
        CloseServiceHandle( scm );
        return FALSE;
    }

    if (StartServiceW(service, 0, NULL) || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
    {
        ULONGLONG start_time = GetTickCount64();
        do
        {
            DWORD dummy;

            if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (BYTE *)&status, sizeof(status), &dummy))
                break;
            if (status.dwCurrentState == SERVICE_RUNNING)
            {
                ret = TRUE;
                break;
            }
            if (GetTickCount64() - start_time > 30000) break;
            Sleep( 100 );

        } while (status.dwCurrentState == SERVICE_START_PENDING);

        if (status.dwCurrentState != SERVICE_RUNNING)
            WARN("RpcSs failed to start %u\n", status.dwCurrentState);
    }
    else
        ERR("Failed to start RpcSs service\n");

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return ret;
}

static RPC_BINDING_HANDLE get_rpc_handle(unsigned short *protseq, unsigned short *endpoint)
{
    RPC_BINDING_HANDLE handle = NULL;
    RPC_STATUS status;
    RPC_WSTR binding;

    status = RpcStringBindingComposeW(NULL, protseq, NULL, endpoint, NULL, &binding);
    if (status == RPC_S_OK)
    {
        status = RpcBindingFromStringBindingW(binding, &handle);
        RpcStringFreeW(&binding);
    }

    return handle;
}

static RPC_BINDING_HANDLE get_irpcss_handle(void)
{
    static RPC_BINDING_HANDLE irpcss_handle;

    if (!irpcss_handle)
    {
        unsigned short protseq[] = IRPCSS_PROTSEQ;
        unsigned short endpoint[] = IRPCSS_ENDPOINT;

        RPC_BINDING_HANDLE new_handle = get_rpc_handle(protseq, endpoint);
        if (InterlockedCompareExchangePointer(&irpcss_handle, new_handle, NULL))
            /* another thread beat us to it */
            RpcBindingFree(&new_handle);
    }
    return irpcss_handle;
}

static RPC_BINDING_HANDLE get_irot_handle(void)
{
    static RPC_BINDING_HANDLE irot_handle;

    if (!irot_handle)
    {
        unsigned short protseq[] = IROT_PROTSEQ;
        unsigned short endpoint[] = IROT_ENDPOINT;

        RPC_BINDING_HANDLE new_handle = get_rpc_handle(protseq, endpoint);
        if (InterlockedCompareExchangePointer(&irot_handle, new_handle, NULL))
            /* another thread beat us to it */
            RpcBindingFree(&new_handle);
    }
    return irot_handle;
}

#define RPCSS_CALL_START \
    HRESULT hr; \
    for (;;) { \
        __TRY {

#define RPCSS_CALL_END \
        } __EXCEPT(rpc_filter) { \
            hr = HRESULT_FROM_WIN32(GetExceptionCode()); \
        } \
        __ENDTRY \
        if (hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE)) { \
            if (start_rpcss()) \
                continue; \
        } \
        break; \
    } \
    return hr;

HRESULT rpcss_get_next_seqid(DWORD *id)
{
    RPCSS_CALL_START
    hr = irpcss_get_thread_seq_id(get_irpcss_handle(), id);
    RPCSS_CALL_END
}

HRESULT WINAPI InternalIrotRegister(const MonikerComparisonData *moniker_data,
        const InterfaceData *object, const InterfaceData *moniker,
        const FILETIME *time, DWORD flags, IrotCookie *cookie, IrotContextHandle *ctxt_handle)
{
    RPCSS_CALL_START
    hr = IrotRegister(get_irot_handle(), moniker_data, object, moniker, time, flags, cookie, ctxt_handle);
    RPCSS_CALL_END
}

HRESULT WINAPI InternalIrotIsRunning(const MonikerComparisonData *moniker_data)
{
    RPCSS_CALL_START
    hr = IrotIsRunning(get_irot_handle(), moniker_data);
    RPCSS_CALL_END
}

HRESULT WINAPI InternalIrotGetObject(const MonikerComparisonData *moniker_data, PInterfaceData *obj,
        IrotCookie *cookie)
{
    RPCSS_CALL_START
    hr = IrotGetObject(get_irot_handle(), moniker_data, obj, cookie);
    RPCSS_CALL_END
}

HRESULT WINAPI InternalIrotNoteChangeTime(IrotCookie cookie, const FILETIME *time)
{
    RPCSS_CALL_START
    hr = IrotNoteChangeTime(get_irot_handle(), cookie, time);
    RPCSS_CALL_END
}

HRESULT WINAPI InternalIrotGetTimeOfLastChange(const MonikerComparisonData *moniker_data, FILETIME *time)
{
    RPCSS_CALL_START
    hr = IrotGetTimeOfLastChange(get_irot_handle(), moniker_data, time);
    RPCSS_CALL_END
}

HRESULT WINAPI InternalIrotEnumRunning(PInterfaceList *list)
{
    RPCSS_CALL_START
    hr = IrotEnumRunning(get_irot_handle(), list);
    RPCSS_CALL_END
}

HRESULT WINAPI InternalIrotRevoke(IrotCookie cookie, IrotContextHandle *ctxt_handle, PInterfaceData *object,
        PInterfaceData *moniker)
{
    RPCSS_CALL_START
    hr = IrotRevoke(get_irot_handle(), cookie, ctxt_handle, object, moniker);
    RPCSS_CALL_END
}
