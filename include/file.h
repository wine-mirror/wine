/*
 * File handling declarations
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_FILE_H
#define __WINE_FILE_H

#include <time.h> /* time_t */
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <sys/types.h>
#include "winbase.h"
#include "wine/windef16.h"  /* HFILE16 */

#define MAX_PATHNAME_LEN   1024

/* Definition of a full DOS file name */
typedef struct
{
    char  long_name[MAX_PATHNAME_LEN];  /* Long pathname in Unix format */
    WCHAR short_name[MAX_PATHNAME_LEN]; /* Short pathname in DOS 8.3 format */
    int   drive;
} DOS_FULL_NAME;

#define IS_END_OF_NAME(ch)  (!(ch) || ((ch) == '/') || ((ch) == '\\'))

/* DOS device descriptor */
typedef struct
{
    const WCHAR name[9];
    int flags;
    UINT codepage;
} DOS_DEVICE;

/* locale-independent case conversion */
inline static char FILE_tolower( char c )
{
    if (c >= 'A' && c <= 'Z') c += 32;
    return c;
}
inline static char FILE_toupper( char c )
{
    if (c >= 'a' && c <= 'z') c -= 32;
    return c;
}

inline static int FILE_contains_path (LPCSTR name)
{
    return ((*name && (name[1] == ':')) ||
            strchr (name, '/') || strchr (name, '\\'));
}

/* files/file.c */
extern mode_t FILE_umask;
extern int FILE_strcasecmp( const char *str1, const char *str2 );
extern int FILE_strncasecmp( const char *str1, const char *str2, int len );
extern void FILE_SetDosError(void);
extern int FILE_GetUnixHandle( HANDLE handle, DWORD access );
extern BOOL FILE_Stat( LPCSTR unixName, BY_HANDLE_FILE_INFORMATION *info, BOOL *is_symlink );
extern HFILE16 FILE_Dup2( HFILE16 hFile1, HFILE16 hFile2 );
extern HANDLE FILE_CreateFile( LPCSTR filename, DWORD access, DWORD sharing,
                               LPSECURITY_ATTRIBUTES sa, DWORD creation,
                               DWORD attributes, HANDLE template, BOOL fail_read_only,
                               UINT drive_type );
extern HANDLE FILE_CreateDevice( int client_id, DWORD access, LPSECURITY_ATTRIBUTES sa );

extern LONG WINAPI WIN16_hread(HFILE16,SEGPTR,LONG);

/* files/directory.c */
extern int DIR_Init(void);
extern UINT DIR_GetWindowsUnixDir( LPSTR path, UINT count );
extern UINT DIR_GetSystemUnixDir( LPSTR path, UINT count );
extern DWORD DIR_SearchAlternatePath( LPCSTR dll_path, LPCSTR name, LPCSTR ext,
                                      DWORD buflen, LPSTR buffer, LPSTR *lastpart);
extern DWORD DIR_SearchPath( LPCWSTR path, LPCWSTR name, LPCWSTR ext,
                             DOS_FULL_NAME *full_name, BOOL win32 );

/* files/dos_fs.c */
extern void DOSFS_UnixTimeToFileTime( time_t unixtime, LPFILETIME ft,
                                      DWORD remainder );
extern time_t DOSFS_FileTimeToUnixTime( const FILETIME *ft, DWORD *remainder );
extern BOOL DOSFS_ToDosFCBFormat( LPCWSTR name, LPWSTR buffer );
extern const DOS_DEVICE *DOSFS_GetDevice( LPCWSTR name );
extern const DOS_DEVICE *DOSFS_GetDeviceByHandle( HANDLE hFile );
extern HANDLE DOSFS_OpenDevice( LPCWSTR name, DWORD access, DWORD attributes, LPSECURITY_ATTRIBUTES sa);
extern BOOL DOSFS_FindUnixName( const DOS_FULL_NAME *path, LPCWSTR name, char *long_buf,
                                INT long_len, LPWSTR short_buf, BOOL ignore_case );
extern BOOL DOSFS_GetFullName( LPCWSTR name, BOOL check_last,
                                 DOS_FULL_NAME *full );
extern int DOSFS_FindNext( const char *path, const char *short_mask,
                           const char *long_mask, int drive, BYTE attr,
                           int skip, WIN32_FIND_DATAA *entry );

/* profile.c */
extern void PROFILE_UsageWineIni(void);
extern int PROFILE_GetWineIniString( LPCWSTR section, LPCWSTR key_name,
                                     LPCWSTR def, LPWSTR buffer, int len );
extern int PROFILE_GetWineIniBool( LPCWSTR section, LPCWSTR key_name, int def );

/* win32/device.c */
extern HANDLE DEVICE_Open( LPCWSTR filename, DWORD access, LPSECURITY_ATTRIBUTES sa );

/* ntdll/cdrom.c.c */
extern BOOL CDROM_DeviceIoControl(DWORD clientID, HANDLE hDevice, DWORD dwIoControlCode,
                                  LPVOID lpInBuffer, DWORD nInBufferSize,
                                  LPVOID lpOutBuffer, DWORD nOutBufferSize,
                                  LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);

#endif  /* __WINE_FILE_H */
