/*
 * Wine porting definitions
 *
 * Copyright 1996 Alexandre Julliard
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

#ifndef __WINE_WINE_PORT_H
#define __WINE_WINE_PORT_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#ifdef __WINE_BASETSD_H
# error You must include port.h before all other headers
#endif

#ifndef _GNU_SOURCE
# define _GNU_SOURCE  /* for pread/pwrite, isfinite */
#endif
#include <fcntl.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


/****************************************************************
 * Hard-coded values for the Windows platform
 */

#if defined(_WIN32) && !defined(__CYGWIN__)

#include <direct.h>
#include <errno.h>
#include <io.h>
#include <process.h>

static inline void *dlopen(const char *name, int flags) { return NULL; }
static inline void *dlsym(void *handle, const char *name) { return NULL; }
static inline int dlclose(void *handle) { return 0; }
static inline const char *dlerror(void) { return "No dlopen support on Windows"; }
static inline int symlink(const char *from, const char *to) { errno = ENOSYS; return -1; }

#ifdef _MSC_VER
/* The UCRT headers in the Windows SDK #error out if we #define snprintf.
 * The C headers that came with previous Visual Studio versions do not have
 * snprintf. Check for VS 2015, which appears to be the first version to
 * use the UCRT headers by default. */
#if _MSC_VER < 1900
# define snprintf _snprintf
#endif
#endif

#endif  /* _WIN32 */

/****************************************************************
 * Macro definitions
 */

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#else
#define RTLD_LAZY    0x001
#define RTLD_NOW     0x002
#define RTLD_GLOBAL  0x100
#endif

/****************************************************************
 * Constants
 */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.570796326794896619
#endif

#endif /* !defined(__WINE_WINE_PORT_H) */
