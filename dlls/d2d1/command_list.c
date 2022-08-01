/*
 * Copyright 2022 Nikolay Sivov for CodeWeavers
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

#include "d2d1_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

enum d2d_command_type
{
    D2D_COMMAND_SET_ANTIALIAS_MODE,
    D2D_COMMAND_SET_TAGS,
    D2D_COMMAND_SET_TEXT_ANTIALIAS_MODE,
    D2D_COMMAND_SET_TRANSFORM,
    D2D_COMMAND_SET_PRIMITIVE_BLEND,
    D2D_COMMAND_SET_UNIT_MODE,
    D2D_COMMAND_PUSH_CLIP,
    D2D_COMMAND_POP_CLIP,
};

struct d2d_command
{
    enum d2d_command_type op;
    size_t size;
};

struct d2d_command_set_antialias_mode
{
    struct d2d_command c;
    D2D1_ANTIALIAS_MODE mode;
};

struct d2d_command_set_tags
{
    struct d2d_command c;
    D2D1_TAG tag1, tag2;
};

struct d2d_command_set_text_antialias_mode
{
    struct d2d_command c;
    D2D1_TEXT_ANTIALIAS_MODE mode;
};

struct d2d_command_set_transform
{
    struct d2d_command c;
    D2D1_MATRIX_3X2_F transform;
};

struct d2d_command_set_primitive_blend
{
    struct d2d_command c;
    D2D1_PRIMITIVE_BLEND primitive_blend;
};

struct d2d_command_set_unit_mode
{
    struct d2d_command c;
    D2D1_UNIT_MODE mode;
};

struct d2d_command_push_clip
{
    struct d2d_command c;
    D2D1_RECT_F rect;
    D2D1_ANTIALIAS_MODE mode;
};

static inline struct d2d_command_list *impl_from_ID2D1CommandList(ID2D1CommandList *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_command_list, ID2D1CommandList_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_command_list_QueryInterface(ID2D1CommandList *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1CommandList)
            || IsEqualGUID(iid, &IID_ID2D1Image)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1CommandList_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_command_list_AddRef(ID2D1CommandList *iface)
{
    struct d2d_command_list *command_list = impl_from_ID2D1CommandList(iface);
    ULONG refcount = InterlockedIncrement(&command_list->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_command_list_Release(ID2D1CommandList *iface)
{
    struct d2d_command_list *command_list = impl_from_ID2D1CommandList(iface);
    ULONG refcount = InterlockedDecrement(&command_list->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        ID2D1Factory_Release(command_list->factory);
        free(command_list->data);
        free(command_list);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_command_list_GetFactory(ID2D1CommandList *iface, ID2D1Factory **factory)
{
    struct d2d_command_list *command_list = impl_from_ID2D1CommandList(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = command_list->factory);
}

static HRESULT STDMETHODCALLTYPE d2d_command_list_Stream(ID2D1CommandList *iface, ID2D1CommandSink *sink)
{
    struct d2d_command_list *command_list = impl_from_ID2D1CommandList(iface);
    const void *data, *end;
    HRESULT hr;

    TRACE("iface %p, sink %p.\n", iface, sink);

    if (command_list->state != D2D_COMMAND_LIST_STATE_CLOSED) return S_OK;

    if (FAILED(hr = ID2D1CommandSink_BeginDraw(sink)))
        return hr;

    data = command_list->data;
    end = (char *)command_list->data + command_list->size;
    while (data < end)
    {
        const struct d2d_command *command = data;

        switch (command->op)
        {
            case D2D_COMMAND_SET_ANTIALIAS_MODE:
            {
                const struct d2d_command_set_antialias_mode *c = data;
                hr = ID2D1CommandSink_SetAntialiasMode(sink, c->mode);
                break;
            }
            case D2D_COMMAND_SET_TAGS:
            {
                const struct d2d_command_set_tags *c = data;
                hr = ID2D1CommandSink_SetTags(sink, c->tag1, c->tag2);
                break;
            }
            case D2D_COMMAND_SET_TEXT_ANTIALIAS_MODE:
            {
                const struct d2d_command_set_text_antialias_mode *c = data;
                hr = ID2D1CommandSink_SetTextAntialiasMode(sink, c->mode);
                break;
            }
            case D2D_COMMAND_SET_TRANSFORM:
            {
                const struct d2d_command_set_transform *c = data;
                hr = ID2D1CommandSink_SetTransform(sink, &c->transform);
                break;
            }
            case D2D_COMMAND_SET_PRIMITIVE_BLEND:
            {
                const struct d2d_command_set_primitive_blend *c = data;
                hr = ID2D1CommandSink_SetPrimitiveBlend(sink, c->primitive_blend);
                break;
            }
            case D2D_COMMAND_SET_UNIT_MODE:
            {
                const struct d2d_command_set_unit_mode *c = data;
                hr = ID2D1CommandSink_SetUnitMode(sink, c->mode);
                break;
            }
            case D2D_COMMAND_PUSH_CLIP:
            {
                const struct d2d_command_push_clip *c = data;
                hr = ID2D1CommandSink_PushAxisAlignedClip(sink, &c->rect, c->mode);
                break;
            }
            case D2D_COMMAND_POP_CLIP:
                hr = ID2D1CommandSink_PopAxisAlignedClip(sink);
                break;
            default:
                FIXME("Unhandled command %u.\n", command->op);
                hr = E_UNEXPECTED;
        }

        if (FAILED(hr)) return hr;

        data = (char *)data + command->size;
    }

    return ID2D1CommandSink_EndDraw(sink);
}

static HRESULT STDMETHODCALLTYPE d2d_command_list_Close(ID2D1CommandList *iface)
{
    struct d2d_command_list *command_list = impl_from_ID2D1CommandList(iface);

    FIXME("iface %p stub.\n", iface);

    if (command_list->state != D2D_COMMAND_LIST_STATE_OPEN)
        return D2DERR_WRONG_STATE;

    command_list->state = D2D_COMMAND_LIST_STATE_CLOSED;

    /* FIXME: reset as a target */

    return S_OK;
}

