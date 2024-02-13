/*
 * Alex vs Bus
 * Copyright (C) 2021-2024 M374LX
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

/*
 * File:
 * defs.h
 *
 * Description:
 * Miscellaneous definitions for constants and structs
 *
 */

//------------------------------------------------------------------------------

#ifndef ALEXVSBUS_DEFS_H
#define ALEXVSBUS_DEFS_H

#include <stdbool.h>



//==========================================================================
// Constants: general
//

//Game title, version, and repository URL
#define GAME_TITLE "Alex vs Bus: The Race"
#define RELEASE "2024.02.13.0"
#define REPOSITORY "https://github.com/M374LX/alexvsbus"

//Maximum delta time
#define MAX_DT (1.0f / 30.0f)

//Screen types
enum {
	SCR_BLANK = 0,
	SCR_PLAY = 1,
	SCR_PLAY_FREEZE = 2, //Render a play session without updating it
	SCR_FINALSCORE = 3,
};

//Delayed action types
enum {
	DELACT_TITLE = 0,
	DELACT_NEXT_DIFFICULTY = 1,
	DELACT_TRY_AGAIN = 2,
};

//Difficulty
enum {
	DIFFICULTY_NORMAL = 0,
	DIFFICULTY_HARD = 1,
	DIFFICULTY_SUPER = 2,
	DIFFICULTY_MAX = DIFFICULTY_SUPER,
};

//Level load errors
enum {
	LVLERR_NONE = 0,
	LVLERR_CANNOT_OPEN = 1,
	LVLERR_TOO_LARGE = 2,
	LVLERR_INVALID = 3,
};

//Maximum supported size for the virtual screen
#define VSCREEN_MAX_WIDTH  480
#define VSCREEN_MAX_HEIGHT 270

//Minimum supported size for the virtual screen
#define VSCREEN_MIN_WIDTH  256
#define VSCREEN_MIN_HEIGHT 192

//Minimum size for the virtual screen when using automatic sizing (except
//if the screen is very small)
#define VSCREEN_AUTO_MIN_WIDTH  416
#define VSCREEN_AUTO_MIN_HEIGHT 240

//Screen wiping commands
enum {
	WIPECMD_IN = 0,
	WIPECMD_OUT = 1,
	WIPECMD_CLEAR = 2,
};

//Other screen wiping constants
#define WIPE_MAX_VALUE VSCREEN_MAX_WIDTH
#define WIPE_DELTA 16
#define WIPE_MAX_DELAY 0.0005f

//Text colors
enum {
	TXTCOL_WHITE = 0,
	TXTCOL_GREEN = 1,
	TXTCOL_GRAY  = 2,
};

//As in many retro games, 8x8 tiles are commonly used as the basic unit for
//positioning and size
#define TILE_SIZE 8

//Special constant meaning that something does not exist or is unset or
//inactive
//
//Commonly used as an object's X position or as an index within
//PlayCtx.objs[]
#define NONE (-1)



//==========================================================================
// Constants: input
//

//Player input actions (bitfield)
enum {
	INPUT_UP = (1 << 0),
	INPUT_DOWN = (1 << 1),
	INPUT_LEFT = (1 << 2),
	INPUT_RIGHT = (1 << 3),
	INPUT_JUMP = (1 << 4),
	INPUT_PAUSE = (1 << 5),
	INPUT_PAUSE_TOUCH = (1 << 6),
	INPUT_MENU_CONFIRM = (1 << 7),
	INPUT_MENU_RETURN = (1 << 8),
	INPUT_CFG_AUDIO_TOGGLE = (1 << 9),
};

#define MAX_GAMEPADS 8

#define MAX_TOUCH_POINTS 10

//Touchscreen button positions
#define TOUCH_LEFT_X 0
#define TOUCH_LEFT_OFFSET_Y 72
#define TOUCH_RIGHT_X 64
#define TOUCH_RIGHT_OFFSET_Y 64
#define TOUCH_JUMP_OFFSET_X 64 //Offset from right side of screen
#define TOUCH_JUMP_OFFSET_Y 64

