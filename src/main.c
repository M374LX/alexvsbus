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
 * main.c
 *
 * Description:
 * The main source file, which includes the main() function and the main loop
 *
 */

//------------------------------------------------------------------------------

#include "defs.h"

#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//------------------------------------------------------------------------------

//From renderer.c
bool renderer_init(DisplayParams* dp, Config* cfg, PlayCtx* pctx, MenuCtx* mctx);
bool renderer_load_gfx();
void renderer_draw(int screen_type, int input_state, int wipe_value);
void renderer_show_save_error(bool show);
void renderer_cleanup();

//From audio.c
void audio_init(Config* cfg);
void audio_load_sfx();
void audio_stop_bgm();
void audio_play_bgm(int id);
void audio_stop_all_sfx();
void audio_update();
void audio_handle_toggling();
void audio_cleanup();

//From input.c
void input_init(DisplayParams* dp, Config* cfg);
int input_get_tap_x();
int input_get_tap_y();
void input_handle_touch();
int input_read();

//From play.c
PlayCtx* play_init(DisplayParams* dp);
void play_clear();
void play_set_input(int input_held);
void play_update(float dt);
void play_adapt_to_screen_size();

//From lineread.c
bool lineread_open(const char* path);
bool lineread_ended();
void lineread_getline(char* dst);
int lineread_num_tokens(const char* str);
const char* lineread_token(const char* str, int token);
int lineread_token_int(const char* str, int token);

//From levelload.c
void levelload_init(PlayCtx* ctx);
int levelload_load(const char* filename);

//From menu.c
MenuCtx* menu_init(DisplayParams* dp, Config* cfg);
bool menu_is_open();
void menu_handle_tap(int x, int y);
void menu_handle_keys(int input_held, int input_hit);
void menu_open(int menu_type);
void menu_close_all();
void menu_update(float dt);
void menu_show_error(const char* msg);
void menu_adapt_to_screen_size();

//From util.c
bool str_starts_with(const char* str, const char* start);
bool str_only_whitespaces(const char* str);
int get_file_size(const char* path);
const char* file_from_path(const char* path);
void process_path(const char* in, char* out, size_t maxlen);
bool readable_dir(const char* path);
bool create_parent_dirs(const char* path);
void msgbox_error(const char* msg);

//From win32.c
void win32_msgbox(const char* msg);
void win32_console_msg(const char* msg);

//From window.c
void window_setup(DisplayParams* dp, Config* cfg);
bool window_is_fullscreen();
void window_update();

//From data.c
extern const int data_screen_widths[];
extern const int data_screen_heights[];
extern const int data_difficulty_num_levels[];
extern unsigned char data_window_icon[];

//------------------------------------------------------------------------------

static bool quit;

//Command-line parameters
static struct {
	bool error;
	bool help;
	bool version;
	const char* config;
	const char* assets_dir;
	bool touch_enabled;
	bool fullscreen;
	bool windowed;
	bool fixed_window_mode;
	int window_scale;          //0 = unset
	int scanlines_enabled;     //0 = unset; -1 = disable; 1 = enable
	int audio_enabled;         //0 = unset; -1 = disable; 1 = enable
	int music_enabled;         //0 = unset; -1 = disable; 1 = enable
	int sfx_enabled;           //0 = unset; -1 = disable; 1 = enable
	int touch_buttons_enabled; //0 = unset; -1 = disable; 1 = enable
	int vscreen_width;         //0 = unset; -1 = auto
	int vscreen_height;        //0 = unset; -1 = auto
} cli;

//Path to the config file
static char config_path[512];

//Delta time (seconds since the previous frame)
static float delta_time;

//Display parameters
static DisplayParams display_params;

//Configuration
static Config config;

//Game progress
static bool progress_checked;

//Screen type (playing game, final score, ...)
static int screen_type;

//Gameplay
static PlayCtx* play_ctx;

//Delayed action
static int delayed_action_type;
static float action_delay;

//Menu
static MenuCtx* menu_ctx;

//Input
static int input_hit;
static int input_held;
static int old_input_held;
static bool wait_input_up;

//Screen wiping effects
static int wipe_cmd;
static int wipe_value;
static int wipe_delta;
static float wipe_delay;

//------------------------------------------------------------------------------

//Function prototypes
static void parse_cli(int argc, char* argv[]);
static bool parse_vscreen_size_arg(const char* arg);
static void show_help();
static void show_version();
static void show_error(const char* err);
static bool init();
static void main_loop();
static void cleanup();
static void get_delta_time();
static void handle_input();
static void handle_menu_action();
static void handle_pause();
static void check_game_progress();
static void handle_level_end();
static void handle_delayed_action();
static void update_screen_wipe();
static void adapt_to_screen_size();
static void show_title();
static void show_final_score();
static void start_level(int level_num, int difficulty, bool skip_initial_sequence);
static void start_ending_sequence();
static bool find_assets_dir();
static void find_config_path();
static void load_config();
static bool save_config();
static void auto_size_vscreen();
static void scale_manual_vscreen();

