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

#define NUM_THREADS 5
#define MEMORY_SIZE 1000

struct node
{
    int id;
    int size;
};


queue<node> myqueue; // shared que
pthread_mutex_t sharedLock = PTHREAD_MUTEX_INITIALIZER; // mutex
pthread_t server; // server thread handle
sem_t semlist[NUM_THREADS]; // thread semaphores

int thread_message[NUM_THREADS]; // thread memory information
char  memory[MEMORY_SIZE]; // memory size

int emptyMemoryAddressPointer = 0; //points to empty memory address
int finished = 0; //number of finished threads

void release_function()
{
    for (int i = 0; i < MEMORY_SIZE; i++)  //empty the memory
    {char zero = '0'; memory[i] = zero;}
    for(int i = 0; i < NUM_THREADS; i++) //deletes semaphores
    {sem_destroy(&semlist[i]);}
    //This function will be called
    //whenever the memory is no longer needed.
    //It will kill all the threads and deallocate all the data structures.
}

void my_malloc(int thread_id, int size)
{
    node * request = new node();
    request->id = thread_id;
    request->size = size;
    myqueue.push(*request);
    //This function will add the struct to the queue
    
}

void * server_function(void *)
{
    while(true)
    {
        while(!myqueue.empty())
        {
            node * request = new node();
            request = &myqueue.front();
            myqueue.pop();
            if(MEMORY_SIZE - emptyMemoryAddressPointer < request->size)
                thread_message[request->id] = -1;
            else
                thread_message[request->id] = emptyMemoryAddressPointer;
            sem_post(&semlist[request->id]);
        }
        if(finished == NUM_THREADS)
            break;
    }
    //This function should grant or decline a thread depending on memory size.
    return 0;
}

void * thread_function(void * id)
{
    pthread_mutex_lock(&sharedLock);    //lock
    int * idPointer = (int *) id;
    int size = rand() % 1000;
    my_malloc(*idPointer,size);
    sem_wait(&semlist[*idPointer]);
    if(thread_message[*idPointer] == -1)
        cout << "Thread " << *idPointer << ": Not enough memory" << endl;
    else
    {
        for(int i=emptyMemoryAddressPointer; i < size + emptyMemoryAddressPointer; i++)
            memory[i] = '1';
        emptyMemoryAddressPointer += size;
    }
    pthread_mutex_unlock(&sharedLock); //unlock
    //This function will create a random size, and call my_malloc
    //Block
    //Then fill the memory with 1's or give an error prompt
    return 0;
}

void init()
{
    pthread_mutex_lock(&sharedLock);    //lock
    for(int i = 0; i < NUM_THREADS; i++) //initialize semaphores
    {sem_init(&semlist[i],0,0);}
    for (int i = 0; i < MEMORY_SIZE; i++)    //initialize memory
      {char zero = '0'; memory[i] = zero;}
       pthread_create(&server,NULL,server_function,NULL); //start server
    pthread_mutex_unlock(&sharedLock); //unlock
}



void dump_memory()
{
 // You need to print the whole memory array here.
    cout << "Memory Dump:" << endl;
    for(int i=0; i < MEMORY_SIZE; i++)
        cout << memory[i];
}

int main (int argc, char *argv[])
 {
     int thread_id[NUM_THREADS];
     for(int i=0; i < NUM_THREADS; i++)
         thread_id[i] = i;
     //You need to create a thread ID array here

     init();    // call init

     srand(time(NULL)); //to have different random numbers for every trial
     
     //You need to create threads with using thread ID array, using pthread_create()
     pthread_t threads[NUM_THREADS];
     for(int i=0; i < NUM_THREADS; i++)
         pthread_create(&threads[i],NULL,thread_function,(void *) &thread_id[i]);
     
     //You need to join the threads
     pthread_mutex_lock(&sharedLock);    //lock
     finished++;
     pthread_mutex_unlock(&sharedLock); //unlock
     for(int i=0; i < NUM_THREADS; i++)
         pthread_join(threads[i],NULL);
     
     dump_memory(); // this will print out the memory
     printf("\nMemory Indexes:\n" );
     for (int i = 0; i < NUM_THREADS; i++)
     {
         printf("[%d]" ,thread_message[i]); // this will print out the memory indexes
     }
     printf("\nTerminating...\n");
     
     release_function();
     
     return 0;
 }
