/*
 * Thread safe wrappers around XShm calls.
 *
 * Copyright 1998 Kristian Nielsen
 */

#include "tsx11defs.h"
#include "stddebug.h"
#include "debug.h"

CRITICAL_SECTION *TSX11_SectionPtr = NULL;
static CRITICAL_SECTION TSX11_Section;

int TSX11_Init(void)
{
    InitializeCriticalSection( &TSX11_Section );
    dprintf_x11(stddeb, "TSX11_Init: X11 critical section is %p\n",
                &TSX11_Section);
    TSX11_SectionPtr = &TSX11_Section;
    return TRUE;
}
