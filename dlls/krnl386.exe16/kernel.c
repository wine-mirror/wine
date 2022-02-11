/*
 * 16-bit kernel initialization code
 *
 * Copyright 2000 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>

#define WINE_NO_INLINE_STRING
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wownt32.h"
#include "wine/winuser16.h"

#include "kernel16_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);

extern DWORD WINAPI GetProcessFlags( DWORD processid );

void *dummy = RaiseException;  /* force importing it from kernel32 */

static DWORD process_dword;

/***********************************************************************
 *           KERNEL thread initialisation routine
 */
static void thread_attach(void)
{
    /* allocate the 16-bit stack (FIXME: should be done lazily) */
    HGLOBAL16 hstack = WOWGlobalAlloc16( GMEM_FIXED, 0x10000 );
    CURRENT_SS = kernel_get_thread_data()->stack_sel = GlobalHandleToSel16( hstack );
    CURRENT_SP = 0x10000 - sizeof(STACK16FRAME);
    memset( (char *)GlobalLock16(hstack) + 0x10000 - sizeof(STACK16FRAME), 0, sizeof(STACK16FRAME) );
}


/***********************************************************************
 *           KERNEL thread finalisation routine
 */
static void thread_detach(void)
{
    /* free the 16-bit stack */
    WOWGlobalFree16( kernel_get_thread_data()->stack_sel );
    CURRENT_SS = CURRENT_SP = 0;
    if (NtCurrentTeb()->Tib.SubSystemTib) TASK_ExitTask();
}


/**************************************************************************
 *		DllMain
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        init_selectors();
        if (LoadLibrary16( "krnl386.exe" ) < 32) return FALSE;
        /* fall through */
    case DLL_THREAD_ATTACH:
        thread_attach();
        break;
    case DLL_THREAD_DETACH:
        thread_detach();
        break;
    }
    return TRUE;
}


/**************************************************************************
 *		DllEntryPoint   (KERNEL.669)
 */
BOOL WINAPI KERNEL_DllEntryPoint( DWORD reasion, HINSTANCE16 inst, WORD ds,
                                  WORD heap, DWORD reserved1, WORD reserved2 )
{
    static BOOL done;

    /* the entry point can be called multiple times */
    if (done) return TRUE;
    done = TRUE;

    /* create the shared heap for broken win95 native dlls */
    HeapCreate( HEAP_SHARED, 0, 0 );

    /* setup emulation of protected instructions from 32-bit code */
    if (GetVersion() & 0x80000000) RtlAddVectoredExceptionHandler( TRUE, INSTR_vectored_handler );

    /* Initialize 16-bit thunking entry points */
    if (!WOWTHUNK_Init()) return FALSE;

    /* Initialize DOS memory */
    if (!DOSMEM_Init()) return FALSE;

    /* Initialize special KERNEL entry points */

    NE_SetEntryPoint( inst, 178, GetWinFlags16() );

    NE_SetEntryPoint( inst, 454, get_cs() );
    NE_SetEntryPoint( inst, 455, get_ds() );

    NE_SetEntryPoint( inst, 183, DOSMEM_0000H );       /* KERNEL.183: __0000H */
    NE_SetEntryPoint( inst, 173, DOSMEM_BiosSysSeg );  /* KERNEL.173: __ROMBIOS */
    NE_SetEntryPoint( inst, 193, DOSMEM_BiosDataSeg ); /* KERNEL.193: __0040H */
    NE_SetEntryPoint( inst, 194, DOSMEM_BiosSysSeg );  /* KERNEL.194: __F000H */

    /* Initialize KERNEL.THHOOK */
    TASK_InstallTHHook(MapSL((SEGPTR)GetProcAddress16( inst, (LPCSTR)332 )));
    TASK_CreateMainTask();

    /* Initialize the real-mode selector entry points */
#define SET_ENTRY_POINT( num, addr ) \
    NE_SetEntryPoint( inst, (num), GLOBAL_CreateBlock( GMEM_FIXED, \
                      DOSMEM_MapDosToLinear(addr), 0x10000, inst, \
                      LDT_FLAGS_DATA ))

    SET_ENTRY_POINT( 174, 0xa0000 );  /* KERNEL.174: __A000H */
    SET_ENTRY_POINT( 181, 0xb0000 );  /* KERNEL.181: __B000H */
    SET_ENTRY_POINT( 182, 0xb8000 );  /* KERNEL.182: __B800H */
    SET_ENTRY_POINT( 195, 0xc0000 );  /* KERNEL.195: __C000H */
    SET_ENTRY_POINT( 179, 0xd0000 );  /* KERNEL.179: __D000H */
    SET_ENTRY_POINT( 190, 0xe0000 );  /* KERNEL.190: __E000H */
#undef SET_ENTRY_POINT

    /* Force loading of some dlls */
    LoadLibrary16( "system.drv" );
    LoadLibrary16( "comm.drv" );

    return TRUE;
}

