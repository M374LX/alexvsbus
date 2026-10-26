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
extern const int data_gush_move_pattern_2[];

//------------------------------------------------------------------------------

static PlayCtx* ctx;

static bool invalid;

static int x_max;
static int num_banana_peels;
static int num_coins;
static int num_gushes;
static int num_solids;
static int num_deep_holes;
static int num_passageways;
static int num_pushable_crates;
static int num_overhead_signs;
static int num_parked_vehicles;
static int num_respawn_points;
static int num_ropes;
static int num_springs;
static int num_triggers;
static int num_car_triggers;
static int num_entries;

static LevelEntry entries[MAX_LEVEL_ENTRIES];

//------------------------------------------------------------------------------

//Function prototypes
static void add_level_entry(int type, int x, int y, bool use_y);
static void add_crates(int x, int w, int h);
static void add_deep_hole(int x, int w);
static void add_horizontal_rope(int x);
static void add_hydrant(int x);
static void add_parked_vehicle(int type, int x);
static void add_passageway(int x, int y);
static void add_respawn_point(int x, int y);
static void add_trigger(int x, int what);
static void validate_level_columns();
static void validate_positions();
static void convert_positions();
static void add_objects_to_lists();
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
	bool no_entries = true;
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

	x_max = 0;
	num_banana_peels = 0;
	num_coins = 0;
	num_gushes = 0;
	num_solids = 0;
	num_deep_holes = 0;
	num_overhead_signs = 0;
	num_parked_vehicles = 0;
	num_passageways = 0;
	num_pushable_crates = 0;
	num_respawn_points = 0;
	num_ropes = 0;
	num_springs = 0;
	num_triggers = 0;
	num_car_triggers = 0;
	num_entries = 0;

	ctx->level_size_pixels = 0;
	ctx->sky_color = 0;
	ctx->bgm = 0;
	ctx->goal_scene = 0;

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

		if (token1 < 0) {
			return LVLERR_INVALID;
		}

		if (str_starts_with(tmp, "level-size ")) {
			//Error: level size redefinition
			if (ctx->level_size_pixels != 0) {
				return LVLERR_INVALID;
			}

			//Error: size out of the allowed range
			if (token1 < 8 || token1 > 24) {
				return LVLERR_INVALID;
			}

			//Just before the last screen
			x_max = (token1 - 1) * VSCREEN_MAX_WIDTH_LEVEL_BLOCKS;

			ctx->level_size_pixels = token1 * VSCREEN_MAX_WIDTH;
			ctx->num_level_columns = ctx->level_size_pixels / LEVEL_BLOCK_SIZE;

			continue;
		} else if (str_starts_with(tmp, "sky-color ")) {
			//Error: sky color redefinition
			if (ctx->sky_color != 0) {
				return LVLERR_INVALID;
			}

			switch (token1) {
				case 1:  ctx->sky_color = SPR_BG_SKY1; break;
				case 2:  ctx->sky_color = SPR_BG_SKY2; break;
				case 3:  ctx->sky_color = SPR_BG_SKY3; break;
				default: return LVLERR_INVALID;
			}

			continue;
		} else if (str_starts_with(tmp, "bgm ")) {
			//Error: BGM redefinition
			if (ctx->bgm != 0) {
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
			if (ctx->goal_scene != 0) {
				return LVLERR_INVALID;
			}

			//Error: invalid goal cutscene number
			if (token1 < 1 || token1 > 5) {
				return LVLERR_INVALID;
			}

			ctx->goal_scene = token1;

			continue;
		}

		//Error: adding entries without defining the level size, sky color,
		//BGM track, and goal cutscene
		if (ctx->level_size_pixels == 0 || ctx->sky_color == 0 ||
				ctx->bgm == 0 || ctx->goal_scene == 0) {

			return LVLERR_INVALID;
		}

		//The value of token1 is relative to the previous X position
		x += token1;

		if (str_starts_with(tmp, "banana-peel ")) {
			add_level_entry(LVLENTRY_BANANA_PEEL, x, token2, true);
		} else if (str_starts_with(tmp, "car-blue ")) {
			add_parked_vehicle(PARKED_CAR_BLUE, x);
		} else if (str_starts_with(tmp, "car-silver ")) {
			add_parked_vehicle(PARKED_CAR_SILVER, x);
		} else if (str_starts_with(tmp, "car-yellow ")) {
			add_parked_vehicle(PARKED_CAR_YELLOW, x);
		} else if (str_starts_with(tmp, "coin-silver ")) {
			add_level_entry(LVLENTRY_COIN_SILVER, x, token2, true);
		} else if (str_starts_with(tmp, "coin-gold ")) {
			add_level_entry(LVLENTRY_COIN_GOLD, x, token2, true);
		} else if (str_starts_with(tmp, "crates ")) {
			add_crates(x, token2, token3);
		} else if (str_starts_with(tmp, "gush ")) {
			add_level_entry(LVLENTRY_GUSH, x, -1, false);

			if (num_gushes >= MAX_GUSHES) {
				return LVLERR_INVALID;
			}
		} else if (str_starts_with(tmp, "gush-crack ")) {
			add_level_entry(LVLENTRY_GUSH_CRACK, x, -1, false);
		} else if (str_starts_with(tmp, "hydrant ")) {
			add_hydrant(x);
		} else if (str_starts_with(tmp, "overhead-sign ")) {
			add_level_entry(LVLENTRY_OVERHEAD_SIGN, x, token2, true);
		} else if (str_starts_with(tmp, "rope ")) {
			add_horizontal_rope(x);
			add_level_entry(LVLENTRY_ROPE, x, -1, false);
		} else if (str_starts_with(tmp, "spring ")) {
			//10 is the Y position corresponding to the floor
			add_level_entry(LVLENTRY_SPRING, x, 10, true);
		} else if (str_starts_with(tmp, "truck ")) {
			add_parked_vehicle(PARKED_TRUCK, x);
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
			add_level_entry(LVLENTRY_SPRING, x + token2 - 1, 14, true);

			//Pushable crate over passageway entry
			add_level_entry(LVLENTRY_PUSHABLE_CRATE, x, -1, false);
			num_pushable_crates++;
		} else if (str_starts_with(tmp, "passageway-arrow ")) {
			add_passageway(x, token2);

			//Spring under passageway exit (14 is the Y position corresponding
			//to a passageway bottom)
			add_level_entry(LVLENTRY_SPRING, x + token2 - 1, 14, true);

			//Pushable crate over passageway entry
			add_level_entry(LVLENTRY_PUSHABLE_CRATE, x, -1, false);
			ctx->pushable_crates[num_pushable_crates].show_arrow = true;
			num_pushable_crates++;
		} else {
			//Error: invalid entry
			return LVLERR_INVALID;
		}

		if (invalid) {
			return LVLERR_INVALID;
		}

		no_entries = false;
	}

	//Error: no entries
	if (no_entries) {
		return LVLERR_INVALID;
	}

	//Error: not enough slots for all banana peels, which can be present in the
	//level or thrown from cars
	if (num_banana_peels + num_car_triggers > MAX_BANANA_PEELS) {
		return LVLERR_INVALID;
	}

	//Error: the number of respawn points is not the same as the number of
	//deep holes
	if (num_respawn_points != num_deep_holes) {
		return LVLERR_INVALID;
	}

	//Ensure there are no unpushable crates on deep holes or passageway edges
	for (i = 0; i < MAX_LEVEL_COLUMNS; i++) {
		int hole_type = ctx->level_columns[i].hole_type;
		int num_crates = ctx->level_columns[i].num_crates;

		if (hole_type != HOLE_NONE && hole_type != HOLE_PASSAGEWAY_MIDDLE) {
			if (num_crates > 0) {
				return LVLERR_INVALID;
			}
		}
	}

	validate_level_columns();
	validate_positions();
	convert_positions();
	add_objects_to_lists();
	add_solids();

	if (invalid) {
		return LVLERR_INVALID;
	}

	ctx->num_banana_peels = num_banana_peels;
	ctx->num_coins = num_coins;
	ctx->num_gushes = num_gushes;
	ctx->num_overhead_signs = num_overhead_signs;
	ctx->num_parked_vehicles = num_parked_vehicles;
	ctx->num_respawn_points = num_respawn_points;
	ctx->num_ropes = num_ropes;
	ctx->num_solids = num_solids;
	ctx->num_springs = num_springs;
	ctx->num_triggers = num_triggers;

	return LVLERR_NONE;
}