//------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	parse_cli(argc, argv);

	if (cli.error) {
		show_help();
		return 1;
	} else if (cli.help) {
		show_help();
		return 0;
	} else if (cli.version) {
		show_version();
		return 0;
	}

	if (!init()) {
		cleanup();
		return 1;
	}

	main_loop();
	cleanup();

	return 0;
}

static void parse_cli(int argc, char* argv[])
{
	int i;

	for (i = 1; i < argc; i++) {
		const char* a = argv[i];

		if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
			cli.help = true;
			return;
		} else if (strcmp(a, "-v") == 0 || strcmp(a, "--version") == 0) {
			cli.version = true;
			return;
		} else if (strcmp(a, "-f") == 0 || strcmp(a, "--fullscreen") == 0) {
			cli.fullscreen = true;
			cli.windowed = false;
		} else if (strcmp(a, "-w") == 0 || strcmp(a, "--windowed") == 0) {
			cli.windowed = true;
			cli.fullscreen = false;
		} else if (strcmp(a, "-c") == 0 || strcmp(a, "--config") == 0) {
			i++;
			if (i >= argc) {
				cli.error = true;
				return;
			}

			if (argv[i][0] != '\0' && !str_only_whitespaces(argv[i])) {
				cli.config = argv[i];
			}
		} else if (strcmp(a, "--assets-dir") == 0) {
			i++;
			if (i >= argc) {
				cli.error = true;
				return;
			}

			if (argv[i][0] != '\0' && !str_only_whitespaces(argv[i])) {
				cli.assets_dir = argv[i];
			}
		} else if (strcmp(a, "--vscreen-size") == 0) {
			i++;
			if (i >= argc) {
				cli.error = true;
				return;
			}

			if (!parse_vscreen_size_arg(argv[i])) {
				cli.error = true;
				return;
			}
		} else if (strcmp(a, "--window-scale") == 0) {
			i++;
			if (i >= argc) {
				cli.error = true;
				return;
			}

			a = argv[i];
			if (strcmp(a, "1") == 0) {
				cli.window_scale = 1;
			} else if (strcmp(a, "2") == 0) {
				cli.window_scale = 2;
			} else if (strcmp(a, "3") == 0) {
				cli.window_scale = 3;
			} else {
				cli.error = true;
				return;
			}
		} else if (strcmp(a, "--fixed-window-mode") == 0) {
			cli.fixed_window_mode = true;
		} else if (strcmp(a, "--scanlines-on") == 0) {
			cli.scanlines_enabled = 1;
		} else if (strcmp(a, "--scanlines-off") == 0) {
			cli.scanlines_enabled = -1;
		} else if (strcmp(a, "--audio-on") == 0) {
			cli.audio_enabled = 1;
		} else if (strcmp(a, "--audio-off") == 0) {
			cli.audio_enabled = -1;
		} else if (strcmp(a, "--music-on") == 0) {
			cli.music_enabled = 1;
		} else if (strcmp(a, "--music-off") == 0) {
			cli.music_enabled = -1;
		} else if (strcmp(a, "--sfx-on") == 0) {
			cli.sfx_enabled = 1;
		} else if (strcmp(a, "--sfx-off") == 0) {
			cli.sfx_enabled = -1;
		} else if (strcmp(a, "--touch") == 0) {
			cli.touch_enabled = true;
		} else if (strcmp(a, "--touch-buttons-on") == 0) {
			cli.touch_buttons_enabled = 1;
		} else if (strcmp(a, "--touch-buttons-off") == 0) {
			cli.touch_buttons_enabled = -1;
		} else if (strcmp(a, "--mobile") == 0) {
			//Shorthand for --fixed-window-mode and --touch
			cli.fixed_window_mode = true;
			cli.touch_enabled = true;
		} else {
			cli.error = true;
			return;
		}
	}
}

static bool parse_vscreen_size_arg(const char* arg)
{
	if (strcmp(arg, "auto") == 0) {
		cli.vscreen_width  = -1;
		cli.vscreen_height = -1;
	} else {
		int sep_pos = -1; //Separator position
		int width, height;
		bool supported_width, supported_height;
		int len = strlen(arg);
		int i;

		if (len > 7) {
			return false;
		}

		//Find "x" separator
		for (i = 0; i < len; i++) {
			if (arg[i] == 'X' || arg[i] == 'x') {
				if (sep_pos == -1) {
					sep_pos = i;
				} else {
					//Error: more than one separator
					return false;
				}
			}
		}

		//Ensure the argument contains only digits (besides the separator)
		for (i = 0; i < len; i++) {
			if (i != sep_pos && (arg[i] < '0' || arg[i] > '9')) {
				return false;
			}
		}

		if (sep_pos < 1 || sep_pos > 3) {
			return false;
		}

		width  = atoi(arg);
		height = atoi(&arg[sep_pos + 1]);
		supported_width  = false;
		supported_height = false;

		for (i = 0; data_screen_widths[i] > -1; i++) {
			if (width == data_screen_widths[i]) {
				supported_width = true;
				break;
			}
		}
		for (i = 0; data_screen_heights[i] > -1; i++) {
			if (height == data_screen_heights[i]) {
				supported_height = true;
				break;
			}
		}

		if (!supported_width || !supported_height) {
			return false;
		}

		cli.vscreen_width  = width;
		cli.vscreen_height = height;
	}

	return true;
}

