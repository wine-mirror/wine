/*
 * FormatMessage implementation
 *
 * Copyright 1996 Marcus Meissner
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"

#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(resource);


/* Messages...used by FormatMessage32* (KERNEL32.something)
 * 
 * They can be specified either directly or using a message ID and
 * loading them from the resource.
 * 
 * The resourcedata has following format:
 * start:
 * 0: DWORD nrofentries
 * nrofentries * subentry:
 *	0: DWORD firstentry
 *	4: DWORD lastentry
 *      8: DWORD offset from start to the stringentries
 *
 * (lastentry-firstentry) * stringentry:
 * 0: WORD len (0 marks end)
 * 2: WORD flags
 * 4: CHAR[len-4]
 * 	(stringentry i of a subentry refers to the ID 'firstentry+i')
 *
 * Yes, ANSI strings in win32 resources. Go figure.
 */

/**********************************************************************
 *	load_messageA		(internal)
 */
static INT load_messageA( HMODULE instance, UINT id, WORD lang,
                          LPSTR buffer, INT buflen )
{
    HGLOBAL	hmem;
    HRSRC	hrsrc;
    PMESSAGE_RESOURCE_DATA	mrd;
    PMESSAGE_RESOURCE_BLOCK	mrb;
    PMESSAGE_RESOURCE_ENTRY	mre;
    int		i,slen;

    TRACE("instance = %08lx, id = %08lx, buffer = %p, length = %ld\n", (DWORD)instance, (DWORD)id, buffer, (DWORD)buflen);

    /*FIXME: I am not sure about the '1' ... But I've only seen those entries*/
    hrsrc = FindResourceExW(instance,RT_MESSAGELISTW,(LPWSTR)1,lang);
    if (!hrsrc) return 0;
    hmem = LoadResource( instance, hrsrc );
    if (!hmem) return 0;
    
    mrd = (PMESSAGE_RESOURCE_DATA)LockResource(hmem);
    mre = NULL;
    mrb = &(mrd->Blocks[0]);
    for (i=mrd->NumberOfBlocks;i--;) {
    	if ((id>=mrb->LowId) && (id<=mrb->HighId)) {
	    mre = (PMESSAGE_RESOURCE_ENTRY)(((char*)mrd)+mrb->OffsetToEntries);
	    id	-= mrb->LowId;
	    break;
	}
	mrb++;
    }
    if (!mre)
    	return 0;
    for (i=id;i--;) {
    	if (!mre->Length)
		return 0;
    	mre = (PMESSAGE_RESOURCE_ENTRY)(((char*)mre)+(mre->Length)+2*sizeof(USHORT));
    }
    slen=mre->Length;
    TRACE("	- strlen=%d\n",slen);
    i = min(buflen - 1, slen);
    if (buffer == NULL)
	return slen;
    if (i>0) {
	lstrcpynA(buffer,(char*)mre->Text,i);
	buffer[i]=0;
    } else {
	if (buflen>1) {
	    buffer[0]=0;
	    return 0;
	}
    }
    if (buffer)
	    TRACE("'%s' copied !\n", buffer);
    return i;
}

#if 0  /* FIXME */
/**********************************************************************
 *	load_messageW   (internal)
 */
static INT load_messageW( HMODULE instance, UINT id, WORD lang,
                          LPWSTR buffer, INT buflen )
{
    INT retval;
    LPSTR buffer2 = NULL;
    if (buffer && buflen)
	buffer2 = HeapAlloc( GetProcessHeap(), 0, buflen );
    retval = load_messageA(instance,id,lang,buffer2,buflen);
    if (buffer)
    {
	if (retval) {
	    lstrcpynAtoW( buffer, buffer2, buflen );
	    retval = lstrlenW( buffer );
	}
	HeapFree( GetProcessHeap(), 0, buffer2 );
    }
    return retval;
}
#endif


/***********************************************************************
 *           FormatMessageA   (KERNEL32.138)
 * FIXME: missing wrap,
 */
