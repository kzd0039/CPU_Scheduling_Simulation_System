#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <unistd.h> 
#include <pthread.h> 
#include <math.h>
#include "Job.h"
/*
*   JobQueue struct
*/
typedef struct{
    Job queue[500];         /* job array */
    int schedulePolicy;     /* current schedule algorithm. 0: fcfs, 1: sjf, 2: priority */
    int start;              /* start index of unfinished jobs */
    int end;                /* index where new job being addeded */
    int maxSize;            /* masSize of queue, which is 500 */
    int latestBenchMarkStart;   /* start index of benchmarkjob */
    int latestBenchMarkEnd;     /* end index of benchmarkjob */
    time_t randomSeed;      /* used for the rand() */
}JobQueue;

void JobQueueInit(JobQueue *p);
void schedule(JobQueue *p);
int addSingleJob(JobQueue *p, Job job);
void showSingleJobInfo(JobQueue *p, int index);
void clearQueue(JobQueue *p);
int runSingleJob(JobQueue *p, Job job);
void switchPolicy(JobQueue *p, int newPolicy);
int randInRange(int lower, int upper);
double calculateSD(int *array, int len);
void performanceEvaluate(JobQueue *p, int left, int right);
int runBenchMark(JobQueue *p, int *info, int policy, char* name);
int jobOnQueue(JobQueue *p);

/*
*   initialize the jobQueue
*   set the schedulepolicy, start, end, maxSize to default value
*   set the latestBenchMarkEnd and latestBenchMarkStart to -1(no benchmark jobs when initialized)
*   assign the local time when create the jobQueue to randomSeed
*/
void JobQueueInit(JobQueue *p) {
    p -> schedulePolicy = 0;
    p -> start = 0;
    p -> end = 0;
    p -> maxSize = 500;
    p -> latestBenchMarkEnd = -1;
    p -> latestBenchMarkStart = -1;
    time(&(p -> randomSeed));
}

/*
*   schedule the job queue according to current policy
*/
void schedule(JobQueue *p) {
    /* only sort the jobs that haven't being executed */
    int left = p -> start;
    int end = p -> end;
    /* get the policy */
    int policy = p -> schedulePolicy;
    /* if no job need to be sorted, return */
    if (left == end) return;

    // printf("------------------------------------------------------------------------\n");
    // printf("jobs before schedule:\n");
    // jobOnQueue(p);

    /* 
    *   bubble sort the queue
    *   if fcfs or sjf, sort in assending order
    *   if priority, sort in dessending order
    */

    // fcfs
    if (policy == 0) {
        for (int j = end; j > left; j--) {
            for (int i = left; i < j - 1; i++) {
                if ((p -> queue)[i].order > (p -> queue)[i+1].order) {
                    Job temp = (p -> queue)[i];
                    (p -> queue)[i] = (p -> queue)[i+1];
                    (p -> queue)[i+1] = temp;
                }
            }
        }
    }
    // SJF
    else if (policy == 1) {
        for (int j = end; j > left; j--) {
            for (int i = left; i < j - 1; i++) {
                if ((p -> queue)[i].estimateTime > (p -> queue)[i+1].estimateTime) {
                    Job temp = (p -> queue)[i];
                    (p -> queue)[i] = (p -> queue)[i+1];
                    (p -> queue)[i+1] = temp;
                }
            }
        }
    }
    // Priority
    else {
        for (int j = end; j > left; j--) {
            for (int i = left; i < j - 1; i++) {
                if ((p -> queue)[i].priority < (p -> queue)[i+1].priority) {
                    Job temp = (p -> queue)[i];
                    (p -> queue)[i] = (p -> queue)[i+1];
                    (p -> queue)[i+1] = temp;
                }
            }
        }
    }
    // printf("------------------------------------------------------------------------\n");
    // printf("jobs after schedule:\n");
    // jobOnQueue(p);
    // printf("------------------------------------------------------------------------\n");
}


/*
*   reset the queue by reset the parameters
*/
void clearQueue(JobQueue *p) {
    p -> schedulePolicy = 0;
    p -> start = 0;
    p -> end = 0;
}

