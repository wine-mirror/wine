/*
 * Copyright 2012 Jacek Caban for CodeWeavers
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

#ifndef JSVAL_H
#define JSVAL_H

typedef enum {
    JSV_UNDEFINED,
    JSV_NULL,
    JSV_OBJECT,
    JSV_STRING,
    JSV_NUMBER,
    JSV_BOOL,
    JSV_VARIANT
} jsval_type_t;

struct _jsval_t {
    jsval_type_t type;
    union {
        IDispatch *obj;
        BSTR str;
        double n;
        BOOL b;
        VARIANT *v;
    } u;
};

static inline jsval_t jsval_bool(BOOL b)
{
    jsval_t ret;
    ret.type = JSV_BOOL;
    ret.u.b = b;
    return ret;
}

static inline jsval_t jsval_string(BSTR str)
{
    jsval_t ret;
    ret.type = JSV_STRING;
    ret.u.str = str;
    return ret;
}

static inline jsval_t jsval_disp(IDispatch *obj)
{
    jsval_t ret;
    ret.type = JSV_OBJECT;
    ret.u.obj = obj;
    return ret;
}

static inline jsval_t jsval_obj(jsdisp_t *obj)
{
    return jsval_disp(to_disp(obj));
}

static inline jsval_t jsval_null(void)
{
    jsval_t ret = { JSV_NULL };
    return ret;
}

static inline jsval_t jsval_undefined(void)
{
    jsval_t ret = { JSV_UNDEFINED };
    return ret;
}

static inline jsval_t jsval_number(double n)
{
    jsval_t ret;
    ret.type = JSV_NUMBER;
    ret.u.n = n;
    return ret;
}

static inline jsval_type_t jsval_type(jsval_t v)
{
    return v.type;
}

static inline BOOL is_object_instance(jsval_t v)
{
    return v.type == JSV_OBJECT;
}

static inline BOOL is_undefined(jsval_t v)
{
    return v.type == JSV_UNDEFINED;
}

static inline BOOL is_null(jsval_t v)
{
    return v.type == JSV_NULL;
}

static inline BOOL is_null_instance(jsval_t v)
{
    return v.type == JSV_NULL || (v.type == JSV_OBJECT && !v.u.obj);
}

static inline BOOL is_string(jsval_t v)
{
    return v.type == JSV_STRING;
}

static inline BOOL is_number(jsval_t v)
{
    return v.type == JSV_NUMBER;
}

static inline BOOL is_variant(jsval_t v)
{
    return v.type == JSV_VARIANT;
}

static inline BOOL is_bool(jsval_t v)
{
    return v.type == JSV_BOOL;
}

static inline IDispatch *get_object(jsval_t v)
{
    return v.u.obj;
}

static inline double get_number(jsval_t v)
{
    return v.u.n;
}

static inline BSTR get_string(jsval_t v)
{
    return v.u.str;
}

static inline VARIANT *get_variant(jsval_t v)
{
    return v.u.v;
}

static inline BOOL get_bool(jsval_t v)
{
    return v.u.b;
}

HRESULT variant_to_jsval(VARIANT*,jsval_t*) DECLSPEC_HIDDEN;
HRESULT jsval_to_variant(jsval_t,VARIANT*) DECLSPEC_HIDDEN;
void jsval_release(jsval_t) DECLSPEC_HIDDEN;
HRESULT jsval_variant(jsval_t*,VARIANT*) DECLSPEC_HIDDEN;
HRESULT jsval_copy(jsval_t,jsval_t*) DECLSPEC_HIDDEN;

#endif
