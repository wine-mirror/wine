/*
 * Sample Wine Driver for Open Sound System (featured in Linux and FreeBSD)
 *
 * Copyright 1994 Martin Ayotte
 *           1999 Eric Pouech (async playing in waveOut/waveIn)
 *	     2000 Eric Pouech (loops in waveOut)
 *           2002 Eric Pouech (full duplex)
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

WINE_DEFAULT_DEBUG_CHANNEL(wave);

#ifdef HAVE_OSS

/*======================================================================*
 *                  Low level DSOUND definitions                        *
 *======================================================================*/

typedef struct IDsDriverPropertySetImpl IDsDriverPropertySetImpl;
typedef struct IDsDriverNotifyImpl IDsDriverNotifyImpl;
typedef struct IDsDriverImpl IDsDriverImpl;
typedef struct IDsDriverBufferImpl IDsDriverBufferImpl;

struct IDsDriverPropertySetImpl
{
    /* IUnknown fields */
    const IDsDriverPropertySetVtbl *lpVtbl;
    LONG                        ref;

    IDsDriverBufferImpl*        buffer;
};

struct IDsDriverNotifyImpl
{
    /* IUnknown fields */
    const IDsDriverNotifyVtbl  *lpVtbl;
    LONG                        ref;

    /* IDsDriverNotifyImpl fields */
    LPDSBPOSITIONNOTIFY         notifies;
    int                         nrofnotifies;

    IDsDriverBufferImpl*        buffer;
};

struct IDsDriverImpl
{
    /* IUnknown fields */
    const IDsDriverVtbl        *lpVtbl;
    LONG                        ref;

    /* IDsDriverImpl fields */
    UINT                        wDevID;
    IDsDriverBufferImpl*        primary;

    int                         nrofsecondaries;
    IDsDriverBufferImpl**       secondaries;
};

struct IDsDriverBufferImpl
{
    /* IUnknown fields */
    const IDsDriverBufferVtbl  *lpVtbl;
    LONG                        ref;

    /* IDsDriverBufferImpl fields */
    IDsDriverImpl*              drv;
    DWORD                       buflen;
    WAVEFORMATPCMEX             wfex;
    LPBYTE                      mapping;
    DWORD                       maplen;
    int                         fd;
    DWORD                       dwFlags;

    /* IDsDriverNotifyImpl fields */
    IDsDriverNotifyImpl*        notify;
    int                         notify_index;

    /* IDsDriverPropertySetImpl fields */
    IDsDriverPropertySetImpl*   property_set;
};

static HRESULT WINAPI IDsDriverPropertySetImpl_Create(
    IDsDriverBufferImpl * dsdb,
    IDsDriverPropertySetImpl **pdsdps);

static HRESULT WINAPI IDsDriverNotifyImpl_Create(
    IDsDriverBufferImpl * dsdb,
    IDsDriverNotifyImpl **pdsdn);

/*======================================================================*
 *                  Low level DSOUND property set implementation        *
 *======================================================================*/

