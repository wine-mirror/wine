/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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

/*
 * Code in this file is based on files:
 * js/src/jsregexp.h
 * js/src/jsregexp.c
 * from Mozilla project, released under LGPL 2.1 or later.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 */

#define JSREG_FOLD      0x01    /* fold uppercase to lowercase */
#define JSREG_GLOB      0x02    /* global exec, creates array of matches */
#define JSREG_MULTILINE 0x04    /* treat ^ and $ as begin and end of line */
#define JSREG_STICKY    0x08    /* only match starting at lastIndex */

typedef struct RECapture {
    ptrdiff_t index;            /* start of contents, -1 for empty  */
    size_t length;              /* length of capture */
} RECapture;

typedef struct REMatchState {
    const WCHAR *cp;
    RECapture parens[1];      /* first of 're->parenCount' captures,
                                 allocated at end of this struct */
} REMatchState;


typedef BYTE JSPackedBool;
typedef BYTE jsbytecode;

/*
 * This struct holds a bitmap representation of a class from a regexp.
 * There's a list of these referenced by the classList field in the JSRegExp
 * struct below. The initial state has startIndex set to the offset in the
 * original regexp source of the beginning of the class contents. The first
 * use of the class converts the source representation into a bitmap.
 *
 */
typedef struct RECharSet {
    JSPackedBool    converted;
    JSPackedBool    sense;
    WORD            length;
    union {
        BYTE        *bits;
        struct {
            size_t  startIndex;
            size_t  length;
        } src;
    } u;
} RECharSet;

typedef struct JSRegExp {
    WORD         flags;         /* flags, see jsapi.h's JSREG_* defines */
    size_t       parenCount;    /* number of parenthesized submatches */
    size_t       classCount;    /* count [...] bitmaps */
    RECharSet    *classList;    /* list of [...] bitmaps */
    const WCHAR  *source;       /* locked source string, sans // */
    DWORD        source_len;
    jsbytecode   program[1];    /* regular expression bytecode */
} JSRegExp;

JSRegExp* js_NewRegExp(void *cx, heap_pool_t *pool, const WCHAR *str,
        DWORD str_len, UINT flags, BOOL flat) DECLSPEC_HIDDEN;
void js_DestroyRegExp(JSRegExp *re) DECLSPEC_HIDDEN;
HRESULT MatchRegExpNext(JSRegExp *jsregexp, const WCHAR *str,
        DWORD str_len, const WCHAR **cp, heap_pool_t *pool,
        REMatchState **result, DWORD *matchlen) DECLSPEC_HIDDEN;
