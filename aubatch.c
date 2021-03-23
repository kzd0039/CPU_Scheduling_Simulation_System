#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include "JobQueue.h"

#define true 1

void *commandLineParser(void *ptr);
void *dispatcher(void *ptr);
int list(char *cmd);
int help(char *cmd);
int run(char *cmd);
int test(char *cmd);
int reschedule(char *cmd);

/*
*  global variables:
*       jobQueue: queue stores all the information of jobs
*       run_dispatch_thread: current number of jobs need to be executed (unfinished jobs)
*       jobQueue_lock: mutex lock of jobQueue
*       dispatcher_threadhold: condition variable for dispatcher_thread
*       command_thread: command_thread
*       dispatcher_thread:  dispatcher_thread
*/
JobQueue jobQueue;
int run_dispatch_thread = 0;
pthread_mutex_t jobQueue_lock;
pthread_cond_t dispatcher_threadhold;
pthread_t command_thread, dispatcher_thread;

/*
*   main function, lauches dispatcher_thread and command_thread
*/
int main(int argc, char* argv[]) {
    printf("welcome to Kun Ding's batch job scheduler Version 1.0\n");
    printf("Type \'help\' to find more about AUbach commands.\n");
    
    /* Two concurrent threads */
    char *message1 = "Command Thread";
    char *message2 = "dDspatcher Thread";
    int  iret1, iret2;

    /* Create two independent threads:command line and dispatcher */
    iret1 = pthread_create(&command_thread, NULL, commandLineParser, (void*) message1);
    iret2 = pthread_create(&dispatcher_thread, NULL, dispatcher, (void*) message2);

    /* Initialize the lock and condition variable */
    pthread_mutex_init(&jobQueue_lock, NULL);
    pthread_cond_init(&dispatcher_threadhold, NULL);
     
    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */
    pthread_join(dispatcher_thread, NULL); 
    pthread_join(command_thread, NULL);
    

    pthread_mutex_destroy(&jobQueue_lock);
    pthread_cond_destroy(&dispatcher_threadhold);

    pthread_exit (NULL);
}

/*
*   Command_thread, keep reading command from user, 
*   call corresponding functions for each command.
*
*   The fuctions may be called are the following:
*       1. fcfs/sjf/priority                -> reschedule(char *cmd)
*       2. list                             -> list(char *cmd)
*       3. run <job> <CPUTime> <priority>   -> run(char *cmd)
*       4. test benchmark                   -> test(char *cmd)
*       5. quit                             -> performanceEvaluate(JobQueue *p);
*       6. help                             -> help(char *cmd)
*/
void *commandLineParser(void *ptr ) {
    /* initialize the job queue */
    JobQueueInit(&jobQueue);    

    /* initialize buffer for user input */
    size_t bufsize = 64;    
    char *buffer = (char*) malloc(bufsize * sizeof(char)); 

    /* start the command line process main loop */
    while (true) {
        /* 
         * 1. read the input from user
         * 2. identify the command according to the first char
         * 3. call corresponding fuctions to do the job
         * 4. if a calling function return -1, which means command error, 
         *     print corresponding message
         */
        printf(">");
        getline(&buffer, &bufsize, stdin);
        char key = buffer[0];
        switch (key) {
            case 'h':
            case 'H':
                if (help(buffer) == -1) {
                    printf("To get help info, please enter \'help\' or \'help\' -[command name]\n");   
                }
                break;

            case 'r':
            case 'R':
                if (run(buffer) == -1) {
                    printf("please provide the required info in following order:\n");
                    printf("run <job> <time> <pri> (<pri> is optional)\n");
                }   
                break;

            case 'l':
            case 'L':
                if (list(buffer) == -1) {
                    printf("To show all the finished jobs, please enter \'list\'.\n");
                }
                break;

            case 'f':
            case 'F':
            case 's':
            case 'S':
            case 'p':
            case 'P':
                if (reschedule(buffer) == -1) {
                    printf("Current available schedule algorithms: \'fcfs\', \'sjf\', \'priority\'\n");
                    printf("To change the algorithms, please enter the name of the algorithm.\n");
                }
                break;
            
            case 't':
            case 'T':
                if (test(buffer) == -1) {
                    printf("please provide the required info in following order:\n");
                    printf("test <benchmark> <policy> <num_of_jobs> <priority_levels>\n");
                    printf("\t<min_CPU_time> <max_CPU_time>\n");
                }
                break;

            case 'q':
            case 'Q':
                //evaluate from the first job, to the last finished job
                performanceEvaluate(&jobQueue, 0, jobQueue.start);
                // terminate two processes
                pthread_cancel(dispatcher_thread);
                pthread_exit(NULL);
                
            default:
                printf("Please enter valid instruction\n");
                printf("Try \'help\' for more information\n");
                break;
        }
    }
}


