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

struct resource
{
        int id,type;
        char *name;
        unsigned char* bytes;
        unsigned size;
};

#if defined(__GNUC__) && (__GNUC__ == 2) && (__GNUC_MINOR__ >= 7)
#define WINE_CONSTRUCTOR  __attribute__((constructor))
#define HAVE_WINE_CONSTRUCTOR
#else
#define WINE_CONSTRUCTOR
#endif

#endif /* __WINE_RESOURCE_H */
