/*
 * Copyright 2025 Alistair Leslie-Hughes
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

#include "initguid.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "rpcproxy.h"

#include "peninputpanel.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(inkobj);

struct input_panel
{
    ITextInputPanel ITextInputPanel_iface;
    LONG ref;
};

static inline struct input_panel *impl_from_ITextInputPanel( ITextInputPanel *iface )
{
    return CONTAINING_RECORD( iface, struct input_panel, ITextInputPanel_iface );
}

static HRESULT WINAPI input_panel_QueryInterface(ITextInputPanel *iface, REFIID riid, void **obj)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );
    TRACE( "%p, %s, %p\n", iface, debugstr_guid(riid), obj );

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown)    ||
        IsEqualIID(riid, &IID_ITextInputPanel))
    {
        *obj = &input->ITextInputPanel_iface;
    }
    else
    {
        FIXME( "interface %s not implemented\n", debugstr_guid(riid) );
        return E_NOINTERFACE;
    }

    IUnknown_AddRef( (IUnknown*)*obj );
    return S_OK;
}

static ULONG WINAPI input_panel_AddRef(ITextInputPanel *iface)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );
    return InterlockedIncrement( &input->ref );
}

static ULONG WINAPI input_panel_Release(ITextInputPanel *iface)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );
    LONG ref = InterlockedDecrement( &input->ref );
    if (!ref)
    {
        free( input );
    }
    return ref;
}

static HRESULT WINAPI input_panel_get_AttachedEditWindow(ITextInputPanel *iface, HWND *window)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, window);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_put_AttachedEditWindow(ITextInputPanel *iface, HWND window)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, window);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_get_CurrentInteractionMode(ITextInputPanel *iface, InteractionMode *mode)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_get_DefaultInPlaceState(ITextInputPanel *iface, InPlaceState *state)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_put_DefaultInPlaceState(ITextInputPanel *iface, InPlaceState state)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %d\n", input, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_get_CurrentInPlaceState(ITextInputPanel *iface, InPlaceState *state)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, state);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_get_DefaultInputArea(ITextInputPanel *iface, PanelInputArea *area)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, area);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_put_DefaultInputArea(ITextInputPanel *iface, PanelInputArea area)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %d\n", input, area);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_get_CurrentInputArea(ITextInputPanel *iface, PanelInputArea *area)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, area);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_get_CurrentCorrectionMode(ITextInputPanel *iface, CorrectionMode *mode)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_get_PreferredInPlaceDirection(ITextInputPanel *iface, InPlaceDirection *direction)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, direction);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_put_PreferredInPlaceDirection(ITextInputPanel *iface, InPlaceDirection direction)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %d\n", input, direction);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_get_ExpandPostInsertionCorrection(ITextInputPanel *iface, BOOL *expand)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, expand);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_put_ExpandPostInsertionCorrection(ITextInputPanel *iface, BOOL expand)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %d\n", input, expand);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_get_InPlaceVisibleOnFocus(ITextInputPanel *iface, BOOL *visible)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, visible);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_put_InPlaceVisibleOnFocus(ITextInputPanel *iface, BOOL visible)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %d\n", input, visible);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_get_InPlaceBoundingRectangle(ITextInputPanel *iface, RECT *bounding_rect)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, bounding_rect);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_get_PopUpCorrectionHeight(ITextInputPanel *iface, int *height)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_get_PopDownCorrectionHeight(ITextInputPanel *iface, int *height)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_CommitPendingInput(ITextInputPanel *iface)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p\n", input);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_SetInPlaceVisibility(ITextInputPanel *iface, BOOL visible)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %d\n", input, visible);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_SetInPlacePosition(ITextInputPanel *iface, int x, int y, CorrectionPosition position)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %d, %d, %d\n", input, x, y, position);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_SetInPlaceHoverTargetPosition(ITextInputPanel *iface, int x, int y)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %d, %d\n", input, x, y);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_Advise( ITextInputPanel *iface, ITextInputPanelEventSink *sink, DWORD mask)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p 0x%lx\n", input, sink, mask);

    return E_NOTIMPL;
}

static HRESULT WINAPI input_panel_Unadvise(ITextInputPanel *iface, ITextInputPanelEventSink *sink)
{
    struct input_panel *input = impl_from_ITextInputPanel( iface );

    FIXME("%p, %p\n", input, sink);

    return E_NOTIMPL;
}

static const struct ITextInputPanelVtbl text_panel_vtbl =
{
    input_panel_QueryInterface,
    input_panel_AddRef,
    input_panel_Release,
    input_panel_get_AttachedEditWindow,
    input_panel_put_AttachedEditWindow,
    input_panel_get_CurrentInteractionMode,
    input_panel_get_DefaultInPlaceState,
    input_panel_put_DefaultInPlaceState,
    input_panel_get_CurrentInPlaceState,
    input_panel_get_DefaultInputArea,
    input_panel_put_DefaultInputArea,
    input_panel_get_CurrentInputArea,
    input_panel_get_CurrentCorrectionMode,
    input_panel_get_PreferredInPlaceDirection,
    input_panel_put_PreferredInPlaceDirection,
    input_panel_get_ExpandPostInsertionCorrection,
    input_panel_put_ExpandPostInsertionCorrection,
    input_panel_get_InPlaceVisibleOnFocus,
    input_panel_put_InPlaceVisibleOnFocus,
    input_panel_get_InPlaceBoundingRectangle,
    input_panel_get_PopUpCorrectionHeight,
    input_panel_get_PopDownCorrectionHeight,
    input_panel_CommitPendingInput,
    input_panel_SetInPlaceVisibility,
    input_panel_SetInPlacePosition,
    input_panel_SetInPlaceHoverTargetPosition,
    input_panel_Advise,
    input_panel_Unadvise
};


static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

HRESULT WINAPI panelinput_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    struct input_panel *panel;

    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    if (!(panel = malloc( sizeof(*panel) ))) return E_OUTOFMEMORY;
    panel->ITextInputPanel_iface.lpVtbl = &text_panel_vtbl;
    panel->ref = 1;

    *ppv = &panel->ITextInputPanel_iface;
    TRACE( "returning iface %p\n", *ppv );
    return S_OK;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static const IClassFactoryVtbl cfpanelinputlVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    panelinput_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory cf_ink = { &cfpanelinputlVtbl };

HRESULT WINAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, void **ppv )
{
    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (IsEqualGUID(&CLSID_TextInputPanel, rclsid))
    {
        return IClassFactory_QueryInterface(&cf_ink, riid, ppv);
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}
