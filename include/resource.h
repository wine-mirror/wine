/*
 * Resource definitions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_RESOURCE_H
#define __WINE_RESOURCE_H

#include "windef.h"
#include "wrc_rsc.h"

#if defined(__GNUC__) && ((__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 7)))
#define WINE_CONSTRUCTOR  __attribute__((constructor))
#define HAVE_WINE_CONSTRUCTOR
#else
#define WINE_CONSTRUCTOR
#undef HAVE_WINE_CONSTRUCTOR
#endif

#endif /* __WINE_RESOURCE_H */
