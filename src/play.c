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
 * play.c
 *
 * Description:
 * Implementation of gameplay logic
 *
 */

//------------------------------------------------------------------------------

#include "defs.h"

#include <stdbool.h>

//------------------------------------------------------------------------------

//From audio.c
void audio_stop_sfx(int id);
void audio_play_sfx(int id);

//From data.c
extern const int data_gush_move_pattern_1[];
extern const int data_gush_move_pattern_2[];

//------------------------------------------------------------------------------

static DisplayParams* display_params;
static PlayCtx ctx; //Gameplay context

static float delta_time; //Time elapsed since the previous frame

static bool  ignore_user_input;
static bool  input_left,  old_input_left;
static bool  input_right, old_input_right;
static bool  input_jump,  old_input_jump;
static float jump_timeout;

//------------------------------------------------------------------------------

//Function prototypes
static void position_camera();
static void set_animation(int anim, bool running, bool loop, bool reverse,
	int num_frames, float delay);
static void start_animation(int anim);
static void add_crack_particles(int x, int y);
static void move_bus_to_end();
static void show_player_in_bus();
static void start_score_count();
static void begin_update();
static void update_remaining_time();
static void update_score_count();
static void move_objects();
static void handle_car_thrown_peel();
static void move_player();
static void handle_solids();
static void handle_passageways();
static void handle_player_interactions();
static void handle_triggers();
static void do_player_state_specifics();
static void handle_fall_sound();
static void handle_respawn();
static void handle_player_state_change();
static void move_camera();
static void keep_player_within_limits();
static void handle_player_animation_change();
static void update_animations();
static void move_push_arrow();
static void position_bus_stop_sign();
static void position_light_pole();
static void update_sequence();

//------------------------------------------------------------------------------

PlayCtx* play_init(DisplayParams* dp)
{
	display_params = dp;

	return &ctx;
}

void play_clear()
{
	int i;

	delta_time = 0;

	ignore_user_input = true;
	input_left = false;
	input_right = false;
	input_jump = false;
	jump_timeout = 0;

	ctx.difficulty = 0;
	ctx.level_num = 0;
	ctx.last_level = false;
	ctx.ending = false;
	ctx.level_size = 0;
	ctx.bg_color = 0;
	ctx.bgm = 0;
	ctx.goal_scene = 0;

	ctx.time = 90;
	ctx.time_running = false;
	ctx.time_up = false;
	ctx.goal_reached = false;
	ctx.counting_score = false;
	ctx.can_pause = false;

	ctx.crate_push_remaining = 0.75f;

	ctx.cam.x = 0;
	ctx.cam.y = 0;
	ctx.cam.xvel = 0;
	ctx.cam.yvel = 0;
	ctx.cam.follow_player = false;
	ctx.cam.fixed_at_leftmost = false;
	ctx.cam.fixed_at_rightmost = false;

	ctx.player.x = 96;
	ctx.player.oldx = 96;
	ctx.player.y = 204;
	ctx.player.oldy = 200;
	ctx.player.xvel = 0;
	ctx.player.yvel = 0;
	ctx.player.fell = false;
	ctx.player.on_floor = false;
	ctx.player.anim_type = PLAYER_ANIM_STAND;
	ctx.player.old_anim_type = PLAYER_ANIM_STAND;
	ctx.player.state = PLAYER_STATE_NORMAL;
	ctx.player.old_state = NONE;
	handle_player_state_change();

	ctx.bus.x = 24;
	ctx.bus.xvel = 0;
	ctx.bus.acc = 0;

	ctx.grabbed_rope.obj = NONE;
	ctx.hit_spring = NONE;
	ctx.car.x = NONE;
	ctx.hen.x = NONE;

	ctx.cur_passageway = NONE;

	for (i = 0; i < MAX_LEVEL_COLUMNS; i++) {
		ctx.level_columns[i].type = LVLCOL_NORMAL_FLOOR;
		ctx.level_columns[i].num_crates = 0;
	}

	for (i = 0; i < MAX_OBJS; i++) {
		ctx.objs[i].type = NONE;
	}

	for (i = 0; i < MAX_GUSHES; i++) {
		ctx.gushes[i].obj = NONE;
	}

	for (i = 0; i < MAX_MOVING_PEELS; i++) {
		ctx.moving_peels[i].obj = NONE;
	}

	for (i = 0; i < MAX_PASSAGEWAYS; i++) {
		ctx.passageways[i].x = NONE;
		ctx.passageways[i].exit_opened = false;
	}

	for (i = 0; i < MAX_PUSHABLE_CRATES; i++) {
		ctx.pushable_crates[i].obj = NONE;
		ctx.pushable_crates[i].pushed = false;
		ctx.pushable_crates[i].show_arrow = false;
	}

	for (i = 0; i < MAX_RESPAWN_POINTS; i++) {
		ctx.respawn_points[i].x = NONE;
	}

	for (i = 0; i < MAX_SOLIDS; i++) {
		ctx.solids[i].type = NONE;
	}

	for (i = 0; i < MAX_TRIGGERS; i++) {
		ctx.triggers[i].x = NONE;
	}

	for (i = 0; i < MAX_CUTSCENE_OBJECTS; i++) {
		ctx.cutscene_objects[i].sprite = NONE;
		ctx.cutscene_objects[i].x = 0;
		ctx.cutscene_objects[i].y = 0;
		ctx.cutscene_objects[i].xvel = 0;
		ctx.cutscene_objects[i].yvel = 0;
		ctx.cutscene_objects[i].acc = 0;
		ctx.cutscene_objects[i].grav = 0;
		ctx.cutscene_objects[i].in_bus = false;
	}

	for (i = 0; i < MAX_COIN_SPARKS; i++) {
		ctx.coin_sparks[i].x = NONE;
	}

	for (i = 0; i < MAX_CRACK_PARTICLES; i++) {
		ctx.crack_particles[i].x = NONE;
	}

	set_animation(ANIM_PLAYER, true, true, false, 1, 0.1f);
	set_animation(ANIM_COINS, true, true, false, 3, 0.1f);
	set_animation(ANIM_GUSHES, true, true, false, 3, 0.05f);
	set_animation(ANIM_HIT_SPRING, false, false, false, 6, 0.02f);
	set_animation(ANIM_CRACK_PARTICLES, true, true, false, 2, 0.1f);
	set_animation(ANIM_BUS_WHEELS, false, true, false, 3, 0.1f);
	set_animation(ANIM_BUS_DOOR_REAR, false, false, false, 4, 0.1f);
	set_animation(ANIM_BUS_DOOR_FRONT, false, false, false, 4, 0.1f);
	set_animation(ANIM_CAR_WHEELS, false, true, false, 2, 0.05f);
	set_animation(ANIM_HEN, false, true, false, 4, 0.05f);

	for (i = 0; i < MAX_COIN_SPARKS; i++) {
		set_animation(ANIM_COIN_SPARKS + i, false, false, false, 4, 0.05f);
	}
	for (i = 0; i < MAX_CUTSCENE_OBJECTS; i++) {
		set_animation(ANIM_CUTSCENE_OBJECTS + i, false, false, false, 1, 0);
	}

	ctx.next_coin_spark = 0;
	ctx.next_crack_particle = 0;

	ctx.push_arrow.xoffs = 0;
	ctx.push_arrow.xvel = 0;
	ctx.push_arrow.delay = 1;

	ctx.player_reached_flagman = false;
	ctx.hen_reached_flagman = false;
	ctx.bus_reached_flagman = false;

	ctx.sequence_step = 0;
	ctx.sequence_delay = 0;
	ctx.wipe_in = false;
	ctx.wipe_out = false;
}

void play_set_input(int input_state)
{
	if (ignore_user_input) return;

	old_input_left  = input_left;
	old_input_right = input_right;
	old_input_jump  = input_jump;

	input_left  = (input_state & INPUT_LEFT)  > 0;
	input_right = (input_state & INPUT_RIGHT) > 0;
	input_jump  = (input_state & INPUT_JUMP)  > 0;

	if (input_jump && !old_input_jump) {
		jump_timeout = JUMP_TIMEOUT;
	}
}

void play_update(float dt)
{
	delta_time = dt;

	begin_update();
	update_remaining_time();
	update_score_count();
	move_objects();
	handle_car_thrown_peel();
	move_player();
	handle_solids();
	handle_passageways();
	handle_player_interactions();
	handle_triggers();
	do_player_state_specifics();
	handle_fall_sound();
	handle_respawn();
	handle_player_state_change();
	move_camera();
	keep_player_within_limits();
	handle_player_animation_change();
	update_animations();
	move_push_arrow();
	position_bus_stop_sign();
	position_light_pole();
	update_sequence();
}

void play_adapt_to_screen_size()
{
	PlayCamera* cam = &ctx.cam;
	int vscreen_width = display_params->vscreen_width;

	cam->xmin = 0;
	cam->xmax = ctx.level_size - vscreen_width;
	cam->follow_player_min_x = 64;
	cam->follow_player_max_x = vscreen_width / 2;

	if (vscreen_width <= 256) {
		cam->follow_player_min_x  = 32;
		cam->follow_player_max_x -= 64;
		cam->xmin = 40;
	} else if (vscreen_width <= 320) {
		cam->follow_player_min_x  = 32;
		cam->follow_player_max_x -= 56;
		cam->xmin = 40;
	}

	position_camera();
}

//------------------------------------------------------------------------------

