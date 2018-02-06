/*
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 Andriy Palamarchuk
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

#include "resource.h"

#define KEY_MAX_LEN             1024

#define REG_FORMAT_5 1
#define REG_FORMAT_4 2

void WINAPIV output_message(unsigned int id, ...);
void WINAPIV error_exit(unsigned int id, ...);

char *GetMultiByteString(const WCHAR *strW);
void *heap_xalloc(size_t size);
void *heap_xrealloc(void *buf, size_t size);
BOOL import_registry_file(FILE *reg_file);
void delete_registry_key(WCHAR *reg_key_name);
BOOL export_registry_key(WCHAR *file_name, WCHAR *path, DWORD format);
