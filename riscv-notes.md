sudo apt install lld llvm strace libc6-dev-riscv64-cross gh xxd tree
apt-cache policy clang lld llvm

make -B dlls/ntdll/actctx.o \
    CFLAGS="-g -O2 -fshort-wchar" --verbose

autoreconf -fi

CFLAGS="-g -O2 -fshort-wchar" ./configure --without-mingw && make clean && make -j3 && make install

./configure \
    --enable-archs=riscv64 \
    --without-mingw \
    CFLAGS="-g -O2 -fshort-wchar" && make clean && make -j3

ln -s ../winebuild/winebuild tools/winegcc/winebuild

make -C loader wine-preloader \
    WINEPRELOADER_LDFLAGS="-nostartfiles -nodefaultlibs -static -Wl,-Ttext=0x200100000" \
    -B V=1


the LLVM/Clang ecosystem fully supports the RISC-V PE/COFF binary format. While traditional GNU Binutils focuses almost exclusively on ELF for RISC-V, LLVM handles Windows and UEFI-style PE/COFF compilation. [1, 2, 3]

1. LLVM Linker (lld-link)

LLVM's LLD is the primary linker that supports generating PE/COFF binaries for RISC-V. [1, 2]
- How it works: When targeting Windows or UEFI on RISC-V, Clang switches its binary object format from ELF to COFF. LLD then acts as lld-link (emulating MSVC's linker) to generate a standard Portable Executable (.exe, .dll, or .efi).
- Upstream Status: Official support for riscv32-windows-coff and riscv64-windows-coff triples is upstreamed in the LLVM project. [1, 2, 3]

How to use it

If you are developing for Windows on RISC-V or doing RISC-V UEFI firmware development, you can leverage the LLVM toolchain directly: ]

bash
# Compile and link directly to a PE/COFF binary using Clang and LLD
clang --target=riscv64-windows-coff -fuse-ld=lld main.c -o main.exe














That is useful news for the Wine RISC-V64 port, but there is an important distinction: LLVM's ability to generate RISC-V PE/COFF executables does not automatically mean Wine can run them. It does, however, give us a way to separate compiler and linker problems from Wine's loader and runtime problems.

For our current project, there are three different pieces:
1. PE/COFF generation: Clang and LLD can potentially produce RISC-V64 Windows-format executables.
2. Wine's RISC-V64 PE loader: Wine must recognize the PE machine type, validate the image, map it, and locate its entry point.
3. Winelib runtime: Our Linux-native .exe.so application must link and run against Wine's Unix-side libraries and runtime.
Your existing Wine build has already generated RISC-V64 PE executables for wineboot and start. Our immediate blocker was Wine returning STATUS_INVALID_IMAGE_WIN_64 (c000035a), which points to an architecture-validation problem rather than simply a missing PE linker.
What I recommend next
Before introducing another compiler into the Wine build, let's confirm exactly what LLVM on your Orange Pi supports. Run these commands:
clang --version
ld.lld --version
command -v lld-link

Then test whether Clang accepts the target triple:
cat >/tmp/riscv-pe-test.c <<'EOF'
int main(void)
{
    return 0;
}
EOF

clang --target=riscv64-windows-coff \
    -c /tmp/riscv-pe-test.c \
    -o /tmp/riscv-pe-test.obj

file /tmp/riscv-pe-test.obj

This tests COFF object generation, not a complete Windows executable. The target triple and LLVM's RISC-V COFF support are documented in the upstream LLVM discussion. LLVM Phabricator
How this helps our Wine port
- If the object builds, we can investigate whether your installed LLD can link RISC-V64 PE executables.
- If it fails, the exact error will show whether the issue is target recognition, code generation, or another missing component.
- Either way, we should continue fixing Wine's architecture validation independently; switching linkers won't fix the is_machine_64bit() omission.
One important clarification: the command you found compiles for Windows, whereas our Winelib hello-world test is a Linux-native shared object loaded by Wine. We should retain that original test because it checks the native Wine runtime path we are trying to get working.
