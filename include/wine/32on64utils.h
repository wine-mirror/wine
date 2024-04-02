/*
 * Utilities for easing 32-on-64-bit mode compatibility.
 *
 * Copyright 2019 Ken Thomases for CodeWeavers Inc.
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

#ifndef __WINE_WINE_32ON64UTILS_H
#define __WINE_WINE_32ON64UTILS_H

#ifndef HOSTPTR
#ifdef __i386_on_x86_64__
#define HOSTPTR __ptr64
#else
#define HOSTPTR
#endif
#endif

#ifndef HOSTSTORAGE
#ifdef __i386_on_x86_64__
#define HOSTSTORAGE __attribute__((address_space(0)))
#else
#define HOSTSTORAGE
#endif
#endif

#ifndef WIN32PTR
#ifdef __i386_on_x86_64__
#define WIN32PTR __ptr32
#else
#define WIN32PTR
#endif
#endif

#ifndef WIN32STORAGE
#ifdef __i386_on_x86_64__
#define WIN32STORAGE __attribute__((address_space(32)))
#else
#define WIN32STORAGE
#endif
#endif

#ifndef TRUNCCAST
#ifdef __has_warning
#if __has_warning("-Wint-to-pointer-cast-truncates")
#define TRUNCCAST(type, expr) ((__truncate type)(expr))
#endif
#endif

#ifndef TRUNCCAST
#define TRUNCCAST(type, expr) ((type)(expr))
#endif
#endif

#ifndef ADDRSPACECAST
#ifdef __i386_on_x86_64__
#define ADDRSPACECAST(type, expr) ((__addrspace type)(expr))
#else
#define ADDRSPACECAST(type, expr) ((type)(expr))
#endif
#endif

#ifndef MAP_FAILED_HOSTPTR
#define MAP_FAILED_HOSTPTR ((void * HOSTPTR)-1)
#endif

#ifndef RTLD_DEFAULT_HOSTPTR
#if defined(__i386_on_x86_64__) && defined(__APPLE__)
#define RTLD_DEFAULT_HOSTPTR ((void * HOSTPTR)-2)
#else
#define RTLD_DEFAULT_HOSTPTR RTLD_DEFAULT
#endif
#endif

#if !defined(RTLD_SELF_HOSTPTR) && defined(RTLD_SELF)
#if defined(__i386_on_x86_64__) && defined(__APPLE__)
#define RTLD_SELF_HOSTPTR ((void * HOSTPTR)-3)
#else
#define RTLD_SELF_HOSTPTR RTLF_SELF
#endif
#endif

#endif /* __WINE_WINE_32ON64UTILS_H */
