/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

typedef struct {
    const WCHAR *code;
    const WCHAR *ptr;
    const WCHAR *end;

    BOOL parse_complete;
    HRESULT hres;

    int last_token;
    unsigned last_nl;
} parser_ctx_t;

HRESULT parse_script(parser_ctx_t*,const WCHAR*) DECLSPEC_HIDDEN;
int parser_lex(void*,parser_ctx_t*) DECLSPEC_HIDDEN;
void *parser_alloc(parser_ctx_t*,size_t) DECLSPEC_HIDDEN;
