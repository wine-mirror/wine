/*
 * 	Registry Functions
 *
 * Copyright 1996 Marcus Meissner
 *
 * December 21, 1997 - Kevin Cozens
 * Fixed bugs in the _w95_loadreg() function. Added extra information
 * regarding the format of the Windows '95 registry files.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <assert.h>
#include <time.h>
#include "windows.h"
#include "win.h"
#include "winerror.h"
#include "file.h"
#include "heap.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "winreg.h"

#define	DEBUG_W95_LOADREG	0

/* FIXME: following defines should be configured global ... */

/* NOTE: do not append a /. linux' mkdir() WILL FAIL if you do that */
#define WINE_PREFIX			"/.wine"
#define SAVE_USERS_DEFAULT		"/usr/local/etc/wine.userreg"
#define SAVE_LOCAL_MACHINE_DEFAULT	"/usr/local/etc/wine.systemreg"

/* relative in ~user/.wine/ : */
#define SAVE_CURRENT_USER		"user.reg"
#define SAVE_LOCAL_MACHINE		"system.reg"

#define KEY_REGISTRY	"Software\\The WINE team\\WINE\\Registry"
#define VAL_SAVEUPDATED	"SaveOnlyUpdatedKeys"

/* one value of a key */
typedef struct tagKEYVALUE
{
    LPWSTR   name;          /* name of value (UNICODE) or NULL for win31 */
    DWORD    type;          /* type of value */
    DWORD    len;           /* length of data */
    DWORD    lastmodified;  /* time of seconds since 1.1.1970 */
    LPBYTE   data;          /* content, may be strings, binaries, etc. */
} KEYVALUE,*LPKEYVALUE;

/* a registry key */
typedef struct tagKEYSTRUCT
{
    LPWSTR               keyname;       /* name of THIS key (UNICODE) */
    DWORD                flags;         /* flags. */
    LPWSTR               class;
    /* values */
    DWORD                nrofvalues;    /* nr of values in THIS key */
    LPKEYVALUE           values;        /* values in THIS key */
    /* key management pointers */
    struct tagKEYSTRUCT	*next;          /* next key on same hierarchy */
    struct tagKEYSTRUCT	*nextsub;       /* keys that hang below THIS key */
} KEYSTRUCT, *LPKEYSTRUCT;


static KEYSTRUCT	*key_classes_root=NULL;	/* windows 3.1 global values */
static KEYSTRUCT	*key_current_user=NULL;	/* user specific values */
static KEYSTRUCT	*key_local_machine=NULL;/* machine specific values */
static KEYSTRUCT	*key_users=NULL;	/* all users? */

/* dynamic, not saved */
static KEYSTRUCT	*key_performance_data=NULL;
static KEYSTRUCT	*key_current_config=NULL;
static KEYSTRUCT	*key_dyn_data=NULL;

/* what valuetypes do we need to convert? */
#define UNICONVMASK	((1<<REG_SZ)|(1<<REG_MULTI_SZ)|(1<<REG_EXPAND_SZ))

extern LPWSTR __cdecl CRTDLL_wcschr(LPWSTR a,WCHAR c);

static LPWSTR strdupA2W(LPCSTR src)
{
	LPWSTR dest=xmalloc(2*strlen(src)+2);
	lstrcpyAtoW(dest,src);
	return dest;
}

static LPWSTR strdupW(LPCWSTR a) {
	LPWSTR	b;
	int	len;

	len=sizeof(WCHAR)*(lstrlen32W(a)+1);
	b=(LPWSTR)xmalloc(len);
	memcpy(b,a,len);
	return b;
}


static struct openhandle {
	LPKEYSTRUCT	lpkey;
	HKEY		hkey;
	REGSAM		accessmask;
}  *openhandles=NULL;
static int	nrofopenhandles=0;
static int	currenthandle=1;

static void
add_handle(HKEY hkey,LPKEYSTRUCT lpkey,REGSAM accessmask) {
	int	i;

	for (i=0;i<nrofopenhandles;i++) {
		if (openhandles[i].lpkey==lpkey) {
			dprintf_reg(stddeb,"add_handle:Tried to add %p twice!\n",lpkey);
		}
		if (openhandles[i].hkey==hkey) {
			dprintf_reg(stddeb,"add_handle:Tried to add %lx twice!\n",(LONG)hkey);
		}
	}
	openhandles=xrealloc(	openhandles,
				sizeof(struct openhandle)*(nrofopenhandles+1)
		);
	openhandles[i].lpkey	= lpkey;
	openhandles[i].hkey	= hkey;
	openhandles[i].accessmask= accessmask;
	nrofopenhandles++;
}

static LPKEYSTRUCT
get_handle(HKEY hkey) {
	int	i;

	for (i=0;i<nrofopenhandles;i++)
		if (openhandles[i].hkey==hkey)
			return openhandles[i].lpkey;
	dprintf_reg(stddeb,"get_handle:Didn't find handle %lx?\n",(LONG)hkey);
	return NULL;
}

static void
remove_handle(HKEY hkey) {
	int	i;

	for (i=0;i<nrofopenhandles;i++)
		if (openhandles[i].hkey==hkey)
			break;
	if (i==nrofopenhandles) {
		dprintf_reg(stddeb,"remove_handle:Didn't find handle %08x?\n",hkey);
		return;
	}
	memcpy(	openhandles+i,
		openhandles+i+1,
		sizeof(struct openhandle)*(nrofopenhandles-i-1)
	);
	openhandles=xrealloc(openhandles,sizeof(struct openhandle)*(nrofopenhandles-1));
	nrofopenhandles--;
	return;
}


/* debug function, converts a unicode into a static memory area 
 * (sub for using two static strings, in case we need them in a single call)
 */
LPSTR
W2C(LPCWSTR x,int sub) {
	static	LPSTR	unicodedebug[2]={NULL,NULL};
	if (x==NULL)
		return "<NULL>";
	if (sub!=0 && sub!=1)
		return "<W2C:bad sub>";
	if (unicodedebug[sub]) HeapFree( SystemHeap, 0, unicodedebug[sub] );
	unicodedebug[sub] = HEAP_strdupWtoA( SystemHeap, 0, x );
	return unicodedebug[sub];
}

static LPKEYSTRUCT
lookup_hkey(HKEY hkey) {
	switch (hkey) {
	case 0x00000000:
	case 0x00000001:
	case HKEY_CLASSES_ROOT:
		return key_classes_root;
	case HKEY_CURRENT_USER:
		return key_current_user;
	case HKEY_LOCAL_MACHINE:
		return key_local_machine;
	case HKEY_USERS:
		return key_users;
	case HKEY_PERFORMANCE_DATA:
		return key_performance_data;
	case HKEY_DYN_DATA:
		return key_dyn_data;
	case HKEY_CURRENT_CONFIG:
		return key_current_config;
	default:
		dprintf_reg(stddeb,"lookup_hkey(%lx), special key!\n",
			(LONG)hkey
		);
		return get_handle(hkey);
	}
	/*NOTREACHED*/
}

/* 
 * splits the unicode string 'wp' into an array of strings.
 * the array is allocated by this function. 
 * the number of components will be stored in 'wpc'
 * Free the array using FREE_KEY_PATH
 */
static void
split_keypath(LPCWSTR wp,LPWSTR **wpv,int *wpc) {
	int	i,j,len;
	LPWSTR	ws;

	ws	= HEAP_strdupW( SystemHeap, 0, wp );
	*wpc	= 1;
	for (i=0;ws[i];i++) {
		if (ws[i]=='\\') {
			ws[i]=0;
			(*wpc)++;
		}
	}
	len	= i;
	*wpv	= (LPWSTR*)HeapAlloc( SystemHeap, 0, sizeof(LPWSTR)*(*wpc+2));
	(*wpv)[0]= ws;
	j	= 1;
	for (i=1;i<len;i++)
		if (ws[i-1]==0)
			(*wpv)[j++]=ws+i;
	(*wpv)[j]=NULL;
}
#define FREE_KEY_PATH HeapFree(SystemHeap,0,wps[0]);HeapFree(SystemHeap,0,wps);

/*
 * Shell initialisation, allocates keys. 
 */
void SHELL_StartupRegistry();
void
SHELL_Init() {
	struct	passwd	*pwd;

	HKEY	cl_r_hkey,c_u_hkey;
#define ADD_ROOT_KEY(xx) \
	xx = (LPKEYSTRUCT)xmalloc(sizeof(KEYSTRUCT));\
	memset(xx,'\0',sizeof(KEYSTRUCT));\
	xx->keyname= strdupA2W("<should_not_appear_anywhere>");

	ADD_ROOT_KEY(key_local_machine);
	if (RegCreateKey16(HKEY_LOCAL_MACHINE,"\\SOFTWARE\\Classes",&cl_r_hkey)!=ERROR_SUCCESS) {
		fprintf(stderr,"couldn't create HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes. This is impossible.\n");
		exit(1);
	}
	key_classes_root = lookup_hkey(cl_r_hkey);

	ADD_ROOT_KEY(key_users);

#if 0
	/* FIXME: load all users and their resp. pwd->pw_dir/.wine/user.reg 
	 *	  (later, when a win32 registry editing tool becomes avail.)
	 */
	while (pwd=getpwent()) {
		if (pwd->pw_name == NULL)
			continue;
		RegCreateKey16(HKEY_USERS,pwd->pw_name,&c_u_hkey);
		RegCloseKey(c_u_hkey);
	}
#endif
	pwd=getpwuid(getuid());
	if (pwd && pwd->pw_name) {
		RegCreateKey16(HKEY_USERS,pwd->pw_name,&c_u_hkey);
		key_current_user = lookup_hkey(c_u_hkey);
	} else {
		ADD_ROOT_KEY(key_current_user);
	}
	ADD_ROOT_KEY(key_performance_data);
	ADD_ROOT_KEY(key_current_config);
	ADD_ROOT_KEY(key_dyn_data);
#undef ADD_ROOT_KEY
	SHELL_StartupRegistry();
}


void
SHELL_StartupRegistry() {
	HKEY	hkey,xhkey=0;
	FILE	*F;
	char	buf[200],cpubuf[200];

	RegCreateKey16(HKEY_DYN_DATA,"\\PerfStats\\StatData",&xhkey);
	RegCloseKey(xhkey);
        xhkey = 0;
	RegCreateKey16(HKEY_LOCAL_MACHINE,"\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor",&hkey);
#ifdef linux
	F=fopen("/proc/cpuinfo","r");
	if (F) {
		int	procnr=-1,x;
		while (NULL!=fgets(buf,200,F)) {
			if (sscanf(buf,"processor\t: %d",&x)) {
				sprintf(buf,"%d",x);
				if (xhkey)
					RegCloseKey(xhkey);
				procnr=x;
				RegCreateKey16(hkey,buf,&xhkey);
			}
			if (sscanf(buf,"cpu\t\t: %s",cpubuf)) {
				sprintf(buf,"CPU %s",cpubuf);
				if (xhkey)
					RegSetValueEx32A(xhkey,"Identifier",0,REG_SZ,buf,strlen(buf));
			}
		}
		fclose(F);
	}
	if (xhkey)
		RegCloseKey(xhkey);
	RegCloseKey(hkey);
#else
	/* FIXME */
	RegCreateKey16(hkey,"0",&xhkey);
	RegSetValueEx32A(xhkey,"Identifier",0,REG_SZ,"CPU 386",strlen("CPU 386"));
#endif
	RegOpenKey16(HKEY_LOCAL_MACHINE,"\\HARDWARE\\DESCRIPTION\\System",&hkey);
	RegSetValueEx32A(hkey,"Identifier",0,REG_SZ,"SystemType WINE",strlen("SystemType WINE"));
	RegCloseKey(hkey);
	/* \\SOFTWARE\\Microsoft\\Window NT\\CurrentVersion
	 *						CurrentVersion
	 *						CurrentBuildNumber
	 *						CurrentType
	 *					string	RegisteredOwner
	 *					string	RegisteredOrganization
	 *
	 */
	/* System\\CurrentControlSet\\Services\\SNMP\\Parameters\\RFC1156Agent
	 * 					string	SysContact
	 * 					string	SysLocation
	 * 						SysServices
	 */						
	if (-1!=gethostname(buf,200)) {
		RegCreateKey16(HKEY_LOCAL_MACHINE,"System\\CurrentControlSet\\Control\\ComputerName\\ComputerName",&xhkey);
		RegSetValueEx16(xhkey,"ComputerName",0,REG_SZ,buf,strlen(buf)+1);
		RegCloseKey(xhkey);
	}
}
/************************ SAVE Registry Function ****************************/

#define REGISTRY_SAVE_VERSION	0x00000001

/* Registry saveformat:
 * If you change it, increase above number by 1, which will flush
 * old registry database files.
 * 
 * Global:
 * 	"WINE REGISTRY Version %d"
 * 	subkeys....
 * Subkeys:
 * 	keyname
 *		valuename=lastmodified,type,data
 *		...
 *		subkeys
 *	...
 * keyname,valuename,stringdata:
 *	the usual ascii characters from 0x00-0xff (well, not 0x00)
 *	and \uXXXX as UNICODE value XXXX with XXXX>0xff
 *	( "=\\\t" escaped in \uXXXX form.)
 * type,lastmodified: 
 *	int
 * 
 * FIXME: doesn't save 'class' (what does it mean anyway?), nor flags.
 *
 * [HKEY_CURRENT_USER\\Software\\The WINE team\\WINE\\Registry]
 * SaveOnlyUpdatedKeys=yes
 */
static int
_save_check_tainted(LPKEYSTRUCT lpkey) {
	int		tainted;

	if (!lpkey)
		return 0;
	if (lpkey->flags & REG_OPTION_TAINTED)
		tainted = 1;
	else
		tainted = 0;
	while (lpkey) {
		if (_save_check_tainted(lpkey->nextsub)) {
			lpkey->flags |= REG_OPTION_TAINTED;
			tainted = 1;
		}
		lpkey	= lpkey->next;
	}
	return tainted;
}

