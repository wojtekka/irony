#ifndef RC5_H
#define RC5_H

#include "ir.h"

void *rc5_new(void);
void rc5_free(void *rc5);
int rc5_parse(void *rc5, struct timeval *ts, ir_event_t *queue, int queue_len, ir_code_t *event);

#endif /* RC5_H */
