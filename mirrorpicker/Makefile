bin = mirrorpicker
src = $(wildcard *.cxx)
obj = $(src:.cxx=.o)

CXXFLAGS ?= -Os
CXXFLAGS += -Wall -Wextra -fno-rtti -fno-exceptions -pthread
CXXFLAGS += -ffunction-sections -fdata-sections
CXXFLAGS += -s $(shell fltk-config --cxxflags)

LDFLAGS += -Wl,-gc-sections
LDFLAGS += -Wl,-O1
LDFLAGS += $(shell fltk-config --ldflags)

.PHONY: all clean

all: $(bin)

$(bin): $(obj)
	g++ -o $(bin) $(obj) $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f *.o $(bin)

%.o: %.cxx
	$(CXX) -c $(CXXFLAGS) $< -o $@