static void
_save_USTRING(FILE *F,LPWSTR wstr,int escapeeq) {
	LPWSTR	s;
	int	doescape;

	if (wstr==NULL)
		return;
	s=wstr;
	while (*s) {
		doescape=0;
		if (*s>0xff)
			doescape = 1;
		if (*s=='\n')
			doescape = 1;
		if (escapeeq && *s=='=')
			doescape = 1;
		if (*s=='\\')
                        fputc(*s,F); /* if \\ then put it twice. */
		if (doescape)
			fprintf(F,"\\u%04x",*((unsigned short*)s));
		else
			fputc(*s,F);
		s++;
	}
}

static int
_savesubkey(FILE *F,LPKEYSTRUCT lpkey,int level,int all) {
	LPKEYSTRUCT	lpxkey;
	int		i,tabs,j;

	lpxkey	= lpkey;
	while (lpxkey) {
		if (	!(lpxkey->flags & REG_OPTION_VOLATILE) &&
			(all || (lpxkey->flags & REG_OPTION_TAINTED))
		) {
			for (tabs=level;tabs--;)
				fputc('\t',F);
			_save_USTRING(F,lpxkey->keyname,1);
			fputs("\n",F);
			for (i=0;i<lpxkey->nrofvalues;i++) {
				LPKEYVALUE	val=lpxkey->values+i;

				for (tabs=level+1;tabs--;)
					fputc('\t',F);
				_save_USTRING(F,val->name,0);
				fputc('=',F);
				fprintf(F,"%ld,%ld,",val->type,val->lastmodified);
				if ((1<<val->type) & UNICONVMASK)
					_save_USTRING(F,(LPWSTR)val->data,0);
				else
					for (j=0;j<val->len;j++)
						fprintf(F,"%02x",*((unsigned char*)val->data+j));
				fputs("\n",F);
			}
			/* descend recursively */
			if (!_savesubkey(F,lpxkey->nextsub,level+1,all))
				return 0;
		}
		lpxkey=lpxkey->next;
	}
	return 1;
}

static int
_savesubreg(FILE *F,LPKEYSTRUCT lpkey,int all) {
	fprintf(F,"WINE REGISTRY Version %d\n",REGISTRY_SAVE_VERSION);
	_save_check_tainted(lpkey->nextsub);
	return _savesubkey(F,lpkey->nextsub,0,all);
}

static BOOL32
_savereg(LPKEYSTRUCT lpkey,char *fn,int all) {
	FILE	*F;

	F=fopen(fn,"w");
	if (F==NULL) {
		fprintf(stddeb,__FILE__":_savereg:Couldn't open %s for writing: %s\n",
			fn,strerror(errno)
		);
		return FALSE;
	}
	if (!_savesubreg(F,lpkey,all)) {
		fclose(F);
		unlink(fn);
		fprintf(stddeb,__FILE__":_savereg:Failed to save keys, perhaps no more diskspace for %s?\n",fn);
		return FALSE;
	}
	fclose(F);
        return TRUE;
}

void
SHELL_SaveRegistry() {
	char	*fn;
	struct	passwd	*pwd;
	char	buf[4];
	HKEY	hkey;
	int	all;

	all=0;
	if (RegOpenKey16(HKEY_CURRENT_USER,KEY_REGISTRY,&hkey)!=ERROR_SUCCESS) {
		strcpy(buf,"yes");
	} else {
		DWORD len,junk,type;

		len=4;
		if (	(ERROR_SUCCESS!=RegQueryValueEx32A(
				hkey,
				VAL_SAVEUPDATED,
				&junk,
				&type,
				buf,
				&len
			))|| (type!=REG_SZ)
		)
			strcpy(buf,"yes");
		RegCloseKey(hkey);
	}
	if (lstrcmpi32A(buf,"yes"))
		all=1;
	pwd=getpwuid(getuid());
	if (pwd!=NULL && pwd->pw_dir!=NULL)
        {
                char *tmp;

		fn=(char*)xmalloc( strlen(pwd->pw_dir) + strlen(WINE_PREFIX) +
                                   strlen(SAVE_CURRENT_USER) + 2 );
		strcpy(fn,pwd->pw_dir);
		strcat(fn,WINE_PREFIX);
		/* create the directory. don't care about errorcodes. */
		mkdir(fn,0755); /* drwxr-xr-x */
		strcat(fn,"/"SAVE_CURRENT_USER);
		tmp = (char*)xmalloc(strlen(fn)+strlen(".tmp")+1);
		strcpy(tmp,fn);strcat(tmp,".tmp");
		if (_savereg(key_current_user,tmp,all)) {
			if (-1==rename(tmp,fn)) {
				perror("rename tmp registry");
				unlink(tmp);
			}
		}
		free(tmp);
		free(fn);
		fn=(char*)xmalloc(strlen(pwd->pw_dir)+strlen(WINE_PREFIX)+strlen(SAVE_LOCAL_MACHINE)+2);
		strcpy(fn,pwd->pw_dir);
		strcat(fn,WINE_PREFIX"/"SAVE_LOCAL_MACHINE);
		tmp = (char*)xmalloc(strlen(fn)+strlen(".tmp")+1);
		strcpy(tmp,fn);strcat(tmp,".tmp");
		if (_savereg(key_local_machine,tmp,all)) {
			if (-1==rename(tmp,fn)) {
				perror("rename tmp registry");
				unlink(tmp);
			}
		}
		free(tmp);
		free(fn);
	} else
		fprintf(stderr,"SHELL_SaveRegistry:failed to get homedirectory of UID %d.\n",getuid());
}

/************************ LOAD Registry Function ****************************/

static LPKEYSTRUCT
_find_or_add_key(LPKEYSTRUCT lpkey,LPWSTR keyname) {
	LPKEYSTRUCT	lpxkey,*lplpkey;

	lplpkey= &(lpkey->nextsub);
	lpxkey	= *lplpkey;
	while (lpxkey) {
		if (!lstrcmpi32W(lpxkey->keyname,keyname))
			break;
		lplpkey	= &(lpxkey->next);
		lpxkey	= *lplpkey;
	}
	if (lpxkey==NULL) {
		*lplpkey = (LPKEYSTRUCT)xmalloc(sizeof(KEYSTRUCT));
		lpxkey	= *lplpkey;
		memset(lpxkey,'\0',sizeof(KEYSTRUCT));
		lpxkey->keyname	= keyname;
	} else
		free(keyname);
	return lpxkey;
}

static void
_find_or_add_value(
	LPKEYSTRUCT lpkey,LPWSTR name,DWORD type,LPBYTE data,DWORD len,
	DWORD lastmodified
) {
	LPKEYVALUE	val=NULL;
	int		i;

	if (name && !*name) {/* empty string equals default (NULL) value */
		free(name);
		name = NULL;
	}

	for (i=0;i<lpkey->nrofvalues;i++) {
		val=lpkey->values+i;
		if (name==NULL) {
			if (val->name==NULL)
				break;
		} else {
			if (	val->name!=NULL && 
				!lstrcmpi32W(val->name,name)
			)
				break;
		}
	}
	if (i==lpkey->nrofvalues) {
		lpkey->values = xrealloc(
			lpkey->values,
			(++lpkey->nrofvalues)*sizeof(KEYVALUE)
		);
		val=lpkey->values+i;
		memset(val,'\0',sizeof(KEYVALUE));
		val->name = name;
	} else {
		if (name)
			free(name);
	}
	if (val->lastmodified<lastmodified) {
		val->lastmodified=lastmodified;
		val->type = type;
		val->len  = len;
		if (val->data) 
			free(val->data);
		val->data = data;
	} else
		free(data);
}


/* reads a line including dynamically enlarging the readbuffer and throwing
 * away comments
 */
static int 
_wine_read_line(FILE *F,char **buf,int *len) {
	char	*s,*curread;
	int	mylen,curoff;

	curread	= *buf;
	mylen	= *len;
	**buf	= '\0';
	while (1) {
		while (1) {
			s=fgets(curread,mylen,F);
			if (s==NULL)
				return 0; /* EOF */
			if (NULL==(s=strchr(curread,'\n'))) {
				/* buffer wasn't large enough */
				curoff	= strlen(*buf);
				*buf	= xrealloc(*buf,*len*2);
				curread	= *buf + curoff;
				mylen	= *len;	/* we filled up the buffer and 
						 * got new '*len' bytes to fill
						 */
				*len	= *len * 2;
			} else {
				*s='\0';
				break;
			}
		}
		/* throw away comments */
		if (**buf=='#' || **buf==';') {
			curread	= *buf;
			mylen	= *len;
			continue;
		}
		if (s) 	/* got end of line */
			break;
	}
	return 1;
}

/* converts a char* into a UNICODE string (up to a special char)
 * and returns the position exactly after that string
 */
static char*
_wine_read_USTRING(char *buf,LPWSTR *str) {
	char	*s;
	LPWSTR	ws;

	/* read up to "=" or "\0" or "\n" */
	s	= buf;
	if (*s == '=') {
		/* empty string is the win3.1 default value(NULL)*/
		*str	= NULL;
		return s;
	}
	*str	= (LPWSTR)xmalloc(2*strlen(buf)+2);
	ws	= *str;
	while (*s && (*s!='\n') && (*s!='=')) {
		if (*s!='\\')
			*ws++=*((unsigned char*)s++);
		else {
			s++;
			if (*s=='\\') {
				*ws++='\\';
				s++;
				continue;
			}
			if (*s!='u') {
				fprintf(stderr,"_wine_read_USTRING:Non unicode escape sequence \\%c found in |%s|\n",*s,buf);
				*ws++='\\';
				*ws++=*s++;
			} else {
				char	xbuf[5];
				int	wc;

				s++;
				memcpy(xbuf,s,4);xbuf[4]='\0';
				if (!sscanf(xbuf,"%x",&wc))
					fprintf(stderr,"_wine_read_USTRING:strange escape sequence %s found in |%s|\n",xbuf,buf);
				s+=4;
				*ws++	=(unsigned short)wc;
			}
		}
	}
	*ws	= 0;
	ws	= *str;
	if (*ws)
		*str	= strdupW(*str);
	else
		*str	= NULL;
	free(ws);
	return s;
}

static int
_wine_loadsubkey(
	FILE *F,LPKEYSTRUCT lpkey,int level,char **buf,int *buflen,int optflag
) {
	LPKEYSTRUCT	lpxkey;
	int		i;
	char		*s;
	LPWSTR		name;

	lpkey->flags |= optflag;

	/* good. we already got a line here ... so parse it */
	lpxkey	= NULL;
	while (1) {
		i=0;s=*buf;
		while (*s=='\t') {
			s++;
			i++;
		}
		if (i>level) {
			if (lpxkey==NULL) {
				fprintf(stderr,"_load_subkey:Got a subhierarchy without resp. key?\n");
				return 0;
			}
			_wine_loadsubkey(F,lpxkey,level+1,buf,buflen,optflag);
			continue;
		}
		/* let the caller handle this line */
		if (i<level || **buf=='\0')
			return 1;

		/* it can be: a value or a keyname. Parse the name first */
		s=_wine_read_USTRING(s,&name);

		/* switch() default: hack to avoid gotos */
		switch (0) {
		default:
			if (*s=='\0') {
				lpxkey=_find_or_add_key(lpkey,name);
			} else {
				LPBYTE		data;
				int		len,lastmodified,type;

				if (*s!='=') {
					fprintf(stderr,"_wine_load_subkey:unexpected character: %c\n",*s);
					break;
				}
				s++;
				if (2!=sscanf(s,"%d,%d,",&type,&lastmodified)) {
					fprintf(stderr,"_wine_load_subkey: haven't understood possible value in |%s|, skipping.\n",*buf);
					break;
				}
				/* skip the 2 , */
				s=strchr(s,',');s++;
				s=strchr(s,',');s++;
				if ((1<<type) & UNICONVMASK) {
					s=_wine_read_USTRING(s,(LPWSTR*)&data);
					if (data)
						len = lstrlen32W((LPWSTR)data)*2+2;
					else	
						len = 0;
				} else {
					len=strlen(s)/2;
					data = (LPBYTE)xmalloc(len+1);
					for (i=0;i<len;i++) {
						data[i]=0;
						if (*s>='0' && *s<='9')
							data[i]=(*s-'0')<<4;
						if (*s>='a' && *s<='f')
							data[i]=(*s-'a')<<4;
						if (*s>='A' && *s<='F')
							data[i]=(*s-'A')<<4;
						s++;
						if (*s>='0' && *s<='9')
							data[i]|=*s-'0';
						if (*s>='a' && *s<='f')
							data[i]|=*s-'a';
						if (*s>='A' && *s<='F')
							data[i]|=*s-'A';
						s++;
					}
				}
				_find_or_add_value(lpkey,name,type,data,len,lastmodified);
			}
		}
		/* read the next line */
		if (!_wine_read_line(F,buf,buflen))
			return 1;
	}
	return 1;
}

static int
_wine_loadsubreg(FILE *F,LPKEYSTRUCT lpkey,int optflag) {
	int	ver;
	char	*buf;
	int	buflen;

	buf=xmalloc(10);buflen=10;
	if (!_wine_read_line(F,&buf,&buflen)) {
		free(buf);
		return 0;
	}
	if (!sscanf(buf,"WINE REGISTRY Version %d",&ver)) {
		free(buf);
		return 0;
	}
	if (ver!=REGISTRY_SAVE_VERSION) {
		dprintf_reg(stddeb,__FILE__":_wine_loadsubreg:Old format (%d) registry found, ignoring it. (buf was %s).\n",ver,buf);
		free(buf);
		return 0;
	}
	if (!_wine_read_line(F,&buf,&buflen)) {
		free(buf);
		return 0;
	}
	if (!_wine_loadsubkey(F,lpkey,0,&buf,&buflen,optflag)) {
		free(buf);
		return 0;
	}
	free(buf);
	return 1;
}

