/*
 * Win32 definitions for Windows NT
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_WINNT_H
#define __WINE_WINNT_H

#include "basetsd.h"

#ifndef RC_INVOKED
#include <ctype.h>
#include <stddef.h>
#include <string.h>
#endif


/* On Windows winnt.h depends on a few windef.h types and macros and thus 
 * is not self-contained. Furthermore windef.h includes winnt.h so that it 
 * would be pointless to try to use winnt.h directly.
 * But for Wine and Winelib I decided to make winnt.h self-contained by 
 * moving these definitions to winnt.h. It makes no difference to Winelib 
 * programs since they are not using winnt.h directly anyway, and it allows 
 * us to use winnt.h and get a minimal set of definitions.
 */

/**** Some Wine specific definitions *****/

/* Architecture dependent settings. */
/* These are hardcoded to avoid dependencies on config.h in Winelib apps. */
#if defined(__i386__)
# undef  WORDS_BIGENDIAN
# undef  BITFIELDS_BIGENDIAN
# define ALLOW_UNALIGNED_ACCESS
#elif defined(__sparc__)
# define WORDS_BIGENDIAN
# define BITFIELDS_BIGENDIAN
# undef  ALLOW_UNALIGNED_ACCESS
#elif defined(__PPC__)
# define WORDS_BIGENDIAN
# define BITFIELDS_BIGENDIAN
# undef  ALLOW_UNALIGNED_ACCESS
#elif !defined(RC_INVOKED)
# error Unknown CPU architecture!
#endif


/* Calling conventions definitions */

#ifdef __i386__
# ifndef _X86_
#  define _X86_
# endif
# if defined(__GNUC__) && ((__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 7)))
#  define __stdcall __attribute__((__stdcall__))
#  define __cdecl   __attribute__((__cdecl__))
# else
#  error You need gcc >= 2.7 to build Wine on a 386
# endif  /* __GNUC__ */
#else  /* __i386__ */
# define __stdcall
# define __cdecl
#endif  /* __i386__ */

#ifndef __WINE__
#define pascal      __stdcall
#define _pascal     __stdcall
#define _stdcall    __stdcall
#define _fastcall   __stdcall
#define __fastcall  __stdcall
#define __export    __stdcall
#define cdecl       __cdecl
#define _cdecl      __cdecl

#define near
#define far
#define _near
#define _far
#define NEAR
#define FAR

#ifndef _declspec
#define _declspec(x)
#endif
#ifndef __declspec
#define __declspec(x)
#endif
#endif /* __WINE__ */

#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define APIPRIVATE  __stdcall
#define PASCAL      __stdcall
#define CDECL       __cdecl
#define _CDECL      __cdecl
#define WINAPIV     __cdecl
#define APIENTRY    WINAPI
#define CONST       const

/* Macro for structure packing and more. */

#ifdef __GNUC__
#define WINE_PACKED   __attribute__((packed))
#define WINE_UNUSED   __attribute__((unused))
#define WINE_NORETURN __attribute__((noreturn))
#else
#define WINE_PACKED    /* nothing */
#define WINE_UNUSED    /* nothing */
#define WINE_NORETURN  /* nothing */
#endif

/* Anonymous union/struct handling */

#ifdef __WINE__
# define NONAMELESSSTRUCT
# define NONAMELESSUNION
#else
/* Anonymous struct support starts with gcc/g++ 2.96 */
# if !defined(NONAMELESSSTRUCT) && defined(__GNUC__) && ((__GNUC__ < 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ < 96)))
#  define NONAMELESSSTRUCT
# endif
/* Anonymous unions support starts with gcc 2.96/g++ 2.95 */
# if !defined(NONAMELESSUNION) && defined(__GNUC__) && ((__GNUC__ < 2) || ((__GNUC__ == 2) && ((__GNUC_MINOR__ < 95) || ((__GNUC_MINOR__ == 95) && !defined(__cplusplus)))))
#  define NONAMELESSUNION
# endif
#endif

#ifndef NONAMELESSSTRUCT
#define DUMMYSTRUCTNAME
#define DUMMYSTRUCTNAME1
#define DUMMYSTRUCTNAME2
#define DUMMYSTRUCTNAME3
#define DUMMYSTRUCTNAME4
#define DUMMYSTRUCTNAME5
#else /* !defined(NONAMELESSSTRUCT) */
#define DUMMYSTRUCTNAME   s
#define DUMMYSTRUCTNAME1  s1
#define DUMMYSTRUCTNAME2  s2
#define DUMMYSTRUCTNAME3  s3
#define DUMMYSTRUCTNAME4  s4
#define DUMMYSTRUCTNAME5  s5
#endif /* !defined(NONAMELESSSTRUCT) */

#ifndef NONAMELESSUNION
#define DUMMYUNIONNAME
#define DUMMYUNIONNAME1
#define DUMMYUNIONNAME2
#define DUMMYUNIONNAME3
#define DUMMYUNIONNAME4
#define DUMMYUNIONNAME5
#define DUMMYUNIONNAME6
#define DUMMYUNIONNAME7
#define DUMMYUNIONNAME8
#else /* !defined(NONAMELESSUNION) */
#define DUMMYUNIONNAME   u
#define DUMMYUNIONNAME1  u1
#define DUMMYUNIONNAME2  u2
#define DUMMYUNIONNAME3  u3
#define DUMMYUNIONNAME4  u4
#define DUMMYUNIONNAME5  u5
#define DUMMYUNIONNAME6  u6
#define DUMMYUNIONNAME7  u7
#define DUMMYUNIONNAME8  u8
#endif /* !defined(NONAMELESSUNION) */


/**** Parts of windef.h that are needed here *****/

/* Misc. constants. */

#undef NULL
#ifdef __cplusplus
#define NULL  0
#else
#define NULL  ((void*)0)
#endif

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0

#ifdef TRUE
#undef TRUE
#endif
#define TRUE  1

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

/* Standard data types */
typedef const void                  *PCVOID,   *LPCVOID;
typedef int             BOOL,       *PBOOL,    *LPBOOL;
typedef unsigned char   BYTE,       *PBYTE,    *LPBYTE;
typedef unsigned char   UCHAR,      *PUCHAR;
typedef unsigned short  USHORT,     *PUSHORT,  *LPUSHORT;
typedef unsigned short  WORD,       *PWORD,    *LPWORD;
typedef int             INT,        *PINT,     *LPINT;
typedef unsigned int    UINT,       *PUINT,    *LPUINT;
typedef unsigned long   DWORD,      *PDWORD,   *LPDWORD;
typedef unsigned long   ULONG,      *PULONG,   *LPULONG;
typedef float           FLOAT,      *PFLOAT,   *LPFLOAT;
typedef double          DOUBLE,     *PDOUBLE,  *LPDOUBLE;
typedef double          DATE;


/**** winnt.h proper *****/

/* Microsoft's macros for declaring functions */

#ifdef __cplusplus
# define EXTERN_C    extern "C"
#else
# define EXTERN_C    extern
#endif

#ifndef __WINE__
#define STDMETHODCALLTYPE       __stdcall
#define STDMETHODVCALLTYPE      __cdecl
#define STDAPICALLTYPE          __stdcall
#define STDAPIVCALLTYPE         __cdecl

#define STDAPI                  EXTERN_C HRESULT STDAPICALLTYPE
#define STDAPI_(type)           EXTERN_C type STDAPICALLTYPE
#define STDMETHODIMP            HRESULT STDMETHODCALLTYPE
#define STDMETHODIMP_(type)     type STDMETHODCALLTYPE
#define STDAPIV                 EXTERN_C HRESULT STDAPIVCALLTYPE
#define STDAPIV_(type)          EXTERN_C type STDAPIVCALLTYPE
#define STDMETHODIMPV           HRESULT STDMETHODVCALLTYPE
#define STDMETHODIMPV_(type)    type STDMETHODVCALLTYPE
#endif

/* Define the basic types */
#ifndef VOID
#define VOID void
#endif
typedef VOID           *PVOID,      *LPVOID;
typedef BYTE            BOOLEAN,    *PBOOLEAN;
typedef char            CHAR,       *PCHAR;
typedef short           SHORT,      *PSHORT;
typedef long            LONG,       *PLONG,    *LPLONG;

/* Some systems might have wchar_t, but we really need 16 bit characters */
#ifndef WINE_WCHAR_DEFINED
#ifdef WINE_UNICODE_NATIVE
typedef wchar_t         WCHAR,      *PWCHAR;
#else
typedef unsigned short  WCHAR,      *PWCHAR;
#endif
#define WINE_WCHAR_DEFINED
#endif

/* 'Extended/Wide' numerical types */
#ifndef _ULONGLONG_
#define _ULONGLONG_
typedef __int64         LONGLONG,   *PLONGLONG;
typedef __uint64        ULONGLONG,  *PULONGLONG;
#endif

#ifndef _DWORDLONG_
#define _DWORDLONG_
typedef ULONGLONG       DWORDLONG,  *PDWORDLONG;
#endif

/* ANSI string types */
typedef CHAR           *PCH,        *LPCH;
typedef const CHAR     *PCCH,       *LPCCH;
typedef CHAR           *PSTR,       *LPSTR;
typedef const CHAR     *PCSTR,      *LPCSTR;

/* Unicode string types */
typedef WCHAR          *PWCH,       *LPWCH;
typedef const WCHAR    *PCWCH,      *LPCWCH;
typedef WCHAR          *PWSTR,      *LPWSTR;
typedef const WCHAR    *PCWSTR,     *LPCWSTR;

/* Neutral character and string types */
/* These are only defined for Winelib, i.e. _not_ defined for 
 * the emulator. The reason is they depend on the UNICODE 
 * macro which only exists in the user's code.
 */
#ifndef __WINE__
# ifdef WINE_UNICODE_REWRITE

/* Use this if your compiler does not provide a 16bit wchar_t type. 
 * Note that you will need to specify -fwritable-strings or an option
 * to this effect.
 * In C++ both WINE_UNICODE_TEXT('c') and WINE_UNICODE_TEXT("str") are 
 * supported, but only the string form can be supported in C.
 */
EXTERN_C unsigned short* wine_rewrite_s4tos2(const wchar_t* str4);
#  ifdef __cplusplus
inline WCHAR* wine_unicode_text(const wchar_t* str4)
{
  return (WCHAR*)wine_rewrite_s4tos2(str4);
}
inline WCHAR wine_unicode_text(wchar_t chr4)
{
  return (WCHAR)chr4;
}
#   define WINE_UNICODE_TEXT(x)       wine_unicode_text(L##x)
#  else  /* __cplusplus */
#   define WINE_UNICODE_TEXT(x)       ((WCHAR*)wine_rewrite_s4tos2(L##x))
#  endif  /* __cplusplus */

# else  /* WINE_UNICODE_REWRITE */

/* Define WINE_UNICODE_NATIVE if:
 * - your compiler provides a 16bit wchar_t type, e.g. gcc >= 2.96 with 
 *   -fshort-wchar option
 * - or if you decide to use the native 32bit Unix wchar_t type. Be aware 
 *   though that the Wine APIs only support 16bit WCHAR characters for 
 *   binary compatibility reasons.
 * - or define nothing at all if you don't use Unicode, and blissfully 
 *   ignore the issue :-)
 */
#  define WINE_UNICODE_TEXT(string)   L##string

# endif  /* WINE_UNICODE_REWRITE */

# ifdef UNICODE
typedef WCHAR           TCHAR,      *PTCHAR;
typedef LPWSTR          PTSTR,       LPTSTR;
typedef LPCWSTR         PCTSTR,      LPCTSTR;
#  define __TEXT(string) WINE_UNICODE_TEXT(string)
# else  /* UNICODE */
typedef CHAR            TCHAR,      *PTCHAR;
typedef LPSTR           PTSTR,       LPTSTR;
typedef LPCSTR          PCTSTR,      LPCTSTR;
#  define __TEXT(string) string
# endif /* UNICODE */
# define TEXT(quote) __TEXT(quote)
#endif   /* __WINE__ */

/* Misc common WIN32 types */
typedef LONG            HRESULT;
typedef DWORD           LCID,       *PLCID;
typedef WORD            LANGID;
typedef DWORD		EXECUTION_STATE;

/* Handle type */

/* FIXME: Wine does not compile with strict on, therefore strict
 * handles are presently only usable on machines where sizeof(UINT) ==
 * sizeof(void*).  HANDLEs are supposed to be void* but a large amount
 * of WINE code operates on HANDLES as if they are UINTs. So to WINE
 * they exist as UINTs but to the Winelib user who turns on strict,
 * they exist as void*. If there is a size difference between UINT and
 * void* then things get ugly.
 *
 * Here is the plan to convert Wine to STRICT:
 *
 * Types will be converted one at a time by volunteers who will compile
 * Wine with STRICT turned on. Handles that have not been converted yet 
 * will be declared with DECLARE_OLD_HANDLE. Converted handles are 
 * declared with DECLARE_HANDLE.
 * See the bug report 90 for more details:
 *    http://wine.codeweavers.com/bugzilla/show_bug.cgi?id=90
 */
/*
 * when compiling Wine we always treat HANDLE as an UINT. Then when 
 * we're ready we'll remove the '!defined(__WINE__)' (the equivalent 
 * of converting it from DECLARE_OLD_HANDLE to DECLARE_HANDLE).
 */
#if defined(STRICT) && !defined(__WINE__)
typedef VOID*           HANDLE;
#define DECLARE_OLD_HANDLE(a) \
    typedef struct a##__ { int unused; } *a; \
    typedef a          *P##a,       *LP##a

#else
typedef UINT            HANDLE;
#define DECLARE_OLD_HANDLE(a) \
    typedef HANDLE      a; \
    typedef a          *P##a,       *LP##a
#endif
typedef HANDLE         *PHANDLE,    *LPHANDLE;

#ifdef STRICT
#define DECLARE_HANDLE(a) \
    typedef struct a##__ { int unused; } *a; \
    typedef a          *P##a,       *LP##a
#else /*STRICT*/
#define DECLARE_HANDLE(a) \
    typedef HANDLE      a; \
    typedef a          *P##a,       *LP##a
#endif /*STRICT*/


#include "pshpack1.h"
/* Defines */

/* Argument 1 passed to the DllEntryProc. */
#define	DLL_PROCESS_DETACH	0	/* detach process (unload library) */
#define	DLL_PROCESS_ATTACH	1	/* attach process (load library) */
#define	DLL_THREAD_ATTACH	2	/* attach new thread */
#define	DLL_THREAD_DETACH	3	/* detach thread */


/* u.x.wProcessorArchitecture (NT) */
#define PROCESSOR_ARCHITECTURE_INTEL	0
#define PROCESSOR_ARCHITECTURE_MIPS	1
#define PROCESSOR_ARCHITECTURE_ALPHA	2
#define PROCESSOR_ARCHITECTURE_PPC	3
#define PROCESSOR_ARCHITECTURE_SHX	4
#define PROCESSOR_ARCHITECTURE_ARM	5
#define PROCESSOR_ARCHITECTURE_UNKNOWN	0xFFFF

/* dwProcessorType */
#define PROCESSOR_INTEL_386      386
#define PROCESSOR_INTEL_486      486
#define PROCESSOR_INTEL_PENTIUM  586
#define PROCESSOR_INTEL_860      860
#define PROCESSOR_MIPS_R2000     2000
#define PROCESSOR_MIPS_R3000     3000
#define PROCESSOR_MIPS_R4000     4000
#define PROCESSOR_ALPHA_21064    21064
#define PROCESSOR_PPC_601        601
#define PROCESSOR_PPC_603        603
#define PROCESSOR_PPC_604        604
#define PROCESSOR_PPC_620        620
#define PROCESSOR_HITACHI_SH3    10003
#define PROCESSOR_HITACHI_SH3E   10004
#define PROCESSOR_HITACHI_SH4    10005
#define PROCESSOR_MOTOROLA_821   821
#define PROCESSOR_SHx_SH3        103
#define PROCESSOR_SHx_SH4        104
#define PROCESSOR_STRONGARM      2577
#define PROCESSOR_ARM720         1824    /* 0x720 */
#define PROCESSOR_ARM820         2080    /* 0x820 */
#define PROCESSOR_ARM920         2336    /* 0x920 */
#define PROCESSOR_ARM_7TDMI      70001

typedef struct _MEMORY_BASIC_INFORMATION
{
    LPVOID   BaseAddress;
    LPVOID   AllocationBase;
    DWORD    AllocationProtect;
    DWORD    RegionSize;
    DWORD    State;
    DWORD    Protect;
    DWORD    Type;
} MEMORY_BASIC_INFORMATION,*LPMEMORY_BASIC_INFORMATION,*PMEMORY_BASIC_INFORMATION;

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
#define MEM_RESET               0x00080000
#define MEM_TOP_DOWN            0x00100000
#ifdef __WINE__
#define MEM_SYSTEM              0x80000000
#endif

#define SEC_FILE                0x00800000
#define SEC_IMAGE               0x01000000
#define SEC_RESERVE             0x04000000
#define SEC_COMMIT              0x08000000
#define SEC_NOCACHE             0x10000000
#define MEM_IMAGE               SEC_IMAGE


#define MINCHAR       0x80
#define MAXCHAR       0x7f
#define MINSHORT      0x8000
#define MAXSHORT      0x7fff
#define MINLONG       0x80000000
#define MAXLONG       0x7fffffff
#define MAXBYTE       0xff
#define MAXWORD       0xffff
#define MAXDWORD      0xffffffff

#define FIELD_OFFSET(type, field) \
  ((LONG)(INT)&(((type *)0)->field))

#define CONTAINING_RECORD(address, type, field) \
  ((type *)((PCHAR)(address) - (PCHAR)(&((type *)0)->field)))

/* Types */

typedef struct _LIST_ENTRY {
  struct _LIST_ENTRY *Flink;
  struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

typedef struct _SINGLE_LIST_ENTRY {
  struct _SINGLE_LIST_ENTRY *Next;
} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;

/* Heap flags */

#define HEAP_NO_SERIALIZE               0x00000001
#define HEAP_GROWABLE                   0x00000002
#define HEAP_GENERATE_EXCEPTIONS        0x00000004
#define HEAP_ZERO_MEMORY                0x00000008
#define HEAP_REALLOC_IN_PLACE_ONLY      0x00000010
#define HEAP_TAIL_CHECKING_ENABLED      0x00000020
#define HEAP_FREE_CHECKING_ENABLED      0x00000040
#define HEAP_DISABLE_COALESCE_ON_FREE   0x00000080
#define HEAP_CREATE_ALIGN_16            0x00010000
#define HEAP_CREATE_ENABLE_TRACING      0x00020000

/* This flag allows it to create heaps shared by all processes under win95,
   FIXME: correct name */
#define HEAP_SHARED                     0x04000000  

#define HEAP_WINE_SEGPTR                0x10000000  /* Not a Win32 flag */
#define HEAP_WINE_CODESEG               0x20000000  /* Not a Win32 flag */
#define HEAP_WINE_CODE16SEG             0x40000000  /* Not a Win32 flag */

/* Processor feature flags.  */
#define PF_FLOATING_POINT_PRECISION_ERRATA	0
#define PF_FLOATING_POINT_EMULATED		1
#define PF_COMPARE_EXCHANGE_DOUBLE		2
#define PF_MMX_INSTRUCTIONS_AVAILABLE		3
#define PF_PPC_MOVEMEM_64BIT_OK			4
#define PF_ALPHA_BYTE_INSTRUCTIONS		5
#define PF_XMMI_INSTRUCTIONS_AVAILABLE		6
#define PF_AMD3D_INSTRUCTIONS_AVAILABLE		7
#define PF_RDTSC_INSTRUCTION_AVAILABLE		8


/* Execution state flags */
#define ES_SYSTEM_REQUIRED    0x00000001
#define ES_DISPLAY_REQUIRED   0x00000002
#define ES_USER_PRESENT       0x00000004
#define ES_CONTINUOUS         0x80000000

/* The Win32 register context */

/* CONTEXT is the CPU-dependent context; it should be used        */
/* wherever a platform-specific context is needed (e.g. exception */
/* handling, Win32 register functions). */

/* CONTEXT86 is the i386-specific context; it should be used     */
/* wherever only a 386 context makes sense (e.g. DOS interrupts, */
/* Win16 register functions), so that this code can be compiled  */
/* on all platforms. */

#define SIZE_OF_80387_REGISTERS      80

typedef struct _FLOATING_SAVE_AREA
{
    DWORD   ControlWord;
    DWORD   StatusWord;
    DWORD   TagWord;    
    DWORD   ErrorOffset;
    DWORD   ErrorSelector;
    DWORD   DataOffset;
    DWORD   DataSelector;    
    BYTE    RegisterArea[SIZE_OF_80387_REGISTERS];
    DWORD   Cr0NpxState;
} FLOATING_SAVE_AREA, *PFLOATING_SAVE_AREA;

typedef struct _CONTEXT86
{
    DWORD   ContextFlags;

    /* These are selected by CONTEXT_DEBUG_REGISTERS */
    DWORD   Dr0;
    DWORD   Dr1;
    DWORD   Dr2;
    DWORD   Dr3;
    DWORD   Dr6;
    DWORD   Dr7;

    /* These are selected by CONTEXT_FLOATING_POINT */
    FLOATING_SAVE_AREA FloatSave;

    /* These are selected by CONTEXT_SEGMENTS */
    DWORD   SegGs;
    DWORD   SegFs;
    DWORD   SegEs;
    DWORD   SegDs;    

    /* These are selected by CONTEXT_INTEGER */
    DWORD   Edi;
    DWORD   Esi;
    DWORD   Ebx;
    DWORD   Edx;    
    DWORD   Ecx;
    DWORD   Eax;

    /* These are selected by CONTEXT_CONTROL */
    DWORD   Ebp;    
    DWORD   Eip;
    DWORD   SegCs;
    DWORD   EFlags;
    DWORD   Esp;
    DWORD   SegSs;
} CONTEXT86;

#define CONTEXT_X86       0x00010000
#define CONTEXT_i386      CONTEXT_X86
#define CONTEXT_i486      CONTEXT_X86

#define CONTEXT86_CONTROL   (CONTEXT_i386 | 0x0001) /* SS:SP, CS:IP, FLAGS, BP */
#define CONTEXT86_INTEGER   (CONTEXT_i386 | 0x0002) /* AX, BX, CX, DX, SI, DI */
#define CONTEXT86_SEGMENTS  (CONTEXT_i386 | 0x0004) /* DS, ES, FS, GS */
#define CONTEXT86_FLOATING_POINT  (CONTEXT_i386 | 0x0008L) /* 387 state */
#define CONTEXT86_DEBUG_REGISTERS (CONTEXT_i386 | 0x0010L) /* DB 0-3,6,7 */
#define CONTEXT86_FULL (CONTEXT86_CONTROL | CONTEXT86_INTEGER | CONTEXT86_SEGMENTS)

/* i386 context definitions */
#ifdef __i386__

#define CONTEXT_CONTROL         CONTEXT86_CONTROL
#define CONTEXT_INTEGER         CONTEXT86_INTEGER
#define CONTEXT_SEGMENTS        CONTEXT86_SEGMENTS
#define CONTEXT_FLOATING_POINT  CONTEXT86_FLOATING_POINT
#define CONTEXT_DEBUG_REGISTERS CONTEXT86_DEBUG_REGISTERS
#define CONTEXT_FULL            CONTEXT86_FULL

typedef CONTEXT86 CONTEXT;

#endif  /* __i386__ */

/* Alpha context definitions */
#ifdef _ALPHA_

#define CONTEXT_ALPHA   0x00020000
 
#define CONTEXT_CONTROL		(CONTEXT_ALPHA | 0x00000001L)
#define CONTEXT_FLOATING_POINT	(CONTEXT_ALPHA | 0x00000002L)
#define CONTEXT_INTEGER		(CONTEXT_ALPHA | 0x00000004L)
#define CONTEXT_FULL  (CONTEXT_CONTROL | CONTEXT_FLOATING_POINT | CONTEXT_INTEGER)

typedef struct _CONTEXT
{
    /* selected by CONTEXT_FLOATING_POINT */
    ULONGLONG FltF0;
    ULONGLONG FltF1;
    ULONGLONG FltF2;
    ULONGLONG FltF3;
    ULONGLONG FltF4;
    ULONGLONG FltF5;
    ULONGLONG FltF6;
    ULONGLONG FltF7;
    ULONGLONG FltF8;
    ULONGLONG FltF9;
    ULONGLONG FltF10;
    ULONGLONG FltF11;
    ULONGLONG FltF12;
    ULONGLONG FltF13;
    ULONGLONG FltF14;
    ULONGLONG FltF15;
    ULONGLONG FltF16;
    ULONGLONG FltF17;
    ULONGLONG FltF18;
    ULONGLONG FltF19;
    ULONGLONG FltF20;
    ULONGLONG FltF21;
    ULONGLONG FltF22;
    ULONGLONG FltF23;
    ULONGLONG FltF24;
    ULONGLONG FltF25;
    ULONGLONG FltF26;
    ULONGLONG FltF27;
    ULONGLONG FltF28;
    ULONGLONG FltF29;
    ULONGLONG FltF30;
    ULONGLONG FltF31;

    /* selected by CONTEXT_INTEGER */
    ULONGLONG IntV0;
    ULONGLONG IntT0;
    ULONGLONG IntT1;
    ULONGLONG IntT2;
    ULONGLONG IntT3;
    ULONGLONG IntT4;
    ULONGLONG IntT5;
    ULONGLONG IntT6;
    ULONGLONG IntT7;
    ULONGLONG IntS0;
    ULONGLONG IntS1;
    ULONGLONG IntS2;
    ULONGLONG IntS3;
    ULONGLONG IntS4;
    ULONGLONG IntS5;
    ULONGLONG IntFp;
    ULONGLONG IntA0;
    ULONGLONG IntA1;
    ULONGLONG IntA2;
    ULONGLONG IntA3;
    ULONGLONG IntA4;
    ULONGLONG IntA5;
    ULONGLONG IntT8;
    ULONGLONG IntT9;
    ULONGLONG IntT10;
    ULONGLONG IntT11;
    ULONGLONG IntRa;
    ULONGLONG IntT12;
    ULONGLONG IntAt;
    ULONGLONG IntGp;
    ULONGLONG IntSp;
    ULONGLONG IntZero;

    /* selected by CONTEXT_FLOATING_POINT */
    ULONGLONG Fpcr;
    ULONGLONG SoftFpcr;

    /* selected by CONTEXT_CONTROL */
    ULONGLONG Fir;
    DWORD Psr;
    DWORD ContextFlags;
    DWORD Fill[4];
} CONTEXT;

#define _QUAD_PSR_OFFSET   HighSoftFpcr
#define _QUAD_FLAGS_OFFSET HighFir

#endif  /* _ALPHA_ */

/* Mips context definitions */
#ifdef _MIPS_

#define CONTEXT_R4000   0x00010000

#define CONTEXT_CONTROL         (CONTEXT_R4000 | 0x00000001)
#define CONTEXT_FLOATING_POINT  (CONTEXT_R4000 | 0x00000002)
#define CONTEXT_INTEGER         (CONTEXT_R4000 | 0x00000004)

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_FLOATING_POINT | CONTEXT_INTEGER)

