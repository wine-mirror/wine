/*
 * String functions
 *
 * Copyright 1993 Yngvi Sigurjonsson (yngvi@hafro.is)
 * Copyright 1996 Marcus Meissner
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "windows.h"
#include "ldt.h"
#include "module.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "string32.h"

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

/* IsCharAlpha USER 433 */
BOOL16 IsCharAlpha16(CHAR ch)
{
  return isalpha(ch);   /* This is probably not right for NLS */
}

/* IsCharAlphanumeric USER 434 */
BOOL16 IsCharAlphanumeric16(CHAR ch)
{
    return isalnum(ch);
}

/* IsCharUpper USER 435 */
BOOL16 IsCharUpper16(CHAR ch)
{
  return isupper(ch);
}

/* IsCharLower USER 436 */
BOOL16 IsCharLower16(CHAR ch)
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
INT AnsiToOem(LPCSTR lpAnsiStr, LPSTR lpOemStr)
{
    dprintf_keyboard(stddeb, "AnsiToOem: %s\n", lpAnsiStr);
    while(*lpAnsiStr){
	*lpOemStr++=Ansi2Oem[(unsigned char)(*lpAnsiStr++)];
    }
    *lpOemStr = 0;
    return -1;
}

/* OemToAnsi Keyboard.6 */
BOOL OemToAnsi(LPCSTR lpOemStr, LPSTR lpAnsiStr)
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
void OemToAnsiBuff(LPCSTR lpOemStr, LPSTR lpAnsiStr, INT nLength)
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
    free( buffer );
}

/***********************************************************************
 *           CharNextA   (USER32.28)
 */
LPSTR CharNext32A(LPCSTR x)
{
    if (*x) return (LPSTR)(x+1);
    else return (LPSTR)x;
}

/***********************************************************************
 *           CharNextExA   (USER32.29)
 */
LPSTR CharNextEx32A(WORD codepage,LPCSTR x,DWORD flags)
{
    /* FIXME: add DBCS / codepage stuff */
    if (*x) return (LPSTR)(x+1);
    else return (LPSTR)x;
}

/***********************************************************************
 *           CharNextExW   (USER32.30)
 */
LPWSTR CharNextEx32W(WORD codepage,LPCWSTR x,DWORD flags)
{
    /* FIXME: add DBCS / codepage stuff */
    if (*x) return (LPWSTR)(x+1);
    else return (LPWSTR)x;
}

/***********************************************************************
 *           CharNextW   (USER32.31)
 */
LPWSTR CharNext32W(LPCWSTR x)
{
    if (*x) return (LPWSTR)(x+1);
    else return (LPWSTR)x;
}

/***********************************************************************
 *           CharPrevA   (USER32.32)
 */
LPSTR CharPrev32A(LPCSTR start,LPCSTR x)
{
    if (x>start) return (LPSTR)(x-1);
    else return (LPSTR)x;
}

/***********************************************************************
 *           CharPrevExA   (USER32.33)
 */
LPSTR CharPrevEx32A(WORD codepage,LPCSTR start,LPCSTR x,DWORD flags)
{
    /* FIXME: add DBCS / codepage stuff */
    if (x>start) return (LPSTR)(x-1);
    else return (LPSTR)x;
}

/***********************************************************************
 *           CharPrevExW   (USER32.34)
 */
LPWSTR CharPrevEx32W(WORD codepage,LPCWSTR start,LPCWSTR x,DWORD flags)
{
    /* FIXME: add DBCS / codepage stuff */
    if (x>start) return (LPWSTR)(x-1);
    else return (LPWSTR)x;
}

/***********************************************************************
 *           CharPrevW   (USER32.35)
 */
LPWSTR CharPrev32W(LPCWSTR start,LPCWSTR x)
{
    if (x>start) return (LPWSTR)(x-1);
    else return (LPWSTR)x;
}

/***********************************************************************
 *           CharLowerA   (USER32.24)
 * FIXME: handle current locale
 */
