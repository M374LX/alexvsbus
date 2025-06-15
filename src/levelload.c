/*
 * Alex vs Bus
 * Copyright (C) 2021-2025 M374LX
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
 * levelload.c
 *
 * Description:
 * Level loading functions
 *
 */

//------------------------------------------------------------------------------

#include "defs.h"

#include <string.h>

//------------------------------------------------------------------------------

//From lineread.c
bool lineread_open(const char* path);
bool lineread_invalid();
bool lineread_ended();
void lineread_getline(char* dst);
int lineread_num_tokens(const char* str);
int lineread_token_int(const char* str, int token);

//From util.c
bool str_starts_with(const char* str, const char* start);
int get_file_size(const char* path);

//From data.c
extern const int data_gush_move_pattern_1[];

//------------------------------------------------------------------------------

static PlayCtx* ctx;

static bool invalid;

static int x_max;
static int num_objs;
static int num_crate_blocks;
static int num_gushes, num_gush_cracks;
static int num_solids;
static int num_deep_holes, num_passageways;
static int num_respawn_points;
static int num_triggers, num_car_triggers;

//------------------------------------------------------------------------------

//Function prototypes
static void add_obj(int type, int x, int y, bool use_y);
static void add_crate_block(int x, int w, int h);
static void add_deep_hole(int x, int w);
static void add_passageway(int x, int y);
static void add_respawn_point(int x, int y);
static void add_trigger(int x, int what);
static void validate_positions();
static void convert_positions();
static int add_solid(int type, int x, int y, int width, int height);
static void add_solids();

//------------------------------------------------------------------------------

void levelload_init(PlayCtx* pctx)
{
	ctx = pctx;
}

