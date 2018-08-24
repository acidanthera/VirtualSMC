CXX = clang++
CXXFLAGS = -flto -mmacosx-version-min=10.6 -Os -Wall -Wno-deprecated
LFLAGS = -flto -framework IOKit -framework CoreFoundation

all: smc32 smc64 smc 

smc: smc32 smc64
	lipo -create smc32 smc64 -output smc

smc32: smc32.o
	$(CXX) -m32 $(CXXFLAGS) $(LFLAGS) -o smc32 smc32.o
	strip -x smc32

smc64: smc64.o
	$(CXX) -m64 $(CXXFLAGS) $(LFLAGS) -o smc64 smc64.o
	strip -x smc64

smc32.o: smc.h smc.cpp
	$(CXX) -m32 $(CXXFLAGS) -c smc.cpp -o smc32.o

smc64.o: smc.h smc.cpp
	$(CXX) -m64 $(CXXFLAGS) -c smc.cpp -o smc64.o

clean:
	-rm -f smc smc32 smc64 smc32.o smc64.o
