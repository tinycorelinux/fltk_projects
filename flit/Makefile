#makefile for fltk program
CXXFLAGS="-mtune=generic -Os -pipe -fno-exceptions -fno-rtti"
PROG=flit

all: 
	g++ -mtune=generic -Os -pipe -fno-exceptions -fno-rtti `fltk-config --cxxflags` -c ${PROG}.cpp
	g++ `fltk-config --ldflags` -lfltk_images ${PROG}.o -o ${PROG}
	strip ${PROG}
#	sudo cp ${PROG} /usr/local/bin

clean:
	rm *.o ${PROG}
