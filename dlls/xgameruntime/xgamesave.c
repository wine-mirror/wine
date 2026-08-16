/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XGameSave and XGameSaveFiles
 *
 * Copyright 2026 Olivia Ryan
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

#include "private.h"
#include <shlobj.h>

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

/* Backing structs for opaque GDK handles */
struct XGameSaveProvider {
    WCHAR base_path[MAX_PATH];
};

/* Pending blob write: name + heap-copied data */
struct gamesave_blob_write {
    char  name[256];
    UINT8 *data;
    SIZE_T size;
};

/* Pending blob delete: name */
struct gamesave_blob_delete {
    char name[256];
};

struct XGameSaveContainer {
    WCHAR container_path[MAX_PATH];
};

struct XGameSaveUpdate {
    WCHAR container_path[MAX_PATH];
    struct gamesave_blob_write *writes;
    UINT32 nwrites;
    struct gamesave_blob_delete *deletes;
    UINT32 ndeletes;
};

/* Helper: build LOCALAPPDATA\WineEX\GameSave\<configId>\ into dest */
static HRESULT gamesave_make_base_path( const char *configurationId, WCHAR *dest, SIZE_T destLen )
{
    WCHAR local_app[MAX_PATH];
    if (!SHGetSpecialFolderPathW( NULL, local_app, CSIDL_LOCAL_APPDATA, TRUE ))
        return HRESULT_FROM_WIN32( GetLastError() );
    _snwprintf( dest, destLen, L"%s\\WineEX\\GameSave\\%S\\", local_app, configurationId );
    CreateDirectoryW( dest, NULL ); /* OK if already exists */
    return S_OK;
}

struct x_game_save
{
    IXGameSaveImpl3 IXGameSaveImpl3_iface;
    LONG ref;
};

static inline struct x_game_save *impl_from_IXGameSaveImpl3( IXGameSaveImpl3 *iface )
{
    return CONTAINING_RECORD( iface, struct x_game_save, IXGameSaveImpl3_iface );
}