LPSTR CharLower32A(LPSTR x)
{
    LPSTR	s;

    if (HIWORD(x))
    {
        s=x;
        while (*s)
        {
            *s=tolower(*s);
            s++;
        }
        return x;
    }
    else return (LPSTR)tolower(LOWORD(x));
}

/***********************************************************************
 *           CharLowerBuffA   (USER32.25)
 * FIXME: handle current locale
 */
DWORD CharLowerBuff32A(LPSTR x,DWORD buflen)
{
    DWORD done=0;

    while (*x && (buflen--))
    {
        *x=tolower(*x);
        x++;
        done++;
    }
    return done;
}

/***********************************************************************
 *           CharLowerBuffW   (USER32.26)
 * FIXME: handle current locale
 */
DWORD CharLowerBuff32W(LPWSTR x,DWORD buflen)
{
    DWORD done=0;

    while (*x && (buflen--))
    {
        *x=tolower(*x);
        x++;
        done++;
    }
    return done;
}

/***********************************************************************
 *           CharLowerW   (USER32.27)
 * FIXME: handle current locale
 */
LPWSTR CharLower32W(LPWSTR x)
{
    if (HIWORD(x))
    {
        LPWSTR s = x;
        while (*s)
        {
            *s=tolower(*s);
            s++;
        }
        return x;
    }
    else return (LPWSTR)tolower(LOWORD(x));
}

/***********************************************************************
 *           CharUpperA   (USER32.40)
 * FIXME: handle current locale
 */
LPSTR CharUpper32A(LPSTR x)
{
    if (HIWORD(x))
    {
        LPSTR s = x;
        while (*s)
        {
            *s=toupper(*s);
            s++;
        }
        return x;
    }
    else return (LPSTR)toupper(LOWORD(x));
}

/***********************************************************************
 *           CharUpperBuffA   (USER32.41)
 * FIXME: handle current locale
 */
DWORD CharUpperBuff32A(LPSTR x,DWORD buflen)
{
    DWORD done=0;

    while (*x && (buflen--))
    {
        *x=toupper(*x);
        x++;
        done++;
    }
    return done;
}

/***********************************************************************
 *           CharUpperBuffW   (USER32.42)
 * FIXME: handle current locale
 */
DWORD CharUpperBuff32W(LPWSTR x,DWORD buflen)
{
    DWORD done=0;

    while (*x && (buflen--))
    {
        *x=toupper(*x);
        x++;
        done++;
    }
    return done;
}

/***********************************************************************
 *           CharUpperW   (USER32.43)
 * FIXME: handle current locale
 */
LPWSTR CharUpper32W(LPWSTR x)
{
    if (HIWORD(x))
    {
        LPWSTR s = x;
        while (*s)
        {
            *s=toupper(*s);
            s++;
        }
        return x;
    }
    else return (LPWSTR)toupper(LOWORD(x));
}

/***********************************************************************
 *           IsCharAlphaA   (USER32.330)
 * FIXME: handle current locale
 */
BOOL32 IsCharAlpha32A(CHAR x)
{
    return isalpha(x);
}

/***********************************************************************
 *           IsCharAlphaNumericA   (USER32.331)
 * FIXME: handle current locale
 */
BOOL32 IsCharAlphaNumeric32A(CHAR x)
{
    return isalnum(x);
}

/***********************************************************************
 *           IsCharAlphaNumericW   (USER32.332)
 * FIXME: handle current locale
 */
BOOL32 IsCharAlphaNumeric32W(WCHAR x)
{
    return isalnum(x);
}

/***********************************************************************
 *           IsCharAlphaW   (USER32.333)
 * FIXME: handle current locale
 */
BOOL32 IsCharAlpha32W(WCHAR x)
{
    return isalpha(x);
}

/***********************************************************************
 *           IsCharAlphaW   (USER32.334)
 * FIXME: handle current locale
 */
BOOL32 IsCharLower32A(CHAR x)
{
    return islower(x);
}

/***********************************************************************
 *           IsCharAlphaW   (USER32.335)
 * FIXME: handle current locale
 */
BOOL32 IsCharLower32W(WCHAR x)
{
    return islower(x);
}

