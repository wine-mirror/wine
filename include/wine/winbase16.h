/*
 * Copyright (C) the Wine project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINE_WINBASE16_H
#define __WINE_WINE_WINBASE16_H

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/windef16.h"
#include "wine/library.h"

#include "pshpack1.h"
typedef struct _SEGINFO {
    UINT16    offSegment;
    UINT16    cbSegment;
    UINT16    flags;
    UINT16    cbAlloc;
    HGLOBAL16 h;
    UINT16    alignShift;
    UINT16    reserved[2];
} SEGINFO;

/* GetWinFlags */

#define WF_PMODE 	0x0001
#define WF_CPU286 	0x0002
#define	WF_CPU386	0x0004
#define	WF_CPU486 	0x0008
#define	WF_STANDARD	0x0010
#define	WF_WIN286 	0x0010
#define	WF_ENHANCED	0x0020
#define	WF_WIN386	0x0020
#define	WF_CPU086	0x0040
#define	WF_CPU186	0x0080
#define	WF_LARGEFRAME	0x0100
#define	WF_SMALLFRAME	0x0200
#define	WF_80x87	0x0400
#define	WF_PAGING	0x0800
#define	WF_HASCPUID     0x2000
#define	WF_WIN32WOW     0x4000	/* undoc */
#define	WF_WLO          0x8000

/* Parameters for LoadModule() */
typedef struct
{
    HGLOBAL16 hEnvironment;         /* Environment segment */
    SEGPTR    cmdLine WINE_PACKED;  /* Command-line */
    SEGPTR    showCmd WINE_PACKED;  /* Code for ShowWindow() */
    SEGPTR    reserved WINE_PACKED;
} LOADPARAMS16;

/* Debugging support (DEBUG SYSTEM ONLY) */
typedef struct
{
    WORD    flags;
    DWORD   dwOptions WINE_PACKED;
    DWORD   dwFilter WINE_PACKED;
    CHAR    achAllocModule[8] WINE_PACKED;
    DWORD   dwAllocBreak WINE_PACKED;
    DWORD   dwAllocCount WINE_PACKED;
} WINDEBUGINFO16, *LPWINDEBUGINFO16;

#include "poppack.h"

#define INVALID_HANDLE_VALUE16  ((HANDLE16) -1)
#define INFINITE16      0xFFFF

typedef struct {
        DWORD dwOSVersionInfoSize;
        DWORD dwMajorVersion;
        DWORD dwMinorVersion;
        DWORD dwBuildNumber;
        DWORD dwPlatformId;
        CHAR szCSDVersion[128];
} OSVERSIONINFO16;


/*
 * NE Header FORMAT FLAGS
 */
#define NE_FFLAGS_SINGLEDATA    0x0001
#define NE_FFLAGS_MULTIPLEDATA  0x0002
#define NE_FFLAGS_WIN32         0x0010
#define NE_FFLAGS_BUILTIN       0x0020  /* Wine built-in module */
#define NE_FFLAGS_FRAMEBUF      0x0100  /* OS/2 fullscreen app */
#define NE_FFLAGS_CONSOLE       0x0200  /* OS/2 console app */
#define NE_FFLAGS_GUI           0x0300  /* right, (NE_FFLAGS_FRAMEBUF | NE_FFLAGS_CONSOLE) */
#define NE_FFLAGS_SELFLOAD      0x0800
#define NE_FFLAGS_LINKERROR     0x2000
#define NE_FFLAGS_CALLWEP       0x4000
#define NE_FFLAGS_LIBMODULE     0x8000

/*
 * NE Header OPERATING SYSTEM
 */
#define NE_OSFLAGS_UNKNOWN      0x01
#define NE_OSFLAGS_WINDOWS      0x04

/*
 * NE Header ADDITIONAL FLAGS
 */
#define NE_AFLAGS_WIN2_PROTMODE 0x02
#define NE_AFLAGS_WIN2_PROFONTS 0x04
#define NE_AFLAGS_FASTLOAD      0x08

/*
 * Segment Flags
 */
#define NE_SEGFLAGS_DATA        0x0001
#define NE_SEGFLAGS_ALLOCATED   0x0002
#define NE_SEGFLAGS_LOADED      0x0004
#define NE_SEGFLAGS_ITERATED    0x0008
#define NE_SEGFLAGS_MOVEABLE    0x0010
#define NE_SEGFLAGS_SHAREABLE   0x0020
#define NE_SEGFLAGS_PRELOAD     0x0040
#define NE_SEGFLAGS_EXECUTEONLY 0x0080
#define NE_SEGFLAGS_READONLY    0x0080
#define NE_SEGFLAGS_RELOC_DATA  0x0100
#define NE_SEGFLAGS_SELFLOAD    0x0800
#define NE_SEGFLAGS_DISCARDABLE 0x1000
#define NE_SEGFLAGS_32BIT       0x2000

