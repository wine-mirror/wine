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

struct _jsstr_t {
    unsigned length_flags;
    unsigned ref;
};

#define JSSTR_LENGTH_SHIFT 4
#define JSSTR_MAX_LENGTH (1 << (32-JSSTR_LENGTH_SHIFT))
#define JSSTR_FLAGS_MASK ((1 << JSSTR_LENGTH_SHIFT)-1)

#define JSSTR_FLAG_NULLBSTR 1

static inline unsigned jsstr_length(jsstr_t *str)
{
    return str->length_flags >> JSSTR_LENGTH_SHIFT;
}

typedef struct {
    jsstr_t str;
    WCHAR buf[1];
} jsstr_inline_t;

jsstr_t *jsstr_alloc_len(const WCHAR*,unsigned) DECLSPEC_HIDDEN;
WCHAR *jsstr_alloc_buf(unsigned,jsstr_t**) DECLSPEC_HIDDEN;

static inline jsstr_t *jsstr_alloc(const WCHAR *str)
{
    return jsstr_alloc_len(str, strlenW(str));
}

static inline void jsstr_release(jsstr_t *str)
{
    if(!--str->ref)
        heap_free(str);
}

static inline jsstr_t *jsstr_addref(jsstr_t *str)
{
    str->ref++;
    return str;
}

static inline jsstr_inline_t *jsstr_as_inline(jsstr_t *str)
{
    return CONTAINING_RECORD(str, jsstr_inline_t, str);
}

/* This will be failable in the future. */
static inline const WCHAR *jsstr_flatten(jsstr_t *str)
{
    return jsstr_as_inline(str)->buf;
}

static inline BOOL jsstr_eq(jsstr_t *str1, jsstr_t *str2)
{
    unsigned len = jsstr_length(str1);
    return len == jsstr_length(str2) && !memcmp(jsstr_as_inline(str1)->buf, jsstr_as_inline(str2)->buf, len*sizeof(WCHAR));
}

static inline void jsstr_extract(jsstr_t *str, unsigned off, unsigned len, WCHAR *buf)
{
    memcpy(buf, jsstr_as_inline(str)->buf+off, len*sizeof(WCHAR));
}

static inline unsigned jsstr_flush(jsstr_t *str, WCHAR *buf)
{
    unsigned len = jsstr_length(str);
    memcpy(buf, jsstr_as_inline(str)->buf, len*sizeof(WCHAR));
    return len;
}

static inline jsstr_t *jsstr_substr(jsstr_t *str, unsigned off, unsigned len)
{
    jsstr_t *ret;
    WCHAR *ptr;

    ptr = jsstr_alloc_buf(len, &ret);
    if(ptr)
        jsstr_extract(str, off, len, ptr);
    return ret;
}

int jsstr_cmp(jsstr_t*,jsstr_t*) DECLSPEC_HIDDEN;
jsstr_t *jsstr_concat(jsstr_t*,jsstr_t*) DECLSPEC_HIDDEN;

jsstr_t *jsstr_nan(void) DECLSPEC_HIDDEN;
jsstr_t *jsstr_empty(void) DECLSPEC_HIDDEN;
jsstr_t *jsstr_undefined(void) DECLSPEC_HIDDEN;

BOOL init_strings(void) DECLSPEC_HIDDEN;
void free_strings(void) DECLSPEC_HIDDEN;

const char *debugstr_jsstr(jsstr_t*) DECLSPEC_HIDDEN;
