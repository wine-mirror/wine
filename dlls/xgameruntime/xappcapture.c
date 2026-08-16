/*
 * Xbox Game runtime Library
 *  GDK Component: System API -> XAppCapture
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

struct capture
{
    IXAppCaptureImpl4 IXAppCaptureImpl4_iface;
    IXAppCaptureMetadataImpl IXAppCaptureMetadataImpl_iface;
    LONG ref;
};

static inline struct capture *impl_from_IXAppCaptureImpl4( IXAppCaptureImpl4 *iface )
{
    return CONTAINING_RECORD( iface, struct capture, IXAppCaptureImpl4_iface );
}

static HRESULT WINAPI capture_QueryInterface( IXAppCaptureImpl4 *iface, REFIID iid, void **out )
{
    struct capture *impl = impl_from_IXAppCaptureImpl4( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown          ) ||
        IsEqualGUID( iid, &IID_IXAppCaptureImpl  ) ||
        IsEqualGUID( iid, &IID_IXAppCaptureImpl2 ) ||
        IsEqualGUID( iid, &IID_IXAppCaptureImpl3 ) ||
        IsEqualGUID( iid, &IID_IXAppCaptureImpl4 ))
    {
        IXAppCaptureImpl4_AddRef( *out = &impl->IXAppCaptureImpl4_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI capture_AddRef( IXAppCaptureImpl4 *iface )
{
    struct capture *impl = impl_from_IXAppCaptureImpl4( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI capture_Release( IXAppCaptureImpl4 *iface )
{
    struct capture *impl = impl_from_IXAppCaptureImpl4( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );
    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );
    return ref;
}

static HRESULT WINAPI capture_XAppCaptureTakeDiagnosticScreenshot( IXAppCaptureImpl4 *iface, BOOLEAN gamescreenOnly, XAppCaptureScreenshotFormatFlag captureFlags, const char *filenamePrefix, XAppCaptureDiagnosticScreenshotResult *result )
{
    TRACE( "iface %p, gamescreenOnly %d, captureFlags %x, filenamePrefix %s, result %p\n", iface, gamescreenOnly, captureFlags, debugstr_a( filenamePrefix ), result );
    if (result) memset( result, 0, sizeof(*result) );
    return S_OK;
}

static HRESULT WINAPI capture_XAppCaptureRecordDiagnosticClip( IXAppCaptureImpl4 *iface, time_t startTime, UINT32 durationInMs, const char *filenamePrefix, XAppCaptureRecordClipResult *result )
{
    TRACE( "iface %p, startTime %Iu, durationInMs %u, filenamePrefix %s, result %p\n", iface, startTime, durationInMs, debugstr_a( filenamePrefix ), result );
    if (result) memset( result, 0, sizeof(*result) );
    return S_OK;
}

static HRESULT WINAPI capture_XAppCaptureTakeScreenshot( IXAppCaptureImpl4 *iface, XUserHandle requestingUser, XAppCaptureTakeScreenshotResult *result )
{
    TRACE( "iface %p, requestingUser %p, result %p\n", iface, requestingUser, result );
    if (result) memset( result, 0, sizeof(*result) );
    return S_OK;
}

static HRESULT WINAPI capture_XAppCaptureOpenScreenshotStream( IXAppCaptureImpl4 *iface, const char *localId, XAppCaptureScreenshotFormatFlag screenshotFormat, XAppCaptureScreenshotStreamHandle *handle, UINT64 *totalBytes )
{
    FIXME( "iface %p, localId %s, screenshotFormat %d, handle %p, totalBytes %p stub!\n", iface, debugstr_a( localId ), screenshotFormat, handle, totalBytes );
    return E_NOTIMPL;
}

static HRESULT WINAPI capture_XAppCaptureReadScreenshotStream( IXAppCaptureImpl4 *iface, XAppCaptureScreenshotStreamHandle handle, UINT64 startPosition, UINT32 bytesToRead, UINT8 *bytes, UINT32 *bytesWritten )
{
    FIXME( "iface %p, handle %p, startPosition %llu, bytesToRead %u, bytes %p, bytesWritten %p stub!\n", iface, handle, startPosition, bytesToRead, bytes, bytesWritten );
    return E_NOTIMPL;
}

static HRESULT WINAPI capture_XAppCaptureCloseScreenshotStream( IXAppCaptureImpl4 *iface, XAppCaptureScreenshotStreamHandle handle )
{
    TRACE( "iface %p, handle %p\n", iface, handle );
    return S_OK;
}

static HRESULT WINAPI capture_XAppCaptureEnableRecord( IXAppCaptureImpl4 *iface )
{
    TRACE( "iface %p\n", iface );
    return S_OK;
}

static HRESULT WINAPI capture_XAppCaptureDisableRecord( IXAppCaptureImpl4 *iface )
{
    TRACE( "iface %p\n", iface );
    return S_OK;
}

static HRESULT WINAPI capture_XAppCaptureGetVideoCaptureSettings( IXAppCaptureImpl4 *iface, XAppCaptureVideoCaptureSettings *userCaptureSettings )
{
    TRACE( "iface %p, userCaptureSettings %p\n", iface, userCaptureSettings );
    if (!userCaptureSettings) return E_POINTER;
    memset( userCaptureSettings, 0, sizeof(*userCaptureSettings) );
    return S_OK;
}

static HRESULT WINAPI capture_XAppCaptureRecordTimespan( IXAppCaptureImpl4 *iface, const SYSTEMTIME *startTimestamp, UINT64 durationInMilliseconds, XAppCaptureLocalResult *result )
{
    TRACE( "iface %p, startTimestamp %p, durationInMilliseconds %llu, result %p\n", iface, startTimestamp, durationInMilliseconds, result );
    if (result) memset( result, 0, sizeof(*result) );
    return S_OK;
}

static HRESULT WINAPI capture_XAppCaptureReadLocalStream( IXAppCaptureImpl4 *iface, XAppCaptureLocalStreamHandle handle, SIZE_T startPosition, UINT32 bytesToRead, UINT8 *bytes, UINT32 *bytesWritten )
{
    FIXME( "iface %p, handle %p, startPosition %Iu, bytesToRead %u, bytes %p, bytesWritten %p stub!\n", iface, handle, startPosition, bytesToRead, bytes, bytesWritten );
    return E_NOTIMPL;
}

static HRESULT WINAPI capture_XAppCaptureCloseLocalStream( IXAppCaptureImpl4 *iface, XAppCaptureLocalStreamHandle handle )
{
    TRACE( "iface %p, handle %p\n", iface, handle );
    return S_OK;
}

static HRESULT WINAPI capture_XAppCaptureStartUserRecord( IXAppCaptureImpl4 *iface, XUserHandle requestingUser, UINT32 localIdBufferLength, char *localIdBuffer )
{
    TRACE( "iface %p, requestingUser %p, localIdBufferLength %u, localIdBuffer %p\n", iface, requestingUser, localIdBufferLength, localIdBuffer );
    if (localIdBuffer && localIdBufferLength > 0)
        strncpy( localIdBuffer, "WINE_CAPTURE_000000", localIdBufferLength - 1 );
    return S_OK;
}

static HRESULT WINAPI capture_XAppCaptureStopUserRecord( IXAppCaptureImpl4 *iface, const char *localId, XAppCaptureUserRecordingResult *result )
{
    TRACE( "iface %p, localId %s, result %p\n", iface, debugstr_a( localId ), result );
    if (result) memset( result, 0, sizeof(*result) );
    return S_OK;
}

static HRESULT WINAPI capture_XAppCaptureCancelUserRecord( IXAppCaptureImpl4 *iface, const char *localId )
{
    TRACE( "iface %p, localId %s\n", iface, debugstr_a( localId ) );
    return S_OK;
}

static const struct IXAppCaptureImpl4Vtbl capture_vtbl =
{
    capture_QueryInterface,
    capture_AddRef,
    capture_Release,
    /* IXAppCaptureImpl methods */
    capture_XAppCaptureTakeDiagnosticScreenshot,
    capture_XAppCaptureRecordDiagnosticClip,
    capture_XAppCaptureTakeScreenshot,
    capture_XAppCaptureOpenScreenshotStream,
    capture_XAppCaptureReadScreenshotStream,
    capture_XAppCaptureCloseScreenshotStream,
    capture_XAppCaptureEnableRecord,
    capture_XAppCaptureDisableRecord,
    /* IXAppCaptureImpl2 methods */
    capture_XAppCaptureGetVideoCaptureSettings,
    capture_XAppCaptureRecordTimespan,
    capture_XAppCaptureReadLocalStream,
    capture_XAppCaptureCloseLocalStream,
    /* IXAppCaptureImpl3 methods */
    capture_XAppCaptureStartUserRecord,
    capture_XAppCaptureStopUserRecord,
    /* IXAppCaptureImpl4 methods */
    capture_XAppCaptureCancelUserRecord,
};

