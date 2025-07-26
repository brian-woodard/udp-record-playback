CXXFLAGS = -g -std=c++17
#CXXFLAGS = -O2 -std=c++17

all:
	g++  $(CXXFLAGS) main.cpp -o main

clean:
	rm -f main
