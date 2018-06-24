#include "TcpServer.h"

//c'tor :  Starts and runs the server on a separate thread.
TcpServer::TcpServer(unsigned short port, CallbackFunction cb)
{
		  //for machines where sizeof(unsigned short) is larger than 16 bit,  make sure port number isn't too high:
		  //negative numbers are translated to unsigned short by the compiler, so no need really to check for non negativity.
		  if(port >= 65535 || port < 0)
		  {
		  	printf("ERROR:  Port number should be in the range 0 - 65534.  Server not created !\n");
		  	return;
		  }
		  server_port = port;
		  //TcpServer::connectCallback(cb);
		  m_cb = cb;
		  //initialize the messages queue:
		  queue = new Queue<message>;
		  if(NULL == queue){
		      exit(-1);
		  }
		  int initVal = queue->Queue<message>::initialize_Queue();
		  if(initVal != 0){
		      printf("ERROR: initialize_Queue failed. Return code from initialize_Queue is %d\n", initVal);
		      exit(-1); 
		  }
          //spawn the main thread of the server - the one that handles the socket I/O.   It spawns worker threads.
		  th = spawn();
}



message TcpServer::dequeueMessage()
{
	message messageToDequeue;
    int deq_val = getQueue()->Queue<message>::dequeue_data(&messageToDequeue);
    if(deq_val != 0)
    {
             printf( "Error: dequeue failed. Return code from dequeue is %d\n", deq_val);
              exit(-1);
    }  
    return std::move(messageToDequeue);
    //return messageToDequeue;
}


//init functions:
void TcpServer::init_and_run()
{
  
  s_catch_signals ();
  
  if (start_listening_socket(&listen_sock) != 0)
    exit(EXIT_FAILURE);
	
  fd_set read_fds;
  fd_set write_fds;
  fd_set except_fds;
  
  int high_sock = listen_sock;
  
  printf("Waiting for incoming connections.\n");

  int numThreads = std::thread::hardware_concurrency() - 2;  // 1 thread for the main program , 1 thread for sockets I/O
  if(numThreads <= 0 )
  	numThreads = 1;
  
  /*create and detach worker threads that deque messages and run the callback function (which contains heavy work such as
  write to the disk,  heavy calculation, etc) */
  std::thread threadArr[numThreads];
  int i;
  for(i = 0; i < numThreads; i++)
  	threadArr[i] = TcpServer::spawn_worker();

  for(i = 0; i < numThreads; i++)
  	threadArr[i].detach();
  
  
  while (1)
  {
            rebuild_sets(&read_fds, &write_fds, &except_fds);
            
            high_sock = listen_sock;
            for (i = 0; i < MAX_CLIENTS; ++i) {
              if (connection_list[i].socket > high_sock)
                high_sock = connection_list[i].socket;
            }
            
            int activity = select(high_sock + 1, &read_fds, &write_fds, &except_fds, NULL);
         
            switch (activity)
            {
	              case -1:
	                perror("select()");
	                shutdown_properly(EXIT_FAILURE);
	         
	              case 0:
	                //should never get here
	                printf("select() returns 0.\n");
	                shutdown_properly(EXIT_FAILURE);
	              
	              default:
	                /* All set fds should be checked. */
	             
	             	//A new client asked to connect:
	                if (FD_ISSET(listen_sock, &read_fds)) {
	                  handle_new_connection();
	                }
	                
	              
	                if (FD_ISSET(listen_sock, &except_fds)) {
	                  printf("Exception listen socket fd.\n");
	                  shutdown_properly(EXIT_FAILURE);
	                }
	                
	                for (i = 0; i < MAX_CLIENTS; ++i) 
	                {
	                      	if (connection_list[i].socket != NO_SOCKET && FD_ISSET(connection_list[i].socket, &read_fds)) 
	                        {
	                            int res = receive_message(&connection_list[i]);
	                            if (res< 0) 
	                            {
	                                close_client_connection(&connection_list[i]);
	                                continue;
	                            }
	                            else if(res == 2)
	                            {
	                            	//for debug:
	                            	print_message(  &(connection_list[i].receive_buffer)  );
	                            	
	                                //replay ok in a non blocking manner:
	                                if(replayOK(&connection_list[i]) < 0)
	                                {
	                                	//In case of error in the send function - close the socket:
	                                	printf("Error in send function. Client may have shut down.  Closing client's socket\n");
	   									close_client_connection(&connection_list[i]);
	                                }
	   								
	                                //TODO:  turn Queue into a lockless queue,  so that the master thread will not have to wait
	                                //in order to enqueu the message.
	                                //Notice we have to pass the message by value to the queue.  Alternatevely, we can make a copy
	                                //of the message on the heap,  and then have a Queue<message*>.
	                                int enq_val = queue->Queue<message>::enqueue_data(connection_list[i].receive_buffer);
	                                if(enq_val != 0)
	                                {
	                                        printf("Error : enqueue failed. Return code from enqueue is %d\n", enq_val);
	                                        exit(-1);
	                                }
	                            }
		                    }
	                  
	                          if (connection_list[i].socket != NO_SOCKET && FD_ISSET(connection_list[i].socket, &write_fds)) 
	                          {
	                                    if (send_message(&connection_list[i]) < 0) {
	                                      printf("Error in send function. Client may have shut down.  Closing client's socket\n");
	                                      close_client_connection(&connection_list[i]);
	                                      continue;
	                                    }
	                          }

	                          if (connection_list[i].socket != NO_SOCKET && FD_ISSET(connection_list[i].socket, &except_fds))
	                          {
	                                    printf("Exception client fd.\n");
	                                    close_client_connection(&connection_list[i]);
	                                    continue;
	                          }
	                }
	        }
    }

}


