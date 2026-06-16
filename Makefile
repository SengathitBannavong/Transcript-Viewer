CC   = gcc
SRCDIR = src
SRCS = $(SRCDIR)/main.c $(SRCDIR)/ui.c $(SRCDIR)/clay_renderer_raylib.c \
       $(SRCDIR)/db.c $(SRCDIR)/score_logic.c $(SRCDIR)/cmd.c $(SRCDIR)/app_config.c
OBJS = $(SRCS:.c=.o)
APP_DEPS = $(SRCDIR)/app_config.h $(SRCDIR)/app_data.h \
           $(SRCDIR)/cmd.h $(SRCDIR)/db.h $(SRCDIR)/score_logic.h \
           $(SRCDIR)/struct_table.h $(SRCDIR)/ui.h $(SRCDIR)/clay_renderer_raylib.h


# ── OS detection ──────────────────────────────────────────────────────────
ifeq ($(OS),Windows_NT)
    RAYLIB       = raylib-5.5_win64_mingw-w64
    RAYLIB_DL    = curl -fsSL "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_win64_mingw-w64.zip" \
                       -o raylib_tmp.zip && unzip -q raylib_tmp.zip && rm raylib_tmp.zip
    SQLITE_OBJ   = sqlite3.o
    SQLITE_DL    = curl -fsSL "https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip" \
                       -o sqlite_tmp.zip && unzip -qj sqlite_tmp.zip "*/sqlite3.c" "*/sqlite3.h" && rm sqlite_tmp.zip
    EXT          = .exe
    OS_LDFLAGS   = -lopengl32 -lgdi32 -lwinmm -lws2_32 -mwindows
    OS_TLDFLAGS  =
else
    RAYLIB       = raylib-5.5_linux_amd64
    RAYLIB_DL    = curl -fsSL "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_linux_amd64.tar.gz" \
                       -o raylib_tmp.tar.gz && tar xzf raylib_tmp.tar.gz && rm raylib_tmp.tar.gz
    SQLITE_OBJ   =
    SQLITE_DL    = true
    EXT          =
    OS_LDFLAGS   = -lGL -lpthread -ldl -lX11 -lXrandr -lXi -lXinerama -lXcursor -lsqlite3
    OS_TLDFLAGS  = -lsqlite3
endif

CFLAGS      = -O2 -Wall -Wno-missing-braces -I$(RAYLIB)/include -I$(SRCDIR) -I.
LDFLAGS     = $(RAYLIB)/lib/libraylib.a $(SQLITE_OBJ) $(OS_LDFLAGS) -lm
TARGET      = ./bin/program$(EXT)

TEST_TARGET  = ./bin/test_logic$(EXT)
TEST_SRC     = $(SRCDIR)/test_logic.c
TEST_CFLAGS  = -O0 -g -Wall -Wno-missing-braces -I$(RAYLIB)/include -I$(SRCDIR)
TEST_LDFLAGS = $(SQLITE_OBJ) $(OS_TLDFLAGS) -lm

CLAY_URL = https://raw.githubusercontent.com/nicbarker/clay/main/clay.h

# ── WebAssembly (emscripten) build ────────────────────────────────────────
# Requires the emscripten SDK on PATH (emcc). Outputs a static site to
# bin/web/ (index.html + .js + .wasm + .data). Serve it over HTTP — opening
# index.html via file:// will not work because the browser blocks fetch().
EMCC          = emcc
WEB_OUTDIR    = bin/web
RAYLIB_WEB    = raylib-5.5_webassembly
RAYLIB_WEB_DL = curl -fsSL "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_webassembly.zip" \
                    -o raylib_web_tmp.zip && unzip -q raylib_web_tmp.zip && rm raylib_web_tmp.zip
# raylib's web release has shipped the static lib under both names; pick whichever exists.
WEB_RAYLIB_LIB = $(firstword $(wildcard $(RAYLIB_WEB)/lib/libraylib.web.a $(RAYLIB_WEB)/lib/libraylib.a))
SQLITE_WEB_OBJ = sqlite3.web.o
SQLITE_DL_URL  = https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip

