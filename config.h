#ifndef CONFIG_H
#define CONFIG_H

int config_mode;

typedef struct {
	char *name;
} config_mode_t;

config_mode_t *config_modes;

int config_modes_count;

typedef enum {
	CONFIG_ACTION_NONE = 0,
	CONFIG_ACTION_SEND_KEY,
	CONFIG_ACTION_MOVE_MOUSE,
	CONFIG_ACTION_SET_MODE,
	CONFIG_ACTION_CYCLE_MODES,
	CONFIG_ACTION_TOGGLE_MODES,
	CONFIG_ACTION_EXEC,
} config_action_type_t;

typedef struct {
	config_action_type_t type;

	union {
		int send_key[3];
		int move_mouse[3];
		int set_mode;
		int toggle_modes[2];
		char *exec;
	};
} config_action_t;

config_action_t *config_actions;

int config_actions_count;

int config_init(void);
int config_read(void);
int config_write(void);
void config_free(void);

#endif /* CONFIG_H */