/*
*   add one single job
*   return the order of this job
*/
int addSingleJob(JobQueue *p, Job job) {
    /* if reaches the max size, print the warning and clear the queue*/
    if (p -> end == p -> maxSize) {
        printf("Warning! Reaching the max capacity of the queue, queue will be automatically cleared!");
        clearQueue(p);
    }

    /* add the job to end, set the arrival time and order, then update end index*/
    int index = p -> end;
    setArrivalTime(&job);
    setOrder(&job, index);
    (p -> queue)[index] = job;
    p -> end = index + 1;
    return index;
}

/*
*   print the info of a specific job
*/
void showSingleJobInfo(JobQueue *p, int index) {
    /*
    *   in the waiting jobs, search for the specific job according to the order
    *   print the job info and calculate the total CPU time of jobs before it
    */

    char *policies[] = {"FCFS", "SJF", "PRIORITY"};
    int left = p -> start;
    int right = p -> end;
    int estimateWaitTime = 0;
    /* 
    *   Important here!
    *   If there's job running here, add the remaining time of that job to estimateWaitTime
    */
    if (left - 1 >= 0 && (p -> queue)[left - 1].runningState == 1) {
        int remainingTime = (int) (time(NULL) - (p -> queue)[left - 1].startToExecuteTime);
        estimateWaitTime += (p -> queue)[left - 1].estimateTime - remainingTime;
    }
    for (int i = left; i < right; i++) {
        if (i == index) break;
        estimateWaitTime += (p -> queue)[i].estimateTime;
    }
    Job job = (p -> queue)[index];
    printf("Job %s was submitted.\n", job.name);
    printf("Total number of jobs in the queue: %d\n", right - left);
    printf("Expected waiting time: %d seconds\n",estimateWaitTime);
    printf("Schedule Policy: %s.\n", policies[p -> schedulePolicy]);
    
}

/*
*   run a single by adding the job to queue first
*   then reschedule the queue and show the job infor and waitting time
*/
int runSingleJob(JobQueue *p, Job job) {
    int index = addSingleJob(p, job);
    schedule(p);
    showSingleJobInfo(p, index);
    return 0;
}

/*
*   change the scheule algorithm and rescedule the queue
*/
void switchPolicy(JobQueue *p, int newPolicy) {
    char *policies[] = {"FCFS", "SJF", "PRIORITY"};
    p -> schedulePolicy = newPolicy;
    schedule(p);
    printf("Scheduling policy is switched to %s. All the %d waiting jobs has been rescheduled\n",
            policies[newPolicy], p -> end - p -> start);
}


/*
*   get a random number in the given range
*/
int randInRange(int lower, int upper) {
    return (rand() % (upper - lower + 1)) + lower;
}

/*
*   calculate the standard deviation of the first len elements of input array, 
*/
double calculateSD(int *array, int len) {
    double sum = 0, mean, SD = 0.0;
    for (int i = 0; i < len; i++) {
        sum += array[i];
    }
    mean = sum / len;
    for (int i = 0; i < len; i++) {
        SD += pow(array[i] - mean, 2);
    }
    return sqrt(SD / len);
}