typedef struct _CONTEXT
{
    DWORD Argument[4];
    /* These are selected by CONTEXT_FLOATING_POINT */
    DWORD FltF0;
    DWORD FltF1;
    DWORD FltF2;
    DWORD FltF3;
    DWORD FltF4;
    DWORD FltF5;
    DWORD FltF6;
    DWORD FltF7;
    DWORD FltF8;
    DWORD FltF9;
    DWORD FltF10;
    DWORD FltF11;
    DWORD FltF12;
    DWORD FltF13;
    DWORD FltF14;
    DWORD FltF15;
    DWORD FltF16;
    DWORD FltF17;
    DWORD FltF18;
    DWORD FltF19;
    DWORD FltF20;
    DWORD FltF21;
    DWORD FltF22;
    DWORD FltF23;
    DWORD FltF24;
    DWORD FltF25;
    DWORD FltF26;
    DWORD FltF27;
    DWORD FltF28;
    DWORD FltF29;
    DWORD FltF30;
    DWORD FltF31;

    /* These are selected by CONTEXT_INTEGER */
    DWORD IntZero;
    DWORD IntAt;
    DWORD IntV0;
    DWORD IntV1;
    DWORD IntA0;
    DWORD IntA1;
    DWORD IntA2;
    DWORD IntA3;
    DWORD IntT0;
    DWORD IntT1;
    DWORD IntT2;
    DWORD IntT3;
    DWORD IntT4;
    DWORD IntT5;
    DWORD IntT6;
    DWORD IntT7;
    DWORD IntS0;
    DWORD IntS1;
    DWORD IntS2;
    DWORD IntS3;
    DWORD IntS4;
    DWORD IntS5;
    DWORD IntS6;
    DWORD IntS7;
    DWORD IntT8;
    DWORD IntT9;
    DWORD IntK0;
    DWORD IntK1;
    DWORD IntGp;
    DWORD IntSp;
    DWORD IntS8;
    DWORD IntRa;
    DWORD IntLo;
    DWORD IntHi;

    /* These are selected by CONTEXT_FLOATING_POINT */
    DWORD Fsr;

    /* These are selected by CONTEXT_CONTROL */
    DWORD Fir;
    DWORD Psr;

    DWORD ContextFlags;
    DWORD Fill[2];
} CONTEXT;

#endif  /* _MIPS_ */

/* PowerPC context definitions */
#ifdef __PPC__

#define CONTEXT_CONTROL         0x0001
#define CONTEXT_FLOATING_POINT  0x0002
#define CONTEXT_INTEGER         0x0004
#define CONTEXT_DEBUG_REGISTERS 0x0008
#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_FLOATING_POINT | CONTEXT_INTEGER)

typedef struct
{
    /* These are selected by CONTEXT_FLOATING_POINT */
    double Fpr0;
    double Fpr1;
    double Fpr2;
    double Fpr3;
    double Fpr4;
    double Fpr5;
    double Fpr6;
    double Fpr7;
    double Fpr8;
    double Fpr9;
    double Fpr10;
    double Fpr11;
    double Fpr12;
    double Fpr13;
    double Fpr14;
    double Fpr15;
    double Fpr16;
    double Fpr17;
    double Fpr18;
    double Fpr19;
    double Fpr20;
    double Fpr21;
    double Fpr22;
    double Fpr23;
    double Fpr24;
    double Fpr25;
    double Fpr26;
    double Fpr27;
    double Fpr28;
    double Fpr29;
    double Fpr30;
    double Fpr31;
    double Fpscr;

    /* These are selected by CONTEXT_INTEGER */
    DWORD Gpr0;
    DWORD Gpr1;
    DWORD Gpr2;
    DWORD Gpr3;
    DWORD Gpr4;
    DWORD Gpr5;
    DWORD Gpr6;
    DWORD Gpr7;
    DWORD Gpr8;
    DWORD Gpr9;
    DWORD Gpr10;
    DWORD Gpr11;
    DWORD Gpr12;
    DWORD Gpr13;
    DWORD Gpr14;
    DWORD Gpr15;
    DWORD Gpr16;
    DWORD Gpr17;
    DWORD Gpr18;
    DWORD Gpr19;
    DWORD Gpr20;
    DWORD Gpr21;
    DWORD Gpr22;
    DWORD Gpr23;
    DWORD Gpr24;
    DWORD Gpr25;
    DWORD Gpr26;
    DWORD Gpr27;
    DWORD Gpr28;
    DWORD Gpr29;
    DWORD Gpr30;
    DWORD Gpr31;

    DWORD Cr;
    DWORD Xer;

    /* These are selected by CONTEXT_CONTROL */
    DWORD Msr;
    DWORD Iar;
    DWORD Lr;
    DWORD Ctr;

    DWORD ContextFlags;
    DWORD Fill[3];

    /* These are selected by CONTEXT_DEBUG_REGISTERS */
    DWORD Dr0;
    DWORD Dr1;
    DWORD Dr2;
    DWORD Dr3;
    DWORD Dr4;
    DWORD Dr5;
    DWORD Dr6;
    DWORD Dr7;
} CONTEXT;

typedef struct _STACK_FRAME_HEADER
{
    DWORD BackChain;
    DWORD GlueSaved1;
    DWORD GlueSaved2;
    DWORD Reserved1;
    DWORD Spare1;
    DWORD Spare2;

    DWORD Parameter0;
    DWORD Parameter1;
    DWORD Parameter2;
    DWORD Parameter3;
    DWORD Parameter4;
    DWORD Parameter5;
    DWORD Parameter6;
    DWORD Parameter7;
} STACK_FRAME_HEADER,*PSTACK_FRAME_HEADER;

#endif  /* __PPC__ */

#ifdef __sparc__

/* 
 * FIXME:  
 *
 * There is no official CONTEXT structure defined for the SPARC 
 * architecture, so I just made one up.
 *
 * This structure is valid only for 32-bit SPARC architectures,
 * not for 64-bit SPARC.
 *
 * Note that this structure contains only the 'top-level' registers;
 * the rest of the register window chain is not visible.
 *
 * The layout follows the Solaris 'prgregset_t' structure.
 * 
 */ 

#define CONTEXT_SPARC            0x10000000

#define CONTEXT_CONTROL         (CONTEXT_SPARC | 0x00000001)
#define CONTEXT_FLOATING_POINT  (CONTEXT_SPARC | 0x00000002)
#define CONTEXT_INTEGER         (CONTEXT_SPARC | 0x00000004)

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_FLOATING_POINT | CONTEXT_INTEGER)

typedef struct _CONTEXT
{
    DWORD ContextFlags;

    /* These are selected by CONTEXT_INTEGER */
    DWORD g0;
    DWORD g1;
    DWORD g2;
    DWORD g3;
    DWORD g4;
    DWORD g5;
    DWORD g6;
    DWORD g7;
    DWORD o0;
    DWORD o1;
    DWORD o2;
    DWORD o3;
    DWORD o4;
    DWORD o5;
    DWORD o6;
    DWORD o7;
    DWORD l0;
    DWORD l1;
    DWORD l2;
    DWORD l3;
    DWORD l4;
    DWORD l5;
    DWORD l6;
    DWORD l7;
    DWORD i0;
    DWORD i1;
    DWORD i2;
    DWORD i3;
    DWORD i4;
    DWORD i5;
    DWORD i6;
    DWORD i7;

    /* These are selected by CONTEXT_CONTROL */
    DWORD psr;
    DWORD pc;
    DWORD npc;
    DWORD y;
    DWORD wim;
    DWORD tbr;

    /* FIXME: floating point registers missing */

} CONTEXT;

#endif  /* __sparc__ */

#if !defined(CONTEXT_FULL) && !defined(RC_INVOKED)
#error You need to define a CONTEXT for your CPU
#endif

typedef CONTEXT *PCONTEXT;

#ifdef __WINE__

/* Macros for easier access to i386 context registers */

#define AX_reg(context)      (*(WORD*)&(context)->Eax)
#define BX_reg(context)      (*(WORD*)&(context)->Ebx)
#define CX_reg(context)      (*(WORD*)&(context)->Ecx)
#define DX_reg(context)      (*(WORD*)&(context)->Edx)
#define SI_reg(context)      (*(WORD*)&(context)->Esi)
#define DI_reg(context)      (*(WORD*)&(context)->Edi)
#define BP_reg(context)      (*(WORD*)&(context)->Ebp)

#define AL_reg(context)      (*(BYTE*)&(context)->Eax)
#define AH_reg(context)      (*((BYTE*)&(context)->Eax + 1))
#define BL_reg(context)      (*(BYTE*)&(context)->Ebx)
#define BH_reg(context)      (*((BYTE*)&(context)->Ebx + 1))
#define CL_reg(context)      (*(BYTE*)&(context)->Ecx)
#define CH_reg(context)      (*((BYTE*)&(context)->Ecx + 1))
#define DL_reg(context)      (*(BYTE*)&(context)->Edx)
#define DH_reg(context)      (*((BYTE*)&(context)->Edx + 1))

#define SET_CFLAG(context)   ((context)->EFlags |= 0x0001)
#define RESET_CFLAG(context) ((context)->EFlags &= ~0x0001)
#define SET_ZFLAG(context)   ((context)->EFlags |= 0x0040)
#define RESET_ZFLAG(context) ((context)->EFlags &= ~0x0040)
#define ISV86(context)       ((context)->EFlags & 0x00020000)


/* Macros to retrieve the current context */

#ifdef NEED_UNDERSCORE_PREFIX
# define __ASM_NAME(name) "_" name
#else
# define __ASM_NAME(name) name
#endif

#ifdef NEED_TYPE_IN_DEF
# define __ASM_FUNC(name) ".def " __ASM_NAME(name) "; .scl 2; .type 32; .endef"
#else
# define __ASM_FUNC(name) ".type " __ASM_NAME(name) ",@function"
#endif

