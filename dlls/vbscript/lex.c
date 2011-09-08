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

#include <assert.h>

#include "vbscript.h"
#include "parse.h"
#include "parser.tab.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vbscript);

static const WCHAR callW[] = {'c','a','l','l',0};

static const struct {
    const WCHAR *word;
    int token;
} keywords[] = {
    {callW, tCALL}
};

static inline BOOL is_identifier_char(WCHAR c)
{
    return isalnumW(c) || c == '_';
}

static int check_keyword(parser_ctx_t *ctx, const WCHAR *word)
{
    const WCHAR *p1 = ctx->ptr;
    const WCHAR *p2 = word;
    WCHAR c;

    while(p1 < ctx->end && *p2) {
        c = tolowerW(*p1);
        if(c != *p2)
            return c - *p2;
        p1++;
        p2++;
    }

    if(*p2 || (p1 < ctx->end && is_identifier_char(*p1)))
        return 1;

    ctx->ptr = p1;
    return 0;
}

static int check_keywords(parser_ctx_t *ctx)
{
    int min = 0, max = sizeof(keywords)/sizeof(keywords[0])-1, r, i;

    while(min <= max) {
        i = (min+max)/2;

        r = check_keyword(ctx, keywords[i].word);
        if(!r)
            return keywords[i].token;

        if(r > 0)
            min = i+1;
        else
            max = i-1;
    }

    return 0;
}

static int parse_identifier(parser_ctx_t *ctx, const WCHAR **ret)
{
    const WCHAR *ptr = ctx->ptr++;
    WCHAR *str;
    int len;

    while(ctx->ptr < ctx->end && is_identifier_char(*ctx->ptr))
        ctx->ptr++;
    len = ctx->ptr-ptr;

    str = parser_alloc(ctx, (len+1)*sizeof(WCHAR));
    if(!str)
        return 0;

    memcpy(str, ptr, (len+1)*sizeof(WCHAR));
    str[len] = 0;
    *ret = str;
    return tIdentifier;
}

static int parse_next_token(void *lval, parser_ctx_t *ctx)
{
    WCHAR c;

    while(*ctx->ptr == ' ' || *ctx->ptr == '\t' || *ctx->ptr == '\r')
        ctx->ptr++;
    if(ctx->ptr == ctx->end)
        return ctx->last_token == tNL ? tEOF : tNL;

    c = *ctx->ptr;

    if(isalphaW(c)) {
        int ret = check_keywords(ctx);
        if(!ret)
            return parse_identifier(ctx, lval);
        return ret;
    }

    switch(c) {
    case '\n':
        ctx->ptr++;
        return tNL;
    case '\'':
        ctx->ptr = strchrW(ctx->ptr, '\n');
        if(ctx->ptr)
            ctx->ptr++;
        else
            ctx->ptr = ctx->end;
        return tNL;
    case '(':
    case ')':
    case ',':
    case '=':
    case '+':
    case '-':
    case '*':
    case '/':
    case '^':
    case '\\':
    case '.':
        return *ctx->ptr++;
    default:
        FIXME("Unhandled char %c in %s\n", *ctx->ptr, debugstr_w(ctx->ptr));
    }

    return 0;
}

int parser_lex(void *lval, parser_ctx_t *ctx)
{
    int ret;

    while(1) {
        ret = parse_next_token(lval, ctx);
        if(ret != tNL || ctx->last_token != tNL)
            break;

        ctx->last_nl = ctx->ptr-ctx->code;
    }

    return (ctx->last_token = ret);
}