static void
_wine_loadreg(LPKEYSTRUCT lpkey,char *fn,int optflag) {
	FILE	*F;

	F=fopen(fn,"rb");
	if (F==NULL) {
		dprintf_reg(stddeb,__FILE__":Couldn't open %s for reading: %s\n",
			fn,strerror(errno)
		);
		return;
	}
	if (!_wine_loadsubreg(F,lpkey,optflag)) {
		fclose(F);
		unlink(fn);
		return;
	}
	fclose(F);
}

static void
_copy_registry(LPKEYSTRUCT from,LPKEYSTRUCT to) {
	LPKEYSTRUCT	lpxkey;
	int		j;
	LPKEYVALUE	valfrom;

	from=from->nextsub;
	while (from) {
		lpxkey = _find_or_add_key(to,strdupW(from->keyname));

		for (j=0;j<from->nrofvalues;j++) {
			LPWSTR	name;
			LPBYTE	data;

			valfrom = from->values+j;
			name=valfrom->name;
			if (name) name=strdupW(name);
			data=(LPBYTE)malloc(valfrom->len);
			memcpy(data,valfrom->data,valfrom->len);

			_find_or_add_value(
				lpxkey,
				name,
				valfrom->type,
				data,
				valfrom->len,
				valfrom->lastmodified
			);
		}
		_copy_registry(from,lpxkey);
		from = from->next;
	}
}

/* WINDOWS 95 REGISTRY LOADER */
/* 
 * Structure of a win95 registry database.
 * main header:
 * 0 :	"CREG"	- magic
 * 4 :	DWORD version
 * 8 :	DWORD offset_of_RGDB_part
 * 0C..0F:	? (someone fill in please)
 * 10:  WORD	number of RGDB blocks
 * 12:  WORD	?
 * 14:  WORD	always 0000?
 * 16:  WORD	always 0001?
 * 18..1F:	? (someone fill in please)
 *
 * 20: RGKN_section:
 *   header:
 * 	0 :		"RGKN"	- magic
 *      4 : DWORD	offset to first RGDB section
 *      8 : DWORD	offset to ?
 * 	C..0x1B: 	? (fill in)
 *      0x20 ... offset_of_RGDB_part: Disk Key Entry structures
 *
 *   Disk Key Entry Structure:
 *	00: DWORD	- unknown
 *	04: DWORD	- unknown
 *	08: DWORD	- unknown, but usually 0xFFFFFFFF on win95 systems
 *	0C: DWORD	- disk address of PreviousLevel Key.
 *	10: DWORD	- disk address of Next Sublevel Key.
 *	14: DWORD	- disk address of Next Key (on same level).
 * DKEP>18: WORD	- Nr, Low Significant part.
 *	1A: WORD	- Nr, High Significant part.
 *
 * The disk address always points to the nr part of the previous key entry 
 * of the referenced key. Don't ask me why, or even if I got this correct
 * from staring at 1kg of hexdumps. (DKEP)
 *
 * The number of the entry is the low byte of the Low Significant Part ored
 * with 0x100 * (low byte of the High Significant part)
 * (C expression : nr = (nrLS & 0xFF) | ((nrHS &0xFF)<<8))
 *
 * There are two minor corrections to the position of that structure.
 * 1. If the address is xxx014 or xxx018 it will be aligned to xxx01c AND 
 *    the DKE reread from there.
 * 2. If the address is xxxFFx it will be aligned to (xxx+1)000.
 * (FIXME: slightly better explanation needed here)
 *
 * RGDB_section:
 * 	00:		"RGDB"	- magic
 *	04: DWORD	offset to next RGDB section
 *	08: DWORD	?
 *	0C: WORD	always 000d?
 *	0E: WORD	RGDB block number
 *	10:	DWORD	? (equals value at offset 4 - value at offset 8)
 *	14..1F:		?
 *	20.....:	disk keys
 *
 * disk key:
 * 	00: 	DWORD	nextkeyoffset	- offset to the next disk key structure
 *	08: 	WORD	nrLS		- low significant part of NR
 *	0A: 	WORD	nrHS		- high significant part of NR
 *	0C: 	DWORD	bytesused	- bytes used in this structure.
 *	10: 	WORD	name_len	- length of name in bytes. without \0
 *	12: 	WORD	nr_of_values	- number of values.
 *	14: 	char	name[name_len]	- name string. No \0.
 *	14+name_len: disk values
 *	nextkeyoffset: ... next disk key
 *
 * disk value:
 *	00:	DWORD	type		- value type (hmm, could be WORD too)
 *	04:	DWORD			- unknown, usually 0
 *	08:	WORD	namelen		- length of Name. 0 means name=NULL
 *	0C:	WORD	datalen		- length of Data.
 *	10:	char	name[namelen]	- name, no \0
 *	10+namelen: BYTE	data[datalen] - data, without \0 if string
 *	10+namelen+datalen: next values or disk key
 *
 * Disk keys are layed out flat ... But, sometimes, nrLS and nrHS are both
 * 0xFFFF, which means skipping over nextkeyoffset bytes (including this
 * structure) and reading another RGDB_section.
 * repeat until end of file.
 *
 * An interesting relationship exists in RGDB_section. The value at offset
 * 10 equals the value at offset 4 minus the value at offset 8. I have no
 * idea at the moment what this means.  (Kevin Cozens)
 *
 * FIXME: this description needs some serious help, yes.
 */

struct	_w95keyvalue {
	unsigned long		type;
	unsigned short		datalen;
	char			*name;
	unsigned char		*data;
	unsigned long		x1;
	int			lastmodified;
};

struct 	_w95key {
	char			*name;
	int			nrofvals;
	struct	_w95keyvalue	*values;
	unsigned long		dkeaddr;
	unsigned long 		x1;
	unsigned long 		x2;
	unsigned long 		x3;
	unsigned long 		xx1;
	struct _w95key		*prevlvl;
	struct _w95key		*nextsub;
	struct _w95key		*next;
};

/* fast lookup table dkeaddr->nr */
struct 	_w95nr2da {
	unsigned long		dkeaddr;
	unsigned long		nr;
	struct _w95key		*key;
};


static void
_w95_walk_tree(LPKEYSTRUCT lpkey,struct _w95key *key) {
	int		i;
	LPKEYSTRUCT	lpxkey;
	LPWSTR		name;

	while (key) {
		if (key->name == NULL) {
			fprintf(stderr,"_w95_walk_tree:Please report: key with dkeaddr %lx not loaded, skipping hierarchy\n",
				key->dkeaddr
			);
			return;
		}
		lpxkey=_find_or_add_key(lpkey,strdupA2W(key->name));

		if (key->nrofvals<0) {
			/* shouldn't happen */
			fprintf(stderr,"key %s already processed!\n",key->name);
			key = key->next;
			continue;
		}
		for (i=0;i<key->nrofvals;i++) {
			LPBYTE	data;
			int	len;

			name = strdupA2W(key->values[i].name);
			if (!*name) {
				free(name);
				name = NULL;
			}
			free(key->values[i].name);

			len	= key->values[i].datalen;
			data	= key->values[i].data;
			if ((1<<key->values[i].type) & UNICONVMASK) {
				data = (BYTE*)strdupA2W(data);
				len  = lstrlen32W((LPWSTR)data)*2+2;
				free(key->values[i].data);
			}
			_find_or_add_value(
				lpxkey,
				name,
				key->values[i].type,
				data,
				len,
				key->values[i].lastmodified
			);
		}
		if (key->values) {
			free(key->values);
			key->values = NULL;
		}
		key->nrofvals=-key->nrofvals-1;
		_w95_walk_tree(lpxkey,key->nextsub);
		key=key->next;
	}
}

/* small helper function to adjust address offset (dkeaddrs) */
static unsigned long
_w95_adj_da(unsigned long dkeaddr) {
	if ((dkeaddr&0xFFF)<0x018) {
		int	diff;

		diff=0x1C-(dkeaddr&0xFFF);
		return dkeaddr+diff;
	}
	if (((dkeaddr+0x1C)&0xFFF)<0x1C) {
		/* readjust to 0x000,
		 * but ONLY if we are >0x1000 already
		 */
		if (dkeaddr & ~0xFFF)
			return dkeaddr & ~0xFFF;
	}
	return dkeaddr;
}

static int
_w95dkecomp(struct _w95nr2da *a,struct _w95nr2da *b){return a->dkeaddr-b->dkeaddr;}

static struct _w95key*
_w95dkelookup(unsigned long dkeaddr,int n,struct _w95nr2da *nr2da,struct _w95key *keys) {
int	i;
int left, right;

	if (dkeaddr == 0xFFFFFFFF)
		return NULL;
	if (dkeaddr<0x20)
		return NULL;
	dkeaddr=_w95_adj_da(dkeaddr+0x1c);
	left=0;
	right=n-1;
	while(left<=right)
	{
		i=(left+right)/2;

		if(nr2da[i].dkeaddr == dkeaddr)
			return nr2da[i].key;
		else if(nr2da[i].dkeaddr < dkeaddr)
			left=i+1;
		else
			right=i-1;
	}
	/* 0x3C happens often, just report unusual values */
	if (dkeaddr!=0x3c)
		dprintf_reg(stddeb,"search hasn't found dkeaddr %lx?\n",dkeaddr);
	return NULL;
}

