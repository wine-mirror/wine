/*
 * String functions
 *
 * Copyright 1993 Yngvi Sigurjonsson (yngvi@hafro.is)
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "windows.h"
#include "ldt.h"
#include "module.h"
#include "stackframe.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

#define ToUpper(c)	toupper(c)
#define ToLower(c)	tolower(c)


static const BYTE Oem2Ansi[256] =
"\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\244"
"\020\021\022\023\266\247\026\027\030\031\032\033\034\035\036\037"
"\040\041\042\043\044\045\046\047\050\051\052\053\054\055\056\057"
"\060\061\062\063\064\065\066\067\070\071\072\073\074\075\076\077"
"\100\101\102\103\104\105\106\107\110\111\112\113\114\115\116\117"
"\120\121\122\123\124\125\126\127\130\131\132\133\134\135\136\137"
"\140\141\142\143\144\145\146\147\150\151\152\153\154\155\156\157"
"\160\161\162\163\164\165\166\167\170\171\172\173\174\175\176\177"
"\307\374\351\342\344\340\345\347\352\353\350\357\356\354\304\305"
"\311\346\306\364\366\362\373\371\377\326\334\242\243\245\120\203"
"\341\355\363\372\361\321\252\272\277\137\254\275\274\241\253\273"
"\137\137\137\246\246\246\246\053\053\246\246\053\053\053\053\053"
"\053\055\055\053\055\053\246\246\053\053\055\055\246\055\053\055"
"\055\055\055\053\053\053\053\053\053\053\053\137\137\246\137\137"
"\137\337\137\266\137\137\265\137\137\137\137\137\137\137\137\137"
"\137\261\137\137\137\137\367\137\260\225\267\137\156\262\137\137";

static const BYTE Ansi2Oem[256] =
"\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017"
"\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037"
"\040\041\042\043\044\045\046\047\050\051\052\053\054\055\056\057"
"\060\061\062\063\064\065\066\067\070\071\072\073\074\075\076\077"
"\100\101\102\103\104\105\106\107\110\111\112\113\114\115\116\117"
"\120\121\122\123\124\125\126\127\130\131\132\133\134\135\136\137"
"\140\141\142\143\144\145\146\147\150\151\152\153\154\155\156\157"
"\160\161\162\163\164\165\166\167\170\171\172\173\174\175\176\177"
"\200\201\054\237\054\137\375\374\210\045\123\074\117\215\216\217"
"\220\140\047\042\042\371\055\137\230\231\163\076\157\235\236\131"
"\040\255\233\234\017\235\335\025\042\143\246\256\252\055\162\137"
"\370\361\375\063\047\346\024\372\054\061\247\257\254\253\137\250"
"\101\101\101\101\216\217\222\200\105\220\105\105\111\111\111\111"
"\104\245\117\117\117\117\231\170\117\125\125\125\232\131\137\341"
"\205\240\203\141\204\206\221\207\212\202\210\211\215\241\214\213"
"\144\244\225\242\223\157\224\366\157\227\243\226\201\171\137\230";

/* Funny to divide them between user and kernel. */

/* KERNEL.89 */
SEGPTR lstrcat( SEGPTR target, SEGPTR source )
{
    strcat( (char *)PTR_SEG_TO_LIN(target), (char *)PTR_SEG_TO_LIN(source) );
    return target;
}

/* USER.430 */
INT lstrcmp(LPCSTR str1,LPCSTR str2)
{
    return strcmp( str1, str2 );
}

/* USER.471 */
INT lstrcmpi( LPCSTR str1, LPCSTR str2 )
{
    INT res;

    while (*str1)
    {
        if ((res = toupper(*str1) - toupper(*str2)) != 0) return res;
        str1++;
        str2++;
    }
    return toupper(*str1) - toupper(*str2);
}

/* Not a Windows API*/
INT lstrncmpi( LPCSTR str1, LPCSTR str2, int n )
{
    INT res;

    if (!n) return 0;
    while ((--n > 0) && *str1)
    {
        if ((res = toupper(*str1) - toupper(*str2)) != 0) return res;
        str1++;
        str2++;
    }
    return toupper(*str1) - toupper(*str2);
}

/* KERNEL.88 */
SEGPTR lstrcpy( SEGPTR target, SEGPTR source )
{
    strcpy( (char *)PTR_SEG_TO_LIN(target), (char *)PTR_SEG_TO_LIN(source) );
    return target;
}

/* KERNEL.353 32-bit version*/
LPSTR lstrcpyn( LPSTR dst, LPCSTR src, int n )
{
    char *tmp = dst;
    while(n-- > 1 && *src)
    	*tmp++ = *src++;
    *tmp = 0;
    return dst;
}

