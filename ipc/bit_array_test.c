#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "bit_array.h"
#define SIZE (8*sizeof(int)*3)

static bit_array array;
static int simple_array[SIZE];
static int bits;

int are_equal()
{
  int i;
  for (i=0 ; i < SIZE ; i++)
     if (SampleBit(&array,i) != simple_array[i]){
	printf("failed bit %d (packed=%d, simple=%d)\n", i,
	       SampleBit(&array,i), simple_array[i]);
	return 0;
     }
  return 1;
}

int is_same_vacant()
{
  int vacant;
  for (vacant =0 ; simple_array[vacant]!=0  ; vacant++)
     if ( vacant >= SIZE) {
	vacant=-1;
	break;
     }


  if ( VacantBit(&array) == vacant )
     return 1;
  else
     return 0;
}
void assign_both(int bit_nr, int bit_val)
{
  int old_bit= simple_array[bit_nr];

  simple_array[bit_nr]= bit_val;

  bits+=bit_val - old_bit;

  assert(AssignBit(&array, bit_nr, bit_val) == old_bit);
  assert(are_equal());
  assert(is_same_vacant());
}


int main()
{
  unsigned int integers[SIZE >> 5];
  int i,j;

  assert( AssembleArray(&array, integers, SIZE)
	  == &array);
  ResetArray(&array);
  for (i=0 ; i<SIZE ; i++)
     simple_array[i]=0;

  for (j=5 ; j ; j--) {
     printf("\rleft %d\r",j);

     for (i=0 ; VacantBit(&array) != -1 ; i++ ) {
	if (i % 256 == 0) {
	   printf("left %d ",j);
	   printf("%3d up  \r", bits);
	   fflush(stdout);
	}
	assign_both(rand() % SIZE,
		    (rand()% SIZE > bits ) ? 0 : 1 );
     }

     assign_both(rand() % SIZE, 1);

     for (i=0 ; bits ; i++ ) {
	if (i % 256 == 0) {
	   printf("left %d ",j);
	   printf("%3d down\r", bits);
	   fflush(stdout);
	}
	assign_both(rand() % SIZE,
		    (rand()% SIZE <= (SIZE-bits) ) ? 0 : 1 );
     }

     assign_both(rand() % SIZE, 0);
  }

  putchar('\n');
  return 0;
}
