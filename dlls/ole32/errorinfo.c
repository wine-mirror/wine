/*
 * ErrorInfo API
 *
 * Copyright 2000 Patrik Stridvall
 *
 */

#include "debugtools.h"
#include "oleauto.h"
#include "windef.h"
#include "winerror.h"

DEFAULT_DEBUG_CHANNEL(ole)

/***********************************************************************
 *		CreateErrorInfo
 */
HRESULT WINAPI CreateErrorInfo(ICreateErrorInfo **pperrinfo)
{
  FIXME("(%p): stub:\n", pperrinfo);
  
  return S_OK;
}

/***********************************************************************
 *		GetErrorInfo
 */
HRESULT WINAPI GetErrorInfo(ULONG dwReserved, IErrorInfo **pperrinfo)
{
  FIXME("(%ld, %p): stub:\n", dwReserved, pperrinfo);
  
  return S_OK;
}

/***********************************************************************
 *		SetErrorInfo
 */
HRESULT WINAPI SetErrorInfo(ULONG dwReserved, IErrorInfo *perrinfo)
{
  FIXME("(%ld, %p): stub:\n", dwReserved, perrinfo);
  
  return S_OK;
}
