/* IDirectMusicScript
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 * Copyright (C) 2003-2004 Raphael Junqueira
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include <stdio.h>

#include "dmscript_private.h"
#include "dmobject.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmscript);

/*****************************************************************************
 * IDirectMusicScript implementation
 */
struct script {
    IDirectMusicScript IDirectMusicScript_iface;
    struct dmobject dmobj;
    LONG ref;
    IDirectMusicPerformance *pPerformance;
    DMUS_IO_SCRIPT_HEADER header;
    DMUS_IO_VERSION version;
    WCHAR *lang;
    WCHAR *source;
};

static inline struct script *impl_from_IDirectMusicScript(IDirectMusicScript *iface)
{
  return CONTAINING_RECORD(iface, struct script, IDirectMusicScript_iface);
}

static HRESULT WINAPI IDirectMusicScriptImpl_QueryInterface(IDirectMusicScript *iface, REFIID riid,
        void **ret_iface)
{
    struct script *This = impl_from_IDirectMusicScript(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IDirectMusicScript))
        *ret_iface = iface;
    else if (IsEqualIID(riid, &IID_IDirectMusicObject))
        *ret_iface = &This->dmobj.IDirectMusicObject_iface;
    else if (IsEqualIID(riid, &IID_IPersistStream))
        *ret_iface = &This->dmobj.IPersistStream_iface;
    else {
        WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ret_iface);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ret_iface);
    return S_OK;
}

static ULONG WINAPI IDirectMusicScriptImpl_AddRef(IDirectMusicScript *iface)
{
    struct script *This = impl_from_IDirectMusicScript(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectMusicScriptImpl_Release(IDirectMusicScript *iface)
{
    struct script *This = impl_from_IDirectMusicScript(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref) {
        free(This->lang);
        free(This->source);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI IDirectMusicScriptImpl_Init(IDirectMusicScript *iface,
        IDirectMusicPerformance *pPerformance, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  struct script *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %p, %p): stub\n", This, pPerformance, pErrorInfo);
  This->pPerformance = pPerformance;
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_CallRoutine(IDirectMusicScript *iface,
        WCHAR *pwszRoutineName, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  struct script *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %p): stub\n", This, debugstr_w(pwszRoutineName), pErrorInfo);
  /*return E_NOTIMPL;*/
  return S_OK;
  /*return E_FAIL;*/
}

static HRESULT WINAPI IDirectMusicScriptImpl_SetVariableVariant(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, VARIANT varValue, BOOL fSetRef, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  struct script *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, FIXME, %d, %p): stub\n", This, debugstr_w(pwszVariableName),/* varValue,*/ fSetRef, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_GetVariableVariant(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, VARIANT *pvarValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  struct script *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %p, %p): stub\n", This, debugstr_w(pwszVariableName), pvarValue, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_SetVariableNumber(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, LONG lValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  struct script *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %li, %p): stub\n", This, debugstr_w(pwszVariableName), lValue, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_GetVariableNumber(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, LONG *plValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  struct script *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %p, %p): stub\n", This, debugstr_w(pwszVariableName), plValue, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_SetVariableObject(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, IUnknown *punkValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  struct script *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %p, %p): stub\n", This, debugstr_w(pwszVariableName), punkValue, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_GetVariableObject(IDirectMusicScript *iface,
        WCHAR *pwszVariableName, REFIID riid, void **ppv, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
  struct script *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %s, %s, %p, %p): stub\n", This, debugstr_w(pwszVariableName), debugstr_dmguid(riid), ppv, pErrorInfo);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_EnumRoutine(IDirectMusicScript *iface, DWORD dwIndex,
        WCHAR *pwszName)
{
  struct script *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);
  return S_OK;
}

static HRESULT WINAPI IDirectMusicScriptImpl_EnumVariable(IDirectMusicScript *iface, DWORD dwIndex,
        WCHAR *pwszName)
{
  struct script *This = impl_from_IDirectMusicScript(iface);
  FIXME("(%p, %ld, %p): stub\n", This, dwIndex, pwszName);
  return S_OK;
}

