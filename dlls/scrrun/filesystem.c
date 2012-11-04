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
#include "olectl.h"
#include "dispex.h"
#include "scrrun.h"
#include "scrrun_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(scrrun);

struct folder {
    IFolder IFolder_iface;
    LONG ref;
};

static inline struct folder *impl_from_IFolder(IFolder *iface)
{
    return CONTAINING_RECORD(iface, struct folder, IFolder_iface);
}

static HRESULT WINAPI folder_QueryInterface(IFolder *iface, REFIID riid, void **obj)
{
    struct folder *This = impl_from_IFolder(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualGUID( riid, &IID_IFolder ) ||
        IsEqualGUID( riid, &IID_IUnknown))
    {
        *obj = iface;
        IFolder_AddRef(iface);
    }
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI folder_AddRef(IFolder *iface)
{
    struct folder *This = impl_from_IFolder(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI folder_Release(IFolder *iface)
{
    struct folder *This = impl_from_IFolder(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI folder_GetTypeInfoCount(IFolder *iface, UINT *pctinfo)
{
    struct folder *This = impl_from_IFolder(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI folder_GetTypeInfo(IFolder *iface, UINT iTInfo,
                                        LCID lcid, ITypeInfo **ppTInfo)
{
    struct folder *This = impl_from_IFolder(iface);
    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IFolder_tid, ppTInfo);
}

static HRESULT WINAPI folder_GetIDsOfNames(IFolder *iface, REFIID riid,
                                        LPOLESTR *rgszNames, UINT cNames,
                                        LCID lcid, DISPID *rgDispId)
{
    struct folder *This = impl_from_IFolder(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IFolder_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI folder_Invoke(IFolder *iface, DISPID dispIdMember,
                                      REFIID riid, LCID lcid, WORD wFlags,
                                      DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                      EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    struct folder *This = impl_from_IFolder(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IFolder_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI folder_get_Path(IFolder *iface, BSTR *path)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, path);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_Name(IFolder *iface, BSTR *name)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_put_Name(IFolder *iface, BSTR name)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_ShortPath(IFolder *iface, BSTR *path)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, path);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_ShortName(IFolder *iface, BSTR *name)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_Drive(IFolder *iface, IDrive **drive)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, drive);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_ParentFolder(IFolder *iface, IFolder **parent)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, parent);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_Attributes(IFolder *iface, FileAttribute *attr)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, attr);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_put_Attributes(IFolder *iface, FileAttribute attr)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(0x%x): stub\n", This, attr);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_DateCreated(IFolder *iface, DATE *date)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, date);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_DateLastModified(IFolder *iface, DATE *date)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, date);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_DateLastAccessed(IFolder *iface, DATE *date)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, date);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_Type(IFolder *iface, BSTR *type)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_Delete(IFolder *iface, VARIANT_BOOL force)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%x): stub\n", This, force);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_Copy(IFolder *iface, BSTR dest, VARIANT_BOOL overwrite)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%s %x): stub\n", This, debugstr_w(dest), overwrite);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_Move(IFolder *iface, BSTR dest)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%s): stub\n", This, debugstr_w(dest));
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_IsRootFolder(IFolder *iface, VARIANT_BOOL *isroot)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, isroot);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_Size(IFolder *iface, VARIANT *size)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_SubFolders(IFolder *iface, IFolderCollection **folders)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, folders);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_get_Files(IFolder *iface, IFileCollection **files)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%p): stub\n", This, files);
    return E_NOTIMPL;
}

static HRESULT WINAPI folder_CreateTextFile(IFolder *iface, BSTR filename, VARIANT_BOOL overwrite,
    VARIANT_BOOL unicode, ITextStream **stream)
{
    struct folder *This = impl_from_IFolder(iface);
    FIXME("(%p)->(%s %x %x %p): stub\n", This, debugstr_w(filename), overwrite, unicode, stream);
    return E_NOTIMPL;
}

static const IFolderVtbl foldervtbl = {
    folder_QueryInterface,
    folder_AddRef,
    folder_Release,
    folder_GetTypeInfoCount,
    folder_GetTypeInfo,
    folder_GetIDsOfNames,
    folder_Invoke,
    folder_get_Path,
    folder_get_Name,
    folder_put_Name,
    folder_get_ShortPath,
    folder_get_ShortName,
    folder_get_Drive,
    folder_get_ParentFolder,
    folder_get_Attributes,
    folder_put_Attributes,
    folder_get_DateCreated,
    folder_get_DateLastModified,
    folder_get_DateLastAccessed,
    folder_get_Type,
    folder_Delete,
    folder_Copy,
    folder_Move,
    folder_get_IsRootFolder,
    folder_get_Size,
    folder_get_SubFolders,
    folder_get_Files,
    folder_CreateTextFile
};

static HRESULT create_folder(IFolder **folder)
{
    struct folder *This;

    This = heap_alloc(sizeof(struct folder));
    if (!This) return E_OUTOFMEMORY;

    This->IFolder_iface.lpVtbl = &foldervtbl;
    This->ref = 1;

    *folder = &This->IFolder_iface;

    return S_OK;
}

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
    return get_typeinfo(IFileSystem3_tid, ppTInfo);
}

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
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags,
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
    DWORD attrs;
    TRACE("%p %s %p\n", iface, debugstr_w(path), ret);

    if (!ret) return E_POINTER;

    attrs = GetFileAttributesW(path);
    *ret = attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI filesys_FolderExists(IFileSystem3 *iface, BSTR path, VARIANT_BOOL *ret)
{
    DWORD attrs;
    TRACE("%p %s %p\n", iface, debugstr_w(path), ret);

    if (!ret) return E_POINTER;

    attrs = GetFileAttributesW(path);
    *ret = attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) ? VARIANT_TRUE : VARIANT_FALSE;

    return S_OK;
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

static HRESULT WINAPI filesys_CreateFolder(IFileSystem3 *iface, BSTR path,
                                            IFolder **folder)
{
    BOOL ret;

    TRACE("(%p)->(%s %p)\n", iface, debugstr_w(path), folder);

    ret = CreateDirectoryW(path, NULL);
    if (!ret)
    {
        *folder = NULL;
        if (GetLastError() == ERROR_ALREADY_EXISTS) return CTL_E_FILEALREADYEXISTS;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return create_folder(folder);
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
