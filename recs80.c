#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ir.h"
#include "recs80.h"

#define RECS80_PULSE_WIDTH 158
#define RECS80_ZERO_WIDTH (5060 - (RECS80_PULSE_WIDTH))
#define RECS80_ONE_WIDTH (7590 - (RECS80_PULSE_WIDTH))
#define RECS80_REPETITION_RATE 121500

#define RECS80_BIT_COUNT 11
#define RECS80_START_BIT 10
#define RECS80_TOGGLE_BIT 9
#define RECS80_ADDRESS_SHIFT 6
#define RECS80_ADDRESS_WIDTH 3
#define RECS80_COMMAND_SHIFT 0
#define RECS80_COMMAND_WIDTH 6

struct recs80_priv {
	int toggle;
};

void *recs80_new(void)
{
	struct recs80_priv *recs80;

	recs80 = malloc(sizeof(struct recs80_priv));

	if (!recs80)
		return NULL;

	memset(recs80, 0, sizeof(struct recs80_priv));

	/* Initialize to some arbitrary value different than 0 and
	 * (1 << RECS80_TOGGLE_BIT) so it won't match. */

	recs80->toggle = -1;

	return recs80;
}

void recs80_free(void *recs80_ptr)
{
	free(recs80_ptr);
}

int recs80_parse(void *recs80_ptr, struct timeval *ts, ir_event_t *queue, int queue_len, ir_code_t *code)
{
	struct recs80_priv *recs80 = (struct recs80_priv*) recs80_ptr;
	int value = 0;
	int index = 0;
	int toggle;
	int i;

	if (queue_len != RECS80_BIT_COUNT * 2 + 1)
		return 0;

	for (i = 0; i < queue_len; i++) {
		ir_event_t *e = &queue[i];

		if (e->type == IR_EVENT_PULSE) {
			/* Verify pulse length */

			if (e->length > RECS80_PULSE_WIDTH * 4) {
//				printf("recs80: pulse too long %dus\n", e->length);
				return 0;
			}
		} else {
			/* Verify space length */
			if (e->length < RECS80_ZERO_WIDTH / 2 || e->length > RECS80_ONE_WIDTH * 2) {
//				printf("recs80: space too short/long %dus\n", e->length);
				return 0;
			}

			if (e->length > (RECS80_ZERO_WIDTH + RECS80_ONE_WIDTH) / 2)
				value |= (1 << (RECS80_BIT_COUNT - index - 1));

			index++;
		}
	}

//	printf("recs80: ");
//	for (i = 0; i < RECS80_BIT_COUNT; i++)
//		printf("%d", (value & (1 << (RECS80_BIT_COUNT - 1 - i))) ? 1 : 0);
//	printf("\n");

	/* Verify the start bit. */

	if (!(value & (1 << RECS80_START_BIT)))
		return 0;

	toggle = value & (1 << RECS80_TOGGLE_BIT);

	code->type = IR_CODE_RECS80;
	code->address = ((value >> RECS80_ADDRESS_SHIFT) - 1) & ((1 << RECS80_ADDRESS_WIDTH) - 1);
	code->command = (value >> RECS80_COMMAND_SHIFT) & ((1 << RECS80_COMMAND_WIDTH) - 1);

	code->repeat = (toggle == recs80->toggle);

	if (toggle != recs80->toggle)
		recs80->toggle = toggle;

	return 1;
}
