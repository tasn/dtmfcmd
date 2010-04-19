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


#include <string.h>
#include <stdint.h>

#include "general_functions.h"
#include "sound_info.h"	

/* a bit hackish, this function makes a sign number out
 * of a data chunck pointed by &x */
#define MAKE_SIGNED(bits, x) *((int ##bits## _t *) &x)
#define US_SIGNED_DIFF 32768


static int fread16_same_endianess(FILE *fd, uint16_t *num);
static int fread24_same_endianess(FILE *fd, uint32_t *num);
static int fread32_same_endianess(FILE *fd, uint32_t *num);

/* the function to be used, they are set according to the endianess
 * when init_sound_processing is invoked, they can either be the
 * same endianess versions, or the opposite endianess.
 *
 * this way is better than in function checks because it's more efficient
 */
static int (*fread16)(FILE *, uint16_t*) = fread16_same_endianess;
static int (*fread24)(FILE *, uint32_t*) = fread24_same_endianess;
static int (*fread32)(FILE *, uint32_t*) = fread32_same_endianess;

/* all the following static functions (fread($1)_($2)_endianess) 
 * read $1 bits off the file descriptor passed as fd
 * and put them inside the second parameter as unsigned ints of the $1 size
 *
 * $2 means the file endianess, same means it's the same as the system,
 * switch means it's the opposite.
 */
/* 8bit */
static int
fread8(FILE *fd, uint8_t *num) 
{
	short tmp;
	if((tmp = fgetc(fd)) == EOF)  
		return -1;
	*num = (uint8_t) tmp;
	return 0;
}

/*16 bit*/
static int
fread16_same_endianess(FILE *fd, uint16_t *num)
{
	if (fread(num, 1, 2, fd) < 1)
		return -1;
	else
		return 0;
}

static int
fread16_switch_endianess(FILE *fd, uint16_t *num)
{
	uint8_t tmp[2];
	if (fread(tmp, 1, 2, fd) < 1) {
		return -1;
	}
	else {
		((uint8_t *) num)[0] = tmp[1];
		((uint8_t *) num)[1] = tmp[0];
		return 0;
	}
}

/*24 bit*/
static int
fread24_same_endianess(FILE *fd, uint32_t *num)
{
	if (fread(num, 1, 3, fd) < 1)
		return -1;
	else
		((uint8_t *) num)[3] = 0;
		return 0;
}

static int
fread24_switch_endianess(FILE *fd, uint32_t *num)
{
	uint8_t tmp[4];
	if (fread(tmp, 1, 3, fd) < 1) {
		return -1;
	}
	else {
		((uint8_t *) num)[0] = tmp[3];
		((uint8_t *) num)[1] = tmp[2];
		((uint8_t *) num)[2] = tmp[1];
		((uint8_t *) num)[3] = 0;
		return 0;
	}
}

/*32 bit*/
static int
fread32_same_endianess(FILE *fd, uint32_t *num)
{
	if (fread(num, 1, 4, fd) < 1)
		return -1;
	else
		return 0;
}

static int
fread32_switch_endianess(FILE *fd, uint32_t *num)
{
	uint8_t tmp[4];
	if (fread(tmp, 1, 4, fd) < 1) {
		return -1;
	}
	else {
		((uint8_t *) num)[0] = tmp[3];
		((uint8_t *) num)[1] = tmp[2];
		((uint8_t *) num)[2] = tmp[1];
		((uint8_t *) num)[3] = tmp[0];
		return 0;
	}
}

/**
 * init_sound_processing() - must be called before starting to process sound
 * @sound_endianess: the endianess of the sound source, assumed correct
 *
 * This function initates the fread* functions according to the endianess.
 */
int
init_sound_processing(enum endianess sound_endianess)
{
	enum endianess endian = endian_type();
	/* check if it's an unknown endianess */
	if (endian != LITTLE_ENDIAN_TYPE && endian != BIG_ENDIAN_TYPE) 	{
		return -1;
	}

	if (endian == sound_endianess) {
		fread16 = fread16_same_endianess;
		fread24 = fread24_same_endianess;
		fread32 = fread32_same_endianess;
	}
	else {
		fread16 = fread16_switch_endianess;
		fread24 = fread24_switch_endianess;
		fread32 = fread32_switch_endianess;
	}
	return 0;
}

/**
 * do_mono_samples() - function samples the sound from fd and puts it in smp
 * @fd: 		the open fd
 * @smp: 		the holder for the samples
 * @samples:		the number of samples to take
 * @bits:		the sounds bits (8/16/24/32)
 * @channels:		the number of channels
 * @is_signed:		the 
 *
 * Samples the sound from the file descriptor fd and puts the sampled
 * sound in the array smp.
 *
 * Returns the number of samples actually got. -ERR on error
 */