/*
*   evaluate the finished jobs in the given range [left, right)
*/
void performanceEvaluate(JobQueue *p, int left, int right) {
    
    int numOfJobs = right - left;
    int responsetimes[numOfJobs];

    double sumCPUtime = 0;
    double sumTurnaroundTime = 0;
    double sumWaitTime = 0;

    /* ititialize all the following variables as the first job */
    double maxResponseTime = (int) ((p -> queue)[0].startToExecuteTime - (p -> queue)[0].arrivalTime);
    double minResponseTime = maxResponseTime;

    /* iterate the jobs, update the variables */
    for (int i = left; i < right; i++) {
        Job job = (p -> queue)[i];
        /* 
        *   it is important to check if the job is still running, 
        *   because the running job desn't has finish time, which will cause error.
        *   If still running, skip it (actually it will only happen at index start)
        */
        if (job.runningState == 1) {
            numOfJobs--;
            continue;
        }
        sumCPUtime += (int) (job.endTime - job.startToExecuteTime);
        int waitTime = (int) (job.startToExecuteTime - job.arrivalTime);
        responsetimes[i - left] = waitTime;
        sumWaitTime += waitTime;
        sumTurnaroundTime += (int) (job.endTime - job.arrivalTime);
        if (waitTime > maxResponseTime) maxResponseTime = waitTime;
        if (waitTime < minResponseTime) minResponseTime = waitTime;
    }

    if (numOfJobs == 0) {
        printf("No job finished so far.\n");
        return;
    }
    printf("------------------------------------------------------------------------\n");
    printf("Total number of job submitted:    %d\n", numOfJobs);
    printf("Average turnaround time:          %.2f seconds\n", sumTurnaroundTime / numOfJobs);
    printf("Average CPU time:                 %.2f seconds\n", sumCPUtime / numOfJobs);
    printf("Average waiting time:             %.2f seconds\n", sumWaitTime / numOfJobs);
    printf("Throughput:                       %.3f No./second\n", numOfJobs / sumCPUtime);
    printf("Maximum response time:            %.2f seconds\n", maxResponseTime);
    printf("Minimum response time:            %.2f seconds\n", minResponseTime);
    printf("Response time standard deviation: %.2f\n", calculateSD(responsetimes, numOfJobs));
    printf("------------------------------------------------------------------------\n");
}


/*
*   add benchmark jobs
*   return the starting index of these benchmark jobs, which will be used in performanceEvaluate()
*/
int runBenchMark(JobQueue *p, int *info, int policy, char* name) {
    /* By default, this operation will clear the queue */
    if (p -> end > p -> start) {
        printf("Warning! run benchmark will clear all the jobs on the queue!\n");
        p -> end = p -> start;
    }
    /* set the range of bennchmark job */
    p -> latestBenchMarkStart = p -> end;
    p -> latestBenchMarkEnd =  p -> end + info[0];
  
    int CPUtime = 0;
    int priority = 0;
    p -> schedulePolicy = policy;

    /* set the randomSeed to predefined value to make sure it generates same sequence */
    srand(p -> randomSeed);

    /* new and add jobs */
    for (int i = 0; i < info[0];i++) {
        Job job;
        char jobName[25];
        snprintf(jobName, 25, "%s%d", name, i);
        CPUtime = randInRange(info[2], info[3]);
        priority = randInRange(1, info[1]);
        JobInit(&job, jobName, CPUtime, priority);
        addSingleJob(p, job);
    }

    /* reschedule the queue */
    schedule(p);
    printf("Succesfully launch benchmark, wait for executing and evaluating...\n");
    return 0;
}


/*
*   print the information of the jobs running and waiting in the queue
*/
int jobOnQueue(JobQueue *p) {
    char *policies[] = {"FCFS", "SJF", "PRIORITY"};
    int left = p -> start;
    /* 
    * the jobs that are waiting within start and end
    * However, the job at start - 1 may still running, we should check this possibility
    */
    if (left - 1 > -1 && (p -> queue)[left-1].runningState == 1) left--;
    int right = p -> end;
    printf("Total number of jobs in the queue: %d\n", right - left);
    printf("Scheduling Policy: %s\n", policies[p -> schedulePolicy]);
    printf("%-16s%-12s%-6s%-15s%-12s\n", "Name", "CPU_Time", "Pri", "Arrival_time", "Progress");
    
    /* print the job info */
    for (int i = left; i < right; i++) {
        Job job = (p -> queue)[i];
        char *status = "waiting";
        if (job.runningState == 1) status = "run";
        /* convert time from seconds to hour:min:second */
        struct tm *local = localtime(&(job.arrivalTime));
        printf("%-16s%-12d%-6d%02d:%02d:%02d%-7s%-12s\n", 
                job.name,
                job.estimateTime,
                job.priority,
                local -> tm_hour,
                local -> tm_min,
                local -> tm_sec,
                " ",
                status);
    }
    return 0;
}

