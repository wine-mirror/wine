/*
 *    Misc marshaling routinues
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
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

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

HRESULT CALLBACK IDispatchEx_InvokeEx_Proxy(IDispatchEx* This, DISPID id, LCID lcid, WORD wFlags,
                                            DISPPARAMS *pdp, VARIANT *pvarRes, EXCEPINFO *pei,
                                            IServiceProvider *pspCaller)
{
    FIXME("(%p)->(%08x, %04x, %04x, %p, %p, %p, %p): stub\n", This, id, lcid, wFlags,
          pdp, pvarRes, pei, pspCaller);
    return E_NOTIMPL;
}

HRESULT __RPC_STUB IDispatchEx_InvokeEx_Stub(IDispatchEx* This, DISPID id, LCID lcid, DWORD dwFlags,
                                             DISPPARAMS *pdp, VARIANT *pvarRes, EXCEPINFO *pei,
                                             IServiceProvider *pspCaller, UINT cvarRefArg,
                                             UINT *rgiRefArg, VARIANT *rgvarRefArg)
{
    FIXME("(%p)->(%08x, %04x, %08x, %p, %p, %p, %p, %d, %p, %p): stub\n", This, id, lcid, dwFlags,
          pdp, pvarRes, pei, pspCaller, cvarRefArg, rgiRefArg, rgvarRefArg);
    return E_NOTIMPL;

}
