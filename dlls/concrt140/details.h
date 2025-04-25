/*
 * Copyright 2021 Piotr Caban for CodeWeavers
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

#include <stdbool.h>
#include <stdlib.h>
#include "windef.h"
#include "winternl.h"
#include "cppexcept.h"

void __cdecl operator_delete(void*);
void* __cdecl operator_new(size_t) __WINE_ALLOC_SIZE(1) __WINE_DEALLOC(operator_delete) __WINE_MALLOC;

bool __cdecl Context_IsCurrentTaskCollectionCanceling(void);

void init_concurrency_details(void*);

void WINAPI DECLSPEC_NORETURN _CxxThrowException(void*,const cxx_exception_type*);
extern void (__cdecl *_Xmem)(void);
extern void (__cdecl *_Xout_of_range)(const char*);
void DECLSPEC_NORETURN throw_range_error(const char*);
