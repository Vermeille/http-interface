CXX=g++
LDFLAGS=-lmicrohttpd
CXXFLAGS=-std=c++11 -g3 -Wall -Wextra

all: displayer

displayer: displayer.o main.o
	$(CXX) $(OUTPUT_OPTION) $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $^ $(CXXFLAGS) -c $(OUTPUT_OPTION)