/*
*   Change the schedule policy according to the input
*   Return 0 if success, -1 if failed
*/
int reschedule(char *cmd) {
    /*  
    *   Compare the input parameter to fcfs, sjf and priority
    *   if find a match, assign the corresponding value to plicy
    *   else return -1
    */
    int policy;
    if (strcmp(cmd, "fcfs\n") == 0) policy = 0;
    else if (strcmp(cmd, "sjf\n") == 0) policy = 1;
    else if (strcmp(cmd, "priority\n") == 0) policy = 2;
    else return -1;

    /*  
    *   lock the queue, and call switchPolicy to reschedule
    *   After that, unlock the queue and singal dispatcher thread to execute job
    */
    pthread_mutex_lock(&jobQueue_lock);

    switchPolicy(&jobQueue, policy);

    pthread_mutex_unlock(&jobQueue_lock);
    pthread_cond_signal(&dispatcher_threadhold); 
    return 0;
}


/*
*   List all the jobs waiting or running in the queue
*/
int list(char *cmd) {
    /*  
    *   Check the if input operation is 'list'
    *   If not return -1, else call jobOnQueue() to display jobs
    */
    if (strcmp(cmd, "list\n") != 0) return -1;
    jobOnQueue(&jobQueue);
    return 0;
}


/*  
*   Add a single job to queue
*/
int run(char *cmd) {
    /* variables used to store the job information */
    char *jobName;
    int CPUtime = 0;
    int priority = 1;

    /* check if the first command is 'run', if not return -1 */
    char *token = strtok(cmd, " ");
    if (strcmp(token, "run") != 0) return -1;

    /* check if second parameter (job name) is provided */
    token = strtok(NULL, " ");
    if (token != NULL) jobName = token;
    else return -1;

    /* check if third parameter (CPUtime) is provided and if it can be convert to int */
    token = strtok(NULL, " ");
    if (token != NULL)  CPUtime = atoi(token);  
    else return -1;
    if (CPUtime == 0) return -1;
    
    /* check if forth parameter (CPUtime) is provided and if it can be convert to int */
    token = strtok(NULL, " ");
    if (token != NULL) priority = atoi(token);  
    // note that this parameter is optional, 
    // so if not provided or not current form, set it to lowest priority
    if (priority == 0) priority = 1; 

    
    /* 
    *   New a job and initialize it with input parameters 
    *   Then lock the queue, call runSingleJon to submit this job
    *   Increment the number of unfinished jobs by one
    *   Unlock the queue and signal dispather thread to execute job
    */
    Job job;
    JobInit(&job, jobName, CPUtime, priority);
    pthread_mutex_lock(&jobQueue_lock);
    int status = runSingleJob(&jobQueue, job);
    run_dispatch_thread += 1;
    pthread_mutex_unlock(&jobQueue_lock);
    pthread_cond_signal(&dispatcher_threadhold); 
    return 0;
}


/*  
*   benchmark test
*/
int test(char *cmd) {
    /*
    *   variables to store input benchmark info
    *   policy: schedule algorithm
    *   info[4]: 0: job number, 1: priority level, 2: min cpu time, 3: max cpu time
    *   name: benchmark name
    */
    int policy = 0;
    int info[] = {0, 0, 0, 0};
    char *name = " ";

    /*
    *   check all the parameters one by one,
    *   if valid, assign it to corresponding variables
    *   else return -1
    */
    char *token = strtok(cmd, " ");
    if (strcmp(token, "test") != 0) return -1;

    if (token != NULL) {
        token = strtok(NULL, " ");
        name = token;
    }
    else return -1;
     
    token = strtok(NULL, " ");
    if (token != NULL) {
        if (strcmp(token, "fcfs") == 0) policy = 0;
        else if (strcmp(token, "sjf") == 0) policy = 1;
        else if (strcmp(token, "priority") == 0) policy = 2;
        else return -1;
    } else return -1;
      
    token = strtok(NULL, " ");
    int i = 0;
    while (token != NULL) {
        if (i == 4) break;
        info[i] = atoi(token);
        token = strtok(NULL, " ");
        i++;
    }
    for (i = 0; i < 4;i++) {
        if (info[i] == 0) return -1;
    }

    /* Lock the queue call runBenchMark() to submit jobs*/
    pthread_mutex_lock(&jobQueue_lock);
    runBenchMark(&jobQueue, info, policy, name);

    /* Increment the number of unfinished jobs, unlock the queue and signal dispatcher thread to execute*/
    run_dispatch_thread += info[0];
    pthread_mutex_unlock(&jobQueue_lock);
    pthread_cond_signal(&dispatcher_threadhold);
    
    return 0;
}


