About
-----

LSecurity Plugin
Version 1.1
by Christian Kästner
for Miranda ICQ 0.1+
written with Delphi 5 Pro


Description
-----------

Plugin which asks for the ICQ password on startup and thus provides a 
low level security. If the password is incorrect it shuts down
Miranda.
It is only a low level security, because you can simply delete the
plugin to start Miranda, but even in this case you still need the
password to connect via the ICQ protocol.


Features
--------

+ Askes for password on startup and thus prevents other people
  from logging in or reading your history.
+ Deletes password from dat file (stores only a hash value), so 
  that you cannot extract it while Miranda is not running.


Compared to PassProt
--------------------

The PassProt plugin was written nearly at the same time this plugin
was published (also I already started it some month ago).

The PassProt plugin has a smaller filesize and works quite well.

LSecure adds the feature that you cannot open Miranda with the 
wrong password as long as the plugin is presend.
So you cannot 
- disable the plugin from the options without the password.
- browse the history/contactlist without the password.
- connect to other protocols without the password.

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

1.1
Replaced VCL Dialog with Resource Dialog -> smaller filesize
Ability to translate the few texts

1.0
Initial Version


Translation
-----------
[OK]
[Cancel]
[Wrong password!]
[Enter password:]
[LSecure]