static void
_w95_loadreg(char* fn,LPKEYSTRUCT lpkey) {
	/* Disk Key Entry structure (RGKN part) */
	struct	dke {
		unsigned long		x1;
		unsigned long		x2;
		unsigned long		x3;/*usually 0xFFFFFFFF */
		unsigned long		prevlvl;
		unsigned long		nextsub;
		unsigned long		next;
		unsigned short		nrLS;
		unsigned short		nrMS;
	};
	/* Disk Key Header structure (RGDB part) */
	struct	dkh {
		unsigned long		nextkeyoff; 
		unsigned short		nrLS;
		unsigned short		nrMS;
		unsigned long		bytesused;
		unsigned short		keynamelen;
		unsigned short		values;
		unsigned long		xx1;
		/* keyname */
		/* disk key values or nothing */
	};
	/* Disk Key Value structure */
	struct	dkv {
		unsigned long		type;
		unsigned long		x1;
		unsigned short		valnamelen;
		unsigned short		valdatalen;
		/* valname, valdata */
	};
    struct  _w95nr2da   *nr2da;

	HFILE32		hfd;
	int		lastmodified;
	char		magic[5];
	unsigned long	nr,pos,i,j,where,version,rgdbsection,end,off_next_rgdb;
	struct	_w95key	*keys,*key;
	int		nrofdkes;
	unsigned char	*data,*curdata,*nextrgdb;
	OFSTRUCT	ofs;
	BY_HANDLE_FILE_INFORMATION hfdinfo;

	dprintf_reg(stddeb,"Loading Win95 registry database '%s'\n",fn);
	hfd=OpenFile32(fn,&ofs,OF_READ);
	if (hfd==HFILE_ERROR32)
		return;
	magic[4]=0;
	if (4!=_lread32(hfd,magic,4))
		return;
	if (strcmp(magic,"CREG")) {
		fprintf(stddeb,"%s is not a w95 registry.\n",fn);
		return;
	}
	if (4!=_lread32(hfd,&version,4))
		return;
	if (4!=_lread32(hfd,&rgdbsection,4))
		return;
	if (-1==_llseek32(hfd,0x20,SEEK_SET))
		return;
	if (4!=_lread32(hfd,magic,4))
		return;
	if (strcmp(magic,"RGKN")) {
		dprintf_reg(stddeb,"second IFF header not RGKN, but %s\n",magic);
		return;
	}

	/* STEP 1: Keylink structures */
	if (-1==_llseek32(hfd,0x40,SEEK_SET))
		return;
	where	= 0x40;
	end	= rgdbsection;

	/* I removed the '+100' that was here. The adjustments to dkeaddr   */
	/* imply alignments to the data in 'data' which would mean it is    */
	/* larger than the number of dke entries it holds therefore nrofdkes*/
	/* would be equal to or larger than it needs to be without the need */
	/* for the +100 - kc                                                */
	nrofdkes = (end-where)/sizeof(struct dke);

	data = (char*)xmalloc(end-where);
	if ((end-where)!=_lread32(hfd,data,end-where))
		return;
	curdata = data;

	keys = (struct _w95key*)xmalloc(nrofdkes * sizeof(struct _w95key));
	memset(keys,'\0',nrofdkes*sizeof(struct _w95key));
    nr2da= (struct _w95nr2da*)xmalloc(nrofdkes * sizeof(struct _w95nr2da));
    memset(nr2da,'\0',nrofdkes*sizeof(struct _w95nr2da));

#if DEBUG_W95_LOADREG
	dprintf_reg(stddeb,"nrofdkes = %d\n", nrofdkes);
#endif
	for (i=0;i<nrofdkes;i++) {
		struct	dke	dke;
		unsigned long 	dkeaddr;

		pos=curdata-data+0x40;
		memcpy(&dke,curdata,sizeof(dke));
		curdata+=sizeof(dke);
		nr = dke.nrLS + (dke.nrMS<<8);
#if DEBUG_W95_LOADREG
		dprintf_reg(stddeb,"%ld: nr = %ld, nrMS:nrLS = %04X:%X\n",
					i,nr,dke.nrMS,dke.nrLS);
#endif
		dkeaddr=pos-4;
		if ((dkeaddr&0xFFF)<0x018) {
			int	diff;

			diff=0x1C-(dkeaddr&0xFFF);
			dkeaddr+=diff;
			curdata+=diff-sizeof(dke);
			memcpy(&dke,curdata,sizeof(dke));
			nr = dke.nrLS + (dke.nrMS<<8);
#if DEBUG_W95_LOADREG
			dprintf_reg(stddeb,"> nr = %lu, nrMS:nrLS = %04X:%X\n",
						nr,dke.nrMS,dke.nrLS);
#endif
			curdata+=sizeof(dke);
		}
		if (((dkeaddr+0x1C)&0xFFF)<0x1C) {
			/* readjust to 0x000,
			 * but ONLY if we are >0x1000 already
			 */
			if (dkeaddr & ~0xFFF)
				dkeaddr &= ~0xFFF;
		}
		/* For the time being we will assume that all values of */
		/* nr are valid unless the following condition is true. */
		/* This value is obtained when dke.nrLS and dke.nrMS are*/
		/* both FFFF as dke.nrMS is only shifted by 8 before the*/
		/* add and not 16. -kc                                  */
		if (nr==0x0100FEFF)
			continue;
		for (j = 0; j < i; ++j) {
			if (nr2da[j].nr == nr)
				break;
		}
		if (j < i) {
			key = nr2da[j].key;
			if (key && key->dkeaddr) {
				int	x;

				for (x=sizeof(dke);x--;)
					if (((char*)&dke)[x])
						break;
				if (x==-1)
					break;	/* finished reading if we got only 0 */
				if (nr) {
					if ((dke.next!=(long)key->next) ||
						(dke.nextsub!=(long)key->nextsub) ||
						(dke.prevlvl!=(long)key->prevlvl) 
					)
						dprintf_reg(stddeb,"key doubled? nr=%lu,key->dkeaddr=%lx,dkeaddr=%lx\n",nr,key->dkeaddr,dkeaddr);
				}
				continue;
			}
		}
#if DEBUG_W95_LOADREG
		dprintf_reg(stddeb,"- nr=%lu,dkeaddr=%lx\n",nr,dkeaddr);
#endif
		nr2da[i].nr = nr;
		nr2da[i].dkeaddr = dkeaddr;
		nr2da[i].key = &keys[i];

		keys[i].dkeaddr = dkeaddr;
		keys[i].x1 = dke.x1;
		keys[i].x2 = dke.x2;
		keys[i].x3 = dke.x3;
		keys[i].prevlvl= (struct _w95key*)dke.prevlvl;
		keys[i].nextsub= (struct _w95key*)dke.nextsub;
		keys[i].next 	= (struct _w95key*)dke.next;
	}
	free(data);

	nrofdkes = i;	/* This is the real number of dke entries */
#if DEBUG_W95_LOADREG
	dprintf_reg(stddeb,"nrofdkes = %d\n", nrofdkes);
#endif

	qsort(nr2da,nrofdkes,sizeof(nr2da[0]),
              (int(*)(const void *,const void *))_w95dkecomp);

	/* STEP 2: keydata & values */
	if (!GetFileInformationByHandle(hfd,&hfdinfo))
		return;
	end = hfdinfo.nFileSizeLow;
	lastmodified = DOSFS_FileTimeToUnixTime(&hfdinfo.ftLastWriteTime,NULL);

	if (-1==_llseek32(hfd,rgdbsection,SEEK_SET))
		return;
	data = (char*)xmalloc(end-rgdbsection);
	if ((end-rgdbsection)!=_lread32(hfd,data,end-rgdbsection))
		return;
	_lclose32(hfd);
	curdata = data;
	memcpy(magic,curdata,4);
	memcpy(&off_next_rgdb,curdata+4,4);
	nextrgdb = curdata+off_next_rgdb;
	if (strcmp(magic,"RGDB")) {
		dprintf_reg(stddeb,"third IFF header not RGDB, but %s\n",magic);
		return;
	}

	curdata=data+0x20;
	while (1) {
		struct	dkh dkh;
		int		bytesread;
		struct	_w95key	xkey;	/* Used inside second main loop */

		bytesread = 0;
		if (curdata>=nextrgdb) {
			curdata = nextrgdb;
			if (!strncmp(curdata,"RGDB",4)) {
				memcpy(&off_next_rgdb,curdata+4,4);
				nextrgdb = curdata+off_next_rgdb;
				curdata+=0x20;
			} else {
				dprintf_reg(stddeb,"at end of RGDB section, but no next header (%x of %lx). Breaking.\n",curdata-data,end-rgdbsection);
				break;
			}
		}
#define XREAD(whereto,len) \
	if ((curdata-data+len)<end) {\
		memcpy(whereto,curdata,len);\
		curdata+=len;\
		bytesread+=len;\
	}

		XREAD(&dkh,sizeof(dkh));
		nr = dkh.nrLS + (dkh.nrMS<<8);
		if (dkh.nrLS == 0xFFFF) {
				/* skip over key using nextkeyoff */
 				curdata+=dkh.nextkeyoff-sizeof(struct dkh);
				continue;
		} 
		for (i = 0; i < nrofdkes; ++i) {
			if (nr2da[i].nr == nr && nr2da[i].dkeaddr) {
				key = nr2da[i].key;
				break;
			}
		}
		if (i >= nrofdkes) {
			/* Move the next statement to just before the previous for */
			/* loop to prevent the compiler from issuing a warning -kc */
			key = &xkey;
			memset(key,'\0',sizeof(xkey));
			dprintf_reg(stddeb,"haven't found nr %lu.\n",nr);
		} else {
			if (!key->dkeaddr)
				dprintf_reg(stddeb,"key with nr=%lu has no dkeaddr?\n",nr);
		}
		key->nrofvals	= dkh.values;
		key->name	= (char*)xmalloc(dkh.keynamelen+1);
		key->xx1	= dkh.xx1;
		XREAD(key->name,dkh.keynamelen);
		key->name[dkh.keynamelen]=0;
		if (key->nrofvals) {
			key->values = (struct _w95keyvalue*)xmalloc(
				sizeof(struct _w95keyvalue)*key->nrofvals
			);
			for (i=0;i<key->nrofvals;i++) {
				struct	dkv	dkv;

				XREAD(&dkv,sizeof(dkv));
				key->values[i].type = dkv.type;
				key->values[i].name = (char*)xmalloc(
					dkv.valnamelen+1
				);
				key->values[i].datalen = dkv.valdatalen;
				key->values[i].data = (unsigned char*)xmalloc(
					dkv.valdatalen+1
				);
				key->values[i].x1   = dkv.x1;
				XREAD(key->values[i].name,dkv.valnamelen);
				XREAD(key->values[i].data,dkv.valdatalen);
				key->values[i].data[dkv.valdatalen]=0;
				key->values[i].name[dkv.valnamelen]=0;
				key->values[i].lastmodified=lastmodified;
			}
		}
		if (bytesread != dkh.nextkeyoff) {
			if (dkh.bytesused != bytesread)
				dprintf_reg(stddeb,
					"read has difference in read bytes (%d) and nextoffset (%lu) (bytesused=%lu)\n",bytesread,dkh.nextkeyoff,
					dkh.bytesused
				);
			curdata += dkh.nextkeyoff-bytesread;
		}
		key->prevlvl	= _w95dkelookup((long)key->prevlvl,nrofdkes,nr2da,keys);
		key->nextsub	= _w95dkelookup((long)key->nextsub,nrofdkes,nr2da,keys);
		key->next	= _w95dkelookup((long)key->next,nrofdkes,nr2da,keys);
		if (!bytesread)
			break;
	}
	free(data);
	_w95_walk_tree(lpkey,keys);
	free(nr2da);
	free(keys);
}

/* WINDOWS 31 REGISTRY LOADER, supplied by Tor Sjøwall, tor@sn.no */

/*
    reghack - windows 3.11 registry data format demo program.

    The reg.dat file has 3 parts, a header, a table of 8-byte entries that is
    a combined hash table and tree description, and finally a text table.

    The header is obvious from the struct header. The taboff1 and taboff2
    fields are always 0x20, and their usage is unknown.

    The 8-byte entry table has various entry types.

    tabent[0] is a root index. The second word has the index of the root of
            the directory.
    tabent[1..hashsize] is a hash table. The first word in the hash entry is
            the index of the key/value that has that hash. Data with the same
            hash value are on a circular list. The other three words in the
            hash entry are always zero.
    tabent[hashsize..tabcnt] is the tree structure. There are two kinds of
            entry: dirent and keyent/valent. They are identified by context.
    tabent[freeidx] is the first free entry. The first word in a free entry
            is the index of the next free entry. The last has 0 as a link.
            The other three words in the free list are probably irrelevant.

    Entries in text table are preceeded by a word at offset-2. This word
    has the value (2*index)+1, where index is the referring keyent/valent
    entry in the table. I have no suggestion for the 2* and the +1.
    Following the word, there are N bytes of data, as per the keyent/valent
    entry length. The offset of the keyent/valent entry is from the start
    of the text table to the first data byte.

    This information is not available from Microsoft. The data format is
    deduced from the reg.dat file by me. Mistakes may
    have been made. I claim no rights and give no guarantees for this program.

    Tor Sjøwall, tor@sn.no
*/

/* reg.dat header format */
struct _w31_header {
	char		cookie[8];	/* 'SHCC3.10' */
	unsigned long	taboff1;	/* offset of hash table (??) = 0x20 */
	unsigned long	taboff2;	/* offset of index table (??) = 0x20 */
	unsigned long	tabcnt;		/* number of entries in index table */
	unsigned long	textoff;	/* offset of text part */
	unsigned long	textsize;	/* byte size of text part */
	unsigned short	hashsize;	/* hash size */
	unsigned short	freeidx;	/* free index */
};

/* generic format of table entries */
struct _w31_tabent {
	unsigned short w0, w1, w2, w3;
};

/* directory tabent: */
struct _w31_dirent {
	unsigned short	sibling_idx;	/* table index of sibling dirent */
	unsigned short	child_idx;	/* table index of child dirent */
	unsigned short	key_idx;	/* table index of key keyent */
	unsigned short	value_idx;	/* table index of value valent */
};

/* key tabent: */
struct _w31_keyent {
	unsigned short	hash_idx;	/* hash chain index for string */
	unsigned short	refcnt;		/* reference count */
	unsigned short	length;		/* length of string */
	unsigned short	string_off;	/* offset of string in text table */
};

/* value tabent: */
struct _w31_valent {
	unsigned short	hash_idx;	/* hash chain index for string */
	unsigned short	refcnt;		/* reference count */
	unsigned short	length;		/* length of string */
	unsigned short	string_off;	/* offset of string in text table */
};

/* recursive helper function to display a directory tree */
void
__w31_dumptree(	unsigned short idx,
		unsigned char *txt,
		struct _w31_tabent *tab,
		struct _w31_header *head,
		LPKEYSTRUCT	lpkey,
		time_t		lastmodified,
		int		level
) {
	struct _w31_dirent	*dir;
	struct _w31_keyent	*key;
	struct _w31_valent	*val;
	LPKEYSTRUCT		xlpkey = NULL;
	LPWSTR			name,value;
	static char		tail[400];

	while (idx!=0) {
		dir=(struct _w31_dirent*)&tab[idx];

		if (dir->key_idx) {
			key = (struct _w31_keyent*)&tab[dir->key_idx];

			memcpy(tail,&txt[key->string_off],key->length);
			tail[key->length]='\0';
			/* all toplevel entries AND the entries in the 
			 * toplevel subdirectory belong to \SOFTWARE\Classes
			 */
			if (!level && !lstrcmp32A(tail,".classes")) {
				__w31_dumptree(dir->child_idx,txt,tab,head,lpkey,lastmodified,level+1);
				idx=dir->sibling_idx;
				continue;
			}
			name=strdupA2W(tail);

			xlpkey=_find_or_add_key(lpkey,name);

			/* only add if leaf node or valued node */
			if (dir->value_idx!=0||dir->child_idx==0) {
				if (dir->value_idx) {
					val=(struct _w31_valent*)&tab[dir->value_idx];
					memcpy(tail,&txt[val->string_off],val->length);
					tail[val->length]='\0';
					value=strdupA2W(tail);
					_find_or_add_value(xlpkey,NULL,REG_SZ,(LPBYTE)value,lstrlen32W(value)*2+2,lastmodified);
				}
			}
		} else {
			dprintf_reg(stddeb,"__w31_dumptree:strange: no directory key name, idx=%04x\n", idx);
		}
		__w31_dumptree(dir->child_idx,txt,tab,head,xlpkey,lastmodified,level+1);
		idx=dir->sibling_idx;
	}
}

void
_w31_loadreg() {
	HFILE32			hf;
	struct _w31_header	head;
	struct _w31_tabent	*tab;
	unsigned char		*txt;
	int			len;
	OFSTRUCT		ofs;
	BY_HANDLE_FILE_INFORMATION hfinfo;
	time_t			lastmodified;
	HKEY			hkey;
	LPKEYSTRUCT		lpkey;

	hf = OpenFile32("reg.dat",&ofs,OF_READ);
	if (hf==HFILE_ERROR32)
		return;

	/* read & dump header */
	if (sizeof(head)!=_lread32(hf,&head,sizeof(head))) {
		dprintf_reg(stddeb,"_w31_loadreg:reg.dat is too short.\n");
		_lclose32(hf);
		return;
	}
	if (memcmp(head.cookie, "SHCC3.10", sizeof(head.cookie))!=0) {
		dprintf_reg(stddeb,"_w31_loadreg:reg.dat has bad signature.\n");
		_lclose32(hf);
		return;
	}

	len = head.tabcnt * sizeof(struct _w31_tabent);
	/* read and dump index table */
	tab = xmalloc(len);
	if (len!=_lread32(hf,tab,len)) {
		dprintf_reg(stderr,"_w31_loadreg:couldn't read %d bytes.\n",len); 
		free(tab);
		_lclose32(hf);
		return;
	}

	/* read text */
	txt = xmalloc(head.textsize);
	if (-1==_llseek32(hf,head.textoff,SEEK_SET)) {
		dprintf_reg(stderr,"_w31_loadreg:couldn't seek to textblock.\n"); 
		free(tab);
		free(txt);
		_lclose32(hf);
		return;
	}
	if (head.textsize!=_lread32(hf,txt,head.textsize)) {
		dprintf_reg(stderr,"_w31_loadreg:textblock too short (%d instead of %ld).\n",len,head.textsize); 
		free(tab);
		free(txt);
		_lclose32(hf);
		return;
	}

	if (!GetFileInformationByHandle(hf,&hfinfo)) {
		dprintf_reg(stderr,"_w31_loadreg:GetFileInformationByHandle failed?.\n"); 
		free(tab);
		free(txt);
		_lclose32(hf);
		return;
	}
	lastmodified = DOSFS_FileTimeToUnixTime(&hfinfo.ftLastWriteTime,NULL);

	if (RegCreateKey16(HKEY_LOCAL_MACHINE,"\\SOFTWARE\\Classes",&hkey)!=ERROR_SUCCESS)
		return;
	lpkey = lookup_hkey(hkey);
	__w31_dumptree(tab[0].w1,txt,tab,&head,lpkey,lastmodified,0);
	free(tab);
	free(txt);
	_lclose32(hf);
	return;
}