DWORD WINAPI FormatMessageA(
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPSTR	lpBuffer,
	DWORD	nSize,
	LPDWORD	args /* va_list *args */
) {
#ifdef __i386__
/* This implementation is completely dependant on the format of the va_list on x86 CPUs */
	LPSTR	target,t;
	DWORD	talloced;
	LPSTR	from,f;
	DWORD	width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
	BOOL    eos = FALSE;
	INT	bufsize;
	HMODULE	hmodule = (HMODULE)lpSource;

	TRACE("(0x%lx,%p,%ld,0x%lx,%p,%ld,%p)\n",
		     dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
	if ((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
		&& (dwFlags & FORMAT_MESSAGE_FROM_HMODULE)) return 0;
	if ((dwFlags & FORMAT_MESSAGE_FROM_STRING)
		&&((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
			|| (dwFlags & FORMAT_MESSAGE_FROM_HMODULE))) return 0;

	if (width && width != FORMAT_MESSAGE_MAX_WIDTH_MASK) 
		FIXME("line wrapping (%lu) not supported.\n", width);
	from = NULL;
	if (dwFlags & FORMAT_MESSAGE_FROM_STRING) {
		from = HEAP_strdupA( GetProcessHeap(), 0, (LPSTR)lpSource);
	}
	else {
		dwMessageId &= 0xFFFF;
		if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
			hmodule = GetModuleHandleA("kernel32");
		bufsize=load_messageA(hmodule,dwMessageId,dwLanguageId,NULL,100);
		if (!bufsize) {
			if (dwLanguageId) {
				SetLastError (ERROR_RESOURCE_LANG_NOT_FOUND);
				return 0;
			}
			bufsize=load_messageA(hmodule,dwMessageId,
				MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),NULL,100);
			if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
				MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),NULL,100);
			if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
				MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT),NULL,100);
			if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
				MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT),NULL,100);
			if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
				MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),NULL,100);
			if (!bufsize) {
				SetLastError (ERROR_RESOURCE_LANG_NOT_FOUND);
				return 0;
			}
		}
		from = HeapAlloc( GetProcessHeap(), 0, bufsize + 1 );
		load_messageA(hmodule,dwMessageId,dwLanguageId,from,bufsize+1);
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

						if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
						        argliststart=args+insertnr-1;
						else
						        argliststart=(*(DWORD**)args)+insertnr-1;

						/* CMF - This makes a BIG assumption about va_list */
                                                /* FIXME: wvsnprintfA is a USER function */
						while (wvsnprintfA(b, sz, fmtstr, (va_list) argliststart) < 0) {
						        b = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, b, sz += 100);
						}
						for (x=b; *x; x++) ADD_TO_T(*x);

						HeapFree(GetProcessHeap(),0,b);
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
				case 'n':
					ADD_TO_T('\r');
					ADD_TO_T('\n');
					f++;
					break;
				case '0':
					eos = TRUE;
					f++;
					break;
				default:
				        ADD_TO_T(*f++)
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
		*((LPVOID*)lpBuffer) = (LPVOID)LocalAlloc(GMEM_ZEROINIT,max(nSize, talloced));
		memcpy(*(LPSTR*)lpBuffer,target,talloced);
	} else {
		lstrcpynA(lpBuffer,target,nSize);
	}
	HeapFree(GetProcessHeap(),0,target);
	if (from) HeapFree(GetProcessHeap(),0,from);
	return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ? 
			strlen(*(LPSTR*)lpBuffer):
			strlen(lpBuffer);
#else
	return 0;
#endif /* __i386__ */
}
#undef ADD_TO_T


/***********************************************************************
 *           FormatMessageW   (KERNEL32.138)
 */
