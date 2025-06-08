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
 * menu.c
 *
 * Description:
 * Implementation of the game's menus
 *
 */

//------------------------------------------------------------------------------

#include "defs.h"

#include <stdio.h>
#include <string.h>

//------------------------------------------------------------------------------

//From audio.c
void audio_stop_bgm();
void audio_play_bgm(int id);
void audio_play_sfx(int id);

//From util.c
bool str_starts_with(const char* str, const char* start);

//From data.c
extern const int data_screen_widths[];
extern const int data_screen_heights[];
extern const int data_difficulty_num_levels[];
extern const int data_cheat_sequence[];
extern const char* data_menu_display_names[];

//------------------------------------------------------------------------------

static MenuCtx ctx;
static DisplayParams* display_params;
static Config* config;
static bool wait_input_up;
static int cursor_direction;
static int prev_cursor_direction;
static float cursor_delay;
static int difficulty; //Selected difficulty
static int level_num;   //Selected level number
static float level_start_delay;
static int cheat_pos;

//If the selected item is between store_sel_min and store_sel_max (inclusive),
//then it is stored in stored_sel; if an item's target is -2 (-1 would
//conflict with the constant NONE), then the item to be selected is
//retrieved from stored_sel;
static int stored_sel;
static int store_sel_min;
static int store_sel_max;

//------------------------------------------------------------------------------

//Function prototypes
static void update_audio_icon();
static void update_values();
static void confirm(int item, bool change_item);
static void open(int menu_type);
static void close();
static void close_all();
static void load_menu(int menu_type);
static void set_item_position(int item, int align, int offset_x, int offset_y);
static void position_items_center(int first_item, int last_item, bool vertical,
		int pos_diff, int offset_y);
static void set_item(int item, int width, int height,
		int target_up, int target_down, int target_left, int target_right,
		int icon_sprite);
static const char* url(const char* str);

//------------------------------------------------------------------------------

MenuCtx* menu_init(DisplayParams* dp, Config* cfg)
{
	display_params = dp;
	config = cfg;

	difficulty = DIFFICULTY_NORMAL;
	cursor_direction = NONE;
	prev_cursor_direction = NONE;

	ctx.display_name = "";
	ctx.action = NONE;
	ctx.use_cursor = !config->touch_enabled;
	ctx.selected_visible = true;
	ctx.text[0] = '\0';

	return &ctx;
}

bool menu_is_open()
{
	return (ctx.stack_size > 0);
}

//Returns the Y position in tiles corresponding to the center of the menu's
//main area (where most items are placed), which is distinct from the top area
//(where the display name and often the return item are placed)
int menu_center_tile_y()
{
	int vscreen_height_tiles = display_params->vscreen_height / TILE_SIZE;
	int top_area_height = (display_params->vscreen_height <= 192) ? 3 : 7;
	int main_area_height = vscreen_height_tiles - top_area_height;

	return main_area_height / 2 + top_area_height;
}

//Returns the absolute X position of an item on the screen in pixels
int menu_item_x(MenuItem* item)
{
	int x = item->offset_x;

	if (item->align == ALIGN_TOPRIGHT) {
		x += (display_params->vscreen_width / TILE_SIZE) - item->width;
	} else if (item->align == ALIGN_CENTER) {
		x += ((display_params->vscreen_width / TILE_SIZE) - item->width) / 2;
	}

	return x * TILE_SIZE;
}

//Returns the absolute Y position of an item on the screen in pixels
int menu_item_y(MenuItem* item)
{
	int y = item->offset_y;

	if (item->align == ALIGN_CENTER) {
		y += menu_center_tile_y() - item->height / 2;
	}

	return y * TILE_SIZE;
}

//Handles a touchscreen tap
void menu_handle_tap(int x, int y)
{
	bool change_item;
	int menu_type;

	//Nothing to do if no menu is open or a level has been selected
	if (!menu_is_open() || ctx.level_selected) return;

	//Touchscreen not tapped
	if (x < 0 || y < 0) return;

	change_item = true;
	menu_type = ctx.stack[ctx.stack_size - 1].type;

	for (int i = 0; i < ctx.num_items; i++) {
		int ix = menu_item_x(&ctx.items[i]);
		int iy = menu_item_y(&ctx.items[i]);
		int w = ctx.items[i].width;
		int h = ctx.items[i].height;

		if (x < ix) continue;
		if (x > ix + (w * TILE_SIZE)) continue;
		if (y < iy) continue;
		if (y > iy + (h * TILE_SIZE)) continue;

		if (ctx.items[i].disabled) continue;
		if (ctx.items[i].hidden) continue;

		//Do not change the selection to the audio toggle item when touching it
		if ((menu_type == MENU_MAIN  && i == 5) ||
			(menu_type == MENU_PAUSE && i == 4)) {

			change_item = false;
		} else {
			//Tapping normally disables the cursor, except when selecting a
			//BGM track (not the "return" item) on the jukebox
			ctx.use_cursor = false;
			if (menu_type == MENU_JUKEBOX && i < 4) {
				ctx.use_cursor = true;
			}
		}

		confirm(i, change_item);

		return;
	}
}

