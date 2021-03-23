#include <time.h>
/*
*   Job structure
*/
typedef struct{
    char name[25];      /* name of job */
    int order;          /* the order comming in */
    int estimateTime;   /* estiated CPT time*/
    int priority;       /* priority */
    int runningState;   /* 0: waiting, 1: running, 2: finished */
    time_t startToExecuteTime;  /* time when start to execute */
    time_t arrivalTime;         /* time when the job is submitted */    
    time_t endTime;             /* time when the job is end */
}Job;

/*
*   initialize the job with name, CPUtime and priority
*   the running state of the job is default to 0 (waiting)
*/
void JobInit(Job *p, char *inputName, int inputTime, int inputPriority) {
    strcpy(p -> name, inputName);
    p -> estimateTime = inputTime;
    p -> priority = inputPriority;
    p -> runningState = 0;
}

/*
*   set the order of job to the given order
*/
void setOrder(Job *p, int inputOrder) {
    p -> order = inputOrder;
}

/*
*   set the job submit time to current local time
*/
void setArrivalTime(Job *p) {
    time(&(p -> arrivalTime));
}

/*
*   set the job start to execute time to current local time
*/
void setStartToExecuteTime(Job *p) {
    time(&(p -> startToExecuteTime));
}

/*
*   set the job finished time to current local time
*/
void setEndTime(Job *p) {
    time(&(p -> endTime));
}

/*
*   set the job running state to run
*/
void setJobToRunning(Job *p) {
    p -> runningState = 1;
}

/*
*   set the job running state to finish
*/
void setJobToFinish(Job *p) {
    p -> runningState = 2;
}
