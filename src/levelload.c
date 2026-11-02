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

struct {
	int x;
	int type;
	int token2;
	int token3;
} entries[MAX_LEVEL_ENTRIES];

struct {
	int left;
	int top;
	int right;
	int bottom;
} occupied_areas[MAX_OCCUPIED_AREAS];

static int num_entries;
static int num_occupied_areas;
static int num_solids;

//------------------------------------------------------------------------------

//Function prototypes
static int  read_file();
static bool check_numbers();
static bool add_deep_holes();
static bool add_passageways();
static bool add_unpushable_crates();
static bool add_hydrants();
static bool add_ropes();
static bool add_parked_vehicles();
static bool add_springs();
static bool add_gushes();
static bool add_banana_peels();
static bool add_triggers();
static bool add_overhead_signs();
static bool add_respawn_poins();
static bool add_coins();
static int  add_solid(int type, int x, int y, int width, int height);
static bool add_solids();
static bool occupy_area(int x, int y, int width, int height);

//------------------------------------------------------------------------------

void levelload_init(PlayCtx* pctx)
{
	ctx = pctx;
}

int levelload_load(const char* filename)
{
	int error_type;

	num_entries = 0;
	num_occupied_areas = 0;
	num_solids = 0;

	//The maximum allowed file size is 4 kB
	if (get_file_size(filename) > 4096) {
		return LVLERR_TOO_LARGE;
	}

	error_type = read_file(filename);
	if (error_type != LVLERR_NONE) {
		return error_type;
	}

	if (!check_numbers())         return LVLERR_INVALID;
	if (!add_deep_holes())        return LVLERR_INVALID;
	if (!add_passageways())       return LVLERR_INVALID;
	if (!add_unpushable_crates()) return LVLERR_INVALID;
	if (!add_hydrants())          return LVLERR_INVALID;
	if (!add_ropes())             return LVLERR_INVALID;
	if (!add_parked_vehicles())   return LVLERR_INVALID;
	if (!add_springs())           return LVLERR_INVALID;
	if (!add_gushes())            return LVLERR_INVALID;
	if (!add_banana_peels())      return LVLERR_INVALID;
	if (!add_triggers())          return LVLERR_INVALID;
	if (!add_overhead_signs())    return LVLERR_INVALID;
	if (!add_respawn_poins())     return LVLERR_INVALID;
	if (!add_coins())             return LVLERR_INVALID;
	if (!add_solids())            return LVLERR_INVALID;

	return LVLERR_NONE;
}

//------------------------------------------------------------------------------