//Handles player's input from keyboard or gamepad, but not screen touches
void menu_handle_keys(int input_held, int input_hit)
{
	//Nothing to do if no menu is open or a level has been selected
	if (!menu_is_open() || ctx.level_selected) return;

	if (wait_input_up) {
		if (input_held == 0) wait_input_up = false;

		cursor_direction = NONE;
		prev_cursor_direction = NONE;

		return;
	}

	//Move cursor
	if ((input_held & INPUT_UP) > 0) {
		cursor_direction = MENUDIR_UP;
	} else if ((input_held & INPUT_DOWN) > 0) {
		cursor_direction = MENUDIR_DOWN;
	} else if ((input_held & INPUT_LEFT) > 0) {
		cursor_direction = MENUDIR_LEFT;
	} else if ((input_held & INPUT_RIGHT) > 0) {
		cursor_direction = MENUDIR_RIGHT;
	} else {
		cursor_direction = NONE;
	}

	//Confirm
	if ((input_hit & INPUT_MENU_CONFIRM) > 0) {
		if (ctx.use_cursor) {
			int item = ctx.stack[ctx.stack_size - 1].selected_item;
			confirm(item, true);
		}
	}

	//Return
	if ((input_hit & INPUT_MENU_RETURN) > 0) {
		int type = ctx.stack[ctx.stack_size - 1].type;

		if (type == MENU_MAIN) {
			open(MENU_QUIT);
		} else if (type == MENU_TRYAGAIN_TIMEUP) {
			//Do nothing
		} else {
			close();
		}
	}
}

void menu_open(int menu_type)
{
	open(menu_type);
}

void menu_close_all()
{
	close_all();
}

void menu_update(float dt)
{
	bool selection_changed;

	//Nothing to do if no menu is open
	if (!menu_is_open()) return;

	if (ctx.level_selected) {
		level_start_delay -= dt;

		//Flash selected item
		ctx.selected_visible = !ctx.selected_visible;

		if (level_start_delay <= 0) {
			ctx.action = MENUACT_PLAY;
			ctx.action_param = level_num | (difficulty << 4);
			ctx.level_selected = false;
			ctx.selected_visible = true;
		}

		return;
	}

	update_audio_icon();
	update_values();

	//Handle selected item change
	selection_changed = false;
	if (cursor_direction != NONE) {
		cursor_delay -= dt;

		if (prev_cursor_direction != cursor_direction) {
			cursor_delay = 0.5f;
			selection_changed = true;
		} else if (cursor_delay <= 0) {
			cursor_delay = 0.1f;
			selection_changed = true;
		}
	}

	prev_cursor_direction = cursor_direction;

	if (selection_changed) {
		int sel = ctx.stack[ctx.stack_size - 1].selected_item;
		int prev_sel = sel;

		if (ctx.use_cursor) {
			do {
				sel = ctx.items[sel].targets[cursor_direction];

				if (sel == -2) {
					sel = stored_sel;
				}
			} while (ctx.items[sel].disabled || ctx.items[sel].hidden);

			if (sel != prev_sel) {
				audio_play_sfx(SFX_SELECT);
			}
		} else {
			sel = 0;
			ctx.use_cursor = true;
			audio_play_sfx(SFX_SELECT);
		}

		if (sel >= store_sel_min && sel <= store_sel_max) {
			stored_sel = sel;
		}

		ctx.stack[ctx.stack_size - 1].selected_item = sel;
	}
}

//Shows an error message
void menu_show_error(const char* msg)
{
	close_all();
	open(MENU_ERROR);

	//Move the text downwards by two lines because the word "ERROR" is drawn
	//separately by renderer.c on the first line
	strcpy(ctx.text, "\n\n");
	strcat(ctx.text, msg);

	ctx.text_offset_x = 0;
	ctx.text_offset_y = -3;
	ctx.text_width = 26;
	ctx.text_height = 4;
	ctx.text_border = true;
}