static const ID2D1CommandListVtbl d2d_command_list_vtbl =
{
    d2d_command_list_QueryInterface,
    d2d_command_list_AddRef,
    d2d_command_list_Release,
    d2d_command_list_GetFactory,
    d2d_command_list_Stream,
    d2d_command_list_Close,
};

HRESULT d2d_command_list_create(ID2D1Factory *factory, struct d2d_command_list **command_list)
{
    if (!(*command_list = calloc(1, sizeof(**command_list))))
        return E_OUTOFMEMORY;

    (*command_list)->ID2D1CommandList_iface.lpVtbl = &d2d_command_list_vtbl;
    (*command_list)->refcount = 1;
    ID2D1Factory_AddRef((*command_list)->factory = factory);

    TRACE("Created command list %p.\n", *command_list);

    return S_OK;
}

struct d2d_command_list *unsafe_impl_from_ID2D1CommandList(ID2D1CommandList *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (ID2D1CommandListVtbl *)&d2d_command_list_vtbl);
    return CONTAINING_RECORD(iface, struct d2d_command_list, ID2D1CommandList_iface);
}

static void * d2d_command_list_require_space(struct d2d_command_list *command_list, size_t size)
{
    struct d2d_command *command;

    if (!d2d_array_reserve(&command_list->data, &command_list->capacity, command_list->size + size, 1))
        return NULL;

    command = (struct d2d_command *)((char *)command_list->data + command_list->size);
    command->size = size;

    command_list->size += size;

    return command;
}

void d2d_command_list_set_antialias_mode(struct d2d_command_list *command_list,
        D2D1_ANTIALIAS_MODE mode)
{
    struct d2d_command_set_antialias_mode *command;

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->c.op = D2D_COMMAND_SET_ANTIALIAS_MODE;
    command->mode = mode;
}

void d2d_command_list_set_primitive_blend(struct d2d_command_list *command_list,
        D2D1_PRIMITIVE_BLEND primitive_blend)
{
    struct d2d_command_set_primitive_blend *command;

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->c.op = D2D_COMMAND_SET_PRIMITIVE_BLEND;
    command->primitive_blend = primitive_blend;
}

void d2d_command_list_set_unit_mode(struct d2d_command_list *command_list, D2D1_UNIT_MODE mode)
{
    struct d2d_command_set_unit_mode *command;

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->c.op = D2D_COMMAND_SET_UNIT_MODE;
    command->mode = mode;
}

void d2d_command_list_set_text_antialias_mode(struct d2d_command_list *command_list,
        D2D1_TEXT_ANTIALIAS_MODE mode)
{
    struct d2d_command_set_text_antialias_mode *command;

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->c.op = D2D_COMMAND_SET_TEXT_ANTIALIAS_MODE;
    command->mode = mode;
}

void d2d_command_list_set_tags(struct d2d_command_list *command_list, D2D1_TAG tag1, D2D1_TAG tag2)
{
    struct d2d_command_set_tags *command;

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->c.op = D2D_COMMAND_SET_TAGS;
    command->tag1 = tag1;
    command->tag2 = tag2;
}

void d2d_command_list_set_transform(struct d2d_command_list *command_list,
        const D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_command_set_transform *command;

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->c.op = D2D_COMMAND_SET_TRANSFORM;
    command->transform = *transform;
}

void d2d_command_list_begin_draw(struct d2d_command_list *command_list,
        const struct d2d_device_context *context)
{
    if (command_list->state != D2D_COMMAND_LIST_STATE_INITIAL)
    {
        command_list->state = D2D_COMMAND_LIST_STATE_ERROR;
        return;
    }

    d2d_command_list_set_antialias_mode(command_list, context->drawing_state.antialiasMode);
    d2d_command_list_set_primitive_blend(command_list, context->drawing_state.primitiveBlend);
    d2d_command_list_set_unit_mode(command_list, context->drawing_state.unitMode);
    d2d_command_list_set_text_antialias_mode(command_list, context->drawing_state.textAntialiasMode);
    d2d_command_list_set_tags(command_list, context->drawing_state.tag1, context->drawing_state.tag2);
    d2d_command_list_set_transform(command_list, &context->drawing_state.transform);

    command_list->state = D2D_COMMAND_LIST_STATE_OPEN;
}

void d2d_command_list_push_clip(struct d2d_command_list *command_list, const D2D1_RECT_F *rect,
        D2D1_ANTIALIAS_MODE mode)
{
    struct d2d_command_push_clip *command;

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->c.op = D2D_COMMAND_PUSH_CLIP;
    command->rect = *rect;
    command->mode = mode;
}

void d2d_command_list_pop_clip(struct d2d_command_list *command_list)
{
    struct d2d_command *command;

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->op = D2D_COMMAND_POP_CLIP;
}