//------------------------------------------------------------------------------

static void add_level_entry(int type, int x, int y, bool use_y)
{
	int i;

	if (invalid) {
		return;
	}

	//Check if there are too many entries
	if (num_entries >= MAX_LEVEL_ENTRIES) {
		invalid = true;
		return;
	}

	//Check if the entry's position is within the allowed range
	if (y > 14 || (y != -1 && y < 2) || (use_y && y == -1) || x > x_max) {
		invalid = true;
		return;
	}

	//Check entry repetition
	for (i = 0; i < num_entries; i++) {
		LevelEntry entry = entries[i];

		if (entry.type == type && entry.x == x && entry.y == y) {
			invalid = true;
			return;
		}
	}

	//Increase the number of objects of the corresponding type
	switch (type) {
		case LVLENTRY_BANANA_PEEL:
			num_banana_peels++;
			break;

		case LVLENTRY_COIN_SILVER:
		case LVLENTRY_COIN_GOLD:
			num_coins++;
			break;

		case LVLENTRY_GUSH:
		case LVLENTRY_GUSH_CRACK:
			num_gushes++;
			break;

		case LVLENTRY_OVERHEAD_SIGN:
			num_overhead_signs++;
			break;

		case LVLENTRY_PUSHABLE_CRATE:
			num_pushable_crates++;
			break;

		case LVLENTRY_ROPE:
			num_ropes++;
			break;

		case LVLENTRY_SPRING:
			num_springs++;
			break;
	}

	//Check if the number of objects of different types is greater than the
	//maximum
	if (num_banana_peels > MAX_BANANA_PEELS
			|| num_coins > MAX_COINS
			|| num_gushes > MAX_GUSHES
			|| num_overhead_signs > MAX_OVERHEAD_SIGNS
			|| num_pushable_crates > MAX_PUSHABLE_CRATES
			|| num_ropes > MAX_ROPES
			|| num_springs > MAX_SPRINGS) {

		invalid = true;
		return;
	}

	entries[num_entries].type = type;
	entries[num_entries].x = x;
	entries[num_entries].y = y;
	num_entries++;
}