static int read_file(const char* filename)
{
	char tmp[48];
	int level_size_pixels = 0;
	int num_level_columns = 0;
	int sky_color = 0;
	int bgm = 0;
	int goal_scene = 0;
	int x = 0;
	int max_x = 0;

	if (!lineread_open(filename)) {
		return LVLERR_CANNOT_OPEN;
	}

	x = VSCREEN_MAX_WIDTH_LEVEL_BLOCKS;

	while (!lineread_ended()) {
		int entry_type = -1;
		int width = 1;
		int num_tokens;
		int num_expected_tokens = 0;
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
			if (num_tokens != 2) {
				return LVLERR_INVALID;
			}

			//Error: level size redefinition
			if (level_size_pixels != 0) {
				return LVLERR_INVALID;
			}

			//Error: size out of the allowed range
			if (token1 < 8 || token1 > 24) {
				return LVLERR_INVALID;
			}

			//Just before the last screen
			max_x = (token1 - 1) * VSCREEN_MAX_WIDTH_LEVEL_BLOCKS;

			level_size_pixels = token1 * VSCREEN_MAX_WIDTH;
			num_level_columns = level_size_pixels / LEVEL_BLOCK_SIZE;

			continue;
		} else if (str_starts_with(tmp, "sky-color ")) {
			if (num_tokens != 2) {
				return LVLERR_INVALID;
			}

			//Error: sky color redefinition
			if (sky_color != 0) {
				return LVLERR_INVALID;
			}

			switch (token1) {
				case 1:  sky_color = SPR_BG_SKY1; break;
				case 2:  sky_color = SPR_BG_SKY2; break;
				case 3:  sky_color = SPR_BG_SKY3; break;
				default: return LVLERR_INVALID;
			}

			continue;
		} else if (str_starts_with(tmp, "bgm ")) {
			if (num_tokens != 2) {
				return LVLERR_INVALID;
			}

			//Error: BGM redefinition
			if (bgm != 0) {
				return LVLERR_INVALID;
			}

			switch (token1) {
				case 1:  bgm = BGM1; break;
				case 2:  bgm = BGM2; break;
				case 3:  bgm = BGM3; break;
				default: return LVLERR_INVALID;
			}

			continue;
		} else if (str_starts_with(tmp, "goal-scene ")) {
			if (num_tokens != 2) {
				return LVLERR_INVALID;
			}

			//Error: goal cutscene redefinition
			if (goal_scene != 0) {
				return LVLERR_INVALID;
			}

			//Error: invalid goal cutscene number
			if (token1 < 1 || token1 > 5) {
				return LVLERR_INVALID;
			}

			goal_scene = token1;

			continue;
		}

		//Error: adding entries without defining the level size, sky color,
		//BGM track, and goal cutscene
		if (level_size_pixels == 0
				|| sky_color == 0
				|| bgm == 0
				|| goal_scene == 0) {

			return LVLERR_INVALID;
		}

		if (str_starts_with(tmp, "banana-peel ")) {
			entry_type = LVLENTRY_BANANA_PEEL;
			num_expected_tokens = 3;
		} else if (str_starts_with(tmp, "car-blue ")) {
			entry_type = LVLENTRY_CAR_BLUE;
			num_expected_tokens = 2;
			width = 6;
		} else if (str_starts_with(tmp, "car-silver ")) {
			entry_type = LVLENTRY_CAR_SILVER;
			num_expected_tokens = 2;
			width = 6;
		} else if (str_starts_with(tmp, "car-yellow ")) {
			entry_type = LVLENTRY_CAR_YELLOW;
			num_expected_tokens = 2;
			width = 6;
		} else if (str_starts_with(tmp, "coin-silver ")) {
			entry_type = LVLENTRY_COIN_SILVER;
			num_expected_tokens = 3;
		} else if (str_starts_with(tmp, "coin-gold ")) {
			entry_type = LVLENTRY_COIN_GOLD;
			num_expected_tokens = 3;
		} else if (str_starts_with(tmp, "crates ")) {
			entry_type = LVLENTRY_CRATES;
			num_expected_tokens = 4;
			width = token2;
		} else if (str_starts_with(tmp, "gush ")) {
			entry_type = LVLENTRY_GUSH;
			num_expected_tokens = 2;
		} else if (str_starts_with(tmp, "gush-crack ")) {
			entry_type = LVLENTRY_GUSH_CRACK;
			num_expected_tokens = 2;
		} else if (str_starts_with(tmp, "hydrant ")) {
			entry_type = LVLENTRY_HYDRANT;
			num_expected_tokens = 2;
		} else if (str_starts_with(tmp, "overhead-sign ")) {
			entry_type = LVLENTRY_OVERHEAD_SIGN;
			num_expected_tokens = 3;
		} else if (str_starts_with(tmp, "rope ")) {
			entry_type = LVLENTRY_ROPE;
			num_expected_tokens = 2;
		} else if (str_starts_with(tmp, "spring ")) {
			entry_type = LVLENTRY_SPRING;
			num_expected_tokens = 2;
		} else if (str_starts_with(tmp, "truck ")) {
			entry_type = LVLENTRY_TRUCK;
			num_expected_tokens = 2;
			width = 12;
		} else if (str_starts_with(tmp, "trigger-car-blue ")) {
			entry_type = LVLENTRY_TRIGGER_CAR_BLUE;
			num_expected_tokens = 2;
		} else if (str_starts_with(tmp, "trigger-car-silver ")) {
			entry_type = LVLENTRY_TRIGGER_CAR_SILVER;
			num_expected_tokens = 2;
		} else if (str_starts_with(tmp, "trigger-car-yellow ")) {
			entry_type = LVLENTRY_TRIGGER_CAR_YELLOW;
			num_expected_tokens = 2;
		} else if (str_starts_with(tmp, "trigger-hen ")) {
			entry_type = LVLENTRY_TRIGGER_HEN;
			num_expected_tokens = 2;
		} else if (str_starts_with(tmp, "respawn-point ")) {
			entry_type = LVLENTRY_RESPAWN_POINT;
			num_expected_tokens = 3;
		} else if (str_starts_with(tmp, "deep-hole ")) {
			entry_type = LVLENTRY_DEEP_HOLE;
			num_expected_tokens = 3;
			width = token2;
		} else if (str_starts_with(tmp, "passageway ")) {
			entry_type = LVLENTRY_PASSAGEWAY;
			num_expected_tokens = 3;
			width = token2;
		} else if (str_starts_with(tmp, "passageway-arrow ")) {
			entry_type = LVLENTRY_PASSAGEWAY_ARROW;
			num_expected_tokens = 3;
			width = token2;
		} else {
			//Error: invalid entry
			return LVLERR_INVALID;
		}

		if (num_tokens != num_expected_tokens) {
			return LVLERR_INVALID;
		}

		//Error: too many entries
		if (num_entries >= MAX_LEVEL_ENTRIES) {
			return LVLERR_INVALID;
		}

		//The value of token1 is relative to the previous X position
		x += token1;

		//Error: entry extends beyond or too close to the level's right boundary
		if (x + width > max_x) {
			return LVLERR_INVALID;
		}

		entries[num_entries].x = x;
		entries[num_entries].type = entry_type;
		entries[num_entries].token2 = token2;
		entries[num_entries].token3 = token3;
		num_entries++;
	}

	//Error: no entries
	if (num_entries == 0) {
		return LVLERR_INVALID;
	}

	ctx->level_size_pixels = level_size_pixels;
	ctx->num_level_columns = num_level_columns;
	ctx->sky_color = sky_color;
	ctx->bgm = bgm;
	ctx->goal_scene = goal_scene;

	return LVLERR_NONE;
}

