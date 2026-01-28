# Arquitetura do Wine - Análise Completa

## 1. ESTRUTURA PRINCIPAL DO REPOSITÓRIO

O Wine possui 723 DLLs distribuídas em uma arquitetura modular bem definida:

### Diretórios Principais:
- `/dlls/` (725 diretórios): Implementações das DLLs Windows
- `/server/` (Wine Server): Gerenciador de recursos central
- `/loader/`: Carregador executável (preloader)
- `/tools/`: Ferramentas de build (winebuild, widl, wrc, winegcc)
- `/include/`: Headers públicos e internos
- `/libs/`: Bibliotecas externas integradas
- `/programs/`: Programas/utilitários executáveis (116 programas)
- `/nls/`: Dados de linguagem/localização
- `/fonts/`: Fontes de renderização

---

## 2. SISTEMA DE BUILD (GNU Autotools)

### Arquivos de Configuração:
- `configure.ac`: Arquivo fonte do autoconf (processado com m4)
- `configure`: Script gerado (versão Wine 10.19)
- `Makefile.in`: Modelos de Makefile para cada componente

### Configuração de Compilação:
```bash
./configure [opções]
make
make install
```

### Opções Principais:
- `--enable-archs`: Suporta múltiplas arquiteturas (i386, x86_64, arm, aarch64)
- `--enable-win64`: Modo Win64 puro
- `--disable-win16`: Desabilita suporte Win16 legado
- `--enable-werror`: Trata avisos como erros
- `--with-mingw`: Usa cross-compiler MinGW
- `--with-wine64`: Suporta builds Wow64 (32-bit em 64-bit)

### Dependências Opcionais:
- Audio: ALSA, CoreAudio, PulseAudio, OSS
- Criptografia: GnuTLS, GSSAPI, Kerberos
- Gráficos: OpenGL, Vulkan, SDL, Wayland, X11 extensions
- Codificação: FFmpeg, GStreamer
- Impressão: CUPS, localspl
- Rede: libpcap, netapi (Samba)
- Hardware: libusb, udev, v4l2, SANE (scanners)

---

## 3. ARQUITETURA DE DLLs

### Padrão de Estrutura de uma DLL:

Cada DLL em `/dlls/{name}/` contém:
```
{name}/
├── Makefile.in          # Configuração de build
├── {name}.spec          # Definição de exports (formato Wine)
├── {name}.c/.cpp        # Implementação principal
├── {name}_private.h     # Headers privados
├── unixlib.c           # Bridge para funcionalidades Unix
├── tests/              # Testes de regressão
└── version.rc          # Informações de versão
```

### Arquivo .spec (Definição de Exports):

Formato especial Wine que mapeia funções Windows para implementações:
```
# Functions exported by the DLL
10 stdcall -i386 Function1(str ptr) implementation_name
20 stdcall Function2(long long) another_impl
30 pascal Function3() old_style_func
```

Tipos de chamada: stdcall, cdecl, fastcall, pascal, varargs
Flags: -norelay, -noname, -private, -i386 (x86 only)

### Exemplo: kernel32.spec (88KB!)
- Ordinárias Win95-compatíveis (Win16/Win32 legacy)
- Funções de gerenciamento de processos
- Funções de sincronização
- APIs de heap/memória

---

## 4. DLLs CRÍTICAS (Core APIs)

### Base Runtime:
- **ntdll.dll** (176KB loader.c): NT API base, exceções, RTL
- **kernel32.dll**: Processos, threads, memoria, sincronização
- **kernelbase.dll**: Versão moderna de kernel32
- **msvcrt.dll**: Runtime C (implementação própria)

### Sistema de Suporte:
- **advapi32.dll**: APIs de segurança, registry
- **ole32.dll**: COM/OLE implementation
- **oleaut32.dll**: OLE automation
- **rpcrt4.dll**: Remote Procedure Call
- **ws2_32.dll**: Winsock 2 (networking)

