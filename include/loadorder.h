/*
 * Module/Library loadorder
 *
 * Copyright 1999 Bertho Stultiens
 */

#ifndef __WINE_LOADORDER_H
#define __WINE_LOADORDER_H

#include "windef.h"

enum loadorder_type
{
    LOADORDER_INVALID = 0, /* Must be 0 */
    LOADORDER_DLL,         /* Native DLLs */
    LOADORDER_SO,          /* Native .so libraries */
    LOADORDER_BI,          /* Built-in modules */
    LOADORDER_NTYPES
};

extern void MODULE_InitLoadOrder(void);
extern void MODULE_GetLoadOrder( enum loadorder_type plo[], const char *path, BOOL win32 );
extern void MODULE_AddLoadOrderOption( const char *option );

#endif