//Checks if there are no more than the maximum number of entries of each type,
//as well as if the number of respawn points is the same as the number of deep
//holes
static bool check_numbers()
{
	int num_banana_peels = 0;
	int num_coins = 0;
	int num_deep_holes = 0;
	int num_gushes = 0;
	int num_overhead_signs = 0;
	int num_parked_vehicles = 0;
	int num_passageways = 0;
	int num_respawn_points = 0;
	int num_ropes = 0;
	int num_springs = 0;
	int num_triggers = 0;
	int num_car_triggers = 0;
	int i;

	for (i = 0; i < num_entries; i++) {
		switch (entries[i].type) {
			case LVLENTRY_BANANA_PEEL:
				num_banana_peels++;
				break;

			case LVLENTRY_CAR_BLUE:
				num_parked_vehicles++;
				break;

			case LVLENTRY_CAR_SILVER:
				num_parked_vehicles++;
				break;

			case LVLENTRY_CAR_YELLOW:
				num_parked_vehicles++;
				break;

			case LVLENTRY_COIN_SILVER:
				num_coins++;
				break;

			case LVLENTRY_COIN_GOLD:
				num_coins++;
				break;

			case LVLENTRY_CRATES:
				//No limit
				break;

			case LVLENTRY_DEEP_HOLE:
				num_deep_holes++;
				break;

			case LVLENTRY_GUSH:
				num_gushes++;
				break;

			case LVLENTRY_GUSH_CRACK:
				num_gushes++;
				break;

			case LVLENTRY_HYDRANT:
				//No limit
				break;

			case LVLENTRY_OVERHEAD_SIGN:
				num_overhead_signs++;
				break;

			case LVLENTRY_PASSAGEWAY:
				num_passageways++;
				break;

			case LVLENTRY_PASSAGEWAY_ARROW:
				num_passageways++;
				break;

			case LVLENTRY_RESPAWN_POINT:
				num_respawn_points++;
				break;

			case LVLENTRY_ROPE:
				num_ropes++;
				break;

			case LVLENTRY_SPRING:
				num_springs++;
				break;

			case LVLENTRY_TRIGGER_CAR_BLUE:
				num_triggers++;
				num_car_triggers++;
				break;

			case LVLENTRY_TRIGGER_CAR_SILVER:
				num_triggers++;
				num_car_triggers++;
				break;

			case LVLENTRY_TRIGGER_CAR_YELLOW:
				num_triggers++;
				num_car_triggers++;
				break;

			case LVLENTRY_TRIGGER_HEN:
				num_triggers++;
				break;

			case LVLENTRY_TRUCK:
				num_parked_vehicles++;
				break;
		}
	}

	if (num_banana_peels > MAX_BANANA_PEELS) return false;
	if (num_coins > MAX_COINS) return false;
	if (num_deep_holes > MAX_DEEP_HOLES) return false;
	if (num_gushes > MAX_GUSHES) return false;
	if (num_overhead_signs > MAX_OVERHEAD_SIGNS) return false;
	if (num_parked_vehicles > MAX_PARKED_VEHICLES) return false;
	if (num_passageways > MAX_PASSAGEWAYS) return false;
	if (num_respawn_points > MAX_RESPAWN_POINTS) return false;
	if (num_ropes > MAX_ROPES) return false;
	if (num_springs > MAX_SPRINGS) return false;
	if (num_triggers > MAX_TRIGGERS) return false;

	//Ensure there are enough slots for the banana peels thrown from cars
	if (num_banana_peels + num_car_triggers > MAX_BANANA_PEELS) return false;

	//Ensure there are enough slots for springs in passageways
	if (num_springs + num_passageways > MAX_SPRINGS) return false;

	//Ensure there is exactly one respawn points for each deep hole
	if (num_respawn_points != num_deep_holes) return false;

	return true;
}

