/*
 * Copyright 2015 Iván Matellanes
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

#include "stdlib.h"
#include "windef.h"
#include "cxx.h"

typedef LONG streamoff;
typedef LONG streampos;
typedef int filedesc;
typedef void* (__cdecl *allocFunction)(LONG);
typedef void (__cdecl *freeFunction)(void*);

typedef enum {
    IOSTATE_goodbit   = 0x0,
    IOSTATE_eofbit    = 0x1,
    IOSTATE_failbit   = 0x2,
    IOSTATE_badbit    = 0x4
} ios_io_state;

typedef enum {
    OPENMODE_in          = 0x1,
    OPENMODE_out         = 0x2,
    OPENMODE_ate         = 0x4,
    OPENMODE_app         = 0x8,
    OPENMODE_trunc       = 0x10,
    OPENMODE_nocreate    = 0x20,
    OPENMODE_noreplace   = 0x40,
    OPENMODE_binary      = 0x80
} ios_open_mode;

typedef enum {
    SEEKDIR_beg = 0,
    SEEKDIR_cur = 1,
    SEEKDIR_end = 2
} ios_seek_dir;

typedef enum {
    FLAGS_skipws     = 0x1,
    FLAGS_left       = 0x2,
    FLAGS_right      = 0x4,
    FLAGS_internal   = 0x8,
    FLAGS_dec        = 0x10,
    FLAGS_oct        = 0x20,
    FLAGS_hex        = 0x40,
    FLAGS_showbase   = 0x80,
    FLAGS_showpoint  = 0x100,
    FLAGS_uppercase  = 0x200,
    FLAGS_showpos    = 0x400,
    FLAGS_scientific = 0x800,
    FLAGS_fixed      = 0x1000,
    FLAGS_unitbuf    = 0x2000,
    FLAGS_stdio      = 0x4000
} ios_flags;

void* __cdecl operator_new(SIZE_T);
void __cdecl operator_delete(void*);

void init_exception(void*);
