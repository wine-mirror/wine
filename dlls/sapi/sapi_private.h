/*
 * Speech API (SAPI) private header file.
 *
 * Copyright (C) 2017 Huw Davies
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

#include "wine/heap.h"

HRESULT data_key_create( IUnknown *outer, REFIID iid, void **obj ) DECLSPEC_HIDDEN;
HRESULT file_stream_create( IUnknown *outer, REFIID iid, void **obj ) DECLSPEC_HIDDEN;
HRESULT resource_manager_create( IUnknown *outer, REFIID iid, void **obj ) DECLSPEC_HIDDEN;
HRESULT speech_stream_create( IUnknown *outer, REFIID iid, void **obj ) DECLSPEC_HIDDEN;
HRESULT speech_voice_create( IUnknown *outer, REFIID iid, void **obj ) DECLSPEC_HIDDEN;
HRESULT token_category_create( IUnknown *outer, REFIID iid, void **obj ) DECLSPEC_HIDDEN;
HRESULT token_enum_create( IUnknown *outer, REFIID iid, void **obj ) DECLSPEC_HIDDEN;
HRESULT token_create( IUnknown *outer, REFIID iid, void **obj ) DECLSPEC_HIDDEN;
