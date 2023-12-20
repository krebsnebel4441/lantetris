#include "protocol.h"
#include <stdio.h>

union converter32 {
	uint32_t integer;
	char bytes[4];
};

bytemsg_t encode_message(message_t * message) {
	union converter32 bytes32;
	char * buf;
	bytemsg_t ret;
	switch (message->opcode) {
		case JOIN: 
			size_t buflen = 1 + 4 + message->strlen; 
			buf = (char *)malloc(sizeof(char) * buflen);
			buf[0] = 0x01;
			bytes32.integer = message->strlen;
			for (int i = 0; i < 4; i++) buf[i+1] = bytes32.bytes[i];
			for (int i = 0; i < message->strlen; i++) buf[i+5] = message->name[i];
			ret.size = buflen; ret.buf = buf;
			return ret;
		case START:
			buf = (char *)malloc(sizeof(char) * 5);
			buf[0] = 0x02;
			bytes32.integer = message->seed;
			for (int i = 0; i < 4; i++) buf[i+1] = bytes32.bytes[i];
			ret.size = 5; ret.buf = buf;
			return ret;
		case STATUS:
			size_t len = 1 + 4 + 1 + message->linecount * 11;
			buf = (char *)malloc(sizeof(char) * len);
			buf[0] = 0x03;
			bytes32.integer = message->score;
			for (int i = 0; i < 4; i++) buf[i+1] = bytes32.bytes[i];
			buf[5] = (char)message->linecount;
			union { struct statusline a; char b[11]; } c;
			for (int i = 0; i < message->linecount; i++) {
				c.a = message->lines[i];
				for (int j = 0; j < 11; j++) {
					buf[6 + 11*i + j] = c.b[j];
				}
			}
			ret.size = len; ret.buf = buf;
			return ret;
		case GAMEOVER: 
			buf = (char *)malloc(sizeof(char));
			buf[0] = 0x04;
			ret.size = 1; ret.buf = buf;
			return ret;
	}
}

message_t decode_message(char * buf, size_t size) {
	message_t message;
	union converter32 conv;
	if (size < 1) {
		puts("message too small");
		message.opcode = INVALIDMSG;
		return message;
	}
	switch(buf[0]) {
		case 0x01:
			if (size < 5) { message.opcode = INVALIDMSG; return message; }
			for (int i = 0; i < 4; i++) conv.bytes[i] = buf[i+1];
			int strlen = conv.integer;
			if (size >= 5 + strlen) {
				message.opcode = JOIN;
				message.strlen = strlen;
				message.name = (char *)malloc(sizeof(char) * strlen);
				for (int i = 0; i < strlen; i++) message.name[i] = buf[i+5];
			} else { message.opcode = INVALIDMSG; }
			return message;
		case 0x02:
			if (size >= 5) {
				message.opcode = START;
				for (int i = 0; i < 4; i++) conv.bytes[i] = buf[i+1];
				message.seed = conv.integer;
			} else { message.opcode = INVALIDMSG; }
			return message;	
		case 0x03:
			if (size < 6) {
				message.opcode = INVALIDMSG;
				return message;
			}
			message.opcode = STATUS;
			for (int i = 0; i < 4; i++) conv.bytes[i] = buf[i+1];
			message.score = conv.integer;
			message.linecount = buf[5];
			if (size < 1 + 4 + 1 + message.linecount * 11) {
				message.opcode = INVALIDMSG; 
				return message;
			}
			message.lines = malloc(sizeof(struct statusline) * message.linecount);
			union { struct statusline a; char b[11]; } lineconvert;
			for (int i = 0; i < message.linecount; i++) {
				for (int j = 0; j < 11; j++) {
					lineconvert.b[j] = buf[6 + 11*i + j];
				}
				message.lines[i] = lineconvert.a;
			}
			return message;	
		case 0x04: message.opcode = GAMEOVER; return message; 
		default: message.opcode = INVALIDMSG; return message;
	}
}
