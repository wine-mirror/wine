/******************************************************************************
 * Print Spooler Functions
 *
 *
 * Copyright 1999 Thuy Nguyen
 */

#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(winspool);


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
      break;

    case DLL_PROCESS_DETACH:
      break;
  }

  return TRUE;
}


