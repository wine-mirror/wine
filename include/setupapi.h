/*
 * Copyright (C) 2000 James Hatheway
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

#ifndef __SETUPAPI__
#define __SETUPAPI__

#include "commctrl.h"

/* Define type for handle to a loaded inf file */
typedef PVOID HINF;

/* Define type for handle to a device information set */
typedef PVOID HDEVINFO;

/* Define type for setup file queue */
typedef PVOID HSPFILEQ;

/* inf structure. */
typedef struct _INFCONTEXT
{
   PVOID Inf;
   PVOID CurrentInf;
   UINT  Section;
   UINT  Line;
} INFCONTEXT, *PINFCONTEXT;

typedef UINT (CALLBACK *PSP_FILE_CALLBACK_A)( PVOID Context, UINT Notification,
                                              UINT Param1, UINT Param2 );
typedef UINT (CALLBACK *PSP_FILE_CALLBACK_W)( PVOID Context, UINT Notification,
                                              UINT Param1, UINT Param2 );
#define PSP_FILE_CALLBACK WINELIB_NAME_AW(PSP_FILE_CALLBACK_)


/* Device Information structure (references a device instance that is a member
   of a device information set) */
typedef struct _SP_DEVINFO_DATA
{
   DWORD cbSize;
   GUID  ClassGuid;
   DWORD DevInst;   /* DEVINST handle */
   DWORD Reserved;
} SP_DEVINFO_DATA, *PSP_DEVINFO_DATA;

#endif /* __SETUPAPI__ */
