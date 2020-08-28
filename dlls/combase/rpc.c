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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winsvc.h"
#include "servprov.h"

#include "wine/debug.h"
#include "wine/exception.h"
#include "wine/heap.h"

#include "combase_private.h"

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

static void get_localserver_pipe_name(WCHAR *pipefn, REFCLSID rclsid)
{
    static const WCHAR wszPipeRef[] = {'\\','\\','.','\\','p','i','p','e','\\',0};
    lstrcpyW(pipefn, wszPipeRef);
    StringFromGUID2(rclsid, pipefn + ARRAY_SIZE(wszPipeRef) - 1, CHARS_IN_GUID);
}

static DWORD start_local_service(const WCHAR *name, DWORD num, LPCWSTR *params)
{
    SC_HANDLE handle, hsvc;
    DWORD r = ERROR_FUNCTION_FAILED;

    TRACE("Starting service %s %d params\n", debugstr_w(name), num);

    handle = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!handle)
        return r;
    hsvc = OpenServiceW(handle, name, SERVICE_START);
    if (hsvc)
    {
        if(StartServiceW(hsvc, num, params))
            r = ERROR_SUCCESS;
        else
            r = GetLastError();
        if (r == ERROR_SERVICE_ALREADY_RUNNING)
            r = ERROR_SUCCESS;
        CloseServiceHandle(hsvc);
    }
    else
        r = GetLastError();
    CloseServiceHandle(handle);

    TRACE("StartService returned error %u (%s)\n", r, (r == ERROR_SUCCESS) ? "ok":"failed");

    return r;
}

/*
 * create_local_service()  - start a COM server in a service
 *
 *   To start a Local Service, we read the AppID value under
 * the class's CLSID key, then open the HKCR\\AppId key specified
 * there and check for a LocalService value.
 *
 * Note:  Local Services are not supported under Windows 9x
 */
static HRESULT create_local_service(REFCLSID rclsid)
{
    HRESULT hr;
    WCHAR buf[CHARS_IN_GUID];
    HKEY hkey;
    LONG r;
    DWORD type, sz;

    TRACE("Attempting to start Local service for %s\n", debugstr_guid(rclsid));

    hr = open_appidkey_from_clsid(rclsid, KEY_READ, &hkey);
    if (FAILED(hr))
        return hr;

    /* read the LocalService and ServiceParameters values from the AppID key */
    sz = sizeof buf;
    r = RegQueryValueExW(hkey, L"LocalService", NULL, &type, (LPBYTE)buf, &sz);
    if (r == ERROR_SUCCESS && type == REG_SZ)
    {
        DWORD num_args = 0;
        LPWSTR args[1] = { NULL };

        /*
         * FIXME: I'm not really sure how to deal with the service parameters.
         *        I suspect that the string returned from RegQueryValueExW
         *        should be split into a number of arguments by spaces.
         *        It would make more sense if ServiceParams contained a
         *        REG_MULTI_SZ here, but it's a REG_SZ for the services
         *        that I'm interested in for the moment.
         */
        r = RegQueryValueExW(hkey, L"ServiceParams", NULL, &type, NULL, &sz);
        if (r == ERROR_SUCCESS && type == REG_SZ && sz)
        {
            args[0] = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sz);
            num_args++;
            RegQueryValueExW(hkey, L"ServiceParams", NULL, &type, (LPBYTE)args[0], &sz);
        }
        r = start_local_service(buf, num_args, (LPCWSTR *)args);
        if (r != ERROR_SUCCESS)
            hr = REGDB_E_CLASSNOTREG; /* FIXME: check retval */
        HeapFree(GetProcessHeap(),0,args[0]);
    }
    else
    {
        WARN("No LocalService value\n");
        hr = REGDB_E_CLASSNOTREG; /* FIXME: check retval */
    }
    RegCloseKey(hkey);

    return hr;
}

static HRESULT create_server(REFCLSID rclsid, HANDLE *process)
{
    static const WCHAR  embeddingW[] = L" -Embedding";
    HKEY                key;
    HRESULT             hr;
    WCHAR               command[MAX_PATH + ARRAY_SIZE(embeddingW)];
    DWORD               size = (MAX_PATH+1) * sizeof(WCHAR);
    STARTUPINFOW        sinfo;
    PROCESS_INFORMATION pinfo;
    LONG ret;

    hr = open_key_for_clsid(rclsid, L"LocalServer32", KEY_READ, &key);
    if (FAILED(hr))
    {
        ERR("class %s not registered\n", debugstr_guid(rclsid));
        return hr;
    }

    ret = RegQueryValueExW(key, NULL, NULL, NULL, (LPBYTE)command, &size);
    RegCloseKey(key);
    if (ret)
    {
        WARN("No default value for LocalServer32 key\n");
        return REGDB_E_CLASSNOTREG; /* FIXME: check retval */
    }

    memset(&sinfo, 0, sizeof(sinfo));
    sinfo.cb = sizeof(sinfo);

    /* EXE servers are started with the -Embedding switch. */

    lstrcatW(command, embeddingW);

    TRACE("activating local server %s for %s\n", debugstr_w(command), debugstr_guid(rclsid));

    /* FIXME: Win2003 supports a ServerExecutable value that is passed into
     * CreateProcess */
    if (!CreateProcessW(NULL, command, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &sinfo, &pinfo))
    {
        WARN("failed to run local server %s\n", debugstr_w(command));
        return HRESULT_FROM_WIN32(GetLastError());
    }
    *process = pinfo.hProcess;
    CloseHandle(pinfo.hThread);

    return S_OK;
}

