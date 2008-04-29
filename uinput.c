#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/time.h>
#include <sys/types.h>

/**
 * \brief Opens uinput device.
 *
 * \return File descriptor
 */
int uinput_open(void)
{
	const char *uinputs[] = { "/dev/uinput", "/dev/input/uinput", "/dev/misc/uinput", NULL };
	struct uinput_user_dev uud;
	int fd;
        int i;

	for (i = 0; uinputs[i] != NULL; i++) {
        	fd = open(uinputs[i], O_WRONLY | O_NDELAY);

		if (fd != -1)
			break;
	}

	if (fd == -1)
		return -1;

	memset(&uud, 0, sizeof(uud));
	strncpy(uud.name, "Irony", UINPUT_MAX_NAME_SIZE);
	uud.id.version = 4;
	uud.id.bustype = BUS_USB;

	write(fd, &uud, sizeof(uud));

	ioctl(fd, UI_SET_EVBIT, EV_SYN);
	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_EVBIT, EV_REP);
	ioctl(fd, UI_SET_EVBIT, EV_REL);
	ioctl(fd, UI_SET_RELBIT, REL_X);
	ioctl(fd, UI_SET_RELBIT, REL_Y);
	ioctl(fd, UI_SET_RELBIT, REL_WHEEL);

	for (i = 0; i < KEY_MAX; i++)
		ioctl(fd, UI_SET_KEYBIT, i);

	if (ioctl(fd, UI_DEV_CREATE)) {
		int errsv = errno;
		close(fd);
		errno = errsv;
		return -1;
	}

	return fd;
}

/**
 * \brief Closes uinput device.
 *
 * \param fd File descriptor
 */
void uinput_close(int fd)
{
	if (fd != -1) {
		ioctl(fd, UI_DEV_DESTROY);
		close(fd);
	}
}

/**
 * \brief Sends mouse move event.
 *
 * \param fd File descriptor
 * \param x X axis movement
 * \param y Y axis movement
 * \param wheel Mouse wheel movement
 *
 * \return 0 on success, -1 on error
 */
int uinput_mouse_move(int fd, int x, int y, int wheel)
{
	struct input_event ev;

//	printf("uinput_mouse_move(fd, %d, %d, %d)\n", x, y, wheel);

	memset(&ev, 0, sizeof(ev));
	gettimeofday(&ev.time, NULL);

	if (x != 0) {
		ev.type = EV_REL;
		ev.code = REL_X;
		ev.value = x;

		if (write(fd, &ev, sizeof(ev)) != sizeof(ev))
			return -1;
	}

	if (y != 0) {
		ev.type = EV_REL;
		ev.code = REL_Y;
		ev.value = y;

		if (write(fd, &ev, sizeof(ev)) != sizeof(ev))
			return -1;
	}

	if (wheel != 0) {
		ev.type = EV_REL;
		ev.code = REL_WHEEL;
		ev.value = wheel;

		if (write(fd, &ev, sizeof(ev)) != sizeof(ev))
			return -1;
	}

	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	ev.value = 0;

	if (write(fd, &ev, sizeof(ev)) != sizeof(ev))
		return -1;

	return 0;
}

/**
 * \brief Sends single key event.
 *
 * \param fd File descriptor
 * \param key Key number
 * \param value Key value (0 released, 1 pressed)
 *
 * \return 0 on success, -1 on error
 */
static int uinput_send_single_key(int fd, int key, int value)
{
	struct input_event ev;

	memset(&ev, 0, sizeof(ev));
	gettimeofday(&ev.time, NULL);

	ev.type = EV_KEY;
	ev.code = key;
	ev.value = value;

	if (write(fd, &ev, sizeof(ev)) != sizeof(ev))
		return -1;

	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	ev.value = 0;

	if (write(fd, &ev, sizeof(ev)) != sizeof(ev))
		return -1;

	return 0;
}

/**
 * \brief Sends key event with modifiers.
 *
 * \param fd File descriptor
 * \param key Key number
 * \param mod1 First modifier key number or -1 if unused
 * \param mod2 Second modifier key number or -1 if unused
 *
 * \return 0 on success, -1 on error
 */
int uinput_key_press(int fd, int key, int mod1, int mod2)
{
	printf("uinput_key_press(fd, %d, %d, %d)\n", key, mod1, mod2);

	if ((mod1 != -1) && (uinput_send_single_key(fd, mod1, 1) == -1))
		return -1;

	if ((mod2 != -1) && (uinput_send_single_key(fd, mod2, 1) == -1))
		return -1;

	if (uinput_send_single_key(fd, key, 1) == -1)
		return -1;

	if (uinput_send_single_key(fd, key, 0) == -1)
		return -1;

	if ((mod2 != -1) && (uinput_send_single_key(fd, mod2, 0) == -1))
		return -1;

	if ((mod1 != -1) && (uinput_send_single_key(fd, mod1, 0) == -1))
		return -1;

	return 0;
}


