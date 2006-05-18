/*		DirectDrawGammaControl implementation
 *
 * Copyright 2001 TransGaming Technologies Inc.
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

#include "config.h"
#include "winerror.h"

#include <assert.h>
#include <stdlib.h>

#include "wine/debug.h"
#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

#define CONVERT(pddgc) COM_INTERFACE_CAST(IDirectDrawSurfaceImpl,	\
					  IDirectDrawGammaControl,	\
					  IDirectDrawSurface7,		\
					  (pddgc))

static HRESULT WINAPI
DirectDrawGammaControl_QueryInterface(LPDIRECTDRAWGAMMACONTROL iface, REFIID riid,
				      LPVOID *ppObj)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ppObj);
    return E_NOINTERFACE;
}

static ULONG WINAPI
DirectDrawGammaControl_AddRef(LPDIRECTDRAWGAMMACONTROL iface)
{
    return IDirectDrawSurface7_AddRef(CONVERT(iface));
}

static ULONG WINAPI
DirectDrawGammaControl_Release(LPDIRECTDRAWGAMMACONTROL iface)
{
    return IDirectDrawSurface7_Release(CONVERT(iface));
}

static HRESULT WINAPI
DirectDrawGammaControl_GetGammaRamp(LPDIRECTDRAWGAMMACONTROL iface, DWORD dwFlags, LPDDGAMMARAMP lpGammaRamp)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawGammaControl, iface);
    TRACE("(%p)->(%08lx,%p)\n", iface,dwFlags,lpGammaRamp);
    return This->get_gamma_ramp(This, dwFlags, lpGammaRamp);
}

static HRESULT WINAPI
DirectDrawGammaControl_SetGammaRamp(LPDIRECTDRAWGAMMACONTROL iface, DWORD dwFlags, LPDDGAMMARAMP lpGammaRamp)
{
    ICOM_THIS_FROM(IDirectDrawSurfaceImpl, IDirectDrawGammaControl, iface);
    TRACE("(%p)->(%08lx,%p)\n", iface,dwFlags,lpGammaRamp);
    return This->set_gamma_ramp(This, dwFlags, lpGammaRamp);
}

const IDirectDrawGammaControlVtbl DDRAW_IDDGC_VTable =
{
    DirectDrawGammaControl_QueryInterface,
    DirectDrawGammaControl_AddRef,
    DirectDrawGammaControl_Release,
    DirectDrawGammaControl_GetGammaRamp,
    DirectDrawGammaControl_SetGammaRamp
};
