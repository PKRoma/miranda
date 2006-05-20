MSDEV=msdev.exe
MSDEV_PFLAGS=//REBUILD
MIRDIR=$(CVSDIR)/miranda

all: miranda protocols plugins
# done
	
unzip:
# legacy command

miranda: unzip
	$(MSDEV) $(MIRDIR)/src/miranda32.dsp //MAKE "miranda32 - Win32 $(MODE)" $(MSDEV_PFLAGS)
	
protocols: unzip aim icq jabber msn yahoo
# all protocols have been built
	
aim: unzip
	$(MSDEV) $(MIRDIR)/protocols/AimToc/aim.dsp //MAKE "aim - Win32 $(MODE)" $(MSDEV_PFLAGS)

icq: unzip
	$(MSDEV) $(MIRDIR)/protocols/IcqOscar8/icqoscar8.dsp //MAKE "IcqOscar8 - Win32 $(MODE)" $(MSDEV_PFLAGS)

jabber: unzip
	$(MSDEV) $(MIRDIR)/protocols/Jabber/jabber.dsp //MAKE "jabber - Win32 $(MODE)" $(MSDEV_PFLAGS)

msn: unzip
	$(MSDEV) $(MIRDIR)/protocols/MSN/msn.dsp //MAKE "msn - Win32 $(MODE)" $(MSDEV_PFLAGS)

yahoo: unzip
	$(MAKE) --directory=$(MIRDIR)/protocols/Yahoo -f Makefile.win $(if $(DEBUG),DEBUG=1)

plugins: unzip srmm clist mwclist db3x
# all plugins have been built
	
srmm: unzip
	$(MSDEV) $(MIRDIR)/plugins/srmm/srmm.dsp //MAKE "srmm - Win32 $(MODE)" $(MSDEV_PFLAGS)

#tabsrmm: unzip
#	$(MSDEV) $(MIRDIR)/Plugins/tabsrmm/srmm.dsp //MAKE "srmm - Win32 $(MODE)" $(MSDEV_PFLAGS)
	
mwclist: unzip
	$(MSDEV) $(MIRDIR)/plugins/mwclist/clist.dsp //MAKE "clist - Win32 $(MODE)" $(MSDEV_PFLAGS)

clist: unzip
	$(MSDEV) $(MIRDIR)/plugins/clist/clist.dsp //MAKE "clist - Win32 $(MODE)" $(MSDEV_PFLAGS)
	
db3x: unzip
	$(MSDEV) $(MIRDIR)/plugins/db3x/db3x.dsp //MAKE "db3x - Win32 $(MODE)" $(MSDEV_PFLAGS)
	
