/*
 * Module/Library loadorder
 *
 * Copyright 1999 Bertho Stultiens
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "windef.h"
#include "options.h"
#include "loadorder.h"
#include "heap.h"
#include "module.h"
#include "elfdll.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(module)


/* #define DEBUG_LOADORDER */

#define LOADORDER_ALLOC_CLUSTER	32	/* Allocate with 32 entries at a time */

static module_loadorder_t default_loadorder;
static module_loadorder_t *module_loadorder = NULL;
static int nmodule_loadorder = 0;
static int nmodule_loadorder_alloc = 0;

static struct tagDllOverride {
	char *key,*value;
} DefaultDllOverrides[] = {
	{"kernel32,gdi32,user32",	"builtin"},
	{"krnl386,gdi,user",		"builtin"},
	{"toolhelp",			"builtin"},
	{"comdlg32,commdlg",		"elfdll,builtin,native"},
	{"version,ver",			"elfdll,builtin,native"},
	{"shell32,shell",		"builtin,native"},
	{"shlwapi",			"native,builtin"},
	{"lz32,lzexpand",		"builtin,native"},
	{"commctrl,comctl32",		"builtin,native"},
	{"wsock32,winsock",		"builtin"},
	{"ws2_32",			"builtin"},
	{"advapi32,crtdll,ntdll",	"builtin,native"},
	{"mpr,winspool.drv",		"builtin,native"},
	{"ddraw,dinput,dsound",		"builtin,native"},
	{"winmm, mmsystem",		"builtin"},
	{"msvideo, msvfw32",		"builtin, native"},
	{"mcicda.drv, mciseq.drv",	"builtin, native"},
	{"mciwave.drv",			"builtin, native"},
	{"mciavi.drv, mcianim.drv",	"native, builtin"},
	{"msacm.drv, midimap.drv",      "builtin, native"},
	{"w32skrnl",			"builtin"},
	{"wnaspi32,wow32",		"builtin"},
	{"system,display,wprocs	",	"builtin"},
	{"wineps",			"builtin"},
        {"icmp",                        "builtin"},
	/* we have to use libglide2x.so instead of glide2x.dll ... */
	{"glide2x",			"so,native"},
	{"glide3x",			"so,native"},
	{"odbc32",			"builtin"},
	{"opengl32",                    "builtin,native"},
	{"shfolder",                    "builtin,native"},
	{"rpcrt4",                      "native,builtin"},
	{NULL,NULL},
};

static const struct tagDllPair {
    const char *dll1, *dll2;
} DllPairs[] = {
    { "krnl386",  "kernel32" },
    { "gdi",      "gdi32" },
    { "user",     "user32" },
    { "commdlg",  "comdlg32" },
    { "commctrl", "comctl32" },
    { "ver",      "version" },
    { "shell",    "shell32" },
    { "lzexpand", "lz32" },
    { "mmsystem", "winmm" },
    { "msvideo",  "msvfw32" },
    { "winsock",  "wsock32" },
    { NULL,       NULL }
};

/***************************************************************************
 *	cmp_sort_func	(internal, static)
 *
 * Sorting and comparing function used in sort and search of loadorder
 * entries.
 */
static int cmp_sort_func(const void *s1, const void *s2)
{
	return strcasecmp(((module_loadorder_t *)s1)->modulename, ((module_loadorder_t *)s2)->modulename);
}


/***************************************************************************
 *	get_tok	(internal, static)
 *
 * strtok wrapper for non-destructive buffer writing.
 * NOTE: strtok is not reentrant and therefore this code is neither.
 */
static char *get_tok(const char *str, const char *delim)
{
	static char *buf = NULL;
	char *cptr;

	if(!str && !buf)
		return NULL;

	if(str && buf)
	{
		HeapFree(GetProcessHeap(), 0, buf);
		buf = NULL;
	}

	if(str && !buf)
	{
		buf = HEAP_strdupA(GetProcessHeap(), 0, str);
		cptr = strtok(buf, delim);
	}
	else
	{
		cptr = strtok(NULL, delim);
	}

	if(!cptr)
	{
		HeapFree(GetProcessHeap(), 0, buf);
		buf = NULL;
	}
	return cptr;
}


