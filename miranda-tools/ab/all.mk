USER=egodust
TMPDIR=tmp
CVSDIR=$(TMPDIR)/cvs
OUTPUT=$(TMPDIR)/output
CVSHOST=http://xolphin.nl/egodust
CVSFILE=miranda_cvs.zip

export TMPDIR
export CVSDIR
export OUTPUT
export CVSHOST
export CVSFILE
export USER

WGET=bin/wget.exe
WGETFLAGS=--tries=10 --waitretry=60

SFTP=bin/sftp.exe
SFTP_SHELL=shell.sourceforge.net
SFTP_PATH=/home/groups/m/mi/miranda-icq/htdocs/ab
SFTP_BATCH=sftp_batch.tmp

all: prepare build package upload
	@echo Congrats! the nightly for $(CO_YMD) has been built.

prepare:
	-mkdir tmp
	-mkdir tmp\\cvs
	-mkdir tmp\\output
	-mkdir tmp\\cvs\\bin
ifndef DEBUG
	-mkdir tmp\\cvs\\bin\\Release
	-mkdir tmp\\cvs\\bin\\Release\\Plugins
else
	-mkdir tmp\\cvs\\bin\\Debug	
	-mkdir tmp\\cvs\\bin\\Debug\\Plugins
endif


build: fetch unzip changelog
	$(MAKE) -f build.mk all MODE=Debug DEBUG=1 --jobs=2
	$(MAKE) -f build.mk all MODE=Release --jobs=4

fetch: prepare
	$(WGET) $(WGETFLAGS) $(CVSHOST)/$(CVSFILE) --output-document=$(TMPDIR)/$(CVSFILE)
	
unzip: fetch
	bin/unzip -o $(TMPDIR)/$(CVSFILE) -d $(CVSDIR)
# grab the full date/time string 
CO_DATE=$(shell more $(TMPDIR)\\cvs\\checkout.when)
# take the date ab:cd and convert to ab cd then join the first two words
CO_TMP=$(subst :, ,$(word 2,$(CO_DATE)))
CO_TIME=$(word 1, $(CO_TMP))$(word 2, $(CO_TMP))
# take the first word, which is the date in YYYY-MM-DD format, and strip out "-"
CO_YMD=$(subst -,,$(word 1,$(CO_DATE)))$(CO_TIME)
export CO_YMD
ZIPS=miranda_core_$(CO_YMD).zip miranda_core_debug_$(CO_YMD).zip \
miranda_plugins_$(CO_YMD).zip miranda_plugins_debug_$(CO_YMD).zip \
miranda_protocols_$(CO_YMD).zip miranda_protocols_debug_$(CO_YMD).zip
# attach paths
UPLOAD_LIST=$(foreach FILE, $(ZIPS), $(OUTPUT)/$(FILE))
	
package: build
	$(MAKE) -f package.mk all MODE=Debug 
	$(MAKE) -f package.mk all MODE=Release
	
changelog: prepare
	$(WGET) $(WGETFLAGS) http://xolphin.nl/egodust/ChangeLog.part --output-document=$(OUTPUT)/ChangeLog.part
	
upload: build
	$(MAKE) --silent -f all.mk generate_upload_list > $(SFTP_BATCH)
	$(SFTP) -C -b $(SFTP_BATCH) $(USER)@$(SFTP_SHELL) 

generate_upload_list:
define SFTP_BATCH_COMMANDS
cd $(SFTP_PATH)
for file in $(UPLOAD_LIST); do \
  echo put $$file; \
done
echo put $(TMPDIR)/miranda_cvs.zip miranda_cvs_$(CO_YMD).zip
for file in $(ZIPS); do \
  echo chmod 744 $$file; \
done
echo chmod 744 miranda_cvs_$(CO_YMD).zip
echo -symlink miranda_core_$(CO_YMD).zip miranda_core_latest.zip
echo -symlink miranda_plugins_$(CO_YMD).zip miranda_plugins_latest.zip
echo -symlink miranda_protocols_$(CO_YMD).zip miranda_protocols_latest.zip
echo -symlink miranda_cvs_$(CO_YMD).zip miranda_cvs_latest.zip
echo put $(OUTPUT)/ChangeLog.part
echo put $(CVSDIR)/checkout.when
echo chmod 744 ChangeLog.part
echo chmod 744 checkout.when
endef
	-echo $(SFTP_BATCH_COMMANDS)

