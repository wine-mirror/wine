/* DCI driver interface */

#ifndef __WINE_DCIDDI_H
#define __WINE_DCIDDI_H

#ifdef __cplusplus
extern "C" {
#endif

/* DCI Command Escape */
#define DCICOMMAND	3075
#define DCI_VERSION	0x0100

#define DCICREATEPRIMARYSURFACE		1
#define DCICREATEOFFSCREENSURFACE	2
#define DCICREATEOVERLAYSURFACE		3
#define DCIENUMSURFACE			4
#define DCIESCAPE			5

/* DCI Errors */
#define DCI_OK		0


typedef int DCIRVAL; /* DCI callback return type */

/*****************************************************************************
 * Escape command structures
 */
typedef struct _DCICMD {
    DWORD dwCommand;
    DWORD dwParam1;
    DWORD dwParam2;
    DWORD dwVersion;
    DWORD dwReserved;
} DCICMD,*LPDCICMD;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __WINE_DCIDDI_H */