static HRESULT WINAPI x_game_save_QueryInterface( IXGameSaveImpl3 *iface, REFIID iid, void **out )
{
    struct x_game_save *impl = impl_from_IXGameSaveImpl3( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown        ) ||
        IsEqualGUID( iid, &IID_IXGameSaveImpl  ) ||
        IsEqualGUID( iid, &IID_IXGameSaveImpl2 ) ||
        IsEqualGUID( iid, &IID_IXGameSaveImpl3 ))
    {
        IXGameSaveImpl_AddRef( *out = &impl->IXGameSaveImpl3_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_game_save_AddRef( IXGameSaveImpl3 *iface )
{
    struct x_game_save *impl = impl_from_IXGameSaveImpl3( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_game_save_Release( IXGameSaveImpl3 *iface )
{
    struct x_game_save *impl = impl_from_IXGameSaveImpl3( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

/* Helper: apply pending writes and deletes from an update context */
static HRESULT gamesave_apply_update( struct XGameSaveUpdate *upd )
{
    UINT32 i;
    WCHAR file_path[MAX_PATH];
    HANDLE hFile;
    DWORD written;

    CreateDirectoryW( upd->container_path, NULL );

    for (i = 0; i < upd->nwrites; i++) {
        _snwprintf( file_path, MAX_PATH, L"%s%S", upd->container_path, upd->writes[i].name );
        hFile = CreateFileW( file_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
        if (hFile == INVALID_HANDLE_VALUE) continue;
        WriteFile( hFile, upd->writes[i].data, (DWORD)upd->writes[i].size, &written, NULL );
        CloseHandle( hFile );
        HeapFree( GetProcessHeap(), 0, upd->writes[i].data );
    }
    HeapFree( GetProcessHeap(), 0, upd->writes );

    for (i = 0; i < upd->ndeletes; i++) {
        _snwprintf( file_path, MAX_PATH, L"%s%S", upd->container_path, upd->deletes[i].name );
        DeleteFileW( file_path );
    }
    HeapFree( GetProcessHeap(), 0, upd->deletes );

    upd->writes = NULL; upd->nwrites = 0;
    upd->deletes = NULL; upd->ndeletes = 0;
    return S_OK;
}

static HRESULT WINAPI x_game_save_XGameSaveInitializeProvider( IXGameSaveImpl3 *iface, XUserHandle requestingUser, const char *configurationId, BOOLEAN syncOnDemand, XGameSaveProviderHandle *provider )
{
    struct XGameSaveProvider *prov;
    HRESULT hr;

    TRACE( "iface %p, user %p, configId %s\n", iface, requestingUser, debugstr_a( configurationId ) );
    if (!provider) return E_INVALIDARG;
    prov = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*prov) );
    if (!prov) return E_OUTOFMEMORY;
    hr = gamesave_make_base_path( configurationId, prov->base_path, MAX_PATH );
    if (FAILED(hr)) { HeapFree( GetProcessHeap(), 0, prov ); return hr; }
    *provider = prov;
    return S_OK;
}

static HRESULT WINAPI x_game_save_XGameSaveInitializeProviderAsync( IXGameSaveImpl3 *iface, XUserHandle requestingUser, const char *configurationId, BOOLEAN syncOnDemand, XAsyncBlock *async )
{
    struct XGameSaveProvider *prov;
    HRESULT hr;

    TRACE( "iface %p, user %p, configId %s\n", iface, requestingUser, debugstr_a( configurationId ) );
    prov = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*prov) );
    if (!prov) return E_OUTOFMEMORY;
    hr = gamesave_make_base_path( configurationId, prov->base_path, MAX_PATH );
    if (FAILED(hr)) { HeapFree( GetProcessHeap(), 0, prov ); return hr; }
    return xasync_complete_inline( async, S_OK, &prov, sizeof(prov) );
}

static HRESULT WINAPI x_game_save_XGameSaveInitializeProviderResult( IXGameSaveImpl3 *iface, XAsyncBlock *async, XGameSaveProviderHandle *provider )
{
    TRACE( "iface %p, async %p\n", iface, async );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL,
        sizeof(*provider), provider, NULL );
}

static void WINAPI x_game_save_XGameSaveCloseProvider( IXGameSaveImpl3 *iface, XGameSaveProviderHandle provider )
{
    TRACE( "iface %p, provider %p\n", iface, provider );
    HeapFree( GetProcessHeap(), 0, provider );
}

static HRESULT WINAPI x_game_save_XGameSaveGetRemainingQuota( IXGameSaveImpl3 *iface, XGameSaveProviderHandle provider, INT64 *remainingQuota )
{
    TRACE( "iface %p, provider %p\n", iface, provider );
    if (remainingQuota) *remainingQuota = 500LL * 1024 * 1024; /* 500 MB */
    return S_OK;
}

static HRESULT WINAPI x_game_save_XGameSaveGetRemainingQuotaAsync( IXGameSaveImpl3 *iface, XGameSaveProviderHandle provider, XAsyncBlock *async )
{
    INT64 quota = 500LL * 1024 * 1024;
    TRACE( "iface %p, provider %p\n", iface, provider );
    return xasync_complete_inline( async, S_OK, &quota, sizeof(quota) );
}

static HRESULT WINAPI x_game_save_XGameSaveGetRemainingQuotaResult( IXGameSaveImpl3 *iface, XAsyncBlock *async, INT64 *remainingQuota )
{
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL,
        sizeof(*remainingQuota), remainingQuota, NULL );
}

static HRESULT WINAPI x_game_save_XGameSaveDeleteContainer( IXGameSaveImpl3 *iface, XGameSaveProviderHandle provider, const char *containerName )
{
    WCHAR path[MAX_PATH];
    FIXME( "iface %p, container %s — deleting directory\n", iface, debugstr_a( containerName ) );
    if (!provider) return E_INVALIDARG;
    _snwprintf( path, MAX_PATH, L"%s%S", provider->base_path, containerName );
    RemoveDirectoryW( path );
    return S_OK;
}

static HRESULT WINAPI x_game_save_XGameSaveDeleteContainerAsync( IXGameSaveImpl3 *iface, XGameSaveProviderHandle provider, const char *containerName, XAsyncBlock *async )
{
    HRESULT hr = x_game_save_XGameSaveDeleteContainer( iface, provider, containerName );
    return xasync_complete_inline( async, hr, NULL, 0 );
}

static HRESULT WINAPI x_game_save_XGameSaveDeleteContainerResult( IXGameSaveImpl3 *iface, XAsyncBlock *async )
{
    return IXThreadingImpl_XAsyncGetStatus( x_threading_impl, async, FALSE );
}

static HRESULT WINAPI x_game_save_XGameSaveGetContainerInfo( IXGameSaveImpl3 *iface, XGameSaveProviderHandle provider, const char *containerName, void *context, XGameSaveContainerInfoCallback *callback )
{
    WCHAR path[MAX_PATH];
    XGameSaveContainerInfo info = {0};
    TRACE( "iface %p, container %s\n", iface, debugstr_a( containerName ) );
    if (!provider || !callback) return E_INVALIDARG;
    info.name = containerName;
    info.displayName = containerName;
    _snwprintf( path, MAX_PATH, L"%s%S\\", provider->base_path, containerName );
    if (GetFileAttributesW( path ) == INVALID_FILE_ATTRIBUTES) return E_INVALIDARG;
    callback( &info, context );
    return S_OK;
}

static HRESULT WINAPI x_game_save_XGameSaveEnumerateContainerInfo( IXGameSaveImpl3 *iface, XGameSaveProviderHandle provider, void *context, XGameSaveContainerInfoCallback *callback )
{
    WCHAR search[MAX_PATH];
    WIN32_FIND_DATAW fd;
    HANDLE h;
    TRACE( "iface %p, provider %p\n", iface, provider );
    if (!provider || !callback) return E_INVALIDARG;
    _snwprintf( search, MAX_PATH, L"%s*", provider->base_path );
    h = FindFirstFileW( search, &fd );
    if (h == INVALID_HANDLE_VALUE) return S_OK;
    do {
        XGameSaveContainerInfo info = {0};
        char name[256];
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
        WideCharToMultiByte( CP_UTF8, 0, fd.cFileName, -1, name, sizeof(name), NULL, NULL );
        info.name = name; info.displayName = name;
        if (!callback( &info, context )) break;
    } while (FindNextFileW( h, &fd ));
    FindClose( h );
    return S_OK;
}

static HRESULT WINAPI x_game_save_XGameSaveEnumerateContainerInfoByName( IXGameSaveImpl3 *iface, XGameSaveProviderHandle provider, const char *containerNamePrefix, void *context, XGameSaveContainerInfoCallback *callback )
{
    WCHAR search[MAX_PATH];
    WIN32_FIND_DATAW fd;
    HANDLE h;
    WCHAR prefix_w[256];
    TRACE( "iface %p, prefix %s\n", iface, debugstr_a( containerNamePrefix ) );
    if (!provider || !callback) return E_INVALIDARG;
    MultiByteToWideChar( CP_UTF8, 0, containerNamePrefix, -1, prefix_w, 256 );
    _snwprintf( search, MAX_PATH, L"%s*", provider->base_path );
    h = FindFirstFileW( search, &fd );
    if (h == INVALID_HANDLE_VALUE) return S_OK;
    do {
        XGameSaveContainerInfo info = {0};
        char name[256];
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
        if (wcsncmp( fd.cFileName, prefix_w, wcslen(prefix_w) )) continue;
        WideCharToMultiByte( CP_UTF8, 0, fd.cFileName, -1, name, sizeof(name), NULL, NULL );
        info.name = name; info.displayName = name;
        if (!callback( &info, context )) break;
    } while (FindNextFileW( h, &fd ));
    FindClose( h );
    return S_OK;
}

static HRESULT WINAPI x_game_save_XGameSaveCreateContainer( IXGameSaveImpl3 *iface, XGameSaveProviderHandle provider, const char *containerName, XGameSaveContainerHandle *containerContext )
{
    struct XGameSaveContainer *cont;
    TRACE( "iface %p, container %s\n", iface, debugstr_a( containerName ) );
    if (!provider || !containerContext) return E_INVALIDARG;
    cont = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*cont) );
    if (!cont) return E_OUTOFMEMORY;
    _snwprintf( cont->container_path, MAX_PATH, L"%s%S\\", provider->base_path, containerName );
    CreateDirectoryW( cont->container_path, NULL );
    *containerContext = cont;
    return S_OK;
}