static void show_help()
{
	char msg[2048];
	char tmp[16];
	int i;

	strcpy(msg,
		"Alex vs Bus: The Race\n"
		"\n"
		"-h, --help               Show this usage information and exit\n"
		"-v, --version            Show version and license information and exit\n"
		"-c, --config <file>      Set the config file to use\n"
		"-f, --fullscreen         Run in fullscreen mode\n"
		"-w, --windowed           Run in windowed mode\n"
		"--assets-dir <directory> Set assets directory to use\n"
		"--window-scale <scale>   Set window scale (1 to 3)\n"
		"--vscreen-size <size>    Set the size of the virtual screen (vscreen)\n"
		"--fixed-window-mode      Remove the ability to toggle between fullscreen\n"
		"                         and windowed mode and to change the window scale,\n"
		"                         as if the game were running on a mobile device\n"
		"--scanlines-on           Enable scanlines visual effect\n"
		"--scanlines-off          Disable scanlines visual effect\n"
		"--audio-on               Enable audio output\n"
		"--audio-off              Disable audio output\n"
		"--music-on               Enable music\n"
		"--music-off              Disable music\n"
		"--sfx-on                 Enable sound effects\n"
		"--sfx-off                Disable sound effects\n"
		"--touch                  Enable the use of the mouse to simulate a touchscreen\n"
		"--touch-buttons-on       Enable left, right, and jump buttons on\n"
		"                         touchscreen (visible only if --touch is also used)\n"
		"--touch-buttons-off      Disable left, right, and jump buttons on\n"
		"                         touchscreen\n"
		"--mobile                 As a shorthand for --fixed-window-mode and --touch,\n"
		"                         cause the game to act as if it were running on a\n"
		"                         mobile device\n"
		"\n"
		"For --vscreen-size, the size can be either \"auto\" or a width and a height\n"
		"separated by an \"x\" (example: 480x270), with the supported values listed\n"
		"below.\n\n"
	);

	strcat(msg, "Supported width values:\n");
	for (i = 0; data_screen_widths[i] > -1; i++) {
		if (i > 0) strcat(msg, ", ");
		sprintf(tmp, "%d", data_screen_widths[i]);
		strcat(msg, tmp);
	}
	strcat(msg, "\n\n");

	strcat(msg, "Supported height values:\n");
	for (i = 0; data_screen_heights[i] > -1; i++) {
		if (i > 0) strcat(msg, ", ");
		sprintf(tmp, "%d", data_screen_heights[i]);
		strcat(msg, tmp);
	}
	strcat(msg, "\n");

#ifdef _WIN32
	win32_console_msg(msg);
#else
	printf("%s", msg);
#endif
}

static void show_version()
{
	char msg[2048];

	strcpy(msg,
		"Alex vs Bus: The Race\n"
		"Release " RELEASE "\n"
		"\n"
		"Copyright (C) 2021-2024 M374LX\n"
		"\n"
		"This program is free software: you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation, either version 3 of the License, or\n"
		"(at your option) any later version.\n"
		"\n"
		"This program is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		"GNU General Public License for more details.\n"
		"\n"
		"You should have received a copy of the GNU General Public License\n"
		"along with this program.  If not, see <https://www.gnu.org/licenses/>.\n"
	);

#ifdef _WIN32
	win32_msgbox(msg);
#else
	printf("%s", msg);
#endif
}

static void show_error(const char* err)
{
#ifndef _WIN32
	fprintf(stderr, "%s\n", err);
#endif
	msgbox_error(err);
}

static bool init()
{
#ifndef __ANDROID__
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIDDEN);
#else
	SetConfigFlags(FLAG_VSYNC_HINT);
