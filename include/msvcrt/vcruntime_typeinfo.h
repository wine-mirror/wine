/*
 * Copyright 2024 Rémi Bernon for CodeWeavers
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

#ifndef __WINE_VCRUNTIME_TYPEINFO_H
#define __WINE_VCRUNTIME_TYPEINFO_H

#include "vcruntime_exception.h"

struct __std_type_info_data
{
    const char *_UndecoratedName;
    const char _DecoratedName[1];
    __std_type_info_data() = delete;
    __std_type_info_data(const __std_type_info_data&) = delete;
    __std_type_info_data &operator=(const __std_type_info_data&) = delete;
};

struct __type_info_node {};
extern __type_info_node __type_info_root_node;

extern "C"
{
extern int __cdecl __std_type_info_compare(const __std_type_info_data *l, const __std_type_info_data *r);
extern size_t __cdecl __std_type_info_hash(const __std_type_info_data *ti);
extern char const *__cdecl __std_type_info_name(__std_type_info_data *ti, __type_info_node *header);
}

class type_info
{
public:
    size_t hash_code() const noexcept
    {
        return __std_type_info_hash(&data);
    }
    bool operator==(const type_info &other) const
    {
        return __std_type_info_compare(&data, &other.data) == 0;
    }
    bool operator!=(const type_info &other) const
    {
        return __std_type_info_compare(&data, &other.data) != 0;
    }
    bool before(const type_info &other) const
    {
        return __std_type_info_compare(&data, &other.data) < 0;
    }
    const char *name() const
    {
        return __std_type_info_name(&data, &__type_info_root_node);
    }
    const char *raw_name() const
    {
        return data._DecoratedName;
    }
    virtual ~type_info() noexcept {}

    type_info(const type_info&) = delete;
    type_info &operator=(const type_info&) = delete;

private:
    mutable __std_type_info_data data;
};

namespace std
{
using ::type_info;

class bad_cast : public exception
{
public:
    bad_cast() noexcept : exception("bad cast", 1) {}

    static bad_cast __construct_from_string_literal(const char *message) noexcept
    {
        return bad_cast(message, 1);
    }

private:
    bad_cast(const char *const message, int) noexcept : exception(message, 1) {}
};

class bad_typeid : public exception
{
public:
    bad_typeid() noexcept : exception("bad typeid", 1) {}

    static bad_typeid __construct_from_string_literal(const char* message) noexcept
    {
        return bad_typeid(message, 1);
    }

private:
    friend class __non_rtti_object;
    bad_typeid(const char* message, int) noexcept : exception(message, 1) { }
};

class __non_rtti_object : public bad_typeid
{
public:
    static __non_rtti_object __construct_from_string_literal(const char* message) noexcept { return __non_rtti_object(message, 1); }

private:
    __non_rtti_object(const char* message, int) noexcept : bad_typeid(message, 1) { }
};

}

#endif /* __WINE_VCRUNTIME_TYPEINFO_H */
