ZIP=bin/zip.exe
ZIPFLAGS=-9 -X -j
ifeq ($(MODE),Release)
LMODE=
else
LMODE=debug_
endif
BINDIR=$(CVSDIR)/miranda/bin/$(MODE)

#export PLUGIN_NAMES

all:
define ZIPUP
$(ZIP) $(ZIPFLAGS) $(OUTPUT)/miranda32_$(LMODE)$(CO_YMD).zip $(BINDIR)/miranda32.exe
for name in $(PLUGIN_NAMES); do \
$(ZIP) $(ZIPFLAGS) $(OUTPUT)/$$name\_$(LMODE)$(CO_YMD).zip $(BINDIR)/Plugins/$$name.dll; \
done
endef
	$(ZIPUP)