### Subsistema de Janelas:
- **user32.dll** (1.4MB): Janelas, mensagens, dialogs
- **gdi32.dll**: Gráficos (Device Independent)
- **win32u.dll**: Low-level user/gdi functions
- **comctl32.dll**: Controles comuns
- **comdlg32.dll**: Diálogos comuns

---

## 5. DLLs DE FUNCIONALIDADES MODERNAS

### Gráficos e Renderização (500+ MB):
- **wined3d.dll**: Motor 3D principal (OpenGL + Vulkan backends)
  - adapter_gl.c (246KB): Implementação OpenGL
  - adapter_vk.c (103KB): Implementação Vulkan
  - context_gl.c (195KB), context_vk.c (179KB)
  - glsl_shader.c (542KB): Compilador GLSL
  - shader_sm4.c: Compilação de shaders HLSL
  
- **d3d*.dll**: Direct3D (múltiplas versões)
  - d3d9, d3d10, d3d11, d3d12
  - d3dcompiler_* (versões 33-47): Compiladores HLSL
  - d3dx9_*, d3dx10_*, d3dx11_*: Bibliotecas auxiliares
  
- **dxgi.dll**: DirectX Graphics Infrastructure
- **dwrite.dll**: DirectWrite (typography)
- **dwmapi.dll**: Desktop Window Manager

### Áudio/Vídeo:
- **mmdevapi.dll**: Multimedia Device API
- **dsound.dll**: DirectSound
- **mf*.dll**: Media Foundation (8+ DLLs)
- **quartz.dll**: DirectShow
- **winegstreamer.drv**: Backend GStreamer
- **fmod/faudio**: Áudio alternativo

### Rede (Network Stack):
- **nsiproxy.sys**: Network socket interface proxy
- **netio.sys**: Network I/O driver
- **iphlpapi.dll**: IP Helper
- **dnsapi.dll**: DNS resolution
- **httpapi.dll**: HTTP Server API
- **webservices.dll**: WS-* SOAP
- **websocket.dll**: WebSocket support

### Segurança/Criptografia:
- **bcrypt.dll**: Cryptography API (modern)
- **crypt32.dll**: Legacy crypto
- **ncrypt.dll**: Crypto operations
- **schannel.dll**: SSL/TLS
- **secur32.dll**: SSPI security provider

### Windows Moderno (WinRT):
- **windows.*.dll** (40+ DLLs): APIs WinRT
  - windows.ui.xaml
  - windows.storage
  - windows.networking
  - windows.media
  - windows.devices.*
  - windows.security.*

### Runtime Moderno:
- **vcruntime140.dll**, **msvcp140.dll**: Visual C++ 2015+ Runtime
- **concrt140.dll**: Concurrency Runtime
- **vccorlib140.dll**: C++ standard library

---

## 6. SERVIDOR WINE (Wine Server)

### Localização: `/server/`

Componentes principais:
- **main.c**: Entry point do servidor
- **request.c**: Despachador de requisições
- **protocol.def**: Definição de protocolo cliente-servidor
- **request_handlers.h**: Handlers de requisições (gerado)
- **process.c**: Gerenciamento de processos
- **thread.c**: Gerenciamento de threads
- **fd.c**: File descriptor management (98KB)

### Funcionalidades:
- Gerenciamento centralizado de recursos (handles, eventos, mutexes)
- Sincronização inter-processo
- Gerenciamento de windows e mensagens
- File descriptors, pipes, sockets
- Debugger interface
- Clipboard e DDE (Dynamic Data Exchange)
- Registry virtual
- Async I/O handling
- Console emulation (60KB console.c)

### Protocolo:
- Baseado em Unix sockets
- Requisições/respostas síncronas
- Async completion ports
- Structured request/response format

---

## 7. COMPONENTES DE INTERFACE

### Drivers de Tela:
- **winex11.drv**: Backend X11 (1000+ linhas)
- **winemac.drv**: macOS driver
- **wineandroid.drv**: Android backend
- **winewayland.drv**: Wayland driver
- **winewayland.drv**: Novo driver Wayland

