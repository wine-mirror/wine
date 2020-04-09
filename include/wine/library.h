/*
 * Definitions for the Wine library
 *
 * Copyright 2000 Alexandre Julliard
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

#ifndef __WINE_WINE_LIBRARY_H
#define __WINE_WINE_LIBRARY_H

#include <stdarg.h>
#include <sys/types.h>

#include <windef.h>
#include <winbase.h>

#ifdef __WINE_WINE_TEST_H
#error This file should not be used in Wine tests
#endif

#ifdef __WINE_USE_MSVCRT
#error This file should not be used with msvcrt headers
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* configuration */

extern const char *wine_get_build_dir(void);
extern const char *wine_get_config_dir(void);
extern const char *wine_get_data_dir(void);
extern const char *wine_get_server_dir(void);
extern const char *wine_get_user_name(void);
extern const char *wine_get_version(void);
extern const char *wine_get_build_id(void);
extern void wine_init_argv0_path( const char *argv0 );
extern void wine_exec_wine_binary( const char *name, char **argv, const char *env_var );

/* dll loading */

typedef void (*load_dll_callback_t)( void *, const char * );
extern void wine_dll_set_callback( load_dll_callback_t load );
extern const char *wine_dll_enum_load_path( unsigned int index );
extern void __wine_dll_register( const IMAGE_NT_HEADERS *header, const char *filename );
extern void wine_init( int argc, char *argv[], char *error, int error_size );

/* memory mappings */

extern void *wine_anon_mmap( void *start, size_t size, int prot, int flags );
extern void wine_mmap_add_reserved_area( void *addr, size_t size );
extern void wine_mmap_remove_reserved_area( void *addr, size_t size, int unmap );
extern int wine_mmap_is_in_reserved_area( void *addr, size_t size );
extern int wine_mmap_enum_reserved_areas( int (*enum_func)(void *base, size_t size, void *arg),
                                          void *arg, int top_down );

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_WINE_LIBRARY_H */
