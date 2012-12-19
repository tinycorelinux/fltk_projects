DEBUG=-g
CXXFLAGS=-Wall -pedantic $(DEBUG)
FLTKFLAGS=-lfltk

all: main

flwget.o: flwget.cxx flwget.h
	$(CXX) $(CXXFLAGS) -c -o flwget.o flwget.cxx

flwget.a: flwget.o
	ar -rc libflwget.a flwget.o && ranlib libflwget.a

main: flwget.o
	$(CXX) $(CXXFLAGS) $(FLTKFLAGS) -o flwget flwget.o
	sudo strip flwget

clean:
	rm -f *.o *.a main
