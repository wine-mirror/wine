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

HRESULT FILTERGRAPH_create(IUnknown *pUnkOuter, LPVOID *ppObj) ;
HRESULT FilterMapper2_create(IUnknown *pUnkOuter, LPVOID *ppObj);
HRESULT AsyncReader_create(IUnknown * pUnkOuter, LPVOID * ppv);

HRESULT EnumMonikerImpl_Create(IMoniker ** ppMoniker, ULONG nMonikerCount, IEnumMoniker ** ppEnum);

typedef struct tagENUMPINDETAILS
{
	ULONG cPins;
	IPin ** ppPins;
} ENUMPINDETAILS;

typedef struct tagENUMEDIADETAILS
{
	ULONG cMediaTypes;
	AM_MEDIA_TYPE * pMediaTypes;
} ENUMMEDIADETAILS;

HRESULT IEnumPinsImpl_Construct(const ENUMPINDETAILS * pDetails, IEnumPins ** ppEnum);
HRESULT IEnumMediaTypesImpl_Construct(const ENUMMEDIADETAILS * pDetails, IEnumMediaTypes ** ppEnum);

extern const char * qzdebugstr_guid(const GUID * id);
extern const char * qzdebugstr_State(FILTER_STATE state);

void CopyMediaType(AM_MEDIA_TYPE * pDest, const AM_MEDIA_TYPE *pSrc);
BOOL CompareMediaTypes(const AM_MEDIA_TYPE * pmt1, const AM_MEDIA_TYPE * pmt2, BOOL bWildcards);
void dump_AM_MEDIA_TYPE(const AM_MEDIA_TYPE * pmt);

#endif /* __QUARTZ_PRIVATE_INCLUDED__ */