/* FIXME: should call to rpcss instead */
HRESULT rpc_get_local_class_object(REFCLSID rclsid, REFIID riid, void **obj)
{
    HRESULT        hr;
    HANDLE         hPipe;
    WCHAR          pipefn[100];
    DWORD          res, bufferlen;
    char           marshalbuffer[200];
    IStream       *pStm;
    LARGE_INTEGER  seekto;
    ULARGE_INTEGER newpos;
    int            tries = 0;
    IServiceProvider *local_server;

    static const int MAXTRIES = 30; /* 30 seconds */

    TRACE("rclsid %s, riid %s\n", debugstr_guid(rclsid), debugstr_guid(riid));

    get_localserver_pipe_name(pipefn, rclsid);

    while (tries++ < MAXTRIES)
    {
        TRACE("waiting for %s\n", debugstr_w(pipefn));

        WaitNamedPipeW(pipefn, NMPWAIT_WAIT_FOREVER);
        hPipe = CreateFileW(pipefn, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
        if (hPipe == INVALID_HANDLE_VALUE)
        {
            DWORD index;
            DWORD start_ticks;
            HANDLE process = 0;
            if (tries == 1)
            {
                if ((hr = create_local_service(rclsid)) && (hr = create_server(rclsid, &process)))
                    return hr;
            }
            else
            {
                WARN("Connecting to %s, no response yet, retrying: le is %u\n", debugstr_w(pipefn), GetLastError());
            }
            /* wait for one second, even if messages arrive */
            start_ticks = GetTickCount();
            do
            {
                if (SUCCEEDED(CoWaitForMultipleHandles(0, 1000, (process != 0), &process, &index)) && process && !index)
                {
                    WARN("server for %s failed to start\n", debugstr_guid(rclsid));
                    CloseHandle( hPipe );
                    CloseHandle( process );
                    return E_NOINTERFACE;
                }
            } while (GetTickCount() - start_ticks < 1000);
            if (process) CloseHandle(process);
            continue;
        }
        bufferlen = 0;
        if (!ReadFile(hPipe, marshalbuffer, sizeof(marshalbuffer), &bufferlen, NULL))
        {
            FIXME("Failed to read marshal id from classfactory of %s.\n", debugstr_guid(rclsid));
            CloseHandle(hPipe);
            Sleep(1000);
            continue;
        }
        TRACE("read marshal id from pipe\n");
        CloseHandle(hPipe);
        break;
    }

    if (tries >= MAXTRIES)
        return E_NOINTERFACE;

    hr = CreateStreamOnHGlobal(0, TRUE, &pStm);
    if (hr != S_OK) return hr;
    hr = IStream_Write(pStm, marshalbuffer, bufferlen, &res);
    if (hr != S_OK) goto out;
    seekto.u.LowPart = 0;seekto.u.HighPart = 0;
    hr = IStream_Seek(pStm, seekto, STREAM_SEEK_SET, &newpos);
    TRACE("unmarshalling local server\n");
    hr = CoUnmarshalInterface(pStm, &IID_IServiceProvider, (void **)&local_server);
    if(SUCCEEDED(hr))
        hr = IServiceProvider_QueryService(local_server, rclsid, riid, obj);
    IServiceProvider_Release(local_server);
out:
    IStream_Release(pStm);
    return hr;
}

struct local_server_params
{
    CLSID clsid;
    IStream *stream;
    HANDLE pipe;
    HANDLE stop_event;
    HANDLE thread;
    BOOL multi_use;
};

/* FIXME: should call to rpcss instead */
static DWORD WINAPI local_server_thread(void *param)
{
    struct local_server_params * lsp = param;
    WCHAR 		pipefn[100];
    HRESULT		hres;
    IStream		*pStm = lsp->stream;
    STATSTG		ststg;
    unsigned char	*buffer;
    int 		buflen;
    LARGE_INTEGER	seekto;
    ULARGE_INTEGER	newpos;
    ULONG		res;
    BOOL multi_use = lsp->multi_use;
    OVERLAPPED ovl;
    HANDLE pipe_event, hPipe = lsp->pipe, new_pipe;
    DWORD  bytes;

    TRACE("Starting threader for %s.\n",debugstr_guid(&lsp->clsid));

    memset(&ovl, 0, sizeof(ovl));
    get_localserver_pipe_name(pipefn, &lsp->clsid);
    ovl.hEvent = pipe_event = CreateEventW(NULL, FALSE, FALSE, NULL);

    while (1) {
        if (!ConnectNamedPipe(hPipe, &ovl))
        {
            DWORD error = GetLastError();
            if (error == ERROR_IO_PENDING)
            {
                HANDLE handles[2] = { pipe_event, lsp->stop_event };
                DWORD ret;
                ret = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
                if (ret != WAIT_OBJECT_0)
                    break;
            }
            /* client already connected isn't an error */
            else if (error != ERROR_PIPE_CONNECTED)
            {
                ERR("ConnectNamedPipe failed with error %d\n", GetLastError());
                break;
            }
        }

        TRACE("marshalling LocalServer to client\n");

        hres = IStream_Stat(pStm,&ststg,STATFLAG_NONAME);
        if (hres != S_OK)
            break;

        seekto.u.LowPart = 0;
        seekto.u.HighPart = 0;
        hres = IStream_Seek(pStm,seekto,STREAM_SEEK_SET,&newpos);
        if (hres != S_OK) {
            FIXME("IStream_Seek failed, %x\n",hres);
            break;
        }

        buflen = ststg.cbSize.u.LowPart;
        buffer = HeapAlloc(GetProcessHeap(),0,buflen);

        hres = IStream_Read(pStm,buffer,buflen,&res);
        if (hres != S_OK) {
            FIXME("Stream Read failed, %x\n",hres);
            HeapFree(GetProcessHeap(),0,buffer);
            break;
        }

        WriteFile(hPipe,buffer,buflen,&res,&ovl);
        GetOverlappedResult(hPipe, &ovl, &bytes, TRUE);
        HeapFree(GetProcessHeap(),0,buffer);

        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);
        TRACE("done marshalling LocalServer\n");

        if (!multi_use)
        {
            TRACE("single use object, shutting down pipe %s\n", debugstr_w(pipefn));
            break;
        }
        new_pipe = CreateNamedPipeW( pipefn, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                     PIPE_TYPE_BYTE|PIPE_WAIT, PIPE_UNLIMITED_INSTANCES,
                                     4096, 4096, 500 /* 0.5 second timeout */, NULL );
        if (new_pipe == INVALID_HANDLE_VALUE)
        {
            FIXME("pipe creation failed for %s, le is %u\n", debugstr_w(pipefn), GetLastError());
            break;
        }
        CloseHandle(hPipe);
        hPipe = new_pipe;
    }

    CloseHandle(pipe_event);
    CloseHandle(hPipe);
    return 0;
}

