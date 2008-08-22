/*
 *      DirectX File private interfaces (D3DXOF.DLL)
 *
 * Copyright 2004, 2008 Christian Costa
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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

#ifndef __D3DXOF_PRIVATE_INCLUDED__
#define __D3DXOF_PRIVATE_INCLUDED__

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dxfile.h"

#define MAX_NAME_LEN 32
#define MAX_ARRAY_DIM 1
#define MAX_MEMBERS 50
#define MAX_CHILDS 10
#define MAX_TEMPLATES 200

typedef struct {
    DWORD type;
    LONG idx_template;
    char name[MAX_NAME_LEN];
    ULONG nb_dims;
    BOOL  dim_fixed[MAX_ARRAY_DIM]; /* fixed or variable */
    ULONG dim_value[MAX_ARRAY_DIM]; /* fixed value or member index */
} member;

typedef struct {
    char name[MAX_NAME_LEN];
    GUID class_id;
    BOOL open;
    ULONG nb_childs;
    char childs[MAX_CHILDS][MAX_NAME_LEN];
    ULONG nb_members;
    member members[MAX_MEMBERS];
} xtemplate;

typedef struct {
    IDirectXFile lpVtbl;
    LONG ref;
    ULONG nb_xtemplates;
    xtemplate xtemplates[MAX_TEMPLATES];
} IDirectXFileImpl;

typedef struct {
    IDirectXFileBinary lpVtbl;
    LONG ref;
} IDirectXFileBinaryImpl;

typedef struct {
    IDirectXFileData lpVtbl;
    LONG ref;
} IDirectXFileDataImpl;

typedef struct {
    IDirectXFileDataReference lpVtbl;
    LONG ref;
} IDirectXFileDataReferenceImpl;

typedef struct {
    IDirectXFileObject lpVtbl;
    LONG ref;
} IDirectXFileObjectImpl;

typedef struct {
    IDirectXFileEnumObject lpVtbl;
    LONG ref;
} IDirectXFileEnumObjectImpl;

typedef struct {
    IDirectXFileSaveObject lpVtbl;
    LONG ref;
} IDirectXFileSaveObjectImpl;

HRESULT IDirectXFileImpl_Create(IUnknown *pUnkOuter, LPVOID *ppObj);
HRESULT IDirectXFileBinaryImpl_Create(IDirectXFileBinaryImpl** ppObj);
HRESULT IDirectXFileDataImpl_Create(IDirectXFileDataImpl** ppObj);
HRESULT IDirectXFileDataReferenceImpl_Create(IDirectXFileDataReferenceImpl** ppObj);
HRESULT IDirectXFileEnumObjectImpl_Create(IDirectXFileEnumObjectImpl** ppObj);
HRESULT IDirectXFileFileObjectImpl_Create(IDirectXFileObjectImpl** ppObj);
HRESULT IDirectXFileFileSaveObjectImpl_Create(IDirectXFileSaveObjectImpl** ppObj);

#endif /* __D3DXOF_PRIVATE_INCLUDED__ */
