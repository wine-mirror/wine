# Guia de Modernização do Wine - Recomendações Práticas

## 1. PRIORIDADES DE DESENVOLVIMENTO PARA COMPATIBILIDADE MODERNA

### NÍVEL 1: CRÍTICO (Máximo impacto)

#### 1.1 Direct3D 12 / Vulkan
- **Arquivo**: `/home/user/wineX/dlls/d3d12/`
- **Biblioteca base**: VKD3D (em `/home/user/wineX/libs/vkd3d/`)
- **Status**: Funcional em casos básicos, faltam features avançadas
- **Melhorias necessárias**:
  - Command recording em multi-threading
  - Resource binding improvements
  - Shader compilation optimization
  - Memory tier management
  - Ray tracing support

#### 1.2 Media Foundation Stack
- **Arquivos**: 
  - `/home/user/wineX/dlls/mf/` (core)
  - `/home/user/wineX/dlls/mfplat/` (platform)
  - `/home/user/wineX/dlls/mfreadwrite/` (I/O)
  - `/home/user/wineX/dlls/mfplay/` (playback)
- **Status**: Muitos stubs, funcionalidade limitada
- **Melhorias necessárias**:
  - Decoders (H.264, HEVC, VP9)
  - Encoders (H.264, HEVC)
  - Audio processing
  - Stream selection

#### 1.3 Async Operations / Concurrency
- **Arquivo**: `/home/user/wineX/dlls/ntdll/threadpool.c` (108KB)
- **Status**: Implementado, mas pode otimizar
- **Melhorias necessárias**:
  - I/O completion callbacks
  - Timer optimization
  - Work stealing scheduler
  - Numa awareness

### NÍVEL 2: IMPORTANTE (Melhor compatibilidade)

#### 2.1 Networking Moderno
- **Arquivo**: `/home/user/wineX/dlls/ws2_32/`
- **Servidor**: `/home/user/wineX/server/sock.c` (139KB)
- **Melhorias necessárias**:
  - IPv6 completo
  - SCTP support
  - Low-latency options
  - Tclass/QoS handling
  - UDP_GRO/UDP_GSO

#### 2.2 WinRT APIs
- **Arquivos**: `/home/user/wineX/dlls/windows.*.dll/` (40+ DLLs)
- **Prioridade**:
  1. `windows.ui.xaml/` - XAML rendering
  2. `windows.storage/` - File access
  3. `windows.media/` - Media playback
  4. `windows.networking/` - Network APIs
- **Melhorias necessárias**:
  - XAML parser/renderer
  - Async task integration
  - COM apartment threading

#### 2.3 Graphics Modern
- **Arquivo**: `/home/user/wineX/dlls/wined3d/shader.c`
- **Status**: GLSL, SPIR-V compilação funcional
- **Melhorias necessárias**:
  - Mesh shaders
  - Variable rate shading
  - Sampler feedback
  - HLSL 2021 features

### NÍVEL 3: ENHANCEMENT (Polish)

#### 3.1 Windows Modern UI
- **Arquivo**: `/home/user/wineX/dlls/user32/`
- **Melhorias**: Dark mode, DPI awareness, touch input

#### 3.2 Performance
- **Arquivo**: `/home/user/wineX/server/`
- **Melhorias**: Server latency, batch operations, caching

---

## 2. GUIA PRÁTICO DE IMPLEMENTAÇÃO

### Para cada DLL que quer melhorar:

#### Passo 1: Entender estrutura
```bash
cd /home/user/wineX/dlls/{nome}/
ls -la                          # Listar arquivos
cat Makefile.in                 # Ver dependências
cat {nome}.spec                 # Ver exports
head -100 {nome}.c              # Entender implementação
```

#### Passo 2: Identificar funções faltando
```bash
grep -r "FIXME\|TODO\|stub\|unimplemented" .
```

#### Passo 3: Comparar com Windows
```bash
# Documentação Windows SDK (online):
# https://docs.microsoft.com/en-us/windows/win32/
```

