/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      bit_array.c
 * Purpose :  manipulate array of bits
 * Portability: This is not completely portable, non CISC arcitectures
 *              Might not have atomic Clear/Set/Toggle bit. On those
 *              architectures semaphores should be used.
 * Big Endian Concerns: This code is big endian compatible,
 *              but the byte order will be different (i.e. bit 0 will be
 *              located in byte 3).
 ***************************************************************************
 */

#ifdef CONFIG_IPC

/*
** uncoment the following line to disable assertions,
** this may boost performance by up to 50%
*/
/* #define NDEBUG */

#if defined(linux) && !defined(NO_ASM)
#include <linux/version.h>
#if LINUX_VERSION_CODE <= 131328 /* Linux 2.1.x doesn't return values with clear_bit and set_bit */
#define HAS_BITOPS
#endif
#endif

#include <stdio.h>

#include <assert.h>

#include "bit_array.h"
#ifdef HAS_BITOPS
#include <asm/bitops.h>
#else
static inline int clear_bit(int bit, int *mem);
static inline int set_bit(int bit, int *mem);
#endif /* HAS_BITOPS */


#define INT_NR(bit_nr)       ((bit_nr) >> INT_LOG2)
#define INT_COUNT(bit_count) INT_NR( bit_count + BITS_PER_INT - 1 )
#define BIT_IN_INT(bit_nr)   ((bit_nr) & (BITS_PER_INT - 1))

#if !defined(HAS_BITOPS)

/* first_zero maps bytes value to the index of first zero bit */
static char first_zero[256];
static int arrays_initialized=0;


/*
** initialize static arrays used for bit operations speedup.
** Currently initialized: first_zero[256]
** set "arrays_initialized" to inidate that arrays where initialized
*/

static void initialize_arrays()
{
  int i;
  int bit;

  for (i=0 ; i<256 ; i++) {
     /* find the first zero bit in `i' */
     for (bit=0 ; bit < BITS_PER_BYTE ; bit++)
	/* break if the bit is zero */
	if ( ( (1 << bit) & i )
	     == 0)
	   break;
     first_zero[i]= bit;
  }
  arrays_initialized=1;
}

/*
** Find first zero bit in the integer.
** Assume there is at least one zero.
*/
static inline int find_zbit_in_integer(unsigned int integer)
{
  int i;

  /* find the zero bit */
  for (i=0 ; i < sizeof(int) ; i++, integer>>=8) {
     int byte= integer & 0xff;

     if (byte != 0xff)
	return ( first_zero[ byte ]
		 + (i << BYTE_LOG2) );
  }
  assert(0);			   /* never reached */
  return 0;
}

/* return -1 on failure */
static inline int find_first_zero_bit(unsigned *array, int bits)
{
  unsigned int  integer;
  int i;
  int bytes=INT_COUNT(bits);

  if (!arrays_initialized)
     initialize_arrays();

  for ( i=bytes ; i ; i--, array++) {
     integer= *array;

     /* test if integer contains a zero bit */
     if (integer != ~0U)
	return ( find_zbit_in_integer(integer)
		 + ((bytes-i) << INT_LOG2) );
  }

  /* indicate failure */
  return -1;
}

static inline int test_bit(int pos, unsigned *array)
{
  unsigned int integer;
  int bit = BIT_IN_INT(pos);

  integer= array[ pos >> INT_LOG2 ];

  return (  (integer & (1 << bit)) != 0
	    ? 1
	    : 0 ) ;
}

/*
** The following two functions are x86 specific ,
** other processors will need porting
*/

/* inputs: bit number and memory address (32 bit) */
/* output: Value of the bit before modification */
static inline int clear_bit(int bit, int *mem)
{
  int ret;

  __asm__("xor %1,%1\n"
	  "btrl %2,%0\n"
	  "adcl %1,%1\n"
	  :"=m" (*mem), "=&r" (ret)
	  :"r" (bit));
  return (ret);
}

static inline int set_bit(int bit, int *mem)
{
  int ret;
  __asm__("xor %1,%1\n"
	  "btsl %2,%0\n"
	  "adcl %1,%1\n"
	  :"=m" (*mem), "=&r" (ret)
	  :"r" (bit));
  return (ret);
}

#endif /* !deined(HAS_BITOPS) */


/* AssembleArray: assemble an array object using existing data */
bit_array *AssembleArray(bit_array *new_array, unsigned int *buff, int bits)
{
  assert(new_array!=NULL);
  assert(buff!=NULL);
  assert(bits>0);
  assert((1 << INT_LOG2) == BITS_PER_INT); /* if fails, redefine INT_LOG2 */

  new_array->bits=bits;
  new_array->array=buff;
  return new_array;
}

/* ResetArray: reset the bit array to zeros */
int ResetArray(bit_array *bits)
{
  int i;
  int *p;

  assert(bits!=NULL);
  assert(bits->array!=NULL);

  for(i= INT_COUNT(bits->bits), p=bits->array; i ; p++, i--)
     *p=0;
  return 1;
}


/* VacantBit: find a vacant (zero) bit in the array,
 * Return: Bit index on success, -1 on failure.
 */
int VacantBit(bit_array *bits)
{
  int bit;

  assert(bits!=NULL);
  assert(bits->array!=NULL);

  bit= find_first_zero_bit(bits->array, bits->bits);

  if (bit >= bits->bits)	   /* failed? */
     return -1;

  return bit;
}

int SampleBit(bit_array *bits, int i)
{
  assert(bits != NULL);
  assert(bits->array != NULL);
  assert(i >= 0  &&  i < bits->bits);

  return ( test_bit(i,bits->array) != 0
	   ? 1
	   : 0
	   );
}


/*
** Use "compare and exchange" mechanism to make sure
** that bits are not modified while "integer" value
** is calculated.
**
** This may be the slowest technique, but it is the most portable
** (Since most architectures have compare and exchange command)
*/
int AssignBit(bit_array *bits, int bit_nr, int val)
{
  int ret;

  assert(bits != NULL);
  assert(bits->array != NULL);
  assert(val==0 || val==1);
  assert(bit_nr >= 0  &&  bit_nr < bits->bits);

  if (val==0)
     ret= clear_bit(BIT_IN_INT(bit_nr), &bits->array[ INT_NR(bit_nr) ]);
  else
     ret= set_bit(BIT_IN_INT(bit_nr), &bits->array[ INT_NR(bit_nr) ]);

  return ( (ret!=0) ? 1 : 0);
}

/*
** Allocate a free bit (==0) and make it used (==1).
** This operation is guaranteed to resemble an atomic instruction.
**
** Return: allocated bit index, or -1 on failure.
**
** There is a crack between locating free bit, and allocating it.
** We assign 1 to the bit, test it was not '1' before the assignment.
** If it was, restart the seek and assign cycle.
**
*/

int AllocateBit(bit_array *bits)
{
  int bit_nr;
  int orig_bit;

  assert(bits != NULL);
  assert(bits->array != NULL);

  do {
     bit_nr= VacantBit(bits);

     if (bit_nr == -1)		   /* No vacant bit ? */
	return -1;

     orig_bit = AssignBit(bits, bit_nr, 1);
  } while (orig_bit != 0);	   /* it got assigned before we tried */

  return bit_nr;
}

#endif  /* CONFIG_IPC */