static void WINAPI x_game_save_XGameSaveCloseContainer( IXGameSaveImpl3 *iface, XGameSaveContainerHandle context )
{
    TRACE( "iface %p, context %p\n", iface, context );
    HeapFree( GetProcessHeap(), 0, context );
}

static HRESULT WINAPI x_game_save_XGameSaveEnumerateBlobInfo( IXGameSaveImpl3 *iface, XGameSaveContainerHandle container, void *context, XGameSaveBlobInfoCallback *callback )
{
    WCHAR search[MAX_PATH];
    WIN32_FIND_DATAW fd;
    HANDLE h;
    TRACE( "iface %p, container %p\n", iface, container );
    if (!container || !callback) return E_INVALIDARG;
    _snwprintf( search, MAX_PATH, L"%s*", container->container_path );
    h = FindFirstFileW( search, &fd );
    if (h == INVALID_HANDLE_VALUE) return S_OK;
    do {
        XGameSaveBlobInfo info;
        char name[256];
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        WideCharToMultiByte( CP_UTF8, 0, fd.cFileName, -1, name, sizeof(name), NULL, NULL );
        info.name = name;
        info.size = fd.nFileSizeLow;
        if (!callback( &info, context )) break;
    } while (FindNextFileW( h, &fd ));
    FindClose( h );
    return S_OK;
}

