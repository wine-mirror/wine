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
#include "wine/heap.h"
#include "wine/list.h"
#include "wine/strmbase.h"

static inline const char *debugstr_time(REFERENCE_TIME time)
{
    ULONGLONG abstime = time >= 0 ? time : -time;
    unsigned int i = 0, j = 0;
    char buffer[23], rev[23];

    while (abstime || i <= 8)
    {
        buffer[i++] = '0' + (abstime % 10);
        abstime /= 10;
        if (i == 7) buffer[i++] = '.';
    }
    if (time < 0) buffer[i++] = '-';

    while (i--) rev[j++] = buffer[i];
    rev[j] = 0;

    return wine_dbg_sprintf("%s", rev);
}

/* Quality Control */
typedef struct QualityControlImpl {
    IQualityControl IQualityControl_iface;
    struct strmbase_pin *pin;
    IQualityControl *tonotify;

    /* Render stuff */
    REFERENCE_TIME last_in_time, last_left, avg_duration, avg_pt, avg_render, start, stop;
    REFERENCE_TIME current_jitter, current_rstart, current_rstop, clockstart;
    double avg_rate;
    LONG64 rendered, dropped;
    BOOL qos_handled, is_dropped;
} QualityControlImpl;

HRESULT QualityControlImpl_Create(struct strmbase_pin *pin, QualityControlImpl **out);
void QualityControlImpl_Destroy(QualityControlImpl *This);
HRESULT WINAPI QualityControlImpl_QueryInterface(IQualityControl *iface, REFIID riid, void **ppv);
ULONG WINAPI QualityControlImpl_AddRef(IQualityControl *iface);
ULONG WINAPI QualityControlImpl_Release(IQualityControl *iface);
HRESULT WINAPI QualityControlImpl_Notify(IQualityControl *iface, IBaseFilter *sender, Quality qm);
HRESULT WINAPI QualityControlImpl_SetSink(IQualityControl *iface, IQualityControl *tonotify);

void QualityControlRender_Start(QualityControlImpl *This, REFERENCE_TIME tStart);
void QualityControlRender_DoQOS(QualityControlImpl *priv);
void QualityControlRender_BeginRender(QualityControlImpl *This, REFERENCE_TIME start, REFERENCE_TIME stop);
void QualityControlRender_EndRender(QualityControlImpl *This);

HRESULT WINAPI RendererPosPassThru_RegisterMediaTime(IUnknown *iface, REFERENCE_TIME start);
HRESULT WINAPI RendererPosPassThru_ResetMediaTime(IUnknown *iface);
HRESULT WINAPI RendererPosPassThru_EOS(IUnknown *iface);

#endif /* __WINE_STRMBASE_PRIVATE_H */
