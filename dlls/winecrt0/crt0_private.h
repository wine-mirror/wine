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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_CRT0_PRIVATE_H__
#define __WINE_CRT0_PRIVATE_H__

#ifdef __APPLE__
static inline void _init(int argc, char **argv, char **envp ) { /* nothing */ }
static inline void _fini(void) { /* nothing */ }
#else
extern  void _init(int argc, char **argv, char **envp );
extern void _fini(void);
#endif

#endif /* __WINE_CRT0_PRIVATE_H__ */
