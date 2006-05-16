/*
AOL Instant Messenger Plugin for Miranda IM

Copyright © 2003-2005 Robert Rainwater

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
*/
#include "aim.h"

PLUGININFO pluginInfo = {
    sizeof(PLUGININFO),
    "AIM TOC2 Plugin",
    PLUGIN_MAKE_VERSION(1, 4, 2, 4),
#ifdef AIM_CVSBUILD
    "This is a development build of the AIM TOC2 protocol.  Please check the website for updates. (" __DATE__ ")",
#else
    "Provides support for AOL® TOC2 Instant Messenger protocol.",
#endif
	"Original Author: Robert Rainwater/Modification by Aaron Myles Landwehr",
    "rainwater at miranda-im.org",
    "Copyright © 2003-2005 Robert Rainwater",
    "http://www.miranda-im.org/download/details.php?action=viewfile&id=2286",
    0,
    0
};
