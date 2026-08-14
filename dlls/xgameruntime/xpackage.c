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
    FIXME( "iface %p, bufferSize %Iu, buffer %p stub!\n", iface, bufferSize, buffer );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_package_XPackageIsPackagedProcess( IXPackageImpl4 *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return FALSE;
}

static HRESULT WINAPI x_package_XPackageCreateInstallationMonitor( IXPackageImpl4 *iface, const char *packageIdentifier, UINT32 selectorCount, XPackageChunkSelector *selectors, UINT32 minimumUpdateIntervalMs, XTaskQueueHandle queue, XPackageInstallationMonitorHandle *installationMonitor )
{
    FIXME( "iface %p, packageIdentifier %s, selectorCount %u, selectors %p, minimumUpdateIntervalMs %u, queue %p, installationMonitor %p stub!\n", iface, debugstr_a( packageIdentifier ), selectorCount, selectors, minimumUpdateIntervalMs, queue, installationMonitor );
    return E_NOTIMPL;
}

static void WINAPI x_package_XPackageCloseInstallationMonitorHandle( IXPackageImpl4 *iface, XPackageInstallationMonitorHandle installationMonitor )
{
    FIXME( "iface %p, installationMonitor %p stub!\n", iface, installationMonitor );
}

static void WINAPI x_package_XPackageGetInstallationProgress( IXPackageImpl4 *iface, XPackageInstallationMonitorHandle installationMonitor, XPackageInstallationProgress *progress )
{
    FIXME( "iface %p, installationMonitor %p, progress %p stub!\n", iface, installationMonitor, progress );
}

static BOOLEAN WINAPI x_package_XPackageUpdateInstallationMonitor( IXPackageImpl4 *iface, XPackageInstallationMonitorHandle installationMonitor )
{
    FIXME( "iface %p, installationMonitor %p stub!\n", iface, installationMonitor );
    return TRUE;
}

static HRESULT WINAPI x_package_XPackageRegisterInstallationProgressChanged( IXPackageImpl4 *iface, XPackageInstallationMonitorHandle installationMonitor, void *context, XPackageInstallationProgressCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, installationMonitor %p, context %p, callback %p, token %p stub!\n", iface, installationMonitor, context, callback, token );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_package_XPackageUnregisterInstallationProgressChanged( IXPackageImpl4 *iface, XPackageInstallationMonitorHandle installationMonitor, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    FIXME( "iface %p, installationMonitor %p, token %p, wait %d stub!\n", iface, installationMonitor, &token, wait );
    return TRUE;
}