### Drivers de Input:
- **winehid.sys**: HID (Human Interface Devices)
- **winexinput.sys**: XInput gamepad support
- **winebus.sys**: USB bus driver
- **wineusb.sys**: USB support

### Drivers de Som:
- **winealsa.drv**: ALSA audio
- **winepulse.drv**: PulseAudio
- **winecoreaudio.drv**: macOS CoreAudio
- **wineoss.drv**: OSS audio

### Drivers de Impressão:
- **wineps.drv**: PostScript printer
- **winspool.drv**: Print spooler
- **localspl.dll**: Local print provider

---

## 8. COMO WINE IMPLEMENTA APIs WINDOWS

### 1. Tradução de Chamadas (Call Translation)

```
Aplicação Windows → DLL Wine (User-mode) → Wine Server (Kernel-mode) → Sistema Linux
```

### 2. Arquivo .spec → Função C

**kernel32.spec:**
```
140 stdcall CreateProcessA(str str ptr ptr int long ptr str ptr ptr) KERNEL_CreateProcessA
```

**kernel32.c:**
```c
BOOL WINAPI CreateProcessA(
    LPCSTR app, LPCSTR cmd, LPSECURITY_ATTRIBUTES psa, 
    LPSECURITY_ATTRIBUTES tsa, BOOL inherit, DWORD flags,
    LPVOID env, LPCSTR dir, LPSTARTUPINFOA si, LPPROCESS_INFORMATION pi)
{
    // Implementação que chama syscalls Linux
    // ou delegações ao Wine Server
}
```

### 3. Padrão UNIXLIB (Unix Library Bridge)

Alguns DLLs usam bridge para código Unix:

**ws2_32/Makefile.in:**
```
MODULE = ws2_32.dll
UNIXLIB = ws2_32.so
IMPORTLIB = ws2_32
SOURCES = async.c inaddr.c protocol.c socket.c unixlib.c
UNIX_LIBS = $(PTHREAD_LIBS)
```

**unixlib.c** contém código nativo que usa POSIX sockets:
```c
socket_unix_socket(int family, int type, int protocol)
{
    return socket(family, type, protocol);  // Chamada POSIX direta
}
```

### 4. Implementações Híbridas

Muitas DLLs combinam:
- **Wrapper Windows API** (em C)
- **Código nativo Unix** (em unixlib.c)
- **Servidor Win32 server** (para operações compartilhadas)

Exemplos:
- **ws2_32.dll**: Wraps POSIX sockets
- **opengl32.dll**: Wraps GLX/OpenGL nativo
- **dnsapi.dll**: Wraps resolver POSIX
- **ntdll.dll**: Implementação principal do NT

---

## 9. ESTRUTURA DE HEADERS

### `/include/` (36MB):

