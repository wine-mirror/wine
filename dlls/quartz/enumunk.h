/*
 * Implementation of IEnumUnknown (for internal use).
 *
 * hidenori@a2.ctktv.ne.jp
 */

#ifndef	QUARTZ_ENUMUNK_H
#define	QUARTZ_ENUMUNK_H

#include "complist.h"

HRESULT QUARTZ_CreateEnumUnknown(
	REFIID riidEnum, void** ppobj, const QUARTZ_CompList* pCompList );


#endif	/* QUARTZ_ENUMUNK_H */
