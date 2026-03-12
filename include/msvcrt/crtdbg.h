/*
 * Debug API
 *
 * Copyright 2001 Francois Gouget.
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
#ifndef __WINE_CRTDBG_H_
#define __WINE_CRTDBG_H_

#include <corecrt.h>

/* The debug API is not implemented in Winelib.
 * Redirect everything to the regular APIs.
 */

#define _CRT_WARN                       0
#define _CRT_ERROR                      1
#define _CRT_ASSERT                     2
#define _CRT_ERRCNT                     3

#define _CRTDBG_MODE_FILE               0x1
#define _CRTDBG_MODE_DEBUG              0x2
#define _CRTDBG_MODE_WNDW               0x4
#define _CRTDBG_REPORT_MODE             -1

typedef void *_HFILE;

#define _CRTDBG_INVALID_HFILE           ((_HFILE)(intptr_t)-1)
#define _CRTDBG_HFILE_ERROR             ((_HFILE)(intptr_t)-2)
#define _CRTDBG_FILE_STDOUT             ((_HFILE)(intptr_t)-4)
#define _CRTDBG_FILE_STDERR             ((_HFILE)(intptr_t)-5)
#define _CRTDBG_REPORT_FILE             ((_HFILE)(intptr_t)-6)

#define _FREE_BLOCK                     0
#define _NORMAL_BLOCK                   1
#define _CRT_BLOCK                      2
#define _IGNORE_BLOCK                   3
#define _CLIENT_BLOCK                   4
#define _MAX_BLOCKS                     5

#define _BLOCK_TYPE(block)              (block & 0xFFFF)
#define _BLOCK_SUBTYPE(block)           (block >> 16 & 0xFFFF)

typedef struct _CrtMemState
{
    struct _CrtMemBlockHeader* pBlockHeader;
    __msvcrt_ulong lCounts[_MAX_BLOCKS];
    __msvcrt_ulong lSizes[_MAX_BLOCKS];
    __msvcrt_ulong lHighWaterCount;
    __msvcrt_ulong lTotalCount;
} _CrtMemState;

typedef int (__cdecl* _CRT_REPORT_HOOK )(int, char*,    int*);
typedef int (__cdecl* _CRT_REPORT_HOOKW)(int, wchar_t*, int*);

#define _CRT_RPTHOOK_INSTALL        0
#define _CRT_RPTHOOK_REMOVE         1

#define _CRTDBG_ALLOC_MEM_DF        0x01
#define _CRTDBG_DELAY_FREE_MEM_DF   0x02
#define _CRTDBG_CHECK_ALWAYS_DF     0x04
#define _CRTDBG_RESERVED_DF         0x08
#define _CRTDBG_CHECK_CRT_DF        0x10
#define _CRTDBG_LEAK_CHECK_DF       0x20

#define _CRTDBG_CHECK_EVERY_16_DF   0x00100000
#define _CRTDBG_CHECK_EVERY_128_DF  0x00800000
#define _CRTDBG_CHECK_EVERY_1024_DF 0x04000000

#define _CRTDBG_CHECK_DEFAULT_DF    0

#define _CRTDBG_REPORT_FLAG         -1

#ifndef _DEBUG

#define _ASSERT(expr)                   ((void)0)
#define _ASSERTE(expr)                  ((void)0)
#define _CrtDbgBreak()                  ((void)0)

#define _CrtCheckMemory()               ((int)1)
#define _CrtDbgReport(...)              ((int)0)
#define _CrtDumpMemoryLeaks()           ((int)0)
#define _CrtSetBreakAlloc(a)            ((__msvcrt_long)0)
#define _CrtSetDbgFlag(f)               ((int)0)
#define _CrtSetDumpClient(f)            ((void)0)
#define _CrtSetReportMode(t,m)          ((int)0)
#define _CrtSetReportFile(t,f)          ((void*)0)

#define _CrtGetReportHook()             ((_CRT_REPORT_HOOK)0)
#define _CrtSetReportHook(f)            ((_CRT_REPORT_HOOK)0)

#else /* _DEBUG */

#include <assert.h>
#define _ASSERT(expr)                   assert(expr)
#define _ASSERTE(expr)                  assert(expr)
#if defined(_MSC_VER)
#define _CrtDbgBreak()                  __debugbreak()
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define _CrtDbgBreak()                  __asm__ ("\tint $0x3\n")
#else
#define _CrtDbgBreak()                  ((void)0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern int _crtAssertBusy;
extern int _crtBreakAlloc;
extern int _crtDbgFlag;

_ACRTIMP int   __cdecl _CrtCheckMemory(void);
_ACRTIMP int   __cdecl _CrtDbgReport(int reportType, const char *filename, int linenumber,
                                     const char *moduleName, const char *format, ...) __WINE_CRT_PRINTF_ATTR(5, 6);
_ACRTIMP int   __cdecl _CrtDbgReportW(int reportType, const wchar_t *filename, int linenumber,
                                      const wchar_t *moduleName, const wchar_t *format, ...);
_ACRTIMP int   __cdecl _CrtDumpMemoryLeaks(void);
_ACRTIMP int   __cdecl _CrtSetBreakAlloc(int);
_ACRTIMP int   __cdecl _CrtSetDbgFlag(int);
_ACRTIMP void *__cdecl _CrtSetDumpClient(void *dumpClient);
_ACRTIMP int   __cdecl _CrtSetReportMode(int reportType, int reportMode);
_ACRTIMP void *__cdecl _CrtSetReportFile(int reportType, void *reportFile); /* void* for HFILE */

_ACRTIMP _CRT_REPORT_HOOK __cdecl _CrtGetReportHook(void);
_ACRTIMP _CRT_REPORT_HOOK __cdecl _CrtSetReportHook(_CRT_REPORT_HOOK);

#ifdef __cplusplus
}
#endif

#endif /* _DEBUG */

#define _CrtDoForAllClientObjects(f,c)  ((void)0)
#define _CrtIsMemoryBlock(p,s,r,f,l)    ((int)1)
#define _CrtIsValidHeapPointer(p)       ((int)1)
#define _CrtIsValidPointer(p,s,a)       ((int)1)
#define _CrtMemCheckpoint(s)            ((void)0)
#define _CrtMemDifference(s1,s2,s3)     ((int)0)
#define _CrtMemDumpAllObjectsSince(s)   ((void)0)
#define _CrtMemDumpStatistics(s)        ((void)0)
#define _CrtSetAllocHook(f)             ((void)0)

#define _RPT0(t,m)
#define _RPT1(t,m,p1)
#define _RPT2(t,m,p1,p2)
#define _RPT3(t,m,p1,p2,p3)
#define _RPT4(t,m,p1,p2,p3,p4)
#define _RPTF0(t,m)
#define _RPTF1(t,m,p1)
#define _RPTF2(t,m,p1,p2)
#define _RPTF3(t,m,p1,p2,p3)
#define _RPTF4(t,m,p1,p2,p3,p4)


#define _malloc_dbg(s,t,f,l)            malloc(s)
#define _calloc_dbg(c,s,t,f,l)          calloc(c,s)
#define _expand_dbg(p,s,t,f,l)          _expand(p,s)
#define _free_dbg(p,t)                  free(p)
#define _realloc_dbg(p,s,t,f,l)         realloc(p,s)

#endif /* __WINE_CRTDBG_H */
