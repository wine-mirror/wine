/*
 * Wrapper for 16 bit avifile functions
 *
 * Copyright 2016 Michael Müller
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

#include "wine/winbase16.h"
#include "winternl.h"
#include "wingdi.h"
#include "vfw.h"

typedef struct _AVISTREAMINFO16 {
    DWORD   fccType;
    DWORD   fccHandler;
    DWORD   dwFlags;
    DWORD   dwCaps;
    WORD    wPriority;
    WORD    wLanguage;
    DWORD   dwScale;
    DWORD   dwRate;
    DWORD   dwStart;
    DWORD   dwLength;
    DWORD   dwInitialFrames;
    DWORD   dwSuggestedBufferSize;
    DWORD   dwQuality;
    DWORD   dwSampleSize;
    RECT16  rcFrame;
    DWORD   dwEditCount;
    DWORD   dwFormatChangeCount;
    CHAR    szName[64];
} AVISTREAMINFO16, *LPAVISTREAMINFO16, *PAVISTREAMINFO16;

struct frame_wrapper16
{
    PGETFRAME pg;
    PVOID ptr;
    DWORD size;
    WORD sel;
    WORD count;
};

static void free_segptr_frame(struct frame_wrapper16 *wrapper)
{
    int i;

    if (!wrapper->sel)
        return;

    for (i = 0; i < wrapper->count; i++)
        FreeSelector16(wrapper->sel + (i << __AHSHIFT));

    wrapper->sel = 0;
}

static SEGPTR alloc_segptr_frame(struct frame_wrapper16 *wrapper, void *ptr, DWORD size)
{
    int i;

    if (wrapper->sel)
    {
        if (wrapper->ptr == ptr && wrapper->size == size)
            return MAKESEGPTR(wrapper->sel, 0);
        free_segptr_frame(wrapper);
    }

    wrapper->ptr    = ptr;
    wrapper->size   = size;
    wrapper->count  = (size + 0xffff) / 0x10000;
    wrapper->sel    = AllocSelectorArray16(wrapper->count);
    if (!wrapper->sel)
        return 0;

    for (i = 0; i < wrapper->count; i++)
    {
        SetSelectorBase(wrapper->sel + (i << __AHSHIFT), (DWORD)ptr + i * 0x10000);
        SetSelectorLimit16(wrapper->sel + (i << __AHSHIFT), size - 1);
        size -= 0x10000;
    }

    return MAKESEGPTR(wrapper->sel, 0);
}

/***********************************************************************
 *      AVIStreamGetFrameOpen   (AVIFILE.112)
 */
PGETFRAME WINAPI AVIStreamGetFrameOpen16(PAVISTREAM pstream, LPBITMAPINFOHEADER lpbiWanted)
{
    struct frame_wrapper16 *wrapper;
    PGETFRAME pg;

    pg = AVIStreamGetFrameOpen(pstream, lpbiWanted);
    if (!pg) return NULL;

    wrapper = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*wrapper));
    if (!wrapper)
    {
        AVIStreamGetFrameClose(pg);
        return NULL;
    }

    wrapper->pg = pg;
    return (PGETFRAME)wrapper;
}

/***********************************************************************
 *      AVIStreamGetFrame	(AVIFILE.110)
 */
SEGPTR WINAPI AVIStreamGetFrame16(PGETFRAME pg, LONG pos)
{
    struct frame_wrapper16 *wrapper = (void *)pg;
    BITMAPINFOHEADER *bih;

    if (!pg) return 0;

    bih = AVIStreamGetFrame(wrapper->pg, pos);
    if (bih)
    {
        DWORD size = bih->biSize + bih->biSizeImage;
        return alloc_segptr_frame(wrapper, bih, size);
    }

    return 0;
}


/***********************************************************************
 *      AVIStreamGetFrameClose  (AVIFILE.111)
 */
