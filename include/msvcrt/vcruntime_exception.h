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

#ifndef __WINE_VCRUNTIME_EXCEPTION_H
#define __WINE_VCRUNTIME_EXCEPTION_H

#include <eh.h>

struct __std_exception_data
{
    const char *_What;
    char _DoFree;
};

extern "C"
{
void __cdecl __std_exception_copy(const struct __std_exception_data *src, struct __std_exception_data *dst);
void __cdecl __std_exception_destroy(struct __std_exception_data *data);
} /* extern "C" */

namespace std
{

class exception
{
private:
    struct __std_exception_data data;
public:
    exception() noexcept : data() {}
    exception(const exception &other) noexcept
    {
        __std_exception_copy(&other.data, &this->data);
    }
    explicit exception(const char *what) throw() : data()
    {
        struct __std_exception_data new_data = {what, true};
        __std_exception_copy(&new_data, &this->data);
    }
    explicit exception(const char *what, int) noexcept : data()
    {
        data._What = what;
    }
    exception &operator=(const exception &other) noexcept
    {
        if (this != &other)
        {
            __std_exception_destroy(&this->data);
            __std_exception_copy(&other.data, &this->data);
        }
        return *this;
    }
    virtual ~exception() noexcept
    {
        __std_exception_destroy(&data);
    }
    virtual const char *what() const
    {
        return data._What ? data._What : "Unknown exception";
    }
};

class bad_exception : public exception
{
public:
    bad_exception() noexcept : exception("bad exception", 1) {}
};

class bad_alloc : public exception
{
private:
    friend class bad_array_new_length;
    bad_alloc(const char *what) throw() : exception(what, 1) {}
public:
    bad_alloc() throw() : exception("bad allocation", 1) {}
};

class bad_array_new_length : public bad_alloc
{
public:
    bad_array_new_length() noexcept : bad_alloc("bad array new length") {}
};

} /* namespace std */

#endif /* __WINE_VCRUNTIME_EXCEPTION_H */
