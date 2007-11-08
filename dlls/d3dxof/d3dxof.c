/*
 * Implementation of DirectX File Interfaces
 *
 * Copyright 2004 Christian Costa
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

#include "config.h"
#include "wine/debug.h"

#define COBJMACROS

#include "winbase.h"
#include "wingdi.h"

#include "d3dxof_private.h"
#include "dxfile.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dxof);

static const struct IDirectXFileVtbl IDirectXFile_Vtbl;
static const struct IDirectXFileBinaryVtbl IDirectXFileBinary_Vtbl;
static const struct IDirectXFileDataVtbl IDirectXFileData_Vtbl;
static const struct IDirectXFileDataReferenceVtbl IDirectXFileDataReference_Vtbl;
static const struct IDirectXFileEnumObjectVtbl IDirectXFileEnumObject_Vtbl;
static const struct IDirectXFileObjectVtbl IDirectXFileObject_Vtbl;
static const struct IDirectXFileSaveObjectVtbl IDirectXFileSaveObject_Vtbl;

HRESULT IDirectXFileImpl_Create(IUnknown* pUnkOuter, LPVOID* ppObj)
{
    IDirectXFileImpl* object;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileImpl));
    
    object->lpVtbl.lpVtbl = &IDirectXFile_Vtbl;
    object->ref = 1;

    *ppObj = object;
    
    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectXFileImpl_QueryInterface(IDirectXFile* iface, REFIID riid, void** ppvObject)
{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;

  TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IDirectXFile))
  {
    IClassFactory_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectXFileImpl_AddRef(IDirectXFile* iface)
{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p/%p): AddRef from %d\n", iface, This, ref - 1);

  return ref;
}

static ULONG WINAPI IDirectXFileImpl_Release(IDirectXFile* iface)
{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p/%p): ReleaseRef to %d\n", iface, This, ref);

  if (!ref)
    HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

/*** IDirectXFile methods ***/
static HRESULT WINAPI IDirectXFileImpl_CreateEnumObject(IDirectXFile* iface, LPVOID pvSource, DXFILELOADOPTIONS dwLoadOptions, LPDIRECTXFILEENUMOBJECT* ppEnumObj)

{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;
  IDirectXFileEnumObjectImpl* object; 

  FIXME("(%p/%p)->(%p,%x,%p) stub!\n", This, iface, pvSource, dwLoadOptions, ppEnumObj);

  if (dwLoadOptions == 0)
  {
    FIXME("Source is a file '%s'\n", (char*)pvSource);
  }

  object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileEnumObjectImpl));

  object->lpVtbl.lpVtbl = &IDirectXFileEnumObject_Vtbl;
  object->ref = 1;

  *ppEnumObj = (LPDIRECTXFILEENUMOBJECT)object;
    
  return DXFILE_OK;
}

static HRESULT WINAPI IDirectXFileImpl_CreateSaveObject(IDirectXFile* iface, LPCSTR szFileName, DXFILEFORMAT dwFileFormat, LPDIRECTXFILESAVEOBJECT* ppSaveObj)
{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;

  FIXME("(%p/%p)->(%s,%x,%p) stub!\n", This, iface, szFileName, dwFileFormat, ppSaveObj);

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileImpl_RegisterTemplates(IDirectXFile* iface, LPVOID pvData, DWORD cbSize)
{
  IDirectXFileImpl *This = (IDirectXFileImpl *)iface;

  FIXME("(%p/%p)->(%p,%d) stub!\n", This, iface, pvData, cbSize);

  return DXFILEERR_BADVALUE;
}

static const IDirectXFileVtbl IDirectXFile_Vtbl =
{
  IDirectXFileImpl_QueryInterface,
  IDirectXFileImpl_AddRef,
  IDirectXFileImpl_Release,
  IDirectXFileImpl_CreateEnumObject,
  IDirectXFileImpl_CreateSaveObject,
  IDirectXFileImpl_RegisterTemplates
};

HRESULT IDirectXFileBinaryImpl_Create(IDirectXFileBinaryImpl** ppObj)
{
    IDirectXFileBinaryImpl* object;

    TRACE("(%p)\n", ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileBinaryImpl));
    
    object->lpVtbl.lpVtbl = &IDirectXFileBinary_Vtbl;
    object->ref = 1;

    *ppObj = object;
    
    return DXFILE_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectXFileBinaryImpl_QueryInterface(IDirectXFileBinary* iface, REFIID riid, void** ppvObject)
{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;

  TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IDirectXFileObject)
      || IsEqualGUID(riid, &IID_IDirectXFileBinary))
  {
    IClassFactory_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectXFileBinaryImpl_AddRef(IDirectXFileBinary* iface)
{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p/%p): AddRef from %d\n", iface, This, ref - 1);

  return ref;
}