HRESULT WINAPI AVIStreamGetFrameClose16(PGETFRAME pg)
{
    struct frame_wrapper16 *wrapper = (void *)pg;
    HRESULT hr;

    if (!pg) return S_OK;

    hr = AVIStreamGetFrameClose(wrapper->pg);
    free_segptr_frame(wrapper);
    HeapFree(GetProcessHeap(), 0, wrapper);
    return hr;
}

/***********************************************************************
 *      AVIFileCreateStream (AVIFILE.144)
 */
HRESULT WINAPI AVIFileCreateStream16(PAVIFILE pfile, PAVISTREAM *ppavi, LPAVISTREAMINFO16 asi16)
{
    AVISTREAMINFOA asi;

    if (!asi16)
        return AVIFileCreateStreamA(pfile, ppavi, NULL);

    asi.fccType               = asi16->fccType;
    asi.fccHandler            = asi16->fccHandler;
    asi.dwFlags               = asi16->dwFlags;
    asi.dwCaps                = asi16->dwCaps;
    asi.wPriority             = asi16->wPriority;
    asi.wLanguage             = asi16->wLanguage;
    asi.dwScale               = asi16->dwScale;
    asi.dwRate                = asi16->dwRate;
    asi.dwStart               = asi16->dwStart;
    asi.dwLength              = asi16->dwLength;
    asi.dwInitialFrames       = asi16->dwInitialFrames;
    asi.dwSuggestedBufferSize = asi16->dwSuggestedBufferSize;
    asi.dwQuality             = asi16->dwQuality;
    asi.dwSampleSize          = asi16->dwSampleSize;
    asi.rcFrame.left          = asi16->rcFrame.left;
    asi.rcFrame.top           = asi16->rcFrame.top;
    asi.rcFrame.right         = asi16->rcFrame.right;
    asi.rcFrame.bottom        = asi16->rcFrame.bottom;
    asi.dwEditCount           = asi16->dwEditCount;
    asi.dwFormatChangeCount   = asi16->dwFormatChangeCount;
    strcpy( asi.szName, asi16->szName );

    return AVIFileCreateStreamA(pfile, ppavi, &asi);
}


/***********************************************************************
 *      AVIStreamInfo       (AVIFILE.162)
 */
HRESULT WINAPI AVIStreamInfo16(PAVISTREAM pstream, LPAVISTREAMINFO16 asi16, LONG size)
{
    AVISTREAMINFOA asi;
    HRESULT hr;

    if (!asi16)
        return AVIStreamInfoA(pstream, NULL, size);

    if (size < sizeof(AVISTREAMINFO16))
        return AVIERR_BADSIZE;

    hr = AVIStreamInfoA(pstream, &asi, sizeof(asi));
    if (SUCCEEDED(hr))
    {
        asi16->fccType                = asi.fccType;
        asi16->fccHandler             = asi.fccHandler;
        asi16->dwFlags                = asi.dwFlags;
        asi16->dwCaps                 = asi.dwCaps;
        asi16->wPriority              = asi.wPriority;
        asi16->wLanguage              = asi.wLanguage;
        asi16->dwScale                = asi.dwScale;
        asi16->dwRate                 = asi.dwRate;
        asi16->dwStart                = asi.dwStart;
        asi16->dwLength               = asi.dwLength;
        asi16->dwInitialFrames        = asi.dwInitialFrames;
        asi16->dwSuggestedBufferSize  = asi.dwSuggestedBufferSize;
        asi16->dwQuality              = asi.dwQuality;
        asi16->dwSampleSize           = asi.dwSampleSize;
        asi16->rcFrame.left           = asi.rcFrame.left;
        asi16->rcFrame.top            = asi.rcFrame.top;
        asi16->rcFrame.right          = asi.rcFrame.right;
        asi16->rcFrame.bottom         = asi.rcFrame.bottom;
        asi16->dwEditCount            = asi.dwEditCount;
        asi16->dwFormatChangeCount    = asi.dwFormatChangeCount;
        strcpy( asi16->szName, asi.szName );
    }

    return hr;
}
