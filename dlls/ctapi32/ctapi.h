/*
 * CT-API library for the REINER SCT cyberJack pinpad/e-com USB.
 * Copyright (C) 2001  REINER SCT
 * Author: Matthias Bruestle
 * Support: support@reiner-sct.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef H_CTAPI
#define H_CTAPI

/* More unique defines */

#define CT_API_AD_HOST		2
#define CT_API_AD_REMOTE	5

#define CT_API_AD_CT		1
#define CT_API_AD_ICC1		0
#define CT_API_AD_ICC2		2
#define CT_API_AD_ICC3		3
#define CT_API_AD_ICC4		4
#define CT_API_AD_ICC5		5
#define CT_API_AD_ICC6		6
#define CT_API_AD_ICC7		7
#define CT_API_AD_ICC8		8
#define CT_API_AD_ICC9		9
#define CT_API_AD_ICC10		10
#define CT_API_AD_ICC11		11
#define CT_API_AD_ICC12		12
#define CT_API_AD_ICC13		13
#define CT_API_AD_ICC14		14

#define CT_API_RV_OK		0
#define CT_API_RV_ERR_INVALID	-1
#define CT_API_RV_ERR_CT	-8
#define CT_API_RV_ERR_TRANS	-10
#define CT_API_RV_ERR_MEMORY	-11
#define CT_API_RV_ERR_HOST	-127
#define CT_API_RV_ERR_HTSI	-128

/* MUSCLE style defines */

#define OK			 0	/* Success */
#define ERR_INVALID		-1	/* Invalid Data */
#define ERR_CT			-8	/* CT Error */
#define ERR_TRANS		-10	/* Transmission Error */
#define ERR_MEMORY		-11	/* Memory Allocate Error */
#define ERR_HOST		-127	/* Host Error */
#define ERR_HTSI		-128	/* HTSI Error */

#define PORT_COM1		0	/* COM 1 */
#define PORT_COM2		1	/* COM 2 */
#define PORT_COM3		2	/* COM 3 */
#define PORT_COM4		3	/* COM 4 */
#define PORT_Printer		4	/* Printer Port (MAC) */
#define PORT_Modem		5	/* Modem Port (MAC)   */
#define PORT_LPT1		6	/* LPT 1 */
#define PORT_LPT2		7	/* LPT 2 */

#define CT			1
#define HOST			2

/* Short */
#define CJ_CTAPI_MAX_LENC	4+1+255+1
#define CJ_CTAPI_MAX_LENR	256+2
/* Extended */
/* #define CJ_CTAPI_MAX_LENC	5+2+65535+2 */
/* #define CJ_CTAPI_MAX_LENR	65536+2 */
/* Maximum for CTAPI */
/* #define CJ_CTAPI_MAX_LENC	65535 */
/* #define CJ_CTAPI_MAX_LENR	65535 */


typedef unsigned char IU8;
typedef unsigned short IU16;

typedef signed char IS8;
typedef signed short IS16;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

IS8 CT_init( IU16 ctn, IU16 pn );
IS8 CT_data( IU16 ctn, IU8 *dad, IU8 *sad, IU16 lenc, IU8 *command, IU16 *lenr,
	IU8 *response );
IS8 CT_close( IU16 ctn );

/* Proprietary extension */
IS8 CT_keycb( IU16 ctn, void (* cb)(IU16 ctn, IU8 status) );

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* H_CTAPI */