#ifdef __GNUC__
# define __ASM_GLOBAL_FUNC(name,code) \
      __asm__( ".align 4\n\t" \
               ".globl " __ASM_NAME(#name) "\n\t" \
               __ASM_FUNC(#name) "\n" \
               __ASM_NAME(#name) ":\n\t" \
               code );
#else  /* __GNUC__ */
# define __ASM_GLOBAL_FUNC(name,code) \
      void __asm_dummy_##name(void) { \
          asm( ".align 4\n\t" \
               ".globl " __ASM_NAME(#name) "\n\t" \
               __ASM_FUNC(#name) "\n" \
               __ASM_NAME(#name) ":\n\t" \
               code ); \
      }
#endif  /* __GNUC__ */

#ifdef __i386__

#define _DEFINE_REGS_ENTRYPOINT( name, fn, args ) \
    __ASM_GLOBAL_FUNC( name, \
                       "call " __ASM_NAME("CALL32_Regs") "\n\t" \
                       ".long " __ASM_NAME(#fn) "\n\t" \
                       ".byte " #args ", " #args )
#define DEFINE_REGS_ENTRYPOINT_0( name, fn ) \
  extern void WINAPI name(void); \
  _DEFINE_REGS_ENTRYPOINT( name, fn, 0 )
#define DEFINE_REGS_ENTRYPOINT_1( name, fn, t1 ) \
  extern void WINAPI name( t1 a1 ); \
  _DEFINE_REGS_ENTRYPOINT( name, fn, 4 )
#define DEFINE_REGS_ENTRYPOINT_2( name, fn, t1, t2 ) \
  extern void WINAPI name( t1 a1, t2 a2 ); \
  _DEFINE_REGS_ENTRYPOINT( name, fn, 8 )
#define DEFINE_REGS_ENTRYPOINT_3( name, fn, t1, t2, t3 ) \
  extern void WINAPI name( t1 a1, t2 a2, t3 a3 ); \
  _DEFINE_REGS_ENTRYPOINT( name, fn, 12 )
#define DEFINE_REGS_ENTRYPOINT_4( name, fn, t1, t2, t3, t4 ) \
  extern void WINAPI name( t1 a1, t2 a2, t3 a3, t4 a4 ); \
  _DEFINE_REGS_ENTRYPOINT( name, fn, 16 )

#endif  /* __i386__ */

#ifdef __sparc__
/* FIXME: use getcontext() to retrieve full context */
#define _GET_CONTEXT \
    CONTEXT context;   \
    do { memset(&context, 0, sizeof(CONTEXT));            \
         context.ContextFlags = CONTEXT_CONTROL;          \
         context.pc = (DWORD)__builtin_return_address(0); \
       } while (0)

#define DEFINE_REGS_ENTRYPOINT_0( name, fn ) \
  void WINAPI name ( void ) \
  { _GET_CONTEXT; fn( &context ); }
#define DEFINE_REGS_ENTRYPOINT_1( name, fn, t1 ) \
  void WINAPI name ( t1 a1 ) \
  { _GET_CONTEXT; fn( a1, &context ); }
#define DEFINE_REGS_ENTRYPOINT_2( name, fn, t1, t2 ) \
  void WINAPI name ( t1 a1, t2 a2 ) \
  { _GET_CONTEXT; fn( a1, a2, &context ); }
#define DEFINE_REGS_ENTRYPOINT_3( name, fn, t1, t2, t3 ) \
  void WINAPI name ( t1 a1, t2 a2, t3 a3 ) \
  { _GET_CONTEXT; fn( a1, a2, a3, &context ); }
#define DEFINE_REGS_ENTRYPOINT_4( name, fn, t1, t2, t3, t4 ) \
  void WINAPI name ( t1 a1, t2 a2, t3 a3, t4 a4 ) \
  { _GET_CONTEXT; fn( a1, a2, a3, a4, &context ); }

#endif /* __sparc__ */

#ifdef __PPC__

/* FIXME: use getcontext() to retrieve full context */
#define _GET_CONTEXT \
    CONTEXT context;   \
    do { memset(&context, 0, sizeof(CONTEXT));            \
         context.ContextFlags = CONTEXT_CONTROL;          \
       } while (0)

#define DEFINE_REGS_ENTRYPOINT_0( name, fn ) \
  void WINAPI name ( void ) \
  { _GET_CONTEXT; fn( &context ); }
#define DEFINE_REGS_ENTRYPOINT_1( name, fn, t1 ) \
  void WINAPI name ( t1 a1 ) \
  { _GET_CONTEXT; fn( a1, &context ); }
#define DEFINE_REGS_ENTRYPOINT_2( name, fn, t1, t2 ) \
  void WINAPI name ( t1 a1, t2 a2 ) \
  { _GET_CONTEXT; fn( a1, a2, &context ); }
#define DEFINE_REGS_ENTRYPOINT_3( name, fn, t1, t2, t3 ) \
  void WINAPI name ( t1 a1, t2 a2, t3 a3 ) \
  { _GET_CONTEXT; fn( a1, a2, a3, &context ); }
#define DEFINE_REGS_ENTRYPOINT_4( name, fn, t1, t2, t3, t4 ) \
  void WINAPI name ( t1 a1, t2 a2, t3 a3, t4 a4 ) \
  { _GET_CONTEXT; fn( a1, a2, a3, a4, &context ); }

#endif /* __PPC__ */


#ifndef DEFINE_REGS_ENTRYPOINT_0
#error You need to define DEFINE_REGS_ENTRYPOINT macros for your CPU
#endif

/* Constructor functions */

#ifdef __GNUC__
# define DECL_GLOBAL_CONSTRUCTOR(func) \
    static void func(void) __attribute__((constructor)); \
    static void func(void)
#else  /* __GNUC__ */
# ifdef __i386__
#  define DECL_GLOBAL_CONSTRUCTOR(func) \
    static void __dummy_init_##func(void) { \
        asm(".section .init,\"ax\"\n\t" \
            "call " #func "\n\t" \
            ".previous"); } \
    static void func(void)
# else  /* __i386__ */
#  error You must define the DECL_GLOBAL_CONSTRUCTOR macro for your platform
# endif
#endif  /* __GNUC__ */

/* Segment register access */

#ifdef __i386__
# ifdef __GNUC__
#  define __DEFINE_GET_SEG(seg) \
    extern inline unsigned short __get_##seg(void) \
    { unsigned short res; __asm__("movw %%" #seg ",%w0" : "=r"(res)); return res; }
#  define __DEFINE_SET_SEG(seg) \
    extern inline void __set_##seg(int val) { __asm__("movw %w0,%%" #seg : : "r" (val)); }
# else  /* __GNUC__ */
#  define __DEFINE_GET_SEG(seg) extern unsigned short __get_##seg(void);
#  define __DEFINE_SET_SEG(seg) extern void __set_##seg(unsigned int);
# endif /* __GNUC__ */
#else  /* __i386__ */
# define __DEFINE_GET_SEG(seg) inline static unsigned short __get_##seg(void) { return 0; }
# define __DEFINE_SET_SEG(seg) /* nothing */
#endif  /* __i386__ */

__DEFINE_GET_SEG(cs)
__DEFINE_GET_SEG(ds)
__DEFINE_GET_SEG(es)
__DEFINE_GET_SEG(fs)
__DEFINE_GET_SEG(gs)
__DEFINE_GET_SEG(ss)
__DEFINE_SET_SEG(fs)
__DEFINE_SET_SEG(gs)
#undef __DEFINE_GET_SEG
#undef __DEFINE_SET_SEG

#endif  /* __WINE__ */



/*
 * Language IDs
 */

#define MAKELCID(l, s)		(MAKELONG(l, s))

#define MAKELANGID(p, s)        ((((WORD)(s))<<10) | (WORD)(p))
#define PRIMARYLANGID(l)        ((WORD)(l) & 0x3ff)
#define SUBLANGID(l)            ((WORD)(l) >> 10)

#define LANGIDFROMLCID(lcid)	((WORD)(lcid))
#define SORTIDFROMLCID(lcid)	((WORD)((((DWORD)(lcid)) >> 16) & 0x0f))

#define LANG_SYSTEM_DEFAULT	(MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT))
#define LANG_USER_DEFAULT	(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))
#define LOCALE_SYSTEM_DEFAULT	(MAKELCID(LANG_SYSTEM_DEFAULT, SORT_DEFAULT))
#define LOCALE_USER_DEFAULT	(MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT)) 
#define LOCALE_NEUTRAL		(MAKELCID(MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),SORT_DEFAULT))

/* FIXME: are the symbolic names correct for LIDs:  0x17, 0x20, 0x28,
 *	  0x2a, 0x2b, 0x2c, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
 *	  0x37, 0x39, 0x3a, 0x3b, 0x3c, 0x3e, 0x3f, 0x41, 0x43, 0x44,
 *	  0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e,
 *	  0x4f, 0x57
 */
#define LANG_NEUTRAL                     0x00
#define LANG_AFRIKAANS			 0x36
#define LANG_ALBANIAN			 0x1c
#define LANG_ARABIC                      0x01
#define LANG_ARMENIAN			 0x2b
#define LANG_ASSAMESE			 0x4d
#define LANG_AZERI			 0x2c
#define LANG_BASQUE                      0x2d
#define LANG_BENGALI			 0x45
#define LANG_BULGARIAN                   0x02
#define LANG_BYELORUSSIAN                0x23
#define LANG_CATALAN                     0x03
#define LANG_CHINESE                     0x04
#define LANG_SERBO_CROATIAN              0x1a
#define LANG_CROATIAN	  LANG_SERBO_CROATIAN
#define LANG_SERBIAN	  LANG_SERBO_CROATIAN
#define LANG_CZECH                       0x05
#define LANG_DANISH                      0x06
#define LANG_DUTCH                       0x13
#define LANG_ENGLISH                     0x09
#define LANG_ESTONIAN                    0x25
#define LANG_FAEROESE                    0x38
#define LANG_FARSI                       0x29
#define LANG_FINNISH                     0x0b
#define LANG_FRENCH                      0x0c
#define LANG_GAELIC			 0x3c
#define LANG_GEORGIAN			 0x37
#define LANG_GERMAN                      0x07
#define LANG_GREEK                       0x08
#define LANG_GUJARATI			 0x47
#define LANG_HEBREW                      0x0D
#define LANG_HINDI			 0x39
#define LANG_HUNGARIAN                   0x0e
#define LANG_ICELANDIC                   0x0f
#define LANG_INDONESIAN                  0x21
#define LANG_ITALIAN                     0x10
#define LANG_JAPANESE                    0x11
#define LANG_KANNADA			 0x4b
#define LANG_KAZAKH			 0x3f
#define LANG_KONKANI			 0x57
#define LANG_KOREAN                      0x12
#define LANG_LATVIAN                     0x26
#define LANG_LITHUANIAN                  0x27
#define LANG_MACEDONIAN			 0x2f
#define LANG_MALAY			 0x3e
#define LANG_MALAYALAM			 0x4c
#define LANG_MALTESE			 0x3a
#define LANG_MAORI			 0x28
#define LANG_MARATHI			 0x4e
#define LANG_NORWEGIAN                   0x14
#define LANG_ORIYA			 0x48
#define LANG_POLISH                      0x15
#define LANG_PORTUGUESE                  0x16
#define LANG_PUNJABI			 0x46
#define LANG_RHAETO_ROMANCE		 0x17
#define LANG_ROMANIAN                    0x18
#define LANG_RUSSIAN                     0x19
#define LANG_SAAMI			 0x3b
#define LANG_SANSKRIT			 0x4f
#define LANG_SLOVAK                      0x1b
#define LANG_SLOVENIAN                   0x24
#define LANG_SORBIAN                     0x2e
#define LANG_SPANISH                     0x0a
#define LANG_SUTU			 0x30
#define LANG_SWAHILI			 0x41
#define LANG_SWEDISH                     0x1d
#define LANG_TAMIL			 0x49
#define LANG_TATAR			 0x44
#define LANG_TELUGU			 0x4a
#define LANG_THAI                        0x1e
#define LANG_TSONGA			 0x31
#define LANG_TSWANA			 0x32
#define LANG_TURKISH                     0x1f
#define LANG_UKRAINIAN                   0x22
#define LANG_URDU			 0x20
#define LANG_UZBEK			 0x43
#define LANG_VENDA			 0x33
#define LANG_VIETNAMESE			 0x2a
#define LANG_XHOSA			 0x34
#define LANG_ZULU			 0x35
/* non standard; keep the number high enough (but < 0xff) */
#define LANG_ESPERANTO			 0x8f
#define LANG_WALON			 0x90
#define LANG_CORNISH                     0x91
#define LANG_WELSH                       0x92
#define LANG_BRETON                      0x93

/* Sublanguage definitions */
#define SUBLANG_NEUTRAL                  0x00    /* language neutral */
#define SUBLANG_DEFAULT                  0x01    /* user default */
#define SUBLANG_SYS_DEFAULT              0x02    /* system default */

#define SUBLANG_ARABIC                   0x01
#define SUBLANG_ARABIC_SAUDI_ARABIA      0x01
#define SUBLANG_ARABIC_IRAQ              0x02
#define SUBLANG_ARABIC_EGYPT             0x03
#define SUBLANG_ARABIC_LIBYA             0x04
#define SUBLANG_ARABIC_ALGERIA           0x05
#define SUBLANG_ARABIC_MOROCCO           0x06
#define SUBLANG_ARABIC_TUNISIA           0x07
#define SUBLANG_ARABIC_OMAN              0x08
#define SUBLANG_ARABIC_YEMEN             0x09
#define SUBLANG_ARABIC_SYRIA             0x0a
#define SUBLANG_ARABIC_JORDAN            0x0b
#define SUBLANG_ARABIC_LEBANON           0x0c
#define SUBLANG_ARABIC_KUWAIT            0x0d
#define SUBLANG_ARABIC_UAE               0x0e
#define SUBLANG_ARABIC_BAHRAIN           0x0f
#define SUBLANG_ARABIC_QATAR             0x10
#define SUBLANG_CHINESE_TRADITIONAL      0x01
#define SUBLANG_CHINESE_SIMPLIFIED       0x02
#define SUBLANG_CHINESE_HONGKONG         0x03
#define SUBLANG_CHINESE_SINGAPORE        0x04
#define SUBLANG_CHINESE_MACAU            0x05
#define SUBLANG_DUTCH                    0x01
#define SUBLANG_DUTCH_BELGIAN            0x02
#define SUBLANG_DUTCH_SURINAM		 0x03
#define SUBLANG_ENGLISH_US               0x01
#define SUBLANG_ENGLISH_UK               0x02
#define SUBLANG_ENGLISH_AUS              0x03
#define SUBLANG_ENGLISH_CAN              0x04
#define SUBLANG_ENGLISH_NZ               0x05
#define SUBLANG_ENGLISH_EIRE             0x06
#define SUBLANG_ENGLISH_SAFRICA          0x07
#define SUBLANG_ENGLISH_JAMAICA          0x08
#define SUBLANG_ENGLISH_CARRIBEAN        0x09
#define SUBLANG_ENGLISH_BELIZE           0x0a
#define SUBLANG_ENGLISH_TRINIDAD         0x0b
#define SUBLANG_ENGLISH_ZIMBABWE         0x0c
#define SUBLANG_ENGLISH_PHILIPPINES      0x0d
#define SUBLANG_FRENCH                   0x01
#define SUBLANG_FRENCH_BELGIAN           0x02
#define SUBLANG_FRENCH_CANADIAN          0x03
#define SUBLANG_FRENCH_SWISS             0x04
#define SUBLANG_FRENCH_LUXEMBOURG        0x05
#define SUBLANG_FRENCH_MONACO            0x06
#define SUBLANG_GERMAN                   0x01
#define SUBLANG_GERMAN_SWISS             0x02
#define SUBLANG_GERMAN_AUSTRIAN          0x03
#define SUBLANG_GERMAN_LUXEMBOURG        0x04
#define SUBLANG_GERMAN_LIECHTENSTEIN     0x05
#define SUBLANG_ITALIAN                  0x01
#define SUBLANG_ITALIAN_SWISS            0x02
#define SUBLANG_KOREAN                   0x01
#define SUBLANG_KOREAN_JOHAB             0x02
#define SUBLANG_NORWEGIAN_BOKMAL         0x01
#define SUBLANG_NORWEGIAN_NYNORSK        0x02
#define SUBLANG_PORTUGUESE               0x02
#define SUBLANG_PORTUGUESE_BRAZILIAN     0x01
#define SUBLANG_SPANISH                  0x01
#define SUBLANG_SPANISH_MEXICAN          0x02
#define SUBLANG_SPANISH_MODERN           0x03
#define SUBLANG_SPANISH_GUATEMALA        0x04
#define SUBLANG_SPANISH_COSTARICA        0x05
#define SUBLANG_SPANISH_PANAMA           0x06
#define SUBLANG_SPANISH_DOMINICAN        0x07
#define SUBLANG_SPANISH_VENEZUELA        0x08
#define SUBLANG_SPANISH_COLOMBIA         0x09
#define SUBLANG_SPANISH_PERU             0x0a
#define SUBLANG_SPANISH_ARGENTINA        0x0b
#define SUBLANG_SPANISH_ECUADOR          0x0c
#define SUBLANG_SPANISH_CHILE            0x0d
#define SUBLANG_SPANISH_URUGUAY          0x0e
#define SUBLANG_SPANISH_PARAGUAY         0x0f
#define SUBLANG_SPANISH_BOLIVIA          0x10
#define SUBLANG_SPANISH_EL_SALVADOR	 0x11
#define SUBLANG_SPANISH_HONDURAS         0x12
#define SUBLANG_SPANISH_NICARAGUA        0x13
#define SUBLANG_SPANISH_PUERTO_RICO      0x14
/* FIXME: I don't know the symbolic names for those */
#define SUBLANG_ROMANIAN		 0x01
#define SUBLANG_ROMANIAN_MOLDAVIA	 0x02
#define SUBLANG_RUSSIAN			 0x01
#define SUBLANG_RUSSIAN_MOLDAVIA	 0x02
#define SUBLANG_CROATIAN		 0x01
#define SUBLANG_SERBIAN			 0x02
#define SUBLANG_SERBIAN_LATIN		 0x03
#define SUBLANG_SWEDISH			 0x01
#define SUBLANG_SWEDISH_FINLAND		 0x02
#define SUBLANG_LITHUANIAN		 0x01
#define SUBLANG_LITHUANIAN_CLASSIC	 0x02
#define SUBLANG_AZERI			 0x01
#define SUBLANG_AZERI_CYRILLIC		 0x02
#define SUBLANG_GAELIC			 0x01
#define SUBLANG_GAELIC_SCOTTISH		 0x02
#define SUBLANG_GAELIC_MANX              0x03
#define SUBLANG_MALAY			 0x01
#define SUBLANG_MALAY_BRUNEI_DARUSSALAM  0x02
#define SUBLANG_UZBEK			 0x01
#define SUBLANG_UZBEK_CYRILLIC		 0x02
#define SUBLANG_URDU_PAKISTAN            0x01



/*
 * Sort definitions
 */

#define SORT_DEFAULT                     0x0
#define SORT_JAPANESE_XJIS               0x0
#define SORT_JAPANESE_UNICODE            0x1
#define SORT_CHINESE_BIG5                0x0
#define SORT_CHINESE_UNICODE             0x1
#define SORT_KOREAN_KSC                  0x0
#define SORT_KOREAN_UNICODE              0x1



/*
 * Definitions for IsTextUnicode()
 */

#define IS_TEXT_UNICODE_ASCII16		   0x0001
#define IS_TEXT_UNICODE_STATISTICS         0x0002
#define IS_TEXT_UNICODE_CONTROLS           0x0004
#define IS_TEXT_UNICODE_SIGNATURE	   0x0008
#define IS_TEXT_UNICODE_UNICODE_MASK       0x000F
#define IS_TEXT_UNICODE_REVERSE_ASCII16	   0x0010
#define IS_TEXT_UNICODE_REVERSE_STATISTICS 0x0020
#define IS_TEXT_UNICODE_REVERSE_CONTROLS   0x0040
#define IS_TEXT_UNICODE_REVERSE_SIGNATURE  0x0080
#define IS_TEXT_UNICODE_REVERSE_MASK       0x00F0
#define IS_TEXT_UNICODE_ILLEGAL_CHARS	   0x0100
#define IS_TEXT_UNICODE_ODD_LENGTH	   0x0200
#define IS_TEXT_UNICODE_DBCS_LEADBYTE      0x0400
#define IS_TEXT_UNICODE_NOT_UNICODE_MASK   0x0F00
#define IS_TEXT_UNICODE_NULL_BYTES         0x1000
#define IS_TEXT_UNICODE_NOT_ASCII_MASK     0xF000



/*
 * Exception codes
 */

#define STATUS_SUCCESS                   0x00000000
#define STATUS_WAIT_0                    0x00000000
#define STATUS_ABANDONED_WAIT_0          0x00000080
#define STATUS_ABANDONED_WAIT_63         0x000000BF
#define STATUS_USER_APC                  0x000000C0
#define STATUS_ALERTED                   0x00000101
#define STATUS_TIMEOUT                   0x00000102
#define STATUS_PENDING                   0x00000103
#define STATUS_REPARSE                   0x00000104
#define STATUS_MORE_ENTRIES              0x00000105
#define STATUS_NOT_ALL_ASSIGNED          0x00000106
#define STATUS_SOME_NOT_MAPPED           0x00000107
#define STATUS_OPLOCK_BREAK_IN_PROGRESS  0x00000108
#define STATUS_VOLUME_MOUNTED            0x00000109
#define STATUS_RXACT_COMMITTED           0x0000010A
#define STATUS_NOTIFY_CLEANUP            0x0000010B
#define STATUS_NOTIFY_ENUM_DIR           0x0000010C
#define STATUS_NO_QUOTAS_FOR_ACCOUNT     0x0000010D
#define STATUS_PRIMARY_TRANSPORT_CONNECT_FAILED 0x0000010E
#define STATUS_PAGE_FAULT_TRANSITION     0x00000110
#define STATUS_PAGE_FAULT_DEMAND_ZERO    0x00000111
#define STATUS_PAGE_FAULT_COPY_ON_WRITE  0x00000112
#define STATUS_PAGE_FAULT_GUARD_PAGE     0x00000113
#define STATUS_PAGE_FAULT_PAGING_FILE    0x00000114
#define STATUS_CACHE_PAGE_LOCKED         0x00000115
#define STATUS_CRASH_DUMP                0x00000116
#define STATUS_BUFFER_ALL_ZEROS          0x00000117
#define STATUS_REPARSE_OBJECT            0x00000118

#define STATUS_THREAD_WAS_SUSPENDED      0x40000001
#define STATUS_WORKING_SET_LIMIT_RANGE   0x40000002
#define STATUS_IMAGE_NOT_AT_BASE         0x40000003
#define STATUS_RXACT_STATE_CREATED       0x40000004
#define STATUS_SEGMENT_NOTIFICATION      0x40000005
#define STATUS_LOCAL_USER_SESSION_KEY    0x40000006
#define STATUS_BAD_CURRENT_DIRECTORY     0x40000007
#define STATUS_SERIAL_MORE_WRITES        0x40000008
#define STATUS_REGISTRY_RECOVERED        0x40000009
#define STATUS_FT_READ_RECOVERY_FROM_BACKUP 0x4000000A
#define STATUS_FT_WRITE_RECOVERY         0x4000000B
#define STATUS_SERIAL_COUNTER_TIMEOUT    0x4000000C
#define STATUS_NULL_LM_PASSWORD          0x4000000D
#define STATUS_IMAGE_MACHINE_TYPE_MISMATCH 0x4000000E
#define STATUS_RECEIVE_PARTIAL           0x4000000F
#define STATUS_RECEIVE_EXPEDITED         0x40000010
#define STATUS_RECEIVE_PARTIAL_EXPEDITED 0x40000011
#define STATUS_EVENT_DONE                0x40000012
#define STATUS_EVENT_PENDING             0x40000013
#define STATUS_CHECKING_FILE_SYSTEM      0x40000014
#define STATUS_FATAL_APP_EXIT            0x40000015
#define STATUS_PREDEFINED_HANDLE         0x40000016
#define STATUS_WAS_UNLOCKED              0x40000017
#define STATUS_SERVICE_NOTIFICATION      0x40000018
#define STATUS_WAS_LOCKED                0x40000019
#define STATUS_LOG_HARD_ERROR            0x4000001A
#define STATUS_ALREADY_WIN32             0x4000001B
#define STATUS_WX86_UNSIMULATE           0x4000001C
#define STATUS_WX86_CONTINUE             0x4000001D
#define STATUS_WX86_SINGLE_STEP          0x4000001E
#define STATUS_WX86_BREAKPOINT           0x4000001F
#define STATUS_WX86_EXCEPTION_CONTINUE   0x40000020
#define STATUS_WX86_EXCEPTION_LASTCHANCE 0x40000021
#define STATUS_WX86_EXCEPTION_CHAIN      0x40000022
#define STATUS_IMAGE_MACHINE_TYPE_MISMATCH_EXE 0x40000023
#define STATUS_NO_YIELD_PERFORMED        0x40000024
#define STATUS_TIMER_RESUME_IGNORED      0x40000025
	
#define STATUS_GUARD_PAGE_VIOLATION      0x80000001    
#define STATUS_DATATYPE_MISALIGNMENT     0x80000002
#define STATUS_BREAKPOINT                0x80000003
#define STATUS_SINGLE_STEP               0x80000004
#define	STATUS_BUFFER_OVERFLOW           0x80000005
#define STATUS_NO_MORE_FILES             0x80000006
#define STATUS_WAKE_SYSTEM_DEBUGGER      0x80000007

#define STATUS_HANDLES_CLOSED            0x8000000A
#define STATUS_NO_INHERITANCE            0x8000000B
#define STATUS_GUID_SUBSTITUTION_MADE    0x8000000C
#define STATUS_PARTIAL_COPY              0x8000000D
#define STATUS_DEVICE_PAPER_EMPTY        0x8000000E
#define STATUS_DEVICE_POWERED_OFF        0x8000000F
#define STATUS_DEVICE_OFF_LINE           0x80000010
#define STATUS_DEVICE_BUSY               0x80000011
#define STATUS_NO_MORE_EAS               0x80000012
#define STATUS_INVALID_EA_NAME           0x80000013
#define STATUS_EA_LIST_INCONSISTENT      0x80000014
#define STATUS_INVALID_EA_FLAG           0x80000015
#define STATUS_VERIFY_REQUIRED           0x80000016
#define STATUS_EXTRANEOUS_INFORMATION    0x80000017
#define STATUS_RXACT_COMMIT_NECESSARY    0x80000018
#define STATUS_NO_MORE_ENTRIES           0x8000001A
#define STATUS_FILEMARK_DETECTED         0x8000001B
#define STATUS_MEDIA_CHANGED             0x8000001C
#define STATUS_BUS_RESET                 0x8000001D
#define STATUS_END_OF_MEDIA              0x8000001E
#define STATUS_BEGINNING_OF_MEDIA        0x8000001F
#define STATUS_MEDIA_CHECK               0x80000020
#define STATUS_SETMARK_DETECTED          0x80000021
#define STATUS_NO_DATA_DETECTED          0x80000022
#define STATUS_REDIRECTOR_HAS_OPEN_HANDLES 0x80000023
#define STATUS_SERVER_HAS_OPEN_HANDLES   0x80000024
#define STATUS_ALREADY_DISCONNECTED      0x80000025
#define STATUS_LONGJUMP                  0x80000026

#define STATUS_UNSUCCESSFUL              0xC0000001
#define STATUS_NOT_IMPLEMENTED           0xC0000002
#define STATUS_INVALID_INFO_CLASS        0xC0000003
#define STATUS_INFO_LENGTH_MISMATCH      0xC0000004
#define STATUS_ACCESS_VIOLATION          0xC0000005
#define STATUS_IN_PAGE_ERROR             0xC0000006
#define STATUS_PAGEFILE_QUOTA            0xC0000007
#define STATUS_INVALID_HANDLE            0xC0000008
#define STATUS_BAD_INITIAL_STACK         0xC0000009
#define STATUS_BAD_INITIAL_PC            0xC000000A
#define STATUS_INVALID_CID               0xC000000B
#define STATUS_TIMER_NOT_CANCELED        0xC000000C
#define STATUS_INVALID_PARAMETER         0xC000000D
#define STATUS_NO_SUCH_DEVICE            0xC000000E
#define STATUS_NO_SUCH_FILE              0xC000000F
#define STATUS_INVALID_DEVICE_REQUEST    0xC0000010
#define STATUS_END_OF_FILE               0xC0000011
#define STATUS_WRONG_VOLUME              0xC0000012
#define STATUS_NO_MEDIA_IN_DEVICE        0xC0000013
#define STATUS_UNRECOGNIZED_MEDIA        0xC0000014
#define STATUS_NONEXISTENT_SECTOR        0xC0000015
#define STATUS_MORE_PROCESSING_REQUIRED  0xC0000016
#define STATUS_NO_MEMORY                 0xC0000017
#define STATUS_CONFLICTING_ADDRESSES     0xC0000018
#define STATUS_NOT_MAPPED_VIEW           0xC0000019
#define STATUS_UNABLE_TO_FREE_VM         0xC000001A
#define STATUS_UNABLE_TO_DELETE_SECTION  0xC000001B
#define STATUS_INVALID_SYSTEM_SERVICE    0xC000001C
#define STATUS_ILLEGAL_INSTRUCTION       0xC000001D
#define STATUS_INVALID_LOCK_SEQUENCE     0xC000001E
#define STATUS_INVALID_VIEW_SIZE         0xC000001F
#define STATUS_INVALID_FILE_FOR_SECTION  0xC0000020
#define STATUS_ALREADY_COMMITTED         0xC0000021
#define STATUS_ACCESS_DENIED             0xC0000022
#define	STATUS_BUFFER_TOO_SMALL          0xC0000023
#define STATUS_OBJECT_TYPE_MISMATCH      0xC0000024
#define STATUS_NONCONTINUABLE_EXCEPTION  0xC0000025
#define STATUS_INVALID_DISPOSITION       0xC0000026
#define STATUS_UNWIND                    0xC0000027
#define STATUS_BAD_STACK                 0xC0000028
#define STATUS_INVALID_UNWIND_TARGET     0xC0000029
#define STATUS_NOT_LOCKED                0xC000002A
#define STATUS_PARITY_ERROR              0xC000002B
#define STATUS_UNABLE_TO_DECOMMIT_VM     0xC000002C
#define STATUS_NOT_COMMITTED             0xC000002D
#define STATUS_INVALID_PORT_ATTRIBUTES   0xC000002E
#define STATUS_PORT_MESSAGE_TOO_LONG     0xC000002F
#define STATUS_INVALID_PARAMETER_MIX     0xC0000030
#define STATUS_INVALID_QUOTA_LOWER       0xC0000031
#define STATUS_DISK_CORRUPT_ERROR        0xC0000032
#define STATUS_OBJECT_NAME_INVALID       0xC0000033
#define STATUS_OBJECT_NAME_NOT_FOUND     0xC0000034
#define STATUS_OBJECT_NAME_COLLISION     0xC0000035
#define STATUS_PORT_DISCONNECTED         0xC0000037
#define STATUS_DEVICE_ALREADY_ATTACHED   0xC0000038
#define STATUS_OBJECT_PATH_INVALID       0xC0000039
#define STATUS_OBJECT_PATH_NOT_FOUND     0xC000003A
#define STATUS_PATH_SYNTAX_BAD           0xC000003B
#define STATUS_DATA_OVERRUN              0xC000003C
#define STATUS_DATA_LATE_ERROR           0xC000003D
#define STATUS_DATA_ERROR                0xC000003E
#define STATUS_CRC_ERROR                 0xC000003F
#define STATUS_SECTION_TOO_BIG           0xC0000040
#define STATUS_PORT_CONNECTION_REFUSED   0xC0000041
#define STATUS_INVALID_PORT_HANDLE       0xC0000042
#define STATUS_SHARING_VIOLATION         0xC0000043
#define STATUS_QUOTA_EXCEEDED            0xC0000044
#define STATUS_INVALID_PAGE_PROTECTION   0xC0000045
#define STATUS_MUTANT_NOT_OWNED          0xC0000046
#define STATUS_SEMAPHORE_LIMIT_EXCEEDED  0xC0000047
#define STATUS_PORT_ALREADY_SET          0xC0000048
#define STATUS_SECTION_NOT_IMAGE         0xC0000049
#define STATUS_SUSPEND_COUNT_EXCEEDED    0xC000004A
#define STATUS_THREAD_IS_TERMINATING     0xC000004B
#define STATUS_BAD_WORKING_SET_LIMIT     0xC000004C
#define STATUS_INCOMPATIBLE_FILE_MAP     0xC000004D
#define STATUS_SECTION_PROTECTION        0xC000004E
#define STATUS_EAS_NOT_SUPPORTED         0xC000004F
#define STATUS_EA_TOO_LARGE              0xC0000050
#define STATUS_NONEXISTENT_EA_ENTRY      0xC0000051
#define STATUS_NO_EAS_ON_FILE            0xC0000052
#define STATUS_EA_CORRUPT_ERROR          0xC0000053
#define STATUS_LOCK_NOT_GRANTED          0xC0000054  /* FIXME: not sure */
#define STATUS_FILE_LOCK_CONFLICT        0xC0000055  /* FIXME: not sure */
#define STATUS_DELETE_PENDING            0xC0000056
#define STATUS_CTL_FILE_NOT_SUPPORTED    0xC0000057
#define	STATUS_UNKNOWN_REVISION          0xC0000058
#define STATUS_REVISION_MISMATCH         0xC0000059
#define STATUS_INVALID_OWNER             0xC000005A
#define STATUS_INVALID_PRIMARY_GROUP     0xC000005B
#define STATUS_NO_IMPERSONATION_TOKEN    0xC000005C
#define STATUS_CANT_DISABLE_MANDATORY    0xC000005D
#define STATUS_NO_LOGON_SERVERS          0xC000005E
#define STATUS_NO_SUCH_LOGON_SESSION     0xC000005F
#define STATUS_NO_SUCH_PRIVILEGE         0xC0000060
#define STATUS_PRIVILEGE_NOT_HELD        0xC0000061
#define STATUS_INVALID_ACCOUNT_NAME      0xC0000062
#define STATUS_USER_EXISTS               0xC0000063
#define STATUS_NO_SUCH_USER              0xC0000064
#define STATUS_GROUP_EXISTS              0xC0000065
#define STATUS_NO_SUCH_GROUP             0xC0000066
#define STATUS_MEMBER_IN_GROUP           0xC0000067
#define STATUS_MEMBER_NOT_IN_GROUP       0xC0000068
#define STATUS_LAST_ADMIN                0xC0000069
#define STATUS_WRONG_PASSWORD            0xC000006A
#define STATUS_ILL_FORMED_PASSWORD       0xC000006B
#define STATUS_PASSWORD_RESTRICTION      0xC000006C
#define STATUS_LOGON_FAILURE             0xC000006D
#define STATUS_ACCOUNT_RESTRICTION       0xC000006E
#define STATUS_INVALID_LOGON_HOURS       0xC000006F
#define STATUS_INVALID_WORKSTATION       0xC0000070
#define STATUS_PASSWORD_EXPIRED          0xC0000071
#define STATUS_ACCOUNT_DISABLED          0xC0000072
#define STATUS_NONE_MAPPED               0xC0000073
#define STATUS_TOO_MANY_LUIDS_REQUESTED  0xC0000074
#define STATUS_LUIDS_EXHAUSTED           0xC0000075
#define STATUS_INVALID_SUB_AUTHORITY     0xC0000076
#define STATUS_INVALID_ACL               0xC0000077
#define STATUS_INVALID_SID               0xC0000078
#define STATUS_INVALID_SECURITY_DESCR    0xC0000079
#define STATUS_PROCEDURE_NOT_FOUND       0xC000007A
#define STATUS_INVALID_IMAGE_FORMAT      0xC000007B
#define STATUS_NO_TOKEN                  0xC000007C
#define STATUS_BAD_INHERITANCE_ACL       0xC000007D
#define STATUS_RANGE_NOT_LOCKED          0xC000007E
#define STATUS_DISK_FULL                 0xC000007F 
#define STATUS_SERVER_DISABLED           0xC0000080
#define STATUS_SERVER_NOT_DISABLED       0xC0000081
#define STATUS_TOO_MANY_GUIDS_REQUESTED  0xC0000082
#define STATUS_GUIDS_EXHAUSTED           0xC0000083
#define STATUS_INVALID_ID_AUTHORITY      0xC0000084
#define STATUS_AGENTS_EXHAUSTED          0xC0000085
#define STATUS_INVALID_VOLUME_LABEL      0xC0000086
#define STATUS_SECTION_NOT_EXTENDED      0xC0000087
#define STATUS_NOT_MAPPED_DATA           0xC0000088
#define STATUS_RESOURCE_DATA_NOT_FOUND   0xC0000089
#define STATUS_RESOURCE_TYPE_NOT_FOUND   0xC000008A
#define STATUS_RESOURCE_NAME_NOT_FOUND   0xC000008B
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
#define STATUS_TOO_MANY_PAGING_FILES     0xC0000097
#define STATUS_FILE_INVALID              0xC0000098
#define STATUS_ALLOTTED_SPACE_EXCEEDED   0xC0000099
#define STATUS_INSUFFICIENT_RESOURCES    0xC000009A
#define STATUS_DFS_EXIT_PATH_FOUND       0xC000009B
#define STATUS_DEVICE_DATA_ERROR         0xC000009C
#define STATUS_DEVICE_NOT_CONNECTED      0xC000009D
#define STATUS_DEVICE_POWER_FAILURE      0xC000009E
#define STATUS_FREE_VM_NOT_AT_BASE       0xC000009F
#define STATUS_MEMORY_NOT_ALLOCATED      0xC00000A0
#define STATUS_WORKING_SET_QUOTA         0xC00000A1
#define STATUS_MEDIA_WRITE_PROTECTED     0xC00000A2
#define STATUS_DEVICE_NOT_READY          0xC00000A3
#define STATUS_INVALID_GROUP_ATTRIBUTES  0xC00000A4
#define STATUS_BAD_IMPERSONATION_LEVEL   0xC00000A5
#define STATUS_CANT_OPEN_ANONYMOUS       0xC00000A6
#define STATUS_BAD_VALIDATION_CLASS      0xC00000A7
#define STATUS_BAD_TOKEN_TYPE            0xC00000A8
#define STATUS_BAD_MASTER_BOOT_RECORD    0xC00000A9
#define STATUS_INSTRUCTION_MISALIGNMENT  0xC00000AA
#define STATUS_INSTANCE_NOT_AVAILABLE    0xC00000AB
#define STATUS_PIPE_NOT_AVAILABLE        0xC00000AC
#define STATUS_INVALID_PIPE_STATE        0xC00000AD
#define STATUS_PIPE_BUSY                 0xC00000AE
#define STATUS_ILLEGAL_FUNCTION          0xC00000AF
#define STATUS_PIPE_DISCONNECTED         0xC00000B0
#define STATUS_PIPE_CLOSING              0xC00000B1
#define STATUS_PIPE_CONNECTED            0xC00000B2
#define STATUS_PIPE_LISTENING            0xC00000B3
#define STATUS_INVALID_READ_MODE         0xC00000B4
#define STATUS_IO_TIMEOUT                0xC00000B5
#define STATUS_FILE_FORCED_CLOSED        0xC00000B6
#define STATUS_PROFILING_NOT_STARTED     0xC00000B7
#define STATUS_PROFILING_NOT_STOPPED     0xC00000B8
#define STATUS_COULD_NOT_INTERPRET       0xC00000B9
#define STATUS_FILE_IS_A_DIRECTORY       0xC00000BA
#define STATUS_NOT_SUPPORTED             0xC00000BB
#define STATUS_REMOTE_NOT_LISTENING      0xC00000BC
#define STATUS_DUPLICATE_NAME            0xC00000BD
#define STATUS_BAD_NETWORK_PATH          0xC00000BE
#define STATUS_NETWORK_BUSY              0xC00000BF
#define STATUS_DEVICE_DOES_NOT_EXIST     0xC00000C0
#define STATUS_TOO_MANY_COMMANDS         0xC00000C1
#define STATUS_ADAPTER_HARDWARE_ERROR    0xC00000C2
#define STATUS_INVALID_NETWORK_RESPONSE  0xC00000C3
#define STATUS_UNEXPECTED_NETWORK_ERROR  0xC00000C4
#define STATUS_BAD_REMOTE_ADAPTER        0xC00000C5
#define STATUS_PRINT_QUEUE_FULL          0xC00000C6
#define STATUS_NO_SPOOL_SPACE            0xC00000C7
#define STATUS_PRINT_CANCELLED           0xC00000C8
#define STATUS_NETWORK_NAME_DELETED      0xC00000C9
#define STATUS_NETWORK_ACCESS_DENIED     0xC00000CA
#define STATUS_BAD_DEVICE_TYPE           0xC00000CB
#define STATUS_BAD_NETWORK_NAME          0xC00000CC
#define STATUS_TOO_MANY_NAMES            0xC00000CD
#define STATUS_TOO_MANY_SESSIONS         0xC00000CE
#define STATUS_SHARING_PAUSED            0xC00000CF
#define STATUS_REQUEST_NOT_ACCEPTED      0xC00000D0
#define STATUS_REDIRECTOR_PAUSED         0xC00000D1
#define STATUS_NET_WRITE_FAULT           0xC00000D2
#define STATUS_PROFILING_AT_LIMIT        0xC00000D3
#define STATUS_NOT_SAME_DEVICE           0xC00000D4
#define STATUS_FILE_RENAMED              0xC00000D5
#define STATUS_VIRTUAL_CIRCUIT_CLOSED    0xC00000D6
#define STATUS_NO_SECURITY_ON_OBJECT     0xC00000D7
#define STATUS_CANT_WAIT                 0xC00000D8
#define STATUS_PIPE_EMPTY                0xC00000D9
#define STATUS_CANT_ACCESS_DOMAIN_INFO   0xC00000DA
#define STATUS_CANT_TERMINATE_SELF       0xC00000DB
#define STATUS_INVALID_SERVER_STATE      0xC00000DC
#define STATUS_INVALID_DOMAIN_STATE      0xC00000DD
#define STATUS_INVALID_DOMAIN_ROLE       0xC00000DE
#define STATUS_NO_SUCH_DOMAIN            0xC00000DF
#define STATUS_DOMAIN_EXISTS             0xC00000E0
#define STATUS_DOMAIN_LIMIT_EXCEEDED     0xC00000E1
#define STATUS_OPLOCK_NOT_GRANTED        0xC00000E2
#define STATUS_INVALID_OPLOCK_PROTOCOL   0xC00000E3
#define STATUS_INTERNAL_DB_CORRUPTION    0xC00000E4
#define STATUS_INTERNAL_ERROR            0xC00000E5
#define STATUS_GENERIC_NOT_MAPPED        0xC00000E6
#define STATUS_BAD_DESCRIPTOR_FORMAT     0xC00000E7
#define STATUS_INVALID_USER_BUFFER       0xC00000E8
#define STATUS_UNEXPECTED_IO_ERROR       0xC00000E9
#define STATUS_UNEXPECTED_MM_CREATE_ERR  0xC00000EA
#define STATUS_UNEXPECTED_MM_MAP_ERROR   0xC00000EB
#define STATUS_UNEXPECTED_MM_EXTEND_ERR  0xC00000EC
#define STATUS_NOT_LOGON_PROCESS         0xC00000ED
#define STATUS_LOGON_SESSION_EXISTS      0xC00000EE
#define STATUS_INVALID_PARAMETER_1       0xC00000EF
#define	STATUS_INVALID_PARAMETER_2       0xC00000F0
#define STATUS_INVALID_PARAMETER_3       0xC00000F1
#define STATUS_INVALID_PARAMETER_4       0xC00000F2
#define STATUS_INVALID_PARAMETER_5       0xC00000F3
#define STATUS_INVALID_PARAMETER_6       0xC00000F4
#define STATUS_INVALID_PARAMETER_7       0xC00000F5
#define STATUS_INVALID_PARAMETER_8       0xC00000F6
#define STATUS_INVALID_PARAMETER_9       0xC00000F7
#define STATUS_INVALID_PARAMETER_10      0xC00000F8
#define STATUS_INVALID_PARAMETER_11      0xC00000F9
#define STATUS_INVALID_PARAMETER_12      0xC00000FA
#define STATUS_REDIRECTOR_NOT_STARTED    0xC00000FB
#define STATUS_REDIRECTOR_STARTED        0xC00000FC
#define STATUS_STACK_OVERFLOW            0xC00000FD
#define STATUS_BAD_FUNCTION_TABLE        0xC00000FF
#define STATUS_VARIABLE_NOT_FOUND        0xC0000100
#define STATUS_DIRECTORY_NOT_EMPTY       0xC0000101
#define STATUS_FILE_CORRUPT_ERROR        0xC0000102
#define STATUS_NOT_A_DIRECTORY           0xC0000103
#define STATUS_BAD_LOGON_SESSION_STATE   0xC0000104
#define STATUS_LOGON_SESSION_COLLISION   0xC0000105
#define STATUS_NAME_TOO_LONG             0xC0000106
#define STATUS_FILES_OPEN                0xC0000107
#define STATUS_CONNECTION_IN_USE         0xC0000108
#define STATUS_MESSAGE_NOT_FOUND         0xC0000109
#define STATUS_PROCESS_IS_TERMINATING    0xC000010A
#define STATUS_INVALID_LOGON_TYPE        0xC000010B
#define STATUS_NO_GUID_TRANSLATION       0xC000010C
#define STATUS_CANNOT_IMPERSONATE        0xC000010D
#define STATUS_IMAGE_ALREADY_LOADED      0xC000010E
#define STATUS_ABIOS_NOT_PRESENT         0xC000010F
#define STATUS_ABIOS_LID_NOT_EXIST       0xC0000110
#define STATUS_ABIOS_LID_ALREADY_OWNED   0xC0000111
#define STATUS_ABIOS_NOT_LID_OWNER       0xC0000112
#define STATUS_ABIOS_INVALID_COMMAND     0xC0000113
#define STATUS_ABIOS_INVALID_LID         0xC0000114
#define STATUS_ABIOS_SELECTOR_NOT_AVAILABLE 0xC0000115
#define STATUS_ABIOS_INVALID_SELECTOR    0xC0000116
#define STATUS_NO_LDT                    0xC0000117
#define STATUS_INVALID_LDT_SIZE          0xC0000118
#define STATUS_INVALID_LDT_OFFSET        0xC0000119
#define STATUS_INVALID_LDT_DESCRIPTOR    0xC000011A
#define STATUS_INVALID_IMAGE_NE_FORMAT   0xC000011B
#define STATUS_RXACT_INVALID_STATE       0xC000011C
#define STATUS_RXACT_COMMIT_FAILURE      0xC000011D
#define STATUS_MAPPED_FILE_SIZE_ZERO     0xC000011E
#define STATUS_TOO_MANY_OPENED_FILES     0xC000011F
#define STATUS_CANCELLED                 0xC0000120
#define STATUS_CANNOT_DELETE             0xC0000121
#define STATUS_INVALID_COMPUTER_NAME     0xC0000122
#define STATUS_FILE_DELETED              0xC0000123
#define STATUS_SPECIAL_ACCOUNT           0xC0000124
#define STATUS_SPECIAL_GROUP             0xC0000125
#define STATUS_SPECIAL_USER              0xC0000126
#define STATUS_MEMBERS_PRIMARY_GROUP     0xC0000127
#define STATUS_FILE_CLOSED               0xC0000128
#define STATUS_TOO_MANY_THREADS          0xC0000129
#define STATUS_THREAD_NOT_IN_PROCESS     0xC000012A
#define STATUS_TOKEN_ALREADY_IN_USE      0xC000012B
#define STATUS_PAGEFILE_QUOTA_EXCEEDED   0xC000012C
#define STATUS_COMMITMENT_LIMIT          0xC000012D
#define STATUS_INVALID_IMAGE_LE_FORMAT   0xC000012E
#define STATUS_INVALID_IMAGE_NOT_MZ      0xC000012F
#define STATUS_INVALID_IMAGE_PROTECT     0xC0000130
#define STATUS_INVALID_IMAGE_WIN_16      0xC0000131
#define STATUS_LOGON_SERVER_CONFLICT     0xC0000132
#define STATUS_TIME_DIFFERENCE_AT_DC     0xC0000133
#define STATUS_SYNCHRONIZATION_REQUIRED  0xC0000134
#define STATUS_DLL_NOT_FOUND             0xC0000135
#define STATUS_OPEN_FAILED               0xC0000136
#define STATUS_IO_PRIVILEGE_FAILED       0xC0000137
#define STATUS_ORDINAL_NOT_FOUND         0xC0000138
#define STATUS_ENTRYPOINT_NOT_FOUND      0xC0000139
#define STATUS_CONTROL_C_EXIT            0xC000013A
#define STATUS_LOCAL_DISCONNECT          0xC000013B
#define STATUS_REMOTE_DISCONNECT         0xC000013C
#define STATUS_REMOTE_RESOURCES          0xC000013D
#define STATUS_LINK_FAILED               0xC000013E
#define STATUS_LINK_TIMEOUT              0xC000013F
#define STATUS_INVALID_CONNECTION        0xC0000140
#define STATUS_INVALID_ADDRESS           0xC0000141
#define STATUS_DLL_INIT_FAILED           0xC0000142
#define STATUS_MISSING_SYSTEMFILE        0xC0000143
#define STATUS_UNHANDLED_EXCEPTION       0xC0000144
#define STATUS_APP_INIT_FAILURE          0xC0000145
#define STATUS_PAGEFILE_CREATE_FAILED    0xC0000146
#define STATUS_NO_PAGEFILE               0xC0000147
#define STATUS_INVALID_LEVEL             0xC0000148
#define STATUS_WRONG_PASSWORD_CORE       0xC0000149
#define STATUS_ILLEGAL_FLOAT_CONTEXT     0xC000014A
#define STATUS_PIPE_BROKEN               0xC000014B
#define STATUS_REGISTRY_CORRUPT          0xC000014C
#define STATUS_REGISTRY_IO_FAILED        0xC000014D
#define STATUS_NO_EVENT_PAIR             0xC000014E
#define STATUS_UNRECOGNIZED_VOLUME       0xC000014F
#define STATUS_SERIAL_NO_DEVICE_INITED   0xC0000150
#define STATUS_NO_SUCH_ALIAS             0xC0000151
#define STATUS_MEMBER_NOT_IN_ALIAS       0xC0000152
#define STATUS_MEMBER_IN_ALIAS           0xC0000153
#define STATUS_ALIAS_EXISTS              0xC0000154
#define STATUS_LOGON_NOT_GRANTED         0xC0000155
#define STATUS_TOO_MANY_SECRETS          0xC0000156
#define STATUS_SECRET_TOO_LONG           0xC0000157
#define STATUS_INTERNAL_DB_ERROR         0xC0000158
#define STATUS_FULLSCREEN_MODE           0xC0000159
#define STATUS_TOO_MANY_CONTEXT_IDS      0xC000015A
#define STATUS_LOGON_TYPE_NOT_GRANTED    0xC000015B
#define STATUS_NOT_REGISTRY_FILE         0xC000015C
#define STATUS_NT_CROSS_ENCRYPTION_REQUIRED 0xC000015D
#define STATUS_DOMAIN_CTRLR_CONFIG_ERROR 0xC000015E
#define STATUS_FT_MISSING_MEMBER         0xC000015F
#define STATUS_ILL_FORMED_SERVICE_ENTRY  0xC0000160
#define STATUS_ILLEGAL_CHARACTER         0xC0000161
#define STATUS_UNMAPPABLE_CHARACTER      0xC0000162
#define STATUS_UNDEFINED_CHARACTER       0xC0000163
#define STATUS_FLOPPY_VOLUME             0xC0000164
#define STATUS_FLOPPY_ID_MARK_NOT_FOUND  0xC0000165
#define STATUS_FLOPPY_WRONG_CYLINDER     0xC0000166
#define STATUS_FLOPPY_UNKNOWN_ERROR      0xC0000167
#define STATUS_FLOPPY_BAD_REGISTERS      0xC0000168
#define STATUS_DISK_RECALIBRATE_FAILED   0xC0000169
#define STATUS_DISK_OPERATION_FAILED     0xC000016A
#define STATUS_DISK_RESET_FAILED         0xC000016B
#define STATUS_SHARED_IRQ_BUSY           0xC000016C
#define STATUS_FT_ORPHANING              0xC000016D
#define STATUS_BIOS_FAILED_TO_CONNECT_INTERRUPT 0xC000016E

#define STATUS_PARTITION_FAILURE         0xC0000172
#define STATUS_INVALID_BLOCK_LENGTH      0xC0000173
#define STATUS_DEVICE_NOT_PARTITIONED    0xC0000174
#define STATUS_UNABLE_TO_LOCK_MEDIA      0xC0000175
#define STATUS_UNABLE_TO_UNLOAD_MEDIA    0xC0000176
#define STATUS_EOM_OVERFLOW              0xC0000177
#define STATUS_NO_MEDIA                  0xC0000178
#define STATUS_NO_SUCH_MEMBER            0xC000017A
#define STATUS_INVALID_MEMBER            0xC000017B
#define STATUS_KEY_DELETED               0xC000017C
#define STATUS_NO_LOG_SPACE              0xC000017D
#define STATUS_TOO_MANY_SIDS             0xC000017E
#define STATUS_LM_CROSS_ENCRYPTION_REQUIRED 0xC000017F
#define STATUS_KEY_HAS_CHILDREN          0xC0000180
#define STATUS_CHILD_MUST_BE_VOLATILE    0xC0000181
#define STATUS_DEVICE_CONFIGURATION_ERROR 0xC0000182
#define STATUS_DRIVER_INTERNAL_ERROR     0xC0000183
#define STATUS_INVALID_DEVICE_STATE      0xC0000184
#define STATUS_IO_DEVICE_ERROR           0xC0000185
#define STATUS_DEVICE_PROTOCOL_ERROR     0xC0000186
#define STATUS_BACKUP_CONTROLLER         0xC0000187
#define STATUS_LOG_FILE_FULL             0xC0000188
#define STATUS_TOO_LATE                  0xC0000189
#define STATUS_NO_TRUST_LSA_SECRET       0xC000018A
#define STATUS_NO_TRUST_SAM_ACCOUNT      0xC000018B
#define STATUS_TRUSTED_DOMAIN_FAILURE    0xC000018C
#define STATUS_TRUSTED_RELATIONSHIP_FAILURE 0xC000018D
#define STATUS_EVENTLOG_FILE_CORRUPT     0xC000018E
#define STATUS_EVENTLOG_CANT_START       0xC000018F
#define STATUS_TRUST_FAILURE             0xC0000190
#define STATUS_MUTANT_LIMIT_EXCEEDED     0xC0000191
#define STATUS_NETLOGON_NOT_STARTED      0xC0000192
#define STATUS_ACCOUNT_EXPIRED           0xC0000193
#define STATUS_POSSIBLE_DEADLOCK         0xC0000194
#define STATUS_NETWORK_CREDENTIAL_CONFLICT 0xC0000195
#define STATUS_REMOTE_SESSION_LIMIT      0xC0000196
#define STATUS_EVENTLOG_FILE_CHANGED     0xC0000197
#define STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT 0xC0000198
#define STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT 0xC0000199
#define STATUS_NOLOGON_SERVER_TRUST_ACCOUNT 0xC000019A
#define STATUS_DOMAIN_TRUST_INCONSISTENT 0xC000019B
#define STATUS_FS_DRIVER_REQUIRED        0xC000019C

#define STATUS_NO_USER_SESSION_KEY       0xC0000202
#define STATUS_USER_SESSION_DELETED      0xC0000203
#define STATUS_RESOURCE_LANG_NOT_FOUND   0xC0000204
#define STATUS_INSUFF_SERVER_RESOURCES   0xC0000205
#define STATUS_INVALID_BUFFER_SIZE       0xC0000206
#define STATUS_INVALID_ADDRESS_COMPONENT 0xC0000207
#define STATUS_INVALID_ADDRESS_WILDCARD  0xC0000208
#define STATUS_TOO_MANY_ADDRESSES        0xC0000209
#define STATUS_ADDRESS_ALREADY_EXISTS    0xC000020A
#define STATUS_ADDRESS_CLOSED            0xC000020B
#define STATUS_CONNECTION_DISCONNECTED   0xC000020C
#define STATUS_CONNECTION_RESET          0xC000020D
#define STATUS_TOO_MANY_NODES            0xC000020E
#define STATUS_TRANSACTION_ABORTED       0xC000020F
#define STATUS_TRANSACTION_TIMED_OUT     0xC0000210
#define STATUS_TRANSACTION_NO_RELEASE    0xC0000211
#define STATUS_TRANSACTION_NO_MATCH      0xC0000212
#define STATUS_TRANSACTION_RESPONDED     0xC0000213
#define STATUS_TRANSACTION_INVALID_ID    0xC0000214
#define STATUS_TRANSACTION_INVALID_TYPE  0xC0000215
#define STATUS_NOT_SERVER_SESSION        0xC0000216
#define STATUS_NOT_CLIENT_SESSION        0xC0000217
#define STATUS_CANNOT_LOAD_REGISTRY_FILE 0xC0000218
#define STATUS_DEBUG_ATTACH_FAILED       0xC0000219
#define STATUS_SYSTEM_PROCESS_TERMINATED 0xC000021A
#define STATUS_DATA_NOT_ACCEPTED         0xC000021B
#define STATUS_NO_BROWSER_SERVERS_FOUND  0xC000021C
#define STATUS_VDM_HARD_ERROR            0xC000021D
#define STATUS_DRIVER_CANCEL_TIMEOUT     0xC000021E
#define STATUS_REPLY_MESSAGE_MISMATCH    0xC000021F
#define STATUS_MAPPED_ALIGNMENT          0xC0000220
#define STATUS_IMAGE_CHECKSUM_MISMATCH   0xC0000221
#define STATUS_LOST_WRITEBEHIND_DATA     0xC0000222
#define STATUS_CLIENT_SERVER_PARAMETERS_INVALID 0xC0000223
#define STATUS_PASSWORD_MUST_CHANGE      0xC0000224
#define STATUS_NOT_FOUND                 0xC0000225
#define STATUS_NOT_TINY_STREAM           0xC0000226
#define STATUS_RECOVERY_FAILURE          0xC0000227
#define STATUS_STACK_OVERFLOW_READ       0xC0000228
#define STATUS_FAIL_CHECK                0xC0000229
#define STATUS_DUPLICATE_OBJECTID        0xC000022A
#define STATUS_OBJECTID_EXISTS           0xC000022B
#define STATUS_CONVERT_TO_LARGE          0xC000022C
#define STATUS_RETRY                     0xC000022D
#define STATUS_FOUND_OUT_OF_SCOPE        0xC000022E
#define STATUS_ALLOCATE_BUCKET           0xC000022F
#define STATUS_PROPSET_NOT_FOUND         0xC0000230
#define STATUS_MARSHALL_OVERFLOW         0xC0000231
#define STATUS_INVALID_VARIANT           0xC0000232
#define STATUS_DOMAIN_CONTROLLER_NOT_FOUND 0xC0000233
#define STATUS_ACCOUNT_LOCKED_OUT        0xC0000234
#define STATUS_HANDLE_NOT_CLOSABLE       0xC0000235
#define STATUS_CONNECTION_REFUSED        0xC0000236
#define STATUS_GRACEFUL_DISCONNECT       0xC0000237
#define STATUS_ADDRESS_ALREADY_ASSOCIATED 0xC0000238
#define STATUS_ADDRESS_NOT_ASSOCIATED    0xC0000239
#define STATUS_CONNECTION_INVALID        0xC000023A
#define STATUS_CONNECTION_ACTIVE         0xC000023B
#define STATUS_NETWORK_UNREACHABLE       0xC000023C
#define STATUS_HOST_UNREACHABLE          0xC000023D
#define STATUS_PROTOCOL_UNREACHABLE      0xC000023E
#define STATUS_PORT_UNREACHABLE          0xC000023F
#define STATUS_REQUEST_ABORTED           0xC0000240
#define STATUS_CONNECTION_ABORTED        0xC0000241
#define STATUS_BAD_COMPRESSION_BUFFER    0xC0000242
#define STATUS_USER_MAPPED_FILE          0xC0000243
#define STATUS_AUDIT_FAILED              0xC0000244
#define STATUS_TIMER_RESOLUTION_NOT_SET  0xC0000245
#define STATUS_CONNECTION_COUNT_LIMIT    0xC0000246
#define STATUS_LOGIN_TIME_RESTRICTION    0xC0000247
#define STATUS_LOGIN_WKSTA_RESTRICTION   0xC0000248
#define STATUS_IMAGE_MP_UP_MISMATCH      0xC0000249
#define STATUS_INSUFFICIENT_LOGON_INFO   0xC0000250
#define STATUS_BAD_DLL_ENTRYPOINT        0xC0000251
#define STATUS_BAD_SERVICE_ENTRYPOINT    0xC0000252
#define STATUS_LPC_REPLY_LOST            0xC0000253
#define STATUS_IP_ADDRESS_CONFLICT1      0xC0000254
#define STATUS_IP_ADDRESS_CONFLICT2      0xC0000255
#define STATUS_REGISTRY_QUOTA_LIMIT      0xC0000256
#define STATUS_PATH_NOT_COVERED          0xC0000257
#define STATUS_NO_CALLBACK_ACTIVE        0xC0000258
#define STATUS_LICENSE_QUOTA_EXCEEDED    0xC0000259
#define STATUS_PWD_TOO_SHORT             0xC000025A
#define STATUS_PWD_TOO_RECENT            0xC000025B
#define STATUS_PWD_HISTORY_CONFLICT      0xC000025C
#define STATUS_PLUGPLAY_NO_DEVICE        0xC000025E
#define STATUS_UNSUPPORTED_COMPRESSION   0xC000025F
#define STATUS_INVALID_HW_PROFILE        0xC0000260
#define STATUS_INVALID_PLUGPLAY_DEVICE_PATH 0xC0000261
#define STATUS_DRIVER_ORDINAL_NOT_FOUND  0xC0000262
#define STATUS_DRIVER_ENTRYPOINT_NOT_FOUND 0xC0000263
#define STATUS_RESOURCE_NOT_OWNED        0xC0000264
#define STATUS_TOO_MANY_LINKS            0xC0000265
#define STATUS_QUOTA_LIST_INCONSISTENT   0xC0000266
#define STATUS_FILE_IS_OFFLINE           0xC0000267
#define STATUS_EVALUATION_EXPIRATION     0xC0000268
#define STATUS_ILLEGAL_DLL_RELOCATION    0xC0000269
#define STATUS_LICENSE_VIOLATION         0xC000026A
#define STATUS_DLL_INIT_FAILED_LOGOFF    0xC000026B
#define STATUS_DRIVER_UNABLE_TO_LOAD     0xC000026C
#define STATUS_DFS_UNAVAILABLE           0xC000026D
#define STATUS_VOLUME_DISMOUNTED         0xC000026E
#define STATUS_WX86_INTERNAL_ERROR       0xC000026F
#define STATUS_WX86_FLOAT_STACK_CHECK    0xC0000270
#define STATUS_WOW_ASSERTION             0xC0009898
#define RPC_NT_INVALID_STRING_BINDING    0xC0020001
#define RPC_NT_WRONG_KIND_OF_BINDING     0xC0020002
#define RPC_NT_INVALID_BINDING           0xC0020003
#define RPC_NT_PROTSEQ_NOT_SUPPORTED     0xC0020004
#define RPC_NT_INVALID_RPC_PROTSEQ       0xC0020005
#define RPC_NT_INVALID_STRING_UUID       0xC0020006
#define RPC_NT_INVALID_ENDPOINT_FORMAT   0xC0020007
#define RPC_NT_INVALID_NET_ADDR          0xC0020008
#define RPC_NT_NO_ENDPOINT_FOUND         0xC0020009
#define RPC_NT_INVALID_TIMEOUT           0xC002000A
#define RPC_NT_OBJECT_NOT_FOUND          0xC002000B
#define RPC_NT_ALREADY_REGISTERED        0xC002000C
#define RPC_NT_TYPE_ALREADY_REGISTERED   0xC002000D
#define RPC_NT_ALREADY_LISTENING         0xC002000E
#define RPC_NT_NO_PROTSEQS_REGISTERED    0xC002000F
#define RPC_NT_NOT_LISTENING             0xC0020010
#define RPC_NT_UNKNOWN_MGR_TYPE          0xC0020011
#define RPC_NT_UNKNOWN_IF                0xC0020012
#define RPC_NT_NO_BINDINGS               0xC0020013
#define RPC_NT_NO_PROTSEQS               0xC0020014
#define RPC_NT_CANT_CREATE_ENDPOINT      0xC0020015
#define RPC_NT_OUT_OF_RESOURCES          0xC0020016
#define RPC_NT_SERVER_UNAVAILABLE        0xC0020017
#define RPC_NT_SERVER_TOO_BUSY           0xC0020018
#define RPC_NT_INVALID_NETWORK_OPTIONS   0xC0020019
#define RPC_NT_NO_CALL_ACTIVE            0xC002001A
#define RPC_NT_CALL_FAILED               0xC002001B
#define RPC_NT_CALL_FAILED_DNE           0xC002001C
#define RPC_NT_PROTOCOL_ERROR            0xC002001D
#define RPC_NT_UNSUPPORTED_TRANS_SYN     0xC002001F
#define RPC_NT_UNSUPPORTED_TYPE          0xC0020021
#define RPC_NT_INVALID_TAG               0xC0020022
#define RPC_NT_INVALID_BOUND             0xC0020023
#define RPC_NT_NO_ENTRY_NAME             0xC0020024
#define RPC_NT_INVALID_NAME_SYNTAX       0xC0020025
#define RPC_NT_UNSUPPORTED_NAME_SYNTAX   0xC0020026
#define RPC_NT_UUID_NO_ADDRESS           0xC0020028
#define RPC_NT_DUPLICATE_ENDPOINT        0xC0020029
#define RPC_NT_UNKNOWN_AUTHN_TYPE        0xC002002A
#define RPC_NT_MAX_CALLS_TOO_SMALL       0xC002002B
#define RPC_NT_STRING_TOO_LONG           0xC002002C
#define RPC_NT_PROTSEQ_NOT_FOUND         0xC002002D
#define RPC_NT_PROCNUM_OUT_OF_RANGE      0xC002002E
#define RPC_NT_BINDING_HAS_NO_AUTH       0xC002002F
#define RPC_NT_UNKNOWN_AUTHN_SERVICE     0xC0020030
#define RPC_NT_UNKNOWN_AUTHN_LEVEL       0xC0020031
#define RPC_NT_INVALID_AUTH_IDENTITY     0xC0020032
#define RPC_NT_UNKNOWN_AUTHZ_SERVICE     0xC0020033
#define EPT_NT_INVALID_ENTRY             0xC0020034
#define EPT_NT_CANT_PERFORM_OP           0xC0020035
#define EPT_NT_NOT_REGISTERED            0xC0020036
#define RPC_NT_NOTHING_TO_EXPORT         0xC0020037
#define RPC_NT_INCOMPLETE_NAME           0xC0020038
#define RPC_NT_INVALID_VERS_OPTION       0xC0020039
#define RPC_NT_NO_MORE_MEMBERS           0xC002003A
#define RPC_NT_NOT_ALL_OBJS_UNEXPORTED   0xC002003B
#define RPC_NT_INTERFACE_NOT_FOUND       0xC002003C
#define RPC_NT_ENTRY_ALREADY_EXISTS      0xC002003D
#define RPC_NT_ENTRY_NOT_FOUND           0xC002003E
#define RPC_NT_NAME_SERVICE_UNAVAILABLE  0xC002003F
#define RPC_NT_INVALID_NAF_ID            0xC0020040
#define RPC_NT_CANNOT_SUPPORT            0xC0020041
#define RPC_NT_NO_CONTEXT_AVAILABLE      0xC0020042
#define RPC_NT_INTERNAL_ERROR            0xC0020043
#define RPC_NT_ZERO_DIVIDE               0xC0020044
#define RPC_NT_ADDRESS_ERROR             0xC0020045
#define RPC_NT_FP_DIV_ZERO               0xC0020046
#define RPC_NT_FP_UNDERFLOW              0xC0020047
#define RPC_NT_FP_OVERFLOW               0xC0020048
#define RPC_NT_NO_MORE_ENTRIES           0xC0030001
#define RPC_NT_SS_CHAR_TRANS_OPEN_FAIL   0xC0030002
#define RPC_NT_SS_CHAR_TRANS_SHORT_FILE  0xC0030003
#define RPC_NT_SS_IN_NULL_CONTEXT        0xC0030004
#define RPC_NT_SS_CONTEXT_MISMATCH       0xC0030005
#define RPC_NT_SS_CONTEXT_DAMAGED        0xC0030006
#define RPC_NT_SS_HANDLES_MISMATCH       0xC0030007
#define RPC_NT_SS_CANNOT_GET_CALL_HANDLE 0xC0030008
#define RPC_NT_NULL_REF_POINTER          0xC0030009
#define RPC_NT_ENUM_VALUE_OUT_OF_RANGE   0xC003000A
#define RPC_NT_BYTE_COUNT_TOO_SMALL      0xC003000B
#define RPC_NT_BAD_STUB_DATA             0xC003000C
#define RPC_NT_CALL_IN_PROGRESS          0xC0020049
#define RPC_NT_NO_MORE_BINDINGS          0xC002004A
#define RPC_NT_GROUP_MEMBER_NOT_FOUND    0xC002004B
#define EPT_NT_CANT_CREATE               0xC002004C
#define RPC_NT_INVALID_OBJECT            0xC002004D
#define RPC_NT_NO_INTERFACES             0xC002004F
#define RPC_NT_CALL_CANCELLED            0xC0020050
#define RPC_NT_BINDING_INCOMPLETE        0xC0020051
#define RPC_NT_COMM_FAILURE              0xC0020052
#define RPC_NT_UNSUPPORTED_AUTHN_LEVEL   0xC0020053
#define RPC_NT_NO_PRINC_NAME             0xC0020054
#define RPC_NT_NOT_RPC_ERROR             0xC0020055
#define RPC_NT_UUID_LOCAL_ONLY           0x40020056
#define RPC_NT_SEC_PKG_ERROR             0xC0020057
#define RPC_NT_NOT_CANCELLED             0xC0020058
#define RPC_NT_INVALID_ES_ACTION         0xC0030059
#define RPC_NT_WRONG_ES_VERSION          0xC003005A
#define RPC_NT_WRONG_STUB_VERSION        0xC003005B
#define RPC_NT_INVALID_PIPE_OBJECT       0xC003005C
#define RPC_NT_INVALID_PIPE_OPERATION    0xC003005D
#define RPC_NT_WRONG_PIPE_VERSION        0xC003005E
#define RPC_NT_SEND_INCOMPLETE           0x400200AF

#define MAXIMUM_WAIT_OBJECTS 64
#define MAXIMUM_SUSPEND_COUNT 127


/*
 * Return values from the actual exception handlers
 */

#define ExceptionContinueExecution 0
#define ExceptionContinueSearch    1
#define ExceptionNestedException   2
#define ExceptionCollidedUnwind    3
 
/*
 * Return values from filters in except() and from UnhandledExceptionFilter
 */
 
#define EXCEPTION_EXECUTE_HANDLER        1
#define EXCEPTION_CONTINUE_SEARCH        0
#define EXCEPTION_CONTINUE_EXECUTION    -1

/*
 * From OS/2 2.0 exception handling
 * Win32 seems to use the same flags as ExceptionFlags in an EXCEPTION_RECORD
 */

#define EH_NONCONTINUABLE   0x01
#define EH_UNWINDING        0x02
#define EH_EXIT_UNWIND      0x04
#define EH_STACK_INVALID    0x08
#define EH_NESTED_CALL      0x10

#define EXCEPTION_CONTINUABLE        0
#define EXCEPTION_NONCONTINUABLE     EH_NONCONTINUABLE
 
/*
 * The exception record used by Win32 to give additional information 
 * about exception to exception handlers.
 */

#define EXCEPTION_MAXIMUM_PARAMETERS 15

typedef struct __EXCEPTION_RECORD
{
    DWORD    ExceptionCode;
    DWORD    ExceptionFlags;
    struct __EXCEPTION_RECORD *ExceptionRecord;

    LPVOID   ExceptionAddress;
    DWORD    NumberParameters;
    DWORD    ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD;

/*
 * The exception pointers structure passed to exception filters
 * in except() and the UnhandledExceptionFilter().
 */
 
typedef struct _EXCEPTION_POINTERS 
{
  PEXCEPTION_RECORD  ExceptionRecord;
  PCONTEXT           ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;


/*
 * The exception frame, used for registering exception handlers 
 * Win32 cares only about this, but compilers generally emit 
 * larger exception frames for their own use.
 */

struct __EXCEPTION_FRAME;

typedef DWORD (*PEXCEPTION_HANDLER)(PEXCEPTION_RECORD,struct __EXCEPTION_FRAME*,
                                    PCONTEXT,struct __EXCEPTION_FRAME **);

typedef struct __EXCEPTION_FRAME
{
  struct __EXCEPTION_FRAME *Prev;
  PEXCEPTION_HANDLER       Handler;
} EXCEPTION_FRAME, *PEXCEPTION_FRAME;

#include "poppack.h"

/*
 * function pointer to a exception filter
 */

typedef LONG CALLBACK (*PTOP_LEVEL_EXCEPTION_FILTER)(PEXCEPTION_POINTERS ExceptionInfo);
typedef PTOP_LEVEL_EXCEPTION_FILTER LPTOP_LEVEL_EXCEPTION_FILTER;

DWORD WINAPI UnhandledExceptionFilter( PEXCEPTION_POINTERS epointers );
LPTOP_LEVEL_EXCEPTION_FILTER
WINAPI SetUnhandledExceptionFilter( LPTOP_LEVEL_EXCEPTION_FILTER filter );

/* status values for ContinueDebugEvent */
#define DBG_CONTINUE                0x00010002
#define DBG_TERMINATE_THREAD        0x40010003
#define DBG_TERMINATE_PROCESS       0x40010004
#define DBG_CONTROL_C               0x40010005
#define DBG_CONTROL_BREAK           0x40010008
#define DBG_EXCEPTION_NOT_HANDLED   0x80010001

typedef struct _NT_TIB 
{
	struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
	PVOID StackBase;
	PVOID StackLimit;
	PVOID SubSystemTib;
	union {
          PVOID FiberData;
          DWORD Version;
	} DUMMYUNIONNAME;
	PVOID ArbitraryUserPointer;
	struct _NT_TIB *Self;
} NT_TIB, *PNT_TIB;

struct _TEB;

#if defined(__i386__) && defined(__GNUC__)
extern inline struct _TEB WINAPI *NtCurrentTeb(void);
extern inline struct _TEB WINAPI *NtCurrentTeb(void)
{
    struct _TEB *teb;
    __asm__(".byte 0x64\n\tmovl (0x18),%0" : "=r" (teb));
    return teb;
}
#else
extern struct _TEB WINAPI *NtCurrentTeb(void);
#endif


/*
 * File formats definitions
 */

typedef struct _IMAGE_DOS_HEADER {
    WORD  e_magic;      /* 00: MZ Header signature */
    WORD  e_cblp;       /* 02: Bytes on last page of file */
    WORD  e_cp;         /* 04: Pages in file */
    WORD  e_crlc;       /* 06: Relocations */
    WORD  e_cparhdr;    /* 08: Size of header in paragraphs */
    WORD  e_minalloc;   /* 0a: Minimum extra paragraphs needed */
    WORD  e_maxalloc;   /* 0c: Maximum extra paragraphs needed */
    WORD  e_ss;         /* 0e: Initial (relative) SS value */
    WORD  e_sp;         /* 10: Initial SP value */
    WORD  e_csum;       /* 12: Checksum */
    WORD  e_ip;         /* 14: Initial IP value */
    WORD  e_cs;         /* 16: Initial (relative) CS value */
    WORD  e_lfarlc;     /* 18: File address of relocation table */
    WORD  e_ovno;       /* 1a: Overlay number */
    WORD  e_res[4];     /* 1c: Reserved words */
    WORD  e_oemid;      /* 24: OEM identifier (for e_oeminfo) */
    WORD  e_oeminfo;    /* 26: OEM information; e_oemid specific */
    WORD  e_res2[10];   /* 28: Reserved words */
    DWORD e_lfanew;     /* 3c: Offset to extended header */
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

#define IMAGE_DOS_SIGNATURE    0x5A4D     /* MZ   */
#define IMAGE_OS2_SIGNATURE    0x454E     /* NE   */
#define IMAGE_OS2_SIGNATURE_LE 0x454C     /* LE   */
#define IMAGE_OS2_SIGNATURE_LX 0x584C     /* LX */
#define IMAGE_VXD_SIGNATURE    0x454C     /* LE   */
#define IMAGE_NT_SIGNATURE     0x00004550 /* PE00 */

/*
 * This is the Windows executable (NE) header.
 * the name IMAGE_OS2_HEADER is misleading, but in the SDK this way.
 */
typedef struct 
{
    WORD  ne_magic;             /* 00 NE signature 'NE' */
    BYTE  ne_ver;               /* 02 Linker version number */
    BYTE  ne_rev;               /* 03 Linker revision number */
    WORD  ne_enttab;            /* 04 Offset to entry table relative to NE */
    WORD  ne_cbenttab;          /* 06 Length of entry table in bytes */
    LONG  ne_crc;               /* 08 Checksum */
    WORD  ne_flags;             /* 0c Flags about segments in this file */
    WORD  ne_autodata;          /* 0e Automatic data segment number */
    WORD  ne_heap;              /* 10 Initial size of local heap */
    WORD  ne_stack;             /* 12 Initial size of stack */
    DWORD ne_csip;              /* 14 Initial CS:IP */
    DWORD ne_sssp;              /* 18 Initial SS:SP */
    WORD  ne_cseg;              /* 1c # of entries in segment table */
    WORD  ne_cmod;              /* 1e # of entries in module reference tab. */
    WORD  ne_cbnrestab;         /* 20 Length of nonresident-name table     */
    WORD  ne_segtab;            /* 22 Offset to segment table */
    WORD  ne_rsrctab;           /* 24 Offset to resource table */
    WORD  ne_restab;            /* 26 Offset to resident-name table */
    WORD  ne_modtab;            /* 28 Offset to module reference table */
    WORD  ne_imptab;            /* 2a Offset to imported name table */
    DWORD ne_nrestab;           /* 2c Offset to nonresident-name table */
    WORD  ne_cmovent;           /* 30 # of movable entry points */
    WORD  ne_align;             /* 32 Logical sector alignment shift count */
    WORD  ne_cres;              /* 34 # of resource segments */
    BYTE  ne_exetyp;            /* 36 Flags indicating target OS */
    BYTE  ne_flagsothers;       /* 37 Additional information flags */
    WORD  fastload_offset;      /* 38 Offset to fast load area (should be ne_pretthunks)*/
    WORD  fastload_length;      /* 3a Length of fast load area (should be ne_psegrefbytes) */
    WORD  ne_swaparea;          /* 3c Reserved by Microsoft */
    WORD  ne_expver;            /* 3e Expected Windows version number */
} IMAGE_OS2_HEADER,*PIMAGE_OS2_HEADER;

typedef struct _IMAGE_VXD_HEADER {
  WORD  e32_magic;
  BYTE  e32_border;
  BYTE  e32_worder;
  DWORD e32_level;
  WORD  e32_cpu;
  WORD  e32_os;
  DWORD e32_ver;
  DWORD e32_mflags;
  DWORD e32_mpages;
  DWORD e32_startobj;
  DWORD e32_eip;
  DWORD e32_stackobj;
  DWORD e32_esp;
  DWORD e32_pagesize;
  DWORD e32_lastpagesize;
  DWORD e32_fixupsize;
  DWORD e32_fixupsum;
  DWORD e32_ldrsize;
  DWORD e32_ldrsum;
  DWORD e32_objtab;
  DWORD e32_objcnt;
  DWORD e32_objmap;
  DWORD e32_itermap;
  DWORD e32_rsrctab;
  DWORD e32_rsrccnt;
  DWORD e32_restab;
  DWORD e32_enttab;
  DWORD e32_dirtab;
  DWORD e32_dircnt;
  DWORD e32_fpagetab;
  DWORD e32_frectab;
  DWORD e32_impmod;
  DWORD e32_impmodcnt;
  DWORD e32_impproc;
  DWORD e32_pagesum;
  DWORD e32_datapage;
  DWORD e32_preload;
  DWORD e32_nrestab;
  DWORD e32_cbnrestab;
  DWORD e32_nressum;
  DWORD e32_autodata;
  DWORD e32_debuginfo;
  DWORD e32_debuglen;
  DWORD e32_instpreload;
  DWORD e32_instdemand;
  DWORD e32_heapsize;
  BYTE   e32_res3[12];
  DWORD e32_winresoff;
  DWORD e32_winreslen;
  WORD  e32_devid;
  WORD  e32_ddkver;
} IMAGE_VXD_HEADER, *PIMAGE_VXD_HEADER;


/* These defines describe the meanings of the bits in the Characteristics
   field */

#define IMAGE_FILE_RELOCS_STRIPPED	0x0001 /* No relocation info */
#define IMAGE_FILE_EXECUTABLE_IMAGE	0x0002
#define IMAGE_FILE_LINE_NUMS_STRIPPED   0x0004
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED  0x0008
#define IMAGE_FILE_16BIT_MACHINE	0x0040
#define IMAGE_FILE_BYTES_REVERSED_LO	0x0080
#define IMAGE_FILE_32BIT_MACHINE	0x0100
#define IMAGE_FILE_DEBUG_STRIPPED	0x0200
#define IMAGE_FILE_SYSTEM		0x1000
#define IMAGE_FILE_DLL			0x2000
#define IMAGE_FILE_BYTES_REVERSED_HI	0x8000

/* These are the settings of the Machine field. */
#define	IMAGE_FILE_MACHINE_UNKNOWN	0
#define	IMAGE_FILE_MACHINE_I860		0x14d
#define	IMAGE_FILE_MACHINE_I386		0x14c
#define	IMAGE_FILE_MACHINE_R3000	0x162
#define	IMAGE_FILE_MACHINE_R4000	0x166
#define	IMAGE_FILE_MACHINE_R10000	0x168
#define	IMAGE_FILE_MACHINE_ALPHA	0x184
#define	IMAGE_FILE_MACHINE_POWERPC	0x1F0  

#define	IMAGE_SIZEOF_FILE_HEADER	20

/* Possible Magic values */
#define IMAGE_NT_OPTIONAL_HDR_MAGIC        0x10b
#define IMAGE_ROM_OPTIONAL_HDR_MAGIC       0x107

/* These are indexes into the DataDirectory array */
#define IMAGE_FILE_EXPORT_DIRECTORY		0
#define IMAGE_FILE_IMPORT_DIRECTORY		1
#define IMAGE_FILE_RESOURCE_DIRECTORY		2
#define IMAGE_FILE_EXCEPTION_DIRECTORY		3
#define IMAGE_FILE_SECURITY_DIRECTORY		4
#define IMAGE_FILE_BASE_RELOCATION_TABLE	5
#define IMAGE_FILE_DEBUG_DIRECTORY		6
#define IMAGE_FILE_DESCRIPTION_STRING		7
#define IMAGE_FILE_MACHINE_VALUE		8  /* Mips */
#define IMAGE_FILE_THREAD_LOCAL_STORAGE		9
#define IMAGE_FILE_CALLBACK_DIRECTORY		10

/* Directory Entries, indices into the DataDirectory array */

#define	IMAGE_DIRECTORY_ENTRY_EXPORT		0
#define	IMAGE_DIRECTORY_ENTRY_IMPORT		1
#define	IMAGE_DIRECTORY_ENTRY_RESOURCE		2
#define	IMAGE_DIRECTORY_ENTRY_EXCEPTION		3
#define	IMAGE_DIRECTORY_ENTRY_SECURITY		4
#define	IMAGE_DIRECTORY_ENTRY_BASERELOC		5
#define	IMAGE_DIRECTORY_ENTRY_DEBUG		6
#define	IMAGE_DIRECTORY_ENTRY_COPYRIGHT		7
#define	IMAGE_DIRECTORY_ENTRY_GLOBALPTR		8   /* (MIPS GP) */
#define	IMAGE_DIRECTORY_ENTRY_TLS		9
#define	IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG	10
#define	IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT	11
#define	IMAGE_DIRECTORY_ENTRY_IAT		12  /* Import Address Table */
#define	IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT	13
#define	IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR	14

/* Subsystem Values */

#define	IMAGE_SUBSYSTEM_UNKNOWN		0
#define	IMAGE_SUBSYSTEM_NATIVE		1
#define	IMAGE_SUBSYSTEM_WINDOWS_GUI	2	/* Windows GUI subsystem */
#define	IMAGE_SUBSYSTEM_WINDOWS_CUI	3	/* Windows character subsystem*/
#define	IMAGE_SUBSYSTEM_OS2_CUI		5
#define	IMAGE_SUBSYSTEM_POSIX_CUI	7

typedef struct _IMAGE_FILE_HEADER {
  WORD  Machine;
  WORD  NumberOfSections;
  DWORD TimeDateStamp;
  DWORD PointerToSymbolTable;
  DWORD NumberOfSymbols;
  WORD  SizeOfOptionalHeader;
  WORD  Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
  DWORD VirtualAddress;
  DWORD Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct _IMAGE_OPTIONAL_HEADER {

  /* Standard fields */

  WORD  Magic; /* 0x10b or 0x107 */	/* 0x00 */
  BYTE  MajorLinkerVersion;
  BYTE  MinorLinkerVersion;
  DWORD SizeOfCode;
  DWORD SizeOfInitializedData;
  DWORD SizeOfUninitializedData;
  DWORD AddressOfEntryPoint;		/* 0x10 */
  DWORD BaseOfCode;
  DWORD BaseOfData;

  /* NT additional fields */

  DWORD ImageBase;
  DWORD SectionAlignment;		/* 0x20 */
  DWORD FileAlignment;
  WORD  MajorOperatingSystemVersion;
  WORD  MinorOperatingSystemVersion;
  WORD  MajorImageVersion;
  WORD  MinorImageVersion;
  WORD  MajorSubsystemVersion;		/* 0x30 */
  WORD  MinorSubsystemVersion;
  DWORD Win32VersionValue;
  DWORD SizeOfImage;
  DWORD SizeOfHeaders;
  DWORD CheckSum;			/* 0x40 */
  WORD  Subsystem;
  WORD  DllCharacteristics;
  DWORD SizeOfStackReserve;
  DWORD SizeOfStackCommit;
  DWORD SizeOfHeapReserve;		/* 0x50 */
  DWORD SizeOfHeapCommit;
  DWORD LoaderFlags;
  DWORD NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES]; /* 0x60 */
} IMAGE_OPTIONAL_HEADER, *PIMAGE_OPTIONAL_HEADER;

typedef struct _IMAGE_NT_HEADERS {
  DWORD Signature; /* "PE"\0\0 */
  IMAGE_FILE_HEADER FileHeader;
  IMAGE_OPTIONAL_HEADER OptionalHeader;
} IMAGE_NT_HEADERS, *PIMAGE_NT_HEADERS;

#define IMAGE_SIZEOF_SHORT_NAME 8

typedef struct _IMAGE_SECTION_HEADER {
  BYTE  Name[IMAGE_SIZEOF_SHORT_NAME];
  union {
    DWORD PhysicalAddress;
    DWORD VirtualSize;
  } Misc;
  DWORD VirtualAddress;
  DWORD SizeOfRawData;
  DWORD PointerToRawData;
  DWORD PointerToRelocations;
  DWORD PointerToLinenumbers;
  WORD  NumberOfRelocations;
  WORD  NumberOfLinenumbers;
  DWORD Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

#define	IMAGE_SIZEOF_SECTION_HEADER 40

#define IMAGE_FIRST_SECTION(ntheader) \
  ((PIMAGE_SECTION_HEADER)((LPBYTE)&((PIMAGE_NT_HEADERS)(ntheader))->OptionalHeader + \
                           ((PIMAGE_NT_HEADERS)(ntheader))->FileHeader.SizeOfOptionalHeader))

/* These defines are for the Characteristics bitfield. */
/* #define IMAGE_SCN_TYPE_REG			0x00000000 - Reserved */
/* #define IMAGE_SCN_TYPE_DSECT			0x00000001 - Reserved */
/* #define IMAGE_SCN_TYPE_NOLOAD		0x00000002 - Reserved */
/* #define IMAGE_SCN_TYPE_GROUP			0x00000004 - Reserved */
/* #define IMAGE_SCN_TYPE_NO_PAD		0x00000008 - Reserved */
/* #define IMAGE_SCN_TYPE_COPY			0x00000010 - Reserved */

#define IMAGE_SCN_CNT_CODE			0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA		0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA	0x00000080

#define	IMAGE_SCN_LNK_OTHER			0x00000100 
#define	IMAGE_SCN_LNK_INFO			0x00000200  
/* #define	IMAGE_SCN_TYPE_OVER		0x00000400 - Reserved */
#define	IMAGE_SCN_LNK_REMOVE			0x00000800
#define	IMAGE_SCN_LNK_COMDAT			0x00001000

/* 						0x00002000 - Reserved */
/* #define IMAGE_SCN_MEM_PROTECTED 		0x00004000 - Obsolete */
#define	IMAGE_SCN_MEM_FARDATA			0x00008000

/* #define IMAGE_SCN_MEM_SYSHEAP		0x00010000 - Obsolete */
#define	IMAGE_SCN_MEM_PURGEABLE			0x00020000
#define	IMAGE_SCN_MEM_16BIT			0x00020000
#define	IMAGE_SCN_MEM_LOCKED			0x00040000
#define	IMAGE_SCN_MEM_PRELOAD			0x00080000

#define	IMAGE_SCN_ALIGN_1BYTES			0x00100000
#define	IMAGE_SCN_ALIGN_2BYTES			0x00200000
#define	IMAGE_SCN_ALIGN_4BYTES			0x00300000
#define	IMAGE_SCN_ALIGN_8BYTES			0x00400000
#define	IMAGE_SCN_ALIGN_16BYTES			0x00500000  /* Default */
#define IMAGE_SCN_ALIGN_32BYTES			0x00600000
#define IMAGE_SCN_ALIGN_64BYTES			0x00700000
/* 						0x00800000 - Unused */

#define IMAGE_SCN_LNK_NRELOC_OVFL		0x01000000


#define IMAGE_SCN_MEM_DISCARDABLE		0x02000000
#define IMAGE_SCN_MEM_NOT_CACHED		0x04000000
#define IMAGE_SCN_MEM_NOT_PAGED			0x08000000
#define IMAGE_SCN_MEM_SHARED			0x10000000
#define IMAGE_SCN_MEM_EXECUTE			0x20000000
#define IMAGE_SCN_MEM_READ			0x40000000
#define IMAGE_SCN_MEM_WRITE			0x80000000

#include "pshpack2.h"

typedef struct _IMAGE_SYMBOL {
    union {
        BYTE    ShortName[8];
        struct {
            DWORD   Short;
            DWORD   Long;
        } Name;
        DWORD   LongName[2];
    } N;
    DWORD   Value;
    SHORT   SectionNumber;
    WORD    Type;
    BYTE    StorageClass;
    BYTE    NumberOfAuxSymbols;
} IMAGE_SYMBOL;
typedef IMAGE_SYMBOL *PIMAGE_SYMBOL;

#define IMAGE_SIZEOF_SYMBOL 18

typedef struct _IMAGE_LINENUMBER {
    union {
        DWORD   SymbolTableIndex;
        DWORD   VirtualAddress;
    } Type;
    WORD    Linenumber;
} IMAGE_LINENUMBER;
typedef IMAGE_LINENUMBER *PIMAGE_LINENUMBER;

#define IMAGE_SIZEOF_LINENUMBER  6

typedef union _IMAGE_AUX_SYMBOL {
    struct {
        DWORD    TagIndex;
        union {
            struct {
                WORD    Linenumber;
                WORD    Size;
            } LnSz;
           DWORD    TotalSize;
        } Misc;
        union {
            struct {
                DWORD    PointerToLinenumber;
                DWORD    PointerToNextFunction;
            } Function;
            struct {
                WORD     Dimension[4];
            } Array;
        } FcnAry;
        WORD    TvIndex;
    } Sym;
    struct {
        BYTE    Name[IMAGE_SIZEOF_SYMBOL];
    } File;
    struct {
        DWORD   Length;
        WORD    NumberOfRelocations;
        WORD    NumberOfLinenumbers;
        DWORD   CheckSum;
        SHORT   Number;
        BYTE    Selection;
    } Section;
} IMAGE_AUX_SYMBOL;
typedef IMAGE_AUX_SYMBOL *PIMAGE_AUX_SYMBOL;

#define IMAGE_SIZEOF_AUX_SYMBOL 18

#include "poppack.h"

#define IMAGE_SYM_UNDEFINED           (SHORT)0
#define IMAGE_SYM_ABSOLUTE            (SHORT)-1
#define IMAGE_SYM_DEBUG               (SHORT)-2

#define IMAGE_SYM_TYPE_NULL                 0x0000
#define IMAGE_SYM_TYPE_VOID                 0x0001
#define IMAGE_SYM_TYPE_CHAR                 0x0002
#define IMAGE_SYM_TYPE_SHORT                0x0003
#define IMAGE_SYM_TYPE_INT                  0x0004
#define IMAGE_SYM_TYPE_LONG                 0x0005
#define IMAGE_SYM_TYPE_FLOAT                0x0006
#define IMAGE_SYM_TYPE_DOUBLE               0x0007
#define IMAGE_SYM_TYPE_STRUCT               0x0008
#define IMAGE_SYM_TYPE_UNION                0x0009
#define IMAGE_SYM_TYPE_ENUM                 0x000A
#define IMAGE_SYM_TYPE_MOE                  0x000B
#define IMAGE_SYM_TYPE_BYTE                 0x000C
#define IMAGE_SYM_TYPE_WORD                 0x000D
#define IMAGE_SYM_TYPE_UINT                 0x000E
#define IMAGE_SYM_TYPE_DWORD                0x000F
#define IMAGE_SYM_TYPE_PCODE                0x8000

#define IMAGE_SYM_DTYPE_NULL                0
#define IMAGE_SYM_DTYPE_POINTER             1
#define IMAGE_SYM_DTYPE_FUNCTION            2
#define IMAGE_SYM_DTYPE_ARRAY               3

#define IMAGE_SYM_CLASS_END_OF_FUNCTION     (BYTE )-1
#define IMAGE_SYM_CLASS_NULL                0x0000
#define IMAGE_SYM_CLASS_AUTOMATIC           0x0001
#define IMAGE_SYM_CLASS_EXTERNAL            0x0002
#define IMAGE_SYM_CLASS_STATIC              0x0003
#define IMAGE_SYM_CLASS_REGISTER            0x0004
#define IMAGE_SYM_CLASS_EXTERNAL_DEF        0x0005
#define IMAGE_SYM_CLASS_LABEL               0x0006
#define IMAGE_SYM_CLASS_UNDEFINED_LABEL     0x0007
#define IMAGE_SYM_CLASS_MEMBER_OF_STRUCT    0x0008
#define IMAGE_SYM_CLASS_ARGUMENT            0x0009
#define IMAGE_SYM_CLASS_STRUCT_TAG          0x000A
#define IMAGE_SYM_CLASS_MEMBER_OF_UNION     0x000B
#define IMAGE_SYM_CLASS_UNION_TAG           0x000C
#define IMAGE_SYM_CLASS_TYPE_DEFINITION     0x000D
#define IMAGE_SYM_CLASS_UNDEFINED_STATIC    0x000E
#define IMAGE_SYM_CLASS_ENUM_TAG            0x000F
#define IMAGE_SYM_CLASS_MEMBER_OF_ENUM      0x0010
#define IMAGE_SYM_CLASS_REGISTER_PARAM      0x0011
#define IMAGE_SYM_CLASS_BIT_FIELD           0x0012

#define IMAGE_SYM_CLASS_FAR_EXTERNAL        0x0044
#define IMAGE_SYM_CLASS_BLOCK               0x0064
#define IMAGE_SYM_CLASS_FUNCTION            0x0065
#define IMAGE_SYM_CLASS_END_OF_STRUCT       0x0066
#define IMAGE_SYM_CLASS_FILE                0x0067
#define IMAGE_SYM_CLASS_SECTION             0x0068
#define IMAGE_SYM_CLASS_WEAK_EXTERNAL       0x0069

#define N_BTMASK                            0x000F
#define N_TMASK                             0x0030
#define N_TMASK1                            0x00C0
#define N_TMASK2                            0x00F0
#define N_BTSHFT                            4
#define N_TSHIFT                            2

#define BTYPE(x) ((x) & N_BTMASK)

#ifndef ISPTR
#define ISPTR(x) (((x) & N_TMASK) == (IMAGE_SYM_DTYPE_POINTER << N_BTSHFT))
#endif

#ifndef ISFCN
#define ISFCN(x) (((x) & N_TMASK) == (IMAGE_SYM_DTYPE_FUNCTION << N_BTSHFT))
#endif

#ifndef ISARY
#define ISARY(x) (((x) & N_TMASK) == (IMAGE_SYM_DTYPE_ARRAY << N_BTSHFT))
#endif

#ifndef ISTAG
#define ISTAG(x) ((x)==IMAGE_SYM_CLASS_STRUCT_TAG || (x)==IMAGE_SYM_CLASS_UNION_TAG || (x)==IMAGE_SYM_CLASS_ENUM_TAG)
#endif

#ifndef INCREF
#define INCREF(x) ((((x)&~N_BTMASK)<<N_TSHIFT)|(IMAGE_SYM_DTYPE_POINTER<<N_BTSHFT)|((x)&N_BTMASK))
#endif
#ifndef DECREF
#define DECREF(x) ((((x)>>N_TSHIFT)&~N_BTMASK)|((x)&N_BTMASK))
#endif

#define IMAGE_COMDAT_SELECT_NODUPLICATES    1
#define IMAGE_COMDAT_SELECT_ANY             2
#define IMAGE_COMDAT_SELECT_SAME_SIZE       3
#define IMAGE_COMDAT_SELECT_EXACT_MATCH     4
#define IMAGE_COMDAT_SELECT_ASSOCIATIVE     5
#define IMAGE_COMDAT_SELECT_LARGEST         6
#define IMAGE_COMDAT_SELECT_NEWEST          7

#define IMAGE_WEAK_EXTERN_SEARCH_NOLIBRARY  1
#define IMAGE_WEAK_EXTERN_SEARCH_LIBRARY    2
#define IMAGE_WEAK_EXTERN_SEARCH_ALIAS      3

/* Export module directory */

typedef struct _IMAGE_EXPORT_DIRECTORY {
	DWORD	Characteristics;
	DWORD	TimeDateStamp;
	WORD	MajorVersion;
	WORD	MinorVersion;
	DWORD	Name;
	DWORD	Base;
	DWORD	NumberOfFunctions;
	DWORD	NumberOfNames;
	DWORD	AddressOfFunctions;
	DWORD	AddressOfNames;
	DWORD	AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY,*PIMAGE_EXPORT_DIRECTORY;

/* Import name entry */
typedef struct _IMAGE_IMPORT_BY_NAME {
	WORD	Hint;
	BYTE	Name[1];
} IMAGE_IMPORT_BY_NAME,*PIMAGE_IMPORT_BY_NAME;

/* Import thunk */
typedef struct _IMAGE_THUNK_DATA {
	union {
		LPBYTE    ForwarderString;
		PDWORD    Function;
		DWORD     Ordinal;
		PIMAGE_IMPORT_BY_NAME	AddressOfData;
	} u1;
} IMAGE_THUNK_DATA,*PIMAGE_THUNK_DATA;

/* Import module directory */

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
	union {
		DWORD	Characteristics; /* 0 for terminating null import descriptor  */
		PIMAGE_THUNK_DATA OriginalFirstThunk;	/* RVA to original unbound IAT */
	} u;
	DWORD	TimeDateStamp;	/* 0 if not bound,
				 * -1 if bound, and real date\time stamp
				 *    in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT
				 * (new BIND)
				 * otherwise date/time stamp of DLL bound to
				 * (Old BIND)
				 */
	DWORD	ForwarderChain;	/* -1 if no forwarders */
	DWORD	Name;
	/* RVA to IAT (if bound this IAT has actual addresses) */
	PIMAGE_THUNK_DATA FirstThunk;	
} IMAGE_IMPORT_DESCRIPTOR,*PIMAGE_IMPORT_DESCRIPTOR;

#define	IMAGE_ORDINAL_FLAG		0x80000000
#define	IMAGE_SNAP_BY_ORDINAL(Ordinal)	((Ordinal & IMAGE_ORDINAL_FLAG) != 0)
#define	IMAGE_ORDINAL(Ordinal)		(Ordinal & 0xffff)

typedef struct _IMAGE_BOUND_IMPORT_DESCRIPTOR
{
    DWORD   TimeDateStamp;
    WORD    OffsetModuleName;
    WORD    NumberOfModuleForwarderRefs;
/* Array of zero or more IMAGE_BOUND_FORWARDER_REF follows */
} IMAGE_BOUND_IMPORT_DESCRIPTOR,  *PIMAGE_BOUND_IMPORT_DESCRIPTOR;

typedef struct _IMAGE_BOUND_FORWARDER_REF
{
    DWORD   TimeDateStamp;
    WORD    OffsetModuleName;
    WORD    Reserved;
} IMAGE_BOUND_FORWARDER_REF, *PIMAGE_BOUND_FORWARDER_REF;

typedef struct _IMAGE_BASE_RELOCATION
{
	DWORD	VirtualAddress;
	DWORD	SizeOfBlock;
	WORD	TypeOffset[1];
} IMAGE_BASE_RELOCATION,*PIMAGE_BASE_RELOCATION;

typedef struct _IMAGE_RELOCATION
{
    union {
        DWORD   VirtualAddress;
        DWORD   RelocCount;
    } u;
    DWORD   SymbolTableIndex;
    WORD    Type;
} IMAGE_RELOCATION;
typedef IMAGE_RELOCATION *PIMAGE_RELOCATION;

#define IMAGE_SIZEOF_RELOCATION 10

/* generic relocation types */
#define IMAGE_REL_BASED_ABSOLUTE 		0
#define IMAGE_REL_BASED_HIGH			1
#define IMAGE_REL_BASED_LOW			2
#define IMAGE_REL_BASED_HIGHLOW			3
#define IMAGE_REL_BASED_HIGHADJ			4
#define IMAGE_REL_BASED_MIPS_JMPADDR		5
#define IMAGE_REL_BASED_SECTION			6
#define	IMAGE_REL_BASED_REL			7
#define IMAGE_REL_BASED_MIPS_JMPADDR16		9
#define IMAGE_REL_BASED_IA64_IMM64		9 /* yes, 9 too */
#define IMAGE_REL_BASED_DIR64			10
#define IMAGE_REL_BASED_HIGH3ADJ		11

/* I386 relocation types */
#define	IMAGE_REL_I386_ABSOLUTE			0
#define	IMAGE_REL_I386_DIR16			1
#define	IMAGE_REL_I386_REL16			2
#define	IMAGE_REL_I386_DIR32			6
#define	IMAGE_REL_I386_DIR32NB			7
#define	IMAGE_REL_I386_SEG12			9
#define	IMAGE_REL_I386_SECTION			10
#define	IMAGE_REL_I386_SECREL			11
#define	IMAGE_REL_I386_REL32			20

/* MIPS relocation types */
#define IMAGE_REL_MIPS_ABSOLUTE		0x0000
#define IMAGE_REL_MIPS_REFHALF		0x0001
#define IMAGE_REL_MIPS_REFWORD		0x0002
#define IMAGE_REL_MIPS_JMPADDR		0x0003
#define IMAGE_REL_MIPS_REFHI		0x0004
#define IMAGE_REL_MIPS_REFLO		0x0005
#define IMAGE_REL_MIPS_GPREL		0x0006
#define IMAGE_REL_MIPS_LITERAL		0x0007
#define IMAGE_REL_MIPS_SECTION		0x000A
#define IMAGE_REL_MIPS_SECREL		0x000B
#define IMAGE_REL_MIPS_SECRELLO		0x000C
#define IMAGE_REL_MIPS_SECRELHI		0x000D
#define	IMAGE_REL_MIPS_JMPADDR16	0x0010
#define IMAGE_REL_MIPS_REFWORDNB	0x0022
#define IMAGE_REL_MIPS_PAIR		0x0025

/* ALPHA relocation types */
#define IMAGE_REL_ALPHA_ABSOLUTE	0x0000
#define IMAGE_REL_ALPHA_REFLONG		0x0001
#define IMAGE_REL_ALPHA_REFQUAD		0x0002
#define IMAGE_REL_ALPHA_GPREL		0x0003
#define IMAGE_REL_ALPHA_LITERAL		0x0004
#define IMAGE_REL_ALPHA_LITUSE		0x0005
#define IMAGE_REL_ALPHA_GPDISP		0x0006
#define IMAGE_REL_ALPHA_BRADDR		0x0007
#define IMAGE_REL_ALPHA_HINT		0x0008
#define IMAGE_REL_ALPHA_INLINE_REFLONG	0x0009
#define IMAGE_REL_ALPHA_REFHI		0x000A
#define IMAGE_REL_ALPHA_REFLO		0x000B
#define IMAGE_REL_ALPHA_PAIR		0x000C
#define IMAGE_REL_ALPHA_MATCH		0x000D
#define IMAGE_REL_ALPHA_SECTION		0x000E
#define IMAGE_REL_ALPHA_SECREL		0x000F
#define IMAGE_REL_ALPHA_REFLONGNB	0x0010
#define IMAGE_REL_ALPHA_SECRELLO	0x0011
#define IMAGE_REL_ALPHA_SECRELHI	0x0012
#define IMAGE_REL_ALPHA_REFQ3		0x0013
#define IMAGE_REL_ALPHA_REFQ2		0x0014
#define IMAGE_REL_ALPHA_REFQ1		0x0015
#define IMAGE_REL_ALPHA_GPRELLO		0x0016
#define IMAGE_REL_ALPHA_GPRELHI		0x0017

/* PowerPC relocation types */
#define IMAGE_REL_PPC_ABSOLUTE          0x0000
#define IMAGE_REL_PPC_ADDR64            0x0001
#define IMAGE_REL_PPC_ADDR            0x0002
#define IMAGE_REL_PPC_ADDR24            0x0003
#define IMAGE_REL_PPC_ADDR16            0x0004
#define IMAGE_REL_PPC_ADDR14            0x0005
#define IMAGE_REL_PPC_REL24             0x0006
#define IMAGE_REL_PPC_REL14             0x0007
#define IMAGE_REL_PPC_TOCREL16          0x0008
#define IMAGE_REL_PPC_TOCREL14          0x0009
#define IMAGE_REL_PPC_ADDR32NB          0x000A
#define IMAGE_REL_PPC_SECREL            0x000B
#define IMAGE_REL_PPC_SECTION           0x000C
#define IMAGE_REL_PPC_IFGLUE            0x000D
#define IMAGE_REL_PPC_IMGLUE            0x000E
#define IMAGE_REL_PPC_SECREL16          0x000F
#define IMAGE_REL_PPC_REFHI             0x0010
#define IMAGE_REL_PPC_REFLO             0x0011
#define IMAGE_REL_PPC_PAIR              0x0012
#define IMAGE_REL_PPC_SECRELLO          0x0013
#define IMAGE_REL_PPC_SECRELHI          0x0014
#define IMAGE_REL_PPC_GPREL		0x0015
#define IMAGE_REL_PPC_TYPEMASK          0x00FF
/* modifier bits */
#define IMAGE_REL_PPC_NEG               0x0100
#define IMAGE_REL_PPC_BRTAKEN           0x0200
#define IMAGE_REL_PPC_BRNTAKEN          0x0400
#define IMAGE_REL_PPC_TOCDEFN           0x0800

/* SH3 ? relocation type */
#define IMAGE_REL_SH3_ABSOLUTE          0x0000
#define IMAGE_REL_SH3_DIRECT16          0x0001
#define IMAGE_REL_SH3_DIRECT          0x0002
#define IMAGE_REL_SH3_DIRECT8           0x0003
#define IMAGE_REL_SH3_DIRECT8_WORD      0x0004
#define IMAGE_REL_SH3_DIRECT8_LONG      0x0005
#define IMAGE_REL_SH3_DIRECT4           0x0006
#define IMAGE_REL_SH3_DIRECT4_WORD      0x0007
#define IMAGE_REL_SH3_DIRECT4_LONG      0x0008
#define IMAGE_REL_SH3_PCREL8_WORD       0x0009
#define IMAGE_REL_SH3_PCREL8_LONG       0x000A
#define IMAGE_REL_SH3_PCREL12_WORD      0x000B
#define IMAGE_REL_SH3_STARTOF_SECTION   0x000C
#define IMAGE_REL_SH3_SIZEOF_SECTION    0x000D
#define IMAGE_REL_SH3_SECTION           0x000E
#define IMAGE_REL_SH3_SECREL            0x000F
#define IMAGE_REL_SH3_DIRECT32_NB       0x0010

/* ARM (Archimedes?) relocation types */
#define IMAGE_REL_ARM_ABSOLUTE		0x0000
#define IMAGE_REL_ARM_ADDR		0x0001
#define IMAGE_REL_ARM_ADDR32NB		0x0002
#define IMAGE_REL_ARM_BRANCH24		0x0003
#define IMAGE_REL_ARM_BRANCH11		0x0004
#define IMAGE_REL_ARM_SECTION		0x000E
#define IMAGE_REL_ARM_SECREL		0x000F

/* IA64 relocation types */
#define IMAGE_REL_IA64_ABSOLUTE		0x0000
#define IMAGE_REL_IA64_IMM14		0x0001
#define IMAGE_REL_IA64_IMM22		0x0002
#define IMAGE_REL_IA64_IMM64		0x0003
#define IMAGE_REL_IA64_DIR		0x0004
#define IMAGE_REL_IA64_DIR64		0x0005
#define IMAGE_REL_IA64_PCREL21B		0x0006
#define IMAGE_REL_IA64_PCREL21M		0x0007
#define IMAGE_REL_IA64_PCREL21F		0x0008
#define IMAGE_REL_IA64_GPREL22		0x0009
#define IMAGE_REL_IA64_LTOFF22		0x000A
#define IMAGE_REL_IA64_SECTION		0x000B
#define IMAGE_REL_IA64_SECREL22		0x000C
#define IMAGE_REL_IA64_SECREL64I	0x000D
#define IMAGE_REL_IA64_SECREL		0x000E
#define IMAGE_REL_IA64_LTOFF64		0x000F
#define IMAGE_REL_IA64_DIR32NB		0x0010
#define IMAGE_REL_IA64_RESERVED_11	0x0011
#define IMAGE_REL_IA64_RESERVED_12	0x0012
#define IMAGE_REL_IA64_RESERVED_13	0x0013
#define IMAGE_REL_IA64_RESERVED_14	0x0014
#define IMAGE_REL_IA64_RESERVED_15	0x0015
#define IMAGE_REL_IA64_RESERVED_16	0x0016
#define IMAGE_REL_IA64_ADDEND		0x001F

/* archive format */

#define IMAGE_ARCHIVE_START_SIZE             8
#define IMAGE_ARCHIVE_START                  "!<arch>\n"
#define IMAGE_ARCHIVE_END                    "`\n"
#define IMAGE_ARCHIVE_PAD                    "\n"
#define IMAGE_ARCHIVE_LINKER_MEMBER          "/               "
#define IMAGE_ARCHIVE_LONGNAMES_MEMBER       "//              "

typedef struct _IMAGE_ARCHIVE_MEMBER_HEADER
{
    BYTE     Name[16];
    BYTE     Date[12];
    BYTE     UserID[6];
    BYTE     GroupID[6];
    BYTE     Mode[8];
    BYTE     Size[10];
    BYTE     EndHeader[2];
} IMAGE_ARCHIVE_MEMBER_HEADER, *PIMAGE_ARCHIVE_MEMBER_HEADER;

#define IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR 60

/*
 * Resource directory stuff
 */
typedef struct _IMAGE_RESOURCE_DIRECTORY {
	DWORD	Characteristics;
	DWORD	TimeDateStamp;
	WORD	MajorVersion;
	WORD	MinorVersion;
	WORD	NumberOfNamedEntries;
	WORD	NumberOfIdEntries;
	/*  IMAGE_RESOURCE_DIRECTORY_ENTRY DirectoryEntries[]; */
} IMAGE_RESOURCE_DIRECTORY,*PIMAGE_RESOURCE_DIRECTORY;

#define	IMAGE_RESOURCE_NAME_IS_STRING		0x80000000
#define	IMAGE_RESOURCE_DATA_IS_DIRECTORY	0x80000000

typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY {
	union {
		struct {
#ifdef BITFIELDS_BIGENDIAN
			unsigned NameIsString:1;
			unsigned NameOffset:31;
#else
			unsigned NameOffset:31;
			unsigned NameIsString:1;
#endif
		} DUMMYSTRUCTNAME1;
		DWORD   Name;
                struct {
#ifdef WORDS_BIGENDIAN
			WORD    __pad;
			WORD    Id;
#else
			WORD    Id;
			WORD    __pad;
#endif
		} DUMMYSTRUCTNAME2;
	} DUMMYUNIONNAME1;
	union {
		DWORD   OffsetToData;
		struct {
#ifdef BITFIELDS_BIGENDIAN
			unsigned DataIsDirectory:1;
			unsigned OffsetToDirectory:31;
#else
			unsigned OffsetToDirectory:31;
			unsigned DataIsDirectory:1;
#endif
		} DUMMYSTRUCTNAME3;
	} DUMMYUNIONNAME2;
} IMAGE_RESOURCE_DIRECTORY_ENTRY,*PIMAGE_RESOURCE_DIRECTORY_ENTRY;


typedef struct _IMAGE_RESOURCE_DIRECTORY_STRING {
	WORD	Length;
	CHAR	NameString[ 1 ];
} IMAGE_RESOURCE_DIRECTORY_STRING,*PIMAGE_RESOURCE_DIRECTORY_STRING;

typedef struct _IMAGE_RESOURCE_DIR_STRING_U {
	WORD	Length;
	WCHAR	NameString[ 1 ];
} IMAGE_RESOURCE_DIR_STRING_U,*PIMAGE_RESOURCE_DIR_STRING_U;

typedef struct _IMAGE_RESOURCE_DATA_ENTRY {
	DWORD	OffsetToData;
	DWORD	Size;
	DWORD	CodePage;
	DWORD	ResourceHandle;
} IMAGE_RESOURCE_DATA_ENTRY,*PIMAGE_RESOURCE_DATA_ENTRY;


typedef VOID CALLBACK (*PIMAGE_TLS_CALLBACK)(
	LPVOID DllHandle,DWORD Reason,LPVOID Reserved
);

typedef struct _IMAGE_TLS_DIRECTORY {
	DWORD	StartAddressOfRawData;
	DWORD	EndAddressOfRawData;
	LPDWORD	AddressOfIndex;
	PIMAGE_TLS_CALLBACK *AddressOfCallBacks;
	DWORD	SizeOfZeroFill;
	DWORD	Characteristics;
} IMAGE_TLS_DIRECTORY,*PIMAGE_TLS_DIRECTORY;

typedef struct _IMAGE_DEBUG_DIRECTORY {
  DWORD Characteristics;
  DWORD TimeDateStamp;
  WORD  MajorVersion;
  WORD  MinorVersion;
  DWORD Type;
  DWORD SizeOfData;
  DWORD AddressOfRawData;
  DWORD PointerToRawData;
} IMAGE_DEBUG_DIRECTORY, *PIMAGE_DEBUG_DIRECTORY;

#define IMAGE_DEBUG_TYPE_UNKNOWN        0
#define IMAGE_DEBUG_TYPE_COFF           1
#define IMAGE_DEBUG_TYPE_CODEVIEW       2
#define IMAGE_DEBUG_TYPE_FPO            3
#define IMAGE_DEBUG_TYPE_MISC           4
#define IMAGE_DEBUG_TYPE_EXCEPTION      5
#define IMAGE_DEBUG_TYPE_FIXUP          6
#define IMAGE_DEBUG_TYPE_OMAP_TO_SRC    7
#define IMAGE_DEBUG_TYPE_OMAP_FROM_SRC  8
#define IMAGE_DEBUG_TYPE_BORLAND        9
#define IMAGE_DEBUG_TYPE_RESERVED10    10

typedef struct _IMAGE_COFF_SYMBOLS_HEADER {
  DWORD NumberOfSymbols;
  DWORD LvaToFirstSymbol;
  DWORD NumberOfLinenumbers;
  DWORD LvaToFirstLinenumber;
  DWORD RvaToFirstByteOfCode;
  DWORD RvaToLastByteOfCode;
  DWORD RvaToFirstByteOfData;
  DWORD RvaToLastByteOfData;
} IMAGE_COFF_SYMBOLS_HEADER, *PIMAGE_COFF_SYMBOLS_HEADER;

#define FRAME_FPO       0
#define FRAME_TRAP      1
#define FRAME_TSS       2
#define FRAME_NONFPO    3

typedef struct _FPO_DATA {
  DWORD ulOffStart;
  DWORD cbProcSize;
  DWORD cdwLocals;
  WORD  cdwParams;
  unsigned cbProlog : 8;
  unsigned cbRegs   : 3;
  unsigned fHasSEH  : 1;
  unsigned fUseBP   : 1;
  unsigned reserved : 1;
  unsigned cbFrame  : 2;
} FPO_DATA, *PFPO_DATA;

typedef struct _IMAGE_LOAD_CONFIG_DIRECTORY {
  DWORD Characteristics;
  DWORD TimeDateStamp;
  WORD  MajorVersion;
  WORD  MinorVersion;
  DWORD GlobalFlagsClear;
  DWORD GlobalFlagsSet;
  DWORD CriticalSectionDefaultTimeout;
  DWORD DeCommitFreeBlockThreshold;
  DWORD DeCommitTotalFreeThreshold;
  PVOID LockPrefixTable;
  DWORD MaximumAllocationSize;
  DWORD VirtualMemoryThreshold;
  DWORD ProcessHeapFlags;
  DWORD ProcessAffinityMask;
  WORD  CSDVersion;
  WORD  Reserved1;
  PVOID EditList;
  DWORD Reserved[1];
} IMAGE_LOAD_CONFIG_DIRECTORY, *PIMAGE_LOAD_CONFIG_DIRECTORY;

typedef struct _IMAGE_FUNCTION_ENTRY {
  DWORD StartingAddress;
  DWORD EndingAddress;
  DWORD EndOfPrologue;
} IMAGE_FUNCTION_ENTRY, *PIMAGE_FUNCTION_ENTRY;

#define IMAGE_DEBUG_MISC_EXENAME    1
 
typedef struct _IMAGE_DEBUG_MISC {
    DWORD       DataType;
    DWORD       Length;
    BYTE        Unicode;
    BYTE        Reserved[ 3 ];
    BYTE        Data[ 1 ];
} IMAGE_DEBUG_MISC, *PIMAGE_DEBUG_MISC;

/* This is the structure that appears at the very start of a .DBG file. */

typedef struct _IMAGE_SEPARATE_DEBUG_HEADER {
	WORD	Signature;
	WORD	Flags;
	WORD	Machine;
	WORD	Characteristics;
	DWORD	TimeDateStamp;
	DWORD	CheckSum;
	DWORD	ImageBase;
	DWORD	SizeOfImage;
	DWORD	NumberOfSections;
	DWORD	ExportedNamesSize;
	DWORD	DebugDirectorySize;
	DWORD	SectionAlignment;
	DWORD	Reserved[ 2 ];
} IMAGE_SEPARATE_DEBUG_HEADER,*PIMAGE_SEPARATE_DEBUG_HEADER;

#define IMAGE_SEPARATE_DEBUG_SIGNATURE 0x4944


typedef struct tagMESSAGE_RESOURCE_ENTRY {
	WORD	Length;
	WORD	Flags;
	BYTE	Text[1];
} MESSAGE_RESOURCE_ENTRY,*PMESSAGE_RESOURCE_ENTRY;
#define	MESSAGE_RESOURCE_UNICODE	0x0001

typedef struct tagMESSAGE_RESOURCE_BLOCK {
	DWORD	LowId;
	DWORD	HighId;
	DWORD	OffsetToEntries;
} MESSAGE_RESOURCE_BLOCK,*PMESSAGE_RESOURCE_BLOCK;

typedef struct tagMESSAGE_RESOURCE_DATA {
	DWORD			NumberOfBlocks;
	MESSAGE_RESOURCE_BLOCK	Blocks[ 1 ];
} MESSAGE_RESOURCE_DATA,*PMESSAGE_RESOURCE_DATA;

/*
 * Here follows typedefs for security and tokens.
 */ 

/*
 * First a constant for the following typdefs.
 */

#define ANYSIZE_ARRAY   1

/* FIXME:  Orphan.  What does it point to? */
typedef PVOID PACCESS_TOKEN;

/*
 * TOKEN_INFORMATION_CLASS
 */

typedef enum _TOKEN_INFORMATION_CLASS {
  TokenUser = 1, 
  TokenGroups, 
  TokenPrivileges, 
  TokenOwner, 
  TokenPrimaryGroup, 
  TokenDefaultDacl, 
  TokenSource, 
  TokenType, 
  TokenImpersonationLevel, 
  TokenStatistics 
} TOKEN_INFORMATION_CLASS; 

#define TOKEN_TOKEN_ADJUST_DEFAULT   0x0080
#define TOKEN_ADJUST_GROUPS          0x0040
#define TOKEN_ADJUST_PRIVILEGES      0x0020
#define TOKEN_ADJUST_SESSIONID       0x0100
#define TOKEN_ASSIGN_PRIMARY         0x0001
#define TOKEN_DUPLICATE              0x0002
#define TOKEN_EXECUTE                STANDARD_RIGHTS_EXECUTE
#define TOKEN_IMPERSONATE            0x0004
#define TOKEN_QUERY                  0x0008
#define TOKEN_QUERY_SOURCE           0x0010
#define TOKEN_ADJUST_DEFAULT         0x0080
#define TOKEN_READ                   (STANDARD_RIGHTS_READ|TOKEN_QUERY)
#define TOKEN_WRITE                  (STANDARD_RIGHTS_WRITE     | \
					TOKEN_ADJUST_PRIVILEGES | \
					TOKEN_ADJUST_GROUPS | \
					TOKEN_ADJUST_DEFAULT )
#define TOKEN_ALL_ACCESS             (STANDARD_RIGHTS_REQUIRED | \
					TOKEN_ASSIGN_PRIMARY | \
					TOKEN_DUPLICATE | \
					TOKEN_IMPERSONATE | \
					TOKEN_QUERY | \
					TOKEN_QUERY_SOURCE | \
					TOKEN_ADJUST_PRIVILEGES | \
					TOKEN_ADJUST_GROUPS | \
					TOKEN_ADJUST_SESSIONID | \
					TOKEN_ADJUST_DEFAULT )

#ifndef _SECURITY_DEFINED
#define _SECURITY_DEFINED

#include "pshpack1.h"

typedef DWORD ACCESS_MASK, *PACCESS_MASK;

typedef struct _GENERIC_MAPPING {
    ACCESS_MASK GenericRead;
    ACCESS_MASK GenericWrite;
    ACCESS_MASK GenericExecute;
    ACCESS_MASK GenericAll;
} GENERIC_MAPPING, *PGENERIC_MAPPING;

#ifndef SID_IDENTIFIER_AUTHORITY_DEFINED
#define SID_IDENTIFIER_AUTHORITY_DEFINED
typedef struct {
    BYTE Value[6];
} SID_IDENTIFIER_AUTHORITY,*PSID_IDENTIFIER_AUTHORITY,*LPSID_IDENTIFIER_AUTHORITY;
#endif /* !defined(SID_IDENTIFIER_AUTHORITY_DEFINED) */

#ifndef SID_DEFINED
#define SID_DEFINED
typedef struct _SID {
    BYTE Revision;
    BYTE SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    DWORD SubAuthority[1];
} SID,*PSID;
#endif /* !defined(SID_DEFINED) */

#define	SID_REVISION			(1)	/* Current revision */
#define	SID_MAX_SUB_AUTHORITIES		(15)	/* current max subauths */
#define	SID_RECOMMENDED_SUB_AUTHORITIES	(1)	/* recommended subauths */


/* 
 * ACL 
 */

#define ACL_REVISION1 1
#define ACL_REVISION2 2
#define ACL_REVISION3 3
#define ACL_REVISION4 4

#define MIN_ACL_REVISION ACL_REVISION2
#define MAX_ACL_REVISION ACL_REVISION4

typedef struct _ACL {
    BYTE AclRevision;
    BYTE Sbz1;
    WORD AclSize;
    WORD AceCount;
    WORD Sbz2;
} ACL, *PACL;

/* SECURITY_DESCRIPTOR */
#define	SECURITY_DESCRIPTOR_REVISION	1
#define	SECURITY_DESCRIPTOR_REVISION1	1


#define	SE_OWNER_DEFAULTED	0x0001
#define	SE_GROUP_DEFAULTED	0x0002
#define	SE_DACL_PRESENT		0x0004
#define	SE_DACL_DEFAULTED	0x0008
#define	SE_SACL_PRESENT		0x0010
#define	SE_SACL_DEFAULTED	0x0020
#define	SE_SELF_RELATIVE	0x8000

typedef DWORD SECURITY_INFORMATION, *PSECURITY_INFORMATION;
typedef WORD SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;

/* The security descriptor structure */
typedef struct {
    BYTE Revision;
    BYTE Sbz1;
    SECURITY_DESCRIPTOR_CONTROL Control;
    DWORD Owner;
    DWORD Group;
    DWORD Sacl;
    DWORD Dacl;
} SECURITY_DESCRIPTOR_RELATIVE, *PISECURITY_DESCRIPTOR_RELATIVE;

typedef struct {
    BYTE Revision;
    BYTE Sbz1;
    SECURITY_DESCRIPTOR_CONTROL Control;
    PSID Owner;
    PSID Group;
    PACL Sacl;
    PACL Dacl;
} SECURITY_DESCRIPTOR, *PSECURITY_DESCRIPTOR;

#define SECURITY_DESCRIPTOR_MIN_LENGTH   (sizeof(SECURITY_DESCRIPTOR)) 

#include "poppack.h"

#endif /* _SECURITY_DEFINED */

#include "pshpack1.h"

/* 
 * SID_AND_ATTRIBUTES
 */

typedef struct _SID_AND_ATTRIBUTES {
  PSID  Sid; 
  DWORD Attributes; 
} SID_AND_ATTRIBUTES ; 
 
/* security entities */
#define SECURITY_NULL_RID			(0x00000000L)
#define SECURITY_WORLD_RID			(0x00000000L)
#define SECURITY_LOCAL_RID			(0X00000000L)

#define SECURITY_NULL_SID_AUTHORITY		{0,0,0,0,0,0}

/* S-1-1 */
#define SECURITY_WORLD_SID_AUTHORITY		{0,0,0,0,0,1}

/* S-1-2 */
#define SECURITY_LOCAL_SID_AUTHORITY		{0,0,0,0,0,2}

/* S-1-3 */
#define SECURITY_CREATOR_SID_AUTHORITY		{0,0,0,0,0,3}
#define SECURITY_CREATOR_OWNER_RID		(0x00000000L) 
#define SECURITY_CREATOR_GROUP_RID		(0x00000001L)
#define SECURITY_CREATOR_OWNER_SERVER_RID	(0x00000002L)
#define SECURITY_CREATOR_GROUP_SERVER_RID	(0x00000003L)

/* S-1-4 */
#define SECURITY_NON_UNIQUE_AUTHORITY		{0,0,0,0,0,4}

/* S-1-5 */
#define SECURITY_NT_AUTHORITY			{0,0,0,0,0,5} 
#define SECURITY_DIALUP_RID                     0x00000001L
#define SECURITY_NETWORK_RID                    0x00000002L
#define SECURITY_BATCH_RID                      0x00000003L
#define SECURITY_INTERACTIVE_RID                0x00000004L
#define SECURITY_LOGON_IDS_RID                  0x00000005L
#define SECURITY_SERVICE_RID                    0x00000006L
#define SECURITY_ANONYMOUS_LOGON_RID            0x00000007L
#define SECURITY_PROXY_RID                      0x00000008L
#define SECURITY_ENTERPRISE_CONTROLLERS_RID     0x00000009L
#define SECURITY_PRINCIPAL_SELF_RID             0x0000000AL
#define SECURITY_AUTHENTICATED_USER_RID         0x0000000BL
#define SECURITY_RESTRICTED_CODE_RID            0x0000000CL
#define SECURITY_TERMINAL_SERVER_RID            0x0000000DL
#define SECURITY_LOCAL_SYSTEM_RID               0x00000012L
#define SECURITY_NT_NON_UNIQUE                  0x00000015L
#define SECURITY_BUILTIN_DOMAIN_RID             0x00000020L

#define DOMAIN_GROUP_RID_ADMINS                 0x00000200L
#define DOMAIN_GROUP_RID_USERS                  0x00000201L
#define DOMAIN_GROUP_RID_GUESTS                 0x00000202L

#define DOMAIN_ALIAS_RID_ADMINS                 0x00000220L
#define DOMAIN_ALIAS_RID_USERS                  0x00000221L
#define DOMAIN_ALIAS_RID_GUESTS                 0x00000222L

#define SECURITY_SERVER_LOGON_RID		SECURITY_ENTERPRISE_CONTROLLERS_RID

#define SECURITY_LOGON_IDS_RID_COUNT		(3L)

/*
 * TOKEN_USER
 */

typedef struct _TOKEN_USER {
  SID_AND_ATTRIBUTES User; 
} TOKEN_USER; 

/*
 * TOKEN_GROUPS
 */

typedef struct _TOKEN_GROUPS  {
  DWORD GroupCount; 
  SID_AND_ATTRIBUTES Groups[ANYSIZE_ARRAY]; 
} TOKEN_GROUPS; 

/*
 * LUID_AND_ATTRIBUTES
 */

typedef union _LARGE_INTEGER {
    struct {
        DWORD    LowPart;
        LONG     HighPart;
    } DUMMYSTRUCTNAME;
    LONGLONG QuadPart;
} LARGE_INTEGER, *LPLARGE_INTEGER, *PLARGE_INTEGER;

typedef union _ULARGE_INTEGER {
    struct {
        DWORD    LowPart;
        DWORD    HighPart;
    } DUMMYSTRUCTNAME;
    ULONGLONG QuadPart;
} ULARGE_INTEGER, *LPULARGE_INTEGER, *PULARGE_INTEGER;

/*
 * Locally Unique Identifier
 */

typedef LARGE_INTEGER LUID,*PLUID;

typedef struct _LUID_AND_ATTRIBUTES {
  LUID   Luid; 
  DWORD  Attributes; 
} LUID_AND_ATTRIBUTES; 

/*
 * PRIVILEGE_SET
 */

typedef struct _PRIVILEGE_SET {
    DWORD PrivilegeCount;
    DWORD Control;
    LUID_AND_ATTRIBUTES Privilege[ANYSIZE_ARRAY];
} PRIVILEGE_SET, *PPRIVILEGE_SET;

/*
 * TOKEN_PRIVILEGES
 */

typedef struct _TOKEN_PRIVILEGES {
  DWORD PrivilegeCount; 
  LUID_AND_ATTRIBUTES Privileges[ANYSIZE_ARRAY]; 
} TOKEN_PRIVILEGES, *PTOKEN_PRIVILEGES; 

/*
 * TOKEN_OWNER
 */

typedef struct _TOKEN_OWNER {
  PSID Owner; 
} TOKEN_OWNER; 

/*
 * TOKEN_PRIMARY_GROUP
 */

typedef struct _TOKEN_PRIMARY_GROUP {
  PSID PrimaryGroup; 
} TOKEN_PRIMARY_GROUP; 


/*
 * TOKEN_DEFAULT_DACL
 */

typedef struct _TOKEN_DEFAULT_DACL { 
  PACL DefaultDacl; 
} TOKEN_DEFAULT_DACL; 

/*
 * TOKEN_SOURCEL
 */

typedef struct _TOKEN_SOURCE {
  char Sourcename[8]; 
  LUID SourceIdentifier; 
} TOKEN_SOURCE; 

/*
 * TOKEN_TYPE
 */

typedef enum tagTOKEN_TYPE {
  TokenPrimary = 1, 
  TokenImpersonation 
} TOKEN_TYPE; 

/*
 * SECURITY_IMPERSONATION_LEVEL
 */

typedef enum _SECURITY_IMPERSONATION_LEVEL {
  SecurityAnonymous, 
  SecurityIdentification, 
  SecurityImpersonation, 
  SecurityDelegation 
} SECURITY_IMPERSONATION_LEVEL, *PSECURITY_IMPERSONATION_LEVEL; 


typedef BOOLEAN SECURITY_CONTEXT_TRACKING_MODE,
	* PSECURITY_CONTEXT_TRACKING_MODE;
/*
 *	Quality of Service
 */

typedef struct _SECURITY_QUALITY_OF_SERVICE {
  DWORD				Length;
  SECURITY_IMPERSONATION_LEVEL	ImpersonationLevel;
  SECURITY_CONTEXT_TRACKING_MODE ContextTrackingMode;
  BOOL				EffectiveOnly;
} SECURITY_QUALITY_OF_SERVICE, *PSECURITY_QUALITY_OF_SERVICE;

/*
 * TOKEN_STATISTICS
 */

typedef struct _TOKEN_STATISTICS {
  LUID  TokenId; 
  LUID  AuthenticationId; 
  LARGE_INTEGER ExpirationTime; 
  TOKEN_TYPE    TokenType; 
  SECURITY_IMPERSONATION_LEVEL ImpersonationLevel; 
  DWORD DynamicCharged; 
  DWORD DynamicAvailable; 
  DWORD GroupCount; 
  DWORD PrivilegeCount; 
  LUID  ModifiedId; 
} TOKEN_STATISTICS; 

/* 
 *	ACLs of NT 
 */

#define	ACL_REVISION	2

#define	ACL_REVISION1	1
#define	ACL_REVISION2	2

/* ACEs, directly starting after an ACL */
typedef struct _ACE_HEADER {
	BYTE	AceType;
	BYTE	AceFlags;
	WORD	AceSize;
} ACE_HEADER,*PACE_HEADER;

/* AceType */
#define	ACCESS_ALLOWED_ACE_TYPE		0
#define	ACCESS_DENIED_ACE_TYPE		1
#define	SYSTEM_AUDIT_ACE_TYPE		2
#define	SYSTEM_ALARM_ACE_TYPE		3

/* inherit AceFlags */
#define	OBJECT_INHERIT_ACE		0x01
#define	CONTAINER_INHERIT_ACE		0x02
#define	NO_PROPAGATE_INHERIT_ACE	0x04
#define	INHERIT_ONLY_ACE		0x08
#define	VALID_INHERIT_FLAGS		0x0F

/* AceFlags mask for what events we (should) audit */
#define	SUCCESSFUL_ACCESS_ACE_FLAG	0x40
#define	FAILED_ACCESS_ACE_FLAG		0x80

/* different ACEs depending on AceType 
 * SidStart marks the begin of a SID
 * so the thing finally looks like this:
 * 0: ACE_HEADER
 * 4: ACCESS_MASK
 * 8... : SID
 */
typedef struct _ACCESS_ALLOWED_ACE {
	ACE_HEADER	Header;
	DWORD		Mask;
	DWORD		SidStart;
} ACCESS_ALLOWED_ACE,*PACCESS_ALLOWED_ACE;

typedef struct _ACCESS_DENIED_ACE {
	ACE_HEADER	Header;
	DWORD		Mask;
	DWORD		SidStart;
} ACCESS_DENIED_ACE,*PACCESS_DENIED_ACE;

typedef struct _SYSTEM_AUDIT_ACE {
	ACE_HEADER	Header;
	DWORD		Mask;
	DWORD		SidStart;
} SYSTEM_AUDIT_ACE,*PSYSTEM_AUDIT_ACE;

typedef struct _SYSTEM_ALARM_ACE {
	ACE_HEADER	Header;
	DWORD		Mask;
	DWORD		SidStart;
} SYSTEM_ALARM_ACE,*PSYSTEM_ALARM_ACE;

typedef enum tagSID_NAME_USE {
	SidTypeUser = 1,
	SidTypeGroup,
	SidTypeDomain,
	SidTypeAlias,
	SidTypeWellKnownGroup,
	SidTypeDeletedAccount,
	SidTypeInvalid,
	SidTypeUnknown
} SID_NAME_USE,*PSID_NAME_USE;

/* Access rights */

/* DELETE may be already defined via /usr/include/arpa/nameser_compat.h */
#undef  DELETE
#define DELETE                     0x00010000
#define READ_CONTROL               0x00020000
#define WRITE_DAC                  0x00040000
#define WRITE_OWNER                0x00080000
#define SYNCHRONIZE                0x00100000
#define STANDARD_RIGHTS_REQUIRED   0x000f0000

#define STANDARD_RIGHTS_READ       READ_CONTROL
#define STANDARD_RIGHTS_WRITE      READ_CONTROL
#define STANDARD_RIGHTS_EXECUTE    READ_CONTROL

#define STANDARD_RIGHTS_ALL        0x001f0000

#define SPECIFIC_RIGHTS_ALL        0x0000ffff

#define GENERIC_READ               0x80000000
#define GENERIC_WRITE              0x40000000
#define GENERIC_EXECUTE            0x20000000
#define GENERIC_ALL                0x10000000

#define MAXIMUM_ALLOWED            0x02000000
#define ACCESS_SYSTEM_SECURITY     0x01000000

#define EVENT_MODIFY_STATE         0x0002
#define EVENT_ALL_ACCESS           (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x3)

#define SEMAPHORE_MODIFY_STATE     0x0002
#define SEMAPHORE_ALL_ACCESS       (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x3)

#define MUTEX_MODIFY_STATE         0x0001
#define MUTEX_ALL_ACCESS           (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x1)

#define TIMER_QUERY_STATE          0x0001
#define TIMER_MODIFY_STATE         0x0002
#define TIMER_ALL_ACCESS           (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x3)

#define PROCESS_TERMINATE          0x0001
#define PROCESS_CREATE_THREAD      0x0002
#define PROCESS_VM_OPERATION       0x0008
#define PROCESS_VM_READ            0x0010
#define PROCESS_VM_WRITE           0x0020
#define PROCESS_DUP_HANDLE         0x0040
#define PROCESS_CREATE_PROCESS     0x0080
#define PROCESS_SET_QUOTA          0x0100
#define PROCESS_SET_INFORMATION    0x0200
#define PROCESS_QUERY_INFORMATION  0x0400
#define PROCESS_ALL_ACCESS         (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0xfff)

#define THREAD_TERMINATE           0x0001
#define THREAD_SUSPEND_RESUME      0x0002
#define THREAD_GET_CONTEXT         0x0008
#define THREAD_SET_CONTEXT         0x0010
#define THREAD_SET_INFORMATION     0x0020
#define THREAD_QUERY_INFORMATION   0x0040
#define THREAD_SET_THREAD_TOKEN    0x0080
#define THREAD_IMPERSONATE         0x0100
#define THREAD_DIRECT_IMPERSONATION 0x0200
#define THREAD_ALL_ACCESS          (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x3ff)

#define THREAD_BASE_PRIORITY_LOWRT  15 
#define THREAD_BASE_PRIORITY_MAX    2 
#define THREAD_BASE_PRIORITY_MIN   -2
#define THREAD_BASE_PRIORITY_IDLE  -15

#define FILE_READ_DATA            0x0001    /* file & pipe */
#define FILE_LIST_DIRECTORY       0x0001    /* directory */
#define FILE_WRITE_DATA           0x0002    /* file & pipe */
#define FILE_ADD_FILE             0x0002    /* directory */
#define FILE_APPEND_DATA          0x0004    /* file */
#define FILE_ADD_SUBDIRECTORY     0x0004    /* directory */
#define FILE_CREATE_PIPE_INSTANCE 0x0004    /* named pipe */
#define FILE_READ_EA              0x0008    /* file & directory */
#define FILE_READ_PROPERTIES      FILE_READ_EA
#define FILE_WRITE_EA             0x0010    /* file & directory */
#define FILE_WRITE_PROPERTIES     FILE_WRITE_EA
#define FILE_EXECUTE              0x0020    /* file */
#define FILE_TRAVERSE             0x0020    /* directory */
#define FILE_DELETE_CHILD         0x0040    /* directory */
#define FILE_READ_ATTRIBUTES      0x0080    /* all */
#define FILE_WRITE_ATTRIBUTES     0x0100    /* all */
#define FILE_ALL_ACCESS           (STANDARD_RIGHTS_REQUIRED|SYNCHRONIZE|0x1ff)

#define FILE_GENERIC_READ         (STANDARD_RIGHTS_READ | FILE_READ_DATA | \
                                   FILE_READ_ATTRIBUTES | FILE_READ_EA | \
                                   SYNCHRONIZE)
#define FILE_GENERIC_WRITE        (STANDARD_RIGHTS_WRITE | FILE_WRITE_DATA | \
                                   FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | \
                                   FILE_APPEND_DATA | SYNCHRONIZE)
#define FILE_GENERIC_EXECUTE      (STANDARD_RIGHTS_EXECUTE | FILE_EXECUTE | \
                                   FILE_READ_ATTRIBUTES | SYNCHRONIZE)


/* File attribute flags */
#define FILE_SHARE_READ			0x00000001L
#define FILE_SHARE_WRITE		0x00000002L
#define FILE_SHARE_DELETE		0x00000004L
#define FILE_ATTRIBUTE_READONLY         0x00000001L
#define FILE_ATTRIBUTE_HIDDEN           0x00000002L
#define FILE_ATTRIBUTE_SYSTEM           0x00000004L
#define FILE_ATTRIBUTE_LABEL            0x00000008L  /* Not in Windows API */
#define FILE_ATTRIBUTE_DIRECTORY        0x00000010L
#define FILE_ATTRIBUTE_ARCHIVE          0x00000020L
#define FILE_ATTRIBUTE_NORMAL           0x00000080L
#define FILE_ATTRIBUTE_TEMPORARY        0x00000100L
#define FILE_ATTRIBUTE_ATOMIC_WRITE     0x00000200L
#define FILE_ATTRIBUTE_XACTION_WRITE    0x00000400L
#define FILE_ATTRIBUTE_COMPRESSED       0x00000800L
#define FILE_ATTRIBUTE_OFFLINE		0x00001000L
#define FILE_ATTRIBUTE_SYMLINK		0x80000000L  /* Not in Windows API */

/* File notification flags */
#define FILE_NOTIFY_CHANGE_FILE_NAME    0x00000001
#define FILE_NOTIFY_CHANGE_DIR_NAME     0x00000002
#define FILE_NOTIFY_CHANGE_ATTRIBUTES   0x00000004
#define FILE_NOTIFY_CHANGE_SIZE         0x00000008
#define FILE_NOTIFY_CHANGE_LAST_WRITE   0x00000010
#define FILE_NOTIFY_CHANGE_LAST_ACCESS  0x00000020
#define FILE_NOTIFY_CHANGE_CREATION     0x00000040
#define FILE_NOTIFY_CHANGE_SECURITY     0x00000100

#define FILE_ACTION_ADDED               0x00000001
#define FILE_ACTION_REMOVED             0x00000002
#define FILE_ACTION_MODIFIED            0x00000003
#define FILE_ACTION_RENAMED_OLD_NAME    0x00000004
#define FILE_ACTION_RENAMED_NEW_NAME    0x00000005


#define FILE_CASE_SENSITIVE_SEARCH      0x00000001
#define FILE_CASE_PRESERVED_NAMES       0x00000002
#define FILE_UNICODE_ON_DISK            0x00000004
#define FILE_PERSISTENT_ACLS            0x00000008
#define FILE_FILE_COMPRESSION           0x00000010
#define FILE_VOLUME_IS_COMPRESSED       0x00008000

/* File alignments (NT) */
#define	FILE_BYTE_ALIGNMENT		0x00000000
#define	FILE_WORD_ALIGNMENT		0x00000001
#define	FILE_LONG_ALIGNMENT		0x00000003
#define	FILE_QUAD_ALIGNMENT		0x00000007
#define	FILE_OCTA_ALIGNMENT		0x0000000f
#define	FILE_32_BYTE_ALIGNMENT		0x0000001f
#define	FILE_64_BYTE_ALIGNMENT		0x0000003f
#define	FILE_128_BYTE_ALIGNMENT		0x0000007f
#define	FILE_256_BYTE_ALIGNMENT		0x000000ff
#define	FILE_512_BYTE_ALIGNMENT		0x000001ff

#define REG_NONE		0	/* no type */
#define REG_SZ			1	/* string type (ASCII) */
#define REG_EXPAND_SZ		2	/* string, includes %ENVVAR% (expanded by caller) (ASCII) */
#define REG_BINARY		3	/* binary format, callerspecific */
/* YES, REG_DWORD == REG_DWORD_LITTLE_ENDIAN */
#define REG_DWORD		4	/* DWORD in little endian format */
#define REG_DWORD_LITTLE_ENDIAN	4	/* DWORD in little endian format */
#define REG_DWORD_BIG_ENDIAN	5	/* DWORD in big endian format  */
#define REG_LINK		6	/* symbolic link (UNICODE) */
#define REG_MULTI_SZ		7	/* multiple strings, delimited by \0, terminated by \0\0 (ASCII) */
#define REG_RESOURCE_LIST	8	/* resource list? huh? */
#define REG_FULL_RESOURCE_DESCRIPTOR	9	/* full resource descriptor? huh? */
#define REG_RESOURCE_REQUIREMENTS_LIST	10

/* ----------------------------- begin registry ----------------------------- */

/* Registry security values */
#define OWNER_SECURITY_INFORMATION	0x00000001
#define GROUP_SECURITY_INFORMATION	0x00000002
#define DACL_SECURITY_INFORMATION	0x00000004
#define SACL_SECURITY_INFORMATION	0x00000008

#define REG_OPTION_RESERVED		0x00000000
#define REG_OPTION_NON_VOLATILE		0x00000000
#define REG_OPTION_VOLATILE		0x00000001
#define REG_OPTION_CREATE_LINK		0x00000002
#define REG_OPTION_BACKUP_RESTORE	0x00000004 /* FIXME */
#define REG_OPTION_OPEN_LINK		0x00000008
#define REG_LEGAL_OPTION	       (REG_OPTION_RESERVED|  \
					REG_OPTION_NON_VOLATILE|  \
					REG_OPTION_VOLATILE|  \
					REG_OPTION_CREATE_LINK|  \
					REG_OPTION_BACKUP_RESTORE|  \
					REG_OPTION_OPEN_LINK)


#define REG_CREATED_NEW_KEY	0x00000001
#define REG_OPENED_EXISTING_KEY	0x00000002

/* For RegNotifyChangeKeyValue */
#define REG_NOTIFY_CHANGE_NAME	0x1

#define KEY_QUERY_VALUE		0x00000001
#define KEY_SET_VALUE		0x00000002
#define KEY_CREATE_SUB_KEY	0x00000004
#define KEY_ENUMERATE_SUB_KEYS	0x00000008
#define KEY_NOTIFY		0x00000010
#define KEY_CREATE_LINK		0x00000020

#define KEY_READ	      ((STANDARD_RIGHTS_READ|  \
				KEY_QUERY_VALUE|  \
				KEY_ENUMERATE_SUB_KEYS|  \
				KEY_NOTIFY)  \
				& (~SYNCHRONIZE)  \
			      )
#define KEY_WRITE	      ((STANDARD_RIGHTS_WRITE|  \
				KEY_SET_VALUE|  \
				KEY_CREATE_SUB_KEY)  \
				& (~SYNCHRONIZE)  \
			      )
#define KEY_EXECUTE	      ((KEY_READ)  \
				& (~SYNCHRONIZE))  \
			      )
