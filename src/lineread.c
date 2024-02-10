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
 * lineread.c
 *
 * Description:
 * Functions to read each line of config and level files
 *
 */

//------------------------------------------------------------------------------

#include "defs.h"

#include <raylib.h>
#include <stdlib.h>
#include <string.h>

//------------------------------------------------------------------------------

static char* data;
static int offset;
static int num_lines_read;
static bool data_ended;
static bool invalid;

//------------------------------------------------------------------------------

//Function prototypes
static void trim_spaces(char* str);
static void end_data(char* dst);

//------------------------------------------------------------------------------

bool lineread_open(const char* path)
{
	offset = 0;
	num_lines_read = 0;
	data_ended = false;
	invalid = false;

	data = LoadFileText(path);

	if (data == NULL) {
		return false;
	}

	return true;
}

bool lineread_invalid()
{
	return invalid;
}

bool lineread_ended()
{
	return data_ended;
}

void lineread_getline(char* dst)
{
	int len;
	int i;

	if (data_ended) {
		dst[0] = '\0';

		return;
	}

	//Error: too many lines in the file
	if (num_lines_read >= 255) {
		invalid = true;
		end_data(dst);

		return;
	}

	dst[0] = '\0';
	i = 0;
	while (1) {
		unsigned char c = data[offset];
		offset++;

		if (c == '\n') {
			dst[i] = '\0';
			break;
		} else if (c == '\0') {
			dst[i] = '\0';
			end_data(dst);
			break;
		}

		dst[i] = c;

		i++;

		//Line too long
		if (i == 34) {
			dst[34] = '\0';
			break;
		}
	}

	len = strlen(dst);

	//Error: line too long
	if (len > 32) {
		invalid = true;
		end_data(dst);

		return;
	}

	//Check if the line contains any invalid character
	for (i = 0; i < len; i++) {
		char c = dst[i];

		if (c == ' ' || c == '\t') continue; //Whitespace
		if (c >= '0' && c <=  '9') continue; //Digit
		if (c >= 'A' && c <=  'Z') continue; //Alphabetic (upper case)
		if (c >= 'a' && c <=  'z') continue; //Alphabetic (lower case)
		if (c == '-')  continue; //Hyphen

		invalid = true;
		end_data(dst);

		return;
	}

	trim_spaces(dst);

	//Convert to lowercase
	len = strlen(dst);
	for (i = 0; i < len; i++) {
		if (dst[i] >= 'A' && dst[i] <= 'Z') {
			dst[i] += ('a' - 'A');
		}
	}

	num_lines_read++;
}

//Returns the number of tokens
int lineread_num_tokens(const char* str)
{
	int num_spaces = 0;
	int i = 0;

	//Empty string
	if (str[0] == '\0') {
		return 0;
	}

	while (str[i] != '\0') {
		if (str[i] == ' ') {
			num_spaces++;
		}

		i++;
	}

	return num_spaces + 1;
}

//Returns a pointer to the start of a given token
const char* lineread_token(const char* str, int token)
{
	int cur_token = 0;
	int i = 0;

	if (str[0] == '\0') { //Empty string
		return NULL;
	}

	while (str[i] != '\0') {
		if (str[i] == ' ') {
			cur_token++;
		} else if (cur_token == token) {
			return &str[i];
		}

		i++;
	}

	return NULL;
}

//Returns a given token converted to an integer
int lineread_token_int(const char* str, int token)
{
	const char* token_ptr = lineread_token(str, token);
	int token_len = 0;

	if (token_ptr == NULL) {
		return NONE;
	}

	//Determine number of characters in the token
	while (1) {
		char c = token_ptr[token_len];

		if (c < '0' || c > '9') {
			break;
		}

		token_len++;
	}

	//Five digits are allowed at most
	if (token_len == 0 || token_len > 5) {
		return NONE;
	}

	//An integer token is valid only if followed by the end of the line or a
	//space
	if (token_ptr[token_len] != '\0' && token_len[token_ptr] != ' ') {
		return NONE;
	}

	return atoi(token_ptr);
}

//------------------------------------------------------------------------------

//Removes the spaces at the start and end of the string and keeps only one space
//between tokens
static void trim_spaces(char* str)
{
	int i, j;
	int len = strlen(str);

	j = 0;
	if (str[0] != ' ' && str[0] != '\t') {
		j++;
	}

	for (i = 1; i < len; i++) {
		char c1 = str[i - 1];
		char c2 = str[i];

		if (c1 == '\t') c1 = ' ';
		if (c2 == '\t') c2 = ' ';

		if (c2 != ' ') {
			if (c1 == ' ' && j > 0) {
				str[j] = ' ';
				j++;
			}
			str[j] = c2;
			j++;
		}
	}

	str[j] = '\0';
}

static void end_data(char* dst)
{
	UnloadFileText(data);
	data_ended = true;
	dst[0] = '\0';
}