//Touchscreen button size
#define TOUCH_BUTTON_WIDTH  64
#define TOUCH_BUTTON_HEIGHT 64

//Touchscreen button opacity
#define TOUCH_BUTTON_OPACITY 114



//==========================================================================
// Constants: audio
//

//Sound effects
enum {
	SFX_COIN = 0,
	SFX_CRATE = 1,
	SFX_ERROR = 2,
	SFX_FALL = 3,
	SFX_HIT = 4,
	SFX_HOLE = 5,
	SFX_RESPAWN = 6,
	SFX_SCORE = 7,
	SFX_SELECT = 8,
	SFX_SLIP = 9,
	SFX_SPRING = 10,
	SFX_TIME = 11,
	NUM_SFX = 12,
};

//Background music (BGM) tracks
enum {
	BGMTITLE = 0,
	BGM1 = 1,
	BGM2 = 2,
	BGM3 = 3,
};


//==========================================================================
// Constants: graphics
//

//Sprites
enum {
	SPR_BACKGROUND = 0,
	SPR_BANANA_PEEL = 1,
	SPR_BEARDED_MAN_STAND = 2,
	SPR_BEARDED_MAN_WALK = 3,
	SPR_BEARDED_MAN_JUMP = 4,
	SPR_BG_BLACK = 5,
	SPR_BG_SKY1 = 6,
	SPR_BG_SKY2 = 7,
	SPR_BG_SKY3 = 8,
	SPR_BIRD = 9,
	SPR_BUS = 10,
	SPR_BUS_CHARACTER_1 = 11,
	SPR_BUS_CHARACTER_2 = 12,
	SPR_BUS_CHARACTER_3 = 13,
	SPR_BUS_DOOR = 14,
	SPR_BUS_ROUTE = 15,
	SPR_BUS_STOP_SIGN = 16,
	SPR_BUS_WHEEL = 17,
	SPR_CAR_BLUE = 18,
	SPR_CAR_SILVER = 19,
	SPR_CAR_YELLOW = 20,
	SPR_CAR_WHEEL = 21,
	SPR_CHARSET_WHITE = 22,
	SPR_CHARSET_GREEN = 23,
	SPR_CHARSET_GRAY = 24,
	SPR_COIN_SILVER = 25,
	SPR_COIN_GOLD = 26,
	SPR_COIN_SPARK_SILVER = 27,
	SPR_COIN_SPARK_GOLD = 28,
	SPR_CRACK_PARTICLE = 29,
	SPR_CRATE = 30,
	SPR_DEEP_HOLE_LEFT = 31,
	SPR_DEEP_HOLE_LEFT_FG = 32,
	SPR_DEEP_HOLE_MIDDLE = 33,
	SPR_DEEP_HOLE_RIGHT = 34,
	SPR_DUNG = 35,
	SPR_ERROR = 36,
	SPR_FLAGMAN = 37,
	SPR_GUSH = 38,
	SPR_GUSH_CRACK = 39,
	SPR_GUSH_HOLE = 40,
	SPR_HEN = 41,
	SPR_HYDRANT = 42,
	SPR_LOGO_SMALL = 43,
	SPR_LOGO_LARGE = 44,
	SPR_MEDAL1 = 45,
	SPR_MEDAL2 = 46,
	SPR_MEDAL3 = 47,
	SPR_OVERHEAD_SIGN = 48,
	SPR_OVERHEAD_SIGN_BASE = 49,
	SPR_OVERHEAD_SIGN_BASE_TOP = 50,
	SPR_PASSAGEWAY_LEFT = 51,
	SPR_PASSAGEWAY_LEFT_FG = 52,
	SPR_PASSAGEWAY_MIDDLE = 53,
	SPR_PASSAGEWAY_RIGHT = 54,
	SPR_PASSAGEWAY_RIGHT_FG = 55,
	SPR_PASSAGEWAY_RIGHT_CLOSED = 56,
	SPR_PAUSE = 57,
	SPR_PLAYER_STAND = 58,
	SPR_PLAYER_WALK = 59,
	SPR_PLAYER_JUMP = 60,
	SPR_PLAYER_GRABROPE = 61,
	SPR_PLAYER_THROWBACK = 62,
	SPR_PLAYER_SLIP = 63,
	SPR_PLAYER_RUN = 64,
	SPR_PLAYER_CLEAN_DUNG = 65,
	SPR_POLE = 66,
	SPR_PUSH_ARROW = 67,
	SPR_ROPE_HORIZONTAL = 68,
	SPR_ROPE_VERTICAL = 69,
	SPR_SPRING = 70,
	SPR_TOUCH_LEFT = 71,
	SPR_TOUCH_LEFT_HELD = 72,
	SPR_TOUCH_RIGHT = 73,
	SPR_TOUCH_RIGHT_HELD = 74,
	SPR_TOUCH_JUMP = 75,
	SPR_TOUCH_JUMP_HELD = 76,
	SPR_TRUCK = 77,
	SPR_MENU_PLAY = 78,
	SPR_MENU_PLAY_SELECTED = 79,
	SPR_MENU_TRYAGAIN = 80,
	SPR_MENU_TRYAGAIN_SELECTED = 81,
	SPR_MENU_JUKEBOX = 82,
	SPR_MENU_JUKEBOX_SELECTED = 83,
	SPR_MENU_SETTINGS = 84,
	SPR_MENU_SETTINGS_SELECTED = 85,
	SPR_MENU_ABOUT = 86,
	SPR_MENU_ABOUT_SELECTED = 87,
	SPR_MENU_QUIT = 88,
	SPR_MENU_QUIT_SELECTED = 89,
	SPR_MENU_RETURN = 90,
	SPR_MENU_RETURN_SELECTED = 91,
	SPR_MENU_AUDIO_ON = 92,
	SPR_MENU_AUDIO_ON_SELECTED = 93,
	SPR_MENU_AUDIO_OFF = 94,
	SPR_MENU_AUDIO_OFF_SELECTED = 95,
	SPR_MENU_CONFIRM = 96,
	SPR_MENU_CONFIRM_SELECTED = 97,
	SPR_MENU_CANCEL = 98,
	SPR_MENU_CANCEL_SELECTED = 99,
	SPR_MENU_RETURN_SMALL = 100,
	SPR_MENU_RETURN_SMALL_SELECTED = 101,
	SPR_MENU_1 = 102,
	SPR_MENU_1_SELECTED = 103,
	SPR_MENU_2 = 104,
	SPR_MENU_2_SELECTED = 105,
	SPR_MENU_3 = 106,
	SPR_MENU_3_SELECTED = 107,
	SPR_MENU_4 = 108,
	SPR_MENU_4_SELECTED = 109,
	SPR_MENU_5 = 110,
	SPR_MENU_5_SELECTED = 111,
	SPR_MENU_LOCKED = 112,
	SPR_MENU_BORDER_TOPLEFT = 113,
	SPR_MENU_BORDER_TOPLEFT_SELECTED = 114,
	SPR_MENU_BORDER_TOPLEFT_DISABLED = 115,
	SPR_MENU_BORDER_TOP = 116,
	SPR_MENU_BORDER_TOP_SELECTED = 117,
	SPR_MENU_BORDER_TOP_DISABLED = 118,
	SPR_MENU_BORDER_LEFT = 119,
	SPR_MENU_BORDER_LEFT_SELECTED = 120,
	SPR_MENU_BORDER_LEFT_DISABLED = 121,
	SPR_SCANLINE = 122,
};

