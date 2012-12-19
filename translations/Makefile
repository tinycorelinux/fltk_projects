#	Makefile to handle TC tools' translations
#	Copied from Powertop
#

LOCALESDIR=/usr/local/share/locale

SRC=$(wildcard *.po)
OBJ= $(SRC:.po=.mo)
INST= $(SRC:.po=.inst)

all:
	@echo \"make tinycore.pot\" to update the template from the src
	@echo
	@echo \"make uptrans LG=xx\" to update an existing translation

translations: $(OBJ)
	
%.mo: %.po
	msgfmt -o $@  $<

clean:
	rm -f *.mo *~

tinycore.pot: Makefile
	xgettext -C -s -o tinycore.pot -kmygettext ../*/*.cxx ../*/*.h

# hack to automate installation dynamicaly, without previous knowledge of 
# the po/mo file list (we fool make by pretending the need for .inst files).
install: $(OBJ) $(INST)

%.inst: %.mo
	mkdir -p $(DESTDIR)$(LOCALESDIR)/$*/LC_MESSAGES/
	-cp -f $< $(DESTDIR)$(LOCALESDIR)/$*/LC_MESSAGES/tinycore.mo

uptrans: $(LG).po

$(LG).po: tinycore.pot
ifdef LG
	msgmerge -U $@ $<
	msgfmt --statistics $@
else
	@echo "Usage : make uptrans LG=xx # with xx = de, es, fi ..."
	@exit 1
endif