/***************************************************************************
 *	ParseLoadOrder	(internal, static)
 *
 * Parses the loadorder options from the configuration and puts it into
 * a structure.
 */
static BOOL ParseLoadOrder(char *order, module_loadorder_t *mlo)
{
	char *cptr;
	int n = 0;

	memset(mlo->loadorder, 0, sizeof(mlo->loadorder));

	cptr = get_tok(order, ", \t");
	while(cptr)
	{
		char type = MODULE_LOADORDER_INVALID;

		if(n >= MODULE_LOADORDER_NTYPES)
		{
			ERR("More than existing %d module-types specified, rest ignored", MODULE_LOADORDER_NTYPES);
			break;
		}

		switch(*cptr)
		{
		case 'N':	/* Native */
		case 'n': type = MODULE_LOADORDER_DLL; break;

		case 'E':	/* Elfdll */
		case 'e': type = MODULE_LOADORDER_ELFDLL; break;

		case 'S':	/* So */
		case 's': type = MODULE_LOADORDER_SO; break;

		case 'B':	/* Builtin */
		case 'b': type = MODULE_LOADORDER_BI; break;

		default:
			ERR("Invalid load order module-type '%s', ignored\n", cptr);
		}

		if(type != MODULE_LOADORDER_INVALID)
		{
			mlo->loadorder[n++] = type;
		}
		cptr = get_tok(NULL, ", \t");
	}
	return TRUE;
}


/***************************************************************************
 *	AddLoadOrder	(internal, static)
 *
 * Adds an entry in the list of overrides. If the entry exists, then the
 * override parameter determines whether it will be overwritten.
 */
static BOOL AddLoadOrder(module_loadorder_t *plo, BOOL override)
{
	int i;

	/* TRACE(module, "'%s' -> %08lx\n", plo->modulename, *(DWORD *)(plo->loadorder)); */

	for(i = 0; i < nmodule_loadorder; i++)
	{
		if(!cmp_sort_func(plo, &module_loadorder[i]))
		{
			if(!override)
				ERR("Module '%s' is already in the list of overrides, using first definition\n", plo->modulename);
			else
				memcpy(module_loadorder[i].loadorder, plo->loadorder, sizeof(plo->loadorder));
			return TRUE;
		}
	}

	if(nmodule_loadorder >= nmodule_loadorder_alloc)
	{
		/* No space in current array, make it larger */
		nmodule_loadorder_alloc += LOADORDER_ALLOC_CLUSTER;
		module_loadorder = (module_loadorder_t *)HeapReAlloc(GetProcessHeap(),
								     0,
								     module_loadorder,
								     nmodule_loadorder_alloc * sizeof(module_loadorder_t));
		if(!module_loadorder)
		{
			MESSAGE("Virtual memory exhausted\n");
			exit(1);
		}
	}
	memcpy(module_loadorder[nmodule_loadorder].loadorder, plo->loadorder, sizeof(plo->loadorder));
	module_loadorder[nmodule_loadorder].modulename = HEAP_strdupA(GetProcessHeap(), 0, plo->modulename);
	nmodule_loadorder++;
	return TRUE;
}


/***************************************************************************
 *	AddLoadOrderSet	(internal, static)
 *
 * Adds a set of entries in the list of overrides from the key parameter.
 * If the entry exists, then the override parameter determines whether it
 * will be overwritten.
 */
static BOOL AddLoadOrderSet(char *key, char *order, BOOL override)
{
	module_loadorder_t ldo;
	char *cptr;

	/* Parse the loadorder before the rest because strtok is not reentrant */
	if(!ParseLoadOrder(order, &ldo))
		return FALSE;

	cptr = get_tok(key, ", \t");
	while(cptr)
	{
		char *ext = strrchr(cptr, '.');
		if(ext)
		{
			if(strlen(ext) == 4 && (!strcasecmp(ext, ".dll") || !strcasecmp(ext, ".exe")))
				MESSAGE("Warning: Loadorder override '%s' contains an extension and might not be found during lookup\n", cptr);
		}

		ldo.modulename = cptr;
		if(!AddLoadOrder(&ldo, override))
			return FALSE;
		cptr = get_tok(NULL, ", \t");
	}
	return TRUE;
}


/***************************************************************************
 *	ParseCommandlineOverrides	(internal, static)
 *
 * The commandline is in the form:
 * name[,name,...]=native[,b,...][:...]
 */
