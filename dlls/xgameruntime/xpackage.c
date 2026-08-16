/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XPackage
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

WINE_DEFAULT_DEBUG_CHANNEL(gdkc);

static HRESULT xpackage_complete_inline( XAsyncBlock *async, HRESULT hr, const void *data, SIZE_T size )
{
    struct xasync_state *state = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*state) );
    if (!state) return E_OUTOFMEMORY;
    state->result    = hr;
    state->completed = TRUE;
    if (size && data) {
        state->result_buf = HeapAlloc( GetProcessHeap(), 0, size );
        if (!state->result_buf) { HeapFree( GetProcessHeap(), 0, state ); return E_OUTOFMEMORY; }
        memcpy( state->result_buf, data, size );
        state->result_size = size;
    }
    async->internal[0] = state;
    if (async->callback) async->callback( async );
    return S_OK;
}

struct x_package
{
    IXPackageImpl4 IXPackageImpl4_iface;
    LONG ref;
};

static inline struct x_package *impl_from_IXPackageImpl4( IXPackageImpl4 *iface )
{
    return CONTAINING_RECORD( iface, struct x_package, IXPackageImpl4_iface );
}

static HRESULT WINAPI x_package_QueryInterface( IXPackageImpl4 *iface, REFIID iid, void **out )
{
    struct x_package *impl = impl_from_IXPackageImpl4( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown       ) ||
        IsEqualGUID( iid, &IID_IXPackageImpl  ) ||
        IsEqualGUID( iid, &IID_IXPackageImpl2 ) ||
        IsEqualGUID( iid, &IID_IXPackageImpl3 ) ||
        IsEqualGUID( iid, &IID_IXPackageImpl4 ))
    {
        IXPackageImpl_AddRef( *out = &impl->IXPackageImpl4_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI x_package_AddRef( IXPackageImpl4 *iface )
{
    struct x_package *impl = impl_from_IXPackageImpl4( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI x_package_Release( IXPackageImpl4 *iface )
{
    struct x_package *impl = impl_from_IXPackageImpl4( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI x_package_XPackageGetCurrentProcessPackageIdentifier( IXPackageImpl4 *iface, SIZE_T bufferSize, char *buffer )
{
    static const char id[] = "WineEX_Package_1.0.0.0_x64";
    TRACE( "iface %p, bufferSize %Iu, buffer %p\n", iface, bufferSize, buffer );
    if (bufferSize < sizeof(id)) return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    memcpy( buffer, id, sizeof(id) );
    return S_OK;
}

static BOOLEAN WINAPI x_package_XPackageIsPackagedProcess( IXPackageImpl4 *iface )
{
    TRACE( "iface %p\n", iface );
    return FALSE;
}

static HRESULT WINAPI x_package_XPackageCreateInstallationMonitor( IXPackageImpl4 *iface, const char *packageIdentifier, UINT32 selectorCount, XPackageChunkSelector *selectors, UINT32 minimumUpdateIntervalMs, XTaskQueueHandle queue, XPackageInstallationMonitorHandle *installationMonitor )
{
    TRACE( "iface %p, packageIdentifier %s, selectorCount %u\n", iface, debugstr_a( packageIdentifier ), selectorCount );
    *installationMonitor = (XPackageInstallationMonitorHandle)(ULONG_PTR)0x1;
    return S_OK;
}

static void WINAPI x_package_XPackageCloseInstallationMonitorHandle( IXPackageImpl4 *iface, XPackageInstallationMonitorHandle installationMonitor )
{
    TRACE( "iface %p, installationMonitor %p\n", iface, installationMonitor );
}

static void WINAPI x_package_XPackageGetInstallationProgress( IXPackageImpl4 *iface, XPackageInstallationMonitorHandle installationMonitor, XPackageInstallationProgress *progress )
{
    TRACE( "iface %p, installationMonitor %p, progress %p\n", iface, installationMonitor, progress );
    if (!progress) return;
    memset( progress, 0, sizeof(*progress) );
    progress->totalBytes     = 1;
    progress->installedBytes = 1;
    progress->launchBytes    = 1;
    progress->launchable     = TRUE;
    progress->completed      = TRUE;
}

static BOOLEAN WINAPI x_package_XPackageUpdateInstallationMonitor( IXPackageImpl4 *iface, XPackageInstallationMonitorHandle installationMonitor )
{
    TRACE( "iface %p, installationMonitor %p\n", iface, installationMonitor );
    return TRUE;
}

static HRESULT WINAPI x_package_XPackageRegisterInstallationProgressChanged( IXPackageImpl4 *iface, XPackageInstallationMonitorHandle installationMonitor, void *context, XPackageInstallationProgressCallback *callback, XTaskQueueRegistrationToken *token )
{
    TRACE( "iface %p, installationMonitor %p, token %p\n", iface, installationMonitor, token );
    token->value = 1;
    return S_OK;
}

static BOOLEAN WINAPI x_package_XPackageUnregisterInstallationProgressChanged( IXPackageImpl4 *iface, XPackageInstallationMonitorHandle installationMonitor, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    TRACE( "iface %p, installationMonitor %p\n", iface, installationMonitor );
    return TRUE;
}

static HRESULT WINAPI x_package_XPackageGetUserLocale( IXPackageImpl4 *iface, SIZE_T localeSize, char *locale )
{
    static const char en[] = "en-US";
    TRACE( "iface %p, localeSize %Iu, locale %p\n", iface, localeSize, locale );
    if (localeSize < sizeof(en)) return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    memcpy( locale, en, sizeof(en) );
    return S_OK;
}

static HRESULT WINAPI x_package_XPackageFindChunkAvailability( IXPackageImpl4 *iface, const char *packageIdentifier, UINT32 selectorCount, XPackageChunkSelector *selectors, XPackageChunkAvailability *availability )
{
    TRACE( "iface %p, packageIdentifier %s, selectorCount %u\n", iface, packageIdentifier, selectorCount );
    if (availability) *availability = XPackageChunkAvailability_Ready;
    return S_OK;
}

static HRESULT WINAPI x_package_XPackageEnumerateChunkAvailability( IXPackageImpl4 *iface, const char *packageIdentifier, XPackageChunkSelectorType type, void *context, XPackageChunkAvailabilityCallback *callback )
{
    TRACE( "iface %p, packageIdentifier %s — all chunks ready\n", iface, debugstr_a( packageIdentifier ) );
    return S_OK; /* no chunks to enumerate */
}

static HRESULT WINAPI x_package_XPackageChangeChunkInstallOrder( IXPackageImpl4 *iface, const char *packageIdentifier, UINT32 selectorCount, XPackageChunkSelector *selectors )
{
    TRACE( "iface %p, packageIdentifier %s, selectorCount %u\n", iface, debugstr_a( packageIdentifier ), selectorCount );
    return S_OK;
}

static HRESULT WINAPI x_package_XPackageInstallChunks( IXPackageImpl4 *iface, const char *packageIdentifier, UINT32 selectorCount, XPackageChunkSelector *selectors, UINT32 minimumUpdateIntervalMs, BOOLEAN suppressUserConfirmation, XTaskQueueHandle queue, XPackageInstallationMonitorHandle *installationMonitor )
{
    TRACE( "iface %p, packageIdentifier %s — all chunks local\n", iface, debugstr_a( packageIdentifier ) );
    *installationMonitor = (XPackageInstallationMonitorHandle)(ULONG_PTR)0x1;
    return S_OK;
}

static HRESULT WINAPI x_package_XPackageInstallChunksAsync( IXPackageImpl4 *iface, const char *packageIdentifier, UINT32 selectorCount, XPackageChunkSelector *selectors, UINT32 minimumUpdateIntervalMs, BOOLEAN suppressUserConfirmation, XAsyncBlock *asyncBlock )
{
    XPackageInstallationMonitorHandle sentinel = (XPackageInstallationMonitorHandle)(ULONG_PTR)0x1;
    TRACE( "iface %p, packageIdentifier %s, asyncBlock %p\n", iface, packageIdentifier, asyncBlock );
    return xpackage_complete_inline( asyncBlock, S_OK, &sentinel, sizeof(sentinel) );
}

static HRESULT WINAPI x_package_XPackageInstallChunksResult( IXPackageImpl4 *iface, XAsyncBlock *asyncBlock, XPackageInstallationMonitorHandle *installationMonitor )
{
    TRACE( "iface %p, asyncBlock %p\n", iface, asyncBlock );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, asyncBlock, NULL, sizeof(*installationMonitor), installationMonitor, NULL );
}

static HRESULT WINAPI x_package_XPackageEstimateDownloadSize( IXPackageImpl4 *iface, const char *packageIdentifier, UINT32 selectorCount, XPackageChunkSelector *selectors, UINT64 *downloadSize, BOOLEAN *shouldPresentUserConfirmation )
{
    TRACE( "iface %p, packageIdentifier %s\n", iface, packageIdentifier );
    if (downloadSize) *downloadSize = 0;
    if (shouldPresentUserConfirmation) *shouldPresentUserConfirmation = FALSE;
    return S_OK;
}

static HRESULT WINAPI x_package_XPackageUninstallChunks( IXPackageImpl4 *iface, const char *packageIdentifier, UINT32 selectorCount, XPackageChunkSelector *selectors )
{
    TRACE( "iface %p, packageIdentifier %s\n", iface, packageIdentifier );
    return S_OK;
}

static HRESULT WINAPI __PADDING__( IXPackageImpl4 *iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does.\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI __PADDING_2__( IXPackageImpl4 *iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does.\n", iface );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_package_XPackageUnregisterPackageInstalled( IXPackageImpl4 *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    TRACE( "iface %p\n", iface );
    return TRUE;
}

static HRESULT WINAPI __PADDING_3__( IXPackageImpl4 *iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does.\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageGetMountPathSize( IXPackageImpl4 *iface, XPackageMountHandle mount, SIZE_T *pathSize )
{
    char buf[MAX_PATH];
    TRACE( "iface %p, mount %p, pathSize %p\n", iface, mount, pathSize );
    if (!GetCurrentDirectoryA( MAX_PATH, buf )) return HRESULT_FROM_WIN32( GetLastError() );
    *pathSize = strlen( buf ) + 1;
    return S_OK;
}

static HRESULT WINAPI x_package_XPackageGetMountPath( IXPackageImpl4 *iface, XPackageMountHandle mount, SIZE_T pathSize, char *path )
{
    TRACE( "iface %p, mount %p, pathSize %Iu, path %p\n", iface, mount, pathSize, path );
    if (!GetCurrentDirectoryA( (DWORD)pathSize, path )) return HRESULT_FROM_WIN32( GetLastError() );
    return S_OK;
}

static void WINAPI x_package_XPackageCloseMountHandle( IXPackageImpl4 *iface, XPackageMountHandle mount )
{
    TRACE( "iface %p, mount %p\n", iface, mount );
}

static HRESULT WINAPI __PADDING_4__( IXPackageImpl4 *iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does.\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageEnumeratePackages( IXPackageImpl4 *iface, XPackageKind kind, XPackageEnumerationScope scope, void *context, XPackageEnumerationCallback *callback )
{
    TRACE( "iface %p, kind %d, scope %d — empty enumeration\n", iface, kind, scope );
    return S_OK;
}

static HRESULT WINAPI x_package_XPackageRegisterPackageInstalled( IXPackageImpl4 *iface, XTaskQueueHandle queue, void *context, XPackageInstalledCallback *callback, XTaskQueueRegistrationToken *token )
{
    TRACE( "iface %p, queue %p, token %p\n", iface, queue, token );
    token->value = 1;
    return S_OK;
}

static HRESULT WINAPI x_package_XPackageGetWriteStats( IXPackageImpl4 *iface, XPackageWriteStats *writeStats )
{
    TRACE( "iface %p, writeStats %p\n", iface, writeStats );
    if (writeStats) memset( writeStats, 0, sizeof(*writeStats) );
    return S_OK;
}

static HRESULT WINAPI __PADDING_5__( IXPackageImpl4 *iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does.\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageUninstallUWPInstance( IXPackageImpl4 *iface, const char *packageName )
{
    FIXME( "iface %p, packageName %s stub!\n", iface, debugstr_a( packageName ) );
    return E_NOTIMPL; /* can't uninstall what we didn't install */
}

static HRESULT WINAPI x_package_XPackageEnumerateFeatures( IXPackageImpl4 *iface, const char *packageIdentifier, void *context, XPackageFeatureEnumerationCallback *callback )
{
    TRACE( "iface %p, packageIdentifier %s — empty\n", iface, packageIdentifier );
    return S_OK;
}

static BOOLEAN WINAPI x_package_XPackageUninstallPackage( IXPackageImpl4 *iface, const char *packageIdentifier )
{
    FIXME( "iface %p, packageIdentifier %s stub!\n", iface, debugstr_a( packageIdentifier ) );
    return FALSE;
}

static HRESULT WINAPI x_package_XPackageEnumeratePackages2( IXPackageImpl4 *iface, XPackageKind kind, XPackageEnumerationScope scope, void *context, XPackageEnumerationCallback *callback )
{
    TRACE( "iface %p, kind %d, scope %d — empty enumeration\n", iface, kind, scope );
    return S_OK;
}

static HRESULT WINAPI x_package_XPackageRegisterPackageInstalled2( IXPackageImpl4 *iface, XTaskQueueHandle queue, void *context, XPackageInstalledCallback *callback, XTaskQueueRegistrationToken *token )
{
    TRACE( "iface %p, queue %p, token %p\n", iface, queue, token );
    token->value = 1;
    return S_OK;
}

static HRESULT WINAPI x_package_XPackageMountWithUiAsync( IXPackageImpl4 *iface, const char *packageIdentifier, XAsyncBlock *async )
{
    XPackageMountHandle sentinel = (XPackageMountHandle)(ULONG_PTR)0x1;
    TRACE( "iface %p, packageIdentifier %s, async %p\n", iface, debugstr_a( packageIdentifier ), async );
    return xpackage_complete_inline( async, S_OK, &sentinel, sizeof(sentinel) );
}

static HRESULT WINAPI x_package_XPackageMountWithUiResult( IXPackageImpl4 *iface, XAsyncBlock *async, XPackageMountHandle *mount )
{
    TRACE( "iface %p, async %p, mount %p\n", iface, async, mount );
    return IXThreadingImpl_XAsyncGetResult( x_threading_impl, async, NULL, sizeof(*mount), mount, NULL );
}

static HRESULT WINAPI x_package_XPackageEnumeratePackages3( IXPackageImpl4 *iface, XPackageKind kind, XPackageEnumerationScope scope, void *context, XPackageEnumerationCallback *callback )
{
    TRACE( "iface %p, kind %d, scope %d — empty enumeration\n", iface, kind, scope );
    return S_OK;
}

static HRESULT WINAPI x_package_XPackageRegisterPackageInstalled3( IXPackageImpl4 *iface, XTaskQueueHandle queue, void *context, XPackageInstalledCallback *callback, XTaskQueueRegistrationToken *token )
{
    TRACE( "iface %p, queue %p, token %p\n", iface, queue, token );
    token->value = 1;
    return S_OK;
}

static HRESULT WINAPI x_package_XPackageGetPackageKind( IXPackageImpl4 *iface, const char *packageIdentifier, XPackageKind *kind )
{
    TRACE( "iface %p, packageIdentifier %s\n", iface, debugstr_a( packageIdentifier ) );
    if (kind) *kind = XPackageKind_Game;
    return S_OK;
}

static const struct IXPackageImpl4Vtbl x_package_vtbl =
{
    x_package_QueryInterface,
    x_package_AddRef,
    x_package_Release,
    /* IXPackageImpl methods */
    x_package_XPackageGetCurrentProcessPackageIdentifier,
    x_package_XPackageIsPackagedProcess,
    x_package_XPackageCreateInstallationMonitor,
    x_package_XPackageCloseInstallationMonitorHandle,
    x_package_XPackageGetInstallationProgress,
    x_package_XPackageUpdateInstallationMonitor,
    x_package_XPackageRegisterInstallationProgressChanged,
    x_package_XPackageUnregisterInstallationProgressChanged,
    x_package_XPackageGetUserLocale,
    x_package_XPackageFindChunkAvailability,
    x_package_XPackageEnumerateChunkAvailability,
    x_package_XPackageChangeChunkInstallOrder,
    x_package_XPackageInstallChunks,
    x_package_XPackageInstallChunksAsync,
    x_package_XPackageInstallChunksResult,
    x_package_XPackageEstimateDownloadSize,
    x_package_XPackageUninstallChunks,
    __PADDING__,
    __PADDING_2__,
    x_package_XPackageUnregisterPackageInstalled,
    __PADDING_3__,
    x_package_XPackageGetMountPathSize,
    x_package_XPackageGetMountPath,
    x_package_XPackageCloseMountHandle,
    __PADDING_4__,
    x_package_XPackageEnumeratePackages,
    x_package_XPackageRegisterPackageInstalled,
    x_package_XPackageGetWriteStats,
    __PADDING_5__,
    x_package_XPackageUninstallUWPInstance,
    x_package_XPackageEnumerateFeatures,
    x_package_XPackageUninstallPackage,
    /* IXPackageImpl2 methods */
    x_package_XPackageEnumeratePackages2,
    x_package_XPackageRegisterPackageInstalled2,
    x_package_XPackageMountWithUiAsync,
    x_package_XPackageMountWithUiResult,
    /* IXPackageImpl3 methods */
    x_package_XPackageEnumeratePackages3,
    x_package_XPackageRegisterPackageInstalled3,
    /* IXPackageImpl4 methods */
    x_package_XPackageGetPackageKind,
};

static struct x_package x_package =
{
    {&x_package_vtbl},
    0,
};

IXPackageImpl *x_package_impl = (IXPackageImpl *)&x_package.IXPackageImpl4_iface;
