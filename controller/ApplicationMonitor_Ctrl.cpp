#include "ApplicationMonitor.h"
#include <fstream>
#include <iostream>
#include <queue>
#include <signal.h>

monitor_t* monitorInit () {
  //initialize shared memory and the companion semaphore for the controller 
  //(semaphore is used for mutually exclusive access between applications and controller)
  int semid = semget (getpid(), 1, IPC_CREAT | IPC_EXCL | 0666);
  union semun argument;
  unsigned short values[1];
  values[0] = 1;
  argument.array = values;
  int semval = semctl (semid, 0, SETALL, argument);
  int shmid = shmget(getpid(), sizeof(monitor_t), IPC_CREAT | IPC_EXCL | 0666);
  monitor_t* monitor = (monitor_t*) shmat(shmid, 0, 0);
  if (semid == -1 || semval == -1 || shmid == -1 || monitor==(void*)-1){
    std::cout << "Something wrong happened during shared memory initialization" << std::endl;
    exit(EXIT_FAILURE); 
  }

  //initialize counters of applications
  monitor->nAttached = 0;
  monitor->nDetached = 0;
      
  return monitor;
}

void monitorDestroy (monitor_t* monitor) {
  //destroy shared memory containing the monitor data structure and related semaphore. if some crash happens (I don't think so) it may be 
  //because taskset data structures are stored in the shared memory and sched_setaffinity tries to access a destroied memory location. 
  //in that case try to find a solution...
  shmdt(monitor);
  int shmid = shmget (getpid(), sizeof(monitor_t), 0);
  int semid = semget (getpid(), 1, 0);
  int semval1 = shmctl (shmid, IPC_RMID, 0);
  union semun ignored_argument;
  int semval2 = semctl (semid, 1, IPC_RMID, ignored_argument);
  if (shmid == -1 || semid == -1 || semval1 == -1 || semval2 == -1){
    std::cout << "Something wrong happened during shared memory initialization" << std::endl;
    exit(EXIT_FAILURE); 
  }
}

data_t monitorRead (int segmentId) {
  //return data_t structure
  data_t* shm = (data_t*) shmat(segmentId, 0, 0); //attach shared memory
  if(shm == (void*) -1){ 
    std::cout << "Something wrong happened during shared memory access!" << std::endl;
  }
  data_t retval;
  memcpy(&retval, shm, sizeof (data_t));
  int ret1 = shmdt(shm); //detach shared memory
  if(ret1 == -1){ 
    std::cout << "Something wrong happened during shared memory detach!" << std::endl;
  }
  return retval;
}

data_t* monitorPtrRead (int segmentId) {
  //return data_t structure
  data_t* shm = (data_t*) shmat(segmentId, 0, 0); //attach shared memory
  if(shm == (void*) -1){ 
    std::cout << "Something wrong happened during shared memory access!" << std::endl;
  }
  return shm;
}


double getGlobalThroughput (data_t *data) {
  if(data->curr == 0 && data->ticks[data->curr].time == 0)
    return 0;
  //get current time and date
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  long double currTime = (tv.tv_sec + 0.000001*tv.tv_usec) - data->startTime;

  return data->ticks[data->curr].value / currTime;
}

double getCurrThroughput (data_t *data) {
  //no data in the structure!
  if(data->curr == 0 && data->ticks[data->curr].time == 0)
    return 0;

  //get current time and date
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  long double currTime = (tv.tv_sec + 0.000001*tv.tv_usec) - data->startTime;

  //no tick in the previous period
  if(currTime - data->ticks[data->curr].time > data->TPwindow)
    return 0;
  
  //we assume to have at least 2 values in the structure. so if data->curr==0 and we passed the check at the beginning of the function, 
  //the buffer is completelly full and we can move back to the last element!
  int i = (data->curr==0)? MAX_TICKS_SIZE-1 : data->curr-1;
  while(currTime - data->ticks[i].time < data->TPwindow && !(i == 0 && data->ticks[i].time == 0) && i!=data->curr){
    if(i>0) //i = (i+MAX_TICKS_SIZE-1)%MAX_TICKS_SIZE; //it is the same
      i--;
    else
      i = MAX_TICKS_SIZE-1;
  }
  long double actualWindow = data->TPwindow;
  if(i == 0 && data->ticks[i].time == 0) //do note: at the beginning the time window is smaller than the prefedined one
    actualWindow = data->ticks[data->curr].time;
  else if(i == data->curr){ //this is the case in which the application is too fast and it fills more than once the whole window between two iterations of the controller! 
    //so we have to increment i to count the whole list and  
    i = (i+1) % MAX_TICKS_SIZE;
    actualWindow = data->ticks[data->curr].time - data->ticks[i].time;
  }
  return (data->ticks[data->curr].value - data->ticks[i].value) / actualWindow;
}

void printAttachedApplications(monitor_t* monitor){
  //display application table
  std::cout << "---------------------------" << std::endl;
  std::cout << "attached appls: " << monitor->nAttached << std::endl;
  for(int j=0; j < monitor->nAttached; j++){
    data_t data = monitorRead(monitor->appls[j].segmentId);
    std::cout << monitor->appls[j].name << " " << monitor->appls[j].segmentId << " global tp: " << getGlobalThroughput(&data) << " current tp: " << getCurrThroughput(&data) << std::endl;
  }
  std::cout << "---------------------------" << std::endl;
}

void setUseGPU(data_t *data, bool value) {
  data->useGPU = value;
}

void setPrecisionLevel(data_t *data, int value) {
  data->precisionLevel = value;
}

std::vector<pid_t> updateAttachedApplications(monitor_t* monitor){
  std::vector<pid_t> newAppls;
  //look for new applications
  for(int j=0; j < monitor->nAttached; j++){
    if(!monitor->appls[j].alreadyInit){ //if the application is new
      //save pid in the data structure
      monitor->appls[j].alreadyInit = true;
      newAppls.push_back(monitor->appls[j].pid);
    }
  }

  monitor->nDetached = 0; //deleting list...
  return newAppls; //return pids of new appls
}

void killAttachedApplications(monitor_t* monitor){
  //we kill all pending applications
  for(int j=0; j < monitor->nAttached; j++){
    //kill process!
    kill(monitor->appls[j].pid, SIGKILL);
  }
}