void menu_adapt_to_screen_size()
{
	int vscreen_width  = display_params->vscreen_width;
	int vscreen_height = display_params->vscreen_height;
	int menu_type;
	int i;

	//Nothing to do if no menu is open
	if (!menu_is_open()) return;

	menu_type = ctx.stack[ctx.stack_size - 1].type;

	//Set the size of the return icon
	for (i = 0; i < ctx.num_items; i++) {
		int icon_sprite = ctx.items[i].icon_sprite;

		if (icon_sprite == SPR_MENU_RETURN || icon_sprite == SPR_MENU_RETURN_SMALL) {
			if (vscreen_height <= 192) {
				ctx.items[i].icon_sprite = SPR_MENU_RETURN_SMALL;
				ctx.items[i].height = 3;
			} else {
				ctx.items[i].icon_sprite = SPR_MENU_RETURN;
				ctx.items[i].height = 5;
			}
		}
	}

	//Set the text
	if (vscreen_width <= 256) {
		if (menu_type == MENU_ABOUT) {
			ctx.text[0] = '\0';
			strcat(ctx.text,
				"\x1B" "Alex vs Bus: The Race\n"
				"\x7F" " 2021-2024 M374LX\n" //0x7F = copyright symbol
				"\n"
				"\x1B" "Release\n"
				" " RELEASE "\n"
				"\n"
				"\x1B" "Licenses\n"
				" The code is under GNU GPLv3,\n"
				" while the assets are under\n"
				" CC BY-SA 4.0.");
		} else if (menu_type == MENU_CREDITS) {
			ctx.text[0] = '\0';
			strcat(ctx.text,
				"\x1B" "M374LX\n"
				" Game design, programming,\n"
				" music, SFX, graphics\n"
				"\n"
				"\x1B" "Hoton Bastos\n"
				" Additional game design\n"
				"\n"
				"\x1B" "Harim Pires\n"
				" Testing\n"
				"\n"
				"\x1B" "Codeman38\n"
				" \"Press Start 2P\" font\n"
				"\n"
				"\x1B" "YoWorks\n"
				" \"Telegrama\" font");
		}
	} else {
		if (menu_type == MENU_ABOUT) {
			ctx.text[0] = '\0';
			strcat(ctx.text,
				"\x1B" "Alex vs Bus: The Race\n"
				"\x7F" " 2021-2024 M374LX\n" //0x7F = copyright symbol
				"\n"
				"\x1B" "Release\n"
				" " RELEASE "\n"
				"\n"
				"\x1B" "Repository\n ");
			strcat(ctx.text, url(REPOSITORY));
			strcat(ctx.text,
				"\n\n"
				"\x1B" "Licenses\n"
				" The code is under GNU GPLv3, while\n"
				" the assets are under CC BY-SA 4.0.\n"
				"\n ");
			strcat(ctx.text, url("https://www.gnu.org/licenses/gpl-3.0.en.html"));
			strcat(ctx.text, "\n ");
			strcat(ctx.text, url("https://creativecommons.org/licenses/by-sa/4.0"));
		} else if (menu_type == MENU_CREDITS) {
			ctx.text[0] = '\0';
			strcat(ctx.text,
				"\x1B" "M374LX" "\x1B" " (");
			strcat(ctx.text, url("https://m374lx.users.sourceforge.net"));
			strcat(ctx.text,
				")\n"
				" Game design, programming,\n"
				" music, SFX, graphics\n"
				"\n"
				"\x1B" "Hoton Bastos\n"
				" Additional game design\n"
				"\n"
				"\x1B" "Harim Pires\n"
				" Testing\n"
				"\n"
				"\x1B" "Codeman38" "\x1B" " (");
			strcat(ctx.text, url("https://www.zone38.net"));
			strcat(ctx.text,
				")\n"
				" \"Press Start 2P\" font\n"
				"\n"
				"\x1B" "YoWorks" "\x1B" " (");
			strcat(ctx.text, url("https://www.yoworks.com"));
			strcat(ctx.text,
				")\n"
				" \"Telegrama\" font");
		}
	}

	//If the screen is too small, hide the audio toggle icon on the main menu
	if (menu_type == MENU_MAIN) {
		ctx.items[5].hidden = false;

		if (vscreen_width <= 256) {
			ctx.items[5].hidden = true;

			if (ctx.use_cursor) {
				if (ctx.stack[ctx.stack_size - 1].selected_item == 5) {
					ctx.stack[ctx.stack_size - 1].selected_item = 0;
				}
			}
		}
	}

	//In smaller screens, move the "Credits" item upwards a bit
	if (menu_type == MENU_ABOUT) {
		ctx.items[0].offset_y = 11;

		if (vscreen_height <= 256) {
			ctx.items[0].offset_y = 10;
		}
		if (vscreen_height <= 224) {
			ctx.items[0].offset_y = 8;
		}
		if (vscreen_height <= 192) {
			ctx.items[0].offset_y = 9;
		}
	}

	//Set the offset, size, and border of the text on "About" and "Credits"
	//menus
	if (menu_type == MENU_ABOUT || menu_type == MENU_CREDITS) {
		ctx.text_border  = true;
		ctx.text_offset_x = 0;
		ctx.text_offset_y = -2;
		ctx.text_width   = 47;
		ctx.text_height  = 15;

		if (vscreen_width <= 320) {
			ctx.text_width = 36;
		}
		if (vscreen_width <= 256) {
			ctx.text_width = 28;
		}

		if (vscreen_width <= 320 || vscreen_height <= 224) {
			ctx.text_border = false;
			ctx.text_offset_x = -1;
		}

		if (vscreen_height <= 192) {
			ctx.text_offset_y = -1;
		}
	}
}

//------------------------------------------------------------------------------

//Decides whether to show the "enable audio" or "disable audio" icon
static void update_audio_icon()
{
	int spr = config->audio_enabled ? SPR_MENU_AUDIO_ON : SPR_MENU_AUDIO_OFF;
	int type = ctx.stack[ctx.stack_size - 1].type;

	if (type == MENU_MAIN) {
		ctx.items[5].icon_sprite = spr;
	} else if (type == MENU_PAUSE) {
		ctx.items[4].icon_sprite = spr;
	}
}

//Updates the values displayed by menu items
static void update_values()
{
	int menu_type = ctx.stack[ctx.stack_size - 1].type;

	if (menu_type == MENU_SETTINGS) {
		strcpy(ctx.items[2].value, (config->touch_buttons_enabled) ? "ON" : "OFF");
	} else if (menu_type == MENU_DISPLAY_SETTINGS) {
		strcpy(ctx.items[0].value, (config->fullscreen) ? "ON" : "OFF");
		strcpy(ctx.items[3].value, (config->scanlines_enabled) ? "ON" : "OFF");
		sprintf(ctx.items[1].value, "%d", config->window_scale);
	} else if (menu_type == MENU_VSCREEN_SIZE) {
		if (config->vscreen_auto_size) {
			strcpy(ctx.items[0].value, "AUTO");
			sprintf(ctx.items[1].value, "%d", display_params->vscreen_width);
			sprintf(ctx.items[2].value, "%d", display_params->vscreen_height);
		} else {
			strcpy(ctx.items[0].value, "MANUAL");
			sprintf(ctx.items[1].value, "%d", config->vscreen_width);
			sprintf(ctx.items[2].value, "%d", config->vscreen_height);
		}
	} else if (menu_type == MENU_AUDIO_SETTINGS) {
		strcpy(ctx.items[0].value, (config->audio_enabled) ? "ON" : "OFF");
		strcpy(ctx.items[1].value, (config->music_enabled) ? "ON" : "OFF");
		strcpy(ctx.items[2].value, (config->sfx_enabled) ? "ON" : "OFF");
	}
}

