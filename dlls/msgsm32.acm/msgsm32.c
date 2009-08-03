/*
 * GSM 06.10 codec handling
 * Copyright (C) 2009 Maarten Lankhorst
 *
 * Based on msg711.acm
 * Copyright (C) 2002 Eric Pouech
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
#include <wine/port.h>

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#ifdef HAVE_GSM_H
#include <gsm.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "mmsystem.h"
#include "mmreg.h"
#include "msacm.h"
#include "msacmdrv.h"
#include "wine/library.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gsm);

#ifdef SONAME_LIBGSM

static void *libgsm_handle;
#define FUNCPTR(f) static typeof(f) * p##f
FUNCPTR(gsm_create);
FUNCPTR(gsm_destroy);
FUNCPTR(gsm_option);
FUNCPTR(gsm_encode);
FUNCPTR(gsm_decode);

#define LOAD_FUNCPTR(f) \
    if((p##f = wine_dlsym(libgsm_handle, #f, NULL, 0)) == NULL) { \
        wine_dlclose(libgsm_handle, NULL, 0); \
        libgsm_handle = NULL; \
        return 0; \
    }

/***********************************************************************
 *           GSM_drvLoad
 */
static LRESULT GSM_drvLoad(void)
{
    char error[128];

    libgsm_handle = wine_dlopen(SONAME_LIBGSM, RTLD_NOW, error, sizeof(error));
    if (libgsm_handle)
    {
        LOAD_FUNCPTR(gsm_create);
        LOAD_FUNCPTR(gsm_destroy);
        LOAD_FUNCPTR(gsm_option);
        LOAD_FUNCPTR(gsm_encode);
        LOAD_FUNCPTR(gsm_decode);
        return 1;
    }
    else
    {
        ERR("Couldn't load " SONAME_LIBGSM ": %s\n", error);
        return 0;
    }
}

/***********************************************************************
 *           GSM_drvFree
 */
static LRESULT GSM_drvFree(void)
{
    if (libgsm_handle)
        wine_dlclose(libgsm_handle, NULL, 0);
    return 1;
}

#else

static LRESULT GSM_drvLoad(void)
{
    return 1;
}

static LRESULT GSM_drvFree(void)
{
    return 1;
}

#endif

/***********************************************************************
 *           GSM_DriverDetails
 *
 */
static	LRESULT GSM_DriverDetails(PACMDRIVERDETAILSW add)
{
    add->fccType = ACMDRIVERDETAILS_FCCTYPE_AUDIOCODEC;
    add->fccComp = ACMDRIVERDETAILS_FCCCOMP_UNDEFINED;
    /* Details found from probing native msgsm32.acm */
    add->wMid = MM_MICROSOFT;
    add->wPid = 36;
    add->vdwACM = 0x3320000;
    add->vdwDriver = 0x4000000;
    add->fdwSupport = ACMDRIVERDETAILS_SUPPORTF_CODEC;
    add->cFormatTags = 2;
    add->cFilterTags = 0;
    add->hicon = NULL;
    MultiByteToWideChar( CP_ACP, 0, "Wine GSM 6.10", -1,
                         add->szShortName, sizeof(add->szShortName)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Wine GSM 6.10 libgsm codec", -1,
                         add->szLongName, sizeof(add->szLongName)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Brought to you by the Wine team...", -1,
                         add->szCopyright, sizeof(add->szCopyright)/sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, "Refer to LICENSE file", -1,
                         add->szLicensing, sizeof(add->szLicensing)/sizeof(WCHAR) );
    add->szFeatures[0] = 0;
    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 			GSM_DriverProc			[exported]
 */
LRESULT CALLBACK GSM_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
					 LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08lx %p %04x %08lx %08lx);\n",
          dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch (wMsg)
    {
    case DRV_LOAD:		return GSM_drvLoad();
    case DRV_FREE:		return GSM_drvFree();
    case DRV_OPEN:		return 1;
    case DRV_CLOSE:		return 1;
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "GSM 06.10 codec", "Wine Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;

    case ACMDM_DRIVER_NOTIFY:
	/* no caching from other ACM drivers is done so far */
	return MMSYSERR_NOERROR;

    case ACMDM_DRIVER_DETAILS:
	return GSM_DriverDetails((PACMDRIVERDETAILSW)dwParam1);

    /* Not implemented yet */
    case ACMDM_FORMATTAG_DETAILS:
    case ACMDM_FORMAT_DETAILS:
    case ACMDM_FORMAT_SUGGEST:
    case ACMDM_STREAM_OPEN:
    case ACMDM_STREAM_CLOSE:
    case ACMDM_STREAM_SIZE:
    case ACMDM_STREAM_CONVERT:
        return MMSYSERR_NOTSUPPORTED;

    case ACMDM_HARDWARE_WAVE_CAPS_INPUT:
    case ACMDM_HARDWARE_WAVE_CAPS_OUTPUT:
	/* this converter is not a hardware driver */
    case ACMDM_FILTERTAG_DETAILS:
    case ACMDM_FILTER_DETAILS:
	/* this converter is not a filter */
    case ACMDM_STREAM_RESET:
	/* only needed for asynchronous driver... we aren't, so just say it */
	return MMSYSERR_NOTSUPPORTED;
    case ACMDM_STREAM_PREPARE:
    case ACMDM_STREAM_UNPREPARE:
	/* nothing special to do here... so don't do anything */
	return MMSYSERR_NOERROR;

    default:
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
}