/* 
* dispatcher thread
*/
void *dispatcher(void *ptr) {
    /* start the execution loop */
    while (true) {
        /* wait for queue and also signal to execute job */
        pthread_mutex_lock(&jobQueue_lock);
        while (run_dispatch_thread == 0) {
            pthread_cond_wait(&dispatcher_threadhold, &jobQueue_lock);
        }

        /* get the job on the top of queue */
        Job *job = &(jobQueue.queue[jobQueue.start]);
        /* uptate the index */
        (&jobQueue) -> start  = (&jobQueue) -> start + 1;
        /* decrement the number of unfinished jobs by 1 */
        run_dispatch_thread--;
        /* unlock the queue */
        pthread_mutex_unlock(&jobQueue_lock); 

        // update the status of job
        setJobToRunning(job);
        setStartToExecuteTime(job); 
        /*
        *   Start to execute the job using execv()
        *   And only when the current job is finished, can it start to execute the nexe one
        */
        char executionTime[10];
        char *my_args[3];
        my_args[0] = "batch_job";
        my_args[1] = executionTime;
        my_args[2] = NULL;
        int status = 0;
        pid_t child_pid, wpid;

        sprintf(executionTime, "%d", job -> estimateTime);

        if ((child_pid = fork()) == 0) {
            execv("batch_job", my_args);
        }
        else {
            //wait for the execution of job
            while ((wpid = wait(&status)) > 0);  
        }
        // update the status of job
        setJobToFinish(job);
        setEndTime(job);

        // if the current job matches the latestBenchMarkjob end, do performance evaluation
        if (jobQueue.start == jobQueue.latestBenchMarkEnd) {
            performanceEvaluate(&jobQueue, jobQueue.latestBenchMarkStart, jobQueue.latestBenchMarkEnd);
            printf(">\n");
        }
    }
}


/*  
*   help information, identify specific help type and print relevant message
*/
int help(char *cmd) {
    char *message[] = {
        "run <job> <time> <pri>: submit a job named <job>,\n",
        "                        execution time is <time>,\n",
        "                        priority is <pri>.\n",
        "list: display the job status.\n",
        "fcfs: change the scheduling policy to FCFS.\n",
        "sjf: change the scheduling policy to SJF.\n",
        "priority: change the scheduling policy to priority.\n",
        "test <benchmark> <policy> <num_of_jobs> <priority_levels>\n",
        "     <min_CPU_time> <max_CPU_time>\n",
        "quit: exit AUbatch\n"
    };
    char *token = strtok(cmd, " ");
    if (strcmp(token, "help\n") == 0) {
        for (int i = 0; i < 10; i++) printf("%s", message[i]);
        return 0;
    }
    else if (strcmp(token, "help") != 0) return -1;

    token = strtok(NULL, " ");

    if (strcmp(token, "-run\n") == 0){
        for (int i = 0; i < 3; i++) printf("%s", message[i]);
    }
    else if (strcmp(token, "-list\n") == 0){
        printf("%s", message[3]);
    }
    else if (strcmp(token, "-fcfs\n") == 0){
        printf("%s", message[4]);
    }
    else if (strcmp(token, "-sjf\n") == 0){
        printf("%s", message[5]);
    }
    else if (strcmp(token, "-priority\n") == 0){
        printf("%s", message[6]);
    }
    else if (strcmp(token, "-test\n") == 0){
        printf("%s", message[7]);   
        printf("%s", message[8]);
    }
    else if (strcmp(token, "-quit\n") == 0){
        printf("%s", message[9]);
    }
    else return -1; // if not match, return -1

    return 0;
}