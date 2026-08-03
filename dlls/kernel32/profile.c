/*
 * Profile functions
 *
 * Copyright 1993 Miguel de Icaza
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

#include <string.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winerror.h"
#include "winreg.h"
#include "winternl.h"
#include "shlwapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(profile);

static const char bom_utf8[] = {0xEF,0xBB,0xBF};

typedef enum
{
    ENCODING_ANSI = 1,
    ENCODING_UTF8,
    ENCODING_UTF16LE,
    ENCODING_UTF16BE
} ENCODING;

typedef struct tagPROFILEKEY
{
    WCHAR                 *value;
    struct tagPROFILEKEY  *next;
    WCHAR                  name[1];
} PROFILEKEY;

typedef struct tagPROFILESECTION
{
    struct tagPROFILEKEY       *key;
    struct tagPROFILESECTION   *next;
    WCHAR                       name[1];
} PROFILESECTION;


typedef struct
{
    BOOL             changed;
    PROFILESECTION  *section;
    WCHAR           *filename;
    FILETIME LastWriteTime;
    ENCODING encoding;
} PROFILE;


#define N_CACHED_PROFILES 10

/* Cached profile files */
static PROFILE *MRUProfile[N_CACHED_PROFILES]={NULL};

#define CurProfile (MRUProfile[0])

/* Check for comments in profile */
#define IS_ENTRY_COMMENT(str)  ((str)[0] == ';')

static CRITICAL_SECTION PROFILE_CritSect;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &PROFILE_CritSect,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": PROFILE_CritSect") }
};
static CRITICAL_SECTION PROFILE_CritSect = { &critsect_debug, -1, 0, 0, 0, 0 };

static const char hex[16] = "0123456789ABCDEF";

/***********************************************************************
 *           PROFILE_CopyEntry
 *
 * Copy the content of an entry into a buffer, removing quotes, and possibly
 * translating environment variables.
 */
static void PROFILE_CopyEntry( LPWSTR buffer, LPCWSTR value, int len )
{
    WCHAR quote = '\0';

    if(!buffer) return;

    if (*value == '\'' || *value == '\"')
    {
        if (value[1] && (value[lstrlenW(value)-1] == *value)) quote = *value++;
    }

    lstrcpynW( buffer, value, len );
    if (quote && (len >= lstrlenW(value))) buffer[lstrlenW(buffer)-1] = '\0';
}

/* byte-swaps shorts in-place in a buffer. len is in WCHARs */
static inline void PROFILE_ByteSwapShortBuffer(WCHAR * buffer, int len)
{
    int i;
    USHORT * shortbuffer = buffer;
    for (i = 0; i < len; i++)
        shortbuffer[i] = RtlUshortByteSwap(shortbuffer[i]);
}

/* writes any necessary encoding marker to the file */
static inline void PROFILE_WriteMarker(HANDLE hFile, ENCODING encoding)
{
    DWORD dwBytesWritten;
    WCHAR bom;
    switch (encoding)
    {
    case ENCODING_ANSI:
        break;
    case ENCODING_UTF8:
        WriteFile(hFile, bom_utf8, sizeof(bom_utf8), &dwBytesWritten, NULL);
        break;
    case ENCODING_UTF16LE:
        bom = 0xFEFF;
        WriteFile(hFile, &bom, sizeof(bom), &dwBytesWritten, NULL);
        break;
    case ENCODING_UTF16BE:
        bom = 0xFFFE;
        WriteFile(hFile, &bom, sizeof(bom), &dwBytesWritten, NULL);
        break;
    }
}

static void PROFILE_WriteLine( HANDLE hFile, WCHAR * szLine, int len, ENCODING encoding)
{
    char * write_buffer;
    int write_buffer_len;
    DWORD dwBytesWritten;

    TRACE("writing: %s\n", debugstr_wn(szLine, len));

    switch (encoding)
    {
    case ENCODING_ANSI:
        write_buffer_len = WideCharToMultiByte(CP_ACP, 0, szLine, len, NULL, 0, NULL, NULL);
        write_buffer = HeapAlloc(GetProcessHeap(), 0, write_buffer_len);
        if (!write_buffer) return;
        len = WideCharToMultiByte(CP_ACP, 0, szLine, len, write_buffer, write_buffer_len, NULL, NULL);
        WriteFile(hFile, write_buffer, len, &dwBytesWritten, NULL);
        HeapFree(GetProcessHeap(), 0, write_buffer);
        break;
    case ENCODING_UTF8:
        write_buffer_len = WideCharToMultiByte(CP_UTF8, 0, szLine, len, NULL, 0, NULL, NULL);
        write_buffer = HeapAlloc(GetProcessHeap(), 0, write_buffer_len);
        if (!write_buffer) return;
        len = WideCharToMultiByte(CP_UTF8, 0, szLine, len, write_buffer, write_buffer_len, NULL, NULL);
        WriteFile(hFile, write_buffer, len, &dwBytesWritten, NULL);
        HeapFree(GetProcessHeap(), 0, write_buffer);
        break;
    case ENCODING_UTF16LE:
        WriteFile(hFile, szLine, len * sizeof(WCHAR), &dwBytesWritten, NULL);
        break;
    case ENCODING_UTF16BE:
        PROFILE_ByteSwapShortBuffer(szLine, len);
        WriteFile(hFile, szLine, len * sizeof(WCHAR), &dwBytesWritten, NULL);
        break;
    default:
        FIXME("encoding type %d not implemented\n", encoding);
    }
}

/***********************************************************************
 *           PROFILE_Save
 *
 * Save a profile tree to a file.
 */
static void PROFILE_Save( HANDLE hFile, const PROFILESECTION *section, ENCODING encoding )
{
    PROFILEKEY *key;
    WCHAR *buffer, *p;

    PROFILE_WriteMarker(hFile, encoding);

    for ( ; section; section = section->next)
    {
        int len = 0;

        if (section->name[0]) len += lstrlenW(section->name) + 4;

        for (key = section->key; key; key = key->next)
        {
            len += lstrlenW(key->name) + 2;
            if (key->value) len += lstrlenW(key->value) + 1;
        }

        buffer = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!buffer) return;

        p = buffer;
        if (section->name[0])
        {
            *p++ = '[';
            lstrcpyW( p, section->name );
            p += lstrlenW(p);
            *p++ = ']';
            *p++ = '\r';
            *p++ = '\n';
        }

        for (key = section->key; key; key = key->next)
        {
            lstrcpyW( p, key->name );
            p += lstrlenW(p);
            if (key->value)
            {
                *p++ = '=';
                lstrcpyW( p, key->value );
                p += lstrlenW(p);
            }
            *p++ = '\r';
            *p++ = '\n';
        }
        PROFILE_WriteLine( hFile, buffer, len, encoding );
        HeapFree(GetProcessHeap(), 0, buffer);
    }
}


/***********************************************************************
 *           PROFILE_Free
 *
 * Free a profile tree.
 */
static void PROFILE_Free( PROFILESECTION *section )
{
    PROFILESECTION *next_section;
    PROFILEKEY *key, *next_key;

    for ( ; section; section = next_section)
    {
        for (key = section->key; key; key = next_key)
        {
            next_key = key->next;
            HeapFree( GetProcessHeap(), 0, key->value );
            HeapFree( GetProcessHeap(), 0, key );
        }
        next_section = section->next;
        HeapFree( GetProcessHeap(), 0, section );
    }
}

/* returns TRUE if a whitespace character, else FALSE */
static inline BOOL PROFILE_isspaceW(WCHAR c)
{
    /* ^Z (DOS EOF) is a space too  (found on CD-ROMs) */
    return (c >= 0x09 && c <= 0x0d) || c == 0x1a || c == 0x20;
}

static inline ENCODING PROFILE_DetectTextEncoding(const void * buffer, int * len)
{
    int flags = IS_TEXT_UNICODE_SIGNATURE |
                IS_TEXT_UNICODE_REVERSE_SIGNATURE |
                IS_TEXT_UNICODE_ODD_LENGTH;
    if (*len >= sizeof(bom_utf8) && !memcmp(buffer, bom_utf8, sizeof(bom_utf8)))
    {
        *len = sizeof(bom_utf8);
        return ENCODING_UTF8;
    }
    RtlIsTextUnicode(buffer, *len, &flags);
    if (flags & IS_TEXT_UNICODE_SIGNATURE)
    {
        *len = sizeof(WCHAR);
        return ENCODING_UTF16LE;
    }
    if (flags & IS_TEXT_UNICODE_REVERSE_SIGNATURE)
    {
        *len = sizeof(WCHAR);
        return ENCODING_UTF16BE;
    }
    *len = 0;
    return ENCODING_ANSI;
}

