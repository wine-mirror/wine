/*
 * Header file for private strmbase implementations
 *
 * Copyright 2012 Aric Stewart, CodeWeavers
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

#ifndef __WINE_STRMBASE_PRIVATE_H
#define __WINE_STRMBASE_PRIVATE_H

#include <assert.h>
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS
#include "dshow.h"
#include "uuids.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "wine/strmbase.h"
#include "wine/unicode.h"

/* Quality Control */
typedef struct QualityControlImpl {
    IQualityControl IQualityControl_iface;
    IPin *input;
    IBaseFilter *self;
    IQualityControl *tonotify;

    /* Render stuff */
    IReferenceClock *clock;
    REFERENCE_TIME last_in_time, last_left, avg_duration, avg_pt, avg_render, start, stop;
    REFERENCE_TIME current_jitter, current_rstart, current_rstop, clockstart;
    double avg_rate;
    LONG64 rendered, dropped;
    BOOL qos_handled, is_dropped;
} QualityControlImpl;

HRESULT QualityControlImpl_Create(IPin *input, IBaseFilter *self, QualityControlImpl **ppv);
void QualityControlImpl_Destroy(QualityControlImpl *This);
HRESULT WINAPI QualityControlImpl_QueryInterface(IQualityControl *iface, REFIID riid, void **ppv);
ULONG WINAPI QualityControlImpl_AddRef(IQualityControl *iface);
ULONG WINAPI QualityControlImpl_Release(IQualityControl *iface);
HRESULT WINAPI QualityControlImpl_Notify(IQualityControl *iface, IBaseFilter *sender, Quality qm);
HRESULT WINAPI QualityControlImpl_SetSink(IQualityControl *iface, IQualityControl *tonotify);

void QualityControlRender_Start(QualityControlImpl *This, REFERENCE_TIME tStart);
void QualityControlRender_SetClock(QualityControlImpl *This, IReferenceClock *clock);
HRESULT QualityControlRender_WaitFor(QualityControlImpl *This, IMediaSample *sample, HANDLE ev);
void QualityControlRender_DoQOS(QualityControlImpl *priv);
void QualityControlRender_BeginRender(QualityControlImpl *This);
void QualityControlRender_EndRender(QualityControlImpl *This);

HRESULT WINAPI EnumPins_Construct(BaseFilter *base, BaseFilter_GetPin pfn_get_pin,
        BaseFilter_GetPinCount pfn_get_pin_count, BaseFilter_GetPinVersion pfn_get_pin_version,
        IEnumPins **enum_pins);

HRESULT WINAPI RendererPosPassThru_RegisterMediaTime(IUnknown *iface, REFERENCE_TIME start);
HRESULT WINAPI RendererPosPassThru_ResetMediaTime(IUnknown *iface);
HRESULT WINAPI RendererPosPassThru_EOS(IUnknown *iface);

HRESULT WINAPI BaseDispatch_Init(BaseDispatch *disp, REFIID iid);
HRESULT WINAPI BaseDispatch_Destroy(BaseDispatch *disp);
HRESULT WINAPI BaseDispatchImpl_GetIDsOfNames(BaseDispatch *disp, REFIID iid,
        WCHAR **names, UINT count, LCID lcid, DISPID *ids);
HRESULT WINAPI BaseDispatchImpl_GetTypeInfo(BaseDispatch *disp, REFIID iid,
        UINT index, LCID lcid, ITypeInfo **typeinfo);
HRESULT WINAPI BaseDispatchImpl_GetTypeInfoCount(BaseDispatch *disp, UINT *count);

#endif /* __WINE_STRMBASE_PRIVATE_H */
