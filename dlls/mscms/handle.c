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

/*  A simple structure to tie together a pointer to an icc profile, an lcms
 *  color profile handle and a Windows file handle. Windows color profile 
 *  handles are built from indexes into an array of these structures. If
 *  the profile is memory based the file handle field is NULL.
 */

struct handlemap
{
    HANDLE file;
    icProfile *iccprofile;
    cmsHPROFILE cmsprofile;
};

#define CMSMAXHANDLES 0x80

static struct handlemap handlemaptable[CMSMAXHANDLES];

HPROFILE MSCMS_handle2hprofile( HANDLE file )
{
    HPROFILE profile = NULL;
    unsigned int i;

    if (!file) return NULL;

    EnterCriticalSection( &MSCMS_handle_cs );

    for (i = 0; i <= CMSMAXHANDLES; i++)
    {
        if (handlemaptable[i].file == file)
        {
            profile = (HPROFILE)(i + 1); goto out;
        }
    }

out:
    LeaveCriticalSection( &MSCMS_handle_cs );

    return profile;
}

HANDLE MSCMS_hprofile2handle( HPROFILE profile )
{
    HANDLE file;
    unsigned int i;

    EnterCriticalSection( &MSCMS_handle_cs );

    i = (unsigned int)profile - 1;
    file = handlemaptable[i].file;

    LeaveCriticalSection( &MSCMS_handle_cs );

    return file;
}

HPROFILE MSCMS_cmsprofile2hprofile( cmsHPROFILE cmsprofile )
{
    HPROFILE profile = NULL;
    unsigned int i;

    if (!cmsprofile) return NULL;

    EnterCriticalSection( &MSCMS_handle_cs );

    for (i = 0; i <= CMSMAXHANDLES; i++)
    {
        if (handlemaptable[i].cmsprofile == cmsprofile)
        {
            profile = (HPROFILE)(i + 1); goto out;
        }
    }

out:
    LeaveCriticalSection( &MSCMS_handle_cs );

    return profile;
}

cmsHPROFILE MSCMS_hprofile2cmsprofile( HPROFILE profile )
{
    cmsHPROFILE cmshprofile;
    unsigned int i;

    EnterCriticalSection( &MSCMS_handle_cs );

    i = (unsigned int)profile - 1;
    cmshprofile = handlemaptable[i].cmsprofile;

    LeaveCriticalSection( &MSCMS_handle_cs );

    return cmshprofile;
}

HPROFILE MSCMS_iccprofile2hprofile( icProfile *iccprofile )
{
    HPROFILE profile = NULL;
    unsigned int i;

    if (!iccprofile) return NULL;

    EnterCriticalSection( &MSCMS_handle_cs );

    for (i = 0; i <= CMSMAXHANDLES; i++)
    {
        if (handlemaptable[i].iccprofile == iccprofile)
        {
            profile = (HPROFILE)(i + 1); goto out;
        }
    }

out:
    LeaveCriticalSection( &MSCMS_handle_cs );

    return profile;
}

icProfile *MSCMS_hprofile2iccprofile( HPROFILE profile )
{
    icProfile *iccprofile;
    unsigned int i;

    EnterCriticalSection( &MSCMS_handle_cs );

    i = (unsigned int)profile - 1;
    iccprofile = handlemaptable[i].iccprofile;

    LeaveCriticalSection( &MSCMS_handle_cs );

    return iccprofile;
}

HPROFILE MSCMS_create_hprofile_handle( HANDLE file, icProfile *iccprofile, cmsHPROFILE cmsprofile )
{
    HPROFILE profile = NULL;
    unsigned int i;

    if (!cmsprofile || !iccprofile) return NULL;

    EnterCriticalSection( &MSCMS_handle_cs );

    for (i = 0; i <= CMSMAXHANDLES; i++)
    {
        if (handlemaptable[i].iccprofile == 0)
        {
            handlemaptable[i].file = file;
            handlemaptable[i].iccprofile = iccprofile;
            handlemaptable[i].cmsprofile = cmsprofile;

            profile = (HPROFILE)(i + 1); goto out;
        }
    }

out:
    LeaveCriticalSection( &MSCMS_handle_cs );

    return profile;
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