static HRESULT WINAPI IDsDriverPropertySetImpl_QueryInterface(
    PIDSDRIVERPROPERTYSET iface,
    REFIID riid,
    LPVOID *ppobj)
{
    IDsDriverPropertySetImpl *This = (IDsDriverPropertySetImpl *)iface;
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

static ULONG WINAPI IDsDriverPropertySetImpl_AddRef(PIDSDRIVERPROPERTYSET iface)
{
    IDsDriverPropertySetImpl *This = (IDsDriverPropertySetImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsDriverPropertySetImpl_Release(PIDSDRIVERPROPERTYSET iface)
{
    IDsDriverPropertySetImpl *This = (IDsDriverPropertySetImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount + 1);

    if (!refCount) {
        IDsDriverBuffer_Release((PIDSDRIVERBUFFER)This->buffer);
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return refCount;
}

static HRESULT WINAPI IDsDriverPropertySetImpl_Get(
    PIDSDRIVERPROPERTYSET iface,
    PDSPROPERTY pDsProperty,
    LPVOID pPropertyParams,
    ULONG cbPropertyParams,
    LPVOID pPropertyData,
    ULONG cbPropertyData,
    PULONG pcbReturnedData )
{
    IDsDriverPropertySetImpl *This = (IDsDriverPropertySetImpl *)iface;
    FIXME("(%p,%p,%p,%x,%p,%x,%p)\n",This,pDsProperty,pPropertyParams,cbPropertyParams,pPropertyData,cbPropertyData,pcbReturnedData);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverPropertySetImpl_Set(
    PIDSDRIVERPROPERTYSET iface,
    PDSPROPERTY pDsProperty,
    LPVOID pPropertyParams,
    ULONG cbPropertyParams,
    LPVOID pPropertyData,
    ULONG cbPropertyData )
{
    IDsDriverPropertySetImpl *This = (IDsDriverPropertySetImpl *)iface;
    FIXME("(%p,%p,%p,%x,%p,%x)\n",This,pDsProperty,pPropertyParams,cbPropertyParams,pPropertyData,cbPropertyData);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverPropertySetImpl_QuerySupport(
    PIDSDRIVERPROPERTYSET iface,
    REFGUID PropertySetId,
    ULONG PropertyId,
    PULONG pSupport )
{
    IDsDriverPropertySetImpl *This = (IDsDriverPropertySetImpl *)iface;
    FIXME("(%p,%s,%x,%p)\n",This,debugstr_guid(PropertySetId),PropertyId,pSupport);
    return DSERR_UNSUPPORTED;
}

static const IDsDriverPropertySetVtbl dsdpsvt =
{
    IDsDriverPropertySetImpl_QueryInterface,
    IDsDriverPropertySetImpl_AddRef,
    IDsDriverPropertySetImpl_Release,
    IDsDriverPropertySetImpl_Get,
    IDsDriverPropertySetImpl_Set,
    IDsDriverPropertySetImpl_QuerySupport,
};

/*======================================================================*
 *                  Low level DSOUND notify implementation              *
 *======================================================================*/

static HRESULT WINAPI IDsDriverNotifyImpl_QueryInterface(
    PIDSDRIVERNOTIFY iface,
    REFIID riid,
    LPVOID *ppobj)
{
    IDsDriverNotifyImpl *This = (IDsDriverNotifyImpl *)iface;
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

static ULONG WINAPI IDsDriverNotifyImpl_AddRef(PIDSDRIVERNOTIFY iface)
{
    IDsDriverNotifyImpl *This = (IDsDriverNotifyImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsDriverNotifyImpl_Release(PIDSDRIVERNOTIFY iface)
{
    IDsDriverNotifyImpl *This = (IDsDriverNotifyImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount + 1);

    if (!refCount) {
        IDsDriverBuffer_Release((PIDSDRIVERBUFFER)This->buffer);
        HeapFree(GetProcessHeap(), 0, This->notifies);
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return refCount;
}

static HRESULT WINAPI IDsDriverNotifyImpl_SetNotificationPositions(
    PIDSDRIVERNOTIFY iface,
    DWORD howmuch,
    LPCDSBPOSITIONNOTIFY notify)
{
    IDsDriverNotifyImpl *This = (IDsDriverNotifyImpl *)iface;
    TRACE("(%p,0x%08x,%p)\n",This,howmuch,notify);

    if (!notify) {
        WARN("invalid parameter\n");
        return DSERR_INVALIDPARAM;
    }

    if (TRACE_ON(wave)) {
        int i;
        for (i=0;i<howmuch;i++)
            TRACE("notify at %d to 0x%08x\n",
                notify[i].dwOffset,(DWORD)notify[i].hEventNotify);
    }

    /* Make an internal copy of the caller-supplied array.
     * Replace the existing copy if one is already present. */
    if (This->notifies)
        This->notifies = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
        This->notifies, howmuch * sizeof(DSBPOSITIONNOTIFY));
    else
        This->notifies = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
        howmuch * sizeof(DSBPOSITIONNOTIFY));

    memcpy(This->notifies, notify, howmuch * sizeof(DSBPOSITIONNOTIFY));
    This->nrofnotifies = howmuch;

    return S_OK;
}

static const IDsDriverNotifyVtbl dsdnvt =
{
    IDsDriverNotifyImpl_QueryInterface,
    IDsDriverNotifyImpl_AddRef,
    IDsDriverNotifyImpl_Release,
    IDsDriverNotifyImpl_SetNotificationPositions,
};

/*======================================================================*
 *                  Low level DSOUND implementation                     *
 *======================================================================*/

static HRESULT DSDB_MapBuffer(IDsDriverBufferImpl *dsdb)
{
    TRACE("(%p), format=%dx%dx%d\n", dsdb, dsdb->wfex.Format.nSamplesPerSec,
          dsdb->wfex.Format.wBitsPerSample, dsdb->wfex.Format.nChannels);
    if (!dsdb->mapping) {
        dsdb->mapping = mmap(NULL, dsdb->maplen, PROT_WRITE, MAP_SHARED,
                             dsdb->fd, 0);
        if (dsdb->mapping == (LPBYTE)-1) {
            WARN("Could not map sound device for direct access (%s)\n", strerror(errno));
            return DSERR_GENERIC;
        }
        TRACE("The sound device has been mapped for direct access at %p, size=%d\n", dsdb->mapping, dsdb->maplen);

	/* for some reason, es1371 and sblive! sometimes have junk in here.
	 * clear it, or we get junk noise */
	/* some libc implementations are buggy: their memset reads from the buffer...
	 * to work around it, we have to zero the block by hand. We don't do the expected:
	 * memset(dsdb->mapping,0, dsdb->maplen);
	 */
	{
            unsigned char*      p1 = dsdb->mapping;
            unsigned            len = dsdb->maplen;
	    unsigned char	silence = (dsdb->wfex.Format.wBitsPerSample == 8) ? 128 : 0;
	    unsigned long	ulsilence = (dsdb->wfex.Format.wBitsPerSample == 8) ? 0x80808080 : 0;

	    if (len >= 16) /* so we can have at least a 4 long area to store... */
	    {
		/* the mmap:ed value is (at least) dword aligned
		 * so, start filling the complete unsigned long:s
		 */
		int		b = len >> 2;
		unsigned long*	p4 = (unsigned long*)p1;

		while (b--) *p4++ = ulsilence;
		/* prepare for filling the rest */
		len &= 3;
		p1 = (unsigned char*)p4;
	    }
	    /* in all cases, fill the remaining bytes */
	    while (len-- != 0) *p1++ = silence;
	}
    }
    return DS_OK;
}

static HRESULT DSDB_UnmapBuffer(IDsDriverBufferImpl *dsdb)
{
    TRACE("(%p)\n",dsdb);
    if (dsdb->mapping) {
        if (munmap(dsdb->mapping, dsdb->maplen) < 0) {
            ERR("(%p): Could not unmap sound device (%s)\n", dsdb, strerror(errno));
            return DSERR_GENERIC;
        }
        dsdb->mapping = NULL;
        TRACE("(%p): sound device unmapped\n", dsdb);
    }
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_QueryInterface(PIDSDRIVERBUFFER iface, REFIID riid, LPVOID *ppobj)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    TRACE("(%p,%s,%p)\n",iface,debugstr_guid(riid),*ppobj);

    if ( IsEqualGUID(riid, &IID_IUnknown) ||
         IsEqualGUID(riid, &IID_IDsDriverBuffer) ) {
	IDsDriverBuffer_AddRef(iface);
	*ppobj = (LPVOID)This;
	return DS_OK;
    }

    if ( IsEqualGUID( &IID_IDsDriverNotify, riid ) ) {
        if (!This->notify)
            IDsDriverNotifyImpl_Create(This, &(This->notify));
        if (This->notify) {
            IDsDriverNotify_AddRef((PIDSDRIVERNOTIFY)This->notify);
            *ppobj = (LPVOID)This->notify;
            return DS_OK;
        }
        *ppobj = 0;
        return E_FAIL;
    }

    if ( IsEqualGUID( &IID_IDsDriverPropertySet, riid ) ) {
        if (!This->property_set)
            IDsDriverPropertySetImpl_Create(This, &(This->property_set));
        if (This->property_set) {
            IDsDriverPropertySet_AddRef((PIDSDRIVERPROPERTYSET)This->property_set);
            *ppobj = (LPVOID)This->property_set;
            return DS_OK;
        }
	*ppobj = 0;
	return E_FAIL;
    }

    FIXME( "Unknown IID %s\n", debugstr_guid( riid ) );

    *ppobj = 0;

    return E_NOINTERFACE;
}

static ULONG WINAPI IDsDriverBufferImpl_AddRef(PIDSDRIVERBUFFER iface)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsDriverBufferImpl_Release(PIDSDRIVERBUFFER iface)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount + 1);

    if (refCount)
        return refCount;

    if (This == This->drv->primary)
	This->drv->primary = NULL;
    else {
        int i;
        for (i = 0; i < This->drv->nrofsecondaries; i++)
            if (This->drv->secondaries[i] == This)
                break;
        if (i < This->drv->nrofsecondaries) {
            /* Put the last buffer of the list in the (now empty) position */
            This->drv->secondaries[i] = This->drv->secondaries[This->drv->nrofsecondaries - 1];
            This->drv->nrofsecondaries--;
            This->drv->secondaries = HeapReAlloc(GetProcessHeap(),0,
                This->drv->secondaries,
                sizeof(PIDSDRIVERBUFFER)*This->drv->nrofsecondaries);
            TRACE("(%p) buffer count is now %d\n", This, This->drv->nrofsecondaries);
        }

        WOutDev[This->drv->wDevID].ossdev->ds_caps.dwFreeHwMixingAllBuffers++;
        WOutDev[This->drv->wDevID].ossdev->ds_caps.dwFreeHwMixingStreamingBuffers++;
    }

    DSDB_UnmapBuffer(This);
    HeapFree(GetProcessHeap(),0,This);
    TRACE("(%p) released\n",This);
    return 0;
}

static HRESULT WINAPI IDsDriverBufferImpl_Lock(PIDSDRIVERBUFFER iface,
					       LPVOID*ppvAudio1,LPDWORD pdwLen1,
					       LPVOID*ppvAudio2,LPDWORD pdwLen2,
					       DWORD dwWritePosition,DWORD dwWriteLen,
					       DWORD dwFlags)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    /* since we (GetDriverDesc flags) have specified DSDDESC_DONTNEEDPRIMARYLOCK,
     * and that we don't support secondary buffers, this method will never be called */
    TRACE("(%p): stub\n",iface);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_Unlock(PIDSDRIVERBUFFER iface,
						 LPVOID pvAudio1,DWORD dwLen1,
						 LPVOID pvAudio2,DWORD dwLen2)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    TRACE("(%p): stub\n",iface);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetFormat(PIDSDRIVERBUFFER iface,
						    LPWAVEFORMATEX pwfx)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */

    TRACE("(%p,%p)\n",iface,pwfx);
    /* On our request (GetDriverDesc flags), DirectSound has by now used
     * waveOutClose/waveOutOpen to set the format...
     * unfortunately, this means our mmap() is now gone...
     * so we need to somehow signal to our DirectSound implementation
     * that it should completely recreate this HW buffer...
     * this unexpected error code should do the trick... */
    return DSERR_BUFFERLOST;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetFrequency(PIDSDRIVERBUFFER iface, DWORD dwFreq)
{
    /* IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface; */
    TRACE("(%p,%d): stub\n",iface,dwFreq);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetVolumePan(PIDSDRIVERBUFFER iface, PDSVOLUMEPAN pVolPan)
{
    DWORD vol;
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    TRACE("(%p,%p)\n",This,pVolPan);

    vol = pVolPan->dwTotalLeftAmpFactor | (pVolPan->dwTotalRightAmpFactor << 16);

    if (wodSetVolume(This->drv->wDevID, vol) != MMSYSERR_NOERROR) {
	WARN("wodSetVolume failed\n");
	return DSERR_INVALIDPARAM;
    }

    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_SetPosition(PIDSDRIVERBUFFER iface, DWORD dwNewPos)
{
    /* IDsDriverImpl *This = (IDsDriverImpl *)iface; */
    TRACE("(%p,%d): stub\n",iface,dwNewPos);
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverBufferImpl_GetPosition(PIDSDRIVERBUFFER iface,
						      LPDWORD lpdwPlay, LPDWORD lpdwWrite)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    count_info info;
    DWORD ptr;

    TRACE("(%p)\n",iface);
    if (WOutDev[This->drv->wDevID].state == WINE_WS_CLOSED) {
	ERR("device not open, but accessing?\n");
	return DSERR_UNINITIALIZED;
    }
    if (ioctl(This->fd, SNDCTL_DSP_GETOPTR, &info) < 0) {
        ERR("ioctl(%s, SNDCTL_DSP_GETOPTR) failed (%s)\n",
            WOutDev[This->drv->wDevID].ossdev->dev_name, strerror(errno));
	return DSERR_GENERIC;
    }
    ptr = info.ptr & ~3; /* align the pointer, just in case */
    if (lpdwPlay) *lpdwPlay = ptr;
    if (lpdwWrite) {
	/* add some safety margin (not strictly necessary, but...) */
	if (WOutDev[This->drv->wDevID].ossdev->duplex_out_caps.dwSupport & WAVECAPS_SAMPLEACCURATE)
	    *lpdwWrite = ptr + 32;
	else
	    *lpdwWrite = ptr + WOutDev[This->drv->wDevID].dwFragmentSize;
	while (*lpdwWrite >= This->buflen)
	    *lpdwWrite -= This->buflen;
    }
    TRACE("playpos=%d, writepos=%d\n", lpdwPlay?*lpdwPlay:0, lpdwWrite?*lpdwWrite:0);
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_Play(PIDSDRIVERBUFFER iface, DWORD dwRes1, DWORD dwRes2, DWORD dwFlags)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    int enable;
    TRACE("(%p,%x,%x,%x)\n",iface,dwRes1,dwRes2,dwFlags);
    WOutDev[This->drv->wDevID].ossdev->bOutputEnabled = TRUE;
    enable = getEnables(WOutDev[This->drv->wDevID].ossdev);
    if (ioctl(This->fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
	if (errno == EINVAL) {
	    /* Don't give up yet. OSS trigger support is inconsistent. */
	    if (WOutDev[This->drv->wDevID].ossdev->open_count == 1) {
		/* try the opposite input enable */
		if (WOutDev[This->drv->wDevID].ossdev->bInputEnabled == FALSE)
		    WOutDev[This->drv->wDevID].ossdev->bInputEnabled = TRUE;
		else
		    WOutDev[This->drv->wDevID].ossdev->bInputEnabled = FALSE;
		/* try it again */
    		enable = getEnables(WOutDev[This->drv->wDevID].ossdev);
                if (ioctl(This->fd, SNDCTL_DSP_SETTRIGGER, &enable) >= 0)
		    return DS_OK;
	    }
	}
        ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER) failed (%s)\n",
            WOutDev[This->drv->wDevID].ossdev->dev_name, strerror(errno));
	WOutDev[This->drv->wDevID].ossdev->bOutputEnabled = FALSE;
	return DSERR_GENERIC;
    }
    return DS_OK;
}

static HRESULT WINAPI IDsDriverBufferImpl_Stop(PIDSDRIVERBUFFER iface)
{
    IDsDriverBufferImpl *This = (IDsDriverBufferImpl *)iface;
    int enable;
    TRACE("(%p)\n",iface);
    /* no more playing */
    WOutDev[This->drv->wDevID].ossdev->bOutputEnabled = FALSE;
    enable = getEnables(WOutDev[This->drv->wDevID].ossdev);
    if (ioctl(This->fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
	ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER) failed (%s)\n", WOutDev[This->drv->wDevID].ossdev->dev_name, strerror(errno));
	return DSERR_GENERIC;
    }
#if 0
    /* the play position must be reset to the beginning of the buffer */
    if (ioctl(This->fd, SNDCTL_DSP_RESET, 0) < 0) {
	ERR("ioctl(%s, SNDCTL_DSP_RESET) failed (%s)\n", WOutDev[This->drv->wDevID].ossdev->dev_name, strerror(errno));
	return DSERR_GENERIC;
    }
#endif
    /* Most OSS drivers just can't stop the playback without closing the device...
     * so we need to somehow signal to our DirectSound implementation
     * that it should completely recreate this HW buffer...
     * this unexpected error code should do the trick... */
    /* FIXME: ...unless we are doing full duplex, then it's not nice to close the device */
    if (WOutDev[This->drv->wDevID].ossdev->open_count == 1)
	return DSERR_BUFFERLOST;

    return DS_OK;
}

static const IDsDriverBufferVtbl dsdbvt =
{
    IDsDriverBufferImpl_QueryInterface,
    IDsDriverBufferImpl_AddRef,
    IDsDriverBufferImpl_Release,
    IDsDriverBufferImpl_Lock,
    IDsDriverBufferImpl_Unlock,
    IDsDriverBufferImpl_SetFormat,
    IDsDriverBufferImpl_SetFrequency,
    IDsDriverBufferImpl_SetVolumePan,
    IDsDriverBufferImpl_SetPosition,
    IDsDriverBufferImpl_GetPosition,
    IDsDriverBufferImpl_Play,
    IDsDriverBufferImpl_Stop
};

static HRESULT WINAPI IDsDriverImpl_QueryInterface(PIDSDRIVER iface, REFIID riid, LPVOID *ppobj)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    if ( IsEqualGUID(riid, &IID_IUnknown) ||
         IsEqualGUID(riid, &IID_IDsDriver) ) {
	IDsDriver_AddRef(iface);
	*ppobj = (LPVOID)This;
	return DS_OK;
    }

    FIXME( "Unknown IID %s\n", debugstr_guid( riid ) );

    *ppobj = 0;

    return E_NOINTERFACE;
}

static ULONG WINAPI IDsDriverImpl_AddRef(PIDSDRIVER iface)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDsDriverImpl_Release(PIDSDRIVER iface)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, refCount + 1);

    if (!refCount) {
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return refCount;
}

static HRESULT WINAPI IDsDriverImpl_GetDriverDesc(PIDSDRIVER iface,
                                                  PDSDRIVERDESC pDesc)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    TRACE("(%p,%p)\n",iface,pDesc);

    /* copy version from driver */
    memcpy(pDesc, &(WOutDev[This->wDevID].ossdev->ds_desc), sizeof(DSDRIVERDESC));

    pDesc->dwFlags |= DSDDESC_DOMMSYSTEMOPEN | DSDDESC_DOMMSYSTEMSETFORMAT |
        DSDDESC_USESYSTEMMEMORY | DSDDESC_DONTNEEDPRIMARYLOCK |
        DSDDESC_DONTNEEDSECONDARYLOCK;
    pDesc->dnDevNode		= WOutDev[This->wDevID].waveDesc.dnDevNode;
    pDesc->wVxdId		= 0;
    pDesc->wReserved		= 0;
    pDesc->ulDeviceNum		= This->wDevID;
    pDesc->dwHeapType		= DSDHEAP_NOHEAP;
    pDesc->pvDirectDrawHeap	= NULL;
    pDesc->dwMemStartAddress	= 0;
    pDesc->dwMemEndAddress	= 0;
    pDesc->dwMemAllocExtra	= 0;
    pDesc->pvReserved1		= NULL;
    pDesc->pvReserved2		= NULL;
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_Open(PIDSDRIVER iface)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    int enable;
    TRACE("(%p)\n",iface);

    /* make sure the card doesn't start playing before we want it to */
    WOutDev[This->wDevID].ossdev->bOutputEnabled = FALSE;
    WOutDev[This->wDevID].ossdev->bInputEnabled = FALSE;
    enable = getEnables(WOutDev[This->wDevID].ossdev);
    if (ioctl(WOutDev[This->wDevID].ossdev->fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
	ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER) failed (%s)\n",WOutDev[This->wDevID].ossdev->dev_name, strerror(errno));
	return DSERR_GENERIC;
    }
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_Close(PIDSDRIVER iface)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    TRACE("(%p)\n",iface);
    if (This->primary) {
	ERR("problem with DirectSound: primary not released\n");
	return DSERR_GENERIC;
    }
    return DS_OK;
}

static HRESULT WINAPI IDsDriverImpl_GetCaps(PIDSDRIVER iface, PDSDRIVERCAPS pCaps)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    TRACE("(%p,%p)\n",iface,pCaps);
    memcpy(pCaps, &(WOutDev[This->wDevID].ossdev->ds_caps), sizeof(DSDRIVERCAPS));
    return DS_OK;
}

static HRESULT WINAPI DSD_CreatePrimaryBuffer(PIDSDRIVER iface,
                                              LPWAVEFORMATEX pwfx,
                                              DWORD dwFlags,
                                              DWORD dwCardAddress,
                                              LPDWORD pdwcbBufferSize,
                                              LPBYTE *ppbBuffer,
                                              LPVOID *ppvObj)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    IDsDriverBufferImpl** ippdsdb = (IDsDriverBufferImpl**)ppvObj;
    HRESULT err;
    audio_buf_info info;
    int enable = 0;
    TRACE("(%p,%p,%x,%x,%p,%p,%p)\n",iface,pwfx,dwFlags,dwCardAddress,pdwcbBufferSize,ppbBuffer,ppvObj);

    if (This->primary)
	return DSERR_ALLOCATED;
    if (dwFlags & (DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN))
	return DSERR_CONTROLUNAVAIL;

    *ippdsdb = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDsDriverBufferImpl));
    if (*ippdsdb == NULL)
	return DSERR_OUTOFMEMORY;
    (*ippdsdb)->lpVtbl  = &dsdbvt;
    (*ippdsdb)->ref	= 1;
    (*ippdsdb)->drv	= This;
    copy_format(pwfx, &(*ippdsdb)->wfex);
    (*ippdsdb)->fd      = WOutDev[This->wDevID].ossdev->fd;
    (*ippdsdb)->dwFlags = dwFlags;

    /* check how big the DMA buffer is now */
    if (ioctl((*ippdsdb)->fd, SNDCTL_DSP_GETOSPACE, &info) < 0) {
        ERR("ioctl(%s, SNDCTL_DSP_GETOSPACE) failed (%s)\n",
            WOutDev[This->wDevID].ossdev->dev_name, strerror(errno));
	HeapFree(GetProcessHeap(),0,*ippdsdb);
	*ippdsdb = NULL;
	return DSERR_GENERIC;
    }
    (*ippdsdb)->maplen = (*ippdsdb)->buflen = info.fragstotal * info.fragsize;

    /* map the DMA buffer */
    err = DSDB_MapBuffer(*ippdsdb);
    if (err != DS_OK) {
	HeapFree(GetProcessHeap(),0,*ippdsdb);
	*ippdsdb = NULL;
	return err;
    }

    /* primary buffer is ready to go */
    *pdwcbBufferSize    = (*ippdsdb)->maplen;
    *ppbBuffer          = (*ippdsdb)->mapping;

    /* some drivers need some extra nudging after mapping */
    WOutDev[This->wDevID].ossdev->bInputEnabled = FALSE;
    WOutDev[This->wDevID].ossdev->bOutputEnabled = FALSE;
    enable = getEnables(WOutDev[This->wDevID].ossdev);
    if (ioctl((*ippdsdb)->fd, SNDCTL_DSP_SETTRIGGER, &enable) < 0) {
        ERR("ioctl(%s, SNDCTL_DSP_SETTRIGGER) failed (%s)\n",
            WOutDev[This->wDevID].ossdev->dev_name, strerror(errno));
	return DSERR_GENERIC;
    }

    This->primary = *ippdsdb;

    return DS_OK;
}

