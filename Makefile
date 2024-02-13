#Program name (without executable extension)
PROGNAME := alexvsbus

#Default installation prefix (on Unix)
PREFIX := /usr/local

#Default installation prefix (on Unix) for the main executable
EXECPREFIX := $(PREFIX)/games

#Toolchain prefix (none by default, but can be set through the CLI)
TOOLCHAIN_PREFIX :=

#Default C compiler (can be changed to clang or tcc)
CC := gcc

#Header files in the game's source code
HEADERS := $(wildcard src/*.h)

#Add game's source files
CFILES := $(wildcard src/*.c)

#Add raylib's source files (only the modules used by the game)
CFILES += $(addprefix raylib/,raudio.c rcore.c utils.c)

#Common compiler flags
CFLAGS := -std=c99 -ffunction-sections -Wall -O1 -fPIC -fno-strict-aliasing

#Add compiler flags required by raylib
CFLAGS += -DGRAPHICS_API_OPENGL_21 -D_GNU_SOURCE

#Directories to be checked by the #include directive
INCLUDE_DIRS := -Iraylib -Iraylib/external/glfw/include -Iraylib/external/glfw/deps/mingw

#Check if the host system is Windows
ifeq ($(OS),Windows_NT)
	RM := del
	WINDOWS := 1
else
	RM := rm -f
endif

#Set platform-specific variables
ifeq ($(WINDOWS),1) #Building for Windows
	EXECNAME := $(PROGNAME).exe
	LIBS := -lopengl32 -lgdi32 -lwinmm
	RES := alexvsbus.res
	WINDRES := windres
	EXEC_PREREQS := $(CFILES) $(HEADERS) $(RES)
	INSTALL_PREREQ := install_windows
	CLEAN_FILES := $(EXECNAME) $(RES)
	CFLAGS += -Wl,-subsystem,windows
else
	EXECNAME := $(PROGNAME)
	LIBS := -lm -lpthread -ldl -lrt
	RES :=
	WINDRES :=
	EXEC_PREREQS := $(CFILES) $(HEADERS)
	INSTALL_PREREQ := install_unix
	CLEAN_FILES := $(EXECNAME) $(EXECNAME).exe $(EXECNAME).res
endif

#Determine raylib's backend to use (GLFW or SDL)
ifeq ($(SDL),1)
	SDL_INCLUDE_PATH := /usr/include/SDL2
	CFLAGS += -I$(SDL_INCLUDE_PATH) -DPLATFORM_DESKTOP_SDL
	LIBS += -lSDL2
else
	CFILES += raylib/rglfw.c
	CFLAGS += -DPLATFORM_DESKTOP
endif

# ==============================================================================

$(EXECNAME): $(EXEC_PREREQS)
	$(TOOLCHAIN_PREFIX)$(CC) -o $(EXECNAME) $(INCLUDE_DIRS) $(CFLAGS) $(CFILES) $(RES) $(LIBS)
	
$(RES): src/alexvsbus.rc
	$(TOOLCHAIN_PREFIX)$(WINDRES) -O coff $< $@

install: $(INSTALL_PREREQ)

install_windows:
	echo "The install target is not supported on Windows"

install_unix:
	mkdir -p $(EXECPREFIX)
	cp $(EXECNAME) $(EXECPREFIX)/$(EXECNAME)
	rm -rf $(PREFIX)/share/games/$(PROGNAME)
	mkdir -p $(PREFIX)/share/games
	cp -r assets $(PREFIX)/share/games/$(PROGNAME)
	mkdir -p $(PREFIX)/share/pixmaps
	cp icons/icon128.png $(PREFIX)/share/pixmaps/$(PROGNAME).png
	mkdir -p $(PREFIX)/share/applications
	cp icons/alexvsbus.desktop $(PREFIX)/share/applications

clean:
	$(RM) $(CLEAN_FILES)

.PHONY: install install_windows install_unix clean