//Logo width in pixels
#define LOGO_WIDTH_SMALL 224
#define LOGO_WIDTH_LARGE 296



//==========================================================================
// Constants: menu
//

#define MENU_MAX_STACK_SIZE 8
#define MENU_MAX_ITEMS 16

//Cursor movement directions
enum {
	MENUDIR_UP = 0,
	MENUDIR_DOWN = 1,
	MENUDIR_LEFT = 2,
	MENUDIR_RIGHT = 3,
};

//Item alignment
enum {
	ALIGN_CENTER = 0,
	ALIGN_TOPLEFT = 1,
	ALIGN_TOPRIGHT = 2,
};

//Menu types
enum {
	MENU_MAIN = 0,
	MENU_DIFFICULTY = 1,
	MENU_LEVEL = 2,
	MENU_JUKEBOX = 3,
	MENU_SETTINGS = 4,
	MENU_DISPLAY_SETTINGS = 5,
	MENU_VSCREEN_SIZE = 6,
	MENU_VSCREEN_WIDTH = 7,
	MENU_VSCREEN_HEIGHT = 8,
	MENU_WINDOW_SCALE = 9,
	MENU_AUDIO_SETTINGS = 10,
	MENU_ABOUT = 11,
	MENU_CREDITS = 12,
	MENU_PAUSE = 13,
	MENU_TRYAGAIN_PAUSE = 14,
	MENU_TRYAGAIN_TIMEUP = 15,
	MENU_QUIT = 16,
	MENU_ERROR = 17,
};