static HRESULT WINAPI DSD_CreateSecondaryBuffer(PIDSDRIVER iface,
                                                LPWAVEFORMATEX pwfx,
                                                DWORD dwFlags,
                                                DWORD dwCardAddress,
                                                LPDWORD pdwcbBufferSize,
                                                LPBYTE *ppbBuffer,
                                                LPVOID *ppvObj)
{
    IDsDriverImpl *This = (IDsDriverImpl *)iface;
    IDsDriverBufferImpl** ippdsdb = (IDsDriverBufferImpl**)ppvObj;
    FIXME("(%p,%p,%x,%x,%p,%p,%p): stub\n",This,pwfx,dwFlags,dwCardAddress,pdwcbBufferSize,ppbBuffer,ppvObj);

    *ippdsdb = 0;
    return DSERR_UNSUPPORTED;
}

static HRESULT WINAPI IDsDriverImpl_CreateSoundBuffer(PIDSDRIVER iface,
                                                      LPWAVEFORMATEX pwfx,
                                                      DWORD dwFlags,
                                                      DWORD dwCardAddress,
                                                      LPDWORD pdwcbBufferSize,
                                                      LPBYTE *ppbBuffer,
                                                      LPVOID *ppvObj)
{
    TRACE("(%p,%p,%x,%x,%p,%p,%p)\n",iface,pwfx,dwFlags,dwCardAddress,pdwcbBufferSize,ppbBuffer,ppvObj);

    if (dwFlags & DSBCAPS_PRIMARYBUFFER)
        return DSD_CreatePrimaryBuffer(iface,pwfx,dwFlags,dwCardAddress,pdwcbBufferSize,ppbBuffer,ppvObj);

    return DSD_CreateSecondaryBuffer(iface,pwfx,dwFlags,dwCardAddress,pdwcbBufferSize,ppbBuffer,ppvObj);
}

