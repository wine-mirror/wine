/*
 * Copyright 2025 Piotr Caban for CodeWeavers
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

#ifndef DS_INCLUDED
#define DS_INCLUDED

const CLSID CLSID_DataShapeProvider = { 0x3449a1c8, 0xc56c, 0x11d0, { 0xad, 0x72, 0x00, 0xc0, 0x4f, 0xc2, 0x98, 0x63 }};
const CLSID DBPROPSET_MSDSDBINIT = { 0x55cb91a8, 0x5c7a, 0x11d1, { 0xad, 0xad, 0x00, 0xc0, 0x4f, 0xc2, 0x98, 0x63 }};
const CLSID DBPROPSET_MSDSSESSION = { 0xedf17536, 0xafbf, 0x11d1, { 0x88, 0x47, 0x00, 0x00, 0xf8, 0x79, 0xf9, 0x8c }};

const char* PROGID_DataShapeProvider = "MSDataShape";
const char* PROGID_DataShapeProvider_Version = "MSDataShape.1";

enum MSDSDBINITPROPENUM
{
    DBPROP_MSDS_DBINIT_DATAPROVIDER = 2,
};

enum MSDSSESSIONPROPENUM
{
    DBPROP_MSDS_SESS_UNIQUENAMES = 2,
};

#endif /* DS_INCLUDED */