//Menu action types
enum {
	MENUACT_QUIT = 0,
	MENUACT_TITLE = 1, //Go to title screen
	MENUACT_PLAY = 2,
	MENUACT_TRYAGAIN = 3,
};



//==========================================================================
// Constants: gameplay
//

//Internal level number of ending sequence
#define LVLNUM_ENDING 8

//Maximum numbers
#define MAX_OBJS 160
#define MAX_CRATE_BLOCKS 32
#define MAX_HOLES 32
#define MAX_GUSHES 32
#define MAX_PASSAGEWAYS 4
#define MAX_PUSHABLE_CRATES MAX_PASSAGEWAYS
#define MAX_CUTSCENE_OBJECTS 2
#define MAX_SOLIDS 96
#define MAX_TRIGGERS 8
#define MAX_RESPAWN_POINTS 32
#define MAX_COIN_SPARKS 12
#define MAX_CRACK_PARTICLES 12

//A level block is the basic unit for positioning objects in the level, as
//well as for the width of deep holes and passageways
#define LEVEL_BLOCK_SIZE (TILE_SIZE * 3)
#define VSCREEN_MAX_WIDTH_LEVEL_BLOCKS (VSCREEN_MAX_WIDTH / LEVEL_BLOCK_SIZE)

//Floor, holes, light poles, and background
#define BACKGROUND_DRAW_Y 176
#define POLE_DISTANCE 384 //Distance between light poles
#define FLOOR_Y 264
#define PASSAGEWAY_BOTTOM_Y 360

//Crate size
#define CRATE_WIDTH  24
#define CRATE_HEIGHT 24

//Hole types
enum {
	HOLE_DEEP = 0,
	HOLE_PASSAGEWAY_EXIT_CLOSED = 1,
	HOLE_PASSAGEWAY_EXIT_OPENED = 2,
};

//Solid types
enum {
	SOL_FULL = 0,
	SOL_VERTICAL = 1,
	SOL_SLOPE_UP = 2,
	SOL_SLOPE_DOWN = 3,
	SOL_KEEP_ON_TOP = 4,
	SOL_PASSAGEWAY_ENTRY = 5,
	SOL_PASSAGEWAY_EXIT = 6,
};

//Object types (for objects that use PlayCtx.objs[])
enum {
	OBJ_COIN_SILVER = 0,
	OBJ_COIN_GOLD = 1,
	OBJ_CRATE_PUSHABLE = 2,
	OBJ_BANANA_PEEL = 3,
	OBJ_BANANA_PEEL_MOVING = 4,
	OBJ_GUSH = 5,
	OBJ_GUSH_CRACK = 6,
	OBJ_ROPE_HORIZONTAL = 7,
	OBJ_ROPE_VERTICAL = 8,
	OBJ_SPRING = 9,
	OBJ_HYDRANT = 10,
	OBJ_OVERHEAD_SIGN = 11,
	OBJ_PARKED_CAR_BLUE = 12,
	OBJ_PARKED_CAR_SILVER = 13,
	OBJ_PARKED_CAR_YELLOW = 14,
	OBJ_PARKED_TRUCK = 15,
};

