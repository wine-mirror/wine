/***************************************************************************
 * Copyright 1995 Michael Veksler. mveksler@vnet.ibm.com
 ***************************************************************************
 * File:      hash_test.c
 * Purpose :  test generic_hash correctness.
 * NOTE:
 *   This code covers only about 80% of generic_hash code.
 *   There might be bugs in the remaining 20% - although most
 *   of the functionality is tested with wine linckage.
 *   For complete testing a little more work should be done.
 ***************************************************************************
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "generic_hash.h"

#define SIZE 200
typedef struct { int a,b;} DATA ;
DATA data[SIZE];
int keys[SIZE];
int peeks=0;

HASH_CONTAINER *hash1;
HASH_CONTAINER *hash2;		/* actual data is shared with hash1 */

/* test insertion using keys[] and data[] inserting using hash1 and */
/* hash2 periodically, test hash after every 2 insertions */
void test_insert()
{
    int i,j;
    HASH_VAL *item;
    
    printf("testing insertion \n");
    for (i=0 ; i < SIZE-1 ; i+=2) {
	assert(hash_add_item(hash1, keys[i], (HASH_VAL *)&data[i]));
	assert(hash_add_item(hash2, keys[i+1], (HASH_VAL *)&data[i+1]));
	for (j=0 ; j <= i+1 ; j++) {
	    item= hash_locate_item(hash1, keys[j], (HASH_VAL *)&data[j]);
	    if (item == NULL) {
		printf("NULL item: i=%d,j=%d\n",i,j);
		continue;
	    }
	    peeks++;
	    if (memcmp(item,&data[j],sizeof(DATA))!=0) {
		printf("i=%d,j=%d\n",i,j);
		printf("saved=(%d,%d), orig=(%d,%d)\n",
		       ((DATA*)item)->a, ((DATA*)item)->b,
		       data[j].a, data[j].b);
	    }
	}
    } 
}

/* test deletion using keys[] and data[] deleting using hash1 and */
/* hash2 periodicly, test hash after every 2 deletions */
void test_delete()
{
    int i,j;
    HASH_VAL *item;

    printf("testing deletion\n");
    for (i=0 ; i < SIZE-1 ; i+=2) {
	assert(hash_delete_item(hash2, keys[i], NULL));
	assert(hash_delete_item(hash1, keys[i+1], NULL));
	for (j=0 ; j < SIZE ; j++) {
	    item= hash_locate_item(hash2, keys[j], (HASH_VAL *)&data[j]);
	    if (item == NULL) {
		if ( j > i+1) 
		    printf("NULL item: i=%d,j=%d\n",i,j);
		continue;
	    }
	    if (item != NULL && j <= i+1) {
		printf("Non NULL item: i=%d,j=%d\n",i,j);
		continue;
	    }
	    if (memcmp(item,&data[j],sizeof(DATA))!=0) {
		printf("i=%d,j=%d\n",i,j);
		printf("saved=(%d,%d), orig=(%d,%d)\n",
		       ((DATA*)item)->a, ((DATA*)item)->b,
		       data[j].a, data[j].b);
	    }
	}
    } 

}


int main()
{
    int i;
    
    hash1= create_hash(sizeof(DATA), 1);
    assert(hash1);
    hash2= attach_remote_hash(hash1->shared, sizeof(DATA), HASH_MEM_ACCESS);
    assert(hash2);

    for (i=0 ; i< SIZE ; i++) {
	data[i].a= rand();
	data[i].b= rand();
	keys[i]= rand();
    }

    test_insert();
    detach_hash(hash1);
    free(hash1);
    hash1= attach_remote_hash(hash2->shared, sizeof(DATA), HASH_MEM_ACCESS);
    
    test_delete();
    test_insert();

    detach_hash(hash1);
    destroy_hash(hash2);
    printf("peeks=%d\n", peeks);
    return 0;
}