//Add a set of unpushable crates
static void add_crates(int x, int w, int h)
{
	int i;

	if (invalid) {
		return;
	}

	//Check if the size of the set is within the allowed range
	if (w < 1 || w > 4 || h < 1 || h > 5) {
		invalid = true;
		return;
	}

	//Check if the position of the set is within the allowed range
	if (x > x_max - 4) {
		invalid = true;
		return;
	}

	if (x + w > x_max - 4) {
		//Error: crate set width extends beyond or too close to level's
		//right boundary
		invalid = true;
		return;
	}

	if (ctx->level_columns[x - 1].num_crates == h) {
		//Error: the set is adjacent to another set of crates with the
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
		int hole_type;

		//Check if the deep hole is being added to a level column that already
		//has a deep hole or passageway
		if (ctx->level_columns[x + i].hole_type != HOLE_NONE) {
			invalid = true;
			return;
		}

		hole_type = HOLE_DEEP_MIDDLE;
		if (i == 0) {
			hole_type = HOLE_DEEP_LEFT;
		} else if (i == w - 1) {
			hole_type = HOLE_DEEP_RIGHT;
		}

		//Insert deep hole into level columns
		ctx->level_columns[x + i].hole_type = hole_type;
	}

	num_deep_holes++;
}

static void add_horizontal_rope(int x)
{
	int i;

	//Check if the rope is too far to the right
	if (x > x_max - 32) {
		invalid = true;
	}

	//Check if the position corresponds to a light pole
	if (x % 16 != 0) {
		invalid = true;
	}

	//Check if there is already a rope at the position
	if (ctx->level_columns[x + 1].has_rope) {
		invalid = true;
	}

	if (invalid) {
		return;
	}

	for (i = 0; i < 16; i++) {
		ctx->level_columns[x + i + 1].has_rope = true;
	}
}

static void add_hydrant(int x)
{
	//Check if the hydrant is too far to the right
	if (x > x_max - 4) {
		invalid = true;
	}

	//Check if a hydrant is already there
	if (ctx->level_columns[x].has_hydrant) {
		invalid = true;
	}

	if (invalid) {
		return;
	}

	ctx->level_columns[x].has_hydrant = true;
}

