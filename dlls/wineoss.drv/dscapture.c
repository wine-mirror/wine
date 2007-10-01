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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmddk.h"
#include "mmreg.h"
#include "dsound.h"
#include "dsdriver.h"
#include "oss.h"
#include "wine/debug.h"

#include "audio.h"

WINE_DEFAULT_DEBUG_CHANNEL(dscapture);

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
    const IDsDriverPropertySetVtbl     *lpVtbl;
    LONG                                ref;

    IDsCaptureDriverBufferImpl*         capture_buffer;
};

struct IDsCaptureDriverNotifyImpl
{
    /* IUnknown fields */
    const IDsDriverNotifyVtbl          *lpVtbl;
    LONG                                ref;

    IDsCaptureDriverBufferImpl*         capture_buffer;
};

struct IDsCaptureDriverImpl
{
    /* IUnknown fields */
    const IDsCaptureDriverVtbl         *lpVtbl;
    LONG                                ref;

    /* IDsCaptureDriverImpl fields */
    UINT                                wDevID;
    IDsCaptureDriverBufferImpl*         capture_buffer;
};

struct IDsCaptureDriverBufferImpl
{
    /* IUnknown fields */
    const IDsCaptureDriverBufferVtbl   *lpVtbl;
    LONG                                ref;

    /* IDsCaptureDriverBufferImpl fields */
    IDsCaptureDriverImpl*               drv;
    LPBYTE                              buffer;         /* user buffer */
    DWORD                               buflen;         /* user buffer length */
    LPBYTE                              mapping;        /* DMA buffer */
    DWORD                               maplen;         /* DMA buffer length */
    BOOL                                is_direct_map;  /* DMA == user ? */
    DWORD                               fragsize;
    DWORD                               map_writepos;   /* DMA write offset */
    DWORD                               map_readpos;    /* DMA read offset */
    DWORD                               writeptr;       /* user write offset */
    DWORD                               readptr;        /* user read offset */

    /* IDsDriverNotifyImpl fields */
    IDsCaptureDriverNotifyImpl*         notify;
    int                                 notify_index;
    LPDSBPOSITIONNOTIFY                 notifies;
    int                                 nrofnotifies;

    /* IDsDriverPropertySetImpl fields */
    IDsCaptureDriverPropertySetImpl*    property_set;

