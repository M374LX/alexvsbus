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
 * win32.c
 *
 * Description:
 * Some Windows-specific functions
 *
 */

//------------------------------------------------------------------------------

#ifdef _WIN32
#include "defs.h"
#include <string.h>
#include <windows.h>
#endif

//------------------------------------------------------------------------------

//Displays a message box, but only on Windows
void win32_msgbox(const char* msg)
{
#ifdef _WIN32
	MessageBox(NULL, msg, GAME_TITLE, MB_OK);
#endif
}

//Displays a message box with an error icon, but only on Windows
void win32_msgbox_error(const char* msg)
{
#ifdef _WIN32
	MessageBox(NULL, msg, GAME_TITLE, MB_ICONERROR | MB_OK);
#endif
}

//Displays a message on a new console window, but only on Windows
void win32_console_msg(const char* msg)
{
#ifdef _WIN32
	HANDLE handle;
	DWORD written;
	
	AllocConsole();
	handle = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsole(handle, msg, strlen(msg), &written, NULL);
	WriteConsole(handle, "\n", 1, &written, NULL);
	system("pause");
#endif
}

