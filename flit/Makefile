#makefile for fltk program
CXXFLAGS=-march=i486 -mtune=i686 -Os -pipe -fno-exceptions -fno-rtti
PROG=flit

all: 
	g++ ${CXXFLAGS} `fltk-config --cxxflags` -c ${PROG}.cpp
	g++ ${CXXFLAGS} `fltk-config --ldflags` -lfltk_images ${PROG}.o -o ${PROG}
	sstrip ${PROG}
#	sudo cp ${PROG} /usr/local/bin

clean:
	rm *.o ${PROG}
