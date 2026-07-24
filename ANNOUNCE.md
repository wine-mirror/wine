The Wine development release 11.14 is now available.

What's new in this release:
  - 7.1 format conversions in DirectSound.
  - Support for AES-GMAC algorithm in BCrypt.
  - New WoW64 mode supported on FreeBSD.
  - Icons in Start Menu.
  - Various bug fixes.

The source is available at <https://dl.winehq.org/wine/source/11.x/wine-11.14.tar.xz>

Binary packages for various distributions will be available
from the respective [download sites][1].

You will find documentation [here][2].

Wine is available thanks to the work of many people.
See the file [AUTHORS][3] for the complete list.

[1]: https://gitlab.winehq.org/wine/wine/-/wikis/Download
[2]: https://gitlab.winehq.org/wine/wine/-/wikis/Documentation
[3]: https://gitlab.winehq.org/wine/wine/-/raw/wine-11.14/AUTHORS

----------------------------------------------------------------

### Bugs fixed in 11.14 (total 21):

 - #21811  3impact tech demo collection crashes on first demo
 - #34665  Adobe Reader 11 PDF portfolio search crashes
 - #38746  Adobe Reader 9.0 installer fails
 - #43775  Heroes of the Storm crash on start with d3d11
 - #49868  Hedgewars 1.0.0 crashes when a game against the computer is started
 - #52833  Wine's wscript.exe and cscript.exe don't support UTF-16 LE with BOM script files
 - #53574  MSYS2 "pacman -Sy" fails key lookup
 - #56317  Falcon BMS 4.37 crashes on start (needs D3DX11CreateTextureFromFileA implementation)
 - #56501  nProtect Anti-Virus/Spyware 4.0 'tkpl2k64.sys' crashes on unimplemented function 'fltmgr.sys.FltCreateCommunicationPort'
 - #56580  Xdefiant Beta needs unimplemented function pdh.dll.PdhGetFormattedCounterArrayA
 - #56745  Amazing Adventures 2: mouse pointer flashes all over the screen
 - #57801  Marble Marcher Community Edition: Startup error "Compute shader compilation error."
 - #58826  Winhq 10.17 seems to loose window focus??
 - #59378  Black rectangle box in foreground blocking application window on TaxAct 2025
 - #59456  TreeView: TVN_ITEMCHANGING not sent on checkbox change
 - #59816  cmd: inbuilt 'type' fails on expanding *.txt
 - #59913  msvcrt: _aligned_offset_realloc alignment validation failure crashes PCSX2 nightly
 - #59964  Age of Empires I and II both show blank screen in introduction cinematics
 - #60000  msvcrt: fopen("wb, ccs=UTF-8") prepends a BOM unlike Windows
 - #60021  Wine loader segfaults converting Unix executable path when no DOS drive maps it
 - #60039  Microsoft Office Word 2016 hangs after changing a Design style