//Confirms the selection of an item
static void confirm(int item, bool change_item)
{
	wait_input_up = true;

	if (change_item) {
		ctx.stack[ctx.stack_size - 1].selected_item = item;
	}

	switch (ctx.stack[ctx.stack_size - 1].type) {
		case MENU_MAIN:
			switch (item) {
				case 0:
					open(MENU_DIFFICULTY);
					break;

				case 1:
					open(MENU_JUKEBOX);
					break;

				case 2:
					open(MENU_SETTINGS);
					break;

				case 3:
					open(MENU_ABOUT);
					break;

				case 4:
					open(MENU_QUIT);
					break;

				case 5:
					config->audio_enabled = !config->audio_enabled;
					break;
			}
			break;

		case MENU_DIFFICULTY:
			switch (item) {
				case 0:
					difficulty = DIFFICULTY_NORMAL;
					open(MENU_LEVEL);
					break;

				case 1:
					difficulty = DIFFICULTY_HARD;
					open(MENU_LEVEL);
					break;

				case 2:
					difficulty = DIFFICULTY_SUPER;
					open(MENU_LEVEL);
					break;

				case 3:
					close();
					break;
			}
			break;

		case MENU_LEVEL:
			if (item < ctx.num_items - 1) {
				ctx.level_selected = true;
				level_num = item + 1;
				level_start_delay = 0.75f;
			} else {
				//Return
				close();
			}
			break;

		case MENU_JUKEBOX:
			ctx.stack[ctx.stack_size - 1].selected_item = item;
			switch (item) {
				case 0:
					audio_play_bgm(BGM1);
					break;

				case 1:
					audio_play_bgm(BGM2);
					break;

				case 2:
					audio_play_bgm(BGM3);
					break;

				case 3:
					audio_play_bgm(BGMTITLE);
					break;

				case 4:
					close();
					break;
			}

			if (!config->progress_cheat) {
				if (item == data_cheat_sequence[cheat_pos]) {
					cheat_pos++;

					if (data_cheat_sequence[cheat_pos] == -1) {
						config->progress_cheat = true;
						audio_play_sfx(SFX_COIN);
					}
				} else {
					cheat_pos = 0;
				}
			}

			break;

		case MENU_SETTINGS:
			switch (item) {
				case 0:
					open(MENU_DISPLAY_SETTINGS);
					break;

				case 1:
					open(MENU_AUDIO_SETTINGS);
					break;

				case 2:
					if (config->touch_enabled) {
						config->touch_buttons_enabled = !config->touch_buttons_enabled;
					}
					break;

				case 3:
					close();
					break;
			}
			break;

		case MENU_DISPLAY_SETTINGS:
			switch (item) {
				case 0:
					if (!config->fixed_window_mode) {
						config->fullscreen = !config->fullscreen;
					}
					break;

				case 1:
					open(MENU_WINDOW_SCALE);
					break;

				case 2:
					open(MENU_VSCREEN_SIZE);
					break;

				case 3:
					config->scanlines_enabled = !config->scanlines_enabled;
					break;

				case 4:
					close();
					break;
			}
			break;

		case MENU_WINDOW_SCALE:
			if (item < 3) {
				config->window_scale = item + 1;
			}
			close();
			break;

		case MENU_VSCREEN_SIZE:
			switch (item) {
				case 0:
					if (config->vscreen_auto_size) {
						config->vscreen_auto_size = false;
						config->vscreen_width  = display_params->vscreen_width;
						config->vscreen_height = display_params->vscreen_height;

						//Enable vscreen width and height items
						ctx.items[1].disabled = false;
						ctx.items[2].disabled = false;
					} else {
						config->vscreen_auto_size = true;

						//Disable vscreen width and height items
						ctx.items[1].disabled = true;
						ctx.items[2].disabled = true;
					}
					break;

				case 1:
					open(MENU_VSCREEN_WIDTH);
					break;

				case 2:
					open(MENU_VSCREEN_HEIGHT);
					break;

				case 3:
					close();
					break;
			}
			break;

		case MENU_VSCREEN_WIDTH:
			if (item < 5) {
				config->vscreen_width = data_screen_widths[item];
			}
			close();
			break;

		case MENU_VSCREEN_HEIGHT:
			if (item < 5) {
				config->vscreen_height = data_screen_heights[item];
			}
			close();
			break;

		case MENU_AUDIO_SETTINGS:
			switch (item) {
				case 0:
					config->audio_enabled = !config->audio_enabled;
					break;

				case 1:
					config->music_enabled = !config->music_enabled;
					break;

				case 2:
					config->sfx_enabled = !config->sfx_enabled;
					break;

				case 3:
					close();
					break;
			}
			break;

		case MENU_ABOUT:
			switch (item) {
				case 0:
					open(MENU_CREDITS);
					break;

				case 1:
					close();
					break;
			}
			break;

		case MENU_CREDITS:
			close();
			break;

		case MENU_PAUSE:
			switch (item) {
				case 0:
					close_all();
					break;

				case 1:
					open(MENU_TRYAGAIN_PAUSE);
					break;

				case 2:
					open(MENU_SETTINGS);
					break;

				case 3:
					open(MENU_QUIT);
					break;

				case 4:
					config->audio_enabled = !config->audio_enabled;
					break;
			}
			break;

		case MENU_TRYAGAIN_PAUSE:
			switch (item){
				case 0:
					ctx.action = MENUACT_TRYAGAIN;
					break;

				case 1:
					close();
					break;

				case 2:
					close();
					break;
			}
			break;

		case MENU_TRYAGAIN_TIMEUP:
			switch (item){
				case 0:
					ctx.action = MENUACT_TRYAGAIN;
					break;

				case 1:
					ctx.action = MENUACT_TITLE;
					break;
			}
			break;

		case MENU_QUIT:
			switch (item){
				case 0:
					ctx.action = MENUACT_QUIT;
					break;

				case 1:
					close();
					break;

				case 2:
					close();
					break;
			}
			break;

		case MENU_ERROR:
			ctx.action = MENUACT_QUIT;
			break;
	}
}

