/*
 * Queue Manager definitions
 *
 * Copyright 2007 Google (Roy Shea)
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

#ifndef __QMGR_H__
#define __QMGR_H__

#include "windef.h"
#include "objbase.h"

#define COBJMACROS
#include "bits.h"

#include <string.h>

/* Background copy job vtbl and related data */
typedef struct
{
    const IBackgroundCopyJobVtbl *lpVtbl;
    LONG ref;
} BackgroundCopyJobImpl;

/* Background copy manager vtbl and related data */
typedef struct
{
    const IBackgroundCopyManagerVtbl *lpVtbl;
    LONG ref;
} BackgroundCopyManagerImpl;

typedef struct
{
    const IClassFactoryVtbl *lpVtbl;
} ClassFactoryImpl;

extern ClassFactoryImpl BITS_ClassFactory;

HRESULT BackgroundCopyManagerConstructor(IUnknown *pUnkOuter, LPVOID *ppObj);

/* Little helper functions */
static inline char *
qmgr_strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *d = HeapAlloc(GetProcessHeap(), 0, n);
    return d ? memcpy(d, s, n) : NULL;
}

#endif /* __QMGR_H__ */
