#ifndef __TCPSERVER__H_
#define __TCPSERVER__H_

#include "Queue.h"
#include "peer.h"

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

#include <iostream>
#include <thread>           //for hardware_concurrency()
#include <fcntl.h>          //for non blocking sockets ! 
#include <time.h> 
#include <assert.h>
#include <csignal>
#include <signal.h>
#include <functional>

#define MAX_CLIENTS 10
#define SERVER_IPV4_ADDR "127.0.0.1"
#define OK_MESSAGE_SIZE 3

// Callback function.
typedef std::function<void(message*)> CallbackFunction;

class TcpServer 
{

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Members ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
private:
    CallbackFunction m_cb;
    unsigned short server_port;
    int listen_sock;
    peer connection_list[MAX_CLIENTS];  //all clients are handled by a single thread.
    Queue<message> *queue;
    std::thread th;   //thread that runs the server. It will spawn worker threads to handle heavy work done in the callback function

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Ctor/Dtor ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~    
public:
	TcpServer(unsigned short server_port, CallbackFunction cb);
	
   ~TcpServer(void)
   {
        std::cout << "in d'tor" <<std::endl;
        th.join();
        queue->destroy_queue();
        delete queue;
        std::cout << "in d'tor after deleting queue" <<std::endl;
   }

   TcpServer(const TcpServer& other) = delete;
   TcpServer& operator= (const TcpServer & other) = delete;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Getters/Setters ~~~~~~~~~~~~~~~~~~~~~~~~~~
public:
    CallbackFunction getCallBackFunc() {return m_cb;}
    Queue<message> * getQueue() {return queue;}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    message dequeueMessage();        //public function for dequeing messages from the internal queue.
    
    
private:
    //main function of the server - starts and runs it in its c'tor:
    void init_and_run();
    //init functions:
    int rebuild_sets(fd_set *read_fds, fd_set *write_fds, fd_set *except_fds);
    int start_listening_socket(int *listen_sock);
    int handle_new_connection();
    bool set_socket_blocking_mode(int fd, bool blocking);
    int close_client_connection(peer *client);
    
    // void connectCallback(CallbackFunction cb)
    // {
    //     m_cb = cb;
    // }

    /*worker threads' routine:   deques a message from the queue and calls the callback function with the message as the 
    callback's argument.  Designed to perform heavy action such as write to disk or calculations.   */
    void *work_routine ();

    //Threads spawning functions:
    std::thread spawn() { return std::thread(&TcpServer::init_and_run, this);   }
    std::thread spawn_worker() {  return std::thread(&TcpServer::work_routine, this);  }

    int replayOK(peer *p);
    int handle_received_message(message *message);

    
    //shutdown and signal handlers:
    void shutdown_properly(int code);
    
   
    static void s_signal_handler (int signal_value)
    {
        printf("in signal handler function.  Need to implement signal handling\n");
    }


    static void s_catch_signals (void)
    {
        struct sigaction action;
        action.sa_handler = s_signal_handler;
        action.sa_flags = 0;
        sigemptyset (&action.sa_mask);
        sigaction (SIGINT, &action, NULL);
        sigaction (SIGTERM, &action, NULL);
    }


};


#endif