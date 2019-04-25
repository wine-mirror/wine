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
