/*
 * typelib.h  internal wine data structures
 * used to decode typelib's
 *
 * Copyright 1999 Rein KLazes
 *  
 */
#ifndef _WINE_TYPELIB_H
#define _WINE_TYPELIB_H

#include "oleauto.h"
#include "wine/windef16.h"

#define TLBMAGIC2 "MSFT"
#define TLBMAGIC1 "SLTG"
#define HELPDLLFLAG (0x0100)
#define DO_NOT_SEEK (-1)

#define HREFTYPE_INTHISFILE(href) (!((href) & 3))
#define HREFTYPE_INDEX(href) ((href) /sizeof(TLBTypeInfoBase))

/*-------------------------FILE STRUCTURES-----------------------------------*/


/*
 * structure of the typelib type2 header
 * it is at the beginning of a type lib file
 *  
 */
typedef struct tagTLB2Header {
/*0x00*/INT magic1;       /* 0x5446534D "MSFT" */
        INT   magic2;       /* 0x00010002 version nr? */
        INT   posguid;      /* position of libid in guid table  */
                            /* (should be,  else -1) */
        INT   lcid;         /* locale id */
/*0x10*/INT   lcid2;
        INT   varflags;     /* (largely) unknown flags ,seems to be always 41 */
                            /* becomes 0x51 with a helpfile defined */
                            /* if help dll defined its 0x151 */
                            /* update : the lower nibble is syskind */
        INT   version;      /* set with SetVersion() */
        INT   flags;        /* set with SetFlags() */
/*0x20*/INT   nrtypeinfos;  /* number of typeinfo's (till so far) */
        INT   helpstring;   /* position of help string in stringtable */
        INT   helpstringcontext;
        INT   helpcontext;
/*0x30*/INT   nametablecount;   /* number of names in name table */
        INT   nametablechars;   /* nr of characters in name table */
        INT   NameOffset;       /* offset of name in name table */
        INT   helpfile;         /* position of helpfile in stringtable */
/*0x40*/INT   CustomDataOffset; /* if -1 no custom data, else it is offset */
                                /* in customer data/guid offset table */
        INT   res44;            /* unknown always: 0x20 */
        INT   res48;            /* unknown always: 0x80 */
        INT   dispatchpos;      /* gets a value (1+n*0x0c) with Idispatch interfaces */
/*0x50*/INT   res50;            /* is zero becomes one when an interface is derived */
} TLB2Header;

/* segments in the type lib file have a structure like this: */
typedef struct _tptag {
        INT   offset;       /* absolute offset in file */
        INT   length;       /* length of segment */
        INT   res08;        /* unknown always -1 */
        INT   res0c;        /* unknown always 0x0f in the header */
                            /* 0x03 in the typeinfo_data */
} pSeg;

/* layout of the main segment directory */
typedef struct tagTLBSegDir {
/*1*/pSeg pTypeInfoTab; /* each type info get an entry of 0x64 bytes */
                        /* (25 ints) */
/*2*/pSeg pImpInfo;     /* table with info for imported types */
/*3*/pSeg pImpFiles;    /* import libaries */
/*4*/pSeg pRefTab;      /* References table */
/*5*/pSeg pLibtab;      /* always exists, alway same size (0x80) */
                        /* hash table w offsets to guid????? */
/*6*/pSeg pGuidTab;     /* all guids are stored here together with  */
                        /* offset in some table???? */
/*7*/pSeg res07;        /* always created, alway same size (0x200) */
                        /* purpose largely unknown */
/*8*/pSeg pNametab;     /* name tables */
/*9*/pSeg pStringtab;   /*string table */
/*A*/pSeg pTypdescTab;  /* table with type descriptors */
/*B*/pSeg pArrayDescriptions;
/*C*/pSeg pCustData;    /*  data table, used for custom data and default */
                        /* parameter values */
/*D*/pSeg pCDGuids;     /* table with offsets for the guids and into the customer data table */
/*E*/pSeg res0e;            /* unknown */
/*F*/pSeg res0f;            /* unknown  */
} TLBSegDir;


