CPPFLAGS=-O3 -march=native -ffast-math -flto -std=c++14
CPP=g++
CC=gcc
CFLAGS=-O2 -march=native -flto

all: main.o options.o qdbmp.o fractals.o
	$(CPP) -flto main.o fractals.o options.o qdbmp.o -lm -fopenmp -o fractalmake

main.o: main.cpp options.hpp color_scale.hpp fractals.hpp
	$(CPP) $(CPPFLAGS) -c main.cpp 

options.o: options.cpp options.hpp
	$(CPP) $(CPPFLAGS) -c options.cpp

qdbmp.o: qdbmp.c qdbmp.h
	$(CC) $(CFLAGS) -c qdbmp.c

fractals.o: fractals.cpp fractals.hpp
	$(CPP) $(CPPFLAGS) -c fractals.cpp

fractals.hpp: qdbmp.h vector_slice.hpp

options.hpp: fractals.hpp function_parser.hpp

color_scale.hpp: fractals.hpp spline.hpp

clean:
	rm -f fractalmake *.o 
	