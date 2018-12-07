/* This file will contain your solution. Modify it as you wish. */
#include <types.h>
#include "producerconsumer_driver.h"
#include "opt-synchprobs.h"
#include <lib.h>    /* for kprintf */
#include <synch.h>  /* for P(), V(), sem_* */
#include <test.h>

/* Declare any variables you need here to keep track of and
   synchronise your bounded. A sample declaration of a buffer is shown
   below. You can change this if you choose another implementation. */

static struct pc_data buffer[BUFFER_SIZE];

//control access to producer_send()
static struct semaphore *producer_sem;       

// control access to consumer_receiver()
static struct semaphore *consumer_sem; 

//mutual exclusive lock to control access to buffer and index      
static struct lock *mutex; 


//starting and ending index of buffer
static int start, end;            


/* consumer_receive() is called by a consumer to request more data. It
   should block on a sync primitive if no data is available in your
   buffer. */

struct pc_data consumer_receive(void)
{
        struct pc_data thedata;

        //decrease the number of spots occupied 
        P(consumer_sem);

        //enter critical region and get lock
        lock_acquire(mutex);
        
        //get the item from the start of the buffer
        thedata = buffer[start];

        //compute the new starting index
        start++;
        start = start % BUFFER_SIZE;

        //exit critical region and release lock
        lock_release(mutex);

        //increase the number of free spots 
        V(producer_sem);
        return thedata;
}

/* procucer_send() is called by a producer to store data in your
   bounded buffer. */

void producer_send(struct pc_data item)
{
        //decrement the number of free spots
        P(producer_sem);
        
        // enter critical region and decrease semaphore
        lock_acquire(mutex);     

        //compute the new ending index for the buffer
        end++;
        end = end % BUFFER_SIZE;

        //insert new item into the buffer with the new ending index.
        buffer[end] = item; 

        //exit critical region and increase semaphore
        lock_release(mutex);

        //increment the number of spots occupied 
        V(consumer_sem);
}


/* Perform any initialisation (e.g. of global data) you need
   here. Note: You can panic if any allocation fails during setup */

void producerconsumer_startup(void)
{
        //initialise index for the start of buffer and end of buffer. 
        end = BUFFER_SIZE-1;
        start = 0;

        mutex = lock_create("mutex");
        if(mutex == NULL){
                panic("mutex creation failed");
        }
        
        producer_sem = sem_create("producer_sem", BUFFER_SIZE);
        if (producer_sem == NULL){
                panic("producer_sem creation failed");
        }

        consumer_sem = sem_create("consumer_sem", 0);
        if (consumer_sem == NULL){
                panic("consumer_sem creation failed");
        }

}

/* Perform any clean-up you need here */
void producerconsumer_shutdown(void)
{
        sem_destroy(producer_sem);
        sem_destroy(consumer_sem);
        lock_destroy(mutex);
        
}

