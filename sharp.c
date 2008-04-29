/* old sharp protocol, addr always 0, code 000231
 * cmd=1 ch+
 * cmd=2 ch-
 * cmd=3 vol+
 * cmd=4 vol-
 * cmd=5 bright+
 * cmd=6 bright-
 * cmd=7 sat+
 * cmd=8 sat-
 * cmd=9 mute
 * cmd=14 standby
 */

/* new sharp protocol, addr 1, code 001001
 * ch+ 17
 * ch- 18
 * vol+ 20
 * vol- 21
 * standby 22
 * mute 23
 * sat+ 36
 * sat- 37
 * bright+ 41
 * bright- 42
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir.h"
#include "sharp.h"

#define SHARP_OLD_PULSE_WIDTH 500
#define SHARP_OLD_ZERO_WIDTH (2000 - (SHARP_OLD_PULSE_WIDTH))
#define SHARP_OLD_ONE_WIDTH (4000 - (SHARP_OLD_PULSE_WIDTH))
#define SHARP_OLD_REPETITION_RATE 40000

#define SHARP_OLD_BIT_COUNT 6
#define SHARP_OLD_COMMAND_SHIFT 0
#define SHARP_OLD_COMMAND_WIDTH 6 

#define SHARP_NEW_PULSE_WIDTH 320
#define SHARP_NEW_ZERO_WIDTH (1000 - (SHARP_NEW_PULSE_WIDTH))
#define SHARP_NEW_ONE_WIDTH (2000 - (SHARP_NEW_PULSE_WIDTH))
#define SHARP_NEW_REPETITION_RATE 40000

#define SHARP_NEW_BIT_COUNT 15
#define SHARP_NEW_ADDRESS_SHIFT 0
#define SHARP_NEW_ADDRESS_WIDTH 5
#define SHARP_NEW_COMMAND_SHIFT 5
#define SHARP_NEW_COMMAND_WIDTH 8
#define SHARP_NEW_EXPANSION_BIT 13
#define SHARP_NEW_CHECK_BIT 14

struct sharp_priv {
	struct timeval last_time;
	int last_value;
};

void *sharp_new(void)
{
	struct sharp_priv *sharp;

	sharp = malloc(sizeof(struct sharp_priv));

	if (!sharp)
		return NULL;

	memset(sharp, 0, sizeof(struct sharp_priv));

	sharp->last_value = -1;

	return sharp;
}

void sharp_free(void *sharp_ptr)
{
	free(sharp_ptr);
}

int sharp_parse(void *sharp_ptr, struct timeval *ts, ir_event_t *queue, int queue_len, ir_code_t *code)
{
	struct sharp_priv *sharp = (struct sharp_priv*) sharp_ptr;
	int index = 0;
	int value = 0;
	int time_diff;
	int i;
	struct timeval last;
	int type;
	int pulse_width, zero_width, one_width;
	int address_shift, address_width;
	int command_shift, command_width;
	int expansion_bit, check_bit;
	int repetition_time;

	switch (queue_len) {
		case SHARP_OLD_BIT_COUNT * 2 + 1:
			type = IR_CODE_SHARP_OLD;
			pulse_width = SHARP_OLD_PULSE_WIDTH;
			zero_width = SHARP_OLD_ZERO_WIDTH;
			one_width = SHARP_OLD_ONE_WIDTH;
			address_shift = 0;
			address_width = 0;
			command_shift = SHARP_OLD_COMMAND_SHIFT;
			command_width = SHARP_OLD_COMMAND_WIDTH;
			expansion_bit = -1;
			check_bit = -1;
			repetition_time = SHARP_OLD_REPETITION_RATE;
			break;

		case SHARP_NEW_BIT_COUNT * 2 + 1:
			type = IR_CODE_SHARP;
			pulse_width = SHARP_NEW_PULSE_WIDTH;
			zero_width = SHARP_NEW_ZERO_WIDTH;
			one_width = SHARP_NEW_ONE_WIDTH;
			address_shift = SHARP_NEW_ADDRESS_SHIFT;
			address_width = SHARP_NEW_ADDRESS_WIDTH;
			command_shift = SHARP_NEW_COMMAND_SHIFT;
			command_width = SHARP_NEW_COMMAND_WIDTH;
			expansion_bit = SHARP_NEW_EXPANSION_BIT;
			check_bit = SHARP_NEW_CHECK_BIT;
			repetition_time = SHARP_NEW_REPETITION_RATE;
			break;

		default:
			return 0;
	}

	memcpy(&last, &sharp->last_time, sizeof(struct timeval));
	memcpy(&sharp->last_time, ts, sizeof(struct timeval));

//	printf("ts = %d.%06d\n", ts->tv_sec, ts->tv_usec);

	for (i = 0; i < queue_len; i++) {
		ir_event_t *e = &queue[i];

		if (e->type == IR_EVENT_PULSE) {
			if (e->length > pulse_width * 2)
				return 0;
		} else {
			if (e->length < zero_width / 2 && e->length > one_width * 2)
				return 0;

			if (e->length > (zero_width + one_width) / 2) {
//				printf("space %dus, one\n", e->length);
				value |= 1 << index;
			} else {
//				printf("space %dus, zero\n", e->length);
			}

			index++;
		}
	}

//	printf("value = ");
//	for (i = 0; i < index; i++)
//		printf("%d", (value & (1 << i)) ? 1 : 0);
//	printf("\n");

	code->type = type;
	code->address = (value >> address_shift) & ((1 << address_width) - 1);
	code->command = (value >> command_shift) & ((1 << command_width) - 1);

	if (check_bit != -1 && (value & (1 << check_bit))) {
		code->command = ~code->command & ((1 << command_width) - 1);

		/* Normalize value for comparison with previous one */

		if (command_width != 0)
			value ^= ((1 << command_width) - 1) << command_shift;
		if (expansion_bit != -1)
			value ^= (1 << expansion_bit);
		value ^= (1 << check_bit);
	}

	if (ts->tv_sec - last.tv_sec > 5) {
		time_diff = 5000000;
	} else {
		time_diff = (ts->tv_sec - last.tv_sec) * 1000000 +
			    (ts->tv_usec - last.tv_usec);
	}

//	printf("time_diff = %dus\n", time_diff);

	if (value == sharp->last_value && time_diff < SHARP_OLD_REPETITION_RATE * 2) {
		code->repeat = 1;
	} else {
		code->repeat = 0;
	}

	sharp->last_value = value;

	return 1;
}

