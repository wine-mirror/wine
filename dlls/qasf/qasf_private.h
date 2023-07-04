/*
 * DirectShow ASF filters
 *
 * Copyright (C) 2019 Zebediah Figura
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

#ifndef QASF_PRIVATE_H
#define QASF_PRIVATE_H

#define COBJMACROS
#include "dshow.h"
#include "dmo.h"
#include "dmodshow.h"
#include "rpcproxy.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

HRESULT asf_reader_create(IUnknown *outer, IUnknown **out);
HRESULT dmo_wrapper_create(IUnknown *outer, IUnknown **out);

#endif /* QASF_PRIVATE_H */
