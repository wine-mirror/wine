/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
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

#include "mfapi.h"
#include "mfidl.h"
#include "mferror.h"

typedef struct attributes
{
    IMFAttributes IMFAttributes_iface;
    LONG ref;
} mfattributes;

extern void init_attribute_object(mfattributes *object, UINT32 size) DECLSPEC_HIDDEN;

extern void init_system_queues(void) DECLSPEC_HIDDEN;
extern void shutdown_system_queues(void) DECLSPEC_HIDDEN;
extern BOOL is_platform_locked(void) DECLSPEC_HIDDEN;
