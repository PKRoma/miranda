----------------------------oOo----------------------------

               IRC Protocol Plugin v 0.4.3.5

  Implementation of IRC RFC-1459, RFC-2812 for Miranda IM


	     Released 2003-03-03, last updated 2004-06-28

-----------------------------------------------------------


Please visit the online resource for the IRC plugin at
http://members.chello.se/matrix/ for more information.

To install OpenSSL if you need to, go here: http://www.openssl.org/related/binaries.html

This protocols is dependant on the Chat plugin, get it here: http://www.miranda-im.org/download/details.php?action=viewfile&id=1309


History
-------
0.4.3.5
- Bugfix:  Misc. problems with different codepages (scripts)
- Bugfix:  In rare circumstances it could be problematic to join a channel
- Bugfix:  topics were not saved in the channel manager
- Other:   \n is ok as line delimiter now (as opposed to \r\n) which some plugins use (SpamReturner for one)

0.4.3.4
- Bugfix:  Contacts were not shown as online sometimes

0.4.3.3
- Feature: New command: /ECHO [on/off]. Use it to enable or disable local echoing of typed commands
- Bugfix:  The plugin would crash sometimes
- Bugfix:  Multiple modes did not work correctly
- Bugfix:  On some networks messages had unwanted characters in them
- Bugfix:  Misc. bugs related to sending '%' characters
- Bugfix:  /clear did not work in the server window
- Bugfix:  The global status was not correctly set
- Other:   New IRC_servers.ini (Upgrading will delete your old servers)
- Other:   Remapped 'Free for chat' status as Online in IRC
- Other:   Added status messages when (re-)connecting
- Other:   You can now see who sets the topic
- Other:   Owner, admin, op, halfop, voice and normal is rcognized as user modes, let me know if there are more that should be in.
- Other:   'Show server' right click menu item in channels

0.4.3.0
- Feature: Now use chat.dll for its chat windows
- Feature: SSL support, courtesy of Athos (Daniel Bergholm). Get OpenSSL here: http://www.openssl.org/related/binaries.html
- Other:   Very much of the code has been revisited. 

0.4.1.6
- Bugfix:  removed textlimit for the channel log
- Bugfix:  server edit box did not resize properly
- Bugfix:  an extra new line was added to the message history when sending
           on double enter.
