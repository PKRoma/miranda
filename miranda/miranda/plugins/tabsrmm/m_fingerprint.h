/*
Based on Miranda plugin template, originally by Richard Hughes
http://miranda-icq.sourceforge.net/

Miranda IM: the free IM client for Microsoft Windows

Copyright 2000-2006 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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

/************************************************************************/
/*  Author: Artem Shpynov aka FYR     mailto:shpynov@nm.ru              */
/*  icons by Angeli-Ka                                                  */
/*  January 12, 2006													*/
/************************************************************************/


/*
 *   FINGERPRINT PLUGIN SERVICES HEADER
 */

/*
 *   Service SameClients MS_FP_SAMECLIENTS
 *	 wParam - char * first MirVer value 
 *   lParam - char * second MirVer value 
 *	 return pointer to char string - client desription (DO NOT DESTROY) if clients are same otherwise NULL
 */
#define MS_FP_SAMECLIENTS "Fingerprint/SameClients"

/*
 *   ServiceGetClientIcon MS_FP_GETCLIENTICON
 *	 wParam - char * MirVer value to get client for.
 *   lParam - int noCopy - if wParam is equal to "1"  will return icon handler without copiing icon.
 */
#define MS_FP_GETCLIENTICON "Fingerprint/GetClientIcon"
