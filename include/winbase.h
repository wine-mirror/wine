#ifndef __WINE_WINBASE_H
#define __WINE_WINBASE_H

#include "winnt.h"

#pragma pack(1)


#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagCOORD {
    INT16 x;
    INT16 y;
} COORD, *LPCOORD;


  /* Windows Exit Procedure flag values */
#define	WEP_FREE_DLL        0
#define	WEP_SYSTEM_EXIT     1

typedef DWORD (CALLBACK *LPTHREAD_START_ROUTINE)(LPVOID);

/* This is also defined in winnt.h */
/* typedef struct _EXCEPTION_RECORD {
    DWORD   ExceptionCode;
    DWORD   ExceptionFlags;
    struct  _EXCEPTION_RECORD *ExceptionRecord;
    LPVOID  ExceptionAddress;
    DWORD   NumberParameters;
    DWORD   ExceptionInformation[15];
} EXCEPTION_RECORD; */

typedef struct _EXCEPTION_DEBUG_INFO {
/*    EXCEPTION_RECORD ExceptionRecord; */
    DWORD dwFirstChange;
} EXCEPTION_DEBUG_INFO;

typedef struct _CREATE_THREAD_DEBUG_INFO {
    HANDLE hThread;
    LPVOID lpThreadLocalBase;
    LPTHREAD_START_ROUTINE lpStartAddress;
} CREATE_THREAD_DEBUG_INFO;

typedef struct _CREATE_PROCESS_DEBUG_INFO {
    HANDLE hFile;
    HANDLE hProcess;
    HANDLE hThread;
    LPVOID lpBaseOfImage;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    LPVOID lpThreadLocalBase;
    LPTHREAD_START_ROUTINE lpStartAddress;
    LPVOID lpImageName;
    WORD fUnicode;
} CREATE_PROCESS_DEBUG_INFO;

typedef struct _EXIT_THREAD_DEBUG_INFO {
    DWORD dwExitCode;
} EXIT_THREAD_DEBUG_INFO;

typedef struct _EXIT_PROCESS_DEBUG_INFO {
    DWORD dwExitCode;
} EXIT_PROCESS_DEBUG_INFO;

typedef struct _LOAD_DLL_DEBUG_INFO {
    HANDLE hFile;
    LPVOID   lpBaseOfDll;
    DWORD    dwDebugInfoFileOffset;
    DWORD    nDebugInfoSize;
    LPVOID   lpImageName;
    WORD     fUnicode;
} LOAD_DLL_DEBUG_INFO;

typedef struct _UNLOAD_DLL_DEBUG_INFO {
    LPVOID lpBaseOfDll;
} UNLOAD_DLL_DEBUG_INFO;

typedef struct _OUTPUT_DEBUG_STRING_INFO {
    LPSTR lpDebugStringData;
    WORD  fUnicode;
    WORD  nDebugStringLength;
} OUTPUT_DEBUG_STRING_INFO;

typedef struct _RIP_INFO {
    DWORD dwError;
    DWORD dwType;
} RIP_INFO;

typedef struct _DEBUG_EVENT {
    DWORD dwDebugEventCode;
    DWORD dwProcessId;
    DWORD dwThreadId;
    union {
        EXCEPTION_DEBUG_INFO      Exception;
        CREATE_THREAD_DEBUG_INFO  CreateThread;
        CREATE_PROCESS_DEBUG_INFO CreateProcessInfo;
        EXIT_THREAD_DEBUG_INFO    ExitThread;
        EXIT_PROCESS_DEBUG_INFO   ExitProcess;
        LOAD_DLL_DEBUG_INFO       LoadDll;
        UNLOAD_DLL_DEBUG_INFO     UnloadDll;
        OUTPUT_DEBUG_STRING_INFO  DebugString;
        RIP_INFO                  RipInfo;
    } u;
} DEBUG_EVENT, *LPDEBUG_EVENT;

#define OFS_MAXPATHNAME 128
typedef struct
{
    BYTE cBytes;
    BYTE fFixedDisk;
    WORD nErrCode;
    BYTE reserved[4];
    BYTE szPathName[OFS_MAXPATHNAME];
} OFSTRUCT, *LPOFSTRUCT;

#define OF_READ               0x0000
#define OF_WRITE              0x0001
#define OF_READWRITE          0x0002
#define OF_SHARE_COMPAT       0x0000
#define OF_SHARE_EXCLUSIVE    0x0010
#define OF_SHARE_DENY_WRITE   0x0020
#define OF_SHARE_DENY_READ    0x0030
#define OF_SHARE_DENY_NONE    0x0040
#define OF_PARSE              0x0100
#define OF_DELETE             0x0200
#define OF_VERIFY             0x0400   /* Used with OF_REOPEN */
#define OF_SEARCH             0x0400   /* Used without OF_REOPEN */
#define OF_CANCEL             0x0800
#define OF_CREATE             0x1000
#define OF_PROMPT             0x2000
#define OF_EXIST              0x4000
#define OF_REOPEN             0x8000

/* SetErrorMode values */
#define SEM_FAILCRITICALERRORS      0x0001
#define SEM_NOGPFAULTERRORBOX       0x0002
#define SEM_NOALIGNMENTFAULTEXCEPT  0x0004
#define SEM_NOOPENFILEERRORBOX      0x8000

/* CopyFileEx flags */
#define COPY_FILE_FAIL_IF_EXISTS        0x00000001
#define COPY_FILE_RESTARTABLE           0x00000002
#define COPY_FILE_OPEN_SOURCE_FOR_WRITE 0x00000004

/* GetTempFileName() Flags */
#define TF_FORCEDRIVE	        0x80

#define DRIVE_CANNOTDETERMINE      0
#define DRIVE_DOESNOTEXIST         1
#define DRIVE_REMOVABLE            2
#define DRIVE_FIXED                3
#define DRIVE_REMOTE               4
/* Win32 additions */
#define DRIVE_CDROM                5
#define DRIVE_RAMDISK              6

