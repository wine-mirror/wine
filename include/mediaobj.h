/*
 * Copyright (C) 2002 Hidenori Takeshima
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

#ifndef __WINE_MEDIAOBJ_H_
#define __WINE_MEDIAOBJ_H_

/* forward decls. */

typedef struct IDMOQualityControl IDMOQualityControl;
typedef struct IDMOVideoOutputOptimizations IDMOVideoOutputOptimizations;
typedef struct IEnumDMO IEnumDMO;
typedef struct IMediaBuffer IMediaBuffer;
typedef struct IMediaObject IMediaObject;
typedef struct IMediaObjectInPlace IMediaObjectInPlace;



/* structs. */

typedef struct
{
	GUID	majortype;
	GUID	subtype;
	BOOL	bFixedSizeSamples;
	BOOL	bTemporalCompression;
	ULONG	lSampleSize;
	GUID	formattype;
	IUnknown*	pUnk;
	ULONG	cbFormat;
	BYTE*	pbFormat;
} DMO_MEDIA_TYPE;



#endif	/* __WINE_MEDIAOBJ_H_ */
