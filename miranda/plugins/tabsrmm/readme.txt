				Modified SRMM plugin for Miranda IM 0.3.3+
				------------------------------------------
				
Version: 0.0.2a
Created: Jul 2004

1. Intro:
---------

This is a modified version of the standard SRMM plugin for the Miranda Instant 
messenger. It is based on the latest SRMM code (07/01) and it may only work with
recent versions of Miranda IM. It's only tested with the stable 0.3.3.1 and recent 
nightly builds. It will probably NOT work with older miranda builds.

CAUTION: This should be considered "alpha" quality software. You should expect bugs
		 and other problems. I take absolutely no responsibility for any damage
		 caused by this piece of software, including, but not limited to corrupted
		 databases or other loss of data.
		 
		 This plugin is provided on an "as is" basis, but without ANY warranty.

HOW TO INSTALL
--------------

copy one of the .dll files (unicode or non-unicode) to your plugins folder and
thats it. PLEASE NOTE THAT THIS PLUGIN NEEDS AT LEAST MIRANDA VERSION 0.3.3 and
will NOT WORK with older releases.

If you use the popup plugins and want working event notifications with tabSRMM,
you need to copy the NewEventNotify.dll to your Plugins folder as well, replacing
any other version.


2. Features:
------------

* "tabbed messaging". All message dialog windows are now opened within a "container"
   window. A simple tab bar can be used to switch between the message windows.
   Currently, there is only one container and no way to make one or more message
   windows "top level". Future versions of this plugin may behave different. I plan
   to implement multiple container windows and the ability to attach or detach
   message windows to these containers.
   
*  A few  new options for the message history. You can find them on the "message log"
   option page. First, you can decide wheter you want to display the events (the
   actual message text) in a new line (ICQ style)

   You can also display the timestamp including seconds, display the 
   timestamp/nickname underlined and specify an indent value for the 
   actual message.
      
   Some code for these features was partially ripped from the srmm_mod 
   plugin, which is also a SRMM modification written by kreisquadratur.
   
3. Future, TODO etc...
----------------------

- More sophisticated tab containers. I like the way in which they are implemented
  in Trillian, and this is probably the way I will implement them here.

- bug fixes

- more cosmetic changes for the message log (maybe separator lines, borders etc..)

- redesigned message dialogue. Maybe a quote button and some layout changes.


4. Known bugs
-------------

* Window flashing is slightly broken. Tab flashing should work fine, but 
  the container window sometimes flashes when it actually should NOT. 
  
* tabs may not display correctly with some visual styles. I have tested
  a few themes and the plugin did work fine with all but one of them.

  
5. FAQ
------

Q: How to install this plugin?
A: copy the dll to your plugin folder. Make sure, that no other plugin 
   providing the same service (message dialog) is there, OR you will need
   to configure it on the options->plugins page.
   
Q: How can I close a tab?
A: Hit ESC or click the close button (red X) in the top right corner.

6. Credits:
-----------

The Miranda-IM development team and all contributors for making the best
IM client on this planet (and probably in the entire universe :) )

sryo (teodalton@yahoo.com) for the minimal icons. 

License: GPL
Contact me at:         mailto: bof@hell.at.eu.org
				       ICQ:    7769309
                       MSN:    alex_nw@hotmail.com
		 
