/*
 * Copyright 2012 Alistair Leslie-Hughes
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

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "dispex.h"
#include "scrrun.h"
#include "scrrun_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(scrrun);

static HRESULT WINAPI filesys_QueryInterface(IFileSystem3 *iface, REFIID riid, void **ppvObject)
{
    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IFileSystem3 ) ||
         IsEqualGUID( riid, &IID_IFileSystem ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else if ( IsEqualGUID( riid, &IID_IDispatchEx ))
    {
        TRACE("Interface IDispatchEx not supported - returning NULL\n");
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    else if ( IsEqualGUID( riid, &IID_IObjectWithSite ))
    {
        TRACE("Interface IObjectWithSite not supported - returning NULL\n");
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IFileSystem3_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI filesys_AddRef(IFileSystem3 *iface)
{
    TRACE("%p\n", iface);

    return 2;
}

static ULONG WINAPI filesys_Release(IFileSystem3 *iface)
{
    TRACE("%p\n", iface);

    return 1;
}

static HRESULT WINAPI filesys_GetTypeInfoCount(IFileSystem3 *iface, UINT *pctinfo)
{
    TRACE("(%p)->(%p)\n", iface, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI filesys_GetTypeInfo(IFileSystem3 *iface, UINT iTInfo,
                                        LCID lcid, ITypeInfo **ppTInfo)
{
    TRACE("(%p)->(%u %u %p)\n", iface, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IFileSystem3_tid, ppTInfo);}

static HRESULT WINAPI filesys_GetIDsOfNames(IFileSystem3 *iface, REFIID riid,
                                        LPOLESTR *rgszNames, UINT cNames,
                                        LCID lcid, DISPID *rgDispId)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", iface, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IFileSystem3_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI filesys_Invoke(IFileSystem3 *iface, DISPID dispIdMember,
                                      REFIID riid, LCID lcid, WORD wFlags,
                                      DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                      EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", iface, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IFileSystem3_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI filesys_get_Drives(IFileSystem3 *iface, IDriveCollection **ppdrives)
{
    FIXME("%p %p\n", iface, ppdrives);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_BuildPath(IFileSystem3 *iface, BSTR Path,
                                            BSTR Name, BSTR *pbstrResult)
{
    FIXME("%p %s %s %p\n", iface, debugstr_w(Path), debugstr_w(Name), pbstrResult);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetDriveName(IFileSystem3 *iface, BSTR Path,
                                            BSTR *pbstrResult)
{
    FIXME("%p %s %p\n", iface, debugstr_w(Path), pbstrResult);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetParentFolderName(IFileSystem3 *iface, BSTR Path,
                                            BSTR *pbstrResult)
{
    FIXME("%p %s %p\n", iface, debugstr_w(Path), pbstrResult);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetFileName(IFileSystem3 *iface, BSTR Path,
                                            BSTR *pbstrResult)
{
    FIXME("%p %s %p\n", iface, debugstr_w(Path), pbstrResult);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetBaseName(IFileSystem3 *iface, BSTR Path,
                                            BSTR *pbstrResult)
{
    FIXME("%p %s %p\n", iface, debugstr_w(Path), pbstrResult);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetExtensionName(IFileSystem3 *iface, BSTR Path,
                                            BSTR *pbstrResult)
{
    FIXME("%p %s %p\n", iface, debugstr_w(Path), pbstrResult);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetAbsolutePathName(IFileSystem3 *iface, BSTR Path,
                                            BSTR *pbstrResult)
{
    FIXME("%p %s %p\n", iface, debugstr_w(Path), pbstrResult);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetTempName(IFileSystem3 *iface, BSTR *pbstrResult)
{
    FIXME("%p %p\n", iface, pbstrResult);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_DriveExists(IFileSystem3 *iface, BSTR DriveSpec,
                                            VARIANT_BOOL *pfExists)
{
    FIXME("%p %s %p\n", iface, debugstr_w(DriveSpec), pfExists);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_FileExists(IFileSystem3 *iface, BSTR path, VARIANT_BOOL *ret)
{
    TRACE("%p %s %p\n", iface, debugstr_w(path), ret);

    if (!ret) return E_POINTER;

    *ret = GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI filesys_FolderExists(IFileSystem3 *iface, BSTR FolderSpec,
                                            VARIANT_BOOL *pfExists)
{
    FIXME("%p %s %p\n", iface, debugstr_w(FolderSpec), pfExists);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetDrive(IFileSystem3 *iface, BSTR DriveSpec,
                                            IDrive **ppdrive)
{
    FIXME("%p %s %p\n", iface, debugstr_w(DriveSpec), ppdrive);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetFile(IFileSystem3 *iface, BSTR FilePath,
                                            IFile **ppfile)
{
    FIXME("%p %s %p\n", iface, debugstr_w(FilePath), ppfile);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetFolder(IFileSystem3 *iface, BSTR FolderPath,
                                            IFolder **ppfolder)
{
    FIXME("%p %s %p\n", iface, debugstr_w(FolderPath), ppfolder);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetSpecialFolder(IFileSystem3 *iface,
                                            SpecialFolderConst SpecialFolder,
                                            IFolder **ppfolder)
{
    FIXME("%p %d %p\n", iface, SpecialFolder, ppfolder);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_DeleteFile(IFileSystem3 *iface, BSTR FileSpec,
                                            VARIANT_BOOL Force)
{
    FIXME("%p %s %d\n", iface, debugstr_w(FileSpec), Force);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_DeleteFolder(IFileSystem3 *iface, BSTR FolderSpec,
                                            VARIANT_BOOL Force)
{
    FIXME("%p %s %d\n", iface, debugstr_w(FolderSpec), Force);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_MoveFile(IFileSystem3 *iface, BSTR Source,
                                            BSTR Destination)
{
    FIXME("%p %s %s\n", iface, debugstr_w(Source), debugstr_w(Destination));

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_MoveFolder(IFileSystem3 *iface,BSTR Source,
                                            BSTR Destination)
{
    FIXME("%p %s %s\n", iface, debugstr_w(Source), debugstr_w(Destination));

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_CopyFile(IFileSystem3 *iface, BSTR Source,
                                            BSTR Destination, VARIANT_BOOL OverWriteFiles)
{
    FIXME("%p %s %s %d\n", iface, debugstr_w(Source), debugstr_w(Destination), OverWriteFiles);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_CopyFolder(IFileSystem3 *iface, BSTR Source,
                                            BSTR Destination, VARIANT_BOOL OverWriteFiles)
{
    FIXME("%p %s %s %d\n", iface, debugstr_w(Source), debugstr_w(Destination), OverWriteFiles);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_CreateFolder(IFileSystem3 *iface, BSTR Path,
                                            IFolder **ppfolder)
{
    FIXME("%p %s %p\n", iface, debugstr_w(Path), ppfolder);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_CreateTextFile(IFileSystem3 *iface, BSTR FileName,
                                            VARIANT_BOOL Overwrite, VARIANT_BOOL Unicode,
                                            ITextStream **ppts)
{
    FIXME("%p %s %d %d %p\n", iface, debugstr_w(FileName), Overwrite, Unicode, ppts);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_OpenTextFile(IFileSystem3 *iface, BSTR FileName,
                                            IOMode IOMode, VARIANT_BOOL Create,
                                            Tristate Format, ITextStream **ppts)
{
    FIXME("%p %s %d %d %d %p\n", iface, debugstr_w(FileName), IOMode, Create, Format, ppts);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetStandardStream(IFileSystem3 *iface,
                                            StandardStreamTypes StandardStreamType,
                                            VARIANT_BOOL Unicode,
                                            ITextStream **ppts)
{
    FIXME("%p %d %d %p\n", iface, StandardStreamType, Unicode, ppts);

    return E_NOTIMPL;
}

static HRESULT WINAPI filesys_GetFileVersion(IFileSystem3 *iface, BSTR FileName,
                                            BSTR *FileVersion)
{
    FIXME("%p %s %p\n", iface, debugstr_w(FileName), FileVersion);

    return E_NOTIMPL;
}

static const struct IFileSystem3Vtbl filesys_vtbl =
{
    filesys_QueryInterface,
    filesys_AddRef,
    filesys_Release,
    filesys_GetTypeInfoCount,
    filesys_GetTypeInfo,
    filesys_GetIDsOfNames,
    filesys_Invoke,
    filesys_get_Drives,
    filesys_BuildPath,
    filesys_GetDriveName,
    filesys_GetParentFolderName,
    filesys_GetFileName,
    filesys_GetBaseName,
    filesys_GetExtensionName,
    filesys_GetAbsolutePathName,
    filesys_GetTempName,
    filesys_DriveExists,
    filesys_FileExists,
    filesys_FolderExists,
    filesys_GetDrive,
    filesys_GetFile,
    filesys_GetFolder,
    filesys_GetSpecialFolder,
    filesys_DeleteFile,
    filesys_DeleteFolder,
    filesys_MoveFile,
    filesys_MoveFolder,
    filesys_CopyFile,
    filesys_CopyFolder,
    filesys_CreateFolder,
    filesys_CreateTextFile,
    filesys_OpenTextFile,
    filesys_GetStandardStream,
    filesys_GetFileVersion
};

static IFileSystem3 filesystem = { &filesys_vtbl };

HRESULT WINAPI FileSystem_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    return IFileSystem3_QueryInterface(&filesystem, riid, ppv);
}
