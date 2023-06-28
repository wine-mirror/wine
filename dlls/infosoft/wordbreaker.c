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


#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "indexsrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(infosoft);

typedef struct tag_wordbreaker_impl
{
    IWordBreaker IWordBreaker_iface;
    LONG ref;
} wordbreaker_impl;

static inline wordbreaker_impl *impl_from_IWordBreaker(IWordBreaker *iface)
{
    return CONTAINING_RECORD(iface, wordbreaker_impl, IWordBreaker_iface);
}

static HRESULT WINAPI wb_QueryInterface( IWordBreaker *iface,
        REFIID riid, LPVOID *ppvObj)
{
    wordbreaker_impl *This = impl_from_IWordBreaker(iface);

    TRACE("(%p)->(%s)\n",This,debugstr_guid(riid));

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IWordBreaker))
    {
        *ppvObj = &This->IWordBreaker_iface;
        return S_OK;
    }

    TRACE("-- E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI wb_AddRef( IWordBreaker *iface )
{
    wordbreaker_impl *This = impl_from_IWordBreaker(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI wb_Release(IWordBreaker *iface)
{
    wordbreaker_impl *This = impl_from_IWordBreaker(iface);
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

static HRESULT call_sink( IWordSink *pWordSink, TEXT_SOURCE *ts, UINT len )
{
    HRESULT r;

    if (!len)
        return S_OK;

    TRACE("%d %s\n", len, debugstr_w(&ts->awcBuffer[ts->iCur]));

    r = IWordSink_PutWord( pWordSink, len, &ts->awcBuffer[ts->iCur], len, ts->iCur );
    ts->iCur += len;

    return r;
}

static HRESULT WINAPI wb_BreakText( IWordBreaker *iface,
         TEXT_SOURCE *ts, IWordSink *pWordSink, IPhraseSink *pPhraseSink)
{
    UINT len, state = 0;
    WCHAR ch;

    TRACE("%p %p %p\n", ts, pWordSink, pPhraseSink);

    if (pPhraseSink)
        FIXME("IPhraseSink won't be called\n");

    do
    {
        len = 0;
        while ((ts->iCur + len) < ts->iEnd)
        {
            ch = ts->awcBuffer[ts->iCur + len];

            switch (state)
            {
            case 0: /* skip spaces and punctuation */

                if (!ch || iswpunct(ch) || iswspace(ch))
                    ts->iCur ++;
                else
                    state = 1;
                break;

            case 1: /* find the end of the word */

                if (ch && !iswpunct(ch) && !iswspace(ch))
                    len++;
                else
                {
                    call_sink( pWordSink, ts, len );
                    len = 0;
                    state = 0;
                }
                break;

            }
        }
        call_sink( pWordSink, ts, len );

    } while (S_OK == ts->pfnFillTextBuffer( ts ));

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

HRESULT WINAPI wb_Constructor(IUnknown* pUnkOuter, REFIID riid, LPVOID *ppvObject)
{
    wordbreaker_impl *This;
    IWordBreaker *wb;

    TRACE("%p %s %p\n", pUnkOuter, debugstr_guid(riid), ppvObject);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof *This);
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->IWordBreaker_iface.lpVtbl = &wordbreaker_vtbl;

    wb = &This->IWordBreaker_iface;

    return IWordBreaker_QueryInterface(wb, riid, ppvObject);
}
