CXX=g++
CFLAGS=-c -O3 -std=c++11 #-DPROFILING_CALLS
LDFLAGS=-lm -lcgroup -pthread

CONTROLLER_SRC=ApplicationMonitor_Ctrl.cpp ApplicationMonitor_Common.cpp main.cpp
CONTROLLER_OBJ=$(CONTROLLER_SRC:.cpp=.o)
CONTROLLER_EXECUTABLE=controller

DEPS=$(CONTROLLER_SRC:%.cpp=%.d)

all: $(CONTROLLER_EXECUTABLE)

$(CONTROLLER_EXECUTABLE): $(CONTROLLER_OBJ)  
	$(CXX) $(CONTROLLER_OBJ) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) -MMD -MP $< -o $@

clean:
	rm -rf *.o *.d $(CONTROLLER_EXECUTABLE)
	
-include $(DEPS)