#### Passo 4: Implementar
1. Adicionar função no .c
2. Adicionar export no .spec
3. Escrever testes em tests/
4. Compilar e testar

#### Passo 5: Build & Test
```bash
make -C /home/user/wineX/dlls/{nome}/
cd /home/user/wineX/dlls/{nome}/tests/
make test
```

---

## 3. ARQUIVOS-CHAVE POR CATEGORIA

### Gráficos & Rendering
| Arquivo | Linha | Função |
|---------|-------|--------|
| `/home/user/wineX/dlls/wined3d/glsl_shader.c` | 542KB | Compiler |
| `/home/user/wineX/dlls/wined3d/device.c` | 210KB | D3D device |
| `/home/user/wineX/dlls/dxgi/dxgi_main.c` | - | DXGI core |
| `/home/user/wineX/dlls/dwrite/main.c` | - | Typography |
| `/home/user/wineX/dlls/d2d1/d2d1_main.c` | - | 2D graphics |

### Networking & Comunication
| Arquivo | Função |
|---------|--------|
| `/home/user/wineX/dlls/ws2_32/socket.c` | Sockets |
| `/home/user/wineX/server/sock.c` | Server sockets |
| `/home/user/wineX/dlls/httpapi/` | HTTP APIs |
| `/home/user/wineX/dlls/dnsapi/` | DNS resolution |

### UI & Controls
| Arquivo | Função |
|---------|--------|
| `/home/user/wineX/dlls/user32/message.c` | Mensagens |
| `/home/user/wineX/dlls/user32/dialog.c` | Diálogos |
| `/home/user/wineX/dlls/user32/edit.c` | Edit control |
| `/home/user/wineX/dlls/comctl32/` | Controles modernos |

### Runtime & Threading
| Arquivo | Função |
|---------|--------|
| `/home/user/wineX/dlls/ntdll/loader.c` | Module loading |
| `/home/user/wineX/dlls/ntdll/threadpool.c` | Thread pool |
| `/home/user/wineX/dlls/ntdll/exception.c` | Exceptions |
| `/home/user/wineX/server/thread.c` | Thread management |

---

## 4. PROCESSO DE BUILD

### Configuração inicial:
```bash
cd /home/user/wineX
./configure --enable-win64 \
    --with-vulkan \
    --with-ffmpeg \
    --with-gstreamer \
    --with-opengl
make
make install
```

### Build específico de DLL:
```bash
# Rebuild uma DLL específica
make -C dlls/d3d12/

# Rebuild com tests
make -C dlls/d3d12/
make -C dlls/d3d12/tests/

# Clean + rebuild
make -C dlls/d3d12/ clean
make -C dlls/d3d12/
```

### Testing:
```bash
# Test específico
wine ./programs/regedit/regedit.exe

# Com debug
WINEDEBUG=+d3d12,+dxgi wine ./app.exe

# Server em foreground
wineserver -p

# Verificar registro
wine regedit /S registry.reg
```

---

## 5. DEBUGGING AVANÇADO

### Wine Debug Channels:
```bash
# Variável de ambiente
export WINEDEBUG=+d3d12,+dxgi,+server

# Flags
WINEDEBUG=-all,+d3d12,+relay  # Apenas D3D12 + relay
WINEDEBUG=all                  # Tudo
WINEDEBUG=+thread              # Threads
WINEDEBUG=+relay               # Chamadas de função
```

### Wine Server Debug:
```bash
# Iniciar servidor em debug
wineserver -p -d 4

# Monitorar requisições
strace -e write -p $(pgrep wineserver)

# Ver todos os processos/threads
wineserver -p -k
```

### GDB Integration:
```bash
gdb --args wine app.exe
(gdb) break main
(gdb) run
```

---

## 6. TESTES & VALIDAÇÃO

### Teste de Compatibilidade API:
```bash
# Compilar e rodar testes de regressão
cd /home/user/wineX/dlls/d3d12/tests/
make

# Com wine específico
wine d3d12.exe /v 2
```