int levelload_load(const char* filename)
{
	char tmp[48];
	bool no_objects = true;
	int x;
	int i;

	invalid = false;

	//The maximum allowed file size is 4 kB
	if (get_file_size(filename) > 4096) {
		return LVLERR_TOO_LARGE;
	}

	if (!lineread_open(filename)) {
		return LVLERR_CANNOT_OPEN;
	}

	x_max = NONE;
	num_objs = 0;
	num_crate_blocks = 0;
	num_gushes = 0;
	num_gush_cracks = 0;
	num_solids = 0;
	num_deep_holes = 0;
	num_passageways = 0;
	num_respawn_points = 0;
	num_triggers = 0;
	num_car_triggers = 0;

	ctx->level_size = NONE;
	ctx->bg_color = NONE;
	ctx->bgm = NONE;
	ctx->goal_scene = NONE;

	x = VSCREEN_MAX_WIDTH_LEVEL_BLOCKS;

	//Read file
	while (!lineread_ended()) {
		int num_tokens;
		int token1, token2, token3;

		lineread_getline(tmp);

		if (lineread_invalid()) {
			return LVLERR_INVALID;
		}

		//Skip blank lines
		if (tmp[0] == '\0') {
			continue;
		}

		num_tokens = lineread_num_tokens(tmp);
		if (num_tokens < 2 || num_tokens > 4) {
			return LVLERR_INVALID;
		}

		token1 = lineread_token_int(tmp, 1);
		token2 = lineread_token_int(tmp, 2);
		token3 = lineread_token_int(tmp, 3);

		if (token1 == NONE) {
			return LVLERR_INVALID;
		}

		if (str_starts_with(tmp, "level-size ")) {
			//Error: level size redefinition
			if (ctx->level_size != NONE) {
				return LVLERR_INVALID;
			}

			//Error: size out of the allowed range
			if (token1 < 8 || token1 > 24) {
				return LVLERR_INVALID;
			}

			//Just before the last screen
			x_max = (token1 - 1) * VSCREEN_MAX_WIDTH_LEVEL_BLOCKS;

			ctx->level_size = token1 * VSCREEN_MAX_WIDTH;

			continue;
		} else if (str_starts_with(tmp, "sky-color ")) {
			//Error: sky color redefinition
			if (ctx->bg_color != NONE) {
				return LVLERR_INVALID;
			}

			switch (token1) {
				case 1:  ctx->bg_color = SPR_BG_SKY1; break;
				case 2:  ctx->bg_color = SPR_BG_SKY2; break;
				case 3:  ctx->bg_color = SPR_BG_SKY3; break;
				default: return LVLERR_INVALID;
			}

			continue;
		} else if (str_starts_with(tmp, "bgm ")) {
			//Error: BGM redefinition
			if (ctx->bgm != NONE) {
				return LVLERR_INVALID;
			}

			switch (token1) {
				case 1:  ctx->bgm = BGM1; break;
				case 2:  ctx->bgm = BGM2; break;
				case 3:  ctx->bgm = BGM3; break;
				default: return LVLERR_INVALID;
			}

			continue;
		} else if (str_starts_with(tmp, "goal-scene ")) {
			//Error: goal cutscene redefinition
			if (ctx->goal_scene != NONE) {
				return LVLERR_INVALID;
			}

			//Error: invalid goal cutscene number
			if (token1 < 1 || token1 > 5) {
				return LVLERR_INVALID;
			}

			ctx->goal_scene = token1;

			continue;
		}

		//Error: adding objects without defining the level size, sky color,
		//BGM, and goal cutscene
		if (ctx->level_size == NONE || ctx->bg_color == NONE ||
				ctx->bgm == NONE || ctx->goal_scene == NONE) {

			return LVLERR_INVALID;
		}

		//The value of token1 is relative to the previous X position
		x += token1;

		if (str_starts_with(tmp, "banana-peel ")) {
			add_obj(OBJ_BANANA_PEEL, x, token2, true);
		} else if (str_starts_with(tmp, "car-blue ")) {
			add_obj(OBJ_PARKED_CAR_BLUE, x, NONE, false);
		} else if (str_starts_with(tmp, "car-silver ")) {
			add_obj(OBJ_PARKED_CAR_SILVER, x, NONE, false);
		} else if (str_starts_with(tmp, "car-yellow ")) {
			add_obj(OBJ_PARKED_CAR_YELLOW, x, NONE, false);
		} else if (str_starts_with(tmp, "coin-silver ")) {
			add_obj(OBJ_COIN_SILVER, x, token2, true);
		} else if (str_starts_with(tmp, "coin-gold ")) {
			add_obj(OBJ_COIN_GOLD, x, token2, true);
		} else if (str_starts_with(tmp, "crates ")) {
			add_crate_block(x, token2, token3);
		} else if (str_starts_with(tmp, "gush ")) {
			add_obj(OBJ_GUSH, x, NONE, false);

			if (num_gushes >= MAX_GUSHES) {
				return LVLERR_INVALID;
			}

			ctx->gushes[num_gushes].obj = num_objs - 1;
			ctx->gushes[num_gushes].y = GUSH_INITIAL_Y;
			ctx->gushes[num_gushes].move_pattern = data_gush_move_pattern_1;
			ctx->gushes[num_gushes].move_pattern_pos = 0;
			ctx->gushes[num_gushes].yvel = data_gush_move_pattern_1[0];
			ctx->gushes[num_gushes].ydest = data_gush_move_pattern_1[1];

			num_gushes++;
		} else if (str_starts_with(tmp, "gush-crack ")) {
			add_obj(OBJ_GUSH_CRACK, x, NONE, false);
		} else if (str_starts_with(tmp, "hydrant ")) {
			add_obj(OBJ_HYDRANT, x, NONE, false);
		} else if (str_starts_with(tmp, "overhead-sign ")) {
			add_obj(OBJ_OVERHEAD_SIGN, x, token2, true);
		} else if (str_starts_with(tmp, "rope ")) {
			add_obj(OBJ_ROPE_HORIZONTAL, x, NONE, false);
			add_obj(OBJ_ROPE_VERTICAL, x, NONE, false);
		} else if (str_starts_with(tmp, "spring ")) {
			//10 is the Y position corresponding to the floor
			add_obj(OBJ_SPRING, x, 10, true);
		} else if (str_starts_with(tmp, "truck ")) {
			add_obj(OBJ_PARKED_TRUCK, x, NONE, false);
		} else if (str_starts_with(tmp, "trigger-car-blue ")) {
			add_trigger(x, CAR_BLUE);
		} else if (str_starts_with(tmp, "trigger-car-silver ")) {
			add_trigger(x, CAR_SILVER);
		} else if (str_starts_with(tmp, "trigger-car-yellow ")) {
			add_trigger(x, CAR_YELLOW);
		} else if (str_starts_with(tmp, "trigger-hen ")) {
			add_trigger(x, TRIGGER_HEN);
		} else if (str_starts_with(tmp, "respawn-point ")) {
			add_respawn_point(x, token2);
		} else if (str_starts_with(tmp, "deep-hole ")) {
			add_deep_hole(x, token2);
		} else if (str_starts_with(tmp, "passageway ")) {
			add_passageway(x, token2);

			//Spring under passageway exit (14 is the Y position corresponding
			//to a passageway bottom)
			add_obj(OBJ_SPRING, x + token2 - 1, 14, true);

			//Pushable crate over passageway entry
			add_obj(OBJ_CRATE_PUSHABLE, x, NONE, false);
			ctx->pushable_crates[num_passageways - 1].obj = num_objs - 1;
		} else if (str_starts_with(tmp, "passageway-arrow ")) {
			add_passageway(x, token2);

			//Spring under passageway exit (14 is the Y position corresponding
			//to a passageway bottom)
			add_obj(OBJ_SPRING, x + token2 - 1, 14, true);

			//Pushable crate over passageway entry
			add_obj(OBJ_CRATE_PUSHABLE, x, NONE, false);
			ctx->pushable_crates[num_passageways - 1].obj = num_objs - 1;
			ctx->pushable_crates[num_passageways - 1].show_arrow = true;
		} else {
			//Error: invalid object type
			return LVLERR_INVALID;
		}

		if (invalid) {
			return LVLERR_INVALID;
		}

		no_objects = false;
	}

	//Error: no objects
	if (no_objects) {
		return LVLERR_INVALID;
	}

	//Error: running out of gushes due to gush cracks
	if (num_gushes + num_gush_cracks > MAX_GUSHES) {
		return LVLERR_INVALID;
	}

	//Error: running out of positions in ctx->objs[] due to banana peels
	//thrown by triggered cars
	if (num_objs + num_car_triggers > MAX_OBJS) {
		return LVLERR_INVALID;
	}

	//Error: the number of respawn points is not the same as the number of
	//deep holes
	if (num_respawn_points != num_deep_holes) {
		return LVLERR_INVALID;
	}

	//Ensure there are no unpushable crates on deep holes or passageway edges
	for (i = 0; i < MAX_LEVEL_COLUMNS; i++) {
		int type = ctx->level_columns[i].type;
		int num_crates = ctx->level_columns[i].num_crates;

		if (type != LVLCOL_NORMAL_FLOOR && type != LVLCOL_PASSAGEWAY_MIDDLE) {
			if (num_crates > 0) {
				return LVLERR_INVALID;
			}
		}
	}

	validate_positions();
	convert_positions();

	if (invalid) {
		return LVLERR_INVALID;
	}

	//Set properties for ctx->pushable_crates[] (there is exactly one pushable
	//crate for each passageway)
	for (i = 0; i < num_passageways; i++) {
		int obj = ctx->pushable_crates[i].obj;

		x = ctx->objs[obj].x;
		ctx->pushable_crates[i].x = x;
		ctx->pushable_crates[i].xmax = x + LEVEL_BLOCK_SIZE;
	}

	add_solids();

	if (invalid) {
		return LVLERR_INVALID;
	}

	return LVLERR_NONE;
}

