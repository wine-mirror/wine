/*
 * Direct Sound Capture driver
 *
 * Copyright 2004 Robert Reif
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
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "wine/winuser16.h"
#include "mmddk.h"
#include "mmreg.h"
#include "dsound.h"
#include "dsdriver.h"
#include "oss.h"
#include "wine/debug.h"

#include "audio.h"

WINE_DEFAULT_DEBUG_CHANNEL(wave);

#ifdef HAVE_OSS

/*======================================================================*
 *           Low level DSOUND capture definitions                       *
 *======================================================================*/

typedef struct IDsCaptureDriverPropertySetImpl IDsCaptureDriverPropertySetImpl;
typedef struct IDsCaptureDriverNotifyImpl IDsCaptureDriverNotifyImpl;
typedef struct IDsCaptureDriverImpl IDsCaptureDriverImpl;
typedef struct IDsCaptureDriverBufferImpl IDsCaptureDriverBufferImpl;

struct IDsCaptureDriverPropertySetImpl
{
    /* IUnknown fields */
    IDsDriverPropertySetVtbl           *lpVtbl;
    DWORD                               ref;

    IDsCaptureDriverBufferImpl*         capture_buffer;
};

struct IDsCaptureDriverNotifyImpl
{
    /* IUnknown fields */
    IDsDriverNotifyVtbl                *lpVtbl;
    DWORD                               ref;

    IDsCaptureDriverBufferImpl*         capture_buffer;
};

struct IDsCaptureDriverImpl
{
    /* IUnknown fields */
    IDsCaptureDriverVtbl               *lpVtbl;
    DWORD                               ref;

    /* IDsCaptureDriverImpl fields */
    UINT                                wDevID;
    IDsCaptureDriverBufferImpl*         capture_buffer;
};

struct IDsCaptureDriverBufferImpl
{
    /* IUnknown fields */
    IDsCaptureDriverBufferVtbl         *lpVtbl;
    DWORD                               ref;

    /* IDsCaptureDriverBufferImpl fields */
    IDsCaptureDriverImpl*               drv;
    DWORD                               buflen;
    LPBYTE                              buffer;
    DWORD                               writeptr;
    LPBYTE                              mapping;
    DWORD                               maplen;

    /* IDsDriverNotifyImpl fields */
    IDsCaptureDriverNotifyImpl*         notify;
    int                                 notify_index;
    LPDSBPOSITIONNOTIFY                 notifies;
    int                                 nrofnotifies;

    /* IDsDriverPropertySetImpl fields */
    IDsCaptureDriverPropertySetImpl*    property_set;

    BOOL                                is_capturing;
    BOOL                                is_looping;
};

static HRESULT WINAPI IDsCaptureDriverPropertySetImpl_Create(
    IDsCaptureDriverBufferImpl * dscdb,
    IDsCaptureDriverPropertySetImpl **pdscdps);

static HRESULT WINAPI IDsCaptureDriverNotifyImpl_Create(
    IDsCaptureDriverBufferImpl * dsdcb,
    IDsCaptureDriverNotifyImpl **pdscdn);

/*======================================================================*
 *           Low level DSOUND capture property set implementation       *
 *======================================================================*/

