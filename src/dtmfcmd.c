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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <getopt.h>
#include <ctype.h>

#include <sys/signal.h>
#include <wait.h>

#include "general_functions.h"
#include "control.h"
#include "sound_info.h"
#include "dsp.h"


#define DEFAULT_CONFIG_FILE "/etc/dtmfcmd.conf"

static int register_signal_handling(void);
static void usage(const char *appname);

static volatile short run_loop = 1; /* can be changed by the signal handler */

int
main(int argc, char *argv[]) 
{
	/* sampling vars */
	digit_detect_state_t dtmf; 
	FILE	*fd = stdin;
	unsigned int samples = 0;
	int16_t	*smp;
	unsigned int samples_per_chunk = 0;
	struct sound_info source_info;
	enum optimization_options optimize_options = OPTIMIZE_ALL;
	unsigned int chunck_size = 8000; /* the size of a sampled chunck
				 * 8000 is exactly 1 sec, probably should
				 * calc it from the other parameters.
				 */ 
	
	/* general program parameters */
	char *conf_file = NULL;
	short sound_replies = 1;
		
	/* aux vars */
	rules_list *rules;
	char command_buffer[MAX_COMMAND_LENGTH + 1]; /* the command buffer */
	int digit_index = 0; /* the next index to be written in the command buffer */
	short opt_char;
	int i=0;
	int tmp_ret=0;
	

	/* init the other signal handlers */
	if (register_signal_handling()) {
		fprintf(stderr, "Failed to init signal handling %s:%d\n", 
				__FILE__, __LINE__);
		exit(3);
	}
	
	/* set to unbuffered mode */
	setbuf(stdin,  NULL);
	setbuf(stdout, NULL);

	if(argc < 2) {
		usage(argv[0]);
		exit(1);
	}
	memset(&source_info, 0, sizeof(struct sound_info));
	
	opterr = 0;
	while (-1 != (opt_char = getopt(argc, argv, "r:C:c:s:f:o:n"))) {
		switch (opt_char) {
			case 'r':
				source_info.samples_per_sec = atoi(optarg);
				break;
			case 'c':
				source_info.channels = atoi(optarg);
				break;
			case 'f':
				{
				int bits_per_sample;
				char endian;
				char e;
				char sign;
				sscanf(optarg, "%c%d_%c%c",
							&sign,
							&bits_per_sample,
							&endian,
							&e);

				source_info.bits = bits_per_sample;
				if (bits_per_sample != 8 && tolower(e) != 'e') {
					printf("audio type must be of the form s16_le\n");
					usage(argv[0]);
					exit(1);	
				}
				switch (tolower(endian)) {
					case 'b':
						source_info.endian = BIG_ENDIAN_TYPE;
						break;
					case 'l':
						source_info.endian = LITTLE_ENDIAN_TYPE;
						break;
					default:
						/* 8bit doesn't require endianess */
						if (bits_per_sample == 8)
							break;
						
						printf("unknown endianess type!\n");
						usage(argv[0]);
						exit(1);
						break;
				}
				
				switch (tolower(sign)) {
					case 's':
						source_info.is_signed = 1;
						break;
					case 'u':
						source_info.is_signed = 0;
						break;
					default:			
						printf("unknown sign type!\n");
						usage(argv[0]);
						exit(1);
						break;
				}
				}		
				break;
			case 'o':
				optimize_options = atoi(optarg);
				if (optimize_options > 2) {
					usage(argv[0]);
					fprintf(stderr,"Optimization values must be in 0..2\n");
					exit(1);
				}
				break;
			case 's':
				chunck_size = atoi(optarg);
				break;
			case 'C':
				conf_file = malloc(strlen(optarg)+1);
				if (conf_file == NULL) {
					fprintf(stderr, "Failed to malloc() %s:%d\n",__FILE__, __LINE__);
					exit(1);
				}
				strcpy(conf_file, optarg);
				break;
			case 'n':
				sound_replies = 0;
				break;
			case '?':
				if (isprint (optopt))
					fprintf (stderr, "Unknown/requires an arg option `-%c'.\n",
						optopt);
				else
					fprintf (stderr,
						"Unknown option character `\\x%x'.\n",
						optopt);

				exit (1);
				break;
			default:
				fprintf (stderr,
						"Getopt returned '\\x%x', exiting.\n",
						opt_char);

				exit (1);
				break;
		}
	}
	
	
	/* verify everything is set */
	if (!source_info.channels || !source_info.samples_per_sec || !source_info.bits) {
		usage(argv[0]);
		exit(1);
	}		
		
	if (optind < argc) {
		printf("Using %s\n", argv[optind]);
		fd = fopen(argv[optind], "rb");
		if(!fd) {
			perror("Error, ");
			exit(1);
		}
	}
	
	if (conf_file == NULL) {
		conf_file = malloc(strlen(DEFAULT_CONFIG_FILE) + 1);
		if (conf_file == NULL) {
			fprintf(stderr, "Failed to malloc() %s:%d\n",__FILE__, __LINE__);
			exit(1);
		}
		strcpy (conf_file, DEFAULT_CONFIG_FILE);
	}
	
	rules_list_init(&rules);
	
	load_rules(conf_file, rules);
	

	if (init_sound_processing(source_info.endian)) {
		printf("Error getting system endianess, exiting.\n");
		exit(1);
	}
	
	fprintf(stderr,
		"  chunck size:\t %u\n"
		"  channels:\t %hu\n"
		"  samples/sec:\t %u\n"
		"  endianess:\t %s\n"
		"  sign:\t %s\n"
		"  bits:\t %hu\n",
		chunck_size,
		source_info.channels,
		source_info.samples_per_sec,
		(source_info.endian == LITTLE_ENDIAN_TYPE) ? "little" : "big",
		(source_info.is_signed) ? "signed":"unsigned",
		source_info.bits);

	if(chunck_size <= 0) {
		fprintf(stderr,"chunk size must be positive!");
		exit (1);
	}

	samples_per_chunk = (chunck_size / (source_info.bits / 8));
	/* round it according to channels */
	samples_per_chunk += source_info.channels - (samples_per_chunk % source_info.channels);
	smp = malloc(samples_per_chunk * sizeof(int16_t));
	if (smp == NULL) {
		fprintf(stderr, "Failed to malloc() %s:%d\n",__FILE__, __LINE__);
		exit(1);
	}

	ast_digit_detect_init(&dtmf, 1);
	digit_index = 0;
	while (run_loop && !feof(fd)) {
		samples = do_mono_samples(fd, smp, samples_per_chunk, source_info.bits, source_info.channels, source_info.is_signed);
		
		if (samples == 0) /*error*/
			break;
			 
		do_optimize(smp, samples, optimize_options);
		

		/* INIT dtmf detection and detect */		ast_digit_detect_init(&dtmf, 0);
		dtmf_detect(&dtmf, smp, samples, DSP_DIGITMODE_NOQUELCH);

		for (i = 0; i<dtmf.detected_digits && run_loop; i++) {
			switch (dtmf.digits[i]) {
				case '#':/* command end */
					command_buffer[digit_index] = '\0';
					tmp_ret = launch_command(rules, command_buffer);
					/*FIXME:  settings are ugly and hackish */
					if (tmp_ret == -2) {
						fprintf(stderr, "\nAccess Denied\n");
						if (sound_replies)
							play_sound(EVENT_FAILED);
					}
					else if (tmp_ret == RULE_TYPE_IDENTIFY) {
						fprintf(stderr, "\nIdentified to level %d\n",
						       get_access_level());
						if (sound_replies)
							play_sound(EVENT_IDENTIFIED);
					}
					else if (tmp_ret == RULE_TYPE_EXIT) {
						fprintf(stderr, "\nGot exit code, exiting.\n");
						run_loop = 0;
					}
				else if (!tmp_ret) {
						/* launched */
						if (sound_replies)
							play_sound(EVENT_DONE);
					}
					else {
						/* error */
						fprintf(stderr, "\nFailed launching.\n");
						if (sound_replies)
							play_sound(EVENT_FAILED);
					}
					digit_index = 0;
					break;
				case '*':
					digit_index = 0;
					break;
				default: /* assume it's only numbers */
					if (digit_index > MAX_COMMAND_LENGTH) {
						/* command is to long */
						fprintf(stderr, "Command is too long "
								"reseting command, unexpected results "
								"may occour, use '*' to clear.\n");
						digit_index = 0;
					}	
					else {
						command_buffer[digit_index] = dtmf.digits[i];
						digit_index++;
					}
					break;
			}
			fputc(dtmf.digits[i], stdout);
		}
	}

	free (smp);
	
	if(fd != stdin)
			fclose(fd);

	free(conf_file);

	rules_list_free(rules);
	return 0;
}

