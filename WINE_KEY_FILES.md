# Wine - Arquivos-Chave e Diretórios Críticos

## ARQUIVOS PRINCIPAIS DO REPOSITÓRIO

### Raiz:
- `/home/user/wineX/configure.ac` - Definição autoconf (processado com m4)
- `/home/user/wineX/configure` - Script de configuração gerado
- `/home/user/wineX/VERSION` - Versão do Wine (10.19)

---

## ESTRUTURA DE DLLs CRÍTICAS

### 1. RUNTIME BASE: ntdll.dll
```
/home/user/wineX/dlls/ntdll/
├── loader.c (175KB)          # PE module loading, imports handling
├── exception.c                # SEH (Structured Exception Handling)
├── threadpool.c (108KB)       # Thread pool implementation
├── sync.c (47KB)              # Synchronization primitives
├── signal_*.c (múltiplos)     # Signal handlers per architecture
├── relay.c (46KB)             # Function call relay/debugging
├── rtl.c (53KB)               # Runtime Library functions
├── string.c (57KB)            # String operations
├── heap.c (99KB)              # Heap management
├── sec.c (58KB)               # Security functions
├── unwind.c (82KB)            # Exception unwinding
├── env.c (26KB)               # Environment variables
├── version.c (27KB)           # Version info
├── unix/                      # Unix-specific implementations
└── ntsyscalls.h               # Syscall definitions
```

**Propósito**: API base NT, loader de executáveis, exceções, threading

---

### 2. KERNEL API: kernel32.dll
```
/home/user/wineX/dlls/kernel32/
├── kernel32.spec (88KB)       # 600+ exports para Win95+ compatibility
├── process.c (31KB)           # CreateProcess, Process management
├── file.c (17KB)              # File operations
├── thread.c (5KB)             # Thread operations
├── console.c (4KB)            # Console I/O
├── sync.c (28KB)              # WaitForSingleObject, etc
├── heap.c (14KB)              # HeapAlloc, HeapFree
├── atom.c (12KB)              # Atom table
├── module.c (11KB)            # Module loading (legacy)
├── virtual.c (7KB)            # VirtualAlloc/VirtualFree
├── resource.c (42KB)          # Resource management
├── locale.c (18KB)            # Locale/language functions
├── path.c (22KB)              # Path manipulation
└── profile.c (68KB)           # INI file functions
```

**Propósito**: APIs de processo, arquivo, sincronização, legacy Win32

---

### 3. GRÁFICOS: wined3d.dll (Motor 3D Principal)
```
/home/user/wineX/dlls/wined3d/
├── adapter_gl.c (246KB)       # OpenGL adapter implementation
├── adapter_vk.c (103KB)       # Vulkan adapter implementation
├── context_gl.c (195KB)       # OpenGL context management
├── context_vk.c (179KB)       # Vulkan context management
├── device.c (210KB)           # D3D device implementation
├── shader.c (131KB)           # Shader management
├── glsl_shader.c (542KB!)     # GLSL compiler (MAIOR ARQUIVO)
├── shader_sm4.c (96KB)        # Shader Model 4 compilation
├── shader_spirv.c (52KB)      # SPIR-V compilation
├── stateblock.c (158KB)       # Pipeline state management
├── cs.c (167KB)               # Compute shader support
├── ffp_gl.c (71KB)            # Fixed Function Pipeline OpenGL
├── buffer.c (51KB)            # Buffer objects
├── query.c (71KB)             # Query objects (occlusion, timing)
├── sampler.c (13KB)           # Sampler management
├── palette.c                  # Color palettes
├── decoder.c (57KB)           # Video decoding
└── directx.c (167KB)          # DirectX management
```

**Propósito**: Backend 3D para Direct3D, suporta OpenGL e Vulkan

---

### 4. INTERFACE: user32.dll (Janelas e Mensagens)
```
/home/user/wineX/dlls/user32/
├── user32.spec                # Window/message exports
├── message.c (39KB)           # Message queues and dispatch
├── dialog.c (65KB)            # Dialog boxes
├── mdi.c (61KB)               # MDI (Multiple Document Interface)
├── clipboard.c (41KB)         # Clipboard operations
├── hook.c (19KB)              # Window hooks
├── menu.c (29KB)              # Menu handling
├── msgbox.c (18KB)            # Message boxes
├── edit.c (140KB)             # Edit control
├── listbox.c (106KB)          # List box control
├── combo.c (64KB)             # Combo box control
├── button.c (33KB)            # Button control
├── static.c (38KB)            # Static control
├── scroll.c (14KB)            # Scroll bars
├── cursoricon.c (84KB)        # Cursor/icon handling
├── input.c (26KB)             # Input processing
├── class.c (19KB)             # Window class management
└── text.c (51KB)              # Text rendering
```

**Propósito**: Subsistema completo de janelas, controles e mensagens

