/*
 * Console I/O definitions
 *
 * Derived from the mingw header written by Colin Peters.
 * Modified for Wine use by Jon Griffiths and Francois Gouget.
 * This file is in the public domain.
 */
#ifndef __WINE_CONIO_H
#define __WINE_CONIO_H
#define __WINE_USE_MSVCRT


#ifdef __cplusplus
extern "C" {
#endif

char*       _cgets(char*);
int         _cprintf(const char*,...);
int         _cputs(const char*);
int         _cscanf(const char*,...);
int         _getch(void);
int         _getche(void);
int         _kbhit(void);
int         _putch(int);
int         _ungetch(int);

#ifdef _M_IX86
int         _inp(unsigned short);
unsigned long _inpd(unsigned short);
unsigned short _inpw(unsigned short);
int         _outp(unsigned short, int);
unsigned long _outpd(unsigned short, unsigned long);
unsigned short _outpw(unsigned short, unsigned short);
#endif

#ifdef __cplusplus
}
#endif


#ifndef USE_MSVCRT_PREFIX
#define cgets _cgets
#define cprintf _cprintf
#define cputs _cputs
#define cscanf _cscanf
#define getch _getch
#define getche _getche
#define kbhit _kbhit
#define putch _putch
#define ungetch _ungetch
#ifdef _M_IX86
#define inp _inp
#define inpw _inpw
#define outp _outp
#define outpw _outpw
#endif
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_CONIO_H */
