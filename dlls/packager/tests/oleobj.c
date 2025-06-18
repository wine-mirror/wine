/*
 * Copyright 2014 Andrew Eikum for CodeWeavers
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

#include <initguid.h>
#include <olectl.h>

#include "wine/test.h"

DEFINE_GUID(CLSID_Package, 0xf20da720, 0xc02f, 0x11ce, 0x92,0x7b, 0x08,0x00,0x09,0x5a,0xe3,0x40);
DEFINE_GUID(CLSID_Package_Alt, 0x0003000C, 0x0000, 0x0000, 0xC0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46);

static HRESULT WINAPI ole10native_stream_QueryInterface(IStream* This, REFIID riid,
        void **ppvObject)
{
    ok(0, "queryinterface\n");
    return E_NOTIMPL;
}

static ULONG WINAPI ole10native_stream_AddRef(IStream* This)
{
    return 2;
}

static ULONG WINAPI ole10native_stream_Release(IStream* This)
{
    return 1;
}

static UINT offs = 0;

static HRESULT WINAPI ole10native_stream_Read(IStream* This, void *pv, ULONG cb,
        ULONG *pcbRead)
{
    ULONG to_read;

    static BYTE data[] = {
        0xa5, 0x00, 0x00, 0x00, /* total size */
        0x02, 0x00, /* ?? */
        'l','a','b','e','l','.','t','x','t',0, /* label? */
        'f','i','l','e','n','a','m','e','.','t','x','t',0, /* original filename? */
        0x00, 0x00, /* ?? */
        0x03, 0x00, /* ?? */
        0x10, 0x00, 0x00, 0x00, /* ASCIIZ filename size */
        'C',':','\\','f','i','l','e','n','a','m','e','2','.','t','x','t', /* ASCIIZ filename */
        0x0a, 0x00, 0x00, 0x00, /* payload size */
        's','o','m','e',' ','t','e','x','t','\n', /* payload */
        0x10, 0x00, 0x00, 0x00, /* WCHAR filename size */
        'C',0,':',0,'\\',0,'f',0,'i',0,'l',0,'e',0,'n',0,'a',0,'m',0,'e',0,'3',0,'.',0,'t',0,'x',0,'t',0, /* WCHAR filename */
        0x0d, 0x00, 0x00, 0x00, /* another filename size */
        'f',0,'i',0,'l',0,'e',0,'n',0,'a',0,'m',0,'e',0,'4',0,'.',0,'t',0,'x',0,'t',0, /* another filename */
        0x10, 0x00, 0x00, 0x00, /* another filename size */
        'C',0,':',0,'\\',0,'f',0,'i',0,'l',0,'e',0,'n',0,'a',0,'m',0,'e',0,'5',0,'.',0,'t',0,'x',0,'t',0, /* another filename */
    };

    to_read = min(sizeof(data) - offs, cb);

    memcpy(pv, data + offs, to_read);
    if(pcbRead)
        *pcbRead = to_read;

    offs += to_read;

    return cb == to_read ? S_OK : S_FALSE;
}

