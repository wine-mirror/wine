/*
 * Resource definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_RESOURCE_H
#define __WINE_RESOURCE_H

#include "windows.h"

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
    SYSRES_MENU_EDITMENU,
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

typedef HGLOBAL16 (CALLBACK *RESOURCEHANDLER16)(HGLOBAL16, HMODULE16, HRSRC16 );

/* FIXME: convert all NE_ functions to accept NE_MODULE pointer instead
 * of redundant hModule (which is always verified prior to calling these).
 */

extern int NE_AccessResource( HMODULE16 hModule, HRSRC16 hRsrc );
extern BOOL32 NE_FreeResource( HMODULE16 hModule, HGLOBAL16 handle );
extern HRSRC16 NE_FindResource(HMODULE16 hModule, SEGPTR typeId, SEGPTR resId);
extern DWORD NE_SizeofResource( HMODULE16 hModule, HRSRC16 hRsrc );
extern SEGPTR NE_LockResource( HMODULE16 hModule, HGLOBAL16 handle );
extern HGLOBAL16 NE_AllocResource( HMODULE16 hModule, HRSRC16 hRsrc, DWORD size );
extern HGLOBAL16 NE_LoadResource( HMODULE16 hModule,  HRSRC16 hRsrc );
extern BOOL32 NE_InitResourceHandler( HMODULE16 hModule );
extern FARPROC32 NE_SetResourceHandler( HMODULE16 hModule, SEGPTR typeId, FARPROC32 handler);

extern HGLOBAL16 SYSRES_LoadResource( SYSTEM_RESOURCE id );
extern void SYSRES_FreeResource( HGLOBAL16 handle );
extern LPCVOID SYSRES_GetResPtr( SYSTEM_RESOURCE id );

#endif /* __WINE_RESOURCE_H */
