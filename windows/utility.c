/*	utility.c	Utility functions for Wine
 *			Author:		acb
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
#include "windows.h"

static char Copyright[] = "Copyright Andrew C. Bulhak, 1993";

/*#define debug_utility*/

/*	MulDiv is a simple function that may as well have been
 *	implemented as a macro; however Microsoft, in their infinite
 *	wisdom, have implemented it as a DLL function and therefore
 *	so should we. 
 *	Basically, it takes two 16-bit integers, multiplies them
 *	and divides by a third integer.
 */

int MulDiv(int foo, int bar, int baz)
{
	return (long)(((int)foo*bar)/baz);
};

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

/*	UTILITY_qualify(source, dest) takes the format string source and
 *	changes all the parameters to correspond to Linux integer sizes
 *	rather than Windows sizes. For example, it converts %i to %hi
 *	and %lx to %x. No array size checking is done at present.
 */

static void UTILITY_qualify(const char *source, char *dest)
{
#ifdef debug_utility
	printf("UTILITY_qualify(\"%s\", \"%s\");\n", source, dest);
#endif
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
#ifdef debug_utility
			printf("found:\t\"%%");
#endif
			format++;		/* skip past '%' */
			/* First skip the flags, field width, etc. */
			/* First the flags */
			if ((*format == '#') || (*format == '-') || (*format == '+')
				|| (*format == ' ')) {
#ifdef debug_utility
				printf("%c", *format);
#endif
				format++;
			}
			/* Now the field width, etc. */
			while(isdigit(*format)) {
#ifdef debug_utility
				printf("%c", *format);
#endif
				format++;
			}
			if(*format == '.') {
#ifdef debug_utility
				printf("%c", *format);
#endif
				format++;
			}
			while(isdigit(*format)) {
#ifdef debug_utility
				printf("%c", *format);
#endif
				format++;
			}
			/* Now we handle the rest */
			if((*format == 'h') || (*format == 'l') || (*format == 'L')) {
#ifdef debug_utility
				printf("%c", modifier);
#endif
				modifier = *(format++);
			}
			/* Handle the actual type. */
#ifdef debug_utility
				printf("%c\"\n", *format);
#endif
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
#ifdef debug_utility
	printf("UTILITY_argsize: returning %i\n", size);
#endif
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

	while(*format) {
		while((*format) && (*format != '%')) format++;	/* skip ahead */
		if(*format) {
			char modifier = ' ';
#ifdef debug_utility
			printf("found:\t\"%%");
#endif
			format++;		/* skip past '%' */
			/* First skip the flags, field width, etc. */
			/* First the flags */
			if ((*format == '#') || (*format == '-') || (*format == '+')
				|| (*format == ' ')) format++;
			/* Now the field width, etc. */
			while(isdigit(*format)) format++;
			if(*format == '.') format++;
			while(isdigit(*format)) format++;
			/* Now we handle the rest */
			if((*format == 'h') || (*format == 'l') || (*format == 'L'))
				modifier = *(format++);
			/* Handle the actual type. */
#ifdef debug_utility
				printf("%c\"\n", *format);
#endif
			switch(*format) {
				case 'd':
				case 'i':
					*(((int *)rptr)++) = (modifier=='l') ? *(((int *)winarg)++) : *(((short *)winarg)++);
					break;
				case 'o':
				case 'x':
				case 'X':
				case 'u':
				case 'c':
					*(((unsigned int *)rptr)++) = (modifier=='l') ? *(((unsigned int *)winarg)++) 
						: *(((unsigned short *)winarg)++);
					break;
				case 's':
				case 'p':
				case 'n':	/* A pointer, is a pointer, is a pointer... */
					*(((char **)rptr)++) = *(((char **)winarg)++);
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
						*(((long double *)rptr)++) = *(((long double *)winarg)++);
					else *(((double *)rptr)++) = *(((double *)winarg)++);
					break;
			}
		}
	}
	return result;
};


/**************************************************************************
 *                wsprintf        [USER.420]
 */
int wsprintf(LPSTR lpOutput, LPSTR lpFormat, ...)
{
va_list  valist;
int      ArgCnt;
va_start(valist, lpFormat);
ArgCnt = vsprintf(lpOutput, lpFormat, valist);
va_end(valist);
return (ArgCnt);
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
	char qualified_fmt[1536];
	char *newargs;
	int result;

	/* 1.5K is a safe value as wvsprintf can only handle buffers up to
	1K and in a worst case such a buffer would look like "%i%i%i..." */

	if(!buf || !format) return 0;

	/* Change the format string so that ints are handled as short by
	   default */
	UTILITY_qualify(format, qualified_fmt);

	/* Convert agruments to 32-bit values */
	newargs = UTILITY_convertArgs(format, args);

	result = vsprintf(buf, qualified_fmt, newargs);
	free(newargs);
	return result;
};
