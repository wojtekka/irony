#ifndef IR_H
#define IR_H

typedef enum {
	IR_EVENT_SPACE = 0,
	IR_EVENT_PULSE,
} ir_event_type_t;

typedef struct {
	ir_event_type_t type;
	int length;
} ir_event_t;

typedef enum {
	IR_CODE_NONE = 0,
	IR_CODE_RC5,
	IR_CODE_SIRC,
	IR_CODE_RECS80,
	IR_CODE_SHARP_OLD,
	IR_CODE_SHARP,
} ir_code_type_t;

typedef struct {
	ir_code_type_t type;
	int address;
	int command;
	int repeat;
} ir_code_t;

#endif /* IR_H */
