/*
 *	ole2.h - Declarations for OLE2
 */

#ifndef __WINE_OLE2_H
#define __WINE_OLE2_H

/* to be implemented */
typedef LPVOID LPMESSAGEFILTER;
typedef LPVOID LPDROPTARGET;
typedef struct tagMONIKER *LPMONIKER, IMoniker;

#define S_OK	0
#define S_FALSE	1

/* OLE version */
#define rmm             23
#define rup            639

/* FIXME should be in oleidl.h*/
typedef struct  tagOleMenuGroupWidths
{ LONG width[ 6 ];
} OLEMENUGROUPWIDTHS32;

typedef struct tagOleMenuGroupWidths *LPOLEMENUGROUPWIDTHS32;

typedef HGLOBAL32 HOLEMENU32;

#endif  /* __WINE_OLE2_H */
