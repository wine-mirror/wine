/*
 * Copyright 2025 Zhiyi Zhang for CodeWeavers
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

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bcp47langs);

HRESULT WINAPI GetFontFallbackLanguageList(const WCHAR *lang, size_t buffer_length, WCHAR *buffer,
                                           size_t *required_buffer_length)
{
    WCHAR locale[LOCALE_NAME_MAX_LENGTH];
    size_t lang_length;

    FIXME("lang %s, buffer_length %Iu, buffer %p, required_buffer_length %p stub!\n",
          wine_dbgstr_w(lang), buffer_length, buffer, required_buffer_length);

    if (!buffer_length || !buffer)
        return E_INVALIDARG;

    if (!lang)
    {
        if (!GetUserDefaultLocaleName(locale, LOCALE_NAME_MAX_LENGTH))
            return E_FAIL;

        lang = locale;
    }

    /* Return the original language as the fallback language list */
    lang_length = wcslen(lang) + 1;
    *required_buffer_length = lang_length;
    if (buffer_length < lang_length)
        return E_NOT_SUFFICIENT_BUFFER;

    memcpy(buffer, lang, lang_length * sizeof(WCHAR));
    return S_OK;
}
