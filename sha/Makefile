# By default, the code is compiled for a "big endian" machine.
# To compile on a "little endian" machine set the LITTLE_ENDIAN flag.
# To make smaller object code, but run a little slower, don't use UNROLL_LOOPS.
# To use NIST's modified SHA of 7/11/94, define USE_MODIFIED_SHA

CXX = g++
CFLAGS=-c -O3 -std=c++11 #-DPROFILING_CALLS
LDFLAGS=

SRC=sha.cpp sha_driver.cpp ../controller/ApplicationMonitor_Appl.cpp  ../controller/ApplicationMonitor_Common.cpp
OBJ=$(SRC:.cpp=.o)
EXE=sha

DEPS=$(SRC:%.cpp=%.d)

all: $(EXE)

$(EXE): $(OBJ)
	$(CXX) $(OBJ) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) -MMD -MP $< -o $@

clean:
	rm -rf *.o *.d $(EXE)
	
-include $(DEPS)
