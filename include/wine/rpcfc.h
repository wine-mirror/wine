/*
 * RPC format chars, as found by studying MIDL output.
 */

#ifndef __WINE_RPCFC_H
#define __WINE_RPCFC_H

/* base types */
#define RPC_FC_BYTE			0x01
#define RPC_FC_CHAR			0x02
#define RPC_FC_SMALL			0x03
#define RPC_FC_USMALL			0x04
#define RPC_FC_WCHAR			0x05
#define RPC_FC_SHORT			0x06
#define RPC_FC_USHORT			0x07
#define RPC_FC_LONG			0x08
#define RPC_FC_ULONG			0x09
#define RPC_FC_FLOAT			0x0a
#define RPC_FC_HYPER			0x0b
#define RPC_FC_DOUBLE			0x0c

#define RPC_FC_ENUM32			0x0e

/* other stuff */
#define RPC_FC_RP			0x11 /* ? */
#define RPC_FC_UP			0x12 /* unique pointer */
#define RPC_FC_OP			0x13 /* ? */
#define RPC_FC_FP			0x14 /* full pointer */
/* FC_RP/UP/OP/FP: flags, NdrFcShort(typeofs) */
 #define RPC_FC_P_ONSTACK		0x4 /* [alloced_on_stack] */
 #define RPC_FC_P_SIMPLEPOINTER		0x8 /* [simple_pointer] */
 /* flag 10 is not tagged */

#define RPC_FC_STRUCT			0x15
/* FC_STRUCT: fieldcount-1, NdrFcShort(size), fields */

#define RPC_FC_PSTRUCT			0x16
#define RPC_FC_CSTRUCT			0x17

#define RPC_FC_BOGUS_STRUCT		0x1a

#define RPC_FC_CARRAY			0x1b /* conformant array? */
#define RPC_FC_CVARRAY			0x1c /* conformant varying array? */
#define RPC_FC_SMFARRAY			0x1d /* simple fixed array? */
/* FC_SMFARRAY: fieldcount-1, NdrFcShort(count), type */

#define RPC_FC_BOGUS_ARRAY		0x21

#define RPC_FC_C_CSTRING		0x22
#define RPC_FC_C_WSTRING		0x25

#define RPC_FC_ENCAPSULATED_UNION	0x2a
#define RPC_FC_NON_ENCAPSULATED_UNION	0x2b

#define RPC_FC_IP			0x2f /* interface pointer */

#define RPC_FC_AUTO_HANDLE		0x33
/* FC_AUTO_HANDLE: oldflags, NdrFcLong(?), NdrFcShort(vtbl_idx), NdrFcShort(stacksiz),
 *                 NdrFcShort(?), NdrFcShort(?), oi2flags, parmcount
 * parameters: NdrFcShort(flags), NdrFcShort(stackofs), NdrFcShort(typeofs)/basetype */
 /* oldflags: 6c = object + oi2 */
 #define RPC_FC_AH_OI2F_SRVMUSTSIZE	0x01
 #define RPC_FC_AH_OI2F_CLTMUSTSIZE	0x02
 #define RPC_FC_AH_OI2F_HASRETURN	0x04
 #define RPC_FC_AH_PF_IN		0x0008
 #define RPC_FC_AH_PF_OUT		0x0010
 #define RPC_FC_AH_PF_RETURN		0x0020
 #define RPC_FC_AH_PF_BASETYPE		0x0040
 #define RPC_FC_AH_PF_BYVAL		0x0080
 #define RPC_FC_AH_PF_SIMPLEREF		0x0100
 /* PF: 03 = mustsize + mustfree */
 /* 2000 = srv alloc size=8, 4000 = srv alloc size=16 */

#define RPC_FC_POINTER			0x36

#define RPC_FC_ALIGNM4			0x38
#define RPC_FC_ALIGNM8			0x39

#define RPC_FC_STRUCTPAD2		0x3e

#define RPC_FC_NO_REPEAT		0x46

#define RPC_FC_VARIABLE_REPEAT		0x48
#define RPC_FC_FIXED_OFFSET		0x49

#define RPC_FC_PP			0x4b

#define RPC_FC_EMBEDDED_COMPLEX		0x4c
/* FC_EMBEDDED_COMPLEX: fieldcount-1, NdrFcShort(typeofs) */

#define RPC_FC_IN_PARAM			0x4d
/* FC_IN_PARAM: stacksiz, NdrFcShort(typeofs) */
#define RPC_FC_IN_PARAM_BASETYPE	0x4e
/* FC_IN_PARAM_BASETYPE: basetype */
#define RPC_FC_OUT_PARAM		0x51
/* FC_OUT_PARAM: stacksiz, NdrFcShort(typeofs) */
#define RPC_FC_RETURN_PARAM_BASETYPE	0x53
/* FC_RETURN_PARAM_BASETYPE: basetype */

#define RPC_FC_DEREFERENCE		0x54

#define RPC_FC_CONSTANT_IID		0x5a
/* FC_CONSTANT_IID: NdrFcLong(), NdrFcShort(), NdrFcShort(), 8x () */

#define RPC_FC_END			0x5b
#define RPC_FC_PAD			0x5c

#define RPC_FC_USER_MARSHAL		0xb4

#endif /* __WINE_RPCFC_H */
