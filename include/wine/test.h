/*
 * Definitions for Wine C unit tests.
 *
 * Copyright (C) 2002 Alexandre Julliard
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

#ifndef __WINE_TEST_H
#define __WINE_TEST_H

#include <stdarg.h>
#include "windef.h"

/* debug level */
extern int winetest_debug;

/* current platform */
extern const char *winetest_platform;

typedef int (*winetest_ok_funcptr)( int condition, const char *msg, ...);
typedef void (*winetest_trace_funcptr)( const char *msg, ...);

extern winetest_ok_funcptr winetest_set_ok_location( const char* file, int line );
extern winetest_trace_funcptr winetest_set_trace_location( const char* file, int line );
extern void winetest_start_todo( const char* platform );
extern int winetest_loop_todo(void);
extern void winetest_end_todo( const char* platform );

#define START_TEST(name) void func_##name(void)

#define ok             (*winetest_set_ok_location(__FILE__, __LINE__))
#define trace          (*winetest_set_trace_location(__FILE__, __LINE__))

#define todo(platform) for (winetest_start_todo(platform); \
                            winetest_loop_todo(); \
                            winetest_end_todo(platform))
#define todo_wine      todo("wine")

#endif  /* __WINE_TEST_H */
