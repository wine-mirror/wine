/*
 * 	Registry Functions
 *
 * Copyright 1996 Marcus Meissner
 * Copyright 1998 Matthew Becker
 *
 * December 21, 1997 - Kevin Cozens
 * Fixed bugs in the _w95_loadreg() function. Added extra information
 * regarding the format of the Windows '95 registry files.
 *
 * NOTES
 *    When changing this file, please re-run the regtest program to ensure
 *    the conditions are handled properly.
 *
 * TODO
 *    Security access
 *    Option handling
 *    Time for RegEnumKey*, RegQueryInfoKey*
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <assert.h>
#include <time.h>
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wine/winestring.h"
#include "winerror.h"
#include "file.h"
#include "heap.h"
#include "debug.h"
#include "xmalloc.h"
#include "winreg.h"
#include "winversion.h"

static void REGISTRY_Init(void);
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
    DWORD    len;           /* length of data in BYTEs */
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


static struct openhandle {
	LPKEYSTRUCT	lpkey;
	HKEY		hkey;
	REGSAM		accessmask;
}  *openhandles=NULL;
static int	nrofopenhandles=0;
/* Starts after 1 because 0,1 are reserved for Win16 */
/* Note: Should always be even, as Win95 ADVAPI32.DLL reserves odd
         HKEYs for remote registry access */
static int	currenthandle=2;


/*
 * QUESTION
 *   Are these doing the same as HEAP_strdupAtoW and HEAP_strdupWtoA?
 *   If so, can we remove them?
 * ANSWER
 *   No, the memory handling functions are called very often in here, 
 *   just replacing them by HeapAlloc(SystemHeap,...) makes registry
 *   loading 100 times slower. -MM
 */
static LPWSTR strdupA2W(LPCSTR src)
{
    if(src) {
	LPWSTR dest=xmalloc(2*strlen(src)+2);
	lstrcpyAtoW(dest,src);
	return dest;
    }
    return NULL;
}

static LPWSTR strdupW(LPCWSTR a) {
	LPWSTR	b;
	int	len;

    if(a) {
	len=sizeof(WCHAR)*(lstrlen32W(a)+1);
	b=(LPWSTR)xmalloc(len);
	memcpy(b,a,len);
	return b;
    }
    return NULL;
}

LPWSTR strcvtA2W(LPCSTR src, int nchars)

{
   LPWSTR dest = xmalloc (2 * nchars + 2);

   lstrcpynAtoW(dest,src,nchars+1);
   dest[nchars] = 0;
   return dest;
}
/*
 * we need to convert A to W with '\0' in strings (MULTI_SZ) 
 */

static LPWSTR  lmemcpynAtoW( LPWSTR dst, LPCSTR src, INT32 n )
{	LPWSTR p = dst;

	TRACE(reg,"\"%s\" %i\n",src, n);

	while (n-- > 0) *p++ = (WCHAR)(unsigned char)*src++;

	return dst;
}
static LPSTR lmemcpynWtoA( LPSTR dst, LPCWSTR src, INT32 n )
{	LPSTR p = dst;

	TRACE(string,"L\"%s\" %i\n",debugstr_w(src), n);

	while (n-- > 0) *p++ = (CHAR)*src++;

	return dst;
}

static void debug_print_value (LPBYTE lpbData, DWORD type, DWORD len)
{	if (TRACE_ON(reg) && lpbData) 
	{ switch(type)
	  { case REG_SZ:
                TRACE(reg," Data(sz)=%s\n",debugstr_w((LPCWSTR)lpbData));
                break;

	    case REG_DWORD:
                TRACE(reg," Data(dword)=0x%08lx\n",(DWORD)*lpbData);
                break;
		
            case REG_MULTI_SZ:
	    	{ int i;
		  LPCWSTR ptr = (LPCWSTR)lpbData;
		  for (i=0;ptr[0];i++)
		  { TRACE(reg, " MULTI_SZ(%i=%s)\n", i, debugstr_w(ptr));
		    ptr += lstrlen32W(ptr)+1;
		  }
		}
		break;

            case REG_BINARY:
                { char szTemp[100];			/* 3*32 + 3 + 1 */
		  int i;
		  for ( i = 0; i < len ; i++)		  
		  { sprintf (&(szTemp[i*3]),"%02x ", lpbData[i]);
		    if (i>=31)
		    { sprintf (&(szTemp[i*3+3]),"...");
		      break;
		    }
		  }
		  TRACE(reg," Data(raw)=(%s)\n", szTemp);                  
		}
                break;
		
            default:
                FIXME(reg, " Unknown data type %ld\n", type);
	  } /* switch */
	} /* if */
}



/******************************************************************************
 * is_standard_hkey [Internal]
 * Determines if a hkey is a standard key
 */
static BOOL32 is_standard_hkey( HKEY hkey )
{
    switch(hkey) {
        case 0x00000000:
        case 0x00000001:
        case HKEY_CLASSES_ROOT:
        case HKEY_CURRENT_CONFIG:
        case HKEY_CURRENT_USER:
        case HKEY_LOCAL_MACHINE:
        case HKEY_USERS:
        case HKEY_PERFORMANCE_DATA:
        case HKEY_DYN_DATA:
            return TRUE;
        default:
            return FALSE;
    }
}

/******************************************************************************
 * add_handle [Internal]
 */
static void add_handle( HKEY hkey, LPKEYSTRUCT lpkey, REGSAM accessmask )
{
    int i;

    TRACE(reg,"(0x%x,%p,0x%lx)\n",hkey,lpkey,accessmask);
    /* Check for duplicates */
    for (i=0;i<nrofopenhandles;i++) {
        if (openhandles[i].lpkey==lpkey) {
            /* This is not really an error - the user is allowed to create
               two (or more) handles to the same key */
            /*WARN(reg, "Adding key %p twice\n",lpkey);*/
        }
        if (openhandles[i].hkey==hkey) {
            WARN(reg, "Adding handle %x twice\n",hkey);
        }
    }
    openhandles=xrealloc( openhandles, 
                          sizeof(struct openhandle)*(nrofopenhandles+1));

    openhandles[i].lpkey = lpkey;
    openhandles[i].hkey = hkey;
    openhandles[i].accessmask = accessmask;
    nrofopenhandles++;
}


/******************************************************************************
 * get_handle [Internal]
 *
 * RETURNS
 *    Success: Pointer to key
 *    Failure: NULL
 */
static LPKEYSTRUCT get_handle( HKEY hkey )
{
    int i;

    for (i=0; i<nrofopenhandles; i++)
        if (openhandles[i].hkey == hkey)
            return openhandles[i].lpkey;
    WARN(reg, "Could not find handle 0x%x\n",hkey);
    return NULL;
}


/******************************************************************************
 * remove_handle [Internal]
 *
 * PARAMS
 *    hkey [I] Handle of key to remove
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: ERROR_INVALID_HANDLE
 */
static DWORD remove_handle( HKEY hkey )
{
    int i;

    for (i=0;i<nrofopenhandles;i++)
        if (openhandles[i].hkey==hkey)
            break;

    if (i == nrofopenhandles) {
        WARN(reg, "Could not find handle 0x%x\n",hkey);
        return ERROR_INVALID_HANDLE;
    }

    memcpy( openhandles+i,
            openhandles+i+1,
            sizeof(struct openhandle)*(nrofopenhandles-i-1)
    );
    openhandles=xrealloc(openhandles,sizeof(struct openhandle)*(nrofopenhandles-1));
    nrofopenhandles--;
    return ERROR_SUCCESS;
}

/******************************************************************************
 * lookup_hkey [Internal]
 * 
 * Just as the name says. Creates the root keys on demand, so we can call the
 * Reg* functions at any time.
 *
 * RETURNS
 *    Success: Pointer to key structure
 *    Failure: NULL
 */
#define ADD_ROOT_KEY(xx) \
	xx = (LPKEYSTRUCT)xmalloc(sizeof(KEYSTRUCT));\
	memset(xx,'\0',sizeof(KEYSTRUCT));\
	xx->keyname= strdupA2W("<should_not_appear_anywhere>");

static LPKEYSTRUCT lookup_hkey( HKEY hkey )
{
	switch (hkey) {
	/* 0 and 1 are valid rootkeys in win16 shell.dll and are used by
	 * some programs. Do not remove those cases. -MM
	 */
    	case 0x00000000:
	case 0x00000001:
	case HKEY_CLASSES_ROOT: {
		if (!key_classes_root) {
			HKEY	cl_r_hkey;

			/* calls lookup_hkey recursively, TWICE */
			if (RegCreateKey16(HKEY_LOCAL_MACHINE,"SOFTWARE\\Classes",&cl_r_hkey)!=ERROR_SUCCESS) {
				ERR(reg,"Could not create HKLM\\SOFTWARE\\Classes. This is impossible.\n");
				exit(1);
			}
			key_classes_root = lookup_hkey(cl_r_hkey);
		}
		return key_classes_root;
	}
	case HKEY_CURRENT_USER:
		if (!key_current_user) {
			HKEY	c_u_hkey;
			struct	passwd	*pwd;

			pwd=getpwuid(getuid());
			/* calls lookup_hkey recursively, TWICE */
			if (pwd && pwd->pw_name) {
				if (RegCreateKey16(HKEY_USERS,pwd->pw_name,&c_u_hkey)!=ERROR_SUCCESS) {
					ERR(reg,"Could not create HU\\%s. This is impossible.\n",pwd->pw_name);
					exit(1);
				}
				key_current_user = lookup_hkey(c_u_hkey);
			} else {
				/* nothing found, use standalone */
				ADD_ROOT_KEY(key_current_user);
			}
		}
		return key_current_user;
	case HKEY_LOCAL_MACHINE:
		if (!key_local_machine) {
			ADD_ROOT_KEY(key_local_machine);
			REGISTRY_Init();
		}
		return key_local_machine;
	case HKEY_USERS:
		if (!key_users) {
			ADD_ROOT_KEY(key_users);
		}
		return key_users;
	case HKEY_PERFORMANCE_DATA:
		if (!key_performance_data) {
			ADD_ROOT_KEY(key_performance_data);
		}
		return key_performance_data;
	case HKEY_DYN_DATA:
		if (!key_dyn_data) {
			ADD_ROOT_KEY(key_dyn_data);
		}
		return key_dyn_data;
	case HKEY_CURRENT_CONFIG:
		if (!key_current_config) {
			ADD_ROOT_KEY(key_current_config);
		}
		return key_current_config;
	default:
		return get_handle(hkey);
	}
	/*NOTREACHED*/
}
#undef ADD_ROOT_KEY
/* so we don't accidently access them ... */
#define key_current_config NULL NULL
#define key_current_user NULL NULL
#define key_users NULL NULL
#define key_local_machine NULL NULL
#define key_classes_root NULL NULL
#define key_dyn_data NULL NULL
#define key_performance_data NULL NULL

/******************************************************************************
 * split_keypath [Internal]
 * splits the unicode string 'wp' into an array of strings.
 * the array is allocated by this function. 
 * Free the array using FREE_KEY_PATH
 *
 * PARAMS
 *    wp  [I] String to split up
 *    wpv [O] Array of pointers to strings
 *    wpc [O] Number of components
 */
static void split_keypath( LPCWSTR wp, LPWSTR **wpv, int *wpc)
{
    int	i,j,len;
    LPWSTR ws;

    TRACE(reg,"(%s,%p,%p)\n",debugstr_w(wp),wpv,wpc);

    ws	= HEAP_strdupW( SystemHeap, 0, wp );

    /* We know we have at least one substring */
    *wpc = 1;

    /* Replace each backslash with NULL, and increment the count */
    for (i=0;ws[i];i++) {
        if (ws[i]=='\\') {
            ws[i]=0;
            (*wpc)++;
        }
    }

    len = i;

    /* Allocate the space for the array of pointers, leaving room for the
       NULL at the end */
    *wpv = (LPWSTR*)HeapAlloc( SystemHeap, 0, sizeof(LPWSTR)*(*wpc+2));
    (*wpv)[0]= ws;

    /* Assign each pointer to the appropriate character in the string */
    j = 1;
    for (i=1;i<len;i++)
        if (ws[i-1]==0) {
            (*wpv)[j++]=ws+i;
            /* TRACE(reg, " Subitem %d = %s\n",j-1,debugstr_w((*wpv)[j-1])); */
        }

    (*wpv)[j]=NULL;
}
#define FREE_KEY_PATH HeapFree(SystemHeap,0,wps[0]);HeapFree(SystemHeap,0,wps);




