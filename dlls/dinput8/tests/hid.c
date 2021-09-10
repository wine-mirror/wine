/*
 * Copyright 2021 Rémi Bernon for CodeWeavers
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

#include <stdarg.h>
#include <stddef.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winioctl.h"
#include "winternl.h"
#include "wincrypt.h"
#include "winreg.h"
#include "winsvc.h"
#include "winuser.h"
#include "winnls.h"

#include "mscat.h"
#include "mssip.h"
#include "ntsecapi.h"
#include "setupapi.h"
#include "cfgmgr32.h"
#include "newdev.h"

#include "objbase.h"

#include "initguid.h"
#include "ddk/wdm.h"
#include "ddk/hidclass.h"
#include "ddk/hidsdi.h"
#include "ddk/hidpi.h"
#include "ddk/hidport.h"

#include "wine/test.h"
#include "wine/mssign.h"

#include "driver_hid.h"

static struct winetest_shared_data *test_data;
static HANDLE okfile;

static HRESULT (WINAPI *pSignerSign)( SIGNER_SUBJECT_INFO *subject, SIGNER_CERT *cert,
                                      SIGNER_SIGNATURE_INFO *signature, SIGNER_PROVIDER_INFO *provider,
                                      const WCHAR *timestamp, CRYPT_ATTRIBUTES *attr, void *sip_data );

static const WCHAR container_name[] = L"wine_testsign";

static const CERT_CONTEXT *testsign_sign( const WCHAR *filename )
{
    BYTE encoded_name[100], encoded_key_id[200], public_key_info_buffer[1000];
    BYTE hash_buffer[16], cert_buffer[1000], provider_nameA[100], serial[16];
    CERT_PUBLIC_KEY_INFO *public_key_info = (CERT_PUBLIC_KEY_INFO *)public_key_info_buffer;
    SIGNER_SIGNATURE_INFO signature = {sizeof(SIGNER_SIGNATURE_INFO)};
    SIGNER_CERT_STORE_INFO store = {sizeof(SIGNER_CERT_STORE_INFO)};
    SIGNER_ATTR_AUTHCODE authcode = {sizeof(SIGNER_ATTR_AUTHCODE)};
    SIGNER_SUBJECT_INFO subject = {sizeof(SIGNER_SUBJECT_INFO)};
    SIGNER_FILE_INFO file = {sizeof(SIGNER_FILE_INFO)};
    SIGNER_CERT signer = {sizeof(SIGNER_CERT)};
    CRYPT_KEY_PROV_INFO provider_info = {0};
    CRYPT_ALGORITHM_IDENTIFIER algid = {0};
    CERT_AUTHORITY_KEY_ID_INFO key_info;
    HCERTSTORE root_store, pub_store;
    CERT_INFO cert_info = {0};
    WCHAR provider_nameW[100];
    const CERT_CONTEXT *cert;
    CERT_EXTENSION extension;
    DWORD size, index = 0;
    HCRYPTPROV provider;
    HCRYPTKEY key;
    HRESULT hr;
    BOOL ret;

    ret = CryptAcquireContextW( &provider, container_name, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET );
    if (!ret && GetLastError() == NTE_EXISTS)
    {
        ret = CryptAcquireContextW( &provider, container_name, NULL, PROV_RSA_FULL, CRYPT_DELETEKEYSET );
        ok( ret, "Failed to delete container, error %#x\n", GetLastError() );
        ret = CryptAcquireContextW( &provider, container_name, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET );
    }
    ok( ret, "Failed to create container, error %#x\n", GetLastError() );

    ret = CryptGenKey( provider, AT_SIGNATURE, CRYPT_EXPORTABLE, &key );
    ok( ret, "Failed to create key, error %#x\n", GetLastError() );
    ret = CryptDestroyKey( key );
    ok( ret, "Failed to destroy key, error %#x\n", GetLastError() );
    ret = CryptGetUserKey( provider, AT_SIGNATURE, &key );
    ok( ret, "Failed to get user key, error %#x\n", GetLastError() );
    ret = CryptDestroyKey( key );
    ok( ret, "Failed to destroy key, error %#x\n", GetLastError() );

    size = sizeof(encoded_name);
    ret = CertStrToNameW( X509_ASN_ENCODING, L"CN=winetest_cert", CERT_X500_NAME_STR, NULL,
                          encoded_name, &size, NULL );
    ok( ret, "Failed to convert name, error %#x\n", GetLastError() );
    key_info.CertIssuer.cbData = size;
    key_info.CertIssuer.pbData = encoded_name;

    size = sizeof(public_key_info_buffer);
    ret = CryptExportPublicKeyInfo( provider, AT_SIGNATURE, X509_ASN_ENCODING, public_key_info, &size );
    ok( ret, "Failed to export public key, error %#x\n", GetLastError() );
    cert_info.SubjectPublicKeyInfo = *public_key_info;

    size = sizeof(hash_buffer);
    ret = CryptHashPublicKeyInfo( provider, CALG_MD5, 0, X509_ASN_ENCODING, public_key_info, hash_buffer, &size );
    ok( ret, "Failed to hash public key, error %#x\n", GetLastError() );

    key_info.KeyId.cbData = size;
    key_info.KeyId.pbData = hash_buffer;

    RtlGenRandom( serial, sizeof(serial) );
    key_info.CertSerialNumber.cbData = sizeof(serial);
    key_info.CertSerialNumber.pbData = serial;

    size = sizeof(encoded_key_id);
    ret = CryptEncodeObject( X509_ASN_ENCODING, X509_AUTHORITY_KEY_ID, &key_info, encoded_key_id, &size );
    ok( ret, "Failed to convert name, error %#x\n", GetLastError() );

    extension.pszObjId = (char *)szOID_AUTHORITY_KEY_IDENTIFIER;
    extension.fCritical = TRUE;
    extension.Value.cbData = size;
    extension.Value.pbData = encoded_key_id;

    cert_info.dwVersion = CERT_V3;
    cert_info.SerialNumber = key_info.CertSerialNumber;
    cert_info.SignatureAlgorithm.pszObjId = (char *)szOID_RSA_SHA1RSA;
    cert_info.Issuer = key_info.CertIssuer;
    GetSystemTimeAsFileTime( &cert_info.NotBefore );
    GetSystemTimeAsFileTime( &cert_info.NotAfter );
    cert_info.NotAfter.dwHighDateTime += 1;
    cert_info.Subject = key_info.CertIssuer;
    cert_info.cExtension = 1;
    cert_info.rgExtension = &extension;
    algid.pszObjId = (char *)szOID_RSA_SHA1RSA;
    size = sizeof(cert_buffer);
    ret = CryptSignAndEncodeCertificate( provider, AT_SIGNATURE, X509_ASN_ENCODING, X509_CERT_TO_BE_SIGNED,
                                         &cert_info, &algid, NULL, cert_buffer, &size );
    ok( ret, "Failed to create certificate, error %#x\n", GetLastError() );

    cert = CertCreateCertificateContext( X509_ASN_ENCODING, cert_buffer, size );
    ok( !!cert, "Failed to create context, error %#x\n", GetLastError() );

    size = sizeof(provider_nameA);
    ret = CryptGetProvParam( provider, PP_NAME, provider_nameA, &size, 0 );
    ok( ret, "Failed to get prov param, error %#x\n", GetLastError() );
    MultiByteToWideChar( CP_ACP, 0, (char *)provider_nameA, -1, provider_nameW, ARRAY_SIZE(provider_nameW) );

    provider_info.pwszContainerName = (WCHAR *)container_name;
    provider_info.pwszProvName = provider_nameW;
    provider_info.dwProvType = PROV_RSA_FULL;
    provider_info.dwKeySpec = AT_SIGNATURE;
    ret = CertSetCertificateContextProperty( cert, CERT_KEY_PROV_INFO_PROP_ID, 0, &provider_info );
    ok( ret, "Failed to set provider info, error %#x\n", GetLastError() );

    ret = CryptReleaseContext( provider, 0 );
    ok( ret, "failed to release context, error %u\n", GetLastError() );

    root_store = CertOpenStore( CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0, CERT_SYSTEM_STORE_LOCAL_MACHINE, L"root" );
    if (!root_store && GetLastError() == ERROR_ACCESS_DENIED)
    {
        win_skip( "Failed to open root store.\n" );
        ret = CertFreeCertificateContext( cert );
        ok( ret, "Failed to free certificate, error %u\n", GetLastError() );
        return NULL;
    }
    ok( !!root_store, "Failed to open store, error %u\n", GetLastError() );
    ret = CertAddCertificateContextToStore( root_store, cert, CERT_STORE_ADD_ALWAYS, NULL );
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        win_skip( "Failed to add self-signed certificate to store.\n" );
        ret = CertFreeCertificateContext( cert );
        ok( ret, "Failed to free certificate, error %u\n", GetLastError() );
        ret = CertCloseStore( root_store, CERT_CLOSE_STORE_CHECK_FLAG );
        ok( ret, "Failed to close store, error %u\n", GetLastError() );
        return NULL;
    }
    ok( ret, "Failed to add certificate, error %u\n", GetLastError() );
    ret = CertCloseStore( root_store, CERT_CLOSE_STORE_CHECK_FLAG );
    ok( ret, "Failed to close store, error %u\n", GetLastError() );

    pub_store = CertOpenStore( CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
                               CERT_SYSTEM_STORE_LOCAL_MACHINE, L"trustedpublisher" );
    ok( !!pub_store, "Failed to open store, error %u\n", GetLastError() );
    ret = CertAddCertificateContextToStore( pub_store, cert, CERT_STORE_ADD_ALWAYS, NULL );
    ok( ret, "Failed to add certificate, error %u\n", GetLastError() );
    ret = CertCloseStore( pub_store, CERT_CLOSE_STORE_CHECK_FLAG );
    ok( ret, "Failed to close store, error %u\n", GetLastError() );

    subject.dwSubjectChoice = 1;
    subject.pdwIndex = &index;
    subject.pSignerFileInfo = &file;
    file.pwszFileName = (WCHAR *)filename;
    signer.dwCertChoice = 2;
    signer.pCertStoreInfo = &store;
    store.pSigningCert = cert;
    store.dwCertPolicy = 0;
    signature.algidHash = CALG_SHA_256;
    signature.dwAttrChoice = SIGNER_AUTHCODE_ATTR;
    signature.pAttrAuthcode = &authcode;
    authcode.pwszName = L"";
    authcode.pwszInfo = L"";
    hr = pSignerSign( &subject, &signer, &signature, NULL, NULL, NULL, NULL );
    todo_wine
    ok( hr == S_OK || broken( hr == NTE_BAD_ALGID ) /* < 7 */, "Failed to sign, hr %#x\n", hr );

    return cert;
}

static void testsign_cleanup( const CERT_CONTEXT *cert )
{
    HCERTSTORE root_store, pub_store;
    const CERT_CONTEXT *store_cert;
    HCRYPTPROV provider;
    BOOL ret;

    root_store = CertOpenStore( CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0, CERT_SYSTEM_STORE_LOCAL_MACHINE, L"root" );
    ok( !!root_store, "Failed to open store, error %u\n", GetLastError() );
    store_cert = CertFindCertificateInStore( root_store, X509_ASN_ENCODING, 0, CERT_FIND_EXISTING, cert, NULL );
    ok( !!store_cert, "Failed to find root certificate, error %u\n", GetLastError() );
    ret = CertDeleteCertificateFromStore( store_cert );
    ok( ret, "Failed to remove certificate, error %u\n", GetLastError() );
    ret = CertCloseStore( root_store, CERT_CLOSE_STORE_CHECK_FLAG );
    ok( ret, "Failed to close store, error %u\n", GetLastError() );

    pub_store = CertOpenStore( CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
                               CERT_SYSTEM_STORE_LOCAL_MACHINE, L"trustedpublisher" );
    ok( !!pub_store, "Failed to open store, error %u\n", GetLastError() );
    store_cert = CertFindCertificateInStore( pub_store, X509_ASN_ENCODING, 0, CERT_FIND_EXISTING, cert, NULL );
    ok( !!store_cert, "Failed to find publisher certificate, error %u\n", GetLastError() );
    ret = CertDeleteCertificateFromStore( store_cert );
    ok( ret, "Failed to remove certificate, error %u\n", GetLastError() );
    ret = CertCloseStore( pub_store, CERT_CLOSE_STORE_CHECK_FLAG );
    ok( ret, "Failed to close store, error %u\n", GetLastError() );

    ret = CertFreeCertificateContext( cert );
    ok( ret, "Failed to free certificate, error %u\n", GetLastError() );

    ret = CryptAcquireContextW( &provider, container_name, NULL, PROV_RSA_FULL, CRYPT_DELETEKEYSET );
    ok( ret, "Failed to delete container, error %#x\n", GetLastError() );
}

static void load_resource( const WCHAR *name, WCHAR *filename )
{
    static WCHAR path[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW( ARRAY_SIZE(path), path );
    GetTempFileNameW( path, name, 0, filename );

    file = CreateFileW( filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0 );
    ok( file != INVALID_HANDLE_VALUE, "failed to create %s, error %u\n", debugstr_w(filename), GetLastError() );

    res = FindResourceW( NULL, name, L"TESTDLL" );
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleW( NULL ), res ) );
    WriteFile( file, ptr, SizeofResource( GetModuleHandleW( NULL ), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleW( NULL ), res ), "couldn't write resource\n" );
    CloseHandle( file );
}