//Even if the player presses the jump button before the character hits the
//floor, a timer is started and a jump is triggered if the character hits
//the floor before this amount of time passes
#define JUMP_TIMEOUT 0.2f

//Player character's bounding box and height
#define PLAYER_BOX_OFFSET_X 8
#define PLAYER_BOX_WIDTH 12
#define PLAYER_HEIGHT_NORMAL 60
#define PLAYER_HEIGHT_SLIP 38

//Player character's states
enum {
	PLAYER_STATE_NORMAL = 0,
	PLAYER_STATE_SLIP = 1,
	PLAYER_STATE_GETUP = 2,
	PLAYER_STATE_THROWBACK = 3,
	PLAYER_STATE_GRABROPE = 4,
	PLAYER_STATE_FLICKER = 5,
	PLAYER_STATE_INACTIVE = 6,
};

//Player character's animation types
enum {
	PLAYER_ANIM_STAND = 0,
	PLAYER_ANIM_WALK = 1,
	PLAYER_ANIM_WALKBACK = 2,
	PLAYER_ANIM_JUMP = 3,
	PLAYER_ANIM_SLIP = 4,
	PLAYER_ANIM_SLIPREV = 5, //Reverse slip
	PLAYER_ANIM_THROWBACK = 6,
	PLAYER_ANIM_GRABROPE = 7,
};

//Car colors
#define CAR_BLUE 0
#define CAR_SILVER 1
#define CAR_YELLOW 2

//Triggered objects (other than the car colors above)
#define TRIGGER_HEN 3

//Used as a type for the Car struct to represent the traffic jam of the ending
//sequence
#define TRAFFIC_JAM 4

//Objects with a fixed Y position
#define BUS_Y 128
#define BUS_STOP_SIGN_Y 176
#define POLE_Y 120
#define GUSH_CRACK_Y 260
#define GUSH_INITIAL_Y 232
#define HYDRANT_Y 240
#define PARKED_CAR_Y 208
#define PARKED_TRUCK_Y 136
#define ROPE_Y 144
#define PUSHABLE_CRATE_Y 240
#define PASSING_CAR_Y 184
#define HEN_Y 224

//Camera velocity
#define CAMERA_XVEL 700
#define CAMERA_YVEL 400

//Animations
enum {
	ANIM_PLAYER = 0,
	ANIM_COINS = 1,
	ANIM_GUSHES = 2,
	ANIM_HIT_SPRING = 3,
	ANIM_CRACK_PARTICLES = 4,
	ANIM_BUS_WHEELS = 5,
	ANIM_BUS_DOOR_REAR = 6,
	ANIM_BUS_DOOR_FRONT = 7,
	ANIM_CAR_WHEELS = 8,
	ANIM_HEN = 9,
	ANIM_COIN_SPARKS = 10, //12 positions starting at 10
	ANIM_CUTSCENE_OBJECTS = 22, //2 positions starting at 22
	NUM_ANIMS = 24
};

//Sequence types
enum {
	SEQ_NORMAL_PLAY_START = 0,
	SEQ_NORMAL_PLAY = 1,
	SEQ_INITIAL = 10,
	SEQ_BUS_LEAVING = 20,
	SEQ_TIMEUP_BUS_NEAR = 30,
	SEQ_TIMEUP_BUS_FAR = 40,
	SEQ_GOAL_REACHED = 50,
	SEQ_GOAL_REACHED_DEFAULT = 100,
	SEQ_GOAL_REACHED_LEVEL2 = 200,
	SEQ_GOAL_REACHED_LEVEL3 = 300,
	SEQ_GOAL_REACHED_LEVEL4 = 400,
	SEQ_GOAL_REACHED_LEVEL5 = 500,
	SEQ_ENDING = 800,
	SEQ_FINISHED = 999,
};



//==========================================================================
// Structs: display parameters and configuration
//

typedef struct {
	//Size of the game's virtual screen
	int vscreen_width;
	int vscreen_height;

	//Size of the game's window (when in windowed mode) or of the entire
	//physical screen (when in fullscreen mode)
	int win_width;
	int win_height;

	//Screen scale
	int scale;
} DisplayParams;

