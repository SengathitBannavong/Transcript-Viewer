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

---

## Dependencies

| Dependency | Version | How it is used |
|---|---|---|
| [Raylib](https://github.com/raysan5/raylib) | 5.5 | Window, input, 2-D drawing (fonts, rings, text) |
| [Clay](https://github.com/nicbarker/clay) | 0.14  | Immediate-mode UI layout engine (single header) |
| [SQLite3](https://www.sqlite.org) | system | Persistent per-user score database |
| GCC / Clang | any modern | C11 compiler |
| libGL, libm, libpthread, libdl | system | Required by Raylib on Linux |
| libX11, libXrandr, libXi, libXinerama, libXcursor | system | Raylib window/input on Linux X11 |

---

## Setup (Linux)

### 1. Install system packages

**Debian / Ubuntu**
```bash
sudo apt install gcc libsqlite3-dev libgl-dev libx11-dev \
     libxrandr-dev libxi-dev libxinerama-dev libxcursor-dev \
     curl make
```

**Arch / Manjaro**
```bash
sudo pacman -S gcc sqlite mesa libx11 libxrandr libxi \
               libxinerama libxcursor curl make
```

**Fedora / RHEL**
```bash
sudo dnf install gcc sqlite-devel mesa-libGL-devel libX11-devel \
     libXrandr-devel libXi-devel libXinerama-devel libXcursor-devel \
     curl make
```

---

### 2. Clone this repository

```bash
git clone <your-repo-url>
cd clay_project
```

---

### 3. Download dependencies (automatic)

The `Makefile` fetches **Raylib 5.5** and **clay.h** automatically if they are missing:

```bash
make setup
```

Or just run `make` — `setup` is a prerequisite of the default target.

---

### 4. Build

```bash
make
```

The binary is written to `./bin/program`.

---

## Setup (Windows)

Uses `Makefile.win` with **MinGW-w64** (via MSYS2 recommended).  
SQLite3 is compiled from the official amalgamation — no system package needed.

### 1. Install MSYS2

Download and install from https://www.msys2.org, then open the **MSYS2 MinGW 64-bit** shell and run:

```bash
pacman -S mingw-w64-x86_64-gcc make curl unzip
```

### 2. Clone this repository

```bash
git clone <your-repo-url>
cd clay_project
```

### 3. Build

```bash
make -f Makefile.win
```

This will automatically download:
- `raylib-5.5_win64_mingw-w64.zip` (Raylib Windows MinGW build)
- `clay.h` (single-header Clay UI)
- `sqlite-amalgamation-*.zip` (SQLite3 source, compiled inline)

The binary is written to `./bin/program.exe`.

### 4. Run

```bash
./bin/program.exe
```

Or double-click `bin/program.exe` in Explorer (make sure `assets/` and `Font/` folders are next to `bin/`).

### Notes for Windows
- `assets/fonts.cfg` paths use forward slashes — both `/` and `\` work in MinGW.
- System font path example: `C:/Windows/Fonts/arial.ttf`
- The `-mwindows` linker flag suppresses the console window for release builds. Remove it in `Makefile.win` if you want to see `stderr` output.

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

## Running Tests

```bash
make test
```

Runs 49 unit tests covering CPA calculation, credit counting, graduation logic, and alert level computation.

---

## Project Structure

```
clay_project/
├── main.c              — entry point, globals, keyboard handler, donut rendering
├── ui.c                — all Clay UI rendering (included into main.c)
├── db.h                — SQLite backend
├── score_logic.h       — CPA / graduation / alert computation
├── cmd.h               — command palette dispatcher
├── app_data.h          — Player / Subject structs and global instance
├── struct_table.h      — core struct definitions
├── test_logic.c        — unit tests (standalone binary)
├── Makefile            — Linux build
├── Makefile.win        — Windows build (MinGW-w64 / MSYS2)
├── assets/
│   ├── ui.cfg          — font_scale, target_fps
│   ├── fonts.cfg       — font search list
│   ├── subjects.dat    — curriculum data (seeded into new DBs)
│   └── grad_config.cfg — graduation credit requirements per type
├── Font/               — bundled OFL-licensed fonts
└── raylib-5.5_linux_amd64/  — downloaded by make setup
```

---

## Usage Guide

See [program.md](program.md) for the full list of keyboard shortcuts, mouse controls, commands, and config file documentation.

---

## License

This project is released under the **MIT License**.  
See [LICENSE.md](LICENSE.md) for details including third-party component licenses.
