/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 */

#include "windows.h"
#include "winerror.h"
#include "wintypes.h"
#include "debug.h"
#include "mmsystem.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "winversion.h"

/**********************************************************************/

static DWORD MSACM_dwProcessesAttached32 = 0;

/***********************************************************************
 *           MSACM_LibMain32 (MSACM32.init) 
 */
BOOL32 WINAPI MSACM32_LibMain(
  HINSTANCE32 hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
      if(MSACM_dwProcessesAttached32 == 0)
	{
	  MSACM_hHeap32 = HeapCreate(0, 0x10000, 0);
	  MSACM_RegisterAllDrivers32();
	}
      MSACM_dwProcessesAttached32++;
      break;
    case DLL_PROCESS_DETACH:
      MSACM_dwProcessesAttached32--;
      if(MSACM_dwProcessesAttached32 == 0)
	{
	  MSACM_UnregisterAllDrivers32();
	  HeapDestroy(MSACM_hHeap32);
	  MSACM_hHeap32 = (HANDLE32) NULL;
	}
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
 *           XRegThunkEntry (MSACM32.1)
 * FIXME
 *   No documentation found.
 */

/***********************************************************************
 *           acmGetVersion32 (MSACM32.34)
 */
DWORD WINAPI acmGetVersion32()
{
  switch(VERSION_GetVersion()) 
    {
    default: 
      FIXME(msacm, "%s not supported\n", VERSION_GetVersionName());
    case WIN95:
      return 0x04000000; /* 4.0.0 */
    case NT40:
      return 0x04000565; /* 4.0.1381 */
    }
}

/***********************************************************************
 *           acmMessage32 (MSACM32.35)
 * FIXME
 *   No documentation found.
 */

/***********************************************************************
 *           acmMetrics (MSACM32.36)
 */
MMRESULT32 WINAPI acmMetrics32(
  HACMOBJ32 hao, UINT32 uMetric, LPVOID  pMetric)
{
  PWINE_ACMOBJ32 pao = MSACM_GetObj32(hao);
  BOOL32 bLocal = TRUE;

  FIXME(msacm, "(0x%08x, %d, %p): stub\n", hao, uMetric, pMetric);

  switch(uMetric)
    {
    case ACM_METRIC_COUNT_DRIVERS:
      bLocal = FALSE;
    case ACM_METRIC_COUNT_LOCAL_DRIVERS:
      if(!pao)
	return MMSYSERR_INVALHANDLE;  
      return MMSYSERR_NOTSUPPORTED;
    case ACM_METRIC_COUNT_CODECS:
      bLocal = FALSE;
    case ACM_METRIC_COUNT_LOCAL_CODECS:
      return MMSYSERR_NOTSUPPORTED;
    case ACM_METRIC_COUNT_CONVERTERS:
      bLocal = FALSE;
    case ACM_METRIC_COUNT_LOCAL_CONVERTERS:
      return MMSYSERR_NOTSUPPORTED;
    case ACM_METRIC_COUNT_FILTERS:
      bLocal = FALSE;
    case ACM_METRIC_COUNT_LOCAL_FILTERS:
      return MMSYSERR_NOTSUPPORTED;
    case ACM_METRIC_COUNT_DISABLED:
      bLocal = FALSE;
    case ACM_METRIC_COUNT_LOCAL_DISABLED:
      if(!pao)
	return MMSYSERR_INVALHANDLE;  
      return MMSYSERR_NOTSUPPORTED;
    case ACM_METRIC_COUNT_HARDWARE:
    case ACM_METRIC_HARDWARE_WAVE_INPUT:
    case ACM_METRIC_HARDWARE_WAVE_OUTPUT:
    case ACM_METRIC_MAX_SIZE_FORMAT:
    case ACM_METRIC_MAX_SIZE_FILTER:
    case ACM_METRIC_DRIVER_SUPPORT:
    case ACM_METRIC_DRIVER_PRIORITY:
    default:
      return MMSYSERR_NOTSUPPORTED;
    }
  return MMSYSERR_NOERROR;
}