static void position_camera()
{
	PlayCamera* cam = &ctx.cam;

	if (cam->follow_player) {
		if (cam->xvel == 0) {
			if (ctx.player.x > cam->x + cam->follow_player_max_x) {
				//Move right
				cam->x = ctx.player.x - cam->follow_player_max_x;
			} else if (ctx.player.x < cam->x + cam->follow_player_min_x) {
				//Move left
				cam->x = ctx.player.x - cam->follow_player_min_x;
			}
		}

		if (cam->yvel == 0) {
			//This is not the final Y position of the camera, as the camera's
			//vertical movement is ignored if it is over the floor and the
			//virtual screen (vscreen) is high enough
			if (ctx.player.y < 104) {
				cam->y = ctx.player.y - 104;
			}
		}
	}

	//Keep the camera within the level boundaries
	if (cam->fixed_at_rightmost || cam->x > cam->xmax) {
		cam->x = cam->xmax;
		cam->xvel = 0;
	}
	if (cam->fixed_at_leftmost || cam->x < cam->xmin) {
		cam->x = cam->xmin;
		cam->xvel = 0;
	}
}

static void set_animation(int anim, bool running, bool loop, bool reverse,
	int num_frames, float delay)
{
	Anim* a = &ctx.anims[anim];

	a->running = running;
	a->loop = loop;
	a->reverse = reverse;
	a->num_frames = num_frames;
	a->frame = reverse ? num_frames - 1 : 0;
	a->delay = delay;
	a->max_delay = delay;
}

static void start_animation(int anim)
{
	Anim* a = &ctx.anims[anim];

	a->running = true;
	a->delay = a->max_delay;
	a->frame = a->reverse ? a->num_frames - 1 : 0;
}

static void add_crack_particles(int x, int y)
{
	ctx.crack_particles[ctx.next_crack_particle].x = x;
	ctx.crack_particles[ctx.next_crack_particle].y = y;
	ctx.crack_particles[ctx.next_crack_particle].xvel = -15;
	ctx.crack_particles[ctx.next_crack_particle].yvel = -120;
	ctx.crack_particles[ctx.next_crack_particle].grav =  198;
	ctx.next_crack_particle++;
	ctx.next_crack_particle %= MAX_CRACK_PARTICLES;

	ctx.crack_particles[ctx.next_crack_particle].x = x;
	ctx.crack_particles[ctx.next_crack_particle].y = y;
	ctx.crack_particles[ctx.next_crack_particle].xvel = -6;
	ctx.crack_particles[ctx.next_crack_particle].yvel = -192;
	ctx.crack_particles[ctx.next_crack_particle].grav =  198;
	ctx.next_crack_particle++;
	ctx.next_crack_particle %= MAX_CRACK_PARTICLES;

	ctx.crack_particles[ctx.next_crack_particle].x = x;
	ctx.crack_particles[ctx.next_crack_particle].y = y;
	ctx.crack_particles[ctx.next_crack_particle].xvel =  15;
	ctx.crack_particles[ctx.next_crack_particle].yvel = -120;
	ctx.crack_particles[ctx.next_crack_particle].grav =  198;
	ctx.next_crack_particle++;
	ctx.next_crack_particle %= MAX_CRACK_PARTICLES;

	ctx.crack_particles[ctx.next_crack_particle].x = x;
	ctx.crack_particles[ctx.next_crack_particle].y = y;
	ctx.crack_particles[ctx.next_crack_particle].xvel =  6;
	ctx.crack_particles[ctx.next_crack_particle].yvel = -192;
	ctx.crack_particles[ctx.next_crack_particle].grav =  198;
	ctx.next_crack_particle++;
	ctx.next_crack_particle %= MAX_CRACK_PARTICLES;
}

//Moves the bus to the end of the level
static void move_bus_to_end()
{
	ctx.bus.acc = 0;
	ctx.bus.xvel = 0;
	ctx.bus.x = ctx.level_size - 456;

	//Make rear door closed
	ctx.anims[ANIM_BUS_DOOR_REAR].running = false;
	ctx.anims[ANIM_BUS_DOOR_REAR].frame = 0;
	ctx.anims[ANIM_BUS_DOOR_REAR].reverse = false;

	//Make front door open
	ctx.anims[ANIM_BUS_DOOR_FRONT].running = false;
	ctx.anims[ANIM_BUS_DOOR_FRONT].frame = 3;
	ctx.anims[ANIM_BUS_DOOR_FRONT].reverse = true;

	//Bus route sign
	if (ctx.last_level) {
		//Finish (checkered flag) sign
		ctx.bus.route_sign = 0;
	} else {
		//Sign corresponding to the next level
		ctx.bus.route_sign = ctx.level_num + 1;
	}
}

static void show_player_in_bus()
{
	CutsceneObject* cutscene_player = &ctx.cutscene_objects[0];
	Anim* anim = &ctx.anims[ANIM_CUTSCENE_OBJECTS + 0];

	cutscene_player->sprite = SPR_PLAYER_STAND;
	cutscene_player->in_bus = true;
	cutscene_player->x = 342;
	cutscene_player->y = BUS_Y + 36;

	anim->frame = 0;
	anim->num_frames = 1;

	ctx.player.state = PLAYER_STATE_INACTIVE;
	ctx.player.visible = false;
}

static void start_score_count()
{
	ctx.counting_score = true;
	ctx.time_delay = 0.1f;
}

//------------------------------------------------------------------------------

//Begins the update
static void begin_update()
{
	Player* pl = &ctx.player;

	pl->oldx = pl->x;
	pl->oldy = pl->y;
	pl->old_state = pl->state;
	pl->old_anim_type = pl->anim_type;
	pl->on_floor = false;

	jump_timeout -= delta_time;
	if (jump_timeout < 0) jump_timeout = 0;
}

//Updates the remaining time and acts if the time has run out
static void update_remaining_time()
{
	if (!ctx.time_running) return;

	ctx.time_delay -= delta_time;
	if (ctx.time_delay > 0) return;

	ctx.time_delay = 1;
	ctx.time--;

	if (ctx.time <= 10 && ctx.time >= 0) {
		audio_play_sfx(SFX_TIME);
	}

	if (ctx.time < 0) {
		ctx.time = 0;
		ctx.time_running = false;
		ctx.time_up = true;
	}
}

//Does the score counting from the remaining time after the level's goal is
//reached
static void update_score_count()
{
	if (!ctx.counting_score) return;

	if (ctx.time <= 0) {
		ctx.time = 0;
		ctx.counting_score = false;

		return;
	}

	ctx.time_delay -= delta_time;
	if (ctx.time_delay > 0) return;

	ctx.time_delay = 0.1f;
	ctx.time--;
	ctx.score += 10;
	audio_play_sfx(SFX_SCORE);
}

//Updates the positions of most game objects, not including the player
//character and the camera
static void move_objects()
{
	int i;

	//Bus
	ctx.bus.xvel += ctx.bus.acc * delta_time;
	ctx.bus.x += ctx.bus.xvel * delta_time;

	//Moving banana peels
	for (i = 0; i < MAX_MOVING_PEELS; i++) {
		MovingPeel* peel = &ctx.moving_peels[i];
		Obj* obj;

		if (peel->obj == NONE) continue;

		obj = &ctx.objs[peel->obj];

		peel->yvel += peel->grav * delta_time;
		peel->x += peel->xvel * delta_time;
		peel->y += peel->yvel * delta_time;

		//Deactivate the peel when it gets too far downwards
		if (peel->y >= 400) {
			obj->type = NONE;
			peel->obj = NONE;
		}

		//Stop the peel when it hits the destination Y position
		if (peel->y >= peel->ydest) {
			obj->type = OBJ_BANANA_PEEL;
			peel->x = peel->xdest;
			peel->y = peel->ydest;
			peel->obj = NONE;
		}

		obj->x = (int)peel->x;
		obj->y = (int)peel->y;
	}

	//Gushes
	for (i = 0; i < MAX_GUSHES; i++) {
		Gush* gush = &ctx.gushes[i];

		float y = gush->y;
		float yvel = gush->yvel;
		float ydest = gush->ydest;

		//Ignore inexistent gushes
		if (gush->obj == NONE) continue;

		y += yvel * delta_time;

		//If the gush reaches its destination Y position
		if ((yvel < 0 && y <= ydest) || (yvel > 0 && y >= ydest)) {
			y = ydest;

			//Advance within the movement pattern and loop if its end is
			//reached
			gush->move_pattern_pos += 2;
			if (gush->move_pattern[gush->move_pattern_pos] == 0) {
				gush->move_pattern_pos = 0;
			}

			gush->yvel  = gush->move_pattern[gush->move_pattern_pos];
			gush->ydest = gush->move_pattern[gush->move_pattern_pos + 1];
		}

		gush->y = y;
		ctx.objs[gush->obj].y = (int)y;
	}

	//Grabbed rope
	if (ctx.grabbed_rope.obj != NONE) {
		Obj* obj = &ctx.objs[ctx.grabbed_rope.obj];

		ctx.grabbed_rope.x += ctx.grabbed_rope.xvel * delta_time;

		if (ctx.grabbed_rope.x >= ctx.grabbed_rope.xmax) {
			ctx.grabbed_rope.x = ctx.grabbed_rope.xmax;
			ctx.grabbed_rope.xvel = -192;
		} else if (ctx.grabbed_rope.x <= ctx.grabbed_rope.xmin) {
			ctx.grabbed_rope.x = ctx.grabbed_rope.xmin;
			ctx.grabbed_rope.obj = NONE;
		}

		obj->x = (int)ctx.grabbed_rope.x;
	}

	//Pushable crates
	for (i = 0; i < MAX_PUSHABLE_CRATES; i++) {
		PushableCrate* crate = &ctx.pushable_crates[i];

		if (crate->obj != NONE && crate->pushed) {
			Solid* sol = &ctx.solids[crate->solid];

			crate->x += 72 * delta_time;
			if (crate->x >= crate->xmax) crate->x = crate->xmax;

			ctx.objs[crate->obj].x = (int)crate->x;
			sol->left = (int)crate->x;
			sol->right = (int)crate->x + 24;
		}
	}

	//Passing car
	if (ctx.car.x != NONE) {
		ctx.car.x += ctx.car.xvel * delta_time;

		if (ctx.car.x >= ctx.cam.x + VSCREEN_MAX_WIDTH + 64) {
			ctx.car.x = NONE;
		}
	}

	//Hen
	if (ctx.hen.x != NONE) {
		ctx.hen.xvel += ctx.hen.acc * delta_time;
		ctx.hen.x += ctx.hen.xvel * delta_time;

		if (ctx.hen.x > ctx.cam.x + VSCREEN_MAX_WIDTH + 64) {
			ctx.hen.x = NONE;
		}
	}

	//Crack particles
	for (i = 0; i < MAX_CRACK_PARTICLES; i++) {
		CrackParticle* ptcl = &ctx.crack_particles[i];

		//Ignore inexistent particles
		if (ptcl->x == NONE) continue;

		ptcl->yvel += ptcl->grav * delta_time;
		ptcl->x += ptcl->xvel * delta_time;
		ptcl->y += ptcl->yvel * delta_time;

		if (ptcl->y >= 400) {
			ptcl->x = NONE;
		}
	}

	//Cutscene objects
	for (i = 0; i < MAX_CUTSCENE_OBJECTS; i++) {
		CutsceneObject* cobj = &ctx.cutscene_objects[i];

		//Ignore inexistent cutscene objects
		if (cobj->sprite == NONE) continue;

		cobj->xvel += cobj->acc * delta_time;
		cobj->yvel += cobj->grav * delta_time;
		cobj->x += cobj->xvel * delta_time;
		cobj->y += cobj->yvel * delta_time;
	}
}

