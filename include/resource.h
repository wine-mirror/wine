/*
 * Resource definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_RESOURCE_H
#define __WINE_RESOURCE_H

#include "wintypes.h"

extern int NE_AccessResource( HMODULE hModule, HRSRC hRsrc );
extern BOOL NE_FreeResource( HMODULE hModule, HGLOBAL handle );
extern HRSRC NE_FindResource( HMODULE hModule, SEGPTR typeId, SEGPTR resId );
extern DWORD NE_SizeofResource( HMODULE hModule, HRSRC hRsrc );
extern SEGPTR NE_LockResource( HMODULE hModule, HGLOBAL handle );
extern HGLOBAL NE_AllocResource( HMODULE hModule, HRSRC hRsrc, DWORD size );
extern HGLOBAL NE_LoadResource( HMODULE hModule,  HRSRC hRsrc );

struct ResourceTable
{
        int id,type;
        char *name;
        unsigned char* value;
        unsigned size;
};

#endif /* __WINE_RESOURCE_H */
