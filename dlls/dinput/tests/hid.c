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

#define DIRECTINPUT_VERSION 0x0800

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
#include "dbt.h"
#include "devguid.h"

#include "objbase.h"

#define COBJMACROS

#include "initguid.h"
#include "ddk/wdm.h"
#include "ddk/hidclass.h"
#include "ddk/hidsdi.h"
#include "ddk/hidpi.h"
#include "ddk/hidport.h"
#include "dinput.h"
#include "dinputd.h"
#include "hidusage.h"

#include "wine/mssign.h"

#include "dinput_test.h"

HINSTANCE instance;
BOOL localized; /* object names get translated */

const HID_DEVICE_ATTRIBUTES default_attributes =
{
    .Size = sizeof(HID_DEVICE_ATTRIBUTES),
    .VendorID = LOWORD(EXPECT_VIDPID),
    .ProductID = HIWORD(EXPECT_VIDPID),
    .VersionNumber = 0x0100,
};
const WCHAR expect_vidpid_str[] = L"VID_1209&PID_0001";
const GUID expect_guid_product = {EXPECT_VIDPID, 0x0000, 0x0000, {0x00, 0x00, 'P', 'I', 'D', 'V', 'I', 'D'}};
const WCHAR expect_path[] = L"\\\\?\\hid#vid_1209&pid_0001#2&";
const WCHAR expect_path_end[] = L"&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}";
static HANDLE device_added, device_removed;

static struct winetest_shared_data *test_data;
static HANDLE monitor_thread, monitor_stop;
static HANDLE test_data_mapping;
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
        ok( ret, "Failed to delete container, error %#lx\n", GetLastError() );
        ret = CryptAcquireContextW( &provider, container_name, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET );
    }
    ok( ret, "Failed to create container, error %#lx\n", GetLastError() );

    ret = CryptGenKey( provider, AT_SIGNATURE, CRYPT_EXPORTABLE, &key );
    ok( ret, "Failed to create key, error %#lx\n", GetLastError() );
    ret = CryptDestroyKey( key );
    ok( ret, "Failed to destroy key, error %#lx\n", GetLastError() );
    ret = CryptGetUserKey( provider, AT_SIGNATURE, &key );
    ok( ret, "Failed to get user key, error %#lx\n", GetLastError() );
    ret = CryptDestroyKey( key );
    ok( ret, "Failed to destroy key, error %#lx\n", GetLastError() );

    size = sizeof(encoded_name);
    ret = CertStrToNameW( X509_ASN_ENCODING, L"CN=winetest_cert", CERT_X500_NAME_STR, NULL,
                          encoded_name, &size, NULL );
    ok( ret, "Failed to convert name, error %#lx\n", GetLastError() );
    key_info.CertIssuer.cbData = size;
    key_info.CertIssuer.pbData = encoded_name;

    size = sizeof(public_key_info_buffer);
    ret = CryptExportPublicKeyInfo( provider, AT_SIGNATURE, X509_ASN_ENCODING, public_key_info, &size );
    ok( ret, "Failed to export public key, error %#lx\n", GetLastError() );
    cert_info.SubjectPublicKeyInfo = *public_key_info;

    size = sizeof(hash_buffer);
    ret = CryptHashPublicKeyInfo( provider, CALG_MD5, 0, X509_ASN_ENCODING, public_key_info, hash_buffer, &size );
    ok( ret, "Failed to hash public key, error %#lx\n", GetLastError() );

    key_info.KeyId.cbData = size;
    key_info.KeyId.pbData = hash_buffer;

    RtlGenRandom( serial, sizeof(serial) );
    key_info.CertSerialNumber.cbData = sizeof(serial);
    key_info.CertSerialNumber.pbData = serial;

    size = sizeof(encoded_key_id);
    ret = CryptEncodeObject( X509_ASN_ENCODING, X509_AUTHORITY_KEY_ID, &key_info, encoded_key_id, &size );
    ok( ret, "Failed to convert name, error %#lx\n", GetLastError() );

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
    ok( ret, "Failed to create certificate, error %#lx\n", GetLastError() );

    cert = CertCreateCertificateContext( X509_ASN_ENCODING, cert_buffer, size );
    ok( !!cert, "Failed to create context, error %#lx\n", GetLastError() );

    size = sizeof(provider_nameA);
    ret = CryptGetProvParam( provider, PP_NAME, provider_nameA, &size, 0 );
    ok( ret, "Failed to get prov param, error %#lx\n", GetLastError() );
    MultiByteToWideChar( CP_ACP, 0, (char *)provider_nameA, -1, provider_nameW, ARRAY_SIZE(provider_nameW) );

    provider_info.pwszContainerName = (WCHAR *)container_name;
    provider_info.pwszProvName = provider_nameW;
    provider_info.dwProvType = PROV_RSA_FULL;
    provider_info.dwKeySpec = AT_SIGNATURE;
    ret = CertSetCertificateContextProperty( cert, CERT_KEY_PROV_INFO_PROP_ID, 0, &provider_info );
    ok( ret, "Failed to set provider info, error %#lx\n", GetLastError() );

    ret = CryptReleaseContext( provider, 0 );
    ok( ret, "failed to release context, error %lu\n", GetLastError() );

    root_store = CertOpenStore( CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0, CERT_SYSTEM_STORE_LOCAL_MACHINE, L"root" );
    if (!root_store && GetLastError() == ERROR_ACCESS_DENIED)
    {
        win_skip( "Failed to open root store.\n" );
        ret = CertFreeCertificateContext( cert );
        ok( ret, "Failed to free certificate, error %lu\n", GetLastError() );
        return NULL;
    }
    ok( !!root_store, "Failed to open store, error %lu\n", GetLastError() );
    ret = CertAddCertificateContextToStore( root_store, cert, CERT_STORE_ADD_ALWAYS, NULL );
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        win_skip( "Failed to add self-signed certificate to store.\n" );
        ret = CertFreeCertificateContext( cert );
        ok( ret, "Failed to free certificate, error %lu\n", GetLastError() );
        ret = CertCloseStore( root_store, CERT_CLOSE_STORE_CHECK_FLAG );
        ok( ret, "Failed to close store, error %lu\n", GetLastError() );
        return NULL;
    }
    ok( ret, "Failed to add certificate, error %lu\n", GetLastError() );
    ret = CertCloseStore( root_store, CERT_CLOSE_STORE_CHECK_FLAG );
    ok( ret, "Failed to close store, error %lu\n", GetLastError() );

    pub_store = CertOpenStore( CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
                               CERT_SYSTEM_STORE_LOCAL_MACHINE, L"trustedpublisher" );
    ok( !!pub_store, "Failed to open store, error %lu\n", GetLastError() );
    ret = CertAddCertificateContextToStore( pub_store, cert, CERT_STORE_ADD_ALWAYS, NULL );
    ok( ret, "Failed to add certificate, error %lu\n", GetLastError() );
    ret = CertCloseStore( pub_store, CERT_CLOSE_STORE_CHECK_FLAG );
    ok( ret, "Failed to close store, error %lu\n", GetLastError() );

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
    ok( hr == S_OK || broken( hr == NTE_BAD_ALGID ) /* < 7 */, "Failed to sign, hr %#lx\n", hr );

    return cert;
}

static void testsign_cleanup( const CERT_CONTEXT *cert )
{
    HCERTSTORE root_store, pub_store;
    const CERT_CONTEXT *store_cert;
    HCRYPTPROV provider;
    BOOL ret;

    root_store = CertOpenStore( CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0, CERT_SYSTEM_STORE_LOCAL_MACHINE, L"root" );
    ok( !!root_store, "Failed to open store, error %lu\n", GetLastError() );
    store_cert = CertFindCertificateInStore( root_store, X509_ASN_ENCODING, 0, CERT_FIND_EXISTING, cert, NULL );
    ok( !!store_cert, "Failed to find root certificate, error %lu\n", GetLastError() );
    ret = CertDeleteCertificateFromStore( store_cert );
    ok( ret, "Failed to remove certificate, error %lu\n", GetLastError() );
    ret = CertCloseStore( root_store, CERT_CLOSE_STORE_CHECK_FLAG );
    ok( ret, "Failed to close store, error %lu\n", GetLastError() );

    pub_store = CertOpenStore( CERT_STORE_PROV_SYSTEM_REGISTRY_W, 0, 0,
                               CERT_SYSTEM_STORE_LOCAL_MACHINE, L"trustedpublisher" );
    ok( !!pub_store, "Failed to open store, error %lu\n", GetLastError() );
    store_cert = CertFindCertificateInStore( pub_store, X509_ASN_ENCODING, 0, CERT_FIND_EXISTING, cert, NULL );
    ok( !!store_cert, "Failed to find publisher certificate, error %lu\n", GetLastError() );
    ret = CertDeleteCertificateFromStore( store_cert );
    ok( ret, "Failed to remove certificate, error %lu\n", GetLastError() );
    ret = CertCloseStore( pub_store, CERT_CLOSE_STORE_CHECK_FLAG );
    ok( ret, "Failed to close store, error %lu\n", GetLastError() );

    ret = CertFreeCertificateContext( cert );
    ok( ret, "Failed to free certificate, error %lu\n", GetLastError() );

    ret = CryptAcquireContextW( &provider, container_name, NULL, PROV_RSA_FULL, CRYPT_DELETEKEYSET );
    ok( ret, "Failed to delete container, error %#lx\n", GetLastError() );
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
    ok( file != INVALID_HANDLE_VALUE, "failed to create %s, error %lu\n", debugstr_w(filename), GetLastError() );

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
    "Wine Test Bus Device=bus_section,WINETEST\\BUS\n"
    "Wine Test HID Device=hid_section,WINETEST\\WINE_COMP_HID\n"
    "Wine Test HID Polled Device=hid_poll_section,WINETEST\\WINE_COMP_POLLHID\n"

    "[bus_section.NT" EXT "]\n"
    "CopyFiles=file_section\n"

    "[bus_section.NT" EXT ".Services]\n"
    "AddService=winetest_bus,0x2,bus_service\n"

    "[hid_section.NT" EXT "]\n"
    "CopyFiles=file_section\n"

    "[hid_section.NT" EXT ".Services]\n"
    "AddService=winetest_hid,0x2,hid_service\n"

    "[hid_poll_section.NT" EXT "]\n"
    "CopyFiles=file_section\n"

    "[hid_poll_section.NT" EXT ".Services]\n"
    "AddService=winetest_hid_poll,0x2,hid_poll_service\n"

    "[file_section]\n"
    "winetest_bus.sys\n"
    "winetest_hid.sys\n"
    "winetest_hid_poll.sys\n"

    "[SourceDisksFiles]\n"
    "winetest_bus.sys=1\n"
    "winetest_hid.sys=1\n"
    "winetest_hid_poll.sys=1\n"

    "[SourceDisksNames]\n"
    "1=,winetest_bus.sys\n"
    "1=,winetest_hid.sys\n"
    "1=,winetest_hid_poll.sys\n"

    "[DestinationDirs]\n"
    "DefaultDestDir=12\n"

    "[bus_service]\n"
    "ServiceBinary=%12%\\winetest_bus.sys\n"
    "ServiceType=1\n"
    "StartType=3\n"
    "ErrorControl=1\n"
    "LoadOrderGroup=WinePlugPlay\n"
    "DisplayName=\"Wine Test Bus Driver\"\n"

    "[hid_service]\n"
    "ServiceBinary=%12%\\winetest_hid.sys\n"
    "ServiceType=1\n"
    "StartType=3\n"
    "ErrorControl=1\n"
    "LoadOrderGroup=WinePlugPlay\n"
    "DisplayName=\"Wine Test HID Driver\"\n"

    "[hid_poll_service]\n"
    "ServiceBinary=%12%\\winetest_hid_poll.sys\n"
    "ServiceType=1\n"
    "StartType=3\n"
    "ErrorControl=1\n"
    "LoadOrderGroup=WinePlugPlay\n"
    "DisplayName=\"Wine Test HID Polled Driver\"\n"
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
    ok( ret, "Failed to get subject guid, error %lu\n", GetLastError() );

    size = 0;
    subject_info.pgSubjectType = &subject_guid;
    subject_info.pwsFileName = file;
    subject_info.DigestAlgorithm.pszObjId = (char *)szOID_OIWSEC_sha1;
    subject_info.dwFlags = SPC_INC_PE_RESOURCES_FLAG | SPC_INC_PE_IMPORT_ADDR_TABLE_FLAG |
                           SPC_EXC_PE_PAGE_HASHES_FLAG | 0x10000;
    ret = CryptSIPCreateIndirectData( &subject_info, &size, NULL );
    todo_wine
    ok( ret, "Failed to get indirect data size, error %lu\n", GetLastError() );

    indirect_data = malloc( size );
    ret = CryptSIPCreateIndirectData( &subject_info, &size, indirect_data );
    todo_wine
    ok( ret, "Failed to get indirect data, error %lu\n", GetLastError() );
    if (ret)
    {
        memset( hash_buffer, 0, sizeof(hash_buffer) );
        for (i = 0; i < indirect_data->Digest.cbData; ++i)
            swprintf( &hash_buffer[i * 2], 2, L"%02X", indirect_data->Digest.pbData[i] );

        member = CryptCATPutMemberInfo( catalog, (WCHAR *)file, hash_buffer, &subject_guid,
                                        0, size, (BYTE *)indirect_data );
        ok( !!member, "Failed to write member, error %lu\n", GetLastError() );

        if (wcsrchr( file, '\\' )) filepart = wcsrchr( file, '\\' ) + 1;

        ret = !!CryptCATPutAttrInfo( catalog, member, (WCHAR *)L"File",
                                     CRYPTCAT_ATTR_NAMEASCII | CRYPTCAT_ATTR_DATAASCII | CRYPTCAT_ATTR_AUTHENTICATED,
                                     (wcslen( filepart ) + 1) * 2, (BYTE *)filepart );
        ok( ret, "Failed to write attr, error %lu\n", GetLastError() );

        ret = !!CryptCATPutAttrInfo( catalog, member, (WCHAR *)L"OSAttr",
                                     CRYPTCAT_ATTR_NAMEASCII | CRYPTCAT_ATTR_DATAASCII | CRYPTCAT_ATTR_AUTHENTICATED,
                                     sizeof(L"2:6.0"), (BYTE *)L"2:6.0" );
        ok( ret, "Failed to write attr, error %lu\n", GetLastError() );
    }
    free( indirect_data );
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
        ok( ret, "QueryServiceStatus failed: %lu\n", GetLastError() );
    }
    ok( status.dwCurrentState == SERVICE_STOPPED || !status.dwCurrentState,
        "expected SERVICE_STOPPED, got %lu\n", status.dwCurrentState );

    DeleteService( service );
    CloseServiceHandle( service );
}

void bus_device_stop(void)
{
    SP_DEVINFO_DATA device = {sizeof(SP_DEVINFO_DATA)};
    WCHAR path[MAX_PATH], dest[MAX_PATH], *filepart;
    const WCHAR *service_name = L"winetest_bus";
    SC_HANDLE manager, service;
    char buffer[512];
    HDEVINFO set;
    HANDLE file;
    DWORD size;
    BOOL ret;

    if (!test_data) return;

    set = SetupDiCreateDeviceInfoList( NULL, NULL );
    ok( set != INVALID_HANDLE_VALUE, "failed to create device list, error %lu\n", GetLastError() );

    ret = SetupDiOpenDeviceInfoW( set, L"root\\winetest\\0", NULL, 0, &device );
    if (!ret && GetLastError() == ERROR_NO_SUCH_DEVINST)
    {
        ret = SetupDiDestroyDeviceInfoList( set );
        ok( ret, "failed to destroy set, error %lu\n", GetLastError() );
        return;
    }
    ok( ret, "failed to open device, error %lu\n", GetLastError() );

    ret = SetupDiCallClassInstaller( DIF_REMOVE, set, &device );
    ok( ret, "failed to remove device, error %lu\n", GetLastError() );

    file = CreateFileW( L"\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}", 0, 0,
                        NULL, OPEN_EXISTING, 0, NULL );
    ok( file == INVALID_HANDLE_VALUE, "expected failure\n" );
    ok( GetLastError() == ERROR_FILE_NOT_FOUND, "got error %lu\n", GetLastError() );

    ret = SetupDiDestroyDeviceInfoList( set );
    ok( ret, "failed to destroy set, error %lu\n", GetLastError() );

    /* Windows stops the service but does not delete it. */
    manager = OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT );
    ok( !!manager, "failed to open service manager, error %lu\n", GetLastError() );

    service = OpenServiceW( manager, service_name, SERVICE_STOP | DELETE );
    if (service) unload_driver( service );
    else ok( GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST, "got error %lu\n", GetLastError() );

    CloseServiceHandle( manager );

    SetFilePointer( okfile, 0, NULL, FILE_BEGIN );
    do
    {
        ReadFile( okfile, buffer, sizeof(buffer), &size, NULL );
        printf( "%.*s", (int)size, buffer );
    } while (size == sizeof(buffer));
    SetFilePointer( okfile, 0, NULL, FILE_BEGIN );
    SetEndOfFile( okfile );

    InterlockedAdd( &winetest_successes, InterlockedExchange( &test_data->successes, 0 ) );
    winetest_add_failures( InterlockedExchange( &test_data->failures, 0 ) );
    InterlockedAdd( &winetest_todo_successes, InterlockedExchange( &test_data->todo_successes, 0 ) );
    winetest_add_failures( InterlockedExchange( &test_data->todo_failures, 0 ) );
    InterlockedAdd( &winetest_skipped, InterlockedExchange( &test_data->skipped, 0 ) );

    GetFullPathNameW( L"winetest.inf", ARRAY_SIZE(path), path, NULL );
    ret = SetupCopyOEMInfW( path, NULL, 0, 0, dest, ARRAY_SIZE(dest), NULL, &filepart );
    ok( ret, "Failed to copy INF, error %lu\n", GetLastError() );
    ret = SetupUninstallOEMInfW( filepart, SUOI_FORCEDELETE, NULL );
    ok( ret, "Failed to uninstall INF, error %lx\n", GetLastError() );

    ret = DeleteFileW( L"winetest.cat" );
    ok( ret, "Failed to delete file, error %lu\n", GetLastError() );
    ret = DeleteFileW( L"winetest.inf" );
    ok( ret, "Failed to delete file, error %lu\n", GetLastError() );
    ret = DeleteFileW( L"winetest_bus.sys" );
    ok( ret, "Failed to delete file, error %lu\n", GetLastError() );
    ret = DeleteFileW( L"winetest_hid.sys" );
    ok( ret, "Failed to delete file, error %lu\n", GetLastError() );
    ret = DeleteFileW( L"winetest_hid_poll.sys" );
    ok( ret, "Failed to delete file, error %lu\n", GetLastError() );
    /* Windows 10 apparently deletes the image in SetupUninstallOEMInf(). */
    ret = DeleteFileW( L"C:/windows/system32/drivers/winetest_bus.sys" );
    ok( ret || GetLastError() == ERROR_FILE_NOT_FOUND, "Failed to delete file, error %lu\n", GetLastError() );
    ret = DeleteFileW( L"C:/windows/system32/drivers/winetest_hid.sys" );
    todo_wine_if(!ret && GetLastError() == ERROR_ACCESS_DENIED) /* Wine doesn't unload device drivers correctly */
    ok( ret || GetLastError() == ERROR_FILE_NOT_FOUND, "Failed to delete file, error %lu\n", GetLastError() );
    ret = DeleteFileW( L"C:/windows/system32/drivers/winetest_hid_poll.sys" );
    todo_wine_if(!ret && GetLastError() == ERROR_ACCESS_DENIED) /* Wine doesn't unload device drivers correctly */
    ok( ret || GetLastError() == ERROR_FILE_NOT_FOUND, "Failed to delete file, error %lu\n", GetLastError() );
}