/***********************************************************************
 *           IsCharAlphaW   (USER32.336)
 * FIXME: handle current locale
 */
BOOL32 IsCharUpper32A(CHAR x)
{
    return isupper(x);
}

/***********************************************************************
 *           IsCharAlphaW   (USER32.337)
 * FIXME: handle current locale
 */
BOOL32 IsCharUpper32W(WCHAR x)
{
    return isupper(x);
}

/***********************************************************************
 *           CharToOemA   (USER32.36)
 */
BOOL32 CharToOem32A(LPSTR s,LPSTR d)
{
    if (!s || !d)
    	return TRUE;
    AnsiToOem(s,d);
    return TRUE;
}

/***********************************************************************
 *           CharToOemBuffA   (USER32.37)
 */
BOOL32 CharToOemBuff32A(LPSTR s,LPSTR d,DWORD len)
{
    AnsiToOemBuff(s,d,len);
    return TRUE;
}

/***********************************************************************
 *           CharToOemBuffW   (USER32.38)
 */
BOOL32 CharToOemBuff32W(LPCWSTR s,LPSTR d,DWORD len)
{
    LPSTR	x=STRING32_DupUniToAnsi(s);
    AnsiToOemBuff(x,d,len);
    return TRUE;
}

/***********************************************************************
 *           CharToOemW   (USER32.39)
 */
BOOL32 CharToOem32W(LPCWSTR s,LPSTR d)
{
    LPSTR	x=STRING32_DupUniToAnsi(s);
    AnsiToOem(x,d);
    return TRUE;
}

/***********************************************************************
 *           OemToCharA   (USER32.401)
 */
BOOL32 OemToChar32A(LPSTR s,LPSTR d)
{
    OemToAnsi(s,d);
    return TRUE;
}

/***********************************************************************
 *           OemToCharBuffA   (USER32.402)
 */
BOOL32 OemToCharBuff32A(LPSTR s,LPSTR d,DWORD len)
{
    OemToAnsiBuff(s,d,len);
    return TRUE;
}

/***********************************************************************
 *           OemToCharBuffW   (USER32.403)
 */
BOOL32 OemToCharBuff32W(LPCSTR s,LPWSTR d,DWORD len)
{
    LPSTR x=(char*)xmalloc(strlen(s));
    OemToAnsiBuff((LPSTR)s,x,len);
    STRING32_AnsiToUni(d,x);
    return TRUE;
}

/***********************************************************************
 *           OemToCharW   (USER32.404)
 */
BOOL32 OemToChar32W(LPCSTR s,LPWSTR d)
{
    LPSTR x=(char*)xmalloc(strlen(s));
    OemToAnsi((LPSTR)s,x);
    STRING32_AnsiToUni(d,x);
    return TRUE;
}

/***********************************************************************
 *           FormatMessageA   (KERNEL32.138) Library Version
 * FIXME: missing wrap,FROM_SYSTEM message-loading,
 */
DWORD
FormatMessage32A(
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPSTR	lpBuffer,
	DWORD	nSize,
	LPDWORD	args /* va_list *args */
) {
	LPSTR	target,t;
	DWORD	talloced;
	LPSTR	from,f;
	DWORD	width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
	DWORD	nolinefeed = 0;

	dprintf_resource(stddeb,
		"FormatMessage32A(0x%lx,%p,%ld,0x%lx,%p,%ld,%p)\n",
		dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args
	);
	if (width) 
		fprintf(stdnimp,"	- line wrapping not supported.\n");
	from = NULL;
	if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
		from = xstrdup((LPSTR)lpSource);
	if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM) {
		/* gather information from system message tables ... */
		fprintf(stdnimp,"	- FORMAT_MESSAGE_FROM_SYSTEM not implemented.\n");
	}
	if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE) {
		INT32	bufsize;

		dwMessageId &= 0xFFFF;
		bufsize=LoadMessage32A(0,dwMessageId,dwLanguageId,NULL,100);
		if (bufsize) {
			from = (char*)xmalloc(bufsize+1);
			LoadMessage32A(0,dwMessageId,dwLanguageId,from,bufsize+1);
		}
	}
	target	= (char*)xmalloc(100);
	t	= target;
	talloced= 100;
	*t	= 0;