    BOOL                                is_capturing;
    BOOL                                is_looping;
    WAVEFORMATEX                        wfx;
    HANDLE                              hThread;
    DWORD                               dwThreadID;
    HANDLE                              hStartUpEvent;
    HANDLE                              hExitEvent;
    int                                 pipe_fd[2];
    int                                 fd;
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
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsCaptureDriverPropertySetImpl_Release(
    PIDSDRIVERPROPERTYSET iface)
{
    IDsCaptureDriverPropertySetImpl *This = (IDsCaptureDriverPropertySetImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount + 1);

    if (!refCount) {
        IDsCaptureDriverBuffer_Release((PIDSCDRIVERBUFFER)This->capture_buffer);
        This->capture_buffer->property_set = NULL;
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return refCount;
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
    FIXME("(%p,%p,%p,%x,%p,%x,%p)\n",This,pDsProperty,pPropertyParams,
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
    FIXME("(%p,%p,%p,%x,%p,%x)\n",This,pDsProperty,pPropertyParams,
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
    FIXME("(%p,%s,%x,%p)\n",This,debugstr_guid(PropertySetId),PropertyId,
          pSupport);
    return DSERR_UNSUPPORTED;
}

static const IDsDriverPropertySetVtbl dscdpsvt =
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
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsCaptureDriverNotifyImpl_Release(
    PIDSDRIVERNOTIFY iface)
{
    IDsCaptureDriverNotifyImpl *This = (IDsCaptureDriverNotifyImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount + 1);

    if (!refCount) {
        IDsCaptureDriverBuffer_Release((PIDSCDRIVERBUFFER)This->capture_buffer);
        This->capture_buffer->notify = NULL;
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return refCount;
}

static HRESULT WINAPI IDsCaptureDriverNotifyImpl_SetNotificationPositions(
    PIDSDRIVERNOTIFY iface,
    DWORD howmuch,
    LPCDSBPOSITIONNOTIFY notify)
{
    IDsCaptureDriverNotifyImpl *This = (IDsCaptureDriverNotifyImpl *)iface;
    TRACE("(%p,0x%08x,%p)\n",This,howmuch,notify);

    if (!notify) {
        WARN("invalid parameter\n");
        return DSERR_INVALIDPARAM;
    }

    if (TRACE_ON(dscapture)) {
        int i;
        for (i=0;i<howmuch;i++)
            TRACE("notify at %d to 0x%08x\n",
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

static const IDsDriverNotifyVtbl dscdnvt =
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
                              WInDev[dscdb->drv->wDevID].ossdev.fd, 0);
        if (dscdb->mapping == (LPBYTE)-1) {
            TRACE("(%p): Could not map sound device for direct access (%s)\n",
                  dscdb, strerror(errno));
            return DSERR_GENERIC;
        }
        TRACE("(%p): sound device has been mapped for direct access at %p, "
              "size=%d\n", dscdb, dscdb->mapping, dscdb->maplen);
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
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsCaptureDriverBufferImpl_Release(PIDSCDRIVERBUFFER iface)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);
    TRACE("(%p) ref was %d\n", This, refCount + 1);

    if (!refCount) {
        WINE_WAVEIN*        wwi;

        wwi = &WInDev[This->drv->wDevID];

        if (This->hThread) {
            int x = 0;

            /* request thread termination */
            write(This->pipe_fd[1], &x, sizeof(x));

            /* wait for reply */
            WaitForSingleObject(This->hExitEvent, INFINITE);
            CloseHandle(This->hExitEvent);
        }

        close(This->pipe_fd[0]);
        close(This->pipe_fd[1]);

        DSCDB_UnmapBuffer(This);

        OSS_CloseDevice(&wwi->ossdev);
        wwi->state = WINE_WS_CLOSED;
        wwi->dwFragmentSize = 0;
        This->drv->capture_buffer = NULL;

        HeapFree(GetProcessHeap(), 0, This->notifies);
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return refCount;
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
    TRACE("(%p,%p,%p,%p,%p,%d,%d,0x%08x)\n",This,ppvAudio1,pdwLen1,
          ppvAudio2,pdwLen2,dwWritePosition,dwWriteLen,dwFlags);

    if (This->is_direct_map) {
        if (ppvAudio1)
            *ppvAudio1 = This->mapping + dwWritePosition;

        if (dwWritePosition + dwWriteLen < This->maplen) {
            if (pdwLen1)
                *pdwLen1 = dwWriteLen;
            if (ppvAudio2)
                *ppvAudio2 = 0;
            if (pdwLen2)
                *pdwLen2 = 0;
        } else {
            if (pdwLen1)
                *pdwLen1 = This->maplen - dwWritePosition;
            if (ppvAudio2)
                *ppvAudio2 = 0;
            if (pdwLen2)
                *pdwLen2 = dwWriteLen - (This->maplen - dwWritePosition);
        }
    } else {
        if (ppvAudio1)
            *ppvAudio1 = This->buffer + dwWritePosition;

        if (dwWritePosition + dwWriteLen < This->buflen) {
            if (pdwLen1)
                *pdwLen1 = dwWriteLen;
            if (ppvAudio2)
                *ppvAudio2 = 0;
            if (pdwLen2)
                *pdwLen2 = 0;
        } else {
            if (pdwLen1)
                *pdwLen1 = This->buflen - dwWritePosition;
            if (ppvAudio2)
                *ppvAudio2 = 0;
            if (pdwLen2)
                *pdwLen2 = dwWriteLen - (This->buflen - dwWritePosition);
        }
    }

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
    TRACE("(%p,%p,%d,%p,%d)\n",This,pvAudio1,dwLen1,pvAudio2,dwLen2);

    if (This->is_direct_map)
        This->map_readpos = (This->map_readpos + dwLen1 + dwLen2) % This->maplen;
    else
        This->readptr = (This->readptr + dwLen1 + dwLen2) % This->buflen;

    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_GetPosition(
    PIDSCDRIVERBUFFER iface,
    LPDWORD lpdwCapture,
    LPDWORD lpdwRead)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
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

    if (This->is_direct_map) {
        if (lpdwCapture)
            *lpdwCapture = This->map_writepos;
        if (lpdwRead) {
            *lpdwRead = This->map_readpos;
        }
    } else {
        if (lpdwCapture)
            *lpdwCapture = This->writeptr;
        if (lpdwRead)
            *lpdwRead = This->readptr;
    }

    TRACE("capturepos=%d, readpos=%d\n", lpdwCapture?*lpdwCapture:0,
          lpdwRead?*lpdwRead:0);
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
    TRACE("(%p,%x)\n",This,dwFlags);

    if (This->is_capturing)
        return DS_OK;

    if (dwFlags & DSCBSTART_LOOPING)
        This->is_looping = TRUE;

    WInDev[This->drv->wDevID].ossdev.bInputEnabled = TRUE;
    enable = getEnables(&WInDev[This->drv->wDevID].ossdev);
    if (ioctl(WInDev[This->drv->wDevID].ossdev.fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
        if (errno == EINVAL) {
            /* Don't give up yet. OSS trigger support is inconsistent. */
            if (WInDev[This->drv->wDevID].ossdev.open_count == 1) {
                /* try the opposite output enable */
                if (WInDev[This->drv->wDevID].ossdev.bOutputEnabled == FALSE)
                    WInDev[This->drv->wDevID].ossdev.bOutputEnabled = TRUE;
                else
                    WInDev[This->drv->wDevID].ossdev.bOutputEnabled = FALSE;
                /* try it again */
                enable = getEnables(&WInDev[This->drv->wDevID].ossdev);
                if (ioctl(WInDev[This->drv->wDevID].ossdev.fd, SNDCTL_DSP_SETTRIGGER, &enable) >= 0) {
                    This->is_capturing = TRUE;
                    return DS_OK;
                }
            }
        }
        ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER) failed (%s)\n",
            WInDev[This->drv->wDevID].ossdev.dev_name, strerror(errno));
        WInDev[This->drv->wDevID].ossdev.bInputEnabled = FALSE;
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

    /* no more capturing */
    WInDev[This->drv->wDevID].ossdev.bInputEnabled = FALSE;
    enable = getEnables(&WInDev[This->drv->wDevID].ossdev);
    if (ioctl(WInDev[This->drv->wDevID].ossdev.fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
        ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER) failed (%s)\n",
            WInDev[This->drv->wDevID].ossdev.dev_name,  strerror(errno));
        return DSERR_GENERIC;
    }

    /* send a final event if necessary */
    if (This->nrofnotifies > 0) {
        if (This->notifies[This->nrofnotifies - 1].dwOffset == DSBPN_OFFSETSTOP)
            SetEvent(This->notifies[This->nrofnotifies - 1].hEventNotify);
    }

    This->is_capturing = FALSE;
    This->is_looping = FALSE;

    if (This->hThread) {
        int x = 0;
        write(This->pipe_fd[1], &x, sizeof(x));
        WaitForSingleObject(This->hExitEvent, INFINITE);
        CloseHandle(This->hExitEvent);
        This->hExitEvent = INVALID_HANDLE_VALUE;
        This->hThread = 0;
    }

    return DS_OK;
}

static HRESULT WINAPI IDsCaptureDriverBufferImpl_SetFormat(
    PIDSCDRIVERBUFFER iface,
    LPWAVEFORMATEX pwfx)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)iface;
    FIXME("(%p): stub!\n",This);
    return DSERR_UNSUPPORTED;
}

static const IDsCaptureDriverBufferVtbl dscdbvt =
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
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsCaptureDriverImpl_Release(PIDSCDRIVER iface)
{
    IDsCaptureDriverImpl *This = (IDsCaptureDriverImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount + 1);

    if (!refCount) {
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return refCount;
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
    memcpy(pDesc, &(WInDev[This->wDevID].ossdev.ds_desc), sizeof(DSDRIVERDESC));

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
    memcpy(pCaps, &(WInDev[This->wDevID].ossdev.dsc_caps), sizeof(DSCDRIVERCAPS));
    return DS_OK;
}

static void DSCDB_CheckEvent(
    IDsCaptureDriverBufferImpl *dscb,
    DWORD writepos,
    DWORD len,
    DWORD buflen)
{
    LPDSBPOSITIONNOTIFY event = dscb->notifies + dscb->notify_index;
    DWORD offset = event->dwOffset;
    TRACE("(%p,%d,%d,%d)\n", dscb, writepos, len, buflen);

    TRACE("(%p) buflen = %d, writeptr = %d\n",
        dscb, dscb->buflen, dscb->writeptr);
    TRACE("checking %d, position %d, event = %p\n",
        dscb->notify_index, offset, event->hEventNotify);

    if ((writepos + len) > offset) {
         TRACE("signalled event %p (%d) %d\n",
               event->hEventNotify, dscb->notify_index, offset);
         SetEvent(event->hEventNotify);
         dscb->notify_index = (dscb->notify_index + 1) % dscb->nrofnotifies;
         return;
    } else if ((writepos + len) > buflen) {
        writepos = writepos + len - buflen;
        if ((writepos + len) > offset) {
             TRACE("signalled event %p (%d) %d\n",
                   event->hEventNotify, dscb->notify_index, offset);
             SetEvent(event->hEventNotify);
             dscb->notify_index = (dscb->notify_index + 1) % dscb->nrofnotifies;
             return;
        }
    }

    return;
}

/* FIXME: using memcpy can cause strange crashes so use this fake one */
static void * my_memcpy(void * dst, const void * src, int length)
{
    int i;
    for (i = 0; i < length; i++)
        ((char *)dst)[i] = ((const char *)src)[i];
    return dst;
}

static DWORD CALLBACK DSCDB_Thread(LPVOID lpParameter)
{
    IDsCaptureDriverBufferImpl *This = (IDsCaptureDriverBufferImpl *)lpParameter;
    struct pollfd poll_list[2];
    int retval;
    DWORD offset = 0;
    DWORD map_offset = 0;
    TRACE("(%p)\n", lpParameter);

    poll_list[0].fd = This->fd;                /* data available */
    poll_list[1].fd = This->pipe_fd[0];        /* message from parent process */
    poll_list[0].events = POLLIN;
    poll_list[1].events = POLLIN;

    /* let other process know we are running */
    SetEvent(This->hStartUpEvent);

    while (1) {
        /* wait for something to happen */
        retval = poll(poll_list,(unsigned long)2,-1);
        /* Retval will always be greater than 0 or -1 in this case.
         * Since we're doing it while blocking
         */
        if (retval < 0) {
            ERR("Error while polling: %s\n",strerror(errno));
            continue;
        }

        /* check for exit command */
        if ((poll_list[1].revents & POLLIN) == POLLIN) {
            TRACE("(%p) done\n", lpParameter);
            /* acknowledge command and exit */
            SetEvent(This->hExitEvent);
            ExitThread(0);
            return 0;
        }

        /* check for data */
        if ((poll_list[0].revents & POLLIN) == POLLIN) {
            count_info info;
            int fragsize, first, second;

            /* get the current DMA position */
            if (ioctl(This->fd, SNDCTL_DSP_GETIPTR, &info) < 0) {
                ERR("ioctl(%s, SNDCTL_DSP_GETIPTR) failed (%s)\n",
                    WInDev[This->drv->wDevID].ossdev.dev_name, strerror(errno));
                return DSERR_GENERIC;
            }

            if (This->is_direct_map) {
                offset = This->map_writepos;
                This->map_writepos = info.ptr;

                if (info.ptr < offset)
                    fragsize = info.ptr + This->maplen - offset;
                else
                    fragsize = info.ptr - offset;

                DSCDB_CheckEvent(This, offset, fragsize, This->maplen);
            } else {
                map_offset = This->map_writepos;
                offset = This->writeptr;

                /* test for mmap buffer wrap */
                if (info.ptr < map_offset) {
                    /* mmap buffer wrapped */
                    fragsize = info.ptr + This->maplen - map_offset;

                    /* check for user buffer wrap */
                    if ((offset + fragsize) > This->buflen) {
                        /* both buffers wrapped
                         * figure out which wrapped first
                         */
                        if ((This->maplen - map_offset) > (This->buflen - offset)) {
                            /* user buffer wrapped first */
                            first = This->buflen - offset;
                            second = (This->maplen - map_offset) - first;
                            my_memcpy(This->buffer + offset, This->mapping + map_offset, first);
                            my_memcpy(This->buffer, This->mapping + map_offset + first, second);
                            my_memcpy(This->buffer + second, This->mapping, fragsize - (first + second));
                        } else {
                            /* mmap buffer wrapped first */
                            first = This->maplen - map_offset;
                            second = (This->buflen - offset) - first;
                            my_memcpy(This->buffer + offset, This->mapping + map_offset, first);
                            my_memcpy(This->buffer + offset + first, This->mapping, second);
                            my_memcpy(This->buffer, This->mapping + second, fragsize - (first + second));
                        }
                    } else {
                        /* only mmap buffer wrapped */
                        first = This->maplen - map_offset;
                        my_memcpy(This->buffer + offset, This->mapping + map_offset, first);
                        my_memcpy(This->buffer + offset + first, This->mapping, fragsize - first);
                    }
                } else {
                    /* mmap buffer didn't wrap */
                    fragsize = info.ptr - map_offset;

                    /* check for user buffer wrap */
                    if ((offset + fragsize) > This->buflen) {
                        first = This->buflen - offset;
                        my_memcpy(This->buffer + offset, This->mapping + map_offset, first);
                        my_memcpy(This->buffer, This->mapping + map_offset + first, fragsize - first);
                    } else
                        my_memcpy(This->buffer + offset, This->mapping + map_offset, fragsize);
                }

                This->map_writepos = info.ptr;
                This->writeptr = (This->writeptr + fragsize) % This->buflen;
                DSCDB_CheckEvent(This, offset, fragsize, This->buflen);
            }
        }
    }
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
    int audio_fragment, fsize, shift, ret;
    BOOL bNewBuffer = FALSE;
    WINE_WAVEIN* wwi;
    TRACE("(%p,%p,%x,%x,%p,%p,%p)\n",This,pwfx,dwFlags,dwCardAddress,
          pdwcbBufferSize,ppbBuffer,ppvObj);

    if (This->capture_buffer) {
        TRACE("already allocated\n");
        return DSERR_ALLOCATED;
    }

    /* must be given a buffer size */
    if (pdwcbBufferSize == NULL || *pdwcbBufferSize == 0) {
       TRACE("invalid parameter: pdwcbBufferSize\n");
       return DSERR_INVALIDPARAM;
    }

    /* must be given a buffer pointer */
    if (ppbBuffer == NULL) {
       TRACE("invalid parameter: ppbBuffer\n");
       return DSERR_INVALIDPARAM;
    }

    /* may or may not be given a buffer */
    if (*ppbBuffer == NULL) {
        TRACE("creating buffer\n");
        bNewBuffer = TRUE;     /* not given a buffer so create one */
    } else
        TRACE("using supplied buffer\n");

    *ippdscdb = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDsCaptureDriverBufferImpl));
    if (*ippdscdb == NULL) {
        TRACE("out of memory\n");
        return DSERR_OUTOFMEMORY;
    }

    (*ippdscdb)->lpVtbl       = &dscdbvt;
    (*ippdscdb)->ref          = 1;
    (*ippdscdb)->drv          = This;
    (*ippdscdb)->notify       = NULL;
    (*ippdscdb)->notify_index = 0;
    (*ippdscdb)->notifies     = NULL;
    (*ippdscdb)->nrofnotifies = 0;
    (*ippdscdb)->property_set = NULL;
    (*ippdscdb)->is_capturing = FALSE;
    (*ippdscdb)->is_looping   = FALSE;
    (*ippdscdb)->wfx          = *pwfx;
    (*ippdscdb)->buflen       = *pdwcbBufferSize;

    if (bNewBuffer)
        (*ippdscdb)->buffer = NULL;
    else
        (*ippdscdb)->buffer = *ppbBuffer;

    wwi = &WInDev[This->wDevID];

    if (wwi->state == WINE_WS_CLOSED) {
        unsigned int frag_size;

        if (wwi->ossdev.open_count > 0) {
            /* opened already so use existing fragment size */
            audio_fragment = wwi->ossdev.audio_fragment;
        } else {
            /* calculate a fragment size */
            unsigned int mask = 0xffffffff;

            /* calculate largest fragment size less than 10 ms. */
            fsize = pwfx->nAvgBytesPerSec / 100;        /* 10 ms chunk */
            shift = 0;
            while ((1 << shift) <= fsize)
                shift++;
            shift--;
            fsize = 1 << shift;
            TRACE("shift = %d, fragment size = %d\n", shift, fsize);
            TRACE("BufferSize=%d(%08x)\n", *pdwcbBufferSize, *pdwcbBufferSize);

            /* See if we can directly map the buffer first.
             * (buffer length is multiple of a power of 2)
             */
            mask = (mask >> (32 - shift));
            TRACE("mask=%08x\n", mask);
            if (*pdwcbBufferSize & mask) {
                /* no so try a smaller fragment size greater than 1 ms */
                int new_shift = shift - 1;
                int min_shift = 0;
                int min_fsize = pwfx->nAvgBytesPerSec / 1000;
                BOOL found_one = FALSE;
                while ((1 << min_shift) <= min_fsize)
                    min_shift++;
                min_shift--;
                while (new_shift > min_shift) {
                    if (*pdwcbBufferSize & (-1 >> (32 - new_shift))) {
                        new_shift--;
                        continue;
                    } else {
                        found_one = TRUE;
                        break;
                    }
                }
                if (found_one) {
                    /* found a smaller one that will work */
                    audio_fragment = ((*pdwcbBufferSize >> new_shift) << 16) | new_shift;
                    (*ippdscdb)->is_direct_map = TRUE;
                    TRACE("new shift = %d, fragment size = %d\n",
                          new_shift, 1 << (audio_fragment & 0xffff));
                } else {
                    /* buffer can't be direct mapped */
                    audio_fragment = 0x00100000 + shift;        /* 16 fragments of 2^shift */
                    (*ippdscdb)->is_direct_map = FALSE;
                }
            } else {
                /* good fragment size */
                audio_fragment = ((*pdwcbBufferSize >> shift) << 16) | shift;
                (*ippdscdb)->is_direct_map = TRUE;
            }
        }
        frag_size = 1 << (audio_fragment & 0xffff);
        TRACE("is_direct_map = %s\n", (*ippdscdb)->is_direct_map ? "TRUE" : "FALSE");
        TRACE("requesting %d %d byte fragments (%d bytes) (%d ms/fragment)\n",
              audio_fragment >> 16, frag_size, frag_size * (audio_fragment >> 16),
              (frag_size * 1000) / pwfx->nAvgBytesPerSec);

        ret = OSS_OpenDevice(&wwi->ossdev, O_RDWR, &audio_fragment, 1,
                             pwfx->nSamplesPerSec,
                             (pwfx->nChannels > 1) ? 1 : 0,
                             (pwfx->wBitsPerSample == 16)
                             ? AFMT_S16_LE : AFMT_U8);

        if (ret != 0) {
            WARN("OSS_OpenDevice failed\n");
            HeapFree(GetProcessHeap(),0,*ippdscdb);
            *ippdscdb = NULL;
            return DSERR_GENERIC;
        }

        wwi->state = WINE_WS_STOPPED;

        /* find out what fragment and buffer sizes OSS gave us */
        if (ioctl(wwi->ossdev.fd, SNDCTL_DSP_GETISPACE, &info) < 0) {
             ERR("ioctl(%s, SNDCTL_DSP_GETISPACE) failed (%s)\n",
                 wwi->ossdev.dev_name, strerror(errno));
             OSS_CloseDevice(&wwi->ossdev);
             wwi->state = WINE_WS_CLOSED;
             HeapFree(GetProcessHeap(),0,*ippdscdb);
             *ippdscdb = NULL;
             return DSERR_GENERIC;
        }

        TRACE("got %d %d byte fragments (%d bytes) (%d ms/fragment)\n",
              info.fragstotal, info.fragsize, info.fragstotal * info.fragsize,
              info.fragsize * 1000 / pwfx->nAvgBytesPerSec);

        wwi->dwTotalRecorded = 0;
        memcpy(&wwi->waveFormat, pwfx, sizeof(PCMWAVEFORMAT));
        wwi->dwFragmentSize = info.fragsize;

        /* make sure we got what we asked for */
        if ((*ippdscdb)->buflen != info.fragstotal * info.fragsize) {
            TRACE("Couldn't create requested buffer\n");
            if ((*ippdscdb)->is_direct_map) {
                (*ippdscdb)->is_direct_map = FALSE;
                TRACE("is_direct_map = FALSE\n");
            }
        } else if (info.fragsize != frag_size) {
            TRACE("same buffer length but different fragment size\n");
        }
    }
    (*ippdscdb)->fd = WInDev[This->wDevID].ossdev.fd;

    if (pipe((*ippdscdb)->pipe_fd) < 0) {
        TRACE("pipe() failed (%s)\n", strerror(errno));
        OSS_CloseDevice(&wwi->ossdev);
        wwi->state = WINE_WS_CLOSED;
        HeapFree(GetProcessHeap(),0,*ippdscdb);
        *ippdscdb = NULL;
        return DSERR_GENERIC;
    }

    /* check how big the DMA buffer is now */
    if (ioctl(wwi->ossdev.fd, SNDCTL_DSP_GETISPACE, &info) < 0) {
        ERR("ioctl(%s, SNDCTL_DSP_GETISPACE) failed (%s)\n",
            wwi->ossdev.dev_name, strerror(errno));
        OSS_CloseDevice(&wwi->ossdev);
        wwi->state = WINE_WS_CLOSED;
        close((*ippdscdb)->pipe_fd[0]);
        close((*ippdscdb)->pipe_fd[1]);
        HeapFree(GetProcessHeap(),0,*ippdscdb);
        *ippdscdb = NULL;
        return DSERR_GENERIC;
    }

    (*ippdscdb)->maplen = info.fragstotal * info.fragsize;
    (*ippdscdb)->fragsize = info.fragsize;
    (*ippdscdb)->map_writepos = 0;
    (*ippdscdb)->map_readpos = 0;

    /* map the DMA buffer */
    err = DSCDB_MapBuffer(*ippdscdb);
    if (err != DS_OK) {
        OSS_CloseDevice(&wwi->ossdev);
        wwi->state = WINE_WS_CLOSED;
        close((*ippdscdb)->pipe_fd[0]);
        close((*ippdscdb)->pipe_fd[1]);
        HeapFree(GetProcessHeap(),0,*ippdscdb);
        *ippdscdb = NULL;
        return err;
    }

    /* create the buffer if necessary */
    if (!(*ippdscdb)->buffer)
        (*ippdscdb)->buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,(*ippdscdb)->buflen);

    if ((*ippdscdb)->buffer == NULL) {
        OSS_CloseDevice(&wwi->ossdev);
        wwi->state = WINE_WS_CLOSED;
        close((*ippdscdb)->pipe_fd[0]);
        close((*ippdscdb)->pipe_fd[1]);
        HeapFree(GetProcessHeap(),0,*ippdscdb);
        *ippdscdb = NULL;
        return DSERR_OUTOFMEMORY;
    }

    This->capture_buffer = *ippdscdb;

    (*ippdscdb)->hStartUpEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    (*ippdscdb)->hExitEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    (*ippdscdb)->hThread = CreateThread(NULL, 0,  DSCDB_Thread, (LPVOID)(*ippdscdb), 0, &((*ippdscdb)->dwThreadID));
    WaitForSingleObject((*ippdscdb)->hStartUpEvent, INFINITE);
    CloseHandle((*ippdscdb)->hStartUpEvent);
    (*ippdscdb)->hStartUpEvent = INVALID_HANDLE_VALUE;

    return DS_OK;
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

static HRESULT WINAPI IDsCaptureDriverPropertySetImpl_Create(
    IDsCaptureDriverBufferImpl * dscdb,
    IDsCaptureDriverPropertySetImpl **pdscdps)
{
    IDsCaptureDriverPropertySetImpl * dscdps;
    TRACE("(%p,%p)\n",dscdb,pdscdps);

    dscdps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(dscdps));
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

    dscdn = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(dscdn));
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
    if (!(WInDev[wDevID].ossdev.in_caps_support & WAVECAPS_DIRECTSOUND)) {
        ERR("DirectSoundCapture flag not set\n");
        MESSAGE("This sound card's driver does not support direct access\n");
        MESSAGE("The (slower) DirectSound HEL mode will be used instead.\n");
        return MMSYSERR_NOTSUPPORTED;
    }

    *idrv = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDsCaptureDriverImpl));
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
    memcpy(desc, &(WInDev[wDevID].ossdev.ds_desc), sizeof(DSDRIVERDESC));
    return MMSYSERR_NOERROR;
}

#endif /* HAVE_OSS */
