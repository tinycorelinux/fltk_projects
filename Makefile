ARCH := $(shell uname -m)


PREFIX = /usr/local
INSTDIR = $(DESTDIR)/$(PREFIX)/bin

DIRS = apps editor stats tc-install tc-wbarconf
#DIRS = editor

TARGETS = $(foreach dir, $(DIRS),$(dir)/$(dir))
SRC = $(foreach dir, $(DIRS),$(wildcard $(dir)/*.cxx))
OBJ = $(SRC:.cxx=.o)

.PHONY: all clean install

%.o : %.cxx
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@

CXXFLAGS += -Os -s -Wall -Wextra -Wno-missing-field-initializers
CXXFLAGS += -fno-rtti -fno-exceptions
CXXFLAGS += -ffunction-sections -fdata-sections

LDFLAGS += -Wl,-O1 -Wl,-gc-sections
LDFLAGS += -Wl,-as-needed

# Additional flags for x86
ifeq ($(ARCH), i686)
CXXFLAGS += -march=i486 -mtune=i686
LDFLAGS += "-Wl,-T/usr/local/lib/ldscripts/elf_i386.xbn"
endif

# Additional flags for x86_64
ifeq ($(ARCH), x86_64)
CXXFLAGS += -mtune=generic
LDFLAGS += "-Wl,-T/usr/local/lib/ldscripts/elf_x86_64.xbn"
endif

CXXFLAGS += $(shell fltk-config --cxxflags | sed 's@-I@-isystem @')
LDFLAGS += $(shell fltk-config --ldflags)

all: $(TARGETS)

$(TARGETS): $(OBJ)
	$(CXX) -o $@ $(filter $(dir $@)%.o, $(OBJ)) $(CXXFLAGS) $(LDFLAGS)
	sstrip $@

clean:
	rm -f $(TARGETS) $(OBJ)

install: all
	mkdir -p $(INSTDIR)
	cp -a $(TARGETS) $(INSTDIR)