static HRESULT WINAPI x_game_save_XGameSaveEnumerateBlobInfoByName( IXGameSaveImpl3 *iface, XGameSaveContainerHandle container, const char *blobNamePrefix, void *context, XGameSaveBlobInfoCallback *callback )
{
    WCHAR search[MAX_PATH], prefix_w[256];
    WIN32_FIND_DATAW fd;
    HANDLE h;
    TRACE( "iface %p, container %p, prefix %s\n", iface, container, debugstr_a( blobNamePrefix ) );
    if (!container || !callback) return E_INVALIDARG;
    MultiByteToWideChar( CP_UTF8, 0, blobNamePrefix, -1, prefix_w, 256 );
    _snwprintf( search, MAX_PATH, L"%s*", container->container_path );
    h = FindFirstFileW( search, &fd );
    if (h == INVALID_HANDLE_VALUE) return S_OK;
    do {
        XGameSaveBlobInfo info;
        char name[256];
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (wcsncmp( fd.cFileName, prefix_w, wcslen(prefix_w) )) continue;
        WideCharToMultiByte( CP_UTF8, 0, fd.cFileName, -1, name, sizeof(name), NULL, NULL );
        info.name = name; info.size = fd.nFileSizeLow;
        if (!callback( &info, context )) break;
    } while (FindNextFileW( h, &fd ));
    FindClose( h );
    return S_OK;
}

static HRESULT WINAPI x_game_save_XGameSaveReadBlobData( IXGameSaveImpl3 *iface, XGameSaveContainerHandle container, const char **blobNames, UINT32 *countOfBlobs, SIZE_T blobsSize, XGameSaveBlob *blobData )
{
    UINT32 i;
    TRACE( "iface %p, container %p, count %u\n", iface, container, *countOfBlobs );
    if (!container || !blobNames || !countOfBlobs || !blobData) return E_INVALIDARG;
    for (i = 0; i < *countOfBlobs; i++) {
        WCHAR path[MAX_PATH];
        HANDLE hFile;
        DWORD read;
        _snwprintf( path, MAX_PATH, L"%s%S", container->container_path, blobNames[i] );
        blobData[i].info.name = blobNames[i];
        blobData[i].data = NULL;
        hFile = CreateFileW( path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
        if (hFile == INVALID_HANDLE_VALUE) { blobData[i].info.size = 0; continue; }
        blobData[i].info.size = GetFileSize( hFile, NULL );
        if (blobData[i].info.size <= blobsSize) {
            blobData[i].data = (UINT8 *)blobData + sizeof(*blobData) * (*countOfBlobs) + i * blobData[i].info.size;
            ReadFile( hFile, blobData[i].data, blobData[i].info.size, &read, NULL );
        }
        CloseHandle( hFile );
    }
    return S_OK;
}

static HRESULT WINAPI x_game_save_XGameSaveReadBlobDataAsync( IXGameSaveImpl3 *iface, XGameSaveContainerHandle container, const char **blobNames, UINT32 countOfBlobs, XAsyncBlock *async )
{
    FIXME( "iface %p, container %p stub — async blob read, completing with E_NOTIMPL\n", iface, container );
    return xasync_complete_inline( async, E_NOTIMPL, NULL, 0 );
}

static HRESULT WINAPI x_game_save_XGameSaveReadBlobDataResult( IXGameSaveImpl3 *iface, XAsyncBlock *async, SIZE_T blobsSize, XGameSaveBlob *blobData, UINT32 *countOfBlobs )
{
    return IXThreadingImpl_XAsyncGetStatus( x_threading_impl, async, FALSE );
}

static HRESULT WINAPI x_game_save_XGameSaveCreateUpdate( IXGameSaveImpl3 *iface, XGameSaveContainerHandle container, const char *containerDisplayName, XGameSaveUpdateHandle *updateContext )
{
    struct XGameSaveUpdate *upd;
    TRACE( "iface %p, container %p, displayName %s\n", iface, container, debugstr_a( containerDisplayName ) );
    if (!container || !updateContext) return E_INVALIDARG;
    upd = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*upd) );
    if (!upd) return E_OUTOFMEMORY;
    wcscpy( upd->container_path, container->container_path );
    *updateContext = upd;
    return S_OK;
}

