/*
 * Thread safe wrappers around X11 calls
 *
 * Copyright 1998 Kristian Nielsen
 */

#ifndef __WINE_TSX11DEFS_H
#define __WINE_TSX11DEFS_H

#include "winbase.h"

extern CRITICAL_SECTION *TSX11_SectionPtr;

extern int TSX11_Init(void);

#define X11_LOCK() \
    (TSX11_SectionPtr ? EnterCriticalSection(TSX11_SectionPtr) : 0)

#define X11_UNLOCK() \
    (TSX11_SectionPtr ? LeaveCriticalSection(TSX11_SectionPtr) : 0)

#endif  /* __WINE_TSX11DEFS_H */
