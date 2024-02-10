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
 * window.c
 *
 * Description:
 * Manages the game's window on desktop platforms
 *
 */

//------------------------------------------------------------------------------

#include "defs.h"

#include <raylib.h>

//------------------------------------------------------------------------------

//From data.c
extern unsigned char data_window_icon[];

//------------------------------------------------------------------------------

static DisplayParams* display_params;
static Config* config;

static int window_monitor;
static int fullscreen_toggle_step;
static bool change_to_fullscreen;
static bool windowed_once;

static int restore_x;
static int restore_y;

static bool last_vscreen_auto_size;
static int last_vscreen_width;
static int last_vscreen_height;
static int last_window_scale;
static bool last_fullscreen;

//------------------------------------------------------------------------------

//Function prototypes
static void center_window(int window_width, int window_height);

//------------------------------------------------------------------------------

void window_setup(DisplayParams* dp, Config *cfg)
{
#ifndef __ANDROID__
	Image icon;
	int window_width;
	int window_height;

	display_params = dp;
	config = cfg;

	//Set window icon
	icon.data = data_window_icon;
	icon.width = 32;
	icon.height = 32;
	icon.mipmaps = 1;
	icon.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
	SetWindowIcon(icon);

	//Determine minimum and initial window size
	if (config->vscreen_auto_size) {
		window_width  = VSCREEN_MAX_WIDTH  * config->window_scale;
		window_height = VSCREEN_MAX_HEIGHT * config->window_scale;
	} else {
		window_width  = config->vscreen_width  * config->window_scale;
		window_height = config->vscreen_height * config->window_scale;
	}

	ClearWindowState(FLAG_WINDOW_HIDDEN);
	center_window(window_width, window_height);

	if (config->fullscreen) {
		SetWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
		window_monitor = GetCurrentMonitor();
		if (!config->touch_enabled) DisableCursor();
		last_fullscreen = true;
	} else {
		SetWindowSize(window_width, window_height);
		SetWindowPosition(restore_x, restore_y);
		windowed_once = true;
	}
#endif //__ANDROID__
}

bool window_is_fullscreen()
{
#ifdef __ANDROID__
	return true;
#else
	return IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
#endif
}

void window_update()
{
#ifndef __ANDROID__
	bool window_scale_changed = (config->window_scale != last_window_scale);
	bool vscreen_size_changed = false;
	bool change_window_size = false;

	//Check if vscreen size changed
	if (config->vscreen_auto_size != last_vscreen_auto_size) {
		vscreen_size_changed = true;
	}
	if (display_params->vscreen_width  != last_vscreen_width) {
		vscreen_size_changed = true;
	}
	if (display_params->vscreen_height != last_vscreen_height) {
		vscreen_size_changed = true;
	}

	if (window_scale_changed || vscreen_size_changed) {
		if (!window_is_fullscreen()) {
			change_window_size = true;
		}
	}

	if (config->fullscreen != last_fullscreen) {
		if (!config->fixed_window_mode && fullscreen_toggle_step == 0) {
			change_to_fullscreen = config->fullscreen;
			fullscreen_toggle_step = 1;
		}
	}

	//We implement the process of fullscreen toggling in multiple steps taking
	//place across frames, as the operating systems's window manager might
	//prevent the window from being placed in the monitor given by
	//GetCurrentMonitor()
	if (fullscreen_toggle_step != 0) {
		if (change_to_fullscreen) {
			int monitor;
			Vector2 monitor_pos;
			Vector2 window_pos;

			switch (fullscreen_toggle_step) {
				case 1:
					monitor = GetCurrentMonitor();
					monitor_pos = GetMonitorPosition(monitor);
					window_pos = GetWindowPosition();

					restore_x = window_pos.x;
					restore_y = window_pos.y;

					SetWindowPosition(monitor_pos.x, monitor_pos.y);
					if (!config->touch_enabled) DisableCursor();
					fullscreen_toggle_step++;
					break;

				case 2:
					SetWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
					window_monitor = GetCurrentMonitor();
					change_to_fullscreen = false;
					fullscreen_toggle_step = 0;
					break;
			}
		} else {
			switch (fullscreen_toggle_step) {
				case 1:
					ClearWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
					EnableCursor();
					change_window_size = true;
					fullscreen_toggle_step++;
					break;

				case 2:
					SetWindowPosition(restore_x, restore_y);
					windowed_once = true;
					fullscreen_toggle_step = 0;
					break;
			}
		}
	}

	if (change_window_size) {
		bool auto_size = config->vscreen_auto_size;
		int window_width  = (auto_size) ? VSCREEN_MAX_WIDTH  : config->vscreen_width;
		int window_height = (auto_size) ? VSCREEN_MAX_HEIGHT : config->vscreen_height;

		window_width  *= config->window_scale;
		window_height *= config->window_scale;

		SetWindowSize(window_width, window_height);
	}

	last_vscreen_auto_size = config->vscreen_auto_size;
	last_vscreen_width  = display_params->vscreen_width;
	last_vscreen_height = display_params->vscreen_height;
	last_window_scale = config->window_scale;
	last_fullscreen = config->fullscreen;
#endif //__ANDROID__
}

//------------------------------------------------------------------------------

//Centers the window in the monitor and sets the restore position
static void center_window(int window_width, int window_height)
{
#ifndef __ANDROID__
	int monitor = GetCurrentMonitor();
	int monitor_width  = GetMonitorWidth(monitor);
	int monitor_height = GetMonitorHeight(monitor);
	Vector2 monitor_pos = GetMonitorPosition(monitor);

	restore_x = (monitor_width  - window_width)  / 2 + (int)monitor_pos.x;
	restore_y = (monitor_height - window_height) / 2 + (int)monitor_pos.y;

	if (!window_is_fullscreen()) {
		SetWindowPosition(restore_x, restore_y);
	}
#endif
}