//Opens a specific menu
static void open(int menu_type)
{
	int sel = 0;

	ctx.stack[ctx.stack_size].type = menu_type;
	ctx.stack_size++;

	load_menu(menu_type);

	switch (menu_type) {
		case MENU_VSCREEN_SIZE:
			ctx.green_bg = true;
			break;

		case MENU_JUKEBOX:
			audio_stop_bgm();
			break;

		case MENU_PAUSE:
			ctx.show_frame = true;
			break;

		case MENU_ERROR:
			audio_stop_bgm();
			audio_play_sfx(SFX_ERROR);
			break;
	}

	//Determine item to be selected by default (if not the first one)
	if (ctx.use_cursor) {
		if (menu_type == MENU_DIFFICULTY) {
			//Highest unlocked difficulty
			sel = config->progress_difficulty;
		} else if (menu_type == MENU_LEVEL) {
			//Highest unlocked level
			sel = config->progress_level - 1;

			if (difficulty < config->progress_difficulty) {
				sel = ctx.num_items - 2;
			}
			if (config->progress_cheat) {
				sel = ctx.num_items - 2;
			}
			if (config->progress_level > ctx.num_items - 2) {
				sel = ctx.num_items - 2;
			}
		} else if (menu_type == MENU_WINDOW_SCALE) {
			//Current window scale
			sel = config->window_scale - 1;
		} else if (menu_type == MENU_VSCREEN_WIDTH) {
			int i;

			//Current vscreen width
			for (i = 0; data_screen_widths[i] > -1; i++) {
				if (data_screen_widths[i] == config->vscreen_width) {
					sel = i;
				}
			}
		} else if (menu_type == MENU_VSCREEN_HEIGHT) {
			int i;

			//Current vscreen height
			for (i = 0; data_screen_heights[i] > -1; i++) {
				if (data_screen_heights[i] == config->vscreen_height) {
					sel = i;
				}
			}
		}

		//Do not select a hidden item
		while (ctx.items[sel].hidden) sel++;
	}

	stored_sel = sel;
	if (sel < store_sel_min || sel > store_sel_max) {
		stored_sel = store_sel_min;
	}

	update_audio_icon();
	wait_input_up = true;

	ctx.stack[ctx.stack_size - 1].selected_item = sel;
}

//Closes the current menu and returns to the previous one
static void close()
{
	int menu_type = ctx.stack[ctx.stack_size - 1].type;

	wait_input_up = true;

	if (menu_type == MENU_JUKEBOX) {
		audio_play_bgm(BGMTITLE);
	} else if (menu_type == MENU_VSCREEN_SIZE) {
		ctx.green_bg = false;
	} else if (menu_type == MENU_PAUSE) {
		ctx.show_frame = false;
	} else if (menu_type == MENU_ERROR) {
		ctx.action = MENUACT_QUIT;
	}

	ctx.text[0] = '\0';

	ctx.stack_size--;
	if (menu_is_open()) {
		int sel;

		load_menu(ctx.stack[ctx.stack_size - 1].type);

		sel = ctx.stack[ctx.stack_size - 1].selected_item;

		stored_sel = sel;
		if (sel < store_sel_min || sel > store_sel_max) {
			stored_sel = store_sel_min;
		}
	}

	if (config->progress_cheat) {
		cheat_pos = 0;
	}
}

//Closes all menus
static void close_all()
{
	ctx.stack_size = 0;
	ctx.num_items = 0;
	ctx.show_frame = false;
	ctx.text[0] = '\0';
}

