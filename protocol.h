#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

enum opcode { JOIN = 1, START, STATUS, GAMEOVER, INVALIDMSG };

struct statusline {
	uint8_t index;
	uint8_t fields[10];
};

typedef struct {
	enum opcode opcode;
	union {
		struct {
			uint32_t strlen;
			char * name;
		};
		struct {
			uint32_t seed;
			uint8_t level;
		};
		struct {
			uint32_t score;
			uint8_t linecount;
			struct statusline * lines;
		};
	};
} message_t;

typedef struct {
	size_t size;
	char * buf;
} bytemsg_t;

bytemsg_t encode_message(message_t * message);
message_t decode_message(char * buf, size_t size);

#endif
