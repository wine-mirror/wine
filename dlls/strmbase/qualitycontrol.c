/*
 * Quality Control Interfaces
 *
 * Copyright 2010 Maarten Lankhorst for Codeweavers
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

#include "dshow.h"
#include "wine/strmbase.h"

#include "uuids.h"
#include "wine/debug.h"

#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

void QualityControlImpl_init(QualityControlImpl *This, IPin *input, IBaseFilter *self) {
    This->input = input;
    This->self = self;
    This->tonotify = NULL;
}

HRESULT WINAPI QualityControlImpl_QueryInterface(IQualityControl *iface, REFIID riid, void **ppv) {
    QualityControlImpl *This = (QualityControlImpl*)iface;
    return IUnknown_QueryInterface(This->self, riid, ppv);
}

ULONG WINAPI QualityControlImpl_AddRef(IQualityControl *iface) {
    QualityControlImpl *This = (QualityControlImpl*)iface;
    return IUnknown_AddRef(This->self);
}

ULONG WINAPI QualityControlImpl_Release(IQualityControl *iface) {
    QualityControlImpl *This = (QualityControlImpl*)iface;
    return IUnknown_Release(This->self);
}

HRESULT WINAPI QualityControlImpl_Notify(IQualityControl *iface, IBaseFilter *sender, Quality qm) {
    HRESULT hr = S_FALSE;
    QualityControlImpl *This = (QualityControlImpl*)iface;
    if (This->tonotify)
        return IQualityControl_Notify(This->tonotify, This->self, qm);
    if (This->input) {
        IPin *to = NULL;
        IPin_ConnectedTo(This->input, &to);
        if (to) {
            IQualityControl *qc = NULL;
            IUnknown_QueryInterface(to, &IID_IQualityControl, (void**)&qc);
            if (qc) {
                hr = IQualityControl_Notify(qc, This->self, qm);
                IQualityControl_Release(qc);
            }
            IPin_Release(to);
        }
    }
    return hr;
}

HRESULT WINAPI QualityControlImpl_SetSink(IQualityControl *iface, IQualityControl *tonotify) {
    QualityControlImpl *This = (QualityControlImpl*)iface;
    This->tonotify = tonotify;
    return S_OK;
}