static inline struct capture *impl_from_IXAppCaptureMetadataImpl( IXAppCaptureMetadataImpl *iface )
{
    return CONTAINING_RECORD( iface, struct capture, IXAppCaptureMetadataImpl_iface );
}

static HRESULT WINAPI metadata_QueryInterface( IXAppCaptureMetadataImpl *iface, REFIID iid, void **out )
{
    struct capture *impl = impl_from_IXAppCaptureMetadataImpl( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown                 ) ||
        IsEqualGUID( iid, &IID_IXAppCaptureMetadataImpl ))
    {
        IXAppCaptureMetadataImpl_AddRef( *out = &impl->IXAppCaptureMetadataImpl_iface );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI metadata_AddRef( IXAppCaptureMetadataImpl *iface )
{
    struct capture *impl = impl_from_IXAppCaptureMetadataImpl( iface );
    return IXAppCaptureImpl4_AddRef( &impl->IXAppCaptureImpl4_iface );
}

static ULONG WINAPI metadata_Release( IXAppCaptureMetadataImpl *iface )
{
    struct capture *impl = impl_from_IXAppCaptureMetadataImpl( iface );
    return IXAppCaptureImpl4_Release( &impl->IXAppCaptureImpl4_iface );
}

static BOOLEAN WINAPI metadata_XAppBroadcastIsAppBroadcasting( IXAppCaptureMetadataImpl *iface )
{
    TRACE( "iface %p\n", iface );
    return FALSE;
}

static HRESULT WINAPI metadata_XAppBroadcastShowUI( IXAppCaptureMetadataImpl *iface, XUserHandle requestingUser )
{
    TRACE( "iface %p requestingUser %p\n", iface, requestingUser );
    return S_OK;
}

static HRESULT WINAPI metadata_XAppBroadcastGetStatus( IXAppCaptureMetadataImpl *iface, XUserHandle requestingUser, XAppBroadcastStatus *appBroadcastStatus )
{
    TRACE( "iface %p, requestingUser %p, appBroadcastStatus %p\n", iface, requestingUser, appBroadcastStatus );
    if (appBroadcastStatus) memset( appBroadcastStatus, 0, sizeof(*appBroadcastStatus) );
    return S_OK;
}

static HRESULT WINAPI metadata_XAppBroadcastRegisterIsAppBroadcastingChanged( IXAppCaptureMetadataImpl *iface, XTaskQueueHandle queue, void *context, XAppBroadcastMonitorCallback *appBroadcastMonitorCallback, XTaskQueueRegistrationToken *token )
{
    TRACE( "iface %p, queue %p, context %p, appBroadcastMonitorCallback %p, token %p\n", iface, queue, context, appBroadcastMonitorCallback, token );
    if (token) token->value = 1;
    return S_OK;
}

static BOOLEAN WINAPI metadata_XAppBroadcastUnregisterIsAppBroadcastingChanged( IXAppCaptureMetadataImpl *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    TRACE( "iface %p\n", iface );
    return TRUE;
}

static HRESULT WINAPI metadata_XAppCaptureMetadataAddStringEvent( IXAppCaptureMetadataImpl *iface, const char *name, const char *value, XAppCaptureMetadataPriority priority )
{
    TRACE( "iface %p, name %s, value %s, priority %d\n", iface, debugstr_a( name ), debugstr_a( value ), priority );
    return S_OK;
}

static HRESULT WINAPI metadata_XAppCaptureMetadataAddInt32Event( IXAppCaptureMetadataImpl *iface, const char *name, INT32 value, XAppCaptureMetadataPriority priority )
{
    TRACE( "iface %p, name %s, value %d, priority %d\n", iface, debugstr_a( name ), value, priority );
    return S_OK;
}

static HRESULT WINAPI metadata_XAppCaptureMetadataAddDoubleEvent( IXAppCaptureMetadataImpl *iface, const char *name, double value, XAppCaptureMetadataPriority priority )
{
    TRACE( "iface %p, name %s, value %e, priority %d\n", iface, debugstr_a( name ), value, priority );
    return S_OK;
}

static HRESULT WINAPI metadata_XAppCaptureMetadataStartStringState( IXAppCaptureMetadataImpl *iface, const char *name, const char *value, XAppCaptureMetadataPriority priority )
{
    TRACE( "iface %p, name %s, value %s, priority %d\n", iface, debugstr_a( name ), debugstr_a( value ), priority );
    return S_OK;
}

static HRESULT WINAPI metadata_XAppCaptureMetadataStartInt32State( IXAppCaptureMetadataImpl *iface, const char *name, INT32 value, XAppCaptureMetadataPriority priority )
{
    TRACE( "iface %p, name %s, value %d, priority %d\n", iface, debugstr_a( name ), value, priority );
    return S_OK;
}

static HRESULT WINAPI metadata_XAppCaptureMetadataStartDoubleState( IXAppCaptureMetadataImpl *iface, const char *name, double value, XAppCaptureMetadataPriority priority )
{
    TRACE( "iface %p, name %s, value %e, priority %d\n", iface, debugstr_a( name ), value, priority );
    return S_OK;
}

static HRESULT WINAPI metadata_XAppCaptureMetadataStopState( IXAppCaptureMetadataImpl *iface, const char *name )
{
    TRACE( "iface %p, name %s\n", iface, debugstr_a( name ) );
    return S_OK;
}

static HRESULT WINAPI metadata_XAppCaptureMetadataStopAllStates( IXAppCaptureMetadataImpl *iface )
{
    TRACE( "iface %p\n", iface );
    return S_OK;
}

static HRESULT WINAPI metadata_XAppCaptureMetadataRemainingStorageBytesAvailable( IXAppCaptureMetadataImpl *iface, UINT64 *value )
{
    TRACE( "iface %p, value %p\n", iface, value );
    if (value) *value = (UINT64)100 * 1024 * 1024;
    return S_OK;
}

static HRESULT WINAPI metadata_XAppCaptureRegisterMetadataPurged( IXAppCaptureMetadataImpl *iface, XTaskQueueHandle queue, void *context, XAppCaptureMetadataPurgedCallback *callback, XTaskQueueRegistrationToken *token )
{
    TRACE( "iface %p, queue %p, context %p, callback %p, token %p\n", iface, queue, context, callback, token );
    if (token) token->value = 1;
    return S_OK;
}

static BOOLEAN WINAPI metadata_XAppCaptureUnRegisterMetadataPurged( IXAppCaptureMetadataImpl *iface, XTaskQueueRegistrationToken token, BOOLEAN wait )
{
    TRACE( "iface %p\n", iface );
    return TRUE;
}

static const struct IXAppCaptureMetadataImplVtbl metadata_vtbl =
{
    metadata_QueryInterface,
    metadata_AddRef,
    metadata_Release,
    /* IXAppCaptureMetadataImpl methods */
    metadata_XAppBroadcastIsAppBroadcasting,
    metadata_XAppBroadcastShowUI,
    metadata_XAppBroadcastGetStatus,
    metadata_XAppBroadcastRegisterIsAppBroadcastingChanged,
    metadata_XAppBroadcastUnregisterIsAppBroadcastingChanged,
    metadata_XAppCaptureMetadataAddStringEvent,
    metadata_XAppCaptureMetadataAddInt32Event,
    metadata_XAppCaptureMetadataAddDoubleEvent,
    metadata_XAppCaptureMetadataStartStringState,
    metadata_XAppCaptureMetadataStartInt32State,
    metadata_XAppCaptureMetadataStartDoubleState,
    metadata_XAppCaptureMetadataStopState,
    metadata_XAppCaptureMetadataStopAllStates,
    metadata_XAppCaptureMetadataRemainingStorageBytesAvailable,
    metadata_XAppCaptureRegisterMetadataPurged,
    metadata_XAppCaptureUnRegisterMetadataPurged,
};

static struct capture capture =
{
    {&capture_vtbl},
    {&metadata_vtbl},
    0,
};

IXAppCaptureImpl *x_app_capture_impl = (IXAppCaptureImpl *)&capture.IXAppCaptureImpl4_iface;
IXAppCaptureMetadataImpl *x_app_capture_metadata_impl = (IXAppCaptureMetadataImpl *)&capture.IXAppCaptureMetadataImpl_iface;