#define ADD_TO_T(c) \
	*t++=c;\
	if (t-target == talloced) {\
		target	= (char*)xrealloc(target,talloced*2);\
		t	= target+talloced;\
		talloced*=2;\
	}

	if (from) {
		f=from;
		while (*f) {
			if (*f=='%') {
				int	insertnr;
				char	*fmtstr,*sprintfbuf,*x;
				DWORD	*argliststart;

				f++;
				if (!*f) {
					ADD_TO_T('%');
					continue;
				}
				switch (*f) {
				case '1':case '2':case '3':case '4':case '5':
				case '6':case '7':case '8':case '9':
					insertnr=*f-'0';
					switch (f[1]) {
					case '0':case '1':case '2':case '3':
					case '4':case '5':case '6':case '7':
					case '8':case '9':
						f++;
						insertnr=insertnr*10+*f-'0';
						f++;
						break;
					default:
						f++;
						break;
					}
					if (*f=='!') {
						f++;
						if (NULL!=(x=strchr(f,'!'))) {
							*x='\0';
							fmtstr=xmalloc(strlen(f)+2);
							sprintf(fmtstr,"%%%s",f);
							f=x+1;
						}
					} else
						fmtstr=strdup("%s");
					if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
						argliststart=args+insertnr-1;
					else
						/* FIXME: not sure that this is
						 * correct for unix-c-varargs.
						 */
						argliststart=((DWORD*)&args)+insertnr-1;

					if (fmtstr[strlen(fmtstr)]=='s')
						sprintfbuf=(char*)xmalloc(strlen((LPSTR)argliststart[0])+1);
					else
						sprintfbuf=(char*)xmalloc(100);
					vsprintf(sprintfbuf,fmtstr,argliststart);
					x=sprintfbuf;
					while (*x) {
						ADD_TO_T(*x++);
					}
					free(sprintfbuf);
					free(fmtstr);
					break;
				case '0':
					nolinefeed=1;
					f++;
					break;
				default:ADD_TO_T(*f++)
					break;

				}
			} else {
				ADD_TO_T(*f++)
			}
		}
		*t='\0';
	}
	if (!nolinefeed && t[-1]!='\n')
		ADD_TO_T('\n');
	talloced = strlen(target)+1;
	if (nSize && talloced<nSize) {
		target = (char*)xrealloc(target,nSize);
	}
	if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
		/* nSize is the MINIMUM size */
		*((LPVOID*)lpBuffer) = (LPVOID)LocalAlloc32(GMEM_ZEROINIT,talloced);
		memcpy(*(LPSTR*)lpBuffer,target,talloced);
	} else
		strncpy(lpBuffer,target,nSize);
	free(target);
	if (from) free(from);
	return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ? 
			strlen(*(LPSTR*)lpBuffer):
			strlen(lpBuffer);
}

/***********************************************************************
 *           FormatMessageA   (KERNEL32.138) Emulator Version
 */
DWORD
WIN32_FormatMessage32A(DWORD *args) {
	DWORD	dwFlags		= args[0];
	LPCVOID	lpSource	= (LPCVOID)args[1];
	DWORD	dwMessageId	= args[2];
	DWORD	dwLanguageId	= args[3];
	LPSTR	lpBuffer	= (LPSTR)args[4];
	DWORD	nSize		= args[5];
	DWORD	*xargs;

	/* convert possible varargs to an argument array look-a-like */

	if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY) {
		xargs=(DWORD*)args[6];
	} else {
		/* args[6] is a pointer to a pointer to the start of 
		 * a list of arguments.
		 */
		if (args[6])
			xargs=(DWORD*)(((DWORD*)args[6])[0]);
		else
			xargs=NULL;
		dwFlags|=FORMAT_MESSAGE_ARGUMENT_ARRAY;
	}
	return FormatMessage32A(
		dwFlags,
		lpSource,
		dwMessageId,
		dwLanguageId,
		lpBuffer,
		nSize,
		xargs
	);
}
