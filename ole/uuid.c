/*
 * GUID definitions
 */

#include "initguid.h"

/* GUIDs defined in uuids.lib */

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "objbase.h"
#include "servprov.h"

#include "oleauto.h"
#include "oleidl.h"
#include "objidl.h"
#include "olectl.h"

#include "ocidl.h"

#include "docobj.h"

#include "shlguid.h"
#include "shlobj.h"
#include "wine/obj_queryassociations.h"

/* FIXME: cguids declares GUIDs but does not define their values */



/* GUIDs defined in dxguid.lib */

#include "d3d.h"
#include "ddraw.h"
#include "dsound.h"
#include "dplay.h"
#include "dplobby.h"
#include "dinput.h"

#include "ddrawi.h"

/* other GUIDs */

#include "vfw.h"

/* for dshow */
#include "strmif.h"
#include "uuids.h"

/* GUIDs not declared in an exported header file */
DEFINE_GUID(IID_IDirectPlaySP,0xc9f6360,0xcc61,0x11cf,0xac,0xec,0x00,0xaa,0x00,0x68,0x86,0xe3);
DEFINE_GUID(IID_ISFHelper,0x1fe68efb,0x1874,0x9812,0x56,0xdc,0x00,0x00,0x00,0x00,0x00,0x00);
DEFINE_GUID(IID_IDPLobbySP,0x5a4e5a20,0x2ced,0x11d0,0xa8,0x89,0x00,0xa0,0xc9,0x05,0x43,0x3c);
