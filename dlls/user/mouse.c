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

typedef VOID CALLBACK (*LPMOUSE_EVENT_PROC)(DWORD,DWORD,DWORD,DWORD,DWORD);

static LPMOUSE_EVENT_PROC DefMouseEventProc = NULL;

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

/**********************************************************************/

static VOID WINAPI MOUSE_CallMouseEventProc( FARPROC16 proc,
                                             DWORD dwFlags, DWORD dx, DWORD dy,
                                             DWORD cButtons, DWORD dwExtraInfo )
{
    CONTEXT86 context;

    memset( &context, 0, sizeof(context) );
    context.SegCs = SELECTOROF( proc );
    context.Eip   = OFFSETOF( proc );
    context.Eax   = (WORD)dwFlags;
    context.Ebx   = (WORD)dx;
    context.Ecx   = (WORD)dy;
    context.Edx   = (WORD)cButtons;
    context.Esi   = LOWORD( dwExtraInfo );
    context.Edi   = HIWORD( dwExtraInfo );

    wine_call_to_16_regs_short( &context, 0 );
}

/***********************************************************************
 *           Enable                        (MOUSE.2)
 */
VOID WINAPI MOUSE_Enable( FARPROC16 proc )
{
    THUNK_Free( (FARPROC)DefMouseEventProc );
    DefMouseEventProc = (LPMOUSE_EVENT_PROC)THUNK_Alloc( proc, (RELAY)MOUSE_CallMouseEventProc );
}

/***********************************************************************
 *           Disable                       (MOUSE.3)
 */
VOID WINAPI MOUSE_Disable(VOID)
{
    THUNK_Free( (FARPROC)DefMouseEventProc );
    DefMouseEventProc = 0;
}