WEB_CFLAGS  = -Os -Wall -Wno-missing-braces -DPLATFORM_WEB \
              -I$(RAYLIB_WEB)/include -I$(SRCDIR) -I.
# USE_GLFW=3 + ASYNCIFY are raylib's required web flags; preload the data files
# the app fopen()s at runtime into emscripten's virtual filesystem.
WEB_SHELL   = src/web/shell.html
WEB_LDFLAGS = $(WEB_RAYLIB_LIB) $(SQLITE_WEB_OBJ) \
              -s USE_GLFW=3 -s ASYNCIFY -s ALLOW_MEMORY_GROWTH=1 -s FORCE_FILESYSTEM=1 \
              -lidbfs.js --shell-file $(WEB_SHELL) \
              --preload-file assets --preload-file Font -lm

.PHONY: all linux linux-package linux-appimage clean setup test web web-setup web-serve web-clean

all: setup $(TARGET)

# Explicit native (desktop Linux) target — same as `all` on Linux.
linux: all

# ── Linux distributable package ───────────────────────────────────────────
# Produces a self-contained <name>.zip with SQLite statically compiled into
# the binary (no libsqlite3 needed on the target machine) plus assets/ + Font/.
# NOTE: OpenGL (libGL) and X11 must stay dynamically linked — they come from
# the host's graphics drivers, so a *fully* static GUI binary is not possible.
PKG_NAME       = transcript-viewer-linux
PKG_STAGE      = $(PKG_NAME)
PKG_ZIP        = $(PKG_NAME).zip
PKG_BIN        = bin/$(PKG_NAME)
SQLITE_PKG_OBJ = sqlite3.pkg.o
PKG_LDFLAGS    = $(RAYLIB)/lib/libraylib.a $(SQLITE_PKG_OBJ) \
                 -lGL -lpthread -ldl -lX11 -lXrandr -lXi -lXinerama -lXcursor \
                 -lm -static-libgcc

# Shared stripped binary with SQLite statically linked in. Used by both the
# .zip package and the AppImage. `setup` is order-only so it doesn't force a
# rebuild every time.
$(PKG_BIN): $(OBJS) $(SRCDIR)/clay.h $(SQLITE_PKG_OBJ) | setup
	@mkdir -p bin
	$(CC) $(CFLAGS) $(OBJS) -o $(PKG_BIN) $(PKG_LDFLAGS)
	strip $(PKG_BIN)

linux-package: $(PKG_BIN)
	@command -v zip >/dev/null 2>&1 || { echo "ERROR: 'zip' not found. Install it (e.g. dnf install zip)."; exit 1; }
	@echo "Staging package files..."
	@rm -rf $(PKG_STAGE) $(PKG_ZIP)
	@mkdir -p $(PKG_STAGE)
	@cp $(PKG_BIN) $(PKG_STAGE)/program
	@cp -r assets $(PKG_STAGE)/assets
	@cp -r Font   $(PKG_STAGE)/Font
	@if [ -f README.md ]; then cp README.md $(PKG_STAGE)/; fi
	@echo "Zipping..."
	@zip -r -q $(PKG_ZIP) $(PKG_STAGE)
	@rm -rf $(PKG_STAGE)
	@echo "Created $(PKG_ZIP)  (unzip, then run ./program)"

# SQLite compiled from the amalgamation for the packaged build (statically linked).
$(SQLITE_PKG_OBJ): sqlite3.c sqlite3.h
	$(CC) -O2 -DSQLITE_OMIT_LOAD_EXTENSION -c sqlite3.c -o $(SQLITE_PKG_OBJ)