void
SHELL_LoadRegistry() {
	char	*fn;
	struct	passwd	*pwd;
	LPKEYSTRUCT	lpkey;
	HKEY		hkey;


	if (key_classes_root==NULL)
		SHELL_Init();

	/* Load windows 3.1 entries */
	_w31_loadreg();
	/* Load windows 95 entries */
	_w95_loadreg("C:\\system.1st",	key_local_machine);
	_w95_loadreg("system.dat",	key_local_machine);
	_w95_loadreg("user.dat",	key_users);

	/* the global user default is loaded under HKEY_USERS\\.Default */
	RegCreateKey16(HKEY_USERS,".Default",&hkey);
	lpkey = lookup_hkey(hkey);
	_wine_loadreg(lpkey,SAVE_USERS_DEFAULT,0);

	/* HKEY_USERS\\.Default is copied to HKEY_CURRENT_USER */
	_copy_registry(lpkey,key_current_user);
	RegCloseKey(hkey);

	/* the global machine defaults */
	_wine_loadreg(key_local_machine,SAVE_LOCAL_MACHINE_DEFAULT,0);

	/* load the user saved registries */

	/* FIXME: use getenv("HOME") or getpwuid(getuid())->pw_dir ?? */

	pwd=getpwuid(getuid());
	if (pwd!=NULL && pwd->pw_dir!=NULL) {
		fn=(char*)xmalloc(strlen(pwd->pw_dir)+strlen(WINE_PREFIX)+strlen(SAVE_CURRENT_USER)+2);
		strcpy(fn,pwd->pw_dir);
		strcat(fn,WINE_PREFIX"/"SAVE_CURRENT_USER);
		_wine_loadreg(key_current_user,fn,REG_OPTION_TAINTED);
		free(fn);
		fn=(char*)xmalloc(strlen(pwd->pw_dir)+strlen(WINE_PREFIX)+strlen(SAVE_LOCAL_MACHINE)+2);
		strcpy(fn,pwd->pw_dir);
		strcat(fn,WINE_PREFIX"/"SAVE_LOCAL_MACHINE);
		_wine_loadreg(key_local_machine,fn,REG_OPTION_TAINTED);
		free(fn);
	} else
		fprintf(stderr,"SHELL_LoadRegistry:failed to get homedirectory of UID %d.\n",getuid());
	if (ERROR_SUCCESS==RegCreateKey16(HKEY_CURRENT_USER,KEY_REGISTRY,&hkey)) {
		DWORD	junk,type,len;
		char	data[5];

		len=4;
		if ((	RegQueryValueEx32A(
				hkey,
				VAL_SAVEUPDATED,
				&junk,
				&type,
				data,
				&len
			)!=ERROR_SUCCESS) ||
			type != REG_SZ
		)
			RegSetValueEx32A(hkey,VAL_SAVEUPDATED,0,REG_SZ,"yes",4);
		RegCloseKey(hkey);
	}
}


/********************* API FUNCTIONS ***************************************/
/*
 * Open Keys.
 *
 * All functions are stubs to RegOpenKeyEx32W where all the
 * magic happens. 
 *
 * FIXME: security,options,desiredaccess,...
 *
 * Callpath:
 * RegOpenKey16 -> RegOpenKey32A -> RegOpenKeyEx32A \
 *                                  RegOpenKey32W   -> RegOpenKeyEx32W 
 */

/* RegOpenKeyExW		[ADVAPI32.150] */
DWORD WINAPI RegOpenKeyEx32W(
	HKEY	hkey,
	LPCWSTR	lpszSubKey,
	DWORD	dwReserved,
	REGSAM	samDesired,
	LPHKEY	retkey
) {
	LPKEYSTRUCT	lpNextKey,lpxkey;
	LPWSTR		*wps;
	int		wpc,i;
	dprintf_reg(stddeb,"RegOpenKeyEx32W(%lx,%s,%ld,%lx,%p)\n",
		(LONG)hkey,W2C(lpszSubKey,0),dwReserved,samDesired,retkey
	);

	lpNextKey	= lookup_hkey(hkey);
	if (!lpNextKey)
		return SHELL_ERROR_BADKEY;
	if (!lpszSubKey || !*lpszSubKey) {
		add_handle(++currenthandle,lpNextKey,samDesired);
		*retkey=currenthandle;
		return SHELL_ERROR_SUCCESS;
	}
	split_keypath(lpszSubKey,&wps,&wpc);
	i 	= 0;
	while ((i<wpc) && (wps[i][0]=='\0')) i++;
	lpxkey	= lpNextKey;
	while (wps[i]) {
		lpxkey=lpNextKey->nextsub;
		while (lpxkey) {
			if (!lstrcmpi32W(wps[i],lpxkey->keyname))
				break;
			lpxkey=lpxkey->next;
		}
		if (!lpxkey) {
			FREE_KEY_PATH;
			return SHELL_ERROR_BADKEY;
		}
		i++;
		lpNextKey	= lpxkey;
	}
	add_handle(++currenthandle,lpxkey,samDesired);
	*retkey	= currenthandle;
	FREE_KEY_PATH;
	return	SHELL_ERROR_SUCCESS;
}

/* RegOpenKeyW			[ADVAPI32.151] */
DWORD WINAPI RegOpenKey32W(
	HKEY	hkey,
	LPCWSTR	lpszSubKey,
	LPHKEY	retkey
) {
	dprintf_reg(stddeb,"RegOpenKey32W(%lx,%s,%p)\n",
		(LONG)hkey,W2C(lpszSubKey,0),retkey
	);
	return RegOpenKeyEx32W(hkey,lpszSubKey,0,KEY_ALL_ACCESS,retkey);
}


/* RegOpenKeyExA		[ADVAPI32.149] */
DWORD WINAPI RegOpenKeyEx32A(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	DWORD	dwReserved,
	REGSAM	samDesired,
	LPHKEY	retkey
) {
	LPWSTR	lpszSubKeyW;
	DWORD	ret;

	dprintf_reg(stddeb,"RegOpenKeyEx32A(%lx,%s,%ld,%lx,%p)\n",
		(LONG)hkey,lpszSubKey,dwReserved,samDesired,retkey
	);
	if (lpszSubKey)
		lpszSubKeyW=strdupA2W(lpszSubKey);
	else
		lpszSubKeyW=NULL;
	ret=RegOpenKeyEx32W(hkey,lpszSubKeyW,dwReserved,samDesired,retkey);
	if (lpszSubKeyW)
		free(lpszSubKeyW);
	return ret;
}

/* RegOpenKeyA			[ADVAPI32.148] */
DWORD WINAPI RegOpenKey32A(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	LPHKEY	retkey
) {
	dprintf_reg(stddeb,"RegOpenKey32A(%lx,%s,%p)\n",
		(LONG)hkey,lpszSubKey,retkey
	);
	return	RegOpenKeyEx32A(hkey,lpszSubKey,0,KEY_ALL_ACCESS,retkey);
}

/* RegOpenKey			[SHELL.1] [KERNEL.217] */
DWORD WINAPI RegOpenKey16(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	LPHKEY	retkey
) {
	dprintf_reg(stddeb,"RegOpenKey16(%lx,%s,%p)\n",
		(LONG)hkey,lpszSubKey,retkey
	);
	return RegOpenKey32A(hkey,lpszSubKey,retkey);
}

/* 
 * Create keys
 * 
 * All those functions convert their respective 
 * arguments and call RegCreateKeyExW at the end.
 *
 * FIXME: no security,no access attrib,no optionhandling yet.
 *
 * Callpath:
 * RegCreateKey16 -> RegCreateKey32A -> RegCreateKeyEx32A \
 *                                      RegCreateKey32W   -> RegCreateKeyEx32W
 */

/* RegCreateKeyExW		[ADVAPI32.131] */
DWORD WINAPI RegCreateKeyEx32W(
	HKEY	hkey,
	LPCWSTR	lpszSubKey,
	DWORD	dwReserved,
	LPWSTR	lpszClass,
	DWORD	fdwOptions,
	REGSAM	samDesired,
	LPSECURITY_ATTRIBUTES lpSecAttribs,
	LPHKEY	retkey,
	LPDWORD	lpDispos
) {
	LPKEYSTRUCT	*lplpPrevKey,lpNextKey,lpxkey;
	LPWSTR		*wps;
	int		wpc,i;

/*FIXME: handle security/access/whatever */
	dprintf_reg(stddeb,"RegCreateKeyEx32W(%lx,%s,%ld,%s,%lx,%lx,%p,%p,%p)\n",
		(LONG)hkey,
		W2C(lpszSubKey,0),
		dwReserved,
		W2C(lpszClass,1),
		fdwOptions,
		samDesired,
		lpSecAttribs,
		retkey,
		lpDispos
	);

	lpNextKey	= lookup_hkey(hkey);
	if (!lpNextKey)
		return SHELL_ERROR_BADKEY;
	if (!lpszSubKey || !*lpszSubKey) {
		add_handle(++currenthandle,lpNextKey,samDesired);
		*retkey=currenthandle;
		lpNextKey->flags|=REG_OPTION_TAINTED;
		return SHELL_ERROR_SUCCESS;
	}
	split_keypath(lpszSubKey,&wps,&wpc);
	i 	= 0;
	while ((i<wpc) && (wps[i][0]=='\0')) i++;
	lpxkey	= lpNextKey;
	while (wps[i]) {
		lpxkey=lpNextKey->nextsub;
		while (lpxkey) {
			if (!lstrcmpi32W(wps[i],lpxkey->keyname))
				break;
			lpxkey=lpxkey->next;
		}
		if (!lpxkey)
			break;
		i++;
		lpNextKey	= lpxkey;
	}
	if (lpxkey) {
		add_handle(++currenthandle,lpxkey,samDesired);
		lpxkey->flags  |= REG_OPTION_TAINTED;
		*retkey		= currenthandle;
		if (lpDispos)
			*lpDispos	= REG_OPENED_EXISTING_KEY;
		FREE_KEY_PATH;
		return	SHELL_ERROR_SUCCESS;
	}
	/* good. now the hard part */
	while (wps[i]) {
		lplpPrevKey	= &(lpNextKey->nextsub);
		lpxkey		= *lplpPrevKey;
		while (lpxkey) {
			lplpPrevKey	= &(lpxkey->next);
			lpxkey		= *lplpPrevKey;
		}
		*lplpPrevKey=malloc(sizeof(KEYSTRUCT));
		if (!*lplpPrevKey) {
			FREE_KEY_PATH;
			return SHELL_ERROR_OUTOFMEMORY;
		}
		memset(*lplpPrevKey,'\0',sizeof(KEYSTRUCT));
		(*lplpPrevKey)->keyname	= strdupW(wps[i]);
		(*lplpPrevKey)->next	= NULL;
		(*lplpPrevKey)->nextsub	= NULL;
		(*lplpPrevKey)->values	= NULL;
		(*lplpPrevKey)->nrofvalues = 0;
		(*lplpPrevKey)->flags 	= REG_OPTION_TAINTED;
		if (lpszClass)
			(*lplpPrevKey)->class = strdupW(lpszClass);
		else
			(*lplpPrevKey)->class = NULL;
		lpNextKey	= *lplpPrevKey;
		i++;
	}
	add_handle(++currenthandle,lpNextKey,samDesired);

	/*FIXME: flag handling correct? */
	lpNextKey->flags= fdwOptions |REG_OPTION_TAINTED;
	if (lpszClass)
		lpNextKey->class = strdupW(lpszClass);
	else
		lpNextKey->class = NULL;
	*retkey		= currenthandle;
	if (lpDispos)
		*lpDispos	= REG_CREATED_NEW_KEY;
	FREE_KEY_PATH;
	return SHELL_ERROR_SUCCESS;
}

/* RegCreateKeyW		[ADVAPI32.132] */
DWORD WINAPI RegCreateKey32W(
	HKEY	hkey,
	LPCWSTR	lpszSubKey,
	LPHKEY	retkey
) {
	DWORD	junk,ret;

	dprintf_reg(stddeb,"RegCreateKey32W(%lx,%s,%p)\n",
		(LONG)hkey,W2C(lpszSubKey,0),retkey
	);
	ret=RegCreateKeyEx32W(
		hkey,		/* key handle */
		lpszSubKey,	/* subkey name */
		0,		/* reserved = 0 */
		NULL,		/* lpszClass? FIXME: ? */
		REG_OPTION_NON_VOLATILE,	/* options */
		KEY_ALL_ACCESS,	/* desired access attribs */
		NULL,		/* lpsecurity attributes */
		retkey,		/* lpretkey */
		&junk		/* disposition value */
	);
	return	ret;
}

