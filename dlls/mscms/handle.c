/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2004 Hans Leidekker
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
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

#include "mscms_priv.h"

#ifdef HAVE_LCMS_H

static CRITICAL_SECTION MSCMS_handle_cs;
static CRITICAL_SECTION_DEBUG MSCMS_handle_cs_debug =
{
    0, 0, &MSCMS_handle_cs,
    { &MSCMS_handle_cs_debug.ProcessLocksList,
      &MSCMS_handle_cs_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": MSCMS_handle_cs") }
};
static CRITICAL_SECTION MSCMS_handle_cs = { &MSCMS_handle_cs_debug, -1, 0, 0, 0, 0 };

/*  A simple structure to tie together Windows file handles and lcms color
 *  profile handles. Windows color profile handles are built from indexes
 *  into an array of these structures. The 'file' field is set to NULL in
 *  case of a memory based profile
 */

struct handlemap
{
    HANDLE file;
    cmsHPROFILE cmsprofile;
};

#define CMSMAXHANDLES 0x80

static struct handlemap handlemaptable[CMSMAXHANDLES];

HPROFILE MSCMS_cmsprofile2hprofile( cmsHPROFILE cmsprofile )
{
    HPROFILE ret = NULL;
    unsigned int i;

    if (!cmsprofile) return ret;

    EnterCriticalSection( &MSCMS_handle_cs );

    for (i = 0; i <= CMSMAXHANDLES; i++)
    {
        if (handlemaptable[i].cmsprofile == cmsprofile)
        {
            ret = (HPROFILE)(i + 1); goto out;
        }
    }

out:
    LeaveCriticalSection( &MSCMS_handle_cs );

    return ret;
}

cmsHPROFILE MSCMS_hprofile2cmsprofile( HPROFILE profile )
{
    HANDLE ret;
    unsigned int i;

    EnterCriticalSection( &MSCMS_handle_cs );

    i = (unsigned int)profile - 1;
    ret = handlemaptable[i].cmsprofile;

    LeaveCriticalSection( &MSCMS_handle_cs );

    return ret;
}

HANDLE MSCMS_hprofile2handle( HPROFILE profile )
{
    HANDLE ret;
    unsigned int i;

    EnterCriticalSection( &MSCMS_handle_cs );

    i = (unsigned int)profile - 1;
    ret = handlemaptable[i].file;

    LeaveCriticalSection( &MSCMS_handle_cs );

    return ret;
}

HPROFILE MSCMS_create_hprofile_handle( HANDLE file, cmsHPROFILE cmsprofile )
{
    HPROFILE ret = NULL;
    unsigned int i;

    if (!cmsprofile) return ret;

    EnterCriticalSection( &MSCMS_handle_cs );

    for (i = 0; i <= CMSMAXHANDLES; i++)
    {
        if (handlemaptable[i].cmsprofile == 0)
        {
            handlemaptable[i].file = file;
            handlemaptable[i].cmsprofile = cmsprofile;

            ret = (HPROFILE)(i + 1); goto out;
        }
    }

out:
    LeaveCriticalSection( &MSCMS_handle_cs );

    return ret;
}

void MSCMS_destroy_hprofile_handle( HPROFILE profile )
{
    unsigned int i;

    if (profile)
    {
        EnterCriticalSection( &MSCMS_handle_cs );

        i = (unsigned int)profile - 1;
        memset( &handlemaptable[i], 0, sizeof(struct handlemap) );

        LeaveCriticalSection( &MSCMS_handle_cs );
    }
}

#endif /* HAVE_LCMS_H */