#ifdef __i386__
#define EXT "x86"
#elif defined(__x86_64__)
#define EXT "amd64"
#elif defined(__arm__)
#define EXT "arm"
#elif defined(__aarch64__)
#define EXT "arm64"
#else
#define EXT
#endif

static const char inf_text[] =
    "[Version]\n"
    "Signature=$Chicago$\n"
    "ClassGuid={4d36e97d-e325-11ce-bfc1-08002be10318}\n"
    "CatalogFile=winetest.cat\n"
    "DriverVer=09/21/2006,6.0.5736.1\n"

    "[Manufacturer]\n"
    "Wine=mfg_section,NT" EXT "\n"

    "[mfg_section.NT" EXT "]\n"
    "Wine test root driver=device_section,test_hardware_id\n"

    "[device_section.NT" EXT "]\n"
    "CopyFiles=file_section\n"

    "[device_section.NT" EXT ".Services]\n"
    "AddService=winetest,0x2,svc_section\n"

    "[file_section]\n"
    "winetest.sys\n"

    "[SourceDisksFiles]\n"
    "winetest.sys=1\n"

    "[SourceDisksNames]\n"
    "1=,winetest.sys\n"

    "[DestinationDirs]\n"
    "DefaultDestDir=12\n"

    "[svc_section]\n"
    "ServiceBinary=%12%\\winetest.sys\n"
    "ServiceType=1\n"
    "StartType=3\n"
    "ErrorControl=1\n"
    "LoadOrderGroup=Extended Base\n"
    "DisplayName=\"winetest bus driver\"\n"
    "; they don't sleep anymore, on the beach\n";

static void add_file_to_catalog( HANDLE catalog, const WCHAR *file )
{
    SIP_SUBJECTINFO subject_info = {sizeof(SIP_SUBJECTINFO)};
    SIP_INDIRECT_DATA *indirect_data;
    const WCHAR *filepart = file;
    CRYPTCATMEMBER *member;
    WCHAR hash_buffer[100];
    GUID subject_guid;
    unsigned int i;
    DWORD size;
    BOOL ret;

    ret = CryptSIPRetrieveSubjectGuidForCatalogFile( file, NULL, &subject_guid );
    todo_wine
    ok( ret, "Failed to get subject guid, error %u\n", GetLastError() );

    size = 0;
    subject_info.pgSubjectType = &subject_guid;
    subject_info.pwsFileName = file;
    subject_info.DigestAlgorithm.pszObjId = (char *)szOID_OIWSEC_sha1;
    subject_info.dwFlags = SPC_INC_PE_RESOURCES_FLAG | SPC_INC_PE_IMPORT_ADDR_TABLE_FLAG |
                           SPC_EXC_PE_PAGE_HASHES_FLAG | 0x10000;
    ret = CryptSIPCreateIndirectData( &subject_info, &size, NULL );
    todo_wine
    ok( ret, "Failed to get indirect data size, error %u\n", GetLastError() );

    indirect_data = malloc( size );
    ret = CryptSIPCreateIndirectData( &subject_info, &size, indirect_data );
    todo_wine
    ok( ret, "Failed to get indirect data, error %u\n", GetLastError() );
    if (ret)
    {
        memset( hash_buffer, 0, sizeof(hash_buffer) );
        for (i = 0; i < indirect_data->Digest.cbData; ++i)
            swprintf( &hash_buffer[i * 2], 2, L"%02X", indirect_data->Digest.pbData[i] );

        member = CryptCATPutMemberInfo( catalog, (WCHAR *)file, hash_buffer, &subject_guid,
                                        0, size, (BYTE *)indirect_data );
        ok( !!member, "Failed to write member, error %u\n", GetLastError() );

        if (wcsrchr( file, '\\' )) filepart = wcsrchr( file, '\\' ) + 1;

        ret = !!CryptCATPutAttrInfo( catalog, member, (WCHAR *)L"File",
                                     CRYPTCAT_ATTR_NAMEASCII | CRYPTCAT_ATTR_DATAASCII | CRYPTCAT_ATTR_AUTHENTICATED,
                                     (wcslen( filepart ) + 1) * 2, (BYTE *)filepart );
        ok( ret, "Failed to write attr, error %u\n", GetLastError() );

        ret = !!CryptCATPutAttrInfo( catalog, member, (WCHAR *)L"OSAttr",
                                     CRYPTCAT_ATTR_NAMEASCII | CRYPTCAT_ATTR_DATAASCII | CRYPTCAT_ATTR_AUTHENTICATED,
                                     sizeof(L"2:6.0"), (BYTE *)L"2:6.0" );
        ok( ret, "Failed to write attr, error %u\n", GetLastError() );
    }
}

static void unload_driver( SC_HANDLE service )
{
    SERVICE_STATUS status;

    ControlService( service, SERVICE_CONTROL_STOP, &status );
    while (status.dwCurrentState == SERVICE_STOP_PENDING)
    {
        BOOL ret;
        Sleep( 100 );
        ret = QueryServiceStatus( service, &status );
        ok( ret, "QueryServiceStatus failed: %u\n", GetLastError() );
    }
    ok( status.dwCurrentState == SERVICE_STOPPED, "expected SERVICE_STOPPED, got %d\n", status.dwCurrentState );

    DeleteService( service );
    CloseServiceHandle( service );
}

static void pnp_driver_stop(void)
{
    SP_DEVINFO_DATA device = {sizeof(SP_DEVINFO_DATA)};
    WCHAR path[MAX_PATH], dest[MAX_PATH], *filepart;
    SC_HANDLE manager, service;
    char buffer[512];
    HDEVINFO set;
    HANDLE file;
    DWORD size;
    BOOL ret;

    set = SetupDiCreateDeviceInfoList( NULL, NULL );
    ok( set != INVALID_HANDLE_VALUE, "failed to create device list, error %u\n", GetLastError() );

    ret = SetupDiOpenDeviceInfoW( set, L"root\\winetest\\0", NULL, 0, &device );
    if (!ret && GetLastError() == ERROR_NO_SUCH_DEVINST)
    {
        ret = SetupDiDestroyDeviceInfoList( set );
        ok( ret, "failed to destroy set, error %u\n", GetLastError() );
        return;
    }
    ok( ret, "failed to open device, error %u\n", GetLastError() );

    ret = SetupDiCallClassInstaller( DIF_REMOVE, set, &device );
    ok( ret, "failed to remove device, error %u\n", GetLastError() );

    file = CreateFileW( L"\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}", 0, 0,
                        NULL, OPEN_EXISTING, 0, NULL );
    ok( file == INVALID_HANDLE_VALUE, "expected failure\n" );
    ok( GetLastError() == ERROR_FILE_NOT_FOUND, "got error %u\n", GetLastError() );

    ret = SetupDiDestroyDeviceInfoList( set );
    ok( ret, "failed to destroy set, error %u\n", GetLastError() );

    /* Windows stops the service but does not delete it. */
    manager = OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT );
    ok( !!manager, "failed to open service manager, error %u\n", GetLastError() );

    service = OpenServiceW( manager, L"winetest", SERVICE_STOP | DELETE );
    if (service) unload_driver( service );
    else ok( GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST, "got error %u\n", GetLastError() );

    CloseServiceHandle( manager );

    SetFilePointer( okfile, 0, NULL, FILE_BEGIN );
    do
    {
        ReadFile( okfile, buffer, sizeof(buffer), &size, NULL );
        printf( "%.*s", size, buffer );
    } while (size == sizeof(buffer));
    SetFilePointer( okfile, 0, NULL, FILE_BEGIN );
    SetEndOfFile( okfile );

    winetest_add_failures( InterlockedExchange( &test_data->failures, 0 ) );
    winetest_add_failures( InterlockedExchange( &test_data->todo_failures, 0 ) );

    GetFullPathNameW( L"winetest.inf", ARRAY_SIZE(path), path, NULL );
    ret = SetupCopyOEMInfW( path, NULL, 0, 0, dest, ARRAY_SIZE(dest), NULL, &filepart );
    ok( ret, "Failed to copy INF, error %u\n", GetLastError() );
    ret = SetupUninstallOEMInfW( filepart, 0, NULL );
    ok( ret, "Failed to uninstall INF, error %u\n", GetLastError() );

    ret = DeleteFileW( L"winetest.cat" );
    ok( ret, "Failed to delete file, error %u\n", GetLastError() );
    ret = DeleteFileW( L"winetest.inf" );
    ok( ret, "Failed to delete file, error %u\n", GetLastError() );
    ret = DeleteFileW( L"winetest.sys" );
    ok( ret, "Failed to delete file, error %u\n", GetLastError() );
    /* Windows 10 apparently deletes the image in SetupUninstallOEMInf(). */
    ret = DeleteFileW( L"C:/windows/system32/drivers/winetest.sys" );
    ok( ret || GetLastError() == ERROR_FILE_NOT_FOUND, "Failed to delete file, error %u\n", GetLastError() );
}

static BOOL pnp_driver_start( const WCHAR *resource )
{
    static const WCHAR hardware_id[] = L"test_hardware_id\0";
    SP_DEVINFO_DATA device = {sizeof(SP_DEVINFO_DATA)};
    WCHAR path[MAX_PATH], filename[MAX_PATH];
    SC_HANDLE manager, service;
    const CERT_CONTEXT *cert;
    BOOL ret, need_reboot;
    HANDLE catalog;
    HDEVINFO set;
    FILE *f;

    load_resource( resource, filename );
    ret = MoveFileExW( filename, L"winetest.sys", MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING );
    ok( ret, "failed to move file, error %u\n", GetLastError() );

    f = fopen( "winetest.inf", "w" );
    ok( !!f, "failed to open winetest.inf: %s\n", strerror( errno ) );
    fputs( inf_text, f );
    fclose( f );

    /* Create the catalog file. */

    catalog = CryptCATOpen( (WCHAR *)L"winetest.cat", CRYPTCAT_OPEN_CREATENEW, 0, CRYPTCAT_VERSION_1, 0 );
    ok( catalog != INVALID_HANDLE_VALUE, "Failed to create catalog, error %u\n", GetLastError() );

    add_file_to_catalog( catalog, L"winetest.sys" );
    add_file_to_catalog( catalog, L"winetest.inf" );

    ret = CryptCATPersistStore( catalog );
    todo_wine
    ok( ret, "Failed to write catalog, error %u\n", GetLastError() );

    ret = CryptCATClose( catalog );
    ok( ret, "Failed to close catalog, error %u\n", GetLastError() );

    if (!(cert = testsign_sign( L"winetest.cat" )))
    {
        ret = DeleteFileW( L"winetest.cat" );
        ok( ret, "Failed to delete file, error %u\n", GetLastError() );
        ret = DeleteFileW( L"winetest.inf" );
        ok( ret, "Failed to delete file, error %u\n", GetLastError() );
        ret = DeleteFileW( L"winetest.sys" );
        ok( ret, "Failed to delete file, error %u\n", GetLastError() );
        return FALSE;
    }

    /* Install the driver. */

    set = SetupDiCreateDeviceInfoList( NULL, NULL );
    ok( set != INVALID_HANDLE_VALUE, "failed to create device list, error %u\n", GetLastError() );

    ret = SetupDiCreateDeviceInfoW( set, L"root\\winetest\\0", &GUID_NULL, NULL, NULL, 0, &device );
    ok( ret, "failed to create device, error %#x\n", GetLastError() );

    ret = SetupDiSetDeviceRegistryPropertyW( set, &device, SPDRP_HARDWAREID, (const BYTE *)hardware_id,
                                             sizeof(hardware_id) );
    ok( ret, "failed to create set hardware ID, error %u\n", GetLastError() );

    ret = SetupDiCallClassInstaller( DIF_REGISTERDEVICE, set, &device );
    ok( ret, "failed to register device, error %u\n", GetLastError() );

    ret = SetupDiDestroyDeviceInfoList( set );
    ok( ret, "failed to destroy set, error %u\n", GetLastError() );

    GetFullPathNameW( L"winetest.inf", ARRAY_SIZE(path), path, NULL );

    ret = UpdateDriverForPlugAndPlayDevicesW( NULL, hardware_id, path, INSTALLFLAG_FORCE, &need_reboot );
    ok( ret, "failed to install device, error %u\n", GetLastError() );
    ok( !need_reboot, "expected no reboot necessary\n" );

    testsign_cleanup( cert );

    /* Check that the service is created and started. */
    manager = OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT );
    ok( !!manager, "failed to open service manager, error %u\n", GetLastError() );

    service = OpenServiceW( manager, L"winetest", SERVICE_START );
    ok( !!service, "failed to open service, error %u\n", GetLastError() );

    ret = StartServiceW( service, 0, NULL );
    ok( !ret, "service didn't start automatically\n" );
    if (!ret && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
    {
        /* If Secure Boot is enabled or the machine is 64-bit, it will reject an unsigned driver. */
        ok( GetLastError() == ERROR_DRIVER_BLOCKED || GetLastError() == ERROR_INVALID_IMAGE_HASH,
            "unexpected error %u\n", GetLastError() );
        win_skip( "Failed to start service; probably your machine doesn't accept unsigned drivers.\n" );
    }

    CloseServiceHandle( service );
    CloseServiceHandle( manager );
    return ret || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING;
}