static ULONG WINAPI IDirectXFileBinaryImpl_Release(IDirectXFileBinary* iface)
{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p/%p): ReleaseRef to %d\n", iface, This, ref);

  if (!ref)
    HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

/*** IDirectXFileObject methods ***/
static HRESULT WINAPI IDirectXFileBinaryImpl_GetName(IDirectXFileBinary* iface, LPSTR pstrNameBuf, LPDWORD pdwBufLen)

{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;

  FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, pstrNameBuf, pdwBufLen); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileBinaryImpl_GetId(IDirectXFileBinary* iface, LPGUID pGuid)
{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pGuid); 

  return DXFILEERR_BADVALUE;
}

/*** IDirectXFileBinary methods ***/
static HRESULT WINAPI IDirectXFileBinaryImpl_GetSize(IDirectXFileBinary* iface, DWORD* pcbSize)
{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pcbSize); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileBinaryImpl_GetMimeType(IDirectXFileBinary* iface, LPCSTR* pszMimeType)
{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pszMimeType);

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileBinaryImpl_Read(IDirectXFileBinary* iface, LPVOID pvData, DWORD cbSize, LPDWORD pcbRead)
{
  IDirectXFileBinaryImpl *This = (IDirectXFileBinaryImpl *)iface;

  FIXME("(%p/%p)->(%p, %d, %p) stub!\n", This, iface, pvData, cbSize, pcbRead);

  return DXFILEERR_BADVALUE;
}

static const IDirectXFileBinaryVtbl IDirectXFileBinary_Vtbl =
{
    IDirectXFileBinaryImpl_QueryInterface,
    IDirectXFileBinaryImpl_AddRef,
    IDirectXFileBinaryImpl_Release,
    IDirectXFileBinaryImpl_GetName,
    IDirectXFileBinaryImpl_GetId,
    IDirectXFileBinaryImpl_GetSize,
    IDirectXFileBinaryImpl_GetMimeType,
    IDirectXFileBinaryImpl_Read
};

HRESULT IDirectXFileDataImpl_Create(IDirectXFileDataImpl** ppObj)
{
    IDirectXFileDataImpl* object;

    TRACE("(%p)\n", ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileDataImpl));
    
    object->lpVtbl.lpVtbl = &IDirectXFileData_Vtbl;
    object->ref = 1;

    *ppObj = object;
    
    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectXFileDataImpl_QueryInterface(IDirectXFileData* iface, REFIID riid, void** ppvObject)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IDirectXFileObject)
      || IsEqualGUID(riid, &IID_IDirectXFileData))
  {
    IClassFactory_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectXFileDataImpl_AddRef(IDirectXFileData* iface)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p/%p): AddRef from %d\n", iface, This, ref - 1);

  return ref;
}

static ULONG WINAPI IDirectXFileDataImpl_Release(IDirectXFileData* iface)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p/%p): ReleaseRef to %d\n", iface, This, ref);

  if (!ref)
    HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

/*** IDirectXFileObject methods ***/
static HRESULT WINAPI IDirectXFileDataImpl_GetName(IDirectXFileData* iface, LPSTR pstrNameBuf, LPDWORD pdwBufLen)

{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, pstrNameBuf, pdwBufLen); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataImpl_GetId(IDirectXFileData* iface, LPGUID pGuid)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pGuid); 

  return DXFILEERR_BADVALUE;
}

