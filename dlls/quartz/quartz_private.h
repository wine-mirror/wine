/*              DirectShow private interfaces (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
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

#ifndef __QUARTZ_PRIVATE_INCLUDED__
#define __QUARTZ_PRIVATE_INCLUDED__

#include <stdarg.h>
#include <wchar.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"
#include "wine/debug.h"
#include "wine/heap.h"
#include "wine/strmbase.h"
#include "wine/list.h"

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

/* see IAsyncReader::Request on MSDN for the explanation of this */
#define MEDIATIME_FROM_BYTES(x) ((LONGLONG)(x) * 10000000)
#define BYTES_FROM_MEDIATIME(time) ((time) / 10000000)

HRESULT dsound_render_create(IUnknown *outer, void **out) DECLSPEC_HIDDEN;
HRESULT filter_graph_create(IUnknown *outer, void **out) DECLSPEC_HIDDEN;
HRESULT filter_graph_no_thread_create(IUnknown *outer, void **out) DECLSPEC_HIDDEN;
HRESULT FilterMapper2_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT FilterMapper_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT AsyncReader_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT StdMemAllocator_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT AVIDec_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT VideoRenderer_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT VideoRendererDefault_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT QUARTZ_CreateSystemClock(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT ACMWrapper_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT VMR7Impl_create(IUnknown *pUnkOuter, LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT VMR9Impl_create(IUnknown *pUnkOuter, LPVOID *ppv) DECLSPEC_HIDDEN;

HRESULT EnumMonikerImpl_Create(IMoniker ** ppMoniker, ULONG nMonikerCount, IEnumMoniker ** ppEnum) DECLSPEC_HIDDEN;

HRESULT IEnumRegFiltersImpl_Construct(REGFILTER * pInRegFilters, const ULONG size, IEnumRegFilters ** ppEnum) DECLSPEC_HIDDEN;

extern const char * qzdebugstr_guid(const GUID * id) DECLSPEC_HIDDEN;
extern void video_unregister_windowclass(void) DECLSPEC_HIDDEN;

BOOL get_media_type(const WCHAR *filename, GUID *majortype, GUID *subtype, GUID *source_clsid) DECLSPEC_HIDDEN;

#endif /* __QUARTZ_PRIVATE_INCLUDED__ */