//Acts if the passing car has reached the X position at which it throws a
//banana peel
static void handle_car_thrown_peel()
{
	if (ctx.car.x == NONE || ctx.car.threw_peel) return;
	if (ctx.car.x < ctx.car.peel_throw_x) return;

	for (int i = 0; i < MAX_OBJS; i++) {
		if (ctx.objs[i].type == NONE) {
			MovingPeel* peel = &ctx.moving_peels[MOVING_PEEL_THROWN];

			peel->obj = i;
			peel->x = ctx.car.peel_throw_x + 90;
			peel->y = 200;
			peel->xvel = 144;
			peel->yvel = -12;
			peel->grav = 504;
			peel->xdest = peel->x + 70;
			peel->ydest = 256;

			ctx.objs[i].type = OBJ_BANANA_PEEL_MOVING;
			ctx.car.threw_peel = true;

			break;
		}
	}
}

//Updates the position of the player character (without taking solids into
//account, as solids are handled by handle_solids())
static void move_player()
{
	Player* pl = &ctx.player;

	if (pl->state == PLAYER_STATE_INACTIVE) return;

	//Deceleration and acceleration
	if (pl->xvel > 0 && pl->acc <= 0) {
		pl->xvel -= pl->dec * delta_time;
		if (pl->xvel <= 0) pl->xvel = 0;
	} else if (pl->xvel < 0 && pl->acc >= 0) {
		pl->xvel += pl->dec * delta_time;
		if (pl->xvel >= 0) pl->xvel = 0;
	} else {
		pl->xvel += pl->acc * delta_time;

		//Limit velocity
		if (pl->xvel < -90) pl->xvel = -90;
		if (pl->xvel > 210) pl->xvel = 210;
	}

	//Gravity
	pl->yvel += pl->grav * delta_time;
	if (pl->yvel > 300) pl->yvel = 300; //Limit velocity

	//Update position
	pl->x += pl->xvel * delta_time;
	pl->y += pl->yvel * delta_time;

	//Update position relative to the rope if grabbing one
	if (pl->state == PLAYER_STATE_GRABROPE) {
		if (pl->y >= 167) {
			pl->y = 167;
			pl->yvel = 0;
		}

		pl->x = ctx.grabbed_rope.x - 19;
	}
}

//Prevents the player character from moving across solids
static void handle_solids()
{
	Player* pl = &ctx.player;

	int pl_left   = (int)pl->oldx + PLAYER_BOX_OFFSET_X;
	int pl_right  = pl_left + PLAYER_BOX_WIDTH;
	int pl_top    = (int)pl->oldy;
	int pl_bottom = pl_top + pl->height;

	//Do the X axis (if the player character has moved in this axis)
	if (pl->x != pl->oldx) {
		bool moved_right = (pl->x > pl->oldx);
		int limit = moved_right ? 30000 : 0;
		int i;

		//Iterate through the solids to find the horizontal limit
		for (i = 0; i < MAX_SOLIDS; i++) {
			Solid* sol = &ctx.solids[i];

			//No more solids
			if (sol->type == NONE) break;

			//Only solids of type SOL_FULL are taken into account
			if (sol->type != SOL_FULL) continue;

			//Ignore solids that are out of the reach of the player character's
			//bounding box in the opposite axis (Y)
			if (sol->top >= pl_bottom || sol->bottom < pl_top) continue;

			if (moved_right) {
				if (sol->left < limit && sol->left >= pl_right) {
					limit = sol->left;
				}
			} else {
				if (sol->right > limit && sol->right <= pl_left) {
					limit = sol->right;
				}
			}
		}

		pl_left = (int)pl->x + PLAYER_BOX_OFFSET_X;
		pl_right = pl_left + PLAYER_BOX_WIDTH;

		if (moved_right) {
			if (pl_right >= limit) {
				pl_right = limit;
				pl_left = pl_right - PLAYER_BOX_WIDTH;
				pl->x = pl_left - PLAYER_BOX_OFFSET_X;
				pl->xvel = 0;
			}
		} else {
			if (pl_left <= limit) {
				pl_left = limit;
				pl_right = pl_left + PLAYER_BOX_WIDTH;
				pl->x = pl_left - PLAYER_BOX_OFFSET_X;
				pl->xvel = 0;
			}
		}
	}

	//Do the Y axis (if the player character has moved in this axis)
	if (pl->y != pl->oldy) {
		bool moved_down = (pl->y > pl->oldy);
		int limit = moved_down ? 30000 : 0;
		int ledge_right = 0;
		int i;

		//Detect if the player character's bounding box is on a ledge while
		//the sprite appears to be standing on the air, so we can prevent
		//this weird visual effect
		for (i = 0; i < MAX_SOLIDS; i++) {
			Solid* sol = &ctx.solids[i];
			int type = sol->type;

			//No more solids
			if (type == NONE) break;

			//Only solids of these two types are taken into account
			if (type != SOL_FULL && type != SOL_PASSAGEWAY_EXIT) continue;

			//Ignore solids that are out of the reach of the player character's
			//bounding box in the opposite axis (X)
			if (sol->left >= pl_right || sol->right <= pl_left) continue;

			if (pl->xvel != 0 || sol->top != pl_bottom) continue;

			if (ledge_right == 0) {
				if (type == SOL_FULL && sol->right <= pl_left + 4) {
					ledge_right = sol->right;
				}
			} else {
				//Prevent an undesirable ledge detection when the right side of
				//the player character's bounding box is on a passageway exit
				if (type == SOL_PASSAGEWAY_EXIT && sol->left <= pl_right) {
					ledge_right = 0;
				}
			}
		}

		//Iterate through the solids to find the vertical limit
		for (i = 0; i < MAX_SOLIDS; i++) {
			Solid* sol = &ctx.solids[i];

			//No more solids
			if (sol->type == NONE) break;

			//Ignore solids that are out of the reach of the player character's
			//bounding box in the opposite axis (X)
			if (sol->left >= pl_right || sol->right <= pl_left) continue;

			if (moved_down) {
				int type = sol->type;
				int top  = sol->top;
				bool check_limit;

				if (sol->bottom < pl_top) {
					continue;
				}

				if (type == SOL_PASSAGEWAY_ENTRY) {
					//When moving down, ignore passageway entry solids, which
					//are intended to prevent the player character from leaving
					//the passageway through the entry
					continue;
				}

				//Determine the top of slope position the player character is at
				if (type == SOL_SLOPE_UP && pl_right < sol->right) {
					top = sol->bottom + (sol->left - pl_right);
				} else if (type == SOL_SLOPE_DOWN && pl_left > sol->left) {
					top = sol->top - (sol->left - pl_left);
				}

				//Determine if the limit should be checked
				check_limit = false;
				if (top >= pl_bottom) {
					check_limit = true;
				} else if (type == SOL_SLOPE_UP) {
					check_limit = true;
				} else if (type == SOL_SLOPE_DOWN) {
					check_limit = true;
				} else if (type == SOL_KEEP_ON_TOP) {
					check_limit = true;
				}

				if (check_limit && top < limit) {
					limit = top;
				}
			} else {
				if (sol->type == SOL_PASSAGEWAY_EXIT && pl->yvel < -160) {
					//Ignore passageway exit solids if the player character is
					//moving upwards at a high enough velocity, as when hitting
					//a spring
					continue;
				}

				if (sol->bottom > limit && sol->bottom <= pl_top) {
					limit = sol->bottom;
				}
			}
		}

		if (moved_down) {
			if (pl->y + pl->height >= limit) {
				//Move the player character if on a ledge
				if (ledge_right != 0) {
					pl->x = ledge_right - PLAYER_BOX_OFFSET_X;
				}

				pl->y = limit - pl->height;
				pl->yvel = 0;
				pl->on_floor = true;
			}
		} else {
			if (pl->y <= limit) {
				pl->y = limit;
				pl->yvel = 0;
			}
		}
	}
}

