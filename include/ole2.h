/*
 *	ole2.h - Declarations for OLE2
 */

#ifndef __WINE_OLE2_H
#define __WINE_OLE2_H

/* to be implemented */
/* FIXME: this should be defined somewhere in oleidl.h instead, should it be repeated here ? */
typedef LPVOID LPDROPTARGET;


/* OLE version */
#define rmm             23
#define rup            639

/* FIXME: should be in oleidl.h */
typedef struct  tagOleMenuGroupWidths
{ LONG width[ 6 ];
} OLEMENUGROUPWIDTHS32;

typedef struct tagOleMenuGroupWidths *LPOLEMENUGROUPWIDTHS32;

typedef HGLOBAL32 HOLEMENU32;

#endif  /* __WINE_OLE2_H */
