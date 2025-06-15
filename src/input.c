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
 * input.c
 *
 * Description:
 * User input handling functions
 *
 */

//------------------------------------------------------------------------------

#include "defs.h"

#include <raylib.h>
#include <stddef.h>

//------------------------------------------------------------------------------

//From data.c
extern const InputActionKey data_input_keyboard[];
extern const InputActionButton data_input_gamepad[];

//------------------------------------------------------------------------------

static DisplayParams* display_params;
static Config* config;

//Touchscreen
static bool touching;
static bool was_touching;
static bool pause_touched;
static bool left_touched;
static bool right_touched;
static bool jump_touched;

//Screen tap position
static int tap_x;
static int tap_y;

//------------------------------------------------------------------------------

void input_init(DisplayParams* dp, Config* cfg)
{
	display_params = dp;
	config = cfg;
}

int input_get_tap_x()
{
	return tap_x;
}

int input_get_tap_y()
{
	return tap_y;
}

void input_handle_touch()
{
	int button_width   = TOUCH_BUTTON_WIDTH;
	int vscreen_width  = display_params->vscreen_width;
	int vscreen_height = display_params->vscreen_height;
	int win_width     = display_params->win_width;
	int win_height    = display_params->win_height;
	int scale = display_params->scale;

	//Screen touch positions
	bool touched[MAX_TOUCH_POINTS];
	int touch_x[MAX_TOUCH_POINTS];
	int touch_y[MAX_TOUCH_POINTS];

	int x, y;
	int i;

	if (!config->touch_enabled) return;

	//Reset screen touch positions
	for (i = 0; i < MAX_TOUCH_POINTS; i++) {
		touched[i] = false;
		touch_x[i] = -1;
		touch_y[i] = -1;
	}

	//Determine touched positions on screen
	for (i = 0; i < MAX_TOUCH_POINTS; i++) {
		if (i < GetTouchPointCount()) {
			Vector2 pos = GetTouchPosition(i);

			if (pos.y > 0 && pos.y < display_params->win_height) {
				touched[i] = true;
				touch_x[i] = (int)pos.x;
				touch_y[i] = (int)pos.y;
			}
		}
	}

	//Handle mouse, which simulates screen touches
	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		x = GetMouseX();
		y = GetMouseY();

		if (x >= 0 && y >= 0 && x < win_width && y < win_height) {
			touched[0] = true;
			touch_x[0] = x;
			touch_y[0] = y;
		}
	}

	was_touching = touching;
	touching = false;

	//Check if the screen is being touched and handle left, right, and jump
	//buttons
	for (i = 0; i < MAX_TOUCH_POINTS; i++) {
		if (!touched[i]) continue;

		touching = true;

		if (!config->show_touch_controls)   break;
		if (!config->touch_buttons_enabled) break;

		x = touch_x[i];
		y = touch_y[i];

		//If the Y position is above all buttons (other than pause), nothing
		//to do
		if (y < win_height - (70 * scale)) continue;

		if (x < button_width * scale) {
			left_touched = true;
		}
		if (x >= button_width * scale && x < (button_width * 2) * scale) {
			right_touched = true;
		}
		if (x >= win_width - (TOUCH_JUMP_OFFSET_X * scale)) {
			jump_touched = true;
		}
	}

	//Reset tap position
	tap_x = -1;
	tap_y = -1;

	for (i = 0; i < MAX_TOUCH_POINTS; i++) {
		if (touched[i]) {
			//Convert X coordinate from physical screen to virtual screen
			x = touch_x[i];
			x -= (win_width - (vscreen_width * scale)) / 2;
			x /= scale;

			//Convert Y coordinate from physical screen to virtual screen
			y = touch_y[i];
			y -= (win_height - (vscreen_height * scale)) / 2;
			y /= scale;
		} else {
			x = -1;
			y = -1;
		}

		//Ignore touches outside the virtual screen
		if (x < 0 || x > vscreen_width || y < 0 || y > vscreen_height) {
			continue;
		}

		//Handle pause button
		if (config->show_touch_controls && !was_touching) {
			if (x >= vscreen_width - 32 && y <= 32) {
				pause_touched = true;
			}
		}

		//Store tap position
		if (i == 0 && !was_touching) {
			tap_x = x;
			tap_y = y;
		}
	}
}

int input_read()
{
	int input_held = 0;
	int pad;
	int i;

	//Read input from keyboard
	for (i = 0; data_input_keyboard[i].key >= 0; i++) {
		if (IsKeyDown(data_input_keyboard[i].key)) {
			input_held |= data_input_keyboard[i].action;
		}
	}

	//Read input from gamepads
	for (pad = 0; pad < MAX_GAMEPADS; pad++) {
		float axis;

		if (!IsGamepadAvailable(pad)) continue;

		//Read buttons
		for (i = 0; data_input_gamepad[i].button >= 0; i++) {
			if (IsGamepadButtonDown(pad, data_input_gamepad[i].button)) {
				input_held |= data_input_gamepad[i].action;
			}
		}

		//Read X axis
		axis = GetGamepadAxisMovement(pad, 0);
		input_held |= (axis <= -0.5) ? INPUT_LEFT  : 0;
		input_held |= (axis >=  0.5) ? INPUT_RIGHT : 0;

		//Read Y axis
		axis = GetGamepadAxisMovement(pad, 1);
		input_held |= (axis <= -0.5) ? INPUT_UP    : 0;
		input_held |= (axis >=  0.5) ? INPUT_DOWN  : 0;
	}

	//Read input from touchscreen buttons
	if (left_touched)  input_held |= INPUT_LEFT;
	if (right_touched) input_held |= INPUT_RIGHT;
	if (jump_touched)  input_held |= INPUT_JUMP;
	if (pause_touched) input_held |= INPUT_PAUSE_TOUCH;

	//Cannot press left and right at the same time
	if (input_held & INPUT_RIGHT) input_held &= ~INPUT_LEFT;

	//Cannot press up and down at the same time
	if (input_held & INPUT_DOWN)  input_held &= ~INPUT_UP;

	//Reset state of touchscreen buttons
	left_touched  = false;
	right_touched = false;
	jump_touched  = false;
	pause_touched = false;

	return input_held;
}

