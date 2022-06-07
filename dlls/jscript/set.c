/*
 * Copyright 2021 Jacek Caban for CodeWeavers
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

#include <assert.h>
#include <math.h>

#include "jscript.h"

#include "wine/rbtree.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(jscript);

typedef struct {
    jsdisp_t dispex;
    struct wine_rb_tree map;
    struct list entries;
    size_t size;
} MapInstance;

struct jsval_map_entry {
    struct wine_rb_entry entry;
    jsval_t key;
    jsval_t value;

    /*
     * We need to maintain a list as well to support traversal in forEach.
     * If the entry is removed while being processed by forEach, it's
     * still kept in the list and released later, when it's safe.
     */
    struct list list_entry;
    unsigned int ref;
    BOOL deleted;
};

static int jsval_map_compare(const void *k, const struct wine_rb_entry *e)
{
    const struct jsval_map_entry *entry = WINE_RB_ENTRY_VALUE(e, const struct jsval_map_entry, entry);
    const jsval_t *key = k;
    union {
        double d;
        INT64 n;
    } bits1, bits2;

    if(jsval_type(entry->key) != jsval_type(*key))
        return (int)jsval_type(entry->key) - (int)jsval_type(*key);

    switch(jsval_type(*key)) {
    case JSV_UNDEFINED:
    case JSV_NULL:
        return 0;
    case JSV_OBJECT:
        if(get_object(*key) == get_object(entry->key)) return 0;
        return get_object(*key) < get_object(entry->key) ? -1 : 1;
    case JSV_STRING:
        return jsstr_cmp(get_string(*key), get_string(entry->key));
    case JSV_NUMBER:
        if(isnan(get_number(*key))) return isnan(get_number(entry->key)) ? 0 : -1;
        if(isnan(get_number(entry->key))) return 1;

        /* native treats -0 differently than 0, so need to compare bitwise */
        bits1.d = get_number(*key);
        bits2.d = get_number(entry->key);
        return (bits1.n == bits2.n) ? 0 : (bits1.n < bits2.n ? -1 : 1);
    case JSV_BOOL:
        if(get_bool(*key) == get_bool(entry->key)) return 0;
        return get_bool(*key) ? 1 : -1;
    default:
        assert(0);
        return 0;
    }
}

static HRESULT get_map_this(script_ctx_t *ctx, jsval_t vthis, MapInstance **ret)
{
    jsdisp_t *jsdisp;

    if(!is_object_instance(vthis))
        return JS_E_OBJECT_EXPECTED;
    if(!(jsdisp = to_jsdisp(get_object(vthis))) || !is_class(jsdisp, JSCLASS_MAP)) {
        WARN("not a Map object passed as 'this'\n");
        return throw_error(ctx, JS_E_WRONG_THIS, L"Map");
    }

    *ret = CONTAINING_RECORD(jsdisp, MapInstance, dispex);
    return S_OK;
}

static HRESULT get_set_this(script_ctx_t *ctx, jsval_t vthis, MapInstance **ret)
{
    jsdisp_t *jsdisp;

    if(!is_object_instance(vthis))
        return JS_E_OBJECT_EXPECTED;
    if(!(jsdisp = to_jsdisp(get_object(vthis))) || !is_class(jsdisp, JSCLASS_SET)) {
        WARN("not a Set object passed as 'this'\n");
        return throw_error(ctx, JS_E_WRONG_THIS, L"Set");
    }

    *ret = CONTAINING_RECORD(jsdisp, MapInstance, dispex);
    return S_OK;
}

static struct jsval_map_entry *get_map_entry(MapInstance *map, jsval_t key)
{
    struct wine_rb_entry *entry;
    if(!(entry = wine_rb_get(&map->map, &key))) return NULL;
    return CONTAINING_RECORD(entry, struct jsval_map_entry, entry);
}

static void grab_map_entry(struct jsval_map_entry *entry)
{
    entry->ref++;
}

static void release_map_entry(struct jsval_map_entry *entry)
{
    if(--entry->ref) return;
    jsval_release(entry->key);
    jsval_release(entry->value);
    list_remove(&entry->list_entry);
    heap_free(entry);
}