static void profile_trim_spaces(WCHAR **start, WCHAR **end)
{
    WCHAR *s = *start, *e = *end;

    while (s < e && PROFILE_isspaceW(*s)) s++;
    while ((e > s) && PROFILE_isspaceW(e[-1])) e--;

    *start = s;
    *end = e;
}

/***********************************************************************
 *           PROFILE_Load
 *
 * Load a profile tree from a file.
 */
static PROFILESECTION *PROFILE_Load(HANDLE hFile, ENCODING * pEncoding)
{
    void *buffer_base, *pBuffer;
    WCHAR * szFile;
    WCHAR *szLineStart, *szLineEnd, *next_line;
    const WCHAR *szValueStart, *szEnd;
    int len;
    PROFILESECTION *section, *first_section;
    PROFILESECTION **next_section;
    PROFILEKEY *key, *prev_key, **next_key;
    DWORD dwFileSize;
    
    TRACE("%p\n", hFile);
    
    dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == INVALID_FILE_SIZE || dwFileSize == 0)
        return NULL;

    buffer_base = HeapAlloc(GetProcessHeap(), 0 , dwFileSize);
    if (!buffer_base) return NULL;
    
    if (!ReadFile(hFile, buffer_base, dwFileSize, &dwFileSize, NULL))
    {
        HeapFree(GetProcessHeap(), 0, buffer_base);
        WARN("Error %ld reading file\n", GetLastError());
        return NULL;
    }
    len = dwFileSize;
    *pEncoding = PROFILE_DetectTextEncoding(buffer_base, &len);
    /* len is set to the number of bytes in the character marker.
     * we want to skip these bytes */
    pBuffer = (char *)buffer_base + len;
    dwFileSize -= len;
    switch (*pEncoding)
    {
    case ENCODING_ANSI:
        TRACE("ANSI encoding\n");

        len = MultiByteToWideChar(CP_ACP, 0, pBuffer, dwFileSize, NULL, 0);
        szFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!szFile)
        {
            HeapFree(GetProcessHeap(), 0, buffer_base);
            return NULL;
        }
        MultiByteToWideChar(CP_ACP, 0, pBuffer, dwFileSize, szFile, len);
        szEnd = szFile + len;
        break;
    case ENCODING_UTF8:
        TRACE("UTF8 encoding\n");

        len = MultiByteToWideChar(CP_UTF8, 0, pBuffer, dwFileSize, NULL, 0);
        szFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!szFile)
        {
            HeapFree(GetProcessHeap(), 0, buffer_base);
            return NULL;
        }
        MultiByteToWideChar(CP_UTF8, 0, pBuffer, dwFileSize, szFile, len);
        szEnd = szFile + len;
        break;
    case ENCODING_UTF16LE:
        TRACE("UTF16 Little Endian encoding\n");
        szFile = pBuffer;
        szEnd = (WCHAR *)((char *)pBuffer + dwFileSize);
        break;
    case ENCODING_UTF16BE:
        TRACE("UTF16 Big Endian encoding\n");
        szFile = pBuffer;
        szEnd = (WCHAR *)((char *)pBuffer + dwFileSize);
        PROFILE_ByteSwapShortBuffer(szFile, dwFileSize / sizeof(WCHAR));
        break;
    default:
        FIXME("encoding type %d not implemented\n", *pEncoding);
        HeapFree(GetProcessHeap(), 0, buffer_base);
        return NULL;
    }

    first_section = HeapAlloc( GetProcessHeap(), 0, sizeof(*section) );
    if(first_section == NULL)
    {
        if (szFile != pBuffer)
            HeapFree(GetProcessHeap(), 0, szFile);
        HeapFree(GetProcessHeap(), 0, buffer_base);
        return NULL;
    }
    first_section->name[0] = 0;
    first_section->key  = NULL;
    first_section->next = NULL;
    next_section = &first_section->next;
    next_key     = &first_section->key;
    prev_key     = NULL;
    next_line    = szFile;

    while (next_line < szEnd)
    {
        szLineStart = next_line;
        while (next_line < szEnd && *next_line != '\n' && *next_line != '\r') next_line++;
        while (next_line < szEnd && (*next_line == '\n' || *next_line == '\r')) next_line++;
        szLineEnd = next_line;

        /* get rid of white space */
        profile_trim_spaces(&szLineStart, &szLineEnd);

        if (szLineStart >= szLineEnd) continue;

        if (*szLineStart == '[')  /* section start */
        {
            for (len = szLineEnd - szLineStart; len > 0; len--) if (szLineStart[len - 1] == ']') break;
            if (!len)
            {
                WARN("Invalid section header: %s\n",
                    debugstr_wn(szLineStart, (int)(szLineEnd - szLineStart)) );
            }
            else
            {
                /* Skip brackets */
                szLineStart++;
                len -= 2;
                szLineEnd = szLineStart + len;
                profile_trim_spaces(&szLineStart, &szLineEnd);
                len = szLineEnd - szLineStart;

                /* no need to allocate +1 for NULL terminating character as
                 * already included in structure */
                if (!(section = HeapAlloc( GetProcessHeap(), 0, sizeof(*section) + len * sizeof(WCHAR) )))
                    break;
                memcpy(section->name, szLineStart, len * sizeof(WCHAR));
                section->name[len] = '\0';
                section->key  = NULL;
                section->next = NULL;
                *next_section = section;
                next_section  = &section->next;
                next_key      = &section->key;
                prev_key      = NULL;

                TRACE("New section: %s\n", debugstr_w(section->name));

                continue;
            }
        }

        /* get rid of white space after the name and before the start
         * of the value */
        len = szLineEnd - szLineStart;
        for (szValueStart = szLineStart; szValueStart < szLineEnd; szValueStart++) if (*szValueStart == '=') break;
        if (szValueStart < szLineEnd)
        {
            const WCHAR *szNameEnd = szValueStart;
            while ((szNameEnd > szLineStart) && PROFILE_isspaceW(szNameEnd[-1])) szNameEnd--;
            len = szNameEnd - szLineStart;
            szValueStart++;
            while (szValueStart < szLineEnd && PROFILE_isspaceW(*szValueStart)) szValueStart++;
        }
        else szValueStart = NULL;

        if (len || !prev_key || *prev_key->name)
        {
            /* no need to allocate +1 for NULL terminating character as
             * already included in structure */
            if (!(key = HeapAlloc( GetProcessHeap(), 0, sizeof(*key) + len * sizeof(WCHAR) ))) break;
            memcpy(key->name, szLineStart, len * sizeof(WCHAR));
            key->name[len] = '\0';
            if (szValueStart)
            {
                len = (int)(szLineEnd - szValueStart);
                key->value = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) );
                memcpy(key->value, szValueStart, len * sizeof(WCHAR));
                key->value[len] = '\0';
            }
            else key->value = NULL;

           key->next  = NULL;
           *next_key  = key;
           next_key   = &key->next;
           prev_key   = key;

           TRACE("New key: name=%s, value=%s\n",
               debugstr_w(key->name), key->value ? debugstr_w(key->value) : "(none)");
        }
    }
    if (szFile != pBuffer)
        HeapFree(GetProcessHeap(), 0, szFile);
    HeapFree(GetProcessHeap(), 0, buffer_base);
    return first_section;
}


/***********************************************************************
 *           PROFILE_DeleteKey
 *
 * Delete a key from a profile tree.
 */
static BOOL PROFILE_DeleteKey( PROFILESECTION **section,
			       LPCWSTR section_name, LPCWSTR key_name )
{
    while (*section)
    {
        if (!wcsicmp( (*section)->name, section_name ))
        {
            PROFILEKEY **key = &(*section)->key;
            while (*key)
            {
                if (!wcsicmp( (*key)->name, key_name ))
                {
                    PROFILEKEY *to_del = *key;
                    *key = to_del->next;
                    HeapFree( GetProcessHeap(), 0, to_del->value);
                    HeapFree( GetProcessHeap(), 0, to_del );
                    return TRUE;
                }
                key = &(*key)->next;
            }
        }
        section = &(*section)->next;
    }
    return FALSE;
}


/***********************************************************************
 *           PROFILE_DeleteAllKeys
 *
 * Delete all keys from a profile tree.
 */
static void PROFILE_DeleteAllKeys( LPCWSTR section_name)
{
    PROFILESECTION **section= &CurProfile->section;
    while (*section)
    {
        if (!wcsicmp( (*section)->name, section_name ))
        {
            PROFILEKEY **key = &(*section)->key;
            while (*key)
            {
                PROFILEKEY *to_del = *key;
		*key = to_del->next;
                HeapFree( GetProcessHeap(), 0, to_del->value);
		HeapFree( GetProcessHeap(), 0, to_del );
		CurProfile->changed =TRUE;
            }
        }
        section = &(*section)->next;
    }
}


