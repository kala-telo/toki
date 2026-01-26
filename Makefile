# Detect OS
UNAME_S := $(shell uname -s)

CFLAGS=-std=c23 -Ivendor
TARGET=toki

ifeq ($(UNAME_S),Darwin)
    # macOS
    LDFLAGS=-framework Cocoa -framework OpenGL -framework IOKit -lm
    PLATFORM_OBJS=
    PLATFORM_DEPS=build
else
    # Linux (Wayland)
    WAYLAND_LD != pkg-config --libs wayland-egl wayland-cursor xkbcommon
    LDFLAGS=-lEGL -lGLESv2 -lm $(WAYLAND_LD)
    CFLAGS += -Ibuild/wayland_protocols/
    PLATFORM_OBJS=build/wayland_protocols/xdg-shell.o build/wayland_protocols/xdg-toplevel-icon-v1.o build/wayland_protocols/xdg-decoration-unstable-v1.o build/wayland_protocols/relative-pointer-unstable-v1.o build/wayland_protocols/pointer-constraints-unstable-v1.o build/wayland_protocols/xdg-output-unstable-v1.o
    PLATFORM_DEPS=wayland_stuff
endif

optimized: CFLAGS += -O3 -flto -march=native
debug:     CFLAGS += -Og -g

debug optimized: all

all: $(PLATFORM_DEPS) build/main.o build/implementations.o build/irc.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(PLATFORM_OBJS) build/main.o build/implementations.o build/irc.o

clean:
	rm -rf ./build || true

build:
	mkdir -p build

build/implementations.o: src/implementations.c vendor/RGFW.h vendor/clay.h vendor/clay_renderer_gles3_loader_stb.h build
	$(CC) -Wno-unused-result $(CFLAGS) -c src/implementations.c -o build/implementations.o
build/main.o: src/main.c src/colors.h src/da.h src/irc.h build
	$(CC) -Wall $(CFLAGS) -c src/main.c -o build/main.o
build/irc.o: src/irc.c src/da.h src/irc.h build
	$(CC) -Wall $(CFLAGS) -c src/irc.c -o build/irc.o

# Linux Wayland protocol generation
wayland_stuff: build/wayland_protocols/xdg-shell.h build/wayland_protocols/xdg-shell.o build/wayland_protocols/xdg-decoration-unstable-v1.h build/wayland_protocols/xdg-toplevel-icon-v1.o build/wayland_protocols/xdg-toplevel-icon-v1.h build/wayland_protocols/xdg-decoration-unstable-v1.o build/wayland_protocols/relative-pointer-unstable-v1.h build/wayland_protocols/relative-pointer-unstable-v1.o build/wayland_protocols/pointer-constraints-unstable-v1.h build/wayland_protocols/pointer-constraints-unstable-v1.o build/wayland_protocols/xdg-output-unstable-v1.h build/wayland_protocols/xdg-output-unstable-v1.o

build/wayland_protocols:
	mkdir -p build/wayland_protocols/
build/wayland_protocols/xdg-shell.h: /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml build/wayland_protocols
	wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml build/wayland_protocols/xdg-shell.h
build/wayland_protocols/xdg-shell.c: /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml build/wayland_protocols
	wayland-scanner public-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml build/wayland_protocols/xdg-shell.c
build/wayland_protocols/xdg-shell.o: build/wayland_protocols/xdg-shell.c build/wayland_protocols
	$(CC) $(CFLAGS) -c build/wayland_protocols/xdg-shell.c -o build/wayland_protocols/xdg-shell.o
build/wayland_protocols/xdg-decoration-unstable-v1.h: /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml build/wayland_protocols
	wayland-scanner client-header /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml build/wayland_protocols/xdg-decoration-unstable-v1.h
build/wayland_protocols/xdg-toplevel-icon-v1.c: /usr/share/wayland-protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml build/wayland_protocols
	wayland-scanner public-code /usr/share/wayland-protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml build/wayland_protocols/xdg-toplevel-icon-v1.c
build/wayland_protocols/xdg-toplevel-icon-v1.o: build/wayland_protocols/xdg-toplevel-icon-v1.c build/wayland_protocols
	$(CC) $(CFLAGS) -c build/wayland_protocols/xdg-toplevel-icon-v1.c -o build/wayland_protocols/xdg-toplevel-icon-v1.o
build/wayland_protocols/xdg-toplevel-icon-v1.h: /usr/share/wayland-protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml build/wayland_protocols
	wayland-scanner client-header /usr/share/wayland-protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml build/wayland_protocols/xdg-toplevel-icon-v1.h
build/wayland_protocols/xdg-decoration-unstable-v1.c: /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml build/wayland_protocols
	wayland-scanner public-code /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml build/wayland_protocols/xdg-decoration-unstable-v1.c
build/wayland_protocols/xdg-decoration-unstable-v1.o: build/wayland_protocols/xdg-decoration-unstable-v1.c build/wayland_protocols
	$(CC) $(CFLAGS) -c build/wayland_protocols/xdg-decoration-unstable-v1.c -o build/wayland_protocols/xdg-decoration-unstable-v1.o
build/wayland_protocols/relative-pointer-unstable-v1.h: /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml build/wayland_protocols
	wayland-scanner client-header /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml build/wayland_protocols/relative-pointer-unstable-v1.h
build/wayland_protocols/relative-pointer-unstable-v1.c: /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml build/wayland_protocols
	wayland-scanner public-code /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml build/wayland_protocols/relative-pointer-unstable-v1.c
build/wayland_protocols/relative-pointer-unstable-v1.o: build/wayland_protocols/relative-pointer-unstable-v1.c build/wayland_protocols
	$(CC) $(CFLAGS) -c build/wayland_protocols/relative-pointer-unstable-v1.c -o build/wayland_protocols/relative-pointer-unstable-v1.o
build/wayland_protocols/pointer-constraints-unstable-v1.h: /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml build/wayland_protocols
	wayland-scanner client-header /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml build/wayland_protocols/pointer-constraints-unstable-v1.h
build/wayland_protocols/pointer-constraints-unstable-v1.c: /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml build/wayland_protocols
	wayland-scanner public-code /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml build/wayland_protocols/pointer-constraints-unstable-v1.c
build/wayland_protocols/pointer-constraints-unstable-v1.o: build/wayland_protocols/pointer-constraints-unstable-v1.c build/wayland_protocols
	$(CC) $(CFLAGS) -c build/wayland_protocols/pointer-constraints-unstable-v1.c -o build/wayland_protocols/pointer-constraints-unstable-v1.o
build/wayland_protocols/xdg-output-unstable-v1.h: /usr/share/wayland-protocols/unstable/xdg-output/xdg-output-unstable-v1.xml build/wayland_protocols
	wayland-scanner client-header /usr/share/wayland-protocols/unstable/xdg-output/xdg-output-unstable-v1.xml build/wayland_protocols/xdg-output-unstable-v1.h
build/wayland_protocols/xdg-output-unstable-v1.c: /usr/share/wayland-protocols/unstable/xdg-output/xdg-output-unstable-v1.xml build/wayland_protocols
	wayland-scanner public-code /usr/share/wayland-protocols/unstable/xdg-output/xdg-output-unstable-v1.xml build/wayland_protocols/xdg-output-unstable-v1.c
build/wayland_protocols/xdg-output-unstable-v1.o: build/wayland_protocols/xdg-output-unstable-v1.c build/wayland_protocols
	$(CC) $(CFLAGS) -c build/wayland_protocols/xdg-output-unstable-v1.c -o build/wayland_protocols/xdg-output-unstable-v1.o
