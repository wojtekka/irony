#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <inttypes.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include "uinput.h"
#include <linux/input.h>
#include "irony.h"
#include "lirc.h"
#include "rc5.h"
#include "sirc.h"
#include "recs80.h"
#include "sharp.h"

enum {
	MODE_DEFAULT,
	MODE_MOUSE,
	MODE_POWER
};

int mode;
int delta;
int repeat;
int uinput_fd;
void *rc5;
void *sirc;
void *recs80;
void *sharp;

void run(const char *script, ir_code_t *code)
{
	char type[16], address[16], command[16];
	int pid;

	printf("run(\"%s\", ...)\n", script);

	if (code != NULL) {
		snprintf(type, sizeof(type), "%d", code->type);
		snprintf(address, sizeof(address), "%d", code->address);
		snprintf(command, sizeof(command), "%d", code->command);
	}

	pid = fork();

	if (pid == -1) {
		printf("fork() failed: %s\n", strerror(errno));
		return;
	}

	if (pid == 0) {
		if (code != NULL)
			execlp(script, script, type, address, command, NULL);
		else
			execlp(script, script, NULL);

		exit(1);
	}
}

void acpi_fakekey(int key)
{
	char key_str[16];
	int pid;

	printf("acpi_fakekey(%d)\n", key);

	snprintf(key_str, sizeof(key_str), "%d", key);

	pid = fork();

	if (pid == -1) {
		printf("fork() failed: %s\n", strerror(errno));
		return;
	}

	if (pid == 0) {
		execlp("acpi_fakekey", "acpi_fakekey", key_str, NULL);

		exit(1);
	}
}

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

	if (mode != MODE_POWER && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 12) {
		mode = MODE_POWER;
		acpi_fakekey(KEY_POWER);
		return;
	}

	if (mode == MODE_POWER) {
		if (!code->repeat) {
			if (code->type == IR_CODE_RC5 && code->address == 0 && code->command == 12) {
				uinput_key_press(uinput_fd, KEY_ENTER, -1, -1);
			} else if (code->type == IR_CODE_RC5 && code->address == 0 && code->command == 30) {
				uinput_key_press(uinput_fd, KEY_TAB, -1, -1);
			} else {
				uinput_key_press(uinput_fd, KEY_ESC, -1, -1);
				mode = MODE_DEFAULT;
			}
		}

		return;
	}

	if (mode != MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 38)
		uinput_key_press(uinput_fd, KEY_ENTER, -1, -1);

	if (mode != MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 16)
		uinput_key_press(uinput_fd, KEY_VOLUMEUP, -1, -1);

	if (mode != MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 17)
		uinput_key_press(uinput_fd, KEY_VOLUMEDOWN, -1, -1);

	if (mode != MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 32)
		run("irony-ch-up.sh", code);

	if (mode != MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 33)
		run("irony-ch-down.sh", code);

	if (mode != MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 63)
		run("irony-tv.sh", code);

	if (mode != MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 13)
		uinput_key_press(uinput_fd, KEY_MUTE, -1, -1);

	if (mode != MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 30)
		uinput_key_press(uinput_fd, KEY_TAB, -1, -1);

	if (mode == MODE_MOUSE && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 16)
		uinput_mouse_move(uinput_fd, delta, 0, 0);

	if (mode == MODE_MOUSE && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 17)
		uinput_mouse_move(uinput_fd, -delta, 0, 0);

	if (mode == MODE_MOUSE && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 33)
		uinput_mouse_move(uinput_fd, 0, delta, 0);

	if (mode == MODE_MOUSE && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 32)
		uinput_mouse_move(uinput_fd, 0, -delta, 0);

	if (mode == MODE_MOUSE && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 43)
		uinput_mouse_move(uinput_fd, 0, 0, 1);

	if (mode == MODE_MOUSE && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 44)
		uinput_mouse_move(uinput_fd, 0, 0, -1);

	if (mode == MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 59)
		uinput_key_press(uinput_fd, BTN_LEFT, -1, -1);

	if (mode != MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 1)
		uinput_key_press(uinput_fd, KEY_PREVIOUSSONG, -1, -1);

	if (mode != MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 2)
		uinput_key_press(uinput_fd, KEY_PLAYPAUSE, -1, -1);

	if (mode != MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 3)
		uinput_key_press(uinput_fd, KEY_NEXTSONG, -1, -1);

	if (mode != MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 4)
		uinput_key_press(uinput_fd, KEY_ZOOM, -1, -1);

	if (mode != MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 34)
		run("irony-pip.sh", code);

	if (mode != MODE_MOUSE && !code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 52)
		run("tvtime", NULL);

	if (!code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 54) {
		printf("mouse mode off\n");
		mode = MODE_DEFAULT;
	}

	if (!code->repeat && code->type == IR_CODE_RC5 && code->address == 0 && code->command == 50) {
		printf("mouse mode on\n");
		mode = MODE_MOUSE;
	}

	if (code->type == IR_CODE_SIRC && code->address == 1 && code->command == 18)
		uinput_mouse_move(uinput_fd, delta, 0, 0);

	if (code->type == IR_CODE_SIRC && code->address == 1 && code->command == 19)
		uinput_mouse_move(uinput_fd, -delta, 0, 0);

	if (code->type == IR_CODE_SIRC && code->address == 1 && code->command == 17)
		uinput_mouse_move(uinput_fd, 0, delta, 0);

	if (code->type == IR_CODE_SIRC && code->address == 1 && code->command == 16)
		uinput_mouse_move(uinput_fd, 0, -delta, 0);

	if (!code->repeat && code->type == IR_CODE_SIRC && code->address == 1 && code->command == 20)
		uinput_key_press(uinput_fd, BTN_LEFT, -1, -1);
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
	int lirc_fd;
	int flags;
	int do_timeout = 0;
	ir_event_t queue[128];	
	int queue_len = 0;
	struct timeval ts;

	lirc_fd = open("/dev/lirc0", O_RDONLY);

	if (lirc_fd == -1) {
		perror("/dev/lirc0");
		exit(1);
	}

	uinput_fd = uinput_open();

	if (uinput_fd == -1) {
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

	flags = fcntl(lirc_fd, F_GETFL);

	if (flags >= 0) {
		if (fcntl(lirc_fd, F_SETFL, flags | O_NONBLOCK) != -1) {
			lirc_t data;

			printf("Flushing\n");

			while (read(lirc_fd, &data, sizeof(data)) == sizeof(data))
				;

			fcntl(lirc_fd, F_SETFL, flags);
		}
	}

	for (;;) {
		struct timeval tv;
		lirc_t data;
		int result;
		fd_set rds;

		FD_ZERO(&rds);
		FD_SET(lirc_fd, &rds);

		tv.tv_sec = 0;
		tv.tv_usec = 50000;

		while (waitpid(-1, NULL, WNOHANG) > 0)
			;
		
		result = select(lirc_fd + 1, &rds, NULL, NULL, (do_timeout) ? &tv : NULL);

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

		if (FD_ISSET(lirc_fd, &rds)) {
			int pulse, length;

			if (read(lirc_fd, &data, sizeof(data)) != sizeof(data))
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

	close(lirc_fd);
	uinput_close(uinput_fd);
	rc5_free(rc5);
	sirc_free(sirc);
	recs80_free(recs80);
	sharp_free(sharp);

	return 0;
}
