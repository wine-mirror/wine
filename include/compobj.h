#if !defined(COMPOBJ_H)
#define COMPOBJ_H

struct tagCLSID {
  DWORD	Data1;
  WORD	Data2;
  WORD  Data3;
  BYTE	Data4[8];
};

typedef struct tagCLSID CLSID;

OLESTATUS StringFromCLSID(const CLSID *id, LPSTR);
OLESTATUS CLSIDFromString(const LPCSTR, CLSID *);

#endif