#define KEY_ALL_ACCESS        ((STANDARD_RIGHTS_ALL|  \
				KEY_QUERY_VALUE|  \
				KEY_SET_VALUE|  \
				KEY_CREATE_SUB_KEY|  \
				KEY_ENUMERATE_SUB_KEYS|  \
				KEY_NOTIFY|  \
				KEY_CREATE_LINK)  \
				& (~SYNCHRONIZE)  \
			      )
/* ------------------------------ end registry ------------------------------ */


#define EVENTLOG_SUCCESS                0x0000
#define EVENTLOG_ERROR_TYPE             0x0001
#define EVENTLOG_WARNING_TYPE           0x0002
#define EVENTLOG_INFORMATION_TYPE       0x0004
#define EVENTLOG_AUDIT_SUCCESS          0x0008
#define EVENTLOG_AUDIT_FAILURE          0x0010

#define SERVICE_BOOT_START   0x00000000
#define SERVICE_SYSTEM_START 0x00000001
#define SERVICE_AUTO_START   0x00000002
#define SERVICE_DEMAND_START 0x00000003
#define SERVICE_DISABLED     0x00000004

#define SERVICE_ERROR_IGNORE   0x00000000
#define SERVICE_ERROR_NORMAL   0x00000001
#define SERVICE_ERROR_SEVERE   0x00000002
#define SERVICE_ERROR_CRITICAL 0x00000003