static void delete_map_entry(MapInstance *map, struct jsval_map_entry *entry)
{
    map->size--;
    wine_rb_remove(&map->map, &entry->entry);
    entry->deleted = TRUE;
    release_map_entry(entry);
}

static HRESULT set_map_entry(MapInstance *map, jsval_t key, jsval_t value, jsval_t *r)
{
    struct jsval_map_entry *entry;
    HRESULT hres;

    if((entry = get_map_entry(map, key))) {
        jsval_t val;
        hres = jsval_copy(value, &val);
        if(FAILED(hres))
            return hres;

        jsval_release(entry->value);
        entry->value = val;
    }else {
        if(!(entry = heap_alloc_zero(sizeof(*entry)))) return E_OUTOFMEMORY;

        hres = jsval_copy(key, &entry->key);
        if(SUCCEEDED(hres)) {
            hres = jsval_copy(value, &entry->value);
            if(FAILED(hres))
                jsval_release(entry->key);
        }
        if(FAILED(hres)) {
            heap_free(entry);
            return hres;
        }
        grab_map_entry(entry);
        wine_rb_put(&map->map, &entry->key, &entry->entry);
        list_add_tail(&map->entries, &entry->list_entry);
        map->size++;
    }

    if(r) *r = jsval_undefined();
    return S_OK;
}

static HRESULT iterate_map(MapInstance *map, script_ctx_t *ctx, unsigned argc, jsval_t *argv, jsval_t *r)
{
    struct list *iter = list_head(&map->entries);
    IDispatch *context_obj = NULL;
    HRESULT hres;

    if(!argc || !is_object_instance(argv[0])) {
        FIXME("invalid callback %s\n", debugstr_jsval(argc ? argv[0] : jsval_undefined()));
        return E_FAIL;
    }

    if(argc > 1 && !is_undefined(argv[1])) {
        if(!is_object_instance(argv[1])) {
            FIXME("Unsupported context this %s\n", debugstr_jsval(argv[1]));
            return E_NOTIMPL;
        }
        context_obj = get_object(argv[1]);
    }

    while(iter) {
        struct jsval_map_entry *entry = LIST_ENTRY(iter, struct jsval_map_entry, list_entry);
        jsval_t args[3], v;

        if(entry->deleted) {
            iter = list_next(&map->entries, iter);
            continue;
        }

        args[0] = entry->value;
        args[1] = entry->key;
        args[2] = jsval_obj(&map->dispex);
        grab_map_entry(entry);
        hres = disp_call_value(ctx, get_object(argv[0]), context_obj,
                               DISPATCH_METHOD, ARRAY_SIZE(args), args, &v);
        iter = list_next(&map->entries, iter);
        release_map_entry(entry);
        if(FAILED(hres))
            return hres;
        jsval_release(v);
    }

    if(r) *r = jsval_undefined();
    return S_OK;
}

static HRESULT Map_clear(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    struct jsval_map_entry *entry, *entry2;
    MapInstance *map;
    HRESULT hres;

    hres = get_map_this(ctx, vthis, &map);
    if(FAILED(hres))
        return hres;

    TRACE("%p\n", map);

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &map->entries, struct jsval_map_entry, list_entry)
        delete_map_entry(map, entry);

    if(r) *r = jsval_undefined();
    return S_OK;
}

static HRESULT Map_delete(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t key = argc >= 1 ? argv[0] : jsval_undefined();
    struct jsval_map_entry *entry;
    MapInstance *map;
    HRESULT hres;

    hres = get_map_this(ctx, vthis, &map);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", map, debugstr_jsval(key));

    if((entry = get_map_entry(map, key))) delete_map_entry(map, entry);
    if(r) *r = jsval_bool(!!entry);
    return S_OK;
}

static HRESULT Map_forEach(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    MapInstance *map;
    HRESULT hres;

    hres = get_map_this(ctx, vthis, &map);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", map, debugstr_jsval(argc >= 1 ? argv[0] : jsval_undefined()));

    return iterate_map(map, ctx, argc, argv, r);
}