static void add_parked_vehicle(int type, int x)
{
	if (num_parked_vehicles >= MAX_PARKED_VEHICLES) {
		invalid = true;
	}

	if (x > x_max - 8) {
		invalid = true;
	}

	if (invalid) {
		return;
	}

	ctx->parked_vehicles[num_parked_vehicles].type = type;
	ctx->parked_vehicles[num_parked_vehicles].x = x;
	num_parked_vehicles++;
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
		int hole_type;

		//Check if the passageway is being added to a level column that already
		//has a deep hole or passageway
		if (ctx->level_columns[x + i].hole_type != HOLE_NONE) {
			invalid = true;
			return;
		}

		hole_type = HOLE_PASSAGEWAY_MIDDLE;
		if (i == 0) {
			hole_type = HOLE_PASSAGEWAY_LEFT;
		} else if (i == w - 1) {
			hole_type = HOLE_PASSAGEWAY_RIGHT;
		}

		//Insert passageway into level columns
		ctx->level_columns[x + i].hole_type = hole_type;
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

static void validate_level_columns()
{
	int i;

	for (i = 0; i < MAX_LEVEL_COLUMNS; i++) {
		int hole_type = ctx->level_columns[i].hole_type;
		int num_crates = ctx->level_columns[i].num_crates;
		bool has_hydrant = ctx->level_columns[i].has_hydrant;
		bool has_rope = ctx->level_columns[i].has_rope;

		if (has_hydrant) {
			if (has_rope || num_crates != 0 || hole_type != HOLE_NONE) {
				invalid = true;
			}
		}

		if (num_crates > 0) {
			if (hole_type != HOLE_NONE && hole_type != HOLE_PASSAGEWAY_MIDDLE) {
				invalid = true;
			}
		}

		if (invalid) {
			return;
		}
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
		int hole_type = ctx->level_columns[x].hole_type;

		if (y != 9 - ctx->level_columns[x].num_crates) {
			invalid = true;
			return;
		}

		if (hole_type != HOLE_NONE && hole_type != HOLE_DEEP_RIGHT) {
			invalid = true;
			return;
		}

		if (ctx->level_columns[x + 1].hole_type == HOLE_DEEP_LEFT) continue;
		if (ctx->level_columns[x + 2].hole_type == HOLE_DEEP_LEFT) continue;

		invalid = true;
		return;
	}

	//Validate positions of level entries
	for (i = 0; i < num_entries; i++) {
		int x = entries[i].x;
		int y = entries[i].y;

		int col_hole_type  = ctx->level_columns[x].hole_type;
		int col_num_crates = ctx->level_columns[x].num_crates;
		bool col_has_hydrant = ctx->level_columns[x].has_hydrant;
		bool col_has_rope = ctx->level_columns[x].has_rope;

		if (y == 11) {
			//Middle of the floor
			invalid = true;
			return;
		}

		if (y > 11) {
			//An object's Y position can be greater than 11 only if it is in a
			//passageway
			if (col_hole_type != HOLE_PASSAGEWAY_LEFT
					&& col_hole_type != HOLE_PASSAGEWAY_MIDDLE
					&& col_hole_type != HOLE_PASSAGEWAY_RIGHT) {

				invalid = true;
				return;
			}
		}

		if (col_has_rope && entries[i].type != LVLENTRY_ROPE) {
			invalid = true;
			return;
		}

		switch (entries[i].type) {
			case LVLENTRY_BANANA_PEEL:
				if (y != 14) {
					if (y != 10 - col_num_crates) {
						invalid = true;
					}

					if (col_has_hydrant) {
						invalid = true;
					}
				}

				break;

			case LVLENTRY_COIN_SILVER:
			case LVLENTRY_COIN_GOLD:
				if (y < 11) {
					if (y > 10 - col_num_crates) {
						invalid = true;
					}

					if (col_has_hydrant && y > 9) {
						invalid = true;
					}
				}

				break;

			case LVLENTRY_GUSH:
			case LVLENTRY_GUSH_CRACK:
				if (col_num_crates != 0) {
					invalid = true;
				}

				if (col_hole_type != HOLE_NONE) {
					invalid = true;
				}

				if (col_has_hydrant) {
					invalid = true;
				}

				break;

			case LVLENTRY_OVERHEAD_SIGN:
				if (y > 4) {
					invalid = true;
				}

				break;

			case LVLENTRY_SPRING:
				if (y == 10) {
					if (col_num_crates != 0) {
						invalid = true;
					}

					if (col_hole_type != HOLE_NONE
							&& col_hole_type != HOLE_PASSAGEWAY_MIDDLE) {

						invalid = true;
					}

					if (col_has_hydrant) {
						invalid = true;
					}
				} else if (y == 14) {
					if (col_hole_type != HOLE_PASSAGEWAY_RIGHT) {
						invalid = true;
					}
				}

				break;
		}

		if (invalid) {
			return;
		}
	}

	//Validate positions of parked vehicles
	for (i = 0; i < num_parked_vehicles; i++) {
		int x = ctx->parked_vehicles[i].x;
		int w = (ctx->parked_vehicles[i].type == PARKED_TRUCK) ? 12 : 6;
		int j;

		for (j = 0; j < w; j++) {
			int col_hole_type = ctx->level_columns[x + j].hole_type;
			int col_has_crates = (ctx->level_columns[x + j].num_crates > 0);
			bool col_has_hydrant = ctx->level_columns[x + j].has_hydrant;
			bool col_has_rope = ctx->level_columns[x + j].has_rope;

			if (col_has_crates || col_has_hydrant || col_has_rope) {
				invalid = true;
			}

			if (col_hole_type != HOLE_NONE
					&& col_hole_type != HOLE_PASSAGEWAY_MIDDLE) {

				invalid = true;
			}

			if (invalid) {
				return;
			}
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

	//Convert level entry positions
	for (i = 0; i < num_entries; i++) {
		int x = entries[i].x * LEVEL_BLOCK_SIZE;
		int y = entries[i].y;

		if (y >= 0) {
			y *= LEVEL_BLOCK_SIZE;
		}

		switch (entries[i].type) {
			case LVLENTRY_BANANA_PEEL:
				x += 16;
				y += 16;
				break;

			case LVLENTRY_COIN_SILVER:
			case LVLENTRY_COIN_GOLD:
				x += 8;
				break;

			case LVLENTRY_PUSHABLE_CRATE:
				y = PUSHABLE_CRATE_Y;
				break;

			case LVLENTRY_GUSH:
				y = GUSH_INITIAL_Y;
				break;

			case LVLENTRY_GUSH_CRACK:
				y = GUSH_CRACK_Y;
				break;

			case LVLENTRY_OVERHEAD_SIGN:
				y += 16;
				break;

			case LVLENTRY_ROPE:
				x += 32;
				y = ROPE_Y + 5;
				break;

			case LVLENTRY_SPRING:
				x += 8;
				y += 8;
				break;
		}

		//Apply converted position
		entries[i].x = x;
		entries[i].y = y;
	}

	//Convert parked vehicle positions
	for (i = 0; i < num_parked_vehicles; i++) {
		ctx->parked_vehicles[i].x *= LEVEL_BLOCK_SIZE;
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

//Add objects of each type to the corresponding lists
static void add_objects_to_lists()
{
	int i;

	num_banana_peels = 0;
	num_coins = 0;
	num_gushes = 0;
	num_overhead_signs = 0;
	num_pushable_crates = 0;
	num_ropes = 0;
	num_springs = 0;

	for (i = 0; i < num_entries; i++) {
		switch (entries[i].type) {
			case LVLENTRY_BANANA_PEEL:
				ctx->banana_peels[num_banana_peels].x = entries[i].x;
				ctx->banana_peels[num_banana_peels].y = entries[i].y;
				num_banana_peels++;
				break;

			case LVLENTRY_COIN_SILVER:
				ctx->coins[num_coins].x = entries[i].x;
				ctx->coins[num_coins].y = entries[i].y;
				ctx->coins[num_coins].gold = false;
				num_coins++;
				break;

			case LVLENTRY_COIN_GOLD:
				ctx->coins[num_coins].x = entries[i].x;
				ctx->coins[num_coins].y = entries[i].y;
				ctx->coins[num_coins].gold = true;
				num_coins++;
				break;

			case LVLENTRY_GUSH:
				ctx->gushes[num_gushes].x = entries[i].x;
				ctx->gushes[num_gushes].y = GUSH_INITIAL_Y;
				ctx->gushes[num_gushes].move_pattern = data_gush_move_pattern_1;
				ctx->gushes[num_gushes].move_pattern_pos = 0;
				ctx->gushes[num_gushes].yvel = data_gush_move_pattern_1[0];
				ctx->gushes[num_gushes].ydest = data_gush_move_pattern_1[1];
				ctx->gushes[num_gushes].is_crack = false;
				num_gushes++;
				break;

			case LVLENTRY_GUSH_CRACK:
				ctx->gushes[num_gushes].x = entries[i].x;
				ctx->gushes[num_gushes].y = GUSH_CRACK_Y;
				ctx->gushes[num_gushes].move_pattern = data_gush_move_pattern_2;
				ctx->gushes[num_gushes].move_pattern_pos = 0;
				ctx->gushes[num_gushes].yvel = data_gush_move_pattern_1[0];
				ctx->gushes[num_gushes].ydest = data_gush_move_pattern_1[1];
				ctx->gushes[num_gushes].is_crack = true;
				num_gushes++;
				break;

			case LVLENTRY_OVERHEAD_SIGN:
				ctx->overhead_signs[num_overhead_signs].x = entries[i].x;
				ctx->overhead_signs[num_overhead_signs].y = entries[i].y;
				num_overhead_signs++;
				break;

			case LVLENTRY_PUSHABLE_CRATE:
				ctx->pushable_crates[num_pushable_crates].x = entries[i].x;
				ctx->pushable_crates[num_pushable_crates].xmax = entries[i].x + LEVEL_BLOCK_SIZE;
				num_pushable_crates++;
				break;

			case LVLENTRY_ROPE:
				ctx->ropes[num_ropes].x = entries[i].x;
				num_ropes++;
				break;

			case LVLENTRY_SPRING:
				ctx->springs[num_springs].x = entries[i].x;
				ctx->springs[num_springs].y = entries[i].y;
				num_springs++;
				break;
		}
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
	int num_level_columns = (ctx->level_size_pixels / LEVEL_BLOCK_SIZE);
	int i;

	if (invalid) {
		return;
	}

	//Add first floor solid
	add_solid(SOL_FULL, 0, 264, LEVEL_BLOCK_SIZE, 80);

	//Add other floor solids
	for (i = 1; i < num_level_columns; i++) {
		int x;

		switch (ctx->level_columns[i].hole_type) {
			case HOLE_NONE:
				ctx->solids[num_solids - 1].right += LEVEL_BLOCK_SIZE;
				break;

			case HOLE_DEEP_LEFT:
				ctx->solids[num_solids - 1].right += 12;
				break;

			case HOLE_DEEP_RIGHT:
				x = (LEVEL_BLOCK_SIZE * i) + 14;
				add_solid(SOL_FULL, x, 264, 10, 80);
				break;

			case HOLE_PASSAGEWAY_LEFT:
				ctx->solids[num_solids - 1].right += 6;
				break;

			case HOLE_PASSAGEWAY_RIGHT:
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

	//Add solids for overhead signs
	for (i = 0; i < num_overhead_signs; i++)
	{
		int x = ctx->overhead_signs[i].x;
		int y = ctx->overhead_signs[i].y;

		add_solid(SOL_FULL, x + 12, y, 4, 32);

		//Too many solids
		if (invalid) {
			return;
		}
	}

	//Add solids for parked vehicles
	for (i = 0; i < num_parked_vehicles; i++) {
		int type = ctx->parked_vehicles[i].type;
		int x = ctx->parked_vehicles[i].x;
		int y;

		if (type == PARKED_TRUCK) {
			y = PARKED_TRUCK_Y;

			add_solid(SOL_FULL, x, y + 4, 224, 96);
			add_solid(SOL_FULL, x + 224, y + 23, 55, 80);
		} else {
			y = PARKED_CAR_Y;

			add_solid(SOL_FULL, x + 4, y + 18, 20, 4);
			add_solid(SOL_SLOPE_UP, x + 27, y + 2, 15, 15);
			add_solid(SOL_VERTICAL, x + 48, y + 2, 16, 4);
			add_solid(SOL_SLOPE_DOWN, x + 66, y + 2, 18, 18);
			add_solid(SOL_KEEP_ON_TOP, x + 88, y + 20, 16, 4);
			add_solid(SOL_KEEP_ON_TOP, x + 104, y + 22, 16, 4);
			add_solid(SOL_FULL, x + 120, y + 24, 8, 4);
		}

		//Too many solids
		if (invalid) {
			return;
		}
	}

	//Add solids for hydrants
	for (i = 0; i < MAX_LEVEL_COLUMNS; i++) {
		int x = i * LEVEL_BLOCK_SIZE;
		int y = HYDRANT_Y;

		if (ctx->level_columns[i].has_hydrant) {
			add_solid(SOL_FULL, x + 4, y + 8, 8, 4);
		}

		//Too many solids
		if (invalid) {
			return;
		}
	}
}

