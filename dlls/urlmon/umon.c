/*
 * UrlMon
 *
 * Copyright 1999 Corel Corporation
 *
 * Ulrich Czekalla
 *
 */

#include "windows.h"
#include "objbase.h"
#include "debugtools.h"

#include "urlmon.h"

DEFAULT_DEBUG_CHANNEL(win32);

/***********************************************************************
 *           CreateURLMoniker (URLMON.22)
 *
 * Create a url moniker
 *
 * RETURNS
 *    S_OK 		success
 *    E_OUTOFMEMORY	out of memory 
 *    MK_E_SYNTAX	not a valid url
 *
 */
HRESULT CreateURLMoniker(IMoniker *pmkContext, LPWSTR szURL, IMoniker **ppmk)
{
   TRACE("\n");

   if (NULL != pmkContext)
	FIXME("Non-null pmkContext not implemented\n");

   return CreateFileMoniker(szURL, ppmk);
}
