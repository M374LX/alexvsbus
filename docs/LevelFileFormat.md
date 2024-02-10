This document describes the format of a file that stores level data.


## Name

The name of a level file consists of "level" followed by the level number and
then a letter that determines the difficulty (n = normal; h = hard; s = super).
For example, ``level1n`` is the first level of normal difficulty and ``level2h``
is the second level of hard difficulty.


## Contents

Each line of the file starts with the name of a level property or object type
and is followed by one or more numeric values. The level properties must come
before the objects.

The maximum file size is 4 kB.


### Level properties

The properties are:

* `level-size <size>` - The size of the level, which corresponds to ``<size>``
  times the maximum screen width (480). The minimum is 8 and the maximum is 32.

* `sky-color <color>` - One of three sky color options (1-3).

* `bgm <bgm>` - One of three background music (BGM) options (1-3).


### Objects

For objects, the first numeric value is always the X position relative to the
previous object (or to the minimum X position, in the case of the first object).
Other values depend on the object type.

In the following list of object types, `<x>`, `<y>`, `<w>`, and `<h>` refer to
the X position, Y position, width, and height, respectively. These are given in
blocks measuring 24x24 pixels, which means that a width of 1 corresponds to 24
pixels and a width of 2 corresponds to 48 pixels.

The object types are:

* `banana-peel <x> <y>`

* `car-blue <x>`

* `car-silver <x>`

* `car-yellow <x>`

* `coin-silver <x> <y>`

* `coin-gold <x> <y>`

* `crates <x> <y> <w> <h>`

* `gush <x>`

* `gush-crack <x>`

* `hydrant <x>`

* `overhead-sign <x> <y>`

* `rope <x>`

* `spring <x> <y>`

* `truck <x>`

* `trigger-car-blue <x>`

* `trigger-car-silver <x>`

* `trigger-car-yellow <x>`

* `trigger-hen <x>`

* `respawn-point <x> <y>`

* `deep-hole <x> <w>`

* `passageway <x> <w>`

* `passageway-arrow <x> <w>`

