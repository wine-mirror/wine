/* A few helpful macros for implementing COM objects.
 *
 * Copyright 2000 TransGaming Technologies Inc.
 */

#include <stddef.h>

/* Generates the name for a vtable pointer for a given interface. */
/* The canonical name for a single interface is "lpVtbl". */
#define ICOM_VFIELD_MULTI_NAME2(iface) ITF_##iface
#define ICOM_VFIELD_MULTI_NAME(iface) ICOM_VFIELD_MULTI_NAME2(iface)

/* Declares a vtable pointer field in an implementation. */
#define ICOM_VFIELD_MULTI(iface) \
	iface ICOM_VFIELD_MULTI_NAME(iface)

/* Returns the offset of a vtable pointer within an implementation object. */
#define ICOM_VFIELD_OFFSET(impltype, iface) \
	offsetof(impltype, ICOM_VFIELD_MULTI_NAME(iface))

/* Given an interface pointer, returns the implementation pointer. */
#define ICOM_OBJECT(impltype, ifacename, ifaceptr)		\
	(impltype*)((ifaceptr) == NULL ? NULL			\
		  : (char*)(ifaceptr) - ICOM_VFIELD_OFFSET(impltype,ifacename))

#define ICOM_THIS_FROM(impltype, ifacename, ifaceptr) \
	impltype* This = ICOM_OBJECT(impltype, ifacename, ifaceptr)

/* Given an object and interface name, returns a pointer to that interface. */
#define ICOM_INTERFACE(implobj, iface) \
	(&((implobj)->ICOM_VFIELD_MULTI_NAME(iface)))

#define ICOM_INIT_INTERFACE(implobj, ifacename, vtblname) \
	do { \
	  (implobj)->ICOM_VFIELD_MULTI_NAME(ifacename).lpVtbl = &(vtblname); \
	} while (0)

#define COM_INTERFACE_CAST(impltype, ifnamefrom, ifnameto, ifaceptr)	\
	ICOM_INTERFACE(ICOM_OBJECT(impltype, ifnamefrom, ifaceptr), ifnameto)
