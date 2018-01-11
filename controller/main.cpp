#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <set>
#include <iomanip>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sstream>
#include <thread>        


#include "ApplicationMonitor.h"


#define WAKEUP_PERIOD 1 //seconds

void sig_handler (int sig, siginfo_t *info, void *extra);
bool end = false; //states if the controller has to be terminated


int main (int argc, char* argv[]) {
 

  int i = 0;
  int sock;
      timeval tv;
  int bytes_read;
      socklen_t addr_len;
  char recv_data[1024];
  struct sockaddr_in server_addr , client_addr;
  char send_data[1024];
  struct timeval timeout;      
        timeout.tv_sec = 1;
        timeout.tv_usec = 0; 
  
  
  //initiaize application monitor
  monitor_t* appl_monitor;
  appl_monitor = monitorInit();

  //set output precision
  std::cout << std::setprecision(10);
  std::cerr << std::setprecision(10);

  //setup interrupt handler
  struct sigaction action;
  action.sa_flags = SA_SIGINFO;
  action.sa_sigaction = &sig_handler;
  if (sigaction(SIGINT, &action, NULL) == -1) {
    std::cout << "Error registering interrupt handler\n" << std::endl;
    exit(EXIT_FAILURE);
  }

  std::cout << "press Ctrl+C in order to stop monitoring" << std::endl;
  std::cout << "monitoring start..." << std::endl << std::endl;
  


  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
      perror("Socket");
      exit(1);
  }
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(50000);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  bzero(&(server_addr.sin_zero),8);

  if (bind(sock,(struct sockaddr *)&server_addr,
      sizeof(struct sockaddr)) == -1)
  {
      perror("Bind");
      exit(1);
  }

  addr_len = sizeof(struct sockaddr); 

  while (!end) {
 
    //display application table
    std::cout << "---------------------------" << std::endl;
    std::cout << "iteration " << i << std::endl;
    std::cout << "attached appls: " << appl_monitor->nAttached << std::endl;
    /*for(int j=0; j < appl_monitor->nAttached; j++){
      data_t data = monitorRead(appl_monitor->appls[j].segmentId);
      std::cout << appl_monitor->appls[j].name << " throughput: " << getCurrThroughput(&data) << std::endl;
    }*/
    double tp=-1; 
    if(appl_monitor->nAttached>0){
      data_t data = monitorRead(appl_monitor->appls[0].segmentId);
      tp = getCurrThroughput(&data);
      std::cout << appl_monitor->appls[0].name << " throughput: " << tp << std::endl;

    }  
    std::cout << "---------------------------" << std::endl << std::endl;

    i++;

    std::string apps = std::to_string(appl_monitor->nAttached);
    const char *num_app = apps.c_str();
    std::string app_data = std::to_string(tp);
    const char *thr_app = app_data.c_str();
    
          bytes_read = recvfrom(sock,recv_data,1024,0,
                      (struct sockaddr *)&client_addr, &addr_len);
    std::cout << "---------------------------" << std::endl;   
    std::this_thread::sleep_for(std::chrono::seconds(1));  
    recv_data[bytes_read] = '\0';                           
    if ( recv_data[0] == 'f'){
       
      strcpy(send_data,num_app);
      sendto(sock, send_data, strlen(send_data), 0,
          (struct sockaddr *)&client_addr, sizeof(struct sockaddr));   
            
      strcpy(send_data,thr_app);
      sendto(sock, send_data, strlen(send_data), 0,
          (struct sockaddr *)&client_addr, sizeof(struct sockaddr));  
                  
      }
      else
      std::cout << "Bad instruction" << std::endl;
      //close(sock);
  }   
  
  //release resources and kill all appls
  killAttachedApplications(appl_monitor);
  monitorDestroy(appl_monitor);
  close(sock);
  return 0;
}

//interrupt handler that finalizes the execution
void sig_handler (int sig, siginfo_t *info, void *extra) {
  if (sig == SIGINT) {
    std::cout << "\nmonitoring stop." << std::endl;
    end = true;
  }
}


  
  
   
