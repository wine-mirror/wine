/*
 * Copyright 2022 Nikolay Sivov for CodeWeavers
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

#include "media_source.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mfplat);

/***********************************************************************
 *              DllGetClassObject (mfsrcsnk.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    *out = NULL;

    if (IsEqualGUID(clsid, &CLSID_AVIByteStreamPlugin))
        return IClassFactory_QueryInterface(&avi_byte_stream_plugin_factory, riid, out);
    if (IsEqualGUID(clsid, &CLSID_WAVByteStreamPlugin))
        return IClassFactory_QueryInterface(&wav_byte_stream_plugin_factory, riid, out);
    if (IsEqualGUID(clsid, &CLSID_MP3ByteStreamPlugin))
        return IClassFactory_QueryInterface(&mp3_byte_stream_plugin_factory, riid, out);

    if (IsEqualGUID(clsid, &CLSID_MFWAVESinkClassFactory))
        return IClassFactory_QueryInterface(wave_sink_class_factory, riid, out);

    FIXME("Unknown clsid %s.\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
