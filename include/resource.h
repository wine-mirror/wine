/*
 * Resource definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_RESOURCE_H
#define __WINE_RESOURCE_H

#include "wintypes.h"

struct resource
{
    int id;
    int type;
    const char *name;
    const unsigned char* bytes;
    unsigned size;
};

/* Built-in resources */
typedef enum
{
    SYSRES_MENU_SYSMENU,
    SYSRES_DIALOG_MSGBOX,
    SYSRES_DIALOG_SHELL_ABOUT_MSGBOX,
    SYSRES_DIALOG_OPEN_FILE,
    SYSRES_DIALOG_SAVE_FILE,
    SYSRES_DIALOG_PRINT,
    SYSRES_DIALOG_PRINT_SETUP,
    SYSRES_DIALOG_CHOOSE_FONT,
    SYSRES_DIALOG_CHOOSE_COLOR,
    SYSRES_DIALOG_FIND_TEXT,
    SYSRES_DIALOG_REPLACE_TEXT
} SYSTEM_RESOURCE;

extern void LIBRES_RegisterResources(const struct resource* const * Res);

#if defined(__GNUC__) && (__GNUC__ == 2) && (__GNUC_MINOR__ >= 7)
#define WINE_CONSTRUCTOR  __attribute__((constructor))
#define HAVE_WINE_CONSTRUCTOR
#else
#define WINE_CONSTRUCTOR
#endif

extern int NE_AccessResource( HMODULE hModule, HRSRC hRsrc );
extern BOOL NE_FreeResource( HMODULE hModule, HGLOBAL handle );
extern HRSRC NE_FindResource( HMODULE hModule, SEGPTR typeId, SEGPTR resId );
extern DWORD NE_SizeofResource( HMODULE hModule, HRSRC hRsrc );
extern SEGPTR NE_LockResource( HMODULE hModule, HGLOBAL handle );
extern HGLOBAL NE_AllocResource( HMODULE hModule, HRSRC hRsrc, DWORD size );
extern HGLOBAL NE_LoadResource( HMODULE hModule,  HRSRC hRsrc );

extern HANDLE SYSRES_LoadResource( SYSTEM_RESOURCE id );
extern void SYSRES_FreeResource( HANDLE handle );

#endif /* __WINE_RESOURCE_H */
