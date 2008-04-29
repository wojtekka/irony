#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <inttypes.h>
#include <sys/select.h>
#include <errno.h>
#include "uinput.h"
#include <linux/input.h>
#include "ir.h"
#include "lirc.h"
#include "rc5.h"
#include "sirc.h"
#include "recs80.h"
#include "sharp.h"

int mouse;
int delta;
int repeat;
int uifd;
void *rc5;
void *sirc;
void *recs80;
void *sharp;

void handle(ir_code_t *code)
{
	printf("type %d, address %d, command %d, repeat %d\n", code->type, code->address, code->command, code->repeat);

	if (!code->repeat)
		repeat = 0;
	else
		repeat++;

	if (code->type == IR_CODE_SIRC) {
		if (repeat < 15) {
			delta = 1 << (repeat / 3);
		} else
			delta = 32;
	} else {
		if (repeat < 12) {
			delta = 1 << (repeat / 2);
		} else
			delta = 64;
	}

	if (!mouse && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 12)
		uinput_key_press(uifd, KEY_POWER, -1, -1);

	if (!mouse && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 16)
		uinput_key_press(uifd, KEY_VOLUMEUP, -1, -1);

	if (!mouse && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 17)
		uinput_key_press(uifd, KEY_VOLUMEDOWN, -1, -1);

	if (!mouse && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 13)
		uinput_key_press(uifd, KEY_MUTE, -1, -1);

	if (!mouse && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 30)
		uinput_key_press(uifd, KEY_TAB, -1, -1);

	if (mouse && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 16)
		uinput_mouse_move(uifd, delta, 0, 0);

	if (mouse && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 17)
		uinput_mouse_move(uifd, -delta, 0, 0);

	if (mouse && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 33)
		uinput_mouse_move(uifd, 0, delta, 0);

	if (mouse && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 32)
		uinput_mouse_move(uifd, 0, -delta, 0);

	if (mouse && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 43)
		uinput_mouse_move(uifd, 0, 0, 1);

	if (mouse && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 44)
		uinput_mouse_move(uifd, 0, 0, -1);

	if (!code->repeat && mouse && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 59)
		uinput_key_press(uifd, BTN_LEFT, -1, -1);

	if (!mouse && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 1)
		uinput_key_press(uifd, KEY_PREVIOUSSONG, -1, -1);

	if (!mouse && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 2)
		uinput_key_press(uifd, KEY_PLAYPAUSE, -1, -1);

	if (!mouse && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 3)
		uinput_key_press(uifd, KEY_NEXTSONG, -1, -1);

	if (!mouse && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 4)
		uinput_key_press(uifd, KEY_ZOOM, -1, -1);

	if (!code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 54) {
		printf("mouse mode off\n");
		mouse = 0;
	}

	if (!code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 50) {
		printf("mouse mode on\n");
		mouse = 1;
	}

	if (code->type == IR_CODE_SIRC && code->address == 1 && code->command == 18)
		uinput_mouse_move(uifd, delta, 0, 0);

	if (code->type == IR_CODE_SIRC && code->address == 1 && code->command == 19)
		uinput_mouse_move(uifd, -delta, 0, 0);

	if (code->type == IR_CODE_SIRC && code->address == 1 && code->command == 17)
		uinput_mouse_move(uifd, 0, delta, 0);

	if (code->type == IR_CODE_SIRC && code->address == 1 && code->command == 16)
		uinput_mouse_move(uifd, 0, -delta, 0);

	if (!code->repeat && code->type == IR_CODE_SIRC && code->address == 1 && code->command == 20)
		uinput_key_press(uifd, BTN_LEFT, -1, -1);

	if (!code->repeat && code->type == IR_CODE_SIRC && code->address == 1 && code->command == 21)
		uinput_key_press(uifd, KEY_APOSTROPHE, KEY_RIGHTSHIFT, -1);
}

void parse(struct timeval *ts, ir_event_t *queue, int queue_len)
{
	ir_code_t code;
	int i;

	/* Validate the sequence */
	for (i = 0; i < queue_len; i++) {
		if (i % 2 == 0 && queue[i].type != IR_EVENT_PULSE)
			return;

		if (i % 2 == 1 && queue[i].type != IR_EVENT_SPACE)
			return;
	}

	if (rc5_parse(rc5, ts, queue, queue_len, &code))
		handle(&code);

	if (sirc_parse(sirc, ts, queue, queue_len, &code))
		handle(&code);

	if (recs80_parse(recs80, ts, queue, queue_len, &code))
		handle(&code);

	if (sharp_parse(sharp, ts, queue, queue_len, &code))
		handle(&code);
}

int main(void)
{
	int irfd;
	int flags;
	int do_timeout = 0;
	ir_event_t queue[128];	
	int queue_len = 0;
	struct timeval ts;

	irfd = open("/dev/lirc0", O_RDONLY);

	if (irfd == -1) {
		perror("/dev/lirc0");
		exit(1);
	}

	uifd = uinput_open();

	if (uifd == -1) {
		perror("uinput");
		exit(1);
	}

	if (!(rc5 = rc5_new())) {
		perror("rc5_new");
		exit(1);
	}

	if (!(sirc = sirc_new())) {
		perror("sirc_new");
		exit(1);
	}

	if (!(recs80 = recs80_new())) {
		perror("recs80_new");
		exit(1);
	}

	if (!(sharp = sharp_new())) {
		perror("sharp_new");
		exit(1);
	}

	/* Flush buffer */

	flags = fcntl(irfd, F_GETFL);

	if (flags >= 0) {
		if (fcntl(irfd, F_SETFL, flags | O_NONBLOCK) != -1) {
			lirc_t data;

			printf("Flushing\n");

			while (read(irfd, &data, sizeof(data)) == sizeof(data))
				;

			fcntl(irfd, F_SETFL, flags);
		}
	}

	for (;;) {
		struct timeval tv;
		lirc_t data;
		int result;
		fd_set rds;

		FD_ZERO(&rds);
		FD_SET(irfd, &rds);

		tv.tv_sec = 0;
		tv.tv_usec = 50000;
		
		result = select(irfd + 1, &rds, NULL, NULL, (do_timeout) ? &tv : NULL);

		if (result == -1) {
			if (errno == EINTR)
				continue;

			perror("select");
			break;
		}

		if (result == 0 && queue_len > 0) {
//			printf("lirc: timeout, %d events, parsing\n", queue_len);
			parse(&ts, queue, queue_len);
			queue_len = 0;
			do_timeout = 0;
		}

		if (FD_ISSET(irfd, &rds)) {
			int pulse, length;

			if (read(irfd, &data, sizeof(data)) != sizeof(data))
				continue;

			pulse = data & PULSE_BIT;
			length = data & PULSE_MASK;

			if (!pulse && length > 20000) {
//				printf("lirc: long space\n");
				if (queue_len > 0) {
//					printf("lirc: some data in queue, parsing\n");
					parse(&ts, queue, queue_len);
				}

				queue_len = 0;	
				do_timeout = 0;
			} else {
//				printf("lirc: queueing event %d\n", queue_len);
				if (queue_len == 0)
					gettimeofday(&ts, NULL);
				queue[queue_len].type = (pulse) ? IR_EVENT_PULSE : IR_EVENT_SPACE;
				queue[queue_len].length = length;
				if (queue_len < sizeof(queue) / sizeof(queue[0]) - 1)
					queue_len++;
				do_timeout = 1;
			}
		}
	}

	close(irfd);
	uinput_close(uifd);
	rc5_free(rc5);
	sirc_free(sirc);
	recs80_free(recs80);
	sharp_free(sharp);

	return 0;
}
