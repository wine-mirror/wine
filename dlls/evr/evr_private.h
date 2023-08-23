/*
 * Copyright 2017 Fabian Maurer
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

#ifndef __EVR_PRIVATE_INCLUDED__
#define __EVR_PRIVATE_INCLUDED__

#include "dshow.h"
#include "evr.h"
#include "wine/strmbase.h"
#include "wine/debug.h"

static inline const char *debugstr_time(LONGLONG time)
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
    while (rev[j-1] == '0' && rev[j-2] != '.') --j;
    rev[j] = 0;

    return wine_dbg_sprintf("%s", rev);
}

static inline const char *debugstr_normalized_rect(const MFVideoNormalizedRect *rect)
{
    if (!rect) return "(null)";
    return wine_dbg_sprintf("(%.8e,%.8e)-(%.8e,%.8e)", rect->left, rect->top, rect->right, rect->bottom);
}

HRESULT evr_filter_create(IUnknown *outer_unk, void **ppv);
HRESULT evr_mixer_create(IUnknown *outer_unk, void **ppv);
HRESULT evr_presenter_create(IUnknown *outer_unk, void **ppv);

HRESULT create_video_sample_allocator(BOOL lock_notify_release, REFIID riid, void **obj);

#endif /* __EVR_PRIVATE_INCLUDED__ */
