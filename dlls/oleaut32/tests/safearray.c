/*
 * SafeArray test program
 *
 * Copyright 2002 Marcus Meissner
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <time.h>

#include "wine/test.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "winnls.h"
#include "winerror.h"
#include "winnt.h"

#include "wtypes.h"
#include "oleauto.h"

#define VARTYPE_NOT_SUPPORTED 0
static struct {
	enum VARENUM vt; /* VT */
	UINT elemsize; /* elementsize by VT */
	UINT expflags; /* fFeatures from SafeArrayAllocDescriptorEx */
	UINT addflags; /* additional fFeatures from SafeArrayCreate */
} vttypes[] = {
{VT_EMPTY,    VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_NULL,     VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_I2,       2,                    FADF_HAVEVARTYPE,0},
{VT_I4,       4,                    FADF_HAVEVARTYPE,0},
{VT_R4,       4,                    FADF_HAVEVARTYPE,0},
{VT_R8,       8,                    FADF_HAVEVARTYPE,0},
{VT_CY,       8,                    FADF_HAVEVARTYPE,0},
{VT_DATE,     8,                    FADF_HAVEVARTYPE,0},
{VT_BSTR,     sizeof(BSTR),         FADF_HAVEVARTYPE,FADF_BSTR},
{VT_DISPATCH, sizeof(LPDISPATCH),   FADF_HAVEIID,    FADF_DISPATCH},
{VT_ERROR,    4,                    FADF_HAVEVARTYPE,0},
{VT_BOOL,     2,                    FADF_HAVEVARTYPE,0},
{VT_VARIANT,  sizeof(VARIANT),      FADF_HAVEVARTYPE,FADF_VARIANT},
{VT_UNKNOWN,  sizeof(LPUNKNOWN),    FADF_HAVEIID,    FADF_UNKNOWN},
{VT_DECIMAL,  sizeof(DECIMAL),      FADF_HAVEVARTYPE,0},
{15,          VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0}, /* no VT_xxx */
{VT_I1,       1,	            FADF_HAVEVARTYPE,0},
{VT_UI1,      1,		    FADF_HAVEVARTYPE,0},
{VT_UI2,      2,                    FADF_HAVEVARTYPE,0},
{VT_UI4,      4,                    FADF_HAVEVARTYPE,0},
{VT_I8,       VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_UI8,      VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_INT,      sizeof(INT),          FADF_HAVEVARTYPE,0},
{VT_UINT,     sizeof(UINT),         FADF_HAVEVARTYPE,0},
{VT_VOID,     VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_HRESULT,  VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_PTR,      VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_SAFEARRAY,VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_CARRAY,   VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_USERDEFINED,VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_LPSTR,    VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_LPWSTR,   VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_FILETIME, VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_RECORD,   VARTYPE_NOT_SUPPORTED,FADF_RECORD,0},
{VT_BLOB,     VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_STREAM,   VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_STORAGE,  VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_STREAMED_OBJECT,VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_STORED_OBJECT,VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_BLOB_OBJECT,VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_CF,       VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
{VT_CLSID,    VARTYPE_NOT_SUPPORTED,FADF_HAVEVARTYPE,0},
};

START_TEST(safearray)
{
	SAFEARRAY 	*a, b, *c;
	unsigned int 	i;
	HRESULT 	hres;
	SAFEARRAYBOUND	bound;
	VARIANT		v;
	LPVOID		data;
	IID		iid;
	VARTYPE		vt;

	hres = SafeArrayAllocDescriptor(0,&a);
	ok(E_INVALIDARG == hres,"SAAD(0) failed with hres %lx",hres);

	hres=SafeArrayAllocDescriptor(1,&a);
	ok(S_OK == hres,"SAAD(1) failed with %lx",hres);

	for (i=1;i<100;i++) {
		hres=SafeArrayAllocDescriptor(i,&a);
		ok(S_OK == hres,"SAAD(%d) failed with %lx",i,hres);
		
		ok(a->cDims == i,"a->cDims not initialised?");

		hres=SafeArrayDestroyDescriptor(a);
		ok(S_OK == hres,"SADD failed with %lx",hres);
	}

	hres=SafeArrayAllocDescriptor(65535,&a);
	ok(S_OK == hres,"SAAD(65535) failed with %lx",hres);

	hres=SafeArrayDestroyDescriptor(a);
	ok(S_OK == hres,"SADD failed with %lx",hres);

	hres=SafeArrayAllocDescriptor(65536,&a);
	ok(E_INVALIDARG == hres,"SAAD(65536) failed with %lx",hres);

	hres=SafeArrayAllocDescriptor(1,NULL);
	ok(E_POINTER == hres,"SAAD(1,NULL) failed with %lx",hres);

	
	bound.cElements	= 1;
	bound.lLbound	= 0;
	a = SafeArrayCreate(-1, 1, &bound);
	ok(NULL == a,"SAC(-1,1,[1,0]) not failed?");
	
	for (i=0;i<sizeof(vttypes)/sizeof(vttypes[0]);i++) {
		a = SafeArrayCreate(vttypes[i].vt, 1, &bound);
		ok(	((a == NULL) && (vttypes[i].elemsize == 0)) ||
			((a != NULL) && (vttypes[i].elemsize == a->cbElements)),
		"SAC(%d,1,[1,0]), result %ld, expected %d",vttypes[i].vt,(a?a->cbElements:0),vttypes[i].elemsize
		);
		if (a!=NULL)
			ok(a->fFeatures == (vttypes[i].expflags | vttypes[i].addflags),"SAC of %d returned feature flags %x, expected %x", vttypes[i].vt, a->fFeatures, vttypes[i].expflags|vttypes[i].addflags);
		ok(SafeArrayGetElemsize(a) == vttypes[i].elemsize,"SAGE for vt %d returned elemsize %d instead of expected %d",vttypes[i].vt, SafeArrayGetElemsize(a),vttypes[i].elemsize);

		if (!a) continue;

		hres = SafeArrayGetVartype(a, &vt);
		ok(hres == S_OK, "SAGVT of array with vt %d failed with %lx", vttypes[i].vt, hres);
		if (vttypes[i].vt == VT_DISPATCH) {
			/* Special case. Checked against Windows. */
			ok(vt == VT_UNKNOWN, "SAGVT of array with VT_DISPATCH returned not VT_UNKNOWN, but %d", vt);
		} else {
			ok(vt == vttypes[i].vt, "SAGVT of array with vt %d returned %d", vttypes[i].vt, vt);
		}

		hres = SafeArrayCopy(a, &c);
		ok(hres == S_OK, "failed to copy safearray of vt %d with hres %lx", vttypes[i].vt, hres);

		ok(vttypes[i].elemsize == c->cbElements,"copy of SAC(%d,1,[1,0]), result %ld, expected %d",vttypes[i].vt,(c?c->cbElements:0),vttypes[i].elemsize
		);
		ok(c->fFeatures == (vttypes[i].expflags | vttypes[i].addflags),"SAC of %d returned feature flags %x, expected %x", vttypes[i].vt, c->fFeatures, vttypes[i].expflags|vttypes[i].addflags);
		ok(SafeArrayGetElemsize(c) == vttypes[i].elemsize,"SAGE for vt %d returned elemsize %d instead of expected %d",vttypes[i].vt, SafeArrayGetElemsize(c),vttypes[i].elemsize);

		hres = SafeArrayGetVartype(c, &vt);
		ok(hres == S_OK, "SAGVT of array with vt %d failed with %lx", vttypes[i].vt, hres);
		if (vttypes[i].vt == VT_DISPATCH) {
			/* Special case. Checked against Windows. */
			ok(vt == VT_UNKNOWN, "SAGVT of array with VT_DISPATCH returned not VT_UNKNOWN, but %d", vt);
		} else {
			ok(vt == vttypes[i].vt, "SAGVT of array with vt %d returned %d", vttypes[i].vt, vt);
		}
		hres = SafeArrayCopyData(a, c);
		ok(hres == S_OK, "failed to copy safearray data of vt %d with hres %lx", vttypes[i].vt, hres);

		hres = SafeArrayDestroyData(c);
		ok(hres == S_OK,"SADD of copy of array with vt %d failed with hres %lx", vttypes[i].vt, hres);

		hres = SafeArrayDestroy(a);
		ok(hres == S_OK,"SAD of array with vt %d failed with hres %lx", vttypes[i].vt, hres);
	}

	/* Test conversion of type|VT_ARRAY <-> VT_BSTR */
	bound.lLbound = 0;
	bound.cElements = 10;
	a = SafeArrayCreate(VT_UI1, 1, &bound);
	ok(a != NULL, "SAC failed.");
	ok(S_OK == SafeArrayAccessData(a, &data),"SACD failed");
	memcpy(data,"Hello World",10);
	ok(S_OK == SafeArrayUnaccessData(a),"SAUD failed");
	V_VT(&v) = VT_ARRAY|VT_UI1;
	V_ARRAY(&v) = a;
	hres = VariantChangeTypeEx(&v, &v, 0, 0, VT_BSTR);
	ok(hres==S_OK, "CTE VT_ARRAY|VT_UI1 -> VT_BSTR failed with %lx",hres);
	ok(V_VT(&v) == VT_BSTR,"CTE VT_ARRAY|VT_UI1 -> VT_BSTR did not return VT_BSTR, but %d.",V_VT(&v));
	ok(V_BSTR(&v)[0] == 0x6548,"First letter are not 'He', but %x", V_BSTR(&v)[0]);

	/* check locking functions */
	a = SafeArrayCreate(VT_I4, 1, &bound);
	ok(a!=NULL,"SAC should not fail");

	hres = SafeArrayAccessData(a, &data);
	ok(hres == S_OK,"SAAD failed with hres %lx",hres);

	hres = SafeArrayDestroy(a);
	ok(hres == DISP_E_ARRAYISLOCKED,"locked safe array destroy not failed with DISP_E_ARRAYISLOCKED, but with hres %lx", hres);

	hres = SafeArrayDestroyData(a);
	ok(hres == DISP_E_ARRAYISLOCKED,"locked safe array destroy data not failed with DISP_E_ARRAYISLOCKED, but with hres %lx", hres);

	hres = SafeArrayDestroyDescriptor(a);
	ok(hres == DISP_E_ARRAYISLOCKED,"locked safe array destroy descriptor not failed with DISP_E_ARRAYISLOCKED, but with hres %lx", hres);

	hres = SafeArrayUnaccessData(a);
	ok(hres == S_OK,"SAUD failed after lock/destroy test");

	hres = SafeArrayDestroy(a);
	ok(hres == S_OK,"SAD failed after lock/destroy test");

	/* Test if we need to destroy data before descriptor */
	a = SafeArrayCreate(VT_I4, 1, &bound);
	ok(a!=NULL,"SAC should not fail");
	hres = SafeArrayDestroyDescriptor(a);
	ok(hres == S_OK,"SADD with data in array failed with hres %lx",hres);


	/* IID functions */
	/* init a small stack safearray */
	memset(&b, 0, sizeof(b));
	b.cDims = 1;
	memset(&iid, 0x42, sizeof(IID));
	hres = SafeArraySetIID(&b,&iid);
	ok(hres == E_INVALIDARG,"SafeArraySetIID of non IID capable safearray did not return E_INVALIDARG, but %lx",hres);

	hres = SafeArrayAllocDescriptor(1,&a);
	ok((a->fFeatures & FADF_HAVEIID) == 0,"newly allocated descriptor with SAAD should not have FADF_HAVEIID");
	hres = SafeArraySetIID(a,&iid);
	ok(hres == E_INVALIDARG,"SafeArraySetIID of newly allocated descriptor with SAAD should return E_INVALIDARG, but %lx",hres);

	for (i=0;i<sizeof(vttypes)/sizeof(vttypes[0]);i++) {
		hres = SafeArrayAllocDescriptorEx(vttypes[i].vt,1,&a);
		ok(a->fFeatures == vttypes[i].expflags,"SAADE(%d) resulted with flags %x, expected %x\n", vttypes[i].vt, a->fFeatures, vttypes[i].expflags);
		if (a->fFeatures & FADF_HAVEIID) {
			hres = SafeArrayGetIID(a, &iid);
			ok(hres == S_OK,"SAGIID failed for vt %d with hres %lx", vttypes[i].vt,hres);
			switch (vttypes[i].vt) {
			case VT_UNKNOWN:
				ok(IsEqualGUID(((GUID*)a)-1,&IID_IUnknown),"guid for VT_UNKNOWN is not IID_IUnknown");
				ok(IsEqualGUID(&iid, &IID_IUnknown),"SAGIID returned wrong GUID for IUnknown");
				break;
			case VT_DISPATCH:
				ok(IsEqualGUID(((GUID*)a)-1,&IID_IDispatch),"guid for VT_UNKNOWN is not IID_IDispatch");
				ok(IsEqualGUID(&iid, &IID_IDispatch),"SAGIID returned wrong GUID for IDispatch");
				break;
			default:
				ok(FALSE,"unknown vt %d with FADF_HAVEIID",vttypes[i].vt);
				break;
			}
		} else {
			hres = SafeArrayGetIID(a, &iid);
			ok(hres == E_INVALIDARG,"SAGIID did not fail for vt %d with hres %lx", vttypes[i].vt,hres);
		}
		if (a->fFeatures & FADF_RECORD) {
			ok(vttypes[i].vt == VT_RECORD,"FADF_RECORD for non record %d",vttypes[i].vt);
		}
		if (a->fFeatures & FADF_HAVEVARTYPE) {
			ok(vttypes[i].vt == ((DWORD*)a)[-1], "FADF_HAVEVARTYPE set, but vt %d mismatch stored %ld",vttypes[i].vt,((DWORD*)a)[-1]);
		}

		hres = SafeArrayGetVartype(a, &vt);
		ok(hres == S_OK, "SAGVT of array with vt %d failed with %lx", vttypes[i].vt, hres);

		if (vttypes[i].vt == VT_DISPATCH) {
			/* Special case. Checked against Windows. */
			ok(vt == VT_UNKNOWN, "SAGVT of array with VT_DISPATCH returned not VT_UNKNOWN, but %d", vt);
		} else {
			ok(vt == vttypes[i].vt, "SAGVT of array with vt %d returned %d", vttypes[i].vt, vt);
		}

		if (a->fFeatures & FADF_HAVEIID) {
			hres = SafeArraySetIID(a, &IID_IStorage); /* random IID */
			ok(hres == S_OK,"SASIID failed with FADF_HAVEIID set for vt %d with %lx", vttypes[i].vt, hres);
			hres = SafeArrayGetIID(a, &iid);
			ok(hres == S_OK,"SAGIID failed with FADF_HAVEIID set for vt %d with %lx", vttypes[i].vt, hres);
			ok(IsEqualGUID(&iid, &IID_IStorage),"returned iid is not IID_IStorage");
		} else {
			hres = SafeArraySetIID(a, &IID_IStorage); /* random IID */
			ok(hres == E_INVALIDARG,"SASIID did not failed with !FADF_HAVEIID set for vt %d with %lx", vttypes[i].vt, hres);
		}
		hres = SafeArrayDestroyDescriptor(a);
		ok(hres == S_OK,"SADD failed with hres %lx",hres);
	}
}