/***********************************************************************
 *           PROFILE_Find
 *
 * Find a key in a profile tree, optionally creating it.
 */
static PROFILEKEY *PROFILE_Find( PROFILESECTION **section, LPCWSTR section_name,
                                 LPCWSTR key_name, BOOL create, BOOL create_always )
{
    LPCWSTR p;
    int seclen = 0, keylen = 0;

    while (PROFILE_isspaceW(*section_name)) section_name++;
    if (*section_name)
    {
        p = section_name + lstrlenW(section_name) - 1;
        while ((p > section_name) && PROFILE_isspaceW(*p)) p--;
        seclen = p - section_name + 1;
    }

    while (PROFILE_isspaceW(*key_name)) key_name++;
    if (*key_name)
    {
        p = key_name + lstrlenW(key_name) - 1;
        while ((p > key_name) && PROFILE_isspaceW(*p)) p--;
        keylen = p - key_name + 1;
    }

    while (*section)
    {
        if (!wcsnicmp((*section)->name, section_name, seclen) &&
            ((*section)->name)[seclen] == '\0')
        {
            PROFILEKEY **key = &(*section)->key;

            while (*key)
            {
                /* If create_always is FALSE then we check if the keyname
                 * already exists. Otherwise we add it regardless of its
                 * existence, to allow keys to be added more than once in
                 * some cases.
                 */
                if(!create_always)
                {
                    if ( (!(wcsnicmp( (*key)->name, key_name, keylen )))
                         && (((*key)->name)[keylen] == '\0') )
                        return *key;
                }
                key = &(*key)->next;
            }
            if (!create) return NULL;
            if (!(*key = HeapAlloc( GetProcessHeap(), 0, sizeof(PROFILEKEY) + lstrlenW(key_name) * sizeof(WCHAR) )))
                return NULL;
            lstrcpyW( (*key)->name, key_name );
            (*key)->value = NULL;
            (*key)->next  = NULL;
            return *key;
        }
        section = &(*section)->next;
    }
    if (!create) return NULL;
    *section = HeapAlloc( GetProcessHeap(), 0, sizeof(PROFILESECTION) + lstrlenW(section_name) * sizeof(WCHAR) );
    if(*section == NULL) return NULL;
    lstrcpyW( (*section)->name, section_name );
    (*section)->next = NULL;
    if (!((*section)->key  = HeapAlloc( GetProcessHeap(), 0,
                                        sizeof(PROFILEKEY) + lstrlenW(key_name) * sizeof(WCHAR) )))
    {
        HeapFree(GetProcessHeap(), 0, *section);
        return NULL;
    }
    lstrcpyW( (*section)->key->name, key_name );
    (*section)->key->value = NULL;
    (*section)->key->next  = NULL;
    return (*section)->key;
}


/***********************************************************************
 *           PROFILE_FlushFile
 *
 * Flush the current profile to disk if changed.
 */
static BOOL PROFILE_FlushFile(void)
{
    HANDLE hFile = NULL;
    FILETIME LastWriteTime;

    if(!CurProfile)
    {
        WARN("No current profile!\n");
        return FALSE;
    }

    if (!CurProfile->changed) return TRUE;

    hFile = CreateFileW(CurProfile->filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        WARN("could not save profile file %s (error was %ld)\n", debugstr_w(CurProfile->filename), GetLastError());
        return FALSE;
    }

    TRACE("Saving %s\n", debugstr_w(CurProfile->filename));
    PROFILE_Save( hFile, CurProfile->section, CurProfile->encoding );
    if(GetFileTime(hFile, NULL, NULL, &LastWriteTime))
       CurProfile->LastWriteTime=LastWriteTime;
    CloseHandle( hFile );
    CurProfile->changed = FALSE;
    return TRUE;
}


/***********************************************************************
 *           PROFILE_ReleaseFile
 *
 * Flush the current profile to disk and remove it from the cache.
 */
static void PROFILE_ReleaseFile(void)
{
    PROFILE_FlushFile();
    PROFILE_Free( CurProfile->section );
    HeapFree( GetProcessHeap(), 0, CurProfile->filename );
    CurProfile->changed = FALSE;
    CurProfile->section = NULL;
    CurProfile->filename  = NULL;
    CurProfile->encoding = ENCODING_ANSI;
    ZeroMemory(&CurProfile->LastWriteTime, sizeof(CurProfile->LastWriteTime));
}

/***********************************************************************
 *
 * Compares a file time with the current time. If the file time is
 * at least 2.1 seconds in the past, return true.
 *
 * Intended as cache safety measure: The time resolution on FAT is
 * two seconds, so files that are not at least two seconds old might
 * keep their time even on modification, so don't cache them.
 */
static BOOL is_not_current(FILETIME *ft)
{
    LARGE_INTEGER now;
    LONGLONG ftll;

    NtQuerySystemTime( &now );
    ftll = ((LONGLONG)ft->dwHighDateTime << 32) + ft->dwLowDateTime;
    TRACE("%s; %s\n", wine_dbgstr_longlong(ftll), wine_dbgstr_longlong(now.QuadPart));
    return ftll + 21000000 < now.QuadPart;
}

/***********************************************************************
 *           PROFILE_Open
 *
 * Open a profile file, checking the cached file first.
 */
