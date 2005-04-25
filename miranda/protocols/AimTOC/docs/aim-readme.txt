################################################################
# AIM Protocol Plugin 
#
# AOL(r) Instant Messenger Protocol for Miranda IM
################################################################

About
-----
The AIM Protocol plugin for Miranda IM provides AOL(r) Instant 
Messenger support using AOL's TOC protocol. 


Features
--------
 - Send/Receive messages
 - Add/Delete users from server-side list
 - AIM user info tab
 - Show online/idle time for AIM users
 - Auto-idle time for self
 - Warn user
 - Show auto response messages
 - Option to send auto responses to contacts when away
 - Connect using random ports
 - Supports Miranda's visibility user options
 - Read and set profile information
 - Format screenname
 - Change password
 - Group chat support
 - Receive files


Requirements
------------
 - Miranda IM 0.3.4


History
-------
1.2.0.0
 - Chat support uses Chat plugin

1.1.0.0
 - AIM links option didn't register correctly
 - Security fixes

1.0.9.6
 - Show mobile users as 'on the phone'
 - Support Miranda's idle settings for setting idle
 - Extra contacts from server list are added on login (optional)
 - Remove SSI sync support
 - Removing user from deny list didn't always update mode in Miranda
 - Fixed file transfers with Mac users
 - Fixed possible crash

1.0.9.5
 - Problems with some character escape sequences

1.0.9.4
 - Groupchat log to file respects time/date stamp option
 - Minor translation changes
 - Group chat invites were broken in 1.0.9.3

1.0.9.3
 - Try last known working server if connection to AIM fails
 - Don't show empty message in file request description
 - Groupchat log sometimes became full
 - Limit outgoing groupchat message size
 - Added smileyadd support to aim groupchat
 - AIM groupchat invites are now added as a clist event (tray flash, no autopopup)

1.0.9.2
 - Minor threading issues during connection

1.0.9.1
 - Improved connection handling

1.0.9.0
 - Require Miranda 0.3.3 or greater
 - Prevent away message flooding on the server
 - Added user search capability
 - Show when user came online in user info
 - Focus message area when groupchat is opened
 - Remove registry keys when unchecking aim: links support
 - Added abillity receive files
 - Send messages to groupchat users by double clicking name

1.0.8.0
 - Given option to disable plugin when the firstrun dialog is cancelled
 - Groupchat window sometimes flashed even after being focused
 - Add user count to groupchat window title
 - Changed default setting to not minimize groupchat to tray
 - Turned AIM link support off by default (no registry writing for new profiles on firstrun)
 - Removed authentication server options (only login server is needed, can still be set in the database)
 - Ignore empty incoming messages
 - Unable to save "Only reply to users in your contact list" option

1.0.7.29
 - Encoding issues

1.0.7.28
 - Away mode was not always set
 - Web based profile available in background tab of user info

1.0.7.27
 - More UTF8 fixes
 - More nickname fixes
 - Added option to edit display name
 - Other minor changes

1.0.7.26
 - Minor typos
 - Add send button to groupchat
 - Warn menu in groupchat ignored option
 - UTF8 support
 - Import nicks from server-side list
 - AIM screennames are no longer saved as the "Nick"
 - Many language pack changes

1.0.7.25
 - Status flashed to offline to other users when logging in
 - Links inside of profile are clickable

1.0.7.24
 - Maximum length of message in message dialog enforced
 - Show reason in Miranda error message when trying to send message to offline user
 - Warn User menu in groupchat was always disabled
 - Improved url highlighting in groupchat
 - Added lost password link on change password dialog
 - Moved clear history list option to the join chat dialog

1.0.7.23
 - Crash parsing empty server-side list

1.0.7.22
 - Enforce 16 character limitation on screennames in find/add dialog
 - Viewing userinfo from search result didn't always show user's correct status/userinfo
 - Changing keep-alive option does not require a restart/relogin
 - Some minor logging changes

1.0.7.21
 - Automatically turn off server-side list support for list that are to large
 - Crashed when truncating large packets

1.0.7.20
 - Login flood when creating new users
 - Don't add users to empty group from server-side list group

1.0.7.19
 - Bug fix: Couldn't see ICQ users

1.0.7.18
 - Group chat fixes (invites and joins failed sometimes)
 - First run will only run once (OK or Cancel on the login dialog will disable it forever)
 - Other minor code changes

1.0.7.17
 - Status message was sent twice if using away message popups
 - Menu handle wasn't destroyed in groupchat url context menu
 - Added option to minimize chats to system tray

1.0.7.16
 - Users imported from server-side list were shown as offline on first import
 - Added flood protection when sending messages to fast (like the send to multiple feature)
 - Dynamically link to MSVCRT (smaller dll size)

1.0.7.15
 - Increased user info flood detection interval to 3 seconds
 - Changed group chat exchange range to 4-6
 - Bug fix: Minor packet parsing change
 - Bug fix: Startup status was always online

1.0.7.14
 - Added a delete queue while offline to fix server-side list user deletes
 - Bug fix: Blocked users would see you as online briefly during login
 - Bug fix: Users added from the server-side contact list didn't show their status
 - Bug fix: User updates didn't work correctly in non serverlist mode
 - Bug fix: You were able to delete a user if you started offline without setting the server list

1.0.7.13
 - Added chat logging support
 - Minor options changes
 - Minor changes to join chat dialog

1.0.7.12
 - Preserve contact list mode stored on server
 - Preserve permit lists on the server
 - Bug fix: Delete contacts while offline
 - Bug fix: Don't update server-side lists when deleting non AIM contacts