/******************************************************************************
 * REGISTRY_Init [Internal]
 * Registry initialisation, allocates some default keys. 
 */
static void REGISTRY_Init(void) {
	HKEY	hkey;
	char	buf[200];

	TRACE(reg,"(void)\n");

	RegCreateKey16(HKEY_DYN_DATA,"PerfStats\\StatData",&hkey);
	RegCloseKey(hkey);

        /* This was an Open, but since it is called before the real registries
           are loaded, it was changed to a Create - MTB 980507*/
	RegCreateKey16(HKEY_LOCAL_MACHINE,"HARDWARE\\DESCRIPTION\\System",&hkey);
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
		RegCreateKey16(HKEY_LOCAL_MACHINE,"System\\CurrentControlSet\\Control\\ComputerName\\ComputerName",&hkey);
		RegSetValueEx16(hkey,"ComputerName",0,REG_SZ,buf,strlen(buf)+1);
		RegCloseKey(hkey);
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

/******************************************************************************
 * _save_check_tainted [Internal]
 */
static int _save_check_tainted( LPKEYSTRUCT lpkey )
{
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

/******************************************************************************
 * _save_USTRING [Internal]
 */
static void _save_USTRING( FILE *F, LPWSTR wstr, int escapeeq )
{
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

/******************************************************************************
 * _savesubkey [Internal]
 *
 * NOTES
 *  REG_MULTI_SZ is handled as binary (like in win95) (js)
 */
static int _savesubkey( FILE *F, LPKEYSTRUCT lpkey, int level, int all )
{
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
				if ( val->type == REG_SZ || val->type == REG_EXPAND_SZ )
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


/******************************************************************************
 * _savesubreg [Internal]
 */
static int _savesubreg( FILE *F, LPKEYSTRUCT lpkey, int all )
{
	fprintf(F,"WINE REGISTRY Version %d\n",REGISTRY_SAVE_VERSION);
	_save_check_tainted(lpkey->nextsub);
	return _savesubkey(F,lpkey->nextsub,0,all);
}


/******************************************************************************
 * _savereg [Internal]
 */
static BOOL32 _savereg( LPKEYSTRUCT lpkey, char *fn, int all )
{
	FILE	*F;

	F=fopen(fn,"w");
	if (F==NULL) {
		WARN(reg,"Couldn't open %s for writing: %s\n",
			fn,strerror(errno)
		);
		return FALSE;
	}
	if (!_savesubreg(F,lpkey,all)) {
		fclose(F);
		unlink(fn);
		WARN(reg,"Failed to save keys, perhaps no more diskspace for %s?\n",fn);
		return FALSE;
	}
	fclose(F);
        return TRUE;
}


/******************************************************************************
 * SHELL_SaveRegistry [Internal]
 */
void SHELL_SaveRegistry( void )
{
	char	*fn;
	struct	passwd	*pwd;
	char	buf[4];
	HKEY	hkey;
	int	all;

    TRACE(reg,"(void)\n");

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
		if (_savereg(lookup_hkey(HKEY_CURRENT_USER),tmp,all)) {
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
		if (_savereg(lookup_hkey(HKEY_LOCAL_MACHINE),tmp,all)) {
			if (-1==rename(tmp,fn)) {
				perror("rename tmp registry");
				unlink(tmp);
			}
		}
		free(tmp);
		free(fn);
	} else
		WARN(reg,"Failed to get homedirectory of UID %d.\n",getuid());
}


/************************ LOAD Registry Function ****************************/



/******************************************************************************
 * _find_or_add_key [Internal]
 */
static LPKEYSTRUCT _find_or_add_key( LPKEYSTRUCT lpkey, LPWSTR keyname )
{
	LPKEYSTRUCT	lpxkey,*lplpkey;

	if ((!keyname) || (keyname[0]==0)) {
		free(keyname);
		return lpkey;
	}
	lplpkey= &(lpkey->nextsub);
	lpxkey	= *lplpkey;
	while (lpxkey) {
		if (	(lpxkey->keyname[0]==keyname[0]) && 
			!lstrcmpi32W(lpxkey->keyname,keyname)
		)
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

/******************************************************************************
 * _find_or_add_value [Internal]
 */
static void _find_or_add_value( LPKEYSTRUCT lpkey, LPWSTR name, DWORD type,
                                LPBYTE data, DWORD len, DWORD lastmodified )
{
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
				val->name[0]==name[0] &&
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


/******************************************************************************
 * _wine_read_line [Internal]
 *
 * reads a line including dynamically enlarging the readbuffer and throwing
 * away comments
 */
static int _wine_read_line( FILE *F, char **buf, int *len )
{
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


/******************************************************************************
 * _wine_read_USTRING [Internal]
 *
 * converts a char* into a UNICODE string (up to a special char)
 * and returns the position exactly after that string
 */
static char* _wine_read_USTRING( char *buf, LPWSTR *str )
{
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
			if (!*s) {
				/* Dangling \ ... may only happen if a registry
				 * write was short. FIXME: What do to?
				 */
				 break;
			}
			if (*s=='\\') {
				*ws++='\\';
				s++;
				continue;
			}
			if (*s!='u') {
				WARN(reg,"Non unicode escape sequence \\%c found in |%s|\n",*s,buf);
				*ws++='\\';
				*ws++=*s++;
			} else {
				char	xbuf[5];
				int	wc;

				s++;
				memcpy(xbuf,s,4);xbuf[4]='\0';
				if (!sscanf(xbuf,"%x",&wc))
					WARN(reg,"Strange escape sequence %s found in |%s|\n",xbuf,buf);
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


/******************************************************************************
 * _wine_loadsubkey [Internal]
 *
 * NOTES
 *    It seems like this is returning a boolean.  Should it?
 *
 * RETURNS
 *    Success: 1
 *    Failure: 0
 */
static int _wine_loadsubkey( FILE *F, LPKEYSTRUCT lpkey, int level, char **buf,
                             int *buflen, DWORD optflag )
{
	LPKEYSTRUCT	lpxkey;
	int		i;
	char		*s;
	LPWSTR		name;

    TRACE(reg,"(%p,%p,%d,%s,%d,%lx)\n", F, lpkey, level, debugstr_a(*buf),
          *buflen, optflag);

    lpkey->flags |= optflag;

    /* Good.  We already got a line here ... so parse it */
    lpxkey = NULL;
    while (1) {
        i=0;s=*buf;
        while (*s=='\t') {
            s++;
            i++;
        }
        if (i>level) {
            if (lpxkey==NULL) {
                WARN(reg,"Got a subhierarchy without resp. key?\n");
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
					WARN(reg,"Unexpected character: %c\n",*s);
					break;
				}
				s++;
				if (2!=sscanf(s,"%d,%d,",&type,&lastmodified)) {
					WARN(reg,"Haven't understood possible value in |%s|, skipping.\n",*buf);
					break;
				}
				/* skip the 2 , */
				s=strchr(s,',');s++;
				s=strchr(s,',');s++;
				if (type == REG_SZ || type == REG_EXPAND_SZ) {
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
							data[i]=(*s-'a'+'\xa')<<4;
						if (*s>='A' && *s<='F')
							data[i]=(*s-'A'+'\xa')<<4;
						s++;
						if (*s>='0' && *s<='9')
							data[i]|=*s-'0';
						if (*s>='a' && *s<='f')
							data[i]|=*s-'a'+'\xa';
						if (*s>='A' && *s<='F')
							data[i]|=*s-'A'+'\xa';
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


/******************************************************************************
 * _wine_loadsubreg [Internal]
 */
static int _wine_loadsubreg( FILE *F, LPKEYSTRUCT lpkey, DWORD optflag )
{
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
		TRACE(reg,"Old format (%d) registry found, ignoring it. (buf was %s).\n",ver,buf);
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


/******************************************************************************
 * _wine_loadreg [Internal]
 */
static void _wine_loadreg( LPKEYSTRUCT lpkey, char *fn, DWORD optflag )
{
    FILE *F;

    TRACE(reg,"(%p,%s,%lx)\n",lpkey,debugstr_a(fn),optflag);

    F = fopen(fn,"rb");
    if (F==NULL) {
        WARN(reg,"Couldn't open %s for reading: %s\n",fn,strerror(errno) );
        return;
    }
    if (!_wine_loadsubreg(F,lpkey,optflag)) {
        fclose(F);
        unlink(fn);
        return;
    }
    fclose(F);
}


/******************************************************************************
 * _copy_registry [Internal]
 */
static void _copy_registry( LPKEYSTRUCT from, LPKEYSTRUCT to )
{
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
			data=(LPBYTE)xmalloc(valfrom->len);
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
 *      8 : DWORD	offset to the root record
 * 	C..0x1B: 	? (fill in)
 *      0x20 ... offset_of_RGDB_part: Disk Key Entry structures
 *
 *   Disk Key Entry Structure:
 *	00: DWORD	- Free entry indicator(?)
 *	04: DWORD	- Hash = sum of bytes of keyname
 *	08: DWORD	- Root key indicator? unknown, but usually 0xFFFFFFFF on win95 systems
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
 * The High significant part of the structure seems to equal the number
 * of the RGDB section. The low significant part is a unique ID within
 * that RGDB section
 *
 * There are two minor corrections to the position of that structure.
 * 1. If the address is xxx014 or xxx018 it will be aligned to xxx01c AND 
 *    the DKE reread from there.
 * 2. If the address is xxxFFx it will be aligned to (xxx+1)000.
 * CPS - I have not experienced the above phenomenon in my registry files
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
	struct _w95key		*prevlvl;
	struct _w95key		*nextsub;
	struct _w95key		*next;
};


struct _w95_info {
  char *rgknbuffer;
  int  rgknsize;
  char *rgdbbuffer;
  int  rgdbsize;
  int  depth;
  int  lastmodified;
};


/******************************************************************************
 * _w95_processKey [Internal]
 */
static LPKEYSTRUCT _w95_processKey ( LPKEYSTRUCT lpkey, 
                                   int nrLS, int nrMS, struct _w95_info *info )

{
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

	
	struct	dkh dkh;
	int	bytesread = 0;
	char    *rgdbdata = info->rgdbbuffer;
	int     nbytes = info->rgdbsize;
	char    *curdata = rgdbdata;
	char    *end = rgdbdata + nbytes;
	int     off_next_rgdb;
	char    *next = rgdbdata;
	int     nrgdb, i;
	LPKEYSTRUCT	lpxkey;
	
	do {
	  curdata = next;
	  if (strncmp(curdata, "RGDB", 4)) return (NULL);
	    
	  memcpy(&off_next_rgdb,curdata+4,4);
	  next = curdata + off_next_rgdb;
	  nrgdb = (int) *((short *)curdata + 7);

	} while (nrgdb != nrMS && (next < end));

	/* curdata now points to the start of the right RGDB section */
	curdata += 0x20;

#define XREAD(whereto,len) \
	if ((curdata + len) <end) {\
		memcpy(whereto,curdata,len);\
		curdata+=len;\
		bytesread+=len;\
	}

	while (curdata < next) {
	  struct	dkh *xdkh = (struct dkh*)curdata;

	  bytesread += sizeof(dkh); /* FIXME... nextkeyoff? */
	  if (xdkh->nrLS == nrLS) {
	  	memcpy(&dkh,xdkh,sizeof(dkh));
	  	curdata += sizeof(dkh);
	  	break;
	  }
	  curdata += xdkh->nextkeyoff;
	};

	if (dkh.nrLS != nrLS) return (NULL);

	if (nrgdb != dkh.nrMS)
	  return (NULL);

        assert((dkh.keynamelen<2) || curdata[0]);
	lpxkey=_find_or_add_key(lpkey,strcvtA2W(curdata, dkh.keynamelen));
	curdata += dkh.keynamelen;

	for (i=0;i< dkh.values; i++) {
	  struct dkv dkv;
	  LPBYTE data;
	  int len;
	  LPWSTR name;

	  XREAD(&dkv,sizeof(dkv));

	  name = strcvtA2W(curdata, dkv.valnamelen);
	  curdata += dkv.valnamelen;

	  if ((1 << dkv.type) & UNICONVMASK) {
	    data = (LPBYTE) strcvtA2W(curdata, dkv.valdatalen);
	    len = 2*(dkv.valdatalen + 1);
	  } else {
	    /* I don't think we want to NULL terminate all data */
	    data = xmalloc(dkv.valdatalen);
	    memcpy (data, curdata, dkv.valdatalen);
	    len = dkv.valdatalen;
	  }

	  curdata += dkv.valdatalen;
	  
	  _find_or_add_value(
			     lpxkey,
			     name,
			     dkv.type,
			     data,
			     len,
			     info->lastmodified
			     );
	}
	return (lpxkey);
}

/******************************************************************************
 * _w95_walkrgkn [Internal]
 */
static void _w95_walkrgkn( LPKEYSTRUCT prevkey, char *off, 
                           struct _w95_info *info )

{
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
  } *dke = (struct dke *)off;
  LPKEYSTRUCT  lpxkey;

  if (dke == NULL) {
    dke = (struct dke *) ((char *)info->rgknbuffer);
  }

  lpxkey = _w95_processKey(prevkey, dke->nrLS, dke->nrMS, info);
  /* XXX <-- This is a hack*/
  if (!lpxkey) {
    lpxkey = prevkey;
  }

  if (dke->nextsub != -1 && 
      ((dke->nextsub - 0x20) < info->rgknsize) 
      && (dke->nextsub > 0x20)) {
    
    _w95_walkrgkn(lpxkey, 
		  info->rgknbuffer + dke->nextsub - 0x20, 
		  info);
  }
  
  if (dke->next != -1 && 
      ((dke->next - 0x20) < info->rgknsize) && 
      (dke->next > 0x20)) {
    _w95_walkrgkn(prevkey,  
		  info->rgknbuffer + dke->next - 0x20,
		  info);
  }

  return;
}


/******************************************************************************
 * _w95_loadreg [Internal]
 */
static void _w95_loadreg( char* fn, LPKEYSTRUCT lpkey )
{
	HFILE32		hfd;
	char		magic[5];
	unsigned long	where,version,rgdbsection,end;
	struct          _w95_info info;
	OFSTRUCT	ofs;
	BY_HANDLE_FILE_INFORMATION hfdinfo;

	TRACE(reg,"Loading Win95 registry database '%s'\n",fn);
	hfd=OpenFile32(fn,&ofs,OF_READ);
	if (hfd==HFILE_ERROR32)
		return;
	magic[4]=0;
	if (4!=_lread32(hfd,magic,4))
		return;
	if (strcmp(magic,"CREG")) {
		WARN(reg,"%s is not a w95 registry.\n",fn);
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
		WARN(reg, "second IFF header not RGKN, but %s\n", magic);
		return;
	}

	/* STEP 1: Keylink structures */
	if (-1==_llseek32(hfd,0x40,SEEK_SET))
		return;
	where	= 0x40;
	end	= rgdbsection;

	info.rgknsize = end - where;
	info.rgknbuffer = (char*)xmalloc(info.rgknsize);
	if (info.rgknsize != _lread32(hfd,info.rgknbuffer,info.rgknsize))
		return;

	if (!GetFileInformationByHandle(hfd,&hfdinfo))
		return;

	end = hfdinfo.nFileSizeLow;
	info.lastmodified = DOSFS_FileTimeToUnixTime(&hfdinfo.ftLastWriteTime,NULL);

	if (-1==_llseek32(hfd,rgdbsection,SEEK_SET))
		return;

	info.rgdbbuffer = (char*)xmalloc(end-rgdbsection);
	info.rgdbsize = end - rgdbsection;

	if (info.rgdbsize !=_lread32(hfd,info.rgdbbuffer,info.rgdbsize))
		return;
	_lclose32(hfd);

	_w95_walkrgkn(lpkey, NULL, &info);

	free (info.rgdbbuffer);
	free (info.rgknbuffer);
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
			TRACE(reg,"strange: no directory key name, idx=%04x\n", idx);
		}
		__w31_dumptree(dir->child_idx,txt,tab,head,xlpkey,lastmodified,level+1);
		idx=dir->sibling_idx;
	}
}


/******************************************************************************
 * _w31_loadreg [Internal]
 */
void _w31_loadreg(void) {
	HFILE32			hf;
	struct _w31_header	head;
	struct _w31_tabent	*tab;
	unsigned char		*txt;
	int			len;
	OFSTRUCT		ofs;
	BY_HANDLE_FILE_INFORMATION hfinfo;
	time_t			lastmodified;
	LPKEYSTRUCT		lpkey;

	TRACE(reg,"(void)\n");

	hf = OpenFile32("reg.dat",&ofs,OF_READ);
	if (hf==HFILE_ERROR32)
		return;

	/* read & dump header */
	if (sizeof(head)!=_lread32(hf,&head,sizeof(head))) {
		ERR(reg, "reg.dat is too short.\n");
		_lclose32(hf);
		return;
	}
	if (memcmp(head.cookie, "SHCC3.10", sizeof(head.cookie))!=0) {
		ERR(reg, "reg.dat has bad signature.\n");
		_lclose32(hf);
		return;
	}

	len = head.tabcnt * sizeof(struct _w31_tabent);
	/* read and dump index table */
	tab = xmalloc(len);
	if (len!=_lread32(hf,tab,len)) {
		ERR(reg,"couldn't read %d bytes.\n",len); 
		free(tab);
		_lclose32(hf);
		return;
	}

	/* read text */
	txt = xmalloc(head.textsize);
	if (-1==_llseek32(hf,head.textoff,SEEK_SET)) {
		ERR(reg,"couldn't seek to textblock.\n"); 
		free(tab);
		free(txt);
		_lclose32(hf);
		return;
	}
	if (head.textsize!=_lread32(hf,txt,head.textsize)) {
		ERR(reg,"textblock too short (%d instead of %ld).\n",len,head.textsize); 
		free(tab);
		free(txt);
		_lclose32(hf);
		return;
	}

	if (!GetFileInformationByHandle(hf,&hfinfo)) {
		ERR(reg,"GetFileInformationByHandle failed?.\n"); 
		free(tab);
		free(txt);
		_lclose32(hf);
		return;
	}
	lastmodified = DOSFS_FileTimeToUnixTime(&hfinfo.ftLastWriteTime,NULL);
	lpkey = lookup_hkey(HKEY_CLASSES_ROOT);
	__w31_dumptree(tab[0].w1,txt,tab,&head,lpkey,lastmodified,0);
	free(tab);
	free(txt);
	_lclose32(hf);
	return;
}


/**********************************************************************************
 * SHELL_LoadRegistry [Internal]
 */
void SHELL_LoadRegistry( void )
{
	char	*fn;
	struct	passwd	*pwd;
	LPKEYSTRUCT	lpkey;
	HKEY		hkey;

	TRACE(reg,"(void)\n");

	/* Load windows 3.1 entries */
	_w31_loadreg();
	/* Load windows 95 entries */
	_w95_loadreg("C:\\system.1st",	lookup_hkey(HKEY_LOCAL_MACHINE));
	_w95_loadreg("system.dat",	lookup_hkey(HKEY_LOCAL_MACHINE));
	_w95_loadreg("user.dat",	lookup_hkey(HKEY_USERS));

	/* the global user default is loaded under HKEY_USERS\\.Default */
	RegCreateKey16(HKEY_USERS,".Default",&hkey);
	lpkey = lookup_hkey(hkey);
        if(!lpkey)
            WARN(reg,"Could not create global user default key\n");
	_wine_loadreg(lpkey,SAVE_USERS_DEFAULT,0);

	/* HKEY_USERS\\.Default is copied to HKEY_CURRENT_USER */
	_copy_registry(lpkey,lookup_hkey(HKEY_CURRENT_USER));
	RegCloseKey(hkey);

	/* the global machine defaults */
	_wine_loadreg(lookup_hkey(HKEY_LOCAL_MACHINE),SAVE_LOCAL_MACHINE_DEFAULT,0);

	/* load the user saved registries */

	/* FIXME: use getenv("HOME") or getpwuid(getuid())->pw_dir ?? */

	pwd=getpwuid(getuid());
	if (pwd!=NULL && pwd->pw_dir!=NULL) {
		fn=(char*)xmalloc(strlen(pwd->pw_dir)+strlen(WINE_PREFIX)+strlen(SAVE_CURRENT_USER)+2);
		strcpy(fn,pwd->pw_dir);
		strcat(fn,WINE_PREFIX"/"SAVE_CURRENT_USER);
		_wine_loadreg(lookup_hkey(HKEY_CURRENT_USER),fn,REG_OPTION_TAINTED);
		free(fn);
		fn=(char*)xmalloc(strlen(pwd->pw_dir)+strlen(WINE_PREFIX)+strlen(SAVE_LOCAL_MACHINE)+2);
		strcpy(fn,pwd->pw_dir);
		strcat(fn,WINE_PREFIX"/"SAVE_LOCAL_MACHINE);
		_wine_loadreg(lookup_hkey(HKEY_LOCAL_MACHINE),fn,REG_OPTION_TAINTED);
		free(fn);
	} else
		WARN(reg,"Failed to get homedirectory of UID %d.\n",getuid());
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
 * Callpath:
 * RegOpenKey16 -> RegOpenKey32A -> RegOpenKeyEx32A \
 *                                  RegOpenKey32W   -> RegOpenKeyEx32W 
 */


/******************************************************************************
 * RegOpenKeyEx32W [ADVAPI32.150]
 * Opens the specified key
 *
 * Unlike RegCreateKeyEx, this does not create the key if it does not exist.
 *
 * PARAMS
 *    hkey       [I] Handle of open key
 *    lpszSubKey [I] Name of subkey to open
 *    dwReserved [I] Reserved - must be zero
 *    samDesired [I] Security access mask
 *    retkey     [O] Address of handle of open key
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: Error code
 */
DWORD WINAPI RegOpenKeyEx32W( HKEY hkey, LPCWSTR lpszSubKey, DWORD dwReserved,
                              REGSAM samDesired, LPHKEY retkey )
{
	LPKEYSTRUCT	lpNextKey,lpxkey;
	LPWSTR		*wps;
	int		wpc,i;

    TRACE(reg,"(0x%x,%s,%ld,%lx,%p)\n", hkey,debugstr_w(lpszSubKey),dwReserved,
          samDesired,retkey);

    lpNextKey = lookup_hkey( hkey );
    if (!lpNextKey)
        return ERROR_INVALID_HANDLE;

    if (!lpszSubKey || !*lpszSubKey) {
        /* Either NULL or pointer to empty string, so return a new handle
           to the original hkey */
        currenthandle += 2;
        add_handle(currenthandle,lpNextKey,samDesired);
        *retkey=currenthandle;
        return ERROR_SUCCESS;
    }

    if (lpszSubKey[0] == '\\') {
        WARN(reg,"Subkey %s must not begin with backslash.\n",debugstr_w(lpszSubKey));
        return ERROR_BAD_PATHNAME;
    }

	split_keypath(lpszSubKey,&wps,&wpc);
	i = 0;
	while ((i<wpc) && (wps[i][0]=='\0')) i++;
	lpxkey = lpNextKey;

    while (wps[i]) {
        lpxkey=lpNextKey->nextsub;
        while (lpxkey) {
            if (!lstrcmpi32W(wps[i],lpxkey->keyname)) {
                break;
            }
            lpxkey=lpxkey->next;
        }

        if (!lpxkey) {
            TRACE(reg,"Could not find subkey %s\n",debugstr_w(wps[i]));
            FREE_KEY_PATH;
            return ERROR_FILE_NOT_FOUND;
        }
        i++;
        lpNextKey = lpxkey;
    }

    currenthandle += 2;
    add_handle(currenthandle,lpxkey,samDesired);
    *retkey = currenthandle;
    TRACE(reg,"  Returning %x\n", currenthandle);
    FREE_KEY_PATH;
    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegOpenKeyEx32A [ADVAPI32.149]
 */
DWORD WINAPI RegOpenKeyEx32A( HKEY hkey, LPCSTR lpszSubKey, DWORD dwReserved,
                              REGSAM samDesired, LPHKEY retkey )
{
    LPWSTR lpszSubKeyW = strdupA2W(lpszSubKey);
    DWORD ret;

    TRACE(reg,"(%x,%s,%ld,%lx,%p)\n",hkey,debugstr_a(lpszSubKey),dwReserved,
          samDesired,retkey);
    ret = RegOpenKeyEx32W( hkey, lpszSubKeyW, dwReserved, samDesired, retkey );
    free(lpszSubKeyW);
    return ret;
}


/******************************************************************************
 * RegOpenKey32W [ADVAPI32.151]
 *
 * PARAMS
 *    hkey       [I] Handle of open key
 *    lpszSubKey [I] Address of name of subkey to open
 *    retkey     [O] Address of handle of open key
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: Error code
 */
DWORD WINAPI RegOpenKey32W( HKEY hkey, LPCWSTR lpszSubKey, LPHKEY retkey )
{
    TRACE(reg,"(%x,%s,%p)\n",hkey,debugstr_w(lpszSubKey),retkey);
    return RegOpenKeyEx32W( hkey, lpszSubKey, 0, KEY_ALL_ACCESS, retkey );
}


/******************************************************************************
 * RegOpenKey32A [ADVAPI32.148]
 */
DWORD WINAPI RegOpenKey32A( HKEY hkey, LPCSTR lpszSubKey, LPHKEY retkey )
{
    DWORD ret;
    LPWSTR lpszSubKeyW = strdupA2W(lpszSubKey);
    TRACE(reg,"(%x,%s,%p)\n",hkey,debugstr_a(lpszSubKey),retkey);
    ret =  RegOpenKey32W( hkey, lpszSubKeyW, retkey );
    free(lpszSubKeyW);
    return ret;
}


/******************************************************************************
 * RegOpenKey16 [SHELL.1] [KERNEL.217]
 */
DWORD WINAPI RegOpenKey16( HKEY hkey, LPCSTR lpszSubKey, LPHKEY retkey )
{
    TRACE(reg,"(%x,%s,%p)\n",hkey,debugstr_a(lpszSubKey),retkey);
    return RegOpenKey32A( hkey, lpszSubKey, retkey );
}


/* 
 * Create keys
 * 
 * All those functions convert their respective 
 * arguments and call RegCreateKeyExW at the end.
 *
 * We stay away from the Ex functions as long as possible because there are
 * differences in the return values
 *
 * Callpath:
 *                                      RegCreateKeyEx32A \
 * RegCreateKey16 -> RegCreateKey32A -> RegCreateKey32W   -> RegCreateKeyEx32W
 */


/******************************************************************************
 * RegCreateKeyEx32W [ADVAPI32.131]
 *
 * PARAMS
 *    hkey         [I] Handle of an open key
 *    lpszSubKey   [I] Address of subkey name
 *    dwReserved   [I] Reserved - must be 0
 *    lpszClass    [I] Address of class string
 *    fdwOptions   [I] Special options flag
 *    samDesired   [I] Desired security access
 *    lpSecAttribs [I] Address of key security structure
 *    retkey       [O] Address of buffer for opened handle
 *    lpDispos     [O] Receives REG_CREATED_NEW_KEY or REG_OPENED_EXISTING_KEY
 */
DWORD WINAPI RegCreateKeyEx32W( HKEY hkey, LPCWSTR lpszSubKey, 
                                DWORD dwReserved, LPWSTR lpszClass, 
                                DWORD fdwOptions, REGSAM samDesired,
                                LPSECURITY_ATTRIBUTES lpSecAttribs, 
                                LPHKEY retkey, LPDWORD lpDispos )
{
	LPKEYSTRUCT	*lplpPrevKey,lpNextKey,lpxkey;
	LPWSTR		*wps;
	int		wpc,i;

    TRACE(reg,"(%x,%s,%ld,%s,%lx,%lx,%p,%p,%p)\n", hkey,
		debugstr_w(lpszSubKey), dwReserved, debugstr_w(lpszClass),
		fdwOptions, samDesired, lpSecAttribs, retkey, lpDispos);

    lpNextKey = lookup_hkey(hkey);
    if (!lpNextKey)
        return ERROR_INVALID_HANDLE;

    /* Check for valid options */
    switch(fdwOptions) {
        case REG_OPTION_NON_VOLATILE:
        case REG_OPTION_VOLATILE:
        case REG_OPTION_BACKUP_RESTORE:
            break;
        default:
            return ERROR_INVALID_PARAMETER;
    }

    /* Sam has to be a combination of the following */
    if (!(samDesired & 
          (KEY_ALL_ACCESS | KEY_CREATE_LINK | KEY_CREATE_SUB_KEY | 
           KEY_ENUMERATE_SUB_KEYS | KEY_EXECUTE | KEY_NOTIFY |
           KEY_QUERY_VALUE | KEY_READ | KEY_SET_VALUE | KEY_WRITE)))
        return ERROR_INVALID_PARAMETER;

	if (!lpszSubKey || !*lpszSubKey) {
                currenthandle += 2;
		add_handle(currenthandle,lpNextKey,samDesired);
		*retkey=currenthandle;
                TRACE(reg, "Returning %x\n", currenthandle);
		lpNextKey->flags|=REG_OPTION_TAINTED;
		return ERROR_SUCCESS;
	}

    if (lpszSubKey[0] == '\\') {
        WARN(reg,"Subkey %s must not begin with backslash.\n",debugstr_w(lpszSubKey));
        return ERROR_BAD_PATHNAME;
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
                currenthandle += 2;
		add_handle(currenthandle,lpxkey,samDesired);
		lpxkey->flags  |= REG_OPTION_TAINTED;
		*retkey		= currenthandle;
                TRACE(reg, "Returning %x\n", currenthandle);
		if (lpDispos)
			*lpDispos	= REG_OPENED_EXISTING_KEY;
		FREE_KEY_PATH;
		return ERROR_SUCCESS;
	}

	/* Good.  Now the hard part */
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
                        TRACE(reg, "Returning OUTOFMEMORY\n");
			return ERROR_OUTOFMEMORY;
		}
		memset(*lplpPrevKey,'\0',sizeof(KEYSTRUCT));
                TRACE(reg,"Adding %s\n", debugstr_w(wps[i]));
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
        currenthandle += 2;
	add_handle(currenthandle,lpNextKey,samDesired);

	/*FIXME: flag handling correct? */
	lpNextKey->flags= fdwOptions |REG_OPTION_TAINTED;
	if (lpszClass)
		lpNextKey->class = strdupW(lpszClass);
	else
		lpNextKey->class = NULL;
	*retkey		= currenthandle;
        TRACE(reg, "Returning %x\n", currenthandle);
	if (lpDispos)
		*lpDispos	= REG_CREATED_NEW_KEY;
	FREE_KEY_PATH;
	return ERROR_SUCCESS;
}


/******************************************************************************
 * RegCreateKeyEx32A [ADVAPI32.130]
 */
DWORD WINAPI RegCreateKeyEx32A( HKEY hkey, LPCSTR lpszSubKey, DWORD dwReserved,
                                LPSTR lpszClass, DWORD fdwOptions, 
                                REGSAM samDesired, 
                                LPSECURITY_ATTRIBUTES lpSecAttribs, 
                                LPHKEY retkey, LPDWORD lpDispos )
{
    LPWSTR lpszSubKeyW, lpszClassW;
    DWORD  ret;

    TRACE(reg,"(%x,%s,%ld,%s,%lx,%lx,%p,%p,%p)\n",hkey,debugstr_a(lpszSubKey),
          dwReserved,debugstr_a(lpszClass),fdwOptions,samDesired,lpSecAttribs,
          retkey,lpDispos);

    lpszSubKeyW = lpszSubKey?strdupA2W(lpszSubKey):NULL;
    lpszClassW = lpszClass?strdupA2W(lpszClass):NULL;

    ret = RegCreateKeyEx32W( hkey, lpszSubKeyW, dwReserved, lpszClassW, 
                             fdwOptions, samDesired, lpSecAttribs, retkey, 
                             lpDispos );

    if(lpszSubKeyW) free(lpszSubKeyW);
    if(lpszClassW) free(lpszClassW);

    return ret;
}


/******************************************************************************
 * RegCreateKey32W [ADVAPI32.132]
 */
DWORD WINAPI RegCreateKey32W( HKEY hkey, LPCWSTR lpszSubKey, LPHKEY retkey )
{
    DWORD junk;
    LPKEYSTRUCT	lpNextKey;

    TRACE(reg,"(%x,%s,%p)\n", hkey,debugstr_w(lpszSubKey),retkey);

    /* This check is here because the return value is different than the
       one from the Ex functions */
    lpNextKey = lookup_hkey(hkey);
    if (!lpNextKey)
        return ERROR_BADKEY;

    return RegCreateKeyEx32W( hkey, lpszSubKey, 0, NULL, 
                              REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                              retkey, &junk);
}


/******************************************************************************
 * RegCreateKey32A [ADVAPI32.129]
 */
DWORD WINAPI RegCreateKey32A( HKEY hkey, LPCSTR lpszSubKey, LPHKEY retkey )
{
    DWORD ret;
    LPWSTR lpszSubKeyW;

    TRACE(reg,"(%x,%s,%p)\n",hkey,debugstr_a(lpszSubKey),retkey);
    lpszSubKeyW = lpszSubKey?strdupA2W(lpszSubKey):NULL;
    ret = RegCreateKey32W( hkey, lpszSubKeyW, retkey );
    if(lpszSubKeyW) free(lpszSubKeyW);
    return ret;
}


/******************************************************************************
 * RegCreateKey16 [SHELL.2] [KERNEL.218]
 */
DWORD WINAPI RegCreateKey16( HKEY hkey, LPCSTR lpszSubKey, LPHKEY retkey )
{
    TRACE(reg,"(%x,%s,%p)\n",hkey,debugstr_a(lpszSubKey),retkey);
    return RegCreateKey32A( hkey, lpszSubKey, retkey );
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
 *                                        RegQueryValue32W -> RegQueryValueEx32W
 */


/******************************************************************************
 * RegQueryValueEx32W [ADVAPI32.158]
 * Retrieves type and data for a specified name associated with an open key
 *
 * PARAMS
 *    hkey          [I]   Handle of key to query
 *    lpValueName   [I]   Name of value to query
 *    lpdwReserved  [I]   Reserved - must be NULL
 *    lpdwType      [O]   Address of buffer for value type.  If NULL, the type
 *                        is not required.
 *    lpbData       [O]   Address of data buffer.  If NULL, the actual data is
 *                        not required.
 *    lpcbData      [I/O] Address of data buffer size
 *
 * RETURNS 
 *    ERROR_SUCCESS:   Success
 *    ERROR_MORE_DATA: !!! if the specified buffer is not big enough to hold the data
 * 		       buffer is left untouched. The MS-documentation is wrong (js) !!!
 */
DWORD WINAPI RegQueryValueEx32W( HKEY hkey, LPWSTR lpValueName,
                                 LPDWORD lpdwReserved, LPDWORD lpdwType,
                                 LPBYTE lpbData, LPDWORD lpcbData )
{
	LPKEYSTRUCT	lpkey;
	int		i;
	DWORD		ret;

	TRACE(reg,"(0x%x,%s,%p,%p,%p,%p=%ld)\n", hkey, debugstr_w(lpValueName),
          lpdwReserved, lpdwType, lpbData, lpcbData, lpcbData?*lpcbData:0);

	lpkey = lookup_hkey(hkey);

	if (!lpkey)
	  return ERROR_INVALID_HANDLE;

	if ((lpbData && ! lpcbData) || lpdwReserved)
	  return ERROR_INVALID_PARAMETER;

	/* An empty name string is equivalent to NULL */
	if (lpValueName && !*lpValueName)
	  lpValueName = NULL;

	if (lpValueName==NULL) 
	{ /* Use key's unnamed or default value, if any */
	  for (i=0;i<lpkey->nrofvalues;i++)
	    if (lpkey->values[i].name==NULL)
	      break;
	} 
	else 
	{ /* Search for the key name */
	  for (i=0;i<lpkey->nrofvalues;i++)
	    if ( lpkey->values[i].name && !lstrcmpi32W(lpValueName,lpkey->values[i].name))
	      break;
	}

	if (i==lpkey->nrofvalues) 
	{ TRACE(reg," Key not found\n");
	  if (lpValueName==NULL) 
	  { /* Empty keyname not found */
	    if (lpbData) 
	    { *(WCHAR*)lpbData = 0;
	      *lpcbData	= 2;
	    }
	    if (lpdwType)
	      *lpdwType	= REG_SZ;
	    TRACE(reg, " Returning an empty string\n");
	    return ERROR_SUCCESS;
	  }
	  return ERROR_FILE_NOT_FOUND;
	}

	ret = ERROR_SUCCESS;

	if (lpdwType) 					/* type required ?*/
	  *lpdwType = lpkey->values[i].type;

	if (lpbData)					/* data required ?*/
	{ if (*lpcbData >= lpkey->values[i].len)	/* buffer large enought ?*/
	    memcpy(lpbData,lpkey->values[i].data,lpkey->values[i].len);
	  else
	    ret = ERROR_MORE_DATA;
	}

	if (lpcbData) 					/* size required ?*/
	{ *lpcbData = lpkey->values[i].len;
	}

	debug_print_value ( lpbData, lpkey->values[i].type, lpkey->values[i].len);

	TRACE(reg," (ret=%lx, type=%lx, len=%ld)\n", ret, lpdwType?*lpdwType:0,lpcbData?*lpcbData:0);

	return ret;
}


/******************************************************************************
 * RegQueryValue32W [ADVAPI32.159]
 */
DWORD WINAPI RegQueryValue32W( HKEY hkey, LPCWSTR lpszSubKey, LPWSTR lpszData,
                               LPLONG lpcbData )
{
	HKEY	xhkey;
	DWORD	ret,lpdwType;

    TRACE(reg,"(%x,%s,%p,%ld)\n",hkey,debugstr_w(lpszSubKey),lpszData,
          lpcbData?*lpcbData:0);

    /* Only open subkey, if we really do descend */
    if (lpszSubKey && *lpszSubKey) {
        ret = RegOpenKey32W( hkey, lpszSubKey, &xhkey );
        if (ret != ERROR_SUCCESS) {
            WARN(reg, "Could not open %s\n", debugstr_w(lpszSubKey));
            return ret;
        }
    } else
        xhkey = hkey;

    lpdwType = REG_SZ;
    ret = RegQueryValueEx32W( xhkey, NULL, NULL, &lpdwType, (LPBYTE)lpszData,
                              lpcbData );
    if (xhkey != hkey)
        RegCloseKey(xhkey);
    return ret;
}


/******************************************************************************
 * RegQueryValueEx32A [ADVAPI32.157]
 *
 * NOTES:
 * the documantation is wrong: if the buffer is to small it remains untouched 
 *
 * FIXME: check returnvalue (len) for an empty key
 */
DWORD WINAPI RegQueryValueEx32A( HKEY hkey, LPSTR lpszValueName,
                                 LPDWORD lpdwReserved, LPDWORD lpdwType,
                                 LPBYTE lpbData, LPDWORD lpcbData )
{
	LPWSTR	lpszValueNameW;
	LPBYTE	mybuf = NULL;
	DWORD	ret, mytype, mylen = 0;

	TRACE(reg,"(%x,%s,%p,%p,%p,%p=%ld)\n", hkey,debugstr_a(lpszValueName),
          lpdwReserved,lpdwType,lpbData,lpcbData,lpcbData?*lpcbData:0);

	if (!lpcbData && lpbData)			/* buffer without size is illegal */
	{  return ERROR_INVALID_PARAMETER;
	}

	lpszValueNameW = lpszValueName ? strdupA2W(lpszValueName) : NULL;	
	
	/* get just the type first */
	ret = RegQueryValueEx32W( hkey, lpszValueNameW, lpdwReserved, &mytype, NULL, NULL );
	
	if ( ret != ERROR_SUCCESS )			/* failed ?? */
	{ if(lpszValueNameW) free(lpszValueNameW);
	  return ret;
	}
	
	if (lpcbData)					/* at least length requested? */
	{ if (UNICONVMASK & (1<<(mytype)))		/* string requested? */
	  { if (lpbData )				/* value requested? */
	    { mylen = 2*( *lpcbData );
	      mybuf = (LPBYTE)xmalloc( mylen );
	    }

	    ret = RegQueryValueEx32W( hkey, lpszValueNameW, lpdwReserved, lpdwType, mybuf, &mylen );

	    if (ret == ERROR_SUCCESS )
	    { if ( lpbData )
	      { lmemcpynWtoA(lpbData, (LPWSTR)mybuf, mylen/2);
	      }
	    }

	    *lpcbData = mylen/2;			/* size is in byte! */
	  }
	  else						/* no strings, call it straight */
	  { ret = RegQueryValueEx32W( hkey, lpszValueNameW, lpdwReserved, lpdwType, lpbData, lpcbData );
	  }
	}
	
	if (lpdwType)					/* type when requested */
	{ *lpdwType = mytype;
	}

	TRACE(reg," (ret=%lx,type=%lx, len=%ld)\n", ret,mytype,lpcbData?*lpcbData:0);
	
	if(mybuf) free(mybuf);
	if(lpszValueNameW) free(lpszValueNameW);
	return ret;
}


/******************************************************************************
 * RegQueryValueEx16 [KERNEL.225]
 */
DWORD WINAPI RegQueryValueEx16( HKEY hkey, LPSTR lpszValueName,
                                LPDWORD lpdwReserved, LPDWORD lpdwType,
                                LPBYTE lpbData, LPDWORD lpcbData )
{
    TRACE(reg,"(%x,%s,%p,%p,%p,%ld)\n",hkey,debugstr_a(lpszValueName),
          lpdwReserved,lpdwType,lpbData,lpcbData?*lpcbData:0);
    return RegQueryValueEx32A( hkey, lpszValueName, lpdwReserved, lpdwType,
                               lpbData, lpcbData );
}


/******************************************************************************
 * RegQueryValue32A [ADVAPI32.156]
 */
DWORD WINAPI RegQueryValue32A( HKEY hkey, LPCSTR lpszSubKey, LPSTR lpszData,
                               LPLONG lpcbData )
{
    HKEY xhkey;
    DWORD ret, dwType;

    TRACE(reg,"(%x,%s,%p,%ld)\n",hkey,debugstr_a(lpszSubKey),lpszData,
          lpcbData?*lpcbData:0);

    if (lpszSubKey && *lpszSubKey) {
        ret = RegOpenKey16( hkey, lpszSubKey, &xhkey );
        if( ret != ERROR_SUCCESS )
            return ret;
    } else
        xhkey = hkey;

    dwType = REG_SZ;
    ret = RegQueryValueEx32A( xhkey, NULL,NULL, &dwType, (LPBYTE)lpszData,
                              lpcbData );
    if( xhkey != hkey )
        RegCloseKey( xhkey );
    return ret;
}


/******************************************************************************
 * RegQueryValue16 [SHELL.6] [KERNEL.224]
 *
 * NOTES
 *    Is this HACK still applicable?
 *
 * HACK
 *    The 16bit RegQueryValue doesn't handle selectorblocks anyway, so we just
 *    mask out the high 16 bit.  This (not so much incidently) hopefully fixes
 *    Aldus FH4)
 */
DWORD WINAPI RegQueryValue16( HKEY hkey, LPSTR lpszSubKey, LPSTR lpszData,
                              LPDWORD lpcbData )
{
    TRACE(reg,"(%x,%s,%p,%ld)\n",hkey,debugstr_a(lpszSubKey),lpszData,
          lpcbData?*lpcbData:0);

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


/******************************************************************************
 * RegSetValueEx32W [ADVAPI32.170]
 * Sets the data and type of a value under a register key
 *
 * PARAMS
 *    hkey          [I] Handle of key to set value for
 *    lpszValueName [I] Name of value to set
 *    dwReserved    [I] Reserved - must be zero
 *    dwType        [I] Flag for value type
 *    lpbData       [I] Address of value data
 *    cbData        [I] Size of value data
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: Error code
 *
 * NOTES
 *   win95 does not care about cbData for REG_SZ and finds out the len by itself (js) 
 */
DWORD WINAPI RegSetValueEx32W( HKEY hkey, LPWSTR lpszValueName, 
                               DWORD dwReserved, DWORD dwType, LPBYTE lpbData,
                               DWORD cbData)
{
	LPKEYSTRUCT lpkey;
	int i;

	TRACE(reg,"(%x,%s,%ld,%ld,%p,%ld)\n", hkey, debugstr_w(lpszValueName),
          dwReserved, dwType, lpbData, cbData);

	debug_print_value ( lpbData, dwType, cbData );

	lpkey = lookup_hkey( hkey );

	if (!lpkey)
          return ERROR_INVALID_HANDLE;

	lpkey->flags |= REG_OPTION_TAINTED;

	if (lpszValueName==NULL) {
             /* Sets type and name for key's unnamed or default value */
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
	if (lpkey->values[i].name==NULL) {
		if (lpszValueName)
			lpkey->values[i].name = strdupW(lpszValueName);
		else
			lpkey->values[i].name = NULL;
	}

	if (dwType == REG_SZ)
	  cbData = 2 * (lstrlen32W ((LPCWSTR)lpbData) + 1);
	  
	lpkey->values[i].len	= cbData;
	lpkey->values[i].type	= dwType;
	if (lpkey->values[i].data !=NULL)
		free(lpkey->values[i].data);
	lpkey->values[i].data	= (LPBYTE)xmalloc(cbData);
	lpkey->values[i].lastmodified = time(NULL);
	memcpy(lpkey->values[i].data,lpbData,cbData);
	return ERROR_SUCCESS;
}


/******************************************************************************
 * RegSetValueEx32A [ADVAPI32.169]
 *
 * NOTES
 *   win95 does not care about cbData for REG_SZ and finds out the len by itself (js) 
 */
DWORD WINAPI RegSetValueEx32A( HKEY hkey, LPSTR lpszValueName,
                               DWORD dwReserved, DWORD dwType, LPBYTE lpbData,
                               DWORD cbData )
{
	LPBYTE	buf;
	LPWSTR	lpszValueNameW;
	DWORD	ret;

	if (!lpbData)
		return (ERROR_INVALID_PARAMETER);
		
	TRACE(reg,"(%x,%s,%ld,%ld,%p,%ld)\n",hkey,debugstr_a(lpszValueName),
          dwReserved,dwType,lpbData,cbData);

	if ((1<<dwType) & UNICONVMASK) 
	{ if (dwType == REG_SZ)
	    cbData = strlen ((LPCSTR)lpbData)+1;

	  buf = (LPBYTE)xmalloc( cbData *2 );
	  lmemcpynAtoW ((LPVOID)buf, lpbData, cbData );
	  cbData=2*cbData;
	} 
	else
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


/******************************************************************************
 * RegSetValueEx16 [KERNEL.226]
 */
DWORD WINAPI RegSetValueEx16( HKEY hkey, LPSTR lpszValueName, DWORD dwReserved,
                              DWORD dwType, LPBYTE lpbData, DWORD cbData )
{
    TRACE(reg,"(%x,%s,%ld,%ld,%p,%ld)\n",hkey,debugstr_a(lpszValueName),
          dwReserved,dwType,lpbData,cbData);
    return RegSetValueEx32A( hkey, lpszValueName, dwReserved, dwType, lpbData,
                             cbData );
}


/******************************************************************************
 * RegSetValue32W	[ADVAPI32.171]
 */
DWORD WINAPI RegSetValue32W( HKEY hkey, LPCWSTR lpszSubKey, DWORD dwType,
                             LPCWSTR lpszData, DWORD cbData )
{
	HKEY	xhkey;
	DWORD	ret;

	TRACE(reg,"(%x,%s,%ld,%s,%ld)\n",
		hkey,debugstr_w(lpszSubKey),dwType,debugstr_w(lpszData),cbData
	);
	if (lpszSubKey && *lpszSubKey) {
		ret=RegCreateKey32W(hkey,lpszSubKey,&xhkey);
		if (ret!=ERROR_SUCCESS)
			return ret;
	} else
		xhkey=hkey;
	if (dwType!=REG_SZ) {
		TRACE(reg,"dwType=%ld - Changing to REG_SZ\n",dwType);
		dwType=REG_SZ;
	}
	if (cbData!=2*lstrlen32W(lpszData)+2) {
		TRACE(reg,"Len=%ld != strlen(%s)+1=%d!\n",
			cbData,debugstr_w(lpszData),2*lstrlen32W(lpszData)+2
		);
		cbData=2*lstrlen32W(lpszData)+2;
	}
	ret=RegSetValueEx32W(xhkey,NULL,0,dwType,(LPBYTE)lpszData,cbData);
	if (hkey!=xhkey)
		RegCloseKey(xhkey);
	return ret;
}


/******************************************************************************
 * RegSetValue32A [ADVAPI32.168]
 *
 */
DWORD WINAPI RegSetValue32A( HKEY hkey, LPCSTR lpszSubKey, DWORD dwType,
                             LPCSTR lpszData, DWORD cbData )
{
	DWORD	ret;
	HKEY	xhkey;

	TRACE(reg,"(%x,%s,%ld,%s,%ld)\n",hkey,lpszSubKey,dwType,lpszData,cbData);
	if (lpszSubKey && *lpszSubKey) {
		ret=RegCreateKey16(hkey,lpszSubKey,&xhkey);
		if (ret!=ERROR_SUCCESS)
			return ret;
	} else
		xhkey=hkey;

	if (dwType!=REG_SZ) {
		TRACE(reg,"dwType=%ld!\n",dwType);
		dwType=REG_SZ;
	}
	if (cbData!=strlen(lpszData)+1)
		cbData=strlen(lpszData)+1;
	ret=RegSetValueEx32A(xhkey,NULL,0,dwType,(LPBYTE)lpszData,cbData);
	if (xhkey!=hkey)
		RegCloseKey(xhkey);
	return ret;
}


/******************************************************************************
 * RegSetValue16 [KERNEL.221] [SHELL.5]
 */
DWORD WINAPI RegSetValue16( HKEY hkey, LPCSTR lpszSubKey, DWORD	dwType,
                            LPCSTR lpszData, DWORD cbData )
{
    TRACE(reg,"(%x,%s,%ld,%s,%ld)\n",hkey,debugstr_a(lpszSubKey),dwType,
          debugstr_a(lpszData),cbData);
    return RegSetValue32A(hkey,lpszSubKey,dwType,lpszData,cbData);
}


/* 
 * Key Enumeration
 *
 * Callpath:
 * RegEnumKey16 -> RegEnumKey32A -> RegEnumKeyEx32A \
 *                                  RegEnumKey32W   -> RegEnumKeyEx32W
 */


/******************************************************************************
 * RegEnumKeyEx32W [ADVAPI32.139]
 *
 * PARAMS
 *    hkey         [I] Handle to key to enumerate
 *    iSubKey      [I] Index of subkey to enumerate
 *    lpszName     [O] Buffer for subkey name
 *    lpcchName    [O] Size of subkey buffer
 *    lpdwReserved [I] Reserved
 *    lpszClass    [O] Buffer for class string
 *    lpcchClass   [O] Size of class buffer
 *    ft           [O] Time key last written to
 */
DWORD WINAPI RegEnumKeyEx32W( HKEY hkey, DWORD iSubkey, LPWSTR lpszName,
                              LPDWORD lpcchName, LPDWORD lpdwReserved,
                              LPWSTR lpszClass, LPDWORD lpcchClass, 
                              FILETIME *ft )
{
	LPKEYSTRUCT	lpkey,lpxkey;

    TRACE(reg,"(%x,%ld,%p,%ld,%p,%p,%p,%p)\n",hkey,iSubkey,lpszName,
          *lpcchName,lpdwReserved,lpszClass,lpcchClass,ft);

    lpkey = lookup_hkey( hkey );
    if (!lpkey)
        return ERROR_INVALID_HANDLE;

	if (!lpkey->nextsub)
		return ERROR_NO_MORE_ITEMS;
	lpxkey=lpkey->nextsub;

    /* Traverse the subkeys */
	while (iSubkey && lpxkey) {
		iSubkey--;
		lpxkey=lpxkey->next;
	}

	if (iSubkey || !lpxkey)
		return ERROR_NO_MORE_ITEMS;
	if (lstrlen32W(lpxkey->keyname)+1>*lpcchName)
		return ERROR_MORE_DATA;
	memcpy(lpszName,lpxkey->keyname,lstrlen32W(lpxkey->keyname)*2+2);

        if (*lpcchName)
            *lpcchName = lstrlen32W(lpszName);

	if (lpszClass) {
		/* FIXME: what should we write into it? */
		*lpszClass	= 0;
		*lpcchClass	= 2;
	}
	return ERROR_SUCCESS;
}


/******************************************************************************
 * RegEnumKey32W [ADVAPI32.140]
 */
DWORD WINAPI RegEnumKey32W( HKEY hkey, DWORD iSubkey, LPWSTR lpszName, 
                            DWORD lpcchName )
{
    FILETIME	ft;

    TRACE(reg,"(%x,%ld,%p,%ld)\n",hkey,iSubkey,lpszName,lpcchName);
    return RegEnumKeyEx32W(hkey,iSubkey,lpszName,&lpcchName,NULL,NULL,NULL,&ft);
}


/******************************************************************************
 * RegEnumKeyEx32A [ADVAPI32.138]
 */
DWORD WINAPI RegEnumKeyEx32A( HKEY hkey, DWORD iSubkey, LPSTR lpszName,
                              LPDWORD lpcchName, LPDWORD lpdwReserved, 
                              LPSTR lpszClass, LPDWORD lpcchClass, 
                              FILETIME *ft )
{
	DWORD	ret,lpcchNameW,lpcchClassW;
	LPWSTR	lpszNameW,lpszClassW;


	TRACE(reg,"(%x,%ld,%p,%ld,%p,%p,%p,%p)\n",
		hkey,iSubkey,lpszName,*lpcchName,lpdwReserved,lpszClass,lpcchClass,ft
	);
	if (lpszName) {
		lpszNameW	= (LPWSTR)xmalloc(*lpcchName*2);
		lpcchNameW	= *lpcchName;
	} else {
		lpszNameW	= NULL;
		lpcchNameW 	= 0;
	}
	if (lpszClass) {
		lpszClassW		= (LPWSTR)xmalloc(*lpcchClass*2);
		lpcchClassW	= *lpcchClass;
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


/******************************************************************************
 * RegEnumKey32A [ADVAPI32.137]
 */
DWORD WINAPI RegEnumKey32A( HKEY hkey, DWORD iSubkey, LPSTR lpszName,
                            DWORD lpcchName )
{
    FILETIME	ft;

    TRACE(reg,"(%x,%ld,%p,%ld)\n",hkey,iSubkey,lpszName,lpcchName);
    return RegEnumKeyEx32A( hkey, iSubkey, lpszName, &lpcchName, NULL, NULL, 
                            NULL, &ft );
}


/******************************************************************************
 * RegEnumKey16 [SHELL.7] [KERNEL.216]
 */
DWORD WINAPI RegEnumKey16( HKEY hkey, DWORD iSubkey, LPSTR lpszName,
                           DWORD lpcchName )
{
    TRACE(reg,"(%x,%ld,%p,%ld)\n",hkey,iSubkey,lpszName,lpcchName);
    return RegEnumKey32A( hkey, iSubkey, lpszName, lpcchName);
}


/* 
 * Enumerate Registry Values
 *
 * Callpath:
 * RegEnumValue16 -> RegEnumValue32A -> RegEnumValue32W
 */


/******************************************************************************
 * RegEnumValue32W [ADVAPI32.142]
 *
 * PARAMS
 *    hkey        [I] Handle to key to query
 *    iValue      [I] Index of value to query
 *    lpszValue   [O] Value string
 *    lpcchValue  [I/O] Size of value buffer (in wchars)
 *    lpdReserved [I] Reserved
 *    lpdwType    [O] Type code
 *    lpbData     [O] Value data
 *    lpcbData    [I/O] Size of data buffer (in bytes)
 *
 * Note:  wide character functions that take and/or return "character counts"
 *  use TCHAR (that is unsigned short or char) not byte counts.
 */
DWORD WINAPI RegEnumValue32W( HKEY hkey, DWORD iValue, LPWSTR lpszValue,
                              LPDWORD lpcchValue, LPDWORD lpdReserved,
                              LPDWORD lpdwType, LPBYTE lpbData, 
                              LPDWORD lpcbData )
{
	LPKEYSTRUCT	lpkey;
	LPKEYVALUE	val;

	TRACE(reg,"(%x,%ld,%p,%p,%p,%p,%p,%p)\n",hkey,iValue,debugstr_w(lpszValue),
          lpcchValue,lpdReserved,lpdwType,lpbData,lpcbData);

	lpkey = lookup_hkey( hkey );
	
	if (!lpcbData && lpbData)
		return ERROR_INVALID_PARAMETER;
		
	if (!lpkey)
		return ERROR_INVALID_HANDLE;

	if (lpkey->nrofvalues <= iValue)
		return ERROR_NO_MORE_ITEMS;

	val = &(lpkey->values[iValue]);

	if (val->name) {
	        if (lstrlen32W(val->name)+1>*lpcchValue) {
			*lpcchValue = lstrlen32W(val->name)+1;
			return ERROR_MORE_DATA;
		}
		memcpy(lpszValue,val->name,2 * (lstrlen32W(val->name)+1) );
		*lpcchValue=lstrlen32W(val->name);
	} else {
		*lpszValue	= 0;
		*lpcchValue	= 0;
	}

	/* Can be NULL if the type code is not required */
	if (lpdwType)
		*lpdwType = val->type;

	if (lpbData) {
		if (val->len>*lpcbData)
			return ERROR_MORE_DATA;
		memcpy(lpbData,val->data,val->len);
		*lpcbData = val->len;
	}

	debug_print_value ( val->data, val->type, val->len );
	return ERROR_SUCCESS;
}


/******************************************************************************
 * RegEnumValue32A [ADVAPI32.141]
 */
DWORD WINAPI RegEnumValue32A( HKEY hkey, DWORD iValue, LPSTR lpszValue,
                              LPDWORD lpcchValue, LPDWORD lpdReserved,
                              LPDWORD lpdwType, LPBYTE lpbData, 
                              LPDWORD lpcbData )
{
	LPWSTR	lpszValueW;
	LPBYTE	lpbDataW;
	DWORD	ret,lpcbDataW;
	DWORD dwType;

	TRACE(reg,"(%x,%ld,%p,%p,%p,%p,%p,%p)\n",hkey,iValue,lpszValue,lpcchValue,
		lpdReserved,lpdwType,lpbData,lpcbData);

	lpszValueW = (LPWSTR)xmalloc(*lpcchValue*2);
	if (lpbData) {
		lpbDataW = (LPBYTE)xmalloc(*lpcbData*2);
		lpcbDataW = *lpcbData;
	} else
		lpbDataW = NULL;

	ret = RegEnumValue32W( hkey, iValue, lpszValueW, lpcchValue, 
				lpdReserved, &dwType, lpbDataW, &lpcbDataW );

	if (lpdwType)
		*lpdwType = dwType;

	if (ret==ERROR_SUCCESS) {
		lstrcpyWtoA(lpszValue,lpszValueW);
		if (lpbData) {
			if ((1<<dwType) & UNICONVMASK) {
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
    if (lpbDataW) free(lpbDataW);
    if (lpszValueW) free(lpszValueW);
    return ret;
}


/******************************************************************************
 * RegEnumValue16 [KERNEL.223]
 */
DWORD WINAPI RegEnumValue16( HKEY hkey, DWORD iValue, LPSTR lpszValue, 
                             LPDWORD lpcchValue, LPDWORD lpdReserved, 
                             LPDWORD lpdwType, LPBYTE lpbData, 
                             LPDWORD lpcbData )
{
    TRACE(reg,"(%x,%ld,%p,%p,%p,%p,%p,%p)\n",hkey,iValue,lpszValue,lpcchValue,
          lpdReserved,lpdwType,lpbData,lpcbData);
    return RegEnumValue32A( hkey, iValue, lpszValue, lpcchValue, lpdReserved, 
                            lpdwType, lpbData, lpcbData );
}


/******************************************************************************
 * RegCloseKey [SHELL.3] [KERNEL.220] [ADVAPI32.126]
 * Releases the handle of the specified key
 *
 * PARAMS
 *    hkey [I] Handle of key to close
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: Error code
 */
DWORD WINAPI RegCloseKey( HKEY hkey )
{
    TRACE(reg,"(%x)\n",hkey);

    /* The standard handles are allowed to succeed, even though they are not
       closed */
    if (is_standard_hkey(hkey))
        return ERROR_SUCCESS;

    return remove_handle(hkey);
}


/* 
 * Delete registry key
 *
 * Callpath:
 * RegDeleteKey16 -> RegDeleteKey32A -> RegDeleteKey32W
 */


/******************************************************************************
 * RegDeleteKey32W [ADVAPI32.134]
 *
 * PARAMS
 *    hkey       [I] Handle to open key
 *    lpszSubKey [I] Name of subkey to delete
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: Error code
 */
DWORD WINAPI RegDeleteKey32W( HKEY hkey, LPWSTR lpszSubKey )
{
	LPKEYSTRUCT	*lplpPrevKey,lpNextKey,lpxkey;
	LPWSTR		*wps;
	int		wpc,i;

    TRACE(reg,"(%x,%s)\n",hkey,debugstr_w(lpszSubKey));

    lpNextKey = lookup_hkey(hkey);
    if (!lpNextKey)
        return ERROR_INVALID_HANDLE;

    /* Subkey param cannot be NULL */
    if (!lpszSubKey || !*lpszSubKey)
        return ERROR_BADKEY;

    /* We need to know the previous key in the hier. */
	split_keypath(lpszSubKey,&wps,&wpc);
	i 	= 0;
	lpxkey	= lpNextKey;
	while (i<wpc-1) {
		lpxkey=lpNextKey->nextsub;
		while (lpxkey) {
			TRACE(reg, "  Scanning [%s]\n",
				     debugstr_w(lpxkey->keyname));
			if (!lstrcmpi32W(wps[i],lpxkey->keyname))
				break;
			lpxkey=lpxkey->next;
		}
		if (!lpxkey) {
			FREE_KEY_PATH;
			TRACE(reg, "  Not found.\n");
			/* not found is success */
			return ERROR_SUCCESS;
		}
		i++;
		lpNextKey	= lpxkey;
	}
	lpxkey	= lpNextKey->nextsub;
	lplpPrevKey = &(lpNextKey->nextsub);
	while (lpxkey) {
		TRACE(reg, "  Scanning [%s]\n",
			     debugstr_w(lpxkey->keyname));
		if (!lstrcmpi32W(wps[i],lpxkey->keyname))
			break;
		lplpPrevKey	= &(lpxkey->next);
		lpxkey		= lpxkey->next;
	}

	if (!lpxkey) {
		FREE_KEY_PATH;
		WARN(reg , "  Not found.\n");
		return ERROR_FILE_NOT_FOUND;
	}

	if (lpxkey->nextsub) {
		FREE_KEY_PATH;
		WARN(reg , "  Not empty.\n");
		return ERROR_CANTWRITE;
	}
	*lplpPrevKey	= lpxkey->next;
	free(lpxkey->keyname);
	if (lpxkey->class)
		free(lpxkey->class);
	if (lpxkey->values)
		free(lpxkey->values);
	free(lpxkey);
	FREE_KEY_PATH;
	TRACE(reg, "  Done.\n");
	return	ERROR_SUCCESS;
}


/******************************************************************************
 * RegDeleteKey32A [ADVAPI32.133]
 */
DWORD WINAPI RegDeleteKey32A( HKEY hkey, LPCSTR lpszSubKey )
{
    LPWSTR lpszSubKeyW;
    DWORD  ret;

    TRACE(reg,"(%x,%s)\n",hkey,debugstr_a(lpszSubKey));
    lpszSubKeyW = lpszSubKey?strdupA2W(lpszSubKey):NULL;
    ret = RegDeleteKey32W( hkey, lpszSubKeyW );
    if(lpszSubKeyW) free(lpszSubKeyW);
    return ret;
}


/******************************************************************************
 * RegDeleteKey16 [SHELL.4] [KERNEL.219]
 */
DWORD WINAPI RegDeleteKey16( HKEY hkey, LPCSTR lpszSubKey )
{
    TRACE(reg,"(%x,%s)\n",hkey,debugstr_a(lpszSubKey));
    return RegDeleteKey32A( hkey, lpszSubKey );
}


/* 
 * Delete registry value
 *
 * Callpath:
 * RegDeleteValue16 -> RegDeleteValue32A -> RegDeleteValue32W
 */


/******************************************************************************
 * RegDeleteValue32W [ADVAPI32.136]
 *
 * PARAMS
 *    hkey      [I]
 *    lpszValue [I]
 *
 * RETURNS
 */
DWORD WINAPI RegDeleteValue32W( HKEY hkey, LPWSTR lpszValue )
{
	DWORD		i;
	LPKEYSTRUCT	lpkey;
	LPKEYVALUE	val;

    TRACE(reg,"(%x,%s)\n",hkey,debugstr_w(lpszValue));

    lpkey = lookup_hkey( hkey );
    if (!lpkey)
        return ERROR_INVALID_HANDLE;

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

    if (i == lpkey->nrofvalues)
        return ERROR_FILE_NOT_FOUND;

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
	return ERROR_SUCCESS;
}


/******************************************************************************
 * RegDeleteValue32A [ADVAPI32.135]
 */
DWORD WINAPI RegDeleteValue32A( HKEY hkey, LPSTR lpszValue )
{
    LPWSTR lpszValueW;
    DWORD  ret;

    TRACE(reg, "(%x,%s)\n",hkey,debugstr_a(lpszValue));
    lpszValueW = lpszValue?strdupA2W(lpszValue):NULL;
    ret = RegDeleteValue32W( hkey, lpszValueW );
    if(lpszValueW) free(lpszValueW);
    return ret;
}


/******************************************************************************
 * RegDeleteValue16 [KERNEL.222]
 */
DWORD WINAPI RegDeleteValue16( HKEY hkey, LPSTR lpszValue )
{
    TRACE(reg,"(%x,%s)\n", hkey,debugstr_a(lpszValue));
    return RegDeleteValue32A( hkey, lpszValue );
}


/******************************************************************************
 * RegFlushKey [KERNEL.227] [ADVAPI32.143]
 * Writes key to registry
 *
 * PARAMS
 *    hkey [I] Handle of key to write
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: Error code
 */
DWORD WINAPI RegFlushKey( HKEY hkey )
{
    LPKEYSTRUCT	lpkey;
    BOOL32 ret;

    TRACE(reg, "(%x)\n", hkey);

    lpkey = lookup_hkey( hkey );
    if (!lpkey)
        return ERROR_BADKEY;

    ERR(reg, "What is the correct filename?\n");

    ret = _savereg( lpkey, "foo.bar", TRUE);

    if( ret ) {
        return ERROR_SUCCESS;
    } else
        return ERROR_UNKNOWN;  /* FIXME */
}


/* FIXME: lpcchXXXX ... is this counting in WCHARS or in BYTEs ?? */


/******************************************************************************
 * RegQueryInfoKey32W [ADVAPI32.153]
 *
 * PARAMS
 *    hkey                   [I] Handle to key to query
 *    lpszClass              [O] Buffer for class string
 *    lpcchClass             [O] Size of class string buffer
 *    lpdwReserved           [I] Reserved
 *    lpcSubKeys             [I] Buffer for number of subkeys
 *    lpcchMaxSubKey         [O] Buffer for longest subkey name length
 *    lpcchMaxClass          [O] Buffer for longest class string length
 *    lpcValues              [O] Buffer for number of value entries
 *    lpcchMaxValueName      [O] Buffer for longest value name length
 *    lpccbMaxValueData      [O] Buffer for longest value data length
 *    lpcbSecurityDescriptor [O] Buffer for security descriptor length
 *    ft
 * - win95 allows lpszClass to be valid and lpcchClass to be NULL 
 * - winnt returns ERROR_INVALID_PARAMETER if lpszClass is valid and
 *   lpcchClass is NULL
 * - both allow lpszClass to be NULL and lpcchClass to be NULL 
 * (it's hard to test validity, so test !NULL instead)
 */
DWORD WINAPI RegQueryInfoKey32W( HKEY hkey, LPWSTR lpszClass, 
                                 LPDWORD lpcchClass, LPDWORD lpdwReserved,
                                 LPDWORD lpcSubKeys, LPDWORD lpcchMaxSubkey,
                                 LPDWORD lpcchMaxClass, LPDWORD lpcValues,
                                 LPDWORD lpcchMaxValueName, 
                                 LPDWORD lpccbMaxValueData, 
                                 LPDWORD lpcbSecurityDescriptor, FILETIME *ft )
{
	LPKEYSTRUCT	lpkey,lpxkey;
	int		nrofkeys,maxsubkey,maxclass,maxvname,maxvdata;
	int		i;

	TRACE(reg,"(%x,%p,...)\n",hkey,lpszClass);
	lpkey = lookup_hkey(hkey);
	if (!lpkey)
		return ERROR_INVALID_HANDLE;
	if (lpszClass) {
   	        if (VERSION_GetVersion() == NT40 && lpcchClass == NULL) {
		    return ERROR_INVALID_PARAMETER;
		}
		/* either lpcchClass is valid or this is win95 and lpcchClass
		   could be invalid */
		if (lpkey->class) {
		        DWORD classLen = lstrlen32W(lpkey->class);

			if (lpcchClass && classLen+1>*lpcchClass) {
				*lpcchClass=classLen+1;
				return ERROR_MORE_DATA;
			}
			if (lpcchClass)
			    *lpcchClass=classLen;
			memcpy(lpszClass,lpkey->class, classLen*2 + 2);
		} else {
			*lpszClass	= 0;
			if (lpcchClass)
			    *lpcchClass	= 0;
		}
	} else {
		if (lpcchClass)
			*lpcchClass	= lstrlen32W(lpkey->class);
	}
	lpxkey=lpkey->nextsub;
	nrofkeys=maxsubkey=maxclass=maxvname=maxvdata=0;
	while (lpxkey) {
		nrofkeys++;
		if (lstrlen32W(lpxkey->keyname)>maxsubkey)
			maxsubkey=lstrlen32W(lpxkey->keyname);
		if (lpxkey->class && lstrlen32W(lpxkey->class)>maxclass)
			maxclass=lstrlen32W(lpxkey->class);
		lpxkey=lpxkey->next;
	}
	for (i=0;i<lpkey->nrofvalues;i++) {
		LPKEYVALUE	val=lpkey->values+i;

		if (val->name && lstrlen32W(val->name)>maxvname)
			maxvname=lstrlen32W(val->name);
		if (val->len>maxvdata)
			maxvdata=val->len;
	}
	if (!maxclass) maxclass	= 1;
	if (!maxvname) maxvname	= 1;
	if (lpcValues)
		*lpcValues	= lpkey->nrofvalues;
	if (lpcSubKeys)
		*lpcSubKeys	= nrofkeys;
	if (lpcchMaxSubkey)
		*lpcchMaxSubkey	= maxsubkey;
	if (lpcchMaxClass)
		*lpcchMaxClass	= maxclass;
	if (lpcchMaxValueName)
		*lpcchMaxValueName= maxvname;
	if (lpccbMaxValueData)
		*lpccbMaxValueData= maxvdata;
	return ERROR_SUCCESS;
}


/******************************************************************************
 * RegQueryInfoKey32A [ADVAPI32.152]
 */
DWORD WINAPI RegQueryInfoKey32A( HKEY hkey, LPSTR lpszClass, LPDWORD lpcchClass,
                                 LPDWORD lpdwReserved, LPDWORD lpcSubKeys,
                                 LPDWORD lpcchMaxSubkey, LPDWORD lpcchMaxClass,
                                 LPDWORD lpcValues, LPDWORD lpcchMaxValueName,
                                 LPDWORD lpccbMaxValueData, 
                                 LPDWORD lpcbSecurityDescriptor, FILETIME *ft )
{
	LPWSTR		lpszClassW = NULL;
	DWORD		ret;

	TRACE(reg,"(%x,%p,%p......)\n",hkey, lpszClass, lpcchClass);
	if (lpszClass) {
		if (lpcchClass) {
		    lpszClassW  = (LPWSTR)xmalloc((*lpcchClass) * 2);
		} else if (VERSION_GetVersion() == WIN95) {
		    /* win95  allows lpcchClass to be null */
		    /* we don't know how big lpszClass is, would 
		       MAX_PATHNAME_LEN be the correct default? */
		    lpszClassW  = (LPWSTR)xmalloc(MAX_PATHNAME_LEN*2); 
		}

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
	if (lpszClassW)
		free(lpszClassW);
	return ret;
}


/******************************************************************************
 * RegConnectRegistry32W [ADVAPI32.128]
 *
 * PARAMS
 *    lpMachineName [I] Address of name of remote computer
 *    hHey          [I] Predefined registry handle
 *    phkResult     [I] Address of buffer for remote registry handle
 */
LONG WINAPI RegConnectRegistry32W( LPCWSTR lpMachineName, HKEY hKey, 
                                   LPHKEY phkResult )
{
    TRACE(reg,"(%s,%x,%p): stub\n",debugstr_w(lpMachineName),hKey,phkResult);

    if (!lpMachineName || !*lpMachineName) {
        /* Use the local machine name */
        return RegOpenKey16( hKey, "", phkResult );
    }

    FIXME(reg,"Cannot connect to %s\n",debugstr_w(lpMachineName));
    return ERROR_BAD_NETPATH;
}


/******************************************************************************
 * RegConnectRegistry32A [ADVAPI32.127]
 */
LONG WINAPI RegConnectRegistry32A( LPCSTR machine, HKEY hkey, LPHKEY reskey )
{
    DWORD ret;
    LPWSTR machineW = strdupA2W(machine);
    ret = RegConnectRegistry32W( machineW, hkey, reskey );
    free(machineW);
    return ret;
}


/******************************************************************************
 * RegGetKeySecurity [ADVAPI32.144]
 * Retrieves a copy of security descriptor protecting the registry key
 *
 * PARAMS
 *    hkey                   [I]   Open handle of key to set
 *    SecurityInformation    [I]   Descriptor contents
 *    pSecurityDescriptor    [O]   Address of descriptor for key
 *    lpcbSecurityDescriptor [I/O] Address of size of buffer and description
 *
 * RETURNS
 *    Success: ERROR_SUCCESS
 *    Failure: Error code
 */
LONG WINAPI RegGetKeySecurity( HKEY hkey, 
                               SECURITY_INFORMATION SecurityInformation,
                               PSECURITY_DESCRIPTOR pSecurityDescriptor,
                               LPDWORD lpcbSecurityDescriptor )
{
    LPKEYSTRUCT	lpkey;

    TRACE(reg,"(%x,%ld,%p,%ld)\n",hkey,SecurityInformation,pSecurityDescriptor,
          lpcbSecurityDescriptor?*lpcbSecurityDescriptor:0);

    lpkey = lookup_hkey( hkey );
    if (!lpkey)
        return ERROR_INVALID_HANDLE;

    /* FIXME: Check for valid SecurityInformation values */

    if (*lpcbSecurityDescriptor < sizeof(SECURITY_DESCRIPTOR))
        return ERROR_INSUFFICIENT_BUFFER;

    FIXME(reg, "(%x,%ld,%p,%ld): stub\n",hkey,SecurityInformation,
          pSecurityDescriptor,lpcbSecurityDescriptor?*lpcbSecurityDescriptor:0);

    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegLoadKey32W [ADVAPI32.???]
 *
 * PARAMS
 *    hkey       [I] Handle of open key
 *    lpszSubKey [I] Address of name of subkey
 *    lpszFile   [I] Address of filename for registry information
 */
LONG WINAPI RegLoadKey32W( HKEY hkey, LPCWSTR lpszSubKey, LPCWSTR lpszFile )
{
    LPKEYSTRUCT	lpkey;
    TRACE(reg,"(%x,%s,%s)\n",hkey,debugstr_w(lpszSubKey),debugstr_w(lpszFile));

    /* Do this check before the hkey check */
    if (!lpszSubKey || !*lpszSubKey || !lpszFile || !*lpszFile)
        return ERROR_INVALID_PARAMETER;

    lpkey = lookup_hkey( hkey );
    if (!lpkey)
        return ERROR_INVALID_HANDLE;

    FIXME(reg,"(%x,%s,%s): stub\n",hkey,debugstr_w(lpszSubKey),
          debugstr_w(lpszFile));

    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegLoadKey32A [ADVAPI32.???]
 */
LONG WINAPI RegLoadKey32A( HKEY hkey, LPCSTR lpszSubKey, LPCSTR lpszFile )
{
    LONG ret;
    LPWSTR lpszSubKeyW = strdupA2W(lpszSubKey);
    LPWSTR lpszFileW = strdupA2W(lpszFile);
    ret = RegLoadKey32W( hkey, lpszSubKeyW, lpszFileW );
    if(lpszFileW) free(lpszFileW);
    if(lpszSubKeyW) free(lpszSubKeyW);
    return ret;
}


/******************************************************************************
 * RegNotifyChangeKeyValue [ADVAPI32.???]
 *
 * PARAMS
 *    hkey            [I] Handle of key to watch
 *    fWatchSubTree   [I] Flag for subkey notification
 *    fdwNotifyFilter [I] Changes to be reported
 *    hEvent          [I] Handle of signaled event
 *    fAsync          [I] Flag for asynchronous reporting
 */
LONG WINAPI RegNotifyChangeKeyValue( HKEY hkey, BOOL32 fWatchSubTree, 
                                     DWORD fdwNotifyFilter, HANDLE32 hEvent,
                                     BOOL32 fAsync )
{
    LPKEYSTRUCT	lpkey;
    TRACE(reg,"(%x,%i,%ld,%x,%i)\n",hkey,fWatchSubTree,fdwNotifyFilter,
          hEvent,fAsync);

    lpkey = lookup_hkey( hkey );
    if (!lpkey)
        return ERROR_INVALID_HANDLE;

    FIXME(reg,"(%x,%i,%ld,%x,%i): stub\n",hkey,fWatchSubTree,fdwNotifyFilter,
          hEvent,fAsync);

    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegUnLoadKey32W [ADVAPI32.173]
 *
 * PARAMS
 *    hkey     [I] Handle of open key
 *    lpSubKey [I] Address of name of subkey to unload
 */
LONG WINAPI RegUnLoadKey32W( HKEY hkey, LPCWSTR lpSubKey )
{
    FIXME(reg,"(%x,%s): stub\n",hkey, debugstr_w(lpSubKey));
    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegUnLoadKey32A [ADVAPI32.172]
 */
LONG WINAPI RegUnLoadKey32A( HKEY hkey, LPCSTR lpSubKey )
{
    LONG ret;
    LPWSTR lpSubKeyW = strdupA2W(lpSubKey);
    ret = RegUnLoadKey32W( hkey, lpSubKeyW );
    if(lpSubKeyW) free(lpSubKeyW);
    return ret;
}


/******************************************************************************
 * RegSetKeySecurity [ADVAPI32.167]
 *
 * PARAMS
 *    hkey          [I] Open handle of key to set
 *    SecurityInfo  [I] Descriptor contents
 *    pSecurityDesc [I] Address of descriptor for key
 */
LONG WINAPI RegSetKeySecurity( HKEY hkey, SECURITY_INFORMATION SecurityInfo,
                               PSECURITY_DESCRIPTOR pSecurityDesc )
{
    LPKEYSTRUCT	lpkey;

    TRACE(reg,"(%x,%ld,%p)\n",hkey,SecurityInfo,pSecurityDesc);

    /* It seems to perform this check before the hkey check */
    if ((SecurityInfo & OWNER_SECURITY_INFORMATION) ||
        (SecurityInfo & GROUP_SECURITY_INFORMATION) ||
        (SecurityInfo & DACL_SECURITY_INFORMATION) ||
        (SecurityInfo & SACL_SECURITY_INFORMATION)) {
        /* Param OK */
    } else
        return ERROR_INVALID_PARAMETER;

    if (!pSecurityDesc)
        return ERROR_INVALID_PARAMETER;

    lpkey = lookup_hkey( hkey );
    if (!lpkey)
        return ERROR_INVALID_HANDLE;

    FIXME(reg,":(%x,%ld,%p): stub\n",hkey,SecurityInfo,pSecurityDesc);

    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegSaveKey32W [ADVAPI32.166]
 *
 * PARAMS
 *    hkey   [I] Handle of key where save begins
 *    lpFile [I] Address of filename to save to
 *    sa     [I] Address of security structure
 */
LONG WINAPI RegSaveKey32W( HKEY hkey, LPCWSTR lpFile, 
                           LPSECURITY_ATTRIBUTES sa )
{
    LPKEYSTRUCT	lpkey;

    TRACE(reg, "(%x,%s,%p)\n", hkey, debugstr_w(lpFile), sa);

    /* It appears to do this check before the hkey check */
    if (!lpFile || !*lpFile)
        return ERROR_INVALID_PARAMETER;

    lpkey = lookup_hkey( hkey );
    if (!lpkey)
        return ERROR_INVALID_HANDLE;

    FIXME(reg, "(%x,%s,%p): stub\n", hkey, debugstr_w(lpFile), sa);

    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegSaveKey32A [ADVAPI32.165]
 */
LONG WINAPI RegSaveKey32A( HKEY hkey, LPCSTR lpFile, 
                           LPSECURITY_ATTRIBUTES sa )
{
    LONG ret;
    LPWSTR lpFileW = strdupA2W(lpFile);
    ret = RegSaveKey32W( hkey, lpFileW, sa );
    free(lpFileW);
    return ret;
}


/******************************************************************************
 * RegRestoreKey32W [ADVAPI32.164]
 *
 * PARAMS
 *    hkey    [I] Handle of key where restore begins
 *    lpFile  [I] Address of filename containing saved tree
 *    dwFlags [I] Optional flags
 */
LONG WINAPI RegRestoreKey32W( HKEY hkey, LPCWSTR lpFile, DWORD dwFlags )
{
    LPKEYSTRUCT	lpkey;

    TRACE(reg, "(%x,%s,%ld)\n",hkey,debugstr_w(lpFile),dwFlags);

    /* It seems to do this check before the hkey check */
    if (!lpFile || !*lpFile)
        return ERROR_INVALID_PARAMETER;

    lpkey = lookup_hkey( hkey );
    if (!lpkey)
        return ERROR_INVALID_HANDLE;

    FIXME(reg,"(%x,%s,%ld): stub\n",hkey,debugstr_w(lpFile),dwFlags);

    /* Check for file existence */

    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegRestoreKey32A [ADVAPI32.163]
 */
LONG WINAPI RegRestoreKey32A( HKEY hkey, LPCSTR lpFile, DWORD dwFlags )
{
    LONG ret;
    LPWSTR lpFileW = strdupA2W(lpFile);
    ret = RegRestoreKey32W( hkey, lpFileW, dwFlags );
    if(lpFileW) free(lpFileW);
    return ret;
}


/******************************************************************************
 * RegReplaceKey32W [ADVAPI32.162]
 *
 * PARAMS
 *    hkey      [I] Handle of open key
 *    lpSubKey  [I] Address of name of subkey
 *    lpNewFile [I] Address of filename for file with new data
 *    lpOldFile [I] Address of filename for backup file
 */
LONG WINAPI RegReplaceKey32W( HKEY hkey, LPCWSTR lpSubKey, LPCWSTR lpNewFile,
                              LPCWSTR lpOldFile )
{
    LPKEYSTRUCT	lpkey;

    TRACE(reg,"(%x,%s,%s,%s)\n",hkey,debugstr_w(lpSubKey),
          debugstr_w(lpNewFile),debugstr_w(lpOldFile));

    lpkey = lookup_hkey( hkey );
    if (!lpkey)
        return ERROR_INVALID_HANDLE;

    FIXME(reg, "(%x,%s,%s,%s): stub\n", hkey, debugstr_w(lpSubKey), 
          debugstr_w(lpNewFile),debugstr_w(lpOldFile));

    return ERROR_SUCCESS;
}


/******************************************************************************
 * RegReplaceKey32A [ADVAPI32.161]
 */
LONG WINAPI RegReplaceKey32A( HKEY hkey, LPCSTR lpSubKey, LPCSTR lpNewFile,
                              LPCSTR lpOldFile )
{
    LONG ret;
    LPWSTR lpSubKeyW = strdupA2W(lpSubKey);
    LPWSTR lpNewFileW = strdupA2W(lpNewFile);
    LPWSTR lpOldFileW = strdupA2W(lpOldFile);
    ret = RegReplaceKey32W( hkey, lpSubKeyW, lpNewFileW, lpOldFileW );
    free(lpOldFileW);
    free(lpNewFileW);
    free(lpSubKeyW);
    return ret;
}

