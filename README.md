# clkflt — Cloaking Filter

`clkflt.dll` is a file-system filter driver for Windows CE and Windows Phone 7.
It comes from the FullUnlock v4.0 project (© Maxim Menshikov (ultrashot), 2012). The driver hooks
the file-system API set of `FSDMGR` and redirects file lookups. When the OS
opens a stock system module, the driver can serve a patched copy from disk
instead.

This is legacy research and homebrew code for a platform that reached end of
life long ago.

## How it works

The driver installs as an FSDMGR filter. FSDMGR calls `FILTER_HookVolume` (in
`main.cpp`) one time per volume. This function wires the exported entry points to
the underlying `FILTERHOOK`. Every file-system call then passes through this DLL.

- Path redirection. `CreateFileW` and `GetFileAttributesW` send the requested
  name to `Cloaking_GetNewPath` (`Cloaking.cpp`). The function matches the name
  against a replacement list. The driver loads this list one time from the
  registry. On a match, the driver reissues the call against the replacement
  name.
- In-memory cache. `CFastCache` (`CFastCache.cpp`) caches file contents. It uses
  a string hash (`HashString`) as the key. This speeds up repeated reads. The
  driver removes an entry when a file is opened for write. It adds the entry
  again on close.
- Diagnostics. Every hook emits a `RETAILMSG` trace. `DEBUGLOG` in `debug.h`
  controls these traces.

### Registry configuration

The driver reads the redirection list from this key:

```
HKEY_LOCAL_MACHINE\Software\OEM\Cloaking
```

Each value under this key defines one redirect. The value name is the original
file name. The value data (REG_SZ) is the replacement name. The driver loads the
list on first use (`LoadCloakList`).

## Layout

```
clkflt/
├── clkflt.vcproj      Visual Studio 2008 project (WM6 Pro / WP7 ARMv4I)
├── .clang-format      Formatting rules for src/
├── src/               Project sources
│   ├── main.cpp           DllMain, FILTER_HookVolume / FILTER_UnhookVolume
│   ├── File.cpp           Handle-based I/O hooks (read/write/seek/ioctl/…)
│   ├── Find.cpp           FindFirstFile / FindNextFile / FindClose hooks
│   ├── Path.cpp           Path-based hooks (attributes, move, delete, …)
│   ├── Cloaking.cpp/.h     Registry-driven path redirection
│   ├── CFastCache.cpp/.h   File-content cache
│   ├── CList.hpp           Singly-linked list template used by the cache
│   ├── HashString.cpp/.h   djb2-style string hash
│   ├── clkflt.h            VolumeState / HandleState definitions
│   ├── debug.h             Diagnostics macros
│   ├── stdafx.h/.cpp       Precompiled-header stub
│   ├── resource.h, clkflt.rc   Version resource
│   └── clkflt.def         Exported filter entry points
└── sdk/               Vendored WinCE FSD SDK headers + import library
    ├── fsdmgr.h, fsioctl.h, extfile.h, lockmgr*.h
    └── coredll7.lib
```

## Building

You need Visual Studio 2008 with the target device SDKs installed:

- Windows Mobile 6 Professional SDK (ARMV4I). This is the primary release
  target.
- WP7SDK (ARMv4I).

To build the driver:

1. Open `clkflt.vcproj`.
2. Select a configuration.
3. Build the project.

The output is `clkflt.dll`. The include and library paths point at `src/` and
`sdk/`. The project is self-contained and does not need an enclosing solution.