/***********************************************************************
 *         GetVersion   (KERNEL.3)
 */
DWORD WINAPI GetVersion16(void)
{
    static WORD dosver, winver;

    if (!dosver)  /* not determined yet */
    {
        RTL_OSVERSIONINFOEXW info;

        info.dwOSVersionInfoSize = sizeof(info);
        if (RtlGetVersion( &info )) return 0;

        if (info.dwMajorVersion <= 3)
            winver = MAKEWORD( info.dwMajorVersion, info.dwMinorVersion );
        else
            winver = MAKEWORD( 3, 95 );

        switch(info.dwPlatformId)
        {
        case VER_PLATFORM_WIN32s:
            switch(MAKELONG( info.dwMinorVersion, info.dwMajorVersion ))
            {
            case 0x0200:
                dosver = 0x0303;  /* DOS 3.3 for Windows 2.0 */
                break;
            case 0x0300:
                dosver = 0x0500;  /* DOS 5.0 for Windows 3.0 */
                break;
            default:
                dosver = 0x0616;  /* DOS 6.22 for Windows 3.1 and later */
                break;
            }
            break;
        case VER_PLATFORM_WIN32_WINDOWS:
            /* DOS 8.0 for WinME, 7.0 for Win95/98 */
            if (info.dwMinorVersion >= 90) dosver = 0x0800;
            else dosver = 0x0700;
            break;
        case VER_PLATFORM_WIN32_NT:
            dosver = 0x0500;  /* always DOS 5.0 for NT */
            break;
        }
        TRACE( "DOS %d.%02d Win %d.%02d\n",
               HIBYTE(dosver), LOBYTE(dosver), LOBYTE(winver), HIBYTE(winver) );
    }
    return MAKELONG( winver, dosver );
}

/***********************************************************************
 *		Reserved1 (KERNEL.77)
 */
SEGPTR WINAPI KERNEL_AnsiNext16(SEGPTR current)
{
    return (*(char *)MapSL(current)) ? current + 1 : current;
}

/***********************************************************************
 *		Reserved2(KERNEL.78)
 */
SEGPTR WINAPI KERNEL_AnsiPrev16( SEGPTR start, SEGPTR current )
{
    return (current==start)?start:current-1;
}

/***********************************************************************
 *		Reserved3 (KERNEL.79)
 */
