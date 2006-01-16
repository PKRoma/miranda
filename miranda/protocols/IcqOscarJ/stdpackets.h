// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright � 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001,2002 Jon Keating, Richard Hughes
// Copyright � 2002,2003,2004 Martin  berg, Sam Kothari, Robert Rainwater
// Copyright � 2004,2005 Joe Kucera
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
//  Low-level functions that really sends data to server
//
// -----------------------------------------------------------------------------

#ifndef __STDPACKETS_H
#define __STDPACKETS_H

struct icq_contactsend_s
{
  DWORD uin;
  char *uid;
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
  HANDLE hContact;
} fam15_cookie_data;

#define REQUESTTYPE_OWNER        0
#define REQUESTTYPE_USERAUTO     1
#define REQUESTTYPE_USERMINIMAL  2
#define REQUESTTYPE_USERDETAILED 3
#define REQUESTTYPE_PROFILE      4


typedef struct search_cookie_s
{
  BYTE bSearchType;
  char* szObject;
  DWORD dwMainId;
  DWORD dwStatus;
} search_cookie;

#define SEARCHTYPE_UID     0
#define SEARCHTYPE_EMAIL   1
#define SEARCHTYPE_NAMES   2
#define SEARCHTYPE_DETAILS 4


void packMsgColorInfo(icq_packet *packet);

void icq_sendCloseConnection();

void icq_requestnewfamily(WORD wFamily, void (*familyhandler)(HANDLE hConn, char* cookie, WORD cookieLen));

void icq_setidle(int bAllow);
void icq_setstatus(WORD wStatus);
DWORD icq_sendGetInfoServ(DWORD, int);
DWORD icq_sendGetAimProfileServ(HANDLE hContact, char *szUid);
DWORD icq_sendGetAwayMsgServ(DWORD, int);
DWORD icq_sendGetAimAwayMsgServ(char *szUID, int type);
void icq_sendSetAimAwayMsgServ(char *szMsg);
void icq_sendFileSendServv7(DWORD dwUin, DWORD dwCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize);
void icq_sendFileSendServv8(DWORD dwUin, DWORD dwCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize, int nAckType);

void icq_sendFileAcceptServ(DWORD dwUin, filetransfer *ft, int nAckType);
void icq_sendFileAcceptServv7(DWORD dwUin, DWORD TS1, DWORD TS2, DWORD dwCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize, WORD wPort, BOOL accepted, int nAckType);
void icq_sendFileAcceptServv8(DWORD dwUin, DWORD TS1, DWORD TS2, DWORD dwCookie, const char *szFiles, const char *szDescr, DWORD dwTotalSize, WORD wPort, BOOL accepted, int nAckType);

void icq_sendFileDenyServ(DWORD dwUin, filetransfer* ft, char *szReason, int nAckType);

DWORD icq_sendAdvancedSearchServ(BYTE *fieldsBuffer,int bufferLen);
DWORD icq_changeUserDetailsServ(WORD, const unsigned char *, WORD);
void icq_sendNewContact(DWORD dwUin, char* szUid);
void icq_sendRemoveContact(DWORD dwUin, char* szUid);
void icq_sendChangeVisInvis(HANDLE hContact, DWORD dwUin, char* szUID, int list, int add);
void icq_sendEntireVisInvisList(int);
void icq_sendAwayMsgReplyServ(DWORD, DWORD, DWORD, WORD, BYTE, const char **);
void icq_sendAdvancedMsgAck(DWORD, DWORD, DWORD, WORD, BYTE, BYTE);
DWORD icq_sendSMSServ(const char *szPhoneNumber, const char *szMsg);
void icq_sendMessageCapsServ(DWORD dwUin);
void icq_sendGrantAuthServ(DWORD dwUin, char* szUid, char *szMsg);
void icq_sendAuthReqServ(DWORD dwUin,char *szMsg);
void icq_sendAuthResponseServ(DWORD dwUin, char* szUid,int auth,char *szReason);
void icq_sendYouWereAddedServ(DWORD,DWORD);

void sendOwnerInfoRequest(void);
void sendUserInfoAutoRequest(DWORD dwUin);

DWORD icq_SendChannel1Message(DWORD dwUin, char *szUID, HANDLE hContact, char *pszText, message_cookie_data *pCookieData);
DWORD icq_SendChannel1MessageW(DWORD dwUin, char *szUID, HANDLE hContact, wchar_t *pszText, message_cookie_data *pCookieData); // UTF-16
DWORD icq_SendChannel2Message(DWORD dwUin, const char *szMessage, int nBodyLength, WORD wPriority, message_cookie_data *pCookieData, char *szCap);
DWORD icq_SendChannel4Message(DWORD dwUin, BYTE bMsgType, WORD wMsgLen, const char *szMsg, message_cookie_data *pCookieData);

void icq_sendReverseReq(directconnect *dc, DWORD dwCookie);

void icq_sendXtrazRequestServ(DWORD dwUin, DWORD dwCookie, char* szBody, int nBodyLen, int nType);
void icq_sendXtrazResponseServ(DWORD dwUin, DWORD dwMID, DWORD dwMID2, WORD wCookie, char* szBody, int nBodyLen, int nType);

DWORD SearchByUin(DWORD dwUin);
DWORD SearchByNames(char *pszNick, char *pszFirstName, char *pszLastName);
DWORD SearchByEmail(char *pszEmail);

DWORD icq_searchAimByEmail(char* pszEmail, DWORD dwSearchId);

#endif /* __STDPACKETS_H */
