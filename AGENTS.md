# Repository Guidelines

## Project Structure & Module Organization

This repository contains Wine, with experimental Linux RISC-V64 work documented in `risc-v.md`. Windows API implementations live in `dlls/`; module tests are in `dlls/<module>/tests/`. `programs/` contains Windows utilities, `server/` implements wineserver, and `loader/` handles executable startup. Shared headers live in `include/`, supporting libraries in `libs/`, and build tools in `tools/`. Resources include `fonts/`, `nls/`, `po/`, and module-local resource files.

## Build, Test, and Development Commands

Run these commands from the source root unless using a separate build directory:

- `./configure --help`: inspect architecture and dependency options.
- `./configure`: detect dependencies and generate build files.
- `make -j$(nproc)`: compile Wine and supporting binaries.
- `./wine notepad`: run the built Wine locally.
- `make dlls/ntdll/tests/check`: build and run the NTDLL tests.
- `make check`: run the broader test suite.
- `make dlls/ntdll/tests/testclean`: clear cached NTDLL test results before rerunning.

Read `README.md` for prerequisites and `risc-v.md` for port-specific configuration and smoke tests.

## Coding Style & Naming Conventions

Follow `.editorconfig`: four-space indentation, UTF-8, a final newline, and a 100-column line limit; Makefiles use tabs with width eight. Match surrounding C code, brace placement, and Windows API types and calling conventions. Preserve exported API names; use descriptive names consistent with nearby internal helpers. Add sources to the relevant `Makefile.in`. Avoid unrelated formatting changes; use an EditorConfig-aware editor.

## Testing Guidelines

Tests use Wine's C test framework from `include/wine/test.h`, including `START_TEST(name)` and `ok()` assertions. Place regression cases in the affected module's existing test file, or register a new `.c` file in its test `Makefile.in`. Verify Windows behavior where possible. No numeric coverage threshold is specified; cover changed behavior and relevant failure paths. Report the architecture, configuration, commands, and results, distinguishing compilation from runtime validation.

## Commit & Pull Request Guidelines

Recent commits use `component: imperative summary`, such as `ntdll/riscv64: fix context restoration`. Keep changes focused and reviewable. Describe the problem, resulting behavior, tests, and limitations in the review request; link relevant issues. For RISC-V64 changes, preserve the shared server protocol ABI and explicitly state whether hardware testing was performed.
