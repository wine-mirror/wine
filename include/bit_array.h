/***************************************************************************
 * Copyright 1995, Technion, Israel Institute of Technology
 * Electrical Eng, Software Lab.
 * Author:    Michael Veksler.
 ***************************************************************************
 * File:      bit_array.h
 * Purpose :  manipulate array of bits,
 * Important: operations may be considered atomic.
 *
 ***************************************************************************
 */
#ifndef __WINE_BIT_ARRAY_H
#define __WINE_BIT_ARRAY_H


#define BITS_PER_BYTE (8)
#define BITS_PER_INT (sizeof(int)*BITS_PER_BYTE) /* must be power of 2 */

#define BYTE_LOG2 (3)
#if defined(INT_LOG2)
/* nothing to do, IN_LOG2 is ok */
#elif defined(__i386__)
#  define INT_LOG2 (5)
#else
#  error "Can't find log2 of BITS_PER_INT, please code it manualy"
#endif


typedef struct bit_array {
	int bits;		   /* number of bits in the array */
	unsigned int *array;	   /* Actual array data (Never NULL) */
} bit_array ;

bit_array *AssembleArray(bit_array *new_array, unsigned int *buff, int bits);
int ResetArray(bit_array *bits);

/* Return index of first free bit, or -1 on failure */
int VacantBit(bit_array *bits);


/* Return the value of bit 'i' */
int SampleBit(bit_array *bits, int i);

/* Assign 'val' to a bit no. 'i'.      Return: old bit's value */
int AssignBit(bit_array *bits, int i, int val);

/*
** Allocate a free bit (==0) and make it used (==1).
** Return: allocated bit index, or -1 on failure.
*/
int AllocateBit(bit_array *bits);

#endif /* __WINE_BIT_ARRAY_H */
