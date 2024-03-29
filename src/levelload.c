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
static int num_holes, num_deep_holes, num_passageways;
static int num_respawn_points;
static int num_triggers, num_car_triggers;

//------------------------------------------------------------------------------

//Function prototypes
static void add_obj(int type, int x, int y, bool use_y);
static void add_crate_block(int x, int y, int w, int h);
static void add_hole(int type, int x, int w);
static void add_respawn_point(int x, int y);
static void add_trigger(int x, int what);
static int add_solid(int type, int x, int y, int width, int height);

//------------------------------------------------------------------------------

void levelload_init(PlayCtx* pctx)
{
	ctx = pctx;
}

int levelload_load(const char* filename)
{
	char tmp[48];
	bool no_objects = true;
	int x, y, w, h;
	int i, j;

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
	num_holes = 0;
	num_deep_holes = 0;
	num_passageways = 0;
	num_respawn_points = 0;
	num_triggers = 0;
	num_car_triggers = 0;

	ctx->level_size = NONE;
	ctx->bg_color = NONE;
	ctx->bgm = NONE;

	x = VSCREEN_MAX_WIDTH_LEVEL_BLOCKS;

	while (!lineread_ended()) {
		int num_tokens;
		int token1, token2, token3, token4;

		lineread_getline(tmp);

		if (lineread_invalid()) {
			return LVLERR_INVALID;
		}

		//Skip blank lines
		if (tmp[0] == '\0') {
			continue;
		}

		num_tokens = lineread_num_tokens(tmp);
		if (num_tokens < 2 || num_tokens > 5) {
			return LVLERR_INVALID;
		}

		token1 = lineread_token_int(tmp, 1);
		token2 = lineread_token_int(tmp, 2);
		token3 = lineread_token_int(tmp, 3);
		token4 = lineread_token_int(tmp, 4);

		if (token1 == NONE) {
			return LVLERR_INVALID;
		}

		if (str_starts_with(tmp, "level-size ")) {
			//Error: level size redefinition
			if (ctx->level_size != NONE) {
				return LVLERR_INVALID;
			}

			//Error: size out of the allowed range
			if (token1 < 8 || token1 > 32) {
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
		}

		//Error: adding objects without defining the level size, sky color,
		//and BGM
		if (ctx->level_size == NONE || ctx->bg_color == NONE || ctx->bgm == NONE) {
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
			add_crate_block(x, token2, token3, token4);
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
			add_obj(OBJ_SPRING, x, token2, true);
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
			add_hole(HOLE_DEEP, x, token2);
		} else if (str_starts_with(tmp, "passageway ") ||
					str_starts_with(tmp, "passageway-arrow ")) {

			add_hole(HOLE_PASSAGEWAY_EXIT_CLOSED, x, token2);

			//Pushable crate over passageway entry
			add_obj(OBJ_CRATE_PUSHABLE, x, NONE, false);
			ctx->pushable_crates[num_passageways - 1].obj = num_objs - 1;

			//Determine whether or not to show an arrow near the crate
			ctx->pushable_crates[num_passageways - 1].show_arrow = false;
			if (str_starts_with(tmp, "passageway-arrow ")) {
				ctx->pushable_crates[num_passageways - 1].show_arrow = true;
			}
		} else {
			//Error: invalid object type
			return LVLERR_INVALID;
		}

		if (invalid) {
			return LVLERR_INVALID;
		}

		no_objects = false;
	}

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

	//Ensure every respawn point is close enough to the corresponding deep
	//hole but not placed after it or over another deep hole
	for (i = 0, j = 0; i < num_holes; i++) {
		int hx = ctx->holes[i].x;
		int rx = ctx->respawn_points[j].x;

		//Skip passageways
		if (ctx->holes[i].type != HOLE_DEEP) {
			continue;
		}

		if (rx >= hx || rx < hx - 4) {
			return LVLERR_INVALID;
		}

		if (i > 1) {
			int prev_hole = i - 1;

			//Find previous deep hole (skip passageways)
			while (prev_hole >= 0 && ctx->holes[prev_hole].type != HOLE_DEEP) {
				prev_hole--;
			}

			if (prev_hole >= 0) {
				//Right side of previous deep hole
				hx = ctx->holes[prev_hole].x + ctx->holes[prev_hole].width - 2;

				if (rx <= hx) {
					return LVLERR_INVALID;
				}
			}
		}

		j++;
	}

	//First solid for the floor
	add_solid(SOL_FULL, 0, FLOOR_Y, ctx->level_size, 80);

	//Convert positions from level blocks to pixels for deep holes and
	//passageways and adjust solids around them
	for (i = 0; i < num_holes; i++) {
		int prev_sol_right, sol_left, sol_width;
		bool is_deep = (ctx->holes[i].type == HOLE_DEEP);

		x = ctx->holes[i].x;
		w = ctx->holes[i].width;

		prev_sol_right = x * LEVEL_BLOCK_SIZE;
		sol_left = (x + w) * LEVEL_BLOCK_SIZE;
		sol_width = ctx->level_size - sol_left;

		if (is_deep) { //Deep hole
			prev_sol_right += 12;
			sol_left -= 8;
		} else { //Passageway
			prev_sol_right += 6;
		}

		//Adjust the previous floor solid so that it does not cover the hole
		ctx->solids[i].right = prev_sol_right;

		//Add solid for the floor after the hole
		add_solid(SOL_FULL, sol_left, FLOOR_Y, sol_width, 80);

		//Too many solids
		if (invalid) {
			return LVLERR_INVALID;
		}

		//Convert position
		ctx->holes[i].x *= LEVEL_BLOCK_SIZE;
	}

	//Convert positions from level blocks to pixels and add solids for
	//objects in ctx->objs[]
	for (i = 0; i < num_objs; i++) {
		x = ctx->objs[i].x * LEVEL_BLOCK_SIZE;
		y = ctx->objs[i].y;
		if (y != NONE) {
			y *= LEVEL_BLOCK_SIZE;
		}

		switch (ctx->objs[i].type) {
			case OBJ_BANANA_PEEL:
				x += 16;
				y -= 8;
				break;

			case OBJ_PARKED_CAR_BLUE:
			case OBJ_PARKED_CAR_SILVER:
			case OBJ_PARKED_CAR_YELLOW:
				y = PARKED_CAR_Y;
				add_solid(SOL_FULL, x + 4, y + 18, 20, 4);
				add_solid(SOL_SLOPE_UP, x + 27, y + 2, 15, 15);
				add_solid(SOL_VERTICAL, x + 48, y + 2, 16, 4);
				add_solid(SOL_SLOPE_DOWN, x + 66, y + 2, 18, 18);
				add_solid(SOL_KEEP_ON_TOP, x + 88, y + 20, 16, 4);
				add_solid(SOL_KEEP_ON_TOP, x + 104, y + 22, 16, 4);
				add_solid(SOL_FULL, x + 120, y + 24, 8, 4);
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
				add_solid(SOL_FULL, x + 4, y + 8, 8, 4);
				break;

			case OBJ_OVERHEAD_SIGN:
				y -= 8;
				add_solid(SOL_FULL, x + 12, y, 4, 32);
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
				add_solid(SOL_FULL, x, y + 4, 224, 96);
				add_solid(SOL_FULL, x + 224, y + 23, 55, 80);
				break;
		}

		//Too many solids
		if (invalid) {
			return LVLERR_INVALID;
		}

		//Apply converted position
		ctx->objs[i].x = x;
		ctx->objs[i].y = y;
	}

	//Set properties for ctx->pushable_crates[]
	for (i = 0; i < num_passageways; i++) {
		int obj = ctx->pushable_crates[i].obj;
		x = ctx->objs[obj].x;
		ctx->pushable_crates[i].x = x;
		ctx->pushable_crates[i].xmax = x + LEVEL_BLOCK_SIZE;
	}

	//Convert positions from level blocks to pixels and add solids for crate
	//blocks
	for (i = 0; i < num_crate_blocks; i++) {
		x = ctx->crate_blocks[i].x * LEVEL_BLOCK_SIZE;
		y = ctx->crate_blocks[i].y * LEVEL_BLOCK_SIZE;
		w = ctx->crate_blocks[i].width * LEVEL_BLOCK_SIZE;
		h = ctx->crate_blocks[i].height * LEVEL_BLOCK_SIZE;
		add_solid(SOL_FULL, x, y, w, h);

		//Too many solids
		if (invalid) {
			return LVLERR_INVALID;
		}

		//Apply converted position
		ctx->crate_blocks[i].x = x;
		ctx->crate_blocks[i].y = y;
	}

	//Convert respawn point positions from level blocks to pixels
	for (i = 0; i < num_respawn_points; i++) {
		ctx->respawn_points[i].x *= LEVEL_BLOCK_SIZE;
		ctx->respawn_points[i].x += 3;

		ctx->respawn_points[i].y *= LEVEL_BLOCK_SIZE;
		ctx->respawn_points[i].y -= 12;
	}

	//Convert trigger positions from level blocks to pixels
	for (i = 0; i < num_triggers; i++) {
		ctx->triggers[i].x *= LEVEL_BLOCK_SIZE;
	}

	//Add solids for passageways and pushable crates over passageway entries
	//
	//There is exactly one pushable crate for each passageway
	for (i = 0, j = 0; i < num_holes; i++) {
		int sol;

		if (ctx->holes[i].type == HOLE_DEEP) {
			continue;
		}

		x = ctx->holes[i].x;
		w = ctx->holes[i].width * LEVEL_BLOCK_SIZE;

		//Bottom solid
		add_solid(SOL_FULL, x, PASSAGEWAY_BOTTOM_Y, w, 4);

		//Top solid
		add_solid(SOL_FULL, x + LEVEL_BLOCK_SIZE, FLOOR_Y, w - 46, 13);

		//Entry and exit solids, which prevent the player character from
		//leaving the passageway through the entry or entering it through
		//the exit
		add_solid(SOL_PASSAGEWAY_ENTRY, x + 6, FLOOR_Y, 18, 13);
		add_solid(SOL_PASSAGEWAY_EXIT, x + w - 22, FLOOR_Y, 22, 13);

		ctx->holes[i].x = x;

		//Pushable crate solid
		x = (int)ctx->pushable_crates[j].x;
		y = PUSHABLE_CRATE_Y;
		sol = add_solid(SOL_FULL, x, y, CRATE_WIDTH, CRATE_HEIGHT);
		ctx->pushable_crates[j].solid = sol;
		j++;

		//Too many solids
		if (invalid) {
			return LVLERR_INVALID;
		}
	}

	return LVLERR_NONE;
}

//------------------------------------------------------------------------------

static void add_obj(int type, int x, int y, bool use_y)
{
	int i;

	//Check if there are too many objects
	if (num_objs >= MAX_OBJS) {
		invalid = true;
		return;
	}

	//Check if the object's position is within the allowed range
	if (y > 15 || (y != NONE && y < 3) || (use_y && y == NONE) || x > x_max) {
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

static void add_crate_block(int x, int y, int w, int h)
{
	int x2 = x + w - 1;
	int y2 = y + h - 1;
	int i;

	//Check if there are too many crate blocks
	if (num_crate_blocks >= MAX_CRATE_BLOCKS) {
		invalid = true;
		return;
	}

	//Check if the crate block's position is within the allowed range
	if (x > x_max - 2 || y < 3 || y > 15) {
		invalid = true;
		return;
	}

	//Check if the crate block's size is within the allowed range
	if (w < 1 || w > 4 || h < 1 || h > 5) {
		invalid = true;
		return;
	}

	if (x2 > x_max - 2) {
		//Error: crate block width extends beyond or too close to level's
		//right boundary
		invalid = true;
		return;
	}

	//Check crate block repetition or overlap
	for (i = 0; i < num_crate_blocks; i++) {
		CrateBlock ct = ctx->crate_blocks[i];
		int cx2 = ct.x + ct.width - 1;
		int cy2 = ct.y + ct.height - 1;

		if (ct.x == x && ct.y == y) {
			invalid = true;
			return;
		}

		if (x <= cx2 && y2 >= ct.y && y <= cy2) {
			invalid = true;
			return;
		}
	}

	ctx->crate_blocks[num_crate_blocks].x = x;
	ctx->crate_blocks[num_crate_blocks].y = y;
	ctx->crate_blocks[num_crate_blocks].width = w;
	ctx->crate_blocks[num_crate_blocks].height = h;

	num_crate_blocks++;
}

//Adds a deep hole or a passageway
static void add_hole(int type, int x, int w)
{
	int x2 = x + w - 1;
	int max_width = (type == HOLE_DEEP) ? 16 : 32;
	int i;

	//Check if there are too many holes
	if (num_holes >= MAX_HOLES) {
		invalid = true;
		return;
	}

	//Check if the hole's position and size are within the allowed range
	if (x > x_max - 2 || w < 2 || w > max_width) {
		invalid = true;
		return;
	}

	if (x2 > x_max - 2) {
		//Error: hole width extends beyond or too close to level's right
		//boundary
		invalid = true;
		return;
	}

	//Check hole repetition or overlap
	for (i = 0; i < num_holes; i++) {
		int hx1 = ctx->holes[i].x;
		int hx2 = hx1 + ctx->holes[i].width - 1;

		if (hx1 == x || x <= hx2) {
			invalid = true;
			return;
		}
	}

	ctx->holes[num_holes].type = type;
	ctx->holes[num_holes].x = x;
	ctx->holes[num_holes].width = w;

	if (type == HOLE_DEEP) {
		num_deep_holes++;
	} else {
		num_passageways++;

		//Too many passageways
		if (num_passageways > MAX_PASSAGEWAYS) {
			invalid = true;
			return;
		}
	}

	num_holes++;
}

static void add_respawn_point(int x, int y)
{
	int i;

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

static int add_solid(int type, int x, int y, int width, int height)
{
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

