CC=g++
CXX=g++
RANLIB=ranlib

LIBSRC= VirtualMemory.cpp 
LIBOBJ=$(LIBSRC:.cpp=.o)

INCS=-I.
CFLAGS = -Wall -std=c++11 -g $(INCS)
CXXFLAGS = -Wall -std=c++11 -g $(INCS)

FRAMEWORKLIB = libVirtualMemory.a
TARGETS = $(FRAMEWORKLIB)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex4.tar
TARSRCS= VirtualMemory.cpp Makefile README

all: $(TARGETS)

$(TARGETS): $(LIBOBJ)
	$(AR) $(ARFLAGS) $@ $^
	$(RANLIB) $@

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(LIBSRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
