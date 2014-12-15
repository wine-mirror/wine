/*
 * Copyright 2014 Martin Storsjo
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

#include <string.h>

#include "windows.h"
#include "winerror.h"
#include "hstring.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(combase);

struct hstring_private
{
    LPWSTR buffer;
    UINT32 length;
    BOOL   reference;
    LONG   refcount;
};

C_ASSERT(sizeof(struct hstring_private) <= sizeof(HSTRING_HEADER));

static inline struct hstring_private *impl_from_HSTRING(HSTRING string)
{
   return (struct hstring_private *)string;
}

static inline struct hstring_private *impl_from_HSTRING_HEADER(HSTRING_HEADER *header)
{
   return (struct hstring_private *)header;
}

static BOOL alloc_string(UINT32 len, HSTRING *out)
{
    struct hstring_private *priv;
    priv = HeapAlloc(GetProcessHeap(), 0, sizeof(*priv) + (len + 1) * sizeof(*priv->buffer));
    if (!priv)
        return FALSE;
    priv->buffer = (LPWSTR)(priv + 1);
    priv->length = len;
    priv->reference = FALSE;
    priv->refcount = 1;
    priv->buffer[len] = '\0';
    *out = (HSTRING)priv;
    return TRUE;
}

/***********************************************************************
 *      WindowsCreateString (combase.@)
 */
HRESULT WINAPI WindowsCreateString(LPCWSTR ptr, UINT32 len,
                                   HSTRING *out)
{
    struct hstring_private *priv;
    if (out == NULL)
        return E_INVALIDARG;
    if (ptr == NULL && len > 0)
        return E_POINTER;
    if (len == 0)
    {
        *out = NULL;
        return S_OK;
    }
    if (!alloc_string(len, out))
        return E_OUTOFMEMORY;
    priv = impl_from_HSTRING(*out);
    memcpy(priv->buffer, ptr, len * sizeof(*priv->buffer));
    return S_OK;
}

/***********************************************************************
 *      WindowsCreateStringReference (combase.@)
 */
HRESULT WINAPI WindowsCreateStringReference(LPCWSTR ptr, UINT32 len,
                                            HSTRING_HEADER *header, HSTRING *out)
{
    struct hstring_private *priv = impl_from_HSTRING_HEADER(header);
    if (out == NULL || header == NULL)
        return E_INVALIDARG;
    if (ptr == NULL && len > 0)
        return E_POINTER;
    if (ptr[len] != '\0')
        return E_INVALIDARG;
    if (len == 0)
    {
        *out = NULL;
        return S_OK;
    }
    priv->buffer = (LPWSTR)ptr;
    priv->length = len;
    priv->reference = TRUE;
    *out = (HSTRING)header;
    return S_OK;
}

/***********************************************************************
 *      WindowsDeleteString (combase.@)
 */
HRESULT WINAPI WindowsDeleteString(HSTRING str)
{
    struct hstring_private *priv = impl_from_HSTRING(str);
    if (str == NULL)
        return S_OK;
    if (priv->reference)
        return S_OK;
    if (InterlockedDecrement(&priv->refcount) == 0)
        HeapFree(GetProcessHeap(), 0, priv);
    return S_OK;
}

/***********************************************************************
 *      WindowsDuplicateString (combase.@)
 */
HRESULT WINAPI WindowsDuplicateString(HSTRING str, HSTRING *out)
{
    struct hstring_private *priv = impl_from_HSTRING(str);
    if (out == NULL)
        return E_INVALIDARG;
    if (str == NULL)
    {
        *out = NULL;
        return S_OK;
    }
    if (priv->reference)
        return WindowsCreateString(priv->buffer, priv->length, out);
    InterlockedIncrement(&priv->refcount);
    *out = str;
    return S_OK;
}