//Loads a specific menu
static void load_menu(int menu_type)
{
	int i;

	ctx.show_logo = (menu_type == MENU_MAIN);
	ctx.fill_screen = (menu_type != MENU_PAUSE);

	//Set the name to be displayed at the top of the screen
	ctx.display_name = data_menu_display_names[menu_type];

	//Set text for confirmation prompt menus
	switch (menu_type) {
		case MENU_TRYAGAIN_PAUSE:
		case MENU_TRYAGAIN_TIMEUP:
			strcpy(ctx.text, "TRY AGAIN?");
			ctx.text_offset_x = 0;
			ctx.text_offset_y = -4;
			ctx.text_width   = 10;
			ctx.text_height  = 1;
			ctx.text_border  = true;
			break;

		case MENU_QUIT:
			strcpy(ctx.text, "QUIT?");
			ctx.text_offset_x = 0;
			ctx.text_offset_y = -4;
			ctx.text_width   = 10;
			ctx.text_height  = 1;
			ctx.text_border  = true;
			break;
	}

	//Clear items
	ctx.num_items = 0;
	for (i = 0; i < MENU_MAX_ITEMS; i++) {
		MenuItem* it = &ctx.items[i];

		it->offset_x = 0;
		it->offset_y = 0;
		it->caption = "";
		it->value[0] = '\0';
		it->icon_sprite = NONE;
		it->disabled = false;
		it->hidden = false;
	}

	//Load items
	switch (menu_type) {
		case MENU_MAIN:
			set_item(0, 12,  5,  5, -2,  5,  1, SPR_MENU_PLAY);
			set_item(1,  5,  5,  0,  5,  0,  2, SPR_MENU_JUKEBOX);
			set_item(2,  5,  5,  0,  5,  1,  3, SPR_MENU_SETTINGS);
			set_item(3,  5,  5,  0,  5,  2,  4, SPR_MENU_ABOUT);
			set_item(4,  5,  5,  0,  5,  3,  5, SPR_MENU_QUIT);
			set_item(5,  5,  5, -2,  0,  4,  0, SPR_MENU_AUDIO_ON);
			ctx.num_items = 6;
			set_item_position(0, ALIGN_CENTER, 0, -2); //Play
			position_items_center(1, 4, false, 7, 5);
			set_item_position(5, ALIGN_TOPRIGHT, -1, 1); //Audio toggle
			break;

		case MENU_DIFFICULTY:
			set_item(0,  8,  5,  3,  3,  3,  1, NONE);
			set_item(1,  8,  5,  3,  3,  0,  2, NONE);
			set_item(2,  8,  5,  3,  3,  1,  3, NONE);
			set_item(3,  5,  5, -2, -2,  2,  0, SPR_MENU_RETURN);
			ctx.num_items = 4;
			position_items_center(0, 2, false, 10, 0);
			set_item_position(3, ALIGN_TOPLEFT, 1, 1); //Return
			ctx.items[0].caption = "NORMAL";
			ctx.items[1].caption = "HARD";
			ctx.items[2].caption = "SUPER";
			break;

		case MENU_LEVEL:
			set_item(0,  5,  6,  5,  5,  5,  1, SPR_MENU_1);
			set_item(1,  5,  6,  5,  5,  0,  2, SPR_MENU_2);
			set_item(2,  5,  6,  5,  5,  1,  3, SPR_MENU_3);
			set_item(3,  5,  6,  5,  5,  2,  4, SPR_MENU_4);
			set_item(4,  5,  6,  5,  5,  3,  5, SPR_MENU_5);
			set_item(5,  5,  5, -2, -2,  4,  0, SPR_MENU_RETURN);
			ctx.num_items = 6;
			position_items_center(0, 4, false, 6, 0);
			set_item_position(5, ALIGN_TOPLEFT, 1, 1); //Return
			break;

		case MENU_JUKEBOX:
			set_item(0,  5,  6,  4,  4,  4,  1, SPR_MENU_1);
			set_item(1,  5,  6,  4,  4,  0,  2, SPR_MENU_2);
			set_item(2,  5,  6,  4,  4,  1,  3, SPR_MENU_3);
			set_item(3,  5,  6,  4,  4,  2,  4, SPR_MENU_4);
			set_item(4,  5,  5, -2, -2,  3,  0, SPR_MENU_RETURN);
			ctx.num_items = 5;
			position_items_center(0, 3, false, 6, 0);
			set_item_position(4, ALIGN_TOPLEFT, 1, 1); //Return
			break;

		case MENU_SETTINGS:
			set_item(0, 26,  3,  3,  1,  3,  3, NONE);
			set_item(1, 26,  3,  0,  2,  3,  3, NONE);
			set_item(2, 26,  3,  1,  3,  3,  3, NONE);
			set_item(3,  5,  5,  2,  0, -2, -2, SPR_MENU_RETURN);
			ctx.num_items = 4;
			position_items_center(0, 2, true, 4, 0);
			set_item_position(3, ALIGN_TOPLEFT, 1, 1); //Return
			ctx.items[0].caption = "DISPLAY SETTINGS";
			ctx.items[1].caption = "AUDIO SETTINGS";
			ctx.items[2].caption = "TOUCHSCREEN BUTTONS";
			break;

		case MENU_DISPLAY_SETTINGS:
			set_item(0, 26,  3,  4,  1,  4,  4, NONE);
			set_item(1, 26,  3,  0,  2,  4,  4, NONE);
			set_item(2, 26,  3,  1,  3,  4,  4, NONE);
			set_item(3, 26,  3,  2,  4,  4,  4, NONE);
			set_item(4,  5,  5,  3,  0, -2, -2, SPR_MENU_RETURN);
			ctx.num_items = 5;
			position_items_center(0, 3, true, 4, 0);
			set_item_position(4, ALIGN_TOPLEFT, 1, 1); //Return
			ctx.items[0].caption = "FULLSCREEN";
			ctx.items[1].caption = "WINDOW SCALE";
			ctx.items[2].caption = "VSCREEN SIZE";
			ctx.items[3].caption = "SCANLINES";
			break;

		case MENU_WINDOW_SCALE:
			set_item(0, 16,  3,  3,  1,  3,  3, NONE);
			set_item(1, 16,  3,  0,  2,  3,  3, NONE);
			set_item(2, 16,  3,  1,  3,  3,  3, NONE);
			set_item(3,  5,  5,  2,  0, -2, -2, SPR_MENU_RETURN);
			ctx.num_items = 4;
			position_items_center(0, 2, true, 4, 0);
			set_item_position(3, ALIGN_TOPLEFT, 1, 1); //Return
			ctx.items[0].caption = "1";
			ctx.items[1].caption = "2";
			ctx.items[2].caption = "3";
			break;

		case MENU_VSCREEN_SIZE:
			set_item(0, 26,  3,  3,  1,  3,  3, NONE);
			set_item(1, 26,  3,  0,  2,  3,  3, NONE);
			set_item(2, 26,  3,  1,  3,  3,  3, NONE);
			set_item(3,  5,  5,  2,  0, -2, -2, SPR_MENU_RETURN);
			ctx.num_items = 4;
			position_items_center(0, 2, true, 4, 0);
			set_item_position(3, ALIGN_TOPLEFT, 1, 1); //Return
			ctx.items[0].caption = "MODE";
			ctx.items[1].caption = "WIDTH";
			ctx.items[2].caption = "HEIGHT";
			break;

		case MENU_VSCREEN_WIDTH:
			set_item(0,  5,  3,  5,  5,  4,  1, NONE);
			set_item(1,  5,  3,  5,  5,  0,  2, NONE);
			set_item(2,  5,  3,  5,  5,  1,  3, NONE);
			set_item(3,  5,  3,  5,  5,  2,  4, NONE);
			set_item(4,  5,  3,  5,  5,  3,  5, NONE);
			set_item(5,  5,  5, -2, -2,  3,  0, SPR_MENU_RETURN);
			ctx.num_items = 6;
			position_items_center(0, 4, false, 6, 0);
			set_item_position(5, ALIGN_TOPLEFT, 1, 1); //Return

			//Note: the width options here should be the same as in the
			//data_screen_widths[] array, found in data.c
			ctx.items[0].caption = "480";
			ctx.items[1].caption = "432";
			ctx.items[2].caption = "416";
			ctx.items[3].caption = "320";
			ctx.items[4].caption = "256";
			break;

		case MENU_VSCREEN_HEIGHT:
			set_item(0,  5,  3,  5,  5,  5,  1, NONE);
			set_item(1,  5,  3,  5,  5,  0,  2, NONE);
			set_item(2,  5,  3,  5,  5,  1,  3, NONE);
			set_item(3,  5,  3,  5,  5,  2,  4, NONE);
			set_item(4,  5,  3,  5,  5,  3,  5, NONE);
			set_item(5,  5,  5, -2, -2,  4,  0, SPR_MENU_RETURN);
			ctx.num_items = 6;
			position_items_center(0, 4, false, 6, 0);
			set_item_position(5, ALIGN_TOPLEFT, 1, 1); //Return

			//Note: the height options here should be the same as in the
			//data_screen_heights[] array, found in data.c
			ctx.items[0].caption = "270";
			ctx.items[1].caption = "256";
			ctx.items[2].caption = "240";
			ctx.items[3].caption = "224";
			ctx.items[4].caption = "192";
			break;

		case MENU_AUDIO_SETTINGS:
			set_item(0, 26,  3,  3,  1,  3,  3, NONE);
			set_item(1, 26,  3,  0,  2,  3,  3, NONE);
			set_item(2, 26,  3,  1,  3,  3,  3, NONE);
			set_item(3,  5,  5,  2,  0, -2, -2, SPR_MENU_RETURN);
			ctx.num_items = 4;
			position_items_center(0, 2, true, 4, 0);
			set_item_position(3, ALIGN_TOPLEFT, 1, 1); //Return
			ctx.items[0].caption = "AUDIO";
			ctx.items[1].caption = "MUSIC";
			ctx.items[2].caption = "SFX";
			break;

		case MENU_ABOUT:
			set_item(0, 16,  3,  1,  1,  1,  1, NONE);
			set_item(1,  5,  5,  0,  0,  0,  0, SPR_MENU_RETURN);
			ctx.num_items = 2;
			set_item_position(0, ALIGN_CENTER, 0, 14); //Credits
			set_item_position(1, ALIGN_TOPLEFT, 1, 1); //Return
			ctx.items[0].caption = "CREDITS";
			break;

		case MENU_CREDITS:
			set_item(0,  5,  5,  0,  0,  0,  0, SPR_MENU_RETURN);
			ctx.num_items = 1;
			set_item_position(0, ALIGN_TOPLEFT, 1, 1); //Return
			break;

		case MENU_PAUSE:
			set_item(0, 12,  5,  4, -2,  4,  1, SPR_MENU_PLAY);
			set_item(1,  5,  5,  0,  4,  0,  2, SPR_MENU_TRYAGAIN);
			set_item(2,  5,  5,  0,  4,  1,  3, SPR_MENU_SETTINGS);
			set_item(3,  5,  5,  0,  4,  2,  4, SPR_MENU_QUIT);
			set_item(4,  5,  5, -2,  0,  3,  0, SPR_MENU_AUDIO_ON);
			ctx.num_items = 5;
			set_item_position(0, ALIGN_CENTER, 0, -4); //Play
			position_items_center(1, 3, false, 7, 3);
			set_item_position(4, ALIGN_TOPRIGHT, -1, 1); //Audio toggle
			break;

		case MENU_TRYAGAIN_PAUSE:
			set_item(0,  5,  5,  2,  2,  2,  1, SPR_MENU_CONFIRM);
			set_item(1,  5,  5,  2,  2,  0,  2, SPR_MENU_CANCEL);
			set_item(2,  5,  5, -2, -2,  1,  0, SPR_MENU_RETURN);
			ctx.num_items = 3;
			position_items_center(0, 1, false, 9, 4);
			set_item_position(2, ALIGN_TOPLEFT, 1, 1); //Return
			break;

		case MENU_TRYAGAIN_TIMEUP:
			set_item(0,  5,  5,  1,  1,  1,  1, SPR_MENU_CONFIRM);
			set_item(1,  5,  5,  0,  0,  0,  0, SPR_MENU_CANCEL);
			ctx.num_items = 2;
			position_items_center(0, 1, false, 9, 4);
			break;

		case MENU_QUIT:
			set_item(0,  5,  5,  2,  2,  2,  1, SPR_MENU_CONFIRM);
			set_item(1,  5,  5,  2,  2,  0,  2, SPR_MENU_CANCEL);
			set_item(2,  5,  5, -2, -2,  1,  0, SPR_MENU_RETURN);
			ctx.num_items = 3;
			position_items_center(0, 1, false, 9, 4);
			set_item_position(2, ALIGN_TOPLEFT, 1, 1); //Return
			break;

		case MENU_ERROR:
			set_item(0,  5,  5,  0,  0,  0,  0, SPR_MENU_CONFIRM);
			ctx.num_items = 1;
			set_item_position(0, ALIGN_CENTER, 0, 5); //Confirm
	}

	//Determine items to be hidden or disabled
	if (menu_type == MENU_SETTINGS) {
		//Hide "Touchscreen buttons" item if not using a touchscreen
		ctx.items[2].hidden   = !config->touch_enabled;

		//Reposition items
		position_items_center(0, 2, true, 4, 0);
	} else if (menu_type == MENU_DISPLAY_SETTINGS) {
		//Hide "Fullscreen" item if the window mode is fixed
		ctx.items[0].hidden   = config->fixed_window_mode;

		//Hide "Window scale" item if the window mode is fixed
		ctx.items[1].hidden   = config->fixed_window_mode;

		//Reposition items
		position_items_center(0, 3, true, 4, 0);
	} else if (menu_type == MENU_VSCREEN_SIZE) {
		//Disable vscreen width and height items if vscreen auto size is
		//enabled
		ctx.items[1].disabled = config->vscreen_auto_size;
		ctx.items[2].disabled = config->vscreen_auto_size;
	} else if (menu_type == MENU_VSCREEN_WIDTH) {
		if (config->fixed_window_mode) {
			//Disable vscreen width values that are too large for the
			//physical screen (but keep the smallest available value)
			for (i = 0; i < ctx.num_items - 2; i++) {
				if (data_screen_widths[i] > display_params->win_width) {
					ctx.items[i].disabled = true;
				}
			}
		}
	} else if (menu_type == MENU_VSCREEN_HEIGHT) {
		if (config->fixed_window_mode) {
			//Disable vscreen height values that are too large for the
			//physical screen (but keep the smallest available value)
			for (i = 0; i < ctx.num_items - 2; i++) {
				if (data_screen_heights[i] > display_params->win_height) {
					ctx.items[i].disabled = true;
				}
			}
		}
	} else if (menu_type == MENU_DIFFICULTY) {
		if (!config->progress_cheat) {
			if (config->progress_difficulty < DIFFICULTY_HARD) {
				//Disable "hard" if it has not been unlocked
				ctx.items[1].disabled = true;
			}
			if (config->progress_difficulty < DIFFICULTY_SUPER) {
				//Disable "super" if it has not been unlocked
				ctx.items[2].disabled = true;
			}
		}
	} else if (menu_type == MENU_LEVEL) {
		int num_levels = data_difficulty_num_levels[difficulty];

		//Hide levels that do not exist in selected difficulty
		for (i = 0; i < 5; i++) {
			if (i > num_levels - 1) {
				ctx.items[i].hidden = true;
			}
		}
		position_items_center(0, 4, false, 6, 0);

		if (difficulty == config->progress_difficulty && !config->progress_cheat) {
			//Disable selection of locked levels
			for (i = config->progress_level; i <= num_levels - 1; i++) {
				ctx.items[i].icon_sprite = SPR_MENU_LOCKED;
				ctx.items[i].disabled = true;
			}
		}
	}

	//Determine minimum and maximum item to be stored in stored_sel
	if (menu_type == MENU_MAIN || menu_type == MENU_PAUSE) {
		store_sel_min = 1;
	} else {
		store_sel_min = 0;
	}
	store_sel_max = ctx.num_items - 2;

	menu_adapt_to_screen_size();
}

