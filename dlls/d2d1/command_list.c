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
    D2D_COMMAND_SET_TEXT_RENDERING_PARAMS,
    D2D_COMMAND_SET_TRANSFORM,
    D2D_COMMAND_SET_PRIMITIVE_BLEND,
    D2D_COMMAND_SET_UNIT_MODE,
    D2D_COMMAND_CLEAR,
    D2D_COMMAND_DRAW_LINE,
    D2D_COMMAND_DRAW_GEOMETRY,
    D2D_COMMAND_DRAW_RECTANGLE,
    D2D_COMMAND_FILL_GEOMETRY,
    D2D_COMMAND_FILL_RECTANGLE,
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

struct d2d_command_set_text_rendering_params
{
    struct d2d_command c;
    IDWriteRenderingParams *params;
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

struct d2d_command_clear
{
    struct d2d_command c;
    D2D1_COLOR_F color;
};

struct d2d_command_push_clip
{
    struct d2d_command c;
    D2D1_RECT_F rect;
    D2D1_ANTIALIAS_MODE mode;
};

struct d2d_command_draw_line
{
    struct d2d_command c;
    D2D1_POINT_2F p0, p1;
    ID2D1Brush *brush;
    float stroke_width;
    ID2D1StrokeStyle *stroke_style;
};

struct d2d_command_draw_geometry
{
    struct d2d_command c;
    ID2D1Geometry *geometry;
    ID2D1Brush *brush;
    float stroke_width;
    ID2D1StrokeStyle *stroke_style;
};

struct d2d_command_draw_rectangle
{
    struct d2d_command c;
    D2D1_RECT_F rect;
    ID2D1Brush *brush;
    float stroke_width;
    ID2D1StrokeStyle *stroke_style;
};

struct d2d_command_fill_geometry
{
    struct d2d_command c;
    ID2D1Geometry *geometry;
    ID2D1Brush *brush;
    ID2D1Brush *opacity_brush;
};

struct d2d_command_fill_rectangle
{
    struct d2d_command c;
    D2D1_RECT_F rect;
    ID2D1Brush *brush;
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
    size_t i;

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        ID2D1Factory_Release(command_list->factory);
        for (i = 0; i < command_list->objects_count; ++i)
            IUnknown_Release(command_list->objects[i]);
        free(command_list->objects);
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
            case D2D_COMMAND_SET_TEXT_RENDERING_PARAMS:
            {
                const struct d2d_command_set_text_rendering_params *c = data;
                hr = ID2D1CommandSink_SetTextRenderingParams(sink, c->params);
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
            case D2D_COMMAND_CLEAR:
            {
                const struct d2d_command_clear *c = data;
                hr = ID2D1CommandSink_Clear(sink, &c->color);
                break;
            }
            case D2D_COMMAND_DRAW_LINE:
            {
                const struct d2d_command_draw_line *c = data;
                hr = ID2D1CommandSink_DrawLine(sink, c->p0, c->p1, c->brush, c->stroke_width,
                        c->stroke_style);
                break;
            }
            case D2D_COMMAND_DRAW_GEOMETRY:
            {
                const struct d2d_command_draw_geometry *c = data;
                hr = ID2D1CommandSink_DrawGeometry(sink, c->geometry, c->brush, c->stroke_width,
                        c->stroke_style);
                break;
            }
            case D2D_COMMAND_DRAW_RECTANGLE:
            {
                const struct d2d_command_draw_rectangle *c = data;
                hr = ID2D1CommandSink_DrawRectangle(sink, &c->rect, c->brush, c->stroke_width,
                        c->stroke_style);
                break;
            }
            case D2D_COMMAND_FILL_GEOMETRY:
            {
                const struct d2d_command_fill_geometry *c = data;
                hr = ID2D1CommandSink_FillGeometry(sink, c->geometry, c->brush, c->opacity_brush);
                break;
            }
            case D2D_COMMAND_FILL_RECTANGLE:
            {
                const struct d2d_command_fill_rectangle *c = data;
                hr = ID2D1CommandSink_FillRectangle(sink, &c->rect, c->brush);
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

static void d2d_command_list_reference_object(struct d2d_command_list *command_list, void *object)
{
    IUnknown *obj = object;

    if (!obj) return;

    if (!d2d_array_reserve((void **)&command_list->objects, &command_list->objects_capacity,
            command_list->objects_count + 1, sizeof(*command_list->objects)))
    {
        return;
    }

    command_list->objects[command_list->objects_count++] = obj;
    IUnknown_AddRef(obj);
}

static HRESULT d2d_command_list_create_brush(struct d2d_command_list *command_list,
        const struct d2d_device_context *ctx, ID2D1Brush *orig_brush, ID2D1Brush **ret)
{
    ID2D1DeviceContext *context = (ID2D1DeviceContext *)&ctx->ID2D1DeviceContext1_iface;
    struct d2d_brush *brush = unsafe_impl_from_ID2D1Brush(orig_brush);
    D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES linear_properties;
    D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES radial_properties;
    D2D1_BITMAP_BRUSH_PROPERTIES1 bitmap_properties;
    D2D1_IMAGE_BRUSH_PROPERTIES image_properties;
    D2D1_BRUSH_PROPERTIES properties;
    HRESULT hr;

    properties.opacity = brush->opacity;
    properties.transform = brush->transform;

    switch (brush->type)
    {
        case D2D_BRUSH_TYPE_SOLID:
            hr = ID2D1DeviceContext_CreateSolidColorBrush(context, &brush->u.solid.color,
                    &properties, (ID2D1SolidColorBrush **)ret);
            break;
        case D2D_BRUSH_TYPE_LINEAR:
            linear_properties.startPoint = brush->u.linear.start;
            linear_properties.endPoint = brush->u.linear.end;
            hr = ID2D1DeviceContext_CreateLinearGradientBrush(context, &linear_properties,
                    &properties, &brush->u.linear.gradient->ID2D1GradientStopCollection_iface,
                    (ID2D1LinearGradientBrush **)ret);
            break;
        case D2D_BRUSH_TYPE_RADIAL:
            radial_properties.center = brush->u.radial.centre;
            radial_properties.gradientOriginOffset = brush->u.radial.offset;
            radial_properties.radiusX = brush->u.radial.radius.x;
            radial_properties.radiusY = brush->u.radial.radius.y;
            hr = ID2D1DeviceContext_CreateRadialGradientBrush(context, &radial_properties,
                    &properties, &brush->u.radial.gradient->ID2D1GradientStopCollection_iface,
                    (ID2D1RadialGradientBrush **)ret);
            break;
        case D2D_BRUSH_TYPE_BITMAP:
            bitmap_properties.extendModeX = brush->u.bitmap.extend_mode_x;
            bitmap_properties.extendModeY = brush->u.bitmap.extend_mode_y;
            bitmap_properties.interpolationMode = brush->u.bitmap.interpolation_mode;
            hr = ID2D1DeviceContext_CreateBitmapBrush(context, (ID2D1Bitmap *)&brush->u.bitmap.bitmap->ID2D1Bitmap1_iface,
                    &bitmap_properties, &properties, (ID2D1BitmapBrush1 **)ret);
            break;
        case D2D_BRUSH_TYPE_IMAGE:
            image_properties.sourceRectangle = brush->u.image.source_rect;
            image_properties.extendModeX = brush->u.image.extend_mode_x;
            image_properties.extendModeY = brush->u.image.extend_mode_y;
            image_properties.interpolationMode = brush->u.image.interpolation_mode;
            hr = ID2D1DeviceContext_CreateImageBrush(context, brush->u.image.image,
                    &image_properties, &properties, (ID2D1ImageBrush **)ret);
            break;
        default:
            FIXME("Unsupported brush type %u.\n", brush->type);
            return E_UNEXPECTED;
    }

    if (SUCCEEDED(hr))
    {
        d2d_command_list_reference_object(command_list, *ret);
        ID2D1Brush_Release(*ret);
    }

    return hr;
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
    d2d_command_list_set_text_rendering_params(command_list, context->text_rendering_params);

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

void d2d_command_list_clear(struct d2d_command_list *command_list, const D2D1_COLOR_F *color)
{
    struct d2d_command_clear *command;

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->c.op = D2D_COMMAND_CLEAR;
    if (color) command->color = *color;
    else memset(&command->color, 0, sizeof(command->color));
}

void d2d_command_list_draw_line(struct d2d_command_list *command_list,
        const struct d2d_device_context *context, D2D1_POINT_2F p0, D2D1_POINT_2F p1,
        ID2D1Brush *orig_brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_command_draw_line *command;
    ID2D1Brush *brush;

    if (FAILED(d2d_command_list_create_brush(command_list, context, orig_brush, &brush)))
    {
        command_list->state = D2D_COMMAND_LIST_STATE_ERROR;
        return;
    }

    d2d_command_list_reference_object(command_list, stroke_style);

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->c.op = D2D_COMMAND_DRAW_LINE;
    command->p0 = p0;
    command->p1 = p1;
    command->brush = brush;
    command->stroke_width = stroke_width;
    command->stroke_style = stroke_style;
}

void d2d_command_list_draw_geometry(struct d2d_command_list *command_list,
        const struct d2d_device_context *context, ID2D1Geometry *geometry, ID2D1Brush *orig_brush,
        float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_command_draw_geometry *command;
    ID2D1Brush *brush;

    if (FAILED(d2d_command_list_create_brush(command_list, context, orig_brush, &brush)))
    {
        command_list->state = D2D_COMMAND_LIST_STATE_ERROR;
        return;
    }

    d2d_command_list_reference_object(command_list, geometry);
    d2d_command_list_reference_object(command_list, stroke_style);

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->c.op = D2D_COMMAND_DRAW_GEOMETRY;
    command->brush = brush;
    command->stroke_width = stroke_width;
    command->stroke_style = stroke_style;
}

void d2d_command_list_draw_rectangle(struct d2d_command_list *command_list, const struct d2d_device_context *context,
        const D2D1_RECT_F *rect, ID2D1Brush *orig_brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_command_draw_rectangle *command;
    ID2D1Brush *brush;

    if (FAILED(d2d_command_list_create_brush(command_list, context, orig_brush, &brush)))
    {
        command_list->state = D2D_COMMAND_LIST_STATE_ERROR;
        return;
    }

    d2d_command_list_reference_object(command_list, stroke_style);

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->c.op = D2D_COMMAND_DRAW_RECTANGLE;
    command->rect = *rect;
    command->brush = brush;
    command->stroke_width = stroke_width;
    command->stroke_style = stroke_style;
}

void d2d_command_list_fill_geometry(struct d2d_command_list *command_list,
        const struct d2d_device_context *context, ID2D1Geometry *geometry, ID2D1Brush *orig_brush,
        ID2D1Brush *orig_opacity_brush)
{
    ID2D1Brush *brush, *opacity_brush = NULL;
    struct d2d_command_fill_geometry *command;

    if (FAILED(d2d_command_list_create_brush(command_list, context, orig_brush, &brush)))
    {
        command_list->state = D2D_COMMAND_LIST_STATE_ERROR;
        return;
    }

    if (orig_opacity_brush && FAILED(d2d_command_list_create_brush(command_list, context,
            orig_opacity_brush, &opacity_brush)))
    {
        command_list->state = D2D_COMMAND_LIST_STATE_ERROR;
        ID2D1Brush_Release(brush);
        return;
    }

    d2d_command_list_reference_object(command_list, geometry);

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->c.op = D2D_COMMAND_FILL_GEOMETRY;
    command->geometry = geometry;
    command->brush = brush;
    command->opacity_brush = opacity_brush;
}

void d2d_command_list_fill_rectangle(struct d2d_command_list *command_list,
        const struct d2d_device_context *context, const D2D1_RECT_F *rect, ID2D1Brush *orig_brush)
{
    struct d2d_command_fill_rectangle *command;
    ID2D1Brush *brush;

    if (FAILED(d2d_command_list_create_brush(command_list, context, orig_brush, &brush)))
    {
        command_list->state = D2D_COMMAND_LIST_STATE_ERROR;
        return;
    }

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->c.op = D2D_COMMAND_FILL_RECTANGLE;
    command->rect = *rect;
    command->brush = brush;
}

void d2d_command_list_set_text_rendering_params(struct d2d_command_list *command_list,
        IDWriteRenderingParams *params)
{
    struct d2d_command_set_text_rendering_params *command;

    if (!params) return;

    d2d_command_list_reference_object(command_list, params);

    command = d2d_command_list_require_space(command_list, sizeof(*command));
    command->c.op = D2D_COMMAND_SET_TEXT_RENDERING_PARAMS;
    command->params = params;
}
