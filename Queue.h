#ifndef QUEUE_H_
#define QUEUE_H_

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

template <typename T> class Queue
{
private:
    struct Node{
    Node *next;
    T data;
    };

    Node *head;
    Node *tail;
    int queue_size;
    pthread_mutex_t Queue_Lock;
    pthread_cond_t isEmpty; 


public:
  // int initialize_Queue();
  // int enqueue_data (T* newData);
  // int dequeue_data ();
  // int destroy_queue();

//returns 0 if succeeds,  and error number if failed
int initialize_Queue()
{
  
  queue_size = 0;           
  head = NULL;
  tail = NULL;
  
  int ret_val = pthread_mutex_init( &Queue_Lock, NULL );
  if(ret_val !=0){
      return ret_val;  
  }
  ret_val = pthread_cond_init (&isEmpty , NULL);
  if(ret_val !=0){
      pthread_mutex_destroy(&Queue_Lock);
      return ret_val;  
  }
  return 0; 
}


//returns 0 on success, and error number on failure
int enqueue_data (T newData)
{
    Node *newNode = (Node*)malloc(sizeof(Node));
    if(newNode == NULL){
	return -1;
    }
    newNode->data = newData;
    newNode -> next = NULL;
    
    int lock_val = pthread_mutex_lock(&Queue_Lock);
    if (lock_val !=0 )
    {
      
    	free(newNode);
    	return lock_val;
    }
    
    
    if(queue_size == 0){
      head = newNode;
      tail = newNode;
    }
    else{ 
      Node *tempNode = tail;
      tempNode -> next = newNode;
      tail = newNode;
    }
    
    ++queue_size;
    int sig_ret = pthread_cond_signal (&isEmpty);
    
    if(sig_ret !=0){
      pthread_mutex_unlock (&Queue_Lock);
      return sig_ret;
    }
    int unlock_ret = pthread_mutex_unlock (&Queue_Lock);
    if( unlock_ret !=0)
      return unlock_ret;
    
    return 0;
}


//returns 0 in case of success. Returns error number in case of failure
int dequeue_data (T* putData)
{
  int lock_val = pthread_mutex_lock(&Queue_Lock);
  if(lock_val !=0 )
    return lock_val;
  //while queue is empty: wait
  int wait_val;
  while(queue_size == 0 ){
      wait_val = pthread_cond_wait( &isEmpty, &Queue_Lock);
      if(wait_val != 0){
	pthread_mutex_unlock( &Queue_Lock);  //no need to check return value since we return the error of pthread_cond_wait
	return wait_val;
      }
  }
  
  //queue isn't empty now. 
  Node *firstNode = head;
  head = firstNode -> next;
  if( head == NULL)
     tail = NULL;
  --queue_size;
  
  int unlock_val = pthread_mutex_unlock( &Queue_Lock);
  *putData = firstNode->data;
  
  free(firstNode);
  return unlock_val;  //in case of success pthread_mutex_unlock returns 0.  Therefore, in case of success this function returns 0. 
  
}


//if the user allocated queue_ptr dynamically,  it's the user's responsiblity to free it
//returns 0 on success,  and error number on failure
//initiaze, destroy functions are not thread safe.  we can assume that no other thread will call another queue function in parallel.  Therefore, we do not need to lock.
int destroy_queue()
{
    
    Node *currNode = head;
    Node *nextNode = head;
 
    while( currNode != NULL)
    {
      
	nextNode = currNode -> next;
    //free(currNode->data);
	free(currNode);
	currNode = nextNode;
    }
    
    int mutex_val = pthread_mutex_destroy( &Queue_Lock );
    if(mutex_val != 0){  
	return mutex_val;
     }
    
    int cond_ret = pthread_cond_destroy( &isEmpty );
    if(cond_ret != 0){  
      return cond_ret;
    }
    
    return 0;    
}



};


#endif