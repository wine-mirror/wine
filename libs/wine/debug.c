/*
 * Management of the debugging channels
 *
 * Copyright 2000 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "wine/debug.h"
#include "wine/library.h"
#include "wine/unicode.h"

struct dll
{
    struct dll   *next;        /* linked list of dlls */
    struct dll   *prev;
    struct __wine_debug_channel * const *channels;    /* array of channels */
    int           nb_channels; /* number of channels in array */
};

static struct dll *first_dll;

struct debug_option
{
    struct debug_option *next;       /* next option in list */
    unsigned char        set;        /* bits to set */
    unsigned char        clear;      /* bits to clear */
    char                 name[14];   /* channel name, or empty for "all" */
};

static struct debug_option *first_option;
static struct debug_option *last_option;
static struct __wine_debug_functions funcs;

static const char * const debug_classes[] = { "fixme", "err", "warn", "trace" };

static int cmp_name( const void *p1, const void *p2 )
{
    const char *name = p1;
    const struct __wine_debug_channel * const *chan = p2;
    return strcmp( name, (*chan)->name );
}

/* apply a debug option to the channels of a given dll */
static void apply_option( struct dll *dll, const struct debug_option *opt )
{
    if (opt->name[0])
    {
        struct __wine_debug_channel * const *dbch = bsearch( opt->name, dll->channels, dll->nb_channels,
                                                             sizeof(*dll->channels), cmp_name );
        if (dbch) (*dbch)->flags = ((*dbch)->flags & ~opt->clear) | opt->set;
    }
    else /* all */
    {
        int i;
        for (i = 0; i < dll->nb_channels; i++)
            dll->channels[i]->flags = (dll->channels[i]->flags & ~opt->clear) | opt->set;
    }
}

/* register a new set of channels for a dll */
void *__wine_dbg_register( struct __wine_debug_channel * const *channels, int nb )
{
    struct debug_option *opt = first_option;
    struct dll *dll = malloc( sizeof(*dll) );
    if (dll)
    {
        dll->channels = channels;
        dll->nb_channels = nb;
        dll->prev = NULL;
        if ((dll->next = first_dll)) dll->next->prev = dll;
        first_dll = dll;

        /* apply existing options to this dll */
        while (opt)
        {
            apply_option( dll, opt );
            opt = opt->next;
        }
    }
    return dll;
}


/* unregister a set of channels; must pass the pointer obtained from wine_dbg_register */
void __wine_dbg_unregister( void *channel )
{
    struct dll *dll = channel;
    if (dll)
    {
        if (dll->next) dll->next->prev = dll->prev;
        if (dll->prev) dll->prev->next = dll->next;
        else first_dll = dll->next;
        free( dll );
    }
}


/* add a new debug option at the end of the option list */
void wine_dbg_add_option( const char *name, unsigned char set, unsigned char clear )
{
    struct dll *dll = first_dll;
    struct debug_option *opt;
    size_t len = strlen(name);

    if (!(opt = malloc( sizeof(*opt) ))) return;
    opt->next  = NULL;
    opt->set   = set;
    opt->clear = clear;
    if (len >= sizeof(opt->name)) len = sizeof(opt->name) - 1;
    memcpy( opt->name, name, len );
    opt->name[len] = 0;
    if (last_option) last_option->next = opt;
    else first_option = opt;
    last_option = opt;

    /* apply option to all existing dlls */
    while (dll)
    {
        apply_option( dll, opt );
        dll = dll->next;
    }
}

/* parse a set of debugging option specifications and add them to the option list */
int wine_dbg_parse_options( const char *str )
{
    char *opt, *next, *options;
    unsigned int i;
    int errors = 0;

    if (!(options = strdup(str))) return -1;
    for (opt = options; opt; opt = next)
    {
        const char *p;
        unsigned char set = 0, clear = 0;

        if ((next = strchr( opt, ',' ))) *next++ = 0;

        p = opt + strcspn( opt, "+-" );
        if (!p[0]) p = opt;  /* assume it's a debug channel name */

        if (p > opt)
        {
            for (i = 0; i < sizeof(debug_classes)/sizeof(debug_classes[0]); i++)
            {
                int len = strlen(debug_classes[i]);
                if (len != (p - opt)) continue;
                if (!memcmp( opt, debug_classes[i], len ))  /* found it */
                {
                    if (*p == '+') set |= 1 << i;
                    else clear |= 1 << i;
                    break;
                }
            }
            if (i == sizeof(debug_classes)/sizeof(debug_classes[0])) /* bad class name, skip it */
            {
                errors++;
                continue;
            }
        }
        else
        {
            if (*p == '-') clear = ~0;
            else set = ~0;
        }
        if (*p == '+' || *p == '-') p++;
        if (!p[0])
        {
            errors++;
            continue;
        }
        if (!strcmp( p, "all" )) p = "";  /* empty string means all */
        wine_dbg_add_option( p, set, clear );
    }
    free( options );
    return errors;
}

/* varargs wrapper for funcs.dbg_vprintf */
int wine_dbg_printf( const char *format, ... )
{
    int ret;
    va_list valist;

    va_start(valist, format);
    ret = funcs.dbg_vprintf( format, valist );
    va_end(valist);
    return ret;
}

