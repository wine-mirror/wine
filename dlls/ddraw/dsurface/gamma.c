/*		DirectDrawGammaControl implementation
 *
 * Copyright 2001 TransGaming Technologies Inc.
 */

#include "config.h"
#include "winerror.h"

#include <assert.h>
#include <stdlib.h>

#include "debugtools.h"
#include "ddraw_private.h"
#include "dsurface/main.h"

DEFAULT_DEBUG_CHANNEL(ddraw);

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

ICOM_VTABLE(IDirectDrawGammaControl) DDRAW_IDDGC_VTable =
{
    DirectDrawGammaControl_QueryInterface,
    DirectDrawGammaControl_AddRef,
    DirectDrawGammaControl_Release,
    DirectDrawGammaControl_GetGammaRamp,
    DirectDrawGammaControl_SetGammaRamp
};
