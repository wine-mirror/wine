/***************************************************************************
 * Copyright 1995    Michael Veksler. mveksler@vnet.ibm.com
 ***************************************************************************
 * File:      generic_hash.h 
 * Purpose :  dynamically growing hash, may use shared or local memory.
 ***************************************************************************
 */
#ifndef _GENERIC_HASH_H_
#define _GENERIC_HASH_H_

#include "wintypes.h"
#include "shm_block.h"
#include "win.h"
/* default hash values */
#define HASH_LOAD           70
#define HASH_MEM_ALLOC      (HASH_PTR (*)(int size)) malloc
#define HASH_MEM_FREE       (void (*)(HASH_PTR)) free
#define HASH_MEM_ACCESS     access_local_hash
#define HASH_REALLOC_JUMPS  1.5	 /* Relative size of the new memory */
#define MIN_HASH            13

typedef union {
    char string[1];
    WORD words[1];
    DWORD dwords[1];
    char *ptr;
    SEGPTR segptr;
} HASH_VAL;

typedef struct hash_item_struct {
    DWORD key;
    HASH_VAL data;
} HASH_ITEM;

/* point to the hash structure */
typedef union {
    HASH_ITEM* ptr;		/* Local pointer */
    REL_PTR    rel;			/* IPC relative address */
    SEGPTR segptr;		/* Universal (can be IPC or local) */
} HASH_PTR;

typedef struct hash_share_struct {
    int total_items;		/* total number of items (array size) */
    int free_items;		/* number of free items (excluding deleted) */
    int deleted_items;		/* number of deleted items */
    int ptr_updates;		/* Number of updates to `items' pointer */
				/* (of items) - used for intecepting */
				/* changes to the pointer. */
    HASH_PTR items;		/* pointer to the items */
} HASH_SHARED;
typedef BOOL HASH_ITEM_TEST(HASH_VAL *value, HASH_VAL *seeked_data);

/* NOTE:
 * 1. Keys 0 and -1 are reserved.
 * 2. none of these items should be accessed directly, use existing
 *    functions. If they are not enough, add a new function.
 */
typedef struct hash_container_struct {
    int bytes_per_item;
    int maximum_load;		/* in percents (0..100) default is 70 */
    int min_free_items;		/* minimum free items before reallocating 
				   (Function of maximum_load) */

    int last_ptr_update;	/* to be compared with shared.ptr_updates */
    BOOL shared_was_malloced;	/* Need that to know how to destroy hash */
    
    /* This is an optional handler.
     * If not NULL, this function is used for distinguishing between
     * different data with the same key (key field holds integer and
     * is too short for long keys like strings).
     */
    HASH_ITEM_TEST *is_correct_item;

    /* Handlers used for reallocating memory
     * [by allocating new data and then freeing old data]
     */
    HASH_PTR (*allocate_mem)(int size);
    void     (*free_mem)(HASH_PTR);

    /* Translator from HASH_PTR construct to a regular pointer.
       use HASH_MEM_ACCESS, if no translation is needed */
    HASH_ITEM *(*access_mem)(HASH_PTR);

    HASH_ITEM *items;
    HASH_SHARED *shared;	/* Things to be on shared memory. */
} HASH_CONTAINER;


/********** Hash maintenance functions ***********/



/* Attach existing & running remote (i.e. shared) hash.
 * Attach the items using the data stored in "shared"
 */
HASH_CONTAINER *attach_remote_hash(HASH_SHARED *shared, int bytes_per_datum,
				   HASH_ITEM *(*access_mem)(HASH_PTR));


HASH_CONTAINER *create_remote_hash(HASH_SHARED *shared,
				   int bytes_per_datum,
				   int total_items,
				   HASH_PTR (*allocate_mem)(int size),
				   HASH_ITEM *(*access_mem)(HASH_PTR));
/* hash constructor: create brand new hash (not on shared memory) */
HASH_CONTAINER *create_hash(int bytes_per_datum, int total_items);

/* set the extra handlers to non default values */
void set_hash_handlers(HASH_CONTAINER *hash,
		       HASH_ITEM_TEST *is_correct_item,
		       HASH_PTR (*allocate_mem)(int size),
		       void     (*free_mem)(HASH_PTR),
		       HASH_ITEM *(*access_mem)(HASH_PTR));

/* set extra parameters */
void set_hash_parameters(HASH_CONTAINER *hash, int load);

/* hash destructors */
void destroy_hash(HASH_CONTAINER *hash);
void detach_hash(HASH_CONTAINER *hash);


/********** Hash usage *************/

/* All following functions have the same format:
 *  hash- the hash structure to use
 *  key-  used as primary means to get to the entry.
 *  data- 1. a secondary key (used only if `is_correct_item' is set).
 *        2. data to store. (for hash_add_item).
 */
HASH_VAL *hash_locate_item(HASH_CONTAINER* hash,int key, HASH_VAL* seeked_data);
BOOL hash_add_item(HASH_CONTAINER*        hash, int key, HASH_VAL* data);
BOOL hash_delete_item(HASH_CONTAINER*     hash, int key, HASH_VAL* seeked_data);


void *ret_null();		/* function returning null (used for */
				/* disabling memory reallocation) */

/* access function used by local (non IPC) memory */
HASH_ITEM *access_local_hash(HASH_PTR ptr);

#endif /* _GENERIC_HASH_H_ */
