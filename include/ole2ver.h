/*
 *  ole2ver.h - Version number info
 */

#ifndef __WINE_OLE2VER_H
#define __WINE_OLE2VER_H

/*
 * other versions rmm/rup:
 * 23/639
 * 23/700
 * 23/730
 * 23/824
 *
 * Win98 SE original files:
 * COMPOBJ: CoBuildVersion 23/700
 * OLE2: OleBuildVersion -> COMPOBJ.CoBuildVersion
 * OLE32: CoBuildVersion and Ole~ 23/824 
 *
 * We probably should reorganize the OLE version stuff, i.e.
 * use different values for every *BuildVersion function and Win version.
 */

/* bad: we shouldn't make use of it that globally ! */
#define rmm             23
#define rup		824

#endif  /* __WINE_OLE2VER_H */
