/*
 * Wine Resource Compiler structure definitions
 *
 * Copyright 1998 Bertho A. Stultiens
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