#define check_member_( file, line, val, exp, fmt, member )                                         \
    ok_( file, line )((val).member == (exp).member, "got " #member " " fmt ", expected " fmt "\n", \
                      (val).member, (exp).member)
#define check_member( val, exp, fmt, member )                                                      \
    check_member_( __FILE__, __LINE__, val, exp, fmt, member )

#define check_hidp_caps( a, b ) check_hidp_caps_( __LINE__, a, b )
static inline void check_hidp_caps_( int line, HIDP_CAPS *caps, const HIDP_CAPS *exp )
{
    check_member_( __FILE__, line, *caps, *exp, "%04x", Usage );
    check_member_( __FILE__, line, *caps, *exp, "%04x", UsagePage );
    check_member_( __FILE__, line, *caps, *exp, "%d", InputReportByteLength );
    check_member_( __FILE__, line, *caps, *exp, "%d", OutputReportByteLength );
    check_member_( __FILE__, line, *caps, *exp, "%d", FeatureReportByteLength );
    check_member_( __FILE__, line, *caps, *exp, "%d", NumberLinkCollectionNodes );
    check_member_( __FILE__, line, *caps, *exp, "%d", NumberInputButtonCaps );
    check_member_( __FILE__, line, *caps, *exp, "%d", NumberInputValueCaps );
    check_member_( __FILE__, line, *caps, *exp, "%d", NumberInputDataIndices );
    check_member_( __FILE__, line, *caps, *exp, "%d", NumberOutputButtonCaps );
    check_member_( __FILE__, line, *caps, *exp, "%d", NumberOutputValueCaps );
    check_member_( __FILE__, line, *caps, *exp, "%d", NumberOutputDataIndices );
    check_member_( __FILE__, line, *caps, *exp, "%d", NumberFeatureButtonCaps );
    check_member_( __FILE__, line, *caps, *exp, "%d", NumberFeatureValueCaps );
    check_member_( __FILE__, line, *caps, *exp, "%d", NumberFeatureDataIndices );
}

#define check_hidp_link_collection_node( a, b ) check_hidp_link_collection_node_( __LINE__, a, b )
static inline void check_hidp_link_collection_node_( int line, HIDP_LINK_COLLECTION_NODE *node,
                                                     const HIDP_LINK_COLLECTION_NODE *exp )
{
    check_member_( __FILE__, line, *node, *exp, "%04x", LinkUsage );
    check_member_( __FILE__, line, *node, *exp, "%04x", LinkUsagePage );
    check_member_( __FILE__, line, *node, *exp, "%d", Parent );
    check_member_( __FILE__, line, *node, *exp, "%d", NumberOfChildren );
    check_member_( __FILE__, line, *node, *exp, "%d", NextSibling );
    check_member_( __FILE__, line, *node, *exp, "%d", FirstChild );
    check_member_( __FILE__, line, *node, *exp, "%d", CollectionType );
    check_member_( __FILE__, line, *node, *exp, "%d", IsAlias );
}

#define check_hidp_button_caps( a, b ) check_hidp_button_caps_( __LINE__, a, b )
static inline void check_hidp_button_caps_( int line, HIDP_BUTTON_CAPS *caps, const HIDP_BUTTON_CAPS *exp )
{
    check_member_( __FILE__, line, *caps, *exp, "%04x", UsagePage );
    check_member_( __FILE__, line, *caps, *exp, "%d", ReportID );
    check_member_( __FILE__, line, *caps, *exp, "%d", IsAlias );
    check_member_( __FILE__, line, *caps, *exp, "%d", BitField );
    check_member_( __FILE__, line, *caps, *exp, "%d", LinkCollection );
    check_member_( __FILE__, line, *caps, *exp, "%04x", LinkUsage );
    check_member_( __FILE__, line, *caps, *exp, "%04x", LinkUsagePage );
    check_member_( __FILE__, line, *caps, *exp, "%d", IsRange );
    check_member_( __FILE__, line, *caps, *exp, "%d", IsStringRange );
    check_member_( __FILE__, line, *caps, *exp, "%d", IsDesignatorRange );
    check_member_( __FILE__, line, *caps, *exp, "%d", IsAbsolute );

    if (!caps->IsRange && !exp->IsRange)
    {
        check_member_( __FILE__, line, *caps, *exp, "%04x", NotRange.Usage );
        check_member_( __FILE__, line, *caps, *exp, "%d", NotRange.DataIndex );
    }
    else if (caps->IsRange && exp->IsRange)
    {
        check_member_( __FILE__, line, *caps, *exp, "%04x", Range.UsageMin );
        check_member_( __FILE__, line, *caps, *exp, "%04x", Range.UsageMax );
        check_member_( __FILE__, line, *caps, *exp, "%d", Range.DataIndexMin );
        check_member_( __FILE__, line, *caps, *exp, "%d", Range.DataIndexMax );
    }

    if (!caps->IsRange && !exp->IsRange)
        check_member_( __FILE__, line, *caps, *exp, "%d", NotRange.StringIndex );
    else if (caps->IsStringRange && exp->IsStringRange)
    {
        check_member_( __FILE__, line, *caps, *exp, "%d", Range.StringMin );
        check_member_( __FILE__, line, *caps, *exp, "%d", Range.StringMax );
    }

    if (!caps->IsDesignatorRange && !exp->IsDesignatorRange)
        check_member_( __FILE__, line, *caps, *exp, "%d", NotRange.DesignatorIndex );
    else if (caps->IsDesignatorRange && exp->IsDesignatorRange)
    {
        check_member_( __FILE__, line, *caps, *exp, "%d", Range.DesignatorMin );
        check_member_( __FILE__, line, *caps, *exp, "%d", Range.DesignatorMax );
    }
}

#define check_hidp_value_caps( a, b ) check_hidp_value_caps_( __LINE__, a, b )
static inline void check_hidp_value_caps_( int line, HIDP_VALUE_CAPS *caps, const HIDP_VALUE_CAPS *exp )
{
    check_member_( __FILE__, line, *caps, *exp, "%04x", UsagePage );
    check_member_( __FILE__, line, *caps, *exp, "%d", ReportID );
    check_member_( __FILE__, line, *caps, *exp, "%d", IsAlias );
    check_member_( __FILE__, line, *caps, *exp, "%d", BitField );
    check_member_( __FILE__, line, *caps, *exp, "%d", LinkCollection );
    check_member_( __FILE__, line, *caps, *exp, "%d", LinkUsage );
    check_member_( __FILE__, line, *caps, *exp, "%d", LinkUsagePage );
    check_member_( __FILE__, line, *caps, *exp, "%d", IsRange );
    check_member_( __FILE__, line, *caps, *exp, "%d", IsStringRange );
    check_member_( __FILE__, line, *caps, *exp, "%d", IsDesignatorRange );
    check_member_( __FILE__, line, *caps, *exp, "%d", IsAbsolute );

    check_member_( __FILE__, line, *caps, *exp, "%d", HasNull );
    check_member_( __FILE__, line, *caps, *exp, "%d", BitSize );
    check_member_( __FILE__, line, *caps, *exp, "%d", ReportCount );
    check_member_( __FILE__, line, *caps, *exp, "%d", UnitsExp );
    check_member_( __FILE__, line, *caps, *exp, "%d", Units );
    check_member_( __FILE__, line, *caps, *exp, "%d", LogicalMin );
    check_member_( __FILE__, line, *caps, *exp, "%d", LogicalMax );
    check_member_( __FILE__, line, *caps, *exp, "%d", PhysicalMin );
    check_member_( __FILE__, line, *caps, *exp, "%d", PhysicalMax );

    if (!caps->IsRange && !exp->IsRange)
    {
        check_member_( __FILE__, line, *caps, *exp, "%04x", NotRange.Usage );
        check_member_( __FILE__, line, *caps, *exp, "%d", NotRange.DataIndex );
    }
    else if (caps->IsRange && exp->IsRange)
    {
        check_member_( __FILE__, line, *caps, *exp, "%04x", Range.UsageMin );
        check_member_( __FILE__, line, *caps, *exp, "%04x", Range.UsageMax );
        check_member_( __FILE__, line, *caps, *exp, "%d", Range.DataIndexMin );
        check_member_( __FILE__, line, *caps, *exp, "%d", Range.DataIndexMax );
    }

    if (!caps->IsRange && !exp->IsRange)
        check_member_( __FILE__, line, *caps, *exp, "%d", NotRange.StringIndex );
    else if (caps->IsStringRange && exp->IsStringRange)
    {
        check_member_( __FILE__, line, *caps, *exp, "%d", Range.StringMin );
        check_member_( __FILE__, line, *caps, *exp, "%d", Range.StringMax );
    }

    if (!caps->IsDesignatorRange && !exp->IsDesignatorRange)
        check_member_( __FILE__, line, *caps, *exp, "%d", NotRange.DesignatorIndex );
    else if (caps->IsDesignatorRange && exp->IsDesignatorRange)
    {
        check_member_( __FILE__, line, *caps, *exp, "%d", Range.DesignatorMin );
        check_member_( __FILE__, line, *caps, *exp, "%d", Range.DesignatorMax );
    }
}

static BOOL sync_ioctl( HANDLE file, DWORD code, void *in_buf, DWORD in_len, void *out_buf, DWORD *ret_len )
{
    OVERLAPPED ovl = {0};
    DWORD out_len = ret_len ? *ret_len : 0;
    BOOL ret;

    ovl.hEvent = CreateEventW( NULL, TRUE, FALSE, NULL );
    ret = DeviceIoControl( file, code, in_buf, in_len, out_buf, out_len, &out_len, &ovl );
    if (!ret && GetLastError() == ERROR_IO_PENDING)
        ret = GetOverlappedResult( file, &ovl, &out_len, TRUE );
    CloseHandle( ovl.hEvent );

    if (ret_len) *ret_len = out_len;
    return ret;
}

