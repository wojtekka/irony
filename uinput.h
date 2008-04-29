#ifndef UINPUT_H
#define UINPUT_H

int uinput_open(void);
int uinput_key_press(int fd, int key, int mod1, int mod2);
int uinput_mouse_move(int fd, int x, int y, int wheel);
void uinput_close(int fd);

#endif /* UINPUT_H */
