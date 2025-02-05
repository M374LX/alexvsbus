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
 * renderer.c
 *
 * Description:
 * Rendering of graphics
 *
 */

//------------------------------------------------------------------------------

#include "defs.h"

#include <raylib.h>
#include <rlgl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

//stb_image library configuration
#define STBI_ONLY_PNG
#define STBI_MAX_DIMENSIONS 2048
#define STBI_MALLOC RL_MALLOC
#define STBI_FREE RL_FREE
#define STBI_REALLOC RL_REALLOC
#define STBI_NO_STDIO
#define STBI_NO_THREAD_LOCALS

//Prevent build from failing when using the Tiny C Compiler
#ifdef __TINYC__
#define STBI_NO_SIMD
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

//------------------------------------------------------------------------------

//From menu.c
bool menu_is_open();
int menu_center_tile_y();
int menu_item_x(MenuItem* item);
int menu_item_y(MenuItem* item);

//From data.c
extern const int data_sprites[];
extern const int data_player_anim_sprites[];
extern const int data_obj_sprites[];

//------------------------------------------------------------------------------

static RenderTexture2D vscreen;
static Texture2D gfx;

static DisplayParams* display_params;
static Config* config;
static PlayCtx* play_ctx;
static MenuCtx* menu_ctx;

static bool save_failed;

static int draw_offset_x;
static int draw_offset_y;

static int draw_max_x;
static int draw_max_y;

//------------------------------------------------------------------------------

//Function prototypes
static void draw_play();
static void draw_hud();
static void draw_final_score();
static void draw_menu();
static void draw_menu_item(MenuItem* item, bool selected);
static void draw_menu_border(int x, int y, int width, int height,
		bool selected, bool disabled);
static void draw_texture(Texture2D texture, Rectangle src, Rectangle dst,
		bool hflip, bool vflip, int alpha);
static void draw_gfx(Rectangle src, Rectangle dst, bool vflip, bool hflip, int alpha);
static void draw_sprite_part(int spr, int dx, int dy, int sx, int sy, int sw, int sh);
static void draw_sprite_flip(int spr, int dx, int dy, int frame, bool hflip, bool vflip);
static void draw_sprite(int spr, int dx, int dy, int frame);
static void draw_sprite_repeat(int spr, int dx, int dy, int xrep, int yrep);
static void draw_sprite_stretch(int spr, int dx, int dy, int w, int h);
static void draw_digits(int value, int width, int x, int y);
static void draw_text(const char* text, int color, int x, int y);
static void draw_touch_buttons(int input_state);
static void draw_scanlines();

//------------------------------------------------------------------------------