DWORD WINAPI FormatMessageW(
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPWSTR	lpBuffer,
	DWORD	nSize,
	LPDWORD	args /* va_list *args */
) {
#ifdef __i386__
/* This implementation is completely dependant on the format of the va_list on x86 CPUs */
	LPSTR	target,t;
	DWORD	talloced;
	LPSTR	from,f;
	DWORD	width = dwFlags & FORMAT_MESSAGE_MAX_WIDTH_MASK;
	BOOL	eos = FALSE;
	INT	bufsize;
	HMODULE	hmodule = (HMODULE)lpSource;

	TRACE("(0x%lx,%p,%ld,0x%lx,%p,%ld,%p)\n",
		     dwFlags,lpSource,dwMessageId,dwLanguageId,lpBuffer,nSize,args);
	if ((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
		&& (dwFlags & FORMAT_MESSAGE_FROM_HMODULE)) return 0;
	if ((dwFlags & FORMAT_MESSAGE_FROM_STRING)
		&&((dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
			|| (dwFlags & FORMAT_MESSAGE_FROM_HMODULE))) return 0;

	if (width && width != FORMAT_MESSAGE_MAX_WIDTH_MASK) 
		FIXME("line wrapping not supported.\n");
	from = NULL;
	if (dwFlags & FORMAT_MESSAGE_FROM_STRING) {
		from = HEAP_strdupWtoA(GetProcessHeap(),0,(LPWSTR)lpSource);
	}
	else {
		dwMessageId &= 0xFFFF;
		if (dwFlags & FORMAT_MESSAGE_FROM_SYSTEM)
			hmodule = GetModuleHandleA("kernel32");
		bufsize=load_messageA(hmodule,dwMessageId,dwLanguageId,NULL,100);
		if (!bufsize) {
			if (dwLanguageId) {
				SetLastError (ERROR_RESOURCE_LANG_NOT_FOUND);
				return 0;
			}
			bufsize=load_messageA(hmodule,dwMessageId,
				MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),NULL,100);
			if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
				MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),NULL,100);
			if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
				MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT),NULL,100);
			if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
				MAKELANGID(LANG_NEUTRAL,SUBLANG_SYS_DEFAULT),NULL,100);
			if (!bufsize) bufsize=load_messageA(hmodule,dwMessageId,
				MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),NULL,100);
			if (!bufsize) {
				SetLastError (ERROR_RESOURCE_LANG_NOT_FOUND);
				return 0;
			}
		}
		from = HeapAlloc( GetProcessHeap(), 0, bufsize + 1 );
		load_messageA(hmodule,dwMessageId,dwLanguageId,from,bufsize+1);
	}
	target	= HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 100 );
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
				char	*fmtstr,*sprintfbuf,*x;
				DWORD	*argliststart;

				fmtstr = NULL;
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
						if (NULL!=(x=strchr(f,'!')))
                                                {
                                                    *x='\0';
                                                    fmtstr=HeapAlloc( GetProcessHeap(), 0, strlen(f)+2);
							sprintf(fmtstr,"%%%s",f);
							f=x+1;
						} else {
							fmtstr=HeapAlloc(GetProcessHeap(),0,strlen(f));
							sprintf(fmtstr,"%%%s",f);
							f+=strlen(f); /*at \0*/
						}
					} else
					        if(!args)
						  break;
					else
						fmtstr=HEAP_strdupA( GetProcessHeap(),0,"%s");
					if (dwFlags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
						argliststart=args+insertnr-1;
					else
						argliststart=(*(DWORD**)args)+insertnr-1;

					if (fmtstr[strlen(fmtstr)-1]=='s' && argliststart[0]) {
						DWORD	xarr[3];

						xarr[0]=(DWORD)HEAP_strdupWtoA(GetProcessHeap(),0,(LPWSTR)(*(argliststart+0)));
						/* possible invalid pointers */
						xarr[1]=*(argliststart+1);
						xarr[2]=*(argliststart+2);
						sprintfbuf=HeapAlloc(GetProcessHeap(),0,lstrlenW((LPWSTR)argliststart[0])*2+1);

						/* CMF - This makes a BIG assumption about va_list */
						vsprintf(sprintfbuf, fmtstr, (va_list) xarr);
					} else {
						sprintfbuf=HeapAlloc(GetProcessHeap(),0,100);

						/* CMF - This makes a BIG assumption about va_list */
                                                /* FIXME: wvsprintfA is a USER function */
						wvsprintfA(sprintfbuf, fmtstr, (va_list) argliststart);
					}
					x=sprintfbuf;
					while (*x) {
						ADD_TO_T(*x++);
					}
					HeapFree(GetProcessHeap(),0,sprintfbuf);
					HeapFree(GetProcessHeap(),0,fmtstr);
					break;
				case 'n':
				        ADD_TO_T('\r');
					ADD_TO_T('\n');
					f++;
					break;
				case '0':
					eos = TRUE;
					f++;
					break;
				default:
				        ADD_TO_T(*f++)
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
	if (nSize && talloced<nSize)
		target = (char*)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,target,nSize);
	if (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) {
		/* nSize is the MINIMUM size */
		*((LPVOID*)lpBuffer) = (LPVOID)LocalAlloc(GMEM_ZEROINIT,talloced*2+2);
		lstrcpynAtoW(*(LPWSTR*)lpBuffer,target,talloced);
	} else
		lstrcpynAtoW(lpBuffer,target,nSize);
	HeapFree(GetProcessHeap(),0,target);
	if (from) HeapFree(GetProcessHeap(),0,from);
	return (dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) ? 
			lstrlenW(*(LPWSTR*)lpBuffer):
			lstrlenW(lpBuffer);
#else
	return 0;
#endif /* __i386__ */
}
#undef ADD_TO_T
