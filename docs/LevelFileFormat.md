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
  times the maximum screen width (480 pixels). If the value is 20, for example,
  the level will be 9600 pixels wide. The allowed values are between 8 and 24.

* `sky-color <color>` - One of three sky color options (1-3).

* `bgm <bgm>` - One of three background music (BGM) options (1-3).

* `goal-scene <scene>` - One of five available cutscenes to use when the player
  reaches the goal (1-5)


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

* `crates <x> <w> <h>`

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


## Changes

Until release 2024.11.21.0, there was no `goal-scene`, as the cutscene to use
was determined from the level number, and `crates` used four values instead of
three (`crates <x> <y> <w> <h>`). The use of separate Y and height values turned
out to be unnecessary because all blocks of crates are on the floor and the
removal of `<y>` simplifies level file validation. Additionally, the maximum
level size was 32, but it has been since reduced to 24, which is enough for all
levels.

