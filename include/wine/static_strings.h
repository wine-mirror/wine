/*
 * 32-on-64 store for library static strings
 *
 * Copyright 2019 Conor McCarthy for CodeWeavers
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
 * NOTES:
 * This is not optimized in any way. It is intended to store local copies in
 * 32-bit address space of informational strings returned from 64-bit
 * libraries. These are usually static strings declared in the library, e.g.
 * version info, descriptions, error messages, etc. The number of unique
 * strings is therefore limited. The most logical place to free the strings
 * is on DLL unload.
 */

#ifndef __WINE_STATIC_STRINGS_H
#define __WINE_STATIC_STRINGS_H

#ifdef __i386_on_x86_64__

struct wine_static_strings
{
    char **strings;
    size_t string_count;
    size_t string_alloc;
    CRITICAL_SECTION cs;
    CRITICAL_SECTION_DEBUG cs_debug;
};

#define DECLARE_STATIC_STRINGS(ss) \
    static struct wine_static_strings ss = { \
        NULL, 0, 0, { &ss.cs_debug, -1, 0, 0, 0, 0 }, \
        { 0, 0, &ss.cs, { &ss.cs_debug.ProcessLocksList, &ss.cs_debug.ProcessLocksList }, \
          0, 0, { (DWORD_PTR)(__FILE__ ": " # ss) }} \
    }

static inline const char *wine_static_string_locked_add(struct wine_static_strings *static_strings, const char * HOSTPTR p)
{
    size_t i;

    for (i = 0; i < static_strings->string_count; ++i)
    {
        if (!strcmp(p, static_strings->strings[i]))
            return static_strings->strings[i];
    }

    if (static_strings->string_count == static_strings->string_alloc)
    {
        char **strings;
        static_strings->string_alloc = max(16, static_strings->string_alloc << 1);
        strings = heap_realloc(static_strings->strings, static_strings->string_alloc * sizeof(static_strings->strings[0]));
        if (!strings)
            return "";
        static_strings->strings = strings;
    }

    if ((static_strings->strings[i] = heap_strdup(p)))
    {
        static_strings->string_count++;
        return static_strings->strings[i];
    }

    return "";
}

static inline const char *wine_static_string_add(struct wine_static_strings *static_strings, const char * HOSTPTR p)
{
    const char *str;

    if (!p)
        return NULL;

    EnterCriticalSection(&static_strings->cs);
    str = wine_static_string_locked_add(static_strings, p);
    LeaveCriticalSection(&static_strings->cs);

    return str;
}

static inline void wine_static_string_free(struct wine_static_strings *static_strings)
{
    size_t i;

    EnterCriticalSection(&static_strings->cs);

    for (i = 0; i < static_strings->string_count; ++i)
        heap_free(static_strings->strings[i]);
    heap_free(static_strings->strings);

    static_strings->strings = NULL;
    static_strings->string_count = 0;
    static_strings->string_alloc = 0;

    LeaveCriticalSection(&static_strings->cs);
}

#else

struct wine_static_strings
{
    char dummy;
};

#define DECLARE_STATIC_STRINGS(ss) \
    static struct wine_static_strings ss;

static inline const char *wine_static_string_add(struct wine_static_strings *strings, const char * HOSTPTR p)
{
    (void)strings;
    return p;
}

static inline void wine_static_string_free(struct wine_static_strings *strings)
{
    (void)strings;
}

#endif /* __i386_on_x86_64__ */

static inline const unsigned char *wine_static_ustring_add(struct wine_static_strings *strings, const unsigned char * HOSTPTR p)
{
    return (const unsigned char *)wine_static_string_add(strings, (const char * HOSTPTR)p);
}

#endif /*__WINE_STATIC_STRINGS_H */
