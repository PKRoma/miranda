About
-----

Conversation Style Messaging Plugin 
Version 0.995
by Christian Kästner (christian.k@stner.de)
for Miranda ICQ 0.1
written with Delphi 5 Pro


Description
-----------

This plugin offers a conversation style messaging ability for 
Miranda ICQ. Like most instant message programs you see the 
history above the input field. Additionally it has a 3 different 
display styles and couple of nice features and options.


Features
--------

+ Smart interface (as small as possible)
+ 3 different display styles: simple memo, richedit (different fonts)
  and grid (as in History+)
+ Customizable Fonts
+ Different font for incoming and outgoing messages, time and name
+ Customizable Text style (eg. %TIME%: %TEXT%)
+ Save position and send method for every user seperately
+ Possiblity to load recent messages when showing the window
+ Having a message queue that allows to type a new message 
  while sending the old one.
+ Send messages larger than 450 characters by splitting them
+ Autoretry send message if timeout occurs.
+ Customizable timeout length
+ Several send methods like ctrl+s, DoubleEnter or SingleEnter
+ Different styles to popup on incoming message


Options
-------

The option dialog is somehow hided. You can find it in the system
menu which appears when you click the icon at the very top left
of the window in the caption bar.


Installation
------------

If you use 0.1.0.1 everything is fine, just copy it into
the plugin subdirectory.

If you use 0.1.0.0 please read this:

Just copy the dll into Miranda's plugin subdirectory and
delete or rename the splitmsgdialogs.dll.

WARNING: Remove the split msg plugin before, otherwise
you have to choose between the both and cannot undo this
selection until Miranda 0.1.0.1 is out.

I recomment renaming the unused plugin to 
splitmsgdialogs.dl_ or convers.dl_


Source
------

The full source for this plugin can be found in the Miranda CVS
at Sourceforge.


License
-------

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


For more information, e-mail christian.k@stner.de


Changes
-------

0.995
* Fixed crash in 0.1.0.1 or earlier

0.994
+ Added langpack support for the frontend (not options dialog)
* Fixed storing of position when not setting per user

0.993
* fixed that Alt-Gr (right alt key) is not used for shortcuts
* increased timeout by default, because retry is handled now by Miranda

0.992
* fixed arrow keys
* tab enter focus fixed
* typo fixed
* don't flash active window

0.991 (yes I know this version numbers are mad, but I don't want it to be 1.0 
       before the options are integrated into miranda options)
* Better support for shortcuts
      
0.99
++ 3 Different display styles to choose with many
   options each
+ Option how to popup on incoming message
+ customizable timeout 
+ Autoretry
+ Tab+Enter as shortcut
* some minor bugfixes

0.95
+ Added Sound for incoming message
+ Added ability to overwrite send way (bestway, direct, 
  through server), stores last decision
* New buttons used
+ Added customizable historymemo with different fonts for
  incoming and outgoing messages

0.9
* Ported for Miranda 0.1
* Better send message support (waits for timeout etc)
* Font completely customizable
+ Message queue added
+ Ability to load recent messages
+ DoubleEnter and SingleEnter send methods
* window icon is now online icon from contact list
* some more...

0.2
* Changed stdcall to cdecl in callback routines