BOOL find_hid_device_path( WCHAR *device_path )
{
    char buffer[FIELD_OFFSET( SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath[MAX_PATH] )] = {0};
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(SP_DEVICE_INTERFACE_DATA)};
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *iface_detail = (void *)buffer;
    SP_DEVINFO_DATA device = {sizeof(SP_DEVINFO_DATA)};
    ULONG i, len = wcslen( device_path );
    HDEVINFO set;
    BOOL ret;

    set = SetupDiGetClassDevsW( &GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT );
    ok( set != INVALID_HANDLE_VALUE, "Failed to get device list, error %#lx\n", GetLastError() );

    for (i = 0; SetupDiEnumDeviceInfo( set, i, &device ); ++i)
    {
        SetLastError( 0xdeadbeef );
        ret = SetupDiEnumDeviceInterfaces( set, &device, &GUID_DEVINTERFACE_HID, 0, &iface );
        todo_wine_if(!ret && GetLastError() == ERROR_NO_MORE_ITEMS) /* Wine doesn't unload device drivers correctly */
        ok( ret, "Failed to get interface, error %#lx\n", GetLastError() );
        if (!ret) continue;
        ok( IsEqualGUID( &iface.InterfaceClassGuid, &GUID_DEVINTERFACE_HID ), "wrong class %s\n",
            debugstr_guid( &iface.InterfaceClassGuid ) );
        ok( iface.Flags == SPINT_ACTIVE, "got flags %#lx\n", iface.Flags );

        iface_detail->cbSize = sizeof(*iface_detail);
        ret = SetupDiGetDeviceInterfaceDetailW( set, &iface, iface_detail, sizeof(buffer), NULL, NULL );
        ok( ret, "Failed to get interface path, error %#lx\n", GetLastError() );

        if (!wcsncmp( iface_detail->DevicePath, device_path, len )) break;
    }

    ret = SetupDiDestroyDeviceInfoList( set );
    ok( ret, "Failed to destroy set, error %lu\n", GetLastError() );

    ret = !wcsncmp( iface_detail->DevicePath, device_path, len );
    if (ret) wcscpy( device_path, iface_detail->DevicePath );
    return ret;
}

BOOL bus_device_start(void)
{
    static const WCHAR bus_hardware_ids[] = L"WINETEST\\BUS\0";
    SP_DEVINFO_DATA device = {sizeof(SP_DEVINFO_DATA)};
    const WCHAR *service_name = L"winetest_bus";
    WCHAR path[MAX_PATH], filename[MAX_PATH];
    SC_HANDLE manager, service;
    const CERT_CONTEXT *cert;
    int old_mute_threshold;
    BOOL ret, need_reboot;
    HANDLE catalog;
    HDEVINFO set;
    FILE *f;

    if (!test_data) return FALSE;

    old_mute_threshold = winetest_mute_threshold;
    winetest_mute_threshold = 1;

    load_resource( L"driver_bus.dll", filename );
    ret = MoveFileExW( filename, L"winetest_bus.sys", MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING );
    ok( ret, "failed to move file, error %lu\n", GetLastError() );

    load_resource( L"driver_hid.dll", filename );
    ret = MoveFileExW( filename, L"winetest_hid.sys", MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING );
    ok( ret, "failed to move file, error %lu\n", GetLastError() );

    load_resource( L"driver_hid_poll.dll", filename );
    ret = MoveFileExW( filename, L"winetest_hid_poll.sys", MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING );
    ok( ret, "failed to move file, error %lu\n", GetLastError() );

    f = fopen( "winetest.inf", "w" );
    ok( !!f, "failed to open winetest.inf: %s\n", strerror( errno ) );
    fputs( inf_text, f );
    fclose( f );

    /* Create the catalog file. */

    catalog = CryptCATOpen( (WCHAR *)L"winetest.cat", CRYPTCAT_OPEN_CREATENEW, 0, CRYPTCAT_VERSION_1, 0 );
    ok( catalog != INVALID_HANDLE_VALUE, "Failed to create catalog, error %lu\n", GetLastError() );

    add_file_to_catalog( catalog, L"winetest_bus.sys" );
    add_file_to_catalog( catalog, L"winetest_hid.sys" );
    add_file_to_catalog( catalog, L"winetest_hid_poll.sys" );
    add_file_to_catalog( catalog, L"winetest.inf" );

    ret = CryptCATPersistStore( catalog );
    todo_wine
    ok( ret, "Failed to write catalog, error %lu\n", GetLastError() );

    ret = CryptCATClose( catalog );
    ok( ret, "Failed to close catalog, error %lu\n", GetLastError() );

    if (!(cert = testsign_sign( L"winetest.cat" )))
    {
        ret = DeleteFileW( L"winetest.cat" );
        ok( ret, "Failed to delete file, error %lu\n", GetLastError() );
        ret = DeleteFileW( L"winetest.inf" );
        ok( ret, "Failed to delete file, error %lu\n", GetLastError() );
        ret = DeleteFileW( L"winetest_bus.sys" );
        ok( ret, "Failed to delete file, error %lu\n", GetLastError() );
        ret = DeleteFileW( L"winetest_hid.sys" );
        ok( ret, "Failed to delete file, error %lu\n", GetLastError() );
        ret = DeleteFileW( L"winetest_hid_poll.sys" );
        ok( ret, "Failed to delete file, error %lu\n", GetLastError() );
        winetest_mute_threshold = old_mute_threshold;
        return FALSE;
    }

    /* Install the driver. */

    set = SetupDiCreateDeviceInfoList( NULL, NULL );
    ok( set != INVALID_HANDLE_VALUE, "failed to create device list, error %lu\n", GetLastError() );

    ret = SetupDiCreateDeviceInfoW( set, L"root\\winetest\\0", &GUID_NULL, NULL, NULL, 0, &device );
    ok( ret, "failed to create device, error %#lx\n", GetLastError() );

    ret = SetupDiSetDeviceRegistryPropertyW( set, &device, SPDRP_HARDWAREID, (const BYTE *)bus_hardware_ids, sizeof(bus_hardware_ids) );
    ok( ret, "failed to create set hardware ID, error %lu\n", GetLastError() );

    ret = SetupDiCallClassInstaller( DIF_REGISTERDEVICE, set, &device );
    ok( ret, "failed to register device, error %lu\n", GetLastError() );

    ret = SetupDiDestroyDeviceInfoList( set );
    ok( ret, "failed to destroy set, error %lu\n", GetLastError() );

    GetFullPathNameW( L"winetest.inf", ARRAY_SIZE(path), path, NULL );

    ret = UpdateDriverForPlugAndPlayDevicesW( NULL, bus_hardware_ids, path, INSTALLFLAG_FORCE, &need_reboot );
    ok( ret, "failed to install device, error %lu\n", GetLastError() );
    ok( !need_reboot, "expected no reboot necessary\n" );

    testsign_cleanup( cert );

    /* Check that the service is created and started. */
    manager = OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT );
    ok( !!manager, "failed to open service manager, error %lu\n", GetLastError() );

    service = OpenServiceW( manager, service_name, SERVICE_START );
    ok( !!service, "failed to open service, error %lu\n", GetLastError() );

    ret = StartServiceW( service, 0, NULL );
    ok( !ret, "service didn't start automatically\n" );
    if (!ret && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
    {
        /* If Secure Boot is enabled or the machine is 64-bit, it will reject an unsigned driver. */
        ok( GetLastError() == ERROR_DRIVER_BLOCKED || GetLastError() == ERROR_INVALID_IMAGE_HASH,
            "unexpected error %lu\n", GetLastError() );
        win_skip( "Failed to start service; probably your machine doesn't accept unsigned drivers.\n" );
    }

    CloseServiceHandle( service );
    CloseServiceHandle( manager );

    winetest_mute_threshold = old_mute_threshold;
    return ret || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING;
}

void hid_device_stop( struct hid_device_desc *desc, UINT count )
{
    HANDLE control;
    DWORD ret, i;

    control = CreateFileW( L"\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}", 0, 0,
                           NULL, OPEN_EXISTING, 0, NULL );
    ok( control != INVALID_HANDLE_VALUE, "CreateFile failed, error %lu\n", GetLastError() );
    ret = sync_ioctl( control, IOCTL_WINETEST_REMOVE_DEVICE, desc, sizeof(*desc), NULL, 0, 5000 );
    ok( ret || GetLastError() == ERROR_FILE_NOT_FOUND, "IOCTL_WINETEST_REMOVE_DEVICE failed, last error %lu\n", GetLastError() );
    CloseHandle( control );

    if (!ret) return;

    ret = WaitForSingleObject( device_removed, 5000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );

    for (i = 0; i < count; ++i)
    {
        ret = WaitForSingleObject( device_removed, i > 0 ? 500 : 5000 );
        ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    }
}

BOOL hid_device_start_( struct hid_device_desc *desc, UINT count, DWORD timeout )
{
    HANDLE control;
    DWORD ret, i;

    control = CreateFileW( L"\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}", 0, 0,
                           NULL, OPEN_EXISTING, 0, NULL );
    ok( control != INVALID_HANDLE_VALUE, "CreateFile failed, error %lu\n", GetLastError() );
    ret = sync_ioctl( control, IOCTL_WINETEST_CREATE_DEVICE, desc, sizeof(*desc), NULL, 0, 5000 );
    ok( ret, "IOCTL_WINETEST_CREATE_DEVICE failed, last error %lu\n", GetLastError() );
    CloseHandle( control );

    ret = WaitForSingleObject( device_added, 5000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );

    if (ret) return FALSE;

    for (i = 0; i < count; ++i)
    {
        ret = WaitForSingleObject( device_added, timeout );
        ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    }

    return TRUE;
}

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
    check_member_( __FILE__, line, *caps, *exp, "%ld", UnitsExp );
    check_member_( __FILE__, line, *caps, *exp, "%ld", Units );
    check_member_( __FILE__, line, *caps, *exp, "%ld", LogicalMin );
    check_member_( __FILE__, line, *caps, *exp, "%ld", LogicalMax );
    check_member_( __FILE__, line, *caps, *exp, "%ld", PhysicalMin );
    check_member_( __FILE__, line, *caps, *exp, "%ld", PhysicalMax );

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

BOOL sync_ioctl_( const char *file, int line, HANDLE device, DWORD code, void *in_buf, DWORD in_len,
                  void *out_buf, DWORD *ret_len, DWORD timeout )
{
    DWORD res, out_len = ret_len ? *ret_len : 0;
    OVERLAPPED ovl = {0};
    BOOL ret;

    ovl.hEvent = CreateEventW( NULL, TRUE, FALSE, NULL );
    ret = DeviceIoControl( device, code, in_buf, in_len, out_buf, out_len, &out_len, &ovl );
    if (!ret && GetLastError() == ERROR_IO_PENDING)
    {
        res = WaitForSingleObject( ovl.hEvent, timeout );
        ok_(file, line)( !res, "WaitForSingleObject returned %#lx\n", res );
        ret = GetOverlappedResult( device, &ovl, &out_len, FALSE );
        ok_(file, line)( ret, "GetOverlappedResult returned %lu\n", GetLastError() );
        if (!ret)
        {
            CancelIoEx( device, &ovl );
            res = WaitForSingleObject( ovl.hEvent, timeout );
            ok_(file, line)( !res, "WaitForSingleObject returned %#lx\n", res );
        }
    }
    CloseHandle( ovl.hEvent );

    if (ret_len) *ret_len = out_len;
    return ret;
}

void fill_context_( const char *file, int line, char *buffer, SIZE_T size )
{
    const char *source_file;
    source_file = strrchr( file, '/' );
    if (!source_file) source_file = strrchr( file, '\\' );
    if (!source_file) source_file = file;
    else source_file++;
    snprintf( buffer, size, "%s:%d", source_file, line );
}

void set_hid_expect_( const char *file, int line, HANDLE device, struct hid_device_desc *desc,
                      struct hid_expect *expect, DWORD expect_size )
{
    char buffer[sizeof(*desc) + EXPECT_QUEUE_BUFFER_SIZE];
    SIZE_T size;
    BOOL ret;

    if (desc) memcpy( buffer, desc, sizeof(*desc) );
    else memset( buffer, 0, sizeof(*desc) );

    fill_context_( file, line, buffer + sizeof(*desc), ARRAY_SIZE(buffer) - sizeof(*desc) );
    size = sizeof(*desc) + strlen( buffer + sizeof(*desc) ) + 1;
    ret = sync_ioctl_( file, line, device, IOCTL_WINETEST_HID_SET_CONTEXT, buffer, size, NULL, 0, 5000 );
    ok_(file, line)( ret, "IOCTL_WINETEST_HID_SET_CONTEXT failed, last error %lu\n", GetLastError() );

    if (expect) memcpy( buffer + sizeof(*desc), expect, expect_size );
    else memset( buffer + sizeof(*desc), 0, expect_size );

    size = sizeof(*desc) + expect_size;
    ret = sync_ioctl_( file, line, device, IOCTL_WINETEST_HID_SET_EXPECT, buffer, size, NULL, 0, 5000 );
    ok_(file, line)( ret, "IOCTL_WINETEST_HID_SET_EXPECT failed, last error %lu\n", GetLastError() );
}

void wait_hid_expect_( const char *file, int line, HANDLE device, struct hid_device_desc *desc,
                       DWORD timeout, BOOL wait_pending, BOOL todo )
{
    struct wait_expect_params params = {.wait_pending = wait_pending};
    char buffer[sizeof(*desc) + sizeof(params)];
    SIZE_T size;

    if (desc) memcpy( buffer, desc, sizeof(*desc) );
    else memset( buffer, 0, sizeof(*desc) );

    memcpy( buffer + sizeof(*desc), &params, sizeof(params) );
    size = sizeof(*desc) + sizeof(params);

    todo_wine_if(todo) {
    BOOL ret = sync_ioctl_( file, line, device, IOCTL_WINETEST_HID_WAIT_EXPECT, buffer, size, NULL, 0, timeout );
    ok_(file, line)( ret, "IOCTL_WINETEST_HID_WAIT_EXPECT failed, last error %lu\n", GetLastError() );
    }

    set_hid_expect_( file, line, device, desc, NULL, 0 );
}

void send_hid_input_( const char *file, int line, HANDLE device, struct hid_device_desc *desc,
                      struct hid_expect *expect, DWORD expect_size )
{
    char buffer[sizeof(*desc) + EXPECT_QUEUE_BUFFER_SIZE];
    SIZE_T size;
    BOOL ret;

    if (desc) memcpy( buffer, desc, sizeof(*desc) );
    else memset( buffer, 0, sizeof(*desc) );

    fill_context_( file, line, buffer + sizeof(*desc), ARRAY_SIZE(buffer) - sizeof(*desc) );
    size = sizeof(*desc) + strlen( buffer + sizeof(*desc) ) + 1;
    ret = sync_ioctl_( file, line, device, IOCTL_WINETEST_HID_SET_CONTEXT, buffer, size, NULL, 0, 5000 );
    ok_(file, line)( ret, "IOCTL_WINETEST_HID_SET_CONTEXT failed, last error %lu\n", GetLastError() );

    if (expect) memcpy( buffer + sizeof(*desc), expect, expect_size );
    else memset( buffer + sizeof(*desc), 0, expect_size );

    size = sizeof(*desc) + expect_size;
    ret = sync_ioctl_( file, line, device, IOCTL_WINETEST_HID_SEND_INPUT, buffer, size, NULL, 0, 5000 );
    ok_(file, line)( ret, "IOCTL_WINETEST_HID_SEND_INPUT failed, last error %lu\n", GetLastError() );
}

void wait_hid_input_( const char *file, int line, HANDLE device, struct hid_device_desc *desc,
                      DWORD timeout, BOOL todo )
{
    char buffer[sizeof(*desc)];
    SIZE_T size;

    if (desc) memcpy( buffer, desc, sizeof(*desc) );
    else memset( buffer, 0, sizeof(*desc) );
    size = sizeof(*desc);

    todo_wine_if(todo) {
    BOOL ret = sync_ioctl_( file, line, device, IOCTL_WINETEST_HID_WAIT_INPUT, buffer, size, NULL, 0, timeout );
    ok_(file, line)( ret, "IOCTL_WINETEST_HID_WAIT_INPUT failed, last error %lu\n", GetLastError() );
    }

    set_hid_expect_( file, line, device, desc, NULL, 0 );
}

static void test_hidp_get_input( HANDLE file, int report_id, ULONG report_len, PHIDP_PREPARSED_DATA preparsed )
{
    struct hid_expect expect[] =
    {
        {
            .code = IOCTL_HID_GET_INPUT_REPORT,
            .report_id = report_id,
            .report_len = report_len - (report_id ? 0 : 1),
            .report_buf = {report_id ? report_id : 0xa5,0xa5,2},
            .ret_length = 3,
            .ret_status = STATUS_SUCCESS,
        },
        {
            .code = IOCTL_HID_GET_INPUT_REPORT,
            .report_id = report_id,
            .report_len = 2 * report_len - (report_id ? 0 : 1),
            .report_buf = {report_id ? report_id : 0xa5,0xa5,1},
            .ret_length = 3,
            .ret_status = STATUS_SUCCESS,
        },
    };

    char buffer[200], report[200];
    NTSTATUS status;
    ULONG length;
    BOOL ret;

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Input, report_id, preparsed, report, report_len );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#lx\n", status );

    SetLastError( 0xdeadbeef );
    ret = HidD_GetInputReport( file, report, 0 );
    ok( !ret, "HidD_GetInputReport succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER, "HidD_GetInputReport returned error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = HidD_GetInputReport( file, report, report_len - 1 );
    ok( !ret, "HidD_GetInputReport succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
        "HidD_GetInputReport returned error %lu\n", GetLastError() );

    if (!report_id)
    {
        struct hid_expect broken_expect =
        {
            .code = IOCTL_HID_GET_INPUT_REPORT,
            .broken_id = -1,
            .report_len = report_len - 1,
            .report_buf =
            {
                0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,
                0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,
                0x5a,0x5a,0x5a,0x5a,0x5a,
            },
            .ret_length = 3,
            .ret_status = STATUS_SUCCESS,
        };

        set_hid_expect( file, &broken_expect, sizeof(broken_expect) );
    }

    SetLastError( 0xdeadbeef );
    memset( buffer, 0x5a, sizeof(buffer) );
    ret = HidD_GetInputReport( file, buffer, report_len );
    if (report_id || broken( !ret ) /* w7u */)
    {
        ok( !ret, "HidD_GetInputReport succeeded, last error %lu\n", GetLastError() );
        ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
            "HidD_GetInputReport returned error %lu\n", GetLastError() );
    }
    else
    {
        ok( ret, "HidD_GetInputReport failed, last error %lu\n", GetLastError() );
        ok( buffer[0] == 0x5a, "got buffer[0] %x, expected 0x5a\n", (BYTE)buffer[0] );
    }

    set_hid_expect( file, expect, sizeof(expect) );

    SetLastError( 0xdeadbeef );
    ret = HidD_GetInputReport( file, report, report_len );
    ok( ret, "HidD_GetInputReport failed, last error %lu\n", GetLastError() );
    ok( report[0] == report_id, "got report[0] %02x, expected %02x\n", report[0], report_id );

    SetLastError( 0xdeadbeef );
    length = report_len * 2;
    ret = sync_ioctl( file, IOCTL_HID_GET_INPUT_REPORT, NULL, 0, report, &length, 5000 );
    ok( ret, "IOCTL_HID_GET_INPUT_REPORT failed, last error %lu\n", GetLastError() );
    ok( length == 3, "got length %lu, expected 3\n", length );
    ok( report[0] == report_id, "got report[0] %02x, expected %02x\n", report[0], report_id );

    set_hid_expect( file, NULL, 0 );
}