static HRESULT Map_get(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t key = argc >= 1 ? argv[0] : jsval_undefined();
    struct jsval_map_entry *entry;
    MapInstance *map;
    HRESULT hres;

    hres = get_map_this(ctx, vthis, &map);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", map, debugstr_jsval(key));

    if(!(entry = get_map_entry(map, key))) {
        if(r) *r = jsval_undefined();
        return S_OK;
    }

    return r ? jsval_copy(entry->value, r) : S_OK;
}

static HRESULT Map_set(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t key = argc >= 1 ? argv[0] : jsval_undefined();
    jsval_t value = argc >= 2 ? argv[1] : jsval_undefined();
    MapInstance *map;
    HRESULT hres;

    hres = get_map_this(ctx, vthis, &map);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s %s)\n", map, debugstr_jsval(key), debugstr_jsval(value));

    return set_map_entry(map, key, value, r);
}

static HRESULT Map_has(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t key = argc >= 1 ? argv[0] : jsval_undefined();
    struct jsval_map_entry *entry;
    MapInstance *map;
    HRESULT hres;

    hres = get_map_this(ctx, vthis, &map);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", map, debugstr_jsval(key));

    entry = get_map_entry(map, key);
    if(r) *r = jsval_bool(!!entry);
    return S_OK;
}

static HRESULT Map_get_size(script_ctx_t *ctx, jsdisp_t *jsthis, jsval_t *r)
{
    MapInstance *map = (MapInstance*)jsthis;

    TRACE("%p\n", map);

    *r = jsval_number(map->size);
    return S_OK;
}

static HRESULT Map_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static void Map_destructor(jsdisp_t *dispex)
{
    MapInstance *map = (MapInstance*)dispex;

    while(!list_empty(&map->entries)) {
        struct jsval_map_entry *entry = LIST_ENTRY(list_head(&map->entries),
                                                   struct jsval_map_entry, list_entry);
        assert(!entry->deleted);
        release_map_entry(entry);
    }

    heap_free(map);
}
static const builtin_prop_t Map_prototype_props[] = {
    {L"clear",      Map_clear,     PROPF_METHOD},
    {L"delete" ,    Map_delete,    PROPF_METHOD|1},
    {L"forEach",    Map_forEach,   PROPF_METHOD|1},
    {L"get",        Map_get,       PROPF_METHOD|1},
    {L"has",        Map_has,       PROPF_METHOD|1},
    {L"set",        Map_set,       PROPF_METHOD|2},
};

static const builtin_prop_t Map_props[] = {
    {L"size",       NULL,0,        Map_get_size, builtin_set_const},
};

static const builtin_info_t Map_prototype_info = {
    JSCLASS_OBJECT,
    Map_value,
    ARRAY_SIZE(Map_prototype_props),
    Map_prototype_props,
    NULL,
    NULL
};

static const builtin_info_t Map_info = {
    JSCLASS_MAP,
    Map_value,
    ARRAY_SIZE(Map_props),
    Map_props,
    Map_destructor,
    NULL
};

static HRESULT Map_constructor(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    MapInstance *map;
    HRESULT hres;

    switch(flags) {
    case DISPATCH_CONSTRUCT:
        TRACE("\n");

        if(!r)
            return S_OK;
        if(!(map = heap_alloc_zero(sizeof(*map))))
            return E_OUTOFMEMORY;

        hres = init_dispex(&map->dispex, ctx, &Map_info, ctx->map_prototype);
        if(FAILED(hres))
            return hres;

        wine_rb_init(&map->map, jsval_map_compare);
        list_init(&map->entries);
        *r = jsval_obj(&map->dispex);
        return S_OK;

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }
}

static HRESULT Set_add(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t key = argc ? argv[0] : jsval_undefined();
    MapInstance *set;
    HRESULT hres;

    hres = get_set_this(ctx, vthis, &set);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", set, debugstr_jsval(key));

    return set_map_entry(set, key, key, r);
}

static HRESULT Set_clear(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    struct jsval_map_entry *entry, *entry2;
    MapInstance *set;
    HRESULT hres;

    hres = get_set_this(ctx, vthis, &set);
    if(FAILED(hres))
        return hres;

    TRACE("%p\n", set);

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &set->entries, struct jsval_map_entry, list_entry)
        delete_map_entry(set, entry);

    if(r) *r = jsval_undefined();
    return S_OK;
}

