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
 * util.c
 *
 * Description:
 * Miscellaneous utility functions
 *
 */

//------------------------------------------------------------------------------

#include <raylib.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
#include <direct.h> //_mkdir()
#else
#include <sys/wait.h> //waitpid()
#endif

//------------------------------------------------------------------------------

//From win32.c
void win32_msgbox_error(const char* msg);

//------------------------------------------------------------------------------

bool str_starts_with(const char* str, const char* start)
{
	int len = strlen(start);
	int i;

	if (len > strlen(str)) {
		return false;
	}

	for (i = 0; i < len; i++) {
		if (str[i] != start[i]) {
			return false;
		}
	}

	return true;
}

//Returns true if the given string contains only whitespaces
bool str_only_whitespaces(const char* str)
{
	int i = 0;

	while (str[i] != '\0') {
		if (!isspace(str[i])) {
			return false;
		}

		i++;
	}

	return true;
}

int get_file_size(const char* path)
{
	return GetFileLength(path);
}

//Extracts the name of a file from a full path
const char* file_from_path(const char* path)
{
	int last_slash = -1;
	int len = strlen(path);
	int i;

	for (i = 0; i < len; i++) {
		if (path[i] == '/') {
			last_slash = i;
		}
	}

	return &path[last_slash + 1];
}

//Does the following to a path string:
//
//1. If running on Windows, replaces backslashes (\) with forward slashes (/)
//2. Removes consecutive slashes
//3. Removes the slash at the end if present
void process_path(const char* in, char* out, int maxlen)
{
	int outlen = 0;
	int i = 0;

	while (in[i] != '\0') {
		bool is_slash = (in[i] == '/');

#ifdef _WIN32
		is_slash = (in[i] == '/' || in[i] == '\\');
#endif

		out[outlen] = (is_slash ? '/' : in[i]);
		outlen++;
		i++;

		if (is_slash) {
			//Skip consecutive slashes
			while (in[i] == '/') i++;

			is_slash = false;
		}

		if (outlen >= maxlen) {
			outlen = maxlen;

			break;
		}
	}

	out[outlen] = '\0';

	//Remove slash at the end if present
	if (out[outlen - 1] == '/') {
		out[outlen - 1] = '\0';
	}
}

//Checks if a directory exists and is readable
bool readable_dir(const char* path)
{
	struct stat st;

	if (stat(path, &st) != 0) {
		return false;
	}

	if (!S_ISDIR(st.st_mode)) {
		return false;
	}

	if (access(path, R_OK) != 0) {
		return false;
	}

	return true;
}

//Tries to create a single directory, without the parents
bool create_dir(const char* path)
{
	struct stat st;
	int ret;

#ifdef _WIN32
	ret = _mkdir(path);
#else
	ret = mkdir(path, 0744);
#endif

	if (ret == 0) {
		return true;
	}

	if (errno != EEXIST) {
		return false;
	}

	if (stat(path, &st) != 0) {
		return false;
	}

	if (!S_ISDIR(st.st_mode)) {
		return false;
	}

	return true;
}

//Tries to create a file's parent directories
bool create_parent_dirs(const char* path)
{
	char tmp[512];
	int len = strlen(path) + 1;
	int i;

	//Name too long
	if (len > 510) {
		return false;
	}

	//Copy the path into a mutable string
	snprintf(tmp, 512, "%s", path);

	//Ignore final slash
	if (tmp[len - 1] == '/') {
		len--;
	}

	i = 0;
#ifdef _WIN32
	//On Windows, skip drive letter
	if (len >= 2 && path[1] == ':' && path[2] == '/') i = 3;
#endif
	
	for (; i < len; i++) {
		if (tmp[i] == '/') {
			//If the path starts with a slash or is empty, skip (otherwise, the
			//path would be an empty string)
			if (i == 0) {
				continue;
			}

			tmp[i] = '\0';

			if (!create_dir(tmp)) {
				return false;
			}

			tmp[i] = '/';

			//Skip consecutive slashes
			while (tmp[i] == '/') i++;
		}
	}

	return true;
}

void msgbox_error(const char* msg)
{
#if defined(_WIN32)
	win32_msgbox_error(msg);
#elif !defined(__ANDROID__)
	pid_t pid;

	pid = fork();
	if (pid == 0) { //Child process
		execlp("zenity", "zenity", "--error", "--text", msg, NULL);
		execlp("kdialog", "kdialog", "--error", msg, NULL);
		_exit(129);
	} else if (pid > 0) { //Parent process
		int status;

		waitpid(pid, &status, 0);
	}
#endif
}

