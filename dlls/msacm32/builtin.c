/*
 *      MSACM32 library
 *
 *      Copyright 1998  Patrik Stridvall
 */

#include "windows.h"
#include "winerror.h"
#include "wintypes.h"
#include "debug.h"
#include "driver.h"
#include "heap.h"
#include "mmsystem.h"
#include "msacm.h"
#include "msacmdrv.h"

/***********************************************************************
 *           MSACM_BuiltinDrivers
 */
LONG MSACM_DummyDriverProc(
   DWORD dwDriverId, HDRVR32 hdrvr, UINT32 msg,
   LONG lParam1, LONG lParam2); 

WINE_ACMBUILTINDRIVER32 MSACM_BuiltinDrivers32[] = {
  { "MSACM.dummy", &MSACM_DummyDriverProc },
  { NULL, NULL }
};

/***********************************************************************
 *           MSACM_DummyDriverProc
 */
LONG MSACM_DummyDriverProc(
   DWORD dwDriverId, HDRVR32 hdrvr, UINT32 msg,
   LONG lParam1, LONG lParam2)
{
  switch(msg)
    {
    case DRV_LOAD:
    case DRV_ENABLE:
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_DISABLE:
    case DRV_FREE:
    case DRV_CONFIGURE:
    case DRV_QUERYCONFIGURE:
    case DRV_INSTALL:
    case DRV_REMOVE:
    case DRV_EXITSESSION:
    case DRV_EXITAPPLICATION:
    case DRV_POWER:
    case ACMDM_DRIVER_NOTIFY:
    case ACMDM_DRIVER_DETAILS:
    case ACMDM_HARDWARE_WAVE_CAPS_INPUT:
    case ACMDM_HARDWARE_WAVE_CAPS_OUTPUT:
    case ACMDM_FORMATTAG_DETAILS:
    case ACMDM_FORMAT_DETAILS:
    case ACMDM_FORMAT_SUGGEST:   
    case ACMDM_FILTERTAG_DETAILS:
    case ACMDM_FILTER_DETAILS:
    case ACMDM_STREAM_OPEN:
    case ACMDM_STREAM_CLOSE:
    case ACMDM_STREAM_SIZE:
    case ACMDM_STREAM_CONVERT:
    case ACMDM_STREAM_RESET:
    case ACMDM_STREAM_PREPARE:
    case ACMDM_STREAM_UNPREPARE:
    case ACMDM_STREAM_UPDATE:
    default:
      /* FIXME: DefDriverProc not implemented  */
#if 0
      DefDriverProc32(dwDriverId, hdrvr, msg, lParam1, lParam2);
#endif
      break;
    }
  return 0;
} 