SEGPTR WINAPI KERNEL_AnsiUpper16( SEGPTR strOrChar )
{
    /* uppercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        char *s = MapSL(strOrChar);
        while (*s)
        {
            *s = toupper(*s);
            s++;
        }
        return strOrChar;
    }
    else return toupper((char)strOrChar);
}

/***********************************************************************
 *		Reserved4 (KERNEL.80)
 */
SEGPTR WINAPI KERNEL_AnsiLower16( SEGPTR strOrChar )
{
    /* lowercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        char *s = MapSL(strOrChar);
        while (*s)
        {
            *s = tolower(*s);
            s++;
        }
        return strOrChar;
    }
    else return tolower((char)strOrChar);
}

/***********************************************************************
 *		Reserved5 (KERNEL.87)
 */
INT16 WINAPI KERNEL_lstrcmp16( LPCSTR str1, LPCSTR str2 )
{
    int ret = strcmp( str1, str2 );

    /* Looks too complicated, but in optimized strcpy we might get
     * a 32bit wide difference and would truncate it to 16 bit, so
     * erroneously returning equality. */
    if (ret < 0) return -1;
    if (ret > 0) return  1;
    return 0;
}

/***********************************************************************
 *           lstrcpy   (KERNEL.88)
 */
SEGPTR WINAPI lstrcpy16( SEGPTR dst, LPCSTR src )
{
    if (!lstrcpyA( MapSL(dst), src )) dst = 0;
    return dst;
}

/***********************************************************************
 *           lstrcat   (KERNEL.89)
 */
SEGPTR WINAPI lstrcat16( SEGPTR dst, LPCSTR src )
{
    /* Windows does not check for NULL pointers here, so we don't either */
    strcat( MapSL(dst), src );
    return dst;
}

/***********************************************************************
 *           lstrlen   (KERNEL.90)
 */
INT16 WINAPI lstrlen16( LPCSTR str )
{
    return (INT16)lstrlenA( str );
}

/***********************************************************************
 *           OutputDebugString   (KERNEL.115)
 */
void WINAPI OutputDebugString16( LPCSTR str )
{
    OutputDebugStringA( str );
}

/***********************************************************************
 *          GetWinFlags   (KERNEL.132)
 */
DWORD WINAPI GetWinFlags16(void)
{
    static const long cpuflags[5] = { WF_CPU086, WF_CPU186, WF_CPU286, WF_CPU386, WF_CPU486 };
    SYSTEM_INFO si;
    OSVERSIONINFOA ovi;
    DWORD result;

    GetSystemInfo(&si);

    /* There doesn't seem to be any Pentium flag.  */
    result = cpuflags[min(si.wProcessorLevel, 4)] | WF_ENHANCED | WF_PMODE | WF_80x87 | WF_PAGING;
    if (si.wProcessorLevel >= 4) result |= WF_HASCPUID;
    ovi.dwOSVersionInfoSize = sizeof(ovi);
    GetVersionExA(&ovi);
    if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT)
        result |= WF_WIN32WOW; /* undocumented WF_WINNT */
    return result;
}

/***********************************************************************
 *         GetVersionEx   (KERNEL.149)
 */
BOOL16 WINAPI GetVersionEx16(OSVERSIONINFO16 *v)
{
    OSVERSIONINFOA info;

    if (v->dwOSVersionInfoSize < sizeof(OSVERSIONINFO16))
    {
        WARN("wrong OSVERSIONINFO size from app\n");
        return FALSE;
    }

    info.dwOSVersionInfoSize = sizeof(info);
    if (!GetVersionExA( &info )) return FALSE;

    v->dwMajorVersion = info.dwMajorVersion;
    v->dwMinorVersion = info.dwMinorVersion;
    v->dwBuildNumber  = info.dwBuildNumber;
    v->dwPlatformId   = info.dwPlatformId;
    strcpy( v->szCSDVersion, info.szCSDVersion );
    return TRUE;
}

/***********************************************************************
 *           DebugBreak   (KERNEL.203)
 */
void WINAPI DebugBreak16( CONTEXT *context )
{
    EXCEPTION_RECORD rec;

    rec.ExceptionCode    = EXCEPTION_BREAKPOINT;
    rec.ExceptionFlags   = 0;
    rec.ExceptionRecord  = NULL;
    rec.ExceptionAddress = (LPVOID)context->Eip;
    rec.NumberParameters = 0;
    NtRaiseException( &rec, context, TRUE );
}

/***********************************************************************
 *           K329                    (KERNEL.329)
 *
 * TODO:
 * Should fill lpBuffer only if DBO_BUFFERFILL has been set by SetWinDebugInfo()
 */
void WINAPI DebugFillBuffer(LPSTR lpBuffer, WORD wBytes)
{
    memset(lpBuffer, 0xf9 /* DBGFILL_BUFFER */, wBytes);
}

/***********************************************************************
 *           DiagQuery                          (KERNEL.339)
 *
 * returns TRUE if Win called with "/b" (bootlog.txt)
 */
BOOL16 WINAPI DiagQuery16(void)
{
    return FALSE;
}

/***********************************************************************
 *           DiagOutput                         (KERNEL.340)
 *
 * writes a debug string into <windir>\bootlog.txt
 */
void WINAPI DiagOutput16(LPCSTR str)
{
    /* FIXME */
    TRACE("DIAGOUTPUT:%s\n", debugstr_a(str));
}

/***********************************************************************
 *           hmemcpy   (KERNEL.348)
 */
void WINAPI hmemcpy16( LPVOID dst, LPCVOID src, LONG count )
{
    memcpy( dst, src, count );
}

/***********************************************************************
 *           lstrcpyn   (KERNEL.353)
 */
SEGPTR WINAPI lstrcpyn16( SEGPTR dst, LPCSTR src, INT16 n )
{
    if (!lstrcpynA( MapSL(dst), src, n )) return 0;
    return dst;
}

/***********************************************************************
 *           lstrcatn   (KERNEL.352)
 */
SEGPTR WINAPI lstrcatn16( SEGPTR dst, LPCSTR src, INT16 n )
{
    LPSTR p = MapSL(dst);
    LPSTR start = p;

    while (*p) p++;
    if ((n -= (p - start)) <= 0) return dst;
    lstrcpynA( p, src, n );
    return dst;
}

#if 0  /* Not used at this time. This is here for documentation only */

/* WINDEBUGINFO flags values */
#define WDI_OPTIONS         0x0001
#define WDI_FILTER          0x0002
#define WDI_ALLOCBREAK      0x0004

/* dwOptions values */
#define DBO_CHECKHEAP       0x0001
#define DBO_BUFFERFILL      0x0004
#define DBO_DISABLEGPTRAPPING 0x0010
#define DBO_CHECKFREE       0x0020

#define DBO_SILENT          0x8000

#define DBO_TRACEBREAK      0x2000
#define DBO_WARNINGBREAK    0x1000
#define DBO_NOERRORBREAK    0x0800
#define DBO_NOFATALBREAK    0x0400
#define DBO_INT3BREAK       0x0100

/* DebugOutput flags values */
#define DBF_TRACE           0x0000
#define DBF_WARNING         0x4000
#define DBF_ERROR           0x8000
#define DBF_FATAL           0xc000

/* dwFilter values */
#define DBF_KERNEL          0x1000
#define DBF_KRN_MEMMAN      0x0001
#define DBF_KRN_LOADMODULE  0x0002
#define DBF_KRN_SEGMENTLOAD 0x0004
#define DBF_USER            0x0800
#define DBF_GDI             0x0400
#define DBF_MMSYSTEM        0x0040
#define DBF_PENWIN          0x0020
#define DBF_APPLICATION     0x0008
#define DBF_DRIVER          0x0010

#endif /* NOLOGERROR */

/***********************************************************************
 *          GetWinDebugInfo   (KERNEL.355)
 */
BOOL16 WINAPI GetWinDebugInfo16(WINDEBUGINFO16 *lpwdi, UINT16 flags)
{
    FIXME("(%p,%d): stub returning FALSE\n", lpwdi, flags);
    /* FALSE means not in debugging mode/version */
    /* Can this type of debugging be used in wine ? */
    /* Constants: WDI_OPTIONS WDI_FILTER WDI_ALLOCBREAK */
    return FALSE;
}

/***********************************************************************
 *          SetWinDebugInfo   (KERNEL.356)
 */
BOOL16 WINAPI SetWinDebugInfo16(WINDEBUGINFO16 *lpwdi)
{
    FIXME("(%p): stub returning FALSE\n", lpwdi);
    /* FALSE means not in debugging mode/version */
    /* Can this type of debugging be used in wine ? */
    /* Constants: WDI_OPTIONS WDI_FILTER WDI_ALLOCBREAK */
    return FALSE;
}

/***********************************************************************
 *           UnicodeToAnsi   (KERNEL.434)
 */
INT16 WINAPI UnicodeToAnsi16( LPCWSTR src, LPSTR dst, INT16 codepage )
{
    if ( codepage == -1 ) codepage = CP_ACP;
    return WideCharToMultiByte( codepage, 0, src, -1, dst, 0x7fffffff, NULL, NULL );
}

/***********************************************************************
 *       VWin32_EventCreate	(KERNEL.442)
 */
HANDLE WINAPI VWin32_EventCreate(VOID)
{
    HANDLE hEvent = CreateEventW( NULL, FALSE, 0, NULL );
    return ConvertToGlobalHandle( hEvent );
}

/***********************************************************************
 *       VWin32_EventDestroy	(KERNEL.443)
 */
VOID WINAPI VWin32_EventDestroy(HANDLE event)
{
    CloseHandle( event );
}

/***********************************************************************
 *       VWin32_EventWait	(KERNEL.450)
 */
VOID WINAPI VWin32_EventWait(HANDLE event)
{
    DWORD mutex_count;

    ReleaseThunkLock( &mutex_count );
    WaitForSingleObject( event, INFINITE );
    RestoreThunkLock( mutex_count );
}

/***********************************************************************
 *       VWin32_EventSet	(KERNEL.451)
 *       KERNEL_479             (KERNEL.479)
 */
VOID WINAPI VWin32_EventSet(HANDLE event)
{
    SetEvent( event );
}

/***********************************************************************
 *           GetProcAddress32   		(KERNEL.453)
 */
FARPROC WINAPI GetProcAddress32_16( HMODULE hModule, LPCSTR function )
{
    /* FIXME: we used to disable snoop when returning proc for Win16 subsystem */
    return GetProcAddress( hModule, function );
}

/***********************************************************************
 *           CreateW32Event    (KERNEL.457)
 */
HANDLE WINAPI CreateW32Event( BOOL manual_reset, BOOL initial_state )
{
    return CreateEventW( NULL, manual_reset, initial_state, NULL );
}

/***********************************************************************
 *           SetW32Event (KERNEL.458)
 */
BOOL WINAPI SetW32Event( HANDLE handle )
{
    return SetEvent( handle );
}

/***********************************************************************
 *           ResetW32Event (KERNEL.459)
 */
BOOL WINAPI ResetW32Event( HANDLE handle )
{
    return ResetEvent( handle );
}

/***********************************************************************
 *           WaitForSingleObject   (KERNEL.460)
 */
DWORD WINAPI WaitForSingleObject16( HANDLE handle, DWORD timeout )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForSingleObject( handle, timeout );
    RestoreThunkLock( mutex_count );
    return retval;
}