static BOOL PROFILE_Open( LPCWSTR filename, BOOL write_access )
{
    WCHAR buffer[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    FILETIME LastWriteTime;
    int i,j;
    PROFILE *tempProfile;
    
    ZeroMemory(&LastWriteTime, sizeof(LastWriteTime));

    /* First time around */

    if(!CurProfile)
       for(i=0;i<N_CACHED_PROFILES;i++)
       {
          MRUProfile[i]=HeapAlloc( GetProcessHeap(), 0, sizeof(PROFILE) );
          if(MRUProfile[i] == NULL) break;
          MRUProfile[i]->changed=FALSE;
          MRUProfile[i]->section=NULL;
          MRUProfile[i]->filename=NULL;
          MRUProfile[i]->encoding=ENCODING_ANSI;
          ZeroMemory(&MRUProfile[i]->LastWriteTime, sizeof(FILETIME));
       }

    if (!filename)
        filename = L"win.ini";

    if ((RtlDetermineDosPathNameType_U(filename) == RtlPathTypeRelative) &&
        !wcschr(filename, '\\') && !wcschr(filename, '/'))
    {
        WCHAR windirW[MAX_PATH];
        GetWindowsDirectoryW( windirW, MAX_PATH );
        lstrcpyW(buffer, windirW);
        lstrcatW(buffer, L"\\");
        lstrcatW(buffer, filename);
    }
    else
    {
        LPWSTR dummy;
        GetFullPathNameW(filename, ARRAY_SIZE(buffer), buffer, &dummy);
    }
        
    TRACE("path: %s\n", debugstr_w(buffer));

    hFile = CreateFileW(buffer, GENERIC_READ | (write_access ? GENERIC_WRITE : 0),
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if ((hFile == INVALID_HANDLE_VALUE) && (GetLastError() != ERROR_FILE_NOT_FOUND))
    {
        WARN("Error %ld opening file %s\n", GetLastError(), debugstr_w(buffer));
        return FALSE;
    }

    for(i=0;i<N_CACHED_PROFILES;i++)
    {
        if ((MRUProfile[i]->filename && !wcsicmp( buffer, MRUProfile[i]->filename )))
        {
            TRACE("MRU Filename: %s, new filename: %s\n", debugstr_w(MRUProfile[i]->filename), debugstr_w(buffer));
            if(i)
            {
                PROFILE_FlushFile();
                tempProfile=MRUProfile[i];
                for(j=i;j>0;j--)
                    MRUProfile[j]=MRUProfile[j-1];
                CurProfile=tempProfile;
            }

            if (hFile != INVALID_HANDLE_VALUE)
            {
                GetFileTime(hFile, NULL, NULL, &LastWriteTime);
                if (!memcmp( &CurProfile->LastWriteTime, &LastWriteTime, sizeof(FILETIME) ) &&
                    is_not_current(&LastWriteTime))
                    TRACE("(%s): already opened (mru=%d)\n",
                          debugstr_w(buffer), i);
                else
                {
                    TRACE("(%s): already opened, needs refreshing (mru=%d)\n",
                          debugstr_w(buffer), i);
                    PROFILE_Free(CurProfile->section);
                    CurProfile->section = PROFILE_Load(hFile, &CurProfile->encoding);
                    CurProfile->LastWriteTime = LastWriteTime;
                }
                CloseHandle(hFile);
                return TRUE;
            }
            else TRACE("(%s): already opened, not yet created (mru=%d)\n",
                       debugstr_w(buffer), i);
        }
    }

    /* Flush the old current profile */
    PROFILE_FlushFile();

    /* Make the oldest profile the current one only in order to get rid of it */
    if(i==N_CACHED_PROFILES)
      {
       tempProfile=MRUProfile[N_CACHED_PROFILES-1];
       for(i=N_CACHED_PROFILES-1;i>0;i--)
          MRUProfile[i]=MRUProfile[i-1];
       CurProfile=tempProfile;
      }
    if(CurProfile->filename) PROFILE_ReleaseFile();

    /* OK, now that CurProfile is definitely free we assign it our new file */
    CurProfile->filename  = HeapAlloc( GetProcessHeap(), 0, (lstrlenW(buffer)+1) * sizeof(WCHAR) );
    lstrcpyW( CurProfile->filename, buffer );

    if (hFile != INVALID_HANDLE_VALUE)
    {
        CurProfile->section = PROFILE_Load(hFile, &CurProfile->encoding);
        GetFileTime(hFile, NULL, NULL, &CurProfile->LastWriteTime);
        CloseHandle(hFile);
    }
    else
    {
        /* Does not exist yet, we will create it in PROFILE_FlushFile */
        WARN("profile file %s not found\n", debugstr_w(buffer) );
    }
    return TRUE;
}


/***********************************************************************
 *           PROFILE_GetSection
 *
 * Returns all keys of a section.
 * If return_values is TRUE, also include the corresponding values.
 */
static INT PROFILE_GetSection( const WCHAR *filename, LPCWSTR section_name,
			       LPWSTR buffer, UINT len, BOOL return_values )
{
    PROFILESECTION *section;
    PROFILEKEY *key;

    if(!buffer) return 0;

    TRACE("%s,%p,%u\n", debugstr_w(section_name), buffer, len);

    EnterCriticalSection( &PROFILE_CritSect );

    if (!PROFILE_Open( filename, FALSE ))
    {
        LeaveCriticalSection( &PROFILE_CritSect );
        buffer[0] = 0;
        return 0;
    }

    for (section = CurProfile->section; section; section = section->next)
    {
        if (!wcsicmp( section->name, section_name ))
        {
            UINT oldlen = len;
            for (key = section->key; key; key = key->next)
            {
                if (len <= 2) break;
                if (!*key->name && !key->value) continue;  /* Skip empty lines */
                if (IS_ENTRY_COMMENT(key->name)) continue;  /* Skip comments */
                if (!return_values && !key->value) continue;  /* Skip lines w.o. '=' */
                lstrcpynW( buffer, key->name, len - 1 );
                len -= lstrlenW(buffer) + 1;
                buffer += lstrlenW(buffer) + 1;
		if (len < 2)
		    break;
		if (return_values && key->value) {
			buffer[-1] = '=';
                    lstrcpynW( buffer, key->value, len - 1 );
			len -= lstrlenW(buffer) + 1;
			buffer += lstrlenW(buffer) + 1;
                }
            }
            *buffer = '\0';

            LeaveCriticalSection( &PROFILE_CritSect );

            if (len <= 1)
                /*If either lpszSection or lpszKey is NULL and the supplied
                  destination buffer is too small to hold all the strings,
                  the last string is truncated and followed by two null characters.
                  In this case, the return value is equal to cchReturnBuffer
                  minus two. */
            {
		buffer[-1] = '\0';
                return oldlen - 2;
            }
            return oldlen - len;
        }
    }
    buffer[0] = buffer[1] = '\0';

    LeaveCriticalSection( &PROFILE_CritSect );

    return 0;
}

static BOOL PROFILE_DeleteSection( const WCHAR *filename, const WCHAR *name )
{
    PROFILESECTION **section;

    EnterCriticalSection( &PROFILE_CritSect );

    if (!PROFILE_Open( filename, TRUE ))
    {
        LeaveCriticalSection( &PROFILE_CritSect );
        return FALSE;
    }

    for (section = &CurProfile->section; *section; section = &(*section)->next)
    {
        if (!wcsicmp( (*section)->name, name ))
        {
            PROFILESECTION *to_del = *section;
            *section = to_del->next;
            to_del->next = NULL;
            PROFILE_Free( to_del );
            CurProfile->changed = TRUE;
            PROFILE_FlushFile();
            break;
        }
    }

    LeaveCriticalSection( &PROFILE_CritSect );
    return TRUE;
}


/* See GetPrivateProfileSectionNamesA for documentation */
static INT PROFILE_GetSectionNames( LPWSTR buffer, UINT len )
{
    LPWSTR buf;
    UINT buflen,tmplen;
    PROFILESECTION *section;

    TRACE("(%p, %d)\n", buffer, len);

    if (!buffer || !len)
        return 0;
    if (len==1) {
        *buffer='\0';
        return 0;
    }

    buflen=len-1;
    buf=buffer;
    section = CurProfile->section;
    while ((section!=NULL)) {
        if (section->name[0]) {
            tmplen = lstrlenW(section->name)+1;
            if (tmplen >= buflen) {
                if (buflen > 0) {
                    memcpy(buf, section->name, (buflen-1) * sizeof(WCHAR));
                    buf += buflen-1;
                    *buf++='\0';
                }
                *buf='\0';
                return len-2;
            }
            memcpy(buf, section->name, tmplen * sizeof(WCHAR));
            buf += tmplen;
            buflen -= tmplen;
        }
        section = section->next;
    }
    *buf='\0';
    return buf-buffer;
}

/***********************************************************************
 *           PROFILE_SetString
 *
 * Set a profile string.
 */
static BOOL PROFILE_SetString( LPCWSTR section_name, LPCWSTR key_name,
                               LPCWSTR value, BOOL create_always )
{
    if (!value)  /* Delete a key */
    {
        TRACE("(%s,%s)\n", debugstr_w(section_name), debugstr_w(key_name) );
        CurProfile->changed |= PROFILE_DeleteKey( &CurProfile->section,
                                                  section_name, key_name );
        return TRUE;          /* same error handling as above */
    }
    else  /* Set the key value */
    {
        PROFILEKEY *key = PROFILE_Find(&CurProfile->section, section_name,
                                        key_name, TRUE, create_always );
        TRACE("(%s,%s,%s):\n",
              debugstr_w(section_name), debugstr_w(key_name), debugstr_w(value) );
        if (!key) return FALSE;

        /* strip the leading spaces. We can safely strip \n\r and
         * friends too, they should not happen here anyway. */
        while (PROFILE_isspaceW(*value)) value++;

        if (key->value)
        {
            if (!wcscmp( key->value, value ))
            {
                TRACE("  no change needed\n" );
                return TRUE;  /* No change needed */
            }
            TRACE("  replacing %s\n", debugstr_w(key->value) );
            HeapFree( GetProcessHeap(), 0, key->value );
        }
        else TRACE("  creating key\n" );
        key->value = HeapAlloc( GetProcessHeap(), 0, (lstrlenW(value)+1) * sizeof(WCHAR) );
        lstrcpyW( key->value, value );
        CurProfile->changed = TRUE;
    }
    return TRUE;
}

static HKEY open_file_mapping_key( const WCHAR *filename )
{
    static HKEY mapping_key;
    HKEY key;

    EnterCriticalSection( &PROFILE_CritSect );

    if (!mapping_key && RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                                       L"Software\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping",
                                       0, KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY, &mapping_key ))
        mapping_key = NULL;

    LeaveCriticalSection( &PROFILE_CritSect );

    if (mapping_key && !RegOpenKeyExW( mapping_key, PathFindFileNameW( filename ), 0, KEY_READ, &key ))
        return key;
    return NULL;
}

static WCHAR *enum_key( HKEY key, DWORD i )
{
    WCHAR *value, *new_value;
    DWORD max = 256, len;
    LSTATUS res;

    if (!(value = HeapAlloc( GetProcessHeap(), 0, max * sizeof(WCHAR) ))) return NULL;
    len = max;
    while ((res = RegEnumValueW( key, i, value, &len, NULL, NULL, NULL, NULL )) == ERROR_MORE_DATA)
    {
        max *= 2;
        if (!(new_value = HeapReAlloc( GetProcessHeap(), 0, value, max * sizeof(WCHAR) )))
        {
            HeapFree( GetProcessHeap(), 0, value );
            return NULL;
        }
        value = new_value;
        len = max;
    }
    if (!res) return value;
    HeapFree( GetProcessHeap(), 0, value );
    return NULL;
}

static WCHAR *get_key_value( HKEY key, const WCHAR *value )
{
    DWORD size = 0;
    WCHAR *data;

    if (RegGetValueW( key, NULL, value, RRF_RT_REG_SZ | RRF_NOEXPAND, NULL, NULL, &size )) return NULL;
    if (!(data = HeapAlloc( GetProcessHeap(), 0, size ))) return NULL;
    if (!RegGetValueW( key, NULL, value, RRF_RT_REG_SZ | RRF_NOEXPAND, NULL, (BYTE *)data, &size )) return data;
    HeapFree( GetProcessHeap(), 0, data );
    return NULL;
}

static HKEY open_mapped_key( const WCHAR *path, BOOL write )
{
    static const WCHAR usrW[] = {'U','S','R',':'};
    static const WCHAR sysW[] = {'S','Y','S',':'};
    WCHAR *combined_path;
    const WCHAR *p;
    LSTATUS res;
    HKEY key;

    TRACE("%s\n", debugstr_w( path ));

    for (p = path; strchr("!#@", *p); p++)
        FIXME("ignoring %c modifier\n", *p);

    if (!wcsncmp( p, usrW, ARRAY_SIZE( usrW ) ))
    {
        if (write)
            res = RegCreateKeyExW( HKEY_CURRENT_USER, p + 4, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &key, NULL );
        else
            res = RegOpenKeyExW( HKEY_CURRENT_USER, p + 4, 0, KEY_READ, &key );
        return res ? NULL : key;
    }

    if (!wcsncmp( p, sysW, ARRAY_SIZE( sysW ) ))
    {
        p += 4;
        if (!(combined_path = HeapAlloc( GetProcessHeap(), 0,
                                         (ARRAY_SIZE( L"Software\\" ) + lstrlenW( p )) * sizeof(WCHAR) )))
            return NULL;
        lstrcpyW( combined_path, L"Software\\" );
        lstrcatW( combined_path, p );
        if (write)
            res = RegCreateKeyExW( HKEY_LOCAL_MACHINE, combined_path, 0, NULL,
                                   0, KEY_READ | KEY_WRITE, NULL, &key, NULL );
        else
            res = RegOpenKeyExW( HKEY_LOCAL_MACHINE, combined_path, 0, KEY_READ, &key );
        HeapFree( GetProcessHeap(), 0, combined_path );
        return res ? NULL : key;
    }

    FIXME("unhandled path syntax %s\n", debugstr_w( path ));
    return NULL;
}

/* returns TRUE if the given section + name is mapped */
static BOOL get_mapped_section_key( const WCHAR *filename, const WCHAR *section,
                                    const WCHAR *name, BOOL write, HKEY *ret_key )
{
    WCHAR *path = NULL, *combined_path;
    HKEY key, subkey = NULL;

    if (!(key = open_file_mapping_key( filename )))
        return FALSE;

    if (!RegOpenKeyExW( key, section, 0, KEY_READ, &subkey ))
    {
        if (!(path = get_key_value( subkey, name )))
            path = get_key_value( subkey, NULL );
        RegCloseKey( subkey );
        RegCloseKey( key );
        if (!path) return FALSE;
    }
    else
    {
        if (!(path = get_key_value( key, section )))
        {
            if ((path = get_key_value( key, NULL )))
            {
                if ((combined_path = HeapAlloc( GetProcessHeap(), 0,
                                                (lstrlenW( path ) + lstrlenW( section ) + 2) * sizeof(WCHAR) )))
                {
                    lstrcpyW( combined_path, path );
                    lstrcatW( combined_path, L"\\" );
                    lstrcatW( combined_path, section );
                }
                HeapFree( GetProcessHeap(), 0, path );
                path = combined_path;
            }
        }
        RegCloseKey( key );
        if (!path) return FALSE;
    }

    *ret_key = open_mapped_key( path, write );
    HeapFree( GetProcessHeap(), 0, path );
    return TRUE;
}

static DWORD get_mapped_section( HKEY key, WCHAR *buffer, DWORD size, BOOL return_values )
{
    WCHAR *entry, *value;
    DWORD i, ret = 0;

    for (i = 0; (entry = enum_key( key, i )); ++i)
    {
        lstrcpynW( buffer + ret, entry, size - ret - 1 );
        ret = min( ret + lstrlenW( entry ) + 1, size - 1 );
        if (return_values && ret < size - 1 && (value = get_key_value( key, entry )))
        {
            buffer[ret - 1] = '=';
            lstrcpynW( buffer + ret, value, size - ret - 1 );
            ret = min( ret + lstrlenW( value ) + 1, size - 1 );
            HeapFree( GetProcessHeap(), 0, value );
        }
        HeapFree( GetProcessHeap(), 0, entry );
    }

    return ret;
}

static DWORD get_section( const WCHAR *filename, const WCHAR *section,
                          WCHAR *buffer, DWORD size, BOOL return_values )
{
    HKEY key, subkey, section_key;
    BOOL use_ini = TRUE;
    DWORD ret = 0;
    WCHAR *path;

    if ((key = open_file_mapping_key( filename )))
    {
        if (!RegOpenKeyExW( key, section, 0, KEY_READ, &subkey ))
        {
            WCHAR *entry, *value;
            HKEY entry_key;
            DWORD i;

            for (i = 0; (entry = enum_key( subkey, i )); ++i)
            {
                if (!(path = get_key_value( subkey, entry )))
                {
                    HeapFree( GetProcessHeap(), 0, entry );
                    continue;
                }

                entry_key = open_mapped_key( path, FALSE );
                HeapFree( GetProcessHeap(), 0, path );
                if (!entry_key)
                {
                    HeapFree( GetProcessHeap(), 0, entry );
                    continue;
                }

                if (entry[0])
                {
                    if ((value = get_key_value( entry_key, entry )))
                    {
                        lstrcpynW( buffer + ret, entry, size - ret - 1 );
                        ret = min( ret + lstrlenW( entry ) + 1, size - 1 );
                        if (return_values && ret < size - 1)
                        {
                            buffer[ret - 1] = '=';
                            lstrcpynW( buffer + ret, value, size - ret - 1 );
                            ret = min( ret + lstrlenW( value ) + 1, size - 1 );
                        }
                        HeapFree( GetProcessHeap(), 0, value );
                    }
                }
                else
                {
                    ret = get_mapped_section( entry_key, buffer, size, return_values );
                    use_ini = FALSE;
                }

                HeapFree( GetProcessHeap(), 0, entry );
                RegCloseKey( entry_key );
            }

            RegCloseKey( subkey );
        }
        else if (get_mapped_section_key( filename, section, NULL, FALSE, &section_key ))
        {
            ret = get_mapped_section( section_key, buffer, size, return_values );
            use_ini = FALSE;
            RegCloseKey( section_key );
        }

        RegCloseKey( key );
    }

    if (use_ini)
        ret += PROFILE_GetSection( filename, section, buffer + ret, size - ret, return_values );

    return ret;
}

static void delete_key_values( HKEY key )
{
    WCHAR *entry;

    while ((entry = enum_key( key, 0 )))
    {
        RegDeleteValueW( key, entry );
        HeapFree( GetProcessHeap(), 0, entry );
    }
}

static BOOL delete_section( const WCHAR *filename, const WCHAR *section )
{
    HKEY key, subkey, section_key;

    if ((key = open_file_mapping_key( filename )))
    {
        if (!RegOpenKeyExW( key, section, 0, KEY_READ, &subkey ))
        {
            WCHAR *entry, *path;
            HKEY entry_key;
            DWORD i;

            for (i = 0; (entry = enum_key( subkey, i )); ++i)
            {
                if (!(path = get_key_value( subkey, entry )))
                {
                    HeapFree( GetProcessHeap(), 0, entry );
                    continue;
                }

                entry_key = open_mapped_key( path, TRUE );
                HeapFree( GetProcessHeap(), 0, path );
                if (!entry_key)
                {
                    HeapFree( GetProcessHeap(), 0, entry );
                    continue;
                }

                if (entry[0])
                    RegDeleteValueW( entry_key, entry );
                else
                    delete_key_values( entry_key );

                HeapFree( GetProcessHeap(), 0, entry );
                RegCloseKey( entry_key );
            }

            RegCloseKey( subkey );
        }
        else if (get_mapped_section_key( filename, section, NULL, TRUE, &section_key ))
        {
            delete_key_values( section_key );
            RegCloseKey( section_key );
        }

        RegCloseKey( key );
    }

    return PROFILE_DeleteSection( filename, section );
}

/********************* API functions **********************************/


/***********************************************************************
 *           GetProfileIntA   (KERNEL32.@)
 */
UINT WINAPI GetProfileIntA( LPCSTR section, LPCSTR entry, INT def_val )
{
    return GetPrivateProfileIntA( section, entry, def_val, "win.ini" );
}

/***********************************************************************
 *           GetProfileIntW   (KERNEL32.@)
 */
UINT WINAPI GetProfileIntW( LPCWSTR section, LPCWSTR entry, INT def_val )
{
    return GetPrivateProfileIntW( section, entry, def_val, L"win.ini" );
}

/***********************************************************************
 *           GetPrivateProfileStringW   (KERNEL32.@)
 */
INT WINAPI GetPrivateProfileStringW( LPCWSTR section, LPCWSTR entry,
				     LPCWSTR def_val, LPWSTR buffer,
				     UINT len, LPCWSTR filename )
{
    int		ret;
    LPWSTR	defval_tmp = NULL;
    const WCHAR *p;
    HKEY key;

    TRACE("%s,%s,%s,%p,%u,%s\n", debugstr_w(section), debugstr_w(entry),
          debugstr_w(def_val), buffer, len, debugstr_w(filename));

    if (!buffer || !len) return 0;
    if (!def_val) def_val = L"";
    if (!section) return GetPrivateProfileSectionNamesW( buffer, len, filename );
    if (!entry)
    {
        ret = get_section( filename, section, buffer, len, FALSE );
        if (!buffer[0])
        {
            PROFILE_CopyEntry( buffer, def_val, len );
            ret = lstrlenW( buffer );
        }
        return ret;
    }

    /* strip any trailing ' ' of def_val. */
    p = def_val + lstrlenW(def_val) - 1;

    while (p > def_val && *p == ' ') p--;

    if (p >= def_val)
    {
        int vlen = (int)(p - def_val) + 1;

        defval_tmp = HeapAlloc(GetProcessHeap(), 0, (vlen + 1) * sizeof(WCHAR));
        memcpy(defval_tmp, def_val, vlen * sizeof(WCHAR));
        defval_tmp[vlen] = '\0';
        def_val = defval_tmp;
    }

    if (get_mapped_section_key( filename, section, entry, FALSE, &key ))
    {
        if (key)
        {
            WCHAR *value;

            if ((value = get_key_value( key, entry )))
            {
                lstrcpynW( buffer, value, len );
                HeapFree( GetProcessHeap(), 0, value );
            }
            else
                lstrcpynW( buffer, def_val, len );

            RegCloseKey( key );
        }
        else
            lstrcpynW( buffer, def_val, len );

        ret = lstrlenW( buffer );
    }
    else
    {
        EnterCriticalSection( &PROFILE_CritSect );

        if (PROFILE_Open( filename, FALSE ))
        {
            PROFILEKEY *key = PROFILE_Find( &CurProfile->section, section, entry, FALSE, FALSE );
            PROFILE_CopyEntry( buffer, (key && key->value) ? key->value : def_val, len );
            TRACE("-> %s\n", debugstr_w( buffer ));
            ret = lstrlenW( buffer );
        }
        else
        {
           lstrcpynW( buffer, def_val, len );
           ret = lstrlenW( buffer );
        }

        LeaveCriticalSection( &PROFILE_CritSect );
    }

    HeapFree(GetProcessHeap(), 0, defval_tmp);

    TRACE("returning %s, %d\n", debugstr_w(buffer), ret);

    return ret;
}

/***********************************************************************
 *           GetPrivateProfileStringA   (KERNEL32.@)
 */
INT WINAPI GetPrivateProfileStringA( LPCSTR section, LPCSTR entry,
				     LPCSTR def_val, LPSTR buffer,
				     UINT len, LPCSTR filename )
{
    UNICODE_STRING sectionW, entryW, def_valW, filenameW;
    LPWSTR bufferW;
    INT retW, ret = 0;

    bufferW = buffer ? HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR)) : NULL;
    if (section) RtlCreateUnicodeStringFromAsciiz(&sectionW, section);
    else sectionW.Buffer = NULL;
    if (entry) RtlCreateUnicodeStringFromAsciiz(&entryW, entry);
    else entryW.Buffer = NULL;
    if (def_val) RtlCreateUnicodeStringFromAsciiz(&def_valW, def_val);
    else def_valW.Buffer = NULL;
    if (filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;

    retW = GetPrivateProfileStringW( sectionW.Buffer, entryW.Buffer,
                                     def_valW.Buffer, bufferW, len,
                                     filenameW.Buffer);
    if (len && buffer)
    {
        if (retW)
        {
            ret = WideCharToMultiByte(CP_ACP, 0, bufferW, retW, buffer, len - 1, NULL, NULL);
            if (!ret)
                ret = len - 1;
        }
        buffer[ret] = 0;
    }

    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&entryW);
    RtlFreeUnicodeString(&def_valW);
    RtlFreeUnicodeString(&filenameW);
    HeapFree(GetProcessHeap(), 0, bufferW);
    return ret;
}

