#define INITGUID

/* FIXME: we include all the header files containing GUIDs
 * so that the corresponding variables get defined. But they
 * don't even all belong to the same DLL !!!
 *
 * Therefore, as the builtin DLL's get teased apart (e.g. for elf-dlls)
 * then this file will have to be partitioned into per dll files.
 */
#include "initguid.h"

#include "shlguid.h"
#include "docobj.h"
#include "olectl.h"
#include "oleidl.h"
#include "oaidl.h"
#include "ocidl.h"
#include "objbase.h"
#include "servprov.h"
#include "ddraw.h"
#include "d3d.h"
#include "dinput.h"
#include "dsound.h"
#include "dplay.h"
#include "dplobby.h"
#include "vfw.h"
#include "shlobj.h"

/* and now for the one assumed GUID... */
DEFINE_GUID(GUID_NULL,   0,0,0,0,0,0,0,0,0,0,0);
