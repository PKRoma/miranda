----------------------------oOo----------------------------

                      Chat v 0.1.0.3

	     Released 2004-05-26, last updated 2004-06-21

-----------------------------------------------------------

What?
-----
This plugin provides a groupchat interface for any protocol that wish to use it. 
End users need not worry about anything more than setting your preferences in
hte options pages, but developers can take a look at the included m_chat.h for
an introduction on how to use it. Feel free to contact me if you need more 
information on how to get it working.



Who?
----  
My name is Jörgen Persson, also known as Matrix or m3x, and I am a 27 years old 
Swedish student at 'Stockholm School of Economics'. I have no background in 
coding apart from a few courses I took while studying 'Engineering Physics' at 
'Chalmers' in Gothenburg. I have been using Miranda IM now since 2001 and so 
far I have been responsible for the IRC part and I am nowadays considered to be
a 'core guy'. Otherwise I have mostly contributed with graphics to various 
other plugins or for Miranda IM itself. Most well known is probably my 
'Original Gangsta Icons' collection. If you wish to contact me you can e-mail 
me at: i_am_matrix at users dot sourceforge dot net. You can also join #miranda 
on the FreeNode network, where you can talk to me and other fans of Miranda IM. 



Installation?
------------- 
Use an unzipper of choice (I use WinZip) and extract the files to the 'Plugins' 
directory of your Miranda IM installation. Please note that you must restart 
Miranda IM for the installation to complete. If you are upgrading to another 
version you will have to shut down Miranda before extracting the files 
or the process will fail.

The files included in the .zip you have downloaded are: 

chat.dll - the actual plugin. 
chat_Readme.txt - a brief description of the plugin 
chat_license.txt - a copy of the license of the plugin (GNU GPL) 
chat_Translate.txt - translatable strings for people who like to translate 
m_chat.h - for developers
InstallScript.xml - a script for the Miranda Installer 
  


Credits? 
-------  
Egodust, The coding encyclopedia :-) Thanks! 
  


Miranda IM?
-----------   
Miranda IM is an open source instant messenger framework that support plugins for many different networks; 
ICQ, MSN, YAHOO, Jabber, IRC and AIM to mention a few (but not all). http://miranda-im.org 
 


Base Address?
------------  
0x54 110000 (matrix)  
  


Version History?
----------------    
v 0.1.0.5
- Bugfix:  CTRL+Enter did not work correctly
- Bugfix:  there was an annoying sound when using certain special keys
- Bugfix:  the statusbar popups did not work
- Other:   the popups can now use the same color scheme as in the log
- Other:   when using CTRL+Up the caret is placed at the end
- Other:   commas (,) can be used as delimiter in the highlight string

v 0.1.0.4
- Bugfix:  It crashed sometimes when sending a '%' character
- Bugfix:  The highlight was wrong sometimes

v 0.1.0.3
- Feature: command history (CTRL UP-ARROW/DOWN-ARROW)
- Feature: Support for the PopUp (or similar) plugin
- Feature: Blinking icons in the tray for chat room events
- Feature: Option to strip colours from incoming text
- Bugfix:  It was not possible to have an empty group name to add chat rooms to
- Bugfix:  On some incoming text the font size would be extremely big
- Bugfix:  The plugin could crash when moving scrollbars (attempted bugfix) 
- Bugfix:  Colours were sometimes displayed wrong
- Bugfix:  The '%' character were displayed wrong sometimes and identifiers did not work
- Bugfix:  Text containing fomatting were not highlighted correctly
- Bugfix:  It was possible to 'quick connect' even if Chat ws not installed
- Bugfix:  Topic in the statusbar disappeared sometimes
- Bugfix:  Nick name auto find (TAB) did not work correctly
- Bugfix:  The log to file feature was broken and could crash and behave bad
- Other:   The plugin is not dependant on IRC_icons.dll anymore, removed it from the installer also


0.1.0.0 (04-05-26)

- initial release



Copyright and license
---------------------
Copyright (C) 2004  Jörgen Persson

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