//Acts if the player character is entering or leaving an underground
//passageway, which includes the vertical camera movement and opening the
//exit of the passageway
static void handle_passageways()
{
	Player* pl = &ctx.player;
	int pl_left = (int)pl->x + PLAYER_BOX_OFFSET_X;
	int pl_top = (int)pl->y;
	int pl_bottom = pl_top + pl->height;
	int i;

	for (i = 0; i < MAX_PASSAGEWAYS; i++) {
		Passageway* pw = &ctx.passageways[i];
		int pw_left = pw->x;
		int pw_entry_right = pw_left + LEVEL_BLOCK_SIZE;

		if (pw_left == NONE) {
			break; //No more passageways
		}

		//Check if the player character is entering a passageway
		if (ctx.cur_passageway == NONE && pl_bottom >= FLOOR_Y + 4) {
			if (pl_left > pw_left && pl_left < pw_entry_right) {
				ctx.cur_passageway = i;

				//Move camera down
				if (!ctx.time_up) {
					ctx.cam.yvel = CAMERA_YVEL;
				}
			}
		}
	}

	//Check if the player character is leaving a passageway
	if (ctx.cur_passageway != NONE) {
		Passageway* pw = &ctx.passageways[ctx.cur_passageway];
		int pw_right = pw->x + pw->width;

		if (pl_left > pw_right - 32) {
			//Check if the player character is opening the passageway exit, but
			//only if moving upwards at a high enough velocity, as is the case 
			//when hitting a spring
			if (pl->yvel < -162 && pl_top < FLOOR_Y + 8) {
				if (!pw->exit_opened) {
					audio_play_sfx(SFX_HOLE);
					add_crack_particles(pw_right - 16, 276);
					pw->exit_opened = true;
				}
			}

			if (pl_top < FLOOR_Y - 54) {
				ctx.cur_passageway = NONE;

				//Move camera up
				if (!ctx.time_up) {
					ctx.cam.yvel = -CAMERA_YVEL;
				}
			}
		}
	}
}

//Handles the interactions between the player character and most other objects
static void handle_player_interactions()
{
	Player* pl = &ctx.player;
	int pl_left = (int)pl->x + PLAYER_BOX_OFFSET_X;
	int pl_top = (int)pl->y;
	int pl_right = pl_left + PLAYER_BOX_WIDTH;
	int pl_bottom = pl_top + pl->height;
	bool collected_coin = false;
	bool slipped = false;
	bool thrown_back = false;
	int i, j;

	for (i = 0; i < MAX_OBJS; i++) {
		CoinSpark* spk;
		Obj* obj = &ctx.objs[i];
		int obj_left, obj_right, obj_top, obj_bottom;

		//Ignore inexistent objects
		if (obj->type == NONE) continue;

		//Ignore objects the player character does not interact with
		if (obj->type == OBJ_BANANA_PEEL_MOVING) continue;
		if (obj->type == OBJ_HYDRANT) continue;
		if (obj->type == OBJ_OVERHEAD_SIGN) continue;
		if (obj->type == OBJ_PARKED_CAR_BLUE) continue;
		if (obj->type == OBJ_PARKED_CAR_SILVER) continue;
		if (obj->type == OBJ_PARKED_CAR_YELLOW) continue;
		if (obj->type == OBJ_PARKED_TRUCK) continue;
		if (obj->type == OBJ_ROPE_HORIZONTAL) continue;

		//Except for coins, the player character only interacts with other
		//objects when in the normal state
		if (obj->type != OBJ_COIN_SILVER && obj->type != OBJ_COIN_GOLD) {
			if (pl->state != PLAYER_STATE_NORMAL) {
				continue;
			}
		}

		obj_left = obj->x;
		obj_top = obj->y;
		obj_right = obj_left;
		obj_bottom = obj_top;

		//Determine the bounding box of the object
		switch (obj->type) {
			case OBJ_BANANA_PEEL:
				obj_left += 1;
				obj_right = obj_left + 6;
				obj_top += 2;
				obj_bottom = obj_top;
				break;

			case OBJ_COIN_SILVER:
			case OBJ_COIN_GOLD:
				obj_left += 2;
				obj_right = obj_left + 4;
				obj_top += 2;
				obj_bottom = obj_top + 4;
				break;

			case OBJ_GUSH:
				obj_left += 3;
				obj_right = obj_left + 9;
				obj_bottom += 72;
				break;

			case OBJ_GUSH_CRACK:
				obj_left += 3;
				obj_right = obj_left + 10;
				break;

			case OBJ_ROPE_VERTICAL:
				obj_right += 4;
				obj_bottom += 64;
				break;

			case OBJ_SPRING:
				obj_right += 16;
				obj_top += 8;
				obj_bottom += 8;
				break;
		}

		if (obj->type == OBJ_ROPE_VERTICAL) {
			//For vertical ropes, check interaction using a point close to
			//the player character
			int px = (int)pl->x + 21;
			int py = (int)pl->y + 28;

			if (px < obj_left || px > obj_right)  continue;
			if (py < obj_top  || py > obj_bottom) continue;
		} else {
			//For other object types, check interaction using player
			//character's bounding box
			if (pl_right  < obj_left || pl_left > obj_right)  continue;
			if (pl_bottom < obj_top  || pl_top  > obj_bottom) continue;
		}

		switch (obj->type) {
			case OBJ_BANANA_PEEL:
				ctx.moving_peels[MOVING_PEEL_SLIPPED].obj = i;
				ctx.moving_peels[MOVING_PEEL_SLIPPED].x = obj->x;
				ctx.moving_peels[MOVING_PEEL_SLIPPED].y = obj->y;
				obj->type = OBJ_BANANA_PEEL_MOVING;
				slipped = true;
				break;

			case OBJ_COIN_SILVER:
			case OBJ_COIN_GOLD:
				collected_coin = true;
				ctx.score += (obj->type == OBJ_COIN_GOLD) ? 100 : 50;

				//Add spark
				spk = &ctx.coin_sparks[ctx.next_coin_spark];
				spk->x = obj->x;
				spk->y = obj->y;
				spk->gold = (obj->type == OBJ_COIN_GOLD);

				start_animation(ANIM_COIN_SPARKS + ctx.next_coin_spark);

				ctx.next_coin_spark++;
				ctx.next_coin_spark %= MAX_COIN_SPARKS;

				//Remove the coin
				obj->type = NONE;

				break;

			case OBJ_GUSH:
				thrown_back = true;
				break;

			case OBJ_GUSH_CRACK:
				obj->type = OBJ_GUSH;

				for (j = 0; j < MAX_GUSHES; j++) {
					if (ctx.gushes[j].obj == NONE) {
						ctx.gushes[j].obj = i;
						ctx.gushes[j].y = 266;
						ctx.gushes[j].move_pattern = data_gush_move_pattern_2;
						ctx.gushes[j].move_pattern_pos = 0;
						ctx.gushes[j].yvel = -144;
						ctx.gushes[j].ydest = data_gush_move_pattern_2[1];

						add_crack_particles(obj->x + 6, 276);

						if (pl->state == PLAYER_STATE_NORMAL) {
							thrown_back = true;
						}

						break;
					}
				}

				break;

			case OBJ_ROPE_VERTICAL:
				if (ctx.grabbed_rope.obj == i) {
					//Cannot grab the same rope again right after releasing it
					if (ctx.grabbed_rope.x > ctx.grabbed_rope.xmax - 64) {
						break;
					}
				} else if (ctx.grabbed_rope.obj != NONE) {
					Obj* rope = &ctx.objs[ctx.grabbed_rope.obj];
					rope->x = (int)ctx.grabbed_rope.xmin;
					ctx.grabbed_rope.obj = NONE;
				}

				if (ctx.grabbed_rope.obj == NONE) {
					ctx.grabbed_rope.xmin = obj->x;
					ctx.grabbed_rope.xmax = obj->x + 352;
				}

				pl->state = PLAYER_STATE_GRABROPE;
				ctx.grabbed_rope.obj = i;
				ctx.grabbed_rope.x = obj->x;
				ctx.grabbed_rope.xvel = 258;

				break;

			case OBJ_SPRING:
				if (pl->yvel >= 0) {
					audio_play_sfx(SFX_SPRING);
					pl->yvel = -246;
					ctx.hit_spring = i;
					start_animation(ANIM_HIT_SPRING);
				}
				break;
		}
	}

	//Play a sound effect if the player character has collected a coin
	if (collected_coin) {
		audio_play_sfx(SFX_COIN);
	}

	//Act if the player character has slipped on a banana peel
	if (slipped) {
		MovingPeel* peel = &ctx.moving_peels[MOVING_PEEL_SLIPPED];

		audio_play_sfx(SFX_SLIP);
		pl->state = PLAYER_STATE_SLIP;

		peel->xvel = 150;
		peel->yvel = -204;
		peel->grav = 504;

		//For the destination Y position, use a value below the limit,
		//which is 400
		peel->xdest = 0;
		peel->ydest = 500;

	}

	//Act if the player character has been thrown back by a gush
	if (thrown_back) {
		audio_play_sfx(SFX_HIT);
		pl->state = PLAYER_STATE_THROWBACK;
	}

	//Handle pushable crates
	if (!input_right) ctx.crate_push_remaining = 0.75f;
	for (i = 0; i < MAX_PUSHABLE_CRATES; i++) {
		PushableCrate* crate = &ctx.pushable_crates[i];
		Solid* sol;
		int x = (int)ctx.player.x + 24;
		int y = (int)ctx.player.y + 48;

		//Skip crates that do not exist or have been pushed
		if (crate->obj == NONE || crate->pushed) continue;

		//If the point does not overlap the crate's solid, then the player
		//is not pushing the crate
		sol = &ctx.solids[crate->solid];
		if (sol->type == NONE) continue;
		if (x < sol->left) continue;
		if (x > sol->right) continue;
		if (y < sol->top) continue;
		if (y > sol->bottom) continue;

		//If we got here, then the player is pushing the crate
		ctx.crate_push_remaining -= delta_time;
		if (ctx.crate_push_remaining <= 0) {
			//Finished pushing
			ctx.crate_push_remaining = 0.75f;
			crate->show_arrow = false;
			crate->pushed = true;
			audio_play_sfx(SFX_CRATE);
		}
	}
}

