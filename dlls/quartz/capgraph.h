/*
 * Implementation of CLSID_CaptureGraphBuilder[2].
 *
 * Copyright (C) Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
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

#ifndef	WINE_DSHOW_CAPGRAPH_H
#define	WINE_DSHOW_CAPGRAPH_H

#include "iunk.h"

typedef struct CapGraph_ICaptureGraphBuilderImpl
{
	ICOM_VFIELD(ICaptureGraphBuilder);
} CapGraph_ICaptureGraphBuilderImpl;

typedef struct CapGraph_ICaptureGraphBuilder2Impl
{
	ICOM_VFIELD(ICaptureGraphBuilder2);
} CapGraph_ICaptureGraphBuilder2Impl;

typedef struct CCaptureGraph
{
	QUARTZ_IUnkImpl	unk;
	CapGraph_ICaptureGraphBuilderImpl	capgraph1;
	CapGraph_ICaptureGraphBuilder2Impl	capgraph2;

	/* ICaptureGraphBuilder fields. */
	/* ICaptureGraphBuilder2 fields. */
	IGraphBuilder*	m_pfg;
} CCaptureGraph;

#define	CCaptureGraph_THIS(iface,member)	CCaptureGraph*	This = ((CCaptureGraph*)(((char*)iface)-offsetof(CCaptureGraph,member)))

#define CCaptureGraph_ICaptureGraphBuilder(th)	((ICaptureGraphBuilder*)&((th)->capgraph1))
#define CCaptureGraph_ICaptureGraphBuilder2(th)	((ICaptureGraphBuilder2*)&((th)->capgraph2))

HRESULT QUARTZ_CreateCaptureGraph(IUnknown* punkOuter,void** ppobj);


#endif	/* WINE_DSHOW_CAPGRAPH_H */