static bool add_deep_holes()
{
	int i;

	for (i = 0; i < num_entries; i++) {
		int x;
		int width;
		int j;

		if (entries[i].type != LVLENTRY_DEEP_HOLE) continue;

		x = entries[i].x;
		width = entries[i].token2;

		//Ensure the width is within the allowed range
		if (width <= 2 || width > 16) {
			return false;
		}

		for (j = 0; j < width; j++) {
			if (ctx->level_columns[x + j].hole_type != HOLE_NONE) {
				return false;
			}

			ctx->level_columns[x + j].hole_type = HOLE_DEEP_MIDDLE;
		}

		ctx->level_columns[x].hole_type = HOLE_DEEP_LEFT;
		ctx->level_columns[x + width - 1].hole_type = HOLE_DEEP_RIGHT;
	}

	return true;
}

static bool add_passageways()
{
	int num_passageways = 0;
	int i;

	for (i = 0; i < num_entries; i++) {
		int type;
		int x;
		int width;
		int j;

		type = entries[i].type;

		if (type != LVLENTRY_PASSAGEWAY
				&& type != LVLENTRY_PASSAGEWAY_ARROW) continue;

		x = entries[i].x;
		width = entries[i].token2;

		//Ensure the width is within the allowed range
		if (width <= 6 || width > 32) {
			return false;
		}

		for (j = 0; j < width; j++) {
			if (ctx->level_columns[x + j].hole_type != HOLE_NONE) {
				return false;
			}

			ctx->level_columns[x + j].hole_type = HOLE_PASSAGEWAY_MIDDLE;
		}

		ctx->level_columns[x].hole_type = HOLE_PASSAGEWAY_LEFT;
		ctx->level_columns[x + width - 1].hole_type = HOLE_PASSAGEWAY_RIGHT;

		//Area occupied by the pushable crate, which is placed on the
		//previous column and has a width of 3 to in order to ensure nothing
		//obstructs it
		if (!occupy_area(x - 1, 10, 3, 1)) {
			return false;
		}

		x *= LEVEL_BLOCK_SIZE;
		width *= LEVEL_BLOCK_SIZE;

		//Add pushable crate
		ctx->pushable_crates[num_passageways].x = x;
		ctx->pushable_crates[num_passageways].xmax = x + LEVEL_BLOCK_SIZE;
		if (type == LVLENTRY_PASSAGEWAY_ARROW) {
			ctx->pushable_crates[num_passageways].show_arrow = true;
		}

		ctx->passageways[num_passageways].x = x;
		ctx->passageways[num_passageways].width = width;
		ctx->passageways[num_passageways].exit_opened = false;
		num_passageways++;
	}

	return true;
}