static const IDirectMusicScriptVtbl dmscript_vtbl = {
    IDirectMusicScriptImpl_QueryInterface,
    IDirectMusicScriptImpl_AddRef,
    IDirectMusicScriptImpl_Release,
    IDirectMusicScriptImpl_Init,
    IDirectMusicScriptImpl_CallRoutine,
    IDirectMusicScriptImpl_SetVariableVariant,
    IDirectMusicScriptImpl_GetVariableVariant,
    IDirectMusicScriptImpl_SetVariableNumber,
    IDirectMusicScriptImpl_GetVariableNumber,
    IDirectMusicScriptImpl_SetVariableObject,
    IDirectMusicScriptImpl_GetVariableObject,
    IDirectMusicScriptImpl_EnumRoutine,
    IDirectMusicScriptImpl_EnumVariable
};

static HRESULT WINAPI script_IDirectMusicObject_ParseDescriptor(IDirectMusicObject *iface,
        IStream *stream, DMUS_OBJECTDESC *desc)
{
    struct chunk_entry riff = {0};
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", iface, stream, desc);

    if (!stream || !desc)
        return E_POINTER;

     if ((hr = stream_get_chunk(stream, &riff)) != S_OK)
        return hr;
    if (riff.id != FOURCC_RIFF || riff.type != DMUS_FOURCC_SCRIPT_FORM) {
        TRACE("loading failed: unexpected %s\n", debugstr_chunk(&riff));
        stream_skip_chunk(stream, &riff);
        return DMUS_E_SCRIPT_INVALID_FILE;
    }

    hr = dmobj_parsedescriptor(stream, &riff, desc,
            DMUS_OBJ_OBJECT|DMUS_OBJ_NAME|DMUS_OBJ_NAME_INAM|DMUS_OBJ_CATEGORY|DMUS_OBJ_VERSION);
    if (FAILED(hr))
        return hr;

    if (desc->dwValidData) {
        desc->guidClass = CLSID_DirectMusicScript;
        desc->dwValidData |= DMUS_OBJ_CLASS;
    }

    TRACE("returning descriptor:\n");
    dump_DMUS_OBJECTDESC(desc);
    return S_OK;
}

static const IDirectMusicObjectVtbl dmobject_vtbl = {
    dmobj_IDirectMusicObject_QueryInterface,
    dmobj_IDirectMusicObject_AddRef,
    dmobj_IDirectMusicObject_Release,
    dmobj_IDirectMusicObject_GetDescriptor,
    dmobj_IDirectMusicObject_SetDescriptor,
    script_IDirectMusicObject_ParseDescriptor
};

static inline struct script *impl_from_IPersistStream(IPersistStream *iface)
{
    return CONTAINING_RECORD(iface, struct script, dmobj.IPersistStream_iface);
}

static HRESULT load_container(IStream *stream, struct chunk_entry *riff)
{
    DMUS_OBJECTDESC desc = {.dwSize = sizeof(DMUS_OBJECTDESC), .dwValidData = DMUS_OBJ_STREAM | DMUS_OBJ_CLASS,
                            .guidClass = CLSID_DirectMusicContainer};
    IDirectMusicObject *dmobj;
    HRESULT hr;

    if (FAILED(hr = IStream_Clone(stream, &desc.pStream)))
    {
        ERR("Failed to clone IStream (%p): hr %#lx.\n", stream, hr);
        return hr;
    }
    if (SUCCEEDED(hr = stream_reset_chunk_start(desc.pStream, riff)))
    {
        if (SUCCEEDED(hr = stream_get_object(desc.pStream, &desc, &IID_IDirectMusicObject, (void **)&dmobj)))
            IDirectMusicObject_Release(dmobj);
        else
            ERR("Failed to load container: hr %#lx.\n", hr);
    }

    IStream_Release(desc.pStream);

    return hr;
}

