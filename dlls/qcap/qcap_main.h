/*
 * Qcap main header file
 *
 * Copyright (C) 2005 Rolf Kalbermatter
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
#ifndef _QCAP_MAIN_H_DEFINED
#define _QCAP_MAIN_H_DEFINED

#include "wine/heap.h"
#include "wine/strmbase.h"

extern DWORD ObjectRefCount(BOOL increment) DECLSPEC_HIDDEN;

extern IUnknown * WINAPI QCAP_createAudioCaptureFilter(IUnknown *pUnkOuter, HRESULT *phr) DECLSPEC_HIDDEN;
extern IUnknown * WINAPI QCAP_createAVICompressor(IUnknown *pUnkOuter, HRESULT *phr) DECLSPEC_HIDDEN;
extern IUnknown * WINAPI QCAP_createVFWCaptureFilter(IUnknown *pUnkOuter, HRESULT *phr) DECLSPEC_HIDDEN;
extern IUnknown * WINAPI QCAP_createVFWCaptureFilterPropertyPage(IUnknown *pUnkOuter, HRESULT *phr) DECLSPEC_HIDDEN;
extern IUnknown * WINAPI QCAP_createAVICompressor(IUnknown*,HRESULT*) DECLSPEC_HIDDEN;
extern IUnknown * WINAPI QCAP_createAVIMux(IUnknown *pUnkOuter, HRESULT *phr) DECLSPEC_HIDDEN;
extern IUnknown * WINAPI QCAP_createAVIMuxPropertyPage(IUnknown *pUnkOuter, HRESULT *phr) DECLSPEC_HIDDEN;
extern IUnknown * WINAPI QCAP_createAVIMuxPropertyPage1(IUnknown *pUnkOuter, HRESULT *phr) DECLSPEC_HIDDEN;
extern IUnknown * WINAPI QCAP_createFileWriter(IUnknown *pUnkOuter, HRESULT *phr) DECLSPEC_HIDDEN;
extern IUnknown * WINAPI QCAP_createCaptureGraphBuilder2(IUnknown *pUnkOuter, HRESULT *phr) DECLSPEC_HIDDEN;
extern IUnknown * WINAPI QCAP_createInfinitePinTeeFilter(IUnknown *pUnkOuter, HRESULT *phr) DECLSPEC_HIDDEN;
extern IUnknown * WINAPI QCAP_createSmartTeeFilter(IUnknown *pUnkOuter, HRESULT *phr) DECLSPEC_HIDDEN;
extern IUnknown * WINAPI QCAP_createAudioInputMixerPropertyPage(IUnknown *pUnkOuter, HRESULT *phr) DECLSPEC_HIDDEN;

#endif /* _QCAP_MAIN_H_DEFINED */
