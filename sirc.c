#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include "ir.h"
#include "sirc.h"

#define SIRC_START_WIDTH 2400
#define SIRC_ZERO_WIDTH 600
#define SIRC_ONE_WIDTH 1200
#define SIRC_SPACE_WIDTH 600
#define SIRC_REPETITION_RATE 45000

struct sirc_priv {
	struct timeval last_time;
	int last_value;
};

void *sirc_new(void)
{
	struct sirc_priv *sirc;

	sirc = malloc(sizeof(struct sirc_priv));

	if (!sirc)
		return NULL;

	memset(sirc, 0, sizeof(struct sirc_priv));

	sirc->last_value = -1;

	return sirc;
}

void sirc_free(void *sirc_ptr)
{
	free(sirc_ptr);
}

int sirc_parse(void *sirc_ptr, struct timeval *ts, ir_event_t *queue, int queue_len, ir_code_t *code)
{
	struct sirc_priv *sirc = (struct sirc_priv*) sirc_ptr;
	int index = 0;
	int value = 0;
	int time_diff;
	int address_shift, address_width;
	int command_shift, command_width;
	int i;
	struct timeval last;

	memcpy(&last, &sirc->last_time, sizeof(struct timeval));
	memcpy(&sirc->last_time, ts, sizeof(struct timeval));

//	printf("ts = %d.%06d\n", ts->tv_sec, ts->tv_usec);

	if (queue_len < 1)
		return 0;
	
	for (i = 0; i < queue_len; i++) {
		ir_event_t *e = &queue[i];

//		printf("%d: %s %dus\n", i, e->type ? "pulse" : "space", e->length);

		if (i == 0) {
			if ((e->type != IR_EVENT_PULSE) || (e->length < SIRC_START_WIDTH * 0.75) || (queue[0].length > SIRC_START_WIDTH * 1.25)) {
//				printf("sirc: invalid first pulse\n");
				return 0;
			}

			continue;
		}

		if ((e->type == IR_EVENT_PULSE) && (e->length > SIRC_ZERO_WIDTH * 0.5) && (e->length <= SIRC_ZERO_WIDTH * 1.25)) {
//			printf("sirc: zero %dus\n", e->length);
			index++;
		} else if ((e->type == IR_EVENT_PULSE) && (e->length > SIRC_ONE_WIDTH * 0.75) && (e->length <= SIRC_ONE_WIDTH * 1.25)) {
//			printf("sirc: one %dus\n", e->length);
			value |= (1 << index);
			index++;
		} else if ((e->type == IR_EVENT_SPACE) && (e->length > SIRC_SPACE_WIDTH * 0.5) && (e->length < SIRC_SPACE_WIDTH * 1.5)) {
			// regular space, no nothing
		} else {
//			printf("sirc: invalid %s %dus\n", (e->type == IR_EVENT_PULSE) ? "pulse" : "space", e->length);
			return 0;
		}
	}

	switch (index) {
		case 12:
			address_shift = 7;
			address_width = 5;
			command_shift = 0;
			command_width = 7;
			break;
		case 15:
			address_shift = 7;
			address_width = 8;
			command_shift = 0;
			command_width = 7;
			break;
		case 20:
			address_shift = 7;
			address_width = 13;
			command_shift = 0;
			command_width = 7;
			break;
		default:
			return 0;
	}

	code->type = IR_CODE_SIRC;
	code->address = (value >> address_shift) & ((1 << address_width) - 1);
	code->command = (value >> command_shift) & ((1 << command_width) - 1);

	if (ts->tv_sec - last.tv_sec > 5) {
		time_diff = 5000000;
	} else {
		time_diff = (ts->tv_sec - last.tv_sec) * 1000000 +
			    (ts->tv_usec - last.tv_usec);
	}

//	printf("time_diff = %dus\n", time_diff);

	if (value == sirc->last_value && time_diff < SIRC_REPETITION_RATE * 2) {
		code->repeat = 1;
	} else {
		code->repeat = 0;
	}

	sirc->last_value = value;

	return 1;
}
