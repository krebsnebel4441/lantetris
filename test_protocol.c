#include "protocol.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

int main() {
	{
		//initialisation
		message_t message;
		message.opcode = START;
		message.seed = 123423;
		//encode
		bytemsg_t msg = encode_message(&message);
		//decode
		message_t cmp = decode_message(msg.buf, msg.size);
		//test
		assert(message.opcode == cmp.opcode); 
		assert(message.seed == cmp.seed);
		//cleanup
		free(msg.buf);
		
		puts("\033[32mSuccesfully tested start message\033[0m");
	}
	printf("\n\n");
	{
		message_t message;
		message.opcode = JOIN;
		const char name[] = "krebsnebel";
		message.strlen = sizeof(name);
		message.name = malloc(sizeof(char) * message.strlen);
		memcpy(message.name, &name, message.strlen);

		bytemsg_t msg = encode_message(&message);

		message_t cmp = decode_message(msg.buf, msg.size);

		assert(message.opcode == cmp.opcode);
		assert(message.strlen == cmp.strlen);
		assert(memcmp(message.name, cmp.name, cmp.strlen) == 0);
		
		free(msg.buf);
		free(message.name);
		free(cmp.name);

		puts("\033[32mSuccesfully tested join message\033[0m");
	}
	printf("\n\n");
	{
		message_t message;
		message.opcode = STATUS;
		message.score = 12000;
		message.linecount = 2;
		message.lines = malloc(sizeof(struct statusline)*2);
		message.lines[0].index = 5;
		for (int i = 0; i < 10; i++) message.lines[0].fields[i] = 0x0;
		message.lines[0].fields[5] = 0x5;
		message.lines[1].index = 6;
		for (int i = 0; i < 10; i++) message.lines[1].fields[i] = 0x0;
		message.lines[1].fields[9] = 0x2;
		
		bytemsg_t msg = encode_message(&message);
		
		message_t cmp = decode_message(msg.buf, msg.size);
		assert(message.opcode == cmp.opcode);
		assert(message.score == cmp.score);
		assert(message.linecount == cmp.linecount);
		for (int i = 0; i < cmp.linecount; i++) {
			assert(cmp.lines[i].index == message.lines[i].index);
			for (int j = 0; j < 10; j++) {
				assert(cmp.lines[i].fields[j] == message.lines[i].fields[j]);
			}
		}
		free(message.lines);
		free(cmp.lines);
		free(msg.buf);
		
		puts("\033[32mSuccesfully tested status message\033[0m");
	}
	return 0;
}
