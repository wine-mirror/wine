/* 
 * DPLAYX.DLL LibMain
 *
 * Copyright 1999,2000 - Peter Hunnisett 
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

  TRACE( "(%u,0x%08lx,%p) & 0x%08lx\n", hinstDLL, fdwReason, lpvReserved, DPLAYX_dwProcessesAttached );

  switch ( fdwReason ) 
  {
    case DLL_PROCESS_ATTACH:
    {

      if ( DPLAYX_dwProcessesAttached++ == 0 )
      {
        /* First instance perform construction of global processor data */ 
        return DPLAYX_ConstructData();
      } 

      break;
    }

    case DLL_PROCESS_DETACH:
    {

      if ( --DPLAYX_dwProcessesAttached == 0 )
      {
        /* Last instance performs destruction of global processor data */
        return DPLAYX_DestructData(); 
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