/*
 * Resource table structures.
 */
typedef struct
{
    WORD     offset;
    WORD     length;
    WORD     flags;
    WORD     id;
    HANDLE16 handle;
    WORD     usage;
} NE_NAMEINFO;

typedef struct
{
    WORD        type_id;   /* Type identifier */
    WORD        count;     /* Number of resources of this type */
    FARPROC16   resloader; /* SetResourceHandler() */
    /*
     * Name info array.
     */
} NE_TYPEINFO;

#define NE_RSCTYPE_CURSOR         0x8001
#define NE_RSCTYPE_BITMAP         0x8002
#define NE_RSCTYPE_ICON           0x8003
#define NE_RSCTYPE_MENU           0x8004
#define NE_RSCTYPE_DIALOG         0x8005
#define NE_RSCTYPE_STRING         0x8006
#define NE_RSCTYPE_FONTDIR        0x8007
#define NE_RSCTYPE_FONT           0x8008
#define NE_RSCTYPE_ACCELERATOR    0x8009
#define NE_RSCTYPE_RCDATA         0x800a
#define NE_RSCTYPE_GROUP_CURSOR   0x800c
#define NE_RSCTYPE_GROUP_ICON     0x800e


#define __AHSHIFT  3  /* don't change! */
#define __AHINCR   (1 << __AHSHIFT)

/* undocumented functions */
WORD        WINAPI AllocCStoDSAlias16(WORD);
WORD        WINAPI AllocDStoCSAlias16(WORD);
HGLOBAL16   WINAPI AllocResource16(HINSTANCE16,HRSRC16,DWORD);
WORD        WINAPI AllocSelector16(WORD);
WORD        WINAPI AllocSelectorArray16(WORD);
VOID        WINAPI DirectedYield16(HTASK16);
HGLOBAL16   WINAPI DirectResAlloc16(HINSTANCE16,WORD,UINT16);
HANDLE16    WINAPI FarGetOwner16(HGLOBAL16);
VOID        WINAPI FarSetOwner16(HGLOBAL16,HANDLE16);
FARPROC16   WINAPI FileCDR16(FARPROC16);
WORD        WINAPI FreeSelector16(WORD);
HANDLE16    WINAPI GetAtomHandle16(ATOM);
HANDLE16    WINAPI GetCodeHandle16(FARPROC16);
BOOL16      WINAPI GetCodeInfo16(FARPROC16,SEGINFO*);
DWORD       WINAPI GetCurrentPDB16(void);
HTASK16     WINAPI GetCurrentTask(void);
SEGPTR      WINAPI GetDOSEnvironment16(void);
HMODULE16   WINAPI GetExePtr(HANDLE16);
WORD        WINAPI GetExeVersion16(void);
WORD        WINAPI GetExpWinVer16(HMODULE16);
HANDLE      WINAPI GetFastQueue16(void);
DWORD       WINAPI GetHeapSpaces16(HMODULE16);
INT16       WINAPI GetInstanceData16(HINSTANCE16,WORD,INT16);
BOOL16      WINAPI GetModuleName16(HINSTANCE16,LPSTR,INT16);
INT16       WINAPI GetModuleUsage16(HINSTANCE16);
UINT16      WINAPI GetNumTasks16(void);
SEGPTR      WINAPI GetpWin16Lock16(void);
DWORD       WINAPI GetSelectorLimit16(WORD);
FARPROC16   WINAPI GetSetKernelDOSProc16(FARPROC16 DosProc);
HINSTANCE16 WINAPI GetTaskDS16(void);
HQUEUE16    WINAPI GetTaskQueue16(HTASK16);
HQUEUE16    WINAPI GetThreadQueue16(DWORD);
DWORD       WINAPI GetWinFlags16(void);
DWORD       WINAPI GlobalDOSAlloc16(DWORD);
WORD        WINAPI GlobalDOSFree16(WORD);
void        WINAPI GlobalFreeAll16(HGLOBAL16);
DWORD       WINAPI GlobalHandleNoRIP16(WORD);
WORD        WINAPI GlobalHandleToSel16(HGLOBAL16);
HGLOBAL16   WINAPI GlobalLRUNewest16(HGLOBAL16);
HGLOBAL16   WINAPI GlobalLRUOldest16(HGLOBAL16);
VOID        WINAPI GlobalNotify16(FARPROC16);
WORD        WINAPI GlobalPageLock16(HGLOBAL16);
WORD        WINAPI GlobalPageUnlock16(HGLOBAL16);
BOOL16      WINAPI IsSharedSelector16(HANDLE16);
BOOL16      WINAPI IsTask16(HTASK16);
HTASK16     WINAPI IsTaskLocked16(void);
SEGPTR      WINAPI K32WOWGlobalLock16(HGLOBAL16);
VOID        WINAPI LogError16(UINT16, LPVOID);
VOID        WINAPI LogParamError16(UINT16,FARPROC16,LPVOID);
WORD        WINAPI LocalCountFree16(void);
WORD        WINAPI LocalHandleDelta16(WORD);
WORD        WINAPI LocalHeapSize16(void);
BOOL16      WINAPI LocalInit16(HANDLE16,WORD,WORD);
FARPROC16   WINAPI LocalNotify16(FARPROC16);
HTASK16     WINAPI LockCurrentTask16(BOOL16);
VOID        WINAPI OldYield16(void);
VOID        WINAPI WIN32_OldYield16(void);
VOID        WINAPI PostEvent16(HTASK16);
WORD        WINAPI PrestoChangoSelector16(WORD,WORD);
WORD        WINAPI SelectorAccessRights16(WORD,WORD,WORD);
void        WINAPI SetFastQueue16(DWORD,HANDLE);
VOID        WINAPI SetPriority16(HTASK16,INT16);
FARPROC16   WINAPI SetResourceHandler16(HINSTANCE16,LPCSTR,FARPROC16);
WORD        WINAPI SetSelectorLimit16(WORD,DWORD);
HQUEUE16    WINAPI SetTaskQueue16(HTASK16,HQUEUE16);
HQUEUE16    WINAPI SetThreadQueue16(DWORD,HQUEUE16);
VOID        WINAPI SwitchStackTo16(WORD,WORD,WORD);
BOOL16      WINAPI WaitEvent16(HTASK16);
VOID        WINAPI WriteOutProfiles16(void);
VOID        WINAPI hmemcpy16(LPVOID,LPCVOID,LONG);
VOID        WINAPI _CreateSysLevel(SYSLEVEL*,INT);
VOID        WINAPI _EnterWin16Lock(void);
VOID        WINAPI _LeaveWin16Lock(void);