static BOOL ParseCommandlineOverrides(void)
{
	char *cpy;
	char *key;
	char *next;
	char *value;
	BOOL retval = TRUE;

	if(!Options.dllFlags)
		return TRUE;

	cpy = HEAP_strdupA(GetProcessHeap(), 0, Options.dllFlags);
	key = cpy;
	next = key;
	for(; next; key = next)
	{
		next = strchr(key, ':');
		if(next)
		{
			*next = '\0';
			next++;
		}
		value = strchr(key, '=');
		if(!value)
		{
			retval = FALSE;
			goto endit;
		}
		*value = '\0';
		value++;

		TRACE("Commandline override '%s' = '%s'\n", key, value);
		
		if(!AddLoadOrderSet(key, value, TRUE))
		{
			retval = FALSE;
			goto endit;
		}
	}
endit:
	HeapFree(GetProcessHeap(), 0, cpy);
	return retval;;
}


/***************************************************************************
 *	MODULE_InitLoadOrder	(internal)
 *
 * Initialize the load order from the wine.conf file.
 * The section has the following format:
 * Section:
 *	[DllDefaults]
 *
 * Keys:
 *	EXTRA_LD_LIBRARY_PATH=/usr/local/lib/wine[:/more/path/to/search[:...]]
 * The path will be appended to any existing LD_LIBRARY_PATH from the 
 * environment (see note in code below).
 *
 *	DefaultLoadOrder=native,elfdll,so,builtin
 * A comma separated list of module types to try to load in that specific
 * order. The DefaultLoadOrder key is used as a fallback when a module is
 * not specified explicitly. If the DefaultLoadOrder key is not found, 
 * then the order "dll,elfdll,so,bi" is used
 * The possible module types are:
 *	- native	Native windows dll files
 *	- elfdll	Dlls encapsulated in .so libraries
 *	- so		Native .so libraries mapped to dlls
 *	- builtin	Built-in modules
 *
 * Case is not important and only the first letter of each type is enough to
 * identify the type n[ative], e[lfdll], s[o], b[uiltin]. Also whitespace is
 * ignored.
 * E.g.:
 * 	n,el	,s , b
 * is equal to:
 *	native,elfdll,so,builtin
 *
 * Section:
 *	[DllOverrides]
 *
 * Keys:
 * There are no explicit keys defined other than module/library names. A comma
 * separated list of modules is followed by an assignment of the load-order
 * for these specific modules. See above for possible types. You should not
 * specify an extension.
 * Examples:
 * kernel32, gdi32, user32 = builtin
 * kernel, gdi, user = builtin
 * comdlg32 = elfdll, native, builtin
 * commdlg = native, builtin
 * version, ver = elfdll, native, builtin
 *
 */

#define BUFFERSIZE	1024

