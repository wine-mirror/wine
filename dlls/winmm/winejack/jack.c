/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Wine Driver for jack Sound Server
 *   http://jackit.sourceforge.net
 *
 * Copyright 2002 Chris Morgan
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "mmddk.h"
#include "jack.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jack);

#ifdef HAVE_JACK_JACK_H
static int jack = 0;

/* set this to zero or one to enable or disable tracing in here */

#ifndef SONAME_LIBJACK
#define SONAME_LIBJACK "libjack.so"
#endif

void *jackhandle = NULL;

/**************************************************************************
 * 				JACK_drvLoad			[internal]	
 */
static DWORD JACK_drvLoad(void)
{
  TRACE("JACK_drvLoad()\n");

  /* dynamically load the jack library if not already loaded */
  if(!jackhandle)
  {
    jackhandle = wine_dlopen(SONAME_LIBJACK, RTLD_NOW, NULL, 0);
    TRACE("JACK_drvLoad: SONAME_LIBJACK == %s\n", SONAME_LIBJACK);
    TRACE("JACK_drvLoad: jackhandle == 0x%x\n", jackhandle);
    if(!jackhandle)
    {
      FIXME("JACK_drvLoad: error loading the jack library %s, please install this library to use jack\n", SONAME_LIBJACK);
      jackhandle = (void*)-1;
      return 0;
    }
  }

  return 1;
}

/**************************************************************************
 * 				JACK_drvFree			[internal]	
 */
/* unload the jack library on driver free */
static DWORD JACK_drvFree(void)
{
  TRACE("JACK_drvFree()\n");

  if(jackhandle && (jackhandle != (void*)-1))
  {
    TRACE("JACK_drvFree: calling wine_dlclose() on jackhandle\n");
    wine_dlclose(jackhandle, NULL, 0);
    jackhandle = NULL;
  }

  return 1;
}

/**************************************************************************
 * 				JACK_drvOpen			[internal]	
 */
static	DWORD	JACK_drvOpen(LPSTR str)
{
  /* if we were unable to load the jack library then fail the */
  /* driver open */
  if(!jackhandle)
  {
    FIXME("JACK_drvOpen: unable to open the jack library, returning 0\n");
    return 0;
  }

  if (jack)
  {
    FIXME("JACK_drvOpen: jack != 0 (already open), returning 0\n");
    return 0;
  }
    
  /* I know, this is ugly, but who cares... */
  TRACE("JACK_drvOpen: opened jack(set jack = 1), returning 1\n");
  jack = 1;
  return 1;
}

/**************************************************************************
 * 				JACK_drvClose			[internal]	
 */
static	DWORD	JACK_drvClose(DWORD dwDevID)
{
  if (jack)
  {
    TRACE("JACK_drvClose: jack is nonzero, setting jack to 0 and returning 1\n");
    jack = 0;
    return 1;
  }

  TRACE("JACK_drvClose: jack is zero(closed), returning 0\n");
  return 0;
}
#endif /* #ifdef HAVE_JACK_JACK_H */


/**************************************************************************
 * 				DriverProc (WINEJACK.1)
 */
LONG CALLBACK	JACK_DriverProc(DWORD dwDevID, HDRVR hDriv, DWORD wMsg, 
			       DWORD dwParam1, DWORD dwParam2)
{
/* EPP     TRACE("(%08lX, %04X, %08lX, %08lX, %08lX)\n",  */
/* EPP 	  dwDevID, hDriv, wMsg, dwParam1, dwParam2); */
    
    switch(wMsg) {
#ifdef HAVE_JACK_JACK_H
    case DRV_LOAD:
        TRACE("JACK_DriverProc: DRV_LOAD\n");
	return JACK_drvLoad();
    case DRV_FREE:
        TRACE("JACK_DriverProc: DRV_FREE\n");
	return JACK_drvFree();
    case DRV_OPEN:
        TRACE("JACK_DriverProc: DRV_OPEN\n");
	return JACK_drvOpen((LPSTR)dwParam1);
    case DRV_CLOSE:
        TRACE("JACK_DriverProc: DRV_CLOSE\n");
	return JACK_drvClose(dwDevID);
    case DRV_ENABLE:
        TRACE("JACK_DriverProc: DRV_ENABLE\n");
	return 1;
    case DRV_DISABLE:
        TRACE("JACK_DriverProc: DRV_DISABLE\n");
	return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "jack audio driver!", "jack driver", MB_OK);	return 1;
    case DRV_INSTALL:		
      TRACE("JACK_DriverProc: DRV_INSTALL\n");
      return DRVCNF_RESTART;
    case DRV_REMOVE:
      TRACE("JACK_DriverProc: DRV_REMOVE\n");
      return DRVCNF_RESTART;
#endif
    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}
