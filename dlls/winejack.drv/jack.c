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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
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

#ifdef SONAME_LIBJACK

void *jackhandle = NULL;

/**************************************************************************
 * 				JACK_drvLoad			[internal]	
 */
static LRESULT JACK_drvLoad(void)
{
  TRACE("()\n");

  /* dynamically load the jack library if not already loaded */
  jackhandle = wine_dlopen(SONAME_LIBJACK, RTLD_NOW, NULL, 0);
  TRACE("SONAME_LIBJACK == %s\n", SONAME_LIBJACK);
  TRACE("jackhandle == %p\n", jackhandle);
  if(!jackhandle)
  {
    FIXME("error loading the jack library %s, please install this library to use jack\n",
          SONAME_LIBJACK);
    return 0;
  }

  return 1;
}

/**************************************************************************
 * 				JACK_drvFree			[internal]	
 */
/* unload the jack library on driver free */
static LRESULT JACK_drvFree(void)
{
  TRACE("()\n");

  if(jackhandle)
  {
    TRACE("calling wine_dlclose() on jackhandle\n");
    wine_dlclose(jackhandle, NULL, 0);
    jackhandle = NULL;
  }

  return 1;
}

#endif /* #ifdef SONAME_LIBJACK */


/**************************************************************************
 * 				DriverProc (WINEJACK.1)
 */
LRESULT CALLBACK JACK_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg, 
                                 LPARAM dwParam1, LPARAM dwParam2)
{
     TRACE("(%08lX, %p, %s (%08X), %08lX, %08lX)\n",
           dwDevID, hDriv, wMsg == DRV_LOAD ? "DRV_LOAD" :
           wMsg == DRV_FREE ? "DRV_FREE" :
           wMsg == DRV_OPEN ? "DRV_OPEN" :
           wMsg == DRV_CLOSE ? "DRV_CLOSE" :
           wMsg == DRV_ENABLE ? "DRV_ENABLE" :
           wMsg == DRV_DISABLE ? "DRV_DISABLE" :
           wMsg == DRV_QUERYCONFIGURE ? "DRV_QUERYCONFIGURE" :
           wMsg == DRV_CONFIGURE ? "DRV_CONFIGURE" :
           wMsg == DRV_INSTALL ? "DRV_INSTALL" :
           wMsg == DRV_REMOVE ? "DRV_REMOVE" : "UNKNOWN", 
           wMsg, dwParam1, dwParam2);
    
    switch(wMsg) {
#ifdef SONAME_LIBJACK
    case DRV_LOAD:		return JACK_drvLoad();
    case DRV_FREE:		return JACK_drvFree();
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_INSTALL:
    case DRV_REMOVE:
    case DRV_ENABLE:
    case DRV_DISABLE:
    case DRV_QUERYCONFIGURE:
        return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "jack audio driver!", "jack driver", MB_OK);	return 1;
#endif
    default:
	return 0;
    }
}