/*** IDirectXFileData methods ***/
static HRESULT WINAPI IDirectXFileDataImpl_GetData(IDirectXFileData* iface, LPCSTR szMember, DWORD* pcbSize, void** ppvData)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%s,%p,%p) stub!\n", This, iface, szMember, pcbSize, ppvData); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataImpl_GetType(IDirectXFileData* iface, const GUID** pguid)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pguid); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataImpl_GetNextObject(IDirectXFileData* iface, LPDIRECTXFILEOBJECT* ppChildObj)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, ppChildObj); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataImpl_AddDataObject(IDirectXFileData* iface, LPDIRECTXFILEDATA pDataObj)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pDataObj); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataImpl_AddDataReference(IDirectXFileData* iface, LPCSTR szRef, const GUID* pguidRef)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%s,%p) stub!\n", This, iface, szRef, pguidRef); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataImpl_AddBinaryObject(IDirectXFileData* iface, LPCSTR szName, const GUID* pguid, LPCSTR szMimeType, LPVOID pvData, DWORD cbSize)
{
  IDirectXFileDataImpl *This = (IDirectXFileDataImpl *)iface;

  FIXME("(%p/%p)->(%s,%p,%s,%p,%d) stub!\n", This, iface, szName, pguid, szMimeType, pvData, cbSize);

  return DXFILEERR_BADVALUE;
}

static const IDirectXFileDataVtbl IDirectXFileData_Vtbl =
{
    IDirectXFileDataImpl_QueryInterface,
    IDirectXFileDataImpl_AddRef,
    IDirectXFileDataImpl_Release,
    IDirectXFileDataImpl_GetName,
    IDirectXFileDataImpl_GetId,
    IDirectXFileDataImpl_GetData,
    IDirectXFileDataImpl_GetType,
    IDirectXFileDataImpl_GetNextObject,
    IDirectXFileDataImpl_AddDataObject,
    IDirectXFileDataImpl_AddDataReference,
    IDirectXFileDataImpl_AddBinaryObject
};

HRESULT IDirectXFileDataReferenceImpl_Create(IDirectXFileDataReferenceImpl** ppObj)
{
    IDirectXFileDataReferenceImpl* object;

    TRACE("(%p)\n", ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileDataReferenceImpl));
    
    object->lpVtbl.lpVtbl = &IDirectXFileDataReference_Vtbl;
    object->ref = 1;

    *ppObj = object;
    
    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectXFileDataReferenceImpl_QueryInterface(IDirectXFileDataReference* iface, REFIID riid, void** ppvObject)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;

  TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IDirectXFileObject)
      || IsEqualGUID(riid, &IID_IDirectXFileDataReference))
  {
    IClassFactory_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectXFileDataReferenceImpl_AddRef(IDirectXFileDataReference* iface)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p/%p): AddRef from %d\n", iface, This, ref - 1);

  return ref;
}

static ULONG WINAPI IDirectXFileDataReferenceImpl_Release(IDirectXFileDataReference* iface)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p/%p): ReleaseRef to %d\n", iface, This, ref);

  if (!ref)
    HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

/*** IDirectXFileObject methods ***/
static HRESULT WINAPI IDirectXFileDataReferenceImpl_GetName(IDirectXFileDataReference* iface, LPSTR pstrNameBuf, LPDWORD pdwBufLen)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;

  FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, pstrNameBuf, pdwBufLen); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileDataReferenceImpl_GetId(IDirectXFileDataReference* iface, LPGUID pGuid)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pGuid); 

  return DXFILEERR_BADVALUE;
}

/*** IDirectXFileDataReference ***/
static HRESULT WINAPI IDirectXFileDataReferenceImpl_Resolve(IDirectXFileDataReference* iface, LPDIRECTXFILEDATA* ppDataObj)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, ppDataObj); 

  return DXFILEERR_BADVALUE;
}

static const IDirectXFileDataReferenceVtbl IDirectXFileDataReference_Vtbl =
{
    IDirectXFileDataReferenceImpl_QueryInterface,
    IDirectXFileDataReferenceImpl_AddRef,
    IDirectXFileDataReferenceImpl_Release,
    IDirectXFileDataReferenceImpl_GetName,
    IDirectXFileDataReferenceImpl_GetId,
    IDirectXFileDataReferenceImpl_Resolve
};

