/* $Id: dlls.h,v 1.2 1993/07/04 04:04:21 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */

#ifndef DLLS_H
#define DLLS_H

#include "wintypes.h"

#define MAX_NAME_LENGTH		64


#define DLL	0
#define EXE	1


struct dll_table_s
{
    char *  name;          /* DLL name */
    BYTE *  code_start;    /* 32-bit address of DLL code */
    BYTE *  data_start;    /* 32-bit address of DLL data */
    BYTE *  module_start;  /* 32-bit address of the module data */
    BYTE *  module_end;
    BOOL    used;          /* use MS provided if FALSE */
    HMODULE hModule;       /* module created for this DLL */
};

#define DECLARE_DLL(name) \
extern BYTE name##_Code_Start[]; \
extern BYTE name##_Data_Start[]; \
extern BYTE name##_Module_Start[]; \
extern BYTE name##_Module_End[];

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

#define N_BUILTINS	25

extern struct dll_table_s dll_builtin_table[];

#endif /* DLLS_H */