---

### 5. REDE: ws2_32.dll (Winsock 2)
```
/home/user/wineX/dlls/ws2_32/
├── Makefile.in
│   ├── UNIXLIB = ws2_32.so    # Unix library backend
│   └── UNIX_LIBS = $(PTHREAD_LIBS)
├── socket.c (141KB)           # Socket implementation
├── protocol.c (67KB)          # Protocol handling
├── async.c (14KB)             # Asynchronous operations
├── inaddr.c (3KB)             # Address handling
├── unixlib.c (37KB)           # Unix bridge (POSIX sockets)
├── ws2_32.spec                # Winsock 2 exports
└── ws2_32_private.h           # Internal headers
```

**Propósito**: Windows Sockets 2 implementation, mapeia para POSIX sockets

---

### 6. DRIVERS GRÁFICOS

#### Winex11 (X11/Wayland):
```
/home/user/wineX/dlls/winex11.drv/
├── window.c                   # X11 window management
├── graphics.c                 # Graphics operations
├── mouse.c                    # Mouse handling
├── keyboard.c                 # Keyboard input
├── clipboard.c                # X11 clipboard bridge
└── xrender.c                  # XRender extension
```

#### Winewayland (Wayland modern):
```
/home/user/wineX/dlls/winewayland.drv/
├── window.c                   # Wayland surface management
├── pointer.c                  # Pointer/mouse
├── keyboard.c                 # Keyboard
└── vulkan.c                   # Vulkan integration
```

---

### 7. MEDIA FOUNDATION (Moderno)
```
/home/user/wineX/dlls/mf/
├── main.c                     # Core implementation
├── media_engine.c             # Media Engine
└── (contém stubs para muitas funcionalidades)

/home/user/wineX/dlls/mfplat/  # Media Foundation Platform
/home/user/wineX/dlls/mfreadwrite/  # MF Read/Write
/home/user/wineX/dlls/mfplay/  # MF Playback
```

---

### 8. DIRECT3D (Múltiplas Versões)

#### D3D12 (Mais Moderno):
```
/home/user/wineX/dlls/d3d12/
├── d3d12_main.c               # D3D12 entry point
├── IMPORTLIB = d3d12
├── IMPORTS = dxgi dxguid wined3d gdi32 user32 uuid win32u
└── Usa VKD3D para Vulkan mapping
```

#### D3D11:
```
/home/user/wineX/dlls/d3d11/
├── d3d11_main.c
├── device.c
├── context.c
└── Delegado a wined3d para rendering
```

#### D3D9:
```
/home/user/wineX/dlls/d3d9/
├── d3d9_main.c
├── device.c
├── IMPORTLIB = d3d9
└── Legacy Direct3D 9 support
```

---

### 9. DXGI (Graphics Infrastructure)
```
/home/user/wineX/dlls/dxgi/
├── dxgi_main.c                # DXGI implementation
├── device.c                   # DXGI device
├── swapchain.c                # Swap chain management
└── output.c                   # Display output
```

---

### 10. SERVIÇOS MODERNOS

#### WinRT (40+ DLLs):
```
/home/user/wineX/dlls/windows.*.dll/
├── windows.ui.xaml/           # XAML framework stubs
├── windows.storage/           # File system access
├── windows.networking/        # Network APIs
├── windows.media/             # Media playback
├── windows.devices.*/         # Device APIs
└── windows.security.*/        # Security APIs
```

#### Criptografia Moderna:
```
/home/user/wineX/dlls/bcrypt/
├── Makefile.in:
│   ├── UNIXLIB = bcrypt.so
│   └── UNIX_LIBS = crypto library
├── main.c
└── unixlib.c                  # Maps to OpenSSL/GnuTLS
```

---

## WINE SERVER (Componentes Críticos)

```
/home/user/wineX/server/
├── main.c                     # Entry point, event loop
├── request.c (29KB)           # Request dispatcher
├── process.c (70KB)           # Process management
├── thread.c (80KB)            # Thread management
├── fd.c (98KB)                # File descriptor management
├── window.c (110KB)           # Window management
├── sock.c (139KB)             # Socket management
├── named_pipe.c (61KB)        # Named pipe support
├── handle.c (30KB)            # Handle table management
├── object.c (27KB)            # Object management
├── mapping.c (64KB)           # Memory mapping
├── registry.c (76KB)          # Registry emulation
├── token.c (59KB)             # Security tokens
├── console.c (60KB)           # Console emulation
├── queue.c (139KB)            # Message queue
├── protocol.def (147KB)       # Protocol definition
├── request_handlers.h (134KB) # Auto-generated handlers
├── request_trace.h (166KB)    # Debugging trace
├── change.c (34KB)            # File change notification
├── device.c (38KB)            # Device management
└── directory.c (29KB)         # Directory operations
```

