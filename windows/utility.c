/*	utility.c	Utility functions for Wine
 *			Author:		Andrew Bulhak
 *			Commenced:	10-9-1993
 *
 *			This unit contains the implementations of
 *			various Windows API functions that perform
 *			utility tasks; i.e., that do not fit into
 *			any major category but perform useful tasks.
 */

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdlib.h>
#include "windows.h"
#include "ldt.h"
#include "stackframe.h"
#include "stddebug.h"
#include "debug.h"



/*	MulDiv is a simple function that may as well have been
 *	implemented as a macro; however Microsoft, in their infinite
 *	wisdom, have implemented it as a DLL function and therefore
 *	so should we. 
 *	Basically, it takes two 16-bit integers, multiplies them
 *	and divides by a third integer.
 */

#ifdef WINELIB32
INT MulDiv(INT foo, INT bar, INT baz)
{
#ifdef __GNUC__
    long long ret;
    if (!baz) return -1;
    ret = ((long long)foo * bar) / baz;
    if ((ret > 2147483647) || (ret < -2147483647)) return -1;
    return ret;
#else
    if (!baz) return -1;
    return (foo * bar) / baz;
#endif
}
#else  /* WINELIB32 */
INT MulDiv(INT foo, INT bar, INT baz)
{
    int ret;
    if (!baz) return -32768;
    ret = (foo * bar) / baz;
    if ((ret > 32767) || (ret < -32767)) return -32768;
    return ret;
}
#endif

/*	UTILITY_strip015() removes \015 (^M, CR) from a string;
 *	this is done to convert a MS-DOS-style string to a more
 *	UNIX-friendly format. Replacement is done in-place.
 */

void UTILITY_strip015(char *dest) {
	char *src = dest;

	while(*src) {
		while(*src == '\015') src++;	/* Skip \015s */
		while((*src) && (*src != '\015')) *(dest++) = *(src++);
	};
	*dest = '\0';	/* Add null terminator */
};

/**********************************************************************
 *					DebugPrintString
 */
int
DebugPrintString(char *str)
{
    fprintf(stderr, "%s\n", str);
    return 0;
}

/*
 *	OutputDebugString strips CRs from its (string) parameter and
 *	calls DebugPrintString(), which was written by someone else. 
 *	Since this is part of the standard Windows API, it needs no 
 *	references to nonstandard DLLs.
 */

void OutputDebugString(LPSTR foo)
{
	UTILITY_strip015(foo);
	DebugPrintString(foo);
};

#if 0
/*	UTILITY_qualify(source, dest) takes the format string source and
 *	changes all the parameters to correspond to Linux integer sizes
 *	rather than Windows sizes. For example, it converts %i to %hi
 *	and %lx to %x. No array size checking is done at present.
 */

static void UTILITY_qualify(const char *source, char *dest)
{
	dprintf_utility(stddeb, "UTILITY_qualify(\"%s\", \"%s\");\n", 
			source, dest);
	if(!source) return;	/* Dumbass attack! */
	while(*source) {
		/* Find next format code. */
		while((*source != '%') && (*source)) {
			*(dest++) = *(source++);
		}
		/* Yeah, I know I shouldn't use gotos.... */
		if (!(*source)) goto loop_end;
		/* skip the '%' */
		*(dest++) = *(source++);
		/* Now insert a size qualifier, if needed. */
		switch(*source) {
			case 'i':
			case 'd':
			case 'x':
			case 'X':
			case 'u':
			case 'o':
				/* We have a 16-bit value here. */
				*(dest++) = 'h';
				break;
		};
		/* Here we go 'round the mulberry bush... */
loop_end:
	};
	*dest = '\0';
};
#endif

/*	UTILITY_argsize() evaluates the size of the argument list that
 *	accompanies a vsprintf() or wvsprintf() call.
 *	Arguments:
 *		char *format;	printf-style format string.
 *		BOOL windows;	if this is TRUE, we assume that ints are
 *				16 bits in size; otherwise we deal with
 *				32-bit variables.
 *	Returns:
 *		size (in bytes) of the arguments that follow the call.
 */

