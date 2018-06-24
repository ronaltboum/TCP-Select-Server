#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <stdio.h>
#include <string.h>   
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   
#include <arpa/inet.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> 
#include <fcntl.h> 
#include <time.h> 
#include <assert.h>
#include <signal.h>

#include "message.h"

#define NO_SOCKET -1

/**************************************************************************************************************************
 Class Desctiption:
 This sturct is a utility struct that is used by the server in order to read and write to clients in a non blocking manner.
 It maintains information about each connected client.
 Clients are free to use this struct as well.
 ***************************************************************************************************************************/

typedef struct
{
  int socket = NO_SOCKET;
  struct sockaddr_in socket_address;

  message receive_buffer;
  size_t current_receive_byte = 0;
  ssize_t receive_message_length = -1;   // -1 until we read the message length (first byte in the message)
  
  message send_buffer;
  size_t pendingOK = 0;
  ssize_t current_send_byte = -1;
} peer;


//Send and Receive messages in a NON BLOCKING manner:
int send_message(peer *peer);
int receive_message(peer *peer);

/*Utility function that a client can use if it wants to use the peer struct.
Sends data in a non blocking manner.  Sends length-1 bytes from data. The client is free of course to send messages
in a blocking mode (see for example the function in simpleClient). */ 
int send_message_to_peer (peer *p, unsigned char length, char *data);

char *peer_get_socket_address_str(peer *peer);


#endif