/***********************************************************************
 *           GetProfileStringA   (KERNEL32.@)
 */
INT WINAPI GetProfileStringA( LPCSTR section, LPCSTR entry, LPCSTR def_val,
			      LPSTR buffer, UINT len )
{
    return GetPrivateProfileStringA( section, entry, def_val,
                                     buffer, len, "win.ini" );
}

/***********************************************************************
 *           GetProfileStringW   (KERNEL32.@)
 */
INT WINAPI GetProfileStringW( LPCWSTR section, LPCWSTR entry,
			      LPCWSTR def_val, LPWSTR buffer, UINT len )
{
    return GetPrivateProfileStringW( section, entry, def_val, buffer, len, L"win.ini" );
}

/***********************************************************************
 *           WriteProfileStringA   (KERNEL32.@)
 */
BOOL WINAPI WriteProfileStringA( LPCSTR section, LPCSTR entry,
				 LPCSTR string )
{
    return WritePrivateProfileStringA( section, entry, string, "win.ini" );
}

/***********************************************************************
 *           WriteProfileStringW   (KERNEL32.@)
 */
BOOL WINAPI WriteProfileStringW( LPCWSTR section, LPCWSTR entry,
                                     LPCWSTR string )
{
    return WritePrivateProfileStringW( section, entry, string, L"win.ini" );
}


