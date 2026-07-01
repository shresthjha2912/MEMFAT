CXX      = g++
CXXFLAGS = -Wall -O2

all: diskmanager foosh

diskmanager: diskmanager.cpp diskutils.h
	$(CXX) $(CXXFLAGS) diskmanager.cpp diskutils.cpp -o diskmanager

foosh: foosh.cpp diskutils.cpp diskutils.h
	$(CXX) $(CXXFLAGS) foosh.cpp diskutils.cpp -o foosh

clean:
	rm -f diskmanager foosh *.o