/*
 * Thread definitions
 *
 * Copyright 1996 Alexandre Julliard
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

#ifndef __WINE_THREAD_H
#define __WINE_THREAD_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#define WINE_NO_TEB
#include <winternl.h>

#ifndef WINE_TEB_DEFINED
#define WINE_TEB_DEFINED
typedef struct _TEB
{
    NT_TIB       Tib;            /* 12-  00 Thread information block */
    PVOID        EnvironmentPointer; /* 12-  1c EnvironmentPointer (win95: tib flags + win16 mutex count) */
    CLIENT_ID    ClientId;       /* -2-  20 Process and thread id (win95: debug context) */
    PVOID        ActiveRpcHandle;              /* 028 */
    PVOID        ThreadLocalStoragePointer;    /* 02c Pointer to TLS array */
    PEB         *Peb;                          /* 030 owning process PEB */
    DWORD        LastErrorValue;               /* 034 Last error code */
    ULONG        CountOfOwnedCriticalSections; /* 038 */
    PVOID        CsrClientThread;              /* 03c */
    PVOID        Win32ThreadInfo;              /* 040 */
    ULONG        Win32ClientInfo[0x1f];        /* 044 */
    PVOID        WOW32Reserved;                /* 0c0 */
    ULONG        CurrentLocale;                /* 0c4 */
    ULONG        FpSoftwareStatusRegister;     /* 0c8 */
    PVOID        SystemReserved1[54];          /* 0cc */
    LONG         ExceptionCode;                /* 1a4 */
    ACTIVATION_CONTEXT_STACK ActivationContextStack; /* 1a8 */
    BYTE         SpareBytes1[24];              /* 1bc */
    PVOID        SystemReserved2[10];          /* 1d4 */

    /* The following are Wine-specific fields (NT: GdiTebBatch) */
    DWORD        dpmi_vif;            /* 1fc protected mode virtual interrupt flag */
    ULONG_PTR    vm86_pending;        /* 200 data for vm86 mode */
    /* here is plenty space for wine specific fields (don't forget to change pad6!!) */
    DWORD        pad6[310];           /* 204 */

    ULONG        gdiRgn;                     /* 6dc */
    ULONG        gdiPen;                     /* 6e0 */
    ULONG        gdiBrush;                   /* 6e4 */
    CLIENT_ID    RealClientId;               /* 6e8 */
    HANDLE       GdiCachedProcessHandle;     /* 6f0 */
    ULONG        GdiClientPID;               /* 6f4 */
    ULONG        GdiClientTID;               /* 6f8 */
    PVOID        GdiThreadLocaleInfo;        /* 6fc */
    PVOID        UserReserved[5];            /* 700 */
    PVOID        glDispachTable[280];        /* 714 */
    ULONG        glReserved1[26];            /* b74 */
    PVOID        glReserved2;                /* bdc */
    PVOID        glSectionInfo;              /* be0 */
    PVOID        glSection;                  /* be4 */
    PVOID        glTable;                    /* be8 */
    PVOID        glCurrentRC;                /* bec */
    PVOID        glContext;                  /* bf0 */
    ULONG        LastStatusValue;            /* bf4 */
    UNICODE_STRING StaticUnicodeString;      /* bf8 */
    WCHAR        StaticUnicodeBuffer[261];   /* c00 */
    PVOID        DeallocationStack;          /* e0c */
    PVOID        TlsSlots[64];               /* e10 */
    LIST_ENTRY   TlsLinks;                   /* f10 */
    PVOID        Vdm;                        /* f18 */
    PVOID        ReservedForNtRpc;           /* f1c */
    PVOID        DbgSsReserved[2];           /* f20 */
    ULONG        HardErrorDisabled;          /* f28 */
    PVOID        Instrumentation[16];        /* f2c */
    PVOID        WinSockData;                /* f6c */
    ULONG        GdiBatchCount;              /* f70 */
    ULONG        Spare2;                     /* f74 */
    ULONG        Spare3;                     /* f78 */
    ULONG        Spare4;                     /* f7c */
    PVOID        ReservedForOle;             /* f80 */
    ULONG        WaitingOnLoaderLock;        /* f84 */
    PVOID        Reserved5[3];               /* f88 */
    PVOID       *TlsExpansionSlots;          /* f94 */
} TEB;
#endif /* WINE_TEB_DEFINED */


/* The thread information for 16-bit threads */
/* NtCurrentTeb()->SubSystemTib points to this */
typedef struct
{
    void           *unknown;    /* 00 unknown */
    UNICODE_STRING *exe_name;   /* 04 exe module name */

    /* the following fields do not exist under Windows */
    UNICODE_STRING  exe_str;    /* exe name string pointed to by exe_name */
    CURDIR          curdir;     /* current directory */
    WCHAR           curdir_buffer[MAX_PATH];
} WIN16_SUBSYSTEM_TIB;

#endif  /* __WINE_THREAD_H */