#endif

	SetTraceLogLevel(LOG_NONE);
	InitWindow(0, 0, GAME_TITLE);
	SetExitKey(KEY_NULL);

	if (!IsWindowReady()) {
		show_error("Graphics device initialization failed.");
		return false;
	}

	if (!find_assets_dir()) {
		if (cli.assets_dir == NULL) {
			show_error("Unable to find assets directory.");
		} else {
			show_error("Assets directory could not be opened.");
		}

		return false;
	}

	find_config_path();
	load_config();
	audio_init(&config);
	input_init(&display_params, &config);
	play_ctx = play_init(&display_params);
	menu_ctx = menu_init(&display_params, &config);

	if (!renderer_init(&display_params, &config, play_ctx, menu_ctx)) {
		show_error("Renderer initialization failed.");
		return false;
	}

	if (!renderer_load_gfx()) {
		show_error("Failed to load graphics.");
		return false;
	}

	levelload_init(play_ctx);
	audio_load_sfx();
	audio_handle_toggling();
	window_setup(&display_params, &config);
	adapt_to_screen_size();

	config.fullscreen = window_is_fullscreen();

	delayed_action_type = NONE;
	wipe_cmd = NONE;

	play_clear();
	show_title();

	return true;
}

static void main_loop()
{
	quit = false;

	while (!quit) {
		if (WindowShouldClose()) {
			quit = true;
		}

		get_delta_time();
		handle_input();
		audio_update();

		if (menu_is_open()) {
			menu_handle_keys(input_held, input_hit);
			menu_update(delta_time);
			handle_menu_action();

			//Menu just closed
			if (!menu_is_open()) {
				wait_input_up = true;
			}
		} else if (screen_type == SCR_PLAY) {
			play_set_input(input_held);
			play_update(delta_time);
			handle_pause();
			check_game_progress();
			handle_level_end();
		}

		handle_delayed_action();
		audio_handle_toggling();
		update_screen_wipe();
		adapt_to_screen_size();
		renderer_draw(screen_type, input_held, wipe_value);
		window_update();
	}
}

static void cleanup()
{
	save_config();

	renderer_cleanup();
	audio_cleanup();

	if (IsWindowReady()) {
		CloseWindow();
	}
}

//------------------------------------------------------------------------------

static void get_delta_time()
{
	delta_time = GetFrameTime();

	//Limit delta time to prevent problems with collision detection
	if (delta_time > MAX_DT) delta_time = MAX_DT;
}

static void handle_input()
{
	if (config.touch_enabled) {
		//Determine if the touchscreen controls should be shown
		config.show_touch_controls = true;
		if (menu_is_open()) {
			config.show_touch_controls = false;
		}
		if (screen_type != SCR_PLAY) {
			config.show_touch_controls = false;
		}
		if (play_ctx->level_num == LVLNUM_ENDING) {
			config.show_touch_controls = false;
		}

		input_handle_touch();
		menu_handle_tap(input_get_tap_x(), input_get_tap_y());
	}

	input_held = input_read();
	if (wait_input_up) {
		if (input_held == 0) {
			wait_input_up = false;
		} else {
			input_held = 0;
		}
	}

	input_hit = input_held & (~old_input_held);
	old_input_held = input_held;

	if (input_hit & INPUT_CFG_AUDIO_TOGGLE) {
		config.audio_enabled = !config.audio_enabled;
	}
}

static void handle_menu_action()
{
	int param = menu_ctx->action_param;
	int level_num = (param & 0xF);
	int difficulty = (param >> 4);

	switch (menu_ctx->action) {
		case MENUACT_QUIT:
			if (screen_type == SCR_PLAY) {
				show_title();
			} else {
				menu_close_all();
				quit = true;
			}
			break;

		case MENUACT_TITLE:
			show_title();
			break;

		case MENUACT_PLAY:
			menu_close_all();
			start_level(level_num, difficulty, false);
			break;

		case MENUACT_TRYAGAIN:
			menu_close_all();
			delayed_action_type = DELACT_TRY_AGAIN;
			action_delay = 0;
			break;
	}

	menu_ctx->action = NONE;
}

static void handle_pause()
{
	bool pause = (input_hit & INPUT_PAUSE) > 0;
	bool pause_touch = (input_hit & INPUT_PAUSE_TOUCH) > 0;

	if (play_ctx->can_pause && (pause || pause_touch)) {
		menu_ctx->use_cursor = !pause_touch;
		menu_open(MENU_PAUSE);
		audio_stop_all_sfx();
		wait_input_up = true;
	}
}

static void check_game_progress()
{
	int num_levels = data_difficulty_num_levels[play_ctx->difficulty];

	if (progress_checked) return;
	if (!play_ctx->goal_reached) return;

	progress_checked = true;

	if (config.progress_cheat) return;
	if (play_ctx->level_num != config.progress_level) return;
	if (play_ctx->difficulty != config.progress_difficulty) return;

	//Unlock next level
	config.progress_level++;
	if (config.progress_level > num_levels) {
		if (config.progress_difficulty < DIFFICULTY_MAX) {
			config.progress_level = 1;
			config.progress_difficulty++;
		} else {
			config.progress_level = num_levels;
		}
	}

	//Try to save the configuration, which includes the game progress
	if (!save_config()) {
		//Display a message on the screen if it fails
		renderer_show_save_error(true);
	}
}

