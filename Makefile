#
# Makefile for Linux
#
CXX = g++
BINARY = $(NAME).bin
VERSION = $(shell git describe --always --dirty) $(shell date -I)
CXXFLAGS = -O2 -Wextra -Wall `sdl-config --cflags` -ansi -pedantic \
	-Wno-variadic-macros
LDFLAGS = -lGL -lGLU -lGLEW `sdl-config --libs` -lSDL_ttf -lvorbisfile \
	-lpng -g

include Makefile.common

