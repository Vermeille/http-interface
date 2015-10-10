CXX=g++
LDFLAGS=-lmicrohttpd -lglog -lfolly
CXXFLAGS=-std=c++11 -g3 -Wall -Wextra

all: displayer

displayer: displayer.o main.o chart.o
	$(CXX) $(OUTPUT_OPTION) $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $^ $(CXXFLAGS) -c $(OUTPUT_OPTION)