/***********************************************************************
 *           GetPrivateProfileIntW   (KERNEL32.@)
 */
UINT WINAPI GetPrivateProfileIntW( LPCWSTR section, LPCWSTR entry,
                                   INT def_val, LPCWSTR filename )
{
    WCHAR buffer[30];
    UNICODE_STRING bufferW;
    ULONG result;

    if (GetPrivateProfileStringW( section, entry, L"", buffer, ARRAY_SIZE( buffer ),
                                  filename ) == 0)
        return def_val;

    /* FIXME: if entry can be found but it's empty, then Win16 is
     * supposed to return 0 instead of def_val ! Difficult/problematic
     * to implement (every other failure also returns zero buffer),
     * thus wait until testing framework avail for making sure nothing
     * else gets broken that way. */
    if (!buffer[0]) return (UINT)def_val;

    RtlInitUnicodeString( &bufferW, buffer );
    RtlUnicodeStringToInteger( &bufferW, 0, &result);
    return result;
}

/***********************************************************************
 *           GetPrivateProfileIntA   (KERNEL32.@)
 */
UINT WINAPI GetPrivateProfileIntA( LPCSTR section, LPCSTR entry,
				   INT def_val, LPCSTR filename )
{
    UNICODE_STRING entryW, filenameW, sectionW;
    UINT res;
    if(entry) RtlCreateUnicodeStringFromAsciiz(&entryW, entry);
    else entryW.Buffer = NULL;
    if(filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;
    if(section) RtlCreateUnicodeStringFromAsciiz(&sectionW, section);
    else sectionW.Buffer = NULL;
    res = GetPrivateProfileIntW(sectionW.Buffer, entryW.Buffer, def_val,
                                filenameW.Buffer);
    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&filenameW);
    RtlFreeUnicodeString(&entryW);
    return res;
}

/***********************************************************************
 *           GetPrivateProfileSectionW   (KERNEL32.@)
 */