static HRESULT Set_delete(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t key = argc ? argv[0] : jsval_undefined();
    struct jsval_map_entry *entry;
    MapInstance *set;
    HRESULT hres;

    hres = get_set_this(ctx, vthis, &set);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", set, debugstr_jsval(key));

    if((entry = get_map_entry(set, key))) delete_map_entry(set, entry);
    if(r) *r = jsval_bool(!!entry);
    return S_OK;
}

static HRESULT Set_forEach(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    MapInstance *set;
    HRESULT hres;

    hres = get_set_this(ctx, vthis, &set);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", set, debugstr_jsval(argc ? argv[0] : jsval_undefined()));

    return iterate_map(set, ctx, argc, argv, r);
}

static HRESULT Set_has(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    jsval_t key = argc ? argv[0] : jsval_undefined();
    struct jsval_map_entry *entry;
    MapInstance *set;
    HRESULT hres;

    hres = get_set_this(ctx, vthis, &set);
    if(FAILED(hres))
        return hres;

    TRACE("%p (%s)\n", set, debugstr_jsval(key));

    entry = get_map_entry(set, key);
    if(r) *r = jsval_bool(!!entry);
    return S_OK;
}

static HRESULT Set_value(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static const builtin_prop_t Set_prototype_props[] = {
    {L"add",        Set_add,       PROPF_METHOD|1},
    {L"clear",      Set_clear,     PROPF_METHOD},
    {L"delete" ,    Set_delete,    PROPF_METHOD|1},
    {L"forEach",    Set_forEach,   PROPF_METHOD|1},
    {L"has",        Set_has,       PROPF_METHOD|1},
};

static const builtin_info_t Set_prototype_info = {
    JSCLASS_OBJECT,
    Set_value,
    ARRAY_SIZE(Set_prototype_props),
    Set_prototype_props,
    NULL,
    NULL
};

static const builtin_info_t Set_info = {
    JSCLASS_SET,
    Set_value,
    ARRAY_SIZE(Map_props),
    Map_props,
    Map_destructor,
    NULL
};

static HRESULT Set_constructor(script_ctx_t *ctx, jsval_t vthis, WORD flags, unsigned argc, jsval_t *argv,
        jsval_t *r)
{
    MapInstance *set;
    HRESULT hres;

    switch(flags) {
    case DISPATCH_CONSTRUCT:
        TRACE("\n");

        if(!r)
            return S_OK;
        if(!(set = heap_alloc_zero(sizeof(*set))))
            return E_OUTOFMEMORY;

        hres = init_dispex(&set->dispex, ctx, &Set_info, ctx->set_prototype);
        if(FAILED(hres))
            return hres;

        wine_rb_init(&set->map, jsval_map_compare);
        list_init(&set->entries);
        *r = jsval_obj(&set->dispex);
        return S_OK;

    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }
}

HRESULT init_set_constructor(script_ctx_t *ctx)
{
    jsdisp_t *constructor;
    HRESULT hres;

    if(ctx->version < SCRIPTLANGUAGEVERSION_ES6)
        return S_OK;

    hres = create_dispex(ctx, &Set_prototype_info, ctx->object_prototype, &ctx->set_prototype);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, Set_constructor, L"Set", NULL,
                                      PROPF_CONSTR, ctx->set_prototype, &constructor);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Set", PROPF_WRITABLE,
                                       jsval_obj(constructor));
    jsdisp_release(constructor);
    if(FAILED(hres))
        return hres;

    hres = create_dispex(ctx, &Map_prototype_info, ctx->object_prototype, &ctx->map_prototype);
    if(FAILED(hres))
        return hres;

    hres = create_builtin_constructor(ctx, Map_constructor, L"Map", NULL,
                                      PROPF_CONSTR, ctx->map_prototype, &constructor);
    if(FAILED(hres))
        return hres;

    hres = jsdisp_define_data_property(ctx->global, L"Map", PROPF_WRITABLE,
                                       jsval_obj(constructor));
    jsdisp_release(constructor);
    return hres;
}