size_t UTILITY_argsize(const char *format, BOOL windows)
{
	size_t size = 0;

#define INT_SIZE (windows ? 2 : 4)

	while(*format) {
		while((*format) && (*format != '%')) format++;	/* skip ahead */
		if(*format) {
			char modifier = ' ';
			dprintf_utility(stddeb, "found:\t\"%%");
			format++;		/* skip past '%' */
			/* First skip the flags, field width, etc. */
			/* First the flags */
			if ((*format == '#') || (*format == '-') || (*format == '+')
				|| (*format == ' ')) {
				dprintf_utility(stddeb, "%c", *format);
				format++;
			}
			/* Now the field width, etc. */
			while(isdigit(*format)) {
				dprintf_utility(stddeb, "%c", *format);
				format++;
			}
			if(*format == '.') {
				dprintf_utility(stddeb, "%c", *format);
				format++;
			}
			while(isdigit(*format)) {
				dprintf_utility(stddeb, "%c", *format);
				format++;
			}
			/* Now we handle the rest */
			if((*format == 'h') || (*format == 'l') || (*format == 'L')) {
				dprintf_utility(stddeb, "%c", modifier);
				modifier = *(format++);
			}
			/* Handle the actual type. */
				dprintf_utility(stddeb, "%c\"\n", *format);
			switch(*format) {
				case 'd':
				case 'i':
				case 'o':
				case 'x':
				case 'X':
				case 'u':
				case 'c':
					size += ((modifier == 'l') ? 4 : INT_SIZE);
					break;
				case 's': size += sizeof(char *); break;
				case 'e':
				case 'E':
				case 'f':
				case 'g':
				case 'G':
					/* It doesn't look as if Windows' wvsprintf()
					   supports floating-point arguments. However,
					   I'll leave this code here just in case. */
					size += (modifier == 'L') ? sizeof(long double) : sizeof(double);
					break;
				case 'p': size += sizeof(void *); break;
				case 'n': size += sizeof(int *); break;
			};
		};
	};
#undef INT_SIZE
	dprintf_utility(stddeb, "UTILITY_argsize: returning %i\n", size);
	return size;
};

/*	UTILITY_convertArgs() creates a 32-bit argument list from a 16-bit list.
 *	This is used to allow wvsprintf() arguments to be fed through 
 *	vsprintf().
 *
 *	Arguments:
 *		char *fmt;	format string
 *		char *winarg;	Windows-style arguments
 *	
 *	Returns:
 *		malloc()ed pointer to new argument list. This should
 *		be free()d as soon as it is finished with.
 */

char *UTILITY_convertArgs(char *format, char *winarg)
{
    char *result = (char *)malloc(UTILITY_argsize(format, 0));
    char *rptr = result;
    short *wptr = (short *)winarg;
    long *dptr = (long *)result;
    
    while(*format) {
	while((*format) && (*format != '%')) format++;	/* skip ahead */
	if(*format) {
	    char modifier = ' ';
	    dprintf_utility(stddeb, "found:\t\"%%");
	    format++;		/* skip past '%' */
	    
	    /* First skip the flags, field width, etc. */
	    /* First the flags */
	    if (*format == '#' || *format == '-' || *format == '+'
		|| *format == ' ') format++;
	    
	    /* Now the field width, etc. */
	    while (isdigit(*format)) format++;
	    if (*format == '.') format++;
	    while (isdigit(*format)) format++;
	    
	    /* Now we handle the rest */
	    if(*format == 'h' || *format == 'l' || *format == 'L')
	        modifier = *format++;
	    
	    /* Handle the actual type. */
	    dprintf_utility(stddeb, "%c\"\n", *format);
	    
	    switch(*format) {
	     case 'd':
	     case 'i':
		if (modifier == 'l') {
		    *((int *)rptr)++ = *((int *)winarg)++;
		} else {
		    *((int *)rptr)++ = *((short *)winarg)++;
		}
		break;
	     case 'o':
	     case 'x':
	     case 'X':
	     case 'u':
	     case 'c':
		if (modifier == 'l')  {
		    *((unsigned int *)rptr)++ =  *((unsigned int *)winarg)++;
		} else {
		    *((unsigned int *)rptr)++ = *((unsigned short *)winarg)++;
		}
		break;
	     case 's':
	     case 'p':
	     case 'n':	/* A pointer, is a pointer, is a pointer... */
		*((char **)rptr)++ = (char *)PTR_SEG_TO_LIN(*(SEGPTR *)winarg);
		((SEGPTR *)winarg)++;
		break;
	     case 'e':
	     case 'E':
	     case 'f':
	     case 'g':
	     case 'G':
		/* It doesn't look as if Windows' wvsprintf()
		 supports floating-point arguments. However,
		 I'll leave this code here just in case. */
		if(modifier=='L')
		    *((long double *)rptr)++ = *((long double *)winarg)++;
		else
		    *((double *)rptr)++ = *((double *)winarg)++;
		break;
	    }
	}
    }
    for(; (char *)dptr < rptr; dptr ++)
        dprintf_utility( stddeb, "%08lx ", *dptr );
    dprintf_utility( stddeb, "\n" );
    for(; (char *)wptr < winarg; wptr ++)
        dprintf_utility( stddeb, "%04x ", *wptr );
    dprintf_utility( stddeb, "\n" );
    return result;
};

