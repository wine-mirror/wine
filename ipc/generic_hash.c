/***************************************************************************
 * Copyright 1995 Michael Veksler. mveksler@vnet.ibm.com
 ***************************************************************************
 * File:      generic_hash.c
 * Purpose :  dynamically growing hash, may use shared or local memory.
 ***************************************************************************
 */
#ifdef CONFIG_IPC

#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include "generic_hash.h"

#define ROUND_UP4(num) (( (num)+3) & ~3)

#define FREE_ENTRY          0
#define DELETED_ENTRY       ((DWORD)-1)

#define NO_OF_PRIMES   512
#define GET_ITEM(items,size,i)\
           (*(HASH_ITEM*) \
	    (  ((char *)(items))+ \
	       (i)*(size)) )

static HASH_ITEM *locate_entry(HASH_CONTAINER* hash, DWORD key,
			       HASH_VAL *seeked_data, BOOL skip_deleted);

static void copy_hash_items(HASH_CONTAINER *hash, HASH_ITEM *old_items,
			    int old_n_items);

static BOOL arrays_initialized = FALSE;
static int primes[NO_OF_PRIMES];
static int best_primes[NO_OF_PRIMES];
static int no_of_primes;
static int no_of_best_primes;
static int max_num;

/* binary search for `num' in the `primes' array */
static BOOL prime_binary_search_found(int num)
{
  int min_idx, max_idx, idx;
  
  min_idx=0;
  max_idx=no_of_primes-1;
  
  while (min_idx <= max_idx) {
     idx = (max_idx + min_idx) >> 1;
     if (num == primes[idx])
	return TRUE;
     if (num < primes[idx])
	max_idx = idx-1;
     else
	min_idx = idx+1;
  }
  return FALSE;
}

static BOOL is_prime(int num)
{
  int i;
  if ((num & 0x1) == 0)		   /* can be divided by 2 */
     if (num == 2) 
	return TRUE;
     else
	return FALSE;
  
  if (num <= primes[no_of_primes-1]) 
     return prime_binary_search_found(num);

  for (i=0 ; i < no_of_primes ; i++) {
     if (num % primes[i] == 0)
	return FALSE;
     if (num < primes[i] * primes[i])
	return TRUE;
  }
  return TRUE;
}

static void setup_primes()
{
  int num;
  
  primes[0]=2;
  primes[1]=3;
  no_of_primes=2;
  
  /* count in modulo 6 to avoid numbers that divide by 2 or 3 */
  for (num=5 ; ; num+=6) {
     if (is_prime(num)) {
	primes[no_of_primes++]=num;
	if (no_of_primes >= NO_OF_PRIMES)
	   break;
     }
     if (is_prime(num+2)) {
	primes[no_of_primes++]=num+2;
	if (no_of_primes >= NO_OF_PRIMES)
	   break;
     }
  }
  max_num= primes[no_of_primes-1] * primes[no_of_primes-1];
}


/* Find primes which are far "enough" from powers of two */

void setup_best_primes()
{
  int i;
  int num;
  int pow2before, pow2after;
  int min_range, max_range;

  min_range=3;
  max_range=3;
  pow2before= 2;
  pow2after= 4;

  no_of_best_primes= 0;
  for (i=0 ; i < no_of_primes ; i++){
     num= primes[i];

     if (num > pow2after) {
	pow2before= pow2after;
	pow2after <<=1;
	min_range= pow2before+ (pow2before >> 3);
	max_range= pow2after-  (pow2before >> 2);
     }
     if (num > min_range && num < max_range) 
	best_primes[no_of_best_primes++]=num;
  }
}

/* binary search for `num' in the `best_primes' array,
 * Return smallest best_prime >= num.
 */
static int best_prime_binary_search(int num)
{
  int min_idx, max_idx, idx;
  
  min_idx=0;
  max_idx=no_of_best_primes-1;
  
  while (1) {
     idx = (max_idx + min_idx) >> 1;
     if (num == best_primes[idx])
	return num;
     if (num < best_primes[idx]) {
	max_idx = idx-1;
	if (max_idx <= min_idx)
	    return best_primes[idx];
    }
     else {
	min_idx = idx+1;
	if (min_idx >= max_idx)
	    return best_primes[max_idx];
    }
  }

}

