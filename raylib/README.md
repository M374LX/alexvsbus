This directory contains a slightly modified version of raylib 5.0 for use by
the game "Alex vs Bus: The Race". The library's upstream repository can be
found in https://github.com/raysan5/raylib.


## Modifications ##

* In the file ``platforms/rcore_desktop.c``, the function
``ToggleBorderlessWindowed()`` has been modified to make it similar to
``ToggleFullscreen()`` but without a video mode change, which makes the
behavior the same as implemented in ``platforms/rcore_desktop_sdl.c``. This
avoids issues like the window being movable when it should not be.

* The file ``platforms/rcore_android.c`` has been updated to commit 5c25913
from the master branch, as it fixes an issue in which Android keycodes are not
translated.

* The file ``config.h`` has been adapted for the game.

* Files that are not needed for the game have been removed.

