/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "imagehlp.h"
#include "winerror.h"
#include "winbase.h"
#include "wintypes.h"
#include "debug.h"

/**********************************************************************/

HANDLE32 IMAGEHLP_hHeap32 = (HANDLE32) NULL;

static API_VERSION32 IMAGEHLP_ApiVersion = { 4, 0, 0, 5 };

/***********************************************************************
 *           IMAGEHLP_LibMain (IMAGEHLP.init)
 */
BOOL32 WINAPI IMAGEHLP_LibMain(
  HINSTANCE32 hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
      IMAGEHLP_hHeap32 = HeapCreate(0, 0x10000, 0);
      break;
    case DLL_PROCESS_DETACH:
      HeapDestroy(IMAGEHLP_hHeap32);
      IMAGEHLP_hHeap32 = (HANDLE32) NULL;
      break;
    case DLL_THREAD_ATTACH:
      break;
    case DLL_THREAD_DETACH:
      break;
    default:
      break;
    }
  return TRUE;
}

/***********************************************************************
 *           ImagehlpApiVersion32 (IMAGEHLP.22)
 */
PAPI_VERSION32 WINAPI ImagehlpApiVersion32()
{
  return &IMAGEHLP_ApiVersion;
}

/***********************************************************************
 *           ImagehlpApiVersionEx32 (IMAGEHLP.23)
 */
PAPI_VERSION32 WINAPI ImagehlpApiVersionEx32(PAPI_VERSION32 AppVersion)
{
  if(!AppVersion)
    return NULL;

  AppVersion->MajorVersion = IMAGEHLP_ApiVersion.MajorVersion;
  AppVersion->MinorVersion = IMAGEHLP_ApiVersion.MinorVersion;
  AppVersion->Revision = IMAGEHLP_ApiVersion.Revision;
  AppVersion->Reserved = IMAGEHLP_ApiVersion.Reserved;

  return AppVersion;
}

/***********************************************************************
 *           MakeSureDirectoryPathExists32 (IMAGEHLP.24)
 */
BOOL32 WINAPI MakeSureDirectoryPathExists32(LPCSTR DirPath)
{
  FIXME(imagehlp, "(%s): stub\n", debugstr_a(DirPath));
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           MarkImageAsRunFromSwap (IMAGEHLP.29)
 * FIXME
 *   No documentation available.
 */

/***********************************************************************
 *           SearchTreeForFile32 (IMAGEHLP.33)
 */
BOOL32 WINAPI SearchTreeForFile32(
  LPSTR RootPath, LPSTR InputPathName, LPSTR OutputPathBuffer)
{
  FIXME(imagehlp, "(%s, %s, %s): stub\n",
    debugstr_a(RootPath), debugstr_a(InputPathName), 
    debugstr_a(OutputPathBuffer)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           TouchFileTimes32 (IMAGEHLP.56)
 */
BOOL32 WINAPI TouchFileTimes32(
  HANDLE32 FileHandle, LPSYSTEMTIME lpSystemTime)
{
  FIXME(imagehlp, "(0x%08x, %p): stub\n",
    FileHandle, lpSystemTime
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}