/* Find the best prime, near `num' (which can be any number) */
static int best_prime(int num)
{
  int log2;
  int pow2less, pow2more;
  int min_range, max_range;

  if (num < 11)
     return 11;
  
  if (num <= best_primes[no_of_best_primes-1])
     return best_prime_binary_search(num);

  assert( num < max_num );

  for (log2=0 ; num >> log2 ; log2++)
     ;

  pow2less= 1 << log2;
  pow2more= 1 << (log2+1);
  min_range= pow2less + (pow2less >> 3);
  max_range= pow2more - (pow2more >> 3);

  if (num < min_range)
     num= min_range;

  num |= 1;			   /* make sure num can't be divided by 2 */
  
  while (1) {
     if (num >= max_range) {
	pow2less<<= 1;
	pow2more<<= 1;
	min_range= pow2less + (pow2less >> 3);
	max_range= pow2more - (pow2more >> 3);
	num= min_range | 1;	   /* make sure num can't be divided by 2 */
     }
     /* num should be here in the range: (min_range, max_range) */
     if (is_prime(num))
	return num;
     num+=2;
  }
}

/* FIXME: This can be done before compiling. (uning a script)*/
static void setup_arrays()
{
  setup_primes();
  setup_best_primes();
}

/* Discard all DELETED_ENTRYs moving the data to it's correct location.
 * Done without a temporary buffer.
 * May require some efficiency improvements ( currently it's o(N^2)
 * or is it o(N^3) in the worst case ? In the avarege it seems to be
 * something like o(N log (N)))
 */
static void static_collect_garbge(HASH_CONTAINER *hash)
{
   int i;
   BOOL change;
   HASH_ITEM *items;
   HASH_ITEM *located;
   HASH_ITEM *item;
   int key;
  
   items= hash->items;
   
   do {
      change= FALSE;
      for (i=hash->shared->total_items-1 ; i >= 0 ; i--) {
	 item= &GET_ITEM(items,hash->bytes_per_item,i);
	 key= item->key;
	 if (key != DELETED_ENTRY && key != FREE_ENTRY) {
	    /* try to place the entry in a deleted location */
	    located= locate_entry(hash, key, &item->data,
				  0 /* no skip_deleted */);
	    if (located->key == DELETED_ENTRY) {
	       change= TRUE;
	       memcpy(&located, &item,
		      hash->bytes_per_item);
	       item->key= DELETED_ENTRY;
	    }
	 }
      }
   } while (change);

   /* No change means that there is no need to go through a DELETED_ENTRY
    * in order to reach an item, so DELETED_ENTRY looses it's special
    * meaning, and it is the same as FREE_ENTRY.
    */
   for (i=hash->shared->total_items-1 ; i >= 0 ; i--)
      if (GET_ITEM(items,hash->bytes_per_item,i).key == DELETED_ENTRY)
	 GET_ITEM(items,hash->bytes_per_item,i).key = FREE_ENTRY;
   hash->shared->deleted_items=0;
}

static void collect_garbge(HASH_CONTAINER *hash)
{
   HASH_SHARED *shared= hash->shared;
   HASH_ITEM *temp_items;
   int size;
    
   size= shared->total_items * hash->bytes_per_item;
   temp_items= (HASH_ITEM*)malloc(size);
   if (temp_items==NULL) {
      static_collect_garbge(hash);
   } else {
      memcpy(temp_items, hash->items, size);
      copy_hash_items(hash, temp_items, shared->total_items);
   }
}


