#include "protocol.h"


//returns 1 if message was completly sent.  0 in case message was partially sent.  -1 in case of error
int send_message(peer *p)
{
  unsigned char message_length = (p->send_buffer).length;
  //printf("in send_message and :\n (p->send_buffer).length = %d, p->current_send_byte = %d \n", 
  	//(p->send_buffer).length,  p->current_send_byte);
  ssize_t already_sent;
  size_t bytes_to_send;
  size_t total_sent = 0;

  if(p->current_send_byte < 0)
  		p->current_send_byte = 0;
 
  do {
    
    //Message was completely sent
    if (p->current_send_byte >= message_length )
    {
        p->current_send_byte = -1;
        //(p->send_buffer).length = 0;
     	return 1;
    }
    
    bytes_to_send = message_length - p->current_send_byte;
    if (bytes_to_send > MAX_SEND_SIZE)
      bytes_to_send = MAX_SEND_SIZE;
    
    //printf("just before send\n");
    //MSG_NOSIGNAL - to prevent generation of SIGPIPE in case client closed the connection.
    already_sent = send(p->socket, (char *)&p->send_buffer + p->current_send_byte, bytes_to_send, MSG_NOSIGNAL | MSG_DONTWAIT);
    //printf("sleeping for 5 seconds\n");
    //sleep(5);

    if (already_sent < 0) 
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        printf("Peer isn't ready now.\n");
        break;
      }
      else {
        perror("Error in send function");
        return -1;
      }
    }
  
    else if (already_sent == 0) {
      printf("Sent 0 bytes.	Peer cannot accept data now.\n");
      break;
    }
    else if (already_sent > 0) {
      p->current_send_byte += already_sent;
      total_sent += already_sent;
      printf("Sent %zd bytes.\n", already_sent);
    }
  } while (already_sent > 0);

  printf("Total bytes sent = %zu bytes.\n", total_sent);
  return 0;
}



int receive_message(peer *p)
{
  
  ssize_t received;
  size_t received_total = 0;
  size_t bytes_to_receive = sizeof(message);  //before we know the message length

  do {
    //if we already know the message length:
    if (p->receive_message_length >= 0)
    {
    	//message was completly received:
	    if(p->current_receive_byte >= p->receive_message_length)
	    {
	      p->current_receive_byte = 0;
	      p->receive_message_length = -1;  //setting unknow length for the next message
	      return 2;
	    }
	    bytes_to_receive = p->receive_message_length - p->current_receive_byte;
    	
    }
 
    if (bytes_to_receive > MAX_SEND_SIZE)
      		bytes_to_receive = MAX_SEND_SIZE;

    //MSG_DONTWAIT -  gives similar behaviour as setting the socket file descriptor as NON-BLOCKING.
    received = recv(p->socket, (char *)&p->receive_buffer + p->current_receive_byte, bytes_to_receive, MSG_DONTWAIT);
    if (received < 0) 
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK) 
      {
        	printf("Peer's socket isn't ready now.\n");
        	break;
      }
      else 
      {
        perror("Error in recv function");
        return -1;
      }
    } 
    
    // If recv() returned 0 - then peer has performed an orderly shutdown
    else if (received == 0) {
      printf("Peer has shut down the connection cleanly.\n");
      return -1;
    }
    else if (received > 0) 
    {
      //if we have yet to discover the message length:
      if(p->receive_message_length < 0)
      {
      		p->receive_message_length = (p->receive_buffer).length;
      		//printf("in recv function and p->receive_message_length = %zd \n", p->receive_message_length);
      }
      p->current_receive_byte += received;
      received_total += received;
      printf("recveived %zd bytes\n", received);
    }
  } while (received > 0);
  
  printf("Total bytes received =  %zu.\n", received_total);
  return 0;
}


/*Utility function that a client can use if it wants to use the peer struct.  sends length -1 bytes from data.
Sends data in a non blocking manner.	*/
int send_message_to_peer (peer *p, unsigned char length, char *data)
{
	//pack the message in the message struct:
	if(length > MESSAGE_SIZE)
	{
		printf("Messages are limited to %d bytes!  Message not sent.\n", MESSAGE_SIZE);
		return -1;
	}
	//queueing messages not implemented here since server always returns OK.
	p->send_buffer.length = length;
	memcpy( (p->send_buffer).data, data, length - 1);
	printf("in send_message_to_peer and:\n  p->send_buffer.length = %d,  p->send_buffer.data = %s\n", (p->send_buffer).length,  (p->send_buffer).data  );
	//try to send the message
	return send_message(p);
}


char * peer_get_socket_address_str(peer *p)
{
  static char ret[INET_ADDRSTRLEN + 10];
  char peer_ipv4_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &p->socket_address.sin_addr, peer_ipv4_str, INET_ADDRSTRLEN);
  sprintf(ret, "%s:%d", peer_ipv4_str, p->socket_address.sin_port);
  
  return ret;
}