### Changes since 11.13:
```
Aidan Racaniello (4):
      uxtheme: Add GetImmersiveUserColorSetPreference stub.
      uxtheme: Add GetImmersiveColorTypeFromName stub.
      uxtheme: Add GetImmersiveColorFromColorSetEx semi-stub.
      uxtheme: Add GetImmersiveColorNamedTypeByIndex stub.

Alexander Shaikhulin (2):
      ntdll: Support new wow64 mode on FreeBSD.
      ntdll: Work around AMD SYSRET SS descriptor behavior on FreeBSD.

Alexandre Julliard (31):
      setupapi: Delay file copy if truncating fails.
      win32u: Add missing newline in a trace.
      server: Make the add_queue() operation optional.
      server: Make the satisfied() operation optional.
      server: Make the signal() operation optional.
      server: Make the get_fd() operation optional.
      server: Make the get_sync() operation optional.
      server: Make the map_access() operation optional.
      server: Make the get_sd() operation optional.
      server: Make the set_sd() operation optional.
      server: Make the get_full_name() operation optional.
      server: Make the lookup_name() operation optional.
      server: Make the link_name() operation optional.
      server: Make the unlink_name() operation optional.
      server: Make the open_file() operation optional.
      server: Make the get_kernel_obj_list() operation optional.
      server: Make the close_handle() operation optional.
      server: Make the destroy() operation optional.
      server: Use designated initializers for object operations.
      server: Make the get_poll_events() fd operation optional.
      server: Make the poll_event() fd operation optional.
      server: Make the read() fd operation optional.
      server: Make the write() fd operation optional.
      server: Make the flush() fd operation optional.
      server: Make the get_file_info() fd operation optional.
      server: Make the get_volume_info() fd operation optional.
      server: Make the ioctl() fd operation optional.
      server: Make the cancel_async() fd operation optional.
      server: Make the queue_async() fd operation optional.
      server: Make the reselect_async() fd operation optional.
      server: Use designated initializers for fd operations.

Alistair Leslie-Hughes (1):
      ntoskrnl.exe: Implement IoThreadToProcess.

Andrea Faulds (1):
      explorer: Display icons next to Start Menu items.

Anton Baskanov (1):
      dsound: Move the get functions to mixer.c.

Arkadiusz Hiler (1):
      dsound: Add mono and stereo to 7.1 conversions.

Barath Kannan (2):
      cmd/tests: Add tests for the expansion of wildcards in the type command's arguments.
      cmd: Add wildcard expansion for the type command's arguments.

Bernhard Übelacker (4):
      wow64: Handle PS_ATTRIBUTE_GROUP_AFFINITY in ps_attributes_32to64.
      taskmgr: Change the default push button.
      server: Clear connection failure events in IOCTL_AFD_WINE_CONNECT.
      kernel32/tests: Fix test_GetFinalPathNameByHandleW.

Brandow Lucas (1):
      msxml3: Handle NULL value for the SelectionNamespaces property.

Brendan McGrath (2):
      strmbase: Introduce callback for ReceiveCanBlock.
      winegstreamer: Preserve sign of stride from 2D buffer.

Brendan Shanks (1):
      ntdll: Hard code the host address space limit on macOS.

Connor McAdams (8):
      include/ddk: Add some missing enum/structure definitions.
      ntoskrnl/tests: Add tests to highlight PDO specific behavior.
      ntoskrnl/tests: Add tests for AttachedTo field in DEVOBJ_EXTENSION.
      ntoskrnl: Set AttachedTo field in DEVOBJ_EXTENSION when attaching/detaching devices.
      ntoskrnl: Set DO_BUS_ENUMERATED_DEVICE flag for PDO DEVICE_OBJECTs.
      ntoskrnl: Add checks for DEVICE_OBJECTs passed into various PnP functions.
      ntoskrnl: Cache device instance ID for PDOs.
      ntoskrnl: Use cached device instance ID for DevicePropertyEnumeratorName.

Conor McCarthy (9):
      wintypes/tests: Test DataWriter.
      wintypes: Rename struct data_writer to data_writer_factory.
      wintypes: Add DataWriter stubs.
      wintypes: Implement IDataWriter::WriteBytes().
      wintypes: Implement IDataWriter::DetachBuffer().
      wintypes: Add IDataWriterFactory stubs.
      wintypes: Implement IDataWriterFactory::CreateDataWriter().
      wintypes: Implement IDataWriter::StoreAsync().
      wintypes: Implement IDataWriter::DetachStream().

Daniel Lehman (1):
      shell32/tests: Remove stray test file after directory rename test.

Dmitry Timoshkov (1):
      crypt32: Add support for CERT_CLOSE_STORE_FORCE_FLAG.

Elizabeth Figura (13):
      qwave: No longer prefer native.
      rstrtmgr: No longer prefer native.
      svrapi: No longer prefer native.
      uianimation: No longer prefer native.
      uiautomationcore: No longer prefer native.
      websocket: No longer prefer native.
      winnls32: No longer prefer native.
      wldp: No longer prefer native.
      wpc: No longer prefer native.
      wined3d: Do not disable extendedDynamicState3PolygonMode (validation).
      slc: No longer prefer native.
      ncrypt: No longer prefer native.
      ntdll: Handle Unix paths with no corresponding drive in get_full_path().

Eric Pouech (3):
      dbghelp/dwarf: Implement DW_OP_shra by shifting signed quantities.
      dbghelp/dwarf: Follow Dwarf specs for (un)signed arithmetic operations.
      dbghelp/dwarf: Report errors when attempting to divide by zero.

Esme Povirk (2):
      mscoree: Use wide string literals.
      mscoree: Add libmono dll name for arm64.

Georg Lehmann (1):
      winevulkan: Update to VK spec version 1.4.357.

Hans Leidekker (4):
      bcrypt: Support BCRYPT_SIGNATURE_LENGTH for RSA.
      bcrypt: Fix accepted key size for 25519 curve.
      bcrypt: Support BCRYPT_AES_GMAC_ALGORITHM.
      bcrypt: Support BCRYPT_MESSAGE_BLOCK_LENGTH.

Haoyang Chen (1):
      crypt32: Fix CryptRegisterOIDInfo size validation for CRYPT_OID_INFO_HAS_EXTRA_FIELDS compatibility.

Henri Verbeet (2):
      d3dcompiler: Store a D3D11_SHADER_DESC structure in struct d3dcompiler_shader_reflection.
      d3dcompiler: Adopt vkd3d's STAT parser.

Hieu Le Minh (3):
      dxcore: Add support for DedicatedAdapterMemory property.
      dxcore: Add support for SharedSystemMemory property.
      dxcore: Return stub value for DedicatedSystemMemory property.

Jacob Czekalla (2):
      msxml3/tests: Add more tests for setNamedItem().
      msxml3: Return new node from setNamedItem().

Jiangyi Chen (2):
      symcrypt: Avoid SSE intrinsic that is missing in old gcc versions.
      winewayland.drv: Avoid ABBA deadlocks between win_data_mutex and user_mutex.

Kang Kang (1):
      dxgi: Return the first adapter from EnumWarpAdapter().

Kyle Yang (1):
      wininet: Fix out-of-bounds read in InternetCrackUrlW for empty file paths.

Lakulish Antani (1):
      dsound: Add quad and 5.1 to 7.1 conversions.

Lokesh Poovaragan (2):
      wscript: Support UTF-16 LE encoded script files.
      wscript/tests: Add encoding tests for UTF-16 script files.

Louis Lenders (3):
      wminet_utils: Add GetNames.
      wminet_utils: Add BeginMethodEnumeration.
      cldapi: Add stub for CfGetPlaceholderInfo.

Marcus Meissner (1):
      ntdll/tests: GetCurrentDirectoryW gets size in characters not bytes.

Matteo Bruni (1):
      winex11: Report right shift as an extended key.

Michael Brabec (1):
      avifil32: Fix ICM_COMPRESS_FRAMES_INFO not being sent without VIDCF_COMPRESSFRAMES.

Nikolay Sivov (24):
      msxml3/tests: Add some tests for DTD node children.
      msxml3/dom: Add notations as children of a DTD.
      msxml3: Add a stub for IXMLDOMNotation node.
      oleaut32/tests: Add some tests for aggregation with CreateStdDispatch().
      oleaut32: Add aggregation support for CreateStdDispatch().
      msxml3: Add entities as children of a DTD.
      msxml3: Add a stub for IXMLDOMEntity node.
      msxml3/notation: Implement nodeTypeString() property.
      msxml3/entity: Implement nodeTypeString() property.
      msxml3/notation: Implement text() property.
      msxml3/tests: Add more tests for notation nodes.
      msxml3: Return name properties for IXMLDOMNotation.
      msxml3/doctype: Implement hasChildNodes() method.
      msxml3/tests: Move some notation tests.
      msxml3: Add a helper to return attribute count.
      msxml3: Create attributes for IXMLDOMNotation nodes.
      msxml3/tests: Add more tests for entity nodes.
      msxml3: Add entities as DTD children.
      msxml3: Implement name methods for IXMLDOMEntity.
      msxml3: Correct copy-pasted variable names.
      msxml3: Set entity nodes text property.
      msxml3: Add attributes for entity nodes.
      msxml3/doctype: Implement entities() property.
      msxml3/doctype: Implement notations() property.

Pan Hui (1):
      winewayland: Fix restoring window after being maximized on the secondary monitor.

Paul Gofman (5):
      ntdll/tests: Add tests for skipping directory reopen by RtlSetCurrentDirectory_U().
      ntdll: Do not reopen directory file when setting the same directory path.
      winex11.drv: Flush display in acquire_selection().
      shell32: Add AppDataDocuments known folder.
      ntdll: Add BirthVolumeId to loaded dll file id.

Pavel Cheloveckov (1):
      mscoree: Store the sku attribute from supportedRuntime config elements.

Piotr Caban (17):
      comsvcs/tests: Remove non-needed IHolder pointer checks.
      comsvcs/tests: Test creating multiple Dispenser Managers.
      comsvcs: Don't use IDispenserDriver after closing holder.
      comsvcs/tests: Add more Dispenser Manager tests.
      comsvcs: Add partial implementation of resources pooling.
      comsvcs: Implement resource purging thread in Dispenser Manager.
      comsvcs: Remove COM initilization hack from Dispenser Manager.
      comsvcs: Create one Dispenser Manager in a process.
      comsvcs: Exit resource lookup loop when perfect match is found in AllocResource.
      comsvcs: Exit resource purging loop on first resource in use.
      comsvcs: Remove the FIXME message since pool purging is implemented.
      msvcrt/tests: Test BOM handling in files opened in binary mode.
      msvcrt: Don't append/read BOM in fopen binary mode.
      msvcrt/tests: Cleanup _aligned_offset_realloc tests.
      msvcrt/tests: Add more _aligned_offset_realloc tests.
      msvcrt: Allow changing alignment and offset in _aligned_offset_realloc.
      server: Set kernel notification events initial state.

Rose Hellsing (1):
      ntdll: Return empty value instead of failing in NtQueryVolumeInformation.

Shaun Ren (1):
      windows.ui: Stub IUISettings2::add_TextScaleFactorChanged().

Stian Low (1):
      winhlp32: Implement MACRO_PopupContext.

Tim Clem (1):
      winemac.drv: Detect display changes with CGDisplayRegisterReconfigurationCallback.

Yue Zhang (3):
      setupapi: Implement SPDRP_ENUMERATOR_NAME.
      gdiplus: Fix potential integer overflow in GdipDrawImagePointsRect.
      gdiplus: Fix missing newline detection in gdip_format_string.

Zhengyong Chen (3):
      pdh: Implement PdhGetFormattedCounterArrayA/W.
      mshtml: Fire property change notification when document title is set.
      ieframe: Fire TitleChange event on document title change.

Zhiyi Zhang (7):
      ntdll/tests: Add ALPC Port object tests.
      ntdll/tests: Add NtAlpcCreatePort() tests.
      ntdll: Implement NtAlpcCreatePort().
      dxgi/tests: Add IDXGISwapChain::ResizeBuffers() flag tests.
      dxgi: Support DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT in dxgi_swapchain_flags_from_wined3d().
      dxgi: Support modifying swapchain flags in d3d11_swapchain_ResizeBuffers().
      dxgi: Support modifying swapchain flags in d3d12_swapchain_resize_buffers().

Zhou Zhang (1):
      user32: Release drop target at the end of drag_drop_drop and drag_drop_leave.
```
