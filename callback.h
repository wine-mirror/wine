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


/*
 * Windows procedure calling:  
 *     f(a, b, c, d)
 *     wndprocfunc(HWND hwnd, WORD message, WORD wParam, LONG lParam)
 */
#define CALLWNDPROC(f, a, b, c, d) \
    CallBack16((f), 4, CALLBACK_SIZE_WORD, (a), CALLBACK_SIZE_WORD, (b), \
	       CALLBACK_SIZE_WORD, (c), CALLBACK_SIZE_LONG, (d))

#endif /* CALLBACK_H */