//------------------------------------------------------------------------------

static void add_obj(int type, int x, int y, bool use_y)
{
	int i;

	if (invalid) {
		return;
	}

	//Check if there are too many objects
	if (num_objs >= MAX_OBJS) {
		invalid = true;
		return;
	}

	//Check if the object's position is within the allowed range
	if (y > 14 || (y != NONE && y < 2) || (use_y && y == NONE) || x > x_max) {
		invalid = true;
		return;
	}

	//Check object repetition
	for (i = 0; i < num_objs; i++) {
		Obj obj = ctx->objs[i];

		if (obj.type == type && obj.x == x && obj.y == y) {
			invalid = true;
			return;
		}
	}

	ctx->objs[num_objs].type = type;
	ctx->objs[num_objs].x = x;
	ctx->objs[num_objs].y = y;

	num_objs++;
}

//Add a block of unpushable crates
static void add_crate_block(int x, int w, int h)
{
	int i;

	if (invalid) {
		return;
	}

	//Check if there are too many crate blocks
	if (num_crate_blocks >= MAX_CRATE_BLOCKS) {
		invalid = true;
		return;
	}

	//Check if the crate block's size is within the allowed range
	if (w < 1 || w > 4 || h < 1 || h > 5) {
		invalid = true;
		return;
	}

	//Check if the crate block's position is within the allowed range
	if (x > x_max - 4) {
		invalid = true;
		return;
	}

	if (x + w > x_max - 4) {
		//Error: crate block width extends beyond or too close to level's
		//right boundary
		invalid = true;
		return;
	}

	if (ctx->level_columns[x - 1].num_crates == h) {
		//Error: the crate block is adjacent to another crate block with the
		//same height
		invalid = true;
		return;
	}

	for (i = 0; i < w; i++) {
		//Check repetition or overlap of crates
		if (ctx->level_columns[x + i].num_crates != 0) {
			invalid = true;
			return;
		}

		//Add crates to level column
		ctx->level_columns[x + i].num_crates = h;
	}

	num_crate_blocks++;
}

