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

/** enum events - the event code
 * @EVENT_IDENTIFIED:    idnetified event
 * @EVENT_DONE:		issued successfully event
 * @EVENT_FAILED:	faild launching event
 */
enum events {
	EVENT_IDENTIFIED,
	EVENT_DONE,
	EVENT_FAILED
};

/** enum endianess - represents endianess type
 * @LITTLE_ENDIAN_TYPE: little endian
 * @BIG_ENDIAN_TYPE:    big endian 
 * @WEIRD_ENDIAN_TYPE:  unknown endianess type
 */
enum endianess {
	LITTLE_ENDIAN_TYPE,
	BIG_ENDIAN_TYPE,
	WEIRD_ENDIAN_TYPE
};

int
play_sound(enum events action);

int
fork_exec (const char *command);

enum endianess
endian_type (void);