static HRESULT WINAPI IDsDriverImpl_DuplicateSoundBuffer(PIDSDRIVER iface,
							 PIDSDRIVERBUFFER pBuffer,
							 LPVOID *ppvObj)
{
    /* IDsDriverImpl *This = (IDsDriverImpl *)iface; */
    TRACE("(%p,%p): stub\n",iface,pBuffer);
    return DSERR_INVALIDCALL;
}

static const IDsDriverVtbl dsdvt =
{
    IDsDriverImpl_QueryInterface,
    IDsDriverImpl_AddRef,
    IDsDriverImpl_Release,
    IDsDriverImpl_GetDriverDesc,
    IDsDriverImpl_Open,
    IDsDriverImpl_Close,
    IDsDriverImpl_GetCaps,
    IDsDriverImpl_CreateSoundBuffer,
    IDsDriverImpl_DuplicateSoundBuffer
};

static HRESULT WINAPI IDsDriverPropertySetImpl_Create(
    IDsDriverBufferImpl * dsdb,
    IDsDriverPropertySetImpl **pdsdps)
{
    IDsDriverPropertySetImpl * dsdps;
    TRACE("(%p,%p)\n",dsdb,pdsdps);

    dsdps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(dsdps));
    if (dsdps == NULL) {
        WARN("out of memory\n");
        return DSERR_OUTOFMEMORY;
    }

    dsdps->ref = 0;
    dsdps->lpVtbl = &dsdpsvt;
    dsdps->buffer = dsdb;
    dsdb->property_set = dsdps;
    IDsDriverBuffer_AddRef((PIDSDRIVER)dsdb);

    *pdsdps = dsdps;
    return DS_OK;
}