HRESULT IDirectXFileEnumObjectImpl_Create(IDirectXFileEnumObjectImpl** ppObj)
{
    IDirectXFileEnumObjectImpl* object;

    TRACE("(%p)\n", ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileEnumObjectImpl));
    
    object->lpVtbl.lpVtbl = &IDirectXFileEnumObject_Vtbl;
    object->ref = 1;

    *ppObj = object;
    
    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectXFileEnumObjectImpl_QueryInterface(IDirectXFileEnumObject* iface, REFIID riid, void** ppvObject)
{
  IDirectXFileEnumObjectImpl *This = (IDirectXFileEnumObjectImpl *)iface;

  TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IDirectXFileEnumObject))
  {
    IClassFactory_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectXFileEnumObjectImpl_AddRef(IDirectXFileEnumObject* iface)
{
  IDirectXFileEnumObjectImpl *This = (IDirectXFileEnumObjectImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p/%p): AddRef from %d\n", iface, This, ref - 1);

  return ref;
}

static ULONG WINAPI IDirectXFileEnumObjectImpl_Release(IDirectXFileEnumObject* iface)
{
  IDirectXFileEnumObjectImpl *This = (IDirectXFileEnumObjectImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p/%p): ReleaseRef to %d\n", iface, This, ref);

  if (!ref)
    HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

/*** IDirectXFileEnumObject methods ***/
static HRESULT WINAPI IDirectXFileEnumObjectImpl_GetNextDataObject(IDirectXFileEnumObject* iface, LPDIRECTXFILEDATA* ppDataObj)
{
  IDirectXFileEnumObjectImpl *This = (IDirectXFileEnumObjectImpl *)iface;
  IDirectXFileDataImpl* object;
  
  FIXME("(%p/%p)->(%p) stub!\n", This, iface, ppDataObj); 

  object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileDataImpl));

  object->lpVtbl.lpVtbl = &IDirectXFileData_Vtbl;
  object->ref = 1;

  *ppDataObj = (LPDIRECTXFILEDATA)object;

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileEnumObjectImpl_GetDataObjectById(IDirectXFileEnumObject* iface, REFGUID rguid, LPDIRECTXFILEDATA* ppDataObj)
{
  IDirectXFileEnumObjectImpl *This = (IDirectXFileEnumObjectImpl *)iface;

  FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, rguid, ppDataObj); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileEnumObjectImpl_GetDataObjectByName(IDirectXFileEnumObject* iface, LPCSTR szName, LPDIRECTXFILEDATA* ppDataObj)
{
  IDirectXFileEnumObjectImpl *This = (IDirectXFileEnumObjectImpl *)iface;

  FIXME("(%p/%p)->(%s,%p) stub!\n", This, iface, szName, ppDataObj); 

  return DXFILEERR_BADVALUE;
}

static const IDirectXFileEnumObjectVtbl IDirectXFileEnumObject_Vtbl =
{
    IDirectXFileEnumObjectImpl_QueryInterface,
    IDirectXFileEnumObjectImpl_AddRef,
    IDirectXFileEnumObjectImpl_Release,
    IDirectXFileEnumObjectImpl_GetNextDataObject,
    IDirectXFileEnumObjectImpl_GetDataObjectById,
    IDirectXFileEnumObjectImpl_GetDataObjectByName
};

HRESULT IDirectXFileObjectImpl_Create(IDirectXFileObjectImpl** ppObj)
{
    IDirectXFileObjectImpl* object;

    TRACE("(%p)\n", ppObj);
      
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileObjectImpl));
    
    object->lpVtbl.lpVtbl = &IDirectXFileObject_Vtbl;
    object->ref = 1;

    *ppObj = object;
    
    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectXFileObjectImpl_QueryInterface(IDirectXFileObject* iface, REFIID riid, void** ppvObject)
{
  IDirectXFileObjectImpl *This = (IDirectXFileObjectImpl *)iface;

  TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IDirectXFileObject))
  {
    IClassFactory_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectXFileObjectImpl_AddRef(IDirectXFileObject* iface)
{
  IDirectXFileObjectImpl *This = (IDirectXFileObjectImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p/%p): AddRef from %d\n", iface, This, ref - 1);

  return ref;
}

static ULONG WINAPI IDirectXFileObjectImpl_Release(IDirectXFileObject* iface)
{
  IDirectXFileObjectImpl *This = (IDirectXFileObjectImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p/%p): ReleaseRef to %d\n", iface, This, ref);

  if (!ref)
    HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

/*** IDirectXFileObject methods ***/
static HRESULT WINAPI IDirectXFileObjectImpl_GetName(IDirectXFileObject* iface, LPSTR pstrNameBuf, LPDWORD pdwBufLen)
{
  IDirectXFileDataReferenceImpl *This = (IDirectXFileDataReferenceImpl *)iface;

  FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, pstrNameBuf, pdwBufLen); 

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileObjectImpl_GetId(IDirectXFileObject* iface, LPGUID pGuid)
{
  IDirectXFileObjectImpl *This = (IDirectXFileObjectImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, pGuid); 

  return DXFILEERR_BADVALUE;
}

static const IDirectXFileObjectVtbl IDirectXFileObject_Vtbl =
{
    IDirectXFileObjectImpl_QueryInterface,
    IDirectXFileObjectImpl_AddRef,
    IDirectXFileObjectImpl_Release,
    IDirectXFileObjectImpl_GetName,
    IDirectXFileObjectImpl_GetId
};

HRESULT IDirectXFileSaveObjectImpl_Create(IDirectXFileSaveObjectImpl** ppObj)
{
    IDirectXFileSaveObjectImpl* object;

    TRACE("(%p)\n", ppObj);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectXFileSaveObjectImpl));
    
    object->lpVtbl.lpVtbl = &IDirectXFileSaveObject_Vtbl;
    object->ref = 1;

    *ppObj = object;
    
    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectXFileSaveObjectImpl_QueryInterface(IDirectXFileSaveObject* iface, REFIID riid, void** ppvObject)
{
  IDirectXFileSaveObjectImpl *This = (IDirectXFileSaveObjectImpl *)iface;

  TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

  if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_IDirectXFileSaveObject))
  {
    IClassFactory_AddRef(iface);
    *ppvObject = This;
    return S_OK;
  }

  ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectXFileSaveObjectImpl_AddRef(IDirectXFileSaveObject* iface)
{
  IDirectXFileSaveObjectImpl *This = (IDirectXFileSaveObjectImpl *)iface;
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p/%p): AddRef from %d\n", iface, This, ref - 1);

  return ref;
}

