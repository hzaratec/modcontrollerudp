/* NIST Secure Hash Algorithm */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../controller/ApplicationMonitor.h"
#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include "sha.h"
#include <fstream>

//#define PROFILING_CALLS

#ifdef PROFILING_CALLS
#include <iomanip>
#include <chrono>
#endif

int main(int argc, char **argv)
{
    data_t *data; //pointer to the shared memory to communicate with the monitor

    FILE *fin;
    SHA_INFO sha_info;
    int i, times;
    char* filename;

    if(argc != 4){
        printf("%s IN_FILE_NAME NUM_OF_ITERS \n", argv[0]);
        return 1;       
    }
    
    //attach the monitor. parameters are: application name, 
    //minimum and throughout requirements, and the unavailability of the GPU implementation
#ifdef PROFILING_CALLS
    std::cout << std::setprecision(10);
    std::cerr << std::setprecision(10);
    std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
#endif
    data = monitorAttach(argv[0], 15, 20, false);
#ifdef PROFILING_CALLS
    std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_span1 = std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1);
    std::cout << "[PROFILING] Monitor Attach: " << time_span1.count() << " s." << std::endl;
#endif   
    times = atoi(argv[2]);
    filename = argv[1];
    char* ip=argv[3];

    for(i = 0; i < times; i++){
        fin = fopen(filename, "rb");
        if (fin ) {
#ifdef PROFILING_CALLS
            std::chrono::high_resolution_clock::time_point t7 = std::chrono::high_resolution_clock::now();
#endif
            sha_init(&sha_info, i);
            sha_stream(&sha_info, fin);
            //sha_print(&sha_info); //print output
            fclose(fin);
#ifdef PROFILING_CALLS
            std::chrono::high_resolution_clock::time_point t8 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> time_span4 = std::chrono::duration_cast<std::chrono::duration<double> >(t8 - t7);
            std::cout << "[PROFILING] Sha Body: " << time_span4.count() << " s." << std::endl;
#endif   
            //send heartbeat
#ifdef PROFILING_CALLS
            std::chrono::high_resolution_clock::time_point t3 = std::chrono::high_resolution_clock::now();
#endif
            monitorTick(data);
#ifdef PROFILING_CALLS
            std::chrono::high_resolution_clock::time_point t4 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> time_span2 = std::chrono::duration_cast<std::chrono::duration<double> >(t4 - t3);
            std::cout << "[PROFILING] Monitor Tick: " << time_span2.count() << " s." << std::endl;
#endif   
        } else {
            printf("error opening %s for reading\n", filename);
        }
        sha_comm(&sha_info,ip);
    }
    
    //detach the monitor
#ifdef PROFILING_CALLS
    std::chrono::high_resolution_clock::time_point t5 = std::chrono::high_resolution_clock::now();
#endif
    monitorDetach(data);
#ifdef PROFILING_CALLS
    std::chrono::high_resolution_clock::time_point t6 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time_span3 = std::chrono::duration_cast<std::chrono::duration<double> >(t6 - t5);
    std::cout << "[PROFILING] Monitor Detach: " << time_span3.count() << " s." << std::endl;
#endif   
    return 0;
}
