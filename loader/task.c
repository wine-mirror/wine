/*
 * Task functions
 *
 * Copyright 1995 Alexandre Julliard
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "winbase.h"
#include "wingdi.h"
#include "winnt.h"
#include "winuser.h"

#include "wine/winbase16.h"
#include "drive.h"
#include "file.h"
#include "global.h"
#include "instance.h"
#include "module.h"
#include "winternl.h"
#include "selectors.h"
#include "wine/server.h"
#include "stackframe.h"
#include "task.h"
#include "thread.h"
#include "toolhelp.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(task);

static THHOOK DefaultThhook;
THHOOK *pThhook = &DefaultThhook;

#define hFirstTask   (pThhook->HeadTDB)

/***********************************************************************
 *	     TASK_GetPtr
 */
static TDB *TASK_GetPtr( HTASK16 hTask )
{
    return GlobalLock16( hTask );
}


/***********************************************************************
 *	     TASK_GetCurrent
 */
TDB *TASK_GetCurrent(void)
{
    return TASK_GetPtr( GetCurrentTask() );
}


/***********************************************************************
 *           GetCurrentTask   (KERNEL32.@)
 */
HTASK16 WINAPI GetCurrentTask(void)
{
    return NtCurrentTeb()->htask16;
}

/***********************************************************************
 *           GetCurrentPDB   (KERNEL.37)
 *
 * UNDOC: returns PSP of KERNEL in high word
 */
DWORD WINAPI GetCurrentPDB16(void)
{
    TDB *pTask;

    if (!(pTask = TASK_GetCurrent())) return 0;
    return MAKELONG(pTask->hPDB, 0); /* FIXME */
}


/***********************************************************************
 *           GetExePtrHelper
 */
static inline HMODULE16 GetExePtrHelper( HANDLE16 handle, HTASK16 *hTask )
{
    char *ptr;
    HANDLE16 owner;

      /* Check for module handle */

    if (!(ptr = GlobalLock16( handle ))) return 0;
    if (((NE_MODULE *)ptr)->magic == IMAGE_OS2_SIGNATURE) return handle;

      /* Search for this handle inside all tasks */

    *hTask = hFirstTask;
    while (*hTask)
    {
        TDB *pTask = TASK_GetPtr( *hTask );
        if ((*hTask == handle) ||
            (pTask->hInstance == handle) ||
            (pTask->hQueue == handle) ||
            (pTask->hPDB == handle)) return pTask->hModule;
        *hTask = pTask->hNext;
    }

      /* Check the owner for module handle */

    owner = FarGetOwner16( handle );
    if (!(ptr = GlobalLock16( owner ))) return 0;
    if (((NE_MODULE *)ptr)->magic == IMAGE_OS2_SIGNATURE) return owner;

      /* Search for the owner inside all tasks */

    *hTask = hFirstTask;
    while (*hTask)
    {
        TDB *pTask = TASK_GetPtr( *hTask );
        if ((*hTask == owner) ||
            (pTask->hInstance == owner) ||
            (pTask->hQueue == owner) ||
            (pTask->hPDB == owner)) return pTask->hModule;
        *hTask = pTask->hNext;
    }

    return 0;
}

/***********************************************************************
 *           GetExePtr   (KERNEL.133)
 */
HMODULE16 WINAPI WIN16_GetExePtr( HANDLE16 handle )
{
    HTASK16 hTask = 0;
    HMODULE16 hModule = GetExePtrHelper( handle, &hTask );
    STACK16FRAME *frame = CURRENT_STACK16;
    frame->ecx = hModule;
    if (hTask) frame->es = hTask;
    return hModule;
}


/***********************************************************************
 *           K228   (KERNEL.228)
 */
HMODULE16 WINAPI GetExePtr( HANDLE16 handle )
{
    HTASK16 hTask = 0;
    return GetExePtrHelper( handle, &hTask );
}
