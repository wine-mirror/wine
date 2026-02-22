//===--------------------- stdlib_new_delete.cpp --------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//
// This file implements the new and delete operators.
//===----------------------------------------------------------------------===//

#include <new.h>
#include <vcruntime_exception.h>
#include <malloc.h>

#if 0
#pragma makedep implib
#endif

void *operator new(size_t size)
{
    if (size == 0)
        size = 1;
    void* p;
    while ((p = ::malloc(size)) == 0)
    {
        if (!_callnewh(size))
            throw std::bad_alloc();
    }
    return p;
}

void *operator new(size_t size, const std::nothrow_t&) noexcept
{
    void* p = 0;
    try
    {
        p = ::operator new(size);
    }
    catch (...)
    {
    }
    return p;
}

void *operator new[](size_t size)
{
    return ::operator new(size);
}

void *operator new[](size_t size, const std::nothrow_t&) noexcept
{
    void* p = 0;
    try
    {
        p = ::operator new[](size);
    }
    catch (...)
    {
    }
    return p;
}

void operator delete(void* ptr) noexcept
{
    if (ptr)
        ::free(ptr);
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept
{
    ::operator delete(ptr);
}

void operator delete(void* ptr, size_t) noexcept
{
    ::operator delete(ptr);
}

void operator delete[] (void* ptr) noexcept
{
    ::operator delete(ptr);
}

void operator delete[] (void* ptr, const std::nothrow_t&) noexcept
{
    ::operator delete[](ptr);
}

void operator delete[] (void* ptr, size_t) noexcept
{
    ::operator delete[](ptr);
}

void *operator new(size_t size, std::align_val_t alignment)
{
    if (size == 0)
        size = 1;
    if (static_cast<size_t>(alignment) < sizeof(void*))
      alignment = std::align_val_t(sizeof(void*));
    void* p;
    while ((p = _aligned_malloc(size, static_cast<size_t>(alignment))) == nullptr)
    {
        if (!_callnewh(size))
            throw std::bad_alloc();
    }
    return p;
}

void *operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    void* p = 0;
    try
    {
        p = ::operator new(size, alignment);
    }
    catch (...)
    {
    }
    return p;
}

void *operator new[](size_t size, std::align_val_t alignment)
{
    return ::operator new(size, alignment);
}

void *operator new[](size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    void* p = 0;
    try
    {
        p = ::operator new[](size, alignment);
    }
    catch (...)
    {
    }
    return p;
}

void operator delete(void* ptr, std::align_val_t) noexcept
{
    if (ptr)
        ::_aligned_free(ptr);
}

void operator delete(void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    ::operator delete(ptr, alignment);
}

void operator delete(void* ptr, size_t, std::align_val_t alignment) noexcept
{
    ::operator delete(ptr, alignment);
}

void operator delete[] (void* ptr, std::align_val_t alignment) noexcept
{
    ::operator delete(ptr, alignment);
}

void operator delete[] (void* ptr, std::align_val_t alignment, const std::nothrow_t&) noexcept
{
    ::operator delete[](ptr, alignment);
}

void operator delete[] (void* ptr, size_t, std::align_val_t alignment) noexcept
{
    ::operator delete[](ptr, alignment);
}