static void test_hidp( HANDLE file, HANDLE async_file, int report_id, BOOL polled, const HIDP_CAPS *expect_caps )
{
    const HIDP_BUTTON_CAPS expect_button_caps[] =
    {
        {
            .UsagePage = HID_USAGE_PAGE_BUTTON,
            .ReportID = report_id,
            .BitField = 2,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 1,
            .IsRange = TRUE,
            .IsAbsolute = TRUE,
            .Range.UsageMin = 1,
            .Range.UsageMax = 8,
            .Range.DataIndexMin = 2,
            .Range.DataIndexMax = 9,
        },
        {
            .UsagePage = HID_USAGE_PAGE_BUTTON,
            .ReportID = report_id,
            .BitField = 3,
            .LinkCollection = 1,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .IsRange = TRUE,
            .IsAbsolute = TRUE,
            .Range.UsageMin = 0x18,
            .Range.UsageMax = 0x1f,
            .Range.DataIndexMin = 10,
            .Range.DataIndexMax = 17,
        },
        {
            .UsagePage = HID_USAGE_PAGE_KEYBOARD,
            .ReportID = report_id,
            .BitField = 0x1fc,
            .LinkCollection = 1,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .IsRange = TRUE,
            .IsAbsolute = FALSE,
            .Range.UsageMin = 0x8,
            .Range.UsageMax = 0xf,
            .Range.DataIndexMin = 18,
            .Range.DataIndexMax = 25,
        },
        {
            .UsagePage = HID_USAGE_PAGE_BUTTON,
            .ReportID = report_id,
            .BitField = 2,
            .LinkCollection = 1,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .IsRange = FALSE,
            .IsAbsolute = TRUE,
            .NotRange.Usage = 0x20,
            .NotRange.Reserved1 = 0x20,
            .NotRange.DataIndex = 26,
            .NotRange.Reserved4 = 26,
        },
    };
    const HIDP_VALUE_CAPS expect_value_caps[] =
    {
        {
            .UsagePage = HID_USAGE_PAGE_GENERIC,
            .ReportID = report_id,
            .BitField = 2,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 1,
            .IsAbsolute = TRUE,
            .BitSize = 8,
            .ReportCount = 1,
            .LogicalMin = -128,
            .LogicalMax = 127,
            .NotRange.Usage = HID_USAGE_GENERIC_Y,
            .NotRange.Reserved1 = HID_USAGE_GENERIC_Y,
        },
        {
            .UsagePage = HID_USAGE_PAGE_GENERIC,
            .ReportID = report_id,
            .BitField = 2,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 1,
            .IsAbsolute = TRUE,
            .BitSize = 8,
            .ReportCount = 1,
            .LogicalMin = -128,
            .LogicalMax = 127,
            .NotRange.Usage = HID_USAGE_GENERIC_X,
            .NotRange.Reserved1 = HID_USAGE_GENERIC_X,
            .NotRange.DataIndex = 1,
            .NotRange.Reserved4 = 1,
        },
        {
            .UsagePage = HID_USAGE_PAGE_BUTTON,
            .ReportID = report_id,
            .BitField = 2,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 1,
            .IsAbsolute = TRUE,
            .ReportCount = 1,
            .LogicalMax = 1,
            .IsRange = TRUE,
            .Range.UsageMin = 0x21,
            .Range.UsageMax = 0x22,
            .Range.DataIndexMin = 27,
            .Range.DataIndexMax = 28,
        },
        {
            .UsagePage = HID_USAGE_PAGE_GENERIC,
            .ReportID = report_id,
            .BitField = 2,
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .LinkCollection = 1,
            .IsAbsolute = TRUE,
            .BitSize = 4,
            .ReportCount = 2,
            .LogicalMin = 1,
            .LogicalMax = 8,
            .NotRange.Usage = HID_USAGE_GENERIC_HATSWITCH,
            .NotRange.Reserved1 = HID_USAGE_GENERIC_HATSWITCH,
            .NotRange.DataIndex = 29,
            .NotRange.Reserved4 = 29,
        },
    };
    static const HIDP_LINK_COLLECTION_NODE expect_collections[] =
    {
        {
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .CollectionType = 1,
            .NumberOfChildren = 7,
            .FirstChild = 9,
        },
        {
            .LinkUsage = HID_USAGE_GENERIC_JOYSTICK,
            .LinkUsagePage = HID_USAGE_PAGE_GENERIC,
            .CollectionType = 2,
        },
    };
    static const HIDP_DATA expect_data[] =
    {
        { .DataIndex = 0, },
        { .DataIndex = 1, },
        { .DataIndex = 5, .RawValue = 1, },
        { .DataIndex = 7, .RawValue = 1, },
        { .DataIndex = 19, .RawValue = 1, },
        { .DataIndex = 21, .RawValue = 1, },
        { .DataIndex = 30, },
        { .DataIndex = 31, },
        { .DataIndex = 32, .RawValue = 0xfeedcafe, },
        { .DataIndex = 37, .RawValue = 1, },
        { .DataIndex = 39, .RawValue = 1, },
    };

    OVERLAPPED overlapped = {0}, overlapped2 = {0};
    HIDP_LINK_COLLECTION_NODE collections[16];
    PHIDP_PREPARSED_DATA preparsed_data;
    USAGE_AND_PAGE usage_and_pages[16];
    HIDP_BUTTON_CAPS button_caps[32];
    HIDP_VALUE_CAPS value_caps[16];
    char buffer[200], report[200];
    DWORD collection_count;
    DWORD waveform_list;
    HIDP_DATA data[64];
    USAGE usages[16];
    NTSTATUS status;
    HIDP_CAPS caps;
    unsigned int i;
    USHORT count;
    ULONG value;
    BOOL ret;

    ret = HidD_GetPreparsedData( file, &preparsed_data );
    ok( ret, "HidD_GetPreparsedData failed with error %u\n", GetLastError() );

    memset( buffer, 0, sizeof(buffer) );
    status = HidP_GetCaps( (PHIDP_PREPARSED_DATA)buffer, &caps );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetCaps returned %#x\n", status );
    status = HidP_GetCaps( preparsed_data, &caps );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetCaps returned %#x\n", status );
    check_hidp_caps( &caps, expect_caps );

    collection_count = 0;
    status = HidP_GetLinkCollectionNodes( collections, &collection_count, preparsed_data );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetLinkCollectionNodes returned %#x\n", status );
    ok( collection_count == caps.NumberLinkCollectionNodes,
        "got %d collection nodes, expected %d\n", collection_count, caps.NumberLinkCollectionNodes );
    collection_count = ARRAY_SIZE(collections);
    status = HidP_GetLinkCollectionNodes( collections, &collection_count, (PHIDP_PREPARSED_DATA)buffer );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetLinkCollectionNodes returned %#x\n", status );
    status = HidP_GetLinkCollectionNodes( collections, &collection_count, preparsed_data );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetLinkCollectionNodes returned %#x\n", status );
    ok( collection_count == caps.NumberLinkCollectionNodes,
        "got %d collection nodes, expected %d\n", collection_count, caps.NumberLinkCollectionNodes );

    for (i = 0; i < ARRAY_SIZE(expect_collections); ++i)
    {
        winetest_push_context( "collections[%d]", i );
        check_hidp_link_collection_node( &collections[i], &expect_collections[i] );
        winetest_pop_context();
    }

    count = ARRAY_SIZE(button_caps);
    status = HidP_GetButtonCaps( HidP_Output, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetButtonCaps returned %#x\n", status );
    status = HidP_GetButtonCaps( HidP_Feature + 1, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetButtonCaps returned %#x\n", status );
    count = 0;
    status = HidP_GetButtonCaps( HidP_Input, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetButtonCaps returned %#x\n", status );
    ok( count == caps.NumberInputButtonCaps, "HidP_GetButtonCaps returned count %d, expected %d\n",
        count, caps.NumberInputButtonCaps );
    count = ARRAY_SIZE(button_caps);
    status = HidP_GetButtonCaps( HidP_Input, button_caps, &count, (PHIDP_PREPARSED_DATA)buffer );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetButtonCaps returned %#x\n", status );
    memset( button_caps, 0, sizeof(button_caps) );
    status = HidP_GetButtonCaps( HidP_Input, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetButtonCaps returned %#x\n", status );
    ok( count == caps.NumberInputButtonCaps, "HidP_GetButtonCaps returned count %d, expected %d\n",
        count, caps.NumberInputButtonCaps );

    for (i = 0; i < ARRAY_SIZE(expect_button_caps); ++i)
    {
        winetest_push_context( "button_caps[%d]", i );
        check_hidp_button_caps( &button_caps[i], &expect_button_caps[i] );
        winetest_pop_context();
    }

    count = ARRAY_SIZE(button_caps) - 1;
    status = HidP_GetSpecificButtonCaps( HidP_Output, 0, 0, 0, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificButtonCaps returned %#x\n", status );
    status = HidP_GetSpecificButtonCaps( HidP_Feature + 1, 0, 0, 0, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetSpecificButtonCaps returned %#x\n", status );
    count = 0;
    status = HidP_GetSpecificButtonCaps( HidP_Input, 0, 0, 0, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetSpecificButtonCaps returned %#x\n", status );
    ok( count == caps.NumberInputButtonCaps, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n",
        count, caps.NumberInputButtonCaps );
    count = ARRAY_SIZE(button_caps) - 1;
    status = HidP_GetSpecificButtonCaps( HidP_Input, 0, 0, 0, button_caps, &count, (PHIDP_PREPARSED_DATA)buffer );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetSpecificButtonCaps returned %#x\n", status );

    status = HidP_GetSpecificButtonCaps( HidP_Input, 0, 0, 0, button_caps + 1, &count, preparsed_data );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetSpecificButtonCaps returned %#x\n", status );
    ok( count == caps.NumberInputButtonCaps, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n",
        count, caps.NumberInputButtonCaps );
    check_hidp_button_caps( &button_caps[1], &button_caps[0] );

    status = HidP_GetSpecificButtonCaps( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, 5, button_caps + 1,
                                         &count, preparsed_data );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetSpecificButtonCaps returned %#x\n", status );
    ok( count == 1, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n", count, 1 );
    check_hidp_button_caps( &button_caps[1], &button_caps[0] );

    count = 0xbeef;
    status = HidP_GetSpecificButtonCaps( HidP_Input, 0xfffe, 0, 0, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificButtonCaps returned %#x\n", status );
    ok( count == 0, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n", count, 0 );
    count = 0xbeef;
    status = HidP_GetSpecificButtonCaps( HidP_Input, 0, 0xfffe, 0, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificButtonCaps returned %#x\n", status );
    ok( count == 0, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n", count, 0 );
    count = 0xbeef;
    status = HidP_GetSpecificButtonCaps( HidP_Input, 0, 0, 0xfffe, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificButtonCaps returned %#x\n", status );
    ok( count == 0, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n", count, 0 );

    count = ARRAY_SIZE(value_caps);
    status = HidP_GetValueCaps( HidP_Output, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetValueCaps returned %#x\n", status );
    status = HidP_GetValueCaps( HidP_Feature + 1, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetValueCaps returned %#x\n", status );
    count = 0;
    status = HidP_GetValueCaps( HidP_Input, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetValueCaps returned %#x\n", status );
    ok( count == caps.NumberInputValueCaps, "HidP_GetValueCaps returned count %d, expected %d\n",
        count, caps.NumberInputValueCaps );
    count = ARRAY_SIZE(value_caps);
    status = HidP_GetValueCaps( HidP_Input, value_caps, &count, (PHIDP_PREPARSED_DATA)buffer );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetValueCaps returned %#x\n", status );
    status = HidP_GetValueCaps( HidP_Input, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetValueCaps returned %#x\n", status );
    ok( count == caps.NumberInputValueCaps, "HidP_GetValueCaps returned count %d, expected %d\n",
        count, caps.NumberInputValueCaps );

    for (i = 0; i < ARRAY_SIZE(expect_value_caps); ++i)
    {
        winetest_push_context( "value_caps[%d]", i );
        check_hidp_value_caps( &value_caps[i], &expect_value_caps[i] );
        winetest_pop_context();
    }

    count = ARRAY_SIZE(value_caps) - 4;
    status = HidP_GetSpecificValueCaps( HidP_Output, 0, 0, 0, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificValueCaps returned %#x\n", status );
    status = HidP_GetSpecificValueCaps( HidP_Feature + 1, 0, 0, 0, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetSpecificValueCaps returned %#x\n", status );
    count = 0;
    status = HidP_GetSpecificValueCaps( HidP_Input, 0, 0, 0, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetSpecificValueCaps returned %#x\n", status );
    ok( count == caps.NumberInputValueCaps, "HidP_GetSpecificValueCaps returned count %d, expected %d\n",
        count, caps.NumberInputValueCaps );
    count = ARRAY_SIZE(value_caps) - 4;
    status = HidP_GetSpecificValueCaps( HidP_Input, 0, 0, 0, value_caps + 4, &count, (PHIDP_PREPARSED_DATA)buffer );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetSpecificValueCaps returned %#x\n", status );

    status = HidP_GetSpecificValueCaps( HidP_Input, 0, 0, 0, value_caps + 4, &count, preparsed_data );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetSpecificValueCaps returned %#x\n", status );
    ok( count == caps.NumberInputValueCaps, "HidP_GetSpecificValueCaps returned count %d, expected %d\n",
        count, caps.NumberInputValueCaps );
    check_hidp_value_caps( &value_caps[4], &value_caps[0] );
    check_hidp_value_caps( &value_caps[5], &value_caps[1] );
    check_hidp_value_caps( &value_caps[6], &value_caps[2] );
    check_hidp_value_caps( &value_caps[7], &value_caps[3] );

    count = 1;
    status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH,
                                        value_caps + 4, &count, preparsed_data );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetSpecificValueCaps returned %#x\n", status );
    ok( count == 1, "HidP_GetSpecificValueCaps returned count %d, expected %d\n", count, 1 );
    check_hidp_value_caps( &value_caps[4], &value_caps[3] );

    count = 0xdead;
    status = HidP_GetSpecificValueCaps( HidP_Input, 0xfffe, 0, 0, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificValueCaps returned %#x\n", status );
    ok( count == 0, "HidP_GetSpecificValueCaps returned count %d, expected %d\n", count, 0 );
    count = 0xdead;
    status = HidP_GetSpecificValueCaps( HidP_Input, 0, 0xfffe, 0, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificValueCaps returned %#x\n", status );
    ok( count == 0, "HidP_GetSpecificValueCaps returned count %d, expected %d\n", count, 0 );
    count = 0xdead;
    status = HidP_GetSpecificValueCaps( HidP_Input, 0, 0, 0xfffe, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificValueCaps returned %#x\n", status );
    ok( count == 0, "HidP_GetSpecificValueCaps returned count %d, expected %d\n", count, 0 );

    status = HidP_InitializeReportForID( HidP_Input, 0, (PHIDP_PREPARSED_DATA)buffer, report, sizeof(report) );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_InitializeReportForID returned %#x\n", status );
    status = HidP_InitializeReportForID( HidP_Feature + 1, 0, preparsed_data, report, sizeof(report) );
    ok( status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_InitializeReportForID returned %#x\n", status );
    status = HidP_InitializeReportForID( HidP_Input, 0, preparsed_data, report, sizeof(report) );
    ok( status == HIDP_STATUS_INVALID_REPORT_LENGTH, "HidP_InitializeReportForID returned %#x\n", status );
    status = HidP_InitializeReportForID( HidP_Input, 0, preparsed_data, report, caps.InputReportByteLength + 1 );
    ok( status == HIDP_STATUS_INVALID_REPORT_LENGTH, "HidP_InitializeReportForID returned %#x\n", status );
    status = HidP_InitializeReportForID( HidP_Input, 1 - report_id, preparsed_data, report,
                                         caps.InputReportByteLength );
    ok( status == HIDP_STATUS_REPORT_DOES_NOT_EXIST, "HidP_InitializeReportForID returned %#x\n", status );

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Input, report_id, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#x\n", status );

    memset( buffer, 0xcd, sizeof(buffer) );
    memset( buffer, 0, caps.InputReportByteLength );
    buffer[0] = report_id;
    ok( !memcmp( buffer, report, sizeof(buffer) ), "unexpected report data\n" );

    status = HidP_SetUsageValueArray( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, buffer,
                                      sizeof(buffer), preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_NOT_VALUE_ARRAY, "HidP_SetUsageValueArray returned %#x\n", status );
    memset( buffer, 0xcd, sizeof(buffer) );
    status = HidP_SetUsageValueArray( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH,
                                      buffer, 0, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_SetUsageValueArray returned %#x\n", status );
    status = HidP_SetUsageValueArray( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH,
                                      buffer, 8, preparsed_data, report, caps.InputReportByteLength );
    todo_wine
    ok( status == HIDP_STATUS_NOT_IMPLEMENTED, "HidP_SetUsageValueArray returned %#x\n", status );

    status = HidP_GetUsageValueArray( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, buffer,
                                      sizeof(buffer), preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_NOT_VALUE_ARRAY, "HidP_GetUsageValueArray returned %#x\n", status );
    memset( buffer, 0xcd, sizeof(buffer) );
    status = HidP_GetUsageValueArray( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH,
                                      buffer, 0, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetUsageValueArray returned %#x\n", status );
    status = HidP_GetUsageValueArray( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH,
                                      buffer, 8, preparsed_data, report, caps.InputReportByteLength );
    todo_wine
    ok( status == HIDP_STATUS_NOT_IMPLEMENTED, "HidP_GetUsageValueArray returned %#x\n", status );

    value = -128;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status );
    value = 0xdeadbeef;
    status = HidP_GetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, &value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#x\n", status );
    ok( value == 0x80, "got value %x, expected %#x\n", value, 0x80 );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#x\n", status );
    ok( value == -128, "got value %x, expected %#x\n", value, -128 );

    value = 127;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#x\n", status );
    ok( value == 127, "got value %x, expected %#x\n", value, 127 );

    value = 0;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#x\n", status );
    ok( value == 0, "got value %x, expected %#x\n", value, 0 );

    value = 0x7fffffff;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_VALUE_OUT_OF_RANGE, "HidP_GetScaledUsageValue returned %#x\n", status );
    ok( value == 0, "got value %x, expected %#x\n", value, 0 );
    value = 0xdeadbeef;
    status = HidP_GetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z, &value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#x\n", status );
    ok( value == 0x7fffffff, "got value %x, expected %#x\n", value, 0x7fffffff );

    value = 0x3fffffff;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#x\n", status );
    ok( value == 0x7fffffff, "got value %x, expected %#x\n", value, 0x7fffffff );

    value = 0;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#x\n", status );
    ok( value == 0x80000000, "got value %x, expected %#x\n", value, 0x80000000 );

    value = 0;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RX, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RX,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#x\n", status );
    ok( value == 0, "got value %x, expected %#x\n", value, 0 );

    value = 0xfeedcafe;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RY, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RY,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BAD_LOG_PHY_VALUES, "HidP_GetScaledUsageValue returned %#x\n", status );
    ok( value == 0, "got value %x, expected %#x\n", value, 0 );
    status = HidP_SetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RY,
                                       0, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BAD_LOG_PHY_VALUES, "HidP_GetScaledUsageValue returned %#x\n", status );
    ok( value == 0, "got value %x, expected %#x\n", value, 0 );

    value = HidP_MaxUsageListLength( HidP_Feature + 1, 0, preparsed_data );
    ok( value == 0, "HidP_MaxUsageListLength(HidP_Feature + 1, 0) returned %d, expected %d\n", value, 0 );
    value = HidP_MaxUsageListLength( HidP_Input, 0, preparsed_data );
    ok( value == 50, "HidP_MaxUsageListLength(HidP_Input, 0) returned %d, expected %d\n", value, 50 );
    value = HidP_MaxUsageListLength( HidP_Input, HID_USAGE_PAGE_BUTTON, preparsed_data );
    ok( value == 32, "HidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_BUTTON) returned %d, expected %d\n",
        value, 32 );
    value = HidP_MaxUsageListLength( HidP_Input, HID_USAGE_PAGE_LED, preparsed_data );
    ok( value == 8, "HidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_LED) returned %d, expected %d\n",
        value, 8 );
    value = HidP_MaxUsageListLength( HidP_Feature, HID_USAGE_PAGE_BUTTON, preparsed_data );
    ok( value == 8, "HidP_MaxUsageListLength(HidP_Feature, HID_USAGE_PAGE_BUTTON) returned %d, expected %d\n",
        value, 8 );
    value = HidP_MaxUsageListLength( HidP_Feature, HID_USAGE_PAGE_LED, preparsed_data );
    ok( value == 0, "HidP_MaxUsageListLength(HidP_Feature, HID_USAGE_PAGE_LED) returned %d, expected %d\n",
        value, 0 );

    usages[0] = 0xff;
    value = 1;
    status = HidP_SetUsages( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsages returned %#x\n", status );
    usages[1] = 2;
    usages[2] = 0xff;
    value = 3;
    status = HidP_SetUsages( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsages returned %#x\n", status );
    usages[0] = 4;
    usages[1] = 6;
    value = 2;
    status = HidP_SetUsages( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsages returned %#x\n", status );
    usages[0] = 4;
    usages[1] = 6;
    value = 2;
    status = HidP_SetUsages( HidP_Input, HID_USAGE_PAGE_LED, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsages returned %#x\n", status );

    value = ARRAY_SIZE(usages);
    status = HidP_GetUsages( HidP_Input, HID_USAGE_PAGE_KEYBOARD, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsages returned %#x\n", status );
    ok( value == 0, "got usage count %d, expected %d\n", value, 2 );

    usages[0] = 0x9;
    usages[1] = 0xb;
    usages[2] = 0xa;
    value = 3;
    ok( report[6] == 0, "got report[6] %x expected 0\n", report[6] );
    ok( report[7] == 0, "got report[7] %x expected 0\n", report[7] );
    memcpy( buffer, report, caps.InputReportByteLength );
    status = HidP_SetUsages( HidP_Input, HID_USAGE_PAGE_KEYBOARD, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_SetUsages returned %#x\n", status );
    buffer[6] = 2;
    buffer[7] = 4;
    ok( !memcmp( buffer, report, caps.InputReportByteLength ), "unexpected report data\n" );

    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_LED, 0, 6, 1, preparsed_data, report,
                                 caps.InputReportByteLength );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsageValue returned %#x\n", status );

    value = 0xdeadbeef;
    status = HidP_GetUsageValue( HidP_Input, HID_USAGE_PAGE_LED, 0, 6, &value, preparsed_data,
                                 report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsageValue returned %#x\n", status );
    ok( value == 0xdeadbeef, "got value %x, expected %#x\n", value, 0xdeadbeef );

    value = 1;
    status = HidP_GetUsages( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetUsages returned %#x\n", status );
    ok( value == 2, "got usage count %d, expected %d\n", value, 2 );
    value = ARRAY_SIZE(usages);
    memset( usages, 0xcd, sizeof(usages) );
    status = HidP_GetUsages( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsages returned %#x\n", status );
    ok( value == 2, "got usage count %d, expected %d\n", value, 2 );
    ok( usages[0] == 4, "got usages[0] %x, expected %x\n", usages[0], 4 );
    ok( usages[1] == 6, "got usages[1] %x, expected %x\n", usages[1], 6 );

    value = ARRAY_SIZE(usages);
    memset( usages, 0xcd, sizeof(usages) );
    status = HidP_GetUsages( HidP_Input, HID_USAGE_PAGE_LED, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsages returned %#x\n", status );
    ok( value == 2, "got usage count %d, expected %d\n", value, 2 );
    ok( usages[0] == 6, "got usages[0] %x, expected %x\n", usages[0], 6 );
    ok( usages[1] == 4, "got usages[1] %x, expected %x\n", usages[1], 4 );

    value = ARRAY_SIZE(usage_and_pages);
    memset( usage_and_pages, 0xcd, sizeof(usage_and_pages) );
    status = HidP_GetUsagesEx( HidP_Input, 0, usage_and_pages, &value, preparsed_data, report,
                               caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsagesEx returned %#x\n", status );
    ok( value == 6, "got usage count %d, expected %d\n", value, 4 );
    ok( usage_and_pages[0].UsagePage == HID_USAGE_PAGE_BUTTON, "got usage_and_pages[0] UsagePage %x, expected %x\n",
        usage_and_pages[0].UsagePage, HID_USAGE_PAGE_BUTTON );
    ok( usage_and_pages[1].UsagePage == HID_USAGE_PAGE_BUTTON, "got usage_and_pages[1] UsagePage %x, expected %x\n",
        usage_and_pages[1].UsagePage, HID_USAGE_PAGE_BUTTON );
    ok( usage_and_pages[2].UsagePage == HID_USAGE_PAGE_KEYBOARD, "got usage_and_pages[2] UsagePage %x, expected %x\n",
        usage_and_pages[2].UsagePage, HID_USAGE_PAGE_KEYBOARD );
    ok( usage_and_pages[3].UsagePage == HID_USAGE_PAGE_KEYBOARD, "got usage_and_pages[3] UsagePage %x, expected %x\n",
        usage_and_pages[3].UsagePage, HID_USAGE_PAGE_KEYBOARD );
    ok( usage_and_pages[4].UsagePage == HID_USAGE_PAGE_LED, "got usage_and_pages[4] UsagePage %x, expected %x\n",
        usage_and_pages[4].UsagePage, HID_USAGE_PAGE_LED );
    ok( usage_and_pages[5].UsagePage == HID_USAGE_PAGE_LED, "got usage_and_pages[5] UsagePage %x, expected %x\n",
        usage_and_pages[5].UsagePage, HID_USAGE_PAGE_LED );
    ok( usage_and_pages[0].Usage == 4, "got usage_and_pages[0] Usage %x, expected %x\n",
        usage_and_pages[0].Usage, 4 );
    ok( usage_and_pages[1].Usage == 6, "got usage_and_pages[1] Usage %x, expected %x\n",
        usage_and_pages[1].Usage, 6 );
    ok( usage_and_pages[2].Usage == 9, "got usage_and_pages[2] Usage %x, expected %x\n",
        usage_and_pages[2].Usage, 9 );
    ok( usage_and_pages[3].Usage == 11, "got usage_and_pages[3] Usage %x, expected %x\n",
        usage_and_pages[3].Usage, 11 );
    ok( usage_and_pages[4].Usage == 6, "got usage_and_pages[4] Usage %x, expected %x\n",
        usage_and_pages[4].Usage, 6 );
    ok( usage_and_pages[5].Usage == 4, "got usage_and_pages[5] Usage %x, expected %x\n",
        usage_and_pages[5].Usage, 4 );

    value = HidP_MaxDataListLength( HidP_Feature + 1, preparsed_data );
    ok( value == 0, "HidP_MaxDataListLength(HidP_Feature + 1) returned %d, expected %d\n", value, 0 );
    value = HidP_MaxDataListLength( HidP_Input, preparsed_data );
    ok( value == 58, "HidP_MaxDataListLength(HidP_Input) returned %d, expected %d\n", value, 58 );
    value = HidP_MaxDataListLength( HidP_Output, preparsed_data );
    ok( value == 0, "HidP_MaxDataListLength(HidP_Output) returned %d, expected %d\n", value, 0 );
    value = HidP_MaxDataListLength( HidP_Feature, preparsed_data );
    ok( value == 14, "HidP_MaxDataListLength(HidP_Feature) returned %d, expected %d\n", value, 14 );

    value = 1;
    status = HidP_GetData( HidP_Input, data, &value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetData returned %#x\n", status );
    ok( value == 11, "got data count %d, expected %d\n", value, 11 );
    memset( data, 0, sizeof(data) );
    status = HidP_GetData( HidP_Input, data, &value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetData returned %#x\n", status );
    for (i = 0; i < ARRAY_SIZE(expect_data); ++i)
    {
        winetest_push_context( "data[%d]", i );
        check_member( data[i], expect_data[i], "%d", DataIndex );
        check_member( data[i], expect_data[i], "%d", RawValue );
        winetest_pop_context();
    }

    /* HID nary usage collections are set with 1-based usage index in their declaration order */

    memset( report, 0, caps.InputReportByteLength );
    status = HidP_InitializeReportForID( HidP_Input, report_id, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#x\n", status );
    value = 2;
    usages[0] = 0x8e;
    usages[1] = 0x8f;
    status = HidP_SetUsages( HidP_Input, HID_USAGE_PAGE_KEYBOARD, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsages returned %#x\n", status );
    ok( report[caps.InputReportByteLength - 2] == 3, "unexpected usage index %d, expected 3\n",
        report[caps.InputReportByteLength - 2] );
    ok( report[caps.InputReportByteLength - 1] == 4, "unexpected usage index %d, expected 4\n",
        report[caps.InputReportByteLength - 1] );
    status = HidP_UnsetUsages( HidP_Input, HID_USAGE_PAGE_KEYBOARD, 0, usages, &value,
                               preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_UnsetUsages returned %#x\n", status );
    ok( report[caps.InputReportByteLength - 2] == 0, "unexpected usage index %d, expected 0\n",
        report[caps.InputReportByteLength - 2] );
    ok( report[caps.InputReportByteLength - 1] == 0, "unexpected usage index %d, expected 0\n",
        report[caps.InputReportByteLength - 1] );
    status = HidP_UnsetUsages( HidP_Input, HID_USAGE_PAGE_KEYBOARD, 0, usages, &value,
                               preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BUTTON_NOT_PRESSED, "HidP_UnsetUsages returned %#x\n", status );
    value = 1;
    usages[0] = 0x8c;
    status = HidP_SetUsages( HidP_Input, HID_USAGE_PAGE_KEYBOARD, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsages returned %#x\n", status );
    ok( report[caps.InputReportByteLength - 2] == 1, "unexpected usage index %d, expected 1\n",
        report[caps.InputReportByteLength - 2] );

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Feature, 3, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_REPORT_DOES_NOT_EXIST, "HidP_InitializeReportForID returned %#x\n", status );

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Feature, report_id, preparsed_data, report,
                                         caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#x\n", status );

    memset( buffer, 0xcd, sizeof(buffer) );
    memset( buffer, 0, caps.FeatureReportByteLength );
    buffer[0] = report_id;
    ok( !memcmp( buffer, report, sizeof(buffer) ), "unexpected report data\n" );

    for (i = 0; i < caps.NumberLinkCollectionNodes; ++i)
    {
        if (collections[i].LinkUsagePage != HID_USAGE_PAGE_HAPTICS) continue;
        if (collections[i].LinkUsage == HID_USAGE_HAPTICS_WAVEFORM_LIST) break;
    }
    ok( i < caps.NumberLinkCollectionNodes,
        "HID_USAGE_HAPTICS_WAVEFORM_LIST collection not found\n" );
    waveform_list = i;

    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                 HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, (PHIDP_PREPARSED_DATA)buffer,
                                 report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_SetUsageValue returned %#x\n", status );
    status = HidP_SetUsageValue( HidP_Feature + 1, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                 HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                 caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_SetUsageValue returned %#x\n", status );
    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                 HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                 caps.FeatureReportByteLength + 1 );
    ok( status == HIDP_STATUS_INVALID_REPORT_LENGTH, "HidP_SetUsageValue returned %#x\n", status );
    report[0] = 1 - report_id;
    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                 HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                 caps.FeatureReportByteLength );
    ok( status == (report_id ? HIDP_STATUS_SUCCESS : HIDP_STATUS_INCOMPATIBLE_REPORT_ID),
        "HidP_SetUsageValue returned %#x\n", status );
    report[0] = 2;
    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                 HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                 caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_INCOMPATIBLE_REPORT_ID, "HidP_SetUsageValue returned %#x\n", status );
    report[0] = report_id;
    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, 0xdead, 3, HID_USAGE_HAPTICS_WAVEFORM_RUMBLE,
                                 preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsageValue returned %#x\n", status );

    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                 HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                 caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status );

    memset( buffer, 0xcd, sizeof(buffer) );
    memset( buffer, 0, caps.FeatureReportByteLength );
    buffer[0] = report_id;
    value = HID_USAGE_HAPTICS_WAVEFORM_RUMBLE;
    memcpy( buffer + 1, &value, 2 );
    ok( !memcmp( buffer, report, sizeof(buffer) ), "unexpected report data\n" );

    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                 (PHIDP_PREPARSED_DATA)buffer, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetUsageValue returned %#x\n", status );
    status = HidP_GetUsageValue( HidP_Feature + 1, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                 preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetUsageValue returned %#x\n", status );
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                 preparsed_data, report, caps.FeatureReportByteLength + 1 );
    ok( status == HIDP_STATUS_INVALID_REPORT_LENGTH, "HidP_GetUsageValue returned %#x\n", status );
    report[0] = 1 - report_id;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                 preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == (report_id ? HIDP_STATUS_SUCCESS : HIDP_STATUS_INCOMPATIBLE_REPORT_ID),
        "HidP_GetUsageValue returned %#x\n", status );
    report[0] = 2;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                 preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_INCOMPATIBLE_REPORT_ID, "HidP_GetUsageValue returned %#x\n", status );
    report[0] = report_id;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, 0xdead, 3, &value,
                                 preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetUsageValue returned %#x\n", status );

    value = 0xdeadbeef;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                 preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#x\n", status );
    ok( value == HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, "got value %x, expected %#x\n", value,
        HID_USAGE_HAPTICS_WAVEFORM_RUMBLE );

    memset( buffer, 0xff, sizeof(buffer) );
    status = HidP_SetUsageValueArray( HidP_Feature, HID_USAGE_PAGE_HAPTICS, 0,
                                      HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME, buffer, 0,
                                      preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_SetUsageValueArray returned %#x\n", status );
    status = HidP_SetUsageValueArray( HidP_Feature, HID_USAGE_PAGE_HAPTICS, 0,
                                      HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME, buffer, 64,
                                      preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValueArray returned %#x\n", status );
    ok( !memcmp( report + 9, buffer, 8 ), "unexpected report data\n" );

    memset( buffer, 0, sizeof(buffer) );
    status = HidP_GetUsageValueArray( HidP_Feature, HID_USAGE_PAGE_HAPTICS, 0,
                                      HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME, buffer, 0,
                                      preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetUsageValueArray returned %#x\n", status );
    status = HidP_GetUsageValueArray( HidP_Feature, HID_USAGE_PAGE_HAPTICS, 0,
                                      HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME, buffer, 64,
                                      preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValueArray returned %#x\n", status );
    memset( buffer + 16, 0xff, 8 );
    ok( !memcmp( buffer, buffer + 16, 16 ), "unexpected report value\n" );

    value = 0x7fffffff;
    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       (LONG *)&value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_VALUE_OUT_OF_RANGE, "HidP_GetScaledUsageValue returned %#x\n", status );
    ok( value == 0, "got value %x, expected %#x\n", value, 0 );
    value = 0xdeadbeef;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 &value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#x\n", status );
    ok( value == 0x7fffffff, "got value %x, expected %#x\n", value, 0x7fffffff );

    value = 0x7fff;
    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       (LONG *)&value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#x\n", status );
    ok( value == 0x0003ffff, "got value %x, expected %#x\n", value, 0x0003ffff );

    value = 0;
    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#x\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       (LONG *)&value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#x\n", status );
    ok( value == 0xfff90000, "got value %x, expected %#x\n", value, 0xfff90000 );
    status = HidP_SetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       0x1000, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetScaledUsageValue returned %#x\n", status );
    value = 0;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 &value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#x\n", status );
    ok( value == 0xfffff518, "got value %x, expected %#x\n", value, 0xfffff518 );
    status = HidP_SetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       0, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetScaledUsageValue returned %#x\n", status );
    value = 0;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 &value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#x\n", status );
    ok( value == 0xfffff45e, "got value %x, expected %#x\n", value, 0xfffff45e );
    status = HidP_SetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       0xdead, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetScaledUsageValue returned %#x\n", status );
    value = 0;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 &value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#x\n", status );
    ok( value == 0xfffffe7d, "got value %x, expected %#x\n", value, 0xfffffe7d );
    status = HidP_SetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       0xbeef, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetScaledUsageValue returned %#x\n", status );
    value = 0;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 &value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#x\n", status );
    ok( value == 0xfffffd0b, "got value %x, expected %#x\n", value, 0xfffffd0b );

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Input, report_id, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#x\n", status );

    SetLastError( 0xdeadbeef );
    ret = HidD_GetInputReport( file, report, 0 );
    ok( !ret, "HidD_GetInputReport succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER, "HidD_GetInputReport returned error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = HidD_GetInputReport( file, report, caps.InputReportByteLength - 1 );
    ok( !ret, "HidD_GetInputReport succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
        "HidD_GetInputReport returned error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( buffer, 0x5a, sizeof(buffer) );
    ret = HidD_GetInputReport( file, buffer, caps.InputReportByteLength );
    if (report_id || broken( !ret ) /* w7u */)
    {
        ok( !ret, "HidD_GetInputReport succeeded, last error %u\n", GetLastError() );
        ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
            "HidD_GetInputReport returned error %u\n", GetLastError() );
    }
    else
    {
        ok( ret, "HidD_GetInputReport failed, last error %u\n", GetLastError() );
        ok( buffer[0] == 0x5a, "got buffer[0] %x, expected 0x5a\n", (BYTE)buffer[0] );
    }

    SetLastError( 0xdeadbeef );
    ret = HidD_GetInputReport( file, report, caps.InputReportByteLength );
    ok( ret, "HidD_GetInputReport failed, last error %u\n", GetLastError() );
    ok( report[0] == report_id, "got report[0] %02x, expected %02x\n", report[0], report_id );

    SetLastError( 0xdeadbeef );
    value = caps.InputReportByteLength * 2;
    ret = sync_ioctl( file, IOCTL_HID_GET_INPUT_REPORT, NULL, 0, report, &value );
    ok( ret, "IOCTL_HID_GET_INPUT_REPORT failed, last error %u\n", GetLastError() );
    ok( value == 3, "got length %u, expected 3\n", value );
    ok( report[0] == report_id, "got report[0] %02x, expected %02x\n", report[0], report_id );

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Feature, report_id, preparsed_data, report,
                                         caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#x\n", status );

    SetLastError( 0xdeadbeef );
    ret = HidD_GetFeature( file, report, 0 );
    ok( !ret, "HidD_GetFeature succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER, "HidD_GetFeature returned error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = HidD_GetFeature( file, report, caps.FeatureReportByteLength - 1 );
    ok( !ret, "HidD_GetFeature succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
        "HidD_GetFeature returned error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( buffer, 0x5a, sizeof(buffer) );
    ret = HidD_GetFeature( file, buffer, caps.FeatureReportByteLength );
    if (report_id || broken( !ret ))
    {
        ok( !ret, "HidD_GetFeature succeeded, last error %u\n", GetLastError() );
        ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
            "HidD_GetFeature returned error %u\n", GetLastError() );
    }
    else
    {
        ok( ret, "HidD_GetFeature failed, last error %u\n", GetLastError() );
        ok( buffer[0] == 0x5a, "got buffer[0] %x, expected 0x5a\n", (BYTE)buffer[0] );
    }

    SetLastError( 0xdeadbeef );
    ret = HidD_GetFeature( file, report, caps.FeatureReportByteLength );
    ok( ret, "HidD_GetFeature failed, last error %u\n", GetLastError() );
    ok( report[0] == report_id, "got report[0] %02x, expected %02x\n", report[0], report_id );

    value = caps.FeatureReportByteLength * 2;
    SetLastError( 0xdeadbeef );
    ret = sync_ioctl( file, IOCTL_HID_GET_FEATURE, NULL, 0, report, &value );
    ok( ret, "IOCTL_HID_GET_FEATURE failed, last error %u\n", GetLastError() );
    ok( value == 3, "got length %u, expected 3\n", value );
    ok( report[0] == report_id, "got report[0] %02x, expected %02x\n", report[0], report_id );

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Feature, report_id, preparsed_data, report,
                                         caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#x\n", status );

    SetLastError( 0xdeadbeef );
    ret = HidD_SetFeature( file, report, 0 );
    ok( !ret, "HidD_SetFeature succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER, "HidD_SetFeature returned error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = HidD_SetFeature( file, report, caps.FeatureReportByteLength - 1 );
    ok( !ret, "HidD_SetFeature succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
        "HidD_SetFeature returned error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( buffer, 0x5a, sizeof(buffer) );
    ret = HidD_SetFeature( file, buffer, caps.FeatureReportByteLength );
    if (report_id || broken( !ret ))
    {
        ok( !ret, "HidD_SetFeature succeeded, last error %u\n", GetLastError() );
        ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
            "HidD_SetFeature returned error %u\n", GetLastError() );
    }
    else
    {
        ok( ret, "HidD_SetFeature failed, last error %u\n", GetLastError() );
    }

    SetLastError( 0xdeadbeef );
    ret = HidD_SetFeature( file, report, caps.FeatureReportByteLength );
    ok( ret, "HidD_SetFeature failed, last error %u\n", GetLastError() );

    value = caps.FeatureReportByteLength * 2;
    SetLastError( 0xdeadbeef );
    ret = sync_ioctl( file, IOCTL_HID_SET_FEATURE, NULL, 0, report, &value );
    ok( !ret, "IOCTL_HID_SET_FEATURE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER, "IOCTL_HID_SET_FEATURE returned error %u\n",
        GetLastError() );
    value = 0;
    SetLastError( 0xdeadbeef );
    ret = sync_ioctl( file, IOCTL_HID_SET_FEATURE, report, caps.FeatureReportByteLength * 2, NULL, &value );
    ok( ret, "IOCTL_HID_SET_FEATURE failed, last error %u\n", GetLastError() );
    ok( value == 3, "got length %u, expected 3\n", value );

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Output, report_id, preparsed_data, report, caps.OutputReportByteLength );
    ok( status == HIDP_STATUS_REPORT_DOES_NOT_EXIST, "HidP_InitializeReportForID returned %#x\n", status );
    memset( report, 0, caps.OutputReportByteLength );
    report[0] = report_id;

    SetLastError( 0xdeadbeef );
    ret = HidD_SetOutputReport( file, report, 0 );
    ok( !ret, "HidD_SetOutputReport succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER, "HidD_SetOutputReport returned error %u\n",
        GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = HidD_SetOutputReport( file, report, caps.OutputReportByteLength - 1 );
    ok( !ret, "HidD_SetOutputReport succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
        "HidD_SetOutputReport returned error %u\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    memset( buffer, 0x5a, sizeof(buffer) );
    ret = HidD_SetOutputReport( file, buffer, caps.OutputReportByteLength );
    if (report_id || broken( !ret ))
    {
        ok( !ret, "HidD_SetOutputReport succeeded, last error %u\n", GetLastError() );
        ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
            "HidD_SetOutputReport returned error %u\n", GetLastError() );
    }
    else
    {
        ok( ret, "HidD_SetOutputReport failed, last error %u\n", GetLastError() );
    }

    SetLastError( 0xdeadbeef );
    ret = HidD_SetOutputReport( file, report, caps.OutputReportByteLength );
    ok( ret, "HidD_SetOutputReport failed, last error %u\n", GetLastError() );

    value = caps.OutputReportByteLength * 2;
    SetLastError( 0xdeadbeef );
    ret = sync_ioctl( file, IOCTL_HID_SET_OUTPUT_REPORT, NULL, 0, report, &value );
    ok( !ret, "IOCTL_HID_SET_OUTPUT_REPORT succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER,
        "IOCTL_HID_SET_OUTPUT_REPORT returned error %u\n", GetLastError() );
    value = 0;
    SetLastError( 0xdeadbeef );
    ret = sync_ioctl( file, IOCTL_HID_SET_OUTPUT_REPORT, report, caps.OutputReportByteLength * 2, NULL, &value );
    ok( ret, "IOCTL_HID_SET_OUTPUT_REPORT failed, last error %u\n", GetLastError() );
    ok( value == 3, "got length %u, expected 3\n", value );

    SetLastError( 0xdeadbeef );
    ret = WriteFile( file, report, 0, &value, NULL );
    ok( !ret, "WriteFile succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER, "WriteFile returned error %u\n", GetLastError() );
    ok( value == 0, "WriteFile returned %x\n", value );
    SetLastError( 0xdeadbeef );
    ret = WriteFile( file, report, caps.OutputReportByteLength - 1, &value, NULL );
    ok( !ret, "WriteFile succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_INVALID_USER_BUFFER,
        "WriteFile returned error %u\n", GetLastError() );
    ok( value == 0, "WriteFile returned %x\n", value );

    memset( report, 0xcd, sizeof(report) );
    report[0] = 0xa5;
    SetLastError( 0xdeadbeef );
    ret = WriteFile( file, report, caps.OutputReportByteLength * 2, &value, NULL );
    if (report_id || broken( !ret ) /* w7u */)
    {
        ok( !ret, "WriteFile succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "WriteFile returned error %u\n", GetLastError() );
        ok( value == 0, "WriteFile wrote %u\n", value );
        SetLastError( 0xdeadbeef );
        report[0] = report_id;
        ret = WriteFile( file, report, caps.OutputReportByteLength, &value, NULL );
    }

    if (report_id)
    {
        ok( ret, "WriteFile failed, last error %u\n", GetLastError() );
        ok( value == 2, "WriteFile wrote %u\n", value );
    }
    else
    {
        ok( ret, "WriteFile failed, last error %u\n", GetLastError() );
        ok( value == 3, "WriteFile wrote %u\n", value );
    }

    memset( report, 0xcd, sizeof(report) );
    SetLastError( 0xdeadbeef );
    ret = ReadFile( file, report, 0, &value, NULL );
    ok( !ret && GetLastError() == ERROR_INVALID_USER_BUFFER, "ReadFile failed, last error %u\n",
        GetLastError() );
    ok( value == 0, "ReadFile returned %x\n", value );
    SetLastError( 0xdeadbeef );
    ret = ReadFile( file, report, caps.InputReportByteLength - 1, &value, NULL );
    ok( !ret && GetLastError() == ERROR_INVALID_USER_BUFFER, "ReadFile failed, last error %u\n",
        GetLastError() );
    ok( value == 0, "ReadFile returned %x\n", value );

    if (polled)
    {
        memset( report, 0xcd, sizeof(report) );
        SetLastError( 0xdeadbeef );
        ret = ReadFile( file, report, caps.InputReportByteLength, &value, NULL );
        ok( ret, "ReadFile failed, last error %u\n", GetLastError() );
        ok( value == (report_id ? 3 : 4), "ReadFile returned %x\n", value );
        ok( report[0] == report_id, "unexpected report data\n" );

        overlapped.hEvent = CreateEventW( NULL, FALSE, FALSE, NULL );
        overlapped2.hEvent = CreateEventW( NULL, FALSE, FALSE, NULL );

        /* drain available input reports */
        SetLastError( 0xdeadbeef );
        while (ReadFile( async_file, report, caps.InputReportByteLength, NULL, &overlapped ))
            ResetEvent( overlapped.hEvent );
        ok( GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %u\n", GetLastError() );
        ret = GetOverlappedResult( async_file, &overlapped, &value, TRUE );
        ok( ret, "GetOverlappedResult failed, last error %u\n", GetLastError() );
        ok( value == (report_id ? 3 : 4), "GetOverlappedResult returned length %u, expected 3\n", value );
        ResetEvent( overlapped.hEvent );

        memcpy( buffer, report, caps.InputReportByteLength );
        memcpy( buffer + caps.InputReportByteLength, report, caps.InputReportByteLength );

        SetLastError( 0xdeadbeef );
        ret = ReadFile( async_file, report, caps.InputReportByteLength, NULL, &overlapped );
        ok( !ret, "ReadFile succeeded\n" );
        ok( GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %u\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = ReadFile( async_file, buffer, caps.InputReportByteLength, NULL, &overlapped2 );
        ok( !ret, "ReadFile succeeded\n" );
        ok( GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %u\n", GetLastError() );

        /* wait for second report to be ready */
        ret = GetOverlappedResult( async_file, &overlapped2, &value, TRUE );
        ok( ret, "GetOverlappedResult failed, last error %u\n", GetLastError() );
        ok( value == (report_id ? 3 : 4), "GetOverlappedResult returned length %u, expected 3\n", value );
        /* first report should be ready and the same */
        ret = GetOverlappedResult( async_file, &overlapped, &value, FALSE );
        ok( ret, "GetOverlappedResult failed, last error %u\n", GetLastError() );
        ok( value == (report_id ? 3 : 4), "GetOverlappedResult returned length %u, expected 3\n", value );
        ok( memcmp( report, buffer + caps.InputReportByteLength, caps.InputReportByteLength ),
            "expected different report\n" );
        ok( !memcmp( report, buffer, caps.InputReportByteLength ), "expected identical reports\n" );

        CloseHandle( overlapped.hEvent );
        CloseHandle( overlapped2.hEvent );
    }

    HidD_FreePreparsedData( preparsed_data );
}

static void test_hid_device( DWORD report_id, DWORD polled, const HIDP_CAPS *expect_caps )
{
    char buffer[FIELD_OFFSET( SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath[MAX_PATH] )];
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(SP_DEVICE_INTERFACE_DATA)};
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_detail = (void *)buffer;
    SP_DEVINFO_DATA device = {sizeof(SP_DEVINFO_DATA)};
    ULONG count, poll_freq, out_len;
    HANDLE file, async_file;
    BOOL ret, found = FALSE;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING string;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    unsigned int i;
    HDEVINFO set;

    winetest_push_context( "id %d%s", report_id, polled ? " poll" : "" );

    set = SetupDiGetClassDevsW( &GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );
    ok( set != INVALID_HANDLE_VALUE, "failed to get device list, error %#x\n", GetLastError() );

    for (i = 0; SetupDiEnumDeviceInfo( set, i, &device ); ++i)
    {
        ret = SetupDiEnumDeviceInterfaces( set, &device, &GUID_DEVINTERFACE_HID, 0, &iface );
        ok( ret, "failed to get interface, error %#x\n", GetLastError() );
        ok( IsEqualGUID( &iface.InterfaceClassGuid, &GUID_DEVINTERFACE_HID ), "wrong class %s\n",
            debugstr_guid( &iface.InterfaceClassGuid ) );
        ok( iface.Flags == SPINT_ACTIVE, "got flags %#x\n", iface.Flags );

        iface_detail->cbSize = sizeof(*iface_detail);
        ret = SetupDiGetDeviceInterfaceDetailW( set, &iface, iface_detail, sizeof(buffer), NULL, NULL );
        ok( ret, "failed to get interface path, error %#x\n", GetLastError() );

        if (wcsstr( iface_detail->DevicePath, L"\\\\?\\hid#winetest#1" ))
        {
            found = TRUE;
            break;
        }
    }

    SetupDiDestroyDeviceInfoList( set );

    todo_wine
    ok( found, "didn't find device\n" );

    file = CreateFileW( iface_detail->DevicePath, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
    ok( file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError() );

    count = 0xdeadbeef;
    SetLastError( 0xdeadbeef );
    ret = HidD_GetNumInputBuffers( file, &count );
    ok( ret, "HidD_GetNumInputBuffers failed last error %u\n", GetLastError() );
    ok( count == 32, "HidD_GetNumInputBuffers returned %u\n", count );

    SetLastError( 0xdeadbeef );
    ret = HidD_SetNumInputBuffers( file, 1 );
    ok( !ret, "HidD_SetNumInputBuffers succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "HidD_SetNumInputBuffers returned error %u\n",
        GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = HidD_SetNumInputBuffers( file, 513 );
    ok( !ret, "HidD_SetNumInputBuffers succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "HidD_SetNumInputBuffers returned error %u\n",
        GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = HidD_SetNumInputBuffers( file, 16 );
    ok( ret, "HidD_SetNumInputBuffers failed last error %u\n", GetLastError() );

    count = 0xdeadbeef;
    SetLastError( 0xdeadbeef );
    ret = HidD_GetNumInputBuffers( file, &count );
    ok( ret, "HidD_GetNumInputBuffers failed last error %u\n", GetLastError() );
    ok( count == 16, "HidD_GetNumInputBuffers returned %u\n", count );

    async_file = CreateFileW( iface_detail->DevicePath, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                              FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL );
    ok( async_file != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError() );

    count = 0xdeadbeef;
    SetLastError( 0xdeadbeef );
    ret = HidD_GetNumInputBuffers( async_file, &count );
    ok( ret, "HidD_GetNumInputBuffers failed last error %u\n", GetLastError() );
    ok( count == 32, "HidD_GetNumInputBuffers returned %u\n", count );

    SetLastError( 0xdeadbeef );
    ret = HidD_SetNumInputBuffers( async_file, 2 );
    ok( ret, "HidD_SetNumInputBuffers failed last error %u\n", GetLastError() );

    count = 0xdeadbeef;
    SetLastError( 0xdeadbeef );
    ret = HidD_GetNumInputBuffers( async_file, &count );
    ok( ret, "HidD_GetNumInputBuffers failed last error %u\n", GetLastError() );
    ok( count == 2, "HidD_GetNumInputBuffers returned %u\n", count );
    count = 0xdeadbeef;
    SetLastError( 0xdeadbeef );
    ret = HidD_GetNumInputBuffers( file, &count );
    ok( ret, "HidD_GetNumInputBuffers failed last error %u\n", GetLastError() );
    ok( count == 16, "HidD_GetNumInputBuffers returned %u\n", count );

    if (polled)
    {
        out_len = sizeof(ULONG);
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( file, IOCTL_HID_GET_POLL_FREQUENCY_MSEC, NULL, 0, &poll_freq, &out_len );
        ok( ret, "IOCTL_HID_GET_POLL_FREQUENCY_MSEC failed last error %u\n", GetLastError() );
        ok( out_len == sizeof(ULONG), "got out_len %u, expected sizeof(ULONG)\n", out_len );
        todo_wine
        ok( poll_freq == 5, "got poll_freq %u, expected 5\n", poll_freq );

        out_len = 0;
        poll_freq = 500;
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( file, IOCTL_HID_SET_POLL_FREQUENCY_MSEC, &poll_freq, sizeof(ULONG), NULL, &out_len );
        ok( ret, "IOCTL_HID_SET_POLL_FREQUENCY_MSEC failed last error %u\n", GetLastError() );
        ok( out_len == 0, "got out_len %u, expected 0\n", out_len );

        out_len = 0;
        poll_freq = 10001;
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( file, IOCTL_HID_SET_POLL_FREQUENCY_MSEC, &poll_freq, sizeof(ULONG), NULL, &out_len );
        ok( ret, "IOCTL_HID_SET_POLL_FREQUENCY_MSEC failed last error %u\n", GetLastError() );
        ok( out_len == 0, "got out_len %u, expected 0\n", out_len );

        out_len = 0;
        poll_freq = 0;
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( file, IOCTL_HID_SET_POLL_FREQUENCY_MSEC, &poll_freq, sizeof(ULONG), NULL, &out_len );
        ok( ret, "IOCTL_HID_SET_POLL_FREQUENCY_MSEC failed last error %u\n", GetLastError() );
        ok( out_len == 0, "got out_len %u, expected 0\n", out_len );

        out_len = sizeof(ULONG);
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( file, IOCTL_HID_GET_POLL_FREQUENCY_MSEC, NULL, 0, &poll_freq, &out_len );
        ok( ret, "IOCTL_HID_GET_POLL_FREQUENCY_MSEC failed last error %u\n", GetLastError() );
        ok( out_len == sizeof(ULONG), "got out_len %u, expected sizeof(ULONG)\n", out_len );
        ok( poll_freq == 10000, "got poll_freq %u, expected 10000\n", poll_freq );

        out_len = 0;
        poll_freq = 500;
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( file, IOCTL_HID_SET_POLL_FREQUENCY_MSEC, &poll_freq, sizeof(ULONG), NULL, &out_len );
        ok( ret, "IOCTL_HID_SET_POLL_FREQUENCY_MSEC failed last error %u\n", GetLastError() );
        ok( out_len == 0, "got out_len %u, expected 0\n", out_len );

        out_len = sizeof(ULONG);
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( async_file, IOCTL_HID_GET_POLL_FREQUENCY_MSEC, NULL, 0, &poll_freq, &out_len );
        ok( ret, "IOCTL_HID_GET_POLL_FREQUENCY_MSEC failed last error %u\n", GetLastError() );
        ok( out_len == sizeof(ULONG), "got out_len %u, expected sizeof(ULONG)\n", out_len );
        ok( poll_freq == 500, "got poll_freq %u, expected 500\n", poll_freq );
    }

    test_hidp( file, async_file, report_id, polled, expect_caps );

    CloseHandle( async_file );
    CloseHandle( file );

    RtlInitUnicodeString( &string, L"\\??\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}" );
    InitializeObjectAttributes( &attr, &string, OBJ_CASE_INSENSITIVE, NULL, NULL );
    status = NtOpenFile( &file, SYNCHRONIZE, &attr, &io, 0, FILE_SYNCHRONOUS_IO_NONALERT );
    todo_wine
    ok( status == STATUS_UNSUCCESSFUL, "got %#x\n", status );

    winetest_pop_context();
}

static void test_hid_driver( DWORD report_id, DWORD polled )
{
#include "psh_hid_macros.h"
/* Replace REPORT_ID with USAGE_PAGE when id is 0 */
#define REPORT_ID_OR_USAGE_PAGE(size, id, off) SHORT_ITEM_1((id ? 8 : 0), 1, (id + off))
    const unsigned char report_desc[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Logical),
                REPORT_ID_OR_USAGE_PAGE(1, report_id, 0),
                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_X),
                USAGE(1, HID_USAGE_GENERIC_Y),
                LOGICAL_MINIMUM(1, -128),
                LOGICAL_MAXIMUM(1, 127),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 1),
                USAGE_MAXIMUM(1, 8),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE_MINIMUM(1, 0x18),
                USAGE_MAXIMUM(1, 0x1f),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                INPUT(1, Cnst|Var|Abs),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                INPUT(1, Cnst|Var|Abs),
                /* needs to be 8 bit aligned as next has Buff */

                USAGE_MINIMUM(4, (HID_USAGE_PAGE_KEYBOARD<<16)|0x8),
                USAGE_MAXIMUM(4, (HID_USAGE_PAGE_KEYBOARD<<16)|0xf),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 8),
                REPORT_COUNT(1, 2),
                REPORT_SIZE(1, 8),
                INPUT(2, Data|Ary|Rel|Wrap|Lin|Pref|Null|Vol|Buff),

                /* needs to be 8 bit aligned as previous has Buff */
                USAGE(1, 0x20),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                INPUT(1, Data|Var|Abs),
                USAGE_MINIMUM(1, 0x21),
                USAGE_MAXIMUM(1, 0x22),
                REPORT_COUNT(1, 2),
                REPORT_SIZE(1, 0),
                INPUT(1, Data|Var|Abs),
                USAGE(1, 0x23),
                REPORT_COUNT(1, 0),
                REPORT_SIZE(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_HATSWITCH),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 8),
                REPORT_SIZE(1, 4),
                REPORT_COUNT(1, 2),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_Z),
                LOGICAL_MINIMUM(4, 0x00000000),
                LOGICAL_MAXIMUM(4, 0x3fffffff),
                PHYSICAL_MINIMUM(4, 0x80000000),
                PHYSICAL_MAXIMUM(4, 0x7fffffff),
                REPORT_SIZE(1, 32),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),

                /* reset physical range to its default interpretation */
                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_RX),
                PHYSICAL_MINIMUM(4, 0),
                PHYSICAL_MAXIMUM(4, 0),
                REPORT_SIZE(1, 32),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_RY),
                LOGICAL_MINIMUM(4, 0x7fff),
                LOGICAL_MAXIMUM(4, 0x0000),
                PHYSICAL_MINIMUM(4, 0x0000),
                PHYSICAL_MAXIMUM(4, 0x7fff),
                REPORT_SIZE(1, 32),
                REPORT_COUNT(1, 1),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Report),
                REPORT_ID_OR_USAGE_PAGE(1, report_id, 1),
                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 9),
                USAGE_MAXIMUM(1, 10),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_LED),
            USAGE(1, HID_USAGE_LED_GREEN),
            COLLECTION(1, Report),
                REPORT_ID_OR_USAGE_PAGE(1, report_id, 0),
                USAGE_PAGE(1, HID_USAGE_PAGE_LED),
                USAGE(1, 1),
                USAGE(1, 2),
                USAGE(1, 3),
                USAGE(1, 4),
                USAGE(1, 5),
                USAGE(1, 6),
                USAGE(1, 7),
                USAGE(1, 8),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE(4, (HID_USAGE_PAGE_KEYBOARD<<16)|0x8c),
                USAGE(4, (HID_USAGE_PAGE_KEYBOARD<<16)|0x8d),
                USAGE(4, (HID_USAGE_PAGE_KEYBOARD<<16)|0x8e),
                USAGE(4, (HID_USAGE_PAGE_KEYBOARD<<16)|0x8f),
                LOGICAL_MINIMUM(1, 1),
                LOGICAL_MAXIMUM(1, 16),
                REPORT_COUNT(1, 2),
                REPORT_SIZE(1, 8),
                INPUT(1, Data|Ary|Abs),
            END_COLLECTION,

            USAGE_PAGE(2, HID_USAGE_PAGE_HAPTICS),
            USAGE(1, HID_USAGE_HAPTICS_SIMPLE_CONTROLLER),
            COLLECTION(1, Logical),
                REPORT_ID_OR_USAGE_PAGE(1, report_id, 0),
                USAGE_PAGE(2, HID_USAGE_PAGE_HAPTICS),

                USAGE(1, HID_USAGE_HAPTICS_WAVEFORM_LIST),
                COLLECTION(1, NamedArray),
                    USAGE_PAGE(1, HID_USAGE_PAGE_ORDINAL),
                    USAGE(1, 3), /* HID_USAGE_HAPTICS_WAVEFORM_RUMBLE */
                    USAGE(1, 4), /* HID_USAGE_HAPTICS_WAVEFORM_BUZZ */
                    LOGICAL_MINIMUM(2, 0x0000),
                    LOGICAL_MAXIMUM(2, 0xffff),
                    REPORT_COUNT(1, 2),
                    REPORT_SIZE(1, 16),
                    FEATURE(1, Data|Var|Abs|Null),
                END_COLLECTION,

                USAGE_PAGE(2, HID_USAGE_PAGE_HAPTICS),
                USAGE(1, HID_USAGE_HAPTICS_DURATION_LIST),
                COLLECTION(1, NamedArray),
                    USAGE_PAGE(1, HID_USAGE_PAGE_ORDINAL),
                    USAGE(1, 3), /* 0 (HID_USAGE_HAPTICS_WAVEFORM_RUMBLE) */
                    USAGE(1, 4), /* 0 (HID_USAGE_HAPTICS_WAVEFORM_BUZZ) */
                    LOGICAL_MINIMUM(2, 0x0000),
                    LOGICAL_MAXIMUM(2, 0xffff),
                    REPORT_COUNT(1, 2),
                    REPORT_SIZE(1, 16),
                    FEATURE(1, Data|Var|Abs|Null),
                END_COLLECTION,

                USAGE_PAGE(2, HID_USAGE_PAGE_HAPTICS),
                USAGE(1, HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME),
                UNIT(2, 0x1001), /* seconds */
                UNIT_EXPONENT(1, -3), /* 10^-3 */
                LOGICAL_MINIMUM(2, 0x8000),
                LOGICAL_MAXIMUM(2, 0x7fff),
                PHYSICAL_MINIMUM(4, 0x00000000),
                PHYSICAL_MAXIMUM(4, 0xffffffff),
                REPORT_SIZE(1, 32),
                REPORT_COUNT(1, 2),
                FEATURE(1, Data|Var|Abs),
                /* reset global items */
                UNIT(1, 0), /* None */
                UNIT_EXPONENT(1, 0),

                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_Z),
                LOGICAL_MINIMUM(4, 0x0000),
                LOGICAL_MAXIMUM(4, 0x7fff),
                PHYSICAL_MINIMUM(4, 0xfff90000),
                PHYSICAL_MAXIMUM(4, 0x0003ffff),
                REPORT_SIZE(1, 32),
                REPORT_COUNT(1, 1),
                FEATURE(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Report),
                REPORT_ID_OR_USAGE_PAGE(1, report_id, 1),
                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 9),
                USAGE_MAXIMUM(1, 10),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                FEATURE(1, Data|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_LED),
            USAGE(1, HID_USAGE_LED_GREEN),
            COLLECTION(1, Report),
                REPORT_ID_OR_USAGE_PAGE(1, report_id, 0),
                USAGE_PAGE(1, HID_USAGE_PAGE_LED),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                OUTPUT(1, Cnst|Var|Abs),
            END_COLLECTION,

            USAGE_PAGE(1, HID_USAGE_PAGE_LED),
            USAGE(1, HID_USAGE_LED_RED),
            COLLECTION(1, Report),
                REPORT_ID_OR_USAGE_PAGE(1, report_id, 1),
                USAGE_PAGE(1, HID_USAGE_PAGE_LED),
                REPORT_COUNT(1, 8),
                REPORT_SIZE(1, 1),
                OUTPUT(1, Cnst|Var|Abs),
            END_COLLECTION,
        END_COLLECTION,
    };
#undef REPORT_ID_OR_USAGE_PAGE
#include "pop_hid_macros.h"

    static const HID_DEVICE_ATTRIBUTES attributes =
    {
        .Size = sizeof(HID_DEVICE_ATTRIBUTES),
        .VendorID = 0x1209,
        .ProductID = 0x0001,
        .VersionNumber = 0x0100,
    };
    const HIDP_CAPS caps =
    {
        .Usage = HID_USAGE_GENERIC_JOYSTICK,
        .UsagePage = HID_USAGE_PAGE_GENERIC,
        .InputReportByteLength = report_id ? 25 : 26,
        .OutputReportByteLength = report_id ? 2 : 3,
        .FeatureReportByteLength = report_id ? 21 : 22,
        .NumberLinkCollectionNodes = 10,
        .NumberInputButtonCaps = 17,
        .NumberInputValueCaps = 7,
        .NumberInputDataIndices = 47,
        .NumberFeatureButtonCaps = 1,
        .NumberFeatureValueCaps = 6,
        .NumberFeatureDataIndices = 8,
    };

    WCHAR cwd[MAX_PATH], tempdir[MAX_PATH];
    LSTATUS status;
    HKEY hkey;

    GetCurrentDirectoryW( ARRAY_SIZE(cwd), cwd );
    GetTempPathW( ARRAY_SIZE(tempdir), tempdir );
    SetCurrentDirectoryW( tempdir );

    status = RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services\\winetest",
                              0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL );
    ok( !status, "RegCreateKeyExW returned %#x\n", status );

    status = RegSetValueExW( hkey, L"ReportID", 0, REG_DWORD, (void *)&report_id, sizeof(report_id) );
    ok( !status, "RegSetValueExW returned %#x\n", status );

    status = RegSetValueExW( hkey, L"PolledMode", 0, REG_DWORD, (void *)&polled, sizeof(polled) );
    ok( !status, "RegSetValueExW returned %#x\n", status );

    status = RegSetValueExW( hkey, L"Descriptor", 0, REG_BINARY, (void *)report_desc, sizeof(report_desc) );
    ok( !status, "RegSetValueExW returned %#x\n", status );

    status = RegSetValueExW( hkey, L"Attributes", 0, REG_BINARY, (void *)&attributes, sizeof(attributes) );
    ok( !status, "RegSetValueExW returned %#x\n", status );

    status = RegSetValueExW( hkey, L"Caps", 0, REG_BINARY, (void *)&caps, sizeof(caps) );
    ok( !status, "RegSetValueExW returned %#x\n", status );

    if (pnp_driver_start( L"driver_hid.dll" )) test_hid_device( report_id, polled, &caps );

    pnp_driver_stop();
    SetCurrentDirectoryW( cwd );
}

START_TEST( hid )
{
    HANDLE mapping;
    BOOL is_wow64;

    pSignerSign = (void *)GetProcAddress( LoadLibraryW( L"mssign32" ), "SignerSign" );

    if (IsWow64Process( GetCurrentProcess(), &is_wow64 ) && is_wow64)
    {
        skip( "Running in WoW64.\n" );
        return;
    }

    mapping = CreateFileMappingW( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(*test_data),
                                  L"Global\\winetest_dinput_section" );
    if (!mapping && GetLastError() == ERROR_ACCESS_DENIED)
    {
        win_skip( "Failed to create test data mapping.\n" );
        return;
    }
    ok( !!mapping, "got error %u\n", GetLastError() );
    test_data = MapViewOfFile( mapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 1024 );
    test_data->running_under_wine = !strcmp( winetest_platform, "wine" );
    test_data->winetest_report_success = winetest_report_success;
    test_data->winetest_debug = winetest_debug;

    okfile = CreateFileW( L"C:\\windows\\winetest_dinput_okfile", GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL );
    ok( okfile != INVALID_HANDLE_VALUE, "failed to create file, error %u\n", GetLastError() );

    subtest( "driver_hid" );
    test_hid_driver( 0, FALSE );
    test_hid_driver( 1, FALSE );
    test_hid_driver( 0, TRUE );
    test_hid_driver( 1, TRUE );

    UnmapViewOfFile( test_data );
    CloseHandle( mapping );
    CloseHandle( okfile );
    DeleteFileW( L"C:\\windows\\winetest_dinput_okfile" );
}
