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

CFLAGS      = -O2 -Wall -Wno-missing-braces -I$(RAYLIB)/include -I$(SRCDIR)
LDFLAGS     = $(RAYLIB)/lib/libraylib.a $(SQLITE_OBJ) $(OS_LDFLAGS) -lm
TARGET      = ./bin/program$(EXT)

TEST_TARGET  = ./bin/test_logic$(EXT)
TEST_SRC     = $(SRCDIR)/test_logic.c
TEST_CFLAGS  = -O0 -g -Wall -Wno-missing-braces -I$(SRCDIR)
TEST_LDFLAGS = $(SQLITE_OBJ) $(OS_TLDFLAGS) -lm

CLAY_URL = https://raw.githubusercontent.com/nicbarker/clay/main/clay.h

.PHONY: all clean setup test

all: setup $(TARGET)

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

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(SQLITE_OBJ)

