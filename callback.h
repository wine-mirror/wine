/* $Id$
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */

#ifndef CALLBACK_H
#define CALLBACK_H

#include <stdlib.h>
#include <stdarg.h>

#define CALLBACK_SIZE_WORD	0
#define CALLBACK_SIZE_LONG	1

extern int CallTo16(unsigned int csip, unsigned short ds);
extern int CallBack16(void *func, int n_args, ...);


#endif /* CALLBACK_H */