/* RegCreateKeyExA		[ADVAPI32.130] */
DWORD WINAPI RegCreateKeyEx32A(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	DWORD	dwReserved,
	LPSTR	lpszClass,
	DWORD	fdwOptions,
	REGSAM	samDesired,
	LPSECURITY_ATTRIBUTES lpSecAttribs,
	LPHKEY	retkey,
	LPDWORD	lpDispos
) {
	LPWSTR	lpszSubKeyW,lpszClassW;
	DWORD	ret;

	dprintf_reg(stddeb,"RegCreateKeyEx32A(%lx,%s,%ld,%s,%lx,%lx,%p,%p,%p)\n",
		(LONG)hkey,
		lpszSubKey,
		dwReserved,
		lpszClass,
		fdwOptions,
		samDesired,
		lpSecAttribs,
		retkey,
		lpDispos
	);
	if (lpszSubKey)
		lpszSubKeyW=strdupA2W(lpszSubKey);
	else
		lpszSubKeyW=NULL;
	if (lpszClass)
		lpszClassW=strdupA2W(lpszClass);
	else
		lpszClassW=NULL;
	ret=RegCreateKeyEx32W(
		hkey,
		lpszSubKeyW,
		dwReserved,
		lpszClassW,
		fdwOptions,
		samDesired,
		lpSecAttribs,
		retkey,
		lpDispos
	);
	if (lpszSubKeyW)
		free(lpszSubKeyW);
	if (lpszClassW)
		free(lpszClassW);
	return ret;
}

/* RegCreateKeyA		[ADVAPI32.129] */
DWORD WINAPI RegCreateKey32A(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	LPHKEY	retkey
) {
	DWORD	junk;

	dprintf_reg(stddeb,"RegCreateKey32A(%lx,%s,%p)\n",
		(LONG)hkey,lpszSubKey,retkey
	);
	return	RegCreateKeyEx32A(
		hkey,		/* key handle */
		lpszSubKey,	/* subkey name */
		0,		/* reserved = 0 */
		NULL,		/* lpszClass? FIXME: ? */
		REG_OPTION_NON_VOLATILE,/* options */
		KEY_ALL_ACCESS,	/* desired access attribs */
		NULL,		/* lpsecurity attributes */
		retkey,		/* lpretkey */
		&junk		/* disposition value */
	);
}

/* RegCreateKey			[SHELL.2] [KERNEL.218] */
DWORD WINAPI RegCreateKey16(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	LPHKEY	retkey
) {
	dprintf_reg(stddeb,"RegCreateKey16(%lx,%s,%p)\n",
		(LONG)hkey,lpszSubKey,retkey
	);
	return RegCreateKey32A(hkey,lpszSubKey,retkey);
}

/* 
 * Query Value Functions
 * Win32 differs between keynames and valuenames. 
 * multiple values may belong to one key, the special value
 * with name NULL is the default value used by the win31
 * compat functions.
 *
 * Callpath:
 * RegQueryValue16 -> RegQueryValue32A -> RegQueryValueEx32A \
 *                                          RegQueryValue32W -> RegQueryValueEx32W
 */

/* RegQueryValueExW		[ADVAPI32.158] */
DWORD WINAPI RegQueryValueEx32W(
	HKEY	hkey,
	LPWSTR	lpszValueName,
	LPDWORD	lpdwReserved,
	LPDWORD	lpdwType,
	LPBYTE	lpbData,
	LPDWORD	lpcbData
) {
	LPKEYSTRUCT	lpkey;
	int		i;

	dprintf_reg(stddeb,"RegQueryValueEx32W(%x,%s,%p,%p,%p,%ld)\n",
		hkey,W2C(lpszValueName,0),lpdwReserved,lpdwType,lpbData,
		lpcbData?*lpcbData:0
	);

	lpkey	= lookup_hkey(hkey);
	if (!lpkey)
		return SHELL_ERROR_BADKEY;
	if (lpszValueName && !*lpszValueName)
		lpszValueName = NULL;
	if (lpszValueName==NULL) {
		for (i=0;i<lpkey->nrofvalues;i++)
			if (lpkey->values[i].name==NULL)
				break;
	} else {
		for (i=0;i<lpkey->nrofvalues;i++)
			if (	lpkey->values[i].name &&
				!lstrcmpi32W(lpszValueName,lpkey->values[i].name)
			)
				break;
	}
	if (i==lpkey->nrofvalues) {
		if (lpszValueName==NULL) {
			if (lpbData) {
				*(WCHAR*)lpbData = 0;
				*lpcbData	= 2;
			}
			if (lpdwType)
				*lpdwType	= REG_SZ;
			return SHELL_ERROR_SUCCESS;
		}
		return SHELL_ERROR_BADKEY;/*FIXME: correct return? */
	}
	if (lpdwType)
		*lpdwType	= lpkey->values[i].type;
	if (lpbData==NULL) {
		if (lpcbData==NULL)
			return SHELL_ERROR_SUCCESS;
		*lpcbData	= lpkey->values[i].len;
		return SHELL_ERROR_SUCCESS;
	}
	if (*lpcbData<lpkey->values[i].len) {
		*(WCHAR*)lpbData
			= 0;
		*lpcbData	= lpkey->values[i].len;
		return ERROR_MORE_DATA;
	}
	memcpy(lpbData,lpkey->values[i].data,lpkey->values[i].len);
	*lpcbData	= lpkey->values[i].len;
	return SHELL_ERROR_SUCCESS;
}

/* RegQueryValueW		[ADVAPI32.159] */
DWORD WINAPI RegQueryValue32W(
	HKEY	hkey,
	LPWSTR	lpszSubKey,
	LPWSTR	lpszData,
	LPDWORD	lpcbData
) {
	HKEY	xhkey;
	DWORD	ret,lpdwType;

	dprintf_reg(stddeb,"RegQueryValue32W(%x,%s,%p,%ld)\n->",
		hkey,W2C(lpszSubKey,0),lpszData,
		lpcbData?*lpcbData:0
	);

	/* only open subkey, if we really do descend */
	if (lpszSubKey && *lpszSubKey) {
		ret	= RegOpenKey32W(hkey,lpszSubKey,&xhkey);
		if (ret!=ERROR_SUCCESS)
			return ret;
	} else
		xhkey	= hkey;

	lpdwType	= REG_SZ;
	ret	= RegQueryValueEx32W(
		xhkey,
		NULL,		/* varname NULL -> compat */
		NULL,		/* lpdwReserved, must be NULL */
		&lpdwType,
		(LPBYTE)lpszData,
		lpcbData
	);
	if (xhkey!=hkey)
		RegCloseKey(xhkey);
	return ret;
}

/* RegQueryValueExA		[ADVAPI32.157] */
DWORD WINAPI RegQueryValueEx32A(
	HKEY	hkey,
	LPSTR	lpszValueName,
	LPDWORD	lpdwReserved,
	LPDWORD	lpdwType,
	LPBYTE	lpbData,
	LPDWORD	lpcbData
) {
	LPWSTR	lpszValueNameW;
	LPBYTE	buf;
	DWORD	ret,myxlen;
	DWORD	*mylen;
	DWORD	type;

	dprintf_reg(stddeb,"RegQueryValueEx32A(%x,%s,%p,%p,%p,%ld)\n->",
		hkey,lpszValueName,lpdwReserved,lpdwType,lpbData,
		lpcbData?*lpcbData:0
	);
	if (lpszValueName)
		lpszValueNameW=strdupA2W(lpszValueName);
	else 
		lpszValueNameW=NULL;

	if (lpdwType)
		type=*lpdwType;

	if (lpbData) {
		myxlen  = 0;
		mylen	= &myxlen;
		buf	= xmalloc(4);
		ret=RegQueryValueEx32W(
			hkey,
			lpszValueNameW,
			lpdwReserved,
			&type,
			buf,
			mylen
		);
		free(buf);
		if (ret==ERROR_MORE_DATA) {
			buf	= (LPBYTE)xmalloc(*mylen);
		} else {
			buf	= (LPBYTE)xmalloc(2*(*lpcbData));
			myxlen  = 2*(*lpcbData);
		}
	} else {
		buf=NULL;
		if (lpcbData) {
			myxlen	= *lpcbData*2;
			mylen	= &myxlen;
		} else
			mylen	= NULL;
	}
	ret=RegQueryValueEx32W(
		hkey,
		lpszValueNameW,
		lpdwReserved,
		&type,
		buf,
		mylen
	);
	if (lpdwType) 
		*lpdwType=type;
	if (ret==ERROR_SUCCESS) {
		if (buf) {
			if (UNICONVMASK & (1<<(type))) {
				/* convert UNICODE to ASCII */
				lstrcpyWtoA(lpbData,(LPWSTR)buf);
				*lpcbData	= myxlen/2;
			} else {
				if (myxlen>*lpcbData)
					ret	= ERROR_MORE_DATA;
				else
					memcpy(lpbData,buf,myxlen);

				*lpcbData	= myxlen;
			}
		} else {
			if ((UNICONVMASK & (1<<(type))) && lpcbData)
				*lpcbData	= myxlen/2;
		}
	} else {
		if ((UNICONVMASK & (1<<(type))) && lpcbData)
			*lpcbData	= myxlen/2;
	}
	if (buf)
		free(buf);
	return ret;
}

/* RegQueryValueEx		[KERNEL.225] */
DWORD WINAPI RegQueryValueEx16(
	HKEY	hkey,
	LPSTR	lpszValueName,
	LPDWORD	lpdwReserved,
	LPDWORD	lpdwType,
	LPBYTE	lpbData,
	LPDWORD	lpcbData
) {
	dprintf_reg(stddeb,"RegQueryValueEx16(%x,%s,%p,%p,%p,%ld)\n",
		hkey,lpszValueName,lpdwReserved,lpdwType,lpbData,
		lpcbData?*lpcbData:0
	);
	return RegQueryValueEx32A(
		hkey,
		lpszValueName,
		lpdwReserved,
		lpdwType,
		lpbData,
		lpcbData
	);
}

/* RegQueryValueA		[ADVAPI32.156] */
DWORD WINAPI RegQueryValue32A(
	HKEY	hkey,
	LPSTR	lpszSubKey,
	LPSTR	lpszData,
	LPDWORD	lpcbData
) {
	HKEY	xhkey;
	DWORD	ret,lpdwType;

	dprintf_reg(stddeb,"RegQueryValue32A(%x,%s,%p,%ld)\n",
		hkey,lpszSubKey,lpszData,
		lpcbData?*lpcbData:0
	);

	/* only open subkey, if we really do descend */
	if (lpszSubKey && *lpszSubKey) {
		ret	= RegOpenKey16(hkey,lpszSubKey,&xhkey);
		if (ret!=ERROR_SUCCESS)
			return ret;
	} else
		xhkey	= hkey;

	lpdwType	= REG_SZ;
	ret	= RegQueryValueEx32A(
		xhkey,
		NULL,		/* lpszValueName NULL -> compat */
		NULL,		/* lpdwReserved, must be NULL */
		&lpdwType,
		(LPBYTE)lpszData,
		lpcbData
	);
	if (xhkey!=hkey)
		RegCloseKey(xhkey);
	return ret;
}

/* RegQueryValue		[SHELL.6] [KERNEL.224] */
DWORD WINAPI RegQueryValue16(
	HKEY	hkey,
	LPSTR	lpszSubKey,
	LPSTR	lpszData,
	LPDWORD	lpcbData
) {
	dprintf_reg(stddeb,"RegQueryValue16(%x,%s,%p,%ld)\n",
		hkey,lpszSubKey,lpszData,lpcbData?*lpcbData:0
	);
	/* HACK: the 16bit RegQueryValue doesn't handle selectorblocks
	 *       anyway, so we just mask out the high 16 bit.
	 *       (this (not so much incidently;) hopefully fixes Aldus FH4)
	 */
	if (lpcbData)
		*lpcbData &= 0xFFFF;
	return RegQueryValue32A(hkey,lpszSubKey,lpszData,lpcbData);
}

/*
 * Setting values of Registry keys
 *
 * Callpath:
 * RegSetValue16 -> RegSetValue32A -> RegSetValueEx32A \
 *                                    RegSetValue32W   -> RegSetValueEx32W
 */

/* RegSetValueExW		[ADVAPI32.170] */
DWORD WINAPI RegSetValueEx32W(
	HKEY	hkey,
	LPWSTR	lpszValueName,
	DWORD	dwReserved,
	DWORD	dwType,
	LPBYTE	lpbData,
	DWORD	cbData
) {
	LPKEYSTRUCT	lpkey;
	int		i;

	dprintf_reg(stddeb,"RegSetValueEx32W(%x,%s,%ld,%ld,%p,%ld)\n",
		hkey,W2C(lpszValueName,0),dwReserved,dwType,lpbData,cbData
	);
	/* we no longer care about the lpbData type here... */
	lpkey	= lookup_hkey(hkey);
	if (!lpkey)
		return SHELL_ERROR_BADKEY;

	lpkey->flags |= REG_OPTION_TAINTED;

	if (lpszValueName==NULL) {
		for (i=0;i<lpkey->nrofvalues;i++)
			if (lpkey->values[i].name==NULL)
				break;
	} else {
		for (i=0;i<lpkey->nrofvalues;i++)
			if (	lpkey->values[i].name &&
				!lstrcmpi32W(lpszValueName,lpkey->values[i].name)
			)
				break;
	}
	if (i==lpkey->nrofvalues) {
		lpkey->values = (LPKEYVALUE)xrealloc(
					lpkey->values,
					(lpkey->nrofvalues+1)*sizeof(KEYVALUE)
				);
		lpkey->nrofvalues++;
		memset(lpkey->values+i,'\0',sizeof(KEYVALUE));
	}
	if (lpkey->values[i].name==NULL)
		if (lpszValueName)
			lpkey->values[i].name = strdupW(lpszValueName);
		else
			lpkey->values[i].name = NULL;
	lpkey->values[i].len	= cbData;
	lpkey->values[i].type	= dwType;
	if (lpkey->values[i].data !=NULL)
		free(lpkey->values[i].data);
	lpkey->values[i].data	= (LPBYTE)xmalloc(cbData);
	lpkey->values[i].lastmodified = time(NULL);
	memcpy(lpkey->values[i].data,lpbData,cbData);
	return SHELL_ERROR_SUCCESS;
}