INT16       WINAPI AccessResource16(HINSTANCE16,HRSRC16);
ATOM        WINAPI AddAtom16(LPCSTR);
UINT16      WINAPI CompareString16(DWORD,DWORD,LPCSTR,DWORD,LPCSTR,DWORD);
BOOL16      WINAPI CreateDirectory16(LPCSTR,LPVOID);
BOOL16      WINAPI DefineHandleTable16(WORD);
ATOM        WINAPI DeleteAtom16(ATOM);
BOOL16      WINAPI DeleteFile16(LPCSTR);
void        WINAPI ExitKernel16(void);
void        WINAPI FatalAppExit16(UINT16,LPCSTR);
ATOM        WINAPI FindAtom16(LPCSTR);
BOOL16      WINAPI FindClose16(HANDLE16);
VOID        WINAPI FreeLibrary16(HINSTANCE16);
HANDLE16    WINAPI FindFirstFile16(LPCSTR,LPWIN32_FIND_DATAA);
BOOL16      WINAPI FindNextFile16(HANDLE16,LPWIN32_FIND_DATAA);
HRSRC16     WINAPI FindResource16(HINSTANCE16,LPCSTR,LPCSTR);
BOOL16      WINAPI FreeModule16(HMODULE16);
void        WINAPI FreeProcInstance16(FARPROC16);
BOOL16      WINAPI FreeResource16(HGLOBAL16);
UINT16      WINAPI GetAtomName16(ATOM,LPSTR,INT16);
UINT16      WINAPI GetCurrentDirectory16(UINT16,LPSTR);
BOOL16      WINAPI GetDiskFreeSpace16(LPCSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
UINT16      WINAPI GetDriveType16(UINT16); /* yes, the arguments differ */
INT16       WINAPI GetLocaleInfo16(LCID,LCTYPE,LPSTR,INT16);
DWORD       WINAPI GetFileAttributes16(LPCSTR);
DWORD       WINAPI GetFreeSpace16(UINT16);
INT16       WINAPI GetModuleFileName16(HINSTANCE16,LPSTR,INT16);
HMODULE16   WINAPI GetModuleHandle16(LPCSTR);
UINT16      WINAPI GetPrivateProfileInt16(LPCSTR,LPCSTR,INT16,LPCSTR);
INT16       WINAPI GetPrivateProfileSection16(LPCSTR,LPSTR,UINT16,LPCSTR);
WORD        WINAPI GetPrivateProfileSectionNames16(LPSTR,UINT16,LPCSTR);
INT16       WINAPI GetPrivateProfileString16(LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT16,LPCSTR);
BOOL16      WINAPI GetPrivateProfileStruct16(LPCSTR,LPCSTR,LPVOID,UINT16,LPCSTR);
FARPROC16   WINAPI GetProcAddress16(HMODULE16,LPCSTR);
UINT16      WINAPI GetProfileInt16(LPCSTR,LPCSTR,INT16);
INT16       WINAPI GetProfileSection16(LPCSTR,LPSTR,UINT16);
WORD        WINAPI GetProfileSectionNames16(LPSTR,WORD);
INT16       WINAPI GetProfileString16(LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT16);
DWORD       WINAPI GetSelectorBase(WORD);
BOOL16      WINAPI GetStringType16(LCID,DWORD,LPCSTR,INT16,LPWORD);
UINT16      WINAPI GetSystemDirectory16(LPSTR,UINT16);
UINT16      WINAPI GetTempFileName16(BYTE,LPCSTR,UINT16,LPSTR);
LONG        WINAPI GetVersion16(void);
BOOL16      WINAPI GetVersionEx16(OSVERSIONINFO16*);
BOOL16      WINAPI GetWinDebugInfo16(LPWINDEBUGINFO16,UINT16);
UINT16      WINAPI GetWindowsDirectory16(LPSTR,UINT16);
HGLOBAL16   WINAPI GlobalAlloc16(UINT16,DWORD);
DWORD       WINAPI GlobalCompact16(DWORD);
LPVOID      WINAPI GlobalLock16(HGLOBAL16);
WORD        WINAPI GlobalFix16(HGLOBAL16);
UINT16      WINAPI GlobalFlags16(HGLOBAL16);
HGLOBAL16   WINAPI GlobalFree16(HGLOBAL16);
DWORD       WINAPI GlobalHandle16(WORD);
HGLOBAL16   WINAPI GlobalReAlloc16(HGLOBAL16,DWORD,UINT16);
DWORD       WINAPI GlobalSize16(HGLOBAL16);
VOID        WINAPI GlobalUnfix16(HGLOBAL16);
BOOL16      WINAPI GlobalUnlock16(HGLOBAL16);
BOOL16      WINAPI GlobalUnWire16(HGLOBAL16);
SEGPTR      WINAPI GlobalWire16(HGLOBAL16);
WORD        WINAPI InitAtomTable16(WORD);
BOOL16      WINAPI IsBadCodePtr16(SEGPTR);
BOOL16      WINAPI IsBadHugeReadPtr16(SEGPTR,DWORD);
BOOL16      WINAPI IsBadHugeWritePtr16(SEGPTR,DWORD);
BOOL16      WINAPI IsBadReadPtr16(SEGPTR,UINT16);
BOOL16      WINAPI IsBadStringPtr16(SEGPTR,UINT16);
BOOL16      WINAPI IsBadWritePtr16(SEGPTR,UINT16);
BOOL16      WINAPI IsDBCSLeadByte16(BYTE);
HINSTANCE16 WINAPI LoadLibrary16(LPCSTR);
HINSTANCE16 WINAPI LoadModule16(LPCSTR,LPVOID);
HGLOBAL16   WINAPI LoadResource16(HINSTANCE16,HRSRC16);
HLOCAL16    WINAPI LocalAlloc16(UINT16,WORD);
UINT16      WINAPI LocalCompact16(UINT16);
UINT16      WINAPI LocalFlags16(HLOCAL16);
HLOCAL16    WINAPI LocalFree16(HLOCAL16);
HLOCAL16    WINAPI LocalHandle16(WORD);
SEGPTR      WINAPI LocalLock16(HLOCAL16);
HLOCAL16    WINAPI LocalReAlloc16(HLOCAL16,WORD,UINT16);
UINT16      WINAPI LocalShrink16(HGLOBAL16,UINT16);
UINT16      WINAPI LocalSize16(HLOCAL16);
BOOL16      WINAPI LocalUnlock16(HLOCAL16);
LPVOID      WINAPI LockResource16(HGLOBAL16);
HGLOBAL16   WINAPI LockSegment16(HGLOBAL16);
FARPROC16   WINAPI MakeProcInstance16(FARPROC16,HANDLE16);
HFILE16     WINAPI OpenFile16(LPCSTR,OFSTRUCT*,UINT16);
DWORD       WINAPI RegCloseKey16(HKEY);
DWORD       WINAPI RegCreateKey16(HKEY,LPCSTR,PHKEY);
DWORD       WINAPI RegDeleteKey16(HKEY,LPCSTR);
DWORD       WINAPI RegDeleteValue16(HKEY,LPSTR);
DWORD       WINAPI RegEnumKey16(HKEY,DWORD,LPSTR,DWORD);
DWORD       WINAPI RegEnumValue16(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD       WINAPI RegOpenKey16(HKEY,LPCSTR,PHKEY);
DWORD       WINAPI RegQueryValue16(HKEY,LPCSTR,LPSTR,LPDWORD);
DWORD       WINAPI RegQueryValueEx16(HKEY,LPCSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD       WINAPI RegSetValue16(HKEY,LPCSTR,DWORD,LPCSTR,DWORD);
DWORD       WINAPI RegSetValueEx16(HKEY,LPCSTR,DWORD,DWORD,CONST BYTE*,DWORD);
BOOL16      WINAPI RemoveDirectory16(LPCSTR);
BOOL16      WINAPI SetCurrentDirectory16(LPCSTR);
UINT16      WINAPI SetErrorMode16(UINT16);
BOOL16      WINAPI SetFileAttributes16(LPCSTR,DWORD);
UINT16      WINAPI SetHandleCount16(UINT16);
WORD        WINAPI SetSelectorBase(WORD,DWORD);
LONG        WINAPI SetSwapAreaSize16(WORD);
BOOL16      WINAPI SetWinDebugInfo16(LPWINDEBUGINFO16);
DWORD       WINAPI SizeofResource16(HMODULE16,HRSRC16);
void        WINAPI UnlockSegment16(HGLOBAL16);
BOOL16      WINAPI WritePrivateProfileString16(LPCSTR,LPCSTR,LPCSTR,LPCSTR);
BOOL16      WINAPI WriteProfileString16(LPCSTR,LPCSTR,LPCSTR);
/* Yield16 will only be available from kernel module, use K32WOWYield instead */
VOID        WINAPI Yield16(void);
VOID        WINAPI K32WOWYield16(void);
SEGPTR      WINAPI lstrcat16(SEGPTR,LPCSTR);
SEGPTR      WINAPI lstrcatn16(SEGPTR,LPCSTR,INT16);
SEGPTR      WINAPI lstrcpy16(SEGPTR,LPCSTR);
SEGPTR      WINAPI lstrcpyn16(SEGPTR,LPCSTR,INT16);
INT16       WINAPI lstrlen16(LPCSTR);
HINSTANCE16 WINAPI WinExec16(LPCSTR,UINT16);
LONG        WINAPI _hread16(HFILE16,LPVOID,LONG);
LONG        WINAPI _hwrite16(HFILE16,LPCSTR,LONG);
HFILE16     WINAPI _lcreat16(LPCSTR,INT16);
HFILE16     WINAPI _lclose16(HFILE16);
LONG        WINAPI _llseek16(HFILE16,LONG,INT16);
HFILE16     WINAPI _lopen16(LPCSTR,INT16);
UINT16      WINAPI _lread16(HFILE16,LPVOID,UINT16);
UINT16      WINAPI _lwrite16(HFILE16,LPCSTR,UINT16);
BOOL16      WINAPI WritePrivateProfileSection16(LPCSTR,LPCSTR,LPCSTR);
BOOL16      WINAPI WritePrivateProfileStruct16(LPCSTR,LPCSTR,LPVOID,UINT16,LPCSTR);
BOOL16      WINAPI WriteProfileSection16(LPCSTR,LPCSTR);

/* Wine-specific functions */
WORD        WINAPI wine_call_to_16_word( FARPROC16 target, INT nArgs );
LONG        WINAPI wine_call_to_16_long( FARPROC16 target, INT nArgs );
void        WINAPI wine_call_to_16_regs_short( CONTEXT86 *context, INT nArgs );
void        WINAPI wine_call_to_16_regs_long ( CONTEXT86 *context, INT nArgs );

/* Some optimizations */
extern inline LPVOID WINAPI MapSL( SEGPTR segptr )
{
    return (char *)wine_ldt_copy.base[SELECTOROF(segptr) >> __AHSHIFT] + OFFSETOF(segptr);
}

#endif /* __WINE_WINE_WINBASE16_H */
