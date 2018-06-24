#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr
#include "message.h"
#include <errno.h>
//#include <sys/types.h>
//#include <netinet/in.h>
//#include <sys/time.h>
#include <unistd.h>  

#define SERVER_PORT 41250 

void print_Message(message *m)
{
    //printf("server's message length = %d\n", m->length -1);
    printf("%.*s\n", m->length - 1, m->data);
    //printf("server's message length = %u\n", m->length -1);
    //printf("%s\n", m->data);
}

int recv_protocol_message(int socket, message* m)
{
    int total_bytes_received = 0;
    int num_bytes_recv_now;
    unsigned char msg_length = sizeof(message) - 1; //until we find out the real length
    int num_bytes_left = msg_length;
    bool updated = false;
    
    while (total_bytes_received < msg_length)
    {
        //num_bytes_recv_now = recv(socket, (m + total_bytes_received), num_bytes_left, 0);
        num_bytes_recv_now = recv(socket, (char*)m + total_bytes_received, num_bytes_left, 0);
        //printf("num_bytes_recv_now =  %d\n", num_bytes_recv_now);
        if (num_bytes_recv_now < 0) 
        { // error
            printf("Error in recv function: %s\n", strerror(errno));
            return -1;
        } else if (num_bytes_recv_now == 0) {  // other side performed a shutdown
            return 0;
        }
        total_bytes_received += num_bytes_recv_now;
        if(!updated)
        {
            msg_length = m->length;
            num_bytes_left = msg_length - total_bytes_received;
            updated = true;
            continue;
        }
        
        num_bytes_left -= num_bytes_recv_now;
    }
    //printf("received %d bytes\n", total_bytes_received);
    return total_bytes_received;
}


int send_protocol_message(int socket, unsigned char msg_length, char *data)
{
    if(msg_length > MESSAGE_SIZE)
    {
        printf("Messages are limited to %d bytes!  Message not sent.\n", MESSAGE_SIZE);
        return -1;
    }

    int num_bytes_left = msg_length;
    int total_bytes_sent = 0;
    int num_bytes_sent_now = 0;
    message m;
    m.length = msg_length;
    memcpy(m.data, data, m.length - 1);   

    while (total_bytes_sent < msg_length)
    {
        num_bytes_sent_now = send(socket, (char*)(&m) +total_bytes_sent, num_bytes_left, 0);
        if (num_bytes_sent_now < 0){
            printf("Error in send function: %s\n", strerror(errno));
            return -1;
        }
        total_bytes_sent += num_bytes_sent_now;
        num_bytes_left -= num_bytes_sent_now;
    }
      
    return 0;
}


int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char message_buffer[MESSAGE_SIZE];
    message server_reply;
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( SERVER_PORT );
 
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
     
    puts("Connected\n");
     
    //keep communicating with server
    while(1)
    {
        memset(message_buffer, 0, MESSAGE_SIZE);
        //memset(server_reply, 0, MESSAGE_SIZE);
        printf("Enter message : ");
        //scanf("%s" , message_buffer);
        gets(message_buffer);

        if(send_protocol_message(sock, strlen(message_buffer) + 1 , message_buffer) < 0 )
        {
            puts("Send failed");
            return 1;
        }
         
        //Receive a reply from the server - first char is the length,  so need to make a receive function.
        if( recv_protocol_message(sock, &server_reply) < 0)
        //if( recv(sock , server_reply , MESSAGE_SIZE , 0) < 0)
        {
            puts("recv failed");
            break;
        }
         
        puts("\nServer reply :");
        print_Message(&server_reply);
    }
     
    close(sock);
    return 0;
}