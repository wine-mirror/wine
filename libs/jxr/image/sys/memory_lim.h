/*
 * File         : memory_lim.h
 * Author       : Mirco Miranda
 * Created on   : 2026/5/26
 * Description  : Size-limited memory allocation functions. Used to mitigate problems reading corrupted files.
 */
#ifndef MEMORY_LIM_H
#define MEMORY_LIM_H

#include <stdlib.h>

/*!
 * \brief malloc_lim
 * malloc() with maximum allocable memory limited to alloc_get_lim()
 * \note Use free() to release memory allocated with malloc_lim().
 */
void *malloc_lim(size_t size);

/*!
 * \brief calloc_lim
 * calloc() with maximum allocable memory limited to alloc_get_lim()
 * \note Use free() to release memory allocated with calloc_lim().
 */
void *calloc_lim(size_t count, size_t size);

/*!
 * \brief alloc_set_lim
 * Sets the maximum allocable memory (in bytes) for malloc_lim() and calloc_lim().
 * \param max_size The maximum allocable memory (in bytes). 0 means unlimited (default).
 */
void alloc_set_lim(size_t max_size);

/*!
 * \brief alloc_get_lim
 * Gets the maximum allocable memory (in bytes) for malloc_lim() and calloc_lim(). 0 means unlimited (default).
 */
size_t alloc_get_lim();

#endif // MEMORY_LIM_H
