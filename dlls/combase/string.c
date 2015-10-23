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


struct hstring_private
{
    LPWSTR buffer;
    UINT32 length;
    BOOL   reference;
    LONG   refcount;
};

static const WCHAR empty[1];

C_ASSERT(sizeof(struct hstring_private) <= sizeof(HSTRING_HEADER));

static inline struct hstring_private *impl_from_HSTRING(HSTRING string)
{
   return (struct hstring_private *)string;
}

static inline struct hstring_private *impl_from_HSTRING_HEADER(HSTRING_HEADER *header)
{
   return (struct hstring_private *)header;
}

static inline struct hstring_private *impl_from_HSTRING_BUFFER(HSTRING_BUFFER buffer)
{
   return (struct hstring_private *)buffer;
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
    if (len == 0)
    {
        *out = NULL;
        return S_OK;
    }
    if (ptr[len] != '\0')
        return E_INVALIDARG;
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

/***********************************************************************
 *      WindowsPreallocateStringBuffer (combase.@)
 */
HRESULT WINAPI WindowsPreallocateStringBuffer(UINT32 len, WCHAR **outptr,
                                              HSTRING_BUFFER *out)
{
    struct hstring_private *priv;
    HSTRING str;
    if (outptr == NULL || out == NULL)
        return E_POINTER;
    if (len == 0)
    {
        *outptr = (LPWSTR)empty;
        *out = NULL;
        return S_OK;
    }

    if (!alloc_string(len, &str))
        return E_OUTOFMEMORY;
    priv = impl_from_HSTRING(str);
    *outptr = priv->buffer;
    *out = (HSTRING_BUFFER)str;
    return S_OK;
}

/***********************************************************************
 *      WindowsDeleteStringBuffer (combase.@)
 */
HRESULT WINAPI WindowsDeleteStringBuffer(HSTRING_BUFFER buf)
{
    return WindowsDeleteString((HSTRING)buf);
}

/***********************************************************************
 *      WindowsPromoteStringBuffer (combase.@)
 */
HRESULT WINAPI WindowsPromoteStringBuffer(HSTRING_BUFFER buf, HSTRING *out)
{
    struct hstring_private *priv = impl_from_HSTRING_BUFFER(buf);
    if (out == NULL)
        return E_POINTER;
    if (buf == NULL)
    {
        *out = NULL;
        return S_OK;
    }
    if (priv->buffer[priv->length] != 0 || priv->reference || priv->refcount != 1)
        return E_INVALIDARG;
    *out = (HSTRING)buf;
    return S_OK;
}

/***********************************************************************
 *      WindowsGetStringLen (combase.@)
 */
UINT32 WINAPI WindowsGetStringLen(HSTRING str)
{
    struct hstring_private *priv = impl_from_HSTRING(str);
    if (str == NULL)
        return 0;
    return priv->length;
}

/***********************************************************************
 *      WindowsGetStringRawBuffer (combase.@)
 */
LPCWSTR WINAPI WindowsGetStringRawBuffer(HSTRING str, UINT32 *len)
{
    struct hstring_private *priv = impl_from_HSTRING(str);
    if (str == NULL)
    {
        if (len)
            *len = 0;
        return empty;
    }
    if (len)
        *len = priv->length;
    return priv->buffer;
}

/***********************************************************************
 *      WindowsStringHasEmbeddedNull (combase.@)
 */
HRESULT WINAPI WindowsStringHasEmbeddedNull(HSTRING str, BOOL *out)
{
    UINT32 i;
    struct hstring_private *priv = impl_from_HSTRING(str);
    if (out == NULL)
        return E_INVALIDARG;
    if (str == NULL)
    {
        *out = FALSE;
        return S_OK;
    }
    for (i = 0; i < priv->length; i++)
    {
        if (priv->buffer[i] == '\0')
        {
            *out = TRUE;
            return S_OK;
        }
    }
    *out = FALSE;
    return S_OK;
}

/***********************************************************************
 *      WindowsSubstring (combase.@)
 */
HRESULT WINAPI WindowsSubstring(HSTRING str, UINT32 start, HSTRING *out)
{
    struct hstring_private *priv = impl_from_HSTRING(str);
    UINT32 len = WindowsGetStringLen(str);
    if (out == NULL)
        return E_INVALIDARG;
    if (start > len)
        return E_BOUNDS;
    if (start == len)
    {
        *out = NULL;
        return S_OK;
    }
    return WindowsCreateString(&priv->buffer[start], len - start, out);
}

/***********************************************************************
 *      WindowsIsStringEmpty (combase.@)
 */
BOOL WINAPI WindowsIsStringEmpty(HSTRING str)
{
    struct hstring_private *priv = impl_from_HSTRING(str);
    if (str == NULL)
        return TRUE;
    return priv->length == 0;
}
