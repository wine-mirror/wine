/*              DirectShow private interfaces (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __QUARTZ_PRIVATE_INCLUDED__
#define __QUARTZ_PRIVATE_INCLUDED__

#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"

typedef struct _IFilterGraphImpl {
    ICOM_VTABLE(IGraphBuilder) *IGraphBuilder_vtbl;
    ICOM_VTABLE(IMediaControl) *IMediaControl_vtbl;
    ICOM_VTABLE(IMediaSeeking) *IMediaSeeking_vtbl;
    ICOM_VTABLE(IBasicAudio) *IBasicAudio_vtbl;
    ICOM_VTABLE(IBasicVideo) *IBasicVideo_vtbl;
    ICOM_VTABLE(IVideoWindow) *IVideoWindow_vtbl;
    ICOM_VTABLE(IMediaEventEx) *IMediaEventEx_vtbl;
    ULONG ref;
} IFilterGraphImpl;

HRESULT FILTERGRAPH_create(IUnknown *pUnkOuter, LPVOID *ppObj) ;

#endif /* __QUARTZ_PRIVATE_INCLUDED__ */
