ZIP=bin/zip.exe
ZIPFLAGS=-9 -X -j
ifeq ($(MODE),Release)
LMODE=
else
LMODE=_debug
endif
BIN_DIR=$(CVSDIR)/Bin/$(MODE)

PLUGINS=srmm.dll changeinfo.dll import.dll
PROTOCOLS=aim.dll ICQ.dll jabber.dll msn.dll Yahoo.dll
BIN_PLUGINS=$(foreach P, $(PLUGINS), $(BIN_DIR)/Plugins/$(P))
BIN_PROTOCOLS=$(foreach P, $(PROTOCOLS), $(BIN_DIR)/Plugins/$(P))

all:
	$(ZIP) $(ZIPFLAGS) $(OUTPUT)/miranda_core$(LMODE)_$(CO_YMD).zip $(BIN_DIR)/miranda32.exe
	$(ZIP) $(ZIPFLAGS) $(OUTPUT)/miranda_plugins$(LMODE)_$(CO_YMD).zip $(BIN_PLUGINS)
	$(ZIP) $(ZIPFLAGS) $(OUTPUT)/miranda_protocols$(LMODE)_$(CO_YMD).zip $(BIN_PROTOCOLS)