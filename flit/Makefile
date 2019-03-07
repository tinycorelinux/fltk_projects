#makefile for fltk program
CXXFLAGS=-march=i486 -mtune=i686 -Os -pipe -fno-exceptions -fno-rtti
PROG=flit
VERSTR=1_3_1

#Standard Version: has right-click menu and multi-image paging. About 17.0K
#Uses these #defines: (none)
all: compile

install: compile
	sudo cp ${PROG} /usr/local/bin
	sudo cp ${PROG}_help.htm /usr/share/doc/tc/
	

compile:      	
	g++ `fltk-config --cxxflags` ${CXXFLAGS} -c ${PROG}.cpp
	g++ ${PROG}.o -o ${PROG} `fltk-config --use-images --use-forms --ldflags`
	strip --strip-all ${PROG}

upx: compile
	upx --ultra-brute ${PROG}

package: compile
	sudo ./build-tcz.sh

tarsrc:
	tar -czf flit_${VERSTR}_src.tgz Makefile build-tcz.sh flit.cpp soundcard.h pidfile.h flit_help.htm flit.tcz.info

tar: package
	tar -czf flit_${VERSTR}.tar flit.tcz flit.tcz.info flit.tcz.list flit.tcz.md5.txt Makefile build-tcz.sh flit.cpp soundcard.h pidfile.h flit_help.htm flit.tcz.info

clean:
	rm *.o ${PROG}

run: install
	cd /home/tc/; flit &
	
