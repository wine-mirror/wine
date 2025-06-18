/*
 * DirectShow DVD filters
 *
 * Copyright 2020 Gijs Vermeulen
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

#ifndef QDVD_PRIVATE_H
#define QDVD_PRIVATE_H

#define COBJMACROS
#include "dshow.h"
#include "wine/debug.h"

HRESULT graph_builder_create(IUnknown *outer, IUnknown **out);
HRESULT navigator_create(IUnknown *outer, IUnknown **out);

#endif /* QDVD_PRIVATE_H */
