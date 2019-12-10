//  Created by Melih Kurtaran on 19/11/2019.
//  Copyright © 2019 melihkurtaran. All rights reserved.

#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <queue>
#include <semaphore.h>
#include <cstdlib>
#include <ctime>
using namespace std;

#define NUM_THREADS 3
#define MEMORY_SIZE 10

struct Node //linked list for free and allocated memory indexes
{
    int id;
    int size;
    int index; //location of pointed memory
    Node *next, *prev;
    
    Node(int id,int size,int index,Node * next,Node * prev)
    :id(id),size(size),index(index),next(next),prev(prev){}
};

struct Tnode //node for threads
{
    int id;
    int size;
};

queue<Tnode> threadQueue; // shared que
pthread_mutex_t sharedLock = PTHREAD_MUTEX_INITIALIZER; // mutex
sem_t semlist[NUM_THREADS]; // thread semaphores
char  memory[MEMORY_SIZE]; // memory size
pthread_t server; // server thread handle
bool isServerDown = true; //to check server is working or not

//initialization of linked list
Node * head = new Node(-1,MEMORY_SIZE,0,NULL,NULL);

void dump_memory()
{
    cout << "List:" << endl;
    Node * ptr = head;
    cout << "[" << ptr->id << "]" << "[" << ptr->size << "]"
    << "[" << ptr->index << "]";
    ptr = ptr->next;
    while(ptr != NULL)
    {
        cout << "---[" << ptr->id << "]" << "[" << ptr->size << "]"
        << "[" << ptr->index << "]";
        ptr = ptr->next;
    }
    cout << endl << "Memory Dump:" << endl;
    for(int i=0; i < MEMORY_SIZE; i++)
        cout << memory[i];
    cout << endl << "***************************" << endl;
}

void release_function()
{
    //This function will be called whenever the memory is no longer needed.
    for (int i = 0; i < MEMORY_SIZE; i++)  //empty the memory
    {char X = 'X'; memory[i] = X;}
    for(int i = 0; i < NUM_THREADS; i++) //deletes semaphores
    {sem_destroy(&semlist[i]);}
    
    Node * ptr = new Node(0,0,0,NULL,NULL); //deallocate the linked list
    ptr = head;
    while(ptr != NULL)
    {
        delete ptr;
        ptr = ptr->next;
    }
}

void use_mem() //sleep for a while
{
    int sleepingTime = rand() % 5; //time is from 0 to 4
    sleepingTime = sleepingTime + 1; //time is from 1 to 5
    sleep(sleepingTime);
}

void free_mem(int thread_id)
{
    for(int i=0;i<MEMORY_SIZE;i++)
    {
        if(memory[i] == thread_id + 48)
            memory[i] = 'X';
    }
    Node *ptr = head;
    while(ptr!=NULL && (ptr->id != thread_id))
        ptr = ptr->next;
    if(ptr!=NULL)
    {
        ptr->id = -1; //figure 4 case a
        if(ptr->prev && ptr->prev->id == -1 && ptr->next && ptr->next->id == -1)
        /*figure 4 case d*/
        {
            ptr->prev->size += ptr->size + ptr->next->size;
            if(ptr->next->next)
                ptr->next->next->prev = ptr->prev;
            ptr->prev->next =ptr->next->next;
            delete ptr->next;
            delete ptr;
        }
        else if(ptr->prev && ptr->prev->id == -1)
        /*figure 4 case c*/
        {
            ptr->prev->size += ptr->size;
            if(ptr->next)
                ptr->next->prev = ptr->prev;
            ptr->prev->next = ptr->next;
            //delete ptr;
        }
        else if(ptr->next && ptr->next->id == -1)
        /*figure 4 case b*/
        {
            ptr->size += ptr->next->size;
            if(ptr->next->next)
                ptr->next->next->prev = ptr;
            ptr->next = ptr->next->next;
            //delete ptr->next;
        }
    }
}

void my_malloc(int thread_id, int size)
/*This function will add the struct to the queue*/
{
    Tnode * request = new Tnode();
    request->id = thread_id;
    request->size = size;
    threadQueue.push(*request);
}

