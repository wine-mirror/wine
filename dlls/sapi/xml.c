/*
 * Speech API (SAPI) XML parser implementation.
 *
 * Copyright 2025 Shaun Ren for CodeWeavers
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

#include "objbase.h"

#include "sapiddk.h"

#include "wine/debug.h"

#include "sapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sapi);

HRESULT parse_sapi_xml(const WCHAR *contents, DWORD parse_flag, BOOL persist, SPVSTATE *global_state,
        SPVTEXTFRAG **frag_list)
{
    FIXME("(%p, %#lx, %d, %p, %p): stub.\n", contents, parse_flag, persist, global_state, frag_list);
    return E_NOTIMPL;
}
