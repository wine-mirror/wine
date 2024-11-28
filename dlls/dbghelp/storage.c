/*
 * Various storage structures (pool allocation, vector, hash table)
 *
 * Copyright (C) 1993, Eric Youngdale.
 *               2004, Eric Pouech
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
#include <stdlib.h>
#include "wine/debug.h"

#include "dbghelp_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

void pool_init(struct pool* pool, size_t arena_size)
{
    pool->heap = HeapCreate(HEAP_NO_SERIALIZE, arena_size, 0);
}

void pool_destroy(struct pool* pool)
{
    HeapDestroy(pool->heap);
}

void* pool_alloc(struct pool* pool, size_t len)
{
    return HeapAlloc(pool->heap, 0, len);
}

void* pool_realloc(struct pool* pool, void* ptr, size_t len)
{
    return ptr ? HeapReAlloc(pool->heap, 0, ptr, len) : pool_alloc(pool, len);
}

void pool_free(struct pool* pool, void* ptr)
{
#ifdef USE_STATS
    if (ptr)
        mem_stats_down(&pool->stats, HeapSize(pool->heap, 0, ptr));
#endif
    HeapFree(pool->heap, 0, ptr);
}

char* pool_strdup(struct pool* pool, const char* str)
{
    char* ret;
    if ((ret = pool_alloc(pool, strlen(str) + 1))) strcpy(ret, str);
    return ret;
}

WCHAR* pool_wcsdup(struct pool* pool, const WCHAR* str)
{
    WCHAR* ret;
    if ((ret = pool_alloc(pool, (wcslen(str) + 1) * sizeof(WCHAR)))) wcscpy(ret, str);
    return ret;
}

void vector_init(struct vector* v, unsigned esz, unsigned bucket_sz)
{
    v->buckets = NULL;
    /* align size */
    v->elt_size = (esz + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
    switch (bucket_sz)
    {
    case    0: v->shift =  0; break; /* special case see below */
    case    2: v->shift =  1; break;
    case    4: v->shift =  2; break;
    case    8: v->shift =  3; break;
    case   16: v->shift =  4; break;
    case   32: v->shift =  5; break;
    case   64: v->shift =  6; break;
    case  128: v->shift =  7; break;
    case  256: v->shift =  8; break;
    case  512: v->shift =  9; break;
    case 1024: v->shift = 10; break;
    default: assert(0);
    }
    v->num_buckets = 0;
    v->buckets_allocated = 0;
    v->num_elts = 0;
}

unsigned vector_length(const struct vector* v)
{
    return v->num_elts;
}

void* vector_at(const struct vector* v, unsigned pos)
{
    if (pos >= v->num_elts) return NULL;
    if (v->shift)
    {
        unsigned o = pos & ((1 << v->shift) - 1);
        return (char*)v->buckets[pos >> v->shift] + o * v->elt_size;
    }
    else
    {
        return (char*)v->buckets + pos * v->elt_size;
    }
}

void* vector_add(struct vector* v, struct pool* pool)
{
    if (!v->shift)
    {
        if (v->num_elts == 1024)
        {
            /* we'll need a second bucket, so go directly for it */
            void **new = pool_alloc(pool, 2 * sizeof(void*));
            if (!new) return NULL;
            *new = v->buckets;
            v->buckets = new;
            v->num_buckets = 1;
            v->buckets_allocated = 2;
            v->shift = 10;
        }
        else
        {
            if (!v->num_elts || !(v->num_elts & (v->num_elts - 1)))
            {
                void *new = pool_realloc(pool, v->buckets, (v->num_elts ? v->num_elts * 2 : 1) * v->elt_size);
                if (!new) return NULL;
                v->buckets = new;
            }
            return vector_at(v, v->num_elts++);
        }
    }
    if (v->num_elts == (v->num_buckets << v->shift))
    {
        if (v->num_buckets == v->buckets_allocated)
        {
            /* Double the bucket cache, so it scales well with big vectors.*/
            unsigned    new_reserved = v->buckets_allocated ? v->buckets_allocated * 2 : 1;
            void*       new;

            new = pool_realloc(pool, v->buckets, new_reserved * sizeof(void*));
            if (!new) return NULL;
            v->buckets = new;
            v->buckets_allocated = new_reserved;
        }
        v->buckets[v->num_buckets] = pool_alloc(pool, v->elt_size << v->shift);
        if (!v->buckets[v->num_buckets]) return NULL;
        v->num_elts++;
        return v->buckets[v->num_buckets++];
    }
    return vector_at(v, v->num_elts++);
}

/* We construct the sparse array as two vectors (of equal size)
 * The first vector (key2index) is the lookup table between the key and
 * an index in the second vector (elements)
 * When inserting an element, it's always appended in second vector (and
 * never moved in memory later on), only the first vector is reordered
 */
struct key2index
{
    ULONG_PTR           key;
    unsigned            index;
};

void sparse_array_init(struct sparse_array* sa, unsigned elt_sz, unsigned bucket_sz)
{
    vector_init(&sa->key2index, sizeof(struct key2index), bucket_sz);
    vector_init(&sa->elements, elt_sz, bucket_sz);
}

/******************************************************************
 *		sparse_array_lookup
 *
 * Returns the first index which key is >= at passed key
 */