static bool add_unpushable_crates()
{
	int i;

	for (i = 0; i < num_entries; i++) {
		int x;
		int width;
		int height;
		int j;

		if (entries[i].type != LVLENTRY_CRATES) continue;

		x = entries[i].x;
		width = entries[i].token2;
		height = entries[i].token3;

		if (!occupy_area(x, (11 - height), width, height)) {
			return false;
		}

		//Cannot be adjacent to a level column with a stack of crates with the
		//same height
		if (ctx->level_columns[x - 1].num_crates == height) {
			return false;
		}

		for (j = 0; j < width; j++) {
			int hole_type = ctx->level_columns[x + j].hole_type;

			if (hole_type != HOLE_NONE && hole_type != HOLE_PASSAGEWAY_MIDDLE) {
				return false;
			}

			ctx->level_columns[x + j].num_crates = height;
		}
	}

	return true;
}

static bool add_hydrants()
{
	int i;

	for (i = 0; i < num_entries; i++) {
		int x;

		if (entries[i].type != LVLENTRY_HYDRANT) continue;

		x = entries[i].x;

		if (ctx->level_columns[x].hole_type != HOLE_NONE) {
			return false;
		}

		if (!occupy_area(x, 10, 1, 1)) {
			return false;
		}

		ctx->level_columns[x].has_hydrant = true;
	}

	return true;
}

static bool add_ropes()
{
	int num_ropes = 0;
	int i;

	for (i = 0; i < num_entries; i++) {
		int x;
		int j;

		if (entries[i].type != LVLENTRY_ROPE) continue;

		x = entries[i].x;

		//Ensure the position corresponds to a light pole
		if (x % 16 != 0) {
			return false;
		}

		if (!occupy_area(x + 1, 2, 16, 9)) {
			return false;
		}

		for (j = 0; j < 16; j++) {
			int hole_type = ctx->level_columns[x + j].hole_type;

			if (hole_type == HOLE_PASSAGEWAY_LEFT) {
				return false;
			}
			if (hole_type == HOLE_PASSAGEWAY_MIDDLE) {
				return false;
			}
			if (hole_type == HOLE_PASSAGEWAY_RIGHT) {
				return false;
			}

			ctx->level_columns[x + j + 1].has_rope = true;
		}

		ctx->ropes[num_ropes].x = (x * LEVEL_BLOCK_SIZE + 32);
		num_ropes++;
	}

	ctx->num_ropes = num_ropes;

	return true;

}

static bool add_parked_vehicles()
{
	int num_parked_vehicles = 0;
	int i;

	for (i = 0; i < num_entries; i++) {
		int type;
		int width;
		int x;
		int j;

		switch (entries[i].type) {
			case LVLENTRY_CAR_BLUE:
				type = PARKED_CAR_BLUE;
				width = 6;
				break;

			case LVLENTRY_CAR_SILVER:
				type = PARKED_CAR_SILVER;
				width = 6;
				break;

			case LVLENTRY_CAR_YELLOW:
				type = PARKED_CAR_YELLOW;
				width = 6;
				break;

			case LVLENTRY_TRUCK:
				type = PARKED_TRUCK;
				width = 12;
				break;

			default:
				continue;
		}

		x = entries[i].x;

		//Add occupied level columns
		if (type == PARKED_TRUCK) { //Truck
			if (!occupy_area(x, 6, 10, 5)) {
				return false;
			}
			if (!occupy_area(x + 10, 7, 2, 4)) {
				return false;
			}
		} else { //Car
			if (!occupy_area(x, 9, 4, 2)) {
				return false;
			}
			if (!occupy_area(x + 4, 10, 2, 1)) {
				return false;
			}
		}

		for (j = 0; j < width; j++) {
			int hole_type = ctx->level_columns[x + j].hole_type;

			if (hole_type != HOLE_NONE && hole_type != HOLE_PASSAGEWAY_MIDDLE) {
				return false;
			}
		}

		ctx->parked_vehicles[num_parked_vehicles].type = type;
		ctx->parked_vehicles[num_parked_vehicles].x = (x * LEVEL_BLOCK_SIZE);
		num_parked_vehicles++;
	}

	ctx->num_parked_vehicles = num_parked_vehicles;

	return true;
}