# ── AppImage ──────────────────────────────────────────────────────────────
# Self-contained portable single-file app. Bundles the same static-SQLite
# binary plus assets/ + Font/. appimagetool is downloaded on demand.
# At runtime the AppImage mounts read-only, so AppRun runs the program from a
# writable per-user data dir (~/.local/share/transcript-viewer) where the
# db_<user>.db files are written; bundled assets/Font are symlinked in.
APPID            = transcript-viewer
APPDIR           = AppDir
APPIMAGE_OUT     = Transcript_Viewer-x86_64.AppImage
APPIMAGETOOL     = ./appimagetool-x86_64.AppImage
APPIMAGETOOL_URL = https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage

$(APPIMAGETOOL):
	@echo "Downloading appimagetool..."
	curl -fsSL $(APPIMAGETOOL_URL) -o $(APPIMAGETOOL)
	chmod +x $(APPIMAGETOOL)

linux-appimage: $(PKG_BIN) $(APPIMAGETOOL)
	@echo "Assembling AppDir..."
	@rm -rf $(APPDIR) $(APPIMAGE_OUT)
	@mkdir -p $(APPDIR)/usr/bin $(APPDIR)/usr/share/$(APPID) \
	          $(APPDIR)/usr/share/applications \
	          $(APPDIR)/usr/share/icons/hicolor/256x256/apps
	@cp $(PKG_BIN) $(APPDIR)/usr/bin/program
	@cp -r assets  $(APPDIR)/usr/share/$(APPID)/assets
	@cp -r Font    $(APPDIR)/usr/share/$(APPID)/Font
	@printf '%s\n' '[Desktop Entry]' 'Type=Application' 'Name=Transcript Viewer' \
	  'Exec=program' 'Icon=$(APPID)' 'Categories=Education;' 'Terminal=false' \
	  > $(APPDIR)/$(APPID).desktop
	@cp $(APPDIR)/$(APPID).desktop $(APPDIR)/usr/share/applications/$(APPID).desktop
	@python3 -c "import zlib,struct; W=H=256; raw=b''.join(b'\x00'+b'\x26\x5a\xc8'*W for _ in range(H)); ch=lambda t,d: struct.pack('>I',len(d))+t+d+struct.pack('>I',zlib.crc32(t+d)&0xffffffff); open('$(APPDIR)/$(APPID).png','wb').write(b'\x89PNG\r\n\x1a\n'+ch(b'IHDR',struct.pack('>IIBBBBB',W,H,8,2,0,0,0))+ch(b'IDAT',zlib.compress(raw,9))+ch(b'IEND',b''))"
	@cp $(APPDIR)/$(APPID).png $(APPDIR)/usr/share/icons/hicolor/256x256/apps/$(APPID).png
	@printf '%s\n' '#!/bin/sh' \
	  'HERE="$$(dirname "$$(readlink -f "$$0")")"' \
	  'APPDATA="$${XDG_DATA_HOME:-$$HOME/.local/share}/$(APPID)"' \
	  'mkdir -p "$$APPDATA"' \
	  'ln -sfn "$$HERE/usr/share/$(APPID)/assets" "$$APPDATA/assets"' \
	  'ln -sfn "$$HERE/usr/share/$(APPID)/Font" "$$APPDATA/Font"' \
	  'cd "$$APPDATA" || exit 1' \
	  'exec "$$HERE/usr/bin/program" "$$@"' \
	  > $(APPDIR)/AppRun
	@chmod +x $(APPDIR)/AppRun
	@echo "Building AppImage..."
	ARCH=x86_64 APPIMAGE_EXTRACT_AND_RUN=1 $(APPIMAGETOOL) $(APPDIR) $(APPIMAGE_OUT)
	@rm -rf $(APPDIR)
	@echo "Created $(APPIMAGE_OUT)  (chmod +x, then ./$(APPIMAGE_OUT))"

setup:
	@mkdir -p bin
	@if [ ! -f $(SRCDIR)/clay.h ]; then \
		echo "Downloading clay.h..."; \
		curl -fsSL $(CLAY_URL) -o $(SRCDIR)/clay.h; \
	fi
	@if [ ! -d $(RAYLIB) ]; then \
		echo "Downloading Raylib 5.5..."; \
		$(RAYLIB_DL); \
	fi
	@if [ ! -f sqlite3.c ] && [ -n "$(SQLITE_OBJ)" ]; then \
		echo "Downloading SQLite3 amalgamation..."; \
		$(SQLITE_DL); \
	fi