/***********************************************************************
 *           WaitForMultipleObjects   (KERNEL.461)
 */
DWORD WINAPI WaitForMultipleObjects16( DWORD count, const HANDLE *handles,
                                       BOOL wait_all, DWORD timeout )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForMultipleObjectsEx( count, handles, wait_all, timeout, FALSE );
    RestoreThunkLock( mutex_count );
    return retval;
}

/***********************************************************************
 *		GetCurrentThreadId (KERNEL.462)
 */
DWORD WINAPI GetCurrentThreadId16(void)
{
    return GetCurrentThreadId();
}

/***********************************************************************
 *           ExitProcess   (KERNEL.466)
 */
void WINAPI ExitProcess16( WORD status )
{
    DWORD count;
    ReleaseThunkLock( &count );
    ExitProcess( status );
}

/***********************************************************************
 *		GetCurrentProcessId (KERNEL.471)
 */
DWORD WINAPI GetCurrentProcessId16(void)
{
    return GetCurrentProcessId();
}

/*********************************************************************
 *           CloseW32Handle (KERNEL.474)
 */
BOOL WINAPI CloseW32Handle( HANDLE handle )
{
    return CloseHandle( handle );
}

/***********************************************************************
 *           ConvertToGlobalHandle   (KERNEL.476)
 */
