PREFIX = /usr/local
INSTDIR = $(DESTDIR)/$(PREFIX)/bin

DIRS = add2file appbrowser apps appsaudit cpanel datetool editor \
	exittc fdtool filechooser filetool flpdf flrun \
	loadpack mirrorpicker mnttool mousetool \
	network popask popup scmapps scmbrowser services \
	settheme stats swapfile tcemirror tc-grub4dos tc-install \
	tc-wbarconf wallpaper wbarconf wbar_exclude

TARGETS = $(foreach dir, $(DIRS),$(dir)/$(dir))
SRC = $(foreach dir, $(DIRS),$(wildcard $(dir)/*.cxx))
OBJ = $(SRC:.cxx=.o)

.PHONY: all clean install

%.o : %.cxx
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@

CXXFLAGS += -Os -s -Wall -Wextra
CXXFLAGS += -march=i486 -mtune=i686 # change or comment this out on other arches
CXXFLAGS += -fno-rtti -fno-exceptions
CXXFLAGS += -ffunction-sections -fdata-sections

LDFLAGS += -Wl,-O1 -Wl,-gc-sections
LDFLAGS += -Wl,-as-needed

CXXFLAGS += $(shell fltk-config --cxxflags)
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
