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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

/** enum optimization_options - the possible optimizations kinds
 * @OPTIMIZE_OFF:       don't optimize
 * @OPTIMIZE_BIAS:      only fix biases
 * @OPTIMIZE_ALL:       fix biases and normalize
 */
enum optimization_options {
		OPTIMIZE_OFF = 0,
		OPTIMIZE_BIAS = 1,
		OPTIMIZE_ALL = 2
};

/** struct sound_info - stores needed information about the sound source
 * @channels:		the number of channels.
 * @samples_per_sec:    the sampling rate.
 * @bits:		the size of a sample in bits.
 * @endian:		the endianess type
 * @is_signed:		true if the number is signed, false otherwise
 *
 * This structure stores the needed infromation about a sound source
 * and represents one. (except for the actual data)
 * this consists of everything needed for sound processing.
 */
struct sound_info {
	uint16_t	channels; /* the # channels in the stream */
	uint32_t	samples_per_sec; /* #samples in a second */
	uint16_t	bits; /* the size in bits of the samples */
	enum endianess  endian; /* the endianess of the source */
	int is_signed; /* signed or unsigned */
};

unsigned int
do_mono_samples(FILE *fd, int16_t *smp, unsigned int samples, unsigned int bits, unsigned int channels, int is_signed);

void do_optimize(int16_t *smp, int samples, enum optimization_options optimize);

int
init_sound_processing(enum endianess sound_endianess);