bool isMemoryAvailable(Tnode *node)
//if memory is available, it also update accordingly
{
    Node * ptr = head;
    while(ptr)
    {
        if(ptr->id == -1 && (node->size <= ptr->size))
        {
            for(int i=ptr->index; i < ptr->index+node->size;i++)
                memory[i] = 48 + node->id; //memory array updated
            //modify the queue accordingly
            Node * toBeInserted = new Node(node->id,node->size,0,ptr,ptr->prev);
            if(ptr->prev)
                toBeInserted->index = ptr->prev->index + ptr->prev->size;
            ptr->index += node->size;
            ptr->size -= node->size;
            if(ptr->prev)
                ptr->prev->next = toBeInserted;
            else
                head = toBeInserted;
            ptr->prev = toBeInserted;
            if(!(ptr->size)){
                ptr->prev->next = ptr->next;
                if(ptr->next)
                    ptr->next->prev = ptr->prev;
                delete ptr;
            }
            
            return true;
        }
        ptr = ptr->next;
    }
    return false;
}

void * memory_server_function(void *)
{
    isServerDown = false;
    time_t startTime = time(NULL);
    Tnode * request = NULL;
    while(true)
    {
        while(!threadQueue.empty())
        {
            pthread_mutex_lock(&sharedLock);    //lock
            request = &threadQueue.front();
            if(isMemoryAvailable(request))
            {
                threadQueue.pop();
                dump_memory(); //access to shared variables
                sem_post(&semlist[request->id]);
            }
            else
            {
                threadQueue.pop();
                threadQueue.push(*request);
            }
            pthread_mutex_unlock(&sharedLock); //unlock
            time_t endTime = time(NULL);
            time_t timeTakenSec = endTime-startTime;
            if(timeTakenSec >= 10) //time is up
            {
                isServerDown = true; //server is down, threads can stop requesting memory
                return 0;
            }
        }
    }
    //This function should grant or decline a thread depending on memory size.
    return 0;
}

void * thread_function(void * id)
{
    int size, * idPointer;
    while(!isServerDown)
    {
        pthread_mutex_lock(&sharedLock);    //lock
        idPointer = (int *) id;
        size = rand() % (MEMORY_SIZE/3); //size is from 0 to memory/size-1
        size = size + 1; //size is from 1 to memory/size
        my_malloc(*idPointer,size);
        pthread_mutex_unlock(&sharedLock); //unlock
        sem_wait(&semlist[*idPointer]);
        if(isServerDown) //if server down do not wait and leave
            break;
        use_mem(); //thread will go to sleep
        
        //since it is a critical region, mutex is used
        pthread_mutex_lock(&sharedLock);    //lock
        free_mem(*idPointer); //thread will free its memory
        pthread_mutex_unlock(&sharedLock); //unlock
    }
    return 0;
}

void init()
{
    for(int i = 0; i < NUM_THREADS; i++) //initialize semaphores
    {sem_init(&semlist[i],0,0);}
    for (int i = 0; i < MEMORY_SIZE; i++)    //initialize memory
      {char X = 'X'; memory[i] = X;}
}

int main() {

    srand(time(NULL)); //to have different random numbers for every trial
    int thread_id[NUM_THREADS];
    for(int i=0; i < NUM_THREADS; i++)
        thread_id[i] = i;
    
    init(); //initiliaze memory and start the server thread
    pthread_create(&server,NULL,memory_server_function,NULL); //start server
    pthread_t threads[NUM_THREADS];
    for(int i=0; i < NUM_THREADS; i++)
        pthread_create(&threads[i],NULL,thread_function,(void *) &thread_id[i]);
    
    pthread_join(server,NULL); //join server with the main thread
    for(int i = 0; i < NUM_THREADS; i++) //server is down, all semaphore can be up
        sem_post(&semlist[i]);
    for(int i=0; i < NUM_THREADS; i++)
            pthread_join(threads[i],NULL);
    release_function();
    cout << endl << "Time is up!" << endl;
    return 0;
}

