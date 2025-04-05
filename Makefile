ARCH := $(shell uname -m)


PREFIX = /usr/local
INSTDIR = $(DESTDIR)/$(PREFIX)/bin

DIRS = add2file apps cpanel datetool editor \
	exittc filetool flrun \
	mirrorpicker mnttool mousetool \
	network popask popup services \
	stats swapfile tc-install \
	tc-wbarconf wallpaper

TARGETS = $(foreach dir, $(DIRS),$(dir)/$(dir))
SRC = $(foreach dir, $(DIRS),$(wildcard $(dir)/*.cxx))
OBJ = $(SRC:.cxx=.o)

.PHONY: all clean install

%.o : %.cxx
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@

CXXFLAGS += -Os -s -Wall -Wextra
CXXFLAGS += -fno-rtti -fno-exceptions
CXXFLAGS += -ffunction-sections -fdata-sections

LDFLAGS += -Wl,-O1 -Wl,-gc-sections
LDFLAGS += -Wl,-as-needed

# Additional flags for x86
ifeq ($(ARCH), i686)
CXXFLAGS += -march=i486 -mtune=i686
ifneq (ldscripts,$(findstring ldscripts, $(shell fltk-config --ldflags)))
LDFLAGS += "-Wl,-T/usr/local/lib/ldscripts/elf_i386.xbn"
endif
endif

# Additional flags for x86_64
ifeq ($(ARCH), x86_64)
CXXFLAGS += -mtune=generic
ifneq (ldscripts,$(findstring ldscripts, $(shell fltk-config --ldflags)))
LDFLAGS += "-Wl,-T/usr/local/lib/ldscripts/elf_x86_64.xbn"
endif
endif

# Additional flags for aarch64
ifeq ($(ARCH), aarch64)
CXXFLAGS += -march=armv8-a+crc -mtune=cortex-a72
ifneq (ldscripts,$(findstring ldscripts, $(shell fltk-config --ldflags)))
LDFLAGS += 
endif
endif

# Additional flags for armhf
ifeq ($(ARCH), armv*)
CXXFLAGS += -march=armv6zk -mtune=arm1176jzf-s -mfpu=vfp
ifneq (ldscripts,$(findstring ldscripts, $(shell fltk-config --ldflags)))
LDFLAGS += 
endif
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
