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
 * audio.c
 *
 * Description:
 * Audio functions
 *
 */

//------------------------------------------------------------------------------

#include "defs.h"

#include <raylib.h>
#include <stdio.h>
#include <string.h>

//------------------------------------------------------------------------------

//From data.c
extern const char* data_sfx_files[];
extern const char* data_bgm_files[];

//------------------------------------------------------------------------------

static Config* config;

static Music bgm;
static Sound sfx[NUM_SFX];

static bool init_failed;
static bool bgm_loaded;
static bool audio_enabled;
static bool music_enabled;
static bool sfx_enabled;

//------------------------------------------------------------------------------

//Function prototypes
static void unload_bgm();

//------------------------------------------------------------------------------

void audio_init(Config* cfg)
{
	config = cfg;

	InitAudioDevice();

	if (!IsAudioDeviceReady()) {
		init_failed = true;
	}
}

void audio_load_sfx()
{
	char path[530];
	int i;

	if (init_failed) return;

	for (i = 0; i < NUM_SFX; i++) {
		snprintf(path, ARRAY_LENGTH(path), "%s%s.wav", config->assets_dir, data_sfx_files[i]);
		sfx[i] = LoadSound(path);
	}
}

void audio_stop_bgm()
{
	if (bgm_loaded) {
		StopMusicStream(bgm);
	}
}

void audio_play_bgm(int id)
{
	char path[530];

	if (init_failed) return;

	unload_bgm();

	//Try to load the BGM track in OGG format
	snprintf(path, ARRAY_LENGTH(path), "%s%s.ogg", config->assets_dir, data_bgm_files[id]);
	bgm = LoadMusicStream(path);

	if (!IsMusicReady(bgm)) {
		//Try to load the BGM track in XM format
		snprintf(path, ARRAY_LENGTH(path), "%s%s.xm", config->assets_dir, data_bgm_files[id]);
		bgm = LoadMusicStream(path);
	}

	if (IsMusicReady(bgm)) {
		bgm_loaded = true;
	}

	if (bgm_loaded && audio_enabled && music_enabled) {
		PlayMusicStream(bgm);
	}
}

void audio_stop_sfx(int id)
{
	if (init_failed) return;

	if (IsSoundReady(sfx[id])) {
		StopSound(sfx[id]);
	}
}

void audio_stop_all_sfx()
{
	int i;

	if (init_failed) return;

	for (i = 0; i < NUM_SFX; i++) {
		if (IsSoundReady(sfx[i])) {
			StopSound(sfx[i]);
		}
	}
}

void audio_play_sfx(int id)
{
	if (init_failed) return;

	if (audio_enabled && sfx_enabled && IsSoundReady(sfx[id])) {
		audio_stop_sfx(id);
		PlaySound(sfx[id]);
	}
}

void audio_update()
{
	UpdateMusicStream(bgm);
}

void audio_handle_toggling()
{
	bool audio_toggled = false;
	bool music_toggled = false;
	bool sfx_toggled   = false;

	if (init_failed) return;

	if (audio_enabled != config->audio_enabled) {
		audio_enabled = config->audio_enabled;
		audio_toggled = true;
    }
	if (music_enabled != config->music_enabled) {
		music_enabled = config->music_enabled;
		music_toggled = true;
    }
	if (sfx_enabled != config->sfx_enabled) {
		sfx_enabled = config->sfx_enabled;
		sfx_toggled = true;
	}

	if (audio_toggled || music_toggled) {
		if (!audio_enabled || !music_enabled) {
			audio_stop_bgm();
		} else {
			PlayMusicStream(bgm);
		}
	}
	if (audio_toggled || sfx_toggled) {
		if (!audio_enabled || !sfx_enabled) {
			audio_stop_all_sfx();
		}
	}
}

void audio_cleanup()
{
	int i;

	if (init_failed) return;

	unload_bgm();

	for (i = 0; i < NUM_SFX; i++) {
		if (IsSoundReady(sfx[i])) {
			StopSound(sfx[i]);
			UnloadSound(sfx[i]);
		}
	}

	CloseAudioDevice();
}

//------------------------------------------------------------------------------

static void unload_bgm()
{
	if (bgm_loaded) {
		StopMusicStream(bgm);
		UnloadMusicStream(bgm);
		bgm_loaded = false;
	}
}