static void copy_hash_items(HASH_CONTAINER *hash, HASH_ITEM *old_items,
			    int old_n_items)
{
   HASH_SHARED *shared= hash->shared;
   HASH_ITEM *item;
   int i;
   
   shared->deleted_items = 0;
   shared->free_items= shared->total_items;
   
   /* make all items free */
   for (i= shared->total_items-1 ; i>=0 ; i--)
      GET_ITEM(hash->items, hash->bytes_per_item, i).key = FREE_ENTRY;
   
   /* copy items */
   for (i=0 ; i <= old_n_items; i++) {
      item= &GET_ITEM(old_items, hash->bytes_per_item,i);
      if (item->key != FREE_ENTRY && item->key != DELETED_ENTRY)
	 hash_add_item(hash, item->key, &item->data);
   } 
}


static void reorder_hash(HASH_CONTAINER *hash)
{
  HASH_SHARED *shared= hash->shared;
  HASH_ITEM *items, *old_items;
  HASH_PTR shared_items, old_shared_items;
  int n_items, old_n_items;
  int size;

  if (shared->deleted_items > hash->min_free_items) {
     collect_garbge(hash);
     return;
  }
  n_items= best_prime(shared->total_items * HASH_REALLOC_JUMPS);

  size= n_items *
	(sizeof(items[0]) - sizeof(items[0].data) + hash->bytes_per_item);
 
  shared_items= hash->allocate_mem(size);
  items= hash->access_mem(shared_items);
  
  if (items == NULL) {
	collect_garbge(hash);
	return;
  }
  old_shared_items = shared->items;
  old_n_items=       shared->total_items;
  old_items=         hash->items;

  /* setup a new clean hash based on the parameters of the original one */
  hash->items=          items;
  shared->total_items = n_items;
  shared->items=        shared_items;
  set_hash_parameters(hash, hash->maximum_load);
  copy_hash_items(hash, old_items, old_n_items);
  
  hash->free_mem(old_shared_items);
  hash->last_ptr_update= ++shared->ptr_updates;
}

/* low level: attach hash existing hash items, no checks are performed
 * No complex calculations done.
 */
static HASH_CONTAINER *attach_no_check(HASH_ITEM *items, int bytes_per_datum)
{
    HASH_CONTAINER *hash;
    int bytes_per_item;
    HASH_ITEM dummy_item;
    
    hash= (HASH_CONTAINER*) malloc(sizeof(HASH_CONTAINER) );
    if (hash == NULL)
	return NULL;
    
    bytes_per_item= bytes_per_datum+
                    sizeof(dummy_item)-sizeof(dummy_item.data);
    hash->bytes_per_item= ROUND_UP4(bytes_per_item);
    hash->items=          items;
    hash->is_correct_item= NULL;
    hash->allocate_mem=   HASH_MEM_ALLOC;
    hash->access_mem=     HASH_MEM_ACCESS;
    hash->free_mem=       HASH_MEM_FREE;
    set_hash_parameters(hash, HASH_LOAD);
    

    return hash;
}


/* Attach existing & running remote (i.e. shared) hash.
 * Attach the items using the data stored in "shared"
 */
HASH_CONTAINER *attach_remote_hash(HASH_SHARED *shared, int bytes_per_datum,
				   HASH_ITEM *(*access_mem)(HASH_PTR))
{
    HASH_CONTAINER *hash;
    HASH_ITEM *items;

    assert(access_mem != NULL);
    if (! arrays_initialized)
	setup_arrays();

    items=access_mem(shared->items);
    hash= attach_no_check(items, bytes_per_datum);
    if (hash == NULL)
	return NULL;
    
    hash->shared_was_malloced = FALSE;
    hash->shared= shared;
    return (hash);
}