static void WINAPI x_game_save_XGameSaveCloseUpdate( IXGameSaveImpl3 *iface, XGameSaveUpdateHandle context )
{
    UINT32 i;
    struct XGameSaveUpdate *upd = (struct XGameSaveUpdate *)context;
    TRACE( "iface %p, context %p\n", iface, context );
    if (!upd) return;
    for (i = 0; i < upd->nwrites; i++) HeapFree( GetProcessHeap(), 0, upd->writes[i].data );
    HeapFree( GetProcessHeap(), 0, upd->writes );
    HeapFree( GetProcessHeap(), 0, upd->deletes );
    HeapFree( GetProcessHeap(), 0, upd );
}

static HRESULT WINAPI x_game_save_XGameSaveSubmitBlobWrite( IXGameSaveImpl3 *iface, XGameSaveUpdateHandle updateContext, const char *blobName, UINT8 *data, SIZE_T byteCount )
{
    struct XGameSaveUpdate *upd = (struct XGameSaveUpdate *)updateContext;
    struct gamesave_blob_write *entry;
    TRACE( "iface %p, blob %s, size %Iu\n", iface, debugstr_a( blobName ), byteCount );
    if (!upd || !blobName || !data) return E_INVALIDARG;
    upd->writes = HeapReAlloc( GetProcessHeap(), 0, upd->writes, (upd->nwrites + 1) * sizeof(*upd->writes) );
    if (!upd->writes) return E_OUTOFMEMORY;
    entry = &upd->writes[upd->nwrites++];
    lstrcpynA( entry->name, blobName, 256 );
    entry->data = HeapAlloc( GetProcessHeap(), 0, byteCount );
    if (!entry->data) { upd->nwrites--; return E_OUTOFMEMORY; }
    memcpy( entry->data, data, byteCount );
    entry->size = byteCount;
    return S_OK;
}

static HRESULT WINAPI x_game_save_XGameSaveSubmitBlobDelete( IXGameSaveImpl3 *iface, XGameSaveUpdateHandle updateContext, const char *blobName )
{
    struct XGameSaveUpdate *upd = (struct XGameSaveUpdate *)updateContext;
    TRACE( "iface %p, blob %s\n", iface, debugstr_a( blobName ) );
    if (!upd || !blobName) return E_INVALIDARG;
    upd->deletes = HeapReAlloc( GetProcessHeap(), 0, upd->deletes, (upd->ndeletes + 1) * sizeof(*upd->deletes) );
    if (!upd->deletes) return E_OUTOFMEMORY;
    lstrcpynA( upd->deletes[upd->ndeletes++].name, blobName, 256 );
    return S_OK;
}

static HRESULT WINAPI x_game_save_XGameSaveSubmitUpdate( IXGameSaveImpl3 *iface, XGameSaveUpdateHandle updateContext )
{
    TRACE( "iface %p, context %p\n", iface, updateContext );
    if (!updateContext) return E_INVALIDARG;
    return gamesave_apply_update( (struct XGameSaveUpdate *)updateContext );
}

