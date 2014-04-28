/*
 * Implementation of the OLEACC dll
 *
 * Copyright 2003 Mike McCormack for CodeWeavers
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
#include "winuser.h"
#include "ole2.h"
#include "oleacc.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(oleacc);

static const WCHAR lresult_atom_prefix[] = {'w','i','n','e','_','o','l','e','a','c','c',':'};

static HINSTANCE oleacc_handle = 0;

HRESULT WINAPI CreateStdAccessibleObject( HWND hwnd, LONG idObject,
                             REFIID riidInterface, void** ppvObject )
{
    FIXME("%p %d %s %p\n", hwnd, idObject,
          debugstr_guid( riidInterface ), ppvObject );
    return E_NOTIMPL;
}

HRESULT WINAPI ObjectFromLresult( LRESULT result, REFIID riid, WPARAM wParam, void **ppObject )
{
    WCHAR atom_str[sizeof(lresult_atom_prefix)/sizeof(WCHAR)+3*8+3];
    HANDLE server_proc, server_mapping, mapping;
    DWORD proc_id, size;
    IStream *stream;
    HGLOBAL data;
    void *view;
    HRESULT hr;
    WCHAR *p;

    TRACE("%ld %s %ld %p\n", result, debugstr_guid(riid), wParam, ppObject );

    if(wParam)
        FIXME("unsupported wParam = %lx\n", wParam);

    if(!ppObject)
        return E_INVALIDARG;
    *ppObject = NULL;

    if(result != (ATOM)result)
        return E_FAIL;

    if(!GlobalGetAtomNameW(result, atom_str, sizeof(atom_str)/sizeof(WCHAR)))
        return E_FAIL;
    if(memcmp(atom_str, lresult_atom_prefix, sizeof(lresult_atom_prefix)))
        return E_FAIL;
    p = atom_str + sizeof(lresult_atom_prefix)/sizeof(WCHAR);
    proc_id = strtoulW(p, &p, 16);
    if(*p != ':')
        return E_FAIL;
    server_mapping = ULongToHandle( strtoulW(p+1, &p, 16) );
    if(*p != ':')
        return E_FAIL;
    size = strtoulW(p+1, &p, 16);
    if(*p != 0)
        return E_FAIL;

    server_proc = OpenProcess(PROCESS_DUP_HANDLE, FALSE, proc_id);
    if(!server_proc)
        return E_FAIL;

    if(!DuplicateHandle(server_proc, server_mapping, GetCurrentProcess(), &mapping,
                0, FALSE, DUPLICATE_CLOSE_SOURCE|DUPLICATE_SAME_ACCESS))
        return E_FAIL;
    CloseHandle(server_proc);
    GlobalDeleteAtom(result);

    view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);
    if(!view)
        return E_FAIL;

    data = GlobalAlloc(GMEM_FIXED, size);
    memcpy(data, view, size);
    UnmapViewOfFile(view);
    if(!data)
        return E_OUTOFMEMORY;

    hr = CreateStreamOnHGlobal(data, TRUE, &stream);
    if(FAILED(hr)) {
        GlobalFree(data);
        return hr;
    }

    hr = CoUnmarshalInterface(stream, riid, ppObject);
    IStream_Release(stream);
    return hr;
}

LRESULT WINAPI LresultFromObject( REFIID riid, WPARAM wParam, LPUNKNOWN pAcc )
{
    static const WCHAR atom_fmt[] = {'%','0','8','x',':','%','0','8','x',':','%','0','8','x',0};
    static const LARGE_INTEGER seek_zero = {{0}};

    WCHAR atom_str[sizeof(lresult_atom_prefix)/sizeof(WCHAR)+3*8+3];
    IStream *stream;
    HANDLE mapping;
    STATSTG stat;
    HRESULT hr;
    ATOM atom;
    void *view;

    TRACE("%s %ld %p\n", debugstr_guid(riid), wParam, pAcc);

    if(wParam)
        FIXME("unsupported wParam = %lx\n", wParam);

    if(!pAcc)
        return E_INVALIDARG;

    hr = CreateStreamOnHGlobal(NULL, TRUE, &stream);
    if(FAILED(hr))
        return hr;

    hr = CoMarshalInterface(stream, riid, pAcc, MSHCTX_LOCAL, NULL, MSHLFLAGS_NORMAL);
    if(FAILED(hr)) {
        IStream_Release(stream);
        return hr;
    }

    hr = IStream_Seek(stream, seek_zero, STREAM_SEEK_SET, NULL);
    if(FAILED(hr)) {
        IStream_Release(stream);
        return hr;
    }

    hr = IStream_Stat(stream, &stat, STATFLAG_NONAME);
    if(FAILED(hr)) {
        CoReleaseMarshalData(stream);
        IStream_Release(stream);
        return hr;
    }else if(stat.cbSize.u.HighPart) {
        FIXME("stream size to big\n");
        CoReleaseMarshalData(stream);
        IStream_Release(stream);
        return E_NOTIMPL;
    }

    mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
            stat.cbSize.u.HighPart, stat.cbSize.u.LowPart, NULL);
    if(!mapping) {
        CoReleaseMarshalData(stream);
        IStream_Release(stream);
        return hr;
    }

    view = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, 0);
    if(!view) {
        CloseHandle(mapping);
        CoReleaseMarshalData(stream);
        IStream_Release(stream);
        return E_FAIL;
    }

    hr = IStream_Read(stream, view, stat.cbSize.u.LowPart, NULL);
    UnmapViewOfFile(view);
    if(FAILED(hr)) {
        CloseHandle(mapping);
        hr = IStream_Seek(stream, seek_zero, STREAM_SEEK_SET, NULL);
        if(SUCCEEDED(hr))
            CoReleaseMarshalData(stream);
        IStream_Release(stream);
        return hr;

    }

    memcpy(atom_str, lresult_atom_prefix, sizeof(lresult_atom_prefix));
    sprintfW(atom_str+sizeof(lresult_atom_prefix)/sizeof(WCHAR),
             atom_fmt, GetCurrentProcessId(), HandleToUlong(mapping), stat.cbSize.u.LowPart);
    atom = GlobalAddAtomW(atom_str);
    if(!atom) {
        CloseHandle(mapping);
        hr = IStream_Seek(stream, seek_zero, STREAM_SEEK_SET, NULL);
        if(SUCCEEDED(hr))
            CoReleaseMarshalData(stream);
        IStream_Release(stream);
        return E_FAIL;
    }

    IStream_Release(stream);
    return atom;
}

HRESULT WINAPI AccessibleObjectFromPoint( POINT ptScreen, IAccessible** ppacc, VARIANT* pvarChild )
{
    FIXME("{%d,%d} %p %p: stub\n", ptScreen.x, ptScreen.y, ppacc, pvarChild );
    return E_NOTIMPL;
}

HRESULT WINAPI AccessibleObjectFromWindow( HWND hwnd, DWORD dwObjectID,
                             REFIID riid, void** ppvObject )
{
    TRACE("%p %d %s %p\n", hwnd, dwObjectID,
          debugstr_guid( riid ), ppvObject );

    if(!ppvObject)
        return E_INVALIDARG;
    *ppvObject = NULL;

    if(IsWindow(hwnd)) {
        LRESULT lres;

        lres = SendMessageW(hwnd, WM_GETOBJECT, 0xffffffff, dwObjectID);
        if(FAILED(lres))
            return lres;
        else if(lres)
            return ObjectFromLresult(lres, riid, 0, ppvObject);
    }

    return CreateStdAccessibleObject(hwnd, dwObjectID, riid, ppvObject);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason,
                    LPVOID lpvReserved)
{
    TRACE("%p, %d, %p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            oleacc_handle = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }
    return TRUE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, void **ppv)
{
    FIXME("%s %s %p: stub\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);
    return E_NOTIMPL;
}

HRESULT WINAPI DllRegisterServer(void)
{
    FIXME("\n");
    return S_OK;
}

HRESULT WINAPI DllUnregisterServer(void)
{
    FIXME("\n");
    return S_OK;
}

void WINAPI GetOleaccVersionInfo(DWORD* pVersion, DWORD* pBuild)
{
    *pVersion = MAKELONG(0,7); /* Windows 7 version of oleacc: 7.0.0.0 */
    *pBuild = MAKELONG(0,0);
}

