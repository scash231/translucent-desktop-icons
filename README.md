# Win32 Desktop Icon Opacity Control

This is a lightweight utility written in C++ that targets and modifies the opacity/transparency of the Windows desktop icon layer using the native Win32 APIs.

It mirrors the techniques used by software like Wallpaper Engine to fade desktop icons in or out.

## How It Works

1. **Hierarchy Traversal**: The program traverses the Windows desktop shell hierarchy to locate the target layer:
   - Search under `Progman` -> `SHELLDLL_DefView` -> `SysListView32`.
   - If not found, it enumerates top-level `WorkerW` windows to locate `SHELLDLL_DefView` and its child `SysListView32`.
2. **Child Window Layering**: Windows 8 and later allow adding `WS_EX_LAYERED` style attributes to child controls, but it requires that the calling application has a manifest declaring compatibility with Windows 8+. This project embeds this manifest directly.
3. **Commit Frame Styles**: Dynamically added window styles are committed using `SetWindowPos` with `SWP_FRAMECHANGED` to force the Desktop Window Manager (DWM) to update its composition logic.
4. **Apply Transparency**: Calls `SetLayeredWindowAttributes` using the `LWA_ALPHA` flag to set the alpha transparency byte (`0` - completely transparent, `255` - completely opaque). Toggling back to `255` automatically removes `WS_EX_LAYERED` to conserve system composition resources.

---

## File Structure

* `main.cpp`: Main implementation of desktop traversal and style updates.
* `manifest.xml`: Application compatibility manifest declaring Windows 8/10/11 compatibility (critical for child window layering).
* `resource.rc`: Resource script defining the embedded manifest.
* `compile.bat`: Build script to compile resources with `windres` and build the binary with `g++.exe`.

---

## Requirements

- **Windows 8, 10, or 11**
- **MinGW-w64 Compiler Toolchain** (Specifically `g++` and `windres` on your PATH)

---

## Compilation

Double-click `compile.bat` or run it from your command line:

```cmd
compile.bat
```

This will produce `desktop_icons.exe` in the same directory.

---

## Usage

You can run the application in two ways:

### 1. Interactive Mode (Default)
Double-click `desktop_icons.exe` directly from Windows Explorer. The console window will stay open and guide you:

```text
===========================================
   Desktop Icon Opacity Manipulator Tool   
===========================================
Running in Interactive Mode.
Usage with CLI: desktop_icons.exe <opacity_0-255> [listview|defview|workerw]

Current target layer: [listview]
Commands:
  0 - 255  : Change desktop opacity to this value
  t        : Change target layer (listview / defview / workerw)
  q        : Quit application
Enter command: 
```

### 2. Command-Line (CLI) Arguments
Run the utility from a command prompt or script:

```cmd
desktop_icons.exe <opacity_value> [target_layer]
```

- `<opacity_value>`: An integer from `0` (completely transparent) to `255` (completely opaque).
- `[target_layer]` (Optional): Specify which layer to target:
  - `listview` (Default): Targets `SysListView32` (the list control containing icons).
  - `defview`: Targets `SHELLDLL_DefView` (the shell view hosting the list view).
  - `workerw`: Targets the wrapper `WorkerW`/`Progman` window.

#### CLI Examples:
```cmd
# Set desktop icons to 50% opacity
desktop_icons.exe 128

# Make desktop icons completely invisible on the ShellDefView layer
desktop_icons.exe 0 defview

# Restore icons to 100% opaque
desktop_icons.exe 255
```