- Bugfix:  not all that should could use the chan. manager
- Bugfix:  /clear [#channel|server] did not clear the server correctly
- Bugfix:  channels with password were cloned in the "join channel" dialog
- Bugfix:  the channel event icons flashed too often
- Bugfix:  the message headers' text was cut off
- Bugfix:  the popups for highlight could not be turned off
- Bugfix:  crash when doubleclicking empty space in the nick list
- Bugfix:  12 am is not 0 am
- Bugfix:  the kick event used the color for part
- Design:  redesigned most dialogs
- Design:  moved all icons to an external .dll to allow icon packs etc
- Design:  new icons
- Other:   removed all default sounds
- Other:   requires MIM 0.3.3
- Other:   added quick setup dialogs for newbies, and to work better with the installer


0.4.1.1
- Bugfix:  Icons were screwed in win 9x
- Bugfix:  The kickban kicked the user from the channel (hehe)
- Bugfix:  Doubleclicking on a channel window brought up a message window also sometimes
- Other:   Added /IGNORE [ON/OFF] to the ignore system.

0.4.1.0
- Feature: ignore system [734037]
- Feature: channel manager [734083]
- Feature: more menu options, like KickBan etc
- Feature: support for the plugin uninstaller
- Feature: a nice statusbar in channel windows
- Feature: New commands: /IGNORE, /UNIGNORE and /CHANNELMANGER
- Bugfix:  the Perform buffer did not always save
- Bugfix:  if "Group" was blank it did not behave
- Bugfix:  User ID did not allow long strings
- Bugfix:  fixed a few memory leaks
- Bugfix:  it did not always remeber window positions, hopefuly it does now
- Other:   added a %mode identifier
- Other:   added a $?[="text"] identifier. 
- Design:  Removed the topicbar

0.4.0.8
- Bugfix: The perform buffer did not always work
- Bugfix: The 12 hour timestamp showed 00:00PM instead of 12:00PM
- Bugfix: Other minor stuff.


0.4.0.6
- Feature: Options to control the automatic online notification system
- Feature: New commands /BUDDYCHECK [ON] [OFF] [TIME X] where x is the 
           number of seconds the plugin wait to do automatic checks of
           the buddy list. 
- Bugfix:  The plugin crashed randomly
- Bugfix:  The plugin sometimes crashed while joining a channel
- Bugfix:  Longs passwords did not work in the quick connect window
- Bugfix:  The hostmask did not always show in mTooltip
- Bugfix:  It showed outgoing private messages in the channel window also
- Bugfix:  It could not use modified _servers.ini well
- Bugfix:  Channels would show as "parted" incorrectly sometimes
- Bugfix:  Colours leaked to the next line
- Bugfix:  Aliases was broken somewhat
- Bugfix:  Menus show up correctly on mullti monitor systems (Thanks Tornado)
- Other:   Translators, new strings added.
- Other:   Added a new IRC_servers.ini

0.4.0.0

- Feature: Contacts can now be added to the contact list [734040]
- Feature: Online/Away notification for contacts
- Feature: The query windows use mirandas internal message windows
- Feature: Advanced identification of contacts using hostmasks
- Feature: Icons in channel windows now change according to if you
           are on the channel or not
- Feature: Better highlight function using wildcards
- Feature: Option to see addresses in joins, parts etc.
- Feature: Option to keep connection alive, which is also useful 
           for detecting silent disconnects
- Feature: Option to change the color of the nicklist and typing 
           area backgrounds
- Feature: Added sound events to join, part and other major events
- Feature: Redesigned whois dialog to show more info, incl idle 
           time and operator status
- Feature: It can now bounce (change servers by request of the 
           server you are connecting to)
- Feature: Option to remove the topicbar in channel windows
- Feature: Option to join all channels minimized
- Feature: Right click menus for channels
- Bugfix:  The topic of a channel did not always show.
- Bugfix:  Incoming /mode commands could crash miranda and otherwise 
           behave wrongly
- Bugfix:  Leading spaces in messages were removed
- Bugfix:  The plugin shoed popup's even though the window was the 
           active one
- Bugfix:  The plugin did not log channels with characters that are 
           illegal in filenames
- Bugfix:  Ctrl- a did not work in channel windows
- Bugfix:  Passwords did not save in the join channel dialog
- Bugfix:  Various memleaks
- Bugfix:  The retry timer could cause mysterious disconnects and 
           problems logging on.
- Bugfix:  The plugin behaved badly when trying to connect to networks 
           while in connecting status
- Bugfix:  The plugin did not portcycle on reconnect
- Bugfix:  A blank away message did not set away
- Bugfix:  Text did not always show in win 98
- Bugfix:  The plugin could not use long passwords
- Bugfix:  The system tray popup was buggy
- Other:   Changed the autaomatic line-break limit to 400 chars
- Other:   mTooltip support
- Other:   Removed option to be invisible (+i) automatically and added 
           option to automatically force visibility (-i) instead.
- Other:   Removed the ident netlib entry. It still use netlib though
- Other:   The plugin will now go online on an incoming 005 command 
           instead of 001
- Other:   Removed /chanserv, /cs, /ns and /nickserv again. They should 
           not be hardcoded! Use an alias instead
- Other:   The plugin is not linking dynamically to run time libraries 
           anymore. This caused heaps of mysterious problems
- Other:   Removed option to always open link in new browser
- Other:   Removed redundant code related to WinSock and the old query windows
- Other:   The plugin need miranda 0.3.1
- Other:   New commands: SHOWSERVER and HIDESERVER. guess what they do.
- Other:   Translators: New strings added

0.3.4.1 (03-08-14)
- Feature: Option to automatically join a channel when invited
- Bugfix:  Multibyte characters (cyrillic, hebrew etc) did not work well
- Bugfix:  Trying to install on MIM before v 0.3 would crash Miranda. This bug 
           also caused Wassup to crash. Many thanks to EgoDust and Tornado.
- Bugfix:  Quit messages were sometimes redirected wrong
- Bugfix:  Deleting the performfile while keeping 'perform' crashed
- Bugfix:  Multiple line away messages caused weird behaviour or crashed.
- Bugfix:  If first letter in a msg was ':' then that character was not sent
- Bugfix:  Empty line of text showed when doing an emopty 'query' to open the window.
- Other:   Reduced minimum height of the editbox that the user can set.

0.3.4.0 (03-08-10)
- Feature: Option to disable the server error tray balloons
- Bugfix:  Perform and server files were destroyed after hitting apply twice
- Bugfix:  The popups had incorrect automatic linebreaks
- Bugfix:  The tray balloon for server errors showed color codes
- Bugfix:  A colon (:) was sometimes added incorrectly to outgoing commands
- Bugfix:  'Remember window positions' did not handle minimized windows well
- Bugfix:  Channel is now always shown when doubleclicking an event on the CList
- Other:   The notifying of channel messages (italics) does not react to /me commands anymore

0.3.3.2 (03-08-08)

- Feature: New alias identifier #$1 etc. allows you to type /j &channel or /j ##channel
           without modifying your old aliases.
- Feature: New command: /hop (use it like in mIRC)
- Bugfix:  Channels or PM's with invalid file characters could not be reopened
- Bugfix:  Icon sometimes disappeared from windows
- Other:   Removed identifier ## in commands. Sorry, but necessary.
- Other:   Decreased filesize 50kb (Dynamically linking msvcrt.dll)

0.3.3.1 (03-08-01)
- Bugfix:  Connecting to some networks crashed 
- Bugfix:  Aliases did not work correctly (again)
- Feature: New commands: /raw and /quote

0.3.3.0 (03-07-31)

- Feature: Drop down list in 'join channel' and 'change nick' dialogs
- Feature: New commands: /cs and /chanserv [734086]
- Bugfix:  Plugin could only handle channels prefixed with '#'. This is 
           a big change. Report bugs please.
- Bugfix:  Plugin could only handle users prefixed with '@' or '+'
- Bugfix:  /log command behaved badly sometimes.
- Bugfix:  Timestamp displayed wrong sometimes
- Bugfix:  First alias in list did not work
- Bugfix:  Commands did not work with spaces in front
- Bugfix:  'Out to lunch' and 'On the phone' event did not work
- Bugfix:  Plugin could not connect for all users
- Bugfix:  The perform- and serverlist did not always save
- Bugfix:  Reconnect did not always port cycle
- Bugfix:  /MODE can now handle more than three users at one time [734025]
- Bugfix:  Log folder was created even when logging was disabled
- Bugfix:  Alias identifier ## did not always work right.
- Design:  /LIST dialog redesigned, and warning added for large networks
- Other:   Improved performance in /LIST dialog.
- Other:   TRANSLATORS: new strings added.

0.3.2.1 (03-07-28)
- Bugfix:  The Aliases initialized incorrectly. Made it impossible 
to install for new users.
- Bugfix:  The 'Perform on event disconnect' didn't do /quit any good


0.3.2.0 (03-07-28)

- Feature: 'Perfom on event'. Allows for things like automatic
           nick changes when changing status etc
- Feature: Many more identifiers in commands, aliases, perform and
           other. Among other can be mentioned 12 hour timestamp. Do
		   yourself a favour and read the readme.
- Feature: Multiline aliases, see the readme.
- Feature: Formatting the titlebar [734102]
- Feature: Option to remember window positions [734101]
- Feature: Option to set popup color and timeout
- Feature: Sortable columns in the channels list
- Feature: New commands: /ame, /ns, /nickserv
- Bugfix:  Various crashes when sending commands
- Bugfix:  Linebreaks in logfiles was faulty
- Bugfix:  Deleting contact routine locked other protocols
- Bugfix:  Removed the ping-pong event from the server window
- Bugfix:  All windows did not close on disconnect
- Bugfix:  Running an alias will sometimes crash miranda [734028]
- Design:  Added menu option to do a /LIST
- Other:   Redesigned the IRC GUI options
- Other:   *Lots* of under-the-hood changes (report bugs please)

0.3.1.0 (03-07-14)

- Feature: Tray and CList notifying of events.
- Feature: Added new event: channel message. 
- Feature: Option to set the send method independently.
- Feature: Option to minimize to CList on the close (X) button
- Feature: Option to enable old school TAB autocomplete
- Bugfix:  Password box was disabled in options
- Bugfix:  Colorcodes showed in popups
- Bugfix:  CTRL-ENTER did not work as in Miranda IM.
- Bugfix:  It was not possible to minimize to no group
- Bugfix:  The user count in the channelbar was sometimes incorrect
- Bugfix:  Attempt to solve shutdown bug for some people
- Design:  Highlight words should be longer than two characters
- Design:  Slight redesign of the whois dialog
- Design:  Added 'Leave channel' command to the minimized window
- Design:  Added nine icons for different events
- Other:   Removed options 'Catch URL' and 'Hide PING PONG'
- Other:   New base address 0x54
- Other:   TRANSLATORS: New translate strings. The last ones weren't good for all.

0.3.0.2 (03-07-08)

- Bugfix:  Miranda crash on receiving url's longer than 50 characters
- Bugfix:  Miranda crash on receiving colorcodes. Bug in new code.

0.3.0.0 (03-07-07)

- Feature: NetLib support (allows the use of proxy) [734048]
- Feature: Minimize channels to contactlist [734047]
- Feature: Send using ENTER, double ENTER, TAB_ENTER or ALT-S [734058]
- Feature: Use Mirandas away message dialog. [734085]
- Feature: Translation support [734049]
- Feature: Support for plugin sweeper. [734035]
- Feature: Flash window on highlight option [734060]
- Feature: Support for %me in highlight [734064]
- Feature: Option to not connect on startup.
- Feature: new commands: /JOINM, /JOINX 
- Feature: Popups on /join, /part, /mode etc
- Feature: Multiserver through DLL renaming
- Bugfix:  Crash in options dialogs [734013]
- Bugfix:  No "Connecting" status [734068]
- Bugfix:  Sending '%' crash [734009]
- Bugfix:  Editbox resizes if windowborder is clicked [734021]
- Bugfix:  Big icons are not loaded [734033] 
- Bugfix:  Crash if log directory is invalid [740436]
- Bugfix:  Crash if IRC_perform.ini does not exist [734010]
- Bugfix:  Crash in Quick connect dialog [734016]
- Bugfix:  Numbers in Perform combobox [734022]
- Bugfix:  Join channel dialog does not cut leading #'s[734001]
- Bugfix:  Incorrect Menu on dual monitor system. [734026]
- Bugfix:  Improved thread safety (Stability)
- Design:  Send button in channel/query (to allow ALT-S) [734058]
- Design:  Nickname autocomplete is activated by SHIFT-TAB [734058]
- Design:  Command history now use CTRL-UP and CTRL-DOWN
- Design:  Redesigned channel/query/server windows
- Other:   Requires minimum Miranda IM v0.3 or v0.3.1 for Netlib Ident
- Other:   Filesize reduced 10%
- Other:   Moved all options pages to 'Network'
- Other:   Removed 'space' as delimiter in highlight words
- Other:   Updated serverlist

0.1.1.0 (03-03-16)

- Feature: Supports InstallScripts for MirInst.
- Bugfix:  fixed so the messagewindows always scroll to the bottom when they 
           should.
- Bugfix:  fixed various problems with the log window in win 98/95 (colors, 
           newlines and other) 
- Bugfix:  the aliases now works correctly.
- Bugfix:  the right-click menu will now work even if help.dll is installed.
- Bugfix:  the 'Rejoin channels on connect' would not work in some occasions
- Bugfix:  Crash when closing a channel/user window when the name contains 
           letters which are not allowed in windows filenames.
- Design:  Changed the layout of buttons to comply with windows standards

          
0.1.0.0 (03-03-03)

- initial release


Copyright and license
---------------------
Copyright (C) 2003  Jörgen Persson

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