static void add_deep_hole(int x, int w)
{
	int i;

	if (invalid) {
		return;
	}

	//Check if there are too many deep holes
	if (num_deep_holes >= MAX_DEEP_HOLES) {
		invalid = true;
		return;
	}

	//Check if the hole's position and size are within the allowed range
	if (x > x_max - 4 || w < 2 || w > 16) {
		invalid = true;
		return;
	}

	if (x + w > x_max - 2) {
		//Error: hole width extends beyond or too close to level's right
		//boundary
		invalid = true;
		return;
	}

	for (i = 0; i < w; i++) {
		int type;

		//Check if the deep hole is being added to a level column that already
		//has a deep hole or passageway
		if (ctx->level_columns[x + i].type != LVLCOL_NORMAL_FLOOR) {
			invalid = true;
			return;
		}

		type = LVLCOL_DEEP_HOLE_MIDDLE;
		if (i == 0) {
			type = LVLCOL_DEEP_HOLE_LEFT;
		} else if (i == w - 1) {
			type = LVLCOL_DEEP_HOLE_RIGHT;
		}

		//Insert deep hole into level columns
		ctx->level_columns[x + i].type = type;
	}

	num_deep_holes++;
}

static void add_passageway(int x, int w)
{
	int i;

	if (invalid) {
		return;
	}

	//Check if there are too many passageways
	if (num_passageways >= MAX_PASSAGEWAYS) {
		invalid = true;
		return;
	}

	//Check if the passageway's position and size are within the allowed range
	if (x > x_max - 4 || w < 2 || w > 32) {
		invalid = true;
		return;
	}

	if (x + w > x_max - 2) {
		//Error: passageway width extends beyond or too close to level's right
		//boundary
		invalid = true;
		return;
	}

	//Check if there is already a deep hole or passageway on the occupied level
	//columns
	for (i = 0; i < w; i++) {
		int type;

		//Check if the passageway is being added to a level column that already
		//has a deep hole or passageway
		if (ctx->level_columns[x + i].type != LVLCOL_NORMAL_FLOOR) {
			invalid = true;
			return;
		}

		type = LVLCOL_PASSAGEWAY_MIDDLE;
		if (i == 0) {
			type = LVLCOL_PASSAGEWAY_LEFT;
		} else if (i == w - 1) {
			type = LVLCOL_PASSAGEWAY_RIGHT;
		}

		//Insert passageway into level columns
		ctx->level_columns[x + i].type = type;
	}

	ctx->passageways[num_passageways].x = x;
	ctx->passageways[num_passageways].width = w;
	ctx->passageways[num_passageways].exit_opened = false;

	num_passageways++;
}

static void add_respawn_point(int x, int y)
{
	int i;

	if (invalid) {
		return;
	}

	//Check if there are too many respawn points
	if (num_respawn_points >= MAX_RESPAWN_POINTS) {
		invalid = true;
		return;
	}

	//Check if the respawn point's position is within the allowed range
	if (x > x_max || y < 3 || y > 15) {
		invalid = true;
		return;
	}

	//An X position cannot be shared by two or more respawn points
	for (i = 0; i < num_respawn_points; i++) {
		RespawnPoint rp = ctx->respawn_points[i];

		if (rp.x == x) {
			invalid = true;
			return;
		}
	}

	ctx->respawn_points[num_respawn_points].x = x;
	ctx->respawn_points[num_respawn_points].y = y;

	num_respawn_points++;
}