static void
usage(const char *appname) 
{
	printf(	"Usage: %s -r sampling rate -c #channels -f format [OPTIONS] [FILE]...\n"
		"Detect DTMF tones and launch applications by them.\n"
	        "\n"
		"Options:\n"
		"  -o\t value\t choose the optimization type:\n"
		"\t %d no, %d bias only %d all, default is all\n"
		"  -s\t The sampling chunk size, don't make it too small touch only\n"
		"\t if you know what your'e doing.\n"
		, appname, OPTIMIZE_OFF, OPTIMIZE_BIAS, OPTIMIZE_ALL);
	printf(
		"  -C\t a configuration file to use (default is: %s)\n"
		"  -n\t choose if you don't want sound replies to the user\n"
       		"  -r\t sampling rrate n khz \n"
	        "  -c\t number of channels\n"
	       	"  -f\t format\n"
		"\t Recognized sample formats are: S8 U8 S16_LE S16_BE U16_LE\n"
		"\t U16_BE S24_LE S24_BE U24_LE U24_BE S32_LE S32_BE U32_LE U32_BE\n"
	        "\n"
	        "With no FILE, input is stdin.\n"
	        "\n"
	        "Copyright (C) 2009 Tom Hacohen (tom@stosb.com)\n"
		, DEFAULT_CONFIG_FILE);
}

/*****************************
 * signal handling functions *
 *****************************/
static void
zombie_reaper (int signal)
{
	(void) signal;
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

static void
stop_loop (int signal)
{
	(void) signal;
	run_loop = 0;
}

/**
 * register_signal_handling() - registers all the signal handlers
 *
 * This function registers the signal handlers for the system signals.
 *
 * Returns 0 on success, -ERR on error.
 **/
static int
register_signal_handling(void)
{
	struct sigaction signal_handling;
	/* init the other signal handlers */
	/* init the zombie reaper */
	signal_handling.sa_handler = zombie_reaper;
	sigemptyset(&signal_handling.sa_mask);
	signal_handling.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &signal_handling, NULL) != 0) {
		perror("sigaction");
		return -1;
	}
	
	signal_handling.sa_handler = stop_loop;
	sigemptyset(&signal_handling.sa_mask);

	if (sigaction(SIGTERM, &signal_handling, NULL) != 0) {
		perror("sigaction");
		return -1;
	}
	
	return 0;
	
}