HANDLE WINAPI ConvertToGlobalHandle16( HANDLE handle )
{
    return ConvertToGlobalHandle( handle );
}

/*********************************************************************
 *           MapProcessHandle   (KERNEL.483)
 */
DWORD WINAPI MapProcessHandle( HANDLE hProcess )
{
    return GetProcessId( hProcess );
}

/***********************************************************************
 *           SetProcessDword    (KERNEL.484)
 * 'Of course you cannot directly access Windows internal structures'
 */
void WINAPI SetProcessDword( DWORD dwProcessID, INT offset, DWORD value )
{
    TRACE("(%ld, %d)\n", dwProcessID, offset );

    if (dwProcessID && dwProcessID != GetCurrentProcessId())
    {
        ERR("%d: process %lx not accessible\n", offset, dwProcessID);
        return;
    }

    switch ( offset )
    {
    case GPD_APP_COMPAT_FLAGS:
    case GPD_LOAD_DONE_EVENT:
    case GPD_HINSTANCE16:
    case GPD_WINDOWS_VERSION:
    case GPD_THDB:
    case GPD_PDB:
    case GPD_STARTF_SHELLDATA:
    case GPD_STARTF_HOTKEY:
    case GPD_STARTF_SHOWWINDOW:
    case GPD_STARTF_SIZE:
    case GPD_STARTF_POSITION:
    case GPD_STARTF_FLAGS:
    case GPD_PARENT:
    case GPD_FLAGS:
        ERR("Not allowed to modify offset %d\n", offset );
        break;
    case GPD_USERDATA:
        process_dword = value;
        break;
    default:
        ERR("Unknown offset %d\n", offset );
        break;
    }
}

