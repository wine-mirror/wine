/*
 * Copyright (C) 2003 Michael Günnewig
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

#define COM_NO_WINDOWS_H
#include "winbase.h"
#include "objbase.h"
#include "mediaobj.h"
#include "dmort.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msdmo);

/***********************************************************************
 *		MoCreateMediaType	(MSDMO.@)
 */
HRESULT WINAPI MoCreateMediaType(DMO_MEDIA_TYPE** ppmedia, DWORD cbFormat)
{
  HRESULT hr;

  TRACE("(%p,%lu)\n", ppmedia, cbFormat);

  if (ppmedia == NULL)
    return E_POINTER;

  *ppmedia = CoTaskMemAlloc(sizeof(DMO_MEDIA_TYPE));
  if (*ppmedia == NULL)
    return E_OUTOFMEMORY;

  hr = MoInitMediaType(*ppmedia, cbFormat);
  if (FAILED(hr)) {
    CoTaskMemFree(*ppmedia);
    *ppmedia = NULL;
  }

  return hr;
}

/***********************************************************************
 *		MoInitMediaType		(MSDMO.@)
 */
HRESULT WINAPI MoInitMediaType(DMO_MEDIA_TYPE* pmedia, DWORD cbFormat)
{
  TRACE("(%p,%lu)\n", pmedia,cbFormat);

  if (pmedia == NULL)
    return E_POINTER;

  memset(pmedia, 0, sizeof(DMO_MEDIA_TYPE));

  if (cbFormat > 0) {
    pmedia->pbFormat = CoTaskMemAlloc(cbFormat);
    if (pmedia->pbFormat == NULL)
      return E_OUTOFMEMORY;

    pmedia->cbFormat = cbFormat;
  }

  return S_OK;
}

/***********************************************************************
 *		MoDeleteMediaType	(MSDMO.@)
 */
HRESULT WINAPI MoDeleteMediaType(DMO_MEDIA_TYPE* pmedia)
{
  TRACE("(%p)\n", pmedia);

  if (pmedia == NULL)
    return E_POINTER;

  MoFreeMediaType(pmedia);
  CoTaskMemFree(pmedia);

  return S_OK;
}

/***********************************************************************
 *		MoFreeMediaType		(MSDMO.@)
 */
HRESULT WINAPI MoFreeMediaType(DMO_MEDIA_TYPE* pmedia)
{
  TRACE("(%p)\n", pmedia);

  if (pmedia == NULL)
    return E_POINTER;

  if (pmedia->pUnk != NULL) {
    IUnknown_Release(pmedia->pUnk);
    pmedia->pUnk = NULL;
  }

  if (pmedia->pbFormat != NULL) {
    CoTaskMemFree(pmedia->pbFormat);
    pmedia->pbFormat = NULL;
  }

  return S_OK;
}

/***********************************************************************
 *		MoDuplicateMediaType	(MSDMO.@)
 */
HRESULT WINAPI MoDuplicateMediaType(DMO_MEDIA_TYPE** ppdst,
				    const DMO_MEDIA_TYPE* psrc)
{
  HRESULT hr;

  TRACE("(%p,%p)\n", ppdst, psrc);

  if (ppdst == NULL || psrc == NULL)
    return E_POINTER;

  *ppdst = CoTaskMemAlloc(sizeof(DMO_MEDIA_TYPE));
  if (*ppdst == NULL)
    return E_OUTOFMEMORY;

  hr = MoCopyMediaType(*ppdst, psrc);
  if (FAILED(hr)) {
    MoFreeMediaType(*ppdst);
    *ppdst = NULL;
  }

  return hr;
}

/***********************************************************************
 *		MoCopyMediaType		(MSDMO.@)
 */
HRESULT WINAPI MoCopyMediaType(DMO_MEDIA_TYPE* pdst,
			       const DMO_MEDIA_TYPE* psrc)
{
  TRACE("(%p,%p)\n", pdst, psrc);

  if (pdst == NULL || psrc == NULL)
    return E_POINTER;

  memcpy(&pdst->majortype,  &psrc->majortype,  sizeof(psrc->majortype));
  memcpy(&pdst->subtype,    &psrc->subtype,    sizeof(psrc->subtype));
  memcpy(&pdst->formattype, &psrc->formattype, sizeof(psrc->formattype));

  pdst->bFixedSizeSamples    = psrc->bFixedSizeSamples;
  pdst->bTemporalCompression = psrc->bTemporalCompression;
  pdst->lSampleSize          = psrc->lSampleSize;
  pdst->cbFormat             = psrc->cbFormat;

  if (psrc->pbFormat != NULL && psrc->cbFormat > 0) {
    pdst->pbFormat = CoTaskMemAlloc(psrc->cbFormat);
    if (pdst->pbFormat == NULL)
      return E_OUTOFMEMORY;

    memcpy(pdst->pbFormat, psrc->pbFormat, psrc->cbFormat);
  } else
    pdst->pbFormat = NULL;

  if (psrc->pUnk != NULL) {
    pdst->pUnk = psrc->pUnk;
    IUnknown_AddRef(pdst->pUnk);
  } else
    pdst->pUnk = NULL;

  return S_OK;
}
