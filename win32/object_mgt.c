/*
 *      object_mgt.c - Kernel object management functions
 *
 *  Revision History:
 *
 *  Oct 28 1995 C. Heide
 *              First created.
 */

#include <stdio.h>
#include <stdlib.h>

#include "handle32.h"

int ValidateKernelObject(KERNEL_OBJECT *ptr)
{
  return (!ptr || (short int)ptr==-1);
}