static void handle_level_end()
{
	if (play_ctx->sequence_step != SEQ_FINISHED) return;

	if (play_ctx->level_num == LVLNUM_ENDING) {
		show_final_score();
	} else if (play_ctx->time_up) {
		screen_type = SCR_BLANK;
		wipe_cmd = WIPECMD_CLEAR;
		audio_stop_bgm();
		menu_open(MENU_TRYAGAIN_TIMEUP);
	} else if (play_ctx->goal_reached) {
		if (!play_ctx->last_level) {
			start_level(play_ctx->level_num + 1, play_ctx->difficulty, false);
		} else if (play_ctx->difficulty == DIFFICULTY_MAX) {
			show_final_score();
		} else {
			start_ending_sequence();
		}
	}
}

static void handle_delayed_action()
{
	if (delayed_action_type == NONE) return;

	action_delay -= delta_time;
	if (action_delay > 0) return;

	switch (delayed_action_type) {
		case DELACT_TITLE:
			show_title();
			break;

		case DELACT_NEXT_DIFFICULTY:
			start_level(1, play_ctx->difficulty + 1, false);
			break;

		case DELACT_TRY_AGAIN:
			play_ctx->score = 0;
			start_level(play_ctx->level_num, play_ctx->difficulty, true);
			break;
	}

	delayed_action_type = NONE;
}

static void update_screen_wipe()
{
	if (play_ctx->wipe_in) {
		wipe_cmd = WIPECMD_IN;
		play_ctx->wipe_in = false;
	}
	if (play_ctx->wipe_out) {
		wipe_cmd = WIPECMD_OUT;
		play_ctx->wipe_out = false;
	}

	switch (wipe_cmd) {
		case WIPECMD_IN:
			wipe_delay = WIPE_MAX_DELAY;
			wipe_value = WIPE_MAX_VALUE;
			wipe_delta = -WIPE_DELTA;
			break;

		case WIPECMD_OUT:
			wipe_delay = WIPE_MAX_DELAY;
			wipe_value = 0;
			wipe_delta = WIPE_DELTA;
			break;

		case WIPECMD_CLEAR:
			wipe_value = 0;
			wipe_delta = 0;
			break;
	}
	wipe_cmd = NONE;

	wipe_delay -= delta_time;
	if (wipe_delay > 0) return;

	wipe_delay = WIPE_MAX_DELAY;
	wipe_value += wipe_delta;

	if (wipe_value <= 0) {
		wipe_value = 0;
		wipe_delta = 0;
	} else if (wipe_value >= WIPE_MAX_VALUE) {
		wipe_value = WIPE_MAX_VALUE;
		wipe_delta = 0;
	}
}

static void adapt_to_screen_size()
{
	display_params.win_width  = GetScreenWidth();
	display_params.win_height = GetScreenHeight();

	if (config.vscreen_auto_size) {
		auto_size_vscreen();
	} else {
		scale_manual_vscreen();
	}

	if (screen_type == SCR_PLAY || screen_type == SCR_PLAY_FREEZE) {
		play_adapt_to_screen_size();
	}

	menu_adapt_to_screen_size();
}

//------------------------------------------------------------------------------

static void show_title()
{
	screen_type = SCR_BLANK;

	play_ctx->score = 0;
	play_ctx->level_num = -1;

	audio_stop_all_sfx();
	audio_play_bgm(BGMTITLE);

	menu_close_all();
	menu_open(MENU_MAIN);

	wipe_cmd = WIPECMD_CLEAR;
}

static void show_final_score()
{
	screen_type = SCR_FINALSCORE;

	if (play_ctx->difficulty == DIFFICULTY_MAX) {
		delayed_action_type = DELACT_TITLE;
	} else {
		delayed_action_type = DELACT_NEXT_DIFFICULTY;
	}
	action_delay = 4.0f;

	wipe_cmd = WIPECMD_CLEAR;
}

