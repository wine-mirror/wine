/*
 * String functions
 *
 * Copyright 1993 Yngvi Sigurjonsson (yngvi@hafro.is)
 * Copyright 1996 Marcus Meissner
 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"

#include "heap.h"
#include "ldt.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(resource);

/***********************************************************************
 *           FormatMessage16   (USER.606)
 */
DWORD WINAPI FormatMessage16(
    DWORD   dwFlags,
    SEGPTR lpSource, /*not always a valid pointer*/
    WORD   dwMessageId,
    WORD   dwLanguageId,
    LPSTR  lpBuffer, /* *((HLOCAL16*)) for FORMAT_MESSAGE_ALLOCATE_BUFFER*/
    WORD   nSize,
    LPDWORD args /* va_list *args */
) {
#ifdef __i386__
/* This implementation is completely dependant on the format of the va_list on x86 CPUs */
    LPSTR	target,t;
    DWORD	talloced;
    LPSTR	from,f;
    DWORD	width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
    BOOL	eos = FALSE;
    LPSTR       allocstring = NULL;

    TRACE("(0x%lx,%lx,%d,0x%x,%p,%d,%p)\n",
	  dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
	if ((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
		&& (dwFlags & FORMAT_MESSAGE_FROM_HMODULE)) return 0;
	if ((dwFlags & FORMAT_MESSAGE_FROM_STRING)
		&&((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
			|| (dwFlags & FORMAT_MESSAGE_FROM_HMODULE))) return 0;

    if (width && width != FORMAT_MESSAGE_MAX_WIDTH_MASK) 
        FIXME("line wrapping (%lu) not supported.\n", width);
    from = NULL;
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
        from = HEAP_strdupA( GetProcessHeap(), 0, PTR_SEG_TO_LIN(lpSource));
    if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM) {
        from = HeapAlloc( GetProcessHeap(),0,200 );
	sprintf(from,"Systemmessage, messageid = 0x%08x\n",dwMessageId);
    }
    if (dwFlags & FORMAT_MESSAGE_FROM_HMODULE) {
        INT16	bufsize;
	HINSTANCE16 hinst16 = ((HMODULE)lpSource & 0xffff);

	dwMessageId &= 0xFFFF;
	bufsize=LoadString16(hinst16,dwMessageId,NULL,0);
	if (bufsize) {
	    from = HeapAlloc( GetProcessHeap(), 0, bufsize +1);
	    LoadString16(hinst16,dwMessageId,from,bufsize+1);
	}
    }
    target	= HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 100);
    t	= target;
    talloced= 100;

#define ADD_TO_T(c) \
	*t++=c;\
	if (t-target == talloced) {\
		target	= (char*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,talloced*2);\
		t	= target+talloced;\
		talloced*=2;\
	}

    if (from) {
        f=from;
	while (*f && !eos) {
	    if (*f=='%') {
	        int	insertnr;
		char	*fmtstr,*x,*lastf;
		DWORD	*argliststart;

		fmtstr = NULL;
		lastf = f;
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
			    fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f)+2);
			    sprintf(fmtstr,"%%%s",f);
			    f=x+1;
			} else {
			    fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f)+2);
			    sprintf(fmtstr,"%%%s",f);
			    f+=strlen(f); /*at \0*/
			}
		    } else
		        if(!args) 
			    break;
			else
			  fmtstr=HEAP_strdupA(GetProcessHeap(),0,"%s");
		    if (args) {
		        int	sz;
			LPSTR	b = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz = 100);
			
			argliststart=args+insertnr-1;
		       
			/* CMF - This makes a BIG assumption about va_list */
			while (vsnprintf(b, sz, fmtstr, (va_list) argliststart) < 0) {
			    b = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, b, sz += 100);
			}
			for (x=b; *x; x++) ADD_TO_T(*x);
		    } else {
		        /* NULL args - copy formatstr 
			 * (probably wrong)
			 */
		        while ((lastf<f)&&(*lastf)) {
			    ADD_TO_T(*lastf++);
			}
		    }
		    HeapFree(GetProcessHeap(),0,fmtstr);
		    break;
	       	case '0': /* Just stop processing format string */
		    eos = TRUE;
		    f++;
		    break;
		case 'n': /* 16 bit version just outputs 'n' */
		default:
		    ADD_TO_T(*f++);
		    break;
		}
	    } else { /* '\n' or '\r' gets mapped to "\r\n" */
	        if(*f == '\n' || *f == '\r') {
		    if (width == 0) {
		        ADD_TO_T('\r');
		        ADD_TO_T('\n');
			if(*f++ == '\r' && *f == '\n')
			    f++;
		    }
		} else {
		    ADD_TO_T(*f++);
		}
	    }
	}
	*t='\0';
    }
    talloced = strlen(target)+1;
    if (nSize && talloced<nSize) {
        target = (char*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,nSize);
    }
    TRACE("-- %s\n",debugstr_a(target));
    if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
        /* nSize is the MINIMUM size */
        HLOCAL16 h = LocalAlloc16(LPTR,talloced);
        SEGPTR ptr = LocalLock16(h);
	allocstring = PTR_SEG_TO_LIN( ptr );
	memcpy( allocstring,target,talloced);
        LocalUnlock16( h );
        *((HLOCAL16*)lpBuffer) = h;
    } else
        lstrcpynA(lpBuffer,target,nSize);
    HeapFree(GetProcessHeap(),0,target);
    if (from) HeapFree(GetProcessHeap(),0,from);
    return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ? 
        strlen(allocstring):
	strlen(lpBuffer);
#else
	return 0;
#endif /* __i386__ */
}
#undef ADD_TO_T