/***********************************************************************
 *           GetProcessDword    (KERNEL.485)
 * 'Of course you cannot directly access Windows internal structures'
 */
DWORD WINAPI GetProcessDword( DWORD dwProcessID, INT offset )
{
    DWORD               x, y;
    STARTUPINFOW        siw;

    TRACE("(%ld, %d)\n", dwProcessID, offset );

    if (dwProcessID && dwProcessID != GetCurrentProcessId())
    {
        ERR("%d: process %lx not accessible\n", offset, dwProcessID);
        return 0;
    }

    switch ( offset )
    {
    case GPD_APP_COMPAT_FLAGS:
        return GetAppCompatFlags16(0);
    case GPD_LOAD_DONE_EVENT:
        return 0;
    case GPD_HINSTANCE16:
        return GetTaskDS16();
    case GPD_WINDOWS_VERSION:
        return GetExeVersion16();
    case GPD_THDB:
        return (DWORD_PTR)NtCurrentTeb() - 0x10 /* FIXME */;
    case GPD_PDB:
        return (DWORD_PTR)NtCurrentTeb()->Peb; /* FIXME: truncating a pointer */
    case GPD_STARTF_SHELLDATA: /* return stdoutput handle from startupinfo ??? */
        GetStartupInfoW(&siw);
        return HandleToULong(siw.hStdOutput);
    case GPD_STARTF_HOTKEY: /* return stdinput handle from startupinfo ??? */
        GetStartupInfoW(&siw);
        return HandleToULong(siw.hStdInput);
    case GPD_STARTF_SHOWWINDOW:
        GetStartupInfoW(&siw);
        return siw.wShowWindow;
    case GPD_STARTF_SIZE:
        GetStartupInfoW(&siw);
        x = siw.dwXSize;
        if ( (INT)x == CW_USEDEFAULT ) x = CW_USEDEFAULT16;
        y = siw.dwYSize;
        if ( (INT)y == CW_USEDEFAULT ) y = CW_USEDEFAULT16;
        return MAKELONG( x, y );
    case GPD_STARTF_POSITION:
        GetStartupInfoW(&siw);
        x = siw.dwX;
        if ( (INT)x == CW_USEDEFAULT ) x = CW_USEDEFAULT16;
        y = siw.dwY;
        if ( (INT)y == CW_USEDEFAULT ) y = CW_USEDEFAULT16;
        return MAKELONG( x, y );
    case GPD_STARTF_FLAGS:
        GetStartupInfoW(&siw);
        return siw.dwFlags;
    case GPD_PARENT:
        return 0;
    case GPD_FLAGS:
        return GetProcessFlags(0);
    case GPD_USERDATA:
        return process_dword;
    default:
        ERR("Unknown offset %d\n", offset );
        return 0;
    }
}

/***********************************************************************
 *           FreeLibrary32 (KERNEL.486)
 */
BOOL WINAPI FreeLibrary32_16( HINSTANCE module )
{
    return FreeLibrary( module );
}

/***********************************************************************
 *              GetModuleFileName32     (KERNEL.487)
 */
DWORD WINAPI GetModuleFileName32_16( HMODULE module, LPSTR buffer, DWORD size )
{
    return GetModuleFileNameA( module, buffer, size );
}

/***********************************************************************
 *              GetModuleHandle32        (KERNEL.488)
 */
