// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004,2005 Martin Öberg, Sam Kothari, Robert Rainwater
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
// File name      : $Source$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#ifndef __STDPACKETS_H
#define __STDPACKETS_H

struct icq_contactsend_s {
	DWORD uin;
	char *szNick;
};

typedef struct message_cookie_data_s
{
	BYTE bMessageType;
	BYTE nAckType;
} message_cookie_data;

#define ACKTYPE_NONE   0
#define ACKTYPE_SERVER 1
#define ACKTYPE_CLIENT 2


typedef struct fam15_cookie_data_s
{
	BYTE bRequestType;
} fam15_cookie_data;

#define REQUESTTYPE_OWNER 0
#define REQUESTTYPE_USERAUTO  1
#define REQUESTTYPE_USERMINIMAL 2
#define REQUESTTYPE_USERDETAILED 3


void icq_setstatus(WORD wStatus);
WORD icq_sendGetInfoServ(DWORD, int);
WORD icq_sendGetAwayMsgServ(DWORD, int);
void icq_sendFileSendServv7(DWORD dwUin, WORD wCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize);
void icq_sendFileSendServv8(DWORD dwUin, WORD wCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize);

void icq_sendFileAcceptServ(DWORD dwUin, filetransfer *ft);
void icq_sendFileAcceptServv7(DWORD dwUin, DWORD TS1, DWORD TS2, WORD wCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize, WORD wPort, BOOL accepted);
void icq_sendFileAcceptServv8(DWORD dwUin, DWORD TS1, DWORD TS2, WORD wCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize, WORD wPort, BOOL accepted);

void icq_sendFileDenyServ(DWORD dwUin, filetransfer* ft, char *szReason);

WORD icq_sendAdvancedSearchServ(BYTE *fieldsBuffer,int bufferLen);
WORD icq_changeUserDetailsServ(WORD, const unsigned char *, WORD);
void icq_sendNewContact(DWORD);
void icq_sendChangeVisInvis(HANDLE hContact, DWORD dwUin, int list, int add);
void icq_sendEntireVisInvisList(int);
void icq_sendAwayMsgReplyServ(DWORD, DWORD, DWORD, WORD, BYTE, const char **);
void icq_sendAdvancedMsgAck(DWORD, DWORD, DWORD, WORD, BYTE, BYTE);
WORD icq_sendSMSServ(const char *szPhoneNumber, const char *szMsg);
void icq_sendMessageCapsServ(DWORD dwUin);
void icq_sendAuthReqServ(DWORD dwUin,char *szMsg);
void icq_sendAuthResponseServ(DWORD,int,char *);
void icq_sendYouWereAddedServ(DWORD,DWORD);

void sendOwnerInfoRequest(void);
void sendUserInfoAutoRequest(DWORD dwUin);

WORD icq_SendChannel1Message(DWORD dwUin, HANDLE hContact, char *pszText, message_cookie_data *pCookieData);
WORD icq_SendChannel2Message(DWORD dwUin, const char *szMessage, int nBodyLength, WORD wPriority, message_cookie_data *pCookieData);
WORD icq_SendChannel4Message(DWORD dwUin, BYTE bMsgType, WORD wMsgLen, const char *szMsg, message_cookie_data *pCookieData);

WORD SearchByUin(DWORD dwUin);
WORD SearchByNames(char *pszNick, char *pszFirstName, char *pszLastName);
WORD SearchByEmail(char *pszEmail);


#endif /* __STDPACKETS_H */