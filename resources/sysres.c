/*
 * System resources loading
 *
 * Copyright 1995 Alexandre Julliard
 */

#include "winbase.h"
#include "wine/winbase16.h"
#include "global.h"
#include "options.h"
#include "resource.h"
#include "wrc_rsc.h"

extern const wrc_resource32_t * const sysres_En_ResTable[];
extern const wrc_resource32_t * const sysres_Es_ResTable[];
extern const wrc_resource32_t * const sysres_De_ResTable[];
extern const wrc_resource32_t * const sysres_No_ResTable[];
extern const wrc_resource32_t * const sysres_Fr_ResTable[];
extern const wrc_resource32_t * const sysres_Fi_ResTable[];
extern const wrc_resource32_t * const sysres_Da_ResTable[];
extern const wrc_resource32_t * const sysres_Cz_ResTable[];
extern const wrc_resource32_t * const sysres_Eo_ResTable[];
extern const wrc_resource32_t * const sysres_It_ResTable[];
extern const wrc_resource32_t * const sysres_Ko_ResTable[];
extern const wrc_resource32_t * const sysres_Hu_ResTable[];
extern const wrc_resource32_t * const sysres_Pl_ResTable[];
extern const wrc_resource32_t * const sysres_Po_ResTable[];
extern const wrc_resource32_t * const sysres_Sw_ResTable[];
extern const wrc_resource32_t * const sysres_Ca_ResTable[];
extern const wrc_resource32_t * const sysres_Nl_ResTable[];

static const wrc_resource32_t * const * SYSRES_Resources[] =
{
    sysres_En_ResTable,  /* LANG_En */
    sysres_Es_ResTable,  /* LANG_Es */
    sysres_De_ResTable,  /* LANG_De */
    sysres_No_ResTable,  /* LANG_No */
    sysres_Fr_ResTable,  /* LANG_Fr */
    sysres_Fi_ResTable,  /* LANG_Fi */
    sysres_Da_ResTable,  /* LANG_Da */
    sysres_Cz_ResTable,  /* LANG_Cz */
    sysres_Eo_ResTable,  /* LANG_Eo */
    sysres_It_ResTable,  /* LANG_It */
    sysres_Ko_ResTable,  /* LANG_Ko */
    sysres_Hu_ResTable,  /* LANG_Hu */
    sysres_Pl_ResTable,  /* LANG_Pl */
    sysres_Po_ResTable,  /* LANG_Po */
    sysres_Sw_ResTable,  /* LANG_Sw */
    sysres_Ca_ResTable,  /* LANG_Ca */
    sysres_Nl_ResTable   /* LANG_Nl */
};


/***********************************************************************
 *           SYSRES_GetResourcePtr
 *
 * Return a pointer to a system resource.
 */
LPCVOID SYSRES_GetResPtr( SYSTEM_RESOURCE id )
{
    return SYSRES_Resources[Options.language][id]->data;
}


/***********************************************************************
 *           SYSRES_LoadResource
 *
 * Create a global memory block for a system resource.
 */
HGLOBAL16 SYSRES_LoadResource( SYSTEM_RESOURCE id )
{
    const wrc_resource32_t *resPtr;

    resPtr = SYSRES_Resources[Options.language][id];
    return GLOBAL_CreateBlock( GMEM_FIXED, resPtr->data, resPtr->datasize,
			       GetCurrentPDB16(), FALSE, FALSE, TRUE, NULL );
}


/***********************************************************************
 *           SYSRES_FreeResource
 *
 * Free a global memory block for a system resource.
 */
void SYSRES_FreeResource( HGLOBAL16 handle )
{
    GLOBAL_FreeBlock( handle );
}
