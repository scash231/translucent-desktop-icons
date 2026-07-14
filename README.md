# TranslucentICO
# Translucent Desktop Icons

A lightweight C++ utility to change the opacity/transparency of Windows desktop icons.

## Quick Start

### 1. Run (Interactive Mode)
Just double-click **`desktop_icons.exe`**. It will open a console where you can type an opacity value (0-255) and see the change instantly.

### 2. Run (CLI Mode)
```cmd
desktop_icons.exe <opacity_0-255> [listview|defview|workerw]
```
- `listview` (default): Targets the icon grid directly.
- `defview`: Targets the folder view hosting the grid.
- `workerw`: Targets the background worker.

---

## Build

Run the compilation script using the MinGW toolchain:
```cmd
compile.bat
```
