/* see dsp.c for copyright and license info */

#define DSP_DIGITMODE_NOQUELCH		(1 << 8)		/*!< Do not quelch DTMF from in-band */
#define DSP_DIGITMODE_RELAXDTMF		(1 << 11)		/*!< "Radio" mode (relaxed DTMF) */

#define	MAX_DTMF_DIGITS		20

/* Basic DTMF specs:
 *
 * Minimum tone on = 40ms
 * Minimum tone off = 50ms
 * Maximum digit rate = 10 per second
 * Normal twist <= 8dB accepted
 * Reverse twist <= 4dB accepted
 * S/N >= 15dB will detect OK
 * Attenuation <= 26dB will detect OK
 * Frequency tolerance +- 1.5% will detect, +-3.5% will reject
 */

/* #define DTMF_THRESHOLD		8.0e7 */
#define DTMF_THRESHOLD	  800000000.0 /*  aluigi work-around */
#define DTMF_NORMAL_TWIST	6.3	 /* 8dB */
#ifdef	RADIO_RELAX
	#define DTMF_REVERSE_TWIST  ((digitmode & DSP_DIGITMODE_RELAXDTMF) ? 6.5 : 2.5)	 /* 4dB normal */
#else
	#define DTMF_REVERSE_TWIST  ((digitmode & DSP_DIGITMODE_RELAXDTMF) ? 4.0 : 2.5)	 /* 4dB normal */
#endif

#define DTMF_RELATIVE_PEAK_ROW	6.3	 /* 8dB */
#define DTMF_RELATIVE_PEAK_COL	6.3	 /* 8dB */
#define DTMF_2ND_HARMONIC_ROW	   ((digitmode & DSP_DIGITMODE_RELAXDTMF) ? 1.7 : 2.5)	 /* 4dB normal */
#define DTMF_2ND_HARMONIC_COL	63.1	/* 18dB */
#define DTMF_TO_TOTAL_ENERGY	42.0

typedef struct {
	int v2;
	int v3;
	int chunky;
	int fac;
	int samples;
} goertzel_state_t;

typedef struct {
	int value;
	int power;
} goertzel_result_t;

typedef struct
{
	goertzel_state_t row_out[4];
	goertzel_state_t col_out[4];
	int lasthit;
	int current_hit;
	float energy;
	int current_sample;
} dtmf_detect_state_t;

typedef struct
{
	char digits[MAX_DTMF_DIGITS + 1];
	int current_digits;
	int detected_digits;
	int lost_digits;

	union {
		dtmf_detect_state_t dtmf;
	} td;
} digit_detect_state_t;

int
dtmf_detect(digit_detect_state_t *s, int16_t amp[], int samples, int digitmode);

void
ast_digit_detect_init(digit_detect_state_t *s, int mf);