BOOL MODULE_InitLoadOrder(void)
{
	char buffer[BUFFERSIZE];
	char key[256];
	int nbuffer;
        int idx;
        const struct tagDllPair *dllpair;

#if defined(HAVE_DL_API)
	/* Get/set the new LD_LIBRARY_PATH */
	nbuffer = PROFILE_GetWineIniString("DllDefaults", "EXTRA_LD_LIBRARY_PATH", "", buffer, sizeof(buffer));

	if(nbuffer)
	{
		extra_ld_library_path = HEAP_strdupA(GetProcessHeap(), 0, buffer);
		TRACE("Setting extra LD_LIBRARY_PATH=%s\n", buffer);
	}
#endif

	/* Get the default load order */
	nbuffer = PROFILE_GetWineIniString("DllDefaults", "DefaultLoadOrder", "n,b,e,s", buffer, sizeof(buffer));
	if(!nbuffer)
	{
		MESSAGE("MODULE_InitLoadOrder: mysteriously read nothing from default loadorder\n");
		return FALSE;
	}

	TRACE("Setting default loadorder=%s\n", buffer);

	if(!ParseLoadOrder(buffer, &default_loadorder))
		return FALSE;
	default_loadorder.modulename = "<none>";

	{
	    int i;
	    for (i=0;DefaultDllOverrides[i].key;i++)
		AddLoadOrderSet(
		    DefaultDllOverrides[i].key,
		    DefaultDllOverrides[i].value,
		    FALSE
		);
	}

	/* Read the explicitely defined orders for specific modules as an entire section */
        idx = 0;
        while (PROFILE_EnumWineIniString( "DllOverrides", idx++, key, sizeof(key),
                                          buffer, sizeof(buffer)))
        {
            TRACE("Key '%s' uses override '%s'\n", key, buffer);
            if(!AddLoadOrderSet(key, buffer, TRUE))
                return FALSE;
        }

	/* Add the commandline overrides to the pool */
	if(!ParseCommandlineOverrides())
	{
		MESSAGE(	"Syntax: -dll name[,name[,...]]={native|elfdll|so|builtin}[,{n|e|s|b}[,...]][:...]\n"
			"    - 'name' is the name of any dll without extension\n"
			"    - the order of loading (native, elfdll, so and builtin) can be abbreviated\n"
			"      with the first letter\n"
			"    - different loadorders for different dlls can be specified by seperating the\n"
			"      commandline entries with a ':'\n"
			"    Example:\n"
			"    -dll comdlg32,commdlg=n:shell,shell32=b\n"
		   );
		return FALSE;
	}

	/* Sort the array for quick lookup */
	qsort(module_loadorder, nmodule_loadorder, sizeof(module_loadorder[0]), cmp_sort_func);

	/* Check the pairs of dlls */
        dllpair = DllPairs;
        while (dllpair->dll1)
        {
            module_loadorder_t *plo1, *plo2;
            plo1 = MODULE_GetLoadOrder(dllpair->dll1);
            plo2 = MODULE_GetLoadOrder(dllpair->dll2);
            assert(plo1 && plo2);
            if(memcmp(plo1->loadorder, plo2->loadorder, sizeof(plo1->loadorder)))
                MESSAGE("Warning: Modules '%s' and '%s' have different loadorder which may cause trouble\n", dllpair->dll1, dllpair->dll2);
            dllpair++;
        }

	if(TRACE_ON(module))
	{
		int i, j;
		static char types[6] = "-NESB";

		for(i = 0; i < nmodule_loadorder; i++)
		{
			DPRINTF("%3d: %-12s:", i, module_loadorder[i].modulename);
			for(j = 0; j < MODULE_LOADORDER_NTYPES; j++)
				DPRINTF(" %c", types[module_loadorder[i].loadorder[j] % (MODULE_LOADORDER_NTYPES+1)]);
			DPRINTF("\n");
		}
	}

	return TRUE;
}


/***************************************************************************
 *	MODULE_GetLoadOrder	(internal)
 *
 * Locate the loadorder of a module.
 * Any path is stripped from the path-argument and so are the extension
 * '.dll' and '.exe'. A lookup in the table can yield an override for
 * the specific dll. Otherwise the default load order is returned.
 */
module_loadorder_t *MODULE_GetLoadOrder(const char *path)
{
	module_loadorder_t lo, *tmp;
	char fname[256];
	char *cptr;
	char *name;
	int len;

	TRACE("looking for %s\n", path);
	
	assert(path != NULL);

	/* Strip path information */
	cptr = strrchr(path, '\\');
	if(!cptr)
		name = strrchr(path, '/');
	else
		name = strrchr(cptr, '/');

	if(!name)
		name = cptr ? cptr+1 : (char *)path;
	else
		name++;

	if((cptr = strchr(name, ':')) != NULL)	/* Also strip drive if in format 'C:MODULE.DLL' */
		name = cptr+1;

	len = strlen(name);
	if(len >= sizeof(fname) || len <= 0)
	{
		ERR("Path '%s' -> '%s' reduces to zilch or just too large...\n", path, name);
		return &default_loadorder;
	}

	strcpy(fname, name);
	if(len >= 4 && (!lstrcmpiA(fname+len-4, ".dll") || !lstrcmpiA(fname+len-4, ".exe")))
		fname[len-4] = '\0';

	lo.modulename = fname;
	tmp = bsearch(&lo, module_loadorder, nmodule_loadorder, sizeof(module_loadorder[0]), cmp_sort_func);

	TRACE("Looking for '%s' (%s), found '%s'\n", path, fname, tmp ? tmp->modulename : "<nothing>");

	if(!tmp)
		return &default_loadorder;
	return tmp;
}