/* Service types */
#define SERVICE_KERNEL_DRIVER      0x00000001
#define SERVICE_FILE_SYSTEM_DRIVER 0x00000002
#define SERVICE_ADAPTER            0x00000004
#define SERVICE_RECOGNIZER_DRIVER  0x00000008

#define SERVICE_DRIVER ( SERVICE_KERNEL_DRIVER | SERVICE_FILE_SYSTEM_DRIVER | \
                         SERVICE_RECOGNIZER_DRIVER )

#define SERVICE_WIN32_OWN_PROCESS   0x00000010
#define SERVICE_WIN32_SHARE_PROCESS 0x00000020
#define SERVICE_WIN32  (SERVICE_WIN32_OWN_PROCESS | SERVICE_WIN32_SHARE_PROCESS)

#define SERVICE_INTERACTIVE_PROCESS 0x00000100

#define SERVICE_TYPE_ALL ( SERVICE_WIN32 | SERVICE_ADAPTER | \
                           SERVICE_DRIVER | SERVICE_INTERACTIVE_PROCESS )


typedef enum _CM_SERVICE_NODE_TYPE 
{
  DriverType               = SERVICE_KERNEL_DRIVER,
  FileSystemType           = SERVICE_FILE_SYSTEM_DRIVER,
  Win32ServiceOwnProcess   = SERVICE_WIN32_OWN_PROCESS,
  Win32ServiceShareProcess = SERVICE_WIN32_SHARE_PROCESS,
  AdapterType              = SERVICE_ADAPTER,
  RecognizerType           = SERVICE_RECOGNIZER_DRIVER
} SERVICE_NODE_TYPE;