#ifndef WINELIB
INT windows_wsprintf(void)
{
    LPSTR lpOutput, lpFormat, ptr;
    BYTE new_stack[1024], *stack_ptr;
    BOOL fLarge;

    BYTE *win_stack = (BYTE *)CURRENT_STACK16->args;

    lpOutput = (LPSTR) PTR_SEG_TO_LIN(*(DWORD*)win_stack);
    win_stack += sizeof(DWORD);
    lpFormat = (LPSTR) PTR_SEG_TO_LIN(*(DWORD*)win_stack);
    win_stack += sizeof(DWORD);

      /* create 32-bit stack for libc's vsprintf() */

    for (ptr = lpFormat, stack_ptr = new_stack; *ptr; ptr++)
    {
        if (*ptr != '%' || *++ptr == '%')
            continue;

        /* skip width/precision */
        while (*ptr == '-' || *ptr == '+' || *ptr == '.' ||
               *ptr == ' ' || isdigit(*ptr) || *ptr == '#')
            ptr++;

        /* handle modifier */
        fLarge = ((*ptr == 'l') || (*ptr == 'L'));
        if (fLarge) ptr++;

        switch (*ptr)
        {
        case 's':
            *(char**)stack_ptr = (char *)PTR_SEG_TO_LIN(*(DWORD*)win_stack);
            stack_ptr += sizeof(char *);
            win_stack += sizeof(DWORD);
            break;

        case 'c':
	
/* windows' wsprintf() %c ignores 0's, we replace 0 with SPACE to make sure
   that the remaining part of the string isn't ignored by the winapp */
				
            if (*(WORD*)win_stack)
                *(DWORD*)stack_ptr = *(WORD*)win_stack;
            else
                *(DWORD*)stack_ptr = ' ';
            stack_ptr += sizeof(DWORD);
            win_stack += sizeof(WORD);
            break;

        case 'd':
        case 'i':
            if (!fLarge)
            {
                *(int*)stack_ptr = *(INT*)win_stack;
                stack_ptr += sizeof(int);
                win_stack += sizeof(INT);
                break;
            }
            /* else fall through */
        case 'u':
        case 'x':
        case 'X':
            if (fLarge)
            {
                *(DWORD*)stack_ptr = *(DWORD*)win_stack;
                win_stack += sizeof(DWORD);
            }
            else
            {
                *(DWORD*)stack_ptr = *(WORD*)win_stack;
                win_stack += sizeof(WORD);
            }
            stack_ptr += sizeof(DWORD);
            break;

        default:
            *(DWORD*)stack_ptr = 0;
            stack_ptr += sizeof(DWORD);
            win_stack += sizeof(WORD);
            fprintf(stderr, "wsprintf: oops, unknown formattype %c used!\n", *ptr);
            break;
        }
    }

    return vsprintf(lpOutput, lpFormat, new_stack);
}
#endif

/**************************************************************************
 *                wsprintf        [USER.420] (not used by relay)
 */
int wsprintf(LPSTR lpOutput, LPSTR lpFormat, ...)
{
	va_list valist;
	int ArgCnt;

	va_start(valist, lpFormat);
	ArgCnt = vsprintf(lpOutput, lpFormat, valist);
	va_end(valist);

	return ArgCnt;
}


/*	wvsprintf() is an implementation of vsprintf(). This
 *	implementation converts the arguments to 32-bit integers and
 *	calls the standard library function vsprintf().
 *
 *	Known shortcomings:
 *		wvsprintf() doesn't yet convert the arguments back after
 *		calling vsprintf(), so if Windows implements %n and a
 *		program depends on it, we're in trouble.
 */

int wvsprintf(LPSTR buf, LPSTR format, LPSTR args)
{
	char *newargs;
	int result;

	if(!buf || !format) return 0;

	/* Convert agruments to 32-bit values */
	newargs = UTILITY_convertArgs(format, args);
	result = vsprintf(buf, format, newargs);

	free(newargs);
	return result;
};

