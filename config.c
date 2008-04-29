#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "config.h"

int config_init(void)
{
	char *tmp;

	config_modes = malloc(sizeof(config_mode_t));
	
	if (config_modes == NULL)
		return -1;

	memset(config_modes, 0, sizeof(config_mode_t));

	tmp = strdup("default");

	if (tmp == NULL) {
		free(config_modes);
		config_modes = NULL;
		return -1;
	}

	config_modes[0].name = tmp;
	config_modes_count = 1;

	config_actions = NULL;
	config_actions_count = 0;

	return 0;
}

int config_mode_add(config_mode_t *mode)
{
	config_mode_t *tmp;

	tmp = realloc(config_modes, (config_modes_count + 1) * sizeof(config_mode_t));

	if (tmp == NULL)
		return -1;

	config_modes = tmp;
	memcpy(&config_modes[config_modes_count], mode, sizeof(config_mode_t));
	config_modes_count++;

	return 0;
}

int config_mode_find(const char *name)
{
	int i;

	for (i = 0; i < config_modes_count; i++) {
		if (strcmp(config_modes[i].name, name) == 0)
			return i;
	}

	return -1;
}

int config_action_add(config_action_t *action)
{
	config_action_t *tmp;

	tmp = realloc(config_actions, (config_actions_count + 1) * sizeof(config_action_t));

	if (tmp == NULL)
		return -1;

	config_actions = tmp;
	memcpy(&config_actions[config_actions_count], action, sizeof(config_action_t));
	config_actions_count++;

	return 0;
}

void config_free(void)
{
	int i;

	for (i = 0; i < config_modes_count; i++)
		free(config_modes[i].name);

	free(config_modes);
	config_modes = NULL;
	config_modes_count = 0;

	for (i = 0; i < config_actions_count; i++) {
		if (config_actions[i].type == CONFIG_ACTION_EXEC)
			free(config_actions[i].exec);
	}

	free(config_actions);
	config_actions = NULL;
	config_actions_count = 0;
}

int config_read(void)
{
	char buf[256];
	FILE *f;

	f = fopen("ir.conf", "r");

	if (!f)
		return -1;

	while (fgets(buf, sizeof(buf), f)) {
		char *line;
		int len;

		line = buf;
		len = strlen(line);

		while (len > 1 && isspace(line[len - 1])) {
			line[len - 1] = 0;
			len--;
		}

		if (len > 1 && isspace(line[0])) {
			line++;
			len--;
		}

		if (len == 0 || line[0] == '#')
			continue;

		if (strncmp(line, "mode define ", 12) == 0) {
			config_mode_t mode;

			memset(&mode, 0, sizeof(mode));

			mode.name = strdup(line + 12);

			if (!mode.name)
				goto failure;

			config_mode_add(&mode);

			continue;
		}

		if (strncmp(line, "mode default ", 13) == 0) {
			int mode = -1;
			int i;

			if (config_mode_find(line + 13) != -1)
				continue;

			for (i = 0; config_modes && config_modes[i].name; i++) {
				if (strcmp(config_modes[i].name, line + 13) == 0) {
					mode = i;
					break;
				}
			}

			if (mode == -1)
				goto failure;

			config_mode = mode;

			continue;
		}

		if (strncmp(line, "on ", 3) == 0) {

			continue;
		}

		goto failure;
	}

	fclose(f);

	return 0;

failure:
	fclose(f);

	return -1;
}

