CC   = gcc
SRCDIR = src
SRC  = $(SRCDIR)/main.c

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
TEST_CFLAGS  = -O0 -g -Wall -Wno-missing-braces -I$(SRCDIR)
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
WEB_LDFLAGS = $(WEB_RAYLIB_LIB) $(SQLITE_WEB_OBJ) \
              -s USE_GLFW=3 -s ASYNCIFY -s ALLOW_MEMORY_GROWTH=1 -s FORCE_FILESYSTEM=1 \
              -lidbfs.js --preload-file assets --preload-file Font -lm

.PHONY: all linux clean setup test web web-setup web-serve web-clean

all: setup $(TARGET)

# Explicit native (desktop Linux) target — same as `all` on Linux.
linux: all

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

$(TARGET): $(SRC) $(SRCDIR)/clay.h $(SQLITE_OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

test: setup $(SQLITE_OBJ) $(TEST_TARGET)
	$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC) $(SRCDIR)/app_data.h $(SRCDIR)/db.h $(SRCDIR)/score_logic.h $(SRCDIR)/struct_table.h $(SQLITE_OBJ)
	@mkdir -p bin
	$(CC) $(TEST_CFLAGS) $(TEST_SRC) -o $(TEST_TARGET) $(TEST_LDFLAGS)

# ── WebAssembly targets ───────────────────────────────────────────────────
web: web-setup $(SQLITE_WEB_OBJ)
	@command -v $(EMCC) >/dev/null 2>&1 || { \
		echo "ERROR: emcc not found. Install the emscripten SDK and 'source emsdk_env.sh' first."; exit 1; }
	@if [ ! -d Font ]; then \
		echo "ERROR: ./Font directory not found. The .ttf files in assets/fonts.cfg must exist to be preloaded."; exit 1; fi
	@mkdir -p $(WEB_OUTDIR)
	$(EMCC) $(WEB_CFLAGS) $(SRC) -o $(WEB_OUTDIR)/index.html $(WEB_LDFLAGS)
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
	rm -f $(TARGET) $(TEST_TARGET) $(SQLITE_OBJ) $(SQLITE_WEB_OBJ)
	rm -rf $(WEB_OUTDIR)

