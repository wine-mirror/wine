/*
 * Copyright 2018 Nikolay Sivov for CodeWeavers
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

#include "initguid.h"
#include "ole2.h"
#include "rpcproxy.h"
#include "msopc.h"
#include "xmllite.h"

#include "wine/debug.h"

#include "opc_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msopc);

struct opc_filestream
{
    IStream IStream_iface;
    LONG refcount;

    HANDLE hfile;
};

static inline struct opc_filestream *impl_from_IStream(IStream *iface)
{
    return CONTAINING_RECORD(iface, struct opc_filestream, IStream_iface);
}

static HRESULT WINAPI opc_filestream_QueryInterface(IStream *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IStream) ||
            IsEqualIID(iid, &IID_ISequentialStream) ||
            IsEqualIID(iid, &IID_IUnknown))
    {
        *out = iface;
        IStream_AddRef(iface);
        return S_OK;
    }

    *out = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI opc_filestream_AddRef(IStream *iface)
{
    struct opc_filestream *stream = impl_from_IStream(iface);
    ULONG refcount = InterlockedIncrement(&stream->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI opc_filestream_Release(IStream *iface)
{
    struct opc_filestream *stream = impl_from_IStream(iface);
    ULONG refcount = InterlockedDecrement(&stream->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        CloseHandle(stream->hfile);
        free(stream);
    }

    return refcount;
}

static HRESULT WINAPI opc_filestream_Read(IStream *iface, void *buff, ULONG size, ULONG *num_read)
{
    struct opc_filestream *stream = impl_from_IStream(iface);
    DWORD read = 0;

    TRACE("%p, %p, %lu, %p.\n", iface, buff, size, num_read);

    if (!num_read)
        num_read = &read;

    *num_read = 0;
    if (!ReadFile(stream->hfile, buff, size, num_read, NULL))
    {
        WARN("Failed to read file, error %ld.\n", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return *num_read == size ? S_OK : S_FALSE;
}

static HRESULT WINAPI opc_filestream_Write(IStream *iface, const void *data, ULONG size, ULONG *num_written)
{
    struct opc_filestream *stream = impl_from_IStream(iface);
    DWORD written = 0;

    TRACE("%p, %p, %lu, %p.\n", iface, data, size, num_written);

    if (!num_written)
        num_written = &written;

    *num_written = 0;
    if (!WriteFile(stream->hfile, data, size, num_written, NULL))
    {
        WARN("Failed to write to file, error %ld.\n", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

static HRESULT WINAPI opc_filestream_Seek(IStream *iface, LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER *newpos)
{
    struct opc_filestream *stream = impl_from_IStream(iface);

    TRACE("%p, %s, %ld, %p.\n", iface, wine_dbgstr_longlong(move.QuadPart), origin, newpos);

    if (!SetFilePointerEx(stream->hfile, move, (LARGE_INTEGER *)newpos, origin))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

static HRESULT WINAPI opc_filestream_SetSize(IStream *iface, ULARGE_INTEGER size)
{
    FIXME("iface %p, size %s stub!\n", iface, wine_dbgstr_longlong(size.QuadPart));

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_filestream_CopyTo(IStream *iface, IStream *dest, ULARGE_INTEGER size,
        ULARGE_INTEGER *num_read, ULARGE_INTEGER *written)
{
    FIXME("iface %p, dest %p, size %s, num_read %p, written %p stub!\n", iface, dest,
            wine_dbgstr_longlong(size.QuadPart), num_read, written);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_filestream_Commit(IStream *iface, DWORD flags)
{
    FIXME("%p, %#lx stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_filestream_Revert(IStream *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_filestream_LockRegion(IStream *iface, ULARGE_INTEGER offset,
        ULARGE_INTEGER size, DWORD lock_type)
{
    FIXME("%p, %s, %s, %ld stub!\n", iface, wine_dbgstr_longlong(offset.QuadPart),
            wine_dbgstr_longlong(size.QuadPart), lock_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_filestream_UnlockRegion(IStream *iface, ULARGE_INTEGER offset, ULARGE_INTEGER size,
        DWORD lock_type)
{
    FIXME("%p, %s, %s, %ld stub!\n", iface, wine_dbgstr_longlong(offset.QuadPart),
            wine_dbgstr_longlong(size.QuadPart), lock_type);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_filestream_Stat(IStream *iface, STATSTG *statstg, DWORD flag)
{
    struct opc_filestream *stream = impl_from_IStream(iface);
    BY_HANDLE_FILE_INFORMATION fi;

    TRACE("%p, %p, %#lx.\n", iface, statstg, flag);

    if (!statstg)
        return E_POINTER;

    memset(&fi, 0, sizeof(fi));
    GetFileInformationByHandle(stream->hfile, &fi);

    memset(statstg, 0, sizeof(*statstg));
    statstg->type = STGTY_STREAM;
    statstg->cbSize.u.LowPart = fi.nFileSizeLow;
    statstg->cbSize.u.HighPart = fi.nFileSizeHigh;
    statstg->mtime = fi.ftLastWriteTime;
    statstg->ctime = fi.ftCreationTime;
    statstg->atime = fi.ftLastAccessTime;

    return S_OK;
}

static HRESULT WINAPI opc_filestream_Clone(IStream *iface, IStream **result)
{
    FIXME("iface %p, result %p stub!\n", iface, result);

    return E_NOTIMPL;
}

static const IStreamVtbl opc_filestream_vtbl =
{
    opc_filestream_QueryInterface,
    opc_filestream_AddRef,
    opc_filestream_Release,
    opc_filestream_Read,
    opc_filestream_Write,
    opc_filestream_Seek,
    opc_filestream_SetSize,
    opc_filestream_CopyTo,
    opc_filestream_Commit,
    opc_filestream_Revert,
    opc_filestream_LockRegion,
    opc_filestream_UnlockRegion,
    opc_filestream_Stat,
    opc_filestream_Clone,
};

static HRESULT opc_filestream_create(const WCHAR *filename, OPC_STREAM_IO_MODE io_mode, SECURITY_ATTRIBUTES *sa,
        DWORD flags, IStream **out)
{
    struct opc_filestream *stream;
    DWORD access, creation;

    if (!filename || !out)
        return E_POINTER;

    switch (io_mode)
    {
    case OPC_STREAM_IO_READ:
        access = GENERIC_READ;
        creation = OPEN_EXISTING;
        break;
    case OPC_STREAM_IO_WRITE:
        access = GENERIC_WRITE;
        creation = CREATE_ALWAYS;
        break;
    default:
        return E_INVALIDARG;
    }

    if (!(stream = calloc(1, sizeof(*stream))))
        return E_OUTOFMEMORY;

    stream->hfile = CreateFileW(filename, access, 0, sa, creation, flags, NULL);
    if (stream->hfile == INVALID_HANDLE_VALUE)
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        free(stream);
        return hr;
    }

    stream->IStream_iface.lpVtbl = &opc_filestream_vtbl;
    stream->refcount = 1;

    *out = &stream->IStream_iface;
    TRACE("Created file stream %p.\n", *out);
    return S_OK;
}

static HRESULT WINAPI opc_factory_QueryInterface(IOpcFactory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualIID(iid, &IID_IOpcFactory) ||
            IsEqualIID(iid, &IID_IUnknown))
    {
        *out = iface;
        IOpcFactory_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI opc_factory_AddRef(IOpcFactory *iface)
{
    return 2;
}

static ULONG WINAPI opc_factory_Release(IOpcFactory *iface)
{
    return 1;
}

static HRESULT WINAPI opc_factory_CreatePackageRootUri(IOpcFactory *iface, IOpcUri **uri)
{
    TRACE("iface %p, uri %p.\n", iface, uri);

    if (!uri)
        return E_POINTER;

    return opc_root_uri_create(uri);
}

static HRESULT WINAPI opc_factory_CreatePartUri(IOpcFactory *iface, LPCWSTR uri, IOpcPartUri **out)
{
    IUri *part_uri, *root_uri, *combined;
    HRESULT hr;

    TRACE("iface %p, uri %s, out %p.\n", iface, debugstr_w(uri), out);

    if (!out)
        return E_POINTER;

    *out = NULL;

    if (FAILED(hr = CreateUri(uri, Uri_CREATE_ALLOW_RELATIVE, 0, &part_uri)))
    {
        WARN("Failed to create uri, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = CreateUri(L"/", Uri_CREATE_ALLOW_RELATIVE, 0, &root_uri)))
    {
        WARN("Failed to create root uri, hr %#lx.\n", hr);
        IUri_Release(part_uri);
        return hr;
    }

    hr = CoInternetCombineIUri(root_uri, part_uri, 0, &combined, 0);
    IUri_Release(root_uri);
    IUri_Release(part_uri);
    if (FAILED(hr))
    {
        WARN("Failed to combine URIs, hr %#lx.\n", hr);
        return hr;
    }

    hr = opc_part_uri_create(combined, NULL, out);
    IUri_Release(combined);
    return hr;
}

static HRESULT WINAPI opc_factory_CreateStreamOnFile(IOpcFactory *iface, LPCWSTR filename,
        OPC_STREAM_IO_MODE io_mode, SECURITY_ATTRIBUTES *sa, DWORD flags, IStream **stream)
{
    TRACE("%p, %s, %d, %p, %#lx, %p.\n", iface, debugstr_w(filename), io_mode, sa, flags, stream);

    return opc_filestream_create(filename, io_mode, sa, flags, stream);
}

static HRESULT WINAPI opc_factory_CreatePackage(IOpcFactory *iface, IOpcPackage **package)
{
    TRACE("iface %p, package %p.\n", iface, package);

    return opc_package_create(iface, package);
}

static HRESULT WINAPI opc_factory_ReadPackageFromStream(IOpcFactory *iface, IStream *stream,
        OPC_READ_FLAGS flags, IOpcPackage **package)
{
    FIXME("iface %p, stream %p, flags %#x, package %p stub!\n", iface, stream, flags, package);

    return E_NOTIMPL;
}

static HRESULT WINAPI opc_factory_WritePackageToStream(IOpcFactory *iface, IOpcPackage *package, OPC_WRITE_FLAGS flags,
        IStream *stream)
{
    TRACE("iface %p, package %p, flags %#x, stream %p.\n", iface, package, flags, stream);

    if (!package || !stream)
        return E_POINTER;

    return opc_package_write(package, flags, stream);
}

static HRESULT WINAPI opc_factory_CreateDigitalSignatureManager(IOpcFactory *iface, IOpcPackage *package,
        IOpcDigitalSignatureManager **signature_manager)
{
    FIXME("iface %p, package %p, signature_manager %p stub!\n", iface, package, signature_manager);

    return E_NOTIMPL;
}

static const IOpcFactoryVtbl opc_factory_vtbl =
{
    opc_factory_QueryInterface,
    opc_factory_AddRef,
    opc_factory_Release,
    opc_factory_CreatePackageRootUri,
    opc_factory_CreatePartUri,
    opc_factory_CreateStreamOnFile,
    opc_factory_CreatePackage,
    opc_factory_ReadPackageFromStream,
    opc_factory_WritePackageToStream,
    opc_factory_CreateDigitalSignatureManager,
};

static IOpcFactory opc_factory = { &opc_factory_vtbl };

static HRESULT WINAPI opc_class_factory_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IClassFactory) ||
            IsEqualGUID(iid, &IID_IUnknown))
    {
        IClassFactory_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    *out = NULL;
    WARN("Unsupported interface %s.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI opc_class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI opc_class_factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI opc_class_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    TRACE("iface %p, outer %p, iid %s, out %p.\n", iface, outer, debugstr_guid(iid), out);

    if (outer)
        return CLASS_E_NOAGGREGATION;

    return IOpcFactory_QueryInterface(&opc_factory, iid, out);
}

static HRESULT WINAPI opc_class_factory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("iface %p, dolock %d stub!\n", iface, dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl opc_class_factory_vtbl =
{
    opc_class_factory_QueryInterface,
    opc_class_factory_AddRef,
    opc_class_factory_Release,
    opc_class_factory_CreateInstance,
    opc_class_factory_LockServer
};

static IClassFactory opc_class_factory = { &opc_class_factory_vtbl };

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **out)
{
    TRACE("clsid %s, iid %s, out %p\n", debugstr_guid(clsid), debugstr_guid(iid), out);

    if (IsEqualCLSID(clsid, &CLSID_OpcFactory))
        return IClassFactory_QueryInterface(&opc_class_factory, iid, out);

    WARN("Unsupported class %s.\n", debugstr_guid(clsid));
    return E_NOTIMPL;
}