static struct key2index* sparse_array_lookup(const struct sparse_array* sa,
                                             ULONG_PTR key, unsigned* idx)
{
    struct key2index*   pk2i;
    unsigned            low, high;

    if (!sa->elements.num_elts)
    {
        *idx = 0;
        return NULL;
    }
    high = sa->elements.num_elts;
    pk2i = vector_at(&sa->key2index, high - 1);
    if (pk2i->key < key)
    {
        *idx = high;
        return NULL;
    }
    if (pk2i->key == key)
    {
        *idx = high - 1;
        return pk2i;
    }
    low = 0;
    pk2i = vector_at(&sa->key2index, low);
    if (pk2i->key >= key)
    {
        *idx = 0;
        return pk2i;
    }
    /* now we have: sa(lowest key) < key < sa(highest key) */
    while (low < high)
    {
        *idx = (low + high) / 2;
        pk2i = vector_at(&sa->key2index, *idx);
        if (pk2i->key > key)            high = *idx;
        else if (pk2i->key < key)       low = *idx + 1;
        else                            return pk2i;
    }
    /* binary search could return exact item, we search for highest one
     * below the key
     */
    if (pk2i->key < key)
        pk2i = vector_at(&sa->key2index, ++(*idx));
    return pk2i;
}

void*   sparse_array_find(const struct sparse_array* sa, ULONG_PTR key)
{
    unsigned            idx;
    struct key2index*   pk2i;

    if ((pk2i = sparse_array_lookup(sa, key, &idx)) && pk2i->key == key)
        return vector_at(&sa->elements, pk2i->index);
    return NULL;
}

void*   sparse_array_add(struct sparse_array* sa, ULONG_PTR key,
                         struct pool* pool)
{
    unsigned            idx, i;
    struct key2index*   pk2i;
    struct key2index*   to;

    pk2i = sparse_array_lookup(sa, key, &idx);
    if (pk2i && pk2i->key == key)
    {
        FIXME("re-adding an existing key\n");
        return NULL;
    }
    to = vector_add(&sa->key2index, pool);
    if (pk2i)
    {
        /* we need to shift vector's content... */
        /* let's do it brute force... (FIXME) */
        assert(sa->key2index.num_elts >= 2);
        for (i = sa->key2index.num_elts - 1; i > idx; i--)
        {
            pk2i = vector_at(&sa->key2index, i - 1);
            *to = *pk2i;
            to = pk2i;
        }
    }

    to->key = key;
    to->index = sa->elements.num_elts;

    return vector_add(&sa->elements, pool);
}

unsigned sparse_array_length(const struct sparse_array* sa)
{
    return sa->elements.num_elts;
}

static unsigned hash_table_hash(const char* name, unsigned num_buckets)
{
    unsigned    hash = 0;
    while (*name)
    {
        hash += *name++;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash % num_buckets;
}

void hash_table_init(struct pool* pool, struct hash_table* ht, unsigned num_buckets)
{
    ht->num_elts = 0;
    ht->num_buckets = num_buckets;
    ht->pool = pool;
    ht->buckets = NULL;
}

void hash_table_destroy(struct hash_table* ht)
{
#if defined(USE_STATS)
    int                         i;
    unsigned                    len;
    unsigned                    min = 0xffffffff, max = 0, sq = 0;
    struct hash_table_elt*      elt;
    double                      mean, variance;

    for (i = 0; i < ht->num_buckets; i++)
    {
        for (len = 0, elt = ht->buckets[i]; elt; elt = elt->next) len++;
        if (len < min) min = len;
        if (len > max) max = len;
        sq += len * len;
    }
    mean = (double)ht->num_elts / ht->num_buckets;
    variance = (double)sq / ht->num_buckets - mean * mean;
    FIXME("STATS: elts[num:%-4u size:%u mean:%f] buckets[min:%-4u variance:%+f max:%-4u]\n",
          ht->num_elts, ht->num_buckets, mean, min, variance, max);

    for (i = 0; i < ht->num_buckets; i++)
    {
        for (len = 0, elt = ht->buckets[i]; elt; elt = elt->next) len++;
        if (len == max)
        {
            FIXME("Longest bucket:\n");
            for (elt = ht->buckets[i]; elt; elt = elt->next)
                FIXME("\t%s\n", debugstr_a(elt->name));
            break;
        }

    }
#endif
}

void hash_table_add(struct hash_table* ht, struct hash_table_elt* elt)
{
    unsigned                    hash = hash_table_hash(elt->name, ht->num_buckets);

    if (!ht->buckets)
    {
        ht->buckets = pool_alloc(ht->pool, ht->num_buckets * sizeof(struct hash_table_bucket));
        assert(ht->buckets);
        memset(ht->buckets, 0, ht->num_buckets * sizeof(struct hash_table_bucket));
    }

    /* in some cases, we need to get back the symbols of same name in the order
     * in which they've been inserted. So insert new elements at the end of the list.
     */
    if (!ht->buckets[hash].first)
    {
        ht->buckets[hash].first = elt;
    }
    else
    {
        ht->buckets[hash].last->next = elt;
    }
    ht->buckets[hash].last = elt;
    elt->next = NULL;
    ht->num_elts++;
}

void hash_table_iter_init(const struct hash_table* ht, 
                          struct hash_table_iter* hti, const char* name)
{
    hti->ht = ht;
    if (name)
    {
        hti->last = hash_table_hash(name, ht->num_buckets);
        hti->index = hti->last - 1;
    }
    else
    {
        hti->last = ht->num_buckets - 1;
        hti->index = -1;
    }
    hti->element = NULL;
}

void* hash_table_iter_up(struct hash_table_iter* hti)
{
    if (!hti->ht->buckets) return NULL;

    if (hti->element) hti->element = hti->element->next;
    while (!hti->element && hti->index < hti->last) 
        hti->element = hti->ht->buckets[++hti->index].first;
    return hti->element;
}
