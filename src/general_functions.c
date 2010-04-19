/*    dtmfcmd is a program to detect dtmf tones and launch commands accordingly
 *    Copyright (C) 2009  Tom Hacohen (tom@stosb.com)
 *
 *    This file is a part of dtmfcmd   
 * 
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 2 of the License, or
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h> 

#include "general_functions.h"


#define PLAY_SOUND_CMD "aplay -D hw:0,0 "
#define SOUND_DIR "/usr/share/dtmfcmd/sounds/"

/**
 * play_sound() - plays the sound matching to the action.
 * @action:     The action occured. (enum action)
 *
 * This function plays the sound matching to the action occured.
 *
 * Returns 0 on success, -ERR on error.
 **/
int
play_sound(enum events action)
{
	char *command = NULL;
	switch (action) {
		case EVENT_IDENTIFIED:
			command = PLAY_SOUND_CMD SOUND_DIR "identify.wav";
			break;
		case EVENT_DONE:
			command = PLAY_SOUND_CMD SOUND_DIR "done.wav";
			break;
		case EVENT_FAILED:
			command = PLAY_SOUND_CMD SOUND_DIR "failed.wav";
			break;
		default:
			return -1;
			break;
	}

	return fork_exec(command);
}

/**
 * fork_exec() - forks and executes a command.
 * @command:    The command to run.
 *
 * This function forks and executes a command.
 *
 * Returns 0 on success, -ERR on error.
 **/
int
fork_exec (const char *command)
{
	pid_t pid = fork();
	
	if (pid == 0) {
		system(command);
		exit(0);
	}
	else if (pid == -1) {
		return -1;
	}
	return 0;	
}

/**
 * endian_type() - returns the system's endianess
 *
 * This function returns the system's endianess
 **/
enum endianess
endian_type (void)
{
	uint32_t i;
	unsigned char *cp;	

	i = 0x04030201;
	cp = (unsigned char *) &i;

	if (*cp == 1)
		return LITTLE_ENDIAN_TYPE;
	else if (*cp == 4)
		return BIG_ENDIAN_TYPE;
	else
		return WEIRD_ENDIAN_TYPE;
}
