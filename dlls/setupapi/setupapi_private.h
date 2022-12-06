/*
 * Copyright 2001 Andreas Mohr
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

#ifndef __SETUPAPI_PRIVATE_H
#define __SETUPAPI_PRIVATE_H

#include <windef.h>
#include <winuser.h>

#define COPYFILEDLGORD	1000
#define SOURCESTRORD	500
#define DESTSTRORD	501
#define PROGRESSORD	502

#define IDPROMPTFORDISK   1001
#define IDC_FILENEEDED    503
#define IDC_INFO          504
#define IDC_COPYFROM      505
#define IDC_PATH          506
#define IDC_RUNDLG_BROWSE 507

#define IDS_PROMPTDISK  508
#define IDS_UNKNOWN     509
#define IDS_COPYFROM    510
#define IDS_INFO        511

#define REG_INSTALLEDFILES "System\\CurrentControlSet\\Control\\InstalledFiles"
#define REGPART_RENAME "\\Rename"
#define REG_VERSIONCONFLICT "Software\\Microsoft\\VersionConflictManager"

#define PNF_HEADER L"Wine PNF header\n"

extern HINSTANCE SETUPAPI_hInstance DECLSPEC_HIDDEN;

static inline void * __WINE_ALLOC_SIZE(2) heap_realloc_zero(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, len);
}

static inline WCHAR *strdupW( const WCHAR *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        int len = (lstrlenW(str) + 1) * sizeof(WCHAR);
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len ))) memcpy( ret, str, len );
    }
    return ret;
}

static inline char *strdupWtoA( const WCHAR *str )
{
    char *ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline WCHAR *strdupAtoW( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

/* exported functions not in public headers */

void    WINAPI MyFree( void *mem );
void *  WINAPI MyMalloc( DWORD size ) __WINE_ALLOC_SIZE(1) __WINE_DEALLOC(MyFree) __WINE_MALLOC;
void *  WINAPI MyRealloc( void *src, DWORD size ) __WINE_ALLOC_SIZE(2) __WINE_DEALLOC(MyFree);
WCHAR * WINAPI MultiByteToUnicode( const char *str, UINT code_page ) __WINE_DEALLOC(MyFree) __WINE_MALLOC;
char *  WINAPI UnicodeToMultiByte( const WCHAR *str, UINT code_page ) __WINE_DEALLOC(MyFree) __WINE_MALLOC;

/* string substitutions */

struct inf_file;
extern const WCHAR *DIRID_get_string( int dirid ) DECLSPEC_HIDDEN;
extern const WCHAR *PARSER_get_inf_filename( HINF hinf ) DECLSPEC_HIDDEN;
extern WCHAR *PARSER_get_dest_dir( INFCONTEXT *context ) DECLSPEC_HIDDEN;

/* support for ANSI queue callback functions */

struct callback_WtoA_context
{
    void               *orig_context;
    PSP_FILE_CALLBACK_A orig_handler;
};

UINT CALLBACK QUEUE_callback_WtoA( void *context, UINT notification, UINT_PTR, UINT_PTR ) DECLSPEC_HIDDEN;

extern OSVERSIONINFOW OsVersionInfo DECLSPEC_HIDDEN;

extern BOOL create_fake_dll( const WCHAR *name, const WCHAR *source ) DECLSPEC_HIDDEN;
extern void cleanup_fake_dlls(void) DECLSPEC_HIDDEN;

#endif /* __SETUPAPI_PRIVATE_H */