static bool add_springs()
{
	int num_springs = 0;
	int i;

	for (i = 0; i < num_entries; i++) {
		int hole_type;
		int x;

		if (entries[i].type != LVLENTRY_SPRING) continue;

		x = entries[i].x;
		hole_type = ctx->level_columns[x].hole_type;

		if (hole_type != HOLE_NONE && hole_type != HOLE_PASSAGEWAY_MIDDLE) {
			return false;
		}

		if (!occupy_area(x, 10, 1, 1)) {
			return false;
		}

		ctx->springs[num_springs].x = (x * LEVEL_BLOCK_SIZE + 8);
		ctx->springs[num_springs].y = 248;
		num_springs++;
	}

	//Add passageway springs
	for (i = 0; i < MAX_PASSAGEWAYS; i++) {
		int x;
		int width;

		x = ctx->passageways[i].x;
		width = ctx->passageways[i].width;

		//No more passageways
		if (x <= 0) break;

		occupy_area(((x + width - 16) / LEVEL_BLOCK_SIZE), 14, 1, 1);

		ctx->springs[num_springs].x = (x + width - 16);
		ctx->springs[num_springs].y = 344;
		num_springs++;
	}

	//Sort springs by X position using the "selection sort" algorithm
	if (num_springs > 1) {
		for (i = 0; i < num_springs - 1; i++) {
			int tmp_x;
			int tmp_y;
			int min_idx;
			int j;

			min_idx = i;

			for (j = i + 1; j < num_springs; j++) {
				if (ctx->springs[j].x < ctx->springs[min_idx].x) {
					min_idx = j;
				}
			}

			tmp_x = ctx->springs[i].x;
			tmp_y = ctx->springs[i].y;
			ctx->springs[i].x = ctx->springs[min_idx].x;
			ctx->springs[i].y = ctx->springs[min_idx].y;
			ctx->springs[min_idx].x = tmp_x;
			ctx->springs[min_idx].y = tmp_y;
		}
	}

	ctx->num_springs = num_springs;

	return true;
}

static bool add_gushes()
{
	int num_gushes = 0;
	int i;

	for (i = 0; i < num_entries; i++) {
		int type;
		int x;

		type = entries[i].type;

		if (type != LVLENTRY_GUSH && type != LVLENTRY_GUSH_CRACK) continue;

		x = entries[i].x;

		if (ctx->level_columns[x].hole_type != HOLE_NONE) {
			return false;
		}

		if (!occupy_area(x, 9, 1, 3)) {
			return false;
		}

		ctx->gushes[num_gushes].x = (x * LEVEL_BLOCK_SIZE);
		ctx->gushes[num_gushes].move_pattern_pos = 0;

		if (type == LVLENTRY_GUSH_CRACK) {
			ctx->gushes[num_gushes].y = GUSH_CRACK_Y;
			ctx->gushes[num_gushes].move_pattern = data_gush_move_pattern_2;
			ctx->gushes[num_gushes].yvel = data_gush_move_pattern_2[0];
			ctx->gushes[num_gushes].ydest = data_gush_move_pattern_2[1];
			ctx->gushes[num_gushes].is_crack = true;
		} else {
			ctx->gushes[num_gushes].y = GUSH_INITIAL_Y;
			ctx->gushes[num_gushes].move_pattern = data_gush_move_pattern_1;
			ctx->gushes[num_gushes].yvel = data_gush_move_pattern_1[0];
			ctx->gushes[num_gushes].ydest = data_gush_move_pattern_1[1];
			ctx->gushes[num_gushes].is_crack = false;
		}

		num_gushes++;
	}

	ctx->num_gushes = num_gushes;

	return true;
}

static bool add_banana_peels()
{
	int num_banana_peels = 0;
	int i;

	for (i = 0; i < num_entries; i++) {
		int hole_type;
		int x;
		int y;

		if (entries[i].type != LVLENTRY_BANANA_PEEL) continue;

		x = entries[i].x;
		y = entries[i].token2;
		hole_type = ctx->level_columns[x].hole_type;

		if (hole_type == HOLE_PASSAGEWAY_LEFT) {
			return false;
		}
		if (hole_type == HOLE_PASSAGEWAY_RIGHT) {
			return false;
		}

		if (!occupy_area(x, y, 1, 1)) {
			return false;
		}

		if (y > 10 && y != 14) {
			return false;
		} else if (y == 14) {
			//Passageway bottom
			if (hole_type != HOLE_PASSAGEWAY_MIDDLE) {
				return false;
			}
		} else {
			if (y != 10 - ctx->level_columns[x].num_crates) {
				return false;
			}

			if (ctx->level_columns[x + 1].hole_type == HOLE_PASSAGEWAY_LEFT) {
				//If an banana is placed right before a passageway left column,
				//pushing the crate over it becomes impossible without slipping
				//on the peel
				return false;
			}
		}

		ctx->banana_peels[num_banana_peels].x = (x * LEVEL_BLOCK_SIZE + 16);
		ctx->banana_peels[num_banana_peels].y = (y * LEVEL_BLOCK_SIZE + 16);
		num_banana_peels++;
	}

	ctx->num_banana_peels = num_banana_peels;

	return true;
}