bool renderer_init(DisplayParams* dp, Config* cfg, PlayCtx* pctx, MenuCtx* mctx)
{
	int width  = VSCREEN_MAX_WIDTH;
	int height = VSCREEN_MAX_HEIGHT;
	int format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

	display_params = dp;
	config = cfg;
	play_ctx = pctx;
	menu_ctx = mctx;

	vscreen.id = rlLoadFramebuffer(width, height);
	if (vscreen.id <= 0) {
		return false;
	}

	rlEnableFramebuffer(vscreen.id);

	//Create color texture
	vscreen.texture.id      = rlLoadTexture(NULL, width, height, format, 1);
	vscreen.texture.width   = width;
	vscreen.texture.height  = height;
	vscreen.texture.format  = format;
	vscreen.texture.mipmaps = 1;

	//Create depth renderbuffer/texture
	vscreen.depth.id        = rlLoadTextureDepth(width, height, true);
	vscreen.depth.width     = width;
	vscreen.depth.height    = height;
	vscreen.depth.format    = 19; //DEPTH_COMPONENT_24BIT?
	vscreen.depth.mipmaps   = 1;

	//Attach color texture and depth renderbuffer/texture to FBO
	rlFramebufferAttach(vscreen.id, vscreen.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
	rlFramebufferAttach(vscreen.id, vscreen.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);

	rlDisableFramebuffer();

	rlTextureParameters(vscreen.texture.id, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_NEAREST);
	rlTextureParameters(vscreen.texture.id, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_NEAREST);

	return true;
}

bool renderer_load_gfx()
{
	char filename[530];
	int file_size = 0;
	unsigned char* file_data;
	void* img_data;
	int width;
	int height;
	int comp;
	int format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

	snprintf(filename, sizeof(filename), "%sgfx.png", config->assets_dir);

	file_data = LoadFileData(filename, &file_size);
	if (file_data == NULL) {
		return false;
	}

	img_data = stbi_load_from_memory(file_data, file_size, &width, &height, &comp, 4);
	RL_FREE(file_data);

	if (img_data == NULL) {
		return false;
	}

	if (width != 0 && height != 0) {
		gfx.id = rlLoadTexture(img_data, width, height, format, 1);
	}

	RL_FREE(img_data);

	gfx.width = width;
	gfx.height = height;
	gfx.format = format;
	gfx.mipmaps = 1;

	return true;
}

void renderer_draw(int screen_type, int input_state, int wipe_value)
{
	int vscreen_width  = display_params->vscreen_width;
	int vscreen_height = display_params->vscreen_height;
	int win_width  = display_params->win_width;
	int win_height = display_params->win_height;
	float scale = display_params->scale;

	Rectangle src;
	Rectangle dst;

	//Start drawing on the game's virtual screen
	BeginTextureMode(vscreen);

	draw_max_x = vscreen_width;
	draw_max_y = vscreen_height;

	//Clear virtual screen to black
	draw_sprite_stretch(SPR_BG_BLACK, 0, 0, vscreen_width, vscreen_height);

	switch (screen_type) {
		case SCR_BLANK:
			//Do nothing
			break;

		case SCR_PLAY:
		case SCR_PLAY_FREEZE:
			if (!(menu_is_open() && menu_ctx->fill_screen)) {
				draw_play();
				draw_hud();
			}
			break;

		case SCR_FINALSCORE:
			draw_final_score();
			break;
	}

	if (save_failed) {
		char* msg = "UNABLE TO SAVE GAME PROGRESS";
		int len = strlen(msg);
		int x = ((vscreen_width / TILE_SIZE) - len) / 2;

		draw_text(msg, TXTCOL_WHITE, TILE_SIZE * x, TILE_SIZE * 3);
	}

	if (menu_is_open()) {
		draw_menu();
	}

	//Draw screen wiping effects
	draw_sprite_stretch(SPR_BG_BLACK, 0, 0, wipe_value, vscreen_height);

	//Finish drawing on vscreen
	EndTextureMode();

	//Start drawing on physical screen
	BeginDrawing();

	draw_max_x = win_width;
	draw_max_y = win_height;

	//Clear entire physical screen to black or green
	ClearBackground((Color){ 0x00, 0x00, 0x00 });
	if (menu_is_open() && menu_ctx->green_bg) {
		ClearBackground((Color){ 0x00, 0x55, 0x00 });
	}

	//Draw virtual screen on physical screen
	src.x = 0;
	src.y = (VSCREEN_MAX_HEIGHT - vscreen_height);
	src.width  =  vscreen_width;
	src.height =  vscreen_height;
	//The int casts are necessary in order to avoid display distortions
	dst.x = (int)(win_width  - (vscreen_width  * scale)) / 2;
	dst.y = (int)(win_height - (vscreen_height * scale)) / 2;
	dst.width  = vscreen_width  * scale;
	dst.height = vscreen_height * scale;
	draw_texture(vscreen.texture, src, dst, false, true, 255);

	draw_scanlines();

	if (screen_type == SCR_PLAY) {
		draw_touch_buttons(input_state);
	}

	EndDrawing();
}

void renderer_show_save_error(bool show)
{
	save_failed = show;
}

void renderer_cleanup()
{
	if (vscreen.id > 0) {
		if (vscreen.texture.id > 0) {
			rlUnloadTexture(vscreen.texture.id);
			vscreen.texture.id = 0;
		}

		rlUnloadFramebuffer(vscreen.id);
		vscreen.id = 0;
	}

	if (gfx.id > 0) {
		rlUnloadTexture(gfx.id);
		gfx.id = 0;
	}
}

//------------------------------------------------------------------------------

static void draw_play()
{
	PlayCtx* ctx = play_ctx;

	int vscreen_width  = display_params->vscreen_width;
	int vscreen_height = display_params->vscreen_height;

	int x, y, spr, frame;
	int camy, topcamy; //Current and topmost camera Y position
	int i;

	//Background color
	draw_sprite_stretch(ctx->bg_color, 0, 0, vscreen_width, vscreen_height);

	camy = (int)ctx->cam.y;

	//Determine topmost camera Y position from virtual screen (vscreen) height
	topcamy = 0;
	if (vscreen_height < 224) {
		topcamy = vscreen_height - 224;
	}

	if (camy < topcamy) {
		camy = topcamy;
	}

	draw_offset_y = camy - (vscreen_height - VSCREEN_MAX_HEIGHT);

	//Background image
	draw_sprite_repeat(SPR_BACKGROUND, -ctx->bg_offset_x, BACKGROUND_DRAW_Y, 6, 1);

	draw_offset_x = (int)ctx->cam.x;
	if (vscreen_width <= 320 && ctx->level_num == LVLNUM_ENDING) {
		draw_offset_x += 168;
	}

	//Deep holes and passageways (background part)
	for (i = 0; i < MAX_HOLES; i++) {
		int w = ctx->holes[i].width;

		x = ctx->holes[i].x;
		y = BACKGROUND_DRAW_Y + 72;

		if (x != NONE) {
			int type = ctx->holes[i].type;
			bool is_deep = (type == HOLE_DEEP);
			bool exit_opened = (type == HOLE_PASSAGEWAY_EXIT_OPENED);

			//Left
			spr = (is_deep ? SPR_DEEP_HOLE_LEFT : SPR_PASSAGEWAY_LEFT);
			draw_sprite(spr, x, y, 0);

			//Middle
			x += LEVEL_BLOCK_SIZE;
			spr = (is_deep ? SPR_DEEP_HOLE_MIDDLE : SPR_PASSAGEWAY_MIDDLE);
			draw_sprite_repeat(spr, x, y, w - 2, 1);

			//Right
			x = ctx->holes[i].x + ((w - 1) * LEVEL_BLOCK_SIZE);
			spr = (is_deep ? SPR_DEEP_HOLE_RIGHT : SPR_PASSAGEWAY_RIGHT);
			draw_sprite(spr, x, y, 0);

			//Passageway exit if not opened
			if (!is_deep && !exit_opened) {
				spr = SPR_PASSAGEWAY_RIGHT_CLOSED;
				draw_sprite(spr, x, y, 0);
			}
		}
	}

	//Bus body, wheels, and route sign
	x = (int)ctx->bus.x;
	y = BUS_Y;
	draw_sprite(SPR_BUS, x, y, 0);
	if (ctx->bus.route_sign != NONE) {
		if (ctx->bus.route_sign == 0) {
			//Finish (checkered flag) sign
			frame = 4;
		} else {
			//Frame zero corresponds to the sign containing number two, as
			//there is no number one sign
			frame = ctx->bus.route_sign - 2;
		}

		if (frame >= 0) {
			draw_sprite(SPR_BUS_ROUTE, x + 308, y + 48, frame);
		}
	}
	frame = ctx->anims[ANIM_BUS_WHEELS].frame;
	draw_sprite(SPR_BUS_WHEEL, x + 104, y + 80, frame);
	draw_sprite(SPR_BUS_WHEEL, x + 296, y + 80, frame);

	//Characters at bus rear door
	if (ctx->bus.num_characters >= 1) {
		draw_sprite(SPR_BUS_CHARACTER_1, x + 72, y + 24, 0);
	}
	if (ctx->bus.num_characters >= 2) {
		draw_sprite(SPR_BUS_CHARACTER_2, x + 64, y + 24, 0);
	}
	if (ctx->bus.num_characters >= 3) {
		draw_sprite(SPR_BUS_CHARACTER_3, x + 80, y + 24, 0);
	}

	//Cutscene objects (if in the bus)
	for (i = 0; i < MAX_CUTSCENE_OBJECTS; i++) {
		CutsceneObject* cobj = &ctx->cutscene_objects[i];

		if (cobj->sprite == NONE || !cobj->in_bus) continue;

		spr = cobj->sprite;
		x = (int)cobj->x + (int)ctx->bus.x;
		y = (int)cobj->y;
		frame = ctx->anims[ANIM_CUTSCENE_OBJECTS + i].frame;
		draw_sprite(spr, x, y, frame);
	}

	//Bus doors
	x = (int)ctx->bus.x;
	y = BUS_Y;
	frame = ctx->anims[ANIM_BUS_DOOR_REAR].frame;
	draw_sprite(SPR_BUS_DOOR, x + 64,  y + 16, frame);
	frame = ctx->anims[ANIM_BUS_DOOR_FRONT].frame;
	draw_sprite(SPR_BUS_DOOR, x + 344, y + 16, frame);

	//Passing car and ending sequence traffic jam
	if (ctx->car.x != NONE) {
		int num_cars;

		x = (int)ctx->car.x;
		y = PASSING_CAR_Y;
		frame = ctx->anims[ANIM_CAR_WHEELS].frame;

		if (ctx->car.type == TRAFFIC_JAM) { //Traffic jam
			num_cars = 6;
			spr = SPR_CAR_BLUE;
		} else { //Single car
			num_cars = 1;

			spr = SPR_CAR_BLUE;
			if (ctx->car.type == CAR_SILVER) {
				spr = SPR_CAR_SILVER;
			} else if (ctx->car.type == CAR_YELLOW) {
				spr = SPR_CAR_YELLOW;
			}
		}

		for (i = 0; i < num_cars; i++) {
			draw_sprite(spr, x, y, 0); //Car body
			draw_sprite(SPR_CAR_WHEEL, x + 16, y + 32, frame); //Rear wheel
			draw_sprite(SPR_CAR_WHEEL, x + 96, y + 32, frame); //Front wheel

			x += 136;

			//Next car color
			switch (spr) {
				case SPR_CAR_BLUE:   spr = SPR_CAR_SILVER; break;
				case SPR_CAR_SILVER: spr = SPR_CAR_YELLOW; break;
				case SPR_CAR_YELLOW: spr = SPR_CAR_BLUE;   break;
			}
		}
	}

	//Hen
	if (ctx->hen.x != NONE) {
		frame = ctx->anims[ANIM_HEN].frame;
		draw_sprite(SPR_HEN, (int)ctx->hen.x, HEN_Y, frame);
	}

	//Light poles (at most two are visible)
	draw_sprite(SPR_POLE, ctx->pole_x, POLE_Y, 0);
	draw_sprite(SPR_POLE, ctx->pole_x + POLE_DISTANCE, POLE_Y, 0);

	//Bus stop sign
	draw_sprite(SPR_BUS_STOP_SIGN, ctx->bus_stop_sign_x, BUS_STOP_SIGN_Y, 0);

	//Crate blocks
	for (i = 0; i < MAX_CRATE_BLOCKS; i++) {
		CrateBlock* block = &ctx->crate_blocks[i];
		x = block->x;
		y = block->y;

		if (x != NONE) {
			draw_sprite_repeat(SPR_CRATE, x, y, block->width, block->height);
		}
	}

	//Objects that use PlayCtx.objs[] and are drawn behind the player character
	for (i = 0; i < MAX_OBJS; i++) {
		Obj* obj = &ctx->objs[i];

		//Ignore inexistent objects
		if (obj->type == NONE) continue;

		//Skip objects that are drawn in front of the player character, as
		//those will be drawn later
		if (obj->type == OBJ_COIN_SILVER) continue;
		if (obj->type == OBJ_COIN_GOLD) continue;
		if (obj->type == OBJ_BANANA_PEEL) continue;
		if (obj->type == OBJ_BANANA_PEEL_MOVING) continue;

		if (obj->type == OBJ_GUSH) {
			int w = data_sprites[SPR_GUSH * 4 + 2];
			int h = 265 - obj->y;
			if (h <= 0) h = 1;

			frame = ctx->anims[ANIM_GUSHES].frame;

			draw_sprite_part(SPR_GUSH, obj->x, obj->y, frame * w, 0, w, h);

			//Gush hole
			draw_sprite(SPR_GUSH_HOLE, obj->x, 263, 0);
		} else {
			frame = 0;

			if (obj->type == OBJ_SPRING) {
				frame = 5;

				if (i == ctx->hit_spring) {
					frame = ctx->anims[ANIM_HIT_SPRING].frame;
				}
			}

			draw_sprite(data_obj_sprites[obj->type], obj->x, obj->y, frame);
		}
	}

	//Player character
	if (ctx->player.visible) {
		spr = data_player_anim_sprites[ctx->player.anim_type];
		x = (int)ctx->player.x;
		y = (int)ctx->player.y;
		frame = ctx->anims[ANIM_PLAYER].frame;
		draw_sprite(spr, x, y, frame);
	}

	//Cutscene objects (if not in the bus)
	for (i = 0; i < MAX_CUTSCENE_OBJECTS; i++) {
		CutsceneObject* cobj = &ctx->cutscene_objects[i];

		if (cobj->sprite == NONE || cobj->in_bus) continue;

		spr = cobj->sprite;
		x = (int)cobj->x;
		y = (int)cobj->y;
		frame = ctx->anims[ANIM_CUTSCENE_OBJECTS + i].frame;
		draw_sprite(spr, x, y, frame);
	}

	//Medal icons (used in the ending sequence)
	if (ctx->player_reached_flagman) {
		x = (int)ctx->cutscene_objects[0].x;
		y = 160;

		if (ctx->cutscene_objects[0].sprite == SPR_PLAYER_RUN) {
			x += 8;
		}

		draw_sprite(SPR_MEDAL1, x, y, 0);
	}
	if (ctx->hen_reached_flagman) {
		x = (int)ctx->hen.x;
		y = 184;
		draw_sprite(SPR_MEDAL2, x, y, 0);
	}
	if (ctx->bus_reached_flagman) {
		x = (int)ctx->bus.x + 343;
		y = 120;
		draw_sprite(SPR_MEDAL3, x, y, 0);
	}

	//Deep holes and passageways (foreground part)
	for (i = 0; i < MAX_HOLES; i++) {
		x = ctx->holes[i].x;
		y = BACKGROUND_DRAW_Y + 88;

		if (x != NONE) {
			if (ctx->holes[i].type == HOLE_DEEP) {
				draw_sprite(SPR_DEEP_HOLE_LEFT_FG, x, y, 0);
			} else {
				//Left
				draw_sprite(SPR_PASSAGEWAY_LEFT_FG, x, y, 0);

				//Right
				x += (ctx->holes[i].width - 1) * LEVEL_BLOCK_SIZE;
				draw_sprite(SPR_PASSAGEWAY_RIGHT_FG, x, y, 0);
			}
		}
	}

	//When slipping, the player character is drawn in front of hole foregrounds
	if (ctx->player.visible) {
		int state = ctx->player.state;

		if (state == PLAYER_STATE_SLIP || state == PLAYER_STATE_GETUP) {
			spr = data_player_anim_sprites[ctx->player.anim_type];
			x = (int)ctx->player.x;
			y = (int)ctx->player.y;
			frame = ctx->anims[ANIM_PLAYER].frame;
			draw_sprite(spr, x, y, frame);
		}
	}

	//Objects that use PlayCtx.objs[] and are drawn in front of the player
	//character
	for (i = 0; i < MAX_OBJS; i++) {
		Obj* obj = &ctx->objs[i];

		if (obj->type == OBJ_BANANA_PEEL) {
			frame = 0;
		} else if (obj->type == OBJ_BANANA_PEEL_MOVING) {
			frame = 0;
		} else if (obj->type == OBJ_COIN_SILVER) {
			frame = ctx->anims[ANIM_COINS].frame;
		} else if (obj->type == OBJ_COIN_GOLD) {
			frame = ctx->anims[ANIM_COINS].frame;
		} else {
			continue;
		}

		draw_sprite(data_obj_sprites[obj->type], obj->x, obj->y, frame);
	}

	//Pushable crate arrows
	for (i = 0; i < MAX_PUSHABLE_CRATES; i++) {
		PushableCrate* crate = &ctx->pushable_crates[i];

		if (crate->obj != NONE && crate->show_arrow) {
			x = ctx->objs[crate->obj].x - 24 + (int)ctx->push_arrow.xoffs;
			y = FLOOR_Y - 20;

			draw_sprite(SPR_PUSH_ARROW, x, y, 0);
		}
	}

	//Overhead sign bases
	for (i = 0; i < MAX_OBJS; i++) {
		Obj* obj = &ctx->objs[i];
		int h;

		if (obj->type == OBJ_OVERHEAD_SIGN) {
			spr = SPR_OVERHEAD_SIGN_BASE_TOP;
			x = obj->x + 16;
			y = obj->y + 8;
			draw_sprite(spr, x, y, 0);

			spr = SPR_OVERHEAD_SIGN_BASE;
			x = obj->x + 24;
			y = obj->y + 32;
			h = 272 - y;
			draw_sprite_part(spr, x, y, 0, 320 - h, 8, h);
		}
	}

	//Crack particles
	for (i = 0; i < MAX_CRACK_PARTICLES; i++) {
		x = (int)ctx->crack_particles[i].x;
		y = (int)ctx->crack_particles[i].y;
		frame = ctx->anims[ANIM_CRACK_PARTICLES].frame;

		if (x != NONE) {
			draw_sprite(SPR_CRACK_PARTICLE, x, y, frame);
		}
	}

	//Coin sparks
	for (i = 0; i < MAX_COIN_SPARKS; i++) {
		x = ctx->coin_sparks[i].x;
		y = ctx->coin_sparks[i].y;

		if (x != NONE) {
			bool gold = ctx->coin_sparks[i].gold;
			spr = gold ? SPR_COIN_SPARK_GOLD : SPR_COIN_SPARK_SILVER;
			frame = ctx->anims[ANIM_COIN_SPARKS + i].frame;
			draw_sprite(spr, x, y, frame);
		}
	}

	//Reset draw offset
	draw_offset_x = 0;
	draw_offset_y = 0;
}

static void draw_hud()
{
	int x, h;

	//Determine HUD height
	h = TILE_SIZE * 2;
	if (config->touch_enabled && display_params->vscreen_height > 224) {
		h = TILE_SIZE * 3;
	}
	if (save_failed) {
		h = TILE_SIZE * 4;
	}

	draw_sprite_stretch(SPR_BG_BLACK, 0, 0, display_params->vscreen_width, h);

	draw_text("SCORE", TXTCOL_WHITE, 0, 0);
	draw_digits(play_ctx->score, 6, 0, 8);

	x = (display_params->vscreen_width / 2) - (2 * TILE_SIZE);

	draw_text("TIME", TXTCOL_WHITE, x, 0);
	if (play_ctx->level_num == LVLNUM_ENDING) {
		draw_text("--", TXTCOL_WHITE, x + TILE_SIZE, 8);
	} else {
		draw_digits(play_ctx->time, 2, x + TILE_SIZE, 8);
	}

	//Touchscreen pause button
	if (config->show_touch_controls && play_ctx->can_pause) {
		if (!menu_is_open()) {
			draw_sprite(SPR_PAUSE, display_params->vscreen_width - 24, 0, 0);
		}
	}
}

static void draw_final_score()
{
	int cx = (display_params->vscreen_width  / TILE_SIZE) / 2 * TILE_SIZE;
	int cy = (display_params->vscreen_height / TILE_SIZE) / 2 * TILE_SIZE;
	int x = 0;
	const char* msg = "";

	draw_text("SCORE:", TXTCOL_WHITE, cx - 7 * TILE_SIZE, cy - TILE_SIZE);
	draw_digits(play_ctx->score, 6, cx + 1 * TILE_SIZE, cy - TILE_SIZE);

	switch (play_ctx->difficulty) {
		case DIFFICULTY_NORMAL:
			x = cx - 12 * TILE_SIZE;
			msg = "GET READY FOR HARD MODE!";
			break;

		case DIFFICULTY_HARD:
			x = cx - 12 * TILE_SIZE;
			msg = "GET READY FOR SUPER MODE!";
			break;

		case DIFFICULTY_SUPER:
			x = cx - 4 * TILE_SIZE;
			msg = "THE  END";
			break;
	}

	draw_text(msg, TXTCOL_WHITE, x, cy + TILE_SIZE);
}

static void draw_menu()
{
	MenuCtx* ctx = menu_ctx;
	int vscreen_width  = display_params->vscreen_width;
	int vscreen_height = display_params->vscreen_height;
	int selected_item  = ctx->stack[ctx->stack_size - 1].selected_item;
	int i;

	//Center of the main area of the menu in tiles
	int cx = (vscreen_width / TILE_SIZE) / 2;
	int cy = menu_center_tile_y();

	//Draw menu frame
	if (ctx->show_frame) {
		//Frame size and position in tiles
		int tw = 28;
		int th = 18;
		int tx = cx - (tw / 2);
		int ty = cy - (th / 2);

		int w = tw * TILE_SIZE;
		int h = th * TILE_SIZE;
		int x = tx * TILE_SIZE;
		int y = ty * TILE_SIZE;

		draw_sprite_stretch(SPR_BG_BLACK, x, y, w, h);
	}

	//Draw logo
	if (ctx->show_logo) {
		int spr, logo_width, x, y;

		if (vscreen_width <= 320 || vscreen_height <= 224) {
			spr = SPR_LOGO_SMALL;
			logo_width = LOGO_WIDTH_SMALL;
		} else {
			spr = SPR_LOGO_LARGE;
			logo_width = LOGO_WIDTH_LARGE;
		}

		x = (vscreen_width - logo_width) / 2 + 4;
		y = (vscreen_height <= 192) ? 0 : 16;

		draw_sprite(spr, x, y, 0);
	}

	//Draw menu display name
	if (ctx->display_name[0] != '\0' && !ctx->level_selected) {
		//Text position in tiles
		int tx = cx - 10;
		int ty = (vscreen_height <= 192) ? 2 : 3;

		int x = tx * TILE_SIZE;
		int y = ty * TILE_SIZE;
		int w = 20;
		int h = (vscreen_height <= 192) ? 3 : 5;

		if (vscreen_width <= 256) {
			bool center_bar = true;

			//If the virtual screen (vscreen) is 256 pixels wide or less,
			//center the name bar only if there is no selectable item at
			//the top-left corner
			for (i = 0; i < ctx->num_items; i++) {
				if (ctx->items[i].align == ALIGN_TOPLEFT) {
					center_bar = false;
				}
			}

			w = center_bar ? 14 : 16;
			tx = cx - 7;
			x = tx * TILE_SIZE;
		}

		draw_menu_border(x - 16, 8, w + 4, h, false, false);
		draw_text(ctx->display_name, TXTCOL_WHITE, x, y);
	}

	//Draw menu text
	if (ctx->text[0] != '\0') {
		//Text position in tiles
		int tx = cx - (ctx->text_width  / 2) + ctx->text_offset_x;
		int ty = cy - (ctx->text_height / 2) + ctx->text_offset_y;

		int x = tx * TILE_SIZE;
		int y = ty * TILE_SIZE;
		int w = ctx->text_width;
		int h = ctx->text_height;

		if (ctx->text_border) {
			draw_menu_border(x - 16, y - 16, w + 4, h + 4, false, false);
		}

		draw_text(ctx->text, TXTCOL_WHITE, x, y);

		if (ctx->stack[ctx->stack_size - 1].type == MENU_ERROR) {
			draw_sprite(SPR_ERROR, x, y, 0);
		}
	}

	//Draw menu items
	for (i = 0; i < ctx->num_items; i++) {
		if (selected_item != i) {
			if (!ctx->level_selected) {
				draw_menu_item(&ctx->items[i], false);
			}
		} else if (ctx->selected_visible) {
			draw_menu_item(&ctx->items[i], ctx->use_cursor);
		}
	}
}

static void draw_menu_item(MenuItem* item, bool selected)
{
	int x, y;
	int w = item->width;
	int h = item->height;

	if (item->hidden) return;

	x = menu_item_x(item);
	y = menu_item_y(item);

	draw_menu_border(x, y, w, h, selected, item->disabled);

	//Draw item caption
	if (item->caption[0] != '\0') {
		int xoffs = TILE_SIZE;
		int yoffs = TILE_SIZE * (item->height / 2);
		int color;

		if (item->disabled) {
			color = TXTCOL_GRAY;
		} else if (selected) {
			color = TXTCOL_GREEN;
		} else {
			color = TXTCOL_WHITE;
		}

		draw_text(item->caption, color, x + xoffs, y + yoffs);
	}

	//Draw item value
	if (item->value[0] != '\0') {
		int xoffs = ((w - 1) * TILE_SIZE) - (strlen(item->value) * TILE_SIZE);
		int yoffs = TILE_SIZE * (item->height / 2);
		int color;

		if (item->disabled) {
			color = TXTCOL_GRAY;
		} else if (selected) {
			color = TXTCOL_GREEN;
		} else {
			color = TXTCOL_WHITE;
		}

		draw_text(item->value, color, x + xoffs, y + yoffs);
	}

	//Draw item icon
	if (item->icon_sprite != NONE) {
		int spr = item->icon_sprite;
		if (selected) spr++;

		x = menu_item_x(item) + TILE_SIZE;
		y = menu_item_y(item) + TILE_SIZE;

		draw_sprite(spr, x, y, 0);
	}
}

static void draw_menu_border(int x, int y, int width, int height,
		bool selected, bool disabled)
{
	int i, j;
	int spr;

	for (i = 0; i < width; i++) {
		for (j = 0; j < height; j++) {
			bool hflip, vflip;

			if (i == 0) {
				if (j == 0) {
					spr = SPR_MENU_BORDER_TOPLEFT;
					hflip = false;
					vflip = false;
				} else if (j == height - 1) {
					spr = SPR_MENU_BORDER_TOPLEFT;
					hflip = false;
					vflip = true;
				} else {
					spr = SPR_MENU_BORDER_LEFT;
					hflip = false;
					vflip = false;
				}
			} else if (i == width - 1) {
				if (j == 0) {
					spr = SPR_MENU_BORDER_TOPLEFT;
					hflip = true;
					vflip = false;
				} else if (j == height - 1) {
					spr = SPR_MENU_BORDER_TOPLEFT;
					hflip = true;
					vflip = true;
				} else {
					spr = SPR_MENU_BORDER_LEFT;
					hflip = true;
					vflip = false;
				}
			} else {
				if (j == 0) {
					spr = SPR_MENU_BORDER_TOP;
					hflip = false;
					vflip = false;
				} else if (j == height - 1) {
					spr = SPR_MENU_BORDER_TOP;
					hflip = false;
					vflip = true;
				} else {
					spr = SPR_BG_BLACK;
					hflip = false;
					vflip = false;
				}
			}

			if (spr != SPR_BG_BLACK) {
				if (disabled) {
					spr += 2;
				} else if (selected) {
					spr++;
				}
			}

			draw_sprite_flip(spr, x + i * 8, y + j * 8, 0, hflip, vflip);
		}
	}
}

//------------------------------------------------------------------------------

//Draws a texture
static void draw_texture(Texture2D texture, Rectangle src, Rectangle dst,
		bool hflip, bool vflip, int alpha)
{
	//This function has been adapted from raylib's DrawTexturePro()

	if (texture.id > 0) {
		float width  = (float)texture.width;
		float height = (float)texture.height;

		float left   = dst.x;
		float right  = left + dst.width;
		float top    = dst.y;
		float bottom = top + dst.height;

		float tx;
		float ty;

		if (vflip) {
			src.height *= -1;
			src.y -= src.height;
		}

		rlSetTexture(texture.id);
		rlBegin(RL_QUADS);

		rlColor4ub(255, 255, 255, alpha);
		rlNormal3f(0.0f, 0.0f, 1.0f); //Normal vector pointing towards the viewer

		//Top-left corner
		tx = (src.x + (hflip ? src.width : 0)) / width;
		ty = src.y / height;
		rlTexCoord2f(tx, ty);
		rlVertex2f(left, top);

		//Bottom-left corner
		tx = (src.x + (hflip ? src.width : 0)) / width;
		ty = (src.y + src.height) / height;
		rlTexCoord2f(tx, ty);
		rlVertex2f(left, bottom);

		//Bottom-right corner
		tx = (src.x + (hflip ? 0 : src.width)) / width;
		ty = (src.y + src.height) / height;
		rlTexCoord2f(tx, ty);
		rlVertex2f(right, bottom);

		//Top-right corner
		tx = (src.x + (hflip ? 0 : src.width)) / width;
		ty = src.y / height;
		rlTexCoord2f(tx, ty);
		rlVertex2f(right, top);

		rlEnd();
		rlSetTexture(0);
	}
}

//Draws a region of the image containing the game's graphics
static void draw_gfx(Rectangle src, Rectangle dst, bool hflip, bool vflip, int alpha)
{
	dst.x -= draw_offset_x;
	dst.y -= draw_offset_y;

	//Skip drawing what is outside the screen
	if (dst.x < -dst.width  || dst.x > draw_max_x) return;
	if (dst.y < -dst.height || dst.y > draw_max_y) return;

	draw_texture(gfx, src, dst, hflip, vflip, alpha);
}

static void draw_sprite_part(int spr, int dx, int dy, int sx, int sy, int sw, int sh)
{
	Rectangle src;
	Rectangle dst;

	sx += data_sprites[spr * 4 + 0];
	sy += data_sprites[spr * 4 + 1];

	src.x = sx;
	src.y = sy;
	src.width  = sw;
	src.height = sh;

	dst.x = dx;
	dst.y = dy;
	dst.width  = sw;
	dst.height = sh;

	draw_gfx(src, dst, false, false, 255);
}

static void draw_sprite_flip(int spr, int dx, int dy, int frame, bool hflip, bool vflip)
{
	int w  = data_sprites[spr * 4 + 2];
	int h  = data_sprites[spr * 4 + 3];
	int sx = data_sprites[spr * 4 + 0] + (frame * w);
	int sy = data_sprites[spr * 4 + 1];

	Rectangle src;
	Rectangle dst;

	src.x = sx;
	src.y = sy;
	src.width  = w;
	src.height = h;

	dst.x = dx;
	dst.y = dy;
	dst.width  = w;
	dst.height = h;

	draw_gfx(src, dst, hflip, vflip, 255);
}

static void draw_sprite(int spr, int dx, int dy, int frame)
{
	draw_sprite_flip(spr, dx, dy, frame, false, false);
}

static void draw_sprite_repeat(int spr, int dx, int dy, int xrep, int yrep)
{
	int w = data_sprites[spr * 4 + 2];
	int h = data_sprites[spr * 4 + 3];

	for (int i = 0; i < xrep; i++) {
		for (int j = 0; j < yrep; j++) {
			draw_sprite(spr, dx + (i * w), dy + (j * h), 0);
		}
	}
}

static void draw_sprite_stretch(int spr, int dx, int dy, int w, int h)
{
	int sx = data_sprites[spr * 4 + 0];
	int sy = data_sprites[spr * 4 + 1];
	int sw = data_sprites[spr * 4 + 2];
	int sh = data_sprites[spr * 4 + 3];

	Rectangle src;
	Rectangle dst;

	src.x = sx;
	src.y = sy;
	src.width  = sw;
	src.height = sh;

	dst.x = dx;
	dst.y = dy;
	dst.width  = w;
	dst.height = h;

	draw_gfx(src, dst, false, false, 255);
}

static void draw_digits(int value, int width, int x, int y)
{
	char digits[12];
	int num_digits = 0;
	int i;

	//Convert value to decimal digits
	if (value <= 0) {
		digits[0] = 0;
		num_digits = 1;
	} else {
		while (value > 0) {
			if (num_digits >= 12) break;

			digits[num_digits] = value % 10;
			value /= 10;
			num_digits++;
		}
	}

	//Pad with leading zeros
	while (num_digits < width) {
		if (num_digits >= 12) break;

		digits[num_digits] = 0;
		num_digits++;
	}

	for (i = num_digits - 1; i >= 0; i--) {
		draw_sprite_part(SPR_CHARSET_WHITE, x, y, digits[i] * 8, 8, 8, 8);
		x += 8;
	}
}

//The character 0x1B (which corresponds to ASCII Escape) is used by this
//method to switch between white and green characters
//
//The parameter "color" specifies the initial color: TXTCOL_WHITE,
//TXTCOL_GREEN, or TXTCOL_GRAY
//
//The newline (\n) character also reverts to the initial color
static void draw_text(const char* text, int color, int x, int y)
{
	int i;
	int len = strlen(text);
	int initial_color = color;

	int spr;
	int dx = x;
	int dy = y;

	for (i = 0; i < len; i++) {
		int c, sx, sy;

		c = text[i];

		if (c == 0x1B) {
			color = (color == TXTCOL_GREEN) ? TXTCOL_WHITE : TXTCOL_GREEN;
		} else if (c == '\n') {
			dy += 8;
			dx = x;
			color = initial_color;
		} else {
			c -= ' ';
			sx = (c % 16) * 8;
			sy = (c / 16) * 8;

			switch (color) {
				case TXTCOL_GREEN: spr = SPR_CHARSET_GREEN; break;
				case TXTCOL_GRAY:  spr = SPR_CHARSET_GRAY;  break;
				default:           spr = SPR_CHARSET_WHITE; break;
			}

			draw_sprite_part(spr, dx, dy, sx, sy, 8, 8);

			dx += 8;
		}
	}
}

//------------------------------------------------------------------------------

static void draw_touch_buttons(int input_state)
{
	int win_width  = display_params->win_width;
	int win_height = display_params->win_height;
	int scale = display_params->scale;

	Rectangle src;
	Rectangle dst;
	int spr;

	if (!config->show_touch_controls || !config->touch_buttons_enabled) return;

	//Size of the buttons
	src.width  = TOUCH_BUTTON_WIDTH;
	src.height = TOUCH_BUTTON_HEIGHT;
	dst.width  = src.width  * scale;
	dst.height = src.height * scale;

	//Draw left button
	spr   = (input_state & INPUT_LEFT) ? SPR_TOUCH_LEFT_HELD : SPR_TOUCH_LEFT;
	src.x = data_sprites[spr * 4 + 0];
	src.y = data_sprites[spr * 4 + 1];
	dst.x = TOUCH_LEFT_X;
	dst.y = win_height - (TOUCH_LEFT_OFFSET_Y * scale);
	draw_gfx(src, dst, false, false, TOUCH_BUTTON_OPACITY);

	//Draw right button
	spr   = (input_state & INPUT_RIGHT) ? SPR_TOUCH_RIGHT_HELD : SPR_TOUCH_RIGHT;
	src.x = data_sprites[spr * 4 + 0];
	src.y = data_sprites[spr * 4 + 1];
	dst.x = TOUCH_RIGHT_X * scale;
	dst.y = win_height - (TOUCH_RIGHT_OFFSET_Y * scale);
	draw_gfx(src, dst, false, false, TOUCH_BUTTON_OPACITY);

	//Draw jump button
	spr   = (input_state & INPUT_JUMP) ? SPR_TOUCH_JUMP_HELD : SPR_TOUCH_JUMP;
	src.x = data_sprites[spr * 4 + 0];
	src.y = data_sprites[spr * 4 + 1];
	dst.x = win_width  - (TOUCH_JUMP_OFFSET_X * scale);
	dst.y = win_height - (TOUCH_JUMP_OFFSET_Y * scale);
	draw_gfx(src, dst, false, false, TOUCH_BUTTON_OPACITY);
}

static void draw_scanlines()
{
	int vscreen_width  = display_params->vscreen_width;
	int vscreen_height = display_params->vscreen_height;
	int scale = display_params->scale;
	int offset_x = (display_params->win_width  - (vscreen_width  * scale)) / 2;
	int offset_y = (display_params->win_height - (vscreen_height * scale)) / 2;
	int line;

	if (!config->scanlines_enabled || scale < 2) return;
	
	for (line = 0; line < vscreen_height * scale; line += scale) {
		int dy = line + (scale - 1);
		int dw = vscreen_width * scale;
		int sx = data_sprites[SPR_SCANLINE * 4 + 0];
		int sy = data_sprites[SPR_SCANLINE * 4 + 1];

		Rectangle src;
		Rectangle dst;

		src.x = sx;
		src.y = sy;
		src.width  = 8;
		src.height = 1;

		dst.x = offset_x;
		dst.y = offset_y + dy;
		dst.width  = dw;
		dst.height = 1;

		draw_gfx(src, dst, false, false, 127);
	}
}



