#makefile for fltk program
CXXFLAGS="-march=i486 -mtune=i686 -Os -pipe -fno-exceptions -fno-rtti"
PROG=flit

all: 
	gcc ${CXXFLAGS} `fltk-config --cxxflags` -c ${PROG}.cpp
	gcc `fltk-config --ldflags` -lfltk_images ${PROG}.o -o ${PROG}
	strip ${PROG}
#	sudo cp ${PROG} /usr/local/bin

clean:
	rm *.o ${PROG}
