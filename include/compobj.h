#if !defined(COMPOBJ_H)
#define COMPOBJ_H

#include "ole.h"

struct tagGUID
{
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[8];
};

typedef struct tagGUID	GUID;
typedef struct tagGUID	CLSID,*LPCLSID,*REFCLSID;
typedef struct tagGUID	IID,*REFIID,*LPIID;

OLESTATUS WINAPI StringFromCLSID(const CLSID *id, LPSTR);
OLESTATUS WINAPI CLSIDFromString(const LPCSTR, CLSID *);


#ifdef INITGUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        const GUID name =\
	{ l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    extern const GUID name
#endif

#define DEFINE_OLEGUID(name, l, w1, w2) \
	DEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

#define DEFINE_SHLGUID(name, l, w1, w2) DEFINE_OLEGUID(name,l,w1,w2)
#endif
