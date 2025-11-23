/*
 * Copyright 2008 James Hawkins
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

#ifndef __WINE_CORHDR_H
#define __WINE_CORHDR_H

typedef enum CorTokenType
{
    mdtModule                   = 0x00000000,
    mdtTypeRef                  = 0x01000000,
    mdtTypeDef                  = 0x02000000,
    mdtFieldDef                 = 0x04000000,
    mdtMethodDef                = 0x06000000,
    mdtParamDef                 = 0x08000000,
    mdtInterfaceImpl            = 0x09000000,
    mdtMemberRef                = 0x0a000000,
    mdtCustomAttribute          = 0x0c000000,
    mdtPermission               = 0x0e000000,
    mdtSignature                = 0x11000000,
    mdtEvent                    = 0x14000000,
    mdtProperty                 = 0x17000000,
    mdtModuleRef                = 0x1a000000,
    mdtTypeSpec                 = 0x1b000000,
    mdtAssembly                 = 0x20000000,
    mdtAssemblyRef              = 0x23000000,
    mdtFile                     = 0x26000000,
    mdtExportedType             = 0x27000000,
    mdtManifestResource         = 0x28000000,
    mdtGenericParam             = 0x2a000000,
    mdtMethodSpec               = 0x2b000000,
    mdtGenericParamConstraint   = 0x2c000000,
    mdtString                   = 0x70000000,
    mdtName                     = 0x71000000,
    mdtBaseType                 = 0x72000000,
} CorTokenType;

typedef enum CorElementType
{
    ELEMENT_TYPE_END            = 0x00,
    ELEMENT_TYPE_VOID           = 0x01,
    ELEMENT_TYPE_BOOLEAN        = 0x02,
    ELEMENT_TYPE_CHAR           = 0x03,
    ELEMENT_TYPE_I1             = 0x04,
    ELEMENT_TYPE_U1             = 0x05,
    ELEMENT_TYPE_I2             = 0x06,
    ELEMENT_TYPE_U2             = 0x07,
    ELEMENT_TYPE_I4             = 0x08,
    ELEMENT_TYPE_U4             = 0x09,
    ELEMENT_TYPE_I8             = 0x0a,
    ELEMENT_TYPE_U8             = 0x0b,
    ELEMENT_TYPE_R4             = 0x0c,
    ELEMENT_TYPE_R8             = 0x0d,
    ELEMENT_TYPE_STRING         = 0x0e,
    ELEMENT_TYPE_PTR            = 0x0f,
    ELEMENT_TYPE_BYREF          = 0x10,
    ELEMENT_TYPE_VALUETYPE      = 0x11,
    ELEMENT_TYPE_CLASS          = 0x12,
    ELEMENT_TYPE_VAR            = 0x13,
    ELEMENT_TYPE_ARRAY          = 0x14,
    ELEMENT_TYPE_GENERICINST    = 0x15,
    ELEMENT_TYPE_TYPEDBYREF     = 0x16,
    ELEMENT_TYPE_I              = 0x18,
    ELEMENT_TYPE_U              = 0x19,
    ELEMENT_TYPE_FNPTR          = 0x1b,
    ELEMENT_TYPE_OBJECT         = 0x1c,
    ELEMENT_TYPE_SZARRAY        = 0x1d,
    ELEMENT_TYPE_MVAR           = 0x1e,
    ELEMENT_TYPE_CMOD_REQD      = 0x1f,
    ELEMENT_TYPE_CMOD_OPT       = 0x20,
    ELEMENT_TYPE_INTERNAL       = 0x21,
    ELEMENT_TYPE_MAX            = 0x22,
    ELEMENT_TYPE_MODIFIER       = 0x40,
    ELEMENT_TYPE_SENTINEL       = 0x01 | ELEMENT_TYPE_MODIFIER,
    ELEMENT_TYPE_PINNED         = 0x05 | ELEMENT_TYPE_MODIFIER,
    ELEMENT_TYPE_R4_HFA         = 0x06 | ELEMENT_TYPE_MODIFIER,
    ELEMENT_TYPE_R8_HFA         = 0x07 | ELEMENT_TYPE_MODIFIER,

} CorElementType;

#define RidToToken(rid,tktype) ((rid) |= (tktype))
#define TokenFromRid(rid,tktype) ((rid) | (tktype))
#define RidFromToken(tk) ((RID)((tk) & 0x00ffffff))
#define TypeFromToken(tk) ((ULONG32)((tk) & 0xff000000))
#define IsNilToken(tk) ((RidFromToken(tk)) == 0)

typedef ULONG RID;

typedef LPVOID  mdScope;
typedef ULONG32 mdToken;

typedef mdToken mdModule;
typedef mdToken mdTypeRef;
typedef mdToken mdTypeDef;
typedef mdToken mdFieldDef;
typedef mdToken mdMethodDef;
typedef mdToken mdParamDef;
typedef mdToken mdInterfaceImpl;
typedef mdToken mdMemberRef;
typedef mdToken mdCustomAttribute;
typedef mdToken mdPermission;
typedef mdToken mdSignature;
typedef mdToken mdEvent;
typedef mdToken mdProperty;
typedef mdToken mdModuleRef;
typedef mdToken mdAssembly;
typedef mdToken mdAssemblyRef;
typedef mdToken mdFile;
typedef mdToken mdExportedType;
typedef mdToken mdManifestResource;
typedef mdToken mdTypeSpec;
typedef mdToken mdGenericParam;
typedef mdToken mdMethodSpec;
typedef mdToken mdGenericParamConstraint;
typedef mdToken mdString;
typedef mdToken mdCPToken;

#define mdTokenNil                  ((mdToken)0)
#define mdModuleNil                 ((mdModule)mdtModule)
#define mdTypeRefNil                ((mdTypeRef)mdtTypeRef)
#define mdTypeDefNil                ((mdTypeDef)mdtTypeDef)
#define mdFieldDefNil               ((mdFieldDef)mdtFieldDef)
#define mdMethodDefNil              ((mdMethodDef)mdtMethodDef)
#define mdParamDefNil               ((mdParamDef)mdtParamDef)
#define mdInterfaceImplNil          ((mdInterfaceImpl)mdtInterfaceImpl)
#define mdMemberRefNil              ((mdMemberRef)mdtMemberRef)
#define mdCustomAttributeNil        ((mdCustomAttribute)mdtCustomAttribute)
#define mdPermissionNil             ((mdPermission)mdtPermission)
#define mdSignatureNil              ((mdSignature)mdtSignature)
#define mdEventNil                  ((mdEvent)mdtEvent)
#define mdPropertyNil               ((mdProperty)mdtProperty)
#define mdModuleRefNil              ((mdModuleRef)mdtModuleRef)
#define mdTypeSpecNil               ((mdTypeSpec)mdtTypeSpec)
#define mdAssemblyNil               ((mdAssembly)mdtAssembly)
#define mdAssemblyRefNil            ((mdAssemblyRef)mdtAssemblyRef)
#define mdFileNil                   ((mdFile)mdtFile)
#define mdExportedTypeNil           ((mdExportedType)mdtExportedType)
#define mdManifestResourceNil       ((mdManifestResource)mdtManifestResource)
#define mdGenericParamNil           ((mdGenericParam)mdtGenericParam)
#define mdGenericParamConstraintNil ((mdGenericParamConstraint)mdtGenericParamConstraint)
#define mdMethodSpecNil             ((mdMethodSpec)mdtMethodSpec)
#define mdStringNil                 ((mdString)mdtString)

typedef enum CorTypeAttr
{
    tdNotPublic          = 0x000000,
    tdPublic             = 0x000001,
    tdNestedPublic       = 0x000002,
    tdNestedPrivate      = 0x000003,
    tdNestedFamily       = 0x000004,
    tdNestedAssembly     = 0x000005,
    tdNestedFamANDAssem  = 0x000006,
    tdNestedFamORAssem   = 0x000007,
    tdAutoLayout         = 0x000000,
    tdSequentialLayout   = 0x000008,
    tdExplicitLayout     = 0x000010,
    tdExtendedLayout     = 0x000018,
    tdClass              = 0x000000,
    tdInterface          = 0x000020,
    tdAbstract           = 0x000080,
    tdSealed             = 0x000100,
    tdSpecialName        = 0x000400,
    tdImport             = 0x001000,
    tdSerializable       = 0x002000,
    tdWindowsRuntime     = 0x004000,
    tdAnsiClass          = 0x000000,
    tdUnicodeClass       = 0x010000,
    tdAutoClass          = 0x020000,
    tdCustomFormatClass  = 0x030000,
    tdBeforeFieldInit    = 0x100000,
    tdForwarder          = 0x200000,
    tdRTSpecialName      = 0x000800,
    tdHasSecurity        = 0x040000,

    tdVisibilityMask     = 0x000007,
    tdLayoutMask         = 0x000018,
    tdClassSemanticsMask = 0x000020,
    tdStringFormatMask   = 0x030000,
    tdCustomFormatMask   = 0xc00000,
    tdReservedMask       = 0x040800
} CorTypeAttr;

typedef enum CorMethodAttr
{
    mdPrivateScope          =   0x0000,
    mdPrivate               =   0x0001,
    mdFamANDAssem           =   0x0002,
    mdAssem                 =   0x0003,
    mdFamily                =   0x0004,
    mdFamORAssem            =   0x0005,
    mdPublic                =   0x0006,
    mdStatic                =   0x0010,
    mdFinal                 =   0x0020,
    mdVirtual               =   0x0040,
    mdHideBySig             =   0x0080,
    mdReuseSlot             =   0x0000,
    mdNewSlot               =   0x0100,
    mdCheckAccessOnOverride =   0x0200,
    mdAbstract              =   0x0400,
    mdSpecialName           =   0x0800,
    mdPinvokeImpl           =   0x2000,
    mdUnmanagedExport       =   0x0008,
    mdRTSpecialName         =   0x1000,
    mdHasSecurity           =   0x4000,
    mdRequireSecObject      =   0x8000,

    mdMemberAccessMask      =   0x0007,
    mdVtableLayoutMask      =   0x0100,
    mdReservedMask          =   0xd000,
} CorMethodAttr;

typedef enum CorMethodImpl
{
    miIL                     = 0x0000,
    miNative                 = 0x0001,
    miOPTIL                  = 0x0002,
    miRuntime                = 0x0003,
    miUnmanaged              = 0x0004,
    miManaged                = 0x0000,
    miForwardRef             = 0x0010,
    miPreserveSig            = 0x0080,
    miInternalCall           = 0x1000,
    miSynchronized           = 0x0020,
    miNoInlining             = 0x0008,
    miAggressiveInlining     = 0x0100,
    miNoOptimization         = 0x0040,
    miAggressiveOptimization = 0x0200,
    miAsync                  = 0x2000,

    miCodeTypeMask           =  0x0003,
    miManagedMask            =  0x0004,
    miUserMask               =  0x33fc,

    miMaxMethodImplVal       = 0xffff,
} CorMethodImpl;

typedef enum CorPinvokeMap
{
    pmNoMangle                      = 0x0001,
    pmCharSetNotSpec                = 0x0000,
    pmCharSetAnsi                   = 0x0002,
    pmCharSetUnicode                = 0x0004,
    pmCharSetAuto                   = 0x0006,
    pmBestFitUseAssem               = 0x0000,
    pmBestFitEnabled                = 0x0010,
    pmBestFitDisabled               = 0x0020,
    pmThrowOnUnmappableCharUseAssem = 0x0000,
    pmThrowOnUnmappableCharEnabled  = 0x1000,
    pmThrowOnUnmappableCharDisabled = 0x2000,
    pmSupportsLastError             = 0x0040,
    pmCallConvWinapi                = 0x0100,
    pmCallConvCdecl                 = 0x0200,
    pmCallConvStdcall               = 0x0300,
    pmCallConvThiscall              = 0x0400,
    pmCallConvFastcall              = 0x0500,

    pmCharSetMask                   = 0x0006,
    pmBestFitMask                   = 0x0030,
    pmCallConvMask                  = 0x0700,
    pmThrowOnUnmappableCharMask     = 0x3000,

    pmMaxValue          = 0xffff,
} CorPinvokeMap;

typedef enum CorFieldAttr
{
    fdPrivateScope    =  0x0000,
    fdPrivate         =  0x0001,
    fdFamANDAssem     =  0x0002,
    fdAssembly        =  0x0003,
    fdFamily          =  0x0004,
    fdFamORAssem      =  0x0005,
    fdPublic          =  0x0006,
    fdStatic          =  0x0010,
    fdInitOnly        =  0x0020,
    fdLiteral         =  0x0040,
    fdNotSerialized   =  0x0080,
    fdSpecialName     =  0x0200,
    fdPinvokeImpl     =  0x2000,
    fdRTSpecialName   =  0x0400,
    fdHasFieldMarshal =  0x1000,
    fdHasDefault      =  0x8000,
    fdHasFieldRVA     =  0x0100,

    fdFieldAccessMask =  0x0007,
    fdReservedMask    =  0x9500
} CorFieldAttr;

#define COR_ENUM_FIELD_NAME    ("value__")
#define COR_CTOR_METHOD_NAME   (".ctor")

#if defined(_MSC_VER) || defined(__MINGW32__)
#define COR_ENUM_FIELD_NAME_W  (L"value__")
#define COR_CTOR_METHOD_NAME_W (L".ctor")
#else
static const WCHAR COR_ENUM_FIELD_NAME_W[] = {'v','a','l','u','e','_','_',0};
static const WCHAR COR_CTOR_METHOD_NAME_W[] = {'.','c','t','o','r',0};
#endif

typedef enum CorMethodSemanticsAttr
{
    msSetter   = 0x01,
    msGetter   = 0x02,
    msOther    = 0x04,
    msAddOn    = 0x08,
    msRemoveOn = 0x10,
    msFire     = 0x20,
} CorMethodSemanticsAttr;

#endif /* __WINE_CORHDR_H */
