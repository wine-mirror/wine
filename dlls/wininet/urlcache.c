/*
 * Wininet - Url Cache functions
 *
 * Copyright 2001 CodeWeavers
 *
 * Eric Kohl
 *
 */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winerror.h"

#include "debugtools.h"
#include "internet.h"

DEFAULT_DEBUG_CHANNEL(wininet);


INTERNETAPI HANDLE WINAPI FindFirstUrlCacheEntryA(LPCSTR lpszUrlSearchPattern,
 LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo, LPDWORD lpdwFirstCacheEntryInfoBufferSize)
{
  return 0;
}

INTERNETAPI HANDLE WINAPI FindFirstUrlCacheEntryW(LPCWSTR lpszUrlSearchPattern,
 LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo, LPDWORD lpdwFirstCacheEntryInfoBufferSize)
{
  return 0;
}