//worker thread's routine:
void *TcpServer::work_routine ()
{
    while(1)
    {
      message messageToDequeue = TcpServer::dequeueMessage();
      //perform heavy action - callback function:
	  TcpServer::getCallBackFunc()(&messageToDequeue); 
    }  
}


/** Sets the block or non block mode of the socket.  Returns true on success, or false if there was an error */
bool TcpServer::set_socket_blocking_mode(int fd, bool blocking)
{
   if (fd < 0) return false;

   int flags = fcntl(fd, F_GETFL, 0);
   if (flags == -1) return false;
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
}

/* Start listening socket listen_sock. */
int TcpServer::start_listening_socket(int *listen_sock)
{
  *listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (*listen_sock < 0) {
    perror("socket");
    return -1;
  }
 
  int reuse = 1;
  if (setsockopt(*listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) {
    perror("setsockopt");
    return -1;
  }

  /*set the socket to NON BLOCKING mode:
  All of the sockets for the incoming connections will also be nonblocking since they will inherit that state from the 
  listening socket. */
  if(!TcpServer::set_socket_blocking_mode(*listen_sock, 0) )
  {
  	perror("fcntl");
    return -1;
  }

  
  struct sockaddr_in my_addr;
  memset(&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_addr.s_addr = inet_addr(SERVER_IPV4_ADDR);
  my_addr.sin_port = htons(server_port);
 
  if (bind(*listen_sock, (struct sockaddr*)&my_addr, sizeof(struct sockaddr)) != 0) {
    perror("bind");
    return -1;
  }
 
  // start accept client connections
  if (listen(*listen_sock, 10) != 0) {
    perror("listen");
    return -1;
  }
  printf("Server accepting connections on port %d.\n", (unsigned short)server_port);
 
  return 0;
}

int TcpServer::rebuild_sets(fd_set *read_fds, fd_set *write_fds, fd_set *except_fds)
{
  int i;
  
  FD_ZERO(read_fds);
  FD_SET(listen_sock, read_fds);
  for (i = 0; i < MAX_CLIENTS; ++i)
    if (connection_list[i].socket != NO_SOCKET)
      FD_SET(connection_list[i].socket, read_fds);

  FD_ZERO(write_fds);
  for (i = 0; i < MAX_CLIENTS; ++i)
    if (connection_list[i].socket != NO_SOCKET && connection_list[i].current_send_byte >= 0)
      FD_SET(connection_list[i].socket, write_fds);
  
  FD_ZERO(except_fds);  //This set is watched for "exceptional conditions". In practice, only one such exceptional condition is common: the availability of out-of-band (OOB) data for reading from a TCP socket.
  FD_SET(listen_sock, except_fds);
  for (i = 0; i < MAX_CLIENTS; ++i)
    if (connection_list[i].socket != NO_SOCKET)
      FD_SET(connection_list[i].socket, except_fds);
 
  return 0;
}  


int TcpServer::handle_new_connection()
{
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(client_addr));
  socklen_t client_len = sizeof(client_addr);
  int new_client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
  if (new_client_sock < 0) {
    perror("accept()");
    return -1;
  }
  
  char client_ipv4_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &client_addr.sin_addr, client_ipv4_str, INET_ADDRSTRLEN);
  
  printf("Incoming connection from %s:%d.\n", client_ipv4_str, client_addr.sin_port);
  
  int i;
  for (i = 0; i < MAX_CLIENTS; ++i) {
    if (connection_list[i].socket == NO_SOCKET) {
      connection_list[i].socket = new_client_sock;
      connection_list[i].socket_address = client_addr;
	  connection_list[i].receive_message_length = -1;
      connection_list[i].current_send_byte   = -1;
      connection_list[i].current_receive_byte = 0;
      return 0;
    }
  }
  
  printf("There are too many connections! Closing new connection %s:%d.\n", client_ipv4_str, client_addr.sin_port);
  close(new_client_sock);
  return -1;
}

int TcpServer::close_client_connection(peer *client)
{
  printf("Close client socket for %s.\n", peer_get_socket_address_str(client));
  
  close(client->socket);
  client->socket = NO_SOCKET;
  client->current_send_byte   = -1;
  client->current_receive_byte = 0;
  client->receive_message_length = -1;
  client->pendingOK = 0;

  return 0;
}



int TcpServer::handle_received_message(message *message)
{
  printf("Received message from client.\n");
  print_message(message);
  return 0;
}

                                            

//might need to send a number of OK's in case the client was unavailable earlier for receiving an approval for a previous request
int TcpServer::replayOK(peer *p)
{
   if(p->current_send_byte < 0)
   		p->current_send_byte = 0;
   	char arr[OK_MESSAGE_SIZE] = {'O','K', '\0'};      //TODO:  make it global
   //if this is the first time the server replays OK to this client - copy ok message to the client's send buffer.
   if(p->pendingOK == 0)
   {
   		memcpy( (p->send_buffer).data, arr, OK_MESSAGE_SIZE - 1);
   		(p->send_buffer).length = OK_MESSAGE_SIZE;
   }
   ++(p->pendingOK);
   int res;
   while(p->pendingOK > 0)
   {
   		res = send_message(p);
   		//if message was partially sent, or if there has been an error in the send function,  return.
   		if(res != 1)
   			return res;
   		--(p->pendingOK);
   } 
   return 0; 
}




//shutdown functions:

void TcpServer::shutdown_properly(int code)
{
  close(listen_sock);
  
  for (int i = 0; i < MAX_CLIENTS; ++i)
    if (connection_list[i].socket != NO_SOCKET)
      close(connection_list[i].socket);
    
  printf("Shuting down the server properly.\n");
  exit(code);
}