/* KERNEL.353 16-bit version*/
SEGPTR WIN16_lstrcpyn( SEGPTR target, SEGPTR source, WORD n )
{
    lstrcpyn((char *)PTR_SEG_TO_LIN(target), (char *)PTR_SEG_TO_LIN(source),n);
    return target;
}

/* KERNEL.90 */
INT lstrlen(LPCSTR str)
{
  /* looks weird, but win3.1 KERNEL got a GeneralProtection handler
   * in lstrlen() ... we check only for NULL pointer reference.
   * - Marcus Meissner
   */
  if (str==NULL) {
  	fprintf(stddeb,"lstrlen(NULL) caught, returning 0.\n");
  	return 0;
  }
  return strlen(str);
}

/* IsCharAlpha USER 433 */
BOOL IsCharAlpha(char ch)
{
  return isalpha(ch);   /* This is probably not right for NLS */
}

/* IsCharAlphanumeric USER 434 */
BOOL IsCharAlphanumeric(char ch)
{
  return (ch < '0') ? 0 : (ch <= '9');
}

/* IsCharUpper USER 435 */
BOOL IsCharUpper(char ch)
{
  return isupper(ch);
}

/* IsCharLower USER 436 */
BOOL IsCharLower(char ch)
{
  return islower(ch);
}

/***********************************************************************
 *           AnsiUpper   (USER.431)
 */

