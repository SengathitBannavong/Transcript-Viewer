# Transcript Viewer v1.0

A desktop student transcript viewer built in **C** using **Clay UI** and **Raylib**. This is a [Table Program](https://github.com/J9-239717/Table-List-Subject) upgrade.   
Stores per-user data in a local **SQLite3** database. No server or internet connection required at runtime. ( **Vibe Code** )

---

## Features

- Per-user SQLite database (`db_<username>.db`) created automatically on first login
- Subject table with midterm / final score editing via click-to-edit popup
- Grade letter and pass/fail recalculated automatically from scores
- Dashboard page with grade-distribution donut chart, CPA gauge, credit progress bars, and graduation checklist
- Graduation status computed from configurable credit requirements per subject type
- Academic alert level (warning / danger) shown as a banner
- Command palette (`Ctrl+K`) for fast score entry and navigation
- Configurable font and FPS via `assets/ui.cfg` (no recompile needed)
- Import / export the database as a portable `.db` file (move data between machines)

---

## Dependencies

| Dependency | Version | How it is used |
|---|---|---|
| [Raylib](https://github.com/raysan5/raylib) | 5.5 | Window, input, 2-D drawing (fonts, rings, text) |
| [Clay](https://github.com/nicbarker/clay) | 0.14  | Immediate-mode UI layout engine (single header) |
| [SQLite3](https://www.sqlite.org) | 3.46, bundled | Persistent per-user score database |
| GCC / Clang | any modern | C11 compiler |
| libGL, libm, libpthread, libdl | system | Required by Raylib on Linux |
| libX11, libXrandr, libXi, libXinerama, libXcursor | system | Raylib window/input on Linux X11 |

> **SQLite needs no system package on any platform.** The build downloads the
> official SQLite amalgamation and compiles it straight into the binary, so
> neither `sqlite-devel` / `libsqlite3-dev` nor a runtime `libsqlite3` is
> required. Raylib and `clay.h` are fetched automatically the same way.

---

## Fast install (Linux — one command)

No clone needed. This downloads the source, installs dependencies, compiles a
self-contained binary (SQLite statically linked in — no runtime `libsqlite3`
needed), packs the executable + config into `~/transcript-viewer-linux/`, and
registers a **`ctt`** command that launches the app from there:

```bash
curl -fsSL https://raw.githubusercontent.com/SengathitBannavong/Transcript-Viewer/main/install.sh | bash
```

Then reload your shell and start the app with a single command:

```bash
source ~/.bashrc   # or just open a new terminal
ctt                # cd's into ~/transcript-viewer-linux and runs ./program
```

Options (environment variables):

```bash
# skip the (sudo) dependency step
curl -fsSL .../install.sh | SKIP_DEPS=1 bash

# install to a different directory / from a different branch
curl -fsSL .../install.sh | INSTALL_DIR=~/somewhere TV_BRANCH=main bash
```

Your `db_<username>.db` files live in the install directory and are **preserved
across re-installs**. Supports `dnf` / `apt` / `pacman` / `zypper` for the
dependency step. Already cloned the repo? Just run `./install.sh` — it builds
from your local checkout instead of downloading.

> **Windows users:** this script is Linux-only (it compiles the X11/OpenGL
> binary and wires a command into your shell rc). Use the PowerShell one-liner
> instead — see [Setup (Windows)](#setup-windows).

---

## Setup (Linux)

### 1. Install system packages

Only the compiler, the fetch tools and the GL/X11 headers are needed — there is
no SQLite package in these lists on purpose (see the note under *Dependencies*).

**Debian / Ubuntu**
```bash
sudo apt install gcc make curl tar unzip \
     libgl-dev libx11-dev libxrandr-dev libxi-dev \
     libxinerama-dev libxcursor-dev
```

**Arch / Manjaro**
```bash
sudo pacman -S gcc make curl tar unzip \
               mesa libx11 libxrandr libxi libxinerama libxcursor
```

**Fedora / RHEL**
```bash
sudo dnf install gcc make curl tar unzip \
     mesa-libGL-devel libX11-devel libXrandr-devel libXi-devel \
     libXinerama-devel libXcursor-devel
```

---

### 2. Clone this repository

```bash
git clone <your-repo-url>
cd Transcript-Viewer
```

---

### 3. Download dependencies (automatic)

The `Makefile` fetches **Raylib 5.5**, **clay.h** and the **SQLite amalgamation**
automatically if they are missing:

```bash
make setup
```

Or just run `make` — each download is a build prerequisite, so it is fetched
before anything that needs it.

---

### 4. Build

```bash
make
```

The binary is written to `./bin/program`.

---

### 5. Add a font

The program reads font paths from `assets/fonts.cfg`.  
Edit the file and add at least one `.ttf` path (relative or absolute):

```
# assets/fonts.cfg
Font/Quicksand/static/Quicksand-Regular.ttf
Font/Space_Mono/SpaceMono-Bold.ttf
/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
```

Bundled fonts in the `Font/` directory are ready to use.  
Without a valid font entry the program will display an error screen and exit.

---

### 6. Run

```bash
./bin/program
```

Enter any username at the startup screen and press **Enter**.  
- A new database is created automatically if none exists.
- Username `test` is seeded with sample scores for testing.

---

## Setup (Windows)

### Option A — one command in PowerShell (recommended)

No compiler, no clone, no zip to unpack. Open **PowerShell** and run:

```powershell
irm https://raw.githubusercontent.com/SengathitBannavong/Transcript-Viewer/main/install.ps1 | iex
```

This downloads the latest release, installs it to
`%LOCALAPPDATA%\Programs\Transcript-Viewer`, adds a Start Menu entry, and
registers a **`ctt`** command. Open a new terminal and type:

```powershell
ctt
```

Options (environment variables, since `iex` can't take parameters):

```powershell
# install somewhere else
$env:INSTALL_DIR = "D:\Apps\TranscriptViewer"; irm .../install.ps1 | iex

# pin a specific release instead of the latest
$env:TV_VERSION = "v1.2.3"; irm .../install.ps1 | iex
```

Your `db_<username>.db` files live in the install directory and are
**preserved across re-installs**. Running the script again upgrades in place.

> **This is also the fix for the antivirus warning.** A file downloaded by a
> *browser* is tagged with the Mark-of-the-Web, which is what makes Windows
> throw up *"Windows protected your PC"* on first launch. `Invoke-WebRequest`
> does not apply that tag, so the installed app just starts. See
> [Why Windows complains](#why-windows-complains) for the whole story.

### Option B — prebuilt zip (manual)

1. Download **`transcript-viewer-windows.zip`** from the
   [latest release](../../releases/latest).
2. **Right-click the `.zip` → Properties → tick "Unblock" → OK.** Do this
   *before* extracting, or every extracted file inherits the untrusted mark.
3. Extract it to a folder you can write to — your Desktop or Documents.
   Do **not** run it from inside the `.zip` viewer; the app needs to create
   `db_<username>.db` next to itself.
4. Double-click **`Run-Transcript-Viewer.bat`**.

The `.bat` is there because the app resolves `assets/`, `Font/` and the
database relative to the *working directory*. Launching `program.exe` directly
from a shortcut or another folder would start it with the wrong one, and the
config and font would not be found. The `.bat` sets it first.

> Everything is statically linked, so no DLLs or redistributables are needed.

### Why Windows complains

Two different mechanisms get blamed for the same thing, and only one of them
is about the code:

| What you see | Why | What helps |
|---|---|---|
| *"Windows protected your PC"* (SmartScreen) | The binary is unsigned **and** your browser tagged the download with the Mark-of-the-Web. SmartScreen has no reputation record for a brand-new file. | Option A avoids the tag entirely. With Option B, **Unblock** the zip first. Otherwise: **More info → Run anyway**. |
| A detection like `Trojan:Win32/Wacatac` | A **false positive**. Defender's heuristics score any unsigned, statically-linked, low-prevalence C binary as suspicious — MinGW-built executables are a well-known trigger. | Restore it from Windows Security → *Protection history*, and please [report the false positive to Microsoft](https://www.microsoft.com/en-us/wdsi/filesubmission). |

Neither is fixed by shipping a `.exe` installer — an unsigned installer stub is
flagged *more* often, not less. The only complete fix is an
[Authenticode code-signing certificate](https://learn.microsoft.com/en-us/windows/security/threat-protection/windows-defender-application-control/deploy-catalog-files-to-support-wdac),
which costs money this project doesn't spend. Everything here is source-visible
and reproducible — build it yourself with Option C if you'd rather not trust a
prebuilt binary.

### Option C — build from source (MSYS2 / MinGW-w64)

**1. Install MSYS2** from https://www.msys2.org, then open the
**MSYS2 MinGW 64-bit** shell (not the plain MSYS shell — the `MINGW64`
environment is what provides the Windows-native compiler):

```bash
pacman -S --needed mingw-w64-x86_64-gcc make curl unzip zip
```

**2. Clone and build:**

```bash
git clone <your-repo-url>
cd Transcript-Viewer
make
```

The same `Makefile` serves both platforms — it detects Windows via the `OS`
environment variable, so there is no separate makefile to pass with `-f`.
It automatically downloads Raylib (`raylib-5.5_win64_mingw-w64`), `clay.h` and
the SQLite amalgamation, then writes the binary to `./bin/program.exe`.

**3. Run** from the repository root, so `assets/` and `Font/` resolve:

```bash
./bin/program.exe
```

### Notes for Windows

- `assets/fonts.cfg` paths use forward slashes — both `/` and `\` work in MinGW.
  A system font path such as `C:/Windows/Fonts/arial.ttf` is fine.
- Keep `program.exe`, `assets/` and `Font/` together, and start the program with
  that folder as the working directory.
- The `-mwindows` linker flag suppresses the console window. Drop it from
  `OS_LDFLAGS` in the `Makefile` if you want to see `stderr` output while
  debugging.
- File dialogs use the built-in Windows common dialogs (comdlg32) — no extra
  tooling, unlike Linux where `zenity`/`kdialog` are used.

---

## Configuration (`assets/ui.cfg`)

Edit this file and restart the program to apply changes (no recompile needed).

| Key | Default | Range | Description |
|---|---|---|---|
| `font_scale` | `1.8` | `0.1` – `10.0` | Multiplier for all font sizes |
| `target_fps` | `60` | `60` – `240` | Target frame rate |

Example:
```
font_scale 2.0
target_fps 144
```

---

## Import / Export Database

Each user's data lives in a single SQLite file (`db_<username>.db`). The **Export**
and **Import** buttons in the sidebar let you back it up or move it between
machines. The format is identical on Windows and Linux, so a database
copies across platforms unchanged.

**Export** — saves a copy of the current database.
- Opens a native *Save file* dialog — the standard Windows common dialog, or
  `zenity` (falling back to `kdialog`) on Linux.
- If no dialog is available (headless / minimal desktop), the file is copied to
  your home directory as `db_<username>.db` and the destination is shown.

**Import** — replaces the current user's database with a chosen `.db` file.
- Opens a native *Open file* dialog, **or** simply **drag a `.db` file onto the
  window** (works with no dialog tool installed).
- The file is validated as a real SQLite database before anything is replaced;
  an invalid file is rejected and the current data is left untouched.
- The table and dashboard refresh immediately after a successful import.

> **Windows** uses the built-in common dialogs (comdlg32) — nothing to install.
>
> **Linux** uses `zenity` (GNOME) or `kdialog` (KDE), which ship with most
> desktops but are **not** bundled in the AppImage. On a system without either,
> export falls back to your home directory and import works via drag-and-drop —
> so the feature degrades gracefully. To guarantee the dialogs are available:
>
> ```bash
> # Debian/Ubuntu: sudo apt install zenity
> # Fedora:        sudo dnf install zenity
> # Arch:          sudo pacman -S zenity
> ```

---

## Running Tests

```bash
make test
```

Runs 49 unit tests covering CPA calculation, credit counting, graduation logic, and alert level computation.

---

## Project Structure

```
Transcript-Viewer/
├── src/
│   ├── main.c              — entry point, globals, keyboard handler, frame loop
│   ├── ui.c                — all Clay UI rendering
│   ├── db.c / db.h         — SQLite backend, import/export
│   ├── score_logic.c/.h    — CPA / graduation / alert computation
│   ├── cmd.c / cmd.h       — command palette dispatcher
│   ├── app_config.c/.h     — ui.cfg + fonts.cfg loading
│   ├── pdf_export.c/.h     — transcript / simulation PDF writer
│   ├── platform.c/.h       — home directory + native file dialogs per OS
│   ├── app_data.h          — Player / Subject structs and global instance
│   ├── struct_table.h      — core struct definitions
│   ├── clay_renderer_raylib.c/.h — Clay → Raylib draw backend
│   └── test_logic.c        — unit tests (standalone binary)
├── Makefile                — Linux *and* Windows build (detects the OS)
├── install.sh              — one-command Linux installer (builds from source)
├── install.ps1             — one-command Windows installer (fetches the release)
├── assets/
│   ├── ui.cfg              — font_scale, target_fps
│   ├── fonts.cfg           — font search list
│   ├── subjects.dat        — curriculum data (seeded into new DBs)
│   └── grad_config.cfg     — graduation credit requirements per type
├── Font/                   — bundled OFL-licensed fonts (not tracked in git)
├── sqlite3.c / sqlite3.h   — SQLite amalgamation, downloaded by make
└── raylib-5.5_*/           — downloaded by make
```

Only `platform.c` contains OS-specific code; everything else is portable C.

---

## Usage Guide

See [program.md](program.md) for the full list of keyboard shortcuts, mouse controls, commands, and config file documentation.

---

## License

This project is released under the **MIT License**.  
See [LICENSE.md](LICENSE.md) for details including third-party component licenses.
