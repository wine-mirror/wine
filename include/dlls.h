/*
 * Copyright  Robert J. Amstadt, 1993
 */

#ifndef __WINE_DLLS_H
#define __WINE_DLLS_H

#include "wintypes.h"


typedef struct dll_table_s
{
    char       *name;          /* DLL name */
    const BYTE *code_start;    /* 32-bit address of DLL code */
    const BYTE *data_start;    /* 32-bit address of DLL data */
    BYTE       *module_start;  /* 32-bit address of the module data */
    BYTE       *module_end;
    int         flags;         /* flags (see below) */
    HMODULE     hModule;       /* module created for this DLL */
} BUILTIN_DLL;

/* DLL flags */
#define DLL_FLAG_NOT_USED    1  /* Use original Windows DLL if possible */
#define DLL_FLAG_WIN32       2  /* DLL is a Win32 DLL */

#define DECLARE_DLL(name) \
extern const BYTE name##_Code_Start[]; \
extern const BYTE name##_Data_Start[]; \
extern BYTE name##_Module_Start[]; \
extern BYTE name##_Module_End[];

/* 16-bit DLLs */
DECLARE_DLL(KERNEL)
DECLARE_DLL(USER)
DECLARE_DLL(GDI)
DECLARE_DLL(WIN87EM)
DECLARE_DLL(MMSYSTEM)
DECLARE_DLL(SHELL)
DECLARE_DLL(SOUND)
DECLARE_DLL(KEYBOARD)
DECLARE_DLL(WINSOCK)
DECLARE_DLL(STRESS)
DECLARE_DLL(SYSTEM)
DECLARE_DLL(TOOLHELP)
DECLARE_DLL(MOUSE)
DECLARE_DLL(COMMDLG)
DECLARE_DLL(OLE2)
DECLARE_DLL(OLE2CONV)
DECLARE_DLL(OLE2DISP)
DECLARE_DLL(OLE2NLS)
DECLARE_DLL(OLE2PROX)
DECLARE_DLL(OLECLI)
DECLARE_DLL(OLESVR)
DECLARE_DLL(COMPOBJ)
DECLARE_DLL(STORAGE)
DECLARE_DLL(WINPROCS)
DECLARE_DLL(DDEML)
DECLARE_DLL(LZEXPAND)

/* 32-bit DLLs */

DECLARE_DLL(ADVAPI32)
DECLARE_DLL(COMCTL32)
DECLARE_DLL(COMDLG32)
DECLARE_DLL(OLE32)
DECLARE_DLL(GDI32)
DECLARE_DLL(KERNEL32)
DECLARE_DLL(SHELL32)
DECLARE_DLL(USER32)
DECLARE_DLL(WINPROCS32)
DECLARE_DLL(WINSPOOL)

extern BUILTIN_DLL dll_builtin_table[];

#endif /* __WINE_DLLS_H */
