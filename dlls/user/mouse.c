/*
 * MOUSE driver
 * 
 * Copyright 1998 Ulrich Weigand
 * 
 */

#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wine/winbase16.h"

#include "pshpack1.h"
typedef struct _MOUSEINFO
{
    BYTE msExist;
    BYTE msRelative;
    WORD msNumButtons;
    WORD msRate;
    WORD msXThreshold;
    WORD msYThreshold;
    WORD msXRes;
    WORD msYRes;
    WORD msMouseCommPort;
} MOUSEINFO, *LPMOUSEINFO;
#include "poppack.h"

static FARPROC16 DefMouseEventProc;

/***********************************************************************
 *           Inquire                       (MOUSE.1)
 */
WORD WINAPI MOUSE_Inquire(LPMOUSEINFO mouseInfo)
{
    mouseInfo->msExist = TRUE;
    mouseInfo->msRelative = FALSE;
    mouseInfo->msNumButtons = 2;
    mouseInfo->msRate = 34;  /* the DDK says so ... */
    mouseInfo->msXThreshold = 0;
    mouseInfo->msYThreshold = 0;
    mouseInfo->msXRes = 0;
    mouseInfo->msYRes = 0;
    mouseInfo->msMouseCommPort = 0;

    return sizeof(MOUSEINFO);
}

/***********************************************************************
 *           Enable                        (MOUSE.2)
 */
VOID WINAPI MOUSE_Enable( FARPROC16 proc )
{
    DefMouseEventProc = proc;
}

/***********************************************************************
 *           Disable                       (MOUSE.3)
 */
VOID WINAPI MOUSE_Disable(VOID)
{
    DefMouseEventProc = 0;
}
