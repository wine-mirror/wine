/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2004, 2005 Hans Leidekker
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
#include "wine/debug.h"
#include "wine/library.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

#define LCMS_API_NO_REDEFINE
#define LCMS_API_FUNCTION(f) typeof(f) * p##f;
#include "lcms_api.h"
#undef LCMS_API_FUNCTION

WINE_DEFAULT_DEBUG_CHANNEL(mscms);

#ifdef HAVE_LCMS_H
#ifndef SONAME_LIBLCMS
#define SONAME_LIBLCMS "liblcms.so"
#endif

static void *lcmshandle = NULL;
#endif /* HAVE_LCMS_H */

static BOOL MSCMS_init_lcms(void)
{
#ifdef HAVE_LCMS_H
    /* dynamically load lcms if not yet loaded */
    if (!lcmshandle)
    {
        lcmshandle = wine_dlopen( SONAME_LIBLCMS, RTLD_NOW, NULL, 0 );

        /* We can't really do anything useful without liblcms */
        if (!lcmshandle)
        {
            WINE_MESSAGE(
            "Wine cannot find the LittleCMS library. To enable Wine to use color\n"
            "management functions please install a version of LittleCMS greater\n"
            "than or equal to 1.13.\n"
            "http://www.littlecms.com\n" );

            return FALSE;
        }
    }

#define LOAD_FUNCPTR(f) \
            if ((p##f = wine_dlsym( lcmshandle, #f, NULL, 0 )) == NULL) \
                goto sym_not_found;

    LOAD_FUNCPTR(cmsCloseProfile);
    LOAD_FUNCPTR(cmsCreate_sRGBProfile);
    LOAD_FUNCPTR(cmsCreateMultiprofileTransform);
    LOAD_FUNCPTR(cmsCreateTransform);
    LOAD_FUNCPTR(cmsDeleteTransform);
    LOAD_FUNCPTR(cmsDoTransform);
    LOAD_FUNCPTR(cmsOpenProfileFromMem);
    #undef LOAD_FUNCPTR

return TRUE;

sym_not_found:
    WINE_MESSAGE(
      "Wine cannot find certain functions that it needs inside the LittleCMS\n"
      "library. To enable Wine to use LittleCMS for color management please\n"
      "upgrade liblcms to at least version 1.13.\n"
      "http://www.littlecms.com\n" );

    wine_dlclose( lcmshandle, NULL, 0 );
    lcmshandle = NULL;

    return FALSE;

#endif /* HAVE_LCMS_H */
    WINE_MESSAGE(
      "This version of Wine was compiled without support for color management\n"
      "functions. To enable Wine to use LittleCMS for color management please\n"
      "install a liblcms development package version 1.13 or higher and rebuild\n"
      "Wine.\n"
      "http://www.littlecms.com\n" );

    return FALSE;
}

static void MSCMS_deinit_lcms(void)
{
#ifdef HAVE_LCMS_H
    if (lcmshandle)
    {
        wine_dlclose( lcmshandle, NULL, 0 );
        lcmshandle = NULL;
    }
#endif /* HAVE_LCMS_H */
}

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE( "(%p, 0x%08lx, %p)\n", hinst, reason, reserved );

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        return MSCMS_init_lcms();

    case DLL_PROCESS_DETACH:
        MSCMS_deinit_lcms();
        break;
    }
    return TRUE;
}
