#include "TcpServer.h"
#include <thread>
#include <iostream>

void handle_received_message1(message *m)
{
      printf("In the callback adjusted in the c'tor and the message dequeued is:\n");
      print_message(m);
   
}


 void print_inMainThread_message(message* m)
 {
 	std::cout <<" in main thread's and the messaged dequeued is" << std::endl;
 	print_message(m);
 }



int main()  
{
    //Create the server and run it
    TcpServer s(41250, handle_received_message1);
   
    //Use public function to dequeue messages
    message messageToDequeue = s.TcpServer::dequeueMessage();
    print_inMainThread_message(&messageToDequeue);

  
    std::cout<<"Exit of Main function"<<std::endl;
    return 0;
}

