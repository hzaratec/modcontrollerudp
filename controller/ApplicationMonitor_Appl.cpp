#include "ApplicationMonitor.h"
#include <iostream>

data_t* monitorAttach (const char* applName, long double tpMin, long double tpMax, bool gpuImpl, int TPwindow) {
  //get controller pid
  pid_t pid = getMonitorPid(DEFAULT_CONTROLLER_NAME);

  /*--- begin data initialization ------*/
  //create shared memory to store appliation data_t structure
  int segmentId = shmget(getpid(), sizeof(data_t), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
  data_t* data = (data_t*) shmat(segmentId, 0, 0);
  if (segmentId == -1 || data == (void*)-1){
    std::cout << "Something wrong happened during shared memory initialization" << std::endl;
    exit(EXIT_FAILURE); 
  }

  //get current time and date
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);

  //initialize data_t structure
  data->segmentId = segmentId;
  data->useGPU = 0;
  //first entry contains 0 ticks at time zero (from application start!)
  data->ticks[0].value = 0;
  data->ticks[0].time  = 0;
  data->curr = 0; 
  data->TPwindow = TPwindow;
  data->startTime = tv.tv_sec + 0.000001*tv.tv_usec;
  /*--- end data initialization --------*/

  /*--- begin monitor initialization ---*/
  //wait semaphore
  int semid = semget (pid, 1, 0);
  binarySemaphoreWait(semid);
  //attach to monitor_t structure in shared memory
  int shmid_monitor = shmget(pid, sizeof(monitor_t), 0);
  monitor_t* monitor = (monitor_t*) shmat (shmid_monitor, 0, 0);
  if (shmid_monitor == -1 || semid == -1 || monitor == (void*)-1){
    std::cout << "Something wrong happened during controller shared memory attaching" << std::endl;
    exit(EXIT_FAILURE); 
  }
  
  //save application data in the monitor
  if(monitor->nAttached == MAX_NUM_OF_APPLS - 1){
    std::cout << "Maximum number of manageable applications reached!" << std::endl;
    exit (EXIT_FAILURE);  
  }
  monitor->appls[monitor->nAttached].alreadyInit = false;
  monitor->appls[monitor->nAttached].tpMinObj    = tpMin;
  monitor->appls[monitor->nAttached].tpMaxObj    = tpMax;
  monitor->appls[monitor->nAttached].segmentId   = segmentId;
  monitor->appls[monitor->nAttached].pid         = getpid();
  monitor->appls[monitor->nAttached].gpuImpl     = gpuImpl;
  strcpy(monitor->appls[monitor->nAttached].name, applName);

  monitor->nAttached++;

  //post semaphore
  binarySemaphorePost(semid);
  /*--- end monitor initialization -----*/

  return data;
}

int monitorDetach(data_t *data) { 
  //get controller pid
  pid_t pid = getMonitorPid(DEFAULT_CONTROLLER_NAME);

  //wait semaphore  
  int semid = semget (pid, 1, 0);
  binarySemaphoreWait(semid);
  //attach shared memory of the controller
  int shmid_monitor = shmget(pid, sizeof(monitor_t), 0);
  monitor_t* monitor = (monitor_t*) shmat (shmid_monitor, 0, 0);
  if (shmid_monitor == -1 || semid == -1 || monitor == (void*)-1) {
    std::cout << "Something wrong happened during controller shared memory attaching" << std::endl;
    exit(EXIT_FAILURE); 
  }

  //clean entry of the current application in the monitor_t structure. we copy subsequent records a position behind and we decrease n_attached value
  int i = 0;
  for (; monitor->appls[i].segmentId != data->segmentId && i < MAX_NUM_OF_APPLS; i++);

  if(i == MAX_NUM_OF_APPLS){
      std::cout << "Segment_id " << data->segmentId << "  not found!" << std::endl; //may it happens?
      exit (EXIT_FAILURE);  
  }
  
  for (; i < monitor->nAttached-1; i++) {
    monitor->appls[i].segmentId = monitor->appls[i + 1].segmentId;
    monitor->appls[i].pid = monitor->appls[i + 1].pid;
    monitor->appls[i].tpMinObj = monitor->appls[i + 1].tpMinObj;
    monitor->appls[i].tpMaxObj = monitor->appls[i + 1].tpMaxObj;
    strcpy(monitor->appls[i].name, monitor->appls[i + 1].name);
    monitor->appls[i].gpuImpl = monitor->appls[i + 1].gpuImpl;
  }
  monitor->nAttached--;

  //add the pid of the completed application to the deattached list.
  if(monitor->nDetached == MAX_NUM_OF_APPLS-1) {
    std::cout << "Maximum number of manageable deattached applications reached!" << std::endl;
    exit (EXIT_FAILURE);
  }
  monitor->detached[monitor->nDetached] = getpid();
  monitor->nDetached++;

  //post semaphore
  binarySemaphorePost(semid);
  
  //delete shared memory for the appliaction data_t structure
  int segmentId = data->segmentId;
  int retval1 = shmdt(data); //we are removing the memory, therefore we need to previously save the id...
  int retval2 = shmctl(segmentId, IPC_RMID, 0);
  if(retval1 == -1 || retval2 == -1) {
    std::cout << "Something wrong happened during shared memory deletion!" << std::endl;
    exit (EXIT_FAILURE);
  }

  return 0;
}

void monitorTick (data_t *data, int value) {
  //TODO do we need semaphors for ticks?
  //data tick increment
  int next = (data->curr + 1) % MAX_TICKS_SIZE; 

  //get current time and date
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);

  //save data in the next position
  data->ticks[next].value = data->ticks[data->curr].value + value;
  data->ticks[next].time = (tv.tv_sec + 0.000001*tv.tv_usec) - data->startTime;
  
  //update reference to the current position
  data->curr = next;
}

pid_t getMonitorPid(const char* monitorName) {
  pid_t pid;
  // get controller pid given its name
  std::string pipe = std::string("pgrep ") + monitorName;
  FILE *fp;
  fp = popen(pipe.c_str(), "r");
  if (fp == NULL){
    std::cout << "can't read controller PID" << std::endl;
    exit (EXIT_FAILURE); 
  }
  int ret = fscanf(fp, "%d", &pid); //get controller PID
  if (feof(fp)){
    std::cout << "can't read controller PID" << std::endl;
    exit (EXIT_FAILURE); 
  }
  pclose(fp);
  return pid;
}

bool useGPU(data_t *data) {
  return data->useGPU;
}

int usePrecisionLevel(data_t *data) {
  return data->precisionLevel;
}

