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

HRESULT audio_record_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT avi_compressor_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT avi_mux_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT capture_graph_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT smart_tee_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;
HRESULT vfw_capture_create(IUnknown *outer, IUnknown **out) DECLSPEC_HIDDEN;

#endif /* _QCAP_MAIN_H_DEFINED */
