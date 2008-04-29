#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir.h"
#include "rc5.h"

#define RC5_BIT_WIDTH 1778
#define RC5_REPETITION_RATE 113792

#define RC5_BIT_COUNT 14
#define RC5_START_BIT 13
#define RC5_EXTENDED_BIT 12
#define RC5_TOGGLE_BIT 11
#define RC5_ADDRESS_SHIFT 6
#define RC5_ADDRESS_WIDTH 5
#define RC5_COMMAND_SHIFT 0
#define RC5_COMMAND_WIDTH 6
#define RC5_COMMAND_EXTENDED 0x0040

struct rc5_priv {
	int toggle;
};

int rc5_parse(void *rc5_ptr, struct timeval *ts, ir_event_t *queue, int queue_len, ir_code_t *code)
{
	struct rc5_priv *rc5 = (struct rc5_priv*) rc5_ptr;
	int i;
	int value = 0;
	int index = 0;
	int half = 1;
	int toggle;

//	printf("ts = %d.%06d\n", ts->tv_sec, ts->tv_usec);

	for (i = 0; i < queue_len; i++) {
		ir_event_t *e = &queue[i];

		if ((e->length > RC5_BIT_WIDTH * 0.25) && (e->length < RC5_BIT_WIDTH * 0.75)) {
			if (index == 0 && half == 0)
				return 0;

			if (half) {
				if (e->type == IR_EVENT_PULSE)
					value |= 1 << (RC5_BIT_COUNT - index - 1);
				index++;
			}
			half = !half;
		} else if ((e->length >= RC5_BIT_WIDTH * 0.75) && (e->length < RC5_BIT_WIDTH * 1.25)) {
			if (index == 0 || !half)
				return 0;
	
			if (e->type == IR_EVENT_PULSE)
				value |= 1 << (RC5_BIT_COUNT - index - 1);
	
			index++;
		} else {
			return 0;
		}
	}

	if (((index != RC5_BIT_COUNT || half) && (index != RC5_BIT_COUNT - 1 || !half))) {
//		printf("rc5: invalid length\n");
		return 0;
	}

	/* Verify the start bit. */

	if (!(value & (1 << RC5_START_BIT)))
		return 0;

	toggle = value & (1 << RC5_TOGGLE_BIT);

	code->type = IR_CODE_RC5;
	code->address = (value >> RC5_ADDRESS_SHIFT) & ((1 << RC5_ADDRESS_WIDTH) - 1);
	code->command = (value >> RC5_COMMAND_SHIFT) & ((1 << RC5_COMMAND_WIDTH) - 1);
	if (!(value & (1 << RC5_EXTENDED_BIT)))
		code->command |= RC5_COMMAND_EXTENDED;

	code->repeat = (toggle == rc5->toggle);

	if (toggle != rc5->toggle)
		rc5->toggle = toggle;

	return 1;
}

void *rc5_new(void)
{
	struct rc5_priv *rc5;

	rc5 = malloc(sizeof(struct rc5_priv));

	if (!rc5)
		return NULL;

	memset(rc5, 0, sizeof(struct rc5_priv));

	/* Initialize to some arbitrary value different than 0 and
	 * (1 << RC5_TOGGLE_BIT) so it won't match. */

	rc5->toggle = -1;

	return rc5;
}

void rc5_free(void *rc5_ptr)
{
	free(rc5_ptr);
}