static HRESULT WINAPI x_game_save_XGameSaveSubmitUpdateAsync( IXGameSaveImpl3 *iface, XGameSaveUpdateHandle updateContext, XAsyncBlock *async )
{
    HRESULT hr = x_game_save_XGameSaveSubmitUpdate( iface, updateContext );
    return xasync_complete_inline( async, hr, NULL, 0 );
}

static HRESULT WINAPI x_game_save_XGameSaveSubmitUpdateResult( IXGameSaveImpl3 *iface, XAsyncBlock *async )
{
    return IXThreadingImpl_XAsyncGetStatus( x_threading_impl, async, FALSE );
}

static HRESULT WINAPI x_game_save_XGameSaveFilesGetFolderWithUiAsync( IXGameSaveImpl3 *iface, XUserHandle requestingUser, const char *configurationId, XAsyncBlock *async )
{
    WCHAR base[MAX_PATH];
    char base_a[MAX_PATH];
    SIZE_T len;
    FIXME( "iface %p, configId %s stub\n", iface, debugstr_a( configurationId ) );
    gamesave_make_base_path( configurationId, base, MAX_PATH );
    WideCharToMultiByte( CP_UTF8, 0, base, -1, base_a, sizeof(base_a), NULL, NULL );
    len = strlen( base_a ) + 1;
    return xasync_complete_inline( async, S_OK, base_a, len );
}

static HRESULT WINAPI x_game_save_XGameSaveFilesGetFolderWithUiResult( IXGameSaveImpl3 *iface, XAsyncBlock *async, SIZE_T folderSize, char *folderResult )
{
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL,
        folderSize, folderResult, NULL );
}

static HRESULT WINAPI x_game_save_XGameSaveFilesGetRemainingQuota( IXGameSaveImpl3 *iface, XUserHandle userContext, const char *configurationId, INT64 *remainingQuota )
{
    TRACE( "iface %p, configId %s\n", iface, debugstr_a( configurationId ) );
    if (remainingQuota) *remainingQuota = 500LL * 1024 * 1024;
    return S_OK;
}

static const struct IXGameSaveImpl3Vtbl x_game_save_vtbl =
{
    x_game_save_QueryInterface,
    x_game_save_AddRef,
    x_game_save_Release,
    /* IXGameSaveImpl methods */
    x_game_save_XGameSaveInitializeProvider,
    x_game_save_XGameSaveInitializeProviderAsync,
    x_game_save_XGameSaveInitializeProviderResult,
    x_game_save_XGameSaveCloseProvider,
    x_game_save_XGameSaveGetRemainingQuota,
    x_game_save_XGameSaveGetRemainingQuotaAsync,
    x_game_save_XGameSaveGetRemainingQuotaResult,
    x_game_save_XGameSaveDeleteContainer,
    x_game_save_XGameSaveDeleteContainerAsync,
    x_game_save_XGameSaveDeleteContainerResult,
    x_game_save_XGameSaveGetContainerInfo,
    x_game_save_XGameSaveEnumerateContainerInfo,
    x_game_save_XGameSaveEnumerateContainerInfoByName,
    x_game_save_XGameSaveCreateContainer,
    x_game_save_XGameSaveCloseContainer,
    x_game_save_XGameSaveEnumerateBlobInfo,
    x_game_save_XGameSaveEnumerateBlobInfoByName,
    x_game_save_XGameSaveReadBlobData,
    x_game_save_XGameSaveReadBlobDataAsync,
    x_game_save_XGameSaveReadBlobDataResult,
    x_game_save_XGameSaveCreateUpdate,
    x_game_save_XGameSaveCloseUpdate,
    x_game_save_XGameSaveSubmitBlobWrite,
    x_game_save_XGameSaveSubmitBlobDelete,
    x_game_save_XGameSaveSubmitUpdate,
    x_game_save_XGameSaveSubmitUpdateAsync,
    x_game_save_XGameSaveSubmitUpdateResult,
    /* IXGameSaveImpl2 methods */
    x_game_save_XGameSaveFilesGetFolderWithUiAsync,
    x_game_save_XGameSaveFilesGetFolderWithUiResult,
    x_game_save_XGameSaveFilesGetRemainingQuota,
};

static struct x_game_save x_game_save =
{
    {&x_game_save_vtbl},
    0,
};

IXGameSaveImpl *x_game_save_impl = (IXGameSaveImpl *)&x_game_save.IXGameSaveImpl3_iface;