static ULONG WINAPI IDirectXFileSaveObjectImpl_Release(IDirectXFileSaveObject* iface)
{
  IDirectXFileSaveObjectImpl *This = (IDirectXFileSaveObjectImpl *)iface;
  ULONG ref = InterlockedDecrement(&This->ref);

  TRACE("(%p/%p): ReleaseRef to %d\n", iface, This, ref);

  if (!ref)
    HeapFree(GetProcessHeap(), 0, This);

  return ref;
}

static HRESULT WINAPI IDirectXFileSaveObjectImpl_SaveTemplates(IDirectXFileSaveObject* iface, DWORD cTemplates, const GUID** ppguidTemplates)
{
  IDirectXFileSaveObjectImpl *This = (IDirectXFileSaveObjectImpl *)iface;

  FIXME("(%p/%p)->(%d,%p) stub!\n", This, iface, cTemplates, ppguidTemplates);

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileSaveObjectImpl_CreateDataObject(IDirectXFileSaveObject* iface, REFGUID rguidTemplate, LPCSTR szName, const GUID* pguid, DWORD cbSize, LPVOID pvData, LPDIRECTXFILEDATA* ppDataObj)
{
  IDirectXFileSaveObjectImpl *This = (IDirectXFileSaveObjectImpl *)iface;

  FIXME("(%p/%p)->(%p,%s,%p,%d,%p,%p) stub!\n", This, iface, rguidTemplate, szName, pguid, cbSize, pvData, ppDataObj);

  return DXFILEERR_BADVALUE;
}

static HRESULT WINAPI IDirectXFileSaveObjectImpl_SaveData(IDirectXFileSaveObject* iface, LPDIRECTXFILEDATA ppDataObj)
{
  IDirectXFileSaveObjectImpl *This = (IDirectXFileSaveObjectImpl *)iface;

  FIXME("(%p/%p)->(%p) stub!\n", This, iface, ppDataObj); 

  return DXFILEERR_BADVALUE;
}

static const IDirectXFileSaveObjectVtbl IDirectXFileSaveObject_Vtbl =
{
    IDirectXFileSaveObjectImpl_QueryInterface,
    IDirectXFileSaveObjectImpl_AddRef,
    IDirectXFileSaveObjectImpl_Release,
    IDirectXFileSaveObjectImpl_SaveTemplates,
    IDirectXFileSaveObjectImpl_CreateDataObject,
    IDirectXFileSaveObjectImpl_SaveData
};