/* RegSetValueExA		[ADVAPI32.169] */
DWORD WINAPI RegSetValueEx32A(
	HKEY	hkey,
	LPSTR	lpszValueName,
	DWORD	dwReserved,
	DWORD	dwType,
	LPBYTE	lpbData,
	DWORD	cbData
) {
	LPBYTE	buf;
	LPWSTR	lpszValueNameW;
	DWORD	ret;

	dprintf_reg(stddeb,"RegSetValueEx32A(%x,%s,%ld,%ld,%p,%ld)\n->",
		hkey,lpszValueName,dwReserved,dwType,lpbData,cbData
	);
	if ((1<<dwType) & UNICONVMASK) {
		buf=(LPBYTE)strdupA2W(lpbData);
		cbData=2*strlen(lpbData)+2;
	} else
		buf=lpbData;
	if (lpszValueName)
		lpszValueNameW = strdupA2W(lpszValueName);
	else
		lpszValueNameW = NULL;
	ret=RegSetValueEx32W(hkey,lpszValueNameW,dwReserved,dwType,buf,cbData);
	if (lpszValueNameW)
		free(lpszValueNameW);
	if (buf!=lpbData)
		free(buf);
	return ret;
}

/* RegSetValueEx		[KERNEL.226] */
DWORD WINAPI RegSetValueEx16(
	HKEY	hkey,
	LPSTR	lpszValueName,
	DWORD	dwReserved,
	DWORD	dwType,
	LPBYTE	lpbData,
	DWORD	cbData
) {
	dprintf_reg(stddeb,"RegSetValueEx16(%x,%s,%ld,%ld,%p,%ld)\n->",
		hkey,lpszValueName,dwReserved,dwType,lpbData,cbData
	);
	return RegSetValueEx32A(hkey,lpszValueName,dwReserved,dwType,lpbData,cbData);
}

/* RegSetValueW			[ADVAPI32.171] */
DWORD WINAPI RegSetValue32W(
	HKEY	hkey,
	LPCWSTR	lpszSubKey,
	DWORD	dwType,
	LPCWSTR	lpszData,
	DWORD	cbData
) {
	HKEY	xhkey;
	DWORD	ret;

	dprintf_reg(stddeb,"RegSetValue32W(%x,%s,%ld,%s,%ld)\n->",
		hkey,W2C(lpszSubKey,0),dwType,W2C(lpszData,0),cbData
	);
	if (lpszSubKey && *lpszSubKey) {
		ret=RegCreateKey32W(hkey,lpszSubKey,&xhkey);
		if (ret!=ERROR_SUCCESS)
			return ret;
	} else
		xhkey=hkey;
	if (dwType!=REG_SZ) {
		fprintf(stddeb,"RegSetValueX called with dwType=%ld!\n",dwType);
		dwType=REG_SZ;
	}
	if (cbData!=2*lstrlen32W(lpszData)+2) {
		dprintf_reg(stddeb,"RegSetValueX called with len=%ld != strlen(%s)+1=%d!\n",
			cbData,W2C(lpszData,0),2*lstrlen32W(lpszData)+2
		);
		cbData=2*lstrlen32W(lpszData)+2;
	}
	ret=RegSetValueEx32W(xhkey,NULL,0,dwType,(LPBYTE)lpszData,cbData);
	if (hkey!=xhkey)
		RegCloseKey(xhkey);
	return ret;

}
/* RegSetValueA			[ADVAPI32.168] */
DWORD WINAPI RegSetValue32A(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	DWORD	dwType,
	LPCSTR	lpszData,
	DWORD	cbData
) {
	DWORD	ret;
	HKEY	xhkey;

	dprintf_reg(stddeb,"RegSetValue32A(%x,%s,%ld,%s,%ld)\n->",
		hkey,lpszSubKey,dwType,lpszData,cbData
	);
	if (lpszSubKey && *lpszSubKey) {
		ret=RegCreateKey16(hkey,lpszSubKey,&xhkey);
		if (ret!=ERROR_SUCCESS)
			return ret;
	} else
		xhkey=hkey;

	if (dwType!=REG_SZ) {
		dprintf_reg(stddeb,"RegSetValueA called with dwType=%ld!\n",dwType);
		dwType=REG_SZ;
	}
	if (cbData!=strlen(lpszData)+1)
		cbData=strlen(lpszData)+1;
	ret=RegSetValueEx32A(xhkey,NULL,0,dwType,(LPBYTE)lpszData,cbData);
	if (xhkey!=hkey)
		RegCloseKey(xhkey);
	return ret;
}

/* RegSetValue			[KERNEL.221] [SHELL.5] */
DWORD WINAPI RegSetValue16(
	HKEY	hkey,
	LPCSTR	lpszSubKey,
	DWORD	dwType,
	LPCSTR	lpszData,
	DWORD	cbData
) {
	DWORD	ret;
	dprintf_reg(stddeb,"RegSetValue16(%x,%s,%ld,%s,%ld)\n->",
		hkey,lpszSubKey,dwType,lpszData,cbData
	);
	ret=RegSetValue32A(hkey,lpszSubKey,dwType,lpszData,cbData);
	return ret;
}

/* 
 * Key Enumeration
 *
 * Callpath:
 * RegEnumKey16 -> RegEnumKey32A -> RegEnumKeyEx32A \
 *                              RegEnumKey32W   -> RegEnumKeyEx32W
 */

/* RegEnumKeyExW		[ADVAPI32.139] */
DWORD WINAPI RegEnumKeyEx32W(
	HKEY	hkey,
	DWORD	iSubkey,
	LPWSTR	lpszName,
	LPDWORD	lpcchName,
	LPDWORD	lpdwReserved,
	LPWSTR	lpszClass,
	LPDWORD	lpcchClass,
	FILETIME	*ft
) {
	LPKEYSTRUCT	lpkey,lpxkey;

	dprintf_reg(stddeb,"RegEnumKeyEx32W(%x,%ld,%p,%ld,%p,%p,%p,%p)\n",
		hkey,iSubkey,lpszName,*lpcchName,lpdwReserved,lpszClass,lpcchClass,ft
	);
	lpkey=lookup_hkey(hkey);
	if (!lpkey)
		return SHELL_ERROR_BADKEY;
	if (!lpkey->nextsub)
		return ERROR_NO_MORE_ITEMS;
	lpxkey=lpkey->nextsub;
	while (iSubkey && lpxkey) {
		iSubkey--;
		lpxkey=lpxkey->next;
	}
	if (iSubkey || !lpxkey)
		return ERROR_NO_MORE_ITEMS;
	if (2*lstrlen32W(lpxkey->keyname)+2>*lpcchName)
		return ERROR_MORE_DATA;
	memcpy(lpszName,lpxkey->keyname,lstrlen32W(lpxkey->keyname)*2+2);
	if (lpszClass) {
		/* what should we write into it? */
		*lpszClass		= 0;
		*lpcchClass	= 2;
	}
	return ERROR_SUCCESS;

}

/* RegEnumKeyW			[ADVAPI32.140] */
DWORD WINAPI RegEnumKey32W(
	HKEY	hkey,
	DWORD	iSubkey,
	LPWSTR	lpszName,
	DWORD	lpcchName
) {
	FILETIME	ft;

	dprintf_reg(stddeb,"RegEnumKey32W(%x,%ld,%p,%ld)\n->",
		hkey,iSubkey,lpszName,lpcchName
	);
	return RegEnumKeyEx32W(hkey,iSubkey,lpszName,&lpcchName,NULL,NULL,NULL,&ft);
}
/* RegEnumKeyExA		[ADVAPI32.138] */
DWORD WINAPI RegEnumKeyEx32A(
	HKEY	hkey,
	DWORD	iSubkey,
	LPSTR	lpszName,
	LPDWORD	lpcchName,
	LPDWORD	lpdwReserved,
	LPSTR	lpszClass,
	LPDWORD	lpcchClass,
	FILETIME	*ft
) {
	DWORD	ret,lpcchNameW,lpcchClassW;
	LPWSTR	lpszNameW,lpszClassW;


	dprintf_reg(stddeb,"RegEnumKeyEx32A(%x,%ld,%p,%ld,%p,%p,%p,%p)\n->",
		hkey,iSubkey,lpszName,*lpcchName,lpdwReserved,lpszClass,lpcchClass,ft
	);
	if (lpszName) {
		lpszNameW	= (LPWSTR)xmalloc(*lpcchName*2);
		lpcchNameW	= *lpcchName*2;
	} else {
		lpszNameW	= NULL;
		lpcchNameW 	= 0;
	}
	if (lpszClass) {
		lpszClassW		= (LPWSTR)xmalloc(*lpcchClass*2);
		lpcchClassW	= *lpcchClass*2;
	} else {
		lpszClassW	=0;
		lpcchClassW=0;
	}
	ret=RegEnumKeyEx32W(
		hkey,
		iSubkey,
		lpszNameW,
		&lpcchNameW,
		lpdwReserved,
		lpszClassW,
		&lpcchClassW,
		ft
	);
	if (ret==ERROR_SUCCESS) {
		lstrcpyWtoA(lpszName,lpszNameW);
		*lpcchName=strlen(lpszName);
		if (lpszClassW) {
			lstrcpyWtoA(lpszClass,lpszClassW);
			*lpcchClass=strlen(lpszClass);
		}
	}
	if (lpszNameW)
		free(lpszNameW);
	if (lpszClassW)
		free(lpszClassW);
	return ret;
}

/* RegEnumKeyA			[ADVAPI32.137] */
DWORD WINAPI RegEnumKey32A(
	HKEY	hkey,
	DWORD	iSubkey,
	LPSTR	lpszName,
	DWORD	lpcchName
) {
	FILETIME	ft;

	dprintf_reg(stddeb,"RegEnumKey32A(%x,%ld,%p,%ld)\n->",
		hkey,iSubkey,lpszName,lpcchName
	);
	return	RegEnumKeyEx32A(
		hkey,
		iSubkey,
		lpszName,
		&lpcchName,
		NULL,
		NULL,
		NULL,
		&ft
	);
}

/* RegEnumKey			[SHELL.7] [KERNEL.216] */
DWORD WINAPI RegEnumKey16(
	HKEY	hkey,
	DWORD	iSubkey,
	LPSTR	lpszName,
	DWORD	lpcchName
) {
	dprintf_reg(stddeb,"RegEnumKey16(%x,%ld,%p,%ld)\n->",
		hkey,iSubkey,lpszName,lpcchName
	);
	return RegEnumKey32A(hkey,iSubkey,lpszName,lpcchName);
}

/* 
 * Enumerate Registry Values
 *
 * Callpath:
 * RegEnumValue16 -> RegEnumValue32A -> RegEnumValue32W
 */

/* RegEnumValueW		[ADVAPI32.142] */
DWORD WINAPI RegEnumValue32W(
	HKEY	hkey,
	DWORD	iValue,
	LPWSTR	lpszValue,
	LPDWORD	lpcchValue,
	LPDWORD	lpdReserved,
	LPDWORD	lpdwType,
	LPBYTE	lpbData,
	LPDWORD	lpcbData
) {
	LPKEYSTRUCT	lpkey;
	LPKEYVALUE	val;

	dprintf_reg(stddeb,"RegEnumValue32W(%x,%ld,%p,%p,%p,%p,%p,%p)\n",
		hkey,iValue,lpszValue,lpcchValue,lpdReserved,lpdwType,lpbData,lpcbData
	);
	lpkey = lookup_hkey(hkey);
	if (!lpkey)
		return SHELL_ERROR_BADKEY;
	if (lpkey->nrofvalues<=iValue)
		return ERROR_NO_MORE_ITEMS;
	val	= lpkey->values+iValue;

	if (val->name) {
		if (lstrlen32W(val->name)*2+2>*lpcchValue) {
			*lpcchValue = lstrlen32W(val->name)*2+2;
			return ERROR_MORE_DATA;
		}
		memcpy(lpszValue,val->name,2*lstrlen32W(val->name)+2);
		*lpcchValue=lstrlen32W(val->name)*2+2;
	} else {
		*lpszValue	= 0;
		*lpcchValue	= 0;
	}
	if (lpdwType)
		*lpdwType=val->type;
	if (lpbData) {
		if (val->len>*lpcbData)
			return ERROR_MORE_DATA;
		memcpy(lpbData,val->data,val->len);
		*lpcbData = val->len;
	}
	return SHELL_ERROR_SUCCESS;
}

/* RegEnumValueA		[ADVAPI32.141] */
DWORD WINAPI RegEnumValue32A(
	HKEY	hkey,
	DWORD	iValue,
	LPSTR	lpszValue,
	LPDWORD	lpcchValue,
	LPDWORD	lpdReserved,
	LPDWORD	lpdwType,
	LPBYTE	lpbData,
	LPDWORD	lpcbData
) {
	LPWSTR	lpszValueW;
	LPBYTE	lpbDataW;
	DWORD	ret,lpcbDataW;

	dprintf_reg(stddeb,"RegEnumValue32A(%x,%ld,%p,%p,%p,%p,%p,%p)\n",
		hkey,iValue,lpszValue,lpcchValue,lpdReserved,lpdwType,lpbData,lpcbData
	);

	lpszValueW = (LPWSTR)xmalloc(*lpcchValue*2);
	if (lpbData) {
		lpbDataW = (LPBYTE)xmalloc(*lpcbData*2);
		lpcbDataW = *lpcbData*2;
	} else
		lpbDataW = NULL;
	ret=RegEnumValue32W(
		hkey,
		iValue,
		lpszValueW,
		lpcchValue,
		lpdReserved,
		lpdwType,
		lpbDataW,
		&lpcbDataW
	);

	if (ret==ERROR_SUCCESS) {
		lstrcpyWtoA(lpszValue,lpszValueW);
		if (lpbData) {
			if ((1<<*lpdwType) & UNICONVMASK) {
				lstrcpyWtoA(lpbData,(LPWSTR)lpbDataW);
			} else {
				if (lpcbDataW > *lpcbData)
					ret	= ERROR_MORE_DATA;
				else
					memcpy(lpbData,lpbDataW,lpcbDataW);
			}
			*lpcbData = lpcbDataW;
		}
	}
	if (lpbDataW)
		free(lpbDataW);
	if (lpszValueW)
		free(lpszValueW);
	return ret;
}