typedef enum _CM_SERVICE_LOAD_TYPE 
{
  BootLoad    = SERVICE_BOOT_START,
  SystemLoad  = SERVICE_SYSTEM_START,
  AutoLoad    = SERVICE_AUTO_START,
  DemandLoad  = SERVICE_DEMAND_START,
  DisableLoad = SERVICE_DISABLED
} SERVICE_LOAD_TYPE;

typedef enum _CM_ERROR_CONTROL_TYPE 
{
  IgnoreError   = SERVICE_ERROR_IGNORE,
  NormalError   = SERVICE_ERROR_NORMAL,
  SevereError   = SERVICE_ERROR_SEVERE,
  CriticalError = SERVICE_ERROR_CRITICAL
} SERVICE_ERROR_TYPE;



#define RtlEqualMemory(Destination, Source, Length) (!memcmp((Destination),(Source),(Length)))
#define RtlMoveMemory(Destination, Source, Length) memmove((Destination),(Source),(Length))
#define RtlCopyMemory(Destination, Source, Length) memcpy((Destination),(Source),(Length))
#define RtlFillMemory(Destination, Length, Fill) memset((Destination),(Fill),(Length))
#define RtlZeroMemory(Destination, Length) memset((Destination),0,(Length))

#include "guiddef.h"

#include "poppack.h"

typedef struct _RTL_CRITICAL_SECTION_DEBUG 
{
  WORD   Type;
  WORD   CreatorBackTraceIndex;
  struct _RTL_CRITICAL_SECTION *CriticalSection;
  LIST_ENTRY ProcessLocksList;
  DWORD EntryCount;
  DWORD ContentionCount;
  DWORD Spare[ 2 ];
} RTL_CRITICAL_SECTION_DEBUG, *PRTL_CRITICAL_SECTION_DEBUG, RTL_RESOURCE_DEBUG, *PRTL_RESOURCE_DEBUG;

typedef struct _RTL_CRITICAL_SECTION {
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    ULONG_PTR SpinCount;
}  RTL_CRITICAL_SECTION, *PRTL_CRITICAL_SECTION;

#endif  /* __WINE_WINNT_H */
