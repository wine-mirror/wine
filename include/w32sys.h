/*
 * W32SYS
 * 
 * Copyright (c) 1996 Anand Kumria
 */

#ifndef __WINE_W32SYS_H
#define __WINE_W32SYS_H

#include "windef.h"

typedef struct _WIN32SINFO {
    BYTE   bMajor;
    BYTE   bMinor;
    WORD   wBuildNumber;
    BOOL16 fDebug;
} WIN32SINFO, *LPWIN32SINFO;

#endif /* __WINE_W32SYS_H */
