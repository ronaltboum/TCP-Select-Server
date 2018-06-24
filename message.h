#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <stdio.h>
#define MAX_SEND_SIZE 10
#define MAX_DATA_SIZE 255
#define MESSAGE_SIZE (sizeof(message))

/**************************************************************************************************************************
 Class Desctiption:
 Represents a message.  The first byte is an unsigned char which represents the length of the entire message
 (including the first byte).  The protocol is simlpe since the server sends and receives messages in a non-blocking manner.
 In each message only length bytes are sent (and not sizeof(message)).  This is optimal for a server which uses select and uses
 a single thread to handle all the I/O.
 A more complex protocol can be created for example by dividing the data field into parts.  For example:
 data:
	 1st byte:  a signed char that represents the length of the sender's name (limited to 127 bytes)
	 2nd byte:  a signed char that represents the length of the request's name (limited to 127 bytes).
	 sender's name
	 request
 The messages are short and have a max length of 255.
 ***************************************************************************************************************************/

#pragma pack(push, 1)

typedef struct{
    unsigned char length;
    char data[MAX_DATA_SIZE];
} message;

#pragma pack(pop)


void print_message(message *m);

#endif
