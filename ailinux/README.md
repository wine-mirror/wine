# AILinux Wine experimental alpha

Development layer on top of Wine, not a replacement for the system Wine.
Stage progression: alpha (repeatable build + smoke tests), beta (application matrix),
rc (frozen dependencies + reproducible release candidates), release (verified artifacts).

## Components
- Wine: upstream Wine source, Wine LGPL-2.1-or-later licensing applies.
- Wine-Staging: evaluate individual patches against this exact upstream commit; no blind overlays.
- DXVK: optional external D3D8/9/10/11 to Vulkan implementation.
- VKD3D-Proton: optional external D3D12 to Vulkan implementation.
- Windows redistributables: obtain from original vendors only when needed, with licensing respected.
- ReactOS: clean-room code may be studied subject to source licenses, provenance and compatibility.

Never redistribute proprietary Microsoft DLLs as part of this repository.
Do not overwrite /usr/bin/wine or the user default WINEPREFIX.