**Subcategorias:**
- **windows.h**: Headers padrão Windows
- **msvc/**: Headers MSVC
- **ddk/**: Windows Driver Kit headers
- **wine/**: Headers internos Wine

**Headers internos Wine:**
- `server.h`: Interface com Wine Server
- `server_protocol.h` (160KB): Definições de protocolo
- `unixlib.h`: Bridge Unix/Windows
- `exception.h`: Exception handling
- `gdi_driver.h`: Driver interface GDI
- `vulkan_driver.h`: Driver interface Vulkan

---

## 10. BIBLIOTECAS EXTERNAS INTEGRADAS (`/libs/`)

### Gráficos:
- **vkd3d**: Direct3D 12 via Vulkan (Microsoft)
- **png, jpeg, tiff, zlib**: Codificadores de imagem
- **lcms2**: Gerenciamento de cores

### Áudio/Codecs:
- **faudio**: Engenharia de áudio XAudio2 (FNA Project)
- **fluidsynth, mpg123, gsm**: Decoders
- **capstone**: Disassembler

### Criptografia:
- **tomcrypt**: Primitivos criptográficos
- **uuid**: UUID generation

### Busca/Integração:
- **xslt, xml2**: XML processing
- **ldap**: Lightweight Directory Access Protocol

### Helpers:
- **strmbase**: Streaming base classes
- **mfuuid, dxguid**: UUIDs for COM interfaces
- **musl**: C library alternative
- **compiler-rt**: LLVM compiler runtime

---

## 11. FERRAMENTAS DE BUILD (`/tools/`)

- **winebuild**: Cria stubs para DLLs
- **widl**: IDL (Interface Definition Language) compiler
- **wrc**: Resource compiler
- **winegcc**: GCC wrapper para Wine
- **winemaker**: Automatiza builds Wine
- **winedump**: Analyze PE executables
- **wmc**: Message compiler

---

## 12. PRINCIPAIS PADRÕES DE IMPLEMENTAÇÃO

### Padrão 1: Pure Wrapper
```
Windows API → Linux syscall
Exemplo: ntdll syscalls diretas
```

### Padrão 2: Server Delegation
```
Windows API → Wine Server → Recurso compartilhado
Exemplo: CreateProcessA → servidor gerencia processo
```

### Padrão 3: Library Wrapping
```
Windows API → Biblioteca nativa (OpenGL, POSIX)
Exemplo: OpenGL32 → GLX, Vulkan
```

### Padrão 4: COM/RPC
```
Windows COM Interface → RPC Protocol → Implementação
Exemplo: OLE32, Media Foundation
```

---

## 13. COMPATIBILIDADE COM APLICAÇÕES MODERNAS

### Áreas de Foco:
1. **D3D12 / Vulkan**: Via VKD3D (Microsoft)
2. **Media Foundation**: Stack completo em `mf*.dll` (8+ DLLs)
3. **WinRT**: Implementação parcial (40+ namespaces)
4. **Async/Await**: ThreadPool em ntdll, coremessaging
5. **DirectXMath**: Otimizações SIMD
6. **Shader Compilation**: HLSL → GLSL/SPIR-V

### Gargalos Principais:
- Callbacks/Events: Implementados via servidor
- GPU Interop: Requer drivers modernos
- Virtual Memory: Mapeamento Win32 → mmap Linux
- COM Threading: Marshaling entre threads
- Registry: Emulação completa em memória

---

## 14. ARQUIVOS-CHAVE PARA MELHORIA DE COMPATIBILIDADE

### Para APIs Modernas:
1. `/home/user/wineX/dlls/d3d12/` - Direct3D 12
2. `/home/user/wineX/dlls/mf/` - Media Foundation core
3. `/home/user/wineX/dlls/dxgi/` - Graphics infrastructure
4. `/home/user/wineX/dlls/windows.*.dll/` - WinRT stubs

### Para Sincronização:
1. `/home/user/wineX/dlls/ntdll/threadpool.c` (108KB)
2. `/home/user/wineX/server/thread.c` - Server threading

### Para Graphics:
1. `/home/user/wineX/dlls/wined3d/` - D3D implementation
2. `/home/user/wineX/dlls/dwrite/` - Typography
3. `/home/user/wineX/dlls/d2d1/` - Direct2D

### Para Networking:
1. `/home/user/wineX/dlls/ws2_32/` - Winsock 2
2. `/home/user/wineX/dlls/httpapi/` - HTTP
3. `/home/user/wineX/server/sock.c` (139KB)

### Para Runtime:
1. `/home/user/wineX/dlls/ntdll/` - Core runtime
2. `/home/user/wineX/dlls/mscoree.dll` - .NET runtime
3. `/home/user/wineX/dlls/ucrtbase.dll` - Universal CRT

---

## 15. ESTATÍSTICAS DO REPOSITÓRIO

- **Total de DLLs**: 723
- **Programas**: 116 (cmd, explorer, notepad, wordpad, etc)
- **Linhas de código servidor**: ~2.5MB
- **Linhas de código drivers**: ~1MB
- **Suporte de arquitecturas**: i386, x86_64, ARM, ARM64
- **Versão atual**: 10.19
- **Licença**: LGPL 2.1+

