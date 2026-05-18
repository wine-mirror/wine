/*
 * Copyright 2026 Nikolay Sivov for CodeWeavers
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
 *
 */

#include "msxml_private.h"

struct xpath_nodeset
{
    size_t count;
    size_t capacity;
    struct domnode **nodes;
};

enum xpath_object_type
{
    _XPATH_UNDEFINED = 0,
    _XPATH_NODESET = 1,
    _XPATH_BOOLEAN = 2,
    _XPATH_NUMBER = 3,
    _XPATH_STRING = 4,
};

struct xpath_object
{
    enum xpath_object_type type;
    struct xpath_nodeset *nodesetval;
    int boolval;
    double floatval;
    WCHAR *stringval;
    void *user;
    int index;
    void *user2;
    int index2;
};

struct xpath_namespace
{
    struct list entry;
    WCHAR *prefix;
    WCHAR *uri;
};

struct xpath_context;

extern struct xpath_context * xpath_create_context(bool xpath, struct domnode *node);
extern void xpath_free_context(struct xpath_context *ctxt);
extern HRESULT xpath_parse_selection_namespaces(const WCHAR *value, struct domdoc_properties *properties);
extern struct xpath_object * xpath_eval(const WCHAR *str, struct xpath_context *ctx);
extern void xpath_free_object(struct xpath_object *obj);
extern WCHAR *xpath_translate_function(const WCHAR *to, const WCHAR *from, const WCHAR *str);
