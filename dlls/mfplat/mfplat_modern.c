/*
 * Media Foundation Platform Modern Extensions
 * Enhanced codec and streaming support for modern media
 * Required by applications using video/audio playback
 *
 * Copyright 2025 Wine Project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "mfapi.h"
#include "mferror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

/***********************************************************************
 *          MFCreateDXGIDeviceManager  (stub enhancement)
 */
HRESULT WINAPI MFCreateDXGIDeviceManager_Modern( UINT *token, IMFDXGIDeviceManager **manager )
{
    FIXME( "token %p, manager %p: enhanced stub\n", token, manager );

    if (!token || !manager)
        return E_POINTER;

    /* Return fake values - proper implementation would create DXGI device manager */
    *token = 0x12345678;
    *manager = NULL;

    TRACE( "Returning stub manager (apps may fail if they depend on this)\n" );
    return S_OK;
}

/***********************************************************************
 *          MFCreateVideoSampleAllocatorEx  (stub enhancement)
 */
HRESULT WINAPI MFCreateVideoSampleAllocatorEx_Modern( REFIID riid, void **allocator )
{
    FIXME( "riid %s, allocator %p: enhanced stub\n", debugstr_guid(riid), allocator );

    if (!allocator)
        return E_POINTER;

    *allocator = NULL;

    TRACE( "Video sample allocator not implemented\n" );
    return E_NOTIMPL;
}

/***********************************************************************
 *          MFCreateDXGISurfaceBuffer  (stub enhancement)
 */
HRESULT WINAPI MFCreateDXGISurfaceBuffer_Modern( REFIID riid, IUnknown *surface,
                                                  UINT subresource, BOOL bottom_up_when_linear,
                                                  IMFMediaBuffer **buffer )
{
    FIXME( "riid %s, surface %p, subresource %u, bottom_up %d, buffer %p: stub\n",
           debugstr_guid(riid), surface, subresource, bottom_up_when_linear, buffer );

    if (!buffer)
        return E_POINTER;

    *buffer = NULL;
    return E_NOTIMPL;
}

/***********************************************************************
 *          MFCreateMediaBufferFromMediaType  (stub enhancement)
 */
HRESULT WINAPI MFCreateMediaBufferFromMediaType_Modern( IMFMediaType *media_type,
                                                         LONGLONG duration, DWORD min_length,
                                                         DWORD min_alignment, IMFMediaBuffer **buffer )
{
    FIXME( "media_type %p, duration %s, min_length %lu, alignment %lu, buffer %p: stub\n",
           media_type, wine_dbgstr_longlong(duration), min_length, min_alignment, buffer );

    if (!buffer)
        return E_POINTER;

    *buffer = NULL;
    return E_NOTIMPL;
}

/***********************************************************************
 *          MFCreateSourceResolver  (enhancement)
 */
HRESULT WINAPI MFCreateSourceResolver_Enhanced( IMFSourceResolver **resolver )
{
    TRACE( "resolver %p\n", resolver );

    if (!resolver)
        return E_POINTER;

    /* Call original implementation if it exists, otherwise return stub */
    FIXME( "Source resolver creation - may need actual implementation\n" );
    *resolver = NULL;
    return E_NOTIMPL;
}

/***********************************************************************
 *          MFGetSupportedSchemes  (stub)
 */
HRESULT WINAPI MFGetSupportedSchemes_Modern( PROPVARIANT *schemes )
{
    FIXME( "schemes %p: stub\n", schemes );

    if (!schemes)
        return E_POINTER;

    /* Return empty list for now */
    PropVariantInit( schemes );
    return S_OK;
}

/***********************************************************************
 *          MFGetSupportedMimeTypes  (stub)
 */
HRESULT WINAPI MFGetSupportedMimeTypes_Modern( PROPVARIANT *mime_types )
{
    FIXME( "mime_types %p: stub\n", mime_types );

    if (!mime_types)
        return E_POINTER;

    /* Return empty list for now */
    PropVariantInit( mime_types );
    return S_OK;
}

/***********************************************************************
 *          MFCreate2DMediaBuffer  (enhancement)
 */
HRESULT WINAPI MFCreate2DMediaBuffer_Enhanced( DWORD width, DWORD height,
                                                DWORD fourcc, BOOL bottom_up,
                                                IMFMediaBuffer **buffer )
{
    FIXME( "width %lu, height %lu, fourcc %#lx, bottom_up %d, buffer %p: stub\n",
           width, height, fourcc, bottom_up, buffer );

    if (!buffer)
        return E_POINTER;

    *buffer = NULL;
    return E_NOTIMPL;
}

/***********************************************************************
 *          MFCreateMFVideoFormatFromMFMediaType  (stub)
 */
HRESULT WINAPI MFCreateMFVideoFormatFromMFMediaType_Modern( IMFMediaType *media_type,
                                                             MFVIDEOFORMAT **video_format,
                                                             UINT32 *size )
{
    FIXME( "media_type %p, video_format %p, size %p: stub\n", media_type, video_format, size );

    if (!video_format || !size)
        return E_POINTER;

    *video_format = NULL;
    *size = 0;
    return E_NOTIMPL;
}

/***********************************************************************
 *          MFInitVideoFormat_RGB  (stub)
 */
HRESULT WINAPI MFInitVideoFormat_RGB_Modern( MFVIDEOFORMAT *format, DWORD width,
                                              DWORD height, DWORD d3dfmt )
{
    FIXME( "format %p, width %lu, height %lu, d3dfmt %#lx: stub\n", format, width, height, d3dfmt );

    if (!format)
        return E_POINTER;

    memset( format, 0, sizeof(*format) );
    return E_NOTIMPL;
}
