#ifndef __SETUPX_PRIVATE_H
#define __SETUPX_PRIVATE_H

#include "wine/windef16.h"

typedef struct tagLDD_LIST {
        LPLOGDISKDESC pldd;
        struct tagLDD_LIST *next;
} LDD_LIST;

#define INIT_LDD(ldd, LDID) \
  do { \
    memset(&(ldd), 0, sizeof(LOGDISKDESC_S)); \
   (ldd).cbSize = sizeof(LOGDISKDESC_S); \
   ldd.ldid = LDID; \
  } while(0)

typedef struct {
    HINF16 hInf;
    HFILE hInfFile;
    LPSTR lpInfFileName;
} INF_FILE;

extern INF_FILE *InfList;
extern WORD InfNumEntries;

extern LPCSTR IP_GetFileName(HINF16 hInf);

#endif /* __SETUPX_PRIVATE_H */