HRESULT rpc_start_local_server(REFCLSID clsid, IStream *stream, BOOL multi_use, void **registration)
{
    DWORD tid, err;
    struct local_server_params *lsp;
    WCHAR pipefn[100];

    lsp = HeapAlloc(GetProcessHeap(), 0, sizeof(*lsp));
    if (!lsp)
        return E_OUTOFMEMORY;

    lsp->clsid = *clsid;
    lsp->stream = stream;
    IStream_AddRef(stream);
    lsp->stop_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!lsp->stop_event)
    {
        HeapFree(GetProcessHeap(), 0, lsp);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    lsp->multi_use = multi_use;

    get_localserver_pipe_name(pipefn, &lsp->clsid);
    lsp->pipe = CreateNamedPipeW(pipefn, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                 PIPE_TYPE_BYTE|PIPE_WAIT, PIPE_UNLIMITED_INSTANCES,
                                 4096, 4096, 500 /* 0.5 second timeout */, NULL);
    if (lsp->pipe == INVALID_HANDLE_VALUE)
    {
        err = GetLastError();
        FIXME("pipe creation failed for %s, le is %u\n", debugstr_w(pipefn), GetLastError());
        CloseHandle(lsp->stop_event);
        HeapFree(GetProcessHeap(), 0, lsp);
        return HRESULT_FROM_WIN32(err);
    }

    lsp->thread = CreateThread(NULL, 0, local_server_thread, lsp, 0, &tid);
    if (!lsp->thread)
    {
        CloseHandle(lsp->pipe);
        CloseHandle(lsp->stop_event);
        HeapFree(GetProcessHeap(), 0, lsp);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *registration = lsp;
    return S_OK;
}

void rpc_stop_local_server(void *registration)
{
    struct local_server_params *lsp = registration;

    /* signal local_server_thread to stop */
    SetEvent(lsp->stop_event);
    /* wait for it to exit */
    WaitForSingleObject(lsp->thread, INFINITE);

    IStream_Release(lsp->stream);
    CloseHandle(lsp->stop_event);
    CloseHandle(lsp->thread);
    HeapFree(GetProcessHeap(), 0, lsp);
}
