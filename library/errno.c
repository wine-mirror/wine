/*
 * Wine library reentrant errno support
 *
 * Copyright 1998 Alexandre Julliard
 */

/* Get pointers to the static errno and h_errno variables used by Xlib. This
   must be done before including <errno.h> makes the variables invisible.  */
static int *default_errno_location(void)
{
    extern int errno;
    return &errno;
}

static int *default_h_errno_location(void)
{
    extern int h_errno;
    return &h_errno;
}

int* (*wine_errno_location)(void) = default_errno_location;
int* (*wine_h_errno_location)(void) = default_h_errno_location;

#include "config.h"

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