static void add_trigger(int x, int what)
{
	int i;

	if (invalid) {
		return;
	}

	//Check if there are too many triggers
	if (num_triggers >= MAX_TRIGGERS) {
		invalid = true;
		return;
	}

	//Check if the trigger's position is within the allowed range
	if (x > x_max - 20) {
		invalid = true;
		return;
	}

	//Check trigger repetition or excessive proximity
	for (i = 0; i < num_triggers; i++) {
		int tx = ctx->triggers[i].x;

		if (tx == x || tx > x - 28) {
			invalid = true;
			return;
		}
	}

	ctx->triggers[num_triggers].x = x;
	ctx->triggers[num_triggers].what = what;

	num_triggers++;

	if (what != TRIGGER_HEN) {
		num_car_triggers++;
	}
}

static void validate_positions()
{
	int i;

	if (invalid) {
		return;
	}

	//Ensure every respawn point is at a valid Y position and close enough
	//to the corresponding deep hole but not placed after it or over another
	//deep hole
	for (i = 0; i < num_respawn_points; i++) {
		int x = ctx->respawn_points[i].x;
		int y = ctx->respawn_points[i].y;
		int type = ctx->level_columns[x].type;

		if (y != 9 - ctx->level_columns[x].num_crates) {
			invalid = true;
			return;
		}

		if (type != LVLCOL_NORMAL_FLOOR && type != LVLCOL_DEEP_HOLE_RIGHT) {
			invalid = true;
			return;
		}

		if (ctx->level_columns[x + 1].type == LVLCOL_DEEP_HOLE_LEFT) continue;
		if (ctx->level_columns[x + 2].type == LVLCOL_DEEP_HOLE_LEFT) continue;

		invalid = true;
		return;
	}

	//Validate object positions
	for (i = 0; i < num_objs; i++) {
		int x = ctx->objs[i].x;
		int y = ctx->objs[i].y;
		int j;

		int col_type = ctx->level_columns[x].type;
		int col_num_crates = ctx->level_columns[x].num_crates;

		if (y == 11) {
			//Middle of the floor
			invalid = true;
			return;
		}

		if (y > 11) {
			//An object's Y position can be greater than 11 only if it is in a
			//passageway
			if (col_type != LVLCOL_PASSAGEWAY_LEFT
					&& col_type != LVLCOL_PASSAGEWAY_MIDDLE
					&& col_type != LVLCOL_PASSAGEWAY_RIGHT) {

				invalid = true;
				return;
			}
		}

		switch (ctx->objs[i].type) {
			case OBJ_BANANA_PEEL:
				if (y != 14 && y != 10 - col_num_crates) {
					invalid = true;
					return;
				}

				break;

			case OBJ_COIN_SILVER:
			case OBJ_COIN_GOLD:
				if (y < 3) {
					invalid = true;
					return;
				}

				if (y < 11 && y > 10 - col_num_crates) {
					invalid = true;
					return;
				}

				break;

			case OBJ_GUSH:
			case OBJ_GUSH_CRACK:
			case OBJ_HYDRANT:
				if (col_num_crates != 0) {
					invalid = true;
					return;
				}

				if (col_type != LVLCOL_NORMAL_FLOOR) {
					invalid = true;
					return;
				}

				break;

			case OBJ_OVERHEAD_SIGN:
				if (y > 4) {
					invalid = true;
					return;
				}

				break;

			case OBJ_PARKED_CAR_BLUE:
			case OBJ_PARKED_CAR_SILVER:
			case OBJ_PARKED_CAR_YELLOW:
				for (j = 0; j < 6; j++) {
					col_type = ctx->level_columns[x + j].type;
					col_num_crates = ctx->level_columns[x + j].num_crates;

					if (col_num_crates > 0) {
						invalid = true;
						return;
					}

					if (col_type != LVLCOL_NORMAL_FLOOR
							&& col_type != LVLCOL_PASSAGEWAY_MIDDLE) {

						invalid = true;
						return;
					}
				}

				break;

			case OBJ_PARKED_TRUCK:
				for (j = 0; j < 12; j++) {
					col_type = ctx->level_columns[x + j].type;
					col_num_crates = ctx->level_columns[x + j].num_crates;

					if (col_num_crates > 0) {
						invalid = true;
						return;
					}

					if (col_type != LVLCOL_NORMAL_FLOOR
							&& col_type != LVLCOL_PASSAGEWAY_MIDDLE) {

						invalid = true;
						return;
					}
				}

				break;

			case OBJ_ROPE_HORIZONTAL:
				//Check if the X position corresponds to a light pole
				if (x % 16 != 0) {
					invalid = true;
					return;
				}

				break;

			case OBJ_SPRING:
				if (y == 10) {
					if (col_num_crates != 0) {
						invalid = true;
						return;
					}

					if (col_type != LVLCOL_NORMAL_FLOOR
							&& col_type != LVLCOL_PASSAGEWAY_MIDDLE) {

						invalid = true;
						return;
					}
				} else if (y == 14) {
					if (col_type != LVLCOL_PASSAGEWAY_RIGHT) {
						invalid = true;
						return;
					}
				}

				break;
		}
	}
}