static void set_item_position(int item, int align, int offset_x, int offset_y)
{
	MenuItem* it = &ctx.items[item];

	it->align = align;
	it->offset_x = offset_x;
	it->offset_y = offset_y;
}

//Positions a range of items as a line or row (depending on "vertical"
//parameter) at the center of the screen, with an optional Y offset
//(offset_y) if the items are positioned horizontally
static void position_items_center(int first_item, int last_item, bool vertical,
		int pos_diff, int offset_y)
{

	int num_visible_items = 0;
	int pos;
	int i;

	for (i = first_item; i <= last_item; i++) {
		if (!ctx.items[i].hidden) {
			num_visible_items++;
		}
	}

	pos = -((num_visible_items - 1) * pos_diff);
	pos /= 2;

	for (i = first_item; i <= last_item; i++) {
		MenuItem* it = &ctx.items[i];

		if (!it->hidden) {
			it->align = ALIGN_CENTER;
			if (vertical) {
				it->offset_y = pos;
			} else {
				it->offset_x = pos;
				it->offset_y = offset_y;
			}
			pos += pos_diff;
		}
	}
}

//Sets most of the attributes of a menu item
static void set_item(int item, int width, int height,
		int target_up, int target_down, int target_left, int target_right,
		int icon_sprite)
{
	MenuItem* it = &ctx.items[item];

	it->width = width;
	it->height = height;
	it->targets[MENUDIR_UP] = target_up;
	it->targets[MENUDIR_DOWN] = target_down;
	it->targets[MENUDIR_LEFT] = target_left;
	it->targets[MENUDIR_RIGHT] = target_right;
	it->icon_sprite = icon_sprite;
}

//Just returns a pointer to the first character after the protocol (http:// or
//https://) with a string containing a URL if the width of the virtual screen
//(vscreen) is 320 or less; otherwise, it returns a pointer to the same string
static const char* url(const char* str)
{
	if (display_params->vscreen_width <= 320) {
		if (str_starts_with(str, "https://")) {
			return &str[8];
		} else if (str_starts_with(str, "http://")) {
			return &str[7];
		}
	}

	return str;
}

