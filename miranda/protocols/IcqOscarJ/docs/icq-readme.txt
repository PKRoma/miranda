
          ICQ protocol plugin for Miranda IM (Joe)
          ________________________________________


About
_____

This Miranda IM plugin makes it possible to connect to the ICQ
instant messenger network and communicate with other ICQ users.


Changes
_______

0.3.4.2
Improvements
  Server-side list support rewritten, now uses acking.

0.3.4.1

Bugfixes
  Now parses URL send ack correctly, no more timeout
  Now sending ack on contacts receive

New features
  Added reliable client identification (if not identified, gives appropriate ICQ client)
  Added support for avatar images (downloading only).

Improvements
  Basic search now automatically removes garbage from text, e.g. it can search by 123-456-789


0.3.4.0

Bugfixes:  
  Fixed authorisation reply
  Fixed contact transfer ack

New features:
  Added full unicode message support (require SRMM Unicode)
  Added support for sending and receiving Idle time.


0.3.3.1

Bugfixes:
	Could crash when receiving unicode messages.


0.3.3

Bugfixes:
	Failed to send or receive files from ICQ 2003b.
	Fixed a number of smaller memory leaks.
	Contact e-mail info was not displayed correctly.
	Failed to retrieve user details during certain circumstances.
	URL messages could disappear when sent through a Direct Connection.
	Nick name was not deleted from server list when local nick name was deleted.
	Server side contacts could reappear after being deleted if they were on the
	visible/invisible lists.
	Changing status while connecting had no effect.
	A bunch of other fixes that are too boring to list here, have a look at
	the CVS changelog if you want the big list.

New features:
	Added support for sending and receiving Typing Notifications.
	Now accepts messages formatted in unicode (note: this wont solve the problem
	with displaying messages with multiple charsets).

Improvements:
	Uses plugin DLL name in various menus to make it easier to have several ICQ
	plugins loaded.
	More robust packet parsing reduces the risk of any future stability problems.


0.3.2

Bugfixes:
	Prevent your status messages being read when you are invisible.
	Small memory leak when sending an SMS.
	Fixed a dumb bug that caused random disconnections from the ICQ server.
	Cleaned up the code for searches and user info updates, should work better now.
	AIM users in your server contact list would get added locally with uin 0.

New features:
	Added "missed message" notification.

Improvements:
	Added better error messages for message send failures.
	Messages now default to the most reliable delivery method available for a given contact.


0.3.1

Bugfixes:
	Fixed crash when incoming file transfer was cancelled before the transfer started
	Failed to receive SMS messages sent while offline
	Fixed some problems with accepting file transfers from Mirabilis clients
	Increased thread safety to reduce some reported connection/disconnection problems
	Fixed compability problems with sending messages to some jabber clients
	Fixed some message compability problems with Trillian
	Corrected some ack sending
	Added a lot of safety checks to increase general stability
	Plugin didnt load unless winsocks2 was installed

New features:
	Removed restrictions on UIN length for better compability with iserverd
	The password can now be left empty in options, it will be asked for during login
	Server port can now be left empty in options, a random port will be selected during login
	Show logged on since and system uptime info in ICQ tab

Improvements:
	Reduced file size
	Message sending now uses a more reliable delivery method



Support and bug reporting
_________________________

We cannot give support on e-mail or ICQ. Please visit the Miranda IM help page at
http://www.miranda-im.org/help/ if you need help with this plugin.

If the help page does answer your question, visit the Miranda support forum at:
http://forums.miranda-im.org/ and we will try to assist you.

If you want to report a bug, please do so in the official bugtracker at:
http://bugs.miranda-im.org/



Contact
_______

Current maintainer is Joe @ Whale, jokusoftware at users.sourceforge.net
                      Martin Öberg, strickz at miranda-im.org



License and Copyright
_____________________

Copyright (C) 2000-2005 Joe Kucera, Martin berg, Richard Hughes, Jon Keating

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