//Convert positions (and also the width in the case of passageways) from level
//blocks to pixels
static void convert_positions()
{
	int i;

	if (invalid) {
		return;
	}

	//Convert positions of objects in ctx->objs[]
	for (i = 0; i < num_objs; i++) {
		int x = ctx->objs[i].x * LEVEL_BLOCK_SIZE;
		int y = ctx->objs[i].y;

		if (y != NONE) {
			y *= LEVEL_BLOCK_SIZE;
		}

		switch (ctx->objs[i].type) {
			case OBJ_BANANA_PEEL:
				x += 16;
				y += 16;
				break;

			case OBJ_PARKED_CAR_BLUE:
			case OBJ_PARKED_CAR_SILVER:
			case OBJ_PARKED_CAR_YELLOW:
				y = PARKED_CAR_Y;
				break;

			case OBJ_COIN_SILVER:
			case OBJ_COIN_GOLD:
				x += 8;
				break;

			case OBJ_CRATE_PUSHABLE:
				y = PUSHABLE_CRATE_Y;
				break;

			case OBJ_GUSH:
				y = GUSH_INITIAL_Y;
				break;

			case OBJ_GUSH_CRACK:
				y = GUSH_CRACK_Y;
				break;

			case OBJ_HYDRANT:
				y = HYDRANT_Y;
				break;

			case OBJ_OVERHEAD_SIGN:
				y += 16;
				break;

			case OBJ_ROPE_HORIZONTAL:
				x += 10;
				y = ROPE_Y;
				break;

			case OBJ_ROPE_VERTICAL:
				x += 32;
				y = ROPE_Y + 5;
				break;

			case OBJ_SPRING:
				x += 8;
				y += 8;
				break;

			case OBJ_PARKED_TRUCK:
				y = PARKED_TRUCK_Y;
				break;
		}

		//Apply converted position
		ctx->objs[i].x = x;
		ctx->objs[i].y = y;
	}

	//Convert respawn point positions
	for (i = 0; i < num_respawn_points; i++) {
		ctx->respawn_points[i].x *= LEVEL_BLOCK_SIZE;
		ctx->respawn_points[i].x += 3;

		ctx->respawn_points[i].y *= LEVEL_BLOCK_SIZE;
		ctx->respawn_points[i].y -= 12;
	}

	//Convert trigger positions
	for (i = 0; i < num_triggers; i++) {
		ctx->triggers[i].x *= LEVEL_BLOCK_SIZE;
	}

	//Convert passageway positions and widths
	for (i = 0; i < num_passageways; i++) {
		ctx->passageways[i].x *= LEVEL_BLOCK_SIZE;
		ctx->passageways[i].width *= LEVEL_BLOCK_SIZE;
	}
}

static int add_solid(int type, int x, int y, int width, int height)
{
	if (invalid) {
		return -1;
	}

	//Check if there are too many solids
	if (num_solids >= MAX_SOLIDS) {
		invalid = true;
		return -1;
	}

	ctx->solids[num_solids].type = type;
	ctx->solids[num_solids].left = x;
	ctx->solids[num_solids].right = x + width;
	ctx->solids[num_solids].top = y;
	ctx->solids[num_solids].bottom = y + height;
	num_solids++;

	return num_solids - 1;
}