static void start_level(int level_num, int difficulty, bool skip_initial_sequence)
{
	int err;
	char filename[530];
	char diffch = 'n'; //Difficulty character in level filename (n, h, or s)

	switch (difficulty) {
		case DIFFICULTY_NORMAL: diffch = 'n'; break;
		case DIFFICULTY_HARD:   diffch = 'h'; break;
		case DIFFICULTY_SUPER:  diffch = 's'; break;
	}

	snprintf(filename, sizeof(filename), "%slevel%d%c", config.assets_dir, level_num, diffch);

	renderer_show_save_error(false);
	play_clear();

	err = levelload_load(filename);
	if (err != LVLERR_NONE) {
		char msg[64] = "";

		switch (err) {
			case LVLERR_CANNOT_OPEN:
				strcpy(msg, "Cannot open level file");
				break;

			case LVLERR_TOO_LARGE:
				strcpy(msg, "Level file too large");
				break;

			case LVLERR_INVALID:
				strcpy(msg, "Invalid level file");
				break;
		}

		strcat(msg, ":\n");
		strcat(msg, file_from_path(filename));

		screen_type = SCR_BLANK;
		wipe_cmd = WIPECMD_CLEAR;
		menu_show_error(msg);

		return;
	}

	progress_checked = false;
	screen_type = SCR_PLAY;

	play_ctx->difficulty = difficulty;
	play_ctx->level_num = level_num;
	play_ctx->last_level = (level_num == data_difficulty_num_levels[difficulty]);
	play_ctx->sequence_step = SEQ_INITIAL;
	play_ctx->skip_initial_sequence = skip_initial_sequence;

	if (play_ctx->last_level) {
		play_ctx->bus.num_characters = 3;
	} else {
		switch (level_num) {
			case 1: play_ctx->bus.num_characters = 0; break;
			case 2: play_ctx->bus.num_characters = 0; break;
			case 3: play_ctx->bus.num_characters = 1; break;
			case 4: play_ctx->bus.num_characters = 2; break;
			case 5: play_ctx->bus.num_characters = 3; break;
		}
	}

	play_ctx->bus.route_sign = level_num;
	play_ctx->cam.fixed_at_leftmost = true;

	audio_play_bgm(play_ctx->bgm);
	wipe_cmd = WIPECMD_IN;

	play_adapt_to_screen_size();
}

static void start_ending_sequence()
{
	renderer_show_save_error(false);
	play_clear();

	progress_checked = false;
	screen_type = SCR_PLAY;

	play_ctx->level_num = LVLNUM_ENDING;
	play_ctx->last_level = false;

	play_ctx->level_size = 8 * VSCREEN_MAX_WIDTH;
	play_ctx->bg_color = SPR_BG_SKY3;
	play_ctx->bgm = BGM3;

	play_ctx->sequence_step = SEQ_ENDING;
	play_ctx->sequence_delay = 0;

	audio_play_bgm(play_ctx->bgm);
	wipe_cmd = WIPECMD_IN;

	play_adapt_to_screen_size();
}

static bool find_assets_dir()
{
#ifdef __ANDROID__
	strcpy(config.assets_dir, "");
	return true;
#else
	if (cli.assets_dir != NULL) { //Directory set from CLI
		int len;

		process_path(cli.assets_dir, config.assets_dir, sizeof(config.assets_dir));
		len = strlen(cli.assets_dir);

		if (len <= 510) {
			config.assets_dir[len] = '/';
		}

		if (readable_dir(config.assets_dir)) {
			return true;
		}

		return false;
	} else { //Try default directories
		char base_path[480];
		char path[512];

		process_path(GetApplicationDirectory(), base_path, sizeof(base_path));

		snprintf(path, sizeof(path), "%s/%s", base_path, "assets/");
		if (readable_dir(path)) {
			snprintf(config.assets_dir, sizeof(config.assets_dir), "%s", path);

			return true;
		}

#ifndef _WIN32
		snprintf(path, sizeof(path), "%s/%s", base_path, "../share/games/alexvsbus/");
		if (readable_dir(path)) {
			snprintf(config.assets_dir, sizeof(config.assets_dir), "%s", path);

			return true;
		}
#endif //_WIN32

		return false;
	}
#endif //__ANDROID__
}

static void find_config_path()
{
#ifdef __ANDROID__
	strcpy(config_path, "alexvsbus.cfg");
#else
	char tmp[512];

	if (cli.config != NULL) {
		snprintf(tmp, sizeof(tmp), "%s", cli.config);
	} else {
#if defined(_WIN32)
		snprintf(tmp, sizeof(tmp), "%s/alexvsbus/alexvsbus.cfg", getenv("APPDATA"));
#elif defined(__APPLE__)
		snprintf(tmp, sizeof(tmp),
			"%s/Library/Preferences/alexvsbus/alexvsbus.cfg", getenv("HOME"));
#else
		const char* xdg_config = getenv("XDG_CONFIG_HOME");

		if (xdg_config == NULL) {
			snprintf(tmp, sizeof(tmp), "%s/.config/alexvsbus/alexvsbus.cfg", getenv("HOME"));
		} else {
			snprintf(tmp, sizeof(tmp), "%s/alexvsbus/alexvsbus.cfg", xdg_config);
		}
#endif
	}

	process_path(tmp, config_path, sizeof(config_path));
#endif //__ANDROID__
}

