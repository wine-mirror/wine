/*
 *  Shell UID
 *
 *  1998 Juergen Schmied (jsch)  *  <juergen.schmied@metronet.de>
 *
 *  this in in a single file due to interfering definitions
 *
 */
#define INITGUID

#include "shlwapi.h"
/* #include "shlguid.h" */

/*
 * Francis Beaudet <francis@macadamian.com> 
 *
 * I moved the contents of this file to the ole/guid.c file.
 *
 * I know that the purpose of this file being here is that it would
 * separate the definitions reserved to the shell DLL into one place. *
 However, until the shell DLL is a real DLL, as long as it is *
 statically linked with the rest of wine, the initializer of it's * GUIDs
 will have to be in the same place as everybody else. This is * the same
 problem as with "real" Windows programs. */