INT WINAPI GetPrivateProfileSectionW( LPCWSTR section, LPWSTR buffer,
				      DWORD len, LPCWSTR filename )
{
    if (!section || !buffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    TRACE("(%s, %p, %ld, %s)\n", debugstr_w(section), buffer, len, debugstr_w(filename));

    return get_section( filename, section, buffer, len, TRUE );
}

/***********************************************************************
 *           GetPrivateProfileSectionA   (KERNEL32.@)
 */
INT WINAPI GetPrivateProfileSectionA( LPCSTR section, LPSTR buffer,
                                      DWORD len, LPCSTR filename )
{
    UNICODE_STRING sectionW, filenameW;
    LPWSTR bufferW;
    INT retW, ret = 0;

    if (!section || !buffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    bufferW = HeapAlloc(GetProcessHeap(), 0, len * 2 * sizeof(WCHAR));
    RtlCreateUnicodeStringFromAsciiz(&sectionW, section);
    if (filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;

    retW = GetPrivateProfileSectionW(sectionW.Buffer, bufferW, len * 2, filenameW.Buffer);
    if (retW)
    {
        if (retW == len * 2 - 2) retW++;  /* overflow */
        ret = WideCharToMultiByte(CP_ACP, 0, bufferW, retW + 1, buffer, len, NULL, NULL);
        if (!ret || ret == len)  /* overflow */
        {
            ret = len - 2;
            buffer[len-2] = 0;
            buffer[len-1] = 0;
        }
        else ret--;
    }
    else
    {
        buffer[0] = 0;
        buffer[1] = 0;
    }

    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&filenameW);
    HeapFree(GetProcessHeap(), 0, bufferW);
    return ret;
}

/***********************************************************************
 *           GetProfileSectionA   (KERNEL32.@)
 */
INT WINAPI GetProfileSectionA( LPCSTR section, LPSTR buffer, DWORD len )
{
    return GetPrivateProfileSectionA( section, buffer, len, "win.ini" );
}

/***********************************************************************
 *           GetProfileSectionW   (KERNEL32.@)
 */
INT WINAPI GetProfileSectionW( LPCWSTR section, LPWSTR buffer, DWORD len )
{
    return GetPrivateProfileSectionW( section, buffer, len, L"win.ini" );
}


/***********************************************************************
 *           WritePrivateProfileStringW   (KERNEL32.@)
 */
BOOL WINAPI WritePrivateProfileStringW( LPCWSTR section, LPCWSTR entry,
					LPCWSTR string, LPCWSTR filename )
{
    BOOL ret = FALSE;
    HKEY key;

    TRACE("(%s, %s, %s, %s)\n", debugstr_w(section), debugstr_w(entry), debugstr_w(string), debugstr_w(filename));

    if (!section && !entry && !string) /* documented "file flush" case */
    {
        EnterCriticalSection( &PROFILE_CritSect );
        if (!filename || PROFILE_Open( filename, TRUE ))
        {
            if (CurProfile) PROFILE_ReleaseFile();
        }
        LeaveCriticalSection( &PROFILE_CritSect );
        return FALSE;
    }
    if (!entry) return delete_section( filename, section );

    if (get_mapped_section_key( filename, section, entry, TRUE, &key ))
    {
        LSTATUS res;

        if (string)
            res = RegSetValueExW( key, entry, 0, REG_SZ, (const BYTE *)string,
                                  (lstrlenW( string ) + 1) * sizeof(WCHAR) );
        else
            res = RegDeleteValueW( key, entry );
        RegCloseKey( key );
        if (res) SetLastError( res );
        return !res;
    }

    EnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename, TRUE ))
    {
        if (!section)
            SetLastError(ERROR_FILE_NOT_FOUND);
        else
            ret = PROFILE_SetString( section, entry, string, FALSE);
        if (ret) ret = PROFILE_FlushFile();
    }

    LeaveCriticalSection( &PROFILE_CritSect );
    return ret;
}

/***********************************************************************
 *           WritePrivateProfileStringA   (KERNEL32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH WritePrivateProfileStringA( LPCSTR section, LPCSTR entry,
                                                          LPCSTR string, LPCSTR filename )
{
    UNICODE_STRING sectionW, entryW, stringW, filenameW;
    BOOL ret;

    if (section) RtlCreateUnicodeStringFromAsciiz(&sectionW, section);
    else sectionW.Buffer = NULL;
    if (entry) RtlCreateUnicodeStringFromAsciiz(&entryW, entry);
    else entryW.Buffer = NULL;
    if (string) RtlCreateUnicodeStringFromAsciiz(&stringW, string);
    else stringW.Buffer = NULL;
    if (filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;

    ret = WritePrivateProfileStringW(sectionW.Buffer, entryW.Buffer,
                                     stringW.Buffer, filenameW.Buffer);
    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&entryW);
    RtlFreeUnicodeString(&stringW);
    RtlFreeUnicodeString(&filenameW);
    return ret;
}

/***********************************************************************
 *           WritePrivateProfileSectionW   (KERNEL32.@)
 */
BOOL WINAPI WritePrivateProfileSectionW( LPCWSTR section,
                                         LPCWSTR string, LPCWSTR filename )
{
    BOOL ret = FALSE;
    LPWSTR p;
    HKEY key, section_key;

    if (!section && !string)
    {
        EnterCriticalSection( &PROFILE_CritSect );
        if (!filename || PROFILE_Open( filename, TRUE ))
        {
            if (CurProfile) PROFILE_ReleaseFile();
        }
        LeaveCriticalSection( &PROFILE_CritSect );
        return FALSE;
    }
    if (!string) return delete_section( filename, section );

    if ((key = open_file_mapping_key( filename )))
    {
        /* replace existing entries, but only if they are mapped, and do not
         * delete any keys */

        const WCHAR *entry, *p;

        for (entry = string; *entry; entry += lstrlenW( entry ) + 1)
        {
            if ((p = wcschr( entry, '=' )))
            {
                WCHAR *entry_copy;
                p++;
                if (!(entry_copy = HeapAlloc( GetProcessHeap(), 0, (p - entry) * sizeof(WCHAR) )))
                {
                    SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                    RegCloseKey( key );
                    return FALSE;
                }
                lstrcpynW( entry_copy, entry, p - entry );
                if (get_mapped_section_key( filename, section, entry_copy, TRUE, &section_key ))
                {
                    LSTATUS res = RegSetValueExW( section_key, entry_copy, 0, REG_SZ, (const BYTE *)p,
                                                  (lstrlenW( p ) + 1) * sizeof(WCHAR) );
                    RegCloseKey( section_key );
                    if (res)
                    {
                        HeapFree( GetProcessHeap(), 0, entry_copy );
                        SetLastError( res );
                        RegCloseKey( key );
                        return FALSE;
                    }
                }
                HeapFree( GetProcessHeap(), 0, entry_copy );
            }
        }
        RegCloseKey( key );
        return TRUE;
    }

    EnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename, TRUE ))
    {
        PROFILE_DeleteAllKeys(section);
        ret = TRUE;
        while (*string && ret)
        {
            WCHAR *buf = HeapAlloc( GetProcessHeap(), 0, (lstrlenW( string ) + 1) * sizeof(WCHAR) );
            lstrcpyW( buf, string );
            if ((p = wcschr( buf, '=')))
            {
                *p = '\0';
                ret = PROFILE_SetString( section, buf, p+1, TRUE );
            }
            HeapFree( GetProcessHeap(), 0, buf );
            string += lstrlenW( string ) + 1;
        }
        if (ret) ret = PROFILE_FlushFile();
    }

    LeaveCriticalSection( &PROFILE_CritSect );
    return ret;
}

/***********************************************************************
 *           WritePrivateProfileSectionA   (KERNEL32.@)
 */
BOOL WINAPI WritePrivateProfileSectionA( LPCSTR section,
                                         LPCSTR string, LPCSTR filename)

{
    UNICODE_STRING sectionW, filenameW;
    LPWSTR stringW;
    BOOL ret;

    if (string)
    {
        INT lenA, lenW;
        LPCSTR p = string;

        while(*p) p += strlen(p) + 1;
        lenA = p - string + 1;
        lenW = MultiByteToWideChar(CP_ACP, 0, string, lenA, NULL, 0);
        if ((stringW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR))))
            MultiByteToWideChar(CP_ACP, 0, string, lenA, stringW, lenW);
    }
    else stringW = NULL;
    if (section) RtlCreateUnicodeStringFromAsciiz(&sectionW, section);
    else sectionW.Buffer = NULL;
    if (filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;

    ret = WritePrivateProfileSectionW(sectionW.Buffer, stringW, filenameW.Buffer);

    HeapFree(GetProcessHeap(), 0, stringW);
    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&filenameW);
    return ret;
}

/***********************************************************************
 *           WriteProfileSectionA   (KERNEL32.@)
 */
BOOL WINAPI WriteProfileSectionA( LPCSTR section, LPCSTR keys_n_values)

{
    return WritePrivateProfileSectionA( section, keys_n_values, "win.ini");
}

/***********************************************************************
 *           WriteProfileSectionW   (KERNEL32.@)
 */
BOOL WINAPI WriteProfileSectionW( LPCWSTR section, LPCWSTR keys_n_values)
{
   return WritePrivateProfileSectionW(section, keys_n_values, L"win.ini");
}


