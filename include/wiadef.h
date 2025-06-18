/*
 * WIA constants
 *
 * Copyright 2015 Nikolay Sivov for CodeWeavers
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

#define WIA_DEVINFO_ENUM_ALL      0x0000000f
#define WIA_DEVINFO_ENUM_LOCAL    0x00000010

#define FACILITY_WIA 33

#define BASE_VAL_WIA_ERROR        0x00000000

#define WIA_S_NO_DEVICE_AVAILABLE       MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIA, (BASE_VAL_WIA_ERROR + 21))
