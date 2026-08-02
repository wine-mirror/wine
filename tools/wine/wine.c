/*
 * Loader for Wine installed in the bin directory
 *
 * Copyright 2025 Alexandre Julliard
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

#include "config.h"

#include "../tools.h"

#include <dlfcn.h>

static const char *bindir;
static const char *libdir;

static void *load_ntdll(void)
{
    const char *arch_dir = get_arch_dir( get_default_target() );
    struct strarray dllpath;
    void *handle;
    unsigned int i;

    if (bindir && strendswith( bindir, "/tools/wine" ) &&
        ((handle = dlopen( strmake( "%s/../../dlls/ntdll/ntdll.so", bindir ), RTLD_NOW ))))
        return handle;

    if ((handle = dlopen( strmake( "%s/wine%s/ntdll.so", libdir, arch_dir ), RTLD_NOW )))
        return handle;

    dllpath = strarray_frompath( getenv( "WINEDLLPATH" ));
    for (i = 0; i < dllpath.count; i++)
    {
        if ((handle = dlopen( strmake( "%s%s/ntdll.so", dllpath.str[i], arch_dir ), RTLD_NOW )))
            return handle;
        if ((handle = dlopen( strmake( "%s/ntdll.so", dllpath.str[i] ), RTLD_NOW )))
            return handle;
    }
    fprintf( stderr, "wine: could not load ntdll.so: %s\n", dlerror() );
    exit(1);
}

int main( int argc, char *argv[] )
{
    void (*init_func)(int, char **);

    bindir = get_bindir( argv[0] );
    libdir = get_libdir( bindir );
    init_func = dlsym( load_ntdll(), "__wine_main" );
    if (init_func) init_func( argc, argv );

    fprintf( stderr, "wine: __wine_main function not found in ntdll.so\n" );
    exit(1);
}
