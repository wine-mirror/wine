/*
 * Wine Resource Compiler structure definitions
 *
 * Copyright 1998 Bertho A. Stultiens
 *
 */

#if !defined(__WRC_RSC_H) && !defined(__WINE_WRC_RSC_H)
#define __WRC_RSC_H
#define __WINE_WRC_RSC_H

#ifndef __WINE_WINTYPES_H
#include <wintypes.h>		/* For types in structure */
#endif

/*
 * Note on the resource and type names:
 *
 * These are (if non-null) pointers to a pascal-style
 * string. The first character (BYTE for 16 bit and WCHAR
 * for 32 bit resources) contains the length and the
 * rest is the string. They are _not_ '\0' terminated!
 */

typedef struct wrc_resource16
{
	INT32	resid;		/* The resource id if resname == NULL */
	LPSTR	resname;
	INT32	restype;	/* The resource type-id if typename == NULL */
	LPSTR	typename;
	LPBYTE	data;		/* Actual resource data */
	UINT32	datasize;	/* The size of the resource */
} wrc_resource16_t;

typedef struct wrc_resource32
{
	INT32	resid;		/* The resource id if resname == NULL */
	LPWSTR	resname;
	INT32	restype;	/* The resource type-id if typename == NULL */
	LPWSTR	typename;
	LPBYTE	data;		/* Actual resource data */
	UINT32	datasize;	/* The size of the resource */
} wrc_resource32_t;

#endif

