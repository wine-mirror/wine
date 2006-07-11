/*
 *    Word splitter Implementation
 *
 * Copyright 2006 Mike McCormack
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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "indexsvr.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(infosoft);

typedef struct tag_wordbreaker_impl
{
    const IWordBreakerVtbl *lpVtbl;
    LONG ref;
} wordbreaker_impl;

static HRESULT WINAPI wb_QueryInterface( IWordBreaker *iface,
        REFIID riid, LPVOID *ppvObj)
{
    wordbreaker_impl *This = (wordbreaker_impl *)iface;

    TRACE("(%p)->(%s)\n",This,debugstr_guid(riid));

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IWordBreaker))
    {
        *ppvObj = This;
        return S_OK;
    }

    TRACE("-- E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI wb_AddRef( IWordBreaker *iface )
{
    wordbreaker_impl *This = (wordbreaker_impl *)iface;
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI wb_Release(IWordBreaker *iface)
{
    wordbreaker_impl *This = (wordbreaker_impl *)iface;
    LONG refcount;

    refcount = InterlockedDecrement(&This->ref);
    if (!refcount)
        HeapFree(GetProcessHeap(), 0, This);

    return refcount;
}

static HRESULT WINAPI wb_Init( IWordBreaker *iface,
        BOOL fQuery, ULONG ulMaxTokenSize, BOOL *pfLicense )
{
    TRACE("%d %lu\n", fQuery, ulMaxTokenSize);
    *pfLicense = FALSE;
    return S_OK;
}

static HRESULT WINAPI wb_BreakText( IWordBreaker *iface,
         TEXT_SOURCE *pTextSource, IWordSink *pWordSink, IPhraseSink *pPhraseSink)
{
    LPCWSTR p, q;
    DWORD len;

    FIXME("%p %p %p\n", pTextSource, pWordSink, pPhraseSink);

    p = pTextSource->awcBuffer;

    while (*p)
    {
        /* skip spaces and punctuation */
        while(ispunctW(*p) || isspaceW(*p))
            p++;

        /* find the end of the word */
        q = p;
        while(*q && !ispunctW(*q) && !isspaceW(*q))
            q++;

        len = q - p;
        if (!len)
            break;

        IWordSink_PutWord( pWordSink, len, p, len, p - pTextSource->awcBuffer );

        p = q;
    }
    return S_OK;
}

static HRESULT WINAPI wb_ComposePhrase( IWordBreaker *iface,
         const WCHAR *pwcNoun, ULONG cwcNoun,
         const WCHAR *pwcModifier, ULONG cwcModifier,
         ULONG ulAttachmentType, WCHAR *pwcPhrase, ULONG *pcwcPhrase)
{
    FIXME("%p %lu %p %lu %lu %p %p\n", pwcNoun, cwcNoun,
          pwcModifier, cwcModifier, ulAttachmentType, pwcPhrase, pcwcPhrase);
    return S_OK;
}

static HRESULT WINAPI wb_GetLicenseToUse( IWordBreaker *iface, const WCHAR **ppwcsLicense )
{
    FIXME("%p\n", ppwcsLicense);
    *ppwcsLicense = NULL;
    return S_OK;
}

static const IWordBreakerVtbl wordbreaker_vtbl =
{
    wb_QueryInterface,
    wb_AddRef,
    wb_Release,
    wb_Init,
    wb_BreakText,
    wb_ComposePhrase,
    wb_GetLicenseToUse,
};

HRESULT WINAPI wb_en_us_Constructor(IUnknown* pUnkOuter, REFIID riid, LPVOID *ppvObject)
{
    wordbreaker_impl *This;
    IWordBreaker *wb;

    FIXME("%p %s %p\n", pUnkOuter, debugstr_guid(riid), ppvObject);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof *This);
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = &wordbreaker_vtbl;

    wb = (IWordBreaker*) This;

    return IWordBreaker_QueryInterface(wb, riid, ppvObject);
}
