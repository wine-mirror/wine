/*
 * WLDAP32 - LDAP support for Wine
 *
 * Copyright 2005 Hans Leidekker
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

/* A set of helper functions to convert LDAP data structures
 * to and from ansi (A), wide character (W) and utf8 (U) encodings.
 */

static inline LPWSTR strAtoW( LPCSTR str )
{
    LPWSTR ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

static inline LPSTR strWtoA( LPCWSTR str )
{
    LPSTR ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline char *strWtoU( LPCWSTR str )
{
    LPSTR ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_UTF8, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static inline LPWSTR strUtoW( char *str )
{
    LPWSTR ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_UTF8, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_UTF8, 0, str, -1, ret, len );
    }
    return ret;
}

static inline void strfreeA( LPSTR str )
{
    HeapFree( GetProcessHeap(), 0, str );
}

static inline void strfreeW( LPWSTR str )
{
    HeapFree( GetProcessHeap(), 0, str );
}

static inline void strfreeU( char *str )
{
    HeapFree( GetProcessHeap(), 0, str );
}

static inline DWORD strarraylenA( LPSTR *strarray )
{
    LPSTR *p = strarray;
    while (*p) p++;
    return p - strarray;
}

static inline DWORD strarraylenW( LPWSTR *strarray )
{
    LPWSTR *p = strarray;
    while (*p) p++;
    return p - strarray;
}

static inline DWORD strarraylenU( char **strarray )
{
    char **p = strarray;
    while (*p) p++;
    return p - strarray;
}

static inline LPWSTR *strarrayAtoW( LPSTR *strarray )
{
    LPWSTR *strarrayW = NULL;
    DWORD size;

    if (strarray)
    {
        size  = sizeof(WCHAR*) * (strarraylenA( strarray ) + 1);
        strarrayW = HeapAlloc( GetProcessHeap(), 0, size );

        if (strarrayW)
        {
            LPSTR *p = strarray;
            LPWSTR *q = strarrayW;

            while (*p) *q++ = strAtoW( *p++ );
            *q = NULL;
        }
    }
    return strarrayW;
}

static inline LPSTR *strarrayWtoA( LPWSTR *strarray )
{
    LPSTR *strarrayA = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(LPSTR) * (strarraylenW( strarray ) + 1);
        strarrayA = HeapAlloc( GetProcessHeap(), 0, size );

        if (strarrayA)
        {
            LPWSTR *p = strarray;
            LPSTR *q = strarrayA;

            while (*p) *q++ = strWtoA( *p++ );
            *q = NULL;
        }
    }
    return strarrayA;
}

static inline char **strarrayWtoU( LPWSTR *strarray )
{
    char **strarrayU = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(char*) * (strarraylenW( strarray ) + 1);
        strarrayU = HeapAlloc( GetProcessHeap(), 0, size );

        if (strarrayU)
        {
            LPWSTR *p = strarray;
            char **q = strarrayU;

            while (*p) *q++ = strWtoU( *p++ );
            *q = NULL;
        }
    }
    return strarrayU;
}

static inline LPWSTR *strarrayUtoW( char **strarray )
{
    LPWSTR *strarrayW = NULL;
    DWORD size;

    if (strarray)
    {
        size = sizeof(WCHAR*) * (strarraylenU( strarray ) + 1);
        strarrayW = HeapAlloc( GetProcessHeap(), 0, size );

        if (strarrayW)
        {
            char **p = strarray;
            LPWSTR *q = strarrayW;

            while (*p) *q++ = strUtoW( *p++ );
            *q = NULL;
        }
    }
    return strarrayW;
}

static inline void strarrayfreeA( LPSTR *strarray )
{
    if (strarray)
    {
        LPSTR *p = strarray;
        while (*p) strfreeA( *p++ );
        HeapFree( GetProcessHeap(), 0, strarray );
    }
}

static inline void strarrayfreeW( LPWSTR *strarray )
{
    if (strarray)
    {
        LPWSTR *p = strarray;
        while (*p) strfreeW( *p++ );
        HeapFree( GetProcessHeap(), 0, strarray );
    }
}

static inline void strarrayfreeU( char **strarray )
{
    if (strarray)
    {
        char **p = strarray;
        while (*p) strfreeU( *p++ );
        HeapFree( GetProcessHeap(), 0, strarray );
    }
}
