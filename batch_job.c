#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <unistd.h> 
#include <time.h>

/*
*   Process for job execution
*   takes the second commond line arguments as job execution time
*/
int main(int argc, char *argv[]) {
    /* 
    *   check if input parameter satisfies following: 
    *       1. minmum length  of 2 (if > 2, ignore the following)
    *       2. second parameter can be convert to integer
    */
    if (argc < 2) {
        printf("Missing necessary arguments\n");
        return -1;
    }
    int CPUtime = atoi(argv[1]);
    if (CPUtime  == 0) {
        printf("Incorrect format of time.\n");
        return -1;
    }
    // To actually consume the cpu, here use a while loop to keep track of current time,
    //  until loop for the input duration
    time_t startTime, currentTime;
    time(&startTime);
    time(&currentTime);
    while ((int) (currentTime - startTime) < CPUtime ) {
        time(&currentTime);
    }
    return 0;
}