static HRESULT WINAPI IDsDriverNotifyImpl_Create(
    IDsDriverBufferImpl * dsdb,
    IDsDriverNotifyImpl **pdsdn)
{
    IDsDriverNotifyImpl * dsdn;
    TRACE("(%p,%p)\n",dsdb,pdsdn);

    dsdn = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(dsdn));

    if (dsdn == NULL) {
        WARN("out of memory\n");
        return DSERR_OUTOFMEMORY;
    }

    dsdn->ref = 0;
    dsdn->lpVtbl = &dsdnvt;
    dsdn->buffer = dsdb;
    dsdb->notify = dsdn;
    IDsDriverBuffer_AddRef((PIDSDRIVER)dsdb);

    *pdsdn = dsdn;
    return DS_OK;
}

DWORD wodDsCreate(UINT wDevID, PIDSDRIVER* drv)
{
    IDsDriverImpl** idrv = (IDsDriverImpl**)drv;
    TRACE("(%d,%p)\n",wDevID,drv);

    /* the HAL isn't much better than the HEL if we can't do mmap() */
    if (!(WOutDev[wDevID].ossdev->duplex_out_caps.dwSupport & WAVECAPS_DIRECTSOUND)) {
        WARN("Warn DirectSound flag not set, falling back to HEL layer\n");
        return MMSYSERR_NOTSUPPORTED;
    }

    *idrv = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDsDriverImpl));
    if (!*idrv)
	return MMSYSERR_NOMEM;
    (*idrv)->lpVtbl          = &dsdvt;
    (*idrv)->ref             = 1;
    (*idrv)->wDevID          = wDevID;
    (*idrv)->primary         = NULL;
    (*idrv)->nrofsecondaries = 0;
    (*idrv)->secondaries     = NULL;

    return MMSYSERR_NOERROR;
}

DWORD wodDsDesc(UINT wDevID, PDSDRIVERDESC desc)
{
    TRACE("(%d,%p)\n",wDevID,desc);
    memcpy(desc, &(WOutDev[wDevID].ossdev->ds_desc), sizeof(DSDRIVERDESC));
    return MMSYSERR_NOERROR;
}

#endif /* HAVE_OSS */