/* The security attributes structure */
typedef struct
{
    DWORD   nLength;
    LPVOID  lpSecurityDescriptor;
    BOOL  bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

#ifndef _FILETIME_
#define _FILETIME_
/* 64 bit number of 100 nanoseconds intervals since January 1, 1601 */
typedef struct
{
  DWORD  dwLowDateTime;
  DWORD  dwHighDateTime;
} FILETIME, *LPFILETIME;
#endif /* _FILETIME_ */

/* Find* structures */
typedef struct
{
    DWORD     dwFileAttributes;
    FILETIME  ftCreationTime;
    FILETIME  ftLastAccessTime;
    FILETIME  ftLastWriteTime;
    DWORD     nFileSizeHigh;
    DWORD     nFileSizeLow;
    DWORD     dwReserved0;
    DWORD     dwReserved1;
    CHAR      cFileName[260];
    CHAR      cAlternateFileName[14];
} WIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;

typedef struct
{
    DWORD     dwFileAttributes;
    FILETIME  ftCreationTime;
    FILETIME  ftLastAccessTime;
    FILETIME  ftLastWriteTime;
    DWORD     nFileSizeHigh;
    DWORD     nFileSizeLow;
    DWORD     dwReserved0;
    DWORD     dwReserved1;
    WCHAR     cFileName[260];
    WCHAR     cAlternateFileName[14];
} WIN32_FIND_DATAW, *LPWIN32_FIND_DATAW;

DECL_WINELIB_TYPE_AW(WIN32_FIND_DATA)
DECL_WINELIB_TYPE_AW(LPWIN32_FIND_DATA)

#define INVALID_HANDLE_VALUE16  ((HANDLE16) -1)
#define INVALID_HANDLE_VALUE  ((HANDLE) -1)

/* comm */

#define CBR_110	0xFF10
#define CBR_300	0xFF11
#define CBR_600	0xFF12
#define CBR_1200	0xFF13
#define CBR_2400	0xFF14
#define CBR_4800	0xFF15
#define CBR_9600	0xFF16
#define CBR_14400	0xFF17
#define CBR_19200	0xFF18
#define CBR_38400	0xFF1B
#define CBR_56000	0xFF1F
#define CBR_128000	0xFF23
#define CBR_256000	0xFF27

#define NOPARITY	0
#define ODDPARITY	1
#define EVENPARITY	2
#define MARKPARITY	3
#define SPACEPARITY	4
#define ONESTOPBIT	0
#define ONE5STOPBITS	1
#define TWOSTOPBITS	2

#define IGNORE		0
#define INFINITE16      0xFFFF
#define INFINITE      0xFFFFFFFF

#define CE_RXOVER	0x0001
#define CE_OVERRUN	0x0002
#define CE_RXPARITY	0x0004
#define CE_FRAME	0x0008
#define CE_BREAK	0x0010
#define CE_CTSTO	0x0020
#define CE_DSRTO	0x0040
#define CE_RLSDTO	0x0080
#define CE_TXFULL	0x0100
#define CE_PTO		0x0200
#define CE_IOE		0x0400
#define CE_DNS		0x0800
#define CE_OOP		0x1000
#define CE_MODE	0x8000

#define IE_BADID	-1
#define IE_OPEN	-2
#define IE_NOPEN	-3
#define IE_MEMORY	-4
#define IE_DEFAULT	-5
#define IE_HARDWARE	-10
#define IE_BYTESIZE	-11
#define IE_BAUDRATE	-12

#define EV_RXCHAR	0x0001
#define EV_RXFLAG	0x0002
#define EV_TXEMPTY	0x0004
#define EV_CTS		0x0008
#define EV_DSR		0x0010
#define EV_RLSD	0x0020
#define EV_BREAK	0x0040
#define EV_ERR		0x0080
#define EV_RING	0x0100
#define EV_PERR	0x0200
#define EV_CTSS	0x0400
#define EV_DSRS	0x0800
#define EV_RLSDS	0x1000
#define EV_RINGTE	0x2000
#define EV_RingTe	EV_RINGTE

#define SETXOFF	1
#define SETXON		2
#define SETRTS		3
#define CLRRTS		4
#define SETDTR		5
#define CLRDTR		6
#define RESETDEV	7
#define SETBREAK	8
#define CLRBREAK	9

#define GETBASEIRQ	10

/* Purge functions for Comm Port */
#define PURGE_TXABORT       0x0001  /* Kill the pending/current writes to the 
				       comm port */
#define PURGE_RXABORT       0x0002  /*Kill the pending/current reads to 
				     the comm port */
#define PURGE_TXCLEAR       0x0004  /* Kill the transmit queue if there*/
#define PURGE_RXCLEAR       0x0008  /* Kill the typeahead buffer if there*/


/* Modem Status Flags */
#define MS_CTS_ON           ((DWORD)0x0010)
#define MS_DSR_ON           ((DWORD)0x0020)
#define MS_RING_ON          ((DWORD)0x0040)
#define MS_RLSD_ON          ((DWORD)0x0080)

#define	RTS_CONTROL_DISABLE	0
#define	RTS_CONTROL_ENABLE	1
#define	RTS_CONTROL_HANDSHAKE	2
#define	RTS_CONTROL_TOGGLE	3

#define	DTR_CONTROL_DISABLE	0
#define	DTR_CONTROL_ENABLE	1
#define	DTR_CONTROL_HANDSHAKE	2

#define CSTF_CTSHOLD	0x01
#define CSTF_DSRHOLD	0x02
#define CSTF_RLSDHOLD	0x04
#define CSTF_XOFFHOLD	0x08
#define CSTF_XOFFSENT	0x10
#define CSTF_EOF	0x20
#define CSTF_TXIM	0x40

#define MAKEINTRESOURCEA(i) (LPSTR)((DWORD)((WORD)(i)))
#define MAKEINTRESOURCEW(i) (LPWSTR)((DWORD)((WORD)(i)))
#define MAKEINTRESOURCE WINELIB_NAME_AW(MAKEINTRESOURCE)

/* Predefined resource types */
#define RT_CURSORA         MAKEINTRESOURCEA(1)
#define RT_CURSORW         MAKEINTRESOURCEW(1)
#define RT_CURSOR            WINELIB_NAME_AW(RT_CURSOR)
#define RT_BITMAPA         MAKEINTRESOURCEA(2)
#define RT_BITMAPW         MAKEINTRESOURCEW(2)
#define RT_BITMAP            WINELIB_NAME_AW(RT_BITMAP)
#define RT_ICONA           MAKEINTRESOURCEA(3)
#define RT_ICONW           MAKEINTRESOURCEW(3)
#define RT_ICON              WINELIB_NAME_AW(RT_ICON)
#define RT_MENUA           MAKEINTRESOURCEA(4)
#define RT_MENUW           MAKEINTRESOURCEW(4)
#define RT_MENU              WINELIB_NAME_AW(RT_MENU)
#define RT_DIALOGA         MAKEINTRESOURCEA(5)
#define RT_DIALOGW         MAKEINTRESOURCEW(5)
#define RT_DIALOG            WINELIB_NAME_AW(RT_DIALOG)
#define RT_STRINGA         MAKEINTRESOURCEA(6)
#define RT_STRINGW         MAKEINTRESOURCEW(6)
#define RT_STRING            WINELIB_NAME_AW(RT_STRING)
#define RT_FONTDIRA        MAKEINTRESOURCEA(7)
#define RT_FONTDIRW        MAKEINTRESOURCEW(7)
#define RT_FONTDIR           WINELIB_NAME_AW(RT_FONTDIR)
#define RT_FONTA           MAKEINTRESOURCEA(8)
#define RT_FONTW           MAKEINTRESOURCEW(8)
#define RT_FONT              WINELIB_NAME_AW(RT_FONT)
#define RT_ACCELERATORA    MAKEINTRESOURCEA(9)
#define RT_ACCELERATORW    MAKEINTRESOURCEW(9)
#define RT_ACCELERATOR       WINELIB_NAME_AW(RT_ACCELERATOR)
#define RT_RCDATAA         MAKEINTRESOURCEA(10)
#define RT_RCDATAW         MAKEINTRESOURCEW(10)
#define RT_RCDATA            WINELIB_NAME_AW(RT_RCDATA)
#define RT_MESSAGELISTA    MAKEINTRESOURCEA(11)
#define RT_MESSAGELISTW    MAKEINTRESOURCEW(11)
#define RT_MESSAGELIST       WINELIB_NAME_AW(RT_MESSAGELIST)
#define RT_GROUP_CURSORA   MAKEINTRESOURCEA(12)
#define RT_GROUP_CURSORW   MAKEINTRESOURCEW(12)
#define RT_GROUP_CURSOR      WINELIB_NAME_AW(RT_GROUP_CURSOR)
#define RT_GROUP_ICONA     MAKEINTRESOURCEA(14)
#define RT_GROUP_ICONW     MAKEINTRESOURCEW(14)
#define RT_GROUP_ICON        WINELIB_NAME_AW(RT_GROUP_ICON)


#define LMEM_FIXED          0   
#define LMEM_MOVEABLE       0x0002
#define LMEM_NOCOMPACT      0x0010
#define LMEM_NODISCARD      0x0020
#define LMEM_ZEROINIT       0x0040
#define LMEM_MODIFY         0x0080
#define LMEM_DISCARDABLE    0x0F00
#define LMEM_DISCARDED	    0x4000
#define LMEM_LOCKCOUNT	    0x00FF

#define LPTR (LMEM_FIXED | LMEM_ZEROINIT)

#define GMEM_FIXED          0x0000
#define GMEM_MOVEABLE       0x0002
#define GMEM_NOCOMPACT      0x0010
#define GMEM_NODISCARD      0x0020
#define GMEM_ZEROINIT       0x0040
#define GMEM_MODIFY         0x0080
#define GMEM_DISCARDABLE    0x0100
#define GMEM_NOT_BANKED     0x1000
#define GMEM_SHARE          0x2000
#define GMEM_DDESHARE       0x2000
#define GMEM_NOTIFY         0x4000
#define GMEM_LOWER          GMEM_NOT_BANKED
#define GMEM_DISCARDED      0x4000
#define GMEM_LOCKCOUNT      0x00ff
#define GMEM_INVALID_HANDLE 0x8000

#define GHND                (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define GPTR                (GMEM_FIXED | GMEM_ZEROINIT)


typedef struct tagMEMORYSTATUS
{
    DWORD    dwLength;
    DWORD    dwMemoryLoad;
    DWORD    dwTotalPhys;
    DWORD    dwAvailPhys;
    DWORD    dwTotalPageFile;
    DWORD    dwAvailPageFile;
    DWORD    dwTotalVirtual;
    DWORD    dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;


#ifndef NOLOGERROR

/* LogParamError and LogError values */

/* Error modifier bits */
#define ERR_WARNING             0x8000
#define ERR_PARAM               0x4000

#define ERR_SIZE_MASK           0x3000
#define ERR_BYTE                0x1000
#define ERR_WORD                0x2000
#define ERR_DWORD               0x3000

/* LogParamError() values */

/* Generic parameter values */
#define ERR_BAD_VALUE           0x6001
#define ERR_BAD_FLAGS           0x6002
#define ERR_BAD_INDEX           0x6003
#define ERR_BAD_DVALUE          0x7004
#define ERR_BAD_DFLAGS          0x7005
#define ERR_BAD_DINDEX          0x7006
#define ERR_BAD_PTR             0x7007
#define ERR_BAD_FUNC_PTR        0x7008
#define ERR_BAD_SELECTOR        0x6009
#define ERR_BAD_STRING_PTR      0x700a
#define ERR_BAD_HANDLE          0x600b

/* KERNEL parameter errors */
#define ERR_BAD_HINSTANCE       0x6020
#define ERR_BAD_HMODULE         0x6021
#define ERR_BAD_GLOBAL_HANDLE   0x6022
#define ERR_BAD_LOCAL_HANDLE    0x6023
#define ERR_BAD_ATOM            0x6024
#define ERR_BAD_HFILE           0x6025

/* USER parameter errors */
#define ERR_BAD_HWND            0x6040
#define ERR_BAD_HMENU           0x6041
#define ERR_BAD_HCURSOR         0x6042
#define ERR_BAD_HICON           0x6043
#define ERR_BAD_HDWP            0x6044
#define ERR_BAD_CID             0x6045
#define ERR_BAD_HDRVR           0x6046

/* GDI parameter errors */
#define ERR_BAD_COORDS          0x7060
#define ERR_BAD_GDI_OBJECT      0x6061
#define ERR_BAD_HDC             0x6062
#define ERR_BAD_HPEN            0x6063
#define ERR_BAD_HFONT           0x6064
#define ERR_BAD_HBRUSH          0x6065
#define ERR_BAD_HBITMAP         0x6066
#define ERR_BAD_HRGN            0x6067
#define ERR_BAD_HPALETTE        0x6068
#define ERR_BAD_HMETAFILE       0x6069


/* LogError() values */

/* KERNEL errors */
#define ERR_GALLOC              0x0001
#define ERR_GREALLOC            0x0002
#define ERR_GLOCK               0x0003
#define ERR_LALLOC              0x0004
#define ERR_LREALLOC            0x0005
#define ERR_LLOCK               0x0006
#define ERR_ALLOCRES            0x0007
#define ERR_LOCKRES             0x0008
#define ERR_LOADMODULE          0x0009

/* USER errors */
#define ERR_CREATEDLG           0x0040
#define ERR_CREATEDLG2          0x0041
#define ERR_REGISTERCLASS       0x0042
#define ERR_DCBUSY              0x0043
#define ERR_CREATEWND           0x0044
#define ERR_STRUCEXTRA          0x0045
#define ERR_LOADSTR             0x0046
#define ERR_LOADMENU            0x0047
#define ERR_NESTEDBEGINPAINT    0x0048
#define ERR_BADINDEX            0x0049
#define ERR_CREATEMENU          0x004a

/* GDI errors */
#define ERR_CREATEDC            0x0080
#define ERR_CREATEMETA          0x0081
#define ERR_DELOBJSELECTED      0x0082
#define ERR_SELBITMAP           0x0083



/* Debugging support (DEBUG SYSTEM ONLY) */
typedef struct
{
    UINT16  flags;
    DWORD   dwOptions WINE_PACKED;
    DWORD   dwFilter WINE_PACKED;
    CHAR    achAllocModule[8] WINE_PACKED;
    DWORD   dwAllocBreak WINE_PACKED;
    DWORD   dwAllocCount WINE_PACKED;
} WINDEBUGINFO, *LPWINDEBUGINFO;

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

typedef struct {
        WORD wYear;
        WORD wMonth;
        WORD wDayOfWeek;
        WORD wDay;
        WORD wHour;
        WORD wMinute;
        WORD wSecond;
        WORD wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;


/* Code page information.
 */
#define MAX_LEADBYTES     12
#define MAX_DEFAULTCHAR   2

typedef struct
{
    UINT MaxCharSize;
    BYTE   DefaultChar[MAX_DEFAULTCHAR];
    BYTE   LeadByte[MAX_LEADBYTES];
} CPINFO, *LPCPINFO;

/* The 'overlapped' data structure used by async I/O functions.
 */
typedef struct {
        DWORD Internal;
        DWORD InternalHigh;
        DWORD Offset;
        DWORD OffsetHigh;
        HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;

/* Process startup information.
 */

/* STARTUPINFO.dwFlags */
#define	STARTF_USESHOWWINDOW	0x00000001
#define	STARTF_USESIZE		0x00000002
#define	STARTF_USEPOSITION	0x00000004
#define	STARTF_USECOUNTCHARS	0x00000008
#define	STARTF_USEFILLATTRIBUTE	0x00000010
#define	STARTF_RUNFULLSCREEN	0x00000020
#define	STARTF_FORCEONFEEDBACK	0x00000040
#define	STARTF_FORCEOFFFEEDBACK	0x00000080
#define	STARTF_USESTDHANDLES	0x00000100
#define	STARTF_USEHOTKEY	0x00000200

typedef struct {
        DWORD cb;		/* 00: size of struct */
        LPSTR lpReserved;	/* 04: */
        LPSTR lpDesktop;	/* 08: */
        LPSTR lpTitle;		/* 0c: */
        DWORD dwX;		/* 10: */
        DWORD dwY;		/* 14: */
        DWORD dwXSize;		/* 18: */
        DWORD dwYSize;		/* 1c: */
        DWORD dwXCountChars;	/* 20: */
        DWORD dwYCountChars;	/* 24: */
        DWORD dwFillAttribute;	/* 28: */
        DWORD dwFlags;		/* 2c: */
        WORD wShowWindow;	/* 30: */
        WORD cbReserved2;	/* 32: */
        BYTE *lpReserved2;	/* 34: */
        HANDLE hStdInput;	/* 38: */
        HANDLE hStdOutput;	/* 3c: */
        HANDLE hStdError;	/* 40: */
} STARTUPINFOA, *LPSTARTUPINFOA;

typedef struct {
        DWORD cb;
        LPWSTR lpReserved;
        LPWSTR lpDesktop;
        LPWSTR lpTitle;
        DWORD dwX;
        DWORD dwY;
        DWORD dwXSize;
        DWORD dwYSize;
        DWORD dwXCountChars;
        DWORD dwYCountChars;
        DWORD dwFillAttribute;
        DWORD dwFlags;
        WORD wShowWindow;
        WORD cbReserved2;
        BYTE *lpReserved2;
        HANDLE hStdInput;
        HANDLE hStdOutput;
        HANDLE hStdError;
} STARTUPINFOW, *LPSTARTUPINFOW;

DECL_WINELIB_TYPE_AW(STARTUPINFO)
DECL_WINELIB_TYPE_AW(LPSTARTUPINFO)

typedef struct {
	HANDLE	hProcess;
	HANDLE	hThread;
	DWORD		dwProcessId;
	DWORD		dwThreadId;
} PROCESS_INFORMATION,*LPPROCESS_INFORMATION;

typedef struct {
        LONG Bias;
        WCHAR StandardName[32];
        SYSTEMTIME StandardDate;
        LONG StandardBias;
        WCHAR DaylightName[32];
        SYSTEMTIME DaylightDate;
        LONG DaylightBias;
} TIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;

#define TIME_ZONE_ID_UNKNOWN    0
#define TIME_ZONE_ID_STANDARD   1
#define TIME_ZONE_ID_DAYLIGHT   2

/* CreateProcess: dwCreationFlag values
 */
#define DEBUG_PROCESS               0x00000001
#define DEBUG_ONLY_THIS_PROCESS     0x00000002
#define CREATE_SUSPENDED            0x00000004
#define DETACHED_PROCESS            0x00000008
#define CREATE_NEW_CONSOLE          0x00000010
#define NORMAL_PRIORITY_CLASS       0x00000020
#define IDLE_PRIORITY_CLASS         0x00000040
#define HIGH_PRIORITY_CLASS         0x00000080
#define REALTIME_PRIORITY_CLASS     0x00000100
#define CREATE_NEW_PROCESS_GROUP    0x00000200
#define CREATE_UNICODE_ENVIRONMENT  0x00000400
#define CREATE_SEPARATE_WOW_VDM     0x00000800
#define CREATE_SHARED_WOW_VDM       0x00001000
#define CREATE_DEFAULT_ERROR_MODE   0x04000000
#define CREATE_NO_WINDOW            0x08000000
#define PROFILE_USER                0x10000000
#define PROFILE_KERNEL              0x20000000
#define PROFILE_SERVER              0x40000000


/* File object type definitions
 */
#define FILE_TYPE_UNKNOWN       0
#define FILE_TYPE_DISK          1
#define FILE_TYPE_CHAR          2
#define FILE_TYPE_PIPE          3
#define FILE_TYPE_REMOTE        32768

/* File creation flags
 */
#define FILE_FLAG_WRITE_THROUGH    0x80000000UL
#define FILE_FLAG_OVERLAPPED 	   0x40000000L
#define FILE_FLAG_NO_BUFFERING     0x20000000L
#define FILE_FLAG_RANDOM_ACCESS    0x10000000L
#define FILE_FLAG_SEQUENTIAL_SCAN  0x08000000L
#define FILE_FLAG_DELETE_ON_CLOSE  0x04000000L
#define FILE_FLAG_BACKUP_SEMANTICS 0x02000000L
#define FILE_FLAG_POSIX_SEMANTICS  0x01000000L
#define CREATE_NEW              1
#define CREATE_ALWAYS           2
#define OPEN_EXISTING           3
#define OPEN_ALWAYS             4
#define TRUNCATE_EXISTING       5

/* Standard handle identifiers
 */
#define STD_INPUT_HANDLE        ((DWORD) -10)
#define STD_OUTPUT_HANDLE       ((DWORD) -11)
#define STD_ERROR_HANDLE        ((DWORD) -12)

typedef struct
{
  int dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  int dwVolumeSerialNumber;
  int nFileSizeHigh;
  int nFileSizeLow;
  int nNumberOfLinks;
  int nFileIndexHigh;
  int nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION ;


typedef struct _SYSTEM_POWER_STATUS
{
  BOOL16  ACLineStatus;
  BYTE    BatteryFlag;
  BYTE    BatteryLifePercent;
  BYTE    reserved;
  DWORD   BatteryLifeTime;
  DWORD   BatteryFullLifeTime;
} SYSTEM_POWER_STATUS, *LPSYSTEM_POWER_STATUS;

typedef struct _MEMORY_BASIC_INFORMATION
{
    LPVOID   BaseAddress;
    LPVOID   AllocationBase;
    DWORD    AllocationProtect;
    DWORD    RegionSize;
    DWORD    State;
    DWORD    Protect;
    DWORD    Type;
} MEMORY_BASIC_INFORMATION,*LPMEMORY_BASIC_INFORMATION;


typedef BOOL (CALLBACK *CODEPAGE_ENUMPROCA)(LPSTR);
typedef BOOL (CALLBACK *CODEPAGE_ENUMPROCW)(LPWSTR);
DECL_WINELIB_TYPE_AW(CODEPAGE_ENUMPROC)
typedef BOOL (CALLBACK *LOCALE_ENUMPROCA)(LPSTR);
typedef BOOL (CALLBACK *LOCALE_ENUMPROCW)(LPWSTR);
DECL_WINELIB_TYPE_AW(LOCALE_ENUMPROC)

typedef struct tagSYSTEM_INFO
{
    union {
    	DWORD	dwOemId;
	struct {
		WORD wProcessorArchitecture;
		WORD wReserved;
	} x;
    } u;
    DWORD	dwPageSize;
    LPVOID	lpMinimumApplicationAddress;
    LPVOID	lpMaximumApplicationAddress;
    DWORD	dwActiveProcessorMask;
    DWORD	dwNumberOfProcessors;
    DWORD	dwProcessorType;
    DWORD	dwAllocationGranularity;
    WORD	wProcessorLevel;
    WORD	wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;

/* service main function prototype */
typedef VOID (CALLBACK *LPSERVICE_MAIN_FUNCTIONA)(DWORD,LPSTR);
typedef VOID (CALLBACK *LPSERVICE_MAIN_FUNCTIONW)(DWORD,LPWSTR);
DECL_WINELIB_TYPE_AW(LPSERVICE_MAIN_FUNCTION)

/* service start table */
typedef struct
{
    LPSTR			lpServiceName;
    LPSERVICE_MAIN_FUNCTIONA	lpServiceProc;
} *LPSERVICE_TABLE_ENTRYA, SERVICE_TABLE_ENTRYA;

typedef struct
{
    LPWSTR			lpServiceName;
    LPSERVICE_MAIN_FUNCTIONW	lpServiceProc;
} *LPSERVICE_TABLE_ENTRYW, SERVICE_TABLE_ENTRYW;

DECL_WINELIB_TYPE_AW(SERVICE_TABLE_ENTRY)
DECL_WINELIB_TYPE_AW(LPSERVICE_TABLE_ENTRY)

/* {G,S}etPriorityClass */
#define	NORMAL_PRIORITY_CLASS	0x00000020
#define	IDLE_PRIORITY_CLASS	0x00000040
#define	HIGH_PRIORITY_CLASS	0x00000080
#define	REALTIME_PRIORITY_CLASS	0x00000100

typedef BOOL (CALLBACK *ENUMRESTYPEPROCA)(HMODULE,LPSTR,LONG);
typedef BOOL (CALLBACK *ENUMRESTYPEPROCW)(HMODULE,LPWSTR,LONG);
typedef BOOL (CALLBACK *ENUMRESNAMEPROCA)(HMODULE,LPCSTR,LPSTR,LONG);
typedef BOOL (CALLBACK *ENUMRESNAMEPROCW)(HMODULE,LPCWSTR,LPWSTR,LONG);
typedef BOOL (CALLBACK *ENUMRESLANGPROCA)(HMODULE,LPCSTR,LPCSTR,WORD,LONG);
typedef BOOL (CALLBACK *ENUMRESLANGPROCW)(HMODULE,LPCWSTR,LPCWSTR,WORD,LONG);

DECL_WINELIB_TYPE_AW(ENUMRESTYPEPROC)
DECL_WINELIB_TYPE_AW(ENUMRESNAMEPROC)
DECL_WINELIB_TYPE_AW(ENUMRESLANGPROC)

/* flags that can be passed to LoadLibraryEx */
#define	DONT_RESOLVE_DLL_REFERENCES	0x00000001
#define	LOAD_LIBRARY_AS_DATAFILE	0x00000002
#define	LOAD_WITH_ALTERED_SEARCH_PATH	0x00000008

/* ifdef _x86_ ... */
typedef struct _LDT_ENTRY {
    WORD	LimitLow;
    WORD	BaseLow;
    union {
	struct {
	    BYTE	BaseMid;
	    BYTE	Flags1;/*Declare as bytes to avoid alignment problems */
	    BYTE	Flags2; 
	    BYTE	BaseHi;
	} Bytes;
	struct {
	    DWORD	BaseMid		: 8;
	    DWORD	Type		: 5;
	    DWORD	Dpl		: 2;
	    DWORD	Pres		: 1;
	    DWORD	LimitHi		: 4;
	    DWORD	Sys		: 1;
	    DWORD	Reserved_0	: 1;
	    DWORD	Default_Big	: 1;
	    DWORD	Granularity	: 1;
	    DWORD	BaseHi		: 8;
	} Bits;
    } HighWord;
} LDT_ENTRY, *LPLDT_ENTRY;

/* for WOWHandle{16,32} */
typedef enum _WOW_HANDLE_TYPE { /* WOW */
    WOW_TYPE_HWND,
    WOW_TYPE_HMENU,
    WOW_TYPE_HDWP,
    WOW_TYPE_HDROP,
    WOW_TYPE_HDC,
    WOW_TYPE_HFONT,
    WOW_TYPE_HMETAFILE,
    WOW_TYPE_HRGN,
    WOW_TYPE_HBITMAP,
    WOW_TYPE_HBRUSH,
    WOW_TYPE_HPALETTE,
    WOW_TYPE_HPEN,
    WOW_TYPE_HACCEL,
    WOW_TYPE_HTASK,
    WOW_TYPE_FULLHWND
} WOW_HANDLE_TYPE;

/* WOWCallback16Ex defines */
#define WCB16_MAX_CBARGS	16
/* ... dwFlags */
#define WCB16_PASCAL		0x0
#define WCB16_CDECL		0x1

typedef enum _GET_FILEEX_INFO_LEVELS {
    GetFileExInfoStandard
} GET_FILEEX_INFO_LEVELS;

typedef struct _WIN32_FILE_ATTRIBUTES_DATA {
    DWORD    dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD    nFileSizeHigh;
    DWORD    nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;

typedef struct _DllVersionInfo {
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformID;
} DLLVERSIONINFO;

/*
 * This one seems to be a Win32 only definition. It also is defined with
 * WINAPI instead of CALLBACK in the windows headers.
 */
typedef DWORD (WINAPI *LPPROGRESS_ROUTINE)(LARGE_INTEGER, LARGE_INTEGER, LARGE_INTEGER, 
                                           LARGE_INTEGER, DWORD, DWORD, HANDLE,
                                           HANDLE, LPVOID);

#define DM_UPDATE	1
#define DM_COPY		2
#define DM_PROMPT	4
#define DM_MODIFY	8

#define DM_IN_BUFFER      DM_MODIFY
#define DM_IN_PROMPT      DM_PROMPT
#define DM_OUT_BUFFER     DM_COPY
#define DM_OUT_DEFAULT    DM_UPDATE

#define DM_ORIENTATION		0x00000001L
#define DM_PAPERSIZE		0x00000002L
#define DM_PAPERLENGTH		0x00000004L
#define DM_PAPERWIDTH		0x00000008L
#define DM_SCALE		0x00000010L
#define DM_COPIES		0x00000100L
#define DM_DEFAULTSOURCE	0x00000200L
#define DM_PRINTQUALITY		0x00000400L
#define DM_COLOR		0x00000800L
#define DM_DUPLEX		0x00001000L
#define DM_YRESOLUTION		0x00002000L
#define DM_TTOPTION		0x00004000L
#define DM_BITSPERPEL           0x00040000L
#define DM_PELSWIDTH            0x00080000L
#define DM_PELSHEIGHT           0x00100000L
#define DM_DISPLAYFLAGS         0x00200000L
#define DM_DISPLAYFREQUENCY     0x00400000L

/* etc.... */

#define DMORIENT_PORTRAIT	1
#define DMORIENT_LANDSCAPE	2

#define DMPAPER_LETTER		1
#define DMPAPER_LEGAL		5
#define DMPAPER_EXECUTIVE	7
#define DMPAPER_A3		8
#define DMPAPER_A4		9
#define DMPAPER_A5		11
#define DMPAPER_ENV_10		20
#define DMPAPER_ENV_DL		27
#define DMPAPER_ENV_C5		28
#define DMPAPER_ENV_B5		34
#define DMPAPER_ENV_MONARCH	37

#define DMBIN_UPPER		1
#define DMBIN_LOWER		2
#define DMBIN_MIDDLE		3
#define DMBIN_MANUAL		4
#define DMBIN_ENVELOPE		5
#define DMBIN_ENVMANUAL		6
#define DMBIN_AUTO		7
#define DMBIN_LARGECAPACITY	11

#define DMCOLOR_MONOCHROME	1
#define DMCOLOR_COLOR		2

#define DMTT_BITMAP		1
#define DMTT_DOWNLOAD		2
#define DMTT_SUBDEV		3

#define WAIT_FAILED		0xffffffff
#define WAIT_OBJECT_0		0
#define WAIT_ABANDONED		STATUS_ABANDONED_WAIT_0
#define WAIT_ABANDONED_0	STATUS_ABANDONED_WAIT_0
#define WAIT_IO_COMPLETION	STATUS_USER_APC
#define WAIT_TIMEOUT		STATUS_TIMEOUT

#define	PAGE_NOACCESS		0x01
#define	PAGE_READONLY		0x02
#define	PAGE_READWRITE		0x04
#define	PAGE_WRITECOPY		0x08
#define	PAGE_EXECUTE		0x10
#define	PAGE_EXECUTE_READ	0x20
#define	PAGE_EXECUTE_READWRITE	0x40
#define	PAGE_EXECUTE_WRITECOPY	0x80
#define	PAGE_GUARD		0x100
#define	PAGE_NOCACHE		0x200

#define MEM_COMMIT              0x00001000
#define MEM_RESERVE             0x00002000
#define MEM_DECOMMIT            0x00004000
#define MEM_RELEASE             0x00008000
#define MEM_FREE                0x00010000
#define MEM_PRIVATE             0x00020000
#define MEM_MAPPED              0x00040000
#define MEM_TOP_DOWN            0x00100000

#define SEC_FILE                0x00800000
#define SEC_IMAGE               0x01000000
#define SEC_RESERVE             0x04000000
#define SEC_COMMIT              0x08000000
#define SEC_NOCACHE             0x10000000

#define FILE_BEGIN              0
#define FILE_CURRENT            1
#define FILE_END                2

#define FILE_CASE_SENSITIVE_SEARCH      0x00000001
#define FILE_CASE_PRESERVED_NAMES       0x00000002
#define FILE_UNICODE_ON_DISK            0x00000004
#define FILE_PERSISTENT_ACLS            0x00000008

#define FILE_MAP_COPY                   0x00000001
#define FILE_MAP_WRITE                  0x00000002
#define FILE_MAP_READ                   0x00000004
#define FILE_MAP_ALL_ACCESS             0x000f001f

#define MOVEFILE_REPLACE_EXISTING       0x00000001
#define MOVEFILE_COPY_ALLOWED           0x00000002
#define MOVEFILE_DELAY_UNTIL_REBOOT     0x00000004

#define FS_CASE_SENSITIVE               FILE_CASE_SENSITIVE_SEARCH
#define FS_CASE_IS_PRESERVED            FILE_CASE_PRESERVED_NAMES
#define FS_UNICODE_STORED_ON_DISK       FILE_UNICODE_ON_DISK


#define STATUS_SUCCESS                   0x00000000
#define STATUS_WAIT_0                    0x00000000    
#define STATUS_ABANDONED_WAIT_0          0x00000080    
#define STATUS_USER_APC                  0x000000C0    
#define STATUS_TIMEOUT                   0x00000102    
#define STATUS_PENDING                   0x00000103    
#define STATUS_GUARD_PAGE_VIOLATION      0x80000001    
#define STATUS_DATATYPE_MISALIGNMENT     0x80000002    
#define STATUS_BREAKPOINT                0x80000003    
#define STATUS_SINGLE_STEP               0x80000004    
#define	STATUS_BUFFER_OVERFLOW           0x80000005
#define STATUS_ACCESS_VIOLATION          0xC0000005    
#define STATUS_IN_PAGE_ERROR             0xC0000006    
#define STATUS_INVALID_PARAMETER         0xC000000D
#define STATUS_NO_MEMORY                 0xC0000017    
#define STATUS_ILLEGAL_INSTRUCTION       0xC000001D    
#define	STATUS_BUFFER_TOO_SMALL          0xC0000023
#define STATUS_NONCONTINUABLE_EXCEPTION  0xC0000025    
#define STATUS_INVALID_DISPOSITION       0xC0000026    
#define	STATUS_UNKNOWN_REVISION          0xC0000058
#define	STATUS_INVALID_SECURITY_DESCR    0xC0000079
#define STATUS_ARRAY_BOUNDS_EXCEEDED     0xC000008C    
#define STATUS_FLOAT_DENORMAL_OPERAND    0xC000008D    
#define STATUS_FLOAT_DIVIDE_BY_ZERO      0xC000008E    
#define STATUS_FLOAT_INEXACT_RESULT      0xC000008F    
#define STATUS_FLOAT_INVALID_OPERATION   0xC0000090    
#define STATUS_FLOAT_OVERFLOW            0xC0000091    
#define STATUS_FLOAT_STACK_CHECK         0xC0000092    
#define STATUS_FLOAT_UNDERFLOW           0xC0000093    
#define STATUS_INTEGER_DIVIDE_BY_ZERO    0xC0000094    
#define STATUS_INTEGER_OVERFLOW          0xC0000095    
#define STATUS_PRIVILEGED_INSTRUCTION    0xC0000096    
#define	STATUS_INVALID_PARAMETER_2       0xC00000F0
#define STATUS_STACK_OVERFLOW            0xC00000FD    
#define STATUS_CONTROL_C_EXIT            0xC000013A    

#define DUPLICATE_CLOSE_SOURCE		0x00000001
#define DUPLICATE_SAME_ACCESS		0x00000002

#define HANDLE_FLAG_INHERIT             0x00000001
#define HANDLE_FLAG_PROTECT_FROM_CLOSE  0x00000002

#define HINSTANCE_ERROR 32

#define THREAD_PRIORITY_LOWEST          THREAD_BASE_PRIORITY_MIN
#define THREAD_PRIORITY_BELOW_NORMAL    (THREAD_PRIORITY_LOWEST+1)
#define THREAD_PRIORITY_NORMAL          0
#define THREAD_PRIORITY_HIGHEST         THREAD_BASE_PRIORITY_MAX
#define THREAD_PRIORITY_ABOVE_NORMAL    (THREAD_PRIORITY_HIGHEST-1)
#define THREAD_PRIORITY_ERROR_RETURN    (0x7fffffff)
#define THREAD_PRIORITY_TIME_CRITICAL   THREAD_BASE_PRIORITY_LOWRT
#define THREAD_PRIORITY_IDLE            THREAD_BASE_PRIORITY_IDLE

typedef struct 
{
  int type;
} wine_exception;

typedef struct 
{
  int pad[39];
  int edi;
  int esi;
  int ebx;
  int edx;
  int ecx;
  int eax;

  int ebp;
  int eip;
  int cs;
  int eflags;
  int esp;
  int ss;
} exception_info;

/* Could this type be considered opaque? */
typedef struct {
	LPVOID	DebugInfo;
	LONG LockCount;
	LONG RecursionCount;
	HANDLE OwningThread;
	HANDLE LockSemaphore;
	DWORD Reserved;
}CRITICAL_SECTION;

typedef struct {
        DWORD dwOSVersionInfoSize;
        DWORD dwMajorVersion;
        DWORD dwMinorVersion;
        DWORD dwBuildNumber;
        DWORD dwPlatformId;
        CHAR szCSDVersion[128];
} OSVERSIONINFO16;

typedef struct {
	DWORD dwOSVersionInfoSize;
	DWORD dwMajorVersion;
	DWORD dwMinorVersion;
	DWORD dwBuildNumber;
	DWORD dwPlatformId;
	CHAR szCSDVersion[128];
} OSVERSIONINFOA;

typedef struct {
	DWORD dwOSVersionInfoSize;
	DWORD dwMajorVersion;
	DWORD dwMinorVersion;
	DWORD dwBuildNumber;
	DWORD dwPlatformId;
	WCHAR szCSDVersion[128];
} OSVERSIONINFOW;

DECL_WINELIB_TYPE_AW(OSVERSIONINFO)

#define VER_PLATFORM_WIN32s             0
#define VER_PLATFORM_WIN32_WINDOWS      1
#define VER_PLATFORM_WIN32_NT           2

typedef struct tagCOMSTAT
{
    BYTE   status;
    UINT16 cbInQue WINE_PACKED;
    UINT16 cbOutQue WINE_PACKED;
} COMSTAT,*LPCOMSTAT;

typedef struct tagDCB16
{
    BYTE   Id;
    UINT16 BaudRate WINE_PACKED;
    BYTE   ByteSize;
    BYTE   Parity;
    BYTE   StopBits;
    UINT16 RlsTimeout;
    UINT16 CtsTimeout;
    UINT16 DsrTimeout;

    UINT16 fBinary        :1;
    UINT16 fRtsDisable    :1;
    UINT16 fParity        :1;
    UINT16 fOutxCtsFlow   :1;
    UINT16 fOutxDsrFlow   :1;
    UINT16 fDummy         :2;
    UINT16 fDtrDisable    :1;

    UINT16 fOutX          :1;
    UINT16 fInX           :1;
    UINT16 fPeChar        :1;
    UINT16 fNull          :1;
    UINT16 fChEvt         :1;
    UINT16 fDtrflow       :1;
    UINT16 fRtsflow       :1;
    UINT16 fDummy2        :1;

    CHAR   XonChar;
    CHAR   XoffChar;
    UINT16 XonLim;
    UINT16 XoffLim;
    CHAR   PeChar;
    CHAR   EofChar;
    CHAR   EvtChar;
    UINT16 TxDelay WINE_PACKED;
} DCB16, *LPDCB16;

typedef struct tagDCB
{
    DWORD DCBlength;
    DWORD BaudRate;
    DWORD fBinary               :1;
    DWORD fParity               :1;
    DWORD fOutxCtsFlow          :1;
    DWORD fOutxDsrFlow          :1;
    DWORD fDtrControl           :2;
    DWORD fDsrSensitivity       :1;
    DWORD fTXContinueOnXoff     :1;
    DWORD fOutX                 :1;
    DWORD fInX                  :1;
    DWORD fErrorChar            :1;
    DWORD fNull                 :1;
    DWORD fRtsControl           :2;
    DWORD fAbortOnError         :1;
    DWORD fDummy2               :17;
    WORD wReserved;
    WORD XonLim;
    WORD XoffLim;
    BYTE ByteSize;
    BYTE Parity;
    BYTE StopBits;
    char XonChar;
    char XoffChar;
    char ErrorChar;
    char EofChar;
    char EvtChar;
} DCB, *LPDCB;



typedef struct tagCOMMTIMEOUTS {
	DWORD	ReadIntervalTimeout;
	DWORD	ReadTotalTimeoutMultiplier;
	DWORD	ReadTotalTimeoutConstant;
	DWORD	WriteTotalTimeoutMultiplier;
	DWORD	WriteTotalTimeoutConstant;
} COMMTIMEOUTS,*LPCOMMTIMEOUTS;
  
#pragma pack(4)

typedef VOID (CALLBACK *PAPCFUNC)(ULONG_PTR);

BOOL      WINAPI ClearCommError(INT,LPDWORD,LPCOMSTAT);
BOOL      WINAPI BuildCommDCBA(LPCSTR,LPDCB);
BOOL      WINAPI BuildCommDCBW(LPCWSTR,LPDCB);
#define     BuildCommDCB WINELIB_NAME_AW(BuildCommDCB)
BOOL      WINAPI BuildCommDCBAndTimeoutsA(LPCSTR,LPDCB,LPCOMMTIMEOUTS);
BOOL      WINAPI BuildCommDCBAndTimeoutsW(LPCWSTR,LPDCB,LPCOMMTIMEOUTS);
#define     BuildCommDCBAndTimeouts WINELIB_NAME_AW(BuildCommDCBAndTimeouts)
BOOL      WINAPI GetCommTimeouts(INT,LPCOMMTIMEOUTS);
BOOL      WINAPI SetCommTimeouts(INT,LPCOMMTIMEOUTS);
BOOL      WINAPI GetCommState(INT,LPDCB);
BOOL      WINAPI SetCommState(INT,LPDCB);
BOOL      WINAPI TransmitCommChar(INT,CHAR);

  
/*DWORD WINAPI GetVersion( void );*/
BOOL16 WINAPI GetVersionEx16(OSVERSIONINFO16*);
BOOL WINAPI GetVersionExA(OSVERSIONINFOA*);
BOOL WINAPI GetVersionExW(OSVERSIONINFOW*);
#define GetVersionEx WINELIB_NAME_AW(GetVersionEx)

/*int WinMain(HINSTANCE, HINSTANCE prev, char *cmd, int show);*/

void      WINAPI DeleteCriticalSection(CRITICAL_SECTION *lpCrit);
void      WINAPI EnterCriticalSection(CRITICAL_SECTION *lpCrit);
void      WINAPI InitializeCriticalSection(CRITICAL_SECTION *lpCrit);
void      WINAPI LeaveCriticalSection(CRITICAL_SECTION *lpCrit);
void      WINAPI MakeCriticalSectionGlobal(CRITICAL_SECTION *lpCrit);
HANDLE  WINAPI OpenProcess(DWORD access, BOOL inherit, DWORD id);
BOOL    WINAPI GetProcessWorkingSetSize(HANDLE,LPDWORD,LPDWORD);
DWORD     WINAPI QueueUserAPC(PAPCFUNC,HANDLE,ULONG_PTR);
void      WINAPI RaiseException(DWORD,DWORD,DWORD,const LPDWORD);
BOOL    WINAPI SetProcessWorkingSetSize(HANDLE,DWORD,DWORD);
BOOL    WINAPI TerminateProcess(HANDLE,DWORD);
BOOL    WINAPI TerminateThread(HANDLE,DWORD);


/* GetBinaryType return values.
 */

#define SCS_32BIT_BINARY    0
#define SCS_DOS_BINARY      1
#define SCS_WOW_BINARY      2
#define SCS_PIF_BINARY      3
#define SCS_POSIX_BINARY    4
#define SCS_OS216_BINARY    5

BOOL WINAPI GetBinaryTypeA( LPCSTR lpApplicationName, LPDWORD lpBinaryType );
BOOL WINAPI GetBinaryTypeW( LPCWSTR lpApplicationName, LPDWORD lpBinaryType );
#define GetBinaryType WINELIB_NAME_AW(GetBinaryType)

BOOL16      WINAPI GetWinDebugInfo16(LPWINDEBUGINFO,UINT16);
BOOL16      WINAPI SetWinDebugInfo16(LPWINDEBUGINFO);
/* Declarations for functions that exist only in Win32 */

BOOL      WINAPI AllocConsole(void);
BOOL      WINAPI AreFileApisANSI(void);
BOOL      WINAPI Beep(DWORD,DWORD);
BOOL      WINAPI CloseHandle(HANDLE);
HANDLE    WINAPI ConvertToGlobalHandle(HANDLE hSrc);
BOOL      WINAPI CopyFileA(LPCSTR,LPCSTR,BOOL);
BOOL      WINAPI CopyFileW(LPCWSTR,LPCWSTR,BOOL);
#define     CopyFile WINELIB_NAME_AW(CopyFile)
BOOL      WINAPI CopyFileExA(LPCSTR, LPCSTR, LPPROGRESS_ROUTINE, LPVOID, LPBOOL, DWORD);
BOOL      WINAPI CopyFileExW(LPCWSTR, LPCWSTR, LPPROGRESS_ROUTINE, LPVOID, LPBOOL, DWORD);
#define     CopyFileEx WINELIB_NAME_AW(CopyFileEx)
INT       WINAPI CompareFileTime(LPFILETIME,LPFILETIME);
HANDLE    WINAPI CreateEventA(LPSECURITY_ATTRIBUTES,BOOL,BOOL,LPCSTR);
HANDLE    WINAPI CreateEventW(LPSECURITY_ATTRIBUTES,BOOL,BOOL,LPCWSTR);
#define     CreateEvent WINELIB_NAME_AW(CreateEvent)
HFILE     WINAPI CreateFileA(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,
                                 DWORD,DWORD,HANDLE);
HFILE     WINAPI CreateFileW(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,
                                 DWORD,DWORD,HANDLE);
#define     CreateFile WINELIB_NAME_AW(CreateFile)
HANDLE    WINAPI CreateFileMappingA(HANDLE,LPSECURITY_ATTRIBUTES,DWORD,
                                        DWORD,DWORD,LPCSTR);
HANDLE    WINAPI CreateFileMappingW(HANDLE,LPSECURITY_ATTRIBUTES,DWORD,
                                        DWORD,DWORD,LPCWSTR);
#define     CreateFileMapping WINELIB_NAME_AW(CreateFileMapping)
HANDLE    WINAPI CreateMutexA(LPSECURITY_ATTRIBUTES,BOOL,LPCSTR);
HANDLE    WINAPI CreateMutexW(LPSECURITY_ATTRIBUTES,BOOL,LPCWSTR);
#define     CreateMutex WINELIB_NAME_AW(CreateMutex)
BOOL      WINAPI CreateProcessA(LPCSTR,LPSTR,LPSECURITY_ATTRIBUTES,
                                    LPSECURITY_ATTRIBUTES,BOOL,DWORD,LPVOID,LPCSTR,
                                    LPSTARTUPINFOA,LPPROCESS_INFORMATION);
BOOL      WINAPI CreateProcessW(LPCWSTR,LPWSTR,LPSECURITY_ATTRIBUTES,
                                    LPSECURITY_ATTRIBUTES,BOOL,DWORD,LPVOID,LPCWSTR,
                                    LPSTARTUPINFOW,LPPROCESS_INFORMATION);
#define     CreateProcess WINELIB_NAME_AW(CreateProcess)
HANDLE    WINAPI CreateSemaphoreA(LPSECURITY_ATTRIBUTES,LONG,LONG,LPCSTR);
HANDLE    WINAPI CreateSemaphoreW(LPSECURITY_ATTRIBUTES,LONG,LONG,LPCWSTR);
#define     CreateSemaphore WINELIB_NAME_AW(CreateSemaphore)
HANDLE    WINAPI CreateThread(LPSECURITY_ATTRIBUTES,DWORD,LPTHREAD_START_ROUTINE,LPVOID,DWORD,LPDWORD);
BOOL      WINAPI DisableThreadLibraryCalls(HMODULE);
BOOL      WINAPI DosDateTimeToFileTime(WORD,WORD,LPFILETIME);
BOOL      WINAPI DuplicateHandle(HANDLE,HANDLE,HANDLE,HANDLE*,DWORD,BOOL,DWORD);
BOOL      WINAPI EnumDateFormatsA(DATEFMT_ENUMPROCA lpDateFmtEnumProc, LCID Locale, DWORD dwFlags);
BOOL      WINAPI EnumDateFormatsW(DATEFMT_ENUMPROCW lpDateFmtEnumProc, LCID Locale, DWORD dwFlags);
#define     EnumDateFormats WINELIB_NAME_AW(EnumDateFormats)
BOOL      WINAPI EnumResourceLanguagesA(HMODULE,LPCSTR,LPCSTR,
                                            ENUMRESLANGPROCA,LONG);
BOOL      WINAPI EnumResourceLanguagesW(HMODULE,LPCWSTR,LPCWSTR,
                                            ENUMRESLANGPROCW,LONG);
#define     EnumResourceLanguages WINELIB_NAME_AW(EnumResourceLanguages)
BOOL      WINAPI EnumResourceNamesA(HMODULE,LPCSTR,ENUMRESNAMEPROCA,
                                        LONG);
BOOL      WINAPI EnumResourceNamesW(HMODULE,LPCWSTR,ENUMRESNAMEPROCW,
                                        LONG);
#define     EnumResourceNames WINELIB_NAME_AW(EnumResourceNames)
BOOL      WINAPI EnumResourceTypesA(HMODULE,ENUMRESTYPEPROCA,LONG);
BOOL      WINAPI EnumResourceTypesW(HMODULE,ENUMRESTYPEPROCW,LONG);
#define     EnumResourceTypes WINELIB_NAME_AW(EnumResourceTypes)
BOOL      WINAPI EnumSystemCodePagesA(CODEPAGE_ENUMPROCA,DWORD);
BOOL      WINAPI EnumSystemCodePagesW(CODEPAGE_ENUMPROCW,DWORD);
#define     EnumSystemCodePages WINELIB_NAME_AW(EnumSystemCodePages)
BOOL      WINAPI EnumSystemLocalesA(LOCALE_ENUMPROCA,DWORD);
BOOL      WINAPI EnumSystemLocalesW(LOCALE_ENUMPROCW,DWORD);
#define     EnumSystemLocales WINELIB_NAME_AW(EnumSystemLocales)
BOOL      WINAPI EnumTimeFormatsA(TIMEFMT_ENUMPROCA lpTimeFmtEnumProc, LCID Locale, DWORD dwFlags);
BOOL      WINAPI EnumTimeFormatsW(TIMEFMT_ENUMPROCW lpTimeFmtEnumProc, LCID Locale, DWORD dwFlags);
#define     EnumTimeFormats WINELIB_NAME_AW(EnumTimeFormats)
VOID        WINAPI ExitProcess(DWORD);
VOID        WINAPI ExitThread(DWORD);
DWORD       WINAPI ExpandEnvironmentStringsA(LPCSTR,LPSTR,DWORD);
DWORD       WINAPI ExpandEnvironmentStringsW(LPCWSTR,LPWSTR,DWORD);
#define     ExpandEnvironmentStrings WINELIB_NAME_AW(ExpandEnvironmentStrings)
BOOL      WINAPI FileTimeToDosDateTime(const FILETIME*,LPWORD,LPWORD);
BOOL      WINAPI FileTimeToLocalFileTime(const FILETIME*,LPFILETIME);
BOOL      WINAPI FileTimeToSystemTime(const FILETIME*,LPSYSTEMTIME);
HANDLE    WINAPI FindFirstChangeNotificationA(LPCSTR,BOOL,DWORD);
HANDLE    WINAPI FindFirstChangeNotificationW(LPCWSTR,BOOL,DWORD);
#define     FindFirstChangeNotification WINELIB_NAME_AW(FindFirstChangeNotification)
BOOL      WINAPI FindNextChangeNotification(HANDLE);
BOOL      WINAPI FindCloseChangeNotification(HANDLE);
HRSRC     WINAPI FindResourceExA(HMODULE,LPCSTR,LPCSTR,WORD);
HRSRC     WINAPI FindResourceExW(HMODULE,LPCWSTR,LPCWSTR,WORD);
#define     FindResourceEx WINELIB_NAME_AW(FindResourceEx)
BOOL      WINAPI FlushConsoleInputBuffer(HANDLE);
BOOL      WINAPI FlushFileBuffers(HFILE);
BOOL      WINAPI FlushViewOfFile(LPCVOID, DWORD);
DWORD       WINAPI FormatMessageA(DWORD,LPCVOID,DWORD,DWORD,LPSTR,
				    DWORD,LPDWORD);
DWORD       WINAPI FormatMessageW(DWORD,LPCVOID,DWORD,DWORD,LPWSTR,
				    DWORD,LPDWORD);
#define     FormatMessage WINELIB_NAME_AW(FormatMessage)
BOOL      WINAPI FreeConsole(void);
BOOL      WINAPI FreeEnvironmentStringsA(LPSTR);
BOOL      WINAPI FreeEnvironmentStringsW(LPWSTR);
#define     FreeEnvironmentStrings WINELIB_NAME_AW(FreeEnvironmentStrings)
UINT      WINAPI GetACP(void);
LPCSTR      WINAPI GetCommandLineA(void);
LPCWSTR     WINAPI GetCommandLineW(void);
#define     GetCommandLine WINELIB_NAME_AW(GetCommandLine)
BOOL      WINAPI GetComputerNameA(LPSTR,LPDWORD);
BOOL      WINAPI GetComputerNameW(LPWSTR,LPDWORD);
#define     GetComputerName WINELIB_NAME_AW(GetComputerName)
UINT      WINAPI GetConsoleCP(void);
BOOL      WINAPI GetConsoleMode(HANDLE,LPDWORD);
UINT      WINAPI GetConsoleOutputCP(void);
DWORD       WINAPI GetConsoleTitleA(LPSTR,DWORD);
DWORD       WINAPI GetConsoleTitleW(LPWSTR,DWORD);
#define     GetConsoleTitle WINELIB_NAME_AW(GetConsoleTitle)
BOOL      WINAPI GetCPInfo(UINT,LPCPINFO);
BOOL      WINAPI GetCommMask(HANDLE, LPDWORD);
BOOL      WINAPI GetCommModemStatus(HANDLE, LPDWORD);
HANDLE    WINAPI GetCurrentProcess(void);
DWORD       WINAPI GetCurrentProcessId(void);
HANDLE    WINAPI GetCurrentThread(void);
DWORD       WINAPI GetCurrentThreadId(void);
INT       WINAPI GetDateFormatA(LCID,DWORD,LPSYSTEMTIME,LPCSTR,LPSTR,INT);
INT       WINAPI GetDateFormatW(LCID,DWORD,LPSYSTEMTIME,LPCWSTR,LPWSTR,INT);
#define     GetDateFormat WINELIB_NAME_AW(GetDateFormat)
LPSTR       WINAPI GetEnvironmentStringsA(void);
LPWSTR      WINAPI GetEnvironmentStringsW(void);
#define     GetEnvironmentStrings WINELIB_NAME_AW(GetEnvironmentStrings)
DWORD       WINAPI GetEnvironmentVariableA(LPCSTR,LPSTR,DWORD);
DWORD       WINAPI GetEnvironmentVariableW(LPCWSTR,LPWSTR,DWORD);
#define     GetEnvironmentVariable WINELIB_NAME_AW(GetEnvironmentVariable)
BOOL      WINAPI GetFileAttributesExA(LPCSTR,GET_FILEEX_INFO_LEVELS,LPVOID);
BOOL      WINAPI GetFileAttributesExW(LPCWSTR,GET_FILEEX_INFO_LEVELS,LPVOID);
#define     GetFileattributesEx WINELIB_NAME_AW(GetFileAttributesEx)
DWORD       WINAPI GetFileInformationByHandle(HFILE,BY_HANDLE_FILE_INFORMATION*);
DWORD       WINAPI GetFileSize(HFILE,LPDWORD);
BOOL      WINAPI GetFileTime(HFILE,LPFILETIME,LPFILETIME,LPFILETIME);
DWORD       WINAPI GetFileType(HFILE);
DWORD       WINAPI GetFullPathNameA(LPCSTR,DWORD,LPSTR,LPSTR*);
DWORD       WINAPI GetFullPathNameW(LPCWSTR,DWORD,LPWSTR,LPWSTR*);
#define     GetFullPathName WINELIB_NAME_AW(GetFullPathName)
BOOL      WINAPI GetHandleInformation(HANDLE,LPDWORD);
DWORD       WINAPI GetLargestConsoleWindowSize(HANDLE);
VOID        WINAPI GetLocalTime(LPSYSTEMTIME);
DWORD       WINAPI GetLogicalDrives(void);
DWORD       WINAPI GetLongPathNameA(LPCSTR,LPSTR,DWORD);
DWORD       WINAPI GetLongPathNameW(LPCWSTR,LPWSTR,DWORD);
#define     GetLongPathName WINELIB_NAME_AW(GetLongPathName)
BOOL      WINAPI GetNumberOfConsoleInputEvents(HANDLE,LPDWORD);
BOOL      WINAPI GetNumberOfConsoleMouseButtons(LPDWORD);
UINT      WINAPI GetOEMCP(void);
DWORD       WINAPI GetPriorityClass(HANDLE);
HANDLE    WINAPI GetProcessHeap(void);
DWORD       WINAPI GetProcessVersion(DWORD);
DWORD       WINAPI GetShortPathNameA(LPCSTR,LPSTR,DWORD);
DWORD       WINAPI GetShortPathNameW(LPCWSTR,LPWSTR,DWORD);
#define     GetShortPathName WINELIB_NAME_AW(GetShortPathName)
HFILE     WINAPI GetStdHandle(DWORD);
BOOL      WINAPI GetStringTypeExA(LCID,DWORD,LPCSTR,INT,LPWORD);
BOOL      WINAPI GetStringTypeExW(LCID,DWORD,LPCWSTR,INT,LPWORD);
#define     GetStringTypeEx WINELIB_NAME_AW(GetStringTypeEx)
VOID        WINAPI GetSystemInfo(LPSYSTEM_INFO);
BOOL      WINAPI GetSystemPowerStatus(LPSYSTEM_POWER_STATUS);
VOID        WINAPI GetSystemTime(LPSYSTEMTIME);
INT       WINAPI GetTimeFormatA(LCID,DWORD,LPSYSTEMTIME,LPCSTR,LPSTR,INT);
INT       WINAPI GetTimeFormatW(LCID,DWORD,LPSYSTEMTIME,LPCWSTR,LPWSTR,INT);
#define     GetTimeFormat WINELIB_NAME_AW(GetTimeFormat)
LCID        WINAPI GetThreadLocale(void);
INT       WINAPI GetThreadPriority(HANDLE);
BOOL      WINAPI GetThreadSelectorEntry(HANDLE,DWORD,LPLDT_ENTRY);
VOID        WINAPI GlobalMemoryStatus(LPMEMORYSTATUS);
LPVOID      WINAPI HeapAlloc(HANDLE,DWORD,DWORD);
DWORD       WINAPI HeapCompact(HANDLE,DWORD);
HANDLE    WINAPI HeapCreate(DWORD,DWORD,DWORD);
BOOL      WINAPI HeapDestroy(HANDLE);
BOOL      WINAPI HeapFree(HANDLE,DWORD,LPVOID);
BOOL      WINAPI HeapLock(HANDLE);
LPVOID      WINAPI HeapReAlloc(HANDLE,DWORD,LPVOID,DWORD);
DWORD       WINAPI HeapSize(HANDLE,DWORD,LPVOID);
BOOL      WINAPI HeapUnlock(HANDLE);
BOOL      WINAPI HeapValidate(HANDLE,DWORD,LPCVOID);
LONG        WINAPI InterlockedDecrement(LPLONG);
LONG        WINAPI InterlockedExchange(LPLONG,LONG);
LONG        WINAPI InterlockedIncrement(LPLONG);
BOOL      WINAPI IsDBCSLeadByteEx(UINT,BYTE);
BOOL      WINAPI IsProcessorFeaturePresent(DWORD);
BOOL      WINAPI IsValidLocale(DWORD,DWORD);
BOOL      WINAPI LocalFileTimeToFileTime(const FILETIME*,LPFILETIME);
BOOL      WINAPI LockFile(HFILE,DWORD,DWORD,DWORD,DWORD);
HMODULE   WINAPI MapHModuleSL(HMODULE16);
HMODULE16   WINAPI MapHModuleLS(HMODULE);
SEGPTR      WINAPI MapLS(LPVOID);
LPVOID      WINAPI MapSL(SEGPTR);
LPVOID      WINAPI MapViewOfFile(HANDLE,DWORD,DWORD,DWORD,DWORD);
LPVOID      WINAPI MapViewOfFileEx(HANDLE,DWORD,DWORD,DWORD,DWORD,LPVOID);
BOOL      WINAPI MoveFileA(LPCSTR,LPCSTR);
BOOL      WINAPI MoveFileW(LPCWSTR,LPCWSTR);
#define     MoveFile WINELIB_NAME_AW(MoveFile)
BOOL      WINAPI MoveFileExA(LPCSTR,LPCSTR,DWORD);
BOOL      WINAPI MoveFileExW(LPCWSTR,LPCWSTR,DWORD);
#define     MoveFileEx WINELIB_NAME_AW(MoveFileEx)
INT       WINAPI MultiByteToWideChar(UINT,DWORD,LPCSTR,INT,LPWSTR,INT);
INT       WINAPI WideCharToMultiByte(UINT,DWORD,LPCWSTR,INT,LPSTR,INT,LPCSTR,BOOL*);
HANDLE    WINAPI OpenEventA(DWORD,BOOL,LPCSTR);
HANDLE    WINAPI OpenEventW(DWORD,BOOL,LPCWSTR);
#define     OpenEvent WINELIB_NAME_AW(OpenEvent)
HANDLE    WINAPI OpenFileMappingA(DWORD,BOOL,LPCSTR);
HANDLE    WINAPI OpenFileMappingW(DWORD,BOOL,LPCWSTR);
#define     OpenFileMapping WINELIB_NAME_AW(OpenFileMapping)
HANDLE    WINAPI OpenMutexA(DWORD,BOOL,LPCSTR);
HANDLE    WINAPI OpenMutexW(DWORD,BOOL,LPCWSTR);
#define     OpenMutex WINELIB_NAME_AW(OpenMutex)
HANDLE    WINAPI OpenProcess(DWORD,BOOL,DWORD);
HANDLE    WINAPI OpenSemaphoreA(DWORD,BOOL,LPCSTR);
HANDLE    WINAPI OpenSemaphoreW(DWORD,BOOL,LPCWSTR);
#define     OpenSemaphore WINELIB_NAME_AW(OpenSemaphore)
BOOL      WINAPI PulseEvent(HANDLE);
BOOL      WINAPI PurgeComm(HANDLE,DWORD);
DWORD       WINAPI QueryDosDeviceA(LPCSTR,LPSTR,DWORD);
DWORD       WINAPI QueryDosDeviceW(LPCWSTR,LPWSTR,DWORD);
#define     QueryDosDevice WINELIB_NAME_AW(QueryDosDevice)
BOOL      WINAPI QueryPerformanceCounter(PLARGE_INTEGER);
BOOL      WINAPI ReadConsoleA(HANDLE,LPVOID,DWORD,LPDWORD,LPVOID);
BOOL      WINAPI ReadConsoleW(HANDLE,LPVOID,DWORD,LPDWORD,LPVOID);
#define     ReadConsole WINELIB_NAME_AW(ReadConsole)
BOOL      WINAPI ReadConsoleOutputCharacterA(HANDLE,LPSTR,DWORD,
						 COORD,LPDWORD);
#define     ReadConsoleOutputCharacter WINELIB_NAME_AW(ReadConsoleOutputCharacter)
BOOL      WINAPI ReadFile(HANDLE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);
BOOL      WINAPI ReleaseMutex(HANDLE);
BOOL      WINAPI ReleaseSemaphore(HANDLE,LONG,LPLONG);
BOOL      WINAPI ResetEvent(HANDLE);
DWORD       WINAPI ResumeThread(HANDLE);
VOID        WINAPI RtlFillMemory(LPVOID,UINT,UINT);
VOID        WINAPI RtlMoveMemory(LPVOID,LPCVOID,UINT);
VOID        WINAPI RtlZeroMemory(LPVOID,UINT);
DWORD       WINAPI SearchPathA(LPCSTR,LPCSTR,LPCSTR,DWORD,LPSTR,LPSTR*);
DWORD       WINAPI SearchPathW(LPCWSTR,LPCWSTR,LPCWSTR,DWORD,LPWSTR,LPWSTR*);
BOOL      WINAPI SetCommMask(INT,DWORD);
BOOL      WINAPI SetComputerNameA(LPCSTR);
BOOL      WINAPI SetComputerNameW(LPCWSTR);
#define     SetComputerName WINELIB_NAME_AW(SetComputerName)
BOOL      WINAPI SetConsoleCursorPosition(HANDLE,COORD);
BOOL      WINAPI SetConsoleMode(HANDLE,DWORD);
BOOL      WINAPI SetConsoleTitleA(LPCSTR);
BOOL      WINAPI SetConsoleTitleW(LPCWSTR);
#define     SetConsoleTitle WINELIB_NAME_AW(SetConsoleTitle)
BOOL      WINAPI SetEndOfFile(HFILE);
BOOL      WINAPI SetEnvironmentVariableA(LPCSTR,LPCSTR);
BOOL      WINAPI SetEnvironmentVariableW(LPCWSTR,LPCWSTR);
#define     SetEnvironmentVariable WINELIB_NAME_AW(SetEnvironmentVariable)
BOOL      WINAPI SetEvent(HANDLE);
VOID        WINAPI SetFileApisToANSI(void);
VOID        WINAPI SetFileApisToOEM(void);
DWORD       WINAPI SetFilePointer(HFILE,LONG,LPLONG,DWORD);
BOOL      WINAPI SetFileTime(HFILE,const FILETIME*,const FILETIME*,
                               const FILETIME*);
BOOL      WINAPI SetHandleInformation(HANDLE,DWORD,DWORD);
BOOL      WINAPI SetPriorityClass(HANDLE,DWORD);
BOOL      WINAPI SetStdHandle(DWORD,HANDLE);
BOOL      WINAPI SetSystemPowerState(BOOL,BOOL);
BOOL      WINAPI SetSystemTime(const SYSTEMTIME*);
DWORD       WINAPI SetThreadAffinityMask(HANDLE,DWORD);
BOOL      WINAPI SetThreadLocale(LCID);
BOOL      WINAPI SetThreadPriority(HANDLE,INT);
BOOL      WINAPI SetTimeZoneInformation(const LPTIME_ZONE_INFORMATION);
VOID        WINAPI Sleep(DWORD);
DWORD       WINAPI SleepEx(DWORD,BOOL);
DWORD       WINAPI SuspendThread(HANDLE);
BOOL      WINAPI SystemTimeToFileTime(const SYSTEMTIME*,LPFILETIME);
DWORD       WINAPI TlsAlloc(void);
BOOL      WINAPI TlsFree(DWORD);
LPVOID      WINAPI TlsGetValue(DWORD);
BOOL      WINAPI TlsSetValue(DWORD,LPVOID);
VOID        WINAPI UnMapLS(SEGPTR);
BOOL      WINAPI UnlockFile(HFILE,DWORD,DWORD,DWORD,DWORD);
BOOL      WINAPI UnmapViewOfFile(LPVOID);
LPVOID      WINAPI VirtualAlloc(LPVOID,DWORD,DWORD,DWORD);
BOOL      WINAPI VirtualFree(LPVOID,DWORD,DWORD);
BOOL      WINAPI VirtualLock(LPVOID,DWORD);
BOOL      WINAPI VirtualProtect(LPVOID,DWORD,DWORD,LPDWORD);
BOOL      WINAPI VirtualProtectEx(HANDLE,LPVOID,DWORD,DWORD,LPDWORD);
DWORD       WINAPI VirtualQuery(LPCVOID,LPMEMORY_BASIC_INFORMATION,DWORD);
DWORD       WINAPI VirtualQueryEx(HANDLE,LPCVOID,LPMEMORY_BASIC_INFORMATION,DWORD);
BOOL      WINAPI VirtualUnlock(LPVOID,DWORD);
BOOL      WINAPI WaitCommEvent(HANDLE,LPDWORD,LPOVERLAPPED);
BOOL      WINAPI WaitForDebugEvent(LPDEBUG_EVENT,DWORD);
DWORD       WINAPI WaitForMultipleObjects(DWORD,const HANDLE*,BOOL,DWORD);
DWORD       WINAPI WaitForMultipleObjectsEx(DWORD,const HANDLE*,BOOL,DWORD,BOOL);
DWORD       WINAPI WaitForSingleObject(HANDLE,DWORD);
DWORD       WINAPI WaitForSingleObjectEx(HANDLE,DWORD,BOOL);
SEGPTR      WINAPI WOWGlobalAllocLock16(DWORD,DWORD,HGLOBAL16*);
DWORD       WINAPI WOWCallback16(FARPROC16,DWORD);
BOOL      WINAPI WOWCallback16Ex(FARPROC16,DWORD,DWORD,LPVOID,LPDWORD);
HANDLE    WINAPI WOWHandle32(WORD,WOW_HANDLE_TYPE);
BOOL      WINAPI WriteConsoleA(HANDLE,LPCVOID,DWORD,LPDWORD,LPVOID);
BOOL      WINAPI WriteConsoleW(HANDLE,LPCVOID,DWORD,LPDWORD,LPVOID);
#define     WriteConsole WINELIB_NAME_AW(WriteConsole)
BOOL      WINAPI WriteFile(HANDLE,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED);
VOID        WINAPI ZeroMemory(LPVOID,UINT);
#define     ZeroMemory RtlZeroMemory
DWORD       WINAPI GetLastError(void);
LANGID      WINAPI GetSystemDefaultLangID(void);
LCID        WINAPI GetSystemDefaultLCID(void);
LANGID      WINAPI GetUserDefaultLangID(void);
LCID        WINAPI GetUserDefaultLCID(void);
VOID        WINAPI SetLastError(DWORD);
ATOM        WINAPI AddAtomA(LPCSTR);
ATOM        WINAPI AddAtomW(LPCWSTR);
#define     AddAtom WINELIB_NAME_AW(AddAtom)
UINT      WINAPI CompareStringA(DWORD,DWORD,LPCSTR,DWORD,LPCSTR,DWORD);
UINT      WINAPI CompareStringW(DWORD,DWORD,LPCWSTR,DWORD,LPCWSTR,DWORD);
#define     CompareString WINELIB_NAME_AW(CompareString)
BOOL      WINAPI CreateDirectoryA(LPCSTR,LPSECURITY_ATTRIBUTES);
BOOL      WINAPI CreateDirectoryW(LPCWSTR,LPSECURITY_ATTRIBUTES);
#define     CreateDirectory WINELIB_NAME_AW(CreateDirectory)
BOOL      WINAPI CreateDirectoryExA(LPCSTR,LPCSTR,LPSECURITY_ATTRIBUTES);
BOOL      WINAPI CreateDirectoryExW(LPCWSTR,LPCWSTR,LPSECURITY_ATTRIBUTES);
#define     CreateDirectoryEx WINELIB_NAME_AW(CreateDirectoryEx)
#define     DefineHandleTable(w) ((w),TRUE)
ATOM        WINAPI DeleteAtom(ATOM);
BOOL      WINAPI DeleteFileA(LPCSTR);
BOOL      WINAPI DeleteFileW(LPCWSTR);
#define     DeleteFile WINELIB_NAME_AW(DeleteFile)
void        WINAPI FatalAppExitA(UINT,LPCSTR);
void        WINAPI FatalAppExitW(UINT,LPCWSTR);
#define     FatalAppExit WINELIB_NAME_AW(FatalAppExit)
ATOM        WINAPI FindAtomA(LPCSTR);
ATOM        WINAPI FindAtomW(LPCWSTR);
#define     FindAtom WINELIB_NAME_AW(FindAtom)
BOOL      WINAPI FindClose(HANDLE);
HANDLE16    WINAPI FindFirstFile16(LPCSTR,LPWIN32_FIND_DATAA);
HANDLE    WINAPI FindFirstFileA(LPCSTR,LPWIN32_FIND_DATAA);
HANDLE    WINAPI FindFirstFileW(LPCWSTR,LPWIN32_FIND_DATAW);
#define     FindFirstFile WINELIB_NAME_AW(FindFirstFile)
BOOL16      WINAPI FindNextFile16(HANDLE16,LPWIN32_FIND_DATAA);
BOOL      WINAPI FindNextFileA(HANDLE,LPWIN32_FIND_DATAA);
BOOL      WINAPI FindNextFileW(HANDLE,LPWIN32_FIND_DATAW);
#define     FindNextFile WINELIB_NAME_AW(FindNextFile)
HRSRC     WINAPI FindResourceA(HMODULE,LPCSTR,LPCSTR);
HRSRC     WINAPI FindResourceW(HMODULE,LPCWSTR,LPCWSTR);
#define     FindResource WINELIB_NAME_AW(FindResource)
VOID        WINAPI FreeLibrary16(HINSTANCE16);
BOOL      WINAPI FreeLibrary(HMODULE);
#define     FreeModule(handle) FreeLibrary(handle)
#define     FreeProcInstance(proc) /*nothing*/
BOOL      WINAPI FreeResource(HGLOBAL);
UINT      WINAPI GetAtomNameA(ATOM,LPSTR,INT);
UINT      WINAPI GetAtomNameW(ATOM,LPWSTR,INT);
#define     GetAtomName WINELIB_NAME_AW(GetAtomName)
UINT      WINAPI GetCurrentDirectoryA(UINT,LPSTR);
UINT      WINAPI GetCurrentDirectoryW(UINT,LPWSTR);
#define     GetCurrentDirectory WINELIB_NAME_AW(GetCurrentDirectory)
BOOL      WINAPI GetDiskFreeSpaceA(LPCSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
BOOL      WINAPI GetDiskFreeSpaceW(LPCWSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
#define     GetDiskFreeSpace WINELIB_NAME_AW(GetDiskFreeSpace)
BOOL      WINAPI GetDiskFreeSpaceExA(LPCSTR,PULARGE_INTEGER,PULARGE_INTEGER,PULARGE_INTEGER);
BOOL      WINAPI GetDiskFreeSpaceExW(LPCWSTR,PULARGE_INTEGER,PULARGE_INTEGER,PULARGE_INTEGER);
#define     GetDiskFreeSpaceEx WINELIB_NAME_AW(GetDiskFreeSpaceEx)
UINT      WINAPI GetDriveTypeA(LPCSTR);
UINT      WINAPI GetDriveTypeW(LPCWSTR);
#define     GetDriveType WINELIB_NAME_AW(GetDriveType)
DWORD       WINAPI GetFileAttributesA(LPCSTR);
DWORD       WINAPI GetFileAttributesW(LPCWSTR);
#define     GetFileAttributes WINELIB_NAME_AW(GetFileAttributes)
#define     GetFreeSpace(w) (0x100000L)
UINT      WINAPI GetLogicalDriveStringsA(UINT,LPSTR);
UINT      WINAPI GetLogicalDriveStringsW(UINT,LPWSTR);
#define     GetLogicalDriveStrings WINELIB_NAME_AW(GetLogicalDriveStrings)
INT       WINAPI GetLocaleInfoA(LCID,LCTYPE,LPSTR,INT);
INT       WINAPI GetLocaleInfoW(LCID,LCTYPE,LPWSTR,INT);
#define     GetLocaleInfo WINELIB_NAME_AW(GetLocaleInfo)
DWORD       WINAPI GetModuleFileNameA(HMODULE,LPSTR,DWORD);
DWORD       WINAPI GetModuleFileNameW(HMODULE,LPWSTR,DWORD);
#define     GetModuleFileName WINELIB_NAME_AW(GetModuleFileName)
HMODULE   WINAPI GetModuleHandleA(LPCSTR);
HMODULE   WINAPI GetModuleHandleW(LPCWSTR);
#define     GetModuleHandle WINELIB_NAME_AW(GetModuleHandle)
UINT      WINAPI GetPrivateProfileIntA(LPCSTR,LPCSTR,INT,LPCSTR);
UINT      WINAPI GetPrivateProfileIntW(LPCWSTR,LPCWSTR,INT,LPCWSTR);
#define     GetPrivateProfileInt WINELIB_NAME_AW(GetPrivateProfileInt)
INT       WINAPI GetPrivateProfileSectionA(LPCSTR,LPSTR,DWORD,LPCSTR);
INT       WINAPI GetPrivateProfileSectionW(LPCWSTR,LPWSTR,DWORD,LPCWSTR);
#define     GetPrivateProfileSection WINELIB_NAME_AW(GetPrivateProfileSection)
DWORD       WINAPI GetPrivateProfileSectionNamesA(LPSTR,DWORD,LPCSTR);
DWORD       WINAPI GetPrivateProfileSectionNamesW(LPWSTR,DWORD,LPCWSTR);
#define     GetPrivateProfileSectionNames WINELIB_NAME_AW(GetPrivateProfileSectionNames)
INT       WINAPI GetPrivateProfileStringA(LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT,LPCSTR);
INT       WINAPI GetPrivateProfileStringW(LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,UINT,LPCWSTR);
#define     GetPrivateProfileString WINELIB_NAME_AW(GetPrivateProfileString)
BOOL      WINAPI GetPrivateProfileStructA(LPCSTR,LPCSTR,LPVOID,UINT,LPCSTR);
BOOL      WINAPI GetPrivateProfileStructW(LPCWSTR,LPCWSTR,LPVOID,UINT,LPCWSTR);
#define     GetPrivateProfileStruct WINELIB_NAME_AW(GetPrivateProfileStruct)
FARPROC   WINAPI GetProcAddress(HMODULE,LPCSTR);
UINT      WINAPI GetProfileIntA(LPCSTR,LPCSTR,INT);
UINT      WINAPI GetProfileIntW(LPCWSTR,LPCWSTR,INT);
#define     GetProfileInt WINELIB_NAME_AW(GetProfileInt)
INT       WINAPI GetProfileSectionA(LPCSTR,LPSTR,DWORD);
INT       WINAPI GetProfileSectionW(LPCWSTR,LPWSTR,DWORD);
#define     GetProfileSection WINELIB_NAME_AW(GetProfileSection)
INT       WINAPI GetProfileStringA(LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT);
INT       WINAPI GetProfileStringW(LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,UINT);
#define     GetProfileString WINELIB_NAME_AW(GetProfileString)
BOOL      WINAPI GetStringTypeA(LCID,DWORD,LPCSTR,INT,LPWORD);
BOOL      WINAPI GetStringTypeW(DWORD,LPCWSTR,INT,LPWORD);
#define     GetStringType WINELIB_NAME_AW(GetStringType)
UINT      WINAPI GetSystemDirectoryA(LPSTR,UINT);
UINT      WINAPI GetSystemDirectoryW(LPWSTR,UINT);
#define     GetSystemDirectory WINELIB_NAME_AW(GetSystemDirectory)
UINT      WINAPI GetTempFileNameA(LPCSTR,LPCSTR,UINT,LPSTR);
UINT      WINAPI GetTempFileNameW(LPCWSTR,LPCWSTR,UINT,LPWSTR);
#define     GetTempFileName WINELIB_NAME_AW(GetTempFileName)
UINT      WINAPI GetTempPathA(UINT,LPSTR);
UINT      WINAPI GetTempPathW(UINT,LPWSTR);
#define     GetTempPath WINELIB_NAME_AW(GetTempPath)
LONG        WINAPI GetVersion(void);
BOOL      WINAPI GetExitCodeProcess(HANDLE,LPDWORD);
BOOL      WINAPI GetVolumeInformationA(LPCSTR,LPSTR,DWORD,LPDWORD,LPDWORD,LPDWORD,LPSTR,DWORD);
BOOL      WINAPI GetVolumeInformationW(LPCWSTR,LPWSTR,DWORD,LPDWORD,LPDWORD,LPDWORD,LPWSTR,DWORD);
#define     GetVolumeInformation WINELIB_NAME_AW(GetVolumeInformation)
UINT      WINAPI GetWindowsDirectoryA(LPSTR,UINT);
UINT      WINAPI GetWindowsDirectoryW(LPWSTR,UINT);
#define     GetWindowsDirectory WINELIB_NAME_AW(GetWindowsDirectory)
HGLOBAL16   WINAPI GlobalAlloc16(UINT16,DWORD);
HGLOBAL   WINAPI GlobalAlloc(UINT,DWORD);
DWORD       WINAPI GlobalCompact(DWORD);
UINT      WINAPI GlobalFlags(HGLOBAL);
HGLOBAL16   WINAPI GlobalFree16(HGLOBAL16);
HGLOBAL   WINAPI GlobalFree(HGLOBAL);
HGLOBAL   WINAPI GlobalHandle(LPCVOID);
WORD        WINAPI GlobalFix16(HGLOBAL16);
VOID        WINAPI GlobalFix(HGLOBAL);
LPVOID      WINAPI GlobalLock16(HGLOBAL16);
LPVOID      WINAPI GlobalLock(HGLOBAL);
HGLOBAL   WINAPI GlobalReAlloc(HGLOBAL,DWORD,UINT);
DWORD       WINAPI GlobalSize16(HGLOBAL16);
DWORD       WINAPI GlobalSize(HGLOBAL);
VOID        WINAPI GlobalUnfix16(HGLOBAL16);
VOID        WINAPI GlobalUnfix(HGLOBAL);
BOOL16      WINAPI GlobalUnlock16(HGLOBAL16);
BOOL      WINAPI GlobalUnlock(HGLOBAL);
BOOL16      WINAPI GlobalUnWire16(HGLOBAL16);
BOOL      WINAPI GlobalUnWire(HGLOBAL);
SEGPTR      WINAPI GlobalWire16(HGLOBAL16);
LPVOID      WINAPI GlobalWire(HGLOBAL);
BOOL      WINAPI InitAtomTable(DWORD);
BOOL      WINAPI IsBadCodePtr(FARPROC);
BOOL      WINAPI IsBadHugeReadPtr(LPCVOID,UINT);
BOOL      WINAPI IsBadHugeWritePtr(LPVOID,UINT);
BOOL      WINAPI IsBadReadPtr(LPCVOID,UINT);
BOOL      WINAPI IsBadStringPtrA(LPCSTR,UINT);
BOOL      WINAPI IsBadStringPtrW(LPCWSTR,UINT);
#define     IsBadStringPtr WINELIB_NAME_AW(IsBadStringPtr)
BOOL      WINAPI IsBadWritePtr(LPVOID,UINT);
BOOL      WINAPI IsDBCSLeadByte(BYTE);
HINSTANCE16 WINAPI LoadLibrary16(LPCSTR);
HMODULE   WINAPI LoadLibraryA(LPCSTR);
HMODULE   WINAPI LoadLibraryW(LPCWSTR);
#define     LoadLibrary WINELIB_NAME_AW(LoadLibrary)
HMODULE   WINAPI LoadLibraryExA(LPCSTR,HFILE,DWORD);
HMODULE   WINAPI LoadLibraryExW(LPCWSTR,HFILE,DWORD);
#define     LoadLibraryEx WINELIB_NAME_AW(LoadLibraryEx)
HINSTANCE16 WINAPI LoadModule16(LPCSTR,LPVOID);
HINSTANCE WINAPI LoadModule(LPCSTR,LPVOID);
HGLOBAL   WINAPI LoadResource(HMODULE,HRSRC);
HLOCAL    WINAPI LocalAlloc(UINT,DWORD);
UINT      WINAPI LocalCompact(UINT);
UINT      WINAPI LocalFlags(HLOCAL);
HLOCAL    WINAPI LocalFree(HLOCAL);
HLOCAL    WINAPI LocalHandle(LPCVOID);
LPVOID      WINAPI LocalLock(HLOCAL);
HLOCAL    WINAPI LocalReAlloc(HLOCAL,DWORD,UINT);
UINT      WINAPI LocalShrink(HGLOBAL,UINT);
UINT      WINAPI LocalSize(HLOCAL);
BOOL      WINAPI LocalUnlock(HLOCAL);
LPVOID      WINAPI LockResource(HGLOBAL);
#define     LockSegment(handle) GlobalFix((HANDLE)(handle))
#define     MakeProcInstance(proc,inst) (proc)
HFILE16     WINAPI OpenFile16(LPCSTR,OFSTRUCT*,UINT16);
HFILE     WINAPI OpenFile(LPCSTR,OFSTRUCT*,UINT);
VOID        WINAPI OutputDebugStringA(LPCSTR);
VOID        WINAPI OutputDebugStringW(LPCWSTR);
#define     OutputDebugString WINELIB_NAME_AW(OutputDebugString)
BOOL      WINAPI RemoveDirectoryA(LPCSTR);
BOOL      WINAPI RemoveDirectoryW(LPCWSTR);
#define     RemoveDirectory WINELIB_NAME_AW(RemoveDirectory)
BOOL      WINAPI SetCurrentDirectoryA(LPCSTR);
BOOL      WINAPI SetCurrentDirectoryW(LPCWSTR);
#define     SetCurrentDirectory WINELIB_NAME_AW(SetCurrentDirectory)
UINT      WINAPI SetErrorMode(UINT);
BOOL      WINAPI SetFileAttributesA(LPCSTR,DWORD);
BOOL      WINAPI SetFileAttributesW(LPCWSTR,DWORD);
#define     SetFileAttributes WINELIB_NAME_AW(SetFileAttributes)
UINT      WINAPI SetHandleCount(UINT);
#define     SetSwapAreaSize(w) (w)
DWORD       WINAPI SizeofResource(HMODULE,HRSRC);
#define     UnlockSegment(handle) GlobalUnfix((HANDLE)(handle))
DWORD       WINAPI VerLanguageNameA(UINT,LPSTR,UINT);
DWORD       WINAPI VerLanguageNameW(UINT,LPWSTR,UINT);
#define     VerLanguageName WINELIB_NAME_AW(VerLanguageName)
BOOL      WINAPI WritePrivateProfileSectionA(LPCSTR,LPCSTR,LPCSTR);
BOOL      WINAPI WritePrivateProfileSectionW(LPCWSTR,LPCWSTR,LPCWSTR);
#define     WritePrivateProfileSection WINELIB_NAME_AW(WritePrivateProfileSection)
BOOL      WINAPI WritePrivateProfileStringA(LPCSTR,LPCSTR,LPCSTR,LPCSTR);
BOOL      WINAPI WritePrivateProfileStringW(LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR);
BOOL	     WINAPI WriteProfileSectionA(LPCSTR,LPCSTR);
BOOL	     WINAPI WriteProfileSectionW(LPCWSTR,LPCWSTR);
#define     WritePrivateProfileString WINELIB_NAME_AW(WritePrivateProfileString)
BOOL      WINAPI WritePrivateProfileStructA(LPCSTR,LPCSTR,LPVOID,UINT,LPCSTR);
BOOL      WINAPI WritePrivateProfileStructW(LPCWSTR,LPCWSTR,LPVOID,UINT,LPCWSTR);
#define     WritePrivateProfileStruct WINELIB_NAME_AW(WritePrivateProfileStruct)
BOOL      WINAPI WriteProfileStringA(LPCSTR,LPCSTR,LPCSTR);
BOOL      WINAPI WriteProfileStringW(LPCWSTR,LPCWSTR,LPCWSTR);
#define     WriteProfileString WINELIB_NAME_AW(WriteProfileString)
#define     Yield32()
LPSTR       WINAPI lstrcatA(LPSTR,LPCSTR);
LPWSTR      WINAPI lstrcatW(LPWSTR,LPCWSTR);
#define     lstrcat WINELIB_NAME_AW(lstrcat)
LPSTR       WINAPI lstrcatnA(LPSTR,LPCSTR,INT);
LPWSTR      WINAPI lstrcatnW(LPWSTR,LPCWSTR,INT);
#define     lstrcatn WINELIB_NAME_AW(lstrcatn)
LPSTR       WINAPI lstrcpyA(LPSTR,LPCSTR);
LPWSTR      WINAPI lstrcpyW(LPWSTR,LPCWSTR);
#define     lstrcpy WINELIB_NAME_AW(lstrcpy)
LPSTR       WINAPI lstrcpynA(LPSTR,LPCSTR,INT);
LPWSTR      WINAPI lstrcpynW(LPWSTR,LPCWSTR,INT);
#define     lstrcpyn WINELIB_NAME_AW(lstrcpyn)
INT       WINAPI lstrlenA(LPCSTR);
INT       WINAPI lstrlenW(LPCWSTR);
#define     lstrlen WINELIB_NAME_AW(lstrlen)
HINSTANCE WINAPI WinExec(LPCSTR,UINT);
LONG        WINAPI _hread(HFILE,LPVOID,LONG);
LONG        WINAPI _hwrite(HFILE,LPCSTR,LONG);
HFILE     WINAPI _lcreat(LPCSTR,INT);
HFILE     WINAPI _lclose(HFILE);
LONG        WINAPI _llseek(HFILE,LONG,INT);
HFILE     WINAPI _lopen(LPCSTR,INT);
UINT      WINAPI _lread(HFILE,LPVOID,UINT);
UINT      WINAPI _lwrite(HFILE,LPCSTR,UINT);
SEGPTR      WINAPI WIN16_GlobalLock16(HGLOBAL16);
INT       WINAPI lstrcmpA(LPCSTR,LPCSTR);
INT       WINAPI lstrcmpW(LPCWSTR,LPCWSTR);
#define     lstrcmp WINELIB_NAME_AW(lstrcmp)
INT       WINAPI lstrcmpiA(LPCSTR,LPCSTR);
INT       WINAPI lstrcmpiW(LPCWSTR,LPCWSTR);
#define     lstrcmpi WINELIB_NAME_AW(lstrcmpi)


#ifdef __cplusplus
}
#endif

#endif  /* __WINE_WINBASE_H */