//Acts when the player character reaches the X position of a trigger, which
//causes the appearance of a passing car or hen
static void handle_triggers()
{
	int plx = (int)ctx.player.x;
	int i;

	for (i = 0; i < MAX_TRIGGERS; i++) {
		Trigger* tr = &ctx.triggers[i];

		//Ignore triggers that do not exist or the player character has not
		//reached
		if (tr->x == NONE || tr->x > plx) continue;

		if (tr->what == TRIGGER_HEN) {
			ctx.hen.x = tr->x - (VSCREEN_MAX_WIDTH / 2) - 32;
			ctx.hen.xvel = 360;
			ctx.hen.acc = 0;
			start_animation(ANIM_HEN);
		} else { //If not a hen, then trigger a passing car
			ctx.car.x = tr->x - (VSCREEN_MAX_WIDTH / 2) - 128;
			ctx.car.xvel = 1200;
			ctx.car.type = tr->what;
			ctx.car.threw_peel = false;
			ctx.car.peel_throw_x = tr->x + 72;
			start_animation(ANIM_CAR_WHEELS);
		}

		tr->x = NONE;
	}
}

//Does the specifics of the player character's current state
static void do_player_state_specifics()
{
	Player* pl = &ctx.player;
	bool state_changed = (pl->state != pl->old_state);

	if (pl->state == PLAYER_STATE_NORMAL) {
		pl->acc = 0;
		if (input_right) {
			pl->acc = 210;
		} else if (input_left) {
			pl->acc = -210;
		}

		//Jump
		if (pl->on_floor && jump_timeout > 0) {
			pl->yvel = -156;
			jump_timeout = 0;
		}

		//Decide animation type
		if (!pl->on_floor) {
			pl->anim_type = PLAYER_ANIM_JUMP;
		} else if (pl->xvel > 0) {
			pl->anim_type = PLAYER_ANIM_WALK;
		} else if (pl->xvel < 0) {
			pl->anim_type = PLAYER_ANIM_WALKBACK;
		} else {
			pl->anim_type = PLAYER_ANIM_STAND;
		}
	} else if (pl->state == PLAYER_STATE_SLIP) {
		if (!state_changed && pl->on_floor) {
			pl->xvel = 0;

			//Get up on player input
			if (input_left && !old_input_left) {
				pl->state = PLAYER_STATE_GETUP;
			}
			if (input_right && !old_input_right) {
				pl->state = PLAYER_STATE_GETUP;
			}
			if (input_jump && !old_input_jump) {
				pl->state = PLAYER_STATE_GETUP;
			}
		}
	} else if (pl->state == PLAYER_STATE_GETUP) {
		//Prevent jump if the button is held until the character finishes
		//getting up
		jump_timeout = 0;

		if (pl->yvel >= 0) {
			pl->height = PLAYER_HEIGHT_NORMAL;
			if (pl->on_floor) {
				pl->state = PLAYER_STATE_NORMAL;
			}
		}
	} else if (pl->state == PLAYER_STATE_THROWBACK) {
		if (!state_changed && pl->on_floor) {
			pl->state = PLAYER_STATE_NORMAL;
		}
	} else if (pl->state == PLAYER_STATE_GRABROPE) {
		GrabbedRope rope = ctx.grabbed_rope;
		if (pl->x < rope.xmax - 16 && rope.xvel <= 0) {
			//Release the rope
			pl->state = PLAYER_STATE_NORMAL;
		}
	} else if (pl->state == PLAYER_STATE_FLICKER) {
		//Prevent jump if the button is held until the flicker finishes
		jump_timeout = 0;

		pl->visible = !pl->visible;

		pl->flicker_delay -= delta_time;
		if (pl->flicker_delay <= 0) {
			pl->state = PLAYER_STATE_NORMAL;
		}
	}
}

//Checks if the player character has fallen into a deep hole on the ground
//and plays the fall sound effect if so
static void handle_fall_sound()
{
	Player* pl = &ctx.player;
	int pl_bottom = (int)pl->y + pl->height;
	bool in_passageway = (ctx.cur_passageway != NONE);

	if (!ctx.time_up && !pl->fell && !in_passageway) {
		if (pl_bottom > FLOOR_Y + 8 && pl->yvel > 0) {
			audio_play_sfx(SFX_FALL);
			pl->fell = true;
		}
	}
}

//Handles the respawning (reappearance) of the player character after
//falling into a deep hole
static void handle_respawn()
{
	int rx = 0, ry = 0;
	int i;

	//No respawn on time up or if the player character's Y position is
	//above (lower than) 324
	if (ctx.time_up || ctx.player.y < 324) return;

	for (i = 0; i < MAX_RESPAWN_POINTS; i++) {
		RespawnPoint* rp = &ctx.respawn_points[i];

		//Leave the loop on the first respawn point that does not exist or
		//is to the right of the player character
		if (rp->x == NONE || rp->x > ctx.player.x) break;

		rx = rp->x;
		ry = rp->y;
	}

	ctx.player.x = rx;
	ctx.player.y = ry;
	ctx.player.oldx = rx;
	ctx.player.oldy = ry;
	ctx.player.state = PLAYER_STATE_FLICKER;
	ctx.player.fell = false;

	//Retreat camera if needed
	if (ctx.cam.x > rx - 64) {
		ctx.cam.xdest = rx - 64;
		ctx.cam.xvel = -CAMERA_XVEL;
	}

	audio_stop_sfx(SFX_FALL);
	audio_play_sfx(SFX_RESPAWN);
}

//Acts if the player character's state has changed
static void handle_player_state_change()
{
	Player* pl = &ctx.player;

	//Nothing to do if the state has not changed
	if (pl->state == pl->old_state) return;

	pl->acc = 0;
	pl->dec = 0;
	pl->height = PLAYER_HEIGHT_NORMAL;
	pl->visible = true;

	switch (pl->state) {
		case PLAYER_STATE_NORMAL:
			pl->dec = 252;
			pl->grav = 234;
			break;

		case PLAYER_STATE_SLIP:
			pl->xvel = -12;
			pl->yvel = -24;
			pl->height = PLAYER_HEIGHT_SLIP;
			pl->anim_type = PLAYER_ANIM_SLIP;
			break;

		case PLAYER_STATE_GETUP:
			pl->xvel = 0;
			pl->yvel = -120;
			pl->height = PLAYER_HEIGHT_SLIP;
			pl->anim_type = PLAYER_ANIM_SLIPREV;
			break;

		case PLAYER_STATE_THROWBACK:
			pl->xvel = -102;
			pl->yvel = -144;
			pl->anim_type = PLAYER_ANIM_THROWBACK;
			break;

		case PLAYER_STATE_GRABROPE:
			pl->grav = 0;
			pl->xvel = 0;
			pl->yvel = 120;
			pl->anim_type = PLAYER_ANIM_GRABROPE;
			break;

		case PLAYER_STATE_FLICKER:
			pl->flicker_delay = 0.5f;
			pl->grav = 0;
			pl->xvel = 0;
			pl->yvel = 0;
			pl->anim_type = PLAYER_ANIM_STAND;
			break;

		case PLAYER_STATE_INACTIVE:
			pl->x = -1;
			pl->y = -1;
			pl->xvel = 0;
			pl->yvel = 0;
			pl->acc = 0;
			pl->grav = 0;
			pl->visible = false;
			break;
	}
}

//Updates the position of the camera
static void move_camera()
{
	PlayCamera* cam = &ctx.cam;

	//Horizontal camera movement
	if (cam->xvel != 0) {
		cam->x += cam->xvel * delta_time;

		if (cam->xvel > 0 && cam->x >= cam->xdest) {
			cam->xvel = 0;
		} else if (cam->xvel < 0 && cam->x <= cam->xdest) {
			cam->xvel = 0;
		}
	}

	//Vertical camera movement
	if (cam->yvel != 0) {
		cam->y += cam->yvel * delta_time;
		if (cam->yvel < 0 && cam->y <= 0) {
			cam->y = 0;
			cam->yvel = 0;
		} else if (cam->yvel > 0 && cam->y >= 95) {
			cam->y = 95;
			cam->yvel = 0;
		}
	}

	position_camera();
}

