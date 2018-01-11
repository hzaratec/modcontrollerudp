#ifndef MONITOR_H_
#define MONITOR_H_

#include <sys/time.h>
#include <sched.h>
#include <queue>
#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

//constants with maximum sizes for the various sets (implemented by using static arrays for simplicity since stored in a shared memory)
#define MAX_TICKS_SIZE 100
#define MAX_NUM_OF_APPLS 128
#define MAX_NUM_OF_CPU_SETS 32
#define MAX_NAME_SIZE 100
#define DEFAULT_TIME_WINDOW 2
#define DEFAULT_CONTROLLER_NAME "controller"

/*
 * tick_t type is used to collect heartbeat
 */
typedef struct {
  long long   value; //number of accumulated ticks
  long double time;  //since the start time of the application
} tick_t;

/*
 * data_t type is used to specify the data (saved in the shared memory) exchanged between the controller and the application during the execution (performance data and actuation values).
 * A segment in the shared memory is created for each application
 */
typedef struct {
  int         segmentId;             // id of the memory segment where this structure is stored
  long double startTime;             // logged start time of the application
  //monitoring
  tick_t      ticks[MAX_TICKS_SIZE]; // vector of ticks. Actually this vector is used as a sliding window for efficiency reasons. therefore we have following pointers
  int         curr;                  // "pointer" to the current position where the last tick has been saved
  int         TPwindow;              // time window (in seconds) to be used to compute the throughput
  //actuation
  bool         useGPU;               // GPU/notCPU
  int          precisionLevel;            //100:exact computation; <100 approximate computation. it represents the percentage of approximation
} data_t;

/*
 * appl_t type is used as a descriptor of an application.
 */
typedef struct {
  pid_t       pid;                   // application pid
  char        name[MAX_NAME_SIZE+1]; // name of the application
  bool        gpuImpl;               // has a GPU implementation or not
  int         segmentId;             // id of the memory segment containing the corresponding data_t structure
  long double tpMinObj;              // minimum throughput to be guaranteed (if equal to 0, it is not set)
  long double tpMaxObj;              // maximum throughput to be guaranteed (if equal to 0, it is not set)
  bool        alreadyInit;    // states if CGroups have been already initialized by the controller for the application or not
  //this last field is used to understand if cgroups have been already configured for each registered application.
  //indeed we don't have a way for an application to signal the controller on the registration.
  //therefore the controller use to check periodically the application table (i.e. the monitor structure in the shared memory)
  //to detect new applications and configure them
  //DO NOTE: according to this asynchronous scheme it is necessary to check for new applications with a quite high frequency (1 sec);
  //then we should run the resource management policy with a lower frequency
} appl_t;

/*
 * monitor_t type is used to contain the status of the system (regarding to the running applications).
 * a single data structure is allocated for a controller and it is stored in a shared memory (just because in this way the transmission of the appl_t descriptor is easier).
 * TODO this may be refactored in some more efficient way
 */
typedef struct {
  //set of monitored applications
  appl_t  appls[MAX_NUM_OF_APPLS];
  int     nAttached; //current number of monitored applications

  //set of applications that have been detached recently. necessary to update above appls set
  pid_t   detached[MAX_NUM_OF_APPLS];
  int     nDetached; //current number of detached applications
} monitor_t;

//semaphore management, data structure and wait/post functions.
//The semaphore is used to access the monotor_t data structure in the shared memory since both the applications and the controller may write it
union semun {
  int                val;
  struct semid_ds    *buf;
  unsigned short int *array;
  struct seminfo     *__buf;
};

void binarySemaphoreWait(int);
void binarySemaphorePost(int);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//PAY ATTENTION: first set of functions can be used only in the application code, the second set only in the controller code
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//functions used in the application code
//attach the current application on the monitor
data_t* monitorAttach(char const*, long double, long double, bool, int = DEFAULT_TIME_WINDOW);
//Detach the application from the monitor
int monitorDetach(data_t*);
//Send a tick to the monitor which the application is connected
void monitorTick(data_t*, int value = 1);
//Get the current device to be used true=GPU/false=CPU
bool useGPU(data_t*);
//Get the current precision level to be used (100 maximum - 0 minimum)
int usePrecisionLevel(data_t*);
//get the controller pid given its name (usually "main")
pid_t getMonitorPid(const char*);

//functions used in the controller code
//initialize and destroy monitor_t data structure
monitor_t* monitorInit();
void monitorDestroy(monitor_t*);
//read (copy!) data_t values from the shared memory given a segment id
data_t monitorRead(int);
//get a reference of the shared memory given a segment id
data_t* monitorPtrRead (int);

//get current and global throughput
double getGlobalThroughput(data_t*);
double getCurrThroughput(data_t*);
void setUseGPU(data_t*, bool);
void setPrecisionLevel(data_t*, int);

//print the status of the attached applications
void printAttachedApplications(monitor_t*);
std::vector<pid_t> updateAttachedApplications(monitor_t*);
void killAttachedApplications(monitor_t*);

#endif /* MONITOR_H_ */