static void load_config()
{
	char tmp[48];

	//Defaults
	config.fullscreen = true;
	config.fixed_window_mode = false;
	config.window_scale = 2;
	config.scanlines_enabled = false;
	config.vscreen_auto_size = true;
	config.audio_enabled = true;
	config.sfx_enabled = true;
	config.music_enabled = true;
	config.touch_enabled = false;
	config.touch_buttons_enabled = true;
	config.show_touch_controls = false;
	config.progress_difficulty = DIFFICULTY_NORMAL;
	config.progress_level = 1;

	if (get_file_size(config_path) <= 4096 && lineread_open(config_path)) {
		while (!lineread_ended()) {
			lineread_getline(tmp);

			if (lineread_num_tokens(tmp) != 2) {
				continue;
			}

			//Only non-default values need to be checked here
			if (str_starts_with(tmp, "fullscreen ")) {
				const char* val = lineread_token(tmp, 1);

				if (val != NULL && strcmp(val, "false") == 0) {
					config.fullscreen = false;
				}
			} else if (str_starts_with(tmp, "window-scale ")) {
				int val = lineread_token_int(tmp, 1);

				if (val >= 1 && val <= 3) {
					config.window_scale = val;
				}
			} else if (str_starts_with(tmp, "scanlines-enabled ")) {
				const char* val = lineread_token(tmp, 1);

				if (val != NULL && strcmp(val, "true") == 0) {
					config.scanlines_enabled = true;
				}
			} else if (str_starts_with(tmp, "audio-enabled ")) {
				const char* val = lineread_token(tmp, 1);

				if (val != NULL && strcmp(val, "false") == 0) {
					config.audio_enabled = false;
				}
			} else if (str_starts_with(tmp, "music-enabled ")) {
				const char* val = lineread_token(tmp, 1);

				if (val != NULL && strcmp(val, "false") == 0) {
					config.music_enabled = false;
				}
			} else if (str_starts_with(tmp, "sfx-enabled ")) {
				const char* val = lineread_token(tmp, 1);

				if (val != NULL && strcmp(val, "false") == 0) {
					config.sfx_enabled = false;
				}
			} else if (str_starts_with(tmp, "touch-buttons-enabled ")) {
				const char* val = lineread_token(tmp, 1);

				if (val != NULL && strcmp(val, "false") == 0) {
					config.touch_buttons_enabled = false;
				}
			} else if (str_starts_with(tmp, "vscreen-auto-size ")) {
				const char* val = lineread_token(tmp, 1);

				if (val != NULL && strcmp(val, "false") == 0) {
					config.vscreen_auto_size = false;
				}
			} else if (str_starts_with(tmp, "vscreen-width ")) {
				int val = lineread_token_int(tmp, 1);
				int i;

				//Ensure the width in the config file is supported
				for (i = 0; data_screen_widths[i] > -1; i++) {
					if (val == data_screen_widths[i]) {
						config.vscreen_width = val;
						break;
					}
				}
			} else if (str_starts_with(tmp, "vscreen-height ")) {
				int val = lineread_token_int(tmp, 1);
				int i;

				//Ensure the height in the config file is supported
				for (i = 0; data_screen_heights[i] > -1; i++) {
					if (val == data_screen_heights[i]) {
						config.vscreen_height = val;
						break;
					}
				}
			} else if (str_starts_with(tmp, "progress-difficulty ")) {
				const char* val = lineread_token(tmp, 1);

				if (val != NULL) {
					if (strcmp(val, "hard") == 0){
						config.progress_difficulty = DIFFICULTY_HARD;
					} else if (strcmp(val, "super") == 0) {
						config.progress_difficulty = DIFFICULTY_SUPER;
					}
				}
			} else if (str_starts_with(tmp, "progress-level ")) {
				int val = lineread_token_int(tmp, 1);

				if (val >= 1 && val <= 5) {
					config.progress_level = val;
				}
			}
		}
	}

#ifndef __ANDROID__
	//Apply CLI paramers
	if (cli.touch_enabled) {
		config.touch_enabled = true;
	}
	if (cli.fullscreen) {
		config.fullscreen = true;
	}
	if (cli.windowed) {
		config.fullscreen = false;
	}
	if (cli.fixed_window_mode) {
		config.fixed_window_mode = true;
	}
	if (cli.window_scale != 0) {
		config.window_scale = cli.window_scale;
	}
	if (cli.scanlines_enabled != 0) {
		config.scanlines_enabled = (cli.scanlines_enabled == 1);
	}
	if (cli.audio_enabled != 0) {
		config.audio_enabled = (cli.audio_enabled == 1);
	}
	if (cli.music_enabled != 0) {
		config.music_enabled = (cli.music_enabled == 1);
	}
	if (cli.sfx_enabled != 0) {
		config.sfx_enabled = (cli.sfx_enabled == 1);
	}
	if (cli.touch_buttons_enabled != 0) {
		config.touch_buttons_enabled = (cli.touch_buttons_enabled == 1);
	}
	if (cli.vscreen_width != 0) {
		config.vscreen_width = cli.vscreen_width;
		config.vscreen_auto_size = (cli.vscreen_width == -1);
	}
	if (cli.vscreen_height != 0) {
		config.vscreen_height = cli.vscreen_height;
		config.vscreen_auto_size = (cli.vscreen_height == -1);
	}

	if (config.progress_level > data_difficulty_num_levels[config.progress_difficulty]) {
		config.progress_level = 1;
	}
#endif //__ANDROID__

#ifdef __ANDROID__
	//Enforce config options that are fixed on Android
	config.fullscreen = true;
	config.fixed_window_mode = true;
	config.touch_enabled = true;
#endif
}