/* printf with temp buffer allocation */
const char *wine_dbg_sprintf( const char *format, ... )
{
    static const int max_size = 200;
    char *ret;
    int len;
    va_list valist;

    va_start(valist, format);
    ret = funcs.get_temp_buffer( max_size );
    len = vsnprintf( ret, max_size, format, valist );
    if (len == -1 || len >= max_size) ret[max_size-1] = 0;
    else funcs.release_temp_buffer( ret, len + 1 );
    va_end(valist);
    return ret;
}


/* varargs wrapper for funcs.dbg_vlog */
int wine_dbg_log( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                  const char *func, const char *format, ... )
{
    int ret;
    va_list valist;

    va_start(valist, format);
    ret = funcs.dbg_vlog( cls, channel, func, format, valist );
    va_end(valist);
    return ret;
}


/* allocate some tmp string space */
/* FIXME: this is not 100% thread-safe */
static char *get_temp_buffer( size_t size )
{
    static char *list[32];
    static int pos;
    char *ret;
    int idx;

    idx = interlocked_xchg_add( &pos, 1 ) % (sizeof(list)/sizeof(list[0]));
    if ((ret = realloc( list[idx], size ))) list[idx] = ret;
    return ret;
}


/* release unused part of the buffer */
static void release_temp_buffer( char *buffer, size_t size )
{
    /* don't bother doing anything */
}


/* default implementation of wine_dbgstr_an */
static const char *default_dbgstr_an( const char *str, int n )
{
    static const char hex[16] = "0123456789abcdef";
    char *dst, *res;
    size_t size;

    if (!((ULONG_PTR)str >> 16))
    {
        if (!str) return "(null)";
        res = funcs.get_temp_buffer( 6 );
        sprintf( res, "#%04x", LOWORD(str) );
        return res;
    }
    if (n == -1) n = strlen(str);
    if (n < 0) n = 0;
    size = 10 + min( 300, n * 4 );
    dst = res = funcs.get_temp_buffer( size );
    *dst++ = '"';
    while (n-- > 0 && dst <= res + size - 9)
    {
        unsigned char c = *str++;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        case '"':  *dst++ = '\\'; *dst++ = '"'; break;
        case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
        default:
            if (c >= ' ' && c <= 126)
                *dst++ = c;
            else
            {
                *dst++ = '\\';
                *dst++ = 'x';
                *dst++ = hex[(c >> 4) & 0x0f];
                *dst++ = hex[c & 0x0f];
            }
        }
    }
    *dst++ = '"';
    if (*str)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst++ = 0;
    funcs.release_temp_buffer( res, dst - res );
    return res;
}


/* default implementation of wine_dbgstr_wn */
static const char *default_dbgstr_wn( const WCHAR *str, int n )
{
    char *dst, *res;
    size_t size;

    if (!((ULONG_PTR)str >> 16))
    {
        if (!str) return "(null)";
        res = funcs.get_temp_buffer( 6 );
        sprintf( res, "#%04x", LOWORD(str) );
        return res;
    }
    if (n == -1) n = strlenW(str);
    if (n < 0) n = 0;
    size = 12 + min( 300, n * 5 );
    dst = res = funcs.get_temp_buffer( n * 5 + 7 );
    *dst++ = 'L';
    *dst++ = '"';
    while (n-- > 0 && dst <= res + size - 10)
    {
        WCHAR c = *str++;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        case '"':  *dst++ = '\\'; *dst++ = '"'; break;
        case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
        default:
            if (c >= ' ' && c <= 126)
                *dst++ = c;
            else
            {
                *dst++ = '\\';
                sprintf(dst,"%04x",c);
                dst+=4;
            }
        }
    }
    *dst++ = '"';
    if (*str)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst++ = 0;
    funcs.release_temp_buffer( res, dst - res );
    return res;
}


/* default implementation of wine_dbg_vprintf */
static int default_dbg_vprintf( const char *format, va_list args )
{
    return vfprintf( stderr, format, args );
}


/* default implementation of wine_dbg_vlog */
static int default_dbg_vlog( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                             const char *func, const char *format, va_list args )
{
    int ret = 0;

    if (cls < sizeof(debug_classes)/sizeof(debug_classes[0]))
        ret += wine_dbg_printf( "%s:%s:%s ", debug_classes[cls], channel->name, func );
    if (format)
        ret += funcs.dbg_vprintf( format, args );
    return ret;
}

/* wrappers to use the function pointers */

const char *wine_dbgstr_an( const char * s, int n )
{
    return funcs.dbgstr_an(s, n);
}

const char *wine_dbgstr_wn( const WCHAR *s, int n )
{
    return funcs.dbgstr_wn(s, n);
}

const char *wine_dbgstr_a( const char *s )
{
    return funcs.dbgstr_an( s, -1 );
}

const char *wine_dbgstr_w( const WCHAR *s )
{
    return funcs.dbgstr_wn( s, -1 );
}

void __wine_dbg_set_functions( const struct __wine_debug_functions *new_funcs,
                               struct __wine_debug_functions *old_funcs, size_t size )
{
    if (old_funcs) memcpy( old_funcs, &funcs, min(sizeof(funcs),size) );
    if (new_funcs) memcpy( &funcs, new_funcs, min(sizeof(funcs),size) );
}

static struct __wine_debug_functions funcs =
{
    get_temp_buffer,
    release_temp_buffer,
    default_dbgstr_an,
    default_dbgstr_wn,
    default_dbg_vprintf,
    default_dbg_vlog
};
