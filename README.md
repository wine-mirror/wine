# Wine on ppc64le (POWER8/POWER9)

**A fork of [Wine](https://www.winehq.org/) adding a native ppc64le host port.**
Upstream Wine has no PowerPC support; 32-bit PowerPC was removed years ago and
64-bit never existed. This branch adds it.

## Why

To run Windows software on POWER with **only the guest binary emulated**. Today,
a Windows game on a POWER9 workstation emulates every layer — the game, Wine,
and the graphics translation beneath it. A native Wine means the x86-64
emulator ([fastppcx86](https://github.com/daedalao/fastppcx86)) only has to
handle the application itself.

That matters more on POWER than elsewhere: guest x86-64 is total-store-ordered
and POWER is weakly ordered, so every emulated memory operation pays a barrier
tax that native code does not.

## Status

Honest, and in progress.

| | |
|---|---|
| `configure` and the build system | works |
| winebuild PowerPC64 codegen | **done** — import, delayed-import, relay and stub thunks, ELFv2 |
| Unix-side libraries | 33 unixlib `.so` build; `wineserver` and the loader run |
| Windows-side modules | **597 `.dll.so`, 792 `.exe.so`** link, including `kernel32`, `oleaut32`, `vcruntime140`, and `mshtml` at 27 MB |
| PE-side `ntdll.dll` | builds |
| `wineboot -u` | **not yet** — unix-side signal support is being written |
| Wine's own test suite | not yet |

Nothing here is stubbed silently: anything incomplete is recorded as incomplete
in the design notes.

## The interesting part: r2 across unwound frames

On ppc64le ELFv2, **r2 holds the TOC pointer and every module has its own**, so
any unwind across a module boundary must restore it — and **GCC emits no CFI
rule for r2**. Get it wrong and execution continues silently against the wrong
module's data.

The mechanism, verified over 84 unwind steps against independent ground truth:
the ELFv2 CFA *is* the caller's r1, so the restored frame's TOC save slot is at
`*(cfa + 24)`; whether that slot is live is decided by reading one instruction
at the frame's **resume address** — `ld r2,24(r1)` (`0xE8410018`) means load it,
anything else means leave r2 alone. This is what libgcc, nongnu libunwind and
LLVM libunwind already do, and Wine bundles the LLVM version.

The trap: ppc64's return-address register is **65, which maps to `Lr`, not
`Iar`**. On x86-64 a naive implementation works by accident because the
return-address register *is* the pc register. Reading the pc field gets the
marker from the wrong frame and silently loads stack garbage into r2.

## Building

```
./configure --enable-win64
make -j 4
```

Use a modest `-j`. Also note `ninja` — used by some subprojects — does **not**
read `MAKEFLAGS`, so pass `-j` to it explicitly or it will spawn roughly
core-count+2 jobs.

## Design notes

Measured results, adversarial reviews, and the reasoning behind each decision
live outside this tree in the project handbook — including the r2/TOC analysis,
the winebuild codegen record, and a wall-by-wall build log.

## Licence

**Unchanged: LGPL 2.1 or later**, exactly as upstream. See `LICENSE` and
`COPYING.LIB`. This is a fork rather than a re-publication precisely so the
provenance and the modified-source obligation are self-evident.

---

*Upstream Wine's README follows.*

## INTRODUCTION

Wine is a program which allows running Microsoft Windows programs
(including DOS, Windows 3.x, Win32, and Win64 executables) on Unix.
It consists of a program loader which loads and executes a Microsoft
Windows binary, and a library (called Winelib) that implements Windows
API calls using their Unix, X11 or Mac equivalents.  The library may also
be used for porting Windows code into native Unix executables.

Wine is free software, released under the GNU LGPL; see the file
LICENSE for the details.


## QUICK START

From the top-level directory of the Wine source (which contains this file),
run:

```
./configure
make
```

Then either install Wine:

```
make install
```

Or run Wine directly from the build directory:

```
./wine notepad
```

Run programs as `wine program`. For more information and problem
resolution, read the rest of this file, the Wine man page, and
especially the wealth of information found at https://www.winehq.org.


## REQUIREMENTS

To compile and run Wine, you must have one of the following:

- Linux version 2.6.22 or later
- FreeBSD 12.4 or later
- Solaris x86 9 or later
- NetBSD-current
- macOS 10.15 or later

As Wine requires kernel-level thread support to run, only the operating
systems mentioned above are supported.  Other operating systems which
support kernel threads may be supported in the future.

**FreeBSD info**:
  See https://wiki.freebsd.org/Wine for more information.

**Solaris info**:
  You will most likely need to build Wine with the GNU toolchain
  (gcc, gas, etc.). Warning : installing gas does *not* ensure that it
  will be used by gcc. Recompiling gcc after installing gas or
  symlinking cc, as and ld to the gnu tools is said to be necessary.

**NetBSD info**:
  Make sure you have the USER_LDT, SYSVSHM, SYSVSEM, and SYSVMSG options
  turned on in your kernel.

**macOS info**:
  You need Xcode/Xcode Command Line Tools or Apple cctools.

**Supported file systems**:
  Wine should run on most file systems. A few compatibility problems
  have also been reported using files accessed through Samba. Also,
  NTFS does not provide all the file system features needed by some
  applications.  Using a native Unix file system is recommended.

**Basic requirements**:
  You need to have the X11 development include files installed
  (called xorg-dev in Debian and libX11-devel in Red Hat).
  Of course you also need make (most likely GNU make).
  You also need flex version 2.5.33 or later and bison.

**Optional support libraries**:
  Configure will display notices when optional libraries are not found
  on your system. See https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine
  for hints about the packages you should install. On 64-bit
  platforms, you have to make sure to install the 32-bit versions of
  these libraries.


## COMPILATION

To build Wine, do:

```
./configure
make
```

This will build the program "wine" and numerous support libraries/binaries.
The program "wine" will load and run Windows executables.
The library "libwine" ("Winelib") can be used to compile and link
Windows source code under Unix.

To see compile configuration options, do `./configure --help`.

For more information, see https://gitlab.winehq.org/wine/wine/-/wikis/Building-Wine


## SETUP

Once Wine has been built correctly, you can do `make install`; this
will install the wine executable and libraries, the Wine man page, and
other needed files.

Don't forget to uninstall any conflicting previous Wine installation
first.  Try either `dpkg -r wine` or `rpm -e wine` or `make uninstall`
before installing.

Once installed, you can run the `winecfg` configuration tool. See the
Support area at https://www.winehq.org/ for configuration hints.


## RUNNING PROGRAMS

When invoking Wine, you may specify the entire path to the executable,
or a filename only.

For example, to run Notepad:

```
wine notepad            (using the search Path as specified in
wine notepad.exe         the registry to locate the file)

wine c:\\windows\\notepad.exe      (using DOS filename syntax)

wine ~/.wine/drive_c/windows/notepad.exe  (using Unix filename syntax)

wine notepad.exe readme.txt          (calling program with parameters)
```

Wine is not perfect, so some programs may crash. If that happens you
will get a crash log that you should attach to your report when filing
a bug.


## GETTING MORE INFORMATION

- **WWW**: A great deal of information about Wine is available from WineHQ at
	https://www.winehq.org/ : various Wine Guides, application database,
	bug tracking. This is probably the best starting point.

- **FAQ**: The Wine FAQ is located at https://gitlab.winehq.org/wine/wine/-/wikis/FAQ

- **Wiki**: The Wine Wiki is located at https://gitlab.winehq.org/wine/wine/-/wikis/

- **Gitlab**: Wine development is hosted at https://gitlab.winehq.org

- **Mailing lists**:
	There are several mailing lists for Wine users and developers; see
	https://gitlab.winehq.org/wine/wine/-/wikis/Forums for more
	information.

- **Bugs**: Report bugs to Wine Bugzilla at https://bugs.winehq.org
	Please search the bugzilla database to check whether your
	problem is already known or fixed before posting a bug report.

- **IRC**: Online help is available at channel `#WineHQ` on irc.libera.chat.
