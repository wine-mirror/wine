/*
 * Definitions for Wine C unit tests.
 *
 * Copyright (C) 2002 Alexandre Julliard
 */

#ifndef __WINE_TEST_H
#define __WINE_TEST_H

/* debug level */
extern int winetest_debug;

/* current platform */
extern const char *winetest_platform;

extern void winetest_ok( int condition, const char *msg, const char *file, int line );

#define START_TEST(name) void func_##name(void)

#define ok(test,msg) winetest_ok( (test), (msg), __FILE__, __LINE__ )

#endif  /* __WINE_TEST_H */
