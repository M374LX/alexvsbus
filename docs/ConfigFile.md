This document describes the format of the config file, which saves both user
preferences and the game progress.


## File location

On Windows, the file is ``%APPDATA%\alexvsbus\alexvsbus.cfg``, in which
``%APPDATA%`` defaults to ``C:\Users\<username>\AppData\Roaming``.

On macOS, the file is
``/Users/<username>/Library/Preferences/alexvsbus/alexvsbus.cfg``.

On other operating systems, like Linux, the file is
``$XDG_CONFIG_HOME/alexvsbus/alexvsbus.cfg``


If unset, ``$XDG_CONFIG_HOME`` defaults to ``$HOME/.config``, in accordance to
the XDG Base Directory Specification
(https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html),
summarized in https://wiki.archlinux.org/index.php/XDG_Base_Directory.


## File format

It is a simple plain text file in which each line defines a property. The line
starts with the property name, which is followed by a space and then the
property value. The value can be either a string or a number.


## Properties

* fullscreen

  The value ``true`` causes the game to run in fullscreen mode, while ``false``
  causes it to run in windowed mode.

* window-scale

  Sets the scale of the window when running in windowed mode. The allowed
  values are ``1`` (small window), ``2`` (medium-sized window), and ``3``
  (large window).

* scanlines-enabled

  The value ``true`` enables a scanlines visual effect, while the value
  ``false`` disables the effect.

* vscreen-auto-size

  The value ``true`` causes the virtual screen (vscreen) to be automatically
  set to the size that best fits in the physical screen or window, while the
  value ``false`` is used when the size is set manually.

* vscreen-width

  The width of the virtual screen (vscreen) if its size is set manually. It is
  required if the property ``vscreen-auto-size`` is set to ``false``, but
  ignored otherwise.

* vscreen-height

  The height of the virtual screen (vscreen) if its size is set manually. It is
  required if the property ``vscreen-auto-size`` is set to ``false``, but
  ignored otherwise.

* audio-enabled

  The value ``true`` enables audio output, while the value ``false`` disables
  audio output.

* music-enabled

  The value ``true`` enables music, while the value ``false`` disables music.

* sfx-enabled

  The value ``true`` enables sound effects, while the value ``false`` disables
  sound effects.

* touch-buttons-enabled

  The value ``true`` causes the left, right, and jump buttons to be displayed
  if touchscreen functionality is enabled. The player might want to disable the
  buttons when using a phone with a physical gamepad.

* progress-difficulty

  The highest difficulty the player has unlocked. The allowed values are
  ``normal``, ``hard``, and ``super``.

* progress-level

  The highest level number the player has unlocked within the highest unlocked
  difficulty. The allowed values range from 1 to 5 (if ``progress-difficulty``
  is set to ``normal`` or ``hard``) or 1 to 3 (if ``progress-difficulty`` is
  set to ``super``).

## Example file

```
fullscreen true
window-scale 2
scanlines-enabled true
audio-enabled true
music-enabled true
sfx-enabled true
touch-buttons-enabled true
vscreen-auto-size false
vscreen-width 416
vscreen-height 240
progress-difficulty normal
progress-level 1
```

