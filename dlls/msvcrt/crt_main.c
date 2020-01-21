/*
 * mainCRTStartup default entry point
 *
 * Copyright 2019 Jacek Caban for CodeWeavers
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

#if 0
#pragma makedep implib
#endif

#ifdef __MINGW32__

#include "msvcrt.h"

#include "windef.h"
#include "winbase.h"

/* FIXME: Use msvcrt headers once we move to PE file */
void __cdecl exit(int);
void __cdecl __getmainargs(int *, char ***, char ***, int, int *);
void __cdecl __set_app_type(int);

int __cdecl main(int argc, char **argv, char **env);

static const IMAGE_NT_HEADERS *get_nt_header( void )
{
    extern IMAGE_DOS_HEADER __ImageBase;
    return (const IMAGE_NT_HEADERS *)((char *)&__ImageBase + __ImageBase.e_lfanew);
}

int __cdecl mainCRTStartup(void)
{
    int argc, new_mode =  0, ret;
    char **argv, **env;

    __getmainargs(&argc, &argv, &env, 0, &new_mode);
    __set_app_type(get_nt_header()->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI ? 2 : 1);

    ret = main(argc, argv, env);

    exit(ret);
    return ret;
}

#endif
