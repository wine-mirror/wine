/*
 * Wine library reentrant errno support
 *
 * Copyright 1998 Alexandre Julliard
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

#include <assert.h>

/* default errno before threading is initialized */
static int *default_errno_location(void)
{
    static int errno;
    return &errno;
}

/* default h_errno before threading is initialized */
static int *default_h_errno_location(void)
{
    static int h_errno;
    return &h_errno;
}

int* (*wine_errno_location)(void) = default_errno_location;
int* (*wine_h_errno_location)(void) = default_h_errno_location;

/***********************************************************************
 *           __errno_location/__error/___errno
 *
 * Get the per-thread errno location.
 */
#ifdef ERRNO_LOCATION
int *ERRNO_LOCATION(void)
{
    return wine_errno_location();
}
#endif /* ERRNO_LOCATION */

/***********************************************************************
 *           __h_errno_location
 *
 * Get the per-thread h_errno location.
 */
int *__h_errno_location(void)
{
    return wine_h_errno_location();
}

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
