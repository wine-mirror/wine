/*
 * Transform Filter declarations
 *
 * Copyright 2005 Christian Costa
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

typedef struct TransformFilterImpl TransformFilterImpl;

typedef DWORD (*PFN_PROCESS_SAMPLE) (TransformFilterImpl* This, LPBYTE data, DWORD size);
typedef HRESULT (*PFN_CONNECT_INPUT) (TransformFilterImpl* This, const AM_MEDIA_TYPE * pmt);
typedef HRESULT (*PFN_CLEANUP) (TransformFilterImpl* This);

struct TransformFilterImpl
{
    const IBaseFilterVtbl * lpVtbl;

    ULONG refCount;
    CRITICAL_SECTION csFilter;
    FILTER_STATE state;
    REFERENCE_TIME rtStreamStart;
    IReferenceClock * pClock;
    FILTER_INFO filterInfo;
    CLSID clsid;

    IPin ** ppPins;

    PFN_PROCESS_SAMPLE pfnProcessSample;
    PFN_CONNECT_INPUT pfnConnectInput;
    PFN_CLEANUP pfnCleanup;
};

HRESULT TransformFilter_Create(TransformFilterImpl*, const CLSID*, PFN_PROCESS_SAMPLE, PFN_CONNECT_INPUT, PFN_CLEANUP);
