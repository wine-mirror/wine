/*
 * Copyright (C) 2002 Mike McCormack
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

#ifndef __INC_SMB__
#define __INC_SMB__

static inline int SMB_isSepW (WCHAR c) {return (c == '\\' || c == '/');}
static inline int SMB_isUNCW (LPCWSTR filename) {return (filename && SMB_isSepW (filename[0]) && SMB_isSepW (filename[1]));}

NTSTATUS WINAPI SMB_ReadFile(HANDLE hFile, int fd, LPVOID buffer, DWORD bytesToRead,
                             PIO_STATUS_BLOCK io_status);
extern HANDLE WINAPI SMB_CreateFileW( LPCWSTR filename, DWORD access, DWORD sharing,
                              LPSECURITY_ATTRIBUTES sa, DWORD creation,
                              DWORD attributes, HANDLE template );

typedef struct tagSMB_DIR
{
    int current;
    int num_entries;
    unsigned char **entries;
    unsigned char *buffer;
} SMB_DIR;

extern SMB_DIR* WINAPI SMB_FindFirst(LPCWSTR filename);
extern BOOL WINAPI SMB_FindNext(SMB_DIR *dir, WIN32_FIND_DATAW *data );
extern BOOL WINAPI SMB_CloseDir(SMB_DIR *dir);

#endif /* __INC_SMB__ */
