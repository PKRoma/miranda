// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006,2007 Joe Kucera
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $URL$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#ifndef __ICQOSC_SVCS_H
#define __ICQOSC_SVCS_H

/*---------* Functions *---------------*/

int IcqSetAwayMsg(WPARAM wParam, LPARAM lParam);
int IcqGetAwayMsg(WPARAM wParam, LPARAM lParam);
int IcqRecvAwayMsg(WPARAM wParam,LPARAM lParam);
int IcqAuthAllow(WPARAM wParam, LPARAM lParam);
int IcqAuthDeny(WPARAM wParam, LPARAM lParam);
int IcqBasicSearch(WPARAM wParam, LPARAM lParam);
int IcqSearchByEmail(WPARAM wParam, LPARAM lParam);
int IcqCreateAdvSearchUI(WPARAM wParam, LPARAM lParam);
int IcqSearchByAdvanced(WPARAM wParam, LPARAM lParam);
int IcqAddToList(WPARAM wParam, LPARAM lParam);
int IcqAddToListByEvent(WPARAM wParam, LPARAM lParam);
int IcqGetInfo(WPARAM wParam, LPARAM lParam);
int IcqSetApparentMode(WPARAM wParam, LPARAM lParam);
int IcqSendMessage(WPARAM wParam, LPARAM lParam);
int IcqSendUrl(WPARAM wParam, LPARAM lParam);
int IcqSendContacts(WPARAM wParam, LPARAM lParam);
int IcqSendFile(WPARAM wParam, LPARAM lParam);
int IcqFileAllow(WPARAM wParam, LPARAM lParam);
int IcqFileDeny(WPARAM wParam, LPARAM lParam);
int IcqFileCancel(WPARAM wParam, LPARAM lParam);
int IcqFileResume(WPARAM wParam, LPARAM lParam);
int IcqSendAuthRequest(WPARAM,LPARAM);
int IcqSendYouWereAdded(WPARAM,LPARAM);
int IcqSendUserIsTyping(WPARAM wParam, LPARAM lParam);


int IcqRecvMessage(WPARAM wParam, LPARAM lParam);
int IcqRecvContacts(WPARAM wParam, LPARAM lParam);
int IcqRecvFile(WPARAM wParam, LPARAM lParam);
int IcqRecvAuth(WPARAM wParam, LPARAM lParam);

int IcqIdleChanged(WPARAM wParam, LPARAM lParam);

#endif /* __ICQOSC_SVCS_H */
