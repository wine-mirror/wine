/*
 * MOUSE driver
 * 
 * Copyright 1998 Ulrich Weigand
 * 
 */

#include <string.h>

#include "debugtools.h"
#include "callback.h"
#include "builtin16.h"
#include "module.h"
#include "mouse.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winbase16.h"

DEFAULT_DEBUG_CHANNEL(event);

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

/**********************************************************************/

static LPMOUSE_EVENT_PROC DefMouseEventProc = NULL;

/***********************************************************************
 *           MOUSE_Inquire                       (MOUSE.1)
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
 *           MOUSE_Enable                        (MOUSE.2)
 */
VOID WINAPI MOUSE_Enable(LPMOUSE_EVENT_PROC lpMouseEventProc)
{
    THUNK_Free( (FARPROC)DefMouseEventProc );
    DefMouseEventProc = lpMouseEventProc;
    USER_Driver->pInitMouse( lpMouseEventProc );
}

static VOID WINAPI MOUSE_CallMouseEventProc( FARPROC16 proc,
                                             DWORD dwFlags, DWORD dx, DWORD dy,
                                             DWORD cButtons, DWORD dwExtraInfo )
{
    CONTEXT86 context;

    memset( &context, 0, sizeof(context) );
    CS_reg(&context)  = SELECTOROF( proc );
    EIP_reg(&context) = OFFSETOF( proc );
    AX_reg(&context)  = (WORD)dwFlags;
    BX_reg(&context)  = (WORD)dx;
    CX_reg(&context)  = (WORD)dy;
    DX_reg(&context)  = (WORD)cButtons;
    SI_reg(&context)  = LOWORD( dwExtraInfo );
    DI_reg(&context)  = HIWORD( dwExtraInfo );

    CallTo16RegisterShort( &context, 0 );
}

VOID WINAPI WIN16_MOUSE_Enable( FARPROC16 proc )
{
    LPMOUSE_EVENT_PROC thunk = 
      (LPMOUSE_EVENT_PROC)THUNK_Alloc( proc, (RELAY)MOUSE_CallMouseEventProc );

    MOUSE_Enable( thunk );
}

/***********************************************************************
 *           MOUSE_Disable                       (MOUSE.3)
 */
VOID WINAPI MOUSE_Disable(VOID)
{
    THUNK_Free( (FARPROC)DefMouseEventProc );
    DefMouseEventProc = 0;
    USER_Driver->pInitMouse( 0 );
}
