/*
 * Misc. functions for systems that don't have them
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#ifdef __BEOS__
#include <be/kernel/fs_info.h>
#include <be/kernel/OS.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_INTTYPES_H
# include <sys/inttypes.h>
#endif
#ifdef HAVE_SYS_TIME_h
# include <sys/time.h>
#endif
#include <sys/stat.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#include <errno.h>
#include <fcntl.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_LIBIO_H
# include <libio.h>
#endif
#ifdef HAVE_SYSCALL_H
# include <syscall.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif


/***********************************************************************
 *		pthread functions
 */

#ifndef HAVE_PTHREAD_GETSPECIFIC
void pthread_getspecific() { assert(0); }
#endif

#ifndef HAVE_PTHREAD_KEY_CREATE
void pthread_key_create() { assert(0); }
#endif

#ifndef HAVE_PTHREAD_MUTEX_LOCK
void pthread_mutex_lock() { assert(0); }
#endif

#ifndef HAVE_PTHREAD_MUTEX_UNLOCK
void pthread_mutex_unlock() { assert(0); }
#endif

#ifndef HAVE_PTHREAD_SETSPECIFIC
void pthread_setspecific() { assert(0); }
#endif