typedef struct {
	//Display
	bool fullscreen;
	bool fixed_window_mode;
	int window_scale;
	bool scanlines_enabled;

	//Virtual screen size
	bool vscreen_auto_size;
	int vscreen_width;
	int vscreen_height;

	//Audio
	bool audio_enabled;
	bool music_enabled;
	bool sfx_enabled;

	//Touchscreen
	bool touch_enabled;
	bool touch_buttons_enabled; //Left, Right, and Jump buttons
	bool show_touch_controls;

	//Game progress
	int progress_level;
	int progress_difficulty;
	bool progress_cheat;

	//Path to assets directory
	char assets_dir[512];
} Config;



//==========================================================================
// Structs: input
//

//Stores an input action (INPUT_* constants) and the corresponding raylib
//constant for a key on the keyboard
typedef struct {
	int key;
	int action;
} InputActionKey;

//Stores an input action (INPUT_* constants) and the corresponding raylib
//constant for a gamepad button
typedef struct {
	int button;
	int action;
} InputActionButton;



//==========================================================================
// Structs: menu
//

typedef struct {
	int align;
	int offset_x, offset_y;
	int width, height;
	char* caption;
	char value[16];
	int icon_sprite;
	int targets[4]; //One target for each of the four directions
	bool disabled;
	bool hidden;
} MenuItem;

typedef struct {
	int type;
	int selected_item;
} MenuStackEntry;

typedef struct {
	const char* display_name;

	int stack_size;
	MenuStackEntry stack[MENU_MAX_STACK_SIZE];

	int num_items;
	MenuItem items[MENU_MAX_ITEMS];

	char text[512];
	int text_offset_x; //Offset from center of screen in 8x8 tiles
	int text_offset_y;
	int text_width; //Width and height in 8x8 tiles
	int text_height;
	bool text_border;

	bool level_selected;
	bool selected_visible;

	bool use_cursor;
	bool show_logo;
	bool green_bg;
	bool show_frame;
	bool fill_screen;

	int action; //MENUACT_* constants
	int action_param;
} MenuCtx;



//==========================================================================
// Structs: gameplay
//

typedef struct {
	float x, y;
	float xvel, yvel;
	float xdest;
	float xmin;
	float xmax;

	bool follow_player;
	float follow_player_min_x;
	float follow_player_max_x;

	bool fixed_at_leftmost;
	bool fixed_at_rightmost;
} PlayCamera;

typedef struct {
	int state; //PLAYER_STATE_* constants
	bool visible;
	bool on_floor;
	bool fell; //Fell into a deep hole
	int height;
	float flicker_delay;
	int anim_type; //Animation type

	float x, y; //Position
	float xvel, yvel; //Velocity
	float acc; //Acceleration
	float dec; //Deceleration
	float grav; //Gravity

	int old_state;
	float oldx, oldy;
	int old_anim_type;
} Player;

typedef struct {
	float x; //Position
	float xvel; //Velocity
	float acc; //Acceleration

	int route_sign;
	int num_characters; //Number of characters at the rear door
} Bus;

//Struct used for most game objects, which need only a type and a position
typedef struct {
	int type;
	int x, y;
} Obj;

typedef struct {
	int x, y;
	int width, height;
} CrateBlock;

typedef struct {
	int obj; //Index of the gush within PlayCtx.objs[]
	float y;
	float yvel;
	float ydest; //Destination Y position
	const int* move_pattern;
	int move_pattern_pos;
} Gush;

//Rope grabbed by the player character
typedef struct {
	int obj; //Index of the rope within PlayCtx.objs[]
	float x;
	float xmin, xmax;
	float xvel;
} GrabbedRope;

//Moving banana peel
typedef struct {
	int obj; //Index of the peel within PlayCtx.objs[]
	float x, y;
	float xdest; //Used only by thrown peels (not by slipped peels)
	float xvel, yvel;
	float grav;
} MovingPeel;