static void test_hidp_get_feature( HANDLE file, int report_id, ULONG report_len, PHIDP_PREPARSED_DATA preparsed )
{
    struct hid_expect expect[] =
    {
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = report_id,
            .report_len = report_len - (report_id ? 0 : 1),
            .report_buf = {report_id ? report_id : 0xa5,0xa5,0xa5},
            .ret_length = 3,
            .ret_status = STATUS_SUCCESS,
        },
        {
            .code = IOCTL_HID_GET_FEATURE,
            .report_id = report_id,
            .report_len = 2 * report_len - (report_id ? 0 : 1),
            .report_buf = {report_id ? report_id : 0xa5,0xa5,0xa5},
            .ret_length = 3,
            .ret_status = STATUS_SUCCESS,
        },
    };

    char buffer[200], report[200];
    NTSTATUS status;
    ULONG length;
    BOOL ret;

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Feature, report_id, preparsed, report, report_len );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#lx\n", status );

    SetLastError( 0xdeadbeef );
    ret = HidD_GetFeature( file, report, 0 );
    ok( !ret, "HidD_GetFeature succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER, "HidD_GetFeature returned error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = HidD_GetFeature( file, report, report_len - 1 );
    ok( !ret, "HidD_GetFeature succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
        "HidD_GetFeature returned error %lu\n", GetLastError() );

    if (!report_id)
    {
        struct hid_expect broken_expect =
        {
            .code = IOCTL_HID_GET_FEATURE,
            .broken_id = -1,
            .report_len = report_len - 1,
            .report_buf =
            {
                0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,
                0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,
                0x5a,0x5a,0x5a,0x5a,0x5a,
            },
            .ret_length = 3,
            .ret_status = STATUS_SUCCESS,
        };

        set_hid_expect( file, &broken_expect, sizeof(broken_expect) );
    }

    SetLastError( 0xdeadbeef );
    memset( buffer, 0x5a, sizeof(buffer) );
    ret = HidD_GetFeature( file, buffer, report_len );
    if (report_id || broken( !ret ))
    {
        ok( !ret, "HidD_GetFeature succeeded, last error %lu\n", GetLastError() );
        ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
            "HidD_GetFeature returned error %lu\n", GetLastError() );
    }
    else
    {
        ok( ret, "HidD_GetFeature failed, last error %lu\n", GetLastError() );
        ok( buffer[0] == 0x5a, "got buffer[0] %x, expected 0x5a\n", (BYTE)buffer[0] );
    }

    set_hid_expect( file, expect, sizeof(expect) );

    SetLastError( 0xdeadbeef );
    ret = HidD_GetFeature( file, report, report_len );
    ok( ret, "HidD_GetFeature failed, last error %lu\n", GetLastError() );
    ok( report[0] == report_id, "got report[0] %02x, expected %02x\n", report[0], report_id );

    length = report_len * 2;
    SetLastError( 0xdeadbeef );
    ret = sync_ioctl( file, IOCTL_HID_GET_FEATURE, NULL, 0, report, &length, 5000 );
    ok( ret, "IOCTL_HID_GET_FEATURE failed, last error %lu\n", GetLastError() );
    ok( length == 3, "got length %lu, expected 3\n", length );
    ok( report[0] == report_id, "got report[0] %02x, expected %02x\n", report[0], report_id );

    set_hid_expect( file, NULL, 0 );
}

static void test_hidp_set_feature( HANDLE file, int report_id, ULONG report_len, PHIDP_PREPARSED_DATA preparsed )
{
    struct hid_expect expect[] =
    {
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = report_id,
            .report_len = report_len - (report_id ? 0 : 1),
            .report_buf = {report_id},
            .ret_length = 3,
            .ret_status = STATUS_SUCCESS,
        },
        {
            .code = IOCTL_HID_SET_FEATURE,
            .report_id = report_id,
            .report_len = report_len - (report_id ? 0 : 1),
            .report_buf =
            {
                report_id,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0xcd,0xcd,0xcd,0xcd,0xcd,0xcd,0xcd,0xcd,0xcd,0xcd,0xcd,
            },
            .ret_length = 3,
            .ret_status = STATUS_SUCCESS,
        },
    };
    char buffer[200], report[200];
    NTSTATUS status;
    ULONG length;
    BOOL ret;

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Feature, report_id, preparsed, report, report_len );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#lx\n", status );

    SetLastError( 0xdeadbeef );
    ret = HidD_SetFeature( file, report, 0 );
    ok( !ret, "HidD_SetFeature succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER, "HidD_SetFeature returned error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = HidD_SetFeature( file, report, report_len - 1 );
    ok( !ret, "HidD_SetFeature succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
        "HidD_SetFeature returned error %lu\n", GetLastError() );

    if (!report_id)
    {
        struct hid_expect broken_expect =
        {
            .code = IOCTL_HID_SET_FEATURE,
            .broken_id = -1,
            .report_len = report_len - 1,
            .report_buf =
            {
                0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,
                0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,
                0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,
                0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,0x5a,
            },
            .ret_length = 3,
            .ret_status = STATUS_SUCCESS,
        };

        set_hid_expect( file, &broken_expect, sizeof(broken_expect) );
    }

    SetLastError( 0xdeadbeef );
    memset( buffer, 0x5a, sizeof(buffer) );
    ret = HidD_SetFeature( file, buffer, report_len );
    if (report_id || broken( !ret ))
    {
        ok( !ret, "HidD_SetFeature succeeded, last error %lu\n", GetLastError() );
        ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
            "HidD_SetFeature returned error %lu\n", GetLastError() );
    }
    else
    {
        ok( ret, "HidD_SetFeature failed, last error %lu\n", GetLastError() );
    }

    set_hid_expect( file, expect, sizeof(expect) );

    SetLastError( 0xdeadbeef );
    ret = HidD_SetFeature( file, report, report_len );
    ok( ret, "HidD_SetFeature failed, last error %lu\n", GetLastError() );

    length = report_len * 2;
    SetLastError( 0xdeadbeef );
    ret = sync_ioctl( file, IOCTL_HID_SET_FEATURE, NULL, 0, report, &length, 5000 );
    ok( !ret, "IOCTL_HID_SET_FEATURE succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER, "IOCTL_HID_SET_FEATURE returned error %lu\n",
        GetLastError() );
    length = 0;
    SetLastError( 0xdeadbeef );
    ret = sync_ioctl( file, IOCTL_HID_SET_FEATURE, report, report_len * 2, NULL, &length, 5000 );
    ok( ret, "IOCTL_HID_SET_FEATURE failed, last error %lu\n", GetLastError() );
    ok( length == 3, "got length %lu, expected 3\n", length );

    set_hid_expect( file, NULL, 0 );
}

static void test_hidp_set_output( HANDLE file, int report_id, ULONG report_len, PHIDP_PREPARSED_DATA preparsed )
{
    struct hid_expect expect[] =
    {
        {
            .code = IOCTL_HID_SET_OUTPUT_REPORT,
            .report_id = report_id,
            .report_len = report_len - (report_id ? 0 : 1),
            .report_buf = {report_id},
            .ret_length = 3,
            .ret_status = STATUS_SUCCESS,
        },
        {
            .code = IOCTL_HID_SET_OUTPUT_REPORT,
            .report_id = report_id,
            .report_len = report_len - (report_id ? 0 : 1),
            .report_buf = {report_id,0,0xcd,0xcd,0xcd},
            .ret_length = 3,
            .ret_status = STATUS_SUCCESS,
        },
    };

    char buffer[200], report[200];
    NTSTATUS status;
    ULONG length;
    BOOL ret;

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Output, report_id, preparsed, report, report_len );
    ok( status == HIDP_STATUS_REPORT_DOES_NOT_EXIST, "HidP_InitializeReportForID returned %#lx\n", status );
    memset( report, 0, report_len );
    report[0] = report_id;

    SetLastError( 0xdeadbeef );
    ret = HidD_SetOutputReport( file, report, 0 );
    ok( !ret, "HidD_SetOutputReport succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER, "HidD_SetOutputReport returned error %lu\n",
        GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = HidD_SetOutputReport( file, report, report_len - 1 );
    ok( !ret, "HidD_SetOutputReport succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
        "HidD_SetOutputReport returned error %lu\n", GetLastError() );

    if (!report_id)
    {
        struct hid_expect broken_expect =
        {
            .code = IOCTL_HID_SET_OUTPUT_REPORT,
            .broken_id = -1,
            .report_len = report_len - 1,
            .report_buf = {0x5a,0x5a},
            .ret_length = 3,
            .ret_status = STATUS_SUCCESS,
        };

        set_hid_expect( file, &broken_expect, sizeof(broken_expect) );
    }

    SetLastError( 0xdeadbeef );
    memset( buffer, 0x5a, sizeof(buffer) );
    ret = HidD_SetOutputReport( file, buffer, report_len );
    if (report_id || broken( !ret ))
    {
        ok( !ret, "HidD_SetOutputReport succeeded, last error %lu\n", GetLastError() );
        ok( GetLastError() == ERROR_INVALID_PARAMETER || broken( GetLastError() == ERROR_CRC ),
            "HidD_SetOutputReport returned error %lu\n", GetLastError() );
    }
    else
    {
        ok( ret, "HidD_SetOutputReport failed, last error %lu\n", GetLastError() );
    }

    set_hid_expect( file, expect, sizeof(expect) );

    SetLastError( 0xdeadbeef );
    ret = HidD_SetOutputReport( file, report, report_len );
    ok( ret, "HidD_SetOutputReport failed, last error %lu\n", GetLastError() );

    length = report_len * 2;
    SetLastError( 0xdeadbeef );
    ret = sync_ioctl( file, IOCTL_HID_SET_OUTPUT_REPORT, NULL, 0, report, &length, 5000 );
    ok( !ret, "IOCTL_HID_SET_OUTPUT_REPORT succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER,
        "IOCTL_HID_SET_OUTPUT_REPORT returned error %lu\n", GetLastError() );
    length = 0;
    SetLastError( 0xdeadbeef );
    ret = sync_ioctl( file, IOCTL_HID_SET_OUTPUT_REPORT, report, report_len * 2, NULL, &length, 5000 );
    ok( ret, "IOCTL_HID_SET_OUTPUT_REPORT failed, last error %lu\n", GetLastError() );
    ok( length == 3, "got length %lu, expected 3\n", length );

    set_hid_expect( file, NULL, 0 );
}

