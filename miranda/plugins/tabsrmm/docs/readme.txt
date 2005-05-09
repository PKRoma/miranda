				tabbed  SRMM plugin for Miranda IM 0.4.0.0
				------------------------------------------

Project page: http://www.sourceforge.net/projects/tabsrmm/
Support Forum: http://www.miranda.or.at/forums/

----------------------------------------------------------
				
Version: 1.0.0.0
Created: April 2004

1. Overview
-----------

tabSRMM is an advanced messaging module for Miranda IM (version 0.4.x or later
required). It adds many new options and features to make instant messaging more
enjoyable and allows you to tweak almost every aspect of the message window to
fit your needs.

1.1. Requirements
-----------------

a) Miranda IM, Version 0.4.0.0 or later.
b) The unicode version of tabSRMM requires Windows 2000 or XP and will not 
   run under Windows 9x or ME. The ANSI version may run under Windows 98 
   or ME (not under Windows 95), but there are some minor problems. In general,
   I suggest that, if you're still using a 9x based Windows, you should not
   expect tabSRMM to work without problems. I do no longer have access to any
   Win 9x system, and do not use it as a development platform, so it is a bit
   hard to fix issues related to Win 9x.

HOW TO INSTALL
--------------

The archive contains 2 .DLLs:

1. tabsrmm.dll - ANSI version, can not send or receive unicode messages.
2. tabsrmm_unicode.dll - unicode aware.

copy one of the .dll files (unicode or non-unicode) to your plugins folder and
PLEASE NOTE THAT THIS PLUGIN NEEDS AT LEAST MIRANDA VERSION 0.4.0.0 and
will NOT WORK with older releases.

INSTALLING THE ICON PACK - IMPORTANT !!
---------------------------------------

You need to copy ONE of the included tabsrmm_icons.dll into the plugins folder
aswell. If you don't, you'll receive an error message about a missing resource
dll, and you won't see any icons on the toolbar and elsewhere.

In general, there is no need to use the non-unicode version - the unicode release
is far more tested and works with all known protocols. It also has an option to
force sending and receiving ANSI messages only - this may help with some older
or buggy clients and their sometimes broken unicode support.

2. Some features:
-----------------

* "tabbed messaging". All message dialog windows are now opened within a "container"
   window. A simple tab bar can be used to switch between the message windows.
   Currently, there is only one container and no way to make one or more message
   windows "top level". Future versions of this plugin may behave different. I plan
   to implement multiple container windows and the ability to attach or detach
   message windows to these containers.
   
*  Lots of options to tweak the look of your message log.

*  Works with the external IEView plugin to give you a fully customizable message log
   using HTML templates and CSS.
   
*  Avatar support (for protocols which can do it). You can also set a local "user picture"
   for each contact.
      
3. FAQ and general help
-----------------------

   For a lot of knowledge, please visit my forum at http://hell.at.eu.org/forums/
   There you can find a lot of useful information, articles, a small FAQ, and some
   links telling you more about tabSRMM in general.

4. Known bugs and issues
------------------------

*  some minor glitches under Windows 9x/ME, mostly graphical.

*  multisend doesn't work well. I suggest using it with "care" :) It will
   improve in the future. Also, multisend shouldn't be used "excessivly" -
   tabSRMM limits the number of contacts to 20 per message, but even that
   *may* be too much for some IM networks.
  
6. Credits and thanks:
----------------------

Lots of people provided useful suggestions, feature requests and bug reports during
the development phase. I cannot name you all here, because it's just too many.

* The Miranda-IM development team and all contributors for making the best
  IM client on this planet (and probably in the entire universe :) )

* sryo (teodalton@yahoo.com) for the minimal icons. 

* Faith Healer for making icon packs, lots of suggestions, testing and
  feedback.
  
* the members on my own forum (http://hell.at.eu.org/forums/) for a lot of bug
  reports, feature suggestions and testing every new snapshot.

* Angeli-Ka for many bug reports and general feedback,  especially in the 
  early phase of development when using tabSRMM could be really frustrating :)

* Progame - for finding even the most carefully hidden bugs :)

* All other mambers of the Miranda community,  who helped with hunting down 
  sometimes hard to find, bugs, and for suggesting features.
  
License: GPL

Contact me at:         mailto: silvercircle@gmail.com
			       ICQ:    7769309
                       MSN:    silvercircle@gmail.com
		 