static HRESULT WINAPI x_package_XPackageGetUserLocale( IXPackageImpl4 *iface, SIZE_T localeSize, char *locale )
{
    FIXME( "iface %p, localeSize %Iu, locale %p stub!\n", iface, localeSize, locale );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageFindChunkAvailability( IXPackageImpl4 *iface, const char *packageIdentifier, UINT32 selectorCount, XPackageChunkSelector *selectors, XPackageChunkAvailability *availability )
{
    FIXME( "iface %p, packageIdentifier %s, selectorCount %u, selectors %p, availability %p stub!\n", iface, packageIdentifier, selectorCount, selectors, availability );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageEnumerateChunkAvailability( IXPackageImpl4 *iface, const char *packageIdentifier, XPackageChunkSelectorType type, void *context, XPackageChunkAvailabilityCallback *callback )
{
    FIXME( "iface %p, packageIdentifier %s, type %d, context %p, callback %p stub!\n", iface, debugstr_a( packageIdentifier ), type, context, callback );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageChangeChunkInstallOrder( IXPackageImpl4 *iface, const char *packageIdentifier, UINT32 selectorCount, XPackageChunkSelector *selectors )
{
    FIXME( "iface %p, packageIdentifier %s, selectorCount %u, selectors %p stub!\n", iface, debugstr_a( packageIdentifier ), selectorCount, selectors );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageInstallChunks( IXPackageImpl4 *iface, const char *packageIdentifier, UINT32 selectorCount, XPackageChunkSelector *selectors, UINT32 minimumUpdateIntervalMs, BOOLEAN suppressUserConfirmation, XTaskQueueHandle queue, XPackageInstallationMonitorHandle *installationMonitor )
{
    FIXME( "iface %p, packageIdentifier %s, selectorCount %u, selectors %p, minimumUpdateIntervalMs %u, suppressUserConfirmation %d, queue %p, installationMonitor %p stub!\n", iface, debugstr_a( packageIdentifier ), selectorCount, selectors, minimumUpdateIntervalMs, suppressUserConfirmation, queue, installationMonitor );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageInstallChunksAsync( IXPackageImpl4 *iface, const char *packageIdentifier, UINT32 selectorCount, XPackageChunkSelector *selectors, UINT32 minimumUpdateIntervalMs, BOOLEAN suppressUserConfirmation, XAsyncBlock *asyncBlock )
{
    FIXME( "iface %p, packageIdentifier %s, selectorCount %u, selectors %p, minimumUpdateIntervalMs %u, suppressUserConfirmation %d, asyncBlock %p stub!\n", iface, packageIdentifier, selectorCount, selectors, minimumUpdateIntervalMs, suppressUserConfirmation, asyncBlock );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageInstallChunksResult( IXPackageImpl4 *iface, XAsyncBlock *asyncBlock, XPackageInstallationMonitorHandle *installationMonitor )
{
    FIXME( "iface %p, asyncBlock %p, installationMonitor %p stub!\n", iface, asyncBlock, installationMonitor );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageEstimateDownloadSize( IXPackageImpl4 *iface, const char *packageIdentifier, UINT32 selectorCount, XPackageChunkSelector *selectors, UINT64 *downloadSize, BOOLEAN *shouldPresentUserConfirmation )
{
    FIXME( "iface %p, packageIdentifier %s, selectorCount %u, selectors %p, downloadSize %p, shouldPresentUserConfirmation %p stub!\n", iface, packageIdentifier, selectorCount, selectors, downloadSize, shouldPresentUserConfirmation );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageUninstallChunks( IXPackageImpl4 *iface, const char *packageIdentifier, UINT32 selectorCount, XPackageChunkSelector *selectors )
{
    FIXME( "iface %p, packageIdentifier %s, selectorCount %u, selectores %p stub!\n", iface, packageIdentifier, selectorCount, selectors );
    return E_NOTIMPL;
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
    FIXME( "iface %p, token %p, wait %d stub!\n", iface, &token, wait );
    return TRUE;
}

static HRESULT WINAPI __PADDING_3__( IXPackageImpl4 *iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does.\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageGetMountPathSize( IXPackageImpl4 *iface, XPackageMountHandle mount, SIZE_T *pathSize )
{
    FIXME( "iface %p, mount %p, pathSize %p stub!\n", iface, mount, pathSize );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageGetMountPath( IXPackageImpl4 *iface, XPackageMountHandle mount, SIZE_T pathSize, char *path )
{
    FIXME( "iface %p, mount %p, pathSize %Iu, path %p stub!\n", iface, mount, pathSize, path );
    return E_NOTIMPL;
}

static void WINAPI x_package_XPackageCloseMountHandle( IXPackageImpl4 *iface, XPackageMountHandle mount )
{
    FIXME( "iface %p, mount %p stub!\n", iface, mount );
}

static HRESULT WINAPI __PADDING_4__( IXPackageImpl4 *iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does.\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageEnumeratePackages( IXPackageImpl4 *iface, XPackageKind kind, XPackageEnumerationScope scope, void *context, XPackageEnumerationCallback *callback )
{
    FIXME( "iface %p, kind %d, scope %d, context %p, callback %p stub!\n", iface, kind, scope, context, callback );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageRegisterPackageInstalled( IXPackageImpl4 *iface, XTaskQueueHandle queue, void *context, XPackageInstalledCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, context %p, callback %p, token %p stub!\n", iface, queue, context, callback, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageGetWriteStats( IXPackageImpl4 *iface, XPackageWriteStats *writeStats )
{
    FIXME( "iface %p, writeStats %p stub!\n", iface, writeStats );
    return E_NOTIMPL;
}

static HRESULT WINAPI __PADDING_5__( IXPackageImpl4 *iface )
{
    WARN( "iface %p padding function called! It's unknown what this function does.\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageUninstallUWPInstance( IXPackageImpl4 *iface, const char *packageName )
{
    FIXME( "iface %p, packageName %s stub!\n", iface, debugstr_a( packageName ) );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageEnumerateFeatures( IXPackageImpl4 *iface, const char *packageIdentifier, void *context, XPackageFeatureEnumerationCallback *callback )
{
    FIXME( "iface %p, packageIdentifier %s, context %p, callback %p stub!\n", iface, packageIdentifier, context, callback );
    return E_NOTIMPL;
}

static BOOLEAN WINAPI x_package_XPackageUninstallPackage( IXPackageImpl4 *iface, const char *packageIdentifier )
{
    FIXME( "iface %p, packageIdentifier %s", iface, debugstr_a( packageIdentifier ) );
    return FALSE;
}

static HRESULT WINAPI x_package_XPackageEnumeratePackages2( IXPackageImpl4 *iface, XPackageKind kind, XPackageEnumerationScope scope, void *context, XPackageEnumerationCallback *callback )
{
    FIXME( "iface %p, kind %d, scope %d, context %p, callback %p stub!\n", iface, kind, scope, context, callback );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageRegisterPackageInstalled2( IXPackageImpl4 *iface, XTaskQueueHandle queue, void *context, XPackageInstalledCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, context %p, callback %p, token %p stub!\n", iface, queue, context, callback, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageMountWithUiAsync( IXPackageImpl4 *iface, const char *packageIdentifier, XAsyncBlock *async )
{
    FIXME( "iface %p, packageIdentifier %s, async %p stub!\n", iface, debugstr_a( packageIdentifier ), async );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageMountWithUiResult( IXPackageImpl4 *iface, XAsyncBlock *async, XPackageMountHandle *mount )
{
    FIXME( "iface %p, async %p, mount %p stub!\n", iface, async, mount );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageEnumeratePackages3( IXPackageImpl4 *iface, XPackageKind kind, XPackageEnumerationScope scope, void *context, XPackageEnumerationCallback *callback )
{
    FIXME( "iface %p, kind %d, scope %d, context %p, callback %p stub!\n", iface, kind, scope, context, callback );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageRegisterPackageInstalled3( IXPackageImpl4 *iface, XTaskQueueHandle queue, void *context, XPackageInstalledCallback *callback, XTaskQueueRegistrationToken *token )
{
    FIXME( "iface %p, queue %p, context %p, callback %p, token %p stub!\n", iface, queue, context, callback, token );
    return E_NOTIMPL;
}

static HRESULT WINAPI x_package_XPackageGetPackageKind( IXPackageImpl4 *iface, const char *packageIdentifier, XPackageKind *kind )
{
    FIXME( "iface %p, packageIdentifier %s, kind %p stub!\n", iface, debugstr_a( packageIdentifier ), kind );
    return E_NOTIMPL;
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