static void test_write_file( HANDLE file, int report_id, ULONG report_len )
{
    struct hid_expect expect =
    {
        .code = IOCTL_HID_WRITE_REPORT,
        .report_id = report_id,
        .report_len = report_len - (report_id ? 0 : 1),
        .report_buf = {report_id ? report_id : 0xcd,0xcd,0xcd,0xcd,0xcd},
        .ret_length = 3,
        .ret_status = STATUS_SUCCESS,
    };

    char report[200];
    ULONG length;
    BOOL ret;

    SetLastError( 0xdeadbeef );
    ret = WriteFile( file, report, 0, &length, NULL );
    ok( !ret, "WriteFile succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_USER_BUFFER, "WriteFile returned error %lu\n", GetLastError() );
    ok( length == 0, "WriteFile returned %#lx\n", length );
    SetLastError( 0xdeadbeef );
    ret = WriteFile( file, report, report_len - 1, &length, NULL );
    ok( !ret, "WriteFile succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_INVALID_USER_BUFFER,
        "WriteFile returned error %lu\n", GetLastError() );
    ok( length == 0, "WriteFile returned %#lx\n", length );

    set_hid_expect( file, &expect, sizeof(expect) );

    memset( report, 0xcd, sizeof(report) );
    report[0] = 0xa5;
    SetLastError( 0xdeadbeef );
    ret = WriteFile( file, report, report_len * 2, &length, NULL );
    if (report_id || broken( !ret ) /* w7u */)
    {
        ok( !ret, "WriteFile succeeded\n" );
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "WriteFile returned error %lu\n", GetLastError() );
        ok( length == 0, "WriteFile wrote %lu\n", length );
        SetLastError( 0xdeadbeef );
        report[0] = report_id;
        ret = WriteFile( file, report, report_len, &length, NULL );
    }

    if (report_id)
    {
        ok( ret, "WriteFile failed, last error %lu\n", GetLastError() );
        ok( length == 2, "WriteFile wrote %lu\n", length );
    }
    else
    {
        ok( ret, "WriteFile failed, last error %lu\n", GetLastError() );
        ok( length == 3, "WriteFile wrote %lu\n", length );
    }

    set_hid_expect( file, NULL, 0 );
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
            .BitSize = 24,
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
            .NumberOfChildren = 8,
            .FirstChild = 10,
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

    DWORD waveform_list, collection_count, generic_output_list;
    OVERLAPPED overlapped = {0}, overlapped2 = {0};
    HIDP_LINK_COLLECTION_NODE collections[16];
    PHIDP_PREPARSED_DATA preparsed_data;
    USAGE_AND_PAGE usage_and_pages[16];
    HIDP_BUTTON_CAPS button_caps[32];
    HIDP_VALUE_CAPS value_caps[16];
    char buffer[200], report[200];
    HIDP_DATA data[64];
    USAGE usages[16];
    ULONG off, value;
    NTSTATUS status;
    HIDP_CAPS caps;
    unsigned int i;
    USHORT count;
    BOOL ret;

    SetLastError( 0xdeadbeef );
    ret = HidD_GetPhysicalDescriptor( file, buffer, sizeof(buffer) );
    ok( !ret, "HidD_GetPhysicalDescriptor succeeded\n" );
    ok( GetLastError() == ERROR_NOT_SUPPORTED, "got error %lu\n", GetLastError() );

    ret = HidD_GetPreparsedData( file, &preparsed_data );
    ok( ret, "HidD_GetPreparsedData failed with error %lu\n", GetLastError() );

    memset( buffer, 0, sizeof(buffer) );
    status = HidP_GetCaps( (PHIDP_PREPARSED_DATA)buffer, &caps );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetCaps returned %#lx\n", status );
    status = HidP_GetCaps( preparsed_data, &caps );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetCaps returned %#lx\n", status );
    check_hidp_caps( &caps, expect_caps );

    collection_count = 0;
    status = HidP_GetLinkCollectionNodes( collections, &collection_count, preparsed_data );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetLinkCollectionNodes returned %#lx\n", status );
    ok( collection_count == caps.NumberLinkCollectionNodes,
        "got %ld collection nodes, expected %d\n", collection_count, caps.NumberLinkCollectionNodes );
    collection_count = ARRAY_SIZE(collections);
    status = HidP_GetLinkCollectionNodes( collections, &collection_count, (PHIDP_PREPARSED_DATA)buffer );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetLinkCollectionNodes returned %#lx\n", status );
    status = HidP_GetLinkCollectionNodes( collections, &collection_count, preparsed_data );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetLinkCollectionNodes returned %#lx\n", status );
    ok( collection_count == caps.NumberLinkCollectionNodes,
        "got %ld collection nodes, expected %d\n", collection_count, caps.NumberLinkCollectionNodes );

    for (i = 0; i < ARRAY_SIZE(expect_collections); ++i)
    {
        winetest_push_context( "collections[%d]", i );
        check_hidp_link_collection_node( &collections[i], &expect_collections[i] );
        winetest_pop_context();
    }

    count = ARRAY_SIZE(button_caps);
    status = HidP_GetButtonCaps( HidP_Output, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetButtonCaps returned %#lx\n", status );
    status = HidP_GetButtonCaps( HidP_Feature + 1, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetButtonCaps returned %#lx\n", status );
    count = 0;
    status = HidP_GetButtonCaps( HidP_Input, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetButtonCaps returned %#lx\n", status );
    ok( count == caps.NumberInputButtonCaps, "HidP_GetButtonCaps returned count %d, expected %d\n",
        count, caps.NumberInputButtonCaps );
    count = ARRAY_SIZE(button_caps);
    status = HidP_GetButtonCaps( HidP_Input, button_caps, &count, (PHIDP_PREPARSED_DATA)buffer );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetButtonCaps returned %#lx\n", status );
    memset( button_caps, 0, sizeof(button_caps) );
    status = HidP_GetButtonCaps( HidP_Input, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetButtonCaps returned %#lx\n", status );
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
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificButtonCaps returned %#lx\n", status );
    status = HidP_GetSpecificButtonCaps( HidP_Feature + 1, 0, 0, 0, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetSpecificButtonCaps returned %#lx\n", status );
    count = 0;
    status = HidP_GetSpecificButtonCaps( HidP_Input, 0, 0, 0, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetSpecificButtonCaps returned %#lx\n", status );
    ok( count == caps.NumberInputButtonCaps, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n",
        count, caps.NumberInputButtonCaps );
    count = ARRAY_SIZE(button_caps) - 1;
    status = HidP_GetSpecificButtonCaps( HidP_Input, 0, 0, 0, button_caps, &count, (PHIDP_PREPARSED_DATA)buffer );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetSpecificButtonCaps returned %#lx\n", status );

    status = HidP_GetSpecificButtonCaps( HidP_Input, 0, 0, 0, button_caps + 1, &count, preparsed_data );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetSpecificButtonCaps returned %#lx\n", status );
    ok( count == caps.NumberInputButtonCaps, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n",
        count, caps.NumberInputButtonCaps );
    check_hidp_button_caps( &button_caps[1], &button_caps[0] );

    status = HidP_GetSpecificButtonCaps( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, 5, button_caps + 1,
                                         &count, preparsed_data );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetSpecificButtonCaps returned %#lx\n", status );
    ok( count == 1, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n", count, 1 );
    check_hidp_button_caps( &button_caps[1], &button_caps[0] );

    count = 0xbeef;
    status = HidP_GetSpecificButtonCaps( HidP_Input, 0xfffe, 0, 0, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificButtonCaps returned %#lx\n", status );
    ok( count == 0, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n", count, 0 );
    count = 0xbeef;
    status = HidP_GetSpecificButtonCaps( HidP_Input, 0, 0xfffe, 0, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificButtonCaps returned %#lx\n", status );
    ok( count == 0, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n", count, 0 );
    count = 0xbeef;
    status = HidP_GetSpecificButtonCaps( HidP_Input, 0, 0, 0xfffe, button_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificButtonCaps returned %#lx\n", status );
    ok( count == 0, "HidP_GetSpecificButtonCaps returned count %d, expected %d\n", count, 0 );

    count = ARRAY_SIZE(value_caps);
    status = HidP_GetValueCaps( HidP_Output, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetValueCaps returned %#lx\n", status );
    status = HidP_GetValueCaps( HidP_Feature + 1, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetValueCaps returned %#lx\n", status );
    count = 0;
    status = HidP_GetValueCaps( HidP_Input, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetValueCaps returned %#lx\n", status );
    ok( count == caps.NumberInputValueCaps, "HidP_GetValueCaps returned count %d, expected %d\n",
        count, caps.NumberInputValueCaps );
    count = ARRAY_SIZE(value_caps);
    status = HidP_GetValueCaps( HidP_Input, value_caps, &count, (PHIDP_PREPARSED_DATA)buffer );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetValueCaps returned %#lx\n", status );
    status = HidP_GetValueCaps( HidP_Input, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetValueCaps returned %#lx\n", status );
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
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificValueCaps returned %#lx\n", status );
    status = HidP_GetSpecificValueCaps( HidP_Feature + 1, 0, 0, 0, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetSpecificValueCaps returned %#lx\n", status );
    count = 0;
    status = HidP_GetSpecificValueCaps( HidP_Input, 0, 0, 0, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetSpecificValueCaps returned %#lx\n", status );
    ok( count == caps.NumberInputValueCaps, "HidP_GetSpecificValueCaps returned count %d, expected %d\n",
        count, caps.NumberInputValueCaps );
    count = ARRAY_SIZE(value_caps) - 4;
    status = HidP_GetSpecificValueCaps( HidP_Input, 0, 0, 0, value_caps + 4, &count, (PHIDP_PREPARSED_DATA)buffer );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetSpecificValueCaps returned %#lx\n", status );

    status = HidP_GetSpecificValueCaps( HidP_Input, 0, 0, 0, value_caps + 4, &count, preparsed_data );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetSpecificValueCaps returned %#lx\n", status );
    ok( count == caps.NumberInputValueCaps, "HidP_GetSpecificValueCaps returned count %d, expected %d\n",
        count, caps.NumberInputValueCaps );
    check_hidp_value_caps( &value_caps[4], &value_caps[0] );
    check_hidp_value_caps( &value_caps[5], &value_caps[1] );
    check_hidp_value_caps( &value_caps[6], &value_caps[2] );
    check_hidp_value_caps( &value_caps[7], &value_caps[3] );

    count = 1;
    status = HidP_GetSpecificValueCaps( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH,
                                        value_caps + 4, &count, preparsed_data );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetSpecificValueCaps returned %#lx\n", status );
    ok( count == 1, "HidP_GetSpecificValueCaps returned count %d, expected %d\n", count, 1 );
    check_hidp_value_caps( &value_caps[4], &value_caps[3] );

    count = 0xdead;
    status = HidP_GetSpecificValueCaps( HidP_Input, 0xfffe, 0, 0, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificValueCaps returned %#lx\n", status );
    ok( count == 0, "HidP_GetSpecificValueCaps returned count %d, expected %d\n", count, 0 );
    count = 0xdead;
    status = HidP_GetSpecificValueCaps( HidP_Input, 0, 0xfffe, 0, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificValueCaps returned %#lx\n", status );
    ok( count == 0, "HidP_GetSpecificValueCaps returned count %d, expected %d\n", count, 0 );
    count = 0xdead;
    status = HidP_GetSpecificValueCaps( HidP_Input, 0, 0, 0xfffe, value_caps, &count, preparsed_data );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetSpecificValueCaps returned %#lx\n", status );
    ok( count == 0, "HidP_GetSpecificValueCaps returned count %d, expected %d\n", count, 0 );

    status = HidP_InitializeReportForID( HidP_Input, 0, (PHIDP_PREPARSED_DATA)buffer, report, sizeof(report) );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_InitializeReportForID returned %#lx\n", status );
    status = HidP_InitializeReportForID( HidP_Feature + 1, 0, preparsed_data, report, sizeof(report) );
    ok( status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_InitializeReportForID returned %#lx\n", status );
    status = HidP_InitializeReportForID( HidP_Input, 0, preparsed_data, report, sizeof(report) );
    ok( status == HIDP_STATUS_INVALID_REPORT_LENGTH, "HidP_InitializeReportForID returned %#lx\n", status );
    status = HidP_InitializeReportForID( HidP_Input, 0, preparsed_data, report, caps.InputReportByteLength + 1 );
    ok( status == HIDP_STATUS_INVALID_REPORT_LENGTH, "HidP_InitializeReportForID returned %#lx\n", status );
    status = HidP_InitializeReportForID( HidP_Input, 1 - report_id, preparsed_data, report,
                                         caps.InputReportByteLength );
    ok( status == HIDP_STATUS_REPORT_DOES_NOT_EXIST, "HidP_InitializeReportForID returned %#lx\n", status );

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Input, report_id, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#lx\n", status );

    memset( buffer, 0xcd, sizeof(buffer) );
    memset( buffer, 0, caps.InputReportByteLength );
    buffer[0] = report_id;
    ok( !memcmp( buffer, report, sizeof(buffer) ), "unexpected report data\n" );

    status = HidP_SetUsageValueArray( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, buffer,
                                      sizeof(buffer), preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_NOT_VALUE_ARRAY, "HidP_SetUsageValueArray returned %#lx\n", status );
    memset( buffer, 0xa5, sizeof(buffer) );
    status = HidP_SetUsageValueArray( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH,
                                      buffer, 0, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_SetUsageValueArray returned %#lx\n", status );
    status = HidP_SetUsageValueArray( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH,
                                      buffer, 8, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValueArray returned %#lx\n", status );
    ok( report[16] == (char)0xa5, "got report value %d\n", report[16] );
    ok( report[17] == (char)0xa5, "got report value %d\n", report[17] );

    report[16] = 0xa5;
    report[17] = 0xa5;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH,
                                 8, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );
    ok( report[16] == (char)8, "got value %#x\n", report[16] );
    ok( report[17] == (char)0, "got value %#x\n", report[17] );

    status = HidP_GetUsageValueArray( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, buffer,
                                      sizeof(buffer), preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_NOT_VALUE_ARRAY, "HidP_GetUsageValueArray returned %#lx\n", status );
    memset( buffer, 0xa5, sizeof(buffer) );
    status = HidP_GetUsageValueArray( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH,
                                      buffer, 0, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetUsageValueArray returned %#lx\n", status );
    report[16] = 0xcd;
    report[17] = 0xcd;
    status = HidP_GetUsageValueArray( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH,
                                      buffer, 8, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValueArray returned %#lx\n", status );
    ok( buffer[0] == (char)0xcd, "got report value %#x\n", buffer[0] );
    ok( buffer[1] == (char)0xcd, "got report value %#x\n", buffer[1] );

    status = HidP_GetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, 0, &value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetUsageValue returned %#lx\n", status );
    status = HidP_GetUsageValue( HidP_Input, 0, 0, HID_USAGE_GENERIC_X, &value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetUsageValue returned %#lx\n", status );

    report[16] = 0xff;
    report[17] = 0xff;
    status = HidP_GetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_HATSWITCH,
                                 &value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#lx\n", status );
    ok( value == 0xffff, "got value %ld\n", value );

    value = -128;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );
    value = 0xdeadbeef;
    status = HidP_GetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, &value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#lx\n", status );
    ok( value == 0x80, "got value %#lx, expected %#x\n", value, 0x80 );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#lx\n", status );
    ok( value == -128, "got value %#lx, expected %#x\n", value, -128 );

    value = 127;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#lx\n", status );
    ok( value == 127, "got value %#lx, expected %#x\n", value, 127 );

    value = 0;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_X,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#lx\n", status );
    ok( value == 0, "got value %#lx, expected %#x\n", value, 0 );

    value = 0x7fffffff;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_VALUE_OUT_OF_RANGE, "HidP_GetScaledUsageValue returned %#lx\n", status );
    ok( value == 0, "got value %#lx, expected %#x\n", value, 0 );
    value = 0xdeadbeef;
    status = HidP_GetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z, &value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#lx\n", status );
    ok( value == 0x7fffffff, "got value %#lx, expected %#x\n", value, 0x7fffffff );

    value = 0x3fffffff;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#lx\n", status );
    ok( value == 0x7fffffff, "got value %#lx, expected %#x\n", value, 0x7fffffff );

    value = 0;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#lx\n", status );
    ok( value == 0x80000000, "got value %#lx, expected %#x\n", value, 0x80000000 );

    value = 0;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RX, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RX,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#lx\n", status );
    ok( value == 0, "got value %#lx, expected %#x\n", value, 0 );

    value = 0xfeedcafe;
    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RY, value,
                                 preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RY,
                                       (LONG *)&value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BAD_LOG_PHY_VALUES, "HidP_GetScaledUsageValue returned %#lx\n", status );
    ok( value == 0, "got value %#lx, expected %#x\n", value, 0 );
    status = HidP_SetScaledUsageValue( HidP_Input, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_RY,
                                       0, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BAD_LOG_PHY_VALUES, "HidP_GetScaledUsageValue returned %#lx\n", status );

    value = HidP_MaxUsageListLength( HidP_Feature + 1, 0, preparsed_data );
    ok( value == 0, "HidP_MaxUsageListLength(HidP_Feature + 1, 0) returned %ld, expected %d\n", value, 0 );
    value = HidP_MaxUsageListLength( HidP_Input, 0, preparsed_data );
    ok( value == 50, "HidP_MaxUsageListLength(HidP_Input, 0) returned %ld, expected %d\n", value, 50 );
    value = HidP_MaxUsageListLength( HidP_Input, HID_USAGE_PAGE_BUTTON, preparsed_data );
    ok( value == 32, "HidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_BUTTON) returned %ld, expected %d\n",
        value, 32 );
    value = HidP_MaxUsageListLength( HidP_Input, HID_USAGE_PAGE_LED, preparsed_data );
    ok( value == 8, "HidP_MaxUsageListLength(HidP_Input, HID_USAGE_PAGE_LED) returned %ld, expected %d\n",
        value, 8 );
    value = HidP_MaxUsageListLength( HidP_Feature, HID_USAGE_PAGE_BUTTON, preparsed_data );
    ok( value == 8, "HidP_MaxUsageListLength(HidP_Feature, HID_USAGE_PAGE_BUTTON) returned %ld, expected %d\n",
        value, 8 );
    value = HidP_MaxUsageListLength( HidP_Feature, HID_USAGE_PAGE_LED, preparsed_data );
    ok( value == 0, "HidP_MaxUsageListLength(HidP_Feature, HID_USAGE_PAGE_LED) returned %ld, expected %d\n",
        value, 0 );

    usages[0] = 0xff;
    value = 1;
    status = HidP_SetUsages( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsages returned %#lx\n", status );
    usages[1] = 2;
    usages[2] = 0xff;
    value = 3;
    status = HidP_SetUsages( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsages returned %#lx\n", status );
    usages[0] = 4;
    usages[1] = 6;
    value = 2;
    status = HidP_SetUsages( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsages returned %#lx\n", status );
    usages[0] = 4;
    usages[1] = 6;
    value = 2;
    status = HidP_SetUsages( HidP_Input, HID_USAGE_PAGE_LED, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsages returned %#lx\n", status );

    value = ARRAY_SIZE(usages);
    status = HidP_GetUsages( HidP_Input, HID_USAGE_PAGE_KEYBOARD, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsages returned %#lx\n", status );
    ok( value == 0, "got usage count %ld, expected %d\n", value, 2 );

    usages[0] = 0x9;
    usages[1] = 0xb;
    usages[2] = 0xa;
    value = 3;
    ok( report[6] == 0, "got report[6] %x expected 0\n", report[6] );
    ok( report[7] == 0, "got report[7] %x expected 0\n", report[7] );
    memcpy( buffer, report, caps.InputReportByteLength );
    status = HidP_SetUsages( HidP_Input, HID_USAGE_PAGE_KEYBOARD, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_SetUsages returned %#lx\n", status );
    buffer[13] = 2;
    buffer[14] = 4;
    ok( !memcmp( buffer, report, caps.InputReportByteLength ), "unexpected report data\n" );

    status = HidP_SetUsageValue( HidP_Input, HID_USAGE_PAGE_LED, 0, 6, 1, preparsed_data, report,
                                 caps.InputReportByteLength );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsageValue returned %#lx\n", status );

    value = 0xdeadbeef;
    status = HidP_GetUsageValue( HidP_Input, HID_USAGE_PAGE_LED, 0, 6, &value, preparsed_data,
                                 report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsageValue returned %#lx\n", status );
    ok( value == 0xdeadbeef, "got value %#lx, expected %#x\n", value, 0xdeadbeef );

    value = 1;
    status = HidP_GetUsages( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetUsages returned %#lx\n", status );
    ok( value == 2, "got usage count %ld, expected %d\n", value, 2 );
    value = ARRAY_SIZE(usages);
    memset( usages, 0xcd, sizeof(usages) );
    status = HidP_GetUsages( HidP_Input, HID_USAGE_PAGE_BUTTON, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsages returned %#lx\n", status );
    ok( value == 2, "got usage count %ld, expected %d\n", value, 2 );
    ok( usages[0] == 4, "got usages[0] %x, expected %x\n", usages[0], 4 );
    ok( usages[1] == 6, "got usages[1] %x, expected %x\n", usages[1], 6 );

    value = ARRAY_SIZE(usages);
    memset( usages, 0xcd, sizeof(usages) );
    status = HidP_GetUsages( HidP_Input, HID_USAGE_PAGE_LED, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsages returned %#lx\n", status );
    ok( value == 2, "got usage count %ld, expected %d\n", value, 2 );
    ok( usages[0] == 6, "got usages[0] %x, expected %x\n", usages[0], 6 );
    ok( usages[1] == 4, "got usages[1] %x, expected %x\n", usages[1], 4 );

    value = ARRAY_SIZE(usage_and_pages);
    memset( usage_and_pages, 0xcd, sizeof(usage_and_pages) );
    status = HidP_GetUsagesEx( HidP_Input, 0, usage_and_pages, &value, preparsed_data, report,
                               caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsagesEx returned %#lx\n", status );
    ok( value == 6, "got usage count %ld, expected %d\n", value, 4 );
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
    ok( value == 0, "HidP_MaxDataListLength(HidP_Feature + 1) returned %ld, expected %d\n", value, 0 );
    value = HidP_MaxDataListLength( HidP_Input, preparsed_data );
    ok( value == 58, "HidP_MaxDataListLength(HidP_Input) returned %ld, expected %d\n", value, 58 );
    value = HidP_MaxDataListLength( HidP_Output, preparsed_data );
    ok( value == 0, "HidP_MaxDataListLength(HidP_Output) returned %ld, expected %d\n", value, 0 );
    value = HidP_MaxDataListLength( HidP_Feature, preparsed_data );
    ok( value == 24, "HidP_MaxDataListLength(HidP_Feature) returned %ld, expected %d\n", value, 24 );

    value = 1;
    status = HidP_GetData( HidP_Input, data, &value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetData returned %#lx\n", status );
    ok( value == 11, "got data count %ld, expected %d\n", value, 11 );
    memset( data, 0, sizeof(data) );
    status = HidP_GetData( HidP_Input, data, &value, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetData returned %#lx\n", status );
    for (i = 0; i < ARRAY_SIZE(expect_data); ++i)
    {
        winetest_push_context( "data[%d]", i );
        check_member( data[i], expect_data[i], "%d", DataIndex );
        check_member( data[i], expect_data[i], "%ld", RawValue );
        winetest_pop_context();
    }

    /* HID nary usage collections are set with 1-based usage index in their declaration order */

    memset( report, 0, caps.InputReportByteLength );
    status = HidP_InitializeReportForID( HidP_Input, report_id, preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#lx\n", status );
    value = 2;
    usages[0] = 0x8e;
    usages[1] = 0x8f;
    status = HidP_SetUsages( HidP_Input, HID_USAGE_PAGE_KEYBOARD, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsages returned %#lx\n", status );
    ok( report[caps.InputReportByteLength - 2] == 3, "unexpected usage index %d, expected 3\n",
        report[caps.InputReportByteLength - 2] );
    ok( report[caps.InputReportByteLength - 1] == 4, "unexpected usage index %d, expected 4\n",
        report[caps.InputReportByteLength - 1] );
    status = HidP_UnsetUsages( HidP_Input, HID_USAGE_PAGE_KEYBOARD, 0, usages, &value,
                               preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_UnsetUsages returned %#lx\n", status );
    ok( report[caps.InputReportByteLength - 2] == 0, "unexpected usage index %d, expected 0\n",
        report[caps.InputReportByteLength - 2] );
    ok( report[caps.InputReportByteLength - 1] == 0, "unexpected usage index %d, expected 0\n",
        report[caps.InputReportByteLength - 1] );
    status = HidP_UnsetUsages( HidP_Input, HID_USAGE_PAGE_KEYBOARD, 0, usages, &value,
                               preparsed_data, report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_BUTTON_NOT_PRESSED, "HidP_UnsetUsages returned %#lx\n", status );
    value = 1;
    usages[0] = 0x8c;
    status = HidP_SetUsages( HidP_Input, HID_USAGE_PAGE_KEYBOARD, 0, usages, &value, preparsed_data,
                             report, caps.InputReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsages returned %#lx\n", status );
    ok( report[caps.InputReportByteLength - 2] == 1, "unexpected usage index %d, expected 1\n",
        report[caps.InputReportByteLength - 2] );

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Feature, 3, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_REPORT_DOES_NOT_EXIST, "HidP_InitializeReportForID returned %#lx\n", status );

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Feature, report_id, preparsed_data, report,
                                         caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#lx\n", status );

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
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_SetUsageValue returned %#lx\n", status );
    status = HidP_SetUsageValue( HidP_Feature + 1, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                 HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                 caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_SetUsageValue returned %#lx\n", status );
    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                 HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                 caps.FeatureReportByteLength + 1 );
    ok( status == HIDP_STATUS_INVALID_REPORT_LENGTH, "HidP_SetUsageValue returned %#lx\n", status );
    report[0] = 1 - report_id;
    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                 HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                 caps.FeatureReportByteLength );
    ok( status == (report_id ? HIDP_STATUS_SUCCESS : HIDP_STATUS_INCOMPATIBLE_REPORT_ID),
        "HidP_SetUsageValue returned %#lx\n", status );
    report[0] = 2;
    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                 HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                 caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_INCOMPATIBLE_REPORT_ID, "HidP_SetUsageValue returned %#lx\n", status );
    report[0] = report_id;
    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, 0xdead, 3, HID_USAGE_HAPTICS_WAVEFORM_RUMBLE,
                                 preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_SetUsageValue returned %#lx\n", status );

    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3,
                                 HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, preparsed_data, report,
                                 caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );

    memset( buffer, 0xcd, sizeof(buffer) );
    memset( buffer, 0, caps.FeatureReportByteLength );
    buffer[0] = report_id;
    value = HID_USAGE_HAPTICS_WAVEFORM_RUMBLE;
    memcpy( buffer + 1, &value, 2 );
    ok( !memcmp( buffer, report, sizeof(buffer) ), "unexpected report data\n" );

    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                 (PHIDP_PREPARSED_DATA)buffer, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_INVALID_PREPARSED_DATA, "HidP_GetUsageValue returned %#lx\n", status );
    status = HidP_GetUsageValue( HidP_Feature + 1, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                 preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_INVALID_REPORT_TYPE, "HidP_GetUsageValue returned %#lx\n", status );
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                 preparsed_data, report, caps.FeatureReportByteLength + 1 );
    ok( status == HIDP_STATUS_INVALID_REPORT_LENGTH, "HidP_GetUsageValue returned %#lx\n", status );
    report[0] = 1 - report_id;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                 preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == (report_id ? HIDP_STATUS_SUCCESS : HIDP_STATUS_INCOMPATIBLE_REPORT_ID),
        "HidP_GetUsageValue returned %#lx\n", status );
    report[0] = 2;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                 preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_INCOMPATIBLE_REPORT_ID, "HidP_GetUsageValue returned %#lx\n", status );
    report[0] = report_id;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, 0xdead, 3, &value,
                                 preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_USAGE_NOT_FOUND, "HidP_GetUsageValue returned %#lx\n", status );

    value = 0xdeadbeef;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, waveform_list, 3, &value,
                                 preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#lx\n", status );
    ok( value == HID_USAGE_HAPTICS_WAVEFORM_RUMBLE, "got value %#lx, expected %#x\n", value,
        HID_USAGE_HAPTICS_WAVEFORM_RUMBLE );

    memset( buffer, 0xff, sizeof(buffer) );
    status = HidP_SetUsageValueArray( HidP_Feature, HID_USAGE_PAGE_HAPTICS, 0,
                                      HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME, buffer, 0,
                                      preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_SetUsageValueArray returned %#lx\n", status );
    status = HidP_SetUsageValueArray( HidP_Feature, HID_USAGE_PAGE_HAPTICS, 0,
                                      HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME, buffer, 64,
                                      preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValueArray returned %#lx\n", status );
    ok( !memcmp( report + 9, buffer, 8 ), "unexpected report data\n" );

    memset( buffer, 0, sizeof(buffer) );
    status = HidP_GetUsageValueArray( HidP_Feature, HID_USAGE_PAGE_HAPTICS, 0,
                                      HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME, buffer, 0,
                                      preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_BUFFER_TOO_SMALL, "HidP_GetUsageValueArray returned %#lx\n", status );
    status = HidP_GetUsageValueArray( HidP_Feature, HID_USAGE_PAGE_HAPTICS, 0,
                                      HID_USAGE_HAPTICS_WAVEFORM_CUTOFF_TIME, buffer, 64,
                                      preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValueArray returned %#lx\n", status );
    memset( buffer + 16, 0xff, 8 );
    ok( !memcmp( buffer, buffer + 16, 16 ), "unexpected report value\n" );

    value = 0x7fffffff;
    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       (LONG *)&value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_VALUE_OUT_OF_RANGE, "HidP_GetScaledUsageValue returned %#lx\n", status );
    ok( value == 0, "got value %#lx, expected %#x\n", value, 0 );
    value = 0xdeadbeef;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 &value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#lx\n", status );
    ok( value == 0x7fffffff, "got value %#lx, expected %#x\n", value, 0x7fffffff );

    value = 0x7fff;
    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       (LONG *)&value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#lx\n", status );
    ok( value == 0x0003ffff, "got value %#lx, expected %#x\n", value, 0x0003ffff );

    value = 0;
    status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );
    value = 0xdeadbeef;
    status = HidP_GetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       (LONG *)&value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#lx\n", status );
    ok( value == 0xfff90000, "got value %#lx, expected %#x\n", value, 0xfff90000 );
    status = HidP_SetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       0x1000, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetScaledUsageValue returned %#lx\n", status );
    value = 0;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 &value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#lx\n", status );
    ok( value == 0xfffff518, "got value %#lx, expected %#x\n", value, 0xfffff518 );
    status = HidP_SetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       0, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetScaledUsageValue returned %#lx\n", status );
    value = 0;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 &value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#lx\n", status );
    ok( value == 0xfffff45e, "got value %#lx, expected %#x\n", value, 0xfffff45e );
    status = HidP_SetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       0xdead, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetScaledUsageValue returned %#lx\n", status );
    value = 0;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 &value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#lx\n", status );
    ok( value == 0xfffffe7d, "got value %#lx, expected %#x\n", value, 0xfffffe7d );
    status = HidP_SetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                       0xbeef, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_SetScaledUsageValue returned %#lx\n", status );
    value = 0;
    status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_GENERIC, 0, HID_USAGE_GENERIC_Z,
                                 &value, preparsed_data, report, caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_GetUsageValue returned %#lx\n", status );
    ok( value == 0xfffffd0b, "got value %#lx, expected %#x\n", value, 0xfffffd0b );

    memset( report, 0xcd, sizeof(report) );
    status = HidP_InitializeReportForID( HidP_Feature, report_id, preparsed_data, report,
                                         caps.FeatureReportByteLength );
    ok( status == HIDP_STATUS_SUCCESS, "HidP_InitializeReportForID returned %#lx\n", status );

    memset( buffer, 0xcd, sizeof(buffer) );
    memset( buffer, 0, caps.FeatureReportByteLength );
    buffer[0] = report_id;
    ok( !memcmp( buffer, report, sizeof(buffer) ), "unexpected report data\n" );

    for (i = 0; i < caps.NumberLinkCollectionNodes; ++i)
    {
        if (collections[i].LinkUsagePage != HID_USAGE_PAGE_GENERIC) continue;
        if (collections[i].LinkUsage == HID_USAGE_GENERIC_SYSTEM_CTL) break;
    }
    ok( i < caps.NumberLinkCollectionNodes, "HID_USAGE_GENERIC_SYSTEM_CTL collection not found\n" );
    generic_output_list = i;

    for (i = 0; i < 5; ++i)
    {
        status = HidP_SetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, generic_output_list, i + 1,
                                     i, preparsed_data, report, caps.FeatureReportByteLength );
        ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );

        status = HidP_GetUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, generic_output_list, i + 1,
                                     &value, preparsed_data, report, caps.FeatureReportByteLength );
        ok( status == HIDP_STATUS_SUCCESS, "HidP_SetUsageValue returned %#lx\n", status );
        ok( value == i, "got value %#lx, expected %#x\n", value, i );

        status = HidP_SetUsageValueArray( HidP_Feature, HID_USAGE_PAGE_ORDINAL, generic_output_list, i + 1, (void *)&value,
                                          sizeof(value), preparsed_data, report, caps.FeatureReportByteLength );
        ok( status == HIDP_STATUS_NOT_VALUE_ARRAY, "HidP_SetUsageValue returned %#lx\n", status );
        status = HidP_GetUsageValueArray( HidP_Feature, HID_USAGE_PAGE_ORDINAL, generic_output_list, i + 1, (void *)&value,
                                          sizeof(value), preparsed_data, report, caps.FeatureReportByteLength );
        ok( status == HIDP_STATUS_NOT_VALUE_ARRAY, "HidP_SetUsageValue returned %#lx\n", status );

        buffer[i + 21] = i;
    }
    for (i = 5; i < 10; ++i)
    {
        status = HidP_SetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, generic_output_list, i + 1,
                                           i * 16, preparsed_data, report, caps.FeatureReportByteLength );
        ok( status == HIDP_STATUS_SUCCESS, "HidP_SetScaledUsageValue returned %#lx\n", status );

        status = HidP_GetScaledUsageValue( HidP_Feature, HID_USAGE_PAGE_ORDINAL, generic_output_list, i + 1,
                                           (LONG *)&value, preparsed_data, report, caps.FeatureReportByteLength );
        ok( status == HIDP_STATUS_SUCCESS, "HidP_GetScaledUsageValue returned %#lx\n", status );
        ok( value == i * 16, "got value %#lx, expected %#x\n", value, i * 16 );

        buffer[i + 21] = i;
    }
    ok( !memcmp( buffer, report, sizeof(buffer) ), "unexpected report data\n" );

    test_hidp_get_input( file, report_id, caps.InputReportByteLength, preparsed_data );
    test_hidp_get_feature( file, report_id, caps.FeatureReportByteLength, preparsed_data );
    test_hidp_set_feature( file, report_id, caps.FeatureReportByteLength, preparsed_data );
    test_hidp_set_output( file, report_id, caps.OutputReportByteLength, preparsed_data );
    test_write_file( file, report_id, caps.OutputReportByteLength );

    memset( report, 0xcd, sizeof(report) );
    SetLastError( 0xdeadbeef );
    ret = ReadFile( file, report, 0, &value, NULL );
    ok( !ret && GetLastError() == ERROR_INVALID_USER_BUFFER, "ReadFile failed, last error %lu\n",
        GetLastError() );
    ok( value == 0, "ReadFile returned %lx\n", value );
    SetLastError( 0xdeadbeef );
    ret = ReadFile( file, report, caps.InputReportByteLength - 1, &value, NULL );
    ok( !ret && GetLastError() == ERROR_INVALID_USER_BUFFER, "ReadFile failed, last error %lu\n",
        GetLastError() );
    ok( value == 0, "ReadFile returned %lx\n", value );

    if (polled)
    {
        struct hid_expect expect[] =
        {
            {
                .code = IOCTL_HID_READ_REPORT,
                .report_len = caps.InputReportByteLength - (report_id ? 0 : 1),
                .report_buf = {report_id ? report_id : 0x5a,0x5a,0},
                .ret_length = 3,
                .ret_status = STATUS_SUCCESS,
            },
            {
                .code = IOCTL_HID_READ_REPORT,
                .report_len = caps.InputReportByteLength - (report_id ? 0 : 1),
                .report_buf = {report_id ? report_id : 0x5a,0x5a,1},
                .ret_length = 3,
                .ret_status = STATUS_SUCCESS,
            },
        };
        struct hid_expect expect_small[] =
        {
            {
                .code = IOCTL_HID_READ_REPORT,
                .report_len = report_id ? 2 : caps.InputReportByteLength - 1,
                .report_buf = {report_id ? report_id + 1 : 0x5a,0x5a,0x5a},
                .ret_length = report_id ? 2 : caps.InputReportByteLength - 1,
                .ret_status = STATUS_SUCCESS,
            },
        };

        send_hid_input( file, expect, sizeof(expect) );

        memset( report, 0xcd, sizeof(report) );
        SetLastError( 0xdeadbeef );
        ret = ReadFile( file, report, caps.InputReportByteLength, &value, NULL );
        ok( ret, "ReadFile failed, last error %lu\n", GetLastError() );
        ok( value == (report_id ? 3 : 4), "ReadFile returned %lx\n", value );
        ok( report[0] == report_id, "unexpected report data\n" );

        overlapped.hEvent = CreateEventW( NULL, FALSE, FALSE, NULL );
        overlapped2.hEvent = CreateEventW( NULL, FALSE, FALSE, NULL );

        /* drain available input reports */
        SetLastError( 0xdeadbeef );
        while (ReadFile( async_file, report, caps.InputReportByteLength, NULL, &overlapped ))
            ResetEvent( overlapped.hEvent );
        ok( GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %lu\n", GetLastError() );
        ret = GetOverlappedResult( async_file, &overlapped, &value, TRUE );
        ok( ret, "GetOverlappedResult failed, last error %lu\n", GetLastError() );
        ok( value == (report_id ? 3 : 4), "GetOverlappedResult returned length %lu, expected %u\n",
            value, (report_id ? 3 : 4) );
        ResetEvent( overlapped.hEvent );

        memcpy( buffer, report, caps.InputReportByteLength );
        memcpy( buffer + caps.InputReportByteLength, report, caps.InputReportByteLength );

        SetLastError( 0xdeadbeef );
        ret = ReadFile( async_file, report, caps.InputReportByteLength, NULL, &overlapped );
        ok( !ret, "ReadFile succeeded\n" );
        ok( GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = ReadFile( async_file, buffer, caps.InputReportByteLength, NULL, &overlapped2 );
        ok( !ret, "ReadFile succeeded\n" );
        ok( GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %lu\n", GetLastError() );

        /* wait for second report to be ready */
        ret = GetOverlappedResult( async_file, &overlapped2, &value, TRUE );
        ok( ret, "GetOverlappedResult failed, last error %lu\n", GetLastError() );
        ok( value == (report_id ? 3 : 4), "GetOverlappedResult returned length %lu, expected %u\n",
            value, (report_id ? 3 : 4) );
        /* first report should be ready and the same */
        ret = GetOverlappedResult( async_file, &overlapped, &value, FALSE );
        ok( ret, "GetOverlappedResult failed, last error %lu\n", GetLastError() );
        ok( value == (report_id ? 3 : 4), "GetOverlappedResult returned length %lu, expected %u\n",
            value, (report_id ? 3 : 4) );
        ok( memcmp( report, buffer + caps.InputReportByteLength, caps.InputReportByteLength ),
            "expected different report\n" );
        ok( !memcmp( report, buffer, caps.InputReportByteLength ), "expected identical reports\n" );

        value = 10;
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( file, IOCTL_HID_SET_POLL_FREQUENCY_MSEC, &value, sizeof(ULONG), NULL, NULL, 5000 );
        ok( ret, "IOCTL_HID_SET_POLL_FREQUENCY_MSEC failed last error %lu\n", GetLastError() );

        Sleep( 600 );

        SetLastError( 0xdeadbeef );
        ret = ReadFile( async_file, report, caps.InputReportByteLength, NULL, &overlapped );
        ok( !ret, "ReadFile succeeded\n" );
        ok( GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        ret = ReadFile( async_file, buffer, caps.InputReportByteLength, NULL, &overlapped2 );
        ok( !ret, "ReadFile succeeded\n" );
        ok( GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %lu\n", GetLastError() );

        /* wait for second report to be ready */
        ret = GetOverlappedResult( async_file, &overlapped2, &value, TRUE );
        ok( ret, "GetOverlappedResult failed, last error %lu\n", GetLastError() );
        ok( value == (report_id ? 3 : 4), "GetOverlappedResult returned length %lu, expected %u\n",
            value, (report_id ? 3 : 4) );
        /* first report should be ready */
        ret = GetOverlappedResult( async_file, &overlapped, &value, FALSE );
        ok( ret, "GetOverlappedResult failed, last error %lu\n", GetLastError() );
        ok( value == (report_id ? 3 : 4), "GetOverlappedResult returned length %lu, expected %u\n",
            value, (report_id ? 3 : 4) );

        send_hid_input( file, expect_small, sizeof(expect_small) );

        Sleep( 600 );

        SetLastError( 0xdeadbeef );
        memset( report, 0, sizeof(report) );
        ret = ReadFile( async_file, report, caps.InputReportByteLength, NULL, &overlapped );
        ok( !ret, "ReadFile succeeded\n" );
        ok( GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %lu\n", GetLastError() );

        ret = GetOverlappedResult( async_file, &overlapped, &value, TRUE );
        ok( ret, "GetOverlappedResult failed, last error %lu\n", GetLastError() );
        ok( value == report_id ? 2 : caps.InputReportByteLength - 1,
            "got length %lu, expected %u\n", value, report_id ? 2 : caps.InputReportByteLength - 1 );

        CloseHandle( overlapped.hEvent );
        CloseHandle( overlapped2.hEvent );
    }
    else
    {
        struct hid_expect expect[] =
        {
            {
                .code = IOCTL_HID_READ_REPORT,
                .report_len = caps.InputReportByteLength - (report_id ? 0 : 1),
                .report_buf = {report_id ? report_id : 0x5a,0x5a,0x5a},
                .ret_length = 3,
                .ret_status = STATUS_SUCCESS,
            },
            {
                .code = IOCTL_HID_READ_REPORT,
                .report_len = caps.InputReportByteLength - (report_id ? 0 : 1),
                .report_buf = {report_id ? report_id : 0xa5,0xa5,0xa5,0xa5,0xa5},
                .ret_length = caps.InputReportByteLength - (report_id ? 0 : 1),
                .ret_status = STATUS_SUCCESS,
            },
        };
        struct hid_expect expect_small[] =
        {
            {
                .code = IOCTL_HID_READ_REPORT,
                .report_len = report_id ? 2 : caps.InputReportByteLength - 1,
                .report_buf = {report_id ? report_id + 1 : 0x5a,0x5a,0x5a},
                .ret_length = report_id ? 2 : caps.InputReportByteLength - 1,
                .ret_status = STATUS_SUCCESS,
            },
        };

        overlapped.hEvent = CreateEventW( NULL, FALSE, FALSE, NULL );
        overlapped2.hEvent = CreateEventW( NULL, FALSE, FALSE, NULL );

        SetLastError( 0xdeadbeef );
        memset( report, 0, sizeof(report) );
        ret = ReadFile( async_file, report, caps.InputReportByteLength, NULL, &overlapped );
        ok( !ret, "ReadFile succeeded\n" );
        ok( GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %lu\n", GetLastError() );
        Sleep( 50 );
        ret = GetOverlappedResult( async_file, &overlapped, &value, FALSE );
        ok( !ret, "GetOverlappedResult succeeded\n" );
        ok( GetLastError() == ERROR_IO_INCOMPLETE, "GetOverlappedResult returned error %lu\n", GetLastError() );

        SetLastError( 0xdeadbeef );
        memset( buffer, 0, sizeof(buffer) );
        ret = ReadFile( async_file, buffer, caps.InputReportByteLength, NULL, &overlapped2 );
        ok( !ret, "ReadFile succeeded\n" );
        ok( GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %lu\n", GetLastError() );
        Sleep( 50 );
        ret = GetOverlappedResult( async_file, &overlapped2, &value, FALSE );
        ok( !ret, "GetOverlappedResult succeeded\n" );
        ok( GetLastError() == ERROR_IO_INCOMPLETE, "GetOverlappedResult returned error %lu\n", GetLastError() );

        memset( report + caps.InputReportByteLength, 0xa5, 5 );
        if (report_id) report[caps.InputReportByteLength] = report_id;

        send_hid_input( file, expect, sizeof(expect) );

        /* first read should be completed */
        ret = GetOverlappedResult( async_file, &overlapped, &value, TRUE );
        ok( ret, "GetOverlappedResult failed, last error %lu\n", GetLastError() );
        ok( value == caps.InputReportByteLength, "got length %lu, expected %u\n", value, caps.InputReportByteLength );
        /* second read should still be pending */
        Sleep( 50 );
        ret = GetOverlappedResult( async_file, &overlapped2, &value, FALSE );
        ok( !ret, "GetOverlappedResult succeeded\n" );
        ok( GetLastError() == ERROR_IO_INCOMPLETE, "GetOverlappedResult returned error %lu\n", GetLastError() );

        memset( buffer + caps.InputReportByteLength, 0x3b, 5 );
        if (report_id) buffer[caps.InputReportByteLength] = report_id;
        memset( expect[1].report_buf, 0x3b, 5 );
        if (report_id) expect[1].report_buf[0] = report_id;

        send_hid_input( file, expect, sizeof(expect) );

        ret = GetOverlappedResult( async_file, &overlapped2, &value, TRUE );
        ok( ret, "GetOverlappedResult failed, last error %lu\n", GetLastError() );
        ok( value == caps.InputReportByteLength, "got length %lu, expected %u\n", value, caps.InputReportByteLength );

        off = report_id ? 0 : 1;
        ok( memcmp( report, buffer, caps.InputReportByteLength ), "expected different report\n" );
        ok( !memcmp( report + off, report + caps.InputReportByteLength, caps.InputReportByteLength - off ),
            "expected identical reports\n" );
        ok( !memcmp( buffer + off, buffer + caps.InputReportByteLength, caps.InputReportByteLength - off ),
            "expected identical reports\n" );

        SetLastError( 0xdeadbeef );
        memset( report, 0, sizeof(report) );
        ret = ReadFile( async_file, report, caps.InputReportByteLength, NULL, &overlapped );
        ok( !ret, "ReadFile succeeded\n" );
        ok( GetLastError() == ERROR_IO_PENDING, "ReadFile returned error %lu\n", GetLastError() );

        send_hid_input( file, expect_small, sizeof(expect_small) );

        ret = GetOverlappedResult( async_file, &overlapped, &value, TRUE );
        ok( ret, "GetOverlappedResult failed, last error %lu\n", GetLastError() );
        ok( value == caps.InputReportByteLength, "got length %lu, expected %u\n", value, caps.InputReportByteLength );

        CloseHandle( overlapped.hEvent );
        CloseHandle( overlapped2.hEvent );
    }

    HidD_FreePreparsedData( preparsed_data );
}

static void test_hid_device( DWORD report_id, DWORD polled, const HIDP_CAPS *expect_caps, WORD vid, WORD pid )
{
    ULONG count, poll_freq, out_len;
    WCHAR device_path[MAX_PATH];
    HANDLE file, async_file;
    DWORD ret;

    winetest_push_context( "id %ld%s", report_id, polled ? " poll" : "" );

    /* Win7 has a spurious device removal event with polled HID devices */
    if (!polled || !strcmp(winetest_platform, "wine")) ret = WAIT_TIMEOUT;
    else if (!(ret = WaitForSingleObject( device_removed, 2000 )))
    {
        ret = WaitForSingleObject( device_added, 5000 );
        ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    }

    swprintf( device_path, MAX_PATH, L"\\\\?\\hid#vid_%04x&pid_%04x", vid, pid );
    ret = find_hid_device_path( device_path );
    ok( ret, "Failed to find HID device matching %s\n", debugstr_w( device_path ) );

    file = CreateFileW( device_path, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
    ok( file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError() );

    count = 0xdeadbeef;
    SetLastError( 0xdeadbeef );
    ret = HidD_GetNumInputBuffers( file, &count );
    ok( ret, "HidD_GetNumInputBuffers failed last error %lu\n", GetLastError() );
    ok( count == 32, "HidD_GetNumInputBuffers returned %lu\n", count );

    SetLastError( 0xdeadbeef );
    ret = HidD_SetNumInputBuffers( file, 1 );
    ok( !ret, "HidD_SetNumInputBuffers succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "HidD_SetNumInputBuffers returned error %lu\n",
        GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = HidD_SetNumInputBuffers( file, 513 );
    ok( !ret, "HidD_SetNumInputBuffers succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "HidD_SetNumInputBuffers returned error %lu\n",
        GetLastError() );

    SetLastError( 0xdeadbeef );
    ret = HidD_SetNumInputBuffers( file, 16 );
    ok( ret, "HidD_SetNumInputBuffers failed last error %lu\n", GetLastError() );

    count = 0xdeadbeef;
    SetLastError( 0xdeadbeef );
    ret = HidD_GetNumInputBuffers( file, &count );
    ok( ret, "HidD_GetNumInputBuffers failed last error %lu\n", GetLastError() );
    ok( count == 16, "HidD_GetNumInputBuffers returned %lu\n", count );

    async_file = CreateFileW( device_path, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                              FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING, NULL );
    ok( async_file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError() );

    count = 0xdeadbeef;
    SetLastError( 0xdeadbeef );
    ret = HidD_GetNumInputBuffers( async_file, &count );
    ok( ret, "HidD_GetNumInputBuffers failed last error %lu\n", GetLastError() );
    ok( count == 32, "HidD_GetNumInputBuffers returned %lu\n", count );

    SetLastError( 0xdeadbeef );
    ret = HidD_SetNumInputBuffers( async_file, 2 );
    ok( ret, "HidD_SetNumInputBuffers failed last error %lu\n", GetLastError() );

    count = 0xdeadbeef;
    SetLastError( 0xdeadbeef );
    ret = HidD_GetNumInputBuffers( async_file, &count );
    ok( ret, "HidD_GetNumInputBuffers failed last error %lu\n", GetLastError() );
    ok( count == 2, "HidD_GetNumInputBuffers returned %lu\n", count );
    count = 0xdeadbeef;
    SetLastError( 0xdeadbeef );
    ret = HidD_GetNumInputBuffers( file, &count );
    ok( ret, "HidD_GetNumInputBuffers failed last error %lu\n", GetLastError() );
    ok( count == 16, "HidD_GetNumInputBuffers returned %lu\n", count );

    if (polled)
    {
        out_len = sizeof(ULONG);
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( file, IOCTL_HID_GET_POLL_FREQUENCY_MSEC, NULL, 0, &poll_freq, &out_len, 5000 );
        ok( ret, "IOCTL_HID_GET_POLL_FREQUENCY_MSEC failed last error %lu\n", GetLastError() );
        ok( out_len == sizeof(ULONG), "got out_len %lu, expected sizeof(ULONG)\n", out_len );
        todo_wine
        ok( poll_freq == 5, "got poll_freq %lu, expected 5\n", poll_freq );

        out_len = 0;
        poll_freq = 500;
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( file, IOCTL_HID_SET_POLL_FREQUENCY_MSEC, &poll_freq, sizeof(ULONG), NULL, &out_len, 5000 );
        ok( ret, "IOCTL_HID_SET_POLL_FREQUENCY_MSEC failed last error %lu\n", GetLastError() );
        ok( out_len == 0, "got out_len %lu, expected 0\n", out_len );

        out_len = 0;
        poll_freq = 10001;
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( file, IOCTL_HID_SET_POLL_FREQUENCY_MSEC, &poll_freq, sizeof(ULONG), NULL, &out_len, 5000 );
        ok( ret, "IOCTL_HID_SET_POLL_FREQUENCY_MSEC failed last error %lu\n", GetLastError() );
        ok( out_len == 0, "got out_len %lu, expected 0\n", out_len );

        out_len = 0;
        poll_freq = 0;
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( file, IOCTL_HID_SET_POLL_FREQUENCY_MSEC, &poll_freq, sizeof(ULONG), NULL, &out_len, 5000 );
        ok( ret, "IOCTL_HID_SET_POLL_FREQUENCY_MSEC failed last error %lu\n", GetLastError() );
        ok( out_len == 0, "got out_len %lu, expected 0\n", out_len );

        out_len = sizeof(ULONG);
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( file, IOCTL_HID_GET_POLL_FREQUENCY_MSEC, NULL, 0, &poll_freq, &out_len, 5000 );
        ok( ret, "IOCTL_HID_GET_POLL_FREQUENCY_MSEC failed last error %lu\n", GetLastError() );
        ok( out_len == sizeof(ULONG), "got out_len %lu, expected sizeof(ULONG)\n", out_len );
        ok( poll_freq == 10000, "got poll_freq %lu, expected 10000\n", poll_freq );

        out_len = 0;
        poll_freq = 500;
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( file, IOCTL_HID_SET_POLL_FREQUENCY_MSEC, &poll_freq, sizeof(ULONG), NULL, &out_len, 5000 );
        ok( ret, "IOCTL_HID_SET_POLL_FREQUENCY_MSEC failed last error %lu\n", GetLastError() );
        ok( out_len == 0, "got out_len %lu, expected 0\n", out_len );

        out_len = sizeof(ULONG);
        SetLastError( 0xdeadbeef );
        ret = sync_ioctl( async_file, IOCTL_HID_GET_POLL_FREQUENCY_MSEC, NULL, 0, &poll_freq, &out_len, 5000 );
        ok( ret, "IOCTL_HID_GET_POLL_FREQUENCY_MSEC failed last error %lu\n", GetLastError() );
        ok( out_len == sizeof(ULONG), "got out_len %lu, expected sizeof(ULONG)\n", out_len );
        ok( poll_freq == 500, "got poll_freq %lu, expected 500\n", poll_freq );
    }

    test_hidp( file, async_file, report_id, polled, expect_caps );

    CloseHandle( async_file );
    CloseHandle( file );

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
                REPORT_SIZE(1, 8),
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
                REPORT_SIZE(1, 24),
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
            USAGE(1, HID_USAGE_GENERIC_SYSTEM_CTL),
            COLLECTION(1, Report),
                REPORT_ID_OR_USAGE_PAGE(1, report_id, 0),
                REPORT_COUNT(1, 10),
                REPORT_SIZE(1, 8),
                LOGICAL_MINIMUM(2, 0x0000),
                LOGICAL_MAXIMUM(2, 0x00ff),
                PHYSICAL_MINIMUM(2, 0x0000),
                PHYSICAL_MAXIMUM(2, 0x0ff0),
                USAGE_PAGE(1, HID_USAGE_PAGE_ORDINAL),
                STRING_MINIMUM(1, 6),
                STRING_MAXIMUM(1, 16),
                USAGE_MINIMUM(1, 1),  /* Instance 1 */
                USAGE_MAXIMUM(1, 10), /* Instance 10 */
                FEATURE(1, Data|Var|Abs),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 0),
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
    C_ASSERT(sizeof(report_desc) < MAX_HID_DESCRIPTOR_LEN);
#include "pop_hid_macros.h"

    const HIDP_CAPS caps =
    {
        .Usage = HID_USAGE_GENERIC_JOYSTICK,
        .UsagePage = HID_USAGE_PAGE_GENERIC,
        .InputReportByteLength = report_id ? 37 : 38,
        .OutputReportByteLength = report_id ? 2 : 3,
        .FeatureReportByteLength = report_id ? 31 : 32,
        .NumberLinkCollectionNodes = 11,
        .NumberInputButtonCaps = 17,
        .NumberInputValueCaps = 7,
        .NumberInputDataIndices = 47,
        .NumberFeatureButtonCaps = 1,
        .NumberFeatureValueCaps = 7,
        .NumberFeatureDataIndices = 18,
    };
    const struct hid_expect expect_in =
    {
        .code = IOCTL_HID_READ_REPORT,
        .report_len = caps.InputReportByteLength - (report_id ? 0 : 1),
        .report_buf = {report_id ? report_id : 0x5a,0x5a,0x5a},
        .ret_length = 3,
        .ret_status = STATUS_SUCCESS,
    };
    struct hid_device_desc desc =
    {
        .is_polled = polled,
        .use_report_id = report_id,
        .caps = caps,
        .attributes = default_attributes,
    };

    desc.report_descriptor_len = sizeof(report_desc);
    memcpy( desc.report_descriptor_buf, report_desc, sizeof(report_desc) );
    desc.input_size = polled ? sizeof(expect_in) : 0;
    memcpy( desc.input, &expect_in, sizeof(expect_in) );
    fill_context( desc.context, ARRAY_SIZE(desc.context) );

    if (hid_device_start( &desc, 1 )) test_hid_device( report_id, polled, &caps, desc.attributes.VendorID, desc.attributes.ProductID );
    hid_device_stop( &desc, 1 );
}

/* undocumented HID internal preparsed data structure */

struct hidp_kdr_caps
{
    USHORT usage_page;
    UCHAR report_id;
    UCHAR start_bit;
    USHORT bit_size;
    USHORT report_count;
    USHORT start_byte;
    USHORT total_bits;
    ULONG bit_field;
    USHORT end_byte;
    USHORT link_collection;
    USAGE link_usage_page;
    USAGE link_usage;
    ULONG flags;
    ULONG padding[8];
    USAGE usage_min;
    USAGE usage_max;
    USHORT string_min;
    USHORT string_max;
    USHORT designator_min;
    USHORT designator_max;
    USHORT data_index_min;
    USHORT data_index_max;
    USHORT null_value;
    USHORT unknown;
    LONG logical_min;
    LONG logical_max;
    LONG physical_min;
    LONG physical_max;
    LONG units;
    LONG units_exp;
};

/* named array continues on next caps */
#define HIDP_KDR_CAPS_ARRAY_HAS_MORE       0x01
#define HIDP_KDR_CAPS_IS_CONSTANT          0x02
#define HIDP_KDR_CAPS_IS_BUTTON            0x04
#define HIDP_KDR_CAPS_IS_ABSOLUTE          0x08
#define HIDP_KDR_CAPS_IS_RANGE             0x10
#define HIDP_KDR_CAPS_IS_STRING_RANGE      0x40
#define HIDP_KDR_CAPS_IS_DESIGNATOR_RANGE  0x80

struct hidp_kdr_node
{
    USAGE usage;
    USAGE usage_page;
    USHORT parent;
    USHORT number_of_children;
    USHORT next_sibling;
    USHORT first_child;
    ULONG collection_type;
};

struct hidp_kdr
{
    char magic[8];
    USAGE usage;
    USAGE usage_page;
    USHORT unknown[2];
    USHORT input_caps_start;
    USHORT input_caps_count;
    USHORT input_caps_end;
    USHORT input_report_byte_length;
    USHORT output_caps_start;
    USHORT output_caps_count;
    USHORT output_caps_end;
    USHORT output_report_byte_length;
    USHORT feature_caps_start;
    USHORT feature_caps_count;
    USHORT feature_caps_end;
    USHORT feature_report_byte_length;
    USHORT caps_size;
    USHORT number_link_collection_nodes;
    struct hidp_kdr_caps caps[1];
    /* struct hidp_kdr_node nodes[1] */
};

static void check_preparsed_data( HANDLE file, const struct hidp_kdr *expect_kdr,
                                  UINT expect_caps_count, const struct hidp_kdr_caps *expect_caps,
                                  UINT expect_nodes_count, const struct hidp_kdr_node *expect_nodes )
{
    PHIDP_PREPARSED_DATA preparsed_data;
    struct hidp_kdr *kdr;
    BOOL ret;
    UINT i;

    ret = HidD_GetPreparsedData( file, &preparsed_data );
    ok( ret, "HidD_GetPreparsedData failed with error %lu\n", GetLastError() );

    kdr = (struct hidp_kdr *)preparsed_data;
    ok( !strncmp( kdr->magic, expect_kdr->magic, 8 ), "got %s expected %s\n",
        debugstr_an(kdr->magic, 8), debugstr_an(expect_kdr->magic, 8) );

    if (!strncmp( kdr->magic, expect_kdr->magic, 8 ))
    {
        check_member( *kdr, *expect_kdr, "%04x", usage );
        check_member( *kdr, *expect_kdr, "%04x", usage_page );
        check_member( *kdr, *expect_kdr, "%#x", unknown[0] );
        check_member( *kdr, *expect_kdr, "%#x", unknown[1] );
        check_member( *kdr, *expect_kdr, "%d", input_caps_start );
        check_member( *kdr, *expect_kdr, "%d", input_caps_count );
        check_member( *kdr, *expect_kdr, "%d", input_caps_end );
        check_member( *kdr, *expect_kdr, "%d", input_report_byte_length );
        check_member( *kdr, *expect_kdr, "%d", output_caps_start );
        check_member( *kdr, *expect_kdr, "%d", output_caps_count );
        check_member( *kdr, *expect_kdr, "%d", output_caps_end );
        check_member( *kdr, *expect_kdr, "%d", output_report_byte_length );
        check_member( *kdr, *expect_kdr, "%d", feature_caps_start );
        check_member( *kdr, *expect_kdr, "%d", feature_caps_count );
        check_member( *kdr, *expect_kdr, "%d", feature_caps_end );
        check_member( *kdr, *expect_kdr, "%d", feature_report_byte_length );
        check_member( *kdr, *expect_kdr, "%d", caps_size );
        check_member( *kdr, *expect_kdr, "%d", number_link_collection_nodes );

        for (i = 0; i < min( expect_caps_count, kdr->caps_size / sizeof(struct hidp_kdr_caps) ); ++i)
        {
            winetest_push_context( "caps[%u]", i );
            check_member( kdr->caps[i], expect_caps[i], "%04x", usage_page );
            check_member( kdr->caps[i], expect_caps[i], "%d", report_id );
            check_member( kdr->caps[i], expect_caps[i], "%d", start_bit );
            check_member( kdr->caps[i], expect_caps[i], "%d", bit_size );
            check_member( kdr->caps[i], expect_caps[i], "%d", report_count );
            check_member( kdr->caps[i], expect_caps[i], "%d", start_byte );
            check_member( kdr->caps[i], expect_caps[i], "%d", total_bits );
            check_member( kdr->caps[i], expect_caps[i], "%#lx", bit_field );
            check_member( kdr->caps[i], expect_caps[i], "%d", end_byte );
            check_member( kdr->caps[i], expect_caps[i], "%d", link_collection );
            check_member( kdr->caps[i], expect_caps[i], "%04x", link_usage_page );
            check_member( kdr->caps[i], expect_caps[i], "%04x", link_usage );
            check_member( kdr->caps[i], expect_caps[i], "%#lx", flags );
            check_member( kdr->caps[i], expect_caps[i], "%#lx", padding[0] );
            check_member( kdr->caps[i], expect_caps[i], "%#lx", padding[1] );
            check_member( kdr->caps[i], expect_caps[i], "%#lx", padding[2] );
            check_member( kdr->caps[i], expect_caps[i], "%#lx", padding[3] );
            check_member( kdr->caps[i], expect_caps[i], "%#lx", padding[4] );
            check_member( kdr->caps[i], expect_caps[i], "%#lx", padding[5] );
            check_member( kdr->caps[i], expect_caps[i], "%#lx", padding[6] );
            check_member( kdr->caps[i], expect_caps[i], "%#lx", padding[7] );
            check_member( kdr->caps[i], expect_caps[i], "%04x", usage_min );
            check_member( kdr->caps[i], expect_caps[i], "%04x", usage_max );
            check_member( kdr->caps[i], expect_caps[i], "%d", string_min );
            check_member( kdr->caps[i], expect_caps[i], "%d", string_max );
            check_member( kdr->caps[i], expect_caps[i], "%d", designator_min );
            check_member( kdr->caps[i], expect_caps[i], "%d", designator_max );
            check_member( kdr->caps[i], expect_caps[i], "%#x", data_index_min );
            check_member( kdr->caps[i], expect_caps[i], "%#x", data_index_max );
            check_member( kdr->caps[i], expect_caps[i], "%d", null_value );
            check_member( kdr->caps[i], expect_caps[i], "%d", unknown );
            check_member( kdr->caps[i], expect_caps[i], "%ld", logical_min );
            check_member( kdr->caps[i], expect_caps[i], "%ld", logical_max );
            check_member( kdr->caps[i], expect_caps[i], "%ld", physical_min );
            check_member( kdr->caps[i], expect_caps[i], "%ld", physical_max );
            check_member( kdr->caps[i], expect_caps[i], "%#lx", units );
            check_member( kdr->caps[i], expect_caps[i], "%#lx", units_exp );
            winetest_pop_context();
        }

        for (i = 0; i < expect_nodes_count; ++i)
        {
            struct hidp_kdr_node *nodes = (struct hidp_kdr_node *)((char *)kdr->caps + kdr->caps_size);
            winetest_push_context( "nodes[%u]", i );
            check_member( nodes[i], expect_nodes[i], "%04x", usage );
            check_member( nodes[i], expect_nodes[i], "%04x", usage_page );
            check_member( nodes[i], expect_nodes[i], "%d", parent );
            check_member( nodes[i], expect_nodes[i], "%d", number_of_children );
            check_member( nodes[i], expect_nodes[i], "%d", next_sibling );
            check_member( nodes[i], expect_nodes[i], "%d", first_child );
            check_member( nodes[i], expect_nodes[i], "%#lx", collection_type );
            winetest_pop_context();
        }
    }

    HidD_FreePreparsedData( preparsed_data );
}

static void test_hidp_kdr(void)
{
#include "psh_hid_macros.h"
    const unsigned char report_desc[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
        COLLECTION(1, Application),
            USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
            LOGICAL_MINIMUM(1, 1),
            LOGICAL_MAXIMUM(1, 127),
            PHYSICAL_MINIMUM(1, -128),
            PHYSICAL_MAXIMUM(1, 127),

            USAGE(1, HID_USAGE_GENERIC_RZ),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 0),
            FEATURE(1, Data|Var|Abs),
            USAGE(1, HID_USAGE_GENERIC_SLIDER),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 1),
            FEATURE(1, Data|Var|Abs),

            USAGE(1, HID_USAGE_GENERIC_X),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            UNIT(1, 0x100e),
            UNIT_EXPONENT(1, -3),
            INPUT(1, Data|Var|Abs),
            UNIT_EXPONENT(1, 0),
            UNIT(1, 0),
            USAGE(1, HID_USAGE_GENERIC_Y),
            DESIGNATOR_MINIMUM(1, 1),
            DESIGNATOR_MAXIMUM(1, 4),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            INPUT(1, Cnst|Var|Abs),
            USAGE(1, HID_USAGE_GENERIC_Z),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            INPUT(1, Data|Var|Rel),
            USAGE(1, HID_USAGE_GENERIC_RX),
            USAGE(1, HID_USAGE_GENERIC_RY),
            REPORT_SIZE(1, 16),
            REPORT_COUNT(1, 2),
            LOGICAL_MINIMUM(1, 7),
            INPUT(1, Data|Var|Abs|Null),

            COLLECTION(1, Application),
                USAGE(4, (HID_USAGE_PAGE_BUTTON << 16)|1),
                USAGE(4, (HID_USAGE_PAGE_BUTTON << 16)|2),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                INPUT(1, Data|Var|Abs),

                USAGE_MINIMUM(4, (HID_USAGE_PAGE_BUTTON << 16)|3),
                USAGE_MAXIMUM(4, (HID_USAGE_PAGE_BUTTON << 16)|8),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                LOGICAL_MINIMUM(1, 3),
                LOGICAL_MAXIMUM(1, 8),
                INPUT(1, Data|Ary|Abs),

                USAGE_MINIMUM(4, (HID_USAGE_PAGE_BUTTON << 16)|9),
                USAGE_MAXIMUM(4, (HID_USAGE_PAGE_BUTTON << 16)|12),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 4),
                LOGICAL_MINIMUM(1, 9),
                LOGICAL_MAXIMUM(1, 12),
                INPUT(2, Data|Ary|Abs|Buff),

                USAGE(4, (HID_USAGE_PAGE_BUTTON << 16)|13),
                USAGE(4, (HID_USAGE_PAGE_BUTTON << 16)|14),
                USAGE(4, (HID_USAGE_PAGE_BUTTON << 16)|15),
                USAGE(4, (HID_USAGE_PAGE_BUTTON << 16)|16),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 1),
                LOGICAL_MINIMUM(1, 13),
                LOGICAL_MAXIMUM(1, 16),
                INPUT(1, Data|Ary|Abs),
            END_COLLECTION,
        END_COLLECTION,
    };
    C_ASSERT(sizeof(report_desc) < MAX_HID_DESCRIPTOR_LEN);
#include "pop_hid_macros.h"

    struct hid_device_desc desc =
    {
        .caps = { .InputReportByteLength = 15 },
        .attributes = default_attributes,
    };
    static const struct hidp_kdr expect_kdr =
    {
        .magic = "HidP KDR",
        .usage = 0x04,
        .usage_page = 0x01,
        .input_caps_count = 13,
        .input_caps_end = 13,
        .input_report_byte_length = 15,
        .output_caps_start = 13,
        .output_caps_end = 13,
        .feature_caps_start = 13,
        .feature_caps_count = 2,
        .feature_caps_end = 14,
        .feature_report_byte_length = 3,
        .caps_size = 1560,
        .number_link_collection_nodes = 2,
    };
    static const struct hidp_kdr_caps expect_caps[] =
    {
        {
            .usage_page = 0x01, .bit_size = 0x08, .report_count = 0x1, .start_byte = 0x1, .total_bits = 0x08,
            .bit_field = 0x002, .end_byte = 0x2, .link_usage_page = 0x01, .link_usage = 0x04, .flags = 0x08,
            .usage_min = 0x30, .usage_max = 0x30, .logical_min = 1, .logical_max = 127, .physical_min = -128,
            .physical_max = 127, .units = 0xe, .units_exp = -3
        },
        {
            .usage_page = 0x01, .bit_size = 0x08, .report_count = 0x1, .start_byte = 0x2, .total_bits = 0x08,
            .bit_field = 0x003, .end_byte = 0x3, .link_usage_page = 0x01, .link_usage = 0x04, .flags = 0x8a,
            .usage_min = 0x31, .usage_max = 0x31, .designator_min = 1, .designator_max = 4, .data_index_min = 0x01,
            .data_index_max = 0x01, .logical_min = 1, .logical_max = 127, .physical_min = -128, .physical_max = 127
        },
        {
            .usage_page = 0x01, .bit_size = 0x08, .report_count = 0x1, .start_byte = 0x3, .total_bits = 0x08,
            .bit_field = 0x006, .end_byte = 0x4, .link_usage_page = 0x01, .link_usage = 0x04, .usage_min = 0x32,
            .usage_max = 0x32, .data_index_min = 0x02, .data_index_max = 0x02, .logical_min = 1, .logical_max = 127,
            .physical_min = -128, .physical_max = 127
        },
        {
            .usage_page = 0x01, .bit_size = 0x10, .report_count = 0x1, .start_byte = 0x6, .total_bits = 0x10,
            .bit_field = 0x042, .end_byte = 0x8, .link_usage_page = 0x01, .link_usage = 0x04, .flags = 0x08,
            .usage_min = 0x34, .usage_max = 0x34, .data_index_min = 0x03, .data_index_max = 0x03, .null_value = 1,
            .logical_min = 7, .logical_max = 127, .physical_min = -128, .physical_max = 127
        },
        {
            .usage_page = 0x01, .bit_size = 0x10, .report_count = 0x1, .start_byte = 0x4, .total_bits = 0x10,
            .bit_field = 0x042, .end_byte = 0x6, .link_usage_page = 0x01, .link_usage = 0x04, .flags = 0x08,
            .usage_min = 0x33, .usage_max = 0x33, .data_index_min = 0x04, .data_index_max = 0x04, .null_value = 1,
            .logical_min = 7, .logical_max = 127, .physical_min = -128, .physical_max = 127
        },
        {
            .usage_page = 0x09, .start_bit = 1, .bit_size = 0x01, .report_count = 0x7, .start_byte = 0x8, .total_bits = 0x07,
            .bit_field = 0x002, .end_byte = 0x9, .link_collection = 1, .link_usage_page = 0x01, .flags = 0x0c,
            .usage_min = 0x02, .usage_max = 0x02, .data_index_min = 0x05, .data_index_max = 0x05,
        },
        {
            .usage_page = 0x09, .bit_size = 0x01, .report_count = 0x1, .start_byte = 0x8, .total_bits = 0x01,
            .bit_field = 0x002, .end_byte = 0x9, .link_collection = 1, .link_usage_page = 0x01, .flags = 0x0c,
            .usage_min = 0x01, .usage_max = 0x01, .data_index_min = 0x06, .data_index_max = 0x06,
        },
        {
            .usage_page = 0x09, .bit_size = 0x08, .report_count = 0x1, .start_byte = 0x9, .total_bits = 0x08,
            .bit_field = 0x000, .end_byte = 0xa, .link_collection = 1, .link_usage_page = 0x01, .flags = 0x1c,
            .usage_min = 0x03, .usage_max = 0x08, .data_index_min = 0x07, .data_index_max = 0x0c, .null_value = 3,
            .logical_min = 8
        },
        {
            .usage_page = 0x09, .bit_size = 0x08, .report_count = 0x4, .start_byte = 0xa, .total_bits = 0x20,
            .bit_field = 0x100, .end_byte = 0xe, .link_collection = 1, .link_usage_page = 0x01, .flags = 0x1c,
            .usage_min = 0x09, .usage_max = 0x0c, .data_index_min = 0x0d, .data_index_max = 0x10, .null_value = 9,
            .logical_min = 12
        },
        {
            .usage_page = 0x09, .bit_size = 0x08, .report_count = 0x1, .start_byte = 0xe, .total_bits = 0x08,
            .bit_field = 0x000, .end_byte = 0xf, .link_collection = 1, .link_usage_page = 0x01, .flags = 0x0d,
            .usage_min = 0x10, .usage_max = 0x10, .data_index_min = 0x14, .data_index_max = 0x14, .null_value = 13,
            .logical_min = 16
        },
        {
            .usage_page = 0x09, .bit_size = 0x08, .report_count = 0x1, .start_byte = 0xe, .total_bits = 0x08,
            .bit_field = 0x000, .end_byte = 0xf, .link_collection = 1, .link_usage_page = 0x01, .flags = 0x0d,
            .usage_min = 0x0f, .usage_max = 0x0f, .data_index_min = 0x13, .data_index_max = 0x13, .null_value = 13,
            .logical_min = 16
        },
        {
            .usage_page = 0x09, .bit_size = 0x08, .report_count = 0x1, .start_byte = 0xe, .total_bits = 0x08,
            .bit_field = 0x000, .end_byte = 0xf, .link_collection = 1, .link_usage_page = 0x01, .flags = 0x0d,
            .usage_min = 0x0e, .usage_max = 0x0e, .data_index_min = 0x12, .data_index_max = 0x12, .null_value = 13,
            .logical_min = 16
        },
        {
            .usage_page = 0x09, .bit_size = 0x08, .report_count = 0x1, .start_byte = 0xe, .total_bits = 0x08,
            .bit_field = 0x000, .end_byte = 0xf, .link_collection = 1, .link_usage_page = 0x01, .flags = 0x0c,
            .usage_min = 0x0d, .usage_max = 0x0d, .data_index_min = 0x11, .data_index_max = 0x11, .null_value = 13,
            .logical_min = 16
        },
        {
            .usage_page = 0x01, .bit_size = 0x10, .report_count = 0x1, .start_byte = 0x1, .total_bits = 0x10,
            .bit_field = 0x002, .end_byte = 0x3, .link_usage_page = 0x01, .link_usage = 0x04, .flags = 0x08,
            .usage_min = 0x36, .usage_max = 0x36, .logical_min = 1, .logical_max = 127, .physical_min = -128,
            .physical_max = 127
        },
        {
        },
    };
    static const struct hidp_kdr_node expect_nodes[] =
    {
        {
            .usage = 0x04,
            .usage_page = 0x01,
            .parent = 0,
            .number_of_children = 0x1,
            .next_sibling = 0,
            .first_child = 0x1,
            .collection_type = 0x1,
        },
        {
            .usage = 0x00,
            .usage_page = 0x01,
            .parent = 0,
            .number_of_children = 0,
            .next_sibling = 0,
            .first_child = 0,
            .collection_type = 0x1,
        },
    };

    WCHAR device_path[MAX_PATH];
    HANDLE file;
    BOOL ret;

    desc.report_descriptor_len = sizeof(report_desc);
    memcpy( desc.report_descriptor_buf, report_desc, sizeof(report_desc) );
    fill_context( desc.context, ARRAY_SIZE(desc.context) );

    if (!hid_device_start( &desc, 1 )) goto done;

    swprintf( device_path, MAX_PATH, L"\\\\?\\hid#vid_%04x&pid_%04x", desc.attributes.VendorID,
              desc.attributes.ProductID );
    ret = find_hid_device_path( device_path );
    ok( ret, "Failed to find HID device matching %s\n", debugstr_w( device_path ) );

    file = CreateFileW( device_path, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
    ok( file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError() );

    check_preparsed_data( file, &expect_kdr, ARRAY_SIZE(expect_caps), expect_caps,
                          ARRAY_SIZE(expect_nodes), expect_nodes );

    CloseHandle( file );

done:
    hid_device_stop( &desc, 1 );
}

void cleanup_registry_keys(void)
{
    static const WCHAR joystick_oem_path[] = L"System\\CurrentControlSet\\Control\\MediaProperties\\"
                                              "PrivateProperties\\Joystick\\OEM";
    static const WCHAR dinput_path[] = L"System\\CurrentControlSet\\Control\\MediaProperties\\"
                                       "PrivateProperties\\DirectInput";
    HKEY root_key;

    /* These keys are automatically created by DInput and they store the
       list of supported force-feedback effects. OEM drivers are supposed
       to provide a list in HKLM for the vendor-specific force-feedback
       support.

       We need to clean them up, or DInput will not refresh the list of
       effects from the PID report changes.
    */
    RegCreateKeyExW( HKEY_CURRENT_USER, joystick_oem_path, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &root_key, NULL );
    RegDeleteTreeW( root_key, expect_vidpid_str );
    RegCloseKey( root_key );

    RegCreateKeyExW( HKEY_CURRENT_USER, dinput_path, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &root_key, NULL );
    RegDeleteTreeW( root_key, expect_vidpid_str );
    RegCloseKey( root_key );

    RegCreateKeyExW( HKEY_LOCAL_MACHINE, joystick_oem_path, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &root_key, NULL );
    RegDeleteTreeW( root_key, expect_vidpid_str );
    RegCloseKey( root_key );

    RegCreateKeyExW( HKEY_LOCAL_MACHINE, dinput_path, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &root_key, NULL );
    RegDeleteTreeW( root_key, expect_vidpid_str );
    RegCloseKey( root_key );
}

static LRESULT CALLBACK monitor_wndproc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (msg == WM_DEVICECHANGE)
    {
        DEV_BROADCAST_DEVICEINTERFACE_W *iface = (DEV_BROADCAST_DEVICEINTERFACE_W *)lparam;

        if (!IsEqualGUID( &iface->dbcc_classguid, &control_class ) && !IsEqualGUID( &iface->dbcc_classguid, &GUID_DEVINTERFACE_HID ))
            return DefWindowProcW( hwnd, msg, wparam, lparam );
        if (!wcsnicmp( iface->dbcc_name, L"\\\\?\\root#winetest", 17 ))
            return DefWindowProcW( hwnd, msg, wparam, lparam );

        if (wparam == DBT_DEVICEREMOVECOMPLETE) ReleaseSemaphore( device_removed, 1, NULL );
        if (wparam == DBT_DEVICEARRIVAL) ReleaseSemaphore( device_added, 1, NULL );
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

DWORD WINAPI monitor_thread_proc( void *stop_event )
{
    DEV_BROADCAST_DEVICEINTERFACE_A iface_filter_a =
    {
        .dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE_A),
        .dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE,
    };
    WNDCLASSEXW class =
    {
        .cbSize = sizeof(WNDCLASSEXW),
        .hInstance = GetModuleHandleW( NULL ),
        .lpszClassName = L"device_monitor",
        .lpfnWndProc = monitor_wndproc,
    };
    HDEVNOTIFY devnotify;
    HANDLE hwnd;
    MSG msg;

    RegisterClassExW( &class );
    hwnd = CreateWindowW( class.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL );
    ok( !!hwnd, "CreateWindowW failed, error %lu\n", GetLastError() );
    devnotify = RegisterDeviceNotificationA( hwnd, &iface_filter_a, DEVICE_NOTIFY_ALL_INTERFACE_CLASSES );
    ok( !!devnotify, "RegisterDeviceNotificationA failed, error %lu\n", GetLastError() );

    do
    {
        while (PeekMessageW( &msg, hwnd, 0, 0, PM_REMOVE ))
        {
            TranslateMessage( &msg );
            DispatchMessageW( &msg );
        }
    } while (MsgWaitForMultipleObjects( 1, &stop_event, FALSE, INFINITE, QS_ALLINPUT ));

    UnregisterDeviceNotification( devnotify );
    DestroyWindow( hwnd );
    UnregisterClassW( class.lpszClassName, class.hInstance );

    CloseHandle( stop_event );
    return 0;
}

void dinput_test_init_( const char *file, int line )
{
    BOOL is_wow64;

    monitor_stop = CreateEventW( NULL, FALSE, FALSE, NULL );
    ok_(file, line)( !!monitor_stop, "CreateEventW failed, error %lu\n", GetLastError() );
    device_added = CreateSemaphoreW( NULL, 0, LONG_MAX, NULL );
    ok_(file, line)( !!device_added, "CreateSemaphoreW failed, error %lu\n", GetLastError() );
    device_removed = CreateSemaphoreW( NULL, 0, LONG_MAX, NULL );
    ok_(file, line)( !!device_removed, "CreateSemaphoreW failed, error %lu\n", GetLastError() );
    monitor_thread = CreateThread( NULL, 0, monitor_thread_proc, monitor_stop, 0, NULL );
    ok_(file, line)( !!monitor_thread, "CreateThread failed, error %lu\n", GetLastError() );

    CoInitialize( NULL );

    instance = GetModuleHandleW( NULL );
    localized = GetUserDefaultLCID() != MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
    pSignerSign = (void *)GetProcAddress( LoadLibraryW( L"mssign32" ), "SignerSign" );

    if (IsWow64Process( GetCurrentProcess(), &is_wow64 ) && is_wow64)
    {
        skip_(file, line)( "Skipping driver tests: running in wow64.\n" );
        return;
    }

    test_data_mapping = CreateFileMappingW( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                            sizeof(*test_data), L"Global\\winetest_dinput_section" );
    if (!test_data_mapping && GetLastError() == ERROR_ACCESS_DENIED)
    {
        win_skip_(file, line)( "Skipping driver tests: failed to create mapping.\n" );
        return;
    }
    ok_(file, line)( !!test_data_mapping, "got error %lu\n", GetLastError() );

    test_data = MapViewOfFile( test_data_mapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 1024 );
    ok_(file, line)( !!test_data, "MapViewOfFile failed, error %lu\n", GetLastError() );
    test_data->running_under_wine = !strcmp( winetest_platform, "wine" );
    test_data->winetest_report_success = winetest_report_success;
    test_data->winetest_debug = winetest_debug;

    okfile = CreateFileW( L"C:\\windows\\winetest_dinput_okfile", GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL );
    ok_(file, line)( okfile != INVALID_HANDLE_VALUE, "failed to create file, error %lu\n", GetLastError() );

    subtest_(file, line)( "hid" );
    subtest_(file, line)( "driver" );
    subtest_(file, line)( "driver_bus" );
    subtest_(file, line)( "driver_hid" );
    subtest_(file, line)( "driver_hid_poll" );
}

void dinput_test_exit(void)
{
    DWORD ret;

    UnmapViewOfFile( test_data );
    CloseHandle( test_data_mapping );
    CloseHandle( okfile );
    DeleteFileW( L"C:\\windows\\winetest_dinput_okfile" );

    SetEvent( monitor_stop );
    ret = WaitForSingleObject( monitor_thread, 5000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    CloseHandle( monitor_thread );
    CloseHandle( monitor_stop );
    CloseHandle( device_removed );
    CloseHandle( device_added );

    CoUninitialize();
}

BOOL CALLBACK find_test_device( const DIDEVICEINSTANCEW *devinst, void *context )
{
    if (IsEqualGUID( &devinst->guidProduct, &expect_guid_product ))
        *(DIDEVICEINSTANCEW *)context = *devinst;
    return DIENUM_CONTINUE;
}

HRESULT dinput_test_create_device( DWORD version, DIDEVICEINSTANCEW *devinst, IDirectInputDevice8W **device )
{
    IDirectInput8W *di8;
    IDirectInputW *di;
    HRESULT hr;
    ULONG ref;

    if (version >= 0x800)
    {
        hr = DirectInput8Create( instance, version, &IID_IDirectInput8W, (void **)&di8, NULL );
        if (FAILED(hr))
        {
            win_skip( "DirectInput8Create returned %#lx\n", hr );
            return hr;
        }

        hr = IDirectInput8_EnumDevices( di8, DI8DEVCLASS_ALL, find_test_device, devinst, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        if (!IsEqualGUID( &devinst->guidProduct, &expect_guid_product ))
        {
            win_skip( "device not found, skipping tests\n" );
            ref = IDirectInput8_Release( di8 );
            ok( ref == 0, "Release returned %ld\n", ref );
            return DIERR_DEVICENOTREG;
        }

        hr = IDirectInput8_CreateDevice( di8, &expect_guid_product, device, NULL );
        ok( hr == DI_OK, "CreateDevice returned %#lx\n", hr );

        ref = IDirectInput8_Release( di8 );
        ok( ref == 0, "Release returned %ld\n", ref );
    }
    else
    {
        hr = DirectInputCreateEx( instance, version, &IID_IDirectInput2W, (void **)&di, NULL );
        if (FAILED(hr))
        {
            win_skip( "DirectInputCreateEx returned %#lx\n", hr );
            return hr;
        }

        hr = IDirectInput_EnumDevices( di, 0, find_test_device, devinst, DIEDFL_ALLDEVICES );
        ok( hr == DI_OK, "EnumDevices returned: %#lx\n", hr );
        if (!IsEqualGUID( &devinst->guidProduct, &expect_guid_product ))
        {
            win_skip( "device not found, skipping tests\n" );

            ref = IDirectInput_Release( di );
            ok( ref == 0, "Release returned %ld\n", ref );
            return DIERR_DEVICENOTREG;
        }

        hr = IDirectInput_CreateDevice( di, &expect_guid_product, (IDirectInputDeviceW **)device, NULL );
        ok( hr == DI_OK, "CreateDevice returned %#lx\n", hr );

        ref = IDirectInput_Release( di );
        ok( ref == 0, "Release returned %ld\n", ref );
    }

    return DI_OK;
}

DWORD WINAPI dinput_test_device_thread( void *stop_event )
{
#include "psh_hid_macros.h"
    static const unsigned char gamepad_desc[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_GAMEPAD),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_GAMEPAD),
            COLLECTION(1, Physical),
                USAGE(1, HID_USAGE_GENERIC_X),
                USAGE(1, HID_USAGE_GENERIC_Y),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 127),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 127),
                REPORT_SIZE(1, 8),
                REPORT_COUNT(1, 2),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 1),
                USAGE_MAXIMUM(1, 6),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                PHYSICAL_MINIMUM(1, 0),
                PHYSICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,
        END_COLLECTION,
    };
    C_ASSERT(sizeof(gamepad_desc) < MAX_HID_DESCRIPTOR_LEN);
#include "pop_hid_macros.h"
    static const HID_DEVICE_ATTRIBUTES attributes =
    {
        .Size = sizeof(HID_DEVICE_ATTRIBUTES),
        .VendorID = LOWORD(EXPECT_VIDPID),
        .ProductID = HIWORD(EXPECT_VIDPID),
        .VersionNumber = 0x0100,
    };
    static const HIDP_CAPS caps =
    {
        .InputReportByteLength = 3,
    };
    struct hid_device_desc desc =
    {
        .use_report_id = TRUE,
        .caps = caps,
        .attributes = attributes,
    };
    DWORD ret;

    desc.report_descriptor_len = sizeof(gamepad_desc);
    memcpy( desc.report_descriptor_buf, gamepad_desc, sizeof(gamepad_desc) );
    fill_context( desc.context, ARRAY_SIZE(desc.context) );

    hid_device_start( &desc, 1 );
    ret = WaitForSingleObject( stop_event, 5000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    hid_device_stop( &desc, 1 );

    return 0;
}

static void test_bus_driver(void)
{
#include "psh_hid_macros.h"
    const unsigned char report_desc[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_X),
            REPORT_SIZE(1, 8),
            REPORT_COUNT(1, 1),
            INPUT(1, Data|Var|Abs),
        END_COLLECTION,
    };
    C_ASSERT(sizeof(report_desc) < MAX_HID_DESCRIPTOR_LEN);
#include "pop_hid_macros.h"

    static const HID_DEVICE_ATTRIBUTES attributes =
    {
        .Size = sizeof(HID_DEVICE_ATTRIBUTES),
        .VendorID = LOWORD(EXPECT_VIDPID),
        .ProductID = HIWORD(EXPECT_VIDPID),
        .VersionNumber = 0x0100,
    };
    const HIDP_CAPS caps =
    {
        .Usage = HID_USAGE_GENERIC_JOYSTICK,
        .UsagePage = HID_USAGE_PAGE_GENERIC,
        .InputReportByteLength = 2,
        .NumberLinkCollectionNodes = 1,
        .NumberInputValueCaps = 1,
        .NumberInputDataIndices = 1,
    };
    struct hid_device_desc desc = { .caps = caps, .attributes = attributes, };

    WCHAR device_path[MAX_PATH];
    HANDLE control;
    DWORD ret;

    if (!bus_device_start()) goto done;

    control = CreateFileW( L"\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}", 0, 0,
                           NULL, OPEN_EXISTING, 0, NULL );
    ok( control != INVALID_HANDLE_VALUE, "CreateFile failed, error %lu\n", GetLastError() );
    CloseHandle( control );

    bus_device_stop();

    control = CreateFileW( L"\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}", 0, 0,
                           NULL, OPEN_EXISTING, 0, NULL );
    ok( control == INVALID_HANDLE_VALUE, "CreateFile succeeded\n" );

    bus_device_start();

    desc.report_descriptor_len = sizeof(report_desc);
    memcpy( desc.report_descriptor_buf, report_desc, sizeof(report_desc) );

    control = CreateFileW( L"\\\\?\\root#winetest#0#{deadbeef-29ef-4538-a5fd-b69573a362c0}", 0, 0,
                           NULL, OPEN_EXISTING, 0, NULL );
    ok( control != INVALID_HANDLE_VALUE, "CreateFile failed, error %lu\n", GetLastError() );
    ret = sync_ioctl( control, IOCTL_WINETEST_CREATE_DEVICE, &desc, sizeof(desc), NULL, 0, 5000 );
    ok( ret, "IOCTL_WINETEST_CREATE_DEVICE failed, last error %lu\n", GetLastError() );

    ret = WaitForSingleObject( device_added, 5000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    ret = WaitForSingleObject( device_added, 1000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );

    swprintf( device_path, MAX_PATH, L"\\\\?\\hid#vid_%04x&pid_%04x", LOWORD(EXPECT_VIDPID), HIWORD(EXPECT_VIDPID) );
    ret = find_hid_device_path( device_path );
    ok( ret, "Failed to find HID device matching %s\n", debugstr_w(device_path) );

    ret = sync_ioctl( control, IOCTL_WINETEST_REMOVE_DEVICE, &desc, sizeof(desc), NULL, 0, 5000 );
    ok( ret, "IOCTL_WINETEST_REMOVE_DEVICE failed, last error %lu\n", GetLastError() );
    CloseHandle( control );

    ret = WaitForSingleObject( device_removed, 5000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );
    ret = WaitForSingleObject( device_removed, 1000 );
    ok( !ret, "WaitForSingleObject returned %#lx\n", ret );

done:
    bus_device_stop();
}

static void test_hid_multiple_tlc(void)
{
#include "psh_hid_macros.h"
    const unsigned char report_desc[] =
    {
        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_JOYSTICK),
            COLLECTION(1, Physical),
                REPORT_ID(1, 1),

                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 1),
                USAGE_MAXIMUM(1, 2),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 8),
                INPUT(1, Data|Var|Abs),

                USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
                USAGE(1, HID_USAGE_GENERIC_X),
                USAGE(1, HID_USAGE_GENERIC_Y),
                REPORT_SIZE(1, 16),
                REPORT_COUNT(1, 2),
                LOGICAL_MINIMUM(2, -10),
                LOGICAL_MAXIMUM(2, +10),
                INPUT(1, Data|Var|Rel),
            END_COLLECTION,
        END_COLLECTION,

        USAGE_PAGE(1, HID_USAGE_PAGE_GENERIC),
        USAGE(1, HID_USAGE_GENERIC_GAMEPAD),
        COLLECTION(1, Application),
            USAGE(1, HID_USAGE_GENERIC_GAMEPAD),
            COLLECTION(1, Physical),
                REPORT_ID(1, 2),

                USAGE_PAGE(1, HID_USAGE_PAGE_BUTTON),
                USAGE_MINIMUM(1, 1),
                USAGE_MAXIMUM(1, 40),
                LOGICAL_MINIMUM(1, 0),
                LOGICAL_MAXIMUM(1, 1),
                REPORT_SIZE(1, 1),
                REPORT_COUNT(1, 40),
                INPUT(1, Data|Var|Abs),
            END_COLLECTION,
        END_COLLECTION,
    };
    C_ASSERT(sizeof(report_desc) < MAX_HID_DESCRIPTOR_LEN);
#include "pop_hid_macros.h"

    struct hid_device_desc desc =
    {
        .caps = { .InputReportByteLength = 7 },
        .attributes = default_attributes,
    };

    static const struct hidp_kdr expect_kdr_joystick =
    {
        .magic = "HidP KDR",
        .usage = 0x04,
        .usage_page = 0x01,
        .input_caps_count = 3,
        .input_caps_end = 3,
        .input_report_byte_length = 6,
        .output_caps_start = 3,
        .output_caps_end = 3,
        .feature_caps_start = 3,
        .feature_caps_end = 3,
        .caps_size = 312,
        .number_link_collection_nodes = 2,
    };
    static const struct hidp_kdr expect_kdr_gamepad =
    {
        .magic = "HidP KDR",
        .usage = 0x05,
        .usage_page = 0x01,
        .input_caps_count = 1,
        .input_caps_end = 1,
        .input_report_byte_length = 6,
        .output_caps_start = 1,
        .output_caps_end = 1,
        .feature_caps_start = 1,
        .feature_caps_end = 1,
        .caps_size = 104,
        .number_link_collection_nodes = 2,
    };
    static const struct hidp_kdr_caps expect_caps_joystick[] =
    {
        {
            .usage_page = 9, .report_id = 1, .bit_size = 1, .report_count = 8, .start_byte = 1, .total_bits = 8,
            .bit_field = 2, .end_byte = 2, .link_collection = 1, .link_usage_page = 0x01, .link_usage = 0x04, .flags = 0x1c,
            .usage_min = 1, .usage_max = 2, .data_index_max = 1,
        },
        {
            .usage_page = 1, .report_id = 1, .bit_size = 16, .report_count = 1, .start_byte = 4, .total_bits = 16,
            .bit_field = 6, .end_byte = 6, .link_collection = 1, .link_usage_page = 0x01, .link_usage = 0x04,
            .usage_min = 0x31, .usage_max = 0x31, .data_index_min = 2, .data_index_max = 2, .logical_min = -10, .logical_max = +10,
        },
        {
            .usage_page = 1, .report_id = 1, .bit_size = 16, .report_count = 1, .start_byte = 2, .total_bits = 16,
            .bit_field = 6, .end_byte = 4, .link_collection = 1, .link_usage_page = 0x01, .link_usage = 0x04,
            .usage_min = 0x30, .usage_max = 0x30, .data_index_min = 3, .data_index_max = 3, .logical_min = -10, .logical_max = +10,
        },
    };
    static const struct hidp_kdr_caps expect_caps_gamepad[] =
    {
        {
            .usage_page = 9, .report_id = 2, .bit_size = 1, .report_count = 40, .start_byte = 1, .total_bits = 40,
            .bit_field = 2, .end_byte = 6, .link_collection = 1, .link_usage_page = 0x01, .link_usage = 0x05, .flags = 0x1c,
            .usage_min = 0x01, .usage_max = 0x28, .data_index_max = 0x27,
        },
    };
    static const struct hidp_kdr_node expect_nodes_joystick[] =
    {
        {
            .usage = 0x04,
            .usage_page = 0x01,
            .number_of_children = 1,
            .first_child = 1,
            .collection_type = 0x1,
        },
        {
            .usage = 0x04,
            .usage_page = 0x01,
        },
    };
    static const struct hidp_kdr_node expect_nodes_gamepad[] =
    {
        {
            .usage = 0x05,
            .usage_page = 0x01,
            .number_of_children = 1,
            .first_child = 1,
            .collection_type = 1,
        },
        {
            .usage = 0x05,
            .usage_page = 0x01,
        },
    };
    struct hid_expect input[] =
    {
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_len = desc.caps.InputReportByteLength,
            .report_buf = {1,0x01,0x02,0x03,0x04,0x05,0x06},
            .ret_length = 6,
            .ret_status = STATUS_SUCCESS,
        },
        {
            .code = IOCTL_HID_READ_REPORT,
            .report_len = desc.caps.InputReportByteLength,
            .report_buf = {2,0x11,0x12,0x13,0x14,0x15,0x16},
            .ret_length = 6,
            .ret_status = STATUS_SUCCESS,
        },
    };

    WCHAR device_path[MAX_PATH];
    char report[16];
    ULONG value;
    HANDLE file;
    BOOL ret;

    desc.report_descriptor_len = sizeof(report_desc);
    memcpy( desc.report_descriptor_buf, report_desc, sizeof(report_desc) );
    fill_context( desc.context, ARRAY_SIZE(desc.context) );

    if (!hid_device_start( &desc, 2 )) goto done;

    swprintf( device_path, MAX_PATH, L"\\\\?\\hid#vid_%04x&pid_%04x&col01", desc.attributes.VendorID,
              desc.attributes.ProductID );
    ret = find_hid_device_path( device_path );
    ok( ret, "Failed to find HID device matching %s\n", debugstr_w( device_path ) );

    file = CreateFileW( device_path, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
    ok( file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError() );
    check_preparsed_data( file, &expect_kdr_joystick, ARRAY_SIZE(expect_caps_joystick), expect_caps_joystick,
                          ARRAY_SIZE(expect_nodes_joystick), expect_nodes_joystick );

    send_hid_input( file, input, sizeof(input) );

    memset( report, 0xcd, sizeof(report) );
    SetLastError( 0xdeadbeef );
    ret = ReadFile( file, report, desc.caps.InputReportByteLength, &value, NULL );
    ok( ret, "ReadFile failed, last error %lu\n", GetLastError() );
    ok( value == 6, "ReadFile returned %lx\n", value );
    ok( report[0] == 1, "unexpected report data\n" );
    ok( report[1] == 0x01, "unexpected report data\n" );

    CloseHandle( file );

    swprintf( device_path, MAX_PATH, L"\\\\?\\hid#vid_%04x&pid_%04x&col02", desc.attributes.VendorID,
              desc.attributes.ProductID );
    ret = find_hid_device_path( device_path );
    ok( ret, "Failed to find HID device matching %s\n", debugstr_w( device_path ) );

    file = CreateFileW( device_path, FILE_READ_ACCESS | FILE_WRITE_ACCESS,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );
    ok( file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError() );
    check_preparsed_data( file, &expect_kdr_gamepad, ARRAY_SIZE(expect_caps_gamepad), expect_caps_gamepad,
                          ARRAY_SIZE(expect_nodes_gamepad), expect_nodes_gamepad );

    send_hid_input( file, input, sizeof(input) );

    memset( report, 0xcd, sizeof(report) );
    SetLastError( 0xdeadbeef );
    ret = ReadFile( file, report, desc.caps.InputReportByteLength, &value, NULL );
    ok( ret, "ReadFile failed, last error %lu\n", GetLastError() );
    ok( value == 6, "ReadFile returned %lx\n", value );
    ok( report[0] == 2, "unexpected report data\n" );
    ok( report[1] == 0x11, "unexpected report data\n" );

    CloseHandle( file );

    swprintf( device_path, MAX_PATH, L"\\\\?\\hid#vid_%04x&pid_%04x&col03", desc.attributes.VendorID,
              desc.attributes.ProductID );
    ret = find_hid_device_path( device_path );
    ok( !ret, "Failed to find HID device matching %s\n", debugstr_w( device_path ) );

done:
    hid_device_stop( &desc, 2 );
}

START_TEST( hid )
{
    dinput_test_init();

    test_bus_driver();

    if (!bus_device_start()) goto done;
    test_hidp_kdr();
    test_hid_driver( 0, FALSE );
    test_hid_driver( 1, FALSE );
    test_hid_driver( 0, TRUE );
    test_hid_driver( 1, TRUE );
    test_hid_multiple_tlc();

done:
    bus_device_stop();
    dinput_test_exit();
}