static HRESULT WINAPI IDsCaptureDriverPropertySetImpl_QueryInterface(
    PIDSDRIVERPROPERTYSET iface,
    REFIID riid,
    LPVOID *ppobj)
{
    IDsCaptureDriverPropertySetImpl *This = (IDsCaptureDriverPropertySetImpl *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    if ( IsEqualGUID(riid, &IID_IUnknown) ||
         IsEqualGUID(riid, &IID_IDsDriverPropertySet) ) {
        IDsDriverPropertySet_AddRef(iface);
        *ppobj = (LPVOID)This;
        return DS_OK;
    }

    FIXME( "Unknown IID %s\n", debugstr_guid( riid ) );

    *ppobj = 0;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDsCaptureDriverPropertySetImpl_AddRef(
    PIDSDRIVERPROPERTYSET iface)
{
    IDsCaptureDriverPropertySetImpl *This = (IDsCaptureDriverPropertySetImpl *)iface;
    TRACE("(%p) ref was %ld\n", This, This->ref);

    return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI IDsCaptureDriverPropertySetImpl_Release(
    PIDSDRIVERPROPERTYSET iface)
{
    IDsCaptureDriverPropertySetImpl *This = (IDsCaptureDriverPropertySetImpl *)iface;
    DWORD ref;
    TRACE("(%p) ref was %ld\n", This, This->ref);

    ref = InterlockedDecrement(&(This->ref));
    if (ref == 0) {
        IDsCaptureDriverBuffer_Release((PIDSCDRIVERBUFFER)This->capture_buffer);
        This->capture_buffer->property_set = NULL;
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return ref;
}

static HRESULT WINAPI IDsCaptureDriverPropertySetImpl_Get(
    PIDSDRIVERPROPERTYSET iface,
    PDSPROPERTY pDsProperty,
    LPVOID pPropertyParams,
    ULONG cbPropertyParams,
    LPVOID pPropertyData,
    ULONG cbPropertyData,
    PULONG pcbReturnedData )
{
    IDsCaptureDriverPropertySetImpl *This = (IDsCaptureDriverPropertySetImpl *)iface;
    FIXME("(%p,%p,%p,%lx,%p,%lx,%p)\n",This,pDsProperty,pPropertyParams,
          cbPropertyParams,pPropertyData,cbPropertyData,pcbReturnedData);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsCaptureDriverPropertySetImpl_Set(
    PIDSDRIVERPROPERTYSET iface,
    PDSPROPERTY pDsProperty,
    LPVOID pPropertyParams,
    ULONG cbPropertyParams,
    LPVOID pPropertyData,
    ULONG cbPropertyData )
{
    IDsCaptureDriverPropertySetImpl *This = (IDsCaptureDriverPropertySetImpl *)iface;
    FIXME("(%p,%p,%p,%lx,%p,%lx)\n",This,pDsProperty,pPropertyParams,
          cbPropertyParams,pPropertyData,cbPropertyData);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsCaptureDriverPropertySetImpl_QuerySupport(
    PIDSDRIVERPROPERTYSET iface,
    REFGUID PropertySetId,
    ULONG PropertyId,
    PULONG pSupport )
{
    IDsCaptureDriverPropertySetImpl *This = (IDsCaptureDriverPropertySetImpl *)iface;
    FIXME("(%p,%s,%lx,%p)\n",This,debugstr_guid(PropertySetId),PropertyId,
          pSupport);
    return DSERR_UNSUPPORTED;
}

IDsDriverPropertySetVtbl dscdpsvt =
{
    IDsCaptureDriverPropertySetImpl_QueryInterface,
    IDsCaptureDriverPropertySetImpl_AddRef,
    IDsCaptureDriverPropertySetImpl_Release,
    IDsCaptureDriverPropertySetImpl_Get,
    IDsCaptureDriverPropertySetImpl_Set,
    IDsCaptureDriverPropertySetImpl_QuerySupport,
};

/*======================================================================*
 *                  Low level DSOUND capture notify implementation      *
 *======================================================================*/

static HRESULT WINAPI IDsCaptureDriverNotifyImpl_QueryInterface(
    PIDSDRIVERNOTIFY iface,
    REFIID riid,
    LPVOID *ppobj)
{
    IDsCaptureDriverNotifyImpl *This = (IDsCaptureDriverNotifyImpl *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    if ( IsEqualGUID(riid, &IID_IUnknown) ||
         IsEqualGUID(riid, &IID_IDsDriverNotify) ) {
        IDsDriverNotify_AddRef(iface);
        *ppobj = This;
        return DS_OK;
    }

    FIXME( "Unknown IID %s\n", debugstr_guid( riid ) );

    *ppobj = 0;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDsCaptureDriverNotifyImpl_AddRef(
    PIDSDRIVERNOTIFY iface)
{
    IDsCaptureDriverNotifyImpl *This = (IDsCaptureDriverNotifyImpl *)iface;
    TRACE("(%p) ref was %ld\n", This, This->ref);

    return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI IDsCaptureDriverNotifyImpl_Release(
    PIDSDRIVERNOTIFY iface)
{
    IDsCaptureDriverNotifyImpl *This = (IDsCaptureDriverNotifyImpl *)iface;
    DWORD ref;
    TRACE("(%p) ref was %ld\n", This, This->ref);

    ref = InterlockedDecrement(&(This->ref));
    if (ref == 0) {
        IDsCaptureDriverBuffer_Release((PIDSCDRIVERBUFFER)This->capture_buffer);
        This->capture_buffer->notify = NULL;
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return ref;
}

static HRESULT WINAPI IDsCaptureDriverNotifyImpl_SetNotificationPositions(
    PIDSDRIVERNOTIFY iface,
    DWORD howmuch,
    LPCDSBPOSITIONNOTIFY notify)
{
    IDsCaptureDriverNotifyImpl *This = (IDsCaptureDriverNotifyImpl *)iface;
    TRACE("(%p,0x%08lx,%p)\n",This,howmuch,notify);

    if (!notify) {
        WARN("invalid parameter\n");
        return DSERR_INVALIDPARAM;
    }

    if (TRACE_ON(wave)) {
        int i;
        for (i=0;i<howmuch;i++)
            TRACE("notify at %ld to 0x%08lx\n",
                notify[i].dwOffset,(DWORD)notify[i].hEventNotify);
    }

    /* Make an internal copy of the caller-supplied array.
     * Replace the existing copy if one is already present. */
    if (This->capture_buffer->notifies)
        This->capture_buffer->notifies = HeapReAlloc(GetProcessHeap(),
            HEAP_ZERO_MEMORY, This->capture_buffer->notifies,
            howmuch * sizeof(DSBPOSITIONNOTIFY));
    else
        This->capture_buffer->notifies = HeapAlloc(GetProcessHeap(),
            HEAP_ZERO_MEMORY, howmuch * sizeof(DSBPOSITIONNOTIFY));

    memcpy(This->capture_buffer->notifies, notify,
           howmuch * sizeof(DSBPOSITIONNOTIFY));
    This->capture_buffer->nrofnotifies = howmuch;

    return S_OK;
}

IDsDriverNotifyVtbl dscdnvt =
{
    IDsCaptureDriverNotifyImpl_QueryInterface,
    IDsCaptureDriverNotifyImpl_AddRef,
    IDsCaptureDriverNotifyImpl_Release,
    IDsCaptureDriverNotifyImpl_SetNotificationPositions,
};

/*======================================================================*
 *                  Low level DSOUND capture implementation             *
 *======================================================================*/

static HRESULT DSCDB_MapBuffer(IDsCaptureDriverBufferImpl *dscdb)
{
    if (!dscdb->mapping) {
        dscdb->mapping = mmap(NULL, dscdb->maplen, PROT_READ, MAP_SHARED,
                              WInDev[dscdb->drv->wDevID].ossdev->fd, 0);
        if (dscdb->mapping == (LPBYTE)-1) {
            TRACE("(%p): Could not map sound device for direct access (%s)\n",
                  dscdb, strerror(errno));
            return DSERR_GENERIC;
        }
        TRACE("(%p): sound device has been mapped for direct access at %p, size=%ld\n", dscdb, dscdb->mapping, dscdb->maplen);
    }
    return DS_OK;
}

static HRESULT DSCDB_UnmapBuffer(IDsCaptureDriverBufferImpl *dscdb)
{
    if (dscdb->mapping) {
        if (munmap(dscdb->mapping, dscdb->maplen) < 0) {
            ERR("(%p): Could not unmap sound device (%s)\n",
                dscdb, strerror(errno));
            return DSERR_GENERIC;
        }
        dscdb->mapping = NULL;
        TRACE("(%p): sound device unmapped\n", dscdb);
    }
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_QueryInterface(
    PIDSCDRIVERBUFFER iface,
    REFIID riid,
    LPVOID *ppobj)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    *ppobj = 0;

    if ( IsEqualGUID(riid, &IID_IUnknown) ||
         IsEqualGUID(riid, &IID_IDsCaptureDriverBuffer) ) {
        IDsCaptureDriverBuffer_AddRef(iface);
        *ppobj = (LPVOID)This;
        return DS_OK;
    }

    if ( IsEqualGUID( &IID_IDsDriverNotify, riid ) ) {
        if (!This->notify)
            IDsCaptureDriverNotifyImpl_Create(This, &(This->notify));
        if (This->notify) {
            IDsDriverNotify_AddRef((PIDSDRIVERNOTIFY)This->notify);
            *ppobj = (LPVOID)This->notify;
            return DS_OK;
        }
        return E_FAIL;
    }

    if ( IsEqualGUID( &IID_IDsDriverPropertySet, riid ) ) {
        if (!This->property_set)
            IDsCaptureDriverPropertySetImpl_Create(This, &(This->property_set));
        if (This->property_set) {
            IDsDriverPropertySet_AddRef((PIDSDRIVERPROPERTYSET)This->property_set);
            *ppobj = (LPVOID)This->property_set;
            return DS_OK;
        }
        return E_FAIL;
    }

    FIXME("(%p,%s,%p) unsupported GUID\n", This, debugstr_guid(riid), ppobj);
    return DSERR_UNSUPPORTED;
}

static ULONG WINAPI IDsCaptureDriverBufferImpl_AddRef(PIDSCDRIVERBUFFER iface)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    TRACE("(%p) ref was %ld\n", This, This->ref);

    return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI IDsCaptureDriverBufferImpl_Release(PIDSCDRIVERBUFFER iface)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    DWORD ref;
    TRACE("(%p) ref was %ld\n", This, This->ref);

    ref = InterlockedDecrement(&(This->ref));
    if (ref == 0) {
        DSCDB_UnmapBuffer(This);
        if (This->notifies != NULL)
            HeapFree(GetProcessHeap(), 0, This->notifies);
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return ref;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_Lock(
    PIDSCDRIVERBUFFER iface,
    LPVOID* ppvAudio1,
    LPDWORD pdwLen1,
    LPVOID* ppvAudio2,
    LPDWORD pdwLen2,
    DWORD dwWritePosition,
    DWORD dwWriteLen,
    DWORD dwFlags)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    FIXME("(%p,%p,%p,%p,%p,%ld,%ld,0x%08lx): stub!\n",This,ppvAudio1,pdwLen1,
          ppvAudio2,pdwLen2,dwWritePosition,dwWriteLen,dwFlags);
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_Unlock(
    PIDSCDRIVERBUFFER iface,
    LPVOID pvAudio1,
    DWORD dwLen1,
    LPVOID pvAudio2,
    DWORD dwLen2)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    FIXME("(%p,%p,%ld,%p,%ld): stub!\n",This,pvAudio1,dwLen1,pvAudio2,dwLen2);
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_GetPosition(
    PIDSCDRIVERBUFFER iface,
    LPDWORD lpdwCapture,
    LPDWORD lpdwRead)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    count_info info;
    DWORD ptr;
    TRACE("(%p,%p,%p)\n",This,lpdwCapture,lpdwRead);

    if (WInDev[This->drv->wDevID].state == WINE_WS_CLOSED) {
        ERR("device not open, but accessing?\n");
        return DSERR_UNINITIALIZED;
    }

    if (!This->is_capturing) {
        if (lpdwCapture)
            *lpdwCapture = 0;
        if (lpdwRead)
            *lpdwRead = 0;
    }

    if (ioctl(WInDev[This->drv->wDevID].ossdev->fd, SNDCTL_DSP_GETIPTR, &info) < 0) {
        ERR("ioctl(%s, SNDCTL_DSP_GETIPTR) failed (%s)\n",
            WInDev[This->drv->wDevID].ossdev->dev_name, strerror(errno));
        return DSERR_GENERIC;
    }
    ptr = info.ptr & ~3; /* align the pointer, just in case */
    if (lpdwCapture) *lpdwCapture = ptr;
    if (lpdwRead) {
        /* add some safety margin (not strictly necessary, but...) */
        if (WInDev[This->drv->wDevID].ossdev->in_caps_support & WAVECAPS_SAMPLEACCURATE)
            *lpdwRead = ptr + 32;
        else
            *lpdwRead = ptr + WInDev[This->drv->wDevID].dwFragmentSize;
        while (*lpdwRead > This->buflen)
            *lpdwRead -= This->buflen;
    }
    TRACE("capturepos=%ld, readpos=%ld\n", lpdwCapture?*lpdwCapture:0, lpdwRead?*lpdwRead:0);
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_GetStatus(
    PIDSCDRIVERBUFFER iface,
    LPDWORD lpdwStatus)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    TRACE("(%p,%p)\n",This,lpdwStatus);

    if (This->is_capturing) {
        if (This->is_looping)
            *lpdwStatus = DSCBSTATUS_CAPTURING | DSCBSTATUS_LOOPING;
        else
            *lpdwStatus = DSCBSTATUS_CAPTURING;
    } else
        *lpdwStatus = 0;

    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_Start(
    PIDSCDRIVERBUFFER iface,
    DWORD dwFlags)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    int enable;
    TRACE("(%p,%lx)\n",This,dwFlags);

    if (This->is_capturing)
        return DS_OK;

    if (dwFlags & DSCBSTART_LOOPING)
        This->is_looping = TRUE;

    WInDev[This->drv->wDevID].ossdev->bInputEnabled = TRUE;
    enable = getEnables(WInDev[This->drv->wDevID].ossdev);
    if (ioctl(WInDev[This->drv->wDevID].ossdev->fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
        if (errno == EINVAL) {
            /* Don't give up yet. OSS trigger support is inconsistent. */
            if (WInDev[This->drv->wDevID].ossdev->open_count == 1) {
                /* try the opposite output enable */
                if (WInDev[This->drv->wDevID].ossdev->bOutputEnabled == FALSE)
                    WInDev[This->drv->wDevID].ossdev->bOutputEnabled = TRUE;
                else
                    WInDev[This->drv->wDevID].ossdev->bOutputEnabled = FALSE;
                /* try it again */
                enable = getEnables(WInDev[This->drv->wDevID].ossdev);
                if (ioctl(WInDev[This->drv->wDevID].ossdev->fd, SNDCTL_DSP_SETTRIGGER, &enable) >= 0) {
                    This->is_capturing = TRUE;
                    return DS_OK;
                }
            }
        }
        ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER) failed (%s)\n",
            WInDev[This->drv->wDevID].ossdev->dev_name, strerror(errno));
        WInDev[This->drv->wDevID].ossdev->bInputEnabled = FALSE;
        return DSERR_GENERIC;
    }

    This->is_capturing = TRUE;
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_Stop(PIDSCDRIVERBUFFER iface)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    int enable;
    TRACE("(%p)\n",This);

    if (!This->is_capturing)
        return DS_OK;

    /* no more captureing */
    WInDev[This->drv->wDevID].ossdev->bInputEnabled = FALSE;
    enable = getEnables(WInDev[This->drv->wDevID].ossdev);
    if (ioctl(WInDev[This->drv->wDevID].ossdev->fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
        ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER) failed (%s)\n",
            WInDev[This->drv->wDevID].ossdev->dev_name,  strerror(errno));
        return DSERR_GENERIC;
    }

    /* send a final event if necessary */
    if (This->nrofnotifies > 0) {
        if (This->notifies[This->nrofnotifies - 1].dwOffset == DSBPN_OFFSETSTOP)
            SetEvent(This->notifies[This->nrofnotifies - 1].hEventNotify);
    }

    This->is_capturing = FALSE;
    This->is_looping = FALSE;

    /* Most OSS drivers just can't stop capturing without closing the device...
     * so we need to somehow signal to our DirectSound implementation
     * that it should completely recreate this HW buffer...
     * this unexpected error code should do the trick... */
    return DSERR_BUFFERLOST;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_SetFormat(
    PIDSCDRIVERBUFFER iface,
    LPWAVEFORMATEX pwfx)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    FIXME("(%p): stub!\n",This);
    return DSERR_UNSUPPORTED;
}

static IDsCaptureDriverBufferVtbl dscdbvt =
{
    IDsCaptureDriverBufferImpl_QueryInterface,
    IDsCaptureDriverBufferImpl_AddRef,
    IDsCaptureDriverBufferImpl_Release,
    IDsCaptureDriverBufferImpl_Lock,
    IDsCaptureDriverBufferImpl_Unlock,
    IDsCaptureDriverBufferImpl_SetFormat,
    IDsCaptureDriverBufferImpl_GetPosition,
    IDsCaptureDriverBufferImpl_GetStatus,
    IDsCaptureDriverBufferImpl_Start,
    IDsCaptureDriverBufferImpl_Stop
};

static HRESULT WINAPI IDsCaptureDriverImpl_QueryInterface(
    PIDSCDRIVER iface,
    REFIID riid,
    LPVOID *ppobj)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    if ( IsEqualGUID(riid, &IID_IUnknown) ||
         IsEqualGUID(riid, &IID_IDsCaptureDriver) ) {
        IDsCaptureDriver_AddRef(iface);
        *ppobj = (LPVOID)This;
        return DS_OK;
    }

    FIXME( "Unknown IID %s\n", debugstr_guid( riid ) );

    *ppobj = 0;

    return E_NOINTERFACE;
}

static ULONG WINAPI IDsCaptureDriverImpl_AddRef(PIDSCDRIVER iface)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    TRACE("(%p) ref was %ld\n", This, This->ref);

    return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI IDsCaptureDriverImpl_Release(PIDSCDRIVER iface)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    DWORD ref;
    TRACE("(%p) ref was %ld\n", This, This->ref);

    ref = InterlockedDecrement(&(This->ref));
    if (ref == 0) {
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return ref;
}

static HRESULT WINAPI IDsCaptureDriverImpl_GetDriverDesc(
    PIDSCDRIVER iface,
    PDSDRIVERDESC pDesc)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    TRACE("(%p,%p)\n",This,pDesc);

    if (!pDesc) {
        TRACE("invalid parameter\n");
        return DSERR_INVALIDPARAM;
    }

    /* copy version from driver */
    memcpy(pDesc, &(WInDev[This->wDevID].ossdev->ds_desc), sizeof(DSDRIVERDESC));

    pDesc->dwFlags |= DSDDESC_USESYSTEMMEMORY;
    pDesc->dnDevNode            = WInDev[This->wDevID].waveDesc.dnDevNode;
    pDesc->wVxdId               = 0;
    pDesc->wReserved            = 0;
    pDesc->ulDeviceNum          = This->wDevID;
    pDesc->dwHeapType           = DSDHEAP_NOHEAP;
    pDesc->pvDirectDrawHeap     = NULL;
    pDesc->dwMemStartAddress    = 0;
    pDesc->dwMemEndAddress      = 0;
    pDesc->dwMemAllocExtra      = 0;
    pDesc->pvReserved1          = NULL;
    pDesc->pvReserved2          = NULL;
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverImpl_Open(PIDSCDRIVER iface)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    TRACE("(%p)\n",This);
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverImpl_Close(PIDSCDRIVER iface)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    TRACE("(%p)\n",This);
    if (This->capture_buffer) {
        ERR("problem with DirectSound: capture buffer not released\n");
        return DSERR_GENERIC;
    }
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverImpl_GetCaps(
    PIDSCDRIVER iface,
    PDSCDRIVERCAPS pCaps)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    TRACE("(%p,%p)\n",This,pCaps);
    memcpy(pCaps, &(WInDev[This->wDevID].ossdev->dsc_caps), sizeof(DSCDRIVERCAPS));
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverImpl_CreateCaptureBuffer(
    PIDSCDRIVER iface,
    LPWAVEFORMATEX pwfx,
    DWORD dwFlags,
    DWORD dwCardAddress,
    LPDWORD pdwcbBufferSize,
    LPBYTE *ppbBuffer,
    LPVOID *ppvObj)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    IDsCaptureDriverBufferImpl** ippdscdb = (IDsCaptureDriverBufferImpl**)ppvObj;
    HRESULT err;
    audio_buf_info info;
    int enable;
    TRACE("(%p,%p,%lx,%lx,%p,%p,%p)\n",This,pwfx,dwFlags,dwCardAddress,
          pdwcbBufferSize,ppbBuffer,ppvObj);

    if (This->capture_buffer) {
        TRACE("already allocated\n");
        return DSERR_ALLOCATED;
    }

    *ippdscdb = (IDsCaptureDriverBufferImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDsCaptureDriverBufferImpl));
    if (*ippdscdb == NULL) {
        TRACE("out of memory\n");
        return DSERR_OUTOFMEMORY;
    }

    (*ippdscdb)->lpVtbl = &dscdbvt;
    (*ippdscdb)->ref          = 1;
    (*ippdscdb)->drv          = This;
    (*ippdscdb)->notify       = NULL;
    (*ippdscdb)->notify_index = 0;
    (*ippdscdb)->notifies     = NULL;
    (*ippdscdb)->nrofnotifies = 0;
    (*ippdscdb)->property_set = NULL;
    (*ippdscdb)->is_capturing = FALSE;
    (*ippdscdb)->is_looping   = FALSE;

    if (WInDev[This->wDevID].state == WINE_WS_CLOSED) {
        WAVEOPENDESC desc;
        desc.hWave = 0;
        desc.lpFormat = pwfx;
        desc.dwCallback = 0;
        desc.dwInstance = 0;
        desc.uMappedDeviceID = 0;
        desc.dnDevNode = 0;
        err = widOpen(This->wDevID, &desc, dwFlags | WAVE_DIRECTSOUND);
        if (err != MMSYSERR_NOERROR) {
            TRACE("widOpen failed\n");
            return err;
        }
    }

    /* check how big the DMA buffer is now */
    if (ioctl(WInDev[This->wDevID].ossdev->fd, SNDCTL_DSP_GETISPACE, &info) < 0) {
        ERR("ioctl(%s, SNDCTL_DSP_GETISPACE) failed (%s)\n",
            WInDev[This->wDevID].ossdev->dev_name, strerror(errno));
        HeapFree(GetProcessHeap(),0,*ippdscdb);
        *ippdscdb = NULL;
        return DSERR_GENERIC;
    }
    (*ippdscdb)->maplen = (*ippdscdb)->buflen = info.fragstotal * info.fragsize;

    /* map the DMA buffer */
    err = DSCDB_MapBuffer(*ippdscdb);
    if (err != DS_OK) {
        HeapFree(GetProcessHeap(),0,*ippdscdb);
        *ippdscdb = NULL;
        return err;
    }

    /* capture buffer is ready to go */
    *pdwcbBufferSize    = (*ippdscdb)->maplen;
    *ppbBuffer          = (*ippdscdb)->mapping;

    /* some drivers need some extra nudging after mapping */
    WInDev[This->wDevID].ossdev->bInputEnabled = FALSE;
    enable = getEnables(WInDev[This->wDevID].ossdev);
    if (ioctl(WInDev[This->wDevID].ossdev->fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
        ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER) failed (%s)\n",
            WInDev[This->wDevID].ossdev->dev_name, strerror(errno));
        return DSERR_GENERIC;
    }

    This->capture_buffer = *ippdscdb;

    return DS_OK;
}

static IDsCaptureDriverVtbl dscdvt =
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

static HRESULT WINAPI IDsCaptureDriverPropertySetImpl_Create(
    IDsCaptureDriverBufferImpl * dscdb,
    IDsCaptureDriverPropertySetImpl **pdscdps)
{
    IDsCaptureDriverPropertySetImpl * dscdps;
    TRACE("(%p,%p)\n",dscdb,pdscdps);

    dscdps = (IDsCaptureDriverPropertySetImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(dscdps));
    if (dscdps == NULL) {
        WARN("out of memory\n");
        return DSERR_OUTOFMEMORY;
    }

    dscdps->ref = 0;
    dscdps->lpVtbl = &dscdpsvt;
    dscdps->capture_buffer = dscdb;
    dscdb->property_set = dscdps;
    IDsCaptureDriverBuffer_AddRef((PIDSCDRIVER)dscdb);

    *pdscdps = dscdps;
    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverNotifyImpl_Create(
    IDsCaptureDriverBufferImpl * dscdb,
    IDsCaptureDriverNotifyImpl **pdscdn)
{
    IDsCaptureDriverNotifyImpl * dscdn;
    TRACE("(%p,%p)\n",dscdb,pdscdn);

    dscdn = (IDsCaptureDriverNotifyImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(dscdn));
    if (dscdn == NULL) {
        WARN("out of memory\n");
        return DSERR_OUTOFMEMORY;
    }

    dscdn->ref = 0;
    dscdn->lpVtbl = &dscdnvt;
    dscdn->capture_buffer = dscdb;
    dscdb->notify = dscdn;
    IDsCaptureDriverBuffer_AddRef((PIDSCDRIVER)dscdb);

    *pdscdn = dscdn;
    return DS_OK;
}

DWORD widDsCreate(UINT wDevID, PIDSCDRIVER* drv)
{
    IDsCaptureDriverImpl** idrv = (IDsCaptureDriverImpl**)drv;
    TRACE("(%d,%p)\n",wDevID,drv);

    /* the HAL isn't much better than the HEL if we can't do mmap() */
    if (!(WInDev[wDevID].ossdev->in_caps_support & WAVECAPS_DIRECTSOUND)) {
        ERR("DirectSoundCapture flag not set\n");
        MESSAGE("This sound card's driver does not support direct access\n");
        MESSAGE("The (slower) DirectSound HEL mode will be used instead.\n");
        return MMSYSERR_NOTSUPPORTED;
    }

    *idrv = (IDsCaptureDriverImpl*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDsCaptureDriverImpl));
    if (!*idrv)
        return MMSYSERR_NOMEM;
    (*idrv)->lpVtbl	= &dscdvt;
    (*idrv)->ref	= 1;

    (*idrv)->wDevID	= wDevID;
    (*idrv)->capture_buffer = NULL;
    return MMSYSERR_NOERROR;
}

DWORD widDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    memcpy(desc, &(WInDev[wDevID].ossdev->ds_desc), sizeof(DSDRIVERDESC));
    return MMSYSERR_NOERROR;
}

#endif /* HAVE_OSS */
