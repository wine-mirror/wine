/*************************************************************************
**
**    olestd.h
**
*************************************************************************/

#if !defined( __WINE_OLESTD_H_ )
#define __WINE_OLESTD_H_

#if !defined(__cplusplus) && !defined( __TURBOC__)
#define NONAMELESSUNION     /* use strict ANSI standard (for DVOBJ.H) */
#endif

/* Clipboard format strings */
#define CF_EMBEDSOURCE      "Embed Source"
#define CF_EMBEDDEDOBJECT   "Embedded Object"
#define CF_LINKSOURCE       "Link Source"
#define CF_CUSTOMLINKSOURCE "Custom Link Source"
#define CF_OBJECTDESCRIPTOR "Object Descriptor"
#define CF_LINKSRCDESCRIPTOR "Link Source Descriptor"
#define CF_OWNERLINK        "OwnerLink"
#define CF_FILENAME         "FileName"

#define OleStdQueryOleObjectData(lpformatetc)   \
   (((lpformatetc)->tymed & TYMED_ISTORAGE) ?    \
         NOERROR : ResultFromScode(DV_E_FORMATETC))

#define OleStdQueryLinkSourceData(lpformatetc)   \
   (((lpformatetc)->tymed & TYMED_ISTREAM) ?    \
         NOERROR : ResultFromScode(DV_E_FORMATETC))

#define OleStdQueryObjectDescriptorData(lpformatetc)    \
   (((lpformatetc)->tymed & TYMED_HGLOBAL) ?    \
         NOERROR : ResultFromScode(DV_E_FORMATETC))

#define OleStdQueryFormatMedium(lpformatetc, tymd)  \
   (((lpformatetc)->tymed & tymd) ?    \
         NOERROR : ResultFromScode(DV_E_FORMATETC))

/* Make an independent copy of a MetafilePict */
#define OleStdCopyMetafilePict(hpictin, phpictout)  \
   (*(phpictout) = OleDuplicateData(hpictin,CF_METAFILEPICT,GHND|GMEM_SHARE))

#endif /* __WINE_OLESTD_H_ */
