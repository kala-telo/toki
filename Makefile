APP_CFLAGS += -std=c23 -Ivendor
OBJS=build/main.o build/irc.o build/implementations.o
TARGET=toki

HEADERS_WAYLAND=build/wayland_protocols/xdg-shell.h build/wayland_protocols/xdg-decoration-unstable-v1.h build/wayland_protocols/xdg-toplevel-icon-v1.h build/wayland_protocols/relative-pointer-unstable-v1.h build/wayland_protocols/pointer-constraints-unstable-v1.h build/wayland_protocols/xdg-output-unstable-v1.h
OBJS_WAYLAND=build/wayland_protocols/xdg-shell.o build/wayland_protocols/xdg-toplevel-icon-v1.o build/wayland_protocols/xdg-decoration-unstable-v1.o build/wayland_protocols/relative-pointer-unstable-v1.o build/wayland_protocols/pointer-constraints-unstable-v1.o build/wayland_protocols/xdg-output-unstable-v1.o

PLATFORM_HEADERS_Linux=$(HEADERS_WAYLAND)
PLATFORM_OBJS_Linux=$(OBJS_WAYLAND)
PLATFORM_CFLAGS_Linux=-Ibuild/wayland_protocols/
PLATFORM_LDFLAGS_Linux=-lEGL -lGLESv2 -lwayland-egl -lwayland-cursor -lwayland-client -lm -lxkbcommon

PLATFORM_HEADERS_Darwin=
PLATFORM_OBJS_Darwin=
PLATFORM_CFLAGS_Darwin=
PLATFORM_LDFLAGS_Darwin=-framework Cocoa -framework OpenGL -framework IOKit -lm

UNAME_S != uname -s
PLATFORM_HEADERS=$(PLATFORM_HEADERS_$(UNAME_S))
PLATFORM_OBJS=$(PLATFORM_OBJS_$(UNAME_S))
PLATFORM_CFLAGS=$(PLATFORM_CFLAGS_$(UNAME_S))
PLATFORM_LDFLAGS=$(PLATFORM_LDFLAGS_$(UNAME_S))

all: $(OBJS) $(PLATFORM_OBJS)
	$(CC) $(CFLAGS) $(APP_CFLAGS) $(PLATFORM_CFLAGS) $(LDFLAGS) $(PLATFORM_LDFLAGS) -o $(TARGET) $(OBJS) $(PLATFORM_OBJS)

clean:
	rm -rf ./build || true
	rm toki || true

build:
	mkdir -p ./build

build/implementations.o: $(PLATFORM_HEADERS) vendor/RGFW.h src/implementations.c vendor/clay.h vendor/clay_renderer_gles3_loader_stb.h build
	$(CC) -Wno-unused-result $(CFLAGS) $(APP_CFLAGS) $(PLATFORM_CFLAGS) -c src/implementations.c -o build/implementations.o
build/main.o: src/main.c src/colors.h src/da.h src/irc.h vendor/RGFW.h build
	$(CC) -Wall $(CFLAGS) $(APP_CFLAGS) $(PLATFORM_CFLAGS) -c src/main.c -o build/main.o
build/irc.o: src/irc.c src/da.h src/irc.h build
	$(CC) -Wall $(CFLAGS) $(APP_CFLAGS) $(PLATFORM_CFLAGS) -c src/irc.c -o build/irc.o

build/wayland_protocols:
	mkdir -p ./build/wayland_protocols/
build/wayland_protocols/xdg-shell.h: /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml build/wayland_protocols
	wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml build/wayland_protocols/xdg-shell.h
build/wayland_protocols/xdg-shell.c: /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml build/wayland_protocols
	wayland-scanner public-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml build/wayland_protocols/xdg-shell.c
build/wayland_protocols/xdg-shell.o: build/wayland_protocols/xdg-shell.c build/wayland_protocols
	$(CC) $(CFLAGS) $(APP_CFLAGS) $(PLATFORM_CFLAGS) -c build/wayland_protocols/xdg-shell.c -o build/wayland_protocols/xdg-shell.o
build/wayland_protocols/xdg-decoration-unstable-v1.h: /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml build/wayland_protocols
	wayland-scanner client-header /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml build/wayland_protocols/xdg-decoration-unstable-v1.h