HMODULE WINAPI GetModuleHandle32_16(LPCSTR module)
{
    return GetModuleHandleA( module );
}

/***********************************************************************
 *		RegisterServiceProcess (KERNEL.491)
 */
DWORD WINAPI RegisterServiceProcess16( DWORD dwProcessId, DWORD dwType )
{
    return 1; /* success */
}

/***********************************************************************
 *           WaitForMultipleObjectsEx   (KERNEL.495)
 */
DWORD WINAPI WaitForMultipleObjectsEx16( DWORD count, const HANDLE *handles,
                                         BOOL wait_all, DWORD timeout, BOOL alertable )
{
    DWORD retval, mutex_count;

    ReleaseThunkLock( &mutex_count );
    retval = WaitForMultipleObjectsEx( count, handles, wait_all, timeout, alertable );
    RestoreThunkLock( mutex_count );
    return retval;
}

/**********************************************************************
 * VWin32_BoostThreadGroup   (KERNEL.535)
 */
VOID WINAPI VWin32_BoostThreadGroup( DWORD threadId, INT boost )
{
    FIXME("(0x%08lx,%d): stub\n", threadId, boost);
}


/**********************************************************************
 * VWin32_BoostThreadStatic   (KERNEL.536)
 */
VOID WINAPI VWin32_BoostThreadStatic( DWORD threadId, INT boost )
{
    FIXME("(0x%08lx,%d): stub\n", threadId, boost);
}

/***********************************************************************
 *		EnableDos (KERNEL.41)
 *		DisableDos (KERNEL.42)
 *		GetLastDiskChange (KERNEL.98)
 *		ValidateCodeSegments (KERNEL.100)
 *		KbdRst (KERNEL.123)
 *		EnableKernel (KERNEL.124)
 *		DisableKernel (KERNEL.125)
 *		ValidateFreeSpaces (KERNEL.200)
 *		K237 (KERNEL.237)
 *		BUNNY_351 (KERNEL.351)
 *		PIGLET_361 (KERNEL.361)
 *
 * Entry point for kernel functions that do nothing.
 */
LONG WINAPI KERNEL_nop(void)
{
    return 0;
}

/***********************************************************************
 *           ToolHelpHook                             (KERNEL.341)
 *	see "Undocumented Windows"
 */
FARPROC16 WINAPI ToolHelpHook16(FARPROC16 func)
{
    static FARPROC16 hook;

    FIXME("(%p), stub.\n", func);
    return InterlockedExchangePointer( (void **)&hook, func );
}

/* thunk for 16-bit CreateThread */
struct thread_args
{
    FARPROC16 proc;
    DWORD     param;
};

static DWORD CALLBACK start_thread16( LPVOID threadArgs )
{
    struct thread_args args = *(struct thread_args *)threadArgs;
    HeapFree( GetProcessHeap(), 0, threadArgs );
    return K32WOWCallback16( (DWORD)args.proc, args.param );
}

/***********************************************************************
 *           CreateThread16   (KERNEL.441)
 */
HANDLE WINAPI CreateThread16( SECURITY_ATTRIBUTES *sa, DWORD stack,
                              FARPROC16 start, SEGPTR param,
                              DWORD flags, LPDWORD id )
{
    struct thread_args *args = HeapAlloc( GetProcessHeap(), 0, sizeof(*args) );
    if (!args) return INVALID_HANDLE_VALUE;
    args->proc = start;
    args->param = param;
    return CreateThread( sa, stack, start_thread16, args, flags, id );
}

/***********************************************************************
 *           _DebugOutput                    (KERNEL.328)
 */
void WINAPIV _DebugOutput( WORD flags, LPCSTR spec, VA_LIST16 valist )
{
    char caller[101];

    /* Decode caller address */
    if (!GetModuleName16( GetExePtr(CURRENT_STACK16->cs), caller, sizeof(caller) ))
        sprintf( caller, "%04X:%04X", CURRENT_STACK16->cs, CURRENT_STACK16->ip );

    /* FIXME: cannot use wvsnprintf16 from kernel */
    /* wvsnprintf16( temp, sizeof(temp), spec, valist ); */

    /* Output */
    FIXME("%s %04x %s\n", caller, flags, debugstr_a(spec) );
}
