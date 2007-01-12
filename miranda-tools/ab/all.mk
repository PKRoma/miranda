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
SFTP_PATH=/home/groups/m/mi/miranda/htdocs/testing/
SFTP_BATCH=sftp_batch.tmp

all: prepare fetch build package upload
	@echo Congrats! the nightly for $(CO_YMD) has been built.
	
prepare_ex:
define MAKEOUTPUTDIRS 
-mkdir tmp 
-mkdir tmp\\cvs
-mkdir tmp\\output
endef
	$(MAKEOUTPUTDIRS)
	
prepare:
	$(MAKE) --silent -f all.mk prepare_ex
	
build: fetch unzip changelog
#$(MAKE) -f build.mk all MODE=Debug DEBUG=1 --jobs=2
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

changelog: unzip
	$(WGET) $(WGETFLAGS) http://xolphin.nl/egodust/ChangeLog.part --output-document=$(OUTPUT)/ChangeLog_$(CO_YMD).part
	
# all the plugin names
PLUGIN_NAMES=icq msn aim yahoo jabber srmm clist_classic clist_mw dbx_3x

# give these names to the packager
export PLUGIN_NAMES

package: 
#build
	$(MAKE) --silent -f package.mk MODE=Release

upload: package
	$(MAKE) --silent -f all.mk generate_upload_list > $(SFTP_BATCH)
	$(SFTP) -C -b $(SFTP_BATCH) $(USER)@$(SFTP_SHELL) 

generate_upload_list:
define SFTP_BATCH_COMMANDS
cd $(SFTP_PATH)
# put miranda
echo put $(OUTPUT)/miranda32_$(CO_YMD).zip
echo chmod 744 miranda32_$(CO_YMD).zip
# put all the plugins in various modes and change permissions
for file in $(PLUGIN_NAMES); do \
echo put $(OUTPUT)/$$file\_$(CO_YMD).zip; \
echo chmod 744 $$file\_$(CO_YMD).zip; \
done
# put the cvs
echo put $(TMPDIR)/miranda_cvs.zip miranda_cvs_$(CO_YMD).zip
echo chmod 744 miranda_cvs_$(CO_YMD).zip
# generate all the symlinks
for file in $(PLUGIN_NAMES); do \
echo -symlink $$file\_$(CO_YMD).zip $$file\_latest.zip; \
done
echo -symlink miranda32_$(CO_YMD).zip miranda32_latest.zip
echo -symlink miranda_cvs_$(CO_YMD).zip miranda_cvs_latest.zip
echo put $(OUTPUT)/ChangeLog_$(CO_YMD).part
echo put $(CVSDIR)/checkout.when
echo chmod 744 ChangeLog_$(CO_YMD).part
echo chmod 744 checkout.when
endef
	-echo $(SFTP_BATCH_COMMANDS)