1.0.7.11
 - Added server-side contact list support
 - Use hi-colour AIM icon when possible
 - Possible memory leak in contact list loading
 - Bug fix: Remove minimize button on password change dialog

1.0.7.10
 - Userinfo for owner uses change password dialog if online
 - Only one password dialog can be open
 - Bug fix: DNS lookup didn't check DNSThroughProxy setting
 - Bug fix: Possible malformed userinfo requests were sent
 - Bug fix: Recent chats are now remembered correctly (old chats fall of list)
 - Bug fix: Recent chats joined from invite requests are now stored

1.0.7.9
 - Added option to hide group chat main menu item
 - Process aim:goim and aim:gochat links
 - Multiple account support (by copying the dll to another name)
 - Added ability to change your AIM password
 - Bug fix: Join Chat menu item was enabled during login/connecting
 - Bug fix: Groupchat window continued to flash after setting focus

1.0.7.8
 - Added balloon tip support for errors from server (requires Miranda 0.3.1a+)
 - Added balloon tip support for "You were warned" (requires Miranda 0.3.1a+)
 - More error messages from the server are recognized
 - Increased amount of times groupchat will flash on activity
 - Bug fix: Warning dialog was shown when warning was being lowered
 - Bug fix: Possible memory leak with groupchat context menus
 - Bug fix: Online message prompt was shown for online status (if options was on)

1.0.7.7
 - Added ICQ support (other end requires compatible ICQ client, see note in readme)

1.0.7.6
 - Clicking cancel on firstrun dialog will show firstrun the next startup
 - Added option to ignore groupchat invites
 - Bug fix: Improved proxy support
 - Bug fix: Better flood protection during login (due to userinfo requests)

1.0.7.5
 - Added clear chat list history option
 - Changed default exchange to 4
 - Improved window flashing in chats
 - Added show error messages option
 - Sending an invite request shows formatted screenname not contact list name
 - Bug fix: Invalid host name lookup request for firewalled users

1.0.7.4:
 - Flashwindow works for joins/parts
 - Bug fix: Recent chat list in join window was cutoff
 - Bug fix: Search for self never ended
 - Bug fix: Add to list was disabled for some users in groupchat
 - Bug fix: Duplicate chat entries are not added to recently used list
 - Bug fix: Crashes on exit/chat close

1.0.7.3:
 - Added /clear, /quit/ and /exit commands in groupchat
 - Private messages in groupchat are shown as private message
 - Previously joined channels are saved
 - Updated GCC compiler (gcc-3.2.3) used to build plugin

1.0.7.2:
 - Bug fix: Some possible threading/data corruptions fixed

1.0.7.1:
 - Made Miranda the default chat room
 - Option name for AIM Chat was wrong
 - Added note about using port 0 in options

1.0.7.0:
 - Added Group chat support
 - Send/Receive chat invites
 - Bug fix: First run dialog cut off textboxes
 - Bug fix: Parsing of html characters with & was incorrect
 - Bug fix: Possible infinite loop in html stripper
 - Bug fix: All html tags are removed now

1.0.6.0:
 - Acknowledge userinfo update complete when profile is obtained from server
 - Don't request profile for offline users
 - Userinfo dialog has more room for profile info
 - Bug fix: Correctly sets offline state on network errors

1.0.5.0:
 - Added option to add extra contacts from server's list to contact list
 - Groups are imported from the server with contacts
 - Removed server-side list sync support (it was buggy due to AIM's single level group limitations)
 - Added update visible list from server option
 - Bug fix: <p> tags were not parsed correctly

1.0.4.0:
 - Contacts are retrieved from the server on first run
 - Bug fix: Adding extra contacts from server list could flood server
 - Bug fix: Parsing some newlines cut of characters
 - Bug fix: Profile must be converted to html before sending
 - Bug fix: Changing away message while already away didn't send new status message

1.0.3.0:
 - User's profile integrated into user info dialog
 - Set profile information in View/Change Details...
 - Format screenname in View/Change Details...
 - Improved thread management to fix exit crashes using new Miranda 0.3 services
 - Improved parsing support
 - Defaults to use random ports (port 0 in options) for connecting (firewalled may users need to change)
 - Bug fix: Setting another protocol to away set AIM to away

1.0.2.0:
 - Initial Release


Note About ICQ/AIM Interoperability
-----------------------------------
The AIM plugin is able to communicate with ICQ users.  However, this feature
will only work if the users you are trying to communicate with are using
a compatible client.  As of now, this means they must be using ICQLite Alpha 
build #1211 or greater.  More ICQ clients will support this interoperability as
time goes on.  To add an ICQ user, just enter their ICQ UIN in the Screename
field of the Find/Add User dialog.


FAQ
---
 Question: 
 How come contacts are being adding to my contact list everytime I login?
 
 Answer: 
 This is due to the option "Add extra contacts from the server's list to my list".  Uncheck this option if you do not wish to add contacts from your server-side list.
 
 Question: 
 Does the AIM Protocol plugin support file transfer?
 
 Answer: 
 Not yet.  Only file receive is possible using TOC.  This is on the todo list.

 Question:
 I set the Server-Side Contact list option but Miranda doesn't use it.  Why?

 Answer:
 For large lists, server-side contact list support is turned off automatically because of
 a packet-size limitation of TOC.


Thanks To
---------
- AOL's TOC1.0 protocol document
- GAIM's implementation of TOC for which some of the code is based off of
- Matrix and Valkyre for providing the AIM icons


Copyright
---------
Copyright (C) 2003-2005 Robert Rainwater

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