unsigned int
do_mono_samples(FILE *fd, int16_t *smp, unsigned int samples, unsigned int bits, unsigned int channels, int is_signed)
{
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int ret_samples = 0;
	int32_t accumulator = 0 ;
	/*FIXME: signed/unsigned int32_t may cause int overflows? */
	
	/* turn to mono by averaging the different channels */

	switch (bits) {
		case 8:
			{
			uint8_t tmp8;
			for(i = 0; i < samples; i++) {
				accumulator = 0;
				for (j=0; (j+i < samples) && (j < channels) ; j++) {
					if(fread8(fd, &tmp8) < 0) {
						/* assure j != 0 */
						j = (j) ? j : 1;
						break;
					}
					/*upsample*/
					if (is_signed) {
						accumulator += MAKE_SIGNED(8, tmp8) << 8;
					}
					else {
						accumulator += (tmp8 << 8) - US_SIGNED_DIFF;
					}
				}
				/* j is assured by the loop to be different than 0 */
				smp[ret_samples] = (int16_t) (accumulator / j);	
				i += j;
				ret_samples++;
			}
			}
			break;
		case 16:
			{
			uint16_t  tmp16;
			for(i = 0; i < samples; i++) {
				accumulator = 0;
				for (j=0; (j+i < samples) && (j < channels) ; j++) {
					if (fread16(fd, &tmp16) < 0) {
						/* assure j != 0 */
						j = (j) ? j : 1;
						break;
					}
					/*upsample*/
					if (is_signed) {
						accumulator += MAKE_SIGNED(16, tmp16);
					}
					else {
						accumulator += tmp16 - US_SIGNED_DIFF;
					}
				}
				/* j is assured by the loop to be different than 0 */
				smp[ret_samples] = (int16_t) (accumulator / j);
				i += j;
				ret_samples++;
			}
			}
			break; 
		case 24:
			{
			uint32_t  tmp32;
			for(i = 0; i < samples; i++) {
				accumulator = 0;
				for (j=0; (j+i < samples) && (j < channels) ; j++) {
					if (fread24(fd, &tmp32) < 0) {
						/* assure j != 0 */
						j = (j) ? j : 1;
						break;
					}
					/*upsample*/
					if (is_signed) {
						accumulator += MAKE_SIGNED(32, tmp32) >> 8;
					}
					else {
						accumulator += (tmp32 >> 8) - US_SIGNED_DIFF;
					}
				}
				/* j is assured by the loop to be different than 0 */
				smp[ret_samples] = (int16_t) (accumulator / j);
				i += j;
				ret_samples++;
			}
			}
			break; 
		case 32:
			{
			uint32_t  tmp32;
			for(i = 0; i < samples; i++) {
				accumulator = 0;
				for (j=0; (j+i < samples) && (j < channels) ; j++) {
					if (fread32(fd, &tmp32) < 0) {
						/* assure j != 0 */
						j = (j) ? j : 1;
						break;
					}
					/*upsample*/
					if (is_signed) {
						accumulator += MAKE_SIGNED(32, tmp32) >> 16;
					}
					else {
						accumulator += (tmp32 >> 16) - US_SIGNED_DIFF;
					}
				}
				/* j is assured by the loop to be different than 0 */
				smp[ret_samples] = (int16_t) (accumulator / j);
				i += j;
				ret_samples++;
			}
			}
			break; 
		default:
			fprintf(stderr,"number of bits used is not supported\n");
			return 0;
			break;
	}

	return ret_samples;
}


/**
 * do_optimize() - optimizes the sound (normalize and fix biases)
 * @smp: 		the holder for the samples
 * @samples:		the number of samples to take
 * @optimize:		the optimization type as stated in the enum.
 *
 * optimizes the sound samples in smp, by normalizing/fixing biases.
 * it optimizes according to the value of optimize, which is an enum
 * that explains itself.
 */

void
do_optimize(int16_t *smp, int samples, enum optimization_options optimize) 
{
	int	 i;
	int16_t	 bias,
		norm_bias,
		maxneg,
		maxpos;

	if (optimize == OPTIMIZE_OFF) 
		return;
	
	maxneg = 32767;
	maxpos = -32768;
	for(i = 0; i < samples; i++) {
		if(smp[i] < maxneg) {
			maxneg = smp[i];
		} else if(smp[i] > maxpos) {
			maxpos = smp[i];
		}
	}

	bias = (maxneg + maxpos) / 2;

	if (optimize == OPTIMIZE_ALL) {
		
	 	float tmp_factor = 0;

		/* include bias difference */
		maxneg -= bias;
		maxpos -= bias;

		if(maxneg < 0) 
			maxneg = (-maxneg) - 1;
	
		if(maxneg > maxpos) {
			norm_bias = maxneg;
		} 
		else {
			norm_bias = maxpos;
		}
	
		if ((norm_bias == 32767) || (norm_bias == 0)) return;

		tmp_factor = (32767.0 / norm_bias);
		 for(i = 0; i < samples; i++) {
			/*was smp[i] = (smp[i] * 32767) / bias; which probably gets overflowed */
			smp[i] = (smp[i] - bias) * tmp_factor;
		}
	}
	else if (optimize == OPTIMIZE_BIAS) {
		for(i = 0; i < samples; i++) {
			/*was smp[i] = (smp[i] * 32767) / bias; which probably gets overflowed */
			smp[i] = smp[i] - bias;
		}	
	}
}	