static bool add_triggers()
{
	int num_triggers = 0;
	int i;

	for (i = 0; i < num_entries; i++) {
		int what;
		int x;

		switch (entries[i].type) {
			case LVLENTRY_TRIGGER_CAR_BLUE:
				what = CAR_BLUE;
				break;

			case LVLENTRY_TRIGGER_CAR_SILVER:
				what = CAR_SILVER;
				break;

			case LVLENTRY_TRIGGER_CAR_YELLOW:
				what = CAR_YELLOW;
				break;

			case LVLENTRY_TRIGGER_HEN:
				what = TRIGGER_HEN;
				break;

			default:
				continue;
		}

		x = entries[i].x;

		ctx->triggers[num_triggers].x = (x * LEVEL_BLOCK_SIZE);
		ctx->triggers[num_triggers].what = what;
		num_triggers++;
	}

	ctx->num_triggers = num_triggers;

	return true;
}

static bool add_overhead_signs()
{
	int num_overhead_signs = 0;
	int i;

	for (i = 0; i < num_entries; i++) {
		int x;
		int y;

		if (entries[i].type != LVLENTRY_OVERHEAD_SIGN) continue;

		x = entries[i].x;
		y = entries[i].token2;

		if (y < 2 || y > 4) {
			return false;
		}

		if (!occupy_area(x, y, 2, 2)) {
			return false;
		}

		//Validate the distance from the previous sign
		if (num_overhead_signs > 0) {
			int prev_x = ctx->overhead_signs[num_overhead_signs - 1].x;

			if ((x * LEVEL_BLOCK_SIZE) - prev_x < 16) {
				return false;
			}
		}

		ctx->overhead_signs[num_overhead_signs].x = (x * LEVEL_BLOCK_SIZE);
		ctx->overhead_signs[num_overhead_signs].y = (y * LEVEL_BLOCK_SIZE + 16);
		num_overhead_signs++;
	}

	ctx->num_overhead_signs = num_overhead_signs;

	return true;
}

static bool add_respawn_poins()
{
	int num_respawn_points = 0;
	int i;

	for (i = 0; i < num_entries; i++) {
		int hole_type;
		int x;
		int y;

		if (entries[i].type != LVLENTRY_RESPAWN_POINT) continue;

		x = entries[i].x;
		y = entries[i].token2;

		if (!occupy_area(x, y, 1, 2)) {
			return false;
		}

		hole_type = ctx->level_columns[x].hole_type;

		if (hole_type != HOLE_NONE && hole_type != HOLE_DEEP_RIGHT) {
			return false;
		}

		//Ensure the respawn point is close enough to a deep hole
		if (ctx->level_columns[x + 1].hole_type != HOLE_DEEP_LEFT
				&& ctx->level_columns[x + 2].hole_type != HOLE_DEEP_LEFT) {

			return false;
		}

		if (y != 9 - ctx->level_columns[x].num_crates) {
			return false;
		}

		ctx->respawn_points[num_respawn_points].x = (x * LEVEL_BLOCK_SIZE + 3);
		ctx->respawn_points[num_respawn_points].y = (y * LEVEL_BLOCK_SIZE - 12);
		num_respawn_points++;
	}

	ctx->num_respawn_points = num_respawn_points;

	return true;
}