static HRESULT WINAPI script_persist_stream_Load(IPersistStream *iface, IStream *stream)
{
    struct script *This = impl_from_IPersistStream(iface);
    struct chunk_entry script = {0};
    struct chunk_entry chunk = {.parent = &script};
    HRESULT hr;

    TRACE("(%p, %p): Loading\n", This, stream);

    if (!stream)
        return E_POINTER;

    if (stream_get_chunk(stream, &script) != S_OK || script.id != FOURCC_RIFF ||
            script.type != DMUS_FOURCC_SCRIPT_FORM)
    {
        WARN("Invalid chunk %s\n", debugstr_chunk(&script));
        return DMUS_E_UNSUPPORTED_STREAM;
    }
    if (FAILED(hr = dmobj_parsedescriptor(stream, &script, &This->dmobj.desc, DMUS_OBJ_OBJECT))
                || FAILED(hr = stream_reset_chunk_data(stream, &script)))
        return hr;

    while ((hr = stream_next_chunk(stream, &chunk)) == S_OK)
    {
        switch (MAKE_IDTYPE(chunk.id, chunk.type))
        {
            case DMUS_FOURCC_GUID_CHUNK:
            case DMUS_FOURCC_VERSION_CHUNK:
            case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_UNFO_LIST):
                /* already parsed/ignored by dmobj_parsedescriptor */
                break;
            case DMUS_FOURCC_SCRIPT_CHUNK:
                hr = stream_chunk_get_data(stream, &chunk, &This->header, sizeof(This->header));
                if (SUCCEEDED(hr))
                    TRACE("header dwFlags: %#lx\n", This->header.dwFlags);
                break;
            case DMUS_FOURCC_SCRIPTVERSION_CHUNK:
                hr = stream_chunk_get_data(stream, &chunk, &This->version, sizeof(This->version));
                if (SUCCEEDED(hr))
                    TRACE("version: %#lx.%#lx\n", This->version.dwVersionMS, This->version.dwVersionLS);
                break;
            case MAKE_IDTYPE(FOURCC_RIFF, DMUS_FOURCC_CONTAINER_FORM):
                hr = load_container(stream, &chunk);
                break;
            case DMUS_FOURCC_SCRIPTLANGUAGE_CHUNK:
                if (This->lang)
                    free(This->lang);
                This->lang = calloc(1, chunk.size);
                hr = stream_chunk_get_wstr(stream, &chunk, This->lang, chunk.size);
                if (SUCCEEDED(hr))
                    TRACE("language: %s\n", debugstr_w(This->lang));
                break;
            case DMUS_FOURCC_SCRIPTSOURCE_CHUNK:
                if (This->source)
                    free(This->source);
                This->source = calloc(1, chunk.size);
                hr = stream_chunk_get_wstr(stream, &chunk, This->source, chunk.size);
                if (SUCCEEDED(hr))
                    TRACE("source: %s\n", debugstr_w(This->source));
                break;
            case MAKE_IDTYPE(FOURCC_LIST, DMUS_FOURCC_REF_LIST):
                FIXME("Loading the script source from file not implement.");
                break;
            default:
                WARN("Ignoring chunk %s\n", debugstr_chunk(&chunk));
                break;
        }

        if (FAILED(hr))
            return hr;
    }

    return S_OK;
}

static const IPersistStreamVtbl persiststream_vtbl = {
    dmobj_IPersistStream_QueryInterface,
    dmobj_IPersistStream_AddRef,
    dmobj_IPersistStream_Release,
    unimpl_IPersistStream_GetClassID,
    unimpl_IPersistStream_IsDirty,
    script_persist_stream_Load,
    unimpl_IPersistStream_Save,
    unimpl_IPersistStream_GetSizeMax
};

/* for ClassFactory */
HRESULT DMUSIC_CreateDirectMusicScriptImpl(REFIID lpcGUID, void **ppobj, IUnknown *pUnkOuter)
{
  struct script *obj;
  HRESULT hr;

  *ppobj = NULL;

  if (pUnkOuter)
    return CLASS_E_NOAGGREGATION;

  if (!(obj = calloc(1, sizeof(*obj)))) return E_OUTOFMEMORY;
  obj->IDirectMusicScript_iface.lpVtbl = &dmscript_vtbl;
  obj->ref = 1;
  dmobject_init(&obj->dmobj, &CLSID_DirectMusicScript, (IUnknown*)&obj->IDirectMusicScript_iface);
  obj->dmobj.IDirectMusicObject_iface.lpVtbl = &dmobject_vtbl;
  obj->dmobj.IPersistStream_iface.lpVtbl = &persiststream_vtbl;

  hr = IDirectMusicScript_QueryInterface(&obj->IDirectMusicScript_iface, lpcGUID, ppobj);
  IDirectMusicScript_Release(&obj->IDirectMusicScript_iface);

  return hr;
}