/***********************************************************************
 *           GetPrivateProfileSectionNamesW  (KERNEL32.@)
 *
 * Returns the section names contained in the specified file.
 * FIXME: Where do we find this file when the path is relative?
 * The section names are returned as a list of strings with an extra
 * '\0' to mark the end of the list. Except for that the behavior
 * depends on the Windows version.
 *
 * Win95:
 * - if the buffer is 0 or 1 character long then it is as if it was of
 *   infinite length.
 * - otherwise, if the buffer is too small only the section names that fit
 *   are returned.
 * - note that this means if the buffer was too small to return even just
 *   the first section name then a single '\0' will be returned.
 * - the return value is the number of characters written in the buffer,
 *   except if the buffer was too small in which case len-2 is returned
 *
 * Win2000:
 * - if the buffer is 0, 1 or 2 characters long then it is filled with
 *   '\0' and the return value is 0
 * - otherwise if the buffer is too small then the first section name that
 *   does not fit is truncated so that the string list can be terminated
 *   correctly (double '\0')
 * - the return value is the number of characters written in the buffer
 *   except for the trailing '\0'. If the buffer is too small, then the
 *   return value is len-2
 * - Win2000 has a bug that triggers when the section names and the
 *   trailing '\0' fit exactly in the buffer. In that case the trailing
 *   '\0' is missing.
 *
 * Wine implements the observed Win2000 behavior (except for the bug).
 *
 * Note that when the buffer is big enough then the return value may be any
 * value between 1 and len-1 (or len in Win95), including len-2.
 */
DWORD WINAPI GetPrivateProfileSectionNamesW( LPWSTR buffer, DWORD size,
					     LPCWSTR filename)
{
    DWORD ret = 0;
    HKEY key;

    if ((key = open_file_mapping_key( filename )))
    {
        WCHAR *section;
        DWORD i;

        for (i = 0; (section = enum_key( key, i )); ++i)
        {
            lstrcpynW( buffer + ret, section, size - ret - 1 );
            ret = min( ret + lstrlenW( section ) + 1, size - 1 );
            HeapFree( GetProcessHeap(), 0, section );
        }

        RegCloseKey( key );
    }

    RtlEnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename, FALSE ))
        ret += PROFILE_GetSectionNames( buffer + ret, size - ret );

    RtlLeaveCriticalSection( &PROFILE_CritSect );

    return ret;
}


/***********************************************************************
 *           GetPrivateProfileSectionNamesA  (KERNEL32.@)
 */
DWORD WINAPI GetPrivateProfileSectionNamesA( LPSTR buffer, DWORD size,
					     LPCSTR filename)
{
    UNICODE_STRING filenameW;
    LPWSTR bufferW;
    INT retW, ret = 0;

    bufferW = buffer ? HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR)) : NULL;
    if (filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;

    retW = GetPrivateProfileSectionNamesW(bufferW, size, filenameW.Buffer);
    if (retW && size)
    {
        ret = WideCharToMultiByte(CP_ACP, 0, bufferW, retW+1, buffer, size-1, NULL, NULL);
        if (!ret)
        {
            ret = size-2;
            buffer[size-1] = 0;
        }
        else
          ret = ret-1;
    }
    else if(size)
        buffer[0] = '\0';

    RtlFreeUnicodeString(&filenameW);
    HeapFree(GetProcessHeap(), 0, bufferW);
    return ret;
}

static int get_hex_byte( const WCHAR *p )
{
    int val;

    if (*p >= '0' && *p <= '9') val = *p - '0';
    else if (*p >= 'A' && *p <= 'Z') val = *p - 'A' + 10;
    else if (*p >= 'a' && *p <= 'z') val = *p - 'a' + 10;
    else return -1;
    val <<= 4;
    p++;
    if (*p >= '0' && *p <= '9') val += *p - '0';
    else if (*p >= 'A' && *p <= 'Z') val += *p - 'A' + 10;
    else if (*p >= 'a' && *p <= 'z') val += *p - 'a' + 10;
    else return -1;
    return val;
}

/***********************************************************************
 *           GetPrivateProfileStructW (KERNEL32.@)
 *
 * Should match Win95's behaviour pretty much
 */
BOOL WINAPI GetPrivateProfileStructW (LPCWSTR section, LPCWSTR key,
                                      LPVOID buf, UINT len, LPCWSTR filename)
{
    BOOL ret = FALSE;
    LPBYTE data = buf;
    BYTE chksum = 0;
    int val;
    WCHAR *p, *buffer;

    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, (2 * len + 3) * sizeof(WCHAR) ))) return FALSE;

    if (GetPrivateProfileStringW( section, key, NULL, buffer, 2 * len + 3, filename ) != 2 * len + 2)
        goto done;

    for (p = buffer; len; p += 2, len--)
    {
        if ((val = get_hex_byte( p )) == -1) goto done;
        *data++ = val;
        chksum += val;
    }
    /* retrieve stored checksum value */
    if ((val = get_hex_byte( p )) == -1) goto done;
    ret = ((BYTE)val == chksum);

done:
    HeapFree( GetProcessHeap(), 0, buffer );
    return ret;
}

/***********************************************************************
 *           GetPrivateProfileStructA (KERNEL32.@)
 */
BOOL WINAPI GetPrivateProfileStructA (LPCSTR section, LPCSTR key,
				      LPVOID buffer, UINT len, LPCSTR filename)
{
    UNICODE_STRING sectionW, keyW, filenameW;
    INT ret;

    if (section) RtlCreateUnicodeStringFromAsciiz(&sectionW, section);
    else sectionW.Buffer = NULL;
    if (key) RtlCreateUnicodeStringFromAsciiz(&keyW, key);
    else keyW.Buffer = NULL;
    if (filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;

    ret = GetPrivateProfileStructW(sectionW.Buffer, keyW.Buffer, buffer, len,
                                   filenameW.Buffer);
    /* Do not translate binary data. */

    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&keyW);
    RtlFreeUnicodeString(&filenameW);
    return ret;
}



/***********************************************************************
 *           WritePrivateProfileStructW (KERNEL32.@)
 */
BOOL WINAPI WritePrivateProfileStructW (LPCWSTR section, LPCWSTR key,
                                        LPVOID buf, UINT bufsize, LPCWSTR filename)
{
    BOOL ret = FALSE;
    LPBYTE binbuf;
    LPWSTR outstring, p;
    DWORD sum = 0;

    TRACE("(%s %s %p %u %s)\n", debugstr_w(section), debugstr_w(key), buf, bufsize, debugstr_w(filename));

    if (!section && !key && !buf)  /* flush the cache */
        return WritePrivateProfileStringW( NULL, NULL, NULL, filename );

    if (!buf)
        return WritePrivateProfileStringW(section, key, NULL, filename);

    /* allocate string buffer for hex chars + checksum hex char + '\0' */
    outstring = HeapAlloc( GetProcessHeap(), 0, (bufsize*2 + 2 + 1) * sizeof(WCHAR) );
    p = outstring;
    for (binbuf = (LPBYTE)buf; binbuf < (LPBYTE)buf+bufsize; binbuf++) {
      *p++ = hex[*binbuf >> 4];
      *p++ = hex[*binbuf & 0xf];
      sum += *binbuf;
    }
    /* checksum is sum & 0xff */
    *p++ = hex[(sum & 0xf0) >> 4];
    *p++ = hex[sum & 0xf];
    *p++ = '\0';

    ret = WritePrivateProfileStringW( section, key, outstring, filename );
    HeapFree( GetProcessHeap(), 0, outstring );
    return ret;
}

/***********************************************************************
 *           WritePrivateProfileStructA (KERNEL32.@)
 */
BOOL WINAPI WritePrivateProfileStructA (LPCSTR section, LPCSTR key,
					LPVOID buf, UINT bufsize, LPCSTR filename)
{
    UNICODE_STRING sectionW, keyW, filenameW;
    INT ret;

    if (section) RtlCreateUnicodeStringFromAsciiz(&sectionW, section);
    else sectionW.Buffer = NULL;
    if (key) RtlCreateUnicodeStringFromAsciiz(&keyW, key);
    else keyW.Buffer = NULL;
    if (filename) RtlCreateUnicodeStringFromAsciiz(&filenameW, filename);
    else filenameW.Buffer = NULL;

    /* Do not translate binary data. */
    ret = WritePrivateProfileStructW(sectionW.Buffer, keyW.Buffer, buf, bufsize,
                                     filenameW.Buffer);

    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&keyW);
    RtlFreeUnicodeString(&filenameW);
    return ret;
}


/***********************************************************************
 *           OpenProfileUserMapping   (KERNEL32.@)
 */
BOOL WINAPI OpenProfileUserMapping(void) {
    FIXME("(), stub!\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           CloseProfileUserMapping   (KERNEL32.@)
 */
BOOL WINAPI CloseProfileUserMapping(void) {
    FIXME("(), stub!\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
