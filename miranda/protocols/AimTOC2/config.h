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

// Build is a cvs build
//
// If defined, the build will add cvs info to the plugin info
//#define AIM_CVSBUILD

// Default Values
//   
// The following values must be defined.
#define AIM_KEY_WM_DEF	0       // Warn Menu Item off by default
#define AIM_KEY_WD_DEF	1       // You have been warned on by default
#define AIM_KEY_IZ_DEF	1       // Idle time enabled by default
#define AIM_KEY_II_DEF	10      // Default idle time
#define AIM_KEY_AR_DEF	0       // Send autoresponse off by default
#define AIM_KEY_AC_DEF	1       // Only auto-response to clist users by default
#define AIM_KEY_SS_DEF  1       // Use server-side lists by default
#define AIM_KEY_SE_DEF  1       // Show error messages
#define AIM_KEY_GI_DEF  0       // Ignore invites off
#define AIM_KEY_GM_DEF  1       // Show groupchat menu item by default
#define AIM_KEY_SM_DEF  1       // Show Synclist menu item by default
#define AIM_KEY_AL_DEF  0       // aim: links support on
#define AIM_KEY_PM_DEF  1       // show password menu item
#define AIM_EVIL_TO		10      // Evil mode timeout in minutes (can't warn if last message greater)

// TOC Server Options
//
// AIM_TOC_HOST:     TOC server
// AIM_TOC_PORT:     Port to connect to (use 0 for random port)
// AIM_TOC_PORTLOW:  If using random port, this is the low value for the random number chooser
// AIM_TOC_PORTHIGH: If using random port, this is the high value for the random number chooser
#define AIM_TOC_HOST      "toc.oscar.aol.com"
#define AIM_TOC_PORT      5190
#define AIM_TOC_PORTLOW   1000
#define AIM_TOC_PORTHIGH  10000

// AIM Authentication Server Options
//
// See: TOC Server Options
#define AIM_AUTH_HOST     "login.oscar.aol.com"
#define AIM_AUTH_PORT     29999
#define AIM_AUTH_PORTLOW  1000
#define AIM_AUTH_PORTHIGH 10000

#define AIM_SERVERSIDE_GROUP	Translate("Miranda")    // Default group for miranda specific contacts stored on server