/* 16-bit version */
SEGPTR WIN16_AnsiUpper( SEGPTR strOrChar )
{
  /* I am not sure if the locale stuff works with toupper, but then again 
     I am not sure if the Linux libc locale stuffs works at all */

    /* uppercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        char *s = PTR_SEG_TO_LIN(strOrChar);
        while (*s) {
	    *s = ToUpper( *s );
	    s++;
	}
        return strOrChar;
    }
    else return (SEGPTR)ToUpper( (int)strOrChar );
}

/* 32-bit version */
LPSTR AnsiUpper(LPSTR strOrChar)
{
    char *s = strOrChar;
  /* I am not sure if the locale stuff works with toupper, but then again 
     I am not sure if the Linux libc locale stuffs works at all */

    while (*s) {
	*s = ToUpper( *s );
	s++;
    }
    return strOrChar;
}


/***********************************************************************
 *           AnsiUpperBuff   (USER.437)
 */
UINT AnsiUpperBuff(LPSTR str,UINT len)
{
  int i;
  len=(len==0)?65536:len;

  for(i=0;i<len;i++)
    str[i]=toupper(str[i]);
  return i;	
}

/***********************************************************************
 *           AnsiLower   (USER.432)
 */

/* 16-bit version */
SEGPTR WIN16_AnsiLower(SEGPTR strOrChar)
{
  /* I am not sure if the locale stuff works with toupper, but then again 
     I am not sure if the Linux libc locale stuffs works at all */

    /* lowercase only one char if strOrChar < 0x10000 */
    if (HIWORD(strOrChar))
    {
        char *s = PTR_SEG_TO_LIN( strOrChar );
        while (*s) {	    
	    *s = ToLower( *s );
	    s++;
	}
        return strOrChar;
    }
    else return (SEGPTR)ToLower( (int)strOrChar );
}

/* 32-bit version */
LPSTR AnsiLower(LPSTR strOrChar)
{
    char *s = strOrChar;
  /* I am not sure if the locale stuff works with toupper, but then again 
     I am not sure if the Linux libc locale stuffs works at all */

    while (*s) {
	*s = ToLower( *s );
	s++;
    }
    return strOrChar;
}


/***********************************************************************
 *           AnsiLowerBuff   (USER.438)
 */
UINT AnsiLowerBuff(LPSTR str,UINT len)
{
  int i;
  len=(len==0)?65536:len;
  i=0;

  for(i=0;i<len;i++)
    str[i]=tolower(str[i]);
 
  return i;	
}


/* AnsiNext USER.472 */
SEGPTR AnsiNext(SEGPTR current)
{
    return (*(char *)PTR_SEG_TO_LIN(current)) ? current + 1 : current;
}

/* AnsiPrev USER.473 */
SEGPTR AnsiPrev( SEGPTR start, SEGPTR current)
{
    return (current==start)?start:current-1;
}


/* AnsiToOem Keyboard.5 */
INT AnsiToOem(LPSTR lpAnsiStr, LPSTR lpOemStr)
{
    dprintf_keyboard(stddeb, "AnsiToOem: %s\n", lpAnsiStr);
    while(*lpAnsiStr){
	*lpOemStr++=Ansi2Oem[(unsigned char)(*lpAnsiStr++)];
    }
    *lpOemStr = 0;
    return -1;
}

/* OemToAnsi Keyboard.6 */
BOOL OemToAnsi(LPSTR lpOemStr, LPSTR lpAnsiStr)
{
    dprintf_keyboard(stddeb, "OemToAnsi: %s\n", lpOemStr);
    while(*lpOemStr){
	*lpAnsiStr++=Oem2Ansi[(unsigned char)(*lpOemStr++)];
    }
    *lpAnsiStr = 0;
    return -1;
}

/* AnsiToOemBuff Keyboard.134 */
void AnsiToOemBuff(LPCSTR lpAnsiStr, LPSTR lpOemStr, UINT nLength)
{
  int i;
  for(i=0;i<nLength;i++)
    lpOemStr[i]=Ansi2Oem[(unsigned char)(lpAnsiStr[i])];
}

/* OemToAnsi Keyboard.135 */
void OemToAnsiBuff(LPSTR lpOemStr, LPSTR lpAnsiStr, INT nLength)
{
  int i;
  for(i=0;i<nLength;i++)
    lpAnsiStr[i]=Oem2Ansi[(unsigned char)(lpOemStr[i])];
}


/***********************************************************************
 *           OutputDebugString   (KERNEL.115)
 */
void OutputDebugString( LPCSTR str )
{
    char *module;
    char *p, *buffer = xmalloc( strlen(str)+1 );
    /* Remove CRs */
    for (p = buffer; *str; str++) if (*str != '\r') *p++ = *str;
    *p = '\0';
    if ((p > buffer) && (p[-1] == '\n')) p[1] = '\0'; /* Remove trailing \n */
    module = MODULE_GetModuleName( GetExePtr(GetCurrentTask()) );
    fprintf( stderr, "OutputDebugString: %s says '%s'\n",
             module ? module : "???", buffer );
             
}


/***********************************************************************
 *           wsprintf   (USER.420)
 */
#ifndef WINELIB
int wsprintf( LPSTR dummy1, LPSTR dummy2, ... )
{
    LPSTR lpOutput, lpFormat;
    DWORD *win_stack = (DWORD *)CURRENT_STACK16->args;

    lpOutput = (LPSTR) PTR_SEG_TO_LIN(*win_stack);
    win_stack++;
    lpFormat = (LPSTR) PTR_SEG_TO_LIN(*win_stack);
    win_stack++;

    return wvsprintf( lpOutput, lpFormat, (LPCSTR)win_stack );
}
#else  /* WINELIB */
int wsprintf(LPSTR lpOutput, LPSTR lpFormat, ...)
{
    va_list valist;
    int ArgCnt;

    va_start(valist, lpFormat);
    ArgCnt = vsprintf(lpOutput, lpFormat, valist);
    va_end(valist);

    return ArgCnt;
}
#endif  /* WINELIB */


/***********************************************************************
 *           wvsprintf   (USER.421)
 */
int wvsprintf( LPSTR buf, LPCSTR format, LPCSTR args )
{
    LPCSTR ptr;
    DWORD stack[512], *stack_ptr;
    BOOL fLarge;

    /* Create the 32-bit stack for libc's vsprintf() */

    for (stack_ptr = stack, ptr = format; *ptr; ptr++)
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
            *stack_ptr++ = (DWORD)PTR_SEG_TO_LIN(*(DWORD*)args);
            args += sizeof(DWORD);
            break;

        case 'c':
/* windows' wsprintf() %c ignores 0's, we replace 0 with SPACE to make sure
   that the remaining part of the string isn't ignored by the winapp */
            *stack_ptr++ = (DWORD)(*(WORD*)args ? *(WORD*)args : ' ');
            args += sizeof(WORD);
            break;

        case 'd':
        case 'i':
            if (!fLarge)
            {
                *stack_ptr++ = (DWORD)(INT32)(*(INT16 *)args);
                args += sizeof(INT16);
                break;
            }
            /* else fall through */
        case 'u':
        case 'x':
        case 'X':
            if (fLarge)
            {
                *stack_ptr++ = *(DWORD*)args;
                args += sizeof(DWORD);
            }
            else
            {
                *stack_ptr++ = *(WORD*)args;
                args += sizeof(WORD);
            }
            break;

        default:
            *stack_ptr++ = 0;
            args += sizeof(WORD);
            fprintf( stderr, "wsprintf: oops, unknown format %c!\n", *ptr );
            break;
        }
    }

    return vsprintf( buf, format, stack );
}
