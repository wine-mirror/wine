/*
 * Sample Wine Driver for Advanced Linux Sound System (ALSA)
 *      Based on version <final> of the ALSA API
 *
 * Copyright 2007 - Maarten Lankhorst
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

/*======================================================================*
 *              Low level dsound input implementation			*
 *======================================================================*/

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "winuser.h"
#include "mmddk.h"

#include "alsa.h"
#include "wine/library.h"
#include "wine/unicode.h"
#include "wine/debug.h"

#ifdef HAVE_ALSA

WINE_DEFAULT_DEBUG_CHANNEL(dsalsa);

typedef struct IDsCaptureDriverImpl IDsCaptureDriverImpl;

struct IDsCaptureDriverImpl
{
    /* IUnknown fields */
    const IDsCaptureDriverVtbl *lpVtbl;
    LONG ref;

    /* IDsCaptureDriverImpl fields */
    UINT wDevID;
};

static HRESULT WINAPI IDsCaptureDriverImpl_QueryInterface(PIDSCDRIVER iface, REFIID riid, LPVOID *ppobj)
{
    /* IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface; */
    FIXME("(%p): stub!\n",iface);
    return DSERR_UNSUPPORTED;
}

static ULONG WINAPI IDsCaptureDriverImpl_AddRef(PIDSCDRIVER iface)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsCaptureDriverImpl_Release(PIDSCDRIVER iface)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before=%u)\n",This, refCount + 1);

    if (refCount)
        return refCount;

    HeapFree(GetProcessHeap(), 0, This);
    return 0;
}

static HRESULT WINAPI IDsCaptureDriverImpl_GetDriverDesc(PIDSCDRIVER iface, PDSDRIVERDESC pDesc)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    TRACE("(%p,%p)\n",iface,pDesc);
    memcpy(pDesc, &(WInDev[This->wDevID].ds_desc), sizeof(DSDRIVERDESC));
    pDesc->dwFlags		= 0;
    pDesc->dnDevNode		= WInDev[This->wDevID].waveDesc.dnDevNode;
    pDesc->wVxdId		= 0;
    pDesc->wReserved		= 0;
    pDesc->ulDeviceNum		= This->wDevID;
    pDesc->dwHeapType		= DSDHEAP_NOHEAP;
    pDesc->pvDirectDrawHeap	= NULL;
    pDesc->dwMemStartAddress	= 0xDEAD0000;
    pDesc->dwMemEndAddress	= 0xDEAF0000;
    pDesc->dwMemAllocExtra	= 0;
    pDesc->pvReserved1		= NULL;
    pDesc->pvReserved2		= NULL;
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverImpl_Open(PIDSCDRIVER iface)
{
    FIXME("stub\n");
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverImpl_Close(PIDSCDRIVER iface)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    TRACE("(%p) stub, harmless\n",This);
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverImpl_GetCaps(PIDSCDRIVER iface, PDSCDRIVERCAPS pCaps)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    WINE_WAVEDEV *wwi = &WInDev[This->wDevID];
    TRACE("(%p,%p)\n",iface,pCaps);
    pCaps->dwSize = sizeof(DSCDRIVERCAPS);
    pCaps->dwFlags = wwi->ds_caps.dwFlags;
    pCaps->dwFormats = wwi->incaps.dwFormats;
    pCaps->dwChannels = wwi->incaps.wChannels;
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverImpl_CreateCaptureBuffer(PIDSCDRIVER iface,
						      LPWAVEFORMATEX pwfx,
						      DWORD dwFlags, DWORD dwCardAddress,
						      LPDWORD pdwcbBufferSize,
						      LPBYTE *ppbBuffer,
						      LPVOID *ppvObj)
{
    FIXME("stub!\n");
    return DSERR_GENERIC;
}

static const IDsCaptureDriverVtbl dscdvt =
{
    IDsCaptureDriverImpl_QueryInterface,
    IDsCaptureDriverImpl_AddRef,
    IDsCaptureDriverImpl_Release,
    IDsCaptureDriverImpl_GetDriverDesc,
    IDsCaptureDriverImpl_Open,
    IDsCaptureDriverImpl_Close,
    IDsCaptureDriverImpl_GetCaps,
    IDsCaptureDriverImpl_CreateCaptureBuffer
};

/**************************************************************************
 *                              widDsCreate                     [internal]
 */
DWORD widDsCreate(UINT wDevID, PIDSCDRIVER* drv)
{
    IDsCaptureDriverImpl** idrv = (IDsCaptureDriverImpl**)drv;
    TRACE("(%d,%p)\n",wDevID,drv);

    if (!(WInDev[wDevID].dwSupport & WAVECAPS_DIRECTSOUND))
    {
        WARN("Hardware accelerated capture not supported, falling back to wavein\n");
        return MMSYSERR_NOTSUPPORTED;
    }

    *idrv = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDsCaptureDriverImpl));
    if (!*idrv)
        return MMSYSERR_NOMEM;
    (*idrv)->lpVtbl	= &dscdvt;
    (*idrv)->ref	= 1;

    (*idrv)->wDevID	= wDevID;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 *                              widDsDesc                       [internal]
 */
DWORD widDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    memcpy(desc, &(WInDev[wDevID].ds_desc), sizeof(DSDRIVERDESC));
    return MMSYSERR_NOERROR;
}

#endif /* HAVE_ALSA */
