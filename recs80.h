#ifndef RECS80_H
#define RECS80_H

#include "irony.h"

void *recs80_new(void);
void recs80_free(void *recs80);
int recs80_parse(void *recs80, struct timeval *ts, ir_event_t *queue, int queue_len, ir_code_t *event);

#endif /* RECS80_H */