static bool save_config()
{
	char data[2048] = "";
	char line[64] = "";

#ifndef __ANDROID__
	if (!create_parent_dirs(config_path)) {
		perror("Failed to create config directory");
		return false;
	}
#endif

	sprintf(line, "fullscreen %s\n", config.fullscreen ? "true" : "false");
	strcat(data, line);

	sprintf(line, "window-scale %d\n", config.window_scale);
	strcat(data, line);

	sprintf(line, "scanlines-enabled %s\n", config.scanlines_enabled ? "true" : "false");
	strcat(data, line);

	sprintf(line, "audio-enabled %s\n", config.audio_enabled ? "true" : "false");
	strcat(data, line);

	sprintf(line, "music-enabled %s\n", config.music_enabled ? "true" : "false");
	strcat(data, line);

	sprintf(line, "sfx-enabled %s\n", config.sfx_enabled ? "true" : "false");
	strcat(data, line);

	sprintf(line, "touch-buttons-enabled %s\n", config.touch_buttons_enabled ? "true" : "false");
	strcat(data, line);

	sprintf(line, "vscreen-auto-size %s\n", config.vscreen_auto_size ? "true" : "false");
	strcat(data, line);

	if (!config.vscreen_auto_size) {
		sprintf(line, "vscreen-width %d\n",  config.vscreen_width);
		strcat(data, line);

		sprintf(line, "vscreen-height %d\n", config.vscreen_height);
		strcat(data, line);
	}

	strcpy(line, "progress-difficulty ");
	switch (config.progress_difficulty) {
		case DIFFICULTY_NORMAL: strcat(line, "normal"); break;
		case DIFFICULTY_HARD:   strcat(line, "hard");   break;
		case DIFFICULTY_SUPER:  strcat(line, "super");  break;
	}
	strcat(data, line);

	sprintf(line, "\nprogress-level %d\n", config.progress_level);
	strcat(data, line);

	if (!SaveFileText(config_path, data)) {
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------

static void auto_size_vscreen()
{
	bool small_screen = false;
	int win_width  = display_params.win_width;
	int win_height = display_params.win_height;
	int vscreen_width  = 0;
	int vscreen_height = 0;
	int best_scaled_width  = 0;
	int best_scaled_height = 0;
	int scale = 1;
	int i, j;

	if (win_width < VSCREEN_AUTO_MIN_WIDTH) {
		small_screen = true;
	}
	if (win_height < VSCREEN_AUTO_MIN_HEIGHT) {
		small_screen = true;
	}

	//Determine the size for the virtual screen (vscreen) so that it best
	//fits in the physical screen or window
	for (i = 0; data_screen_widths[i] > -1; i++) {
		for (j = 0; data_screen_heights[j] > -1; j++) {
			int scaled_width  = 0;
			int scaled_height = 0;
			int w = data_screen_widths[i];
			int h = data_screen_heights[j];

			if (!small_screen && w < VSCREEN_AUTO_MIN_WIDTH)  continue;
			if (!small_screen && h < VSCREEN_AUTO_MIN_HEIGHT) continue;

			for (scale = 8; scale > 1; scale--) {
				if ((w * scale) <= win_width && (h * scale) <= win_height) {
					break;
				}
			}

			scaled_width  = w * scale;
			scaled_height = h * scale;

			if (scaled_width > win_width || scaled_height > win_height) {
				continue;
			}

			if (scaled_width > best_scaled_width && scaled_height > best_scaled_height) {
				best_scaled_width  = scaled_width;
				best_scaled_height = scaled_height;
				vscreen_width  = w;
				vscreen_height = h;
			}
		}
	}

	display_params.vscreen_width  = vscreen_width;
	display_params.vscreen_height = vscreen_height;
	display_params.scale = scale;
}

static void scale_manual_vscreen()
{
	int scale;

	display_params.vscreen_width  = config.vscreen_width;
	display_params.vscreen_height = config.vscreen_height;

	for (scale = 8; scale >= 1; scale--) {
		int vscreen_width_scaled  = display_params.vscreen_width  * scale;
		int vscreen_height_scaled = display_params.vscreen_height * scale;

		if (vscreen_width_scaled  > display_params.win_width)  continue;
		if (vscreen_height_scaled > display_params.win_height) continue;

		break;
	}

	if (scale < 1) scale = 1;

	display_params.scale = scale;
}

