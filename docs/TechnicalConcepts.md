This document briefly describes the basic technical concepts found in the
source code.


## Screen types and menus

In ``main.c``, the variable ``screen_type`` keeps track of the current screen
type (blank, play, or final score). The different screen types are identified by
constants defined in ``defs.h``: ``SCR_BLANK``, ``SCR_PLAY``,
``SCR_PLAY_FREEZE``, and ``SCR_FINALSCORE``.

The menus, which enable the player to select the difficulty, the level to
play and so on, are handled by the file ``menu.c`` independently of the screen
type. This enables the main menu to be show over a blank screen and the pause
menu to be shown over the play screen, for instance.


## Game objects

Objects that appear during the game include the player character, the bus,
banana peels, crates, and so on.

The file that handles the play session, including the game objects, is
``play.c``.

The ``PlayCtx`` struct, defined in ``defs.h``, keeps track of the current play
session. Its member ``objs[]`` stores the type and position of most but not all
objects. Certain object types require additional data and reference an index
within ``objs[]``. This is why structs like ``Gush`` and ``MovingPeel`` exist
in ``defs.h``. Each type of object that uses ``objs[]`` is identified by one of
the ``OBJ_*`` constants defined also in ``defs.h``.

Objects that do not use ``objs[]`` include the player character, the bus, and
unpushable crates, among others.


## Level columns

Levels are divided into columns, each 24 pixels wide. Each column has a type
(normal floor, deep hole left, deep hole middle, deep hole right, passageway
left, passageway middle, or passageway right) and a number of stacked unpushable
crates (pushable crates are handled separately). The ``LevelColumn`` struct is
defined in ``defs.h``.


## Level blocks

A level block can refer to either the basic positioning unit for game objects or
one of the small images used to draw the level. In either case, a level block is
24x24 pixels.

Each level column type has a fixed stack of blocks.


## Solids

The solids are what prevent the player character from moving through the floor
or objects like crates and parked cars and trucks. These are handled separately
from the objects themselves. The struct that stores information about solids is
``Solid``, which is defined in ``defs.h``.


## Triggers

A trigger is what causes a car that throws a banana peel or a hen to appear
when the player character reaches a certain X position. The ``Trigger`` struct
is defined in ``defs.h``.


## Respawn points

A respawn point is a position at which the player character reappears when he
falls into a deep hole. After falling, the character reappears at the closest
respawn point with an X position lower than that of the hole.


## Sequences

The sequences seen during the game, like the player character automatically
moving and jumping into the bus, or the ending sequence, are handled by the
function ``update_sequence()``, found in ``play.c``.


## Sprites

The graphics are contained in a single image file (gfx.png). Each element in
the file is referred to as a sprite.

A sprite can have more than one animation frame.

The list of sprites with the respective boundaries within the gfx.png file is
in the ``data_sprite[]`` array, found in the ``data.c`` file, and each sprite
has an ``SPR_*`` constant defined in ``defs.h``.