UINT WINAPI GetRoleTextW(DWORD role, LPWSTR lpRole, UINT rolemax)
{
    INT ret;
    WCHAR *resptr;

    TRACE("%u %p %u\n", role, lpRole, rolemax);

    /* return role text length */
    if(!lpRole)
        return LoadStringW(oleacc_handle, role, (LPWSTR)&resptr, 0);

    ret = LoadStringW(oleacc_handle, role, lpRole, rolemax);
    if(!(ret > 0)){
        if(rolemax > 0) lpRole[0] = '\0';
        return 0;
    }

    return ret;
}

UINT WINAPI GetRoleTextA(DWORD role, LPSTR lpRole, UINT rolemax)
{
    UINT length;
    WCHAR *roletextW;

    TRACE("%u %p %u\n", role, lpRole, rolemax);

    length = GetRoleTextW(role, NULL, 0);
    if((length == 0) || (lpRole && !rolemax))
        return 0;

    roletextW = HeapAlloc(GetProcessHeap(), 0, (length + 1)*sizeof(WCHAR));
    if(!roletextW)
        return 0;

    GetRoleTextW(role, roletextW, length + 1);

    length = WideCharToMultiByte( CP_ACP, 0, roletextW, -1, NULL, 0, NULL, NULL );

    if(!lpRole){
        HeapFree(GetProcessHeap(), 0, roletextW);
        return length - 1;
    }

    WideCharToMultiByte( CP_ACP, 0, roletextW, -1, lpRole, rolemax, NULL, NULL );

    if(rolemax < length){
        lpRole[rolemax-1] = '\0';
        length = rolemax;
    }

    HeapFree(GetProcessHeap(), 0, roletextW);

    return length - 1;
}