static bool add_coins()
{
	int num_coins = 0;
	int i;

	for (i = 0; i < num_entries; i++) {
		int type;
		int hole_type;
		int x;
		int y;

		type = entries[i].type;

		if (type != LVLENTRY_COIN_SILVER && type != LVLENTRY_COIN_GOLD) continue;

		x = entries[i].x;
		y = entries[i].token2;

		if (!occupy_area(x, y, 1, 1)) {
			return false;
		}

		hole_type = ctx->level_columns[x].hole_type;

		if (y == 11) {
			//Middle of the floor
			return false;
		} else if (y > 14) {
			//Too far downwards
			return false;
		} else if (y > 11) {
			//In a passageway
			if (hole_type != HOLE_PASSAGEWAY_LEFT
					&& hole_type != HOLE_PASSAGEWAY_MIDDLE
					&& hole_type != HOLE_PASSAGEWAY_RIGHT) {

				return false;
			}
		} else {
			//Over the floor
			int y_max = 10 - ctx->level_columns[x].num_crates;

			if (ctx->level_columns[x].has_hydrant) {
				y_max--;
			}

			if (y > y_max) {
				return false;
			}
		}

		ctx->coins[num_coins].x = (x * LEVEL_BLOCK_SIZE + 8);
		ctx->coins[num_coins].y = (y * LEVEL_BLOCK_SIZE);
		ctx->coins[num_coins].gold = (type == LVLENTRY_COIN_GOLD);
		num_coins++;
	}

	ctx->num_coins = num_coins;

	return true;
}

static int add_solid(int type, int x, int y, int width, int height)
{
	//Check if there are too many solids
	if (num_solids >= MAX_SOLIDS) {
		num_solids = MAX_SOLIDS + 1;

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

static bool add_solids()
{
	int num_level_columns = (ctx->level_size_pixels / LEVEL_BLOCK_SIZE);
	int i;

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
	}

	//Add passageway solids
	for (i = 0; i < MAX_PASSAGEWAYS; i++) {
		int x = ctx->passageways[i].x;
		int w = ctx->passageways[i].width;

		//No more passageways
		if (x <= 0) break;

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
	for (i = 1; i < ctx->num_level_columns; i++) {
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
		}
	}

	//Add solids for pushable crates
	for (i = 0; i < MAX_PUSHABLE_CRATES; i++) {
		int x = (int)ctx->pushable_crates[i].x;
		int y = PUSHABLE_CRATE_Y;
		int w = LEVEL_BLOCK_SIZE;
		int h = LEVEL_BLOCK_SIZE;

		//No more pushable crates
		if (x <= 0) break;

		ctx->pushable_crates[i].solid = add_solid(SOL_FULL, x, y, w, h);
	}

	//Add solids for overhead signs
	for (i = 0; i < ctx->num_overhead_signs; i++)
	{
		int x = ctx->overhead_signs[i].x;
		int y = ctx->overhead_signs[i].y;

		add_solid(SOL_FULL, x + 12, y, 4, 32);
	}

	//Add solids for parked vehicles
	for (i = 0; i < ctx->num_parked_vehicles; i++) {
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
	}

	//Add solids for hydrants
	for (i = 0; i < MAX_LEVEL_COLUMNS; i++) {
		int x = i * LEVEL_BLOCK_SIZE;
		int y = HYDRANT_Y;

		if (ctx->level_columns[i].has_hydrant) {
			add_solid(SOL_FULL, x + 4, y + 8, 8, 4);
		}
	}

	if (num_solids > MAX_SOLIDS) {
		return false;
	}

	ctx->num_solids = num_solids;

	return true;
}

static bool occupy_area(int x, int y, int width, int height)
{
	int this_left   = x;
	int this_top    = y;
	int this_right  = (x + width  - 1);
	int this_bottom = (y + height - 1);
	int i;

	if (num_occupied_areas >= MAX_OCCUPIED_AREAS ) {
		return false;
	}

	//Check if the block is already occupied
	for (i = 0; i < num_occupied_areas; i++) {
		int other_left   = occupied_areas[i].left;
		int other_top    = occupied_areas[i].top;
		int other_right  = occupied_areas[i].right;
		int other_bottom = occupied_areas[i].bottom;

		if (this_left    > other_right)  continue;
		if (this_top     > other_bottom) continue;
		if (this_right   < other_left)   continue;
		if (this_bottom  < other_top)    continue;

		//The area is already occupied
		return false;
	}

	occupied_areas[num_occupied_areas].left   = this_left;
	occupied_areas[num_occupied_areas].top    = this_top;
	occupied_areas[num_occupied_areas].right  = this_right;
	occupied_areas[num_occupied_areas].bottom = this_bottom;
	num_occupied_areas++;

	return true;
}