static HRESULT WINAPI ole10native_stream_Write(IStream* This, const void *pv,
        ULONG cb, ULONG *pcbWritten)
{
    ok(0, "write\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ole10native_stream_Seek(IStream* This, LARGE_INTEGER dlibMove,
        DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    if(dwOrigin == STREAM_SEEK_CUR)
        offs += dlibMove.u.LowPart;
    else if(dwOrigin == STREAM_SEEK_SET)
        offs = dlibMove.u.LowPart;

    if(plibNewPosition){
        plibNewPosition->u.HighPart = 0;
        plibNewPosition->u.LowPart = offs;
    }

    return S_OK;
}

static HRESULT WINAPI ole10native_stream_SetSize(IStream* This,
        ULARGE_INTEGER libNewSize)
{
    ok(0, "setsize\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ole10native_stream_CopyTo(IStream* This, IStream *pstm,
        ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    ok(0, "copyto\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ole10native_stream_Commit(IStream* This, DWORD grfCommitFlags)
{
    ok(0, "commit\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ole10native_stream_Revert(IStream* This)
{
    ok(0, "revert\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ole10native_stream_LockRegion(IStream* This,
        ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    ok(0, "lockregion\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ole10native_stream_UnlockRegion(IStream* This,
        ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    ok(0, "unlockregion\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ole10native_stream_Stat(IStream* This, STATSTG *pstatstg,
        DWORD grfStatFlag)
{
    ok(0, "stat\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ole10native_stream_Clone(IStream* This, IStream **ppstm)
{
    ok(0, "clone\n");
    return E_NOTIMPL;
}

static IStreamVtbl ole10native_stream_vtbl = {
    ole10native_stream_QueryInterface,
    ole10native_stream_AddRef,
    ole10native_stream_Release,
    ole10native_stream_Read,
    ole10native_stream_Write,
    ole10native_stream_Seek,
    ole10native_stream_SetSize,
    ole10native_stream_CopyTo,
    ole10native_stream_Commit,
    ole10native_stream_Revert,
    ole10native_stream_LockRegion,
    ole10native_stream_UnlockRegion,
    ole10native_stream_Stat,
    ole10native_stream_Clone
};

static IStream ole10native_stream = {
    &ole10native_stream_vtbl
};

static HRESULT WINAPI stg_QueryInterface(IStorage* This, REFIID riid,
        void **ppvObject)
{
    ok(0, "queryinterface: %s\n", wine_dbgstr_guid(riid));

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IStorage)){
        *ppvObject = This;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI stg_AddRef(IStorage* This)
{
    return 2;
}

static ULONG WINAPI stg_Release(IStorage* This)
{
    return 1;
}

static HRESULT WINAPI stg_CreateStream(IStorage* This, LPCOLESTR pwcsName,
        DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream **ppstm)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI stg_OpenStream(IStorage* This, LPCOLESTR pwcsName,
        void *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm)
{
    if(lstrcmpW(pwcsName, L"\1Ole10Native"))
        return STG_E_FILENOTFOUND;

    *ppstm = &ole10native_stream;

    return S_OK;
}

static HRESULT WINAPI stg_CreateStorage(IStorage* This, LPCOLESTR pwcsName,
        DWORD grfMode, DWORD dwStgFmt, DWORD reserved2, IStorage **ppstg)
{
    ok(0, "createstorage\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stg_OpenStorage(IStorage* This, LPCOLESTR pwcsName,
        IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved,
        IStorage **ppstg)
{
    ok(0, "openstorage: %s\n", wine_dbgstr_w(pwcsName));
    return E_NOTIMPL;
}

static HRESULT WINAPI stg_CopyTo(IStorage* This, DWORD ciidExclude,
        const IID *rgiidExclude, SNB snbExclude, IStorage *pstgDest)
{
    ok(0, "copyto\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stg_MoveElementTo(IStorage* This, LPCOLESTR pwcsName,
        IStorage *pstgDest, LPCOLESTR pwcsNewName, DWORD grfFlags)
{
    ok(0, "moveelem\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stg_Commit(IStorage* This, DWORD grfCommitFlags)
{
    ok(0, "commit\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stg_Revert(IStorage* This)
{
    ok(0, "revert\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stg_EnumElements(IStorage* This, DWORD reserved1,
        void *reserved2, DWORD reserved3, IEnumSTATSTG **ppenum)
{
    ok(0, "enumelem\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stg_DestroyElement(IStorage* This, LPCOLESTR pwcsName)
{
    ok(0, "destroyelem\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stg_RenameElement(IStorage* This, LPCOLESTR pwcsOldName,
        LPCOLESTR pwcsNewName)
{
    ok(0, "renamelem\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stg_SetElementTimes(IStorage* This, LPCOLESTR pwcsName,
        const FILETIME *pctime, const FILETIME *patime, const FILETIME *pmtime)
{
    ok(0, "setelem\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stg_SetClass(IStorage* This, REFCLSID clsid)
{
    ok(0, "setclass\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stg_SetStateBits(IStorage* This, DWORD grfStateBits,
        DWORD grfMask)
{
    ok(0, "setstate\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI stg_Stat(IStorage* This, STATSTG *pstatstg, DWORD grfStatFlag)
{
    memset(pstatstg, 0, sizeof(*pstatstg));
    pstatstg->clsid = CLSID_Package;
    pstatstg->type = STGTY_STORAGE;
    return S_OK;
}

static IStorageVtbl stg_vtbl = {
    stg_QueryInterface,
    stg_AddRef,
    stg_Release,
    stg_CreateStream,
    stg_OpenStream,
    stg_CreateStorage,
    stg_OpenStorage,
    stg_CopyTo,
    stg_MoveElementTo,
    stg_Commit,
    stg_Revert,
    stg_EnumElements,
    stg_DestroyElement,
    stg_RenameElement,
    stg_SetElementTimes,
    stg_SetClass,
    stg_SetStateBits,
    stg_Stat,
};

static IStorage stg = {
    &stg_vtbl
};

static HRESULT WINAPI clientsite_QueryInterface(IOleClientSite* This,
        REFIID riid, void **ppvObject)
{
    ok(0, "query interface\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI clientsite_AddRef(IOleClientSite* This)
{
    return 2;
}

static ULONG WINAPI clientsite_Release(IOleClientSite* This)
{
    return 1;
}

static HRESULT WINAPI clientsite_SaveObject(IOleClientSite* This)
{
    ok(0, "saveobject\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI clientsite_GetMoniker(IOleClientSite* This,
        DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    ok(0, "getmoniker\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI clientsite_GetContainer(IOleClientSite* This,
        IOleContainer **ppContainer)
{
    ok(0, "getcontainer\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI clientsite_ShowObject(IOleClientSite* This)
{
    ok(0, "showobject\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI clientsite_OnShowWindow(IOleClientSite* This,
        BOOL fShow)
{
    ok(0, "onshowwindow\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI clientsite_RequestNewObjectLayout(IOleClientSite* This)
{
    ok(0, "requestnewobjectlayout\n");
    return E_NOTIMPL;
}

static IOleClientSiteVtbl clientsite_vtbl = {
    clientsite_QueryInterface,
    clientsite_AddRef,
    clientsite_Release,
    clientsite_SaveObject,
    clientsite_GetMoniker,
    clientsite_GetContainer,
    clientsite_ShowObject,
    clientsite_OnShowWindow,
    clientsite_RequestNewObjectLayout
};

static IOleClientSite clientsite = {
    &clientsite_vtbl
};

static void test_packager(void)
{
    IOleObject *oleobj;
    IPersistStorage *persist;
    DWORD len, bytes_read, status;
    HRESULT hr;
    HANDLE file;
    WCHAR filename[MAX_PATH];
    char contents[11];
    BOOL br, extended = FALSE;

    hr = CoCreateInstance(&CLSID_Package, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
            &IID_IOleObject, (void**)&oleobj);
    ok(hr == S_OK ||
            hr == REGDB_E_CLASSNOTREG, "CoCreateInstance(CLSID_Package) failed: %08lx\n", hr);
    if(hr == S_OK){
        IOleObject_Release(oleobj);
        /* older OSes store temporary files in obscure locations, so don't run
         * the full tests on them */
        extended = TRUE;
    }else
        win_skip("Older OS doesn't support modern packager\n");

    hr = CoCreateInstance(&CLSID_Package_Alt, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
            &IID_IOleObject, (void**)&oleobj);
    ok(hr == S_OK, "CoCreateInstance(CLSID_Package_Alt) failed: %08lx\n", hr);

    hr = IOleObject_SetClientSite(oleobj, NULL);
    ok(hr == S_OK, "SetClientSite failed: %08lx\n", hr);

    hr = IOleObject_SetClientSite(oleobj, &clientsite);
    ok(hr == S_OK, "SetClientSite failed: %08lx\n", hr);

    hr = IOleObject_GetMiscStatus(oleobj, DVASPECT_CONTENT, NULL);
    ok(hr == E_INVALIDARG, "GetMiscStatus failed: %08lx\n", hr);

    hr = IOleObject_GetMiscStatus(oleobj, DVASPECT_CONTENT, &status);
    ok(hr == S_OK, "GetMiscStatus failed: %08lx\n", hr);
    ok(status == OLEMISC_ONLYICONIC ||
            status == OLEMISC_CANTLINKINSIDE /* winxp */,
            "Got wrong DVASPECT_CONTENT status: 0x%lx\n", status);

    hr = IOleObject_QueryInterface(oleobj, &IID_IPersistStorage, (void**)&persist);
    ok(hr == S_OK, "QueryInterface(IPersistStorage) failed: %08lx\n", hr);

    hr = IPersistStorage_Load(persist, &stg);
    ok(hr == S_OK, "Load failed: %08lx\n", hr);

    if(extended){
        len = GetTempPathW(ARRAY_SIZE(filename), filename);
        lstrcpyW(filename + len, L"filename3.txt");

        file = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, NULL);
        ok(file != INVALID_HANDLE_VALUE, "Couldn't find temporary file %s: %lu\n",
                wine_dbgstr_w(filename), GetLastError());

        br = ReadFile(file, contents, sizeof(contents), &bytes_read, NULL);
        ok(br == TRUE, "ReadFile failed\n");
        ok(bytes_read == 10, "Got wrong file size: %lu\n", bytes_read);
        ok(!memcmp(contents, "some text\n", 10), "Got wrong file contents\n");

        CloseHandle(file);
    }

    hr = IOleObject_Close(oleobj, OLECLOSE_NOSAVE);
    ok(hr == S_OK, "Close failed: %08lx\n", hr);

    if(extended){
        file = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, NULL);
        ok(file != INVALID_HANDLE_VALUE, "Temporary file shouldn't be deleted\n");
        CloseHandle(file);
    }

    IPersistStorage_Release(persist);
    IOleObject_Release(oleobj);

    if(extended){
        file = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, NULL);
        ok(file == INVALID_HANDLE_VALUE, "Temporary file should be deleted\n");
    }
}

START_TEST(oleobj)
{
    CoInitialize(NULL);

    test_packager();

    CoUninitialize();
}
