/******************************************************************************
 * Print Spooler Functions
 *
 *
 * Copyright 1999 Thuy Nguyen
 */

#include "commctrl.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(winspool)

HINSTANCE hcomctl32 = 0;
HDPA   (WINAPI* WINSPOOL_DPA_CreateEx)(INT, HANDLE);
LPVOID (WINAPI* WINSPOOL_DPA_GetPtr)(const HDPA, INT);
INT    (WINAPI* WINSPOOL_DPA_InsertPtr)(const HDPA, INT, LPVOID);

/******************************************************************************
 *  WINSPOOL_EntryPoint
 *
 * Winspool entry point.
 *
 */
BOOL WINAPI WINSPOOL_EntryPoint(HINSTANCE hInstance,
                                DWORD reason,
                                LPVOID lpReserved)
{
  switch (reason)
  {
    case DLL_PROCESS_ATTACH:

      hcomctl32 = LoadLibraryA("COMCTL32.DLL");

      WINSPOOL_DPA_CreateEx = (void*)GetProcAddress(hcomctl32, (LPCSTR)340L);
      WINSPOOL_DPA_GetPtr = (void*)GetProcAddress(hcomctl32, (LPCSTR)332L);
      WINSPOOL_DPA_InsertPtr = (void*)GetProcAddress(hcomctl32, (LPCSTR)334L);
      break;

    case DLL_PROCESS_DETACH:
      FreeLibrary(hcomctl32);
      break;
  }

  return TRUE;
}