/* base type info data */
typedef struct tagTLBTypeInfoBase {
/*000*/ INT   typekind;             /*  it is the TKIND_xxx */
                                    /* some byte alignment stuf */
        INT     memoffset;          /* points past the file, if no elements */
        INT     res2;               /* zero if no element, N*0x40 */
        INT     res3;               /* -1 if no lement, (N-1)*0x38 */
/*010*/ INT     res4;               /* always? 3 */
        INT     res5;               /* always? zero */
        INT     cElement;           /* counts elements, HI=cVars, LO=cFuncs */
        INT     res7;               /* always? zero */
/*020*/ INT     res8;               /* always? zero */
        INT     res9;               /* always? zero */
        INT     resA;               /* always? zero */
        INT     posguid;            /* position in guid table */
/*030*/ INT     flags;              /* Typeflags */
        INT     NameOffset;         /* offset in name table */
        INT     version;            /* element version */
        INT     docstringoffs;      /* offset of docstring in string tab */
/*040*/ INT     helpstringcontext;  /*  */
        INT     helpcontext;    /* */
        INT     oCustData;          /* offset in customer data table */
        INT16   cImplTypes;     /* nr of implemented interfaces */
        INT16   cbSizeVft;      /* virtual table size, not including inherits */
/*050*/ INT     size;           /* size in bytes, at least for structures */
        /* fixme: name of this field */
        INT     datatype1;      /* position in type description table */
                                /* or in base intefaces */
                                /* if coclass: offset in reftable */
                                /* if interface: reference to inherited if */
        INT     datatype2;      /* if 0x8000, entry above is valid */
                                /* actually dunno */
                                /* else it is zero? */
        INT     res18;          /* always? 0 */
/*060*/ INT     res19;          /* always? -1 */
    } TLBTypeInfoBase;

/* layout of an entry with information on imported types */
typedef struct tagTLBImpInfo {
    INT     res0;           /* unknown */
    INT     oImpFile;       /* offset inthe Import File table */
    INT     oGuid;          /* offset in Guid table */
    } TLBImpInfo;

/* function description data */
typedef struct {
/*  INT   recsize;       record size including some xtra stuff */
    INT   DataType;     /* data type of the memeber, eg return of function */
    INT   Flags;        /* something to do with attribute flags (LOWORD) */
    INT16 VtableOffset; /* offset in vtable */
    INT16 res3;         /* some offset into dunno what */
    INT   FKCCIC;       /* bit string with the following  */
                        /* meaning (bit 0 is the msb): */
                        /* bit 2 indicates that oEntry is numeric */
                        /* bit 3 that parameter has default values */
                        /* calling convention (bits 4-7 ) */
                        /* bit 8 indicates that custom data is present */
                        /* Invokation kind (bits 9-12 ) */
                        /* function kind (eg virtual), bits 13-15  */
    INT16 nrargs;       /* number of arguments (including optional ????) */
    INT16 nroargs;      /* nr of optional arguments */
    /* optional attribute fields, the number of them is variable */
    INT   OptAttr[1];
/*
0*  INT   helpcontext;
1*  INT   oHelpString;
2*  INT   oEntry;       // either offset in string table or numeric as it is //
3*  INT   res9;         // unknown (-1) //
4*  INT   resA;         // unknown (-1) //
5*  INT   HelpStringContext;
    // these are controlled by a bit set in the FKCCIC field  //
6*  INT   oCustData;        // custom data for function //
7*  INT   oArgCustData[1];  // custom data per argument //
*/
} TLBFuncRecord;

/* after this may follow an array with default value pointers if the 
 * appropriate bit in the FKCCIC field has been set: 
 * INT   oDefautlValue[nrargs];
 */

    /* Parameter info one per argument*/
typedef struct {
        INT   DataType;
        INT   oName;
        INT   Flags;
    } TLBParameterInfo;

/* Variable description data */
typedef struct {
/*  INT   recsize;      // record size including some xtra stuff */
    INT   DataType;     /* data type of the variable */
    INT   Flags;        /* VarFlags (LOWORD) */
    INT16 VarKind;      /* VarKind */
    INT16 res3;         /* some offset into dunno what */
    INT   OffsValue;    /* value of the variable or the offset  */
                        /* in the data structure */
    /* optional attribute fields, the number of them is variable */
    /* controlled by record length */
    INT   HelpContext;
    INT   oHelpString;
    INT   res9;         /* unknown (-1) */
    INT   oCustData;        /* custom data for variable */
    INT   HelpStringContext;

} TLBVarRecord;

/* Structure of the reference data  */
typedef struct {
    INT   reftype;  /* either offset in type info table, then its */
                    /* a multiple of 64 */
                    /* or offset in the external reference table */
                    /* with an offset of 1 */
    INT   flags;
    INT   oCustData;    /* custom data */
    INT   onext;    /* next offset, -1 if last */
} TLBRefRecord;

/* this is how a guid is stored */
typedef struct {
    GUID guid;
    INT   unk10;        /* differntiate with libid, classid etc? */
                        /* its -2 for a libary */
                        /* it's 0 for an interface */
    INT   unk14;        /* always? -1 */
} TLBGuidEntry;
/* some data preceding entries in the name table */
typedef struct {
    INT   unk00;        /* sometimes -1 (lib, parameter) ,
                           sometimes 0 (interface, func) */
    INT   unk10;        /* sometimes -1 (lib) , sometimes 0 (interface, func),
                           sometime 0x10 (par) */
    INT   namelen;      /* only lower 8 bits are valid */
} TLBNameIntro;
/* the custom data table directory has enties like this */
typedef struct {
    INT   GuidOffset;
    INT   DataOffset;
    INT   next;     /* next offset in the table, -1 if its the last */
} TLBCDGuid;



/*---------------------------END--------------------------------------------*/
#endif
