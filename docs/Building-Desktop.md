This document describes how to build the game from source on desktop platforms,
including Windows and Linux.

The game uses the raylib library with some modifications, which is included in
the source tree along with its own dependencies. The library's upstream
repository is https://github.com/raysan5/raylib.

The default backend used by raylib is GLFW, but it is also possible to use SDL
instead. For more details, see the section "Using SDL".


## Building for Linux from Linux ##

The required libraries can be installed on Debian by running:

```apt install libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev```

Instructions about installing the libraries on other distros can be found in:
https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux.

By just running ``make``, raylib's default GLFW backend will be used. If using
SDL (as described in "Using SDL"), however, SDL2 with the development files
are also required and can be installed on Debian by running:

```apt install libsdl2-dev```

The C compiler used by default is GCC, but Clang or the Tiny C Compiler can be
used instead by setting the ``CC`` variable:

```
make CC=clang
make CC=tcc
```

The resulting executable will be named ``alexvsbus``.


## Building for Windows from Linux ##

To build the Windows executable from a Unix/Linux host system using MinGW-W64,
you need to set the variable ``WINDOWS`` to 1 and the variable
``TOOLCHAIN_PREFIX`` to the prefix used by MinGW-W64 executables, which is
``x86_64-w64-mingw32-`` for x86_64 (64-bit). Note the hyphen (-) at the end:

```make WINDOWS=1 TOOLCHAIN_PREFIX=x86_64-w64-mingw32-```

For i686 (32-bit), the toolchain prefix is ``i686-w64-mingw32-``:

```make WINDOWS=1 TOOLCHAIN_PREFIX=i686-w64-mingw32-```

The resulting executable will be named ``alexvsbus.exe``.


## Building for Windows from Windows ##

To build the game for Windows from a Windows host system, assuming MinGW-W64's
``bin`` directory is present in the ``PATH`` environment variable, run
``mingw32-make``.

The resulting executable will be named ``alexvsbus.exe``.


## Using SDL ##

To use raylib's SDL backend instead of the default GLFW one, set the variable
``SDL`` to 1:

```make SDL=1```

By default, the directory containing the SDL headers is assumed to be
``/usr/include/SDL2``. If this is not the case on your system, the directory
can be changed by setting the ``SDL_INCLUDE_PATH`` variable. For example:

```make SDL=1 SDL_INCLUDE_PATH=/usr/local/include/SDL2```

As said in "Building from Linux for Linux", SDL with the development files needs
to be installed on the system.


## Installing the game ##

On Linux, you can install the game by running ``make install`` (as superuser)
after it is built.

The default install prefix is ``/usr/local``, but a different one can be
specified by setting the ``PREFIX`` variable:

```make PREFIX=/usr install```

The game's main executable is placed in ``PREFIX/games`` by default, but a
different directory can be specified by the ``EXECPREFIX`` variable, which is
not relative to ``PREFIX`` and needs to contain the full path:

```make PREFIX=/usr EXECPREFIX=/usr/bin install```

The assets are placed in ``PREFIX/share/games/alexvsbus``. Additionally, an
icon is placed in ``PREFIX/share/pixmaps/alexvsbus.png`` and a desktop entry is
placed in ``PREFIX/share/applications``.

The ``install`` target is not supported on Windows, but you can place the
executable (``alexvsbus.exe``) and the ``assets`` folder anywhere you want
(like a subfolder in ``Program Files``) and create a desktop shortcut if you
wish. If using SDL, we recommend also placing a copy of ``SDL2.dll`` in the
same folder as the executable.


## Cleaning ##

To clean up the source tree, run ``make clean``.