build/wayland_protocols/xdg-toplevel-icon-v1.c: /usr/share/wayland-protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml build/wayland_protocols
	wayland-scanner public-code /usr/share/wayland-protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml build/wayland_protocols/xdg-toplevel-icon-v1.c
build/wayland_protocols/xdg-toplevel-icon-v1.o: build/wayland_protocols/xdg-toplevel-icon-v1.c build/wayland_protocols
	$(CC) $(CFLAGS) $(APP_CFLAGS) $(PLATFORM_CFLAGS) -c build/wayland_protocols/xdg-toplevel-icon-v1.c -o build/wayland_protocols/xdg-toplevel-icon-v1.o
build/wayland_protocols/xdg-toplevel-icon-v1.h: /usr/share/wayland-protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml build/wayland_protocols
	wayland-scanner client-header /usr/share/wayland-protocols/staging/xdg-toplevel-icon/xdg-toplevel-icon-v1.xml build/wayland_protocols/xdg-toplevel-icon-v1.h
build/wayland_protocols/xdg-decoration-unstable-v1.c: /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml build/wayland_protocols
	wayland-scanner public-code /usr/share/wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml build/wayland_protocols/xdg-decoration-unstable-v1.c
build/wayland_protocols/xdg-decoration-unstable-v1.o: build/wayland_protocols/xdg-decoration-unstable-v1.c build/wayland_protocols
	$(CC) $(CFLAGS) $(APP_CFLAGS) $(PLATFORM_CFLAGS) -c build/wayland_protocols/xdg-decoration-unstable-v1.c -o build/wayland_protocols/xdg-decoration-unstable-v1.o
build/wayland_protocols/relative-pointer-unstable-v1.h: /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml build/wayland_protocols
	wayland-scanner client-header /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml build/wayland_protocols/relative-pointer-unstable-v1.h
build/wayland_protocols/relative-pointer-unstable-v1.c: /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml build/wayland_protocols
	wayland-scanner public-code /usr/share/wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1.xml build/wayland_protocols/relative-pointer-unstable-v1.c
build/wayland_protocols/relative-pointer-unstable-v1.o: build/wayland_protocols/relative-pointer-unstable-v1.c build/wayland_protocols
	$(CC) $(CFLAGS) $(APP_CFLAGS) $(PLATFORM_CFLAGS) -c build/wayland_protocols/relative-pointer-unstable-v1.c -o build/wayland_protocols/relative-pointer-unstable-v1.o
build/wayland_protocols/pointer-constraints-unstable-v1.h: /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml build/wayland_protocols
	wayland-scanner client-header /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml build/wayland_protocols/pointer-constraints-unstable-v1.h
build/wayland_protocols/pointer-constraints-unstable-v1.c: /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml build/wayland_protocols
	wayland-scanner public-code /usr/share/wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml build/wayland_protocols/pointer-constraints-unstable-v1.c
build/wayland_protocols/pointer-constraints-unstable-v1.o: build/wayland_protocols/pointer-constraints-unstable-v1.c build/wayland_protocols
	$(CC) $(CFLAGS) $(APP_CFLAGS) $(PLATFORM_CFLAGS) -c build/wayland_protocols/pointer-constraints-unstable-v1.c -o build/wayland_protocols/pointer-constraints-unstable-v1.o
build/wayland_protocols/xdg-output-unstable-v1.h: /usr/share/wayland-protocols/unstable/xdg-output/xdg-output-unstable-v1.xml build/wayland_protocols
	wayland-scanner client-header /usr/share/wayland-protocols/unstable/xdg-output/xdg-output-unstable-v1.xml build/wayland_protocols/xdg-output-unstable-v1.h
build/wayland_protocols/xdg-output-unstable-v1.c: /usr/share/wayland-protocols/unstable/xdg-output/xdg-output-unstable-v1.xml build/wayland_protocols
	wayland-scanner public-code /usr/share/wayland-protocols/unstable/xdg-output/xdg-output-unstable-v1.xml build/wayland_protocols/xdg-output-unstable-v1.c
build/wayland_protocols/xdg-output-unstable-v1.o: build/wayland_protocols/xdg-output-unstable-v1.c build/wayland_protocols
	$(CC) $(CFLAGS) $(APP_CFLAGS) $(PLATFORM_CFLAGS) -c build/wayland_protocols/xdg-output-unstable-v1.c -o build/wayland_protocols/xdg-output-unstable-v1.o