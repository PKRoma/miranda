MSDEV=msdev.exe
MSDEV_PFLAGS=//REBUILD

all: miranda protocols plugins
# done
	
unzip:
# legacy command

miranda: unzip
	$(MSDEV) $(CVSDIR)/Miranda-IM/miranda32.dsp //MAKE "miranda32 - Win32 $(MODE)" $(MSDEV_PFLAGS)
	
protocols: unzip aim icq jabber msn yahoo
# all protocols have been built
	
aim: unzip
	$(MSDEV) $(CVSDIR)/Protocols/AimToc/aim.dsp //MAKE "aim - Win32 $(MODE)" $(MSDEV_PFLAGS)

icq: unzip
	$(MSDEV) $(CVSDIR)/Protocols/IcqOscar8/icqoscar8.dsp //MAKE "IcqOscar8 - Win32 $(MODE)" $(MSDEV_PFLAGS)

jabber: unzip
	$(MSDEV) $(CVSDIR)/Protocols/Jabber/jabber.dsp //MAKE "jabber - Win32 $(MODE)" $(MSDEV_PFLAGS)

msn: unzip
	$(MSDEV) $(CVSDIR)/Protocols/MSN/msn.dsp //MAKE "msn - Win32 $(MODE)" $(MSDEV_PFLAGS)

yahoo: unzip
	$(MAKE) --directory=$(CVSDIR)/Protocols/Yahoo -f Makefile.win $(if $(DEBUG),DEBUG=1)

plugins: unzip srmm changeinfo import
# all plugins have been built
	
srmm: unzip
	$(MSDEV) $(CVSDIR)/Plugins/srmm/srmm.dsp //MAKE "srmm - Win32 $(MODE)" $(MSDEV_PFLAGS)

changeinfo: unzip
	$(MSDEV) $(CVSDIR)/Plugins/changeinfo/changeinfo.dsp //MAKE "changeinfo - Win32 $(MODE)" $(MSDEV_PFLAGS)

import: unzip
	$(MSDEV) $(CVSDIR)/Plugins/import/import.dsp //MAKE "import - Win32 $(MODE)" $(MSDEV_PFLAGS)
	
