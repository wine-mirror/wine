/* 
 * DPLAYX.DLL LibMain
 *
 * Copyright 1999 - Peter Hunnisett 
 *
 * contact <hunnise@nortelnetworks.com>
 */

#include "winbase.h"
#include "debugtools.h"
#include "dplayx_global.h"

DEFAULT_DEBUG_CHANNEL(dplay)

static DWORD DPLAYX_dwProcessesAttached = 0;

BOOL WINAPI DPLAYX_LibMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
  switch ( fdwReason ) 
  {
    case DLL_PROCESS_ATTACH:
    {

      if ( DPLAYX_dwProcessesAttached == 0 )
      {
        /* First instance perform construction of global processor data */ 
        DPLAYX_ConstructData();
      } 

      DPLAYX_dwProcessesAttached++;

      break;
    }

    case DLL_PROCESS_DETACH:
    {

      DPLAYX_dwProcessesAttached--;
     
      if ( DPLAYX_dwProcessesAttached == 0 )
      {
        /* Last instance perform destruction of global processor data */
        DPLAYX_DestructData(); 
      }
 
      break;
    }

    case DLL_THREAD_ATTACH: /* Do nothing */
    case DLL_THREAD_DETACH: /* Do nothing */
      break;
    default:                
      break;

  }

  return TRUE;
}