HASH_CONTAINER *create_remote_hash(HASH_SHARED *shared,
				   int bytes_per_datum,
				   int total_items,
				   HASH_PTR (*allocate_mem)(int size),
				   HASH_ITEM *(*access_mem)(HASH_PTR))
{
   HASH_CONTAINER *hash;
   int size;
   int i;
    
   assert(total_items >= 1);
   assert(bytes_per_datum >=1);
   assert(access_mem != NULL);
   assert(allocate_mem != NULL);
   assert(shared != NULL);
   if (! arrays_initialized)
      setup_arrays();

   if (total_items < MIN_HASH)
      total_items= MIN_HASH;
   else 
      total_items= best_prime(total_items);

   hash= attach_no_check(NULL, bytes_per_datum);
    
   if (hash==NULL) {
      free(hash);
      return NULL;
   }
    
   shared->total_items=  total_items;
   hash->shared= shared;
   hash->shared_was_malloced = FALSE;

   size= total_items * hash->bytes_per_item;

   shared->items = allocate_mem(size);
   hash->items=    access_mem(shared->items);
    
   if (hash->items == NULL ) {
      free(hash);
      return NULL;
   }
   shared->items.ptr= hash->items;
    
   /* make all items free */
   for (i=0 ; i < total_items ; i++)
      GET_ITEM(hash->items,hash->bytes_per_item,i).key = FREE_ENTRY;
    
   shared->deleted_items= 0;
   shared->free_items= total_items;
   shared->ptr_updates= 0;
   return hash;

}

/* hash constructor: create brand new hash */
HASH_CONTAINER *create_hash(int bytes_per_datum, int total_items)
{
   HASH_CONTAINER *hash;
   HASH_SHARED *shared;

   
   shared= (HASH_SHARED*)malloc(sizeof(HASH_SHARED));
   if (shared == NULL)
      return NULL;
   
   hash= create_remote_hash(shared, bytes_per_datum, total_items,
			    HASH_MEM_ALLOC, HASH_MEM_ACCESS);

   if (hash == NULL) {
      free(shared);
      return NULL;
   }
   
   hash->shared_was_malloced = TRUE;
   return hash;
}

/* set the extra handlers to non default values */
void set_hash_handlers(HASH_CONTAINER *hash,
		       HASH_ITEM_TEST *is_correct_item,
		       HASH_PTR (*allocate_mem)(int size),
		       void     (*free_mem)(HASH_PTR),
		       HASH_ITEM *(*access_mem)(HASH_PTR))
{
   assert(hash);
   assert(allocate_mem);
   assert(free_mem);
    
   hash->free_mem     = free_mem;
   hash->allocate_mem = allocate_mem;
   hash->access_mem   = access_mem;
   hash->is_correct_item = is_correct_item;
}


/* set extra parameters */
void set_hash_parameters(HASH_CONTAINER *hash, int load)
{
   assert(hash);
   assert(load>30);		/* no sence to realloc with less than */
				/* 50% load, limiting to 30% to be on */
				/* the safe size */
   assert(load<=100);
    
   hash->maximum_load=   load;
   hash->min_free_items= (1.0 - load/100.0) * hash->shared->total_items + 1 ;
}

/* hash destructor: destroy anything related to the hash */
void destroy_hash(HASH_CONTAINER *hash)
{
   assert(hash);
   hash->free_mem(hash->shared->items);
   if (hash->shared_was_malloced)
      free(hash->shared);
   free(hash);
}

/* hash destructor: just detach hash, without destroing it (makes */
/* sence in shared memory environment) */
void detach_hash(HASH_CONTAINER *hash)
{
   assert(hash);
   free(hash);
}


/********** Hash usage *************/
static __inline__ BOOL
correct_entry(HASH_ITEM *item, int key, HASH_VAL *seeked_data,
	      HASH_ITEM_TEST *is_correct_item, BOOL skip_deleted)
{
   switch(item->key) {
      case FREE_ENTRY:
	 return TRUE;
	
      case DELETED_ENTRY:
	 return skip_deleted ? FALSE : TRUE;
	
      default:
	 if (item->key != key)
	    return FALSE;
	 if (is_correct_item != NULL)
	    return is_correct_item(&item->data, seeked_data);
	 else 
	    return TRUE;
   }

}