typedef struct {
	int obj; //Index of the crate within PlayCtx.objs[]
	float x;
	bool show_arrow;
	bool pushed;
	float xmax;
	int solid; //Index within PlayCtx.solids[]
} PushableCrate;

typedef struct {
	int sprite;
	float x, y;
	float xvel, yvel;
	float acc;
	float grav;
	bool in_bus;
} CutsceneObject;

//An invisible area the player character cannot pass through, which is placed
//along with the floor, crates, and so on
typedef struct {
	int type, left, right, top, bottom;
} Solid;

//Used for both deep holes and underground passageways
typedef struct {
	int type;
	int x, width;
} Hole;

//Position the player character can reappear at after falling into a deep hole
typedef struct {
	int x, y;
} RespawnPoint;

//When the player character reaches the X position of a trigger (regardless of
//Y position), either a passing car or a hen is triggered
typedef struct {
	int x;
	int what; //CAR_BLUE, CAR_SILVER, CAR_YELLOW, or TRIGGER_HEN
} Trigger;

//Either a single car that appears when triggered and throws a banana peel or
//the traffic jam of the ending sequence, but not used for parked cars
typedef struct {
	float x;
	float xvel;
	int type; //CAR_BLUE, CAR_SILVER, CAR_YELLOW, or TRAFFIC_JAM
	bool threw_peel;
	int peel_throw_x; //Throw a banana peel when the car reaches this X position
} Car;

typedef struct {
	float x;
	float xvel;
	float acc;
} Hen;

typedef struct {
	int x, y;
	bool gold;
} CoinSpark;

typedef struct {
	float x, y;
	float xvel, yvel;
	float grav;
} CrackParticle;

//Arrow indicating that a crate is pushable
typedef struct {
	float xoffs;
	float xvel;
	float delay;
} PushArrow;

//Animation
typedef struct {
	bool running;
	bool loop;
	bool reverse;
	int frame;
	int num_frames;
	float delay;
	float max_delay;
} Anim;

//Gameplay context
typedef struct {
	bool can_pause;
	int difficulty;
	int level_num;
	bool last_level; //Last level of current difficulty
	int level_size;
	int bg_color;
	int bg_offset_x;
	int bgm;

	int bus_stop_sign_x;
	int pole_x;

	int score;
	int time;
	float time_delay;
	bool time_running;
	bool time_up;
	bool goal_reached;
	bool counting_score;

	float crate_push_remaining;

	PlayCamera cam;
	Player player;
	Bus bus;

	Obj objs[MAX_OBJS];
	CrateBlock crate_blocks[MAX_CRATE_BLOCKS];
	Gush gushes[MAX_GUSHES];
	GrabbedRope grabbed_rope;
	MovingPeel slip_peel;
	MovingPeel thrown_peel;
	PushableCrate pushable_crates[MAX_PUSHABLE_CRATES];
	CutsceneObject cutscene_objects[MAX_CUTSCENE_OBJECTS];
	Solid solids[MAX_SOLIDS];

	int hit_spring; //Index within objs[] of the last spring hit by the
	                //player character

	Hole holes[MAX_HOLES];
	Hole* cur_passageway; //Passageway the player character is in, if any

	RespawnPoint respawn_points[MAX_RESPAWN_POINTS];
	Trigger triggers[MAX_TRIGGERS];

	//Objects that appear when triggered
	Car car;
	Hen hen;

	//Visual effects
	CoinSpark coin_sparks[MAX_COIN_SPARKS];
	CrackParticle crack_particles[MAX_CRACK_PARTICLES];
	PushArrow push_arrow;
	int next_coin_spark;
	int next_crack_particle;

	//Animations
	Anim anims[NUM_ANIMS];

	//Used in the ending sequence
	bool player_reached_flagman;
	bool hen_reached_flagman;
	bool bus_reached_flagman;

	//Sequence
	int sequence_step;
	float sequence_delay;
	bool skip_initial_sequence;
	bool wipe_in;
	bool wipe_out;
} PlayCtx;

#endif //ALEXVSBUS_DEFS_H

