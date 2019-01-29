#makefile for fltk program
PROG=flit

all: 
	g++ -march=i486 -mtune=i686 -Os -pipe -fno-exceptions -fno-rtti `fltk-config --cxxflags` -c ${PROG}.cpp
	g++ `fltk-config --ldflags` -lfltk_images ${PROG}.o -o ${PROG}
	strip ${PROG}
#	sudo cp ${PROG} /usr/local/bin

clean:
	rm *.o ${PROG}
