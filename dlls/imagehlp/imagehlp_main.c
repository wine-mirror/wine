/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "imagehlp.h"
#include "winerror.h"
#include "winbase.h"
#include "windef.h"
#include "debug.h"

/**********************************************************************/

HANDLE IMAGEHLP_hHeap = (HANDLE) NULL;

static API_VERSION IMAGEHLP_ApiVersion = { 4, 0, 0, 5 };

/***********************************************************************
 *           IMAGEHLP_LibMain (IMAGEHLP.init)
 */
BOOL WINAPI IMAGEHLP_LibMain(
  HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
      IMAGEHLP_hHeap = HeapCreate(0, 0x10000, 0);
      break;
    case DLL_PROCESS_DETACH:
      HeapDestroy(IMAGEHLP_hHeap);
      IMAGEHLP_hHeap = (HANDLE) NULL;
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
PAPI_VERSION WINAPI ImagehlpApiVersion()
{
  return &IMAGEHLP_ApiVersion;
}

/***********************************************************************
 *           ImagehlpApiVersionEx32 (IMAGEHLP.23)
 */
PAPI_VERSION WINAPI ImagehlpApiVersionEx(PAPI_VERSION AppVersion)
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
BOOL WINAPI MakeSureDirectoryPathExists(LPCSTR DirPath)
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
BOOL WINAPI SearchTreeForFile(
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
BOOL WINAPI TouchFileTimes(
  HANDLE FileHandle, LPSYSTEMTIME lpSystemTime)
{
  FIXME(imagehlp, "(0x%08x, %p): stub\n",
    FileHandle, lpSystemTime
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}