/* The algorithm of the hash (one of the 2 standard hash implementations):
 *   Iterate through the hash table until
 *    1. The entry has been found.
 *    2. A FREE entry has been found.
 *    3. For insert operations only- A DELETED entry has been found.
 *       The difference between DELETED and FREE entires is that
 *       DELETED entry was one occupied, while FREE was never allocated.
 *       The idea behind this structure to keep other entries reachable.
 */

static HASH_ITEM *locate_entry(HASH_CONTAINER* hash, DWORD key,
			       HASH_VAL *seeked_data, BOOL skip_deleted)
{
   DWORD hash_idx, hash_leaps;
   HASH_ITEM *item;
   int i;
   int total_items;
    
   assert(hash);

   total_items=     hash->shared->total_items;
   hash_idx=        key % total_items;
   
   item= &GET_ITEM(hash->items, hash->bytes_per_item, hash_idx);
    
   if (  correct_entry( item, key, seeked_data,
			hash->is_correct_item, skip_deleted)   )
      return item;

   /* get the WORDs in different order in this DWORD to avoid clustering */
   hash_leaps=((DWORD)MAKELONG(HIWORD(key), LOWORD(key))
	       % (total_items-1)) +1;

   /* interate through the hash table using hash_leaps */
   for (i= total_items ; i ; i--) {
      hash_idx+= hash_leaps;
      if (hash_idx > total_items)
	 hash_idx -= total_items;
	
      item= &GET_ITEM(hash->items,hash->bytes_per_item, hash_idx);
      if (  correct_entry( item, key, seeked_data,
			   hash->is_correct_item, skip_deleted)   )
	 return item;
   }
   return NULL;
    
}

static __inline__ void sync_shared_hash(HASH_CONTAINER *hash)
{
    HASH_SHARED *shared= hash->shared;
    
    if (shared->ptr_updates == hash->last_ptr_update)
	return;
    
    assert(shared->ptr_updates >= hash->last_ptr_update);
    hash->last_ptr_update= shared->ptr_updates;
    hash->min_free_items= (1.0 - hash->maximum_load/100.0) *
	shared->total_items + 1 ;

    hash->items= hash->access_mem(shared->items);
}

HASH_VAL *hash_locate_item(HASH_CONTAINER* hash,
			   int key, HASH_VAL *seeked_data)
{
    HASH_ITEM *item;
    
    assert(hash != NULL);
    sync_shared_hash(hash);
    
    item= locate_entry(hash, key, seeked_data, 1 /* skip_deleted */);
    if (item == NULL)
	return NULL;
    if (item->key == FREE_ENTRY )
	return NULL;

    return &item->data;
}


BOOL hash_add_item(HASH_CONTAINER* hash, int key, HASH_VAL *data)
{
    HASH_SHARED *shared;
    HASH_ITEM *item;
    
    assert(hash != NULL);

    sync_shared_hash(hash);
    shared= hash->shared;
    
    item=locate_entry(hash, key, data, 0 /* no skip_deleted */);
    assert(item != NULL);
    if (item->key == key)
	return FALSE;
    if (item->key == FREE_ENTRY)
       shared->free_items--;
    else
       shared->deleted_items--;
    
    item->key=  key;
    memcpy(&item->data, data, hash->bytes_per_item-sizeof(key));

    if (shared->free_items < hash->min_free_items ||
	shared->deleted_items > hash->min_free_items)
       reorder_hash(hash);
    
    return TRUE;
}


BOOL hash_delete_item(HASH_CONTAINER* hash, int key, HASH_VAL *seeked_data)
{
    HASH_ITEM *item;
    
    assert(hash != NULL);
    sync_shared_hash(hash);

    item=locate_entry(hash, key, seeked_data, 1 /* skip_deleted */);
    if (item == NULL)
	return FALSE;

    if (item->key == FREE_ENTRY) 
	return FALSE;

    item->key = DELETED_ENTRY;
    hash->shared->deleted_items++;

    return TRUE;
}

void *ret_null()
{
    return NULL;
}


HASH_ITEM *access_local_hash(HASH_PTR ptr)
{
   return ptr.ptr;
}

#endif  /* CONFIG_IPC */
