/*
 *    Misc marshaling routines
 *
 * Copyright 2010 Huw Davies
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
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "objbase.h"
#include "oleauto.h"
#include "dispex.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define NULL_RESULT    0x20000
#define NULL_EXCEPINFO 0x40000

HRESULT CALLBACK IDispatchEx_InvokeEx_Proxy(IDispatchEx* This, DISPID id, LCID lcid, WORD wFlags,
                                            DISPPARAMS *pdp, VARIANT *pvarRes, EXCEPINFO *pei,
                                            IServiceProvider *pspCaller)
{
    HRESULT hr;
    VARIANT result;
    EXCEPINFO excep_info;
    UINT byref_args, arg, dummy_idx;
    VARIANT dummy_arg, *ref_arg = &dummy_arg, *copy_arg, *orig_arg = NULL;
    UINT *ref_idx = &dummy_idx;
    DWORD dword_flags = wFlags & 0xf;

    TRACE("(%p)->(%08lx, %04lx, %04x, %p, %p, %p, %p)\n", This, id, lcid, wFlags,
          pdp, pvarRes, pei, pspCaller);

    if(!pvarRes)
    {
        pvarRes = &result;
        dword_flags |= NULL_RESULT;
    }

    if(!pei)
    {
        pei = &excep_info;
        dword_flags |= NULL_EXCEPINFO;
    }

    for(arg = 0, byref_args = 0; arg < pdp->cArgs; arg++)
        if(V_ISBYREF(pdp->rgvarg + arg)) byref_args++;

    if(byref_args)
    {
        DWORD size = pdp->cArgs * sizeof(VARIANT) +
            byref_args * (sizeof(VARIANT) + sizeof(UINT));

        copy_arg = CoTaskMemAlloc(size);
        if(!copy_arg) return E_OUTOFMEMORY;

        ref_arg = copy_arg + pdp->cArgs;
        ref_idx = (UINT*)(ref_arg + byref_args);

        /* copy the byref args to ref_arg[], the others go to copy_arg[] */
        for(arg = 0, byref_args = 0; arg < pdp->cArgs; arg++)
        {
            if(V_ISBYREF(pdp->rgvarg + arg))
            {
                ref_arg[byref_args] = pdp->rgvarg[arg];
                ref_idx[byref_args] = arg;
                VariantInit(copy_arg + arg);
                byref_args++;
            }
            else
                copy_arg[arg] = pdp->rgvarg[arg];
        }

        orig_arg = pdp->rgvarg;
        pdp->rgvarg = copy_arg;
    }

    hr = IDispatchEx_RemoteInvokeEx_Proxy(This, id, lcid, dword_flags, pdp, pvarRes, pei, pspCaller,
                                          byref_args, ref_idx, ref_arg);

    if(byref_args)
    {
        CoTaskMemFree(pdp->rgvarg);
        pdp->rgvarg = orig_arg;
    }

    return hr;
}

HRESULT __RPC_STUB IDispatchEx_InvokeEx_Stub(IDispatchEx* This, DISPID id, LCID lcid, DWORD dwFlags,
                                             DISPPARAMS *pdp, VARIANT *result, EXCEPINFO *pei,
                                             IServiceProvider *pspCaller, UINT byref_args,
                                             UINT *ref_idx, VARIANT *ref_arg)
{
    HRESULT hr;
    UINT arg;
    VARTYPE *vt_list = NULL;

    TRACE("(%p)->(%08lx, %04lx, %08lx, %p, %p, %p, %p, %d, %p, %p)\n", This, id, lcid, dwFlags,
          pdp, result, pei, pspCaller, byref_args, ref_idx, ref_arg);

    VariantInit(result);
    memset(pei, 0, sizeof(*pei));

    for(arg = 0; arg < byref_args; arg++)
        pdp->rgvarg[ref_idx[arg]] = ref_arg[arg];

    if(dwFlags & NULL_RESULT) result = NULL;
    if(dwFlags & NULL_EXCEPINFO) pei = NULL;

    /* Create an array of the original VTs to check that the function doesn't change
       any on return. */
    if(byref_args)
    {
        vt_list = malloc(pdp->cArgs * sizeof(vt_list[0]));
        if(!vt_list) return E_OUTOFMEMORY;
        for(arg = 0; arg < pdp->cArgs; arg++)
            vt_list[arg] = V_VT(pdp->rgvarg + arg);
    }

    hr = IDispatchEx_InvokeEx(This, id, lcid, dwFlags & 0xffff, pdp, result, pei, pspCaller);

    if(SUCCEEDED(hr) && byref_args)
    {
        for(arg = 0; arg < pdp->cArgs; arg++)
        {
            if(vt_list[arg] != V_VT(pdp->rgvarg + arg))
            {
                hr = DISP_E_BADCALLEE;
                break;
            }
        }
    }

    if(hr == DISP_E_EXCEPTION)
    {
        if(pei && pei->pfnDeferredFillIn)
        {
            pei->pfnDeferredFillIn(pei);
            pei->pfnDeferredFillIn = NULL;
        }
    }

    for(arg = 0; arg < byref_args; arg++)
        VariantInit(pdp->rgvarg + ref_idx[arg]);

    free(vt_list);
    return hr;
}
