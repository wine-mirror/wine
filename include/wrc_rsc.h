/*
 * Wine Resource Compiler structure definitions
 *
 * Copyright 1998 Bertho A. Stultiens
 *
 */

#ifndef __WINE_WRC_RSC_H
#define __WINE_WRC_RSC_H

#include "windef.h"		/* For types in structure */

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
	INT	resid;		/* The resource id if resname == NULL */
	LPSTR	resname;
	INT	restype;	/* The resource type-id if typename == NULL */
	LPSTR	restypename;
	LPBYTE	data;		/* Actual resource data */
	UINT	datasize;	/* The size of the resource */
} wrc_resource16_t;

typedef struct wrc_resource32
{
	INT	resid;		/* The resource id if resname == NULL */
	LPWSTR	resname;
	INT	restype;	/* The resource type-id if typename == NULL */
	LPWSTR	restypename;
	LPBYTE	data;		/* Actual resource data */
	UINT	datasize;	/* The size of the resource */
} wrc_resource32_t;

#endif /* __WINE_WRC_RSC_H */

