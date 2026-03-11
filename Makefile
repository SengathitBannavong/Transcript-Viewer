CC      = gcc
RAYLIB  = raylib-5.5_linux_amd64
CFLAGS  = -O2 -Wall -Wno-missing-braces -I$(RAYLIB)/include
LDFLAGS = $(RAYLIB)/lib/libraylib.a -lGL -lm -lpthread -ldl -lX11 -lXrandr -lXi -lXinerama -lXcursor -lsqlite3
TARGET  = ./bin/program
SRC     = main.c

.PHONY: all clean setup

all: setup $(TARGET)

setup:
	@if [ ! -f clay.h ]; then \
		echo "Downloading clay.h..."; \
		curl -fsSL https://raw.githubusercontent.com/nicbarker/clay/main/clay.h -o clay.h; \
	fi
	@if [ ! -d $(RAYLIB) ]; then \
		echo "Downloading raylib 5.5..."; \
		curl -fsSL "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_linux_amd64.tar.gz" -o raylib.tar.gz && tar xzf raylib.tar.gz && rm raylib.tar.gz; \
	fi

$(TARGET): $(SRC) clay.h
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
