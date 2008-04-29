#ifndef SHARP_H
#define SHARP_H

#include "ir.h"

void *sharp_new(void);
void sharp_free(void *rc5);
int sharp_parse(void *sharp, struct timeval *ts, ir_event_t *queue, int queue_len, ir_code_t *event);

#endif /* SHARP_H */
