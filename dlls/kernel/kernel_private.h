/*
 * Kernel32 undocumented and private functions definition
 *
 * Copyright 2003 Eric Pouech
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

#ifndef __WINE_KERNEL_PRIVATE_H
#define __WINE_KERNEL_PRIVATE_H

HANDLE  WINAPI OpenConsoleW(LPCWSTR, DWORD, LPSECURITY_ATTRIBUTES, DWORD);
BOOL    WINAPI VerifyConsoleIoHandle(HANDLE);
HANDLE  WINAPI DuplicateConsoleHandle(HANDLE, DWORD, BOOL, DWORD);
BOOL    WINAPI CloseConsoleHandle(HANDLE handle);
HANDLE  WINAPI GetConsoleInputWaitHandle(void);

static inline BOOL is_console_handle(HANDLE h)
{
    return h != INVALID_HANDLE_VALUE && ((DWORD)h & 3) == 3;
}

/* map a real wineserver handle onto a kernel32 console handle */
static inline HANDLE console_handle_map(HANDLE h)
{
    return h != INVALID_HANDLE_VALUE ? (HANDLE)((DWORD)h ^ 3) : INVALID_HANDLE_VALUE;
}

/* map a kernel32 console handle onto a real wineserver handle */
static inline HANDLE console_handle_unmap(HANDLE h)
{
    return h != INVALID_HANDLE_VALUE ? (HANDLE)((DWORD)h ^ 3) : INVALID_HANDLE_VALUE;
}

#endif
