/*
 * Definitions for the Wine library
 *
 * Copyright 2000 Alexandre Julliard
 */

#ifndef __WINE_WINE_LIBRARY_H
#define __WINE_WINE_LIBRARY_H

#include <sys/types.h>

/* dll loading */

typedef void (*load_dll_callback_t)( void *, const char * );

extern void wine_dll_set_callback( load_dll_callback_t load );
extern void *wine_dll_load( const char *filename );
extern void *wine_dll_load_main_exe( const char *name, int search_path );
extern void wine_dll_unload( void *handle );

/* debugging */

extern void wine_dbg_add_option( const char *name, unsigned char set, unsigned char clear );

/* portability */

extern void *wine_anon_mmap( void *start, size_t size, int prot, int flags );

#endif  /* __WINE_WINE_LIBRARY_H */
