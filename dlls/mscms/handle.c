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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

#include "mscms_priv.h"

#ifdef HAVE_LCMS

static CRITICAL_SECTION MSCMS_handle_cs;
static CRITICAL_SECTION_DEBUG MSCMS_handle_cs_debug =
{
    0, 0, &MSCMS_handle_cs,
    { &MSCMS_handle_cs_debug.ProcessLocksList,
      &MSCMS_handle_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": MSCMS_handle_cs") }
};
static CRITICAL_SECTION MSCMS_handle_cs = { &MSCMS_handle_cs_debug, -1, 0, 0, 0, 0 };

/*  A simple structure to tie together a pointer to an icc profile, an lcms
 *  color profile handle and a Windows file handle. Windows color profile 
 *  handles are built from indexes into an array of these structures. If
 *  the profile is memory based the file handle field is NULL. The 'access'
 *  field records the access parameter supplied to an OpenColorProfile()
 *  call, i.e. PROFILE_READ or PROFILE_READWRITE.
 */

struct profile
{
    HANDLE file;
    DWORD access;
    icProfile *iccprofile;
    cmsHPROFILE cmsprofile;
};

struct transform
{
    cmsHTRANSFORM cmstransform;
};

#define CMSMAXHANDLES 0x80

static struct profile profiletable[CMSMAXHANDLES];
static struct transform transformtable[CMSMAXHANDLES];

HPROFILE MSCMS_handle2hprofile( HANDLE file )
{
    HPROFILE profile = NULL;
    DWORD_PTR i;

    if (!file) return NULL;

    EnterCriticalSection( &MSCMS_handle_cs );

    for (i = 0; i <= CMSMAXHANDLES; i++)
    {
        if (profiletable[i].file == file)
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
    DWORD_PTR i;

    EnterCriticalSection( &MSCMS_handle_cs );

    i = (DWORD_PTR)profile - 1;
    file = profiletable[i].file;

    LeaveCriticalSection( &MSCMS_handle_cs );
    return file;
}

DWORD MSCMS_hprofile2access( HPROFILE profile )
{
    DWORD access;
    DWORD_PTR i;

    EnterCriticalSection( &MSCMS_handle_cs );

    i = (DWORD_PTR)profile - 1;
    access = profiletable[i].access;

    LeaveCriticalSection( &MSCMS_handle_cs );
    return access;
}

HPROFILE MSCMS_cmsprofile2hprofile( cmsHPROFILE cmsprofile )
{
    HPROFILE profile = NULL;
    DWORD_PTR i;

    if (!cmsprofile) return NULL;

    EnterCriticalSection( &MSCMS_handle_cs );

    for (i = 0; i <= CMSMAXHANDLES; i++)
    {
        if (profiletable[i].cmsprofile == cmsprofile)
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
    cmsHPROFILE cmsprofile;
    DWORD_PTR i;

    EnterCriticalSection( &MSCMS_handle_cs );

    i = (DWORD_PTR)profile - 1;
    cmsprofile = profiletable[i].cmsprofile;

    LeaveCriticalSection( &MSCMS_handle_cs );
    return cmsprofile;
}

HPROFILE MSCMS_iccprofile2hprofile( const icProfile *iccprofile )
{
    HPROFILE profile = NULL;
    DWORD_PTR i;

    if (!iccprofile) return NULL;

    EnterCriticalSection( &MSCMS_handle_cs );

    for (i = 0; i <= CMSMAXHANDLES; i++)
    {
        if (profiletable[i].iccprofile == iccprofile)
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
    DWORD_PTR i;

    EnterCriticalSection( &MSCMS_handle_cs );

    i = (DWORD_PTR)profile - 1;
    iccprofile = profiletable[i].iccprofile;

    LeaveCriticalSection( &MSCMS_handle_cs );
    return iccprofile;
}

HPROFILE MSCMS_create_hprofile_handle( HANDLE file, icProfile *iccprofile,
                                       cmsHPROFILE cmsprofile, DWORD access )
{
    HPROFILE profile = NULL;
    DWORD_PTR i;

    if (!cmsprofile || !iccprofile) return NULL;

    EnterCriticalSection( &MSCMS_handle_cs );

    for (i = 0; i <= CMSMAXHANDLES; i++)
    {
        if (profiletable[i].iccprofile == 0)
        {
            profiletable[i].file = file;
            profiletable[i].access = access;
            profiletable[i].iccprofile = iccprofile;
            profiletable[i].cmsprofile = cmsprofile;

            profile = (HPROFILE)(i + 1); goto out;
        }
    }

out:
    LeaveCriticalSection( &MSCMS_handle_cs );
    return profile;
}

void MSCMS_destroy_hprofile_handle( HPROFILE profile )
{
    DWORD_PTR i;

    if (profile)
    {
        EnterCriticalSection( &MSCMS_handle_cs );

        i = (DWORD_PTR)profile - 1;
        memset( &profiletable[i], 0, sizeof(struct profile) );

        LeaveCriticalSection( &MSCMS_handle_cs );
    }
}

cmsHTRANSFORM MSCMS_htransform2cmstransform( HTRANSFORM transform )
{
    cmsHTRANSFORM cmstransform;
    DWORD_PTR i;

    EnterCriticalSection( &MSCMS_handle_cs );

    i = (DWORD_PTR)transform - 1;
    cmstransform = transformtable[i].cmstransform;

    LeaveCriticalSection( &MSCMS_handle_cs );
    return cmstransform;
}

HTRANSFORM MSCMS_create_htransform_handle( cmsHTRANSFORM cmstransform )
{
    HTRANSFORM transform = NULL;
    DWORD_PTR i;

    if (!cmstransform) return NULL;

    EnterCriticalSection( &MSCMS_handle_cs );

    for (i = 0; i <= CMSMAXHANDLES; i++)
    {
        if (transformtable[i].cmstransform == 0)
        {
            transformtable[i].cmstransform = cmstransform;
            transform = (HTRANSFORM)(i + 1); goto out;
        }
    }

out:
    LeaveCriticalSection( &MSCMS_handle_cs );
    return transform;
}

void MSCMS_destroy_htransform_handle( HTRANSFORM transform )
{
    DWORD_PTR i;

    if (transform)
    {
        EnterCriticalSection( &MSCMS_handle_cs );

        i = (DWORD_PTR)transform - 1;
        memset( &transformtable[i], 0, sizeof(struct transform) );

        LeaveCriticalSection( &MSCMS_handle_cs );
    }
}

#endif /* HAVE_LCMS */