//Prevents the player character from moving off the level's boundaries
static void keep_player_within_limits()
{
	if (ctx.player.x < 48) {
		ctx.player.x = 48;
		ctx.player.xvel = 0;

		if (ctx.player.on_floor) {
			ctx.player.anim_type = PLAYER_ANIM_STAND;
		}
	}
}

//Acts if the player character's animation type has changed
static void handle_player_animation_change()
{
	int anim_type = ctx.player.anim_type;

	//Nothing to do if the animation has not changed
	if (anim_type == ctx.player.old_anim_type) return;

	switch (anim_type) {
		case PLAYER_ANIM_STAND:
			set_animation(ANIM_PLAYER, true, false, false, 1, 0.0f);
			break;

		case PLAYER_ANIM_WALK:
			set_animation(ANIM_PLAYER, true, true,  false, 6, 0.1f);
			break;

		case PLAYER_ANIM_WALKBACK:
			set_animation(ANIM_PLAYER, true, true,  true,  6, 0.1f);
			break;

		case PLAYER_ANIM_JUMP:
			set_animation(ANIM_PLAYER, true, true,  false, 1, 0.0f);
			break;

		case PLAYER_ANIM_SLIP:
			set_animation(ANIM_PLAYER, true, false, false, 4, 0.05f);
			break;

		case PLAYER_ANIM_SLIPREV:
			set_animation(ANIM_PLAYER, true, false, true,  4, 0.05f);
			break;

		case PLAYER_ANIM_THROWBACK:
			set_animation(ANIM_PLAYER, true, false, false, 3, 0.05f);
			break;

		case PLAYER_ANIM_GRABROPE:
			set_animation(ANIM_PLAYER, true, false, false, 1, 0.05f);
			break;
	}
}

//Updates all animations
static void update_animations()
{
	int i;

	//Set animation speed for bus wheels
	ctx.anims[ANIM_BUS_WHEELS].running = false;
	if (ctx.bus.xvel > 0) {
		float max_delay = 0.1f;
		if (ctx.bus.xvel > 64)  max_delay = 0.05f;
		if (ctx.bus.xvel > 128) max_delay = 0.025f;

		ctx.anims[ANIM_BUS_WHEELS].running = true;
		ctx.anims[ANIM_BUS_WHEELS].max_delay = max_delay;

		if (ctx.anims[ANIM_BUS_WHEELS].delay > max_delay) {
			ctx.anims[ANIM_BUS_WHEELS].delay = max_delay;
		}
	}

	//Update animations
	for (i = 0; i < NUM_ANIMS; i++) {
		Anim* anim = &ctx.anims[i];

		if (!anim->running) continue;

		anim->delay -= delta_time;
		if (anim->delay > 0) continue;

		anim->delay = anim->max_delay;

		if (anim->reverse) {
			anim->frame--;

			if (anim->frame < 0) {
				anim->frame = anim->loop ? anim->num_frames - 1 : 0;
			}
		} else {
			anim->frame++;

			if (anim->frame >= anim->num_frames) {
				anim->frame = anim->loop ? 0 : anim->num_frames - 1;
			}
		}
	}
}

//Moves the arrows indicating that a crate is pushable
static void move_push_arrow()
{
	ctx.push_arrow.xoffs += ctx.push_arrow.xvel * delta_time;
	if (ctx.push_arrow.xoffs >= 8) {
		ctx.push_arrow.xoffs = 8;
		ctx.push_arrow.xvel = -30;
	}
	if (ctx.push_arrow.xvel < 0 && ctx.push_arrow.xoffs <= 0) {
		ctx.push_arrow.xoffs = 0;
		ctx.push_arrow.xvel = 0;
	}

	ctx.push_arrow.delay -= delta_time;
	if (ctx.push_arrow.delay <= 0) {
		ctx.push_arrow.delay = 0;

		if (ctx.push_arrow.xoffs == 0) {
			ctx.push_arrow.xvel = 30;
			ctx.push_arrow.delay = 1;
		}
	}
}

//Positions the bus stop sign
static void position_bus_stop_sign()
{
	if (ctx.level_num == 1 || ctx.cam.x > VSCREEN_MAX_WIDTH) {
		//The sign is at the end of the level
		ctx.bus_stop_sign_x = ctx.level_size - 40;
	} else {
		//The sign is at the start of the level
		ctx.bus_stop_sign_x = 176;
	}
}

//Positions the first light pole (the position of the second pole is calculated
//later when rendering)
static void position_light_pole()
{
	int camx = (int)ctx.cam.x + (VSCREEN_MAX_WIDTH / 2);
	ctx.pole_x = camx - (camx % POLE_DISTANCE) + 16;
}

