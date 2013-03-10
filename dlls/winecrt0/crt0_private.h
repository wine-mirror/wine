/*
 * crt0 library private definitions
 *
 * Copyright 2005 Alexandre Julliard
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

#ifndef __WINE_CRT0_PRIVATE_H__
#define __WINE_CRT0_PRIVATE_H__

#if defined(__APPLE__) || defined(__ANDROID__)
static inline void _init(int argc, char **argv, char **envp ) { /* nothing */ }
static inline void _fini(void) { /* nothing */ }
#else
extern void _init(int argc, char **argv, char **envp ) DECLSPEC_HIDDEN;
extern void _fini(void) DECLSPEC_HIDDEN;
#endif

enum init_state
{
    NO_INIT_DONE,      /* no initialization done yet */
    DLL_REGISTERED,    /* the dll has been registered */
    CONSTRUCTORS_DONE  /* the constructors have been run (implies dll registered too) */
};

extern enum init_state __wine_spec_init_state DECLSPEC_HIDDEN;

#endif /* __WINE_CRT0_PRIVATE_H__ */