**Propósito**: Gerenciador de recursos centralizado, sincronização global

---

## INCLUDE (Headers Internos)

### Headers Críticos do Wine:
```
/home/user/wineX/include/wine/
├── server.h                   # Wine Server interface
├── server_protocol.h (160KB)  # Server protocol definitions
├── exception.h (10KB)         # Exception handling
├── debug.h (20KB)             # Debug macros
├── gdi_driver.h (19KB)        # GDI driver interface
├── opengl_driver.h (10KB)     # OpenGL driver interface
├── vulkan_driver.h (11KB)     # Vulkan driver interface
├── unixlib.h (8KB)            # Unix library bridge
├── condrv.h (9KB)             # Console driver
├── afd.h (14KB)               # Async File Driver
├── nsi.h (12KB)               # Network Stack Interface
└── list.h                     # Intrusive list implementation
```

---

## BIBLIOTECAS EXTERNAS CRÍTICAS

```
/home/user/wineX/libs/
├── vkd3d/                     # D3D12 → Vulkan translation
├── wbemuuid/                  # WMI UUID definitions
├── mfuuid/                    # Media Foundation UUIDs
├── dxguid/                    # DirectX UUIDs
├── strmiids/                  # Stream media IIDs
├── faudio/                    # XAudio2 implementation
├── png/                       # PNG codec
├── jpeg/                      # JPEG codec
├── tiff/                      # TIFF codec
├── lcms2/                     # Color management
├── xslt/ + xml2/              # XML processing
├── zlib/                      # Compression
├── ldap/                      # LDAP support
└── capstone/                  # Disassembler
```

---

## FERRAMENTAS DE BUILD

```
/home/user/wineX/tools/
├── winebuild/                 # DLL stub generator
├── widl/                      # IDL compiler
├── wrc/                       # Resource compiler
├── wmc/                       # Message compiler
├── winegcc/                   # GCC wrapper
├── winapi/                    # API definitions
├── winemaker/                 # Build automation
└── winedump/                  # PE executable analyzer
```

---

## PROGRAMAS EXECUTÁVEIS

```
/home/user/wineX/programs/
├── explorer/                  # File explorer
├── notepad/                   # Text editor
├── wordpad/                   # Rich text editor
├── regedit/                   # Registry editor
├── cmd/                       # Command interpreter
├── powershell/                # PowerShell
├── taskmgr/                   # Task manager
├── control/                   # Control panel
├── dxdiag/                    # DirectX diagnostics
└── (110+ outros)              # Vários utilitários
```

---

## FLUXO DE IMPLEMENTAÇÃO DE UMA API

### Exemplo: CreateProcessA()

1. **kernel32.spec** define export:
   ```
   140 stdcall CreateProcessA(...)
   ```

2. **kernel32/process.c** implementa:
   ```c
   BOOL WINAPI CreateProcessA(...)
   {
       // Valida argumentos
       // Abre arquivo executável
       // Cria estrutura de processo
       // Chama Wine Server
       // Retorna ao chamador
   }
   ```

3. **Wine Server** (server/process.c) gerencia:
   - Estrutura do processo
   - Lista de threads
   - Handles
   - Memória virtual

4. **ntdll** fornece primitivos baixo-nível usados por kernel32

---

## COMO MELHORAR COMPATIBILIDADE

### Para Gráficos:
- Arquivo: `/home/user/wineX/dlls/wined3d/shader.c`
- Melhorar compilação HLSL → GLSL/SPIR-V
- Referência: `glsl_shader.c` (542KB)

### Para Rede:
- Arquivo: `/home/user/wineX/dlls/ws2_32/socket.c`
- Adicionar syscalls faltantes
- Referência: `server/sock.c` (139KB)

### Para UI Moderna:
- Arquivo: `/home/user/wineX/dlls/windows.ui.xaml/`
- Implementar XAML parsing
- Referência: `user32.dll` para widgets base

### Para Media:
- Arquivo: `/home/user/wineX/dlls/mf/`
- Completar Media Foundation
- Referência: GStreamer backend em `winegstreamer.drv`

---

## RESUMO DOS TAMANHOS DE ARQUIVO (Em Ordem)

Maiores arquivos por funcionalidade:

| Arquivo | Tamanho | Funcionalidade |
|---------|---------|----------------|
| glsl_shader.c | 542KB | Compilador GLSL |
| loader.c | 175KB | PE loader ntdll |
| server/protocol.def | 147KB | Protocolo servidor |
| request_handlers.h | 134KB | Server handlers |
| request_trace.h | 166KB | Debug trace |
| device.c (wined3d) | 210KB | D3D device |
| glsl_shader.c | 542KB | Shader compiler |
| shell32 | 200KB+ | Shell COM |
| edit.c | 140KB | Edit control |
| listbox.c | 106KB | List control |
| threadpool.c | 108KB | Thread pool ntdll |