/* RegEnumValue			[KERNEL.223] */
DWORD WINAPI RegEnumValue16(
	HKEY	hkey,
	DWORD	iValue,
	LPSTR	lpszValue,
	LPDWORD	lpcchValue,
	LPDWORD	lpdReserved,
	LPDWORD	lpdwType,
	LPBYTE	lpbData,
	LPDWORD	lpcbData
) {
	dprintf_reg(stddeb,"RegEnumValue(%x,%ld,%p,%p,%p,%p,%p,%p)\n",
		hkey,iValue,lpszValue,lpcchValue,lpdReserved,lpdwType,lpbData,lpcbData
	);
	return RegEnumValue32A(
		hkey,
		iValue,
		lpszValue,
		lpcchValue,
		lpdReserved,
		lpdwType,
		lpbData,
		lpcbData
	);
}

/* 
 *  Close registry key
 */
/* RegCloseKey			[SHELL.3] [KERNEL.220] [ADVAPI32.126] */
DWORD WINAPI RegCloseKey(HKEY hkey) {
	dprintf_reg(stddeb,"RegCloseKey(%x)\n",hkey);
	remove_handle(hkey);
	return ERROR_SUCCESS;
}
/* 
 * Delete registry key
 *
 * Callpath:
 * RegDeleteKey16 -> RegDeleteKey32A -> RegDeleteKey32W
 */
/* RegDeleteKeyW		[ADVAPI32.134] */
DWORD WINAPI RegDeleteKey32W(HKEY hkey,LPWSTR lpszSubKey) {
	LPKEYSTRUCT	*lplpPrevKey,lpNextKey,lpxkey;
	LPWSTR		*wps;
	int		wpc,i;

	dprintf_reg(stddeb,"RegDeleteKey32W(%x,%s)\n",
		hkey,W2C(lpszSubKey,0)
	);
	lpNextKey	= lookup_hkey(hkey);
	if (!lpNextKey) {
		dprintf_reg (stddeb, "  Badkey[1].\n");
		return SHELL_ERROR_BADKEY;
	}
	/* we need to know the previous key in the hier. */
	if (!lpszSubKey || !*lpszSubKey) {
		dprintf_reg (stddeb, "  Badkey[2].\n");
		return SHELL_ERROR_BADKEY;
	}
	split_keypath(lpszSubKey,&wps,&wpc);
	i 	= 0;
	lpxkey	= lpNextKey;
	while (i<wpc-1) {
		lpxkey=lpNextKey->nextsub;
		while (lpxkey) {
			dprintf_reg (stddeb, "  Scanning [%s]\n",
				     W2C (lpxkey->keyname, 0));
			if (!lstrcmpi32W(wps[i],lpxkey->keyname))
				break;
			lpxkey=lpxkey->next;
		}
		if (!lpxkey) {
			FREE_KEY_PATH;
			dprintf_reg (stddeb, "  Not found.\n");
			/* not found is success */
			return SHELL_ERROR_SUCCESS;
		}
		i++;
		lpNextKey	= lpxkey;
	}
	lpxkey	= lpNextKey->nextsub;
	lplpPrevKey = &(lpNextKey->nextsub);
	while (lpxkey) {
		dprintf_reg (stddeb, "  Scanning [%s]\n",
			     W2C (lpxkey->keyname, 0));
		if (!lstrcmpi32W(wps[i],lpxkey->keyname))
			break;
		lplpPrevKey	= &(lpxkey->next);
		lpxkey		= lpxkey->next;
	}
	if (!lpxkey) {
		FREE_KEY_PATH;
		dprintf_reg (stddeb, "  Not found.\n");
		return SHELL_ERROR_BADKEY;
		return SHELL_ERROR_SUCCESS;
	}
	if (lpxkey->nextsub) {
		FREE_KEY_PATH;
		dprintf_reg (stddeb, "  Not empty.\n");
		return SHELL_ERROR_CANTWRITE;
	}
	*lplpPrevKey	= lpxkey->next;
	free(lpxkey->keyname);
	if (lpxkey->class)
		free(lpxkey->class);
	if (lpxkey->values)
		free(lpxkey->values);
	free(lpxkey);
	FREE_KEY_PATH;
	dprintf_reg (stddeb, "  Done.\n");
	return	SHELL_ERROR_SUCCESS;
}

/* RegDeleteKeyA		[ADVAPI32.133] */
DWORD WINAPI RegDeleteKey32A(HKEY hkey,LPCSTR lpszSubKey) {
	LPWSTR	lpszSubKeyW;
	DWORD	ret;

	dprintf_reg(stddeb,"RegDeleteKey32A(%x,%s)\n",
		hkey,lpszSubKey
	);
	lpszSubKeyW=HEAP_strdupAtoW(GetProcessHeap(),0,lpszSubKey);
	ret=RegDeleteKey32W(hkey,lpszSubKeyW);
	HeapFree(GetProcessHeap(),0,lpszSubKeyW);
	return ret;
}

/* RegDeleteKey			[SHELL.4] [KERNEL.219] */
DWORD WINAPI RegDeleteKey16(HKEY hkey,LPCSTR lpszSubKey) {
	dprintf_reg(stddeb,"RegDeleteKey16(%x,%s)\n",
		hkey,lpszSubKey
	);
	return RegDeleteKey32A(hkey,lpszSubKey);
}

/* 
 * Delete registry value
 *
 * Callpath:
 * RegDeleteValue16 -> RegDeleteValue32A -> RegDeleteValue32W
 */
/* RegDeleteValueW		[ADVAPI32.136] */
DWORD WINAPI RegDeleteValue32W(HKEY hkey,LPWSTR lpszValue)
{
	DWORD		i;
	LPKEYSTRUCT	lpkey;
	LPKEYVALUE	val;

	dprintf_reg(stddeb,"RegDeleteValue32W(%x,%s)\n",
		hkey,W2C(lpszValue,0)
	);
	lpkey=lookup_hkey(hkey);
	if (!lpkey)
		return SHELL_ERROR_BADKEY;
	if (lpszValue) {
		for (i=0;i<lpkey->nrofvalues;i++)
			if (	lpkey->values[i].name &&
				!lstrcmpi32W(lpkey->values[i].name,lpszValue)
			)
				break;
	} else {
		for (i=0;i<lpkey->nrofvalues;i++)
			if (lpkey->values[i].name==NULL)
				break;
	}
	if (i==lpkey->nrofvalues)
		return SHELL_ERROR_BADKEY;/*FIXME: correct errorcode? */
	val	= lpkey->values+i;
	if (val->name) free(val->name);
	if (val->data) free(val->data);
	memcpy(	
		lpkey->values+i,
		lpkey->values+i+1,
		sizeof(KEYVALUE)*(lpkey->nrofvalues-i-1)
	);
	lpkey->values	= (LPKEYVALUE)xrealloc(
				lpkey->values,
				(lpkey->nrofvalues-1)*sizeof(KEYVALUE)
			);
	lpkey->nrofvalues--;
	return SHELL_ERROR_SUCCESS;
}

/* RegDeleteValueA		[ADVAPI32.135] */
DWORD WINAPI RegDeleteValue32A(HKEY hkey,LPSTR lpszValue)
{
	LPWSTR	lpszValueW;
	DWORD	ret;

	dprintf_reg( stddeb, "RegDeleteValue32A(%x,%s)\n", hkey,lpszValue );
        lpszValueW=HEAP_strdupAtoW(GetProcessHeap(),0,lpszValue);
	ret=RegDeleteValue32W(hkey,lpszValueW);
        HeapFree(GetProcessHeap(),0,lpszValueW);
	return ret;
}

/* RegDeleteValue		[KERNEL.222] */
DWORD WINAPI RegDeleteValue16(HKEY hkey,LPSTR lpszValue)
{
	dprintf_reg( stddeb,"RegDeleteValue16(%x,%s)\n", hkey,lpszValue );
	return RegDeleteValue32A(hkey,lpszValue);
}

/* RegFlushKey			[ADVAPI32.143] [KERNEL.227] */
DWORD WINAPI RegFlushKey(HKEY hkey)
{
	dprintf_reg(stddeb,"RegFlushKey(%x), STUB.\n",hkey);
	return SHELL_ERROR_SUCCESS;
}

/* FIXME: lpcchXXXX ... is this counting in WCHARS or in BYTEs ?? */

/* RegQueryInfoKeyW		[ADVAPI32.153] */
DWORD WINAPI RegQueryInfoKey32W(
	HKEY	hkey,
	LPWSTR	lpszClass,
	LPDWORD	lpcchClass,
	LPDWORD	lpdwReserved,
	LPDWORD	lpcSubKeys,
	LPDWORD	lpcchMaxSubkey,
	LPDWORD	lpcchMaxClass,
	LPDWORD	lpcValues,
	LPDWORD	lpcchMaxValueName,
	LPDWORD	lpccbMaxValueData,
	LPDWORD	lpcbSecurityDescriptor,
	FILETIME	*ft
) {
	LPKEYSTRUCT	lpkey,lpxkey;
	int		nrofkeys,maxsubkey,maxclass,maxvalues,maxvname,maxvdata;
	int		i;

	dprintf_reg(stddeb,"RegQueryInfoKey32W(%x,......)\n",hkey);
	lpkey=lookup_hkey(hkey);
	if (!lpkey)
		return SHELL_ERROR_BADKEY;
	if (lpszClass) {
		if (lpkey->class) {
			if (lstrlen32W(lpkey->class)*2+2>*lpcchClass) {
				*lpcchClass=lstrlen32W(lpkey->class)*2;
				return ERROR_MORE_DATA;
			}
			*lpcchClass=lstrlen32W(lpkey->class)*2;
			memcpy(lpszClass,lpkey->class,lstrlen32W(lpkey->class));
		} else {
			*lpszClass	= 0;
			*lpcchClass	= 0;
		}
	} else {
		if (lpcchClass)
			*lpcchClass	= lstrlen32W(lpkey->class)*2;
	}
	lpxkey=lpkey->nextsub;
	nrofkeys=maxsubkey=maxclass=maxvalues=maxvname=maxvdata=0;
	while (lpxkey) {
		nrofkeys++;
		if (lstrlen32W(lpxkey->keyname)>maxsubkey)
			maxsubkey=lstrlen32W(lpxkey->keyname);
		if (lpxkey->class && lstrlen32W(lpxkey->class)>maxclass)
			maxclass=lstrlen32W(lpxkey->class);
		if (lpxkey->nrofvalues>maxvalues)
			maxvalues=lpxkey->nrofvalues;
		for (i=0;i<lpxkey->nrofvalues;i++) {
			LPKEYVALUE	val=lpxkey->values+i;

			if (val->name && lstrlen32W(val->name)>maxvname)
				maxvname=lstrlen32W(val->name);
			if (val->len>maxvdata)
				maxvdata=val->len;
		}
		lpxkey=lpxkey->next;
	}
	if (!maxclass) maxclass	= 1;
	if (!maxvname) maxvname	= 1;
	if (lpcSubKeys)
		*lpcSubKeys	= nrofkeys;
	if (lpcchMaxSubkey)
		*lpcchMaxSubkey	= maxsubkey*2;
	if (lpcchMaxClass)
		*lpcchMaxClass	= maxclass*2;
	if (lpcValues)
		*lpcValues	= maxvalues;
	if (lpcchMaxValueName)
		*lpcchMaxValueName= maxvname;
	if (lpccbMaxValueData)
		*lpccbMaxValueData= maxvdata;
	return SHELL_ERROR_SUCCESS;
}

/* RegQueryInfoKeyA		[ADVAPI32.152] */
DWORD WINAPI RegQueryInfoKey32A(
	HKEY	hkey,
	LPSTR	lpszClass,
	LPDWORD	lpcchClass,
	LPDWORD	lpdwReserved,
	LPDWORD	lpcSubKeys,
	LPDWORD	lpcchMaxSubkey,
	LPDWORD	lpcchMaxClass,
	LPDWORD	lpcValues,
	LPDWORD	lpcchMaxValueName,
	LPDWORD	lpccbMaxValueData,
	LPDWORD	lpcbSecurityDescriptor,
	FILETIME	*ft
) {
	LPWSTR		lpszClassW;
	DWORD		ret;

	dprintf_reg(stddeb,"RegQueryInfoKey32A(%x,......)\n",hkey);
	if (lpszClass) {
		*lpcchClass*= 2;
		lpszClassW  = (LPWSTR)xmalloc(*lpcchClass);

	} else
		lpszClassW  = NULL;
	ret=RegQueryInfoKey32W(
		hkey,
		lpszClassW,
		lpcchClass,
		lpdwReserved,
		lpcSubKeys,
		lpcchMaxSubkey,
		lpcchMaxClass,
		lpcValues,
		lpcchMaxValueName,
		lpccbMaxValueData,
		lpcbSecurityDescriptor,
		ft
	);
	if (ret==ERROR_SUCCESS && lpszClass)
		lstrcpyWtoA(lpszClass,lpszClassW);
	if (lpcchClass)
		*lpcchClass/=2;
	if (lpcchMaxSubkey)
		*lpcchMaxSubkey/=2;
	if (lpcchMaxClass)
		*lpcchMaxClass/=2;
	if (lpcchMaxValueName)
		*lpcchMaxValueName/=2;
	if (lpszClassW)
		free(lpszClassW);
	return ret;
}
/* RegConnectRegistryA		[ADVAPI32.127] */
DWORD WINAPI RegConnectRegistry32A(LPCSTR machine,HKEY hkey,LPHKEY reskey)
{
	fprintf(stderr,"RegConnectRegistry32A(%s,%08x,%p), STUB.\n",
		machine,hkey,reskey
	);
	return ERROR_FILE_NOT_FOUND; /* FIXME */
}
