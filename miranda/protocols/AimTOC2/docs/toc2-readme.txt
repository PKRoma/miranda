Aim TOC2 readme
---------------

Frequently Asked Questions(FAQs)

	Q: Connection Problems?

	A: Reset your server & port to the default by pressing reset on the options dialog.

	Q: Profiles don't work?

	A: Profiles will only work from the default server & port. If they still aren't working re-login.

	Q: My buddies don't appear as online?

	A: Sync your buddies by either right clicking and clicking sync or selecting the full list sync in the AIM main menu properties.

	Q: The default server will not connect!

	A: Sometimes, the aimexpress.oscar.aol.com server hangs, try toc.oscar.aol.com and port 5190 instead.

Contact:

	Original Author: Robert Rainwater (rainwater at miranda-im.org)
	Modification: Aaron Myles Landwehr (aaron at snaphat.com)

Still need help?

	aim id: snaphatvirgil
	msn id: aaron@snaphat.com
	yim id: snapdaemon
	icq id: 197688952

Change Log:
Version 1.4.1.9

	-Removed the check added in 1.4.1.8. On further inspection tdt should never be dereferenced- initiated to NULL, free'd, and reinitialized as needed.
	-Fixed utf bugs that caused groups to not be added to miranda; which, in turn caused users in those groups to not display.
	-Added support for the "INSERTED2" command(buddies that are added to the server-side list from other clients- will be added in real time to miranda's list.

Version 1.4.1.8
	
	-Added a extra check so that miranda doesn't crash if tdt is a zero pointer.
	-Added utf support for group names.(If the database supports utf.

Version 1.4.1.7
	
	-fix profiles to work when connecting via a proxy
	-fixed bug that could potentially cause crashes when connecting via a proxy(MOST LIKELY).
	-Cleaned up some old & somewhat useless code.
	-fix addition of buddies from the clist and messagedialog(should work anytime now).

Version 1.4.1.6

	-Changed strcpy() and strcat() functions to buffer overun safe mir_snprintf().
	-Fixed bug causing main menu to appear in the incorrect spot.
	-Added a note to the options dialog about the default server.
	-Refined Typing notify even more(added handling of "has entered text" case, and corrected the timing of typing notify).
	-Fixed a bug that potentially could cause the deletion of all buddies if a database key was present.
	-Removed old buddylist syncing code. 

Version 1.4.1.5

	-Refined typing notify(user is typing removed after message is sent).
	-Added an option to remove the sync entire list main menu option.
	-Buddies that are online no-longer show the sync button on right click.
	-Show profiles no-longer shown for offline contacts.
	-Moved menu items to group with the "Message" in right click menu.
	-Fixed a bug that caused "temp" user to be sent to AOL for deletion.
	-Changed the default login server to "aimexpress.oscar.aol.com"(For buddy profiles to work).
	-Fixed userinfo to stop attempting to update forever.
	-Removed buddy profile dialog from userinfo.
	-Changed the "Show Profile" code to use the login server instead of "205.188.179.24".

Version 1.4.1.4

	-Fixed an error causing getpeername() to always fail.
	-Changed the default login server and port to 205.188.179.24:5190.
	-Added show profile menu item.
	-Profiles now work.
	-Changed login procedure to better resemble AimExpress's login sequence.


Version 1.4.1.3

	-Fixed Tooltip bubble appearing even when disabled.

Version 1.4.1.2

	-Contacts do not unhide after logging in.(if hidden is set.)
	-Double Buddies should be fixed.(Removes extra copies of a buddy from the server-list.)
	-Colon no longer cuts off the rest of chat messages.
	-Single user sync works again.
	-Removed a login sequence typo.
	-Added typing notifications.
	-Changed options menu item "Show error messages" to "Show tooltip messages".
	-Removed useless "Update buddies from server on nest login".
	-Forced updating of buddy status after addition.

Version 1.4.1.1

	-Fixed chat.
	-Removed annoying message boxes.
	-Fixed auto add buddy on receiving a message bug.
	-Added full buddy list sync.

Version 1.4.1.0

	-Removed alpha build flag
	-fixed crash that happened when server-side buddy list had no buddies
	-changed login sequence to enable ichat users again(many thanks to elaci0 for noticing and supplying network logs)

Version 1.4.0.9

	-Fixed sync menuitem to only appear on aim contacts.(Caused crashes?)
	-Fixed buddy deletions to work when there is a space in the group name.
	-Fixed buddy additions to work on the Find/Add Contacts window.
	-Displays a notification when notified that a buddy has been added to the server list.


Version 1.4.0.8

	-Changed to generic TOC2 instead of AIMExpress TOC2 per request from a user. Allowed to connect from any port now.:)
	-Finally corrected the elusive 'ne:' or 'ne' user addition bug. Turns out it was the final piece of the CONFIG2 command AOL sends: 'done:'
	-Corrected a massive error that wouldn't update new server-side buddies and add them to miranda's database and in some cases caused crashes.

Version 1.4.0.7

	-Redid buddy list syncing(Now manual- you must right click on the buddy and select "Sync Buddy with Server-side list").
	-Doesn't auto add buddies to group "1".
	-Invisible chatting now avaliable..
	-Disallows connections to any ports besides the two working ones.

Version 1.4.0.6

	-Added syncing of buddy list with server-side list.
	-Removed garbage user creation.

Version 1.4.0.5a

	-Just a zip fix. No change to the dll.

Version 1.4.0.5

	-Now compatible with updater plugin.

Version 1.4.0.4

	-Added buddy addition and deletion.
	-Removed memory hogging caused by the creation of extra variables.
	-Added an extra error check.

Version 1.4.0.2

	-fixed a rogue database setting that caused login's to be impossible without manual modification to the miranda database.

Version 1.4.0.1

	-Original modification and implimentation of the TOC2 protocol.