//Updates the sequences, like the player character entering the bus when the
//level's goal is reached
//
//Normal play (SEQ_NORMAL_PLAY) is treated as one of the sequences and is where
//the start of a "goal reached" or "time up" sequence is checked
static void update_sequence()
{
	Player* pl = &ctx.player;
	Bus* bus = &ctx.bus;
	PlayCamera* cam = &ctx.cam;
	int level_size = ctx.level_size;

	MovingPeel* thrown_peel = &ctx.moving_peels[MOVING_PEEL_THROWN];

	//Cutscene objects
	CutsceneObject* cutscene_player = &ctx.cutscene_objects[0];
	CutsceneObject* bearded_man = &ctx.cutscene_objects[1];
	CutsceneObject* bird = &ctx.cutscene_objects[1];
	CutsceneObject* dung = &ctx.cutscene_objects[0];
	CutsceneObject* flagman = &ctx.cutscene_objects[1];

	//Cutscene object animations
	Anim* cutscene_player_anim = &ctx.anims[ANIM_CUTSCENE_OBJECTS + 0];
	Anim* bearded_man_anim = &ctx.anims[ANIM_CUTSCENE_OBJECTS + 1];
	Anim* bird_anim = &ctx.anims[ANIM_CUTSCENE_OBJECTS + 1];
	Anim* flagman_anim = &ctx.anims[ANIM_CUTSCENE_OBJECTS + 1];

	ctx.wipe_in = false;
	ctx.wipe_out = false;

	ctx.sequence_delay -= delta_time;
	if (ctx.sequence_delay > 0) return;

	ctx.sequence_delay = 0;

	switch (ctx.sequence_step) {
		//----------------------------------------------------------------------
		case 0: //SEQ_NORMAL_PLAY_START
			move_bus_to_end();
			ignore_user_input = false;
			cam->follow_player = true;
			cam->fixed_at_leftmost = false;
			ctx.time_running = true;
			ctx.time_delay = 1;
			ctx.can_pause = true;
			ctx.sequence_step = SEQ_NORMAL_PLAY;
			break;


		//----------------------------------------------------------------------
		case 1: //SEQ_NORMAL_PLAY
			if (pl->x >= level_size - 426) {
				ctx.goal_reached = true;
				ctx.time_up = false;
			}
			if (ctx.time_up || ctx.goal_reached) {
				ctx.can_pause = false;
				ctx.time_running = false;
				ignore_user_input = true;
				input_left = false;
				input_right = false;
				input_jump = false;
				jump_timeout = 0;

				if (ctx.time_up) {
					ctx.sequence_delay = 1;
					if (pl->x >= level_size - 960) {
						ctx.sequence_step = SEQ_TIMEUP_BUS_NEAR;
					} else {
						ctx.sequence_step = SEQ_TIMEUP_BUS_FAR;
					}
				} else { //Goal reached
					input_right = true;
					ctx.sequence_step = SEQ_GOAL_REACHED;
				}
			}
			break;


		//----------------------------------------------------------------------
		case 10: //SEQ_INITIAL
			//Start with bus rear door open
			ctx.anims[ANIM_BUS_DOOR_REAR].frame = 3;
			ctx.anims[ANIM_BUS_DOOR_REAR].reverse = true;

			ignore_user_input = true;
			ctx.time_running = false;

			if (ctx.level_num == 1 || ctx.skip_initial_sequence) {
				move_bus_to_end();
				ctx.sequence_step = SEQ_NORMAL_PLAY_START;
			} else {
				ctx.sequence_step++;
			}
			ctx.sequence_delay = 1;

			break;

		case 11:
			start_animation(ANIM_BUS_DOOR_REAR);
			bus->acc = 252;
			bus->xvel = 6;
			ctx.sequence_delay = 2;
			ctx.sequence_step = SEQ_NORMAL_PLAY_START;
			break;


		//----------------------------------------------------------------------
		case 20: //SEQ_BUS_LEAVING
			//Bus leaves while closing the front door
			start_animation(ANIM_BUS_DOOR_FRONT);
			bus->acc = 252;
			bus->xvel = 6;
			ctx.sequence_delay = 2;
			ctx.sequence_step++;
			break;

		case 21:
			//Screen wipes to black
			ctx.wipe_out = true;
			ctx.sequence_delay = 1;
			ctx.sequence_step++;
			break;

		case 22:
			ctx.sequence_step = SEQ_FINISHED;
			break;


		//----------------------------------------------------------------------
		case 30: //SEQ_TIMEUP_BUS_NEAR
			//Camera moves towards the bus
			if (ctx.car.x != NONE) break; //Wait until the car and hen are
			if (ctx.hen.x != NONE) break; //not visible anymore
			cam->follow_player = false;
			cam->xdest = level_size;
			cam->xvel = CAMERA_XVEL;
			cam->yvel = 0;
			ctx.sequence_step++;
			break;

		case 31:
			//Camera stops and bus leaves
			if (cam->xvel != 0) break;
			if (cam->yvel != 0) break;
			cam->fixed_at_rightmost = true;
			ctx.sequence_delay = 0.2f;
			ctx.sequence_step = SEQ_BUS_LEAVING;
			break;


		//----------------------------------------------------------------------
		case 40: //SEQ_TIMEUP_BUS_FAR
			//Screen wipes to black
			cam->follow_player = false;
			cam->xvel = 0;
			cam->yvel = 0;
			ctx.wipe_out = true;
			ctx.sequence_delay = 0.6f;
			ctx.sequence_step++;
			break;

		case 41:
			//Camera is placed so the bus is visible and screen wipes from
			//black
			pl->state = PLAYER_STATE_INACTIVE;
			cam->x = cam->xmax;
			cam->y = 0;
			cam->fixed_at_rightmost = true;
			ctx.car.x = NONE;
			ctx.hen.x = NONE;
			ctx.wipe_in = true;
			ctx.sequence_delay = 0.6f;
			ctx.sequence_step++;
			break;

		case 42:
			ctx.sequence_step = SEQ_BUS_LEAVING;
			break;


		//----------------------------------------------------------------------
		case 50: //SEQ_GOAL_REACHED
			if (ctx.goal_scene == 3) {
				if (pl->x > bus->x + 192) {
					//A banana peel is thrown from the right side of the screen
					ctx.objs[0].type = OBJ_BANANA_PEEL_MOVING;
					ctx.objs[0].x = level_size;
					ctx.objs[0].y = BUS_Y + 72;
					thrown_peel->obj = 0;
					thrown_peel->x = ctx.objs[0].x;
					thrown_peel->y = ctx.objs[0].y;
					thrown_peel->xvel = -510;
					thrown_peel->yvel = 204;
					thrown_peel->grav = 504;
					thrown_peel->xdest = (int)bus->x + 345;
					thrown_peel->ydest = 256;
					ctx.sequence_step++;
				}
			} else if (ctx.goal_scene == 4) {
				if (pl->x >= bus->x + 120) {
					//A bird appears
					bird->sprite = SPR_BIRD;
					bird->x = cam->x - 16;
					bird->y = 120;
					bird->xvel = 300;
					bird_anim->running = true;
					bird_anim->frame = 0;
					bird_anim->num_frames = 4;
					bird_anim->delay = 0.1f;
					bird_anim->max_delay = 0.1f;
					bird_anim->loop = true;
					ctx.sequence_step++;
				}
			} else {
				ctx.sequence_step++;
			}
			break;

		case 51:
			if (pl->x >= bus->x + 256) {
				//Player character decelerates
				pl->x = bus->x + 256;
				input_right = false;
				ctx.sequence_step++;
			}
			break;

		case 52:
			if (pl->state == PLAYER_STATE_SLIP) {
				input_right = false;
				ctx.sequence_step++;
			} else if (bird->sprite == SPR_BIRD) {
				ctx.sequence_step++;
			} else if (pl->xvel <= 0 || pl->x >= bus->x + 342) {
				//Player character jumps into the bus
				pl->x = bus->x + 342;
				pl->xvel = 0;
				jump_timeout = JUMP_TIMEOUT; //Trigger a jump
				ctx.sequence_step++;
			}
			break;

		case 53:
			ctx.sequence_step = SEQ_GOAL_REACHED_SCENE1;
			switch (ctx.goal_scene) {
				case 2: ctx.sequence_step = SEQ_GOAL_REACHED_SCENE2; break;
				case 3: ctx.sequence_step = SEQ_GOAL_REACHED_SCENE3; break;
				case 4: ctx.sequence_step = SEQ_GOAL_REACHED_SCENE4; break;
				case 5: ctx.sequence_step = SEQ_GOAL_REACHED_SCENE5; break;
			}
			break;


		//----------------------------------------------------------------------
		case 60: //SEQ_GOAL_REACHED_SCENE1
			input_jump = false;
			if (pl->yvel > 0 && pl->y >= BUS_Y + 36) {
				//Player character is now in the bus and score count starts
				show_player_in_bus();
				start_score_count();
				ctx.sequence_step++;
			}
			break;

		case 61:
			if (!ctx.counting_score) {
				//Score count finished
				ctx.sequence_delay = 0.5f;
				ctx.sequence_step++;
			}
			break;

		case 62:
			ctx.sequence_step = SEQ_BUS_LEAVING;
			break;


		//----------------------------------------------------------------------
		case 70: //SEQ_GOAL_REACHED_SCENE2
			input_jump = false;
			if (pl->yvel > 0 && pl->y >= BUS_Y + 36) {
				//Player character is now in the bus and score count starts
				show_player_in_bus();
				start_score_count();
				ctx.sequence_step++;
			}
			break;

		case 71:
			if (!ctx.counting_score) {
				//Score count finished
				ctx.sequence_delay = 0.5f;
				ctx.sequence_step++;
			}
			break;

		case 72:
			//Bus front door closes
			start_animation(ANIM_BUS_DOOR_FRONT);
			ctx.sequence_delay = 0.5f;
			ctx.sequence_step++;
			break;

		case 73:
			//Bearded man comes from the right side of the screen
			cutscene_player->sprite = NONE;
			bearded_man->sprite = SPR_BEARDED_MAN_WALK;
			bearded_man->x = ctx.level_size;
			bearded_man->y = 203;
			bearded_man->xvel = -150;
			bearded_man_anim->running = true;
			bearded_man_anim->frame = 0;
			bearded_man_anim->num_frames = 6;
			bearded_man_anim->delay = 0.1f;
			bearded_man_anim->max_delay = 0.1f;
			bearded_man_anim->loop = true;
			ctx.sequence_step++;
			break;

		case 74:
			if (bearded_man->x <= bus->x + 380) {
				//Bearded man decelerates
				bearded_man->x = bus->x + 380;
				bearded_man->acc = 252;
				ctx.sequence_step++;
			}
			break;

		case 75:
			if (bearded_man->xvel >= 0 || bearded_man->x <= bus->x + 337) {
				//Bearded man stops and bus front door opens
				bearded_man->sprite = SPR_BEARDED_MAN_STAND;
				bearded_man->x = bus->x + 337;
				bearded_man->xvel = 0;
				bearded_man->acc = 0;
				bearded_man_anim->frame = 0;
				bearded_man_anim->num_frames = 1;
				ctx.anims[ANIM_BUS_DOOR_FRONT].reverse = false;
				start_animation(ANIM_BUS_DOOR_FRONT);
				ctx.sequence_delay = 0.5f;
				ctx.sequence_step++;
			}
			break;

		case 76:
			//Bearded man jumps into the bus
			bearded_man->sprite = SPR_BEARDED_MAN_JUMP;
			bearded_man->yvel = -156;
			bearded_man->grav = 234;
			ctx.sequence_step++;
			break;

		case 77:
			if (bearded_man->y >= BUS_Y + 35 && bearded_man->yvel > 0) {
				//Bearded man is now in the bus
				bearded_man->sprite = SPR_BEARDED_MAN_STAND;
				bearded_man->grav = 0;
				bearded_man->yvel = 0;
				bearded_man->x -= bus->x; //Make position relative to the bus
				bearded_man->y = BUS_Y + 35;
				bearded_man->in_bus = true;
				ctx.sequence_delay = 0.25f;
				ctx.sequence_step++;
			}
			break;

		case 78:
			ctx.anims[ANIM_BUS_DOOR_FRONT].reverse = true;
			ctx.sequence_step = SEQ_BUS_LEAVING;
			break;


		//----------------------------------------------------------------------
		case 80: //SEQ_GOAL_REACHED_SCENE3
			//Player character slips on a banana peel and hits the floor
			if (pl->on_floor) {
				ctx.sequence_delay = 0.25f;
				ctx.sequence_step++;
			}
			break;

		case 81:
			input_right = !input_right;
			old_input_right = !input_right;
			if (pl->state == PLAYER_STATE_GETUP) {
				//Player character gets up after slipping on a banana peel
				//and starts walking again
				input_right = true;
				old_input_right = false;
				ctx.sequence_step++;
			}
			break;

		case 82:
			if (pl->x >= bus->x + 342) {
				//Player character jumps into the bus
				pl->x = bus->x + 342;
				pl->xvel = 0;
				input_right = false;
				jump_timeout = JUMP_TIMEOUT; //Trigger a jump
				ctx.sequence_step++;
			}
			break;

		case 83:
			if (pl->yvel > 0 && pl->y >= BUS_Y + 36) {
				//Player character is now in the bus and score count starts
				show_player_in_bus();
				start_score_count();
				ctx.sequence_step++;
			}
			break;

		case 84:
			if (!ctx.counting_score) {
				//Score count finished
				ctx.sequence_delay = 0.25f;
				ctx.sequence_step++;
			}
			break;

		case 85:
			ctx.sequence_step = SEQ_BUS_LEAVING;
			break;


		//----------------------------------------------------------------------
		case 90: //SEQ_GOAL_REACHED_SCENE4
			if (pl->x >= bus->x + 342) {
				//Player character stops at bus front door
				pl->x = bus->x + 342;
				pl->xvel = 0;
			}
			if (bird->x >= bus->x + 354) {
				//Bird dung appears
				dung->sprite = SPR_DUNG;
				dung->x = bus->x + 354;
				dung->y = bird->y;
				dung->yvel = 252;
				ctx.sequence_step++;
			}
			break;

		case 91:
			if (dung->y >= pl->y + 12) {
				//Bird dung hits the player character
				dung->sprite = NONE;
				dung->yvel = 0;
				pl->visible = false;
				cutscene_player->sprite = SPR_PLAYER_CLEAN_DUNG;
				cutscene_player->x = pl->x;
				cutscene_player->y = pl->y;
				ctx.sequence_delay = 0.25f;
				ctx.sequence_step++;
			}
			break;

		case 92:
			//Player character cleans the dung
			cutscene_player_anim->running = true;
			cutscene_player_anim->frame = 0;
			cutscene_player_anim->num_frames = 9;
			cutscene_player_anim->delay = 0.2f;
			cutscene_player_anim->max_delay = 0.2f;
			cutscene_player_anim->loop = false;
			ctx.sequence_delay = 2.0f;
			ctx.sequence_step++;
			break;

		case 93:
			//Player character finishes cleaning the dung
			pl->visible = true;
			cutscene_player->sprite = NONE;
			ctx.sequence_delay = 0.25f;
			ctx.sequence_step++;
			break;

		case 94:
			//Player character jumps into the bus
			jump_timeout = JUMP_TIMEOUT; //Trigger a jump
			ctx.sequence_step++;
			break;

		case 95:
			if (pl->yvel > 0 && pl->y >= BUS_Y + 36) {
				//Player character is now in the bus and score count starts
				show_player_in_bus();
				start_score_count();
				ctx.sequence_step++;
			}
			break;

		case 96:
			if (!ctx.counting_score) {
				//Score count finished
				ctx.sequence_delay = 0.5f;
				ctx.sequence_step++;
			}
			break;

		case 97:
			ctx.sequence_step = SEQ_BUS_LEAVING;
			break;


		//----------------------------------------------------------------------
		case 100: //SEQ_GOAL_REACHED_SCENE5
			//Bus leaves before the player character can enter it
			start_animation(ANIM_BUS_DOOR_FRONT);
			bus->acc = 252;
			bus->xvel = 6;
			ctx.sequence_step++;
			break;

		case 101:
			if (bus->x >= ctx.level_size + 32) {
				//Player character starts running crazily
				bus->acc = 0;
				bus->xvel = 0;
				pl->visible = false;
				cutscene_player->sprite = SPR_PLAYER_RUN;
				cutscene_player->x = pl->x;
				cutscene_player->y = pl->y;
				cutscene_player->xvel = 126;
				cutscene_player->acc = 504;
				cutscene_player_anim->running = true;
				cutscene_player_anim->num_frames = 4;
				cutscene_player_anim->loop = true;
				cutscene_player_anim->delay = 0.1f;
				cutscene_player_anim->max_delay = 0.1f;
				ctx.sequence_step++;
			}
			break;

		case 102:
			if (cutscene_player->x >= ctx.level_size + 32) {
				//Score count starts
				start_score_count();
				cutscene_player->xvel = 0;
				ctx.sequence_step++;
			}
			break;

		case 103:
			if (!ctx.counting_score) {
				//Score count finished
				ctx.sequence_delay = 0.5f;
				ctx.sequence_step++;
			}
			break;

		case 104:
			//Screen wipes to black
			ctx.wipe_out = true;
			ctx.sequence_delay = 1;
			ctx.sequence_step++;
			break;

		case 105:
			ctx.sequence_step = SEQ_FINISHED;
			break;


		//----------------------------------------------------------------------
		case 110: //SEQ_ENDING
			//Ending sequence, with a traffic jam and a flagman
			pl->visible = false;
			pl->state = PLAYER_STATE_INACTIVE;

			cam->x = VSCREEN_MAX_WIDTH + 24;

			bus->x = 96;
			bus->xvel = 0;
			bus->route_sign = 0; //Finish (checkered flag) sign

			flagman->sprite = SPR_FLAGMAN;
			flagman->x = VSCREEN_MAX_WIDTH * 2 + 32;
			flagman->y = 180;
			flagman_anim->running = false;
			flagman_anim->loop = false;
			flagman_anim->reverse = false;
			flagman_anim->frame = 3;
			flagman_anim->num_frames = 4;
			flagman_anim->delay = 0.1f;
			flagman_anim->max_delay = 0.1f;

			ctx.sequence_delay = 1;
			ctx.sequence_step++;
			break;

		case 111:
			//Camera moves to the right
			cam->xvel = CAMERA_XVEL / 4;
			cam->xdest = VSCREEN_MAX_WIDTH * 2 - 136;
			ctx.sequence_delay = 3;
			ctx.sequence_step++;
			break;

		case 112:
			//Traffic jam starts moving
			bus->xvel = 60;
			ctx.anims[ANIM_CAR_WHEELS].delay = 0.1f;
			ctx.anims[ANIM_CAR_WHEELS].max_delay = 0.1f;
			start_animation(ANIM_CAR_WHEELS);
			ctx.sequence_step++;
			break;

		case 113:
			if (bus->x >= 232) {
				//Traffic jam stops
				bus->x = 232;
				bus->xvel = 0;
				ctx.anims[ANIM_CAR_WHEELS].running = false;
				ctx.anims[ANIM_CAR_WHEELS].frame = 0;
				ctx.sequence_delay = 1;
				ctx.sequence_step++;
			}
			break;

		case 114:
			//Player character appears from the left side of the screen and
			//is running crazily
			cutscene_player->sprite = SPR_PLAYER_RUN;
			cutscene_player->x = cam->x - 80;
			cutscene_player->y = 204;
			cutscene_player->xvel = 210;
			cutscene_player_anim->running = true;
			cutscene_player_anim->num_frames = 4;
			cutscene_player_anim->loop = true;
			cutscene_player_anim->delay = 0.1f;
			cutscene_player_anim->max_delay = 0.1f;
			ctx.sequence_step++;
			break;

		case 115:
			if (cutscene_player->x > flagman->x && !ctx.player_reached_flagman) {
				//Player character reaches the flagman, who swings the flag
				ctx.player_reached_flagman = true;
				flagman_anim->frame = 0;
				flagman_anim->running = true;
			}
			if (cutscene_player->x >= cam->x + 304) {
				//Player character decelerates
				cutscene_player->x = cam->x + 304;
				cutscene_player->acc = -252;
				ctx.sequence_step++;
			}
			break;

		case 116:
			if (cutscene_player->xvel <= 128) {
				if (cutscene_player->sprite == SPR_PLAYER_RUN) {
					cutscene_player->sprite = SPR_PLAYER_WALK;
					cutscene_player->x += 8;
				}
			}
			if (cutscene_player->xvel <= 0 || cutscene_player->x >= cam->x + 392) {
				//Player character stops
				cutscene_player->x = cam->x + 392;
				cutscene_player->xvel = 0;
				cutscene_player->acc = 0;
				cutscene_player->sprite = SPR_PLAYER_STAND;
				cutscene_player_anim->running = false;
				cutscene_player_anim->frame = 0;
				ctx.sequence_delay = 1;
				ctx.sequence_step++;
			}
			break;

		case 117:
			//Traffic jam starts moving
			bus->xvel = 60;
			start_animation(ANIM_CAR_WHEELS);
			ctx.sequence_step++;
			break;

		case 118:
			if (bus->x >= 504) {
				//Traffic jam stops
				bus->x = 504;
				bus->xvel = 0;
				ctx.anims[ANIM_CAR_WHEELS].running = false;
				ctx.anims[ANIM_CAR_WHEELS].frame = 0;
				ctx.sequence_delay = 1;
				ctx.sequence_step++;
			}
			break;

		case 119:
			//Hen appears from the left side of the screen
			ctx.hen.x = cam->x - 64;
			ctx.hen.xvel = 360;
			start_animation(ANIM_HEN);
			ctx.sequence_step++;
			break;

		case 120:
			if (ctx.hen.x >= cam->x + 120) {
				//Hen decelerates
				ctx.hen.x = cam->x + 120;
				ctx.hen.acc = -252;
				ctx.sequence_step++;
			}
			break;

		case 121:
			if (ctx.hen.x > flagman->x && !ctx.hen_reached_flagman) {
				//Hen reaches the flagman, who swings the flag
				ctx.hen_reached_flagman = true;
				flagman_anim->frame = 0;
				flagman_anim->running = true;
			}
			if (ctx.hen.xvel <= 0 || ctx.hen.x >= cam->x + 352) {
				//Hen stops
				ctx.hen.x = cam->x + 352;
				ctx.hen.xvel = 0;
				ctx.hen.acc = 0;
				ctx.anims[ANIM_HEN].running = false;
				ctx.anims[ANIM_HEN].frame = 1;
				ctx.sequence_delay = 1;
				ctx.sequence_step++;
			}
			break;

		case 122:
			//Traffic jam starts moving
			bus->xvel = 60;
			start_animation(ANIM_CAR_WHEELS);
			ctx.sequence_step++;
			break;

		case 123:
			if (bus->x >= cam->x - 60) {
				//Bus reaches the flagman, who swings the flag
				ctx.bus_reached_flagman = true;
				flagman_anim->frame = 0;
				flagman_anim->running = true;

				//Traffic jam stops
				bus->x = cam->x - 60;
				bus->xvel = 0;
				ctx.anims[ANIM_CAR_WHEELS].running = false;
				ctx.anims[ANIM_CAR_WHEELS].frame = 0;
				start_animation(ANIM_BUS_DOOR_FRONT);
				ctx.sequence_delay = 3;
				ctx.sequence_step++;
			}
			break;

		case 124:
			//Screen wipes to black
			ctx.wipe_out = true;
			ctx.sequence_delay = 1;
			ctx.sequence_step++;
			break;

		case 125:
			ctx.sequence_step = SEQ_FINISHED;
			break;
	}
}