# Windows only: compile SQLite3 from source (Linux uses system -lsqlite3)
ifeq ($(OS),Windows_NT)
$(SQLITE_OBJ): sqlite3.c sqlite3.h
	$(CC) -O2 -c sqlite3.c -o $(SQLITE_OBJ)
endif

%.o: %.c $(APP_DEPS) $(SRCDIR)/clay.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS) $(SRCDIR)/clay.h $(SQLITE_OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

test: setup $(SQLITE_OBJ) $(TEST_TARGET)
	$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC) $(SRCDIR)/db.c $(SRCDIR)/score_logic.c $(APP_DEPS) $(SQLITE_OBJ)
	@mkdir -p bin
	$(CC) $(TEST_CFLAGS) $(TEST_SRC) $(SRCDIR)/db.c $(SRCDIR)/score_logic.c -o $(TEST_TARGET) $(TEST_LDFLAGS)

# ── WebAssembly targets ───────────────────────────────────────────────────
web: web-setup $(SQLITE_WEB_OBJ)
	@command -v $(EMCC) >/dev/null 2>&1 || { \
		echo "ERROR: emcc not found. Install the emscripten SDK and 'source emsdk_env.sh' first."; exit 1; }
	@if [ ! -d Font ]; then \
		echo "ERROR: ./Font directory not found. The .ttf files in assets/fonts.cfg must exist to be preloaded."; exit 1; fi
	@mkdir -p $(WEB_OUTDIR)
	$(EMCC) $(WEB_CFLAGS) $(SRCS) -o $(WEB_OUTDIR)/index.html $(WEB_LDFLAGS)
	@echo "Built $(WEB_OUTDIR)/index.html  —  run 'make web-serve' then open http://localhost:8080"

web-setup:
	@mkdir -p $(WEB_OUTDIR)
	@command -v $(EMCC) >/dev/null 2>&1 || { \
		echo "ERROR: emcc not found. Install the emscripten SDK and 'source emsdk_env.sh' first."; exit 1; }
	@if [ ! -f $(SRCDIR)/clay.h ]; then \
		echo "Downloading clay.h..."; curl -fsSL $(CLAY_URL) -o $(SRCDIR)/clay.h; fi
	@if [ ! -d $(RAYLIB_WEB) ]; then \
		echo "Downloading Raylib 5.5 (WebAssembly)..."; $(RAYLIB_WEB_DL); fi

# Download the SQLite amalgamation on demand (browser has no system libsqlite3).
sqlite3.c:
	@echo "Downloading SQLite3 amalgamation..."
	curl -fsSL "$(SQLITE_DL_URL)" -o sqlite_tmp.zip && unzip -qj sqlite_tmp.zip "*/sqlite3.c" "*/sqlite3.h" && rm sqlite_tmp.zip
sqlite3.h: sqlite3.c

# SQLite compiled to wasm.
$(SQLITE_WEB_OBJ): sqlite3.c sqlite3.h
	$(EMCC) -Os -DSQLITE_OMIT_LOAD_EXTENSION -c sqlite3.c -o $(SQLITE_WEB_OBJ)

# Serve the built site locally (fetch() needs http://, not file://).
web-serve:
	@cd $(WEB_OUTDIR) && python3 -m http.server 8080

web-clean:
	rm -rf $(WEB_OUTDIR) $(SQLITE_WEB_OBJ)

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(SQLITE_OBJ) $(SQLITE_WEB_OBJ) $(SQLITE_PKG_OBJ) $(PKG_ZIP) $(PKG_BIN) $(APPIMAGE_OUT) $(OBJS)
	rm -rf $(WEB_OUTDIR) $(PKG_STAGE) $(APPDIR)
