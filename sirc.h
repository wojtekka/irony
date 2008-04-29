#ifndef SIRC_H
#define SIRC_H

#include "irony.h"

void *sirc_new(void);
void sirc_free(void *sirc);
int sirc_parse(void *sirc, struct timeval *ts, ir_event_t *queue, int queue_len, ir_code_t *event);

#endif /* SIRC_H */
