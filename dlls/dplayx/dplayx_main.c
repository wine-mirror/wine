/* 
 * DPLAYX.DLL LibMain
 *
 * Copyright 1999,2000 - Peter Hunnisett 
 *
 * contact <hunnise@nortelnetworks.com>
 */
#include "winerror.h"
#include "winbase.h"
#include "debugtools.h"
#include "dplayx_global.h"

DEFAULT_DEBUG_CHANNEL(dplay);

/* This is a globally exported variable at ordinal 6 of DPLAYX.DLL */
DWORD gdwDPlaySPRefCount = 0; /* FIXME: Should it be initialized here? */


BOOL WINAPI DPLAYX_LibMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{

  TRACE( "(%u,0x%08lx,%p)\n", hinstDLL, fdwReason, lpvReserved );

  switch ( fdwReason ) 
  {
    case DLL_PROCESS_ATTACH:
        /* First instance perform construction of global processor data */ 
        return DPLAYX_ConstructData();

    case DLL_PROCESS_DETACH:
        /* Last instance performs destruction of global processor data */
        return DPLAYX_DestructData(); 

    case DLL_THREAD_ATTACH: /* Do nothing */
    case DLL_THREAD_DETACH: /* Do nothing */
      break;
    default:                
      break;

  }

  return TRUE;
}

/***********************************************************************
 *              DllCanUnloadNow (DPLAYX.@)
 */
HRESULT WINAPI DPLAYX_DllCanUnloadNow(void)
{
  HRESULT hr = ( gdwDPlaySPRefCount > 0 ) ? S_FALSE : S_OK;

  /* FIXME: Should I be putting a check in for class factory objects 
   *        as well
   */

  TRACE( ": returning 0x%08lx\n", hr );
 
  return hr;
}