### Teste de Performance:
```bash
# Benchmark app
time wine ./benchmark.exe

# Profiling
WINEDEBUG=+server wine ./app.exe 2>&1 | tee debug.log
```

### Teste de Compatibilidade de APP:
```bash
# Lançar aplicação
wine app.exe

# Ativar logging completo
WINEDEBUG=all wine app.exe 2>&1 | grep -i error
```

---

## 7. CHECKLIST DE IMPLEMENTAÇÃO DE API

### Para implementar nova API em DLL existente:

- [ ] Identificar função no SDK Windows
- [ ] Adicionar declaração em include/*.h
- [ ] Implementar em dlls/{dll}/*.c
- [ ] Adicionar export em dlls/{dll}/*.spec
- [ ] Escrever testes em dlls/{dll}/tests/*.c
- [ ] Testar com aplicação real
- [ ] Otimizar se necessário
- [ ] Documentar comportamento especial
- [ ] Submeter patch para wine-devel

---

## 8. RECURSOS ONLINE

### Documentação Oficial:
- Wine Developer Guide: https://wiki.winehq.org/Developer_FAQ
- Windows SDK: https://docs.microsoft.com/windows/win32/
- DirectX Docs: https://docs.microsoft.com/en-us/windows/win32/direct3d12/

### Ferramentas:
- `winetricks`: Instala componentes Windows
- `protontricks`: Para Steam Proton
- `dxvk`: Direct3D 12→Vulkan layer
- `vkd3d`: Oficial Microsoft Direct3D 12→Vulkan

### Comunidade:
- Wine Mailing List: wine-devel@winehq.org
- Wine IRC: #winehq on freenode
- CodeWeavers (maintainer principal)

---

## 9. PADRÕES RECOMENDADOS

### Para nova API moderna:

```c
// header file: include/myapi.h
typedef struct {
    HANDLE handle;
    DWORD flags;
} MY_OBJECT, *PMY_OBJECT;

WINAPI MyFunction(IN LPCSTR name, OUT PMY_OBJECT *ppObj);

// implementation: dlls/myapi/main.c
#include "windef.h"
#include "winbase.h"
#include "myapi.h"

WINAPI MyFunction(IN LPCSTR name, OUT PMY_OBJECT *ppObj)
{
    MY_OBJECT *obj;
    DWORD handle;
    
    // Validação
    if (!name || !ppObj)
        return E_INVALIDARG;
    
    // Alocação
    obj = HeapAlloc(GetProcessHeap(), 0, sizeof(*obj));
    if (!obj)
        return E_OUTOFMEMORY;
    
    // Implementação
    handle = create_resource_on_server(name);
    if (!handle)
    {
        HeapFree(GetProcessHeap(), 0, obj);
        return E_FAIL;
    }
    
    obj->handle = handle;
    obj->flags = 0;
    *ppObj = obj;
    return S_OK;
}
```

### DLL .spec pattern:
```
# myapi.spec - API moderna
@ stdcall MyFunction(str ptr) myapi_MyFunction
@ stdcall MyFunctionEx(str long ptr) myapi_MyFunctionEx
```

---

## 10. RECOMENDAÇÕES FINAIS

### Prioridade Crítica:
1. **D3D12/Vulkan**: Maioria dos jogos modernos usam
2. **Media Foundation**: Streaming, vídeo
3. **WinRT/Windows moderno**: Apps UWP, moderno Windows

### Fácil Quick Win:
1. Corrigir BUGs em APIs existentes (FIXME/TODO)
2. Adicionar flags/opcões faltando
3. Melhorar error handling

### Para máxima compatibilidade:
1. Testar com aplicações reais frequentemente
2. Manter compatibilidade com apps legados
3. Otimizar performance (servidor é gargalo)
4. Usar drivers nativos sempre que possível

### Para performance:
1. Reduzir round-trips servidor
2. Batch operations
3. Cache onde possível
4. Async I/O agressivo

---

