/*
 * MinGW wrapper: makes gcc behave like MinGW.
 *
 * Copyright 2002 Dimitrie O. Paun
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

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifndef GCC_BIN
#define GCC_BIN "gcc"
#endif

#ifndef INCLUDEDIR
#define INCLUDEDIR "/usr/local/include/wine"
#endif

int main(int argc, char **argv)
{
    char **gcc_argv;
    int i, j;

    gcc_argv = malloc(sizeof(char*) * (argc + 20));

    i = 0;
    gcc_argv[i++] = GCC_BIN;

    gcc_argv[i++] = "-fshort-wchar";
    gcc_argv[i++] = "-fPIC";
    gcc_argv[i++] = "-I" INCLUDEDIR;
    gcc_argv[i++] = "-I" INCLUDEDIR "/msvcrt";
    gcc_argv[i++] = "-I" INCLUDEDIR "/windows";
    gcc_argv[i++] = "-DWINE_DEFINE_WCHAR_T";
    gcc_argv[i++] = "-D__int8=char";
    gcc_argv[i++] = "-D__int16=short";
    gcc_argv[i++] = "-D__int32=int";
    gcc_argv[i++] = "-D__int64=long long";

    for ( j = 1 ; j < argc ; j++ ) {
	if (strcmp("-mno-cygwin", argv[j]) == 0) {
	    /* ignore this option */
        } else {
            gcc_argv[i++] = argv[j];
        }
    }
    gcc_argv[i] = NULL;

    return execvp(GCC_BIN, gcc_argv);
}
