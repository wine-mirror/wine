/*
 * Copyright 2022 Piotr Caban
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

extern HRESULT load_file(const WCHAR *path, void **data, DWORD *size) DECLSPEC_HIDDEN;
extern HRESULT load_resourceA(HMODULE module, const char *resource,
        void **data, DWORD *size) DECLSPEC_HIDDEN;
extern HRESULT load_resourceW(HMODULE module, const WCHAR *resource,
        void **data, DWORD *size) DECLSPEC_HIDDEN;
