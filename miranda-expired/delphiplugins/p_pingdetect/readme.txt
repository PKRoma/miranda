About
-----

Ping Detect
Version 1.0
by Christian Kästner (christian.k@stner.de)
for Miranda ICQ 0.1
written with Delphi 5 Pro


Description
-----------

This Plugin detects in certain intervals if an online connection
is available. Therefore it pings a specified host and sets Miranda
to online or offline depending on the ping results.

Features
--------

+ Detects if online connection is avaible
+ 3 Options: Host, Timeout and Ping interval


Options
-------

The options are integrated in Miranda options.

Host: 
  Host to ping. Specify a reliable and fast host. Better specify the
  IP, because in that case the plugin has not to do an extra DNS request.
  I recommend specifing your DNS server here...

Timeout: 
  DNS lookup and Ping timeout

Interval:
  How often the plugin should check the connection. It always pings the
  host 5 times. If it gets an answer at least 3 times it sets the 
  connection to online.


Installation
------------

Just copy the pingdetect.dll into Miranda's plugin subdirectory.


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