static void add_solids()
{
	int num_level_columns = (ctx->level_size / LEVEL_BLOCK_SIZE);
	int i;

	if (invalid) {
		return;
	}

	//Add first floor solid
	add_solid(SOL_FULL, 0, 264, LEVEL_BLOCK_SIZE, 80);

	//Add other floor solids
	for (i = 1; i < num_level_columns; i++) {
		int x;

		switch (ctx->level_columns[i].type) {
			case LVLCOL_NORMAL_FLOOR:
				ctx->solids[num_solids - 1].right += LEVEL_BLOCK_SIZE;
				break;

			case LVLCOL_DEEP_HOLE_LEFT:
				ctx->solids[num_solids - 1].right += 12;
				break;

			case LVLCOL_DEEP_HOLE_RIGHT:
				x = (LEVEL_BLOCK_SIZE * i) + 14;
				add_solid(SOL_FULL, x, 264, 10, 80);
				break;

			case LVLCOL_PASSAGEWAY_LEFT:
				ctx->solids[num_solids - 1].right += 6;
				break;

			case LVLCOL_PASSAGEWAY_RIGHT:
				x = (LEVEL_BLOCK_SIZE * (i + 1));
				add_solid(SOL_FULL, x, 264, 0, 80);
				break;
		}

		//Too many solids
		if (invalid) {
			return;
		}
	}

	//Add passageway solids
	for (i = 0; i < num_passageways; i++) {
		int x = ctx->passageways[i].x;
		int w = ctx->passageways[i].width;

		//Bottom solid
		add_solid(SOL_FULL, x + 8, 360, w - 8, 4);

		//Passageway entry solid
		add_solid(SOL_PASSAGEWAY_ENTRY, x + 6, 264, 18, 13);

		//Top floor solid
		x += LEVEL_BLOCK_SIZE;
		w -= (LEVEL_BLOCK_SIZE * 2);
		add_solid(SOL_FULL, x, 264, w, 13);

		//Passageway exit solid
		x += w;
		add_solid(SOL_PASSAGEWAY_EXIT, x, 264, 22, 13);
	}

	//Add solids for unpushable crates
	for (i = 1; i < num_level_columns; i++) {
		int num_crates = ctx->level_columns[i].num_crates;
		int num_crates_prev = ctx->level_columns[i - 1].num_crates;

		if (num_crates == 0) continue;

		if (num_crates == num_crates_prev) {
			//If multiple consecutive level columns share the same number of
			//crates, just extend the previous solid instead of adding a new one
			ctx->solids[num_solids - 1].right += LEVEL_BLOCK_SIZE;
		} else {
			int x = i * LEVEL_BLOCK_SIZE;
			int y = (11 - num_crates) * LEVEL_BLOCK_SIZE;
			int w = LEVEL_BLOCK_SIZE;
			int h = num_crates * LEVEL_BLOCK_SIZE;

			add_solid(SOL_FULL, x, y, w, h);

			//Too many solids
			if (invalid) {
				return;
			}
		}
	}

	//Add solids for pushable crates (there is exactly one pushable crate for
	//each passageway)
	for (i = 0; i < num_passageways; i++) {
		int x = (int)ctx->pushable_crates[i].x;
		int y = PUSHABLE_CRATE_Y;
		int w = LEVEL_BLOCK_SIZE;
		int h = LEVEL_BLOCK_SIZE;

		ctx->pushable_crates[i].solid = add_solid(SOL_FULL, x, y, w, h);

		//Too many solids
		if (invalid) {
			return;
		}
	}

	//Add solids for objects in ctx->objs[] (except pushable crates)
	for (i = 0; i < num_objs; i++) {
		int x = ctx->objs[i].x;
		int y = ctx->objs[i].y;

		switch (ctx->objs[i].type) {
			case OBJ_HYDRANT:
				add_solid(SOL_FULL, x + 4, y + 8, 8, 4);
				break;

			case OBJ_OVERHEAD_SIGN:
				add_solid(SOL_FULL, x + 12, y, 4, 32);
				break;

			case OBJ_PARKED_CAR_BLUE:
			case OBJ_PARKED_CAR_SILVER:
			case OBJ_PARKED_CAR_YELLOW:
				add_solid(SOL_FULL, x + 4, y + 18, 20, 4);
				add_solid(SOL_SLOPE_UP, x + 27, y + 2, 15, 15);
				add_solid(SOL_VERTICAL, x + 48, y + 2, 16, 4);
				add_solid(SOL_SLOPE_DOWN, x + 66, y + 2, 18, 18);
				add_solid(SOL_KEEP_ON_TOP, x + 88, y + 20, 16, 4);
				add_solid(SOL_KEEP_ON_TOP, x + 104, y + 22, 16, 4);
				add_solid(SOL_FULL, x + 120, y + 24, 8, 4);
				break;

			case OBJ_PARKED_TRUCK:
				add_solid(SOL_FULL, x, y + 4, 224, 96);
				add_solid(SOL_FULL, x + 224, y + 23, 55, 80);
				break;
		}

		//Too many solids
		if (invalid) {
			return